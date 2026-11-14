#include "f_op/f_op_actor.h"
#include "f_op/f_op_actor_mng.h"
#include "f_pc/f_pc_method.h"
#include "f_pc/f_pc_name.h"
#include "m_Do/m_Do_ext.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>

namespace bow_aim {
namespace {

DEFINE_HOOK(&daAlink_c::procBowSubject, BowSubject);
DEFINE_HOOK(&daAlink_c::procBoomerangSubject, BoomerangSubject);
DEFINE_HOOK(&daAlink_c::procHookshotSubject, ClawshotSubject);
DEFINE_HOOK(&daAlink_c::procIronBallSubject, IronBallSubject);
DEFINE_HOOK(&daAlink_c::setStickData, BowSetStickData);
DEFINE_HOOK(&dCamera_c::nextMode, BowCameraNextMode);
DEFINE_HOOK(&dCamera_c::nextType, BowCameraNextType);
DEFINE_HOOK(&dCamera_c::subjectCamera, BowSubjectCamera);
DEFINE_HOOK(&dCamera_c::Run, BowCameraRun);
DEFINE_HOOK(&daAlink_c::setMatrix, AimSetMatrix);
DEFINE_HOOK(&daAlink_c::modelCalc, AimModelCalc);
DEFINE_HOOK(&daAlink_c::modelDraw, BowModelDraw);
DEFINE_HOOK(&daArrow_c::arrowShooting, BowArrowShooting);
DEFINE_HOOK(&daAlink_c::throwBoomerang, BoomerangThrow);
DEFINE_HOOK(&daArrow_c::procMove, BulletTimeArrowMove);
DEFINE_HOOK_SYMBOL("fopAc_Execute", int(void*), BulletTimeActorExecute);
DEFINE_HOOK(&fpcMtd_Execute, BulletTimeProcessExecute);
DEFINE_HOOK(&fopAcM_calcSpeed, BulletTimeCalcSpeed);
DEFINE_HOOK(&fopAcM_posMove, BulletTimePosMove);
DEFINE_HOOK(&daAlink_c::posMove, BulletTimeLinkPosMove);

constexpr f32 kSimulationPeriod = 1.0f / 30.0f;
constexpr f32 kEnemyTimeScale = 0.1f;
constexpr f32 kLinkTimeScale = 0.1f;
constexpr f32 kFlurryLinkTimeScale = 0.25f;
constexpr f32 kFreeBowCameraDegreesPerFrame = 4.0f;
constexpr f32 kFreeBowCameraPitchLimit = 89.0f;
constexpr auto kManualJumpTimeout = std::chrono::seconds(7);
constexpr f32 kPi = 3.14159265358979323846f;
constexpr std::size_t kActorClockCapacity = 256;
constexpr std::size_t kActorExecuteStackCapacity = 8;
constexpr std::size_t kMovementHookStackCapacity = 16;
constexpr std::size_t kAnimationHookStackCapacity = 16;

struct BulletTimeState {
    daAlink_c* owner = nullptr;
    std::chrono::steady_clock::time_point started{};
    f32 verticalSpeed = 0.0f;
    bool active = false;
    bool usedForJump = false;
};

struct SlowActorClock {
    fopAc_ac_c* actor = nullptr;
    fpc_ProcID actorId = fpcM_ERROR_PROCESS_ID_e;
    f32 accumulator = 0.0f;
    std::uint64_t lastSeenFrame = 0;
};

struct LinkPositionStep {
    daAlink_c* link = nullptr;
    cXyz startPosition{};
    cXyz lastGenericPosition{};
    f32 gravity = 0.0f;
    bool genericStepApplied = false;
    bool gravityChanged = false;
    bool active = false;
};

struct EnemyGravityStep {
    fopAc_ac_c* actor = nullptr;
    f32 gravity = 0.0f;
    bool active = false;
};

struct EnemyPositionStep {
    fopAc_ac_c* actor = nullptr;
    cXyz speed{};
    cXyz startPosition{};
    f32 positionScale = 0.0f;
    bool linkPosition = false;
    bool active = false;
};

struct ArrowMoveStep {
    daArrow_c* arrow = nullptr;
    cXyz speed{};
    f32 gravity = 0.0f;
    f32 timeScale = 1.0f;
    int (daArrow_c::*procFunc)() = nullptr;
    bool active = false;
};

struct EnemyAnimationStep {
    mDoExt_morf_c* morf = nullptr;
    f32 playSpeed = 0.0f;
    bool active = false;
};

struct AimMovementInputState {
    daAlink_c* link = nullptr;
    f32 moveValue = 0.0f;
    bool active = false;
};

BulletTimeState g_bulletTime{};
bool g_flurryRushSlowMotionActive = false;
bool g_flurryRushLinkSlowed = false;
daAlink_c* g_flurryRushOwner = nullptr;
daAlink_c* g_manualJumpOwner = nullptr;
std::chrono::steady_clock::time_point g_manualJumpStarted{};
bool g_bowBulletTimePending = false;
bool g_manualJumpBowAimRequested = false;
bool g_manualJumpBowAimActive = false;
bool g_manualJumpBowAimHawkeyeRequested = false;
u8 g_manualJumpBowAimHawkeyeButton = dItemNo_NONE_e;
std::array<SlowActorClock, kActorClockCapacity> g_actorClocks{};
std::array<fopAc_ac_c*, kActorExecuteStackCapacity> g_actorExecuteStack{};
std::array<EnemyGravityStep, kMovementHookStackCapacity> g_enemyGravitySteps{};
std::array<EnemyPositionStep, kMovementHookStackCapacity> g_enemyPositionSteps{};
std::array<ArrowMoveStep, kMovementHookStackCapacity> g_arrowMoveSteps{};
std::array<EnemyAnimationStep, kAnimationHookStackCapacity> g_enemyAnimationSteps{};
std::size_t g_actorExecuteDepth = 0;
std::size_t g_enemyGravityDepth = 0;
std::size_t g_enemyPositionDepth = 0;
std::size_t g_arrowMoveDepth = 0;
std::size_t g_enemyAnimationDepth = 0;
std::uint64_t g_slowMotionFrame = 0;
LinkPositionStep g_linkPositionStep{};
AimMovementInputState g_aimMovementInput{};
bool g_customSightActive = false;
bool g_thirdPersonAimActive = false;
f32 g_thirdPersonDistance = 300.0f;
f32 g_thirdPersonHeight = 130.0f;
f32 g_thirdPersonFovy = 45.0f;
f32 g_cameraFovyBeforeRun = 45.0f;
dCamera_c* g_cameraFovyBeforeRunCamera = nullptr;
dCamera_c* g_zoomCamera = nullptr;
f32 g_zoomBaseFovy = 45.0f;
bool g_zoomApplied = false;
dCamera_c* g_freeBowCamera = nullptr;
daAlink_c* g_freeBowCameraOwner = nullptr;
f32 g_freeBowCameraYaw = 0.0f;
f32 g_freeBowCameraPitch = 0.0f;

bool custom_camera_enabled() {
    return aimMode() != 0;
}

bool is_bow_or_slingshot(u16 item) {
    return daAlink_c::checkBowItem(item) || item == dItemNo_PACHINKO_e;
}

bool is_clawshot_item(u16 item) {
    return item == dItemNo_HOOKSHOT_e || item == dItemNo_W_HOOKSHOT_e;
}

bool aim_features_enabled() {
    return custom_camera_enabled() || aimMovementEnabled();
}

bool bow_subject_active(const daAlink_c* link) {
    return link != nullptr && !link->checkWolf() &&
           (link->mProcID == daAlink_c::PROC_BOW_SUBJECT ||
            (g_manualJumpBowAimActive && g_manualJumpOwner == link &&
             is_bow_or_slingshot(link->mEquipItem)));
}

bool boomerang_subject_active(const daAlink_c* link) {
    return link != nullptr && !link->checkWolf() &&
           link->mProcID == daAlink_c::PROC_BOOMERANG_SUBJECT;
}

bool clawshot_subject_active(const daAlink_c* link) {
    return link != nullptr && !link->checkWolf() &&
           is_clawshot_item(link->mEquipItem) &&
           link->mProcID == daAlink_c::PROC_HOOKSHOT_SUBJECT;
}

bool hookshot_action_active(const daAlink_c* link) {
    if (link == nullptr) return false;
    switch (link->mProcID) {
    case daAlink_c::PROC_HOOKSHOT_SUBJECT:
    case daAlink_c::PROC_HOOKSHOT_MOVE:
    case daAlink_c::PROC_HOOKSHOT_FLY:
    case daAlink_c::PROC_HOOKSHOT_ROOF_WAIT:
    case daAlink_c::PROC_HOOKSHOT_ROOF_SHOOT:
    case daAlink_c::PROC_HOOKSHOT_ROOF_BOOTS:
    case daAlink_c::PROC_HOOKSHOT_WALL_WAIT:
    case daAlink_c::PROC_HOOKSHOT_WALL_SHOOT:
        return true;
    default:
        return false;
    }
}

bool ironball_subject_active(const daAlink_c* link) {
    return link != nullptr && !link->checkWolf() &&
           link->mEquipItem == dItemNo_IRONBALL_e &&
           link->mProcID == daAlink_c::PROC_IRON_BALL_SUBJECT;
}

bool aim_subject_active(const daAlink_c* link) {
    return bow_subject_active(link) || boomerang_subject_active(link) ||
           clawshot_subject_active(link) || ironball_subject_active(link);
}

bool vanilla_aim_camera_active(const daAlink_c* link) {
    return aimMode() == 0 &&
        (boomerang_subject_active(link) || clawshot_subject_active(link) ||
         ironball_subject_active(link) ||
         (bow_subject_active(link) && !link->mLinkAcch.ChkGroundHit()));
}

bool is_hawkeye_bow(const daAlink_c* link) {
    return link != nullptr && link->mEquipItem == dItemNo_HAWK_ARROW_e;
}

camera_process_class* player_camera(daAlink_c* link) {
    return link == nullptr ? nullptr : dComIfGp_getCamera(link->field_0x317c);
}

bool is_player_camera(dCamera_c* camera, daAlink_c* link) {
    if (!coop_ptr_looks_live(camera) || !coop_ptr_looks_live(link)) return false;
    return camera->mpPlayerActor == link;
}

bool face_camera_view_yaw(daAlink_c* link) {
    auto* camera = player_camera(link);
    if (camera == nullptr) {
        return false;
    }

    cXyz direction = *fopCamM_GetCenter_p(camera) - *fopCamM_GetEye_p(camera);
    if (direction.absXZ() <= 0.001f) {
        return false;
    }

    link->shape_angle.y = cM_atan2s(direction.x, direction.z);
    link->field_0x310c = link->shape_angle.y;
    return true;
}

void face_camera_view(daAlink_c* link) {
    auto* process = player_camera(link);
    if (process == nullptr) {
        return;
    }

    const cXyz direction =
        process->mCamera.mViewCache.mCenter - process->mCamera.mViewCache.mEye;
    if (!std::isfinite(direction.x) || !std::isfinite(direction.y) ||
        !std::isfinite(direction.z) || direction.abs() <= 0.001f)
    {
        return;
    }

    if (direction.absXZ() > 0.001f) {
        link->shape_angle.y = cM_atan2s(direction.x, direction.z);
        link->field_0x310c = link->shape_angle.y;
    }
    link->mBodyAngle.x = cM_atan2s(-direction.y, direction.absXZ());
    link->checkBodyAngleX(link->mBodyAngle.x);
    link->field_0x310a = link->mBodyAngle.x;
}

cXyz aim_vector_to_world(daAlink_c* link, cXyz vector) {
    if (link->checkMagneBootsOn()) {
        mDoMtx_multVecSR(link->getMagneBootsMtx(), &vector, &vector);
    }
    return vector;
}

cXyz aim_vector_to_local(daAlink_c* link, cXyz vector) {
    if (link->checkMagneBootsOn()) {
        mDoMtx_multVecSR(link->getMagneBootsInvMtx(), &vector, &vector);
    }
    return vector;
}

bool aim_with_c_stick(daAlink_c* link) {
    auto* camera = player_camera(link);
    if (camera == nullptr) {
        return false;
    }

    const f32 stickValue = link->mStickValue;
    const f32 moveValue = link->mMoveValue;
    const s16 stickAngle = link->mStickAngle;
    const s16 moveAngle = link->mMoveAngle;
    const f32 cStickValue = std::clamp(
        mDoCPd_c::getSubStickValue(PAD_1), 0.0f, 1.0f);

    if (cStickValue <= 0.05f) {
        link->mStickValue = 0.0f;
        link->mMoveValue = 0.0f;
    } else {
        link->mStickValue = cStickValue;
        link->mMoveValue = cStickValue;
        link->mStickAngle = mDoCPd_c::getSubStickAngle(PAD_1) + 0x8000;
        link->mMoveAngle = link->mStickAngle + dCam_getControledAngleY(camera);
    }

    const BOOL result = link->setBodyAngleToCamera();
    link->mStickValue = stickValue;
    link->mMoveValue = moveValue;
    link->mStickAngle = stickAngle;
    link->mMoveAngle = moveAngle;
    return result;
}

void update_move_animation(daAlink_c* link) {
    f32 morph = -1.0f;
    if (link->checkZeroSpeedF()) {
        link->onModeFlg(1);
        if (link->field_0x2f98 != 3) {
            link->field_0x2f98 = 3;
        }
        link->current.angle.y = link->shape_angle.y - 0x4000;
    } else {
        link->offModeFlg(1);
    }
    link->setBlendAtnMoveAnime(morph);
}

void normalize_forward_aim_speed(daAlink_c* link) {
    if (link == nullptr || !aimMovementEnabled() ||
        link->mTargetedActor != nullptr || !link->checkInputOnR())
    {
        return;
    }

    const s16 moveDelta = link->mMoveAngle - link->shape_angle.y;
    if (cM_scos(moveDelta) < 0.99f ||
        std::fabs(cM_ssin(moveDelta)) > 0.02f)
    {
        return;
    }

    const f32 targetSpeed = link->mMaxSpeed * link->mStickValue * link->mStickValue;
    if (link->mNormalSpeed >= 0.0f && link->mNormalSpeed < targetSpeed) {
        link->mNormalSpeed = targetSpeed;
    }
}

bool camera_aim_ray(daAlink_c* link, cXyz& eye, cXyz& direction, f32& range) {
    auto* process = player_camera(link);
    if (process == nullptr) {
        return false;
    }

    auto& camera = process->mCamera;
    eye = camera.mViewCache.mEye;
    direction = camera.mViewCache.mCenter - eye;
    if (direction.abs() <= 0.001f) {
        return false;
    }
    direction.normalize();

    const int64_t offsetY = aimMode() == 1 ? thirdPersonReticleOffsetY() : 0;
    if (offsetY != 0) {
        const f32 fovy = std::clamp(camera.mViewCache.mFovy, 10.0f, 120.0f);
        const f32 screenOffset = static_cast<f32>(offsetY) / 224.0f;
        const f32 angle = std::atan(
            screenOffset * std::tan(fovy * kPi / 360.0f));
        cXyz up(-direction.x * direction.y,
                1.0f - direction.y * direction.y,
                -direction.z * direction.y);
        if (up.abs() > 0.001f) {
            up.normalize();
            const f32 cosine = std::cos(angle);
            const f32 sine = std::sin(angle);
            direction.set(
                direction.x * cosine + up.x * sine,
                direction.y * cosine + up.y * sine,
                direction.z * cosine + up.z * sine);
            direction.normalize();
        }
    }

    f32 arrowSpeed = 0.0f;
    link->getArrowFlyData(&range, &arrowSpeed, TRUE);
    if (!std::isfinite(range) || range <= 0.0f) {
        range = 3000.0f;
    }
    return true;
}

bool bow_reticle_target(daAlink_c* link, cXyz& target, cXyz& direction) {
    cXyz eye;
    f32 range = 0.0f;
    if (!camera_aim_ray(link, eye, direction, range)) {
        return false;
    }
    target.set(eye.x + direction.x * range,
               eye.y + direction.y * range,
               eye.z + direction.z * range);
    return true;
}

void draw_camera_center_sight(daAlink_c* link);

void draw_camera_bow_sight(daAlink_c* link) {
    if (boomerang_subject_active(link) || ironball_subject_active(link))
    {
        draw_camera_center_sight(link);
        return;
    }
    if (clawshot_subject_active(link)) {
        return;
    }

    cXyz target;
    cXyz direction;
    if (link == nullptr || !bow_reticle_target(link, target, direction)) {
        return;
    }
    link->mSight.setPos(&target);
    link->mSight.onDrawFlg();
    link->mSight.offLockFlg();
    g_customSightActive = true;
}

void draw_subject_aim_sight(daAlink_c* link) {
    if (link == nullptr) {
        return;
    }
    if (boomerang_subject_active(link)) {
        link->setBoomerangSight();
        if (custom_camera_enabled()) {
            draw_camera_center_sight(link);
        }
        return;
    }
    if (clawshot_subject_active(link)) {
        return;
    }
    if (ironball_subject_active(link)) {
        if (custom_camera_enabled()) {
            draw_camera_center_sight(link);
        }
        return;
    }
    if (is_hawkeye_bow(link)) return;

    link->setBowSight();
    if (custom_camera_enabled()) {
        link->mSight.onDrawFlg();
    }
    g_customSightActive = custom_camera_enabled();
}

void draw_camera_center_sight(daAlink_c* link) {
    auto* camera = player_camera(link);
    if (camera == nullptr) {
        return;
    }

    cXyz* eye = fopCamM_GetEye_p(camera);
    cXyz* center = fopCamM_GetCenter_p(camera);
    cXyz direction = *center - *eye;
    if (direction.abs() <= 0.001f) {
        return;
    }

    direction.normalize();
    cXyz position = *eye + direction * 10000.0f;
    link->mArrowLinChk.Set(eye, &position, link);
    if (dComIfG_Bgsp().LineCross(&link->mArrowLinChk)) {
        position = link->mArrowLinChk.GetCross();
    }

    link->mSight.setPos(&position);
    link->mSight.onDrawFlg();
    link->mSight.offLockFlg();
    g_customSightActive = true;
}

bool should_keep_camera_aim_sight(daAlink_c* link) {
    return link != nullptr && custom_camera_enabled() &&
           !is_hawkeye_bow(link) && !clawshot_subject_active(link);
}

void prepare_third_person_aim(daAlink_c* link) {
    if (link == nullptr || aimMode() != 1 || is_hawkeye_bow(link) ||
        g_thirdPersonAimActive)
    {
        return;
    }
    auto* process = player_camera(link);
    if (process == nullptr || !face_camera_view_yaw(link)) {
        return;
    }

    auto& camera = process->mCamera;
    const cXyz direction = aim_vector_to_local(
        link, camera.mViewCache.mCenter - camera.mViewCache.mEye);
    link->mBodyAngle.x = cM_atan2s(-direction.y, direction.absXZ());
    link->checkBodyAngleX(link->mBodyAngle.x);
    link->field_0x310a = link->mBodyAngle.x;
    g_thirdPersonDistance = std::clamp(direction.abs(), 200.0f, 600.0f);
    const cXyz centerOffset =
        aim_vector_to_local(link, camera.mViewCache.mCenter - link->current.pos);
    g_thirdPersonHeight = std::clamp(
        centerOffset.y, 100.0f, 180.0f);
    g_thirdPersonFovy = camera.mViewCache.mFovy;
    g_thirdPersonAimActive = true;
}

bool update_subject_aim(daAlink_c* link) {
    if (link == nullptr) {
        return false;
    }
    prepare_third_person_aim(link);
    if (is_hawkeye_bow(link) || !aimMovementEnabled()) {
        return false;
    }

    if (custom_camera_enabled() && aimMode() == 2) {
        face_camera_view_yaw(link);
    }

    const s16 shapeAngle = link->shape_angle.y;
    link->setSpeedAndAngleAtn();
    link->shape_angle.y = shapeAngle;
    update_move_animation(link);
    if (aim_with_c_stick(link)) {
        draw_subject_aim_sight(link);
    } else if (should_keep_camera_aim_sight(link)) {
        draw_camera_center_sight(link);
    }
    normalize_forward_aim_speed(link);
    return true;
}

void keep_camera_bow_sight(daAlink_c* link) {
    if (!should_keep_camera_aim_sight(link)) {
        return;
    }
    if (aimMovementEnabled() && aimMode() == 2) {
        aim_with_c_stick(link);
    }
    draw_camera_center_sight(link);
}

bool update_bullet_time_bow_aim(daAlink_c* link) {
    if (!should_keep_camera_aim_sight(link)) {
        return false;
    }
    prepare_third_person_aim(link);

    if (!aimMovementEnabled()) {
        link->setBodyAngleToCamera();
        draw_camera_center_sight(link);
        return true;
    }

    if (aimMode() == 2) {
        face_camera_view_yaw(link);
    } else {
        aim_with_c_stick(link);
    }
    keep_camera_bow_sight(link);
    return true;
}

void stop_bullet_time() {
    if (g_bulletTime.active && g_bulletTime.owner != nullptr &&
        g_bulletTime.owner == daAlink_getAlinkActorClass())
    {
        g_bulletTime.owner->speed.y =
            g_bulletTime.owner->mLinkAcch.ChkGroundHit()
                ? 0.0f : g_bulletTime.verticalSpeed;
    }
    g_bulletTime.active = false;
    g_bulletTime.owner = nullptr;
    g_bulletTime.started = {};
    g_bulletTime.verticalSpeed = 0.0f;
    if (!g_flurryRushSlowMotionActive) {
        g_actorClocks = {};
        g_linkPositionStep = {};
    }
}

bool botw_bullet_time_height_ready(daAlink_c* link) {
    if (link == nullptr) {
        return false;
    }

    f32 vanillaJumpHeight = 0.0f;
    if (!get_vanilla_standing_jump_height(link, &vanillaJumpHeight)) {
        return false;
    }

    dBgS_LinkGndChk groundCheck;
    groundCheck.SetPos(&link->current.pos);
    const f32 groundY = dComIfG_Bgsp().GroundCross(&groundCheck);
    if (!std::isfinite(groundY) || groundY == -G_CM3D_F_INF) {
        return false;
    }
    return link->current.pos.y - groundY >= vanillaJumpHeight * 2.0f;
}

void clear_bow_aim_state(daAlink_c* link);
void clear_manual_jump_impl(daAlink_c* link);

bool manual_jump_is_airborne(daAlink_c* link) {
    return link != nullptr && !link->mLinkAcch.ChkGroundHit() &&
           (link->mProcID == daAlink_c::PROC_AUTO_JUMP ||
            link->mProcID == daAlink_c::PROC_FALL ||
            link->mProcID == daAlink_c::PROC_BOW_SUBJECT ||
            clawshot_subject_active(link));
}

bool link_is_airborne(daAlink_c* link) {
    return link != nullptr && !link->checkWolf() &&
           !link->mLinkAcch.ChkGroundHit();
}

bool manual_jump_owner_is_airborne(daAlink_c* link) {
    return manual_jump_is_airborne(link) ||
           ((g_manualJumpBowAimActive || aim_subject_active(link)) &&
            link_is_airborne(link));
}

u16 selected_midair_bow_item(u8 slot) {
    if (slot == SELECT_ITEM_DOWN && zItemSlotEnabled() &&
        hawkeyeMidairBowAimEnabled() &&
        selected_item_for_button(slot) == dItemNo_HAWK_ARROW_e)
    {
        return dItemNo_HAWK_ARROW_e;
    }
    return dComIfGp_getSelectItem(slot);
}

bool item_button_triggered(const daAlink_c* link, u8 slot) {
    if (link == nullptr) {
        return false;
    }

    const bool gameTrigger =
        (static_cast<u32>(link->mItemTrigger) & (1u << slot)) != 0;
    switch (slot) {
    case SELECT_ITEM_X:
        return gameTrigger || mDoCPd_c::getTrigX(PAD_1) != 0;
    case SELECT_ITEM_Y:
        return gameTrigger || mDoCPd_c::getTrigY(PAD_1) != 0;
    case SELECT_ITEM_DOWN:
        return gameTrigger || mDoCPd_c::getTrigZ(PAD_1) != 0;
    default:
        return gameTrigger;
    }
}

u8 midair_clawshot_button_triggered(const daAlink_c* link) {
    if (link == nullptr) return dItemNo_NONE_e;

    constexpr std::array<u8, 3> slots = {
        SELECT_ITEM_X, SELECT_ITEM_Y, SELECT_ITEM_DOWN};
    for (const u8 slot : slots) {
        if (slot == SELECT_ITEM_DOWN && !zItemSlotEnabled()) continue;
        if (item_button_triggered(link, slot) &&
            is_clawshot_item(selected_item_for_button(slot)))
        {
            return slot;
        }
    }
    return dItemNo_NONE_e;
}

void capture_midair_clawshot_input(daAlink_c* link) {
    if (!aim_features_enabled() || !link_is_airborne(link)) return;
    if (hookshot_action_active(link)) return;

    const u8 slot = midair_clawshot_button_triggered(link);
    if (slot == dItemNo_NONE_e) return;

    const u16 item = selected_item_for_button(slot);
    link->mSelectItemId = slot;
    if (link->mEquipItem != item) {
        link->itemEquip(item);
        if (link->field_0x2fde != item) return;
        link->commonChangeItem();
    }
    if (!is_clawshot_item(link->mEquipItem)) return;

    link->setHookshotReadyAnime();
    link->procHookshotSubjectInit();
}

u8 hawkeye_combo_button_triggered(const daAlink_c* link) {
    if (link == nullptr || !hawkeyeMidairBowAimEnabled()) {
        return dItemNo_NONE_e;
    }

    constexpr std::array<u8, 3> slots = {
        SELECT_ITEM_X, SELECT_ITEM_Y, SELECT_ITEM_DOWN};
    for (const u8 slot : slots) {
        if (slot == SELECT_ITEM_DOWN &&
            (!zItemSlotEnabled() || secondaryItemAssigned(slot)))
        {
            continue;
        }
        if (item_button_triggered(link, slot) &&
            selected_midair_bow_item(slot) == dItemNo_HAWK_ARROW_e)
        {
            return slot;
        }
    }
    return dItemNo_NONE_e;
}

void capture_midair_hawkeye_combo_input(
    ModContext*, void* args, void*, void*)
{
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr || !link_is_airborne(link)) {
        return;
    }

    const u8 button = hawkeye_combo_button_triggered(link);
    if (button == dItemNo_NONE_e) {
        return;
    }
    if (g_manualJumpOwner != link) {
        g_manualJumpOwner = link;
        g_manualJumpStarted = std::chrono::steady_clock::now();
        g_bulletTime.usedForJump = false;
        g_bowBulletTimePending = false;
    }

    g_manualJumpBowAimRequested = true;
    g_manualJumpBowAimActive = true;
    g_manualJumpBowAimHawkeyeRequested = true;
    g_manualJumpBowAimHawkeyeButton = button;
    if (bulletTimeMode() != 0) {
        g_bowBulletTimePending = true;
    }
}

bool selected_bow_button_requested(daAlink_c* link) {
    if (link == nullptr) return false;
    const u32 buttons =
        static_cast<u32>(link->mItemTrigger) |
        static_cast<u32>(link->mItemButton);
    constexpr std::array<u8, 3> slots = {
        SELECT_ITEM_X, SELECT_ITEM_Y, SELECT_ITEM_DOWN};
    for (const u8 slot : slots) {
        if ((buttons & (1u << slot)) != 0 &&
            is_bow_or_slingshot(selected_midair_bow_item(slot)))
        {
            return true;
        }
    }
    return hawkeye_combo_button_triggered(link) != dItemNo_NONE_e;
}

bool selected_bow_button_triggered(daAlink_c* link) {
    if (link == nullptr) return false;
    constexpr std::array<u8, 3> slots = {
        SELECT_ITEM_X, SELECT_ITEM_Y, SELECT_ITEM_DOWN};
    for (const u8 slot : slots) {
        if ((static_cast<u32>(link->mItemTrigger) & (1u << slot)) != 0 &&
            is_bow_or_slingshot(selected_midair_bow_item(slot)))
        {
            return true;
        }
    }
    return hawkeye_combo_button_triggered(link) != dItemNo_NONE_e;
}

bool bow_aim_requested(daAlink_c* link) {
    return link != nullptr && is_bow_or_slingshot(link->mEquipItem) &&
           link->checkReadyItem() &&
           (selected_bow_button_requested(link) || link->itemTrigger() ||
            link->itemButton());
}

bool equip_selected_bow_for_bullet_time(
    daAlink_c* link, bool replaceCurrentBow = false,
    u8 forcedSlot = dItemNo_NONE_e)
{
    if (link == nullptr ||
        (is_bow_or_slingshot(link->mEquipItem) && !replaceCurrentBow))
    {
        return false;
    }
    const u32 buttons =
        static_cast<u32>(link->mItemTrigger) |
        static_cast<u32>(link->mItemButton);
    constexpr std::array<u8, 3> slots = {
        SELECT_ITEM_X, SELECT_ITEM_Y, SELECT_ITEM_DOWN};
    for (const u8 slot : slots) {
        if ((buttons & (1u << slot)) == 0 && slot != forcedSlot) continue;
        const u16 item = selected_midair_bow_item(slot);
        if (!is_bow_or_slingshot(item) || item == link->mEquipItem) continue;
        link->mSelectItemId = slot;
        link->itemEquip(item);
        if (link->field_0x2fde != item) return false;
        link->commonChangeItem();
        if (!is_bow_or_slingshot(link->mEquipItem)) return false;
        link->setBowReadyAnime();
        link->mItemMode = 0;
        link->setBowOrSlingStatus();
        return true;
    }
    return false;
}

void prepare_bow_aim(daAlink_c* link) {
    if (link == nullptr || !is_bow_or_slingshot(link->mEquipItem) ||
        !link->checkReadyItem())
    {
        return;
    }
    if (!link->checkBowAnime()) {
        prepare_third_person_aim(link);
        link->setBowReadyAnime();
        link->mItemMode = 0;
    }
    link->setBowOrSlingStatus();
}

void start_bullet_time(daAlink_c* link) {
    if (link == nullptr || g_bulletTime.active ||
        g_flurryRushSlowMotionActive ||
        g_bulletTime.usedForJump || bulletTimeMode() == 0 ||
        (bulletTimeMode() == 2 && !botw_bullet_time_height_ready(link)))
    {
        return;
    }
    g_bulletTime.owner = link;
    g_bulletTime.started = std::chrono::steady_clock::now();
    g_bulletTime.verticalSpeed = link->speed.y;
    g_bulletTime.active = true;
    g_bulletTime.usedForJump = true;
}

void update_bullet_time_before_jump(daAlink_c* link) {
    if (link == nullptr) return;
    const u8 hawkeyeComboButton = hawkeye_combo_button_triggered(link);
    const bool hawkeyeCombo = hawkeyeComboButton != dItemNo_NONE_e;
    if (hawkeyeCombo && link_is_airborne(link)) {
        if (g_manualJumpOwner != link) {
            g_manualJumpOwner = link;
            g_manualJumpStarted = std::chrono::steady_clock::now();
            g_bulletTime.usedForJump = false;
            g_bowBulletTimePending = false;
        }
        g_manualJumpBowAimRequested = true;
        g_manualJumpBowAimActive = true;
        g_manualJumpBowAimHawkeyeRequested = true;
        g_manualJumpBowAimHawkeyeButton = hawkeyeComboButton;
        if (bulletTimeMode() != 0) {
            g_bowBulletTimePending = true;
        }
    }
    if (g_manualJumpOwner != link) return;
    const auto now = std::chrono::steady_clock::now();
    if ((!manualJumpEnabled() && !revalisGaleEnabled() &&
         !(hawkeyeMidairBowAimEnabled() && g_manualJumpBowAimActive)) ||
        !manual_jump_owner_is_airborne(link) ||
        (!g_bulletTime.active &&
         now - g_manualJumpStarted >= kManualJumpTimeout) ||
        link->checkEventRun() || dComIfGp_event_runCheck() ||
        dComIfGp_isPauseFlag() || dComIfGp_isEnableNextStage())
    {
        clear_manual_jump_impl(link);
        return;
    }

    if (selected_bow_button_triggered(link)) {
        g_manualJumpBowAimRequested = true;
        g_manualJumpBowAimActive = true;
        g_manualJumpBowAimHawkeyeRequested = hawkeyeCombo;
        g_manualJumpBowAimHawkeyeButton =
            hawkeyeCombo ? hawkeyeComboButton : dItemNo_NONE_e;
        if (hawkeyeCombo || !is_bow_or_slingshot(link->mEquipItem)) {
            if (hawkeyeCombo) {
                if (bulletTimeMode() != 0) {
                    g_bowBulletTimePending = true;
                }
                equip_selected_bow_for_bullet_time(
                    link, true, hawkeyeComboButton);
            } else {
                g_bowBulletTimePending =
                    equip_selected_bow_for_bullet_time(link);
            }
        }
    }

    if (bulletTimeMode() == 0) {
        stop_bullet_time();
        g_bulletTime.usedForJump = false;
        return;
    }

    if (!g_bulletTime.active && !g_bulletTime.usedForJump &&
        link_is_airborne(link) && clawshot_subject_active(link) &&
        link->checkHookshotWait())
    {
        start_bullet_time(link);
    }

    if (g_bulletTime.active && link->doTrigger()) {
        stop_bullet_time();
    }
    if (!g_bulletTime.active && !g_bulletTime.usedForJump) {
        if (!g_bowBulletTimePending && selected_bow_button_requested(link) &&
            !is_bow_or_slingshot(link->mEquipItem))
        {
            g_bowBulletTimePending =
                equip_selected_bow_for_bullet_time(link);
        }
        if ((g_bowBulletTimePending || bow_aim_requested(link)) &&
            is_bow_or_slingshot(link->mEquipItem) && link->checkReadyItem() &&
            link->checkBowAnime())
        {
            start_bullet_time(link);
            g_bowBulletTimePending = false;
        }
    }
    if (g_bulletTime.active) {
        link->setDoStatus(BUTTON_STATUS_BACK);
        prepare_bow_aim(link);
    }
}

void update_bullet_time_after_jump(daAlink_c* link) {
    if (link == nullptr || !g_bulletTime.active ||
        g_bulletTime.owner != link)
    {
        return;
    }
    if (!manual_jump_owner_is_airborne(link)) {
        g_manualJumpOwner = nullptr;
        g_bowBulletTimePending = false;
        g_manualJumpBowAimRequested = false;
        g_manualJumpBowAimActive = false;
        g_manualJumpBowAimHawkeyeRequested = false;
        g_manualJumpBowAimHawkeyeButton = dItemNo_NONE_e;
        stop_bullet_time();
        g_bulletTime.usedForJump = false;
        return;
    }
    link->setDoStatus(BUTTON_STATUS_BACK);
}

void update_manual_jump_bow_aim(daAlink_c* link) {
    if (link == nullptr || g_manualJumpOwner != link ||
        !manual_jump_owner_is_airborne(link))
    {
        return;
    }

    if (g_manualJumpBowAimRequested &&
        g_manualJumpBowAimHawkeyeRequested &&
        link->mEquipItem != dItemNo_HAWK_ARROW_e)
    {
        equip_selected_bow_for_bullet_time(
            link, true, g_manualJumpBowAimHawkeyeButton);
        return;
    }

    if (!is_bow_or_slingshot(link->mEquipItem)) {
        g_manualJumpBowAimRequested = false;
        g_manualJumpBowAimActive = false;
        g_manualJumpBowAimHawkeyeRequested = false;
        g_manualJumpBowAimHawkeyeButton = dItemNo_NONE_e;
        return;
    }

    if (g_manualJumpBowAimRequested) {
        if (!link->checkReadyItem()) return;
        prepare_bow_aim(link);
    } else if (!g_manualJumpBowAimActive || !link->checkReadyItem()) {
        return;
    }

    if (g_bulletTime.active && g_bulletTime.owner == link) {
        if (!update_bullet_time_bow_aim(link) &&
            link->setBodyAngleToCamera())
        {
            link->setBowSight();
        }
    } else if (!update_subject_aim(link) &&
               link->setBodyAngleToCamera())
    {
        link->setBowSight();
    }
    if (g_manualJumpBowAimHawkeyeRequested &&
        is_hawkeye_bow(link) &&
        !dComIfGp_checkPlayerStatus0(0, 0x200000))
    {
        link->procCoSubjectivityInit();
        dComIfGp_setPlayerStatus0(0, 0x200000);
        link->seStartSystem(Z2SE_AL_HAWK_EYE_PUTON);
    }
    g_manualJumpBowAimRequested = false;
    g_manualJumpBowAimHawkeyeRequested = false;
    g_manualJumpBowAimHawkeyeButton = dItemNo_NONE_e;
}

HookAction before_player_execute_impl(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    capture_midair_clawshot_input(link);
    update_bullet_time_before_jump(link);
    return HOOK_CONTINUE;
}

void after_player_execute_impl(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    update_bullet_time_after_jump(link);
    update_manual_jump_bow_aim(link);
    clear_bow_aim_state(link);
}

void mark_manual_jump_started_impl(daAlink_c* link) {
    stop_bullet_time();
    g_manualJumpOwner = link;
    g_manualJumpStarted = std::chrono::steady_clock::now();
    g_bulletTime.usedForJump = false;
    g_bowBulletTimePending = false;
    g_manualJumpBowAimRequested = false;
    g_manualJumpBowAimActive = false;
    g_manualJumpBowAimHawkeyeRequested = false;
    g_manualJumpBowAimHawkeyeButton = dItemNo_NONE_e;
}

void clear_manual_jump_impl(daAlink_c* link) {
    if (link != nullptr && g_manualJumpOwner != link) return;
    stop_bullet_time();
    g_manualJumpOwner = nullptr;
    g_bulletTime.usedForJump = false;
    g_bowBulletTimePending = false;
    g_manualJumpBowAimRequested = false;
    g_manualJumpBowAimActive = false;
    g_manualJumpBowAimHawkeyeRequested = false;
    g_manualJumpBowAimHawkeyeButton = dItemNo_NONE_e;
}

f32 actor_time_scale(fopAc_ac_c* actor) {
    if ((!g_bulletTime.active && !g_flurryRushSlowMotionActive) ||
        actor == nullptr)
    {
        return 1.0f;
    }

    switch (fopAcM_GetName(actor)) {
    case fpcNm_ALINK_e:
        return 1.0f;
    default:
        return kEnemyTimeScale;
    }
}

bool is_slow_motion_target(const fopAc_ac_c* actor) {
    return actor != nullptr &&
        (actor->group == fopAc_ENEMY_e || actor->group == fopAc_NPC_e);
}

bool is_arrow_target_actor(fopAc_ac_c* actor) {
    if (actor == nullptr) {
        return false;
    }
    const auto actorName = fopAcM_GetName(actor);
    return actorName == fpcNm_OBJ_BOUMATO_e ||
           actorName == fpcNm_OBJ_ITAMATO_e;
}

SlowActorClock* actor_clock(fopAc_ac_c* actor) {
    if (actor == nullptr) {
        return nullptr;
    }

    SlowActorClock* oldest = &g_actorClocks.front();
    for (SlowActorClock& entry : g_actorClocks) {
        if (entry.actor == actor && entry.actorId == fopAcM_GetID(actor)) {
            entry.lastSeenFrame = g_slowMotionFrame;
            return &entry;
        }
        if (entry.actor == actor && entry.actorId != fopAcM_GetID(actor)) {
            entry = {};
            entry.actor = actor;
            entry.actorId = fopAcM_GetID(actor);
            entry.accumulator = kSimulationPeriod;
            entry.lastSeenFrame = g_slowMotionFrame;
            return &entry;
        }
        if (entry.actor == nullptr) {
            entry.actor = actor;
            entry.actorId = fopAcM_GetID(actor);
            entry.accumulator = kSimulationPeriod;
            entry.lastSeenFrame = g_slowMotionFrame;
            return &entry;
        }
        if (entry.lastSeenFrame < oldest->lastSeenFrame) {
            oldest = &entry;
        }
    }

    *oldest = {
        .actor = actor,
        .actorId = fopAcM_GetID(actor),
        .accumulator = kSimulationPeriod,
        .lastSeenFrame = g_slowMotionFrame,
    };
    return oldest;
}

bool should_skip_slow_actor(fopAc_ac_c* actor) {
    const f32 scale = actor_time_scale(actor);
    if (scale >= 0.999f) {
        return false;
    }

    SlowActorClock* clock = actor_clock(actor);
    if (clock == nullptr) {
        return false;
    }

    if (clock->accumulator >= kSimulationPeriod) {
        clock->accumulator -= kSimulationPeriod;
        return false;
    }

    clock->accumulator += kSimulationPeriod * scale;
    if (clock->accumulator + 0.000001f < kSimulationPeriod) {
        return true;
    }
    clock->accumulator -= kSimulationPeriod;
    return false;
}

HookAction track_actor_execute(ModContext*, void* args, void*, void*) {
    if (g_actorExecuteDepth < g_actorExecuteStack.size()) {
        g_actorExecuteStack[g_actorExecuteDepth] =
            static_cast<fopAc_ac_c*>(mods::arg<void*>(args, 0));
    }
    ++g_actorExecuteDepth;
    return HOOK_CONTINUE;
}

void untrack_actor_execute(ModContext*, void*, void*, void*) {
    if (g_actorExecuteDepth == 0) {
        return;
    }
    --g_actorExecuteDepth;
    if (g_actorExecuteDepth < g_actorExecuteStack.size()) {
        g_actorExecuteStack[g_actorExecuteDepth] = nullptr;
    }
}

HookAction scale_actor_execution(ModContext*, void* args, void* retval, void*) {
    if (g_actorExecuteDepth == 0 ||
        g_actorExecuteDepth > g_actorExecuteStack.size())
    {
        return HOOK_CONTINUE;
    }

    auto* actor = g_actorExecuteStack[g_actorExecuteDepth - 1];
    if (actor == nullptr || actor->sub_method == nullptr ||
        mods::arg<void*>(args, 1) != actor)
    {
        return HOOK_CONTINUE;
    }

    const auto* methods =
        reinterpret_cast<const process_method_class*>(actor->sub_method);
    if (mods::arg<const process_method_class*>(args, 0) != methods ||
        (!g_bulletTime.active && !g_flurryRushSlowMotionActive))
    {
        return HOOK_CONTINUE;
    }

    if (fopAcM_GetName(actor) == fpcNm_ARROW_e ||
        is_slow_motion_target(actor) || is_arrow_target_actor(actor))
    {
        return HOOK_CONTINUE;
    }

    if (!should_skip_slow_actor(actor)) {
        return HOOK_CONTINUE;
    }

    if (retval != nullptr) {
        *static_cast<int*>(retval) = 1;
    }
    return HOOK_SKIP_ORIGINAL;
}

HookAction scale_enemy_gravity(ModContext*, void* args, void*, void*) {
    const std::size_t depth = g_enemyGravityDepth++;
    if ((!g_bulletTime.active && !g_flurryRushSlowMotionActive) ||
        depth >= g_enemyGravitySteps.size())
    {
        return HOOK_CONTINUE;
    }

    auto* actor = mods::arg<fopAc_ac_c*>(args, 0);
    if (actor == g_bulletTime.owner && g_bulletTime.active) {
        g_enemyGravitySteps[depth] = {
            .actor = actor,
            .gravity = actor->gravity,
            .active = true,
        };
        actor->gravity = 0.0f;
        return HOOK_CONTINUE;
    }
    if (actor == g_flurryRushOwner && g_flurryRushLinkSlowed) {
        return HOOK_CONTINUE;
    }
    if (!is_slow_motion_target(actor)) {
        return HOOK_CONTINUE;
    }

    g_enemyGravitySteps[depth] = {
        .actor = actor,
        .gravity = actor->gravity,
        .active = true,
    };
    actor->gravity *= kEnemyTimeScale;
    return HOOK_CONTINUE;
}

void restore_enemy_gravity(ModContext*, void* args, void*, void*) {
    if (g_enemyGravityDepth == 0) {
        return;
    }

    const std::size_t depth = --g_enemyGravityDepth;
    if (depth >= g_enemyGravitySteps.size()) {
        return;
    }

    EnemyGravityStep& step = g_enemyGravitySteps[depth];
    auto* actor = mods::arg<fopAc_ac_c*>(args, 0);
    if (step.active && actor == step.actor) {
        actor->gravity = step.gravity;
    }
    step = {};
}

HookAction scale_enemy_position_step(ModContext*, void* args, void*, void*) {
    const std::size_t depth = g_enemyPositionDepth++;
    if ((!g_bulletTime.active && !g_flurryRushSlowMotionActive) ||
        depth >= g_enemyPositionSteps.size())
    {
        return HOOK_CONTINUE;
    }

    auto* actor = mods::arg<fopAc_ac_c*>(args, 0);
    if (actor == g_bulletTime.owner && g_bulletTime.active) {
        g_enemyPositionSteps[depth] = {
            .actor = actor,
            .startPosition = actor->current.pos,
            .positionScale = kLinkTimeScale,
            .linkPosition = true,
            .active = true,
        };
        return HOOK_CONTINUE;
    }
    if (actor == g_flurryRushOwner && g_flurryRushLinkSlowed) {
        g_enemyPositionSteps[depth] = {
            .actor = actor,
            .startPosition = actor->current.pos,
            .positionScale = kFlurryLinkTimeScale,
            .linkPosition = true,
            .active = true,
        };
        return HOOK_CONTINUE;
    }
    if (!is_slow_motion_target(actor)) {
        return HOOK_CONTINUE;
    }

    EnemyPositionStep& step = g_enemyPositionSteps[depth];
    step = {
        .actor = actor,
        .speed = actor->speed,
        .positionScale = kEnemyTimeScale,
        .active = true,
    };
    actor->speed.x *= kEnemyTimeScale;
    actor->speed.y *= kEnemyTimeScale;
    actor->speed.z *= kEnemyTimeScale;
    return HOOK_CONTINUE;
}

HookAction scale_arrow_movement(ModContext*, void* args, void*, void*) {
    const std::size_t depth = g_arrowMoveDepth++;
    if ((!g_bulletTime.active && !g_flurryRushSlowMotionActive) ||
        depth >= g_arrowMoveSteps.size())
    {
        return HOOK_CONTINUE;
    }

    auto* arrow = mods::arg<daArrow_c*>(args, 0);
    if (arrow == nullptr) {
        return HOOK_CONTINUE;
    }

    ArrowMoveStep& step = g_arrowMoveSteps[depth];
    step = {
        .arrow = arrow,
        .speed = arrow->speed,
        .gravity = arrow->gravity,
        .timeScale = kEnemyTimeScale,
        .procFunc = arrow->mProcFunc,
        .active = true,
    };
    arrow->speed.x *= step.timeScale;
    arrow->speed.y *= step.timeScale;
    arrow->speed.z *= step.timeScale;
    arrow->gravity *= step.timeScale;
    return HOOK_CONTINUE;
}

void restore_arrow_movement(ModContext*, void* args, void*, void*) {
    if (g_arrowMoveDepth == 0) {
        return;
    }

    const std::size_t depth = --g_arrowMoveDepth;
    if (depth >= g_arrowMoveSteps.size()) {
        return;
    }

    ArrowMoveStep& step = g_arrowMoveSteps[depth];
    auto* arrow = mods::arg<daArrow_c*>(args, 0);
    if (step.active && arrow == step.arrow) {
        if (arrow->mProcFunc == step.procFunc) {
            arrow->speed.x /= step.timeScale;
            arrow->speed.y /= step.timeScale;
            arrow->speed.z /= step.timeScale;
        }
        if (std::fabs(arrow->gravity - step.gravity * step.timeScale) <
            0.00001f)
        {
            arrow->gravity = step.gravity;
        }
    }
    step = {};
}

void restore_enemy_position_step(ModContext*, void* args, void*, void*) {
    if (g_enemyPositionDepth == 0) {
        return;
    }

    const std::size_t depth = --g_enemyPositionDepth;
    if (depth >= g_enemyPositionSteps.size()) {
        return;
    }

    EnemyPositionStep& step = g_enemyPositionSteps[depth];
    auto* actor = mods::arg<fopAc_ac_c*>(args, 0);
    if (step.active && actor == step.actor) {
        if (step.linkPosition) {
            const cXyz displacement = actor->current.pos - step.startPosition;
            actor->current.pos.x =
                step.startPosition.x + displacement.x * step.positionScale;
            actor->current.pos.z =
                step.startPosition.z + displacement.z * step.positionScale;
            actor->current.pos.y = step.startPosition.y;
            actor->old.pos.y = step.startPosition.y;
            if (g_linkPositionStep.active && g_linkPositionStep.link == actor) {
                g_linkPositionStep.genericStepApplied = true;
                g_linkPositionStep.lastGenericPosition = actor->current.pos;
            }
        } else {
            actor->speed = step.speed;
        }
    }
    step = {};
}

HookAction scale_enemy_animation(ModContext*, void* args, void*, void*) {
    const std::size_t depth = g_enemyAnimationDepth++;
    if ((!g_bulletTime.active && !g_flurryRushSlowMotionActive) ||
        depth >= g_enemyAnimationSteps.size() ||
        g_actorExecuteDepth == 0 ||
        g_actorExecuteDepth > g_actorExecuteStack.size())
    {
        return HOOK_CONTINUE;
    }

    fopAc_ac_c* actor = g_actorExecuteStack[g_actorExecuteDepth - 1];
    if (!is_slow_motion_target(actor)) {
        return HOOK_CONTINUE;
    }

    for (const auto& frozenTarget : g_iceArrowFrozenTargets) {
        if (frozenTarget.active &&
            frozenTarget.actorId == g_iceArrowExecutingActorId)
        {
            return HOOK_CONTINUE;
        }
    }

    auto* morf = mods::arg<mDoExt_morf_c*>(args, 0);
    if (morf == nullptr) {
        return HOOK_CONTINUE;
    }

    g_enemyAnimationSteps[depth] = {
        .morf = morf,
        .playSpeed = morf->getPlaySpeed(),
        .active = true,
    };
    morf->setPlaySpeed(morf->getPlaySpeed() * kEnemyTimeScale);
    return HOOK_CONTINUE;
}

void restore_enemy_animation(ModContext*, void*, void*, void*) {
    if (g_enemyAnimationDepth == 0) {
        return;
    }

    const std::size_t depth = --g_enemyAnimationDepth;
    if (depth >= g_enemyAnimationSteps.size()) {
        return;
    }

    EnemyAnimationStep& step = g_enemyAnimationSteps[depth];
    if (step.active && step.morf != nullptr) {
        step.morf->setPlaySpeed(step.playSpeed);
    }
    step = {};
}

HookAction slow_link_movement(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    g_linkPositionStep = {};
    if (!g_bulletTime.active || link != g_bulletTime.owner) {
        return HOOK_CONTINUE;
    }

    g_linkPositionStep = {
        .link = link,
        .startPosition = link->current.pos,
        .lastGenericPosition = link->current.pos,
        .gravity = link->gravity,
        .gravityChanged = true,
        .active = true,
    };
    link->speed.y = 0.0f;
    link->gravity = 0.0f;
    return HOOK_CONTINUE;
}

void finish_slow_link_movement(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!g_linkPositionStep.active || g_linkPositionStep.link != link) {
        return;
    }

    const cXyz startPosition = g_linkPositionStep.genericStepApplied
                                   ? g_linkPositionStep.lastGenericPosition
                                   : g_linkPositionStep.startPosition;
    const cXyz displacement = link->current.pos - startPosition;
    link->current.pos.x =
        startPosition.x + displacement.x * kLinkTimeScale;
    link->current.pos.z =
        startPosition.z + displacement.z * kLinkTimeScale;
    link->current.pos.y = startPosition.y;
    link->old.pos.y = startPosition.y;
    link->speed.y = 0.0f;
    if (g_linkPositionStep.gravityChanged) {
        link->gravity = g_linkPositionStep.gravity;
    }
    g_linkPositionStep = {};
}

HookAction replace_bow_subject(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr || (aimMode() == 0 && !aimMovementEnabled())) {
        return HOOK_CONTINUE;
    }

    link->setDoStatus(BUTTON_STATUS_BACK);
    if (!link->checkNextAction(0)) {
        if (!update_subject_aim(link) && link->setBodyAngleToCamera()) {
            if (boomerang_subject_active(link)) {
                link->setBoomerangSight();
            } else {
                link->setBowSight();
            }
        }
        keep_camera_bow_sight(link);
    }

    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

void adjust_aim_subject(daAlink_c* link) {
    if (link == nullptr || (aimMode() == 0 && !aimMovementEnabled()) ||
        link->checkNextAction(0))
    {
        return;
    }

    face_camera_view_yaw(link);
    const bool movementAim = update_subject_aim(link);
    if (custom_camera_enabled()) {
        face_camera_view(link);
    }
    if (!movementAim) {
        draw_subject_aim_sight(link);
    }
    keep_camera_bow_sight(link);
}

void adjust_boomerang_subject(ModContext*, void* args, void*, void*) {
    adjust_aim_subject(mods::arg<daAlink_c*>(args, 0));
}

HookAction isolate_aim_movement_input(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr || !aimMovementEnabled() ||
        (!clawshot_subject_active(link) && !ironball_subject_active(link)) ||
        g_aimMovementInput.active)
    {
        return HOOK_CONTINUE;
    }

    g_aimMovementInput = {
        .link = link,
        .moveValue = link->mMoveValue,
        .active = true,
    };
    link->mMoveValue = 0.0f;
    return HOOK_CONTINUE;
}

void restore_aim_movement_input(daAlink_c* link) {
    if (!g_aimMovementInput.active || g_aimMovementInput.link != link) {
        return;
    }
    link->mMoveValue = g_aimMovementInput.moveValue;
    g_aimMovementInput = {};
}

void adjust_clawshot_subject(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    restore_aim_movement_input(link);
    if (link == nullptr || !link->checkHookshotWait()) {
        return;
    }
    adjust_aim_subject(link);
}

void adjust_ironball_subject(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    restore_aim_movement_input(link);
    if (link == nullptr || link->mItemVar0.field_0x3018 != 2 ||
        !link->itemButton())
    {
        return;
    }
    adjust_aim_subject(link);
}

void aim_boomerang_throw(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!boomerang_subject_active(link)) {
        return;
    }

    auto* cameraProcess = player_camera(link);
    auto* boomerang =
        static_cast<daBoomerang_c*>(link->getBoomerangActor());
    if (cameraProcess == nullptr || boomerang == nullptr ||
        fopAcM_GetName(boomerang) != fpcNm_BOOMERANG_e ||
        !is_player_camera(&cameraProcess->mCamera, link))
    {
        return;
    }

    auto* camera = &cameraProcess->mCamera;
    const cXyz direction =
        camera->mViewCache.mCenter - camera->mViewCache.mEye;
    if (!std::isfinite(direction.x) || !std::isfinite(direction.y) ||
        !std::isfinite(direction.z) || direction.abs() <= 0.001f)
    {
        return;
    }

    boomerang->current.angle.x = direction.atan2sY_XZ();
    if (direction.absXZ() > 0.001f) {
        boomerang->current.angle.y = direction.atan2sX_Z();
    }
}

void select_bow_camera_mode(ModContext*, void* args, void* retval, void*) {
    auto* camera = mods::arg<dCamera_c*>(args, 0);
    auto* result = static_cast<s32*>(retval);
    auto* link = daAlink_getAlinkActorClass();
    const bool vanillaBoomerang =
        aimMode() == 0 && boomerang_subject_active(link);
    if (camera == nullptr || result == nullptr ||
        (!custom_camera_enabled() && !vanillaBoomerang) ||
        !aim_subject_active(link))
    {
        return;
    }
    if (vanillaBoomerang) {
        *result = 7;
        return;
    }
    if (*result == 7 || *result == 8) {
        *result = 8;
    }
}

bool camera_type_supports_bow_subject(dCamera_c* camera, int type) {
    if (camera == nullptr || type < 0 || type >= camera->mCamTypeNum) {
        return false;
    }
    const auto& modes = camera->mCamTypeData[type].field_0x18[camera->mIsWolf];
    return modes[7] >= 0 && modes[8] >= 0 &&
           camera->mCamParam.Algorythmn(modes[8]) == 4;
}

void select_bow_camera_type(ModContext*, void* args, void* retval, void*) {
    auto* camera = mods::arg<dCamera_c*>(args, 0);
    auto* result = static_cast<s32*>(retval);
    auto* link = daAlink_getAlinkActorClass();
    const bool vanillaBoomerang =
        aimMode() == 0 && boomerang_subject_active(link);
    if (camera == nullptr || result == nullptr ||
        (!custom_camera_enabled() && !vanillaBoomerang) ||
        !aim_subject_active(link))
    {
        return;
    }
    const int scopeType = camera->GetCameraTypeFromCameraName("Scope");
    if (vanillaBoomerang) {
        *result = scopeType;
        return;
    }
    if (*result == scopeType) {
        int aimType = camera->mMapToolType;
        if (!camera_type_supports_bow_subject(camera, aimType)) {
            aimType = camera->GetCameraTypeFromCameraName("FieldS");
        }
        if (camera_type_supports_bow_subject(camera, aimType)) {
            *result = aimType;
        }
    }
}

void adjust_third_person_bow_camera(ModContext*, void* args, void*, void*) {
    auto* camera = mods::arg<dCamera_c*>(args, 0);
    auto* link = daAlink_getAlinkActorClass();
    if (camera == nullptr || link == nullptr || aimMode() != 1 ||
        !g_thirdPersonAimActive || !aim_subject_active(link) ||
        camera->mCurMode != 8)
    {
        return;
    }

    const s16 yaw = link->getCameraAngleY();
    const s16 pitch = link->getCameraAngleX();
    const f32 horizontal = g_thirdPersonDistance * cM_scos(pitch);
    cXyz center = link->current.pos +
        aim_vector_to_world(link, cXyz(0.0f, g_thirdPersonHeight, 0.0f));
    const cXyz offset = aim_vector_to_world(
        link, cXyz(60.0f * cM_scos(yaw) +
                       40.0f * cM_ssin(yaw) * cM_ssin(pitch),
                   40.0f * cM_scos(pitch),
                   -60.0f * cM_ssin(yaw) +
                       40.0f * cM_scos(yaw) * cM_ssin(pitch)));
    cXyz shiftedCenter = center + offset;
    dBgS_CamLinChk shoulderCheck;
    if (camera->lineBGCheck(&center, &shiftedCenter, &shoulderCheck, 0x40b7)) {
        const f32 shoulderDistance =
            std::max(0.0f, (shoulderCheck.GetCross() - center).abs() - 5.0f);
        const f32 offsetLength = offset.abs();
        if (offsetLength > 0.001f) {
            shiftedCenter = center + offset * (shoulderDistance / offsetLength);
        }
    }
    center = shiftedCenter;
    cXyz eye = center + aim_vector_to_world(
        link, cXyz(-horizontal * cM_ssin(yaw),
                   g_thirdPersonDistance * cM_ssin(pitch),
                   -horizontal * cM_scos(yaw)));
    dBgS_CamLinChk cameraCheck;
    if (camera->lineBGCheck(&center, &eye, &cameraCheck, 0x40b7)) {
        cXyz inward = center - eye;
        inward.normalize();
        eye = cameraCheck.GetCross() + inward * 5.0f;
    }
    camera->mViewCache.mCenter = center;
    camera->mViewCache.mEye = eye;
    camera->mViewCache.mDirection.Val(eye - center);
    camera->mViewCache.mFovy = g_thirdPersonFovy;
    if (link->checkMagneBootsOn()) {
        camera->mUpOverride.field_0x0 = center;
        camera->mUpOverride.field_0xc = eye;
        camera->mUpOverride.field_0x24 = camera->mViewCache.mDirection;
        camera->mUpOverride.field_0x18 = *link->getMagneBootsTopVec();
    }
}

HookAction prepare_bow_aim_camera(ModContext*, void* args, void*, void*) {
    auto* camera = mods::arg<dCamera_c*>(args, 0);
    g_cameraFovyBeforeRunCamera = nullptr;
    if (camera == nullptr) {
        return HOOK_CONTINUE;
    }
    if (g_zoomApplied && camera == g_zoomCamera) {
        camera->mFovy = g_zoomBaseFovy;
        g_zoomApplied = false;
    }
    if (!is_player_camera(camera, daAlink_getAlinkActorClass())) {
        return HOOK_CONTINUE;
    }
    g_cameraFovyBeforeRunCamera = camera;
    g_cameraFovyBeforeRun = camera->mFovy;
    return HOOK_CONTINUE;
}

struct AimCameraStickInput {
    f32 x;
    f32 y;
};

AimCameraStickInput aim_camera_stick_input(
    dCamera_c* camera, const daAlink_c* link)
{
    const bool midairBowAim =
        link != nullptr && is_bow_or_slingshot(link->mEquipItem) &&
        !link->mLinkAcch.ChkGroundHit() && !aimMovementEnabled();
    if (midairBowAim) {
        return {
            mDoCPd_c::getStickX3D(camera->mPadID),
            mDoCPd_c::getStickY(camera->mPadID),
        };
    }

    return {
        mDoCPd_c::getSubStickX3D(camera->mPadID),
        mDoCPd_c::getSubStickY(camera->mPadID),
    };
}

cXyz predicted_aim_camera_direction(dCamera_c* camera, daAlink_c* link) {
    auto& view = camera->mViewCache;
    const bool hasCameraState =
        g_freeBowCamera == camera && g_freeBowCameraOwner == link;
    f32 yaw = hasCameraState
        ? g_freeBowCameraYaw
        : view.mDirection.mAzimuth.Degree();
    f32 pitch = hasCameraState
        ? g_freeBowCameraPitch
        : view.mDirection.mInclination.Degree();
    const AimCameraStickInput stick = aim_camera_stick_input(camera, link);
    yaw -= stick.x * kFreeBowCameraDegreesPerFrame;
    pitch = std::clamp(
        pitch - stick.y * kFreeBowCameraDegreesPerFrame,
        -kFreeBowCameraPitchLimit, kFreeBowCameraPitchLimit);

    auto direction = view.mDirection;
    direction.mAzimuth = cSAngle(yaw);
    direction.mInclination = cSAngle(pitch);
    return view.mCenter - (view.mCenter + direction.Xyz());
}

void apply_unconstrained_bow_camera(dCamera_c* camera, daAlink_c* link) {
    auto& view = camera->mViewCache;
    auto& direction = view.mDirection;
    if (g_freeBowCamera != camera || g_freeBowCameraOwner != link) {
        g_freeBowCamera = camera;
        g_freeBowCameraOwner = link;
        g_freeBowCameraYaw = direction.mAzimuth.Degree();
        g_freeBowCameraPitch = direction.mInclination.Degree();
    }

    const AimCameraStickInput stick = aim_camera_stick_input(camera, link);
    if (std::fabs(stick.x) > 0.01f || std::fabs(stick.y) > 0.01f) {
        g_freeBowCameraYaw -= stick.x * kFreeBowCameraDegreesPerFrame;
        g_freeBowCameraPitch -= stick.y * kFreeBowCameraDegreesPerFrame;
    }
    g_freeBowCameraPitch = std::clamp(
        g_freeBowCameraPitch, -kFreeBowCameraPitchLimit,
        kFreeBowCameraPitchLimit);

    direction.mAzimuth = cSAngle(g_freeBowCameraYaw);
    direction.mInclination = cSAngle(g_freeBowCameraPitch);
    cXyz center = view.mCenter;
    cXyz eye = center + direction.Xyz();
    dBgS_CamLinChk check;
    if (camera->lineBGCheck(&center, &eye, &check, 0x40b7)) {
        cXyz ray = eye - center;
        const f32 rayLength = ray.abs();
        if (rayLength > 0.001f) {
            ray.normalize();
            const f32 safeDistance = std::max(
                1.0f, (check.GetCross() - center).abs() - 5.0f);
            eye = center + ray * std::min(rayLength, safeDistance);
        }
    }

    view.mCenter = center;
    view.mEye = eye;
    direction.Val(eye - center);
    camera->mCenter = center;
    camera->mEye = eye;
    camera->mDirection = direction;
    camera->U2(direction.mAzimuth.Inv());
    camera->mUpOverride.field_0x0 = center;
    camera->mUpOverride.field_0xc = eye;
    camera->mUpOverride.field_0x24 = direction;
    camera->mUpOverride.field_0x18 = link->checkMagneBootsOn()
        ? *link->getMagneBootsTopVec()
        : cXyz::BaseY;
}

HookAction sync_airborne_bow_root_yaw(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr || !is_bow_or_slingshot(link->mEquipItem) ||
        link->mLinkAcch.ChkGroundHit() || !aim_subject_active(link) ||
        (!custom_camera_enabled() && !vanilla_aim_camera_active(link)))
    {
        return HOOK_CONTINUE;
    }

    auto* process = player_camera(link);
    if (process == nullptr) {
        return HOOK_CONTINUE;
    }

    const cXyz direction =
        predicted_aim_camera_direction(&process->mCamera, link);
    if (!std::isfinite(direction.x) || !std::isfinite(direction.y) ||
        !std::isfinite(direction.z) || direction.absXZ() <= 0.001f)
    {
        return HOOK_CONTINUE;
    }

    link->shape_angle.y = cM_atan2s(direction.x, direction.z);
    link->field_0x310c = link->shape_angle.y;
    return HOOK_CONTINUE;
}

HookAction sync_held_model_pose_to_camera(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    auto* model = mods::arg<J3DModel*>(args, 1);
    if (link == nullptr || model == nullptr || model != link->mpLinkModel ||
        (!is_bow_or_slingshot(link->mEquipItem) &&
         !is_clawshot_item(link->mEquipItem)) ||
        !aim_subject_active(link) ||
        (!custom_camera_enabled() && !vanilla_aim_camera_active(link)))
    {
        return HOOK_CONTINUE;
    }

    auto* process = player_camera(link);
    if (process == nullptr) {
        return HOOK_CONTINUE;
    }

    const cXyz direction =
        predicted_aim_camera_direction(&process->mCamera, link);
    if (!std::isfinite(direction.x) || !std::isfinite(direction.y) ||
        !std::isfinite(direction.z) || direction.abs() <= 0.001f)
    {
        return HOOK_CONTINUE;
    }

    s16 bodyPitch = cM_atan2s(-direction.y, direction.absXZ());
    if (link->mpHIO != nullptr) {
        bodyPitch = cLib_minMaxLimit<s16>(
            bodyPitch, link->mpHIO->mItem.m.mItemFPUpMaxUnk,
            link->mpHIO->mItem.m.mItemFPMaxUnk);
    } else {
        bodyPitch = cLib_minMaxLimit<s16>(bodyPitch, -0x4000, 0x4000);
    }

    const s16 cameraYaw = cM_atan2s(direction.x, direction.z);
    link->mBodyAngle.x = bodyPitch;
    link->mBodyAngle.y = cLib_minMaxLimit<s16>(
        cameraYaw - link->shape_angle.y, -0x4000, 0x4000);
    link->field_0x30c8 = link->mBodyAngle.y >> 1;
    link->field_0x310a = link->mBodyAngle.x;
    return HOOK_CONTINUE;
}

void adjust_bow_aim_camera(ModContext*, void* args, void*, void*) {
    auto* camera = mods::arg<dCamera_c*>(args, 0);
    auto* link = daAlink_getAlinkActorClass();
    if (camera == nullptr || camera != g_cameraFovyBeforeRunCamera) {
        return;
    }
    g_cameraFovyBeforeRunCamera = nullptr;
    if (!is_player_camera(camera, link) ||
        (!custom_camera_enabled() && !vanilla_aim_camera_active(link)) ||
        !aim_subject_active(link))
    {
        if (g_zoomApplied && camera == g_zoomCamera) {
            camera->mFovy = g_cameraFovyBeforeRun;
        }
        g_zoomCamera = nullptr;
        g_zoomApplied = false;
        g_thirdPersonAimActive = false;
        g_freeBowCamera = nullptr;
        g_freeBowCameraOwner = nullptr;
        if (g_customSightActive && link != nullptr) {
            link->mSight.offDrawFlg();
            link->mSight.offLockFlg();
            g_customSightActive = false;
        }
        return;
    }

    if (aimMode() == 2) {
        const f32 zoom = static_cast<f32>(cinemaZoomPercent()) / 100.0f;
        g_zoomCamera = camera;
        g_zoomBaseFovy = std::clamp(g_cameraFovyBeforeRun, 10.0f, 120.0f);
        camera->mFovy = std::clamp(g_zoomBaseFovy / zoom, 10.0f, 120.0f);
        g_zoomApplied = true;
    } else if (aimMode() == 0 && boomerang_subject_active(link)) {
        camera->mFovy = g_cameraFovyBeforeRun;
        camera->mViewCache.mFovy = g_cameraFovyBeforeRun;
    } else {
        g_zoomCamera = nullptr;
        g_zoomApplied = false;
    }
    apply_unconstrained_bow_camera(camera, link);
    draw_camera_bow_sight(link);
}

void clear_bow_aim_state(daAlink_c* link) {
    if (aim_subject_active(link)) {
        return;
    }
    g_thirdPersonAimActive = false;
    if (g_customSightActive && link != nullptr) {
        link->mSight.offDrawFlg();
        link->mSight.offLockFlg();
        g_customSightActive = false;
    }
}

void aim_arrow_at_bow_reticle(ModContext*, void* args, void*, void*) {
    auto* arrow = mods::arg<daArrow_c*>(args, 0);
    auto* link = daAlink_getAlinkActorClass();
    if (arrow == nullptr || link == nullptr ||
        (!custom_camera_enabled() && !vanilla_aim_camera_active(link)) ||
        !bow_subject_active(link))
    {
        return;
    }

    cXyz target;
    cXyz viewDirection;
    if (!bow_reticle_target(link, target, viewDirection)) {
        return;
    }
    cXyz direction(target.x - arrow->current.pos.x,
                   target.y - arrow->current.pos.y,
                   target.z - arrow->current.pos.z);
    const f32 speed = arrow->speed.abs();
    if (direction.abs() <= 0.001f || speed <= 0.001f) {
        return;
    }
    direction.normalize();
    arrow->current.angle.x = cM_atan2s(direction.y, direction.absXZ());
    arrow->current.angle.y = cM_atan2s(direction.x, direction.z);
    arrow->shape_angle.x = -arrow->current.angle.x;
    arrow->shape_angle.y = arrow->current.angle.y;
    arrow->speed.set(direction.x * speed, direction.y * speed, direction.z * speed);
}

HookAction hide_body_in_vanilla_aim(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    auto* model = mods::arg<J3DModel*>(args, 1);
    if (link == nullptr || model == nullptr || model != link->mpLinkModel ||
        aimMode() != 0 || !bow_subject_active(link))
    {
        return HOOK_CONTINUE;
    }
    return HOOK_SKIP_ORIGINAL;
}

}  // namespace

HookAction update_before_player_execute(
    ModContext* context, void* args, void* retval, void* user)
{
    return before_player_execute_impl(context, args, retval, user);
}

void update_after_player_execute(ModContext* context, void* args, void* retval, void* user) {
    after_player_execute_impl(context, args, retval, user);
}

void mark_manual_jump_started(daAlink_c* link) {
    mark_manual_jump_started_impl(link);
}

void clear_manual_jump(daAlink_c* link) {
    clear_manual_jump_impl(link);
}

void set_flurry_rush_slow_motion(daAlink_c* owner, bool active, bool slowLink) {
    const bool changed = g_flurryRushSlowMotionActive != active ||
        g_flurryRushOwner != (active ? owner : nullptr) ||
        g_flurryRushLinkSlowed != (active && slowLink);
    g_flurryRushSlowMotionActive = active;
    g_flurryRushOwner = active ? owner : nullptr;
    g_flurryRushLinkSlowed = active && slowLink;
    if (changed) {
        g_actorClocks = {};
        g_linkPositionStep = {};
    }
}

bool bullet_time_active() {
    return g_bulletTime.active;
}

void update_bullet_time_for_player(daAlink_c* link) {
    ++g_slowMotionFrame;
    clear_bow_aim_state(link);
    if (g_manualJumpOwner != nullptr &&
        (link != g_manualJumpOwner || !manual_jump_owner_is_airborne(link) ||
         (!g_bulletTime.active &&
          std::chrono::steady_clock::now() - g_manualJumpStarted >=
              kManualJumpTimeout) ||
         link->checkEventRun() || dComIfGp_event_runCheck() ||
         dComIfGp_isPauseFlag() || dComIfGp_isEnableNextStage()))
    {
        clear_manual_jump(g_manualJumpOwner);
    }
    if (g_bulletTime.active && bulletTimeMode() == 0) {
        stop_bullet_time();
    }
}

void stop_bow_bullet_time() {
    stop_bullet_time();
    g_bulletTime = {};
    g_flurryRushSlowMotionActive = false;
    g_flurryRushLinkSlowed = false;
    g_flurryRushOwner = nullptr;
    g_actorExecuteStack = {};
    g_actorExecuteDepth = 0;
    g_slowMotionFrame = 0;
    g_manualJumpOwner = nullptr;
    g_manualJumpStarted = {};
    g_bowBulletTimePending = false;
    g_manualJumpBowAimRequested = false;
    g_manualJumpBowAimActive = false;
    g_manualJumpBowAimHawkeyeRequested = false;
    g_manualJumpBowAimHawkeyeButton = dItemNo_NONE_e;
    g_zoomCamera = nullptr;
    g_zoomApplied = false;
    g_cameraFovyBeforeRunCamera = nullptr;
    g_freeBowCamera = nullptr;
    g_freeBowCameraOwner = nullptr;
}

}  // namespace bow_aim
