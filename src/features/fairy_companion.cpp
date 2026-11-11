// Ported from Fairy Companion v1.2.1 by AgustinFuac with permission.
// The companion, visuals, color options, and light are retained; fairy abilities are omitted.
#include "ma_fairy_companion.hpp"

#include <cmath>

// Dolphin / Hardware
#include <dolphin/gx.h>
// JSystem
#include "JSystem/J3DGraphAnimator/J3DModelData.h"
#include "JSystem/J3DGraphBase/J3DMaterial.h"

// Game Engine
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_player.h"
#include "d/d_attention.h"
#include "d/d_com_inf_game.h"
#include "d/d_kankyo.h"
#include "f_op/f_op_actor_mng.h"
#include "m_Do/m_Do_ext.h"
#include "m_Do/m_Do_mtx.h"
#include "m_Do/m_Do_audio.h"

#include "Z2AudioLib/Z2SeMgr.h"

DEFINE_HOOK_SYMBOL(
    "dMeter2Draw_c::getActionString", char*(dMeter2Draw_c*, u8, u8, u8*),
    FairyActionLabelHook);
DEFINE_HOOK(&daAlink_c::decideDoStatus, FairyDoStatus);
DEFINE_HOOK(&daAlink_c::decideCommonDoStatus, FairyCommonDoStatus);
DEFINE_HOOK(&daAlink_c::setStickData, FairySetStickData);

// ---------------------------------------------------------------------------
// Constants & Configuration
// ---------------------------------------------------------------------------

// Solid heap size for model + animation (matches daObjYOUSEI_c requirements)
static constexpr u32 k_heapSize = 0x1100;

// Default floor color for dKy_tevstr_init
static constexpr u8 k_defaultFloorCol = 0xFF;
static constexpr int64_t kFairyToggleButtonA = 0;
static constexpr int64_t kFairyToggleButtonLeftBumper = 1;
static constexpr int64_t kFairyToggleButtonR = 2;
static constexpr int64_t kFairyToggleButtonR3 = 3;
static constexpr int64_t kFairyToggleButtonL3 = 4;

// Particle update intervals
static constexpr u8 k_particleInterval = 8;
static constexpr u8 k_auraInterval = 1;

// Material register index for fairy tint
static constexpr u32 k_fairyTintReg = 1;

// Math constants
static constexpr f32 k_pi = 3.14159265358979323846f;
static constexpr f32 k_twoPi = 2.0f * k_pi;
static constexpr f32 k_angleToRad = k_pi / 32768.0f; // Converts s16 engine angle to radians

// ---------------------------------------------------------------------------
// Fairy Color Palette
// ---------------------------------------------------------------------------

struct FairyTint_s {
    u8 bodyR, bodyG, bodyB; // Material TEV registers and ambient light
    u8 prmR, prmG, prmB;  // Particle trail & sparkle primary color
    u8 envR, envG, envB;  // Particle trail & sparkle environment color
    u8 auraR, auraG, auraB; // Body aura glow color (0x730)
    u8 auraAlpha;           // Default aura opacity
};

static const FairyTint_s k_fairyTints[] = {
    // FAIRY_COLOR_BLUE (Navi)
    {   0, 117, 235,   80, 180, 255,   20,  60, 200,   70, 170, 255, 140 },
    // FAIRY_COLOR_ORANGE (Tatl)
    { 200, 150,  20,  255, 190,  70,  200,  90,  10,  255, 175,  60, 140 },
    // FAIRY_COLOR_PURPLE (Tael)
    { 204,   0, 240,  102,   0,  80,  100,  20, 200,  175,  90, 255, 140 },
    // FAIRY_COLOR_GREEN (Extra)
    {  50, 255,  80,  110, 255, 140,   20, 180,  50,   90, 255, 120, 140 },
};

static constexpr u8 k_fairyTintCount = static_cast<u8>(sizeof(k_fairyTints) / sizeof(k_fairyTints[0]));

static inline const FairyTint_s& getFairyTint(u8 colorId) {
    if (colorId >= k_fairyTintCount) {
        colorId = FAIRY_COLOR_BLUE;
    }
    return k_fairyTints[colorId];
}

static inline void wrapAngle(f32& angle) {
    if (angle > k_twoPi) {
        angle -= k_twoPi;
    }
}

// ---------------------------------------------------------------------------
// The fairy blends into Link while put away.
// ---------------------------------------------------------------------------

static constexpr f32 k_tuckHeight = 130.0f; // Height above Link's feet where she tucks in
static constexpr f32 k_hideRate = 0.035f; // Blend speed when going into Link
static constexpr f32 k_emergeRate = 0.12f;

// State
static f32  s_hideBlend = 1.0f;  // 0 = out flying, 1 = fully tucked into Link
static f32  s_baseScale = 0.0f;  // Normal model scale, captured on first Execute

static void resetHideState() {
    s_hideBlend = maFairyCompanion_c::isCalled() ? 0.0f : 1.0f;
    s_baseScale = 0.0f;
}

static f32 updateHideBlend() {
    const f32 goal = maFairyCompanion_c::isCalled() ? 0.0f : 1.0f;
    const f32 rate = goal > s_hideBlend ? k_hideRate : k_emergeRate;

    if (s_hideBlend < goal) {
        s_hideBlend = (s_hideBlend + rate > goal) ? goal : (s_hideBlend + rate);
    }
    else if (s_hideBlend > goal) {
        s_hideBlend = (s_hideBlend - rate < goal) ? goal : (s_hideBlend - rate);
    }

    const f32 s = s_baseScale * (1.0f - s_hideBlend);
    return (s < 0.001f) ? 0.001f : s;
}

// ---------------------------------------------------------------------------
// Static Member Initialization
// ---------------------------------------------------------------------------

u8          maFairyCompanion_c::sSelectedColor = FAIRY_COLOR_BLUE;
bool        maFairyCompanion_c::sCalled = false;
bool        maFairyCompanion_c::sLightEnabled = true;
u32         maFairyCompanion_c::sExecTick = 0;
s16         maFairyCompanion_c::sProcName = -1;
ActorHandle maFairyCompanion_c::sActorHandle = -1;

void maFairySfx_play() {
    daAlink_c* player = daAlink_getAlinkActorClass();
    if (player == nullptr) return;
    mDoAud_seStart(
        Z2SE_FAIRY_HANE_OP, &player->current.pos, 0,
        dComIfGp_getReverb(fopAcM_GetRoomNo(player)));
}

void maFairyCompanion_c::setSelectedColor(u8 colorId) {
    sSelectedColor = (colorId < k_fairyTintCount) ? colorId : FAIRY_COLOR_BLUE;
}

namespace {

bool s_fairyActorRegistered = false;
bool s_fairyActorActive = false;
ActorId s_fairyActorId = 0;
u16 s_fairySpawnRetryFrames = 0;
bool s_fairyActionPromptActive = false;
bool s_fairyToggleButtonWasHeld[5] = {};
bool s_fairyToggleUiQueryFailed = false;
char s_fairyCallLabel[] = "Call";
char s_fairyPutAwayLabel[] = "Put Away";

int64_t fairy_toggle_binding() {
    return get_int_option(g_fairyCompanionToggleBinding, kFairyToggleButtonA,
        kFairyToggleButtonA, kFairyToggleButtonL3);
}

bool can_use_fairy_action(daAlink_c* player) {
    return player != nullptr && !player->checkWolf() && !player->checkEventRun() &&
        player->mProcID == daAlink_c::PROC_WAIT &&
        player->mLinkAcch.ChkGroundHit() && player->checkZeroSpeedF() &&
        !player->checkSwordDraw() && !player->checkShieldDraw() &&
        !player->checkItemDraw();
}

void update_fairy_action_prompt(daAlink_c* player) {
    if (fairy_toggle_binding() != kFairyToggleButtonA || !s_fairyActorActive ||
        !can_use_fairy_action(player))
    {
        s_fairyActionPromptActive = false;
        return;
    }
    const bool called = maFairyCompanion_c::isCalled();
    const u8 fairyStatus = called
        ? BUTTON_STATUS_PUT_AWAY
        : BUTTON_STATUS_SPEAK;
    const u8 currentStatus = dComIfGp_getDoStatus();
    if (currentStatus != BUTTON_STATUS_NONE && currentStatus != fairyStatus) {
        s_fairyActionPromptActive = false;
        return;
    }

    dComIfGp_setDoStatus(fairyStatus, BUTTON_STATUS_FLAG_NONE);
    s_fairyActionPromptActive = true;
}

void choose_fairy_action_prompt(ModContext*, void* args, void*, void*) {
    update_fairy_action_prompt(mods::arg<daAlink_c*>(args, 0));
}

bool toggle_fairy_for_binding(daAlink_c* player, int64_t binding) {
    if (!s_fairyActorActive || player == nullptr || player->checkWolf() ||
        player->checkEventRun() || dComIfGp_isPauseFlag() ||
        (binding == kFairyToggleButtonA && !can_use_fairy_action(player)))
    {
        return false;
    }

    bool documentVisible = false;
    if (svc_ui->is_any_document_visible(mod_ctx, &documentVisible) != MOD_OK) {
        if (!s_fairyToggleUiQueryFailed) {
            svc_log->warn(mod_ctx, "could not check Dusklight UI visibility for Fairy toggle");
            s_fairyToggleUiQueryFailed = true;
        }
        return false;
    }
    s_fairyToggleUiQueryFailed = false;
    if (documentVisible) return false;

    maFairyCompanion_c::setCalled(!maFairyCompanion_c::isCalled());
    maFairySfx_play();
    return true;
}

void intercept_fairy_a_toggle(ModContext*, void* args, void*, void*) {
    if (fairy_toggle_binding() != kFairyToggleButtonA) return;
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (player == nullptr || !s_fairyActorActive ||
        (player->mItemTrigger & daAlink_c::BTN_A) == 0)
    {
        return;
    }
    if (!toggle_fairy_for_binding(player, kFairyToggleButtonA)) return;

    player->mItemTrigger &= static_cast<u8>(~daAlink_c::BTN_A);
    player->mItemButton &= static_cast<u8>(~daAlink_c::BTN_A);
}

void update_fairy_toggle_input(daAlink_c* player) {
    const u32 extendedButtons = JUTGamePad::mPadStatus[PAD_1].extButton;
    const bool held[5] = {
        mDoCPd_c::getHoldA(PAD_1) != 0,
        native_left_bumper_held(),
        mDoCPd_c::getHoldLockR(PAD_1) != 0,
        (extendedButtons & PAD_BUTTON_RIGHT_STICK) != 0,
        (extendedButtons & PAD_BUTTON_LEFT_STICK) != 0,
    };
    bool pressed[5] = {};
    for (size_t i = 0; i < 5; ++i) {
        pressed[i] = held[i] && !s_fairyToggleButtonWasHeld[i];
        s_fairyToggleButtonWasHeld[i] = held[i];
    }

    const int64_t binding = fairy_toggle_binding();
    if (binding == kFairyToggleButtonA) return;
    const bool triggered = pressed[binding];
    if (triggered) toggle_fairy_for_binding(player, binding);
}

void replace_fairy_action_label(ModContext*, void* args, void* result, void*) {
    if (!s_fairyActionPromptActive || result == nullptr) return;
    const u8 status = mods::arg<u8>(args, 1);
    if (status == BUTTON_STATUS_SPEAK) {
        *static_cast<char**>(result) = s_fairyCallLabel;
    } else if (status == BUTTON_STATUS_PUT_AWAY) {
        *static_cast<char**>(result) = s_fairyPutAwayLabel;
    }
}

void on_fairy_color_changed(
    ModContext*, ConfigVarHandle, const ConfigVarValue* value,
    const ConfigVarValue*, void*)
{
    maFairyCompanion_c::setSelectedColor(static_cast<u8>(value->int_value));
}

void on_fairy_light_changed(
    ModContext*, ConfigVarHandle, const ConfigVarValue* value,
    const ConfigVarValue*, void*)
{
    maFairyCompanion_c::setLightEnabled(value->bool_value);
}

}

ModResult register_fairy_companion_settings() {
    ConfigVarDesc color = CONFIG_VAR_DESC_INIT;
    color.name = "fairyCompanionColor";
    color.type = CONFIG_VAR_INT;
    color.default_int = FAIRY_COLOR_BLUE;
    ModResult result =
        svc_config->register_var(mod_ctx, &color, &g_fairyCompanionColor);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Fairy Companion color setting");
        return result;
    }
    result = svc_config->subscribe(
        mod_ctx, g_fairyCompanionColor, on_fairy_color_changed, nullptr, nullptr);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to subscribe to Fairy Companion color setting");
        return result;
    }

    ConfigVarDesc light = CONFIG_VAR_DESC_INIT;
    light.name = "fairyCompanionLight";
    light.type = CONFIG_VAR_BOOL;
    light.default_bool = true;
    result = svc_config->register_var(mod_ctx, &light, &g_fairyCompanionLight);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Fairy Companion light setting");
        return result;
    }
    result = svc_config->subscribe(
        mod_ctx, g_fairyCompanionLight, on_fairy_light_changed, nullptr, nullptr);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to subscribe to Fairy Companion light setting");
        return result;
    }

    ConfigVarDesc toggleBinding = CONFIG_VAR_DESC_INIT;
    toggleBinding.name = "fairyCompanionToggleBinding";
    toggleBinding.type = CONFIG_VAR_INT;
    toggleBinding.default_int = kFairyToggleButtonA;
    result = svc_config->register_var(
        mod_ctx, &toggleBinding, &g_fairyCompanionToggleBinding);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Fairy Companion toggle binding");
        return result;
    }

    int64_t selectedColor = FAIRY_COLOR_BLUE;
    result = svc_config->get_int(mod_ctx, g_fairyCompanionColor, &selectedColor);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to read Fairy Companion color setting");
        return result;
    }
    maFairyCompanion_c::setSelectedColor(
        selectedColor >= FAIRY_COLOR_BLUE && selectedColor <= FAIRY_COLOR_GREEN
            ? static_cast<u8>(selectedColor)
            : FAIRY_COLOR_BLUE);

    bool lightEnabled = true;
    result = svc_config->get_bool(mod_ctx, g_fairyCompanionLight, &lightEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to read Fairy Companion light setting");
        return result;
    }
    maFairyCompanion_c::setLightEnabled(lightEnabled);
    return MOD_OK;
}

void initialize_fairy_companion_actor() {
    if (s_manualJumpActorService == nullptr) {
        svc_log->warn(mod_ctx, "Fairy Companion unavailable: actor service is missing");
        return;
    }
    const ModResult result = s_manualJumpActorService->register_actor(
        mod_ctx, &maFairyCompanion_c::sProfile, &maFairyCompanion_c::sProcName,
        &maFairyCompanion_c::sActorHandle);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Fairy Companion actor");
        return;
    }
    s_fairyActorRegistered = true;
}

void update_fairy_companion() {
    if (!s_fairyActorRegistered || s_manualJumpActorService == nullptr) return;
    if (s_fairySpawnRetryFrames > 0) {
        --s_fairySpawnRetryFrames;
        return;
    }

    daAlink_c* player = daAlink_getAlinkActorClass();
    if (player == nullptr) return;
    const int8_t room = fopAcM_GetRoomNo(player);
    int8_t actorRoom = 0;
    const bool alive = s_fairyActorActive &&
        s_manualJumpActorService->get_actor_room_num(
            mod_ctx, s_fairyActorId, &actorRoom) == MOD_OK &&
        actorRoom == room;
    if (alive) {
        return;
    }
    s_fairyActionPromptActive = false;

    if (s_fairyActorActive) {
        const ModResult deleteResult =
            s_manualJumpActorService->delete_actor(mod_ctx, s_fairyActorId);
        if (deleteResult != MOD_OK) {
            s_fairySpawnRetryFrames = 120;
            svc_log->error(mod_ctx, "failed to remove the previous Fairy Companion actor");
            return;
        }
        s_fairyActorActive = false;
    }
    ActorSpawnParams params{};
    params.parameters = 0;
    params.argument = 0;
    params.room_num = room;
    params.position = {player->current.pos.x, player->current.pos.y + 100.0f,
        player->current.pos.z};
    params.angle = {0, 0, 0};
    params.scale = {1.0f, 1.0f, 1.0f};
    const ModResult result = s_manualJumpActorService->create_actor(
        mod_ctx, maFairyCompanion_c::sProcName, &params, &s_fairyActorId);
    if (result == MOD_OK) {
        s_fairyActorActive = true;
        svc_log->info(mod_ctx, "Fairy Companion spawned");
    } else {
        s_fairySpawnRetryFrames = 120;
        svc_log->error(mod_ctx, "failed to spawn Fairy Companion; retrying in four seconds");
    }
}

void shutdown_fairy_companion() {
    for (bool& wasHeld : s_fairyToggleButtonWasHeld) wasHeld = false;
    if (s_manualJumpActorService != nullptr) {
        if (s_fairyActorActive) {
            const ModResult result =
                s_manualJumpActorService->delete_actor(mod_ctx, s_fairyActorId);
            if (result != MOD_OK) {
                svc_log->error(mod_ctx, "failed to delete Fairy Companion during shutdown");
            }
            s_fairyActorActive = false;
        }
        if (s_fairyActorRegistered) {
            const ModResult result = s_manualJumpActorService->unregister_actor(
                mod_ctx, maFairyCompanion_c::sActorHandle);
            if (result != MOD_OK) {
                svc_log->error(mod_ctx, "failed to unregister Fairy Companion actor");
            }
            s_fairyActorRegistered = false;
        }
    }
    s_fairySpawnRetryFrames = 0;
}

// ---------------------------------------------------------------------------
// Lifecycle & Resource Management
// ---------------------------------------------------------------------------

maFairyCompanion_c::~maFairyCompanion_c() {
    // "Always" archive is resident throughout the game; no resource unloading needed.
}

cPhs_Step maFairyCompanion_c::create() {
    fopAcM_ct(this, maFairyCompanion_c);

    if (!fopAcM_entrySolidHeap(this, createHeapCallBack, k_heapSize)) {
        return cPhs_ERROR_e;
    }

    dKy_tevstr_init(&tevStr, fopAcM_GetRoomNo(this), k_defaultFloorCol);

    fopAcM_SetMtx(this, mpModelMorf->getModel()->getBaseTRMtx());
    fopAcM_SetMin(this, -50.0f, -50.0f, -50.0f);
    fopAcM_SetMax(this, 50.0f, 50.0f, 50.0f);

    mSound.init(&current.pos, &eyePos, 3, 1);

    mOrbitAngle = 0.0f;
    mSwayPhase = 0.0f;
    mBobPhase = 1.5f; // Offset phase so bob does not sync with sway
    mColorId = sSelectedColor;
    mParticleId = 0;
    mParticleTimer = 0;
    mSparkleId = 0;
    mSparkleTimer = 0;
    mAuraId = 0;
    mAuraTimer = 0;
    Execute();
    return cPhs_COMPLEATE_e;
}

int maFairyCompanion_c::CreateHeap() {
    // Load fairy model (0x21) and flight animation (0xF) from resident "Always" archive
    void* modelData = dComIfG_getObjectRes("Always", 0x21);
    if (modelData == NULL) {
        return 0;
    }

    mpModelMorf = JKR_NEW mDoExt_McaMorfSO(
        static_cast<J3DModelData*>(modelData), NULL, NULL,
        static_cast<J3DAnmTransform*>(dComIfG_getObjectRes("Always", 0xF)),
        2, 0.4f, 0, -1, &mSound, 0x80000, 0x11000084);

    return mpModelMorf != NULL;
}

int maFairyCompanion_c::createHeapCallBack(fopAc_ac_c* i_this) {
    return static_cast<maFairyCompanion_c*>(i_this)->CreateHeap();
}

int maFairyCompanion_c::Delete() {
    resetHideState();

    if (heap != NULL && mpModelMorf != NULL) {
        mpModelMorf->stopZelAnime();
    }
    this->~maFairyCompanion_c();
    return 1;
}

// ---------------------------------------------------------------------------
// Execution & Movement
// ---------------------------------------------------------------------------

int maFairyCompanion_c::Execute() {
    sExecTick++;
    mColorId = sSelectedColor;

    // Remember the normal model scale (before hide shrinking touches it)
    if (s_baseScale <= 0.0f && scale.x > 0.01f) {
        s_baseScale = scale.x;
    }

    daPy_py_c* player = daPy_getPlayerActorClass();
    daAlink_c* alink = daAlink_getAlinkActorClass();
    update_fairy_toggle_input(alink);

    // Lock-on state
    dAttention_c* attention = dComIfGp_getAttention();
    fopAc_ac_c* lockTarget = attention != NULL && attention->LockonTruth()
        ? attention->LockonTarget(0) : NULL;
    const bool hasLockTarget = lockTarget != NULL;

    const f32 hideScale = updateHideBlend();
    scale.x = hideScale;
    scale.y = hideScale;
    scale.z = hideScale;

    // --- Movement ------------------------------------------------------
    if (hasLockTarget) {
        // Orbit around the locked target
        mOrbitAngle += 0.06f;
        wrapAngle(mOrbitAngle);

        constexpr f32 orbitRadius = 50.0f;
        constexpr f32 orbitHeight = 100.0f;

        const cXyz& centerPos = lockTarget->current.pos;
        cXyz hoverPos;
        hoverPos.x = centerPos.x + orbitRadius * std::sin(mOrbitAngle);
        hoverPos.y = centerPos.y + orbitHeight;
        hoverPos.z = centerPos.z + orbitRadius * std::cos(mOrbitAngle);

        constexpr f32 orbitFollowSpeed = 0.15f;
        current.pos.x += (hoverPos.x - current.pos.x) * orbitFollowSpeed;
        current.pos.y += (hoverPos.y - current.pos.y) * orbitFollowSpeed;
        current.pos.z += (hoverPos.z - current.pos.z) * orbitFollowSpeed;

        cXyz diff = centerPos - current.pos;
        shape_angle.y = cM_atan2s(diff.x, diff.z);
    }
    else if (player != NULL) {
        // Follow Link from behind with lateral sway and vertical bobbing
        const f32 facingRad = player->current.angle.y * k_angleToRad;
        const f32 behindRad = facingRad + k_pi;

        const f32 sinBehind = std::sin(behindRad);
        const f32 cosBehind = std::cos(behindRad);

        constexpr f32 followDist = 25.0f;
        f32 targetX = player->current.pos.x + followDist * sinBehind;
        f32 targetZ = player->current.pos.z + followDist * cosBehind;

        // Sway lateral: perpendicular using trig identity
        // sin(theta + pi/2) = cos(theta), cos(theta + pi/2) = -sin(theta)
        mSwayPhase += 0.025f;
        wrapAngle(mSwayPhase);
        const f32 sway = 70.0f * std::sin(mSwayPhase);
        targetX += sway * cosBehind;
        targetZ -= sway * sinBehind;

        // Bob vertical
        mBobPhase += 0.035f;
        wrapAngle(mBobPhase);
        const f32 bob = 27.0f * std::sin(mBobPhase);

        // Lower height offset when riding horseback / boar
        const f32 heightOffset = (alink != NULL && alink->checkReinRide()) ? 80.0f : 160.0f;

        // Blend the normal flight target with the tuck-in point on Link
        const f32 flyY = player->current.pos.y + heightOffset + bob;
        const f32 tuckY = player->current.pos.y + k_tuckHeight;
        const f32 goalX = targetX + (player->current.pos.x - targetX) * s_hideBlend;
        const f32 goalZ = targetZ + (player->current.pos.z - targetZ) * s_hideBlend;
        const f32 goalY = flyY + (tuckY - flyY) * s_hideBlend;

        const f32 followSpeed = 0.12f + 0.08f * s_hideBlend; // Flies a bit faster when tucking in
        current.pos.x += (goalX - current.pos.x) * followSpeed;
        current.pos.y += (goalY - current.pos.y) * followSpeed;
        current.pos.z += (goalZ - current.pos.z) * followSpeed;

        shape_angle.y = fopAcM_searchPlayerAngleY(this);
    }

    // --- Lighting & particles -----------------------------------------
    // Refresh ambient lighting
    tevStr.room_no = fopAcM_GetRoomNo(this);
    dKy_setLight_nowroom_actor(&tevStr);

    // Keep tint available for the rest of Execute()
    const FairyTint_s& tint = getFairyTint(mColorId);

    // Automatic dynamic fairy light
    if (sLightEnabled && s_hideBlend < 0.5f) {
        GXColor lightCol = { tint.bodyR, tint.bodyG, tint.bodyB, 0xFF };

        if (dKy_Indoor_check()) {
            // Indoors: full intensity
            dKy_WolfEyeLight_set(
                &current.pos, 0.0f, 0.0f, 50.0f,
                &lightCol, 1.0f, 0, 3
            );
        }
        else if (dKy_daynight_check()) {
            // Outdoors at night: reduced intensity
            dKy_WolfEyeLight_set(
                &current.pos, 0.0f, 0.0f, 50.0f,
                &lightCol, 0.5f, 0, 3
            );
        }
    }

    // Trail & sparkles (they stop once she is mostly hidden)
    const GXColor prmColor = { tint.prmR, tint.prmG, tint.prmB, 0xFF };
    const GXColor envColor = { tint.envR, tint.envG, tint.envB, 0xFF };

    if (s_hideBlend < 0.5f && ++mParticleTimer >= k_particleInterval) {
        mParticleTimer = 0;
        mParticleId = dComIfGp_particle_set(
            mParticleId, 0x72F, &current.pos, &tevStr, &shape_angle,
            NULL, 0xFF, NULL, -1, &prmColor, &envColor, NULL);
    }

    if (s_hideBlend < 0.5f && ++mSparkleTimer >= k_particleInterval) {
        mSparkleTimer = 0;
        mSparkleId = dComIfGp_particle_set(
            mSparkleId, 0x731, &current.pos, &tevStr, &shape_angle,
            NULL, 0xFF, NULL, -1, &prmColor, &envColor, NULL);
    }

    // Body aura glow (0x730) with a slow breathing pulse
    if (s_hideBlend < 0.5f && ++mAuraTimer >= k_auraInterval) {
        mAuraTimer = 0;
        const GXColor auraCol = { tint.auraR, tint.auraG, tint.auraB, 0xFF };

        // The pulse reuses mBobPhase, which keeps advancing while she follows Link.
        // (mOrbitAngle only advances during lock-on, exactly when the pulse is not used.)
        const f32 pulse = 0.75f + 0.25f * std::sin(mBobPhase * 2.0f);
        const u8  pulsedAlpha = static_cast<u8>(tint.auraAlpha * pulse);

        mAuraId = dComIfGp_particle_set(
            mAuraId, 0x730, &current.pos, &tevStr, &shape_angle,
            NULL, pulsedAlpha, NULL, -1, &auraCol, &auraCol, NULL);
    }

    // --- Animation & matrices -----------------------------------------
    mpModelMorf->play(0, dComIfGp_getReverb(fopAcM_GetRoomNo(this)));

    eyePos = attention_info.position = current.pos;
    attention_info.flags = 0;

    setBaseMtx();
    return 1;
}

void maFairyCompanion_c::setBaseMtx() {
    mDoMtx_stack_c::transS(current.pos);
    mDoMtx_stack_c::ZXYrotM(shape_angle);
    mDoMtx_stack_c::scaleM(scale);
    mpModelMorf->getModel()->setBaseTRMtx(mDoMtx_stack_c::get());
    mpModelMorf->modelCalc();
}

// ---------------------------------------------------------------------------
// Rendering & Material Tinting
// ---------------------------------------------------------------------------

void maFairyCompanion_c::applyFairyTint(J3DModel* i_model) {
    const FairyTint_s& tint = getFairyTint(mColorId);
    J3DModelData* modelData = i_model->getModelData();
    const u16 matNum = modelData->getMaterialNum();

    for (u16 i = 0; i < matNum; i++) {
        J3DMaterial* mat = modelData->getMaterialNodePointer(i);
        if (mat == NULL) {
            continue;
        }

        J3DGXColorS10* c = mat->getTevColor(k_fairyTintReg);
        if (c != NULL) {
            c->r = static_cast<s16>(tint.bodyR);
            c->g = static_cast<s16>(tint.bodyG);
            c->b = static_cast<s16>(tint.bodyB);
        }
    }
}

int maFairyCompanion_c::Draw() {
    // Fully tucked into Link: nothing to draw
    if (s_hideBlend >= 0.99f) {
        return 1;
    }

    J3DModel* model = mpModelMorf->getModel();

    // 1) Environment system sets up lighting structure
    g_env_light.settingTevStruct(0, &current.pos, &tevStr);

    // 2) Write environment lighting to material registers
    g_env_light.setLightTevColorType_MAJI(model, &tevStr);

    // 3) Apply our custom fairy hue over register 1
    applyFairyTint(model);

    mpModelMorf->entryDL();
    return 1;
}

// ---------------------------------------------------------------------------
// Actor Profile Callbacks
// ---------------------------------------------------------------------------

static cPhs_Step maFairyCompanion_Create(void* i_this) {
    return static_cast<maFairyCompanion_c*>(i_this)->create();
}

static int maFairyCompanion_Delete(void* i_this) {
    return static_cast<maFairyCompanion_c*>(i_this)->Delete();
}

static int maFairyCompanion_Execute(void* i_this) {
    return static_cast<maFairyCompanion_c*>(i_this)->Execute();
}

static int maFairyCompanion_Draw(void* i_this) {
    return static_cast<maFairyCompanion_c*>(i_this)->Draw();
}

static int maFairyCompanion_IsDelete(void*) {
    return 1;
}

const ActorProfileDesc maFairyCompanion_c::sProfile = {
    .name = MFAIRYC_NAME,
    .priority_group = 7,
    .process_size = sizeof(maFairyCompanion_c),
    .draw_priority = fpcDwPi_OBJ_LBOX_e,
    .status = fopAcStts_UNK_0x40000_e | fopAcStts_UNK_0x4000_e | fopAcStts_CULL_e,
    .group = fopAc_ACTOR_e,
    .cull_type = fopAc_CULLBOX_CUSTOM_e,
    .create_function = maFairyCompanion_Create,
    .delete_function = maFairyCompanion_Delete,
    .execute_function = maFairyCompanion_Execute,
    .is_delete_function = maFairyCompanion_IsDelete,
    .draw_function = maFairyCompanion_Draw,
};

// ---------------------------------------------------------------------------
