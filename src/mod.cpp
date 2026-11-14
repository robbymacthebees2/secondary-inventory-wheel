#include "mods/service.hpp"
#include "mods/svc/config.h"
#include "mods/svc/actor.h"
#include "mods/svc/gfx.h"
#include "mods/svc/hook.hpp"
#include "mods/svc/interp.h"
#include "mods/svc/log.h"
#include "mods/svc/resource.h"
#include "mods/svc/save.h"
#include "mods/svc/texture.h"
#include "mods/svc/ui.h"
#include "coop/mod.hpp"
#include "twilit_essentials/stamina.h"

#include "JSystem/J3DGraphBase/J3DMaterial.h"
#include "JSystem/J3DGraphBase/J3DPacket.h"
#include "JSystem/J3DGraphBase/J3DDrawBuffer.h"
#include "JSystem/J3DGraphBase/J3DShapeDraw.h"
#include "JSystem/J3DGraphBase/J3DSys.h"
#include "JSystem/J3DGraphBase/J3DTexture.h"
#include "JSystem/J3DGraphAnimator/J3DModel.h"
#include "JSystem/J3DGraphLoader/J3DModelLoader.h"
#include "JSystem/J2DGraph/J2DGrafContext.h"
#include "JSystem/J2DGraph/J2DPicture.h"
#include "JSystem/J2DGraph/J2DTextBox.h"
#include "JSystem/JAudio2/JAUSectionHeap.h"
#include "JSystem/JAudio2/JASWaveArcLoader.h"
#include "JSystem/JAudio2/JASWaveInfo.h"
#include "JSystem/JKernel/JKRExpHeap.h"
#include "JSystem/JUtility/JUTNameTab.h"
#include "d/actor/d_a_alink.h"
#include "d/actor/d_a_arrow.h"
#include "d/actor/d_a_e_po.h"
#include "d/actor/d_a_ni.h"
#include "d/actor/d_a_obj_glowSphere.h"
#include "d/actor/d_a_spinner.h"
#include "d/actor/d_a_e_zs.h"
#include "d/actor/d_a_obj_iceleaf.h"
#include "d/actor/d_a_npc_chin.h"
#include "d/d_a_item_static.h"
#include "d/actor/d_a_boomerang.h"
#include "d/actor/d_a_nbomb.h"
#include "d/actor/d_a_npc_tk.h"
#include "d/d_bomb.h"
#include "d/d_cc_d.h"
#include "d/d_bg_s.h"
#include "SSystem/SComponent/c_cc_d.h"
#include "SSystem/SComponent/c_cc_s.h"
#include "d/d_com_inf_game.h"
#include "d/d_camera.h"
#include "d/d_lib.h"
#include "d/d_meter2.h"
#include "d/d_particle_name.h"
#include "d/d_meter2_draw.h"
#include "d/d_kantera_icon_meter.h"
#include "d/d_menu_ring.h"
#include "d/d_menu_item_explain.h"
#include "d/d_menu_collect.h"
#include "d/d_menu_window.h"
#include "d/d_meter_map.h"
#include "d/d_meter2_info.h"
#include "d/d_meter_HIO.h"
#include "d/d_msg_object.h"
#include "d/d_kankyo.h"
#include "d/d_kankyo_rain.h"
#include "d/d_item_data.h"
#include "d/d_pane_class.h"
#include "d/d_save.h"
#include "d/d_stage.h"
#include "d/d_select_cursor.h"
#include "Z2AudioLib/Z2AudioMgr.h"
#include "Z2AudioLib/Z2SeMgr.h"
#include "Z2AudioLib/Z2SceneMgr.h"
#include "Z2AudioLib/Z2Param.h"
#include "Z2AudioLib/Z2SeqMgr.h"
#include "Z2AudioLib/Z2SoundMgr.h"
#include "f_op/f_op_actor_mng.h"
#include "f_op/f_op_actor_iter.h"
#include "f_pc/f_pc_name.h"
#include "f_pc/f_pc_layer.h"
#include "f_pc/f_pc_leaf.h"
#include "m_Do/m_Do_controller_pad.h"
#include "m_Do/m_Do_ext.h"

#include <dolphin/os.h>
#include <dolphin/pad.h>
#include <aurora/aurora.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <memory>
#include <new>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

DEFINE_MOD();
IMPORT_SERVICE(ConfigService, svc_config);
IMPORT_SERVICE(HookService, svc_hook);
IMPORT_SERVICE(GfxService, svc_gfx);
IMPORT_SERVICE(InterpService, svc_interp);
IMPORT_SERVICE(LogService, svc_log);
IMPORT_SERVICE(ResourceService, svc_resource);
IMPORT_SERVICE(SaveService, svc_save);
IMPORT_SERVICE(UiService, svc_ui);
IMPORT_OPTIONAL_SERVICE(ActorService, s_manualJumpActorService);
IMPORT_OPTIONAL_SERVICE(TextureService, s_ootHeartsTextureService);
IMPORT_OPTIONAL_SERVICE_VERSION(
    TwilitEssentialsStaminaService, svc_te_stamina, 0);

#include "features/dark_link_eye_bloom.cpp"

namespace {

namespace bow_aim {
void mark_manual_jump_started(daAlink_c* link);
void clear_manual_jump(daAlink_c* link);
}
f32 sprint_jump_speed_multiplier(daAlink_c* link);
bool sprint_requested(daAlink_c* link);
void apply_sprint_jump_speed(daAlink_c* link, f32 multiplier);

constexpr int kWheelOpensLeft = 0;
constexpr int kWheelOpensDown = 1;
constexpr int64_t kJumpButtonDefault = 0;
constexpr int64_t kManualJumpDefaultHeightPercent = 100;
constexpr int64_t kManualJumpMinHeightPercent = 100;
constexpr int64_t kManualJumpMaxHeightPercent = 500;
constexpr int64_t kSprintSpeedDefaultPercent = 150;
constexpr int64_t kSprintStaminaDrainDefault = 5;
constexpr int64_t kSprintStaminaRecoveryDefault = 5;
constexpr int64_t kGlideItemDefault = 0;
constexpr int64_t kGaleHeightDefaultPercent = 500;
constexpr int64_t kGaleHeightMinPercent = 100;
constexpr int64_t kGaleHeightMaxPercent = 1000;
constexpr int64_t kGaleChargeCapacityDefault = 3;
constexpr int64_t kGaleChargeCapacityMax = 12;
constexpr int64_t kGaleRecoveryDefaultSeconds = 120;
constexpr int64_t kGaleRecoveryMaxSeconds = 3600;
constexpr int64_t kAimModeDefault = 2;
constexpr int64_t kCinemaZoomDefaultPercent = 100;
constexpr int64_t kCinemaZoomMinPercent = 25;
constexpr int64_t kCinemaZoomMaxPercent = 400;
constexpr int64_t kThirdPersonReticleDefaultY = 0;
constexpr int64_t kThirdPersonReticleMinY = -40;
constexpr int64_t kThirdPersonReticleMaxY = 40;
constexpr int64_t kBulletTimeModeDefault = 1;
constexpr int64_t kBulletTimeArrowDamageDefaultPercent = 100;
constexpr int64_t kBulletTimeArrowDamageMinPercent = 0;
constexpr int64_t kBulletTimeArrowDamageMaxPercent = 1000;
constexpr int64_t kDarkLinkDamageMultiplierDefaultPercent = 200;
constexpr int64_t kDarkLinkDamageMultiplierMinPercent = 0;
constexpr int64_t kDarkLinkDamageMultiplierMaxPercent = 1000;
constexpr int64_t kGiantMaskDamageMultiplierDefaultPercent = 1100;
constexpr int64_t kGiantMaskDamageMultiplierMinPercent = 0;
constexpr int64_t kGiantMaskDamageMultiplierMaxPercent = 2000;
constexpr int kZItemSelectIndex = 2;
constexpr int kSecondaryAssignmentCount = 3;
constexpr int64_t kHoverPlaneLowerDefaultBinding = 5;
// SDL_GamepadButton::LEFT_SHOULDER; the mod SDK only exposes native button polling.
constexpr s32 kNativeLeftShoulderButton = 9;
// SDL_GamepadButton::RIGHT_STICK.
constexpr s32 kNativeRightStickButton = 8;
constexpr u8 kIceLeafSlot = 0xFB;
constexpr u8 kShieldSurfSlot = 0xFC;
constexpr u8 kSitSlot = 0xFD;
constexpr u8 kGoronTunicSlot = 0xFE;
constexpr u8 kIceLeafItemId = 0xF0;
constexpr u8 kShieldSurfItemId = 0xF7;
constexpr u8 kSitItemId = 0xF9;
constexpr u8 kGoronTunicItemId = 0xFB;
constexpr u8 kHawkGrassItem = 0xFD;
constexpr u8 kHoverBootsItem = 0xFC;
constexpr u8 kDarkLinkItem = 0xFA;
constexpr u8 kIceArrowItem = 0xF8;
constexpr u8 kLensOfTruthItem = 0xF6;
constexpr u8 kTruthMaskItem = 0xF5;
constexpr u8 kGoronMaskItem = 0xF4;
constexpr u8 kGiantMaskItem = 0xF3;
constexpr u8 kArmosHammerItem = 0xF2;
constexpr u8 kDekuNutItem = 0xF1;
constexpr u8 kSecondarySlotBase = 0x80;
constexpr int kHawkGrassIconTexture = 0x59;
constexpr u8 kSecondaryWheelSlotCount = MAX_ITEM_SLOTS + 4;
constexpr f32 kSecondaryIconScale = 0.55f;
constexpr f32 kSecondaryCursorScale = 0.5f;
constexpr u32 kHoverLandingRecoveryFrames = 30;
constexpr u32 kGoronLavaReturnDelayFrames = 5 * 30;
constexpr u16 kIceArrowType = 3;
constexpr u16 kIceArrowFreezeFrames = 90;
constexpr u16 kIceKeeseModelResource = 13;
constexpr f32 kIceKeeseArrowTipOffset = 95.0f;
constexpr f32 kIceKeeseArrowScale = 0.3f;
constexpr size_t kItemTextureBufferSize = 0xC00;
constexpr char kHoverBootsIconResource[] = "hover_boots_wings.bti";
constexpr char kDarkLinkIconResource[] = "dark_link.bti";
constexpr char kIceArrowIconResource[] = "ice_arrow.bti";
constexpr char kLensOfTruthIconResource[] = "lens_of_truth.bti";
constexpr char kTruthMaskIconResource[] = "truth_mask.bti";
constexpr char kGoronMaskIconResource[] = "goron_mask.bti";
constexpr char kGiantMaskIconResource[] = "giants_mask.bti";
constexpr char kArmosHammerIconResource[] = "armos_hammer.bti";
constexpr char kDekuNutIconResource[] = "deku_nut.bti";
constexpr char kIceLeafIconResource[] = "ice_leaf.bti";
constexpr char kSitIconResource[] = "sit.bti";
constexpr char kSecondaryAssignmentsBlobName[] = "secondaryAssignments";
constexpr u8 kGoronTunicDisplayItem = dItemNo_WEAR_KOKIRI_e;
constexpr u32 kHawkAttackType =
    AT_TYPE_THROW_OBJ | AT_TYPE_NORMAL_SWORD | AT_TYPE_BOMB;
constexpr unsigned int kHawkIdleDespawnDelayFrames = 5 * 30;
constexpr unsigned int kHoverWaterMissToleranceFrames = 8;
constexpr f32 kSitModelGroundOffset = -28.0f;
constexpr f32 kSitModelForwardOffset = 65.0f;
constexpr f32 kShieldSurfBoardRaise = 15.0f;
constexpr int64_t kRupeeMagnetDefaultRadiusPercent = 100;
constexpr int64_t kRupeeMagnetMinRadiusPercent = 50;
constexpr int64_t kRupeeMagnetMaxRadiusPercent = 300;
constexpr f32 kRupeeMagnetBaseRadius = 300.0f;
constexpr u32 kSitMusicBgmId = Z2BGM_MENU_SELECT;
constexpr size_t kSitMusicOptionCount = 1;

struct SitMusicWaveMapping {
    u32 track;
    u8 wave1;
    u8 alternateWave1;
    u8 wave2 = 0;
};

struct SitMusicWaveSnapshot {
    bool active = false;
    u8 requestWave1 = 0;
    u8 requestWave2 = 0;
    u8 loadedWave1 = 0;
    u8 loadedWave2 = 0;
    u32 mainTrack = static_cast<u32>(-1);
    u8 mainStatus = 0xff;
};

constexpr SitMusicWaveMapping kSitMusicWaveMappings[] = {
    {Z2BGM_MENU_SELECT, 0, 0},
};

enum class SecondaryIconContext : u8 {
    None,
    IceLeaf,
    Sit,
    GoronTunic,
    HawkGrass,
    HoverBoots,
    DarkLink,
    IceArrow,
    LensOfTruth,
    TruthMask,
    GoronMask,
    GiantMask,
    ArmosHammer,
    DekuNut,
};

enum class SitActionState : u8 {
    Inactive,
    Seated,
};

ConfigVarHandle g_wheelOpenDirection = 0;
ConfigVarHandle g_secondaryWheelBindingEnabled = 0;
ConfigVarHandle g_zItemSlotEnabled = 0;
ConfigVarHandle g_darkLinkDamageMultiplier = 0;
ConfigVarHandle g_giantMaskDamageMultiplier = 0;
ConfigVarHandle g_bulletTimeArrowDamageMultiplier = 0;
ConfigVarHandle g_flurryRushEnabled = 0;
ConfigVarHandle g_ootStyleHeartsEnabled = 0;
ConfigVarHandle g_enemySpawnerProfile = 0;
ConfigVarHandle g_manualJumpEnabled = 0;
ConfigVarHandle g_jumpButton = 0;
ConfigVarHandle g_manualJumpHeightPercent = 0;
ConfigVarHandle g_disableVanillaAutoJump = 0;
ConfigVarHandle g_sprintEnabled = 0;
ConfigVarHandle g_sprintSpeedPercent = 0;
ConfigVarHandle g_wolfSprintEnabled = 0;
ConfigVarHandle g_wolfSprintSpeedPercent = 0;
ConfigVarHandle g_sprintStaminaEnabled = 0;
ConfigVarHandle g_sprintStaminaDrain = 0;
ConfigVarHandle g_sprintStaminaRecovery = 0;
ConfigVarHandle g_glideEnabled = 0;
ConfigVarHandle g_glideItem = 0;
ConfigVarHandle g_revalisGaleEnabled = 0;
ConfigVarHandle g_galeHeightPercent = 0;
ConfigVarHandle g_galeChargeCapacity = 0;
ConfigVarHandle g_galeRecoverySeconds = 0;
ConfigVarHandle g_aimMode = 0;
ConfigVarHandle g_aimMovementEnabled = 0;
ConfigVarHandle g_hawkeyeMidairBowAimEnabled = 0;
ConfigVarHandle g_cinemaZoomPercent = 0;
ConfigVarHandle g_thirdPersonReticleOffsetY = 0;
ConfigVarHandle g_bulletTimeMode = 0;
ConfigVarHandle g_hoverPlaneLowerBinding = 0;
ConfigVarHandle g_infiniteLavaWalk = 0;
ConfigVarHandle g_twilightWard = 0;
ConfigVarHandle g_hawkOneShotDamage = 0;
ConfigVarHandle g_hoverBootsWaterWalking = 0;
ConfigVarHandle g_shieldSurfRBoost = 0;
ConfigVarHandle g_iceLeafRBoost = 0;
ConfigVarHandle g_clawshotUnevenSurfaceStick = 0;
ConfigVarHandle g_indoorMovementUnrestricted = 0;
ConfigVarHandle g_castleTownFirstPerson = 0;
ConfigVarHandle g_clawshotAnySurface = 0;
ConfigVarHandle g_lanternIgnitesEnemies = 0;
ConfigVarHandle g_swordFlourishEnabled = 0;
ConfigVarHandle g_rupeeMagnetEnabled = 0;
ConfigVarHandle g_rupeeMagnetClawshotOrbs = 0;
ConfigVarHandle g_rupeeMagnetRadiusPercent = 0;
ConfigVarHandle g_sitMusicTrack = 0;
ConfigVarHandle g_sitHealingEnabled = 0;
ConfigVarHandle g_disableSpinnerSurfaceBounce = 0;
ConfigVarHandle g_spinnerAttackAlwaysActive = 0;
ConfigVarHandle g_fairyCompanionColor = 0;
ConfigVarHandle g_fairyCompanionAppearance = 0;
ConfigVarHandle g_fairyCompanionLight = 0;
ConfigVarHandle g_fairyCompanionToggleBinding = 0;
std::array<std::array<char, 48>, kSitMusicOptionCount> g_sitMusicOptionLabelStorage{};
std::array<const char*, kSitMusicOptionCount> g_sitMusicOptionLabels{};
bool g_sitMusicOptionsInitialized = false;
u32 g_sitMusicActiveTrack = 0;
bool g_sitMusicAreaFadePending = false;
u16 g_sitMusicAreaFadeFrames = 0;
bool g_sitMusicAreaFadeTimeoutReported = false;
u32 g_sitMusicLastStartedTrack = 0;
u32 g_sitMusicReportedOverrideTrack = 0;
int64_t g_sitMusicLastLoggedSelection = -1;
bool g_sitMusicConfigReadFailureReported = false;
bool g_sitMusicAudioManagerUnavailableReported = false;
SitMusicWaveSnapshot g_sitMusicWaveSnapshot{};
u32 g_sitMusicWaveTrack = 0;
bool g_sitMusicBattleOverrideActive = false;
bool g_sitMusicOriginalBattleBgmOff = true;
bool g_sitMusicRestoreFailureReported = false;
u8 g_sitMusicResolvedWave1 = 0;
u32 g_sitMusicReportedFailureTrack = 0;
u32 g_sitMusicWaveFailureTrack = 0;
std::array<bool, 256> g_sitMusicOwnedWaves{};
std::array<bool, 256> g_sitMusicWaveEraseFailureReported{};
std::array<bool, 256> g_sitMusicWaveReleaseScheduled{};
std::array<u8, 256> g_sitMusicWaveReleaseFrames{};
UiMenuTabHandle g_robinMenuTab = 0;
UiMenuTabHandle g_creditsMenuTab = 0;
UiWindowHandle g_robinSettingsWindow = 0;
SaveObserverHandle g_secondarySaveObserver = 0;
dMenu_Ring_c* g_secondaryRing = nullptr;
bool g_openingSecondaryRing = false;
bool g_goronTunicEquipped = false;
u32 g_goronLavaContactFrames = 0;
bool g_hoverBootsEquipped = false;
bool g_shieldSurfActive = false;
bool g_shieldSurfIsIceLeaf = false;
bool g_shieldSurfStartPending = false;
bool g_shieldSurfDismountPending = false;
bool g_shieldSurfHoverPlaneActive = false;
bool g_shieldSurfPlaneRaisePending = false;
bool g_shieldSurfGroundPolygonValid = false;
cBgS_PolyInfo g_shieldSurfGroundPolygon;
fpc_ProcID g_shieldSurfBoardId = fpcM_ERROR_PROCESS_ID_e;
unsigned int g_shieldSurfStartWaitFrames = 0;
bool g_manualCuccoGlideActive = false;
bool g_manualGlideRetiring = false;
fpc_ProcID g_manualGlideActorId = fpcM_ERROR_PROCESS_ID_e;
u16 g_manualGlideLoadTicks = 0;
bool g_darkLinkVisualActive = false;
SitActionState g_sitActionState = SitActionState::Inactive;
s16 g_sitOriginalShapeAngleX = 0;
s16 g_sitOriginalShapeAngleZ = 0;
struct IceArrowFrozenTarget {
    fpc_ProcID actorId = fpcM_ERROR_PROCESS_ID_e;
    J3DModel* model = nullptr;
    cXyz frozenPosition{};
    csXyz frozenAngle{};
    csXyz frozenShapeAngle{};
    f32 originalGravity = 0.0f;
    f32 originalMaxFallSpeed = 0.0f;
    GXColorS10 originalTevColor{};
    u16 framesRemaining = 0;
    bool shaderColorCaptured = false;
    bool showIceEffect = false;
    bool active = false;
};
std::array<IceArrowFrozenTarget, 32> g_iceArrowFrozenTargets{};
cCcD_Obj* g_iceArrowSuppressedAttackCollider = nullptr;
fpc_ProcID g_iceArrowExecutingActorId = fpcM_ERROR_PROCESS_ID_e;
fpc_ProcID g_iceArrowPreviousExecutingActorIds[16]{};
u32 g_iceArrowActorExecuteDepth = 0;
bool g_iceArrowSelected = false;
bool g_lensOfTruthActive = false;
bool g_secondaryItemInputHandled = false;
bool g_jumpButtonConfigReadFailureReported = false;
bool g_manualJumpHeightConfigReadFailureReported = false;
bool g_glideItemConfigReadFailureReported = false;
bool g_jumpInputVisibilityFailureReported = false;
bool g_hoverPlaneLowerBindingConfigReadFailureReported = false;
bool g_hoverPlaneLowerBindingWasHeld = false;
bool g_hoverGroundHeightValid = false;
bool g_hoverLandingSequenceActive = false;
bool g_hoverAtSurface = false;
bool g_hoverWaterGroundPlaneActive = false;
f32 g_hoverWaterSurfaceHeight = 0.0f;
f32 g_hoverWaterUnderlyingGroundHeight = -G_CM3D_F_INF;
unsigned int g_hoverWaterMissFrames = 0;
bool g_secondaryHudAssignmentsInitialized = false;
SecondaryIconContext g_secondaryIconContext = SecondaryIconContext::None;
dMeter2_c* g_secondaryHudOwner = nullptr;
std::array<u8, MAX_SELECT_ITEM> g_secondaryHudAssignments{};
std::array<bool, MAX_SELECT_ITEM> g_secondaryHudRefreshPending{};
alignas(32) std::array<std::array<u8, kItemTextureBufferSize>, 2>
    g_secondaryZHudTextureBuffers{};
dMeter2Draw_c* g_secondaryZHudOwner = nullptr;
J2DPicture* g_secondaryZHudPicture = nullptr;
std::array<J2DPicture*, 4> g_secondaryZHudAmmoDigits{};
dMeter2Draw_c* g_secondaryZHudAmmoOwner = nullptr;
dMeter2Draw_c* g_secondaryZHudAmmoDigitsOwner = nullptr;
J2DScreen* g_secondaryZHudAmmoScreen = nullptr;
f32 g_secondaryZHudAmmoX = 0.0f;
f32 g_secondaryZHudAmmoY = 0.0f;
f32 g_secondaryZHudAmmoDigitSize = 0.0f;
int g_secondaryZHudAmmoDigitCount = 0;
bool g_secondaryZHudAmmoVisible = false;
dKantera_icon_c* g_secondaryZHudLanternMeter = nullptr;
bool g_secondaryZHudLanternVisible = false;
u8 g_secondaryZHudSlot = dItemNo_NONE_e;
u8 g_secondaryZHudItem = dItemNo_NONE_e;
bool g_secondaryZHudGaleBombCombo = false;
J2DScreen* g_zSlotMidnaScreen = nullptr;
J2DPane* g_zSlotMidnaOriginalParent = nullptr;
f32 g_zSlotMidnaOriginalScaleX = 1.0f;
f32 g_zSlotMidnaOriginalScaleY = 1.0f;
f32 g_zSlotMidnaOriginalTranslateX = 0.0f;
f32 g_zSlotMidnaOriginalTranslateY = 0.0f;
f32 g_hoverGroundHeight = 0.0f;
u32 g_hoverLandingFrames = 0;
u32 g_hoverRippleEmitter = 0;
bool g_hoverMagicEffectActive = false;
std::array<std::array<u32, 3>, 2> g_hoverMagicEmitters{};
bool g_primaryRingOpen = false;
bool g_pendingPrimaryItemSync = false;
bool g_deletingSecondaryRing = false;
bool g_hoverBootsTransitionPending = false;
int g_keyWaitDepth = 0;
int g_mapInputDepth = 0;
request_of_phase_process_class g_hawkGrassPhase{};
request_of_phase_process_class g_iceKeesePhase{};
JKRExpHeap* g_iceKeeseHeap = nullptr;
J3DModel* g_iceKeeseModel = nullptr;
daAlink_c* g_pendingHawkGrassUse = nullptr;
u8 g_pendingHawkGrassButton = 0;
bool g_hawkGrassPhaseRequested = false;
bool g_hawkGrassArchiveLoaded = false;
bool g_iceKeesePhaseRequested = false;
bool g_iceKeeseArchiveLoaded = false;
bool g_iceKeeseLoadFailed = false;
fpc_ProcID g_spawnedHawkId = fpcM_ERROR_PROCESS_ID_e;
bool g_spawnedHawkAttackSeen = false;
bool g_spawnedHawkReturnSeen = false;
unsigned int g_spawnedHawkIdleFrames = 0;

std::array<u8, MAX_ITEM_SLOTS> g_secondarySlots = [] {
    std::array<u8, MAX_ITEM_SLOTS> slots{};
    slots.fill(dItemNo_NONE_e);
    slots[0] = kHawkGrassItem;
    slots[1] = kHoverBootsItem;
    slots[2] = kDarkLinkItem;
    slots[3] = kIceArrowItem;
    slots[4] = kLensOfTruthItem;
    slots[5] = kTruthMaskItem;
    slots[6] = kGoronMaskItem;
    slots[7] = kGiantMaskItem;
    slots[8] = kArmosHammerItem;
    slots[9] = kDekuNutItem;
    return slots;
}();
std::array<u8, MAX_SELECT_ITEM> g_secondarySelectedItems = [] {
    std::array<u8, MAX_SELECT_ITEM> items{};
    items.fill(dItemNo_NONE_e);
    return items;
}();
std::array<u8, kSecondaryWheelSlotCount> g_secondaryWheelSlotIds{};
std::array<f32, kSecondaryWheelSlotCount> g_secondaryWheelSlotPosX{};
std::array<f32, kSecondaryWheelSlotCount> g_secondaryWheelSlotPosY{};
std::array<s16, kSecondaryWheelSlotCount> g_secondaryWheelSlotAngles{};
bool g_secondaryRingInterpolationErrorLogged = false;
std::array<J2DPicture*, 3> g_tunicWheelItemTex{};
std::array<ResTIMG*, 3> g_tunicWheelItemBuf{};
JKRExpHeap* g_tunicWheelItemHeap = nullptr;
J2DPicture* g_sitWheelItemTex = nullptr;
ResTIMG* g_sitWheelItemBuf = nullptr;
JKRExpHeap* g_sitWheelItemHeap = nullptr;
J2DPicture* g_iceLeafWheelItemTex = nullptr;
ResTIMG* g_iceLeafWheelItemBuf = nullptr;
JKRExpHeap* g_iceLeafWheelItemHeap = nullptr;
J2DPicture* g_lensOfTruthWheelItemTex = nullptr;
ResTIMG* g_lensOfTruthWheelItemBuf = nullptr;
JKRExpHeap* g_lensOfTruthWheelItemHeap = nullptr;
J2DPicture* g_goronMaskWheelItemTex = nullptr;
ResTIMG* g_goronMaskWheelItemBuf = nullptr;
JKRExpHeap* g_goronMaskWheelItemHeap = nullptr;
J2DPicture* g_giantMaskWheelItemTex = nullptr;
ResTIMG* g_giantMaskWheelItemBuf = nullptr;
JKRExpHeap* g_giantMaskWheelItemHeap = nullptr;
ResTIMG* g_goronMaskHudIcon = nullptr;
JKRExpHeap* g_goronMaskHudIconHeap = nullptr;
bool g_goronMaskHudIconLoadFailed = false;
bool g_goronMaskHudIconApplied = false;
std::array<J2DPicture*, 3> g_shieldSurfWheelItemTex{};
std::array<ResTIMG*, 3> g_shieldSurfWheelItemBuf{};
JKRExpHeap* g_shieldSurfWheelItemHeap = nullptr;
bool g_truthMaskActive = false;
bool g_truthMaskPhaseRequested = false;
bool g_truthMaskArchiveLoaded = false;
bool g_truthMaskLoadFailed = false;
request_of_phase_process_class g_truthMaskPhase{};
JKRExpHeap* g_truthMaskHeap = nullptr;
J3DModel* g_truthMaskModel = nullptr;
bool g_goronMaskActive = false;
bool g_goronMaskPhaseRequested = false;
bool g_goronMaskArchiveLoaded = false;
bool g_goronMaskLoadFailed = false;
request_of_phase_process_class g_goronMaskPhase{};
JKRExpHeap* g_goronMaskHeap = nullptr;
J3DModel* g_goronMaskModel = nullptr;
u8 g_goronMaskInputBlockFrames = 0;
bool g_giantMaskActive = false;
bool g_giantMaskPhaseRequested = false;
bool g_giantMaskArchiveLoaded = false;
bool g_giantMaskLoadFailed = false;
request_of_phase_process_class g_giantMaskPhase{};
JKRExpHeap* g_giantMaskHeap = nullptr;
J3DModel* g_giantMaskModel = nullptr;
bool g_armosHammerActive = false;
J2DPicture* g_armosHammerWheelItemTex = nullptr;
ResTIMG* g_armosHammerWheelItemBuf = nullptr;
JKRExpHeap* g_armosHammerWheelItemHeap = nullptr;

void reset_shield_surf_state_for_save_change();
void reset_manual_jump_input_state();
bool z_slot_midna_left_triggered();
void begin_deku_nut_use(daAlink_c* alink);
void set_goron_tunic_active(bool active);
namespace fire_arrows {
ModResult add_settings(UiElementHandle panel);
ModResult initialize();
void update();
}
// Feature modules share this translation unit's private state and hook declarations.
#include "features/core_helpers.cpp"
#include "features/gale_bomb.cpp"
#include "features/hover_boots.cpp"
#include "features/secondary_wheel.cpp"
#include "features/world_hooks.cpp"
#include "features/goron_tunic.cpp"
#include "features/dark_link.cpp"
#include "features/dark_link_veil.cpp"
#include "features/character_actions.cpp"
#include "features/sit_music.cpp"
#include "features/shield_surf.cpp"
#include "features/armos_hammer.cpp"
#include "features/manual_jump.cpp"
#include "features/sprint.cpp"
#include "features/bow_aim.cpp"
#include "features/flurry_rush.cpp"
#include "features/arrow_damage.cpp"
#include "features/oot_hearts.cpp"
#include "features/enemy_spawner.cpp"
#include "features/manual_glider_visual.cpp"
#include "features/fairy_companion.cpp"
#include "features/settings.cpp"
#include "features/ice_arrow.cpp"
#include "features/fire_arrows.cpp"
#include "features/deku_nut.cpp"
#include "features/lens_of_truth.cpp"
#include "features/truth_mask.cpp"
#include "features/goron_mask.cpp"
#include "features/giant_mask.cpp"

}

bool coop_fairy_companion_network_state(FairyCompanionNetworkState* out) {
    if (out == nullptr) return false;

    *out = {};
    out->appearance = maFairyCompanion_c::getSelectedAppearance();
    out->color = maFairyCompanion_c::getSelectedColor();
    out->active = s_fairyActorActive ? 1 : 0;
    if (!s_fairyActorActive) return true;

    auto* actor = static_cast<maFairyCompanion_c*>(fopAcM_SearchByID(s_fairyActorId));
    if (actor == nullptr) {
        out->active = 0;
        return true;
    }
    out->x = actor->current.pos.x;
    out->y = actor->current.pos.y;
    out->z = actor->current.pos.z;
    out->scale = actor->scale.x;
    out->yaw = actor->shape_angle.y;
    out->room = fopAcM_GetRoomNo(actor);
    return true;
}

void coop_apply_fairy_tint(J3DModel* model, uint8_t color) {
    maFairyCompanion_c::applyFairyTint(model, color);
}

namespace {

using manual_glider_visual::ManualGliderCarrierDraw;
using manual_glider_visual::ManualGliderDrawHand;
using manual_glider_visual::ManualGliderPlayerDraw;

void set_mod_init_error(ModError* error, ModResult result, const char* message) {
    if (error == nullptr) return;
    error->code = result;
    std::snprintf(error->message, sizeof(error->message), "%s (result %d)", message,
        static_cast<int>(result));
}

ModResult install_hooks(ModError* error) {
    using flurry_rush::FlurryColliderSet;
    using flurry_rush::FlurryLinkDamageAction;
    using arrow_damage::ArrowDamageAttackPower;

    ADD_PRE(OpenExplain, suppress_secondary_explain);
    ADD_POST(SetNameString, name_secondary_item);
    ADD_PRE(RingSetSelectItem, prepare_secondary_equip_icon);
    ADD_POST(RingSetSelectItem, finish_secondary_equip_icon);
    ADD_POST(RingSetJumpItem, clear_secondary_assignment_for_vanilla_ring);
    ADD_PRE(RingSetSelectItemForce, preserve_secondary_z_assignment);
    ADD_PRE(RingSetActiveCursor, assign_secondary_z_from_ring);
    ADD_PRE(RingIsMixItemOn, show_ice_arrow_combo_on_prompt);
    ADD_PRE(RingIsMixItemOff, show_ice_arrow_combo_off_prompt);
    ADD_PRE(ChangeTextureItemXY, prepare_hover_boots_hud_icon);
    ADD_POST(ChangeTextureItemXY, tint_goron_tunic_hud_icon);
    ADD_POST(ChangeTextureItemXY, finish_hover_boots_hud_icon);
    ADD_POST(MeterMidnaAlpha, update_z_slot_hud);
    ADD_POST(ZSlotAmmoDraw, draw_z_slot_ammo_numbers);
    const ModResult cleanupHookResult =
        mods::hook::add_pre<ZSlotAmmoMeterDelete>(release_z_slot_ammo_digits);
    if (cleanupHookResult != MOD_OK) {
        svc_log->warn(mod_ctx,
            "Z-slot ammo digit cleanup hook unavailable; continuing without it");
    }
    ADD_POST(MeterMoveButtonXY, refresh_secondary_hud_assignments);
    ADD_PRE(ReadItemTexture, apply_hawk_grass_equip_icon);
    ADD_POST(ReadItemTexture, replace_secondary_custom_icon);
    ADD_POST(SetWaterY, treat_lava_as_surface);
    ADD_POST(SetWaterY, treat_water_as_surface_with_hover_boots);
    ADD_POST(SetRoomInfo, maintain_lava_ground);
    ADD_POST(SetRoomInfo, maintain_hover_water_surface);
    ADD_POST(AutoGroundHit, maintain_hover_height);
    ADD_POST(PlayerExecute, disable_hover_boots_for_wolf);
    ADD_POST(PlayerExecute, enforce_goron_tunic_clothing);
    ADD_PRE(DeferGlideBomblingExplode, defer_glide_bombling_explode);
    ADD_POST(GlideBomblingWait, finish_glide_bombling_fall);
    ADD_PRE(bow_aim::BulletTimeActorExecute, bow_aim::track_actor_execute);
    ADD_POST(bow_aim::BulletTimeActorExecute, bow_aim::untrack_actor_execute);
    ADD_POST(bow_aim::BulletTimeActorExecute, manual_glider_visual::after_carrier_execute);
    ADD_PRE(bow_aim::BulletTimeProcessExecute, bow_aim::scale_actor_execution);
    ADD_PRE(bow_aim::BulletTimeCalcSpeed, bow_aim::scale_enemy_gravity);
    ADD_POST(bow_aim::BulletTimeCalcSpeed, bow_aim::restore_enemy_gravity);
    ADD_PRE(bow_aim::BulletTimePosMove, bow_aim::scale_enemy_position_step);
    ADD_POST(bow_aim::BulletTimePosMove, bow_aim::restore_enemy_position_step);
    ADD_PRE(bow_aim::BulletTimeArrowMove, bow_aim::scale_arrow_movement);
    ADD_POST(bow_aim::BulletTimeArrowMove, bow_aim::restore_arrow_movement);
    ADD_PRE(bow_aim::BulletTimeLinkPosMove, bow_aim::slow_link_movement);
    ADD_POST(bow_aim::BulletTimeLinkPosMove, bow_aim::finish_slow_link_movement);
    ADD_PRE(bow_aim::BowSubject, bow_aim::replace_bow_subject);
    ADD_POST(bow_aim::BoomerangSubject, bow_aim::adjust_boomerang_subject);
    ADD_PRE(bow_aim::ClawshotSubject, bow_aim::isolate_aim_movement_input);
    ADD_POST(bow_aim::ClawshotSubject, bow_aim::adjust_clawshot_subject);
    ADD_PRE(bow_aim::IronBallSubject, bow_aim::isolate_aim_movement_input);
    ADD_POST(bow_aim::IronBallSubject, bow_aim::adjust_ironball_subject);
    ADD_POST(bow_aim::BowCameraNextMode, bow_aim::select_bow_camera_mode);
    ADD_POST(bow_aim::BowCameraNextType, bow_aim::select_bow_camera_type);
    ADD_POST(bow_aim::BowSubjectCamera, bow_aim::adjust_third_person_bow_camera);
    ADD_PRE(bow_aim::AimSetMatrix, bow_aim::sync_airborne_bow_root_yaw);
    ADD_PRE(bow_aim::AimModelCalc, bow_aim::sync_held_model_pose_to_camera);
    ADD_PRE(bow_aim::BowCameraRun, bow_aim::prepare_bow_aim_camera);
    ADD_POST(bow_aim::BowCameraRun, bow_aim::adjust_bow_aim_camera);
    ADD_PRE(bow_aim::BowModelDraw, bow_aim::hide_body_in_vanilla_aim);
    ADD_POST(bow_aim::BowArrowShooting, bow_aim::aim_arrow_at_bow_reticle);
    ADD_POST(bow_aim::BoomerangThrow, bow_aim::aim_boomerang_throw);
    ADD_PRE(PlayerExecute, bow_aim::update_before_player_execute);
    ADD_POST(PlayerExecute, bow_aim::update_after_player_execute);
    ADD_POST(bow_aim::BowSetStickData, bow_aim::capture_midair_hawkeye_combo_input);
    ADD_POST(gale_bomb::GaleBombThrowBoomerang, gale_bomb::attach_bomb_to_boomerang);
    ADD_PRE(FlurryColliderSet, flurry_rush::before_collider_set_callback);
    ADD_PRE(FlurryLinkDamageAction, flurry_rush::before_link_damage_action_callback);
    ADD_PRE(PlayerExecute, flurry_rush::update);
    ADD_POST(ArrowDamageAttackPower, arrow_damage::apply_modifier);
    ADD_PRE(WheelSprintCheckMoveDoAction, defer_sprint_roll_for_window);
    ADD_PRE(WheelSprintProcMove, before_sprint_proc_move);
    ADD_POST(WheelSprintProcMove, after_sprint_proc_move);
    ADD_PRE(WheelSprintSetDoubleAnime, adjust_sprint_animation_cadence);
    ADD_PRE(WheelSprintGetMainBckData, select_sprint_run_animation);
    ADD_PRE(WheelWolfSprintMovement, before_wolf_sprint_movement);
    ADD_PRE(WheelWolfSprintDash, before_wolf_sprint_movement);
    ADD_PRE(WheelWolfSprintJump, before_wolf_sprint_jump);
    ADD_POST(WheelWolfSprintJump, after_wolf_sprint_jump);
    ADD_PRE(WheelSprintCommonProcInit, before_sprint_common_proc_init);
    ADD_POST(PlayerExecute, update_sprint_stamina);
    ADD_POST(WheelSprintStaminaMeterDraw, draw_sprint_stamina_meter);
    ADD_POST(PlayerExecute, update_revali_gale_visual);
    ADD_POST(FairyActionLabelHook, replace_fairy_action_label);
    ADD_POST(FairyDoStatus, choose_fairy_action_prompt);
    ADD_POST(FairyCommonDoStatus, choose_fairy_action_prompt);
    ADD_POST(FairySetStickData, intercept_fairy_a_toggle);
    ADD_POST(PlayerDraw, draw_revali_gale_visual);
    ADD_POST(ManualGliderPlayerDraw, manual_glider_visual::after_player_draw);
    ADD_POST(ManualGliderDrawHand, manual_glider_visual::after_draw_hand);
    ADD_PRE(ManualGliderCarrierDraw, manual_glider_visual::before_carrier_draw);
    ADD_PRE(CollectChangeClothe, prevent_goron_tunic_clothing_change);
    ADD_PRE(GetGroundAngle, flatten_hover_water_ground_angle);
    ADD_PRE(GetSlidePolygon, suppress_submerged_water_slide_polygon);
    ADD_PRE(CheckSlope, suppress_submerged_water_slope);
    ADD_PRE(SetStepsOffset, suppress_submerged_water_step_offset);
    ADD_PRE(IceSlipBgCheck, skip_submerged_ice_slip);
    ADD_PRE(SwimOutAfter, keep_hover_boots_on_water_surface);
    ADD_PRE(CheckAutoJumpAction, handle_hover_airborne_action);
    ADD_PRE(CheckAutoJumpAction, before_check_auto_jump_action);
    ADD_POST(CheckAutoJumpAction, after_check_auto_jump_action);
    ADD_PRE(DisableAutoJumpFallInit, preserve_sprint_speed_on_ledge_fall);
    ADD_POST(DisableAutoJumpFallInit, finish_run_off_ledge_fall);
    ADD_PRE(DisableAutoJumpWolfFallInit, preserve_sprint_speed_on_ledge_fall);
    ADD_PRE(ProcHangStartInit, suppress_hover_ledge_grab);
    ADD_PRE(ProcHangFallStartInit, suppress_hover_ledge_grab);
    ADD_PRE(CheckLandAction, land_on_lava_surface);
    ADD_PRE(SetSandShapeOffset, lower_sit_model_to_ground);
    ADD_PRE(SetSandShapeOffset, prepare_lava_sinking);
    ADD_PRE(CheckWaterPolygonUnder, suppress_lava_submersion);
    ADD_PRE(CheckPolyDamage, suppress_lava_damage);
    ADD_PRE(ProcDamageInit, suppress_twilight_ward_hit_stun);
    ADD_PRE(ProcCoLargeDamageInit, prevent_shield_surf_collision_knockoff);
    ADD_PRE(SwordFlourishItemChange, enable_sword_putaway_flourish);
    ADD_PRE(ProcCoLargeDamageInit, suppress_twilight_ward_hit_stun);
    ADD_PRE(ProcLargeDamageUpInit, suppress_twilight_ward_hit_stun);
    ADD_PRE(ProcCoLargeDamageWallInit, suppress_twilight_ward_hit_stun);
    ADD_PRE(ProcCoPolyDamageInit, suppress_twilight_ward_hit_stun);
    ADD_PRE(ProcCoElecDamageInit, suppress_twilight_ward_hit_stun);
    ADD_PRE(ProcGuardBreakInit, suppress_twilight_ward_hit_stun);
    ADD_PRE(ProcHorseDamageInit, suppress_twilight_ward_hit_stun);
    ADD_PRE(ProcSwimDamageInit, suppress_twilight_ward_hit_stun);
    ADD_PRE(SetHeavyBoots, handle_hover_boots_heavy_toggle);
    ADD_POST(SetStickData, update_secondary_assigned_item_input);
    ADD_PRE(MidnaTalkTrigger, route_midna_to_dpad_left);
    ADD_POST(MidnaTalkTrigger, restore_midna_z_trigger);
    ADD_POST(PlayerPadRead, update_manual_jump_button_input);
    ADD_POST(PlayerPadRead, capture_z_slot_midna_input);
    ADD_POST(SetStickData, suppress_goron_mask_sword_item_input);
    ADD_POST(SetStickData, suppress_sit_stick_input);
    ADD_PRE(CheckSnowCode, allow_shield_surf_on_any_ground);
    ADD_PRE(CheckShieldDraw, suppress_player_shield_draw_during_surf);
    ADD_PRE(CheckNextActionBow, suppress_bow_action_during_shield_surf);
    ADD_PRE(CheckWaitAction, preserve_sit_idle_animation);
    ADD_POST(HawkGrassVoiceStart, spawn_hawk_after_grass_sound);
    ADD_PRE(RingMove, handle_secondary_ring_move);
    ADD_PRE(RingDraw, handle_secondary_ring_draw);
    ADD_PRE(RingIsMoveEnd, handle_secondary_ring_move_end);
    ADD_PRE(RingIsOpen, open_secondary_ring);
    ADD_PRE(RingIsClose, close_secondary_ring);

    ADD_PRE(KeyWait, begin_key_wait);
    ADD_POST(KeyWait, end_key_wait);
    ADD_PRE(RingCreate, prepare_secondary_wheel);
    ADD_POST(RingCreate, finish_secondary_wheel);
    ADD_PRE(RingDelete, prepare_ring_delete);
    ADD_POST(RingDelete, clear_secondary_wheel);
    ADD_PRE(DownTrigger, remap_down_trigger);
    ADD_PRE(LeftTrigger, remap_left_trigger);

    ADD_PRE(MapMeterMove, begin_map_input);
    ADD_POST(MapMeterMove, end_map_input);
    ADD_PRE(MapKeyCheck, begin_map_input);
    ADD_POST(MapKeyCheck, end_map_input);
    ADD_PRE(MapShow, begin_map_input);
    ADD_POST(MapShow, end_map_input);
    ADD_PRE(CheckRestartRoom, prevent_lava_return);
    ADD_POST(CheckItemChangeFromButton, handle_vanilla_z_item_change);
    ADD_PRE(CheckSetItemTrigger, activate_vanilla_z_slot_item);
    ADD_POST(CheckItemSetButtonHook, count_z_item_as_assigned);
    const ModResult zSelectItemHookResult =
        mods::hook::add_post<ZSelectItemAccessorHook>(
            svc_hook, resolve_z_selected_item);
    if (zSelectItemHookResult != MOD_OK) {
        return zSelectItemHookResult;
    }
    ADD_PRE(ItemTriggerCheck, suppress_goron_mask_sword_trigger);
    ADD_PRE(ItemButtonCheck, suppress_goron_mask_sword_button);
    ADD_PRE(DrawButtonB, set_goron_mask_roll_action);
    ADD_POST(DrawButtonB, show_goron_mask_roll_prompt);
    ADD_PRE(PlayerExecute, begin_goron_mask_roll);
    ADD_POST(PlayerExecute, continue_goron_mask_roll);
    ADD_POST(PlayerExecute, enable_goron_roll_attack);
    ADD_POST(PlayerExecute, fire_arrows::update_lantern_ignition_after_player_execute);
    ADD_POST(GiantMaskSetMatrix, apply_giant_mask_player_scale);
    ADD_PRE(PlayerExecute, update_ice_arrow_freezes);
    ADD_POST(PlayerExecute, process_shield_surf_transition);
    ADD_POST(PlayerDraw, draw_truth_mask_on_player);
    ADD_POST(PlayerDraw, draw_goron_mask_model);
    ADD_POST(PlayerDraw, draw_giant_mask_on_player);
    ADD_POST(PlayerDraw, draw_armos_hammer_on_player);
    ADD_POST(PlayerExecute, record_armos_hammer_hand_matrix);
    ADD_POST(PlayerExecute, hide_armos_hammer_sword);
    ADD_POST(ArmosHammerGetSwordAtType, apply_armos_hammer_attack_type);
    ADD_PRE(DekuNutBombExecute, handle_deku_nut_bomb_execute);
    ADD_PRE(DekuNutBombExplodeInit, intercept_deku_nut_bomb_explosion);
    ADD_PRE(DekuNutBombCheckTimerStop, suppress_deku_nut_bomb_fuse_effects);
    ADD_PRE(DekuNutBombDraw, draw_deku_nut_bomb);
    ADD_POST(CheckSceneChange, disable_hover_boots_after_scene_change);
    ADD_POST(CheckSceneChange, clear_deku_nut_bombs_after_scene_change);
    ADD_POST(CheckSceneChange, deactivate_giant_mask_after_scene_change);
    ADD_POST(CheckSceneChange, deactivate_armos_hammer_after_scene_change);
    ADD_PRE(OffWolfEyeUp, preserve_lens_of_truth_vision);
    ADD_PRE(PoeTargetHitCheck, allow_human_lens_poe_hit);
    ADD_PRE(ProcAutoJump, release_manual_cucco_glide_on_a);
    ADD_POST(BoardCommon, boost_shield_surf_speed);
    ADD_POST(ClawshotUnevenSurfaceGetTriPla, allow_clawshot_uneven_surface_stick);
    ADD_PRE(SetNormalSpeedF, begin_unrestricted_room_movement);
    ADD_POST(SetNormalSpeedF, end_unrestricted_room_movement);
    ADD_PRE(DecideCommonDoStatus, begin_unrestricted_room_movement);
    ADD_POST(DecideCommonDoStatus, end_unrestricted_room_movement);
    ADD_PRE(SetFrontWallType, begin_unrestricted_room_movement);
    ADD_POST(SetFrontWallType, end_unrestricted_room_movement);
    ADD_POST(CheckRoomOnlyForMovement, allow_unrestricted_room_movement);
    ADD_POST(CheckNotAutoJumpStage, allow_unrestricted_room_auto_jump);
    ADD_POST(CheckNoSubjectModeCamera, allow_castle_town_first_person);
    ADD_POST(CastleTownChangeModeOK, allow_castle_town_subject_mode);
    ADD_POST(bow_aim::BowCameraNextType, use_castle_town_subject_camera_type);
    ADD_PRE(CheckHookshotStickBG, allow_clawshots_any_surface);
    ADD_PRE(BoardCommon, prepare_shield_surf_hover_plane_board_common);
    ADD_POST(BoardCommon, restore_shield_surf_hover_plane_board_common);
    ADD_PRE(ShieldSurfGetTriPla, flatten_shield_surf_hover_plane_polygon);
    ADD_PRE(SpinnerSetReflectAngle, prevent_spinner_surface_bounce);
    ADD_PRE(SpinnerReflectAccept, keep_spinner_attack_always_active);
    ADD_PRE(IceLeafExecute, keep_shield_surf_board_in_ride_mode);
    ADD_PRE(IceLeafDraw, draw_shield_surf_on_native_board);
    ADD_POST(MakeArrow, set_ice_arrow_actor_type_after_creation);
    ADD_POST(ArrowDraw, draw_ice_keese_on_held_arrow);
    ADD_PRE(ChangeArrowType, apply_ice_arrow_bow_selection);
    ADD_PRE(ArrowAtHitCallBack, register_ice_arrow_hit);
    ADD_PRE(MorfFrameUpdate, freeze_ice_arrow_target_animation);
    ADD_PRE(MorfFrameUpdate, bow_aim::scale_enemy_animation);
    ADD_POST(MorfFrameUpdate, bow_aim::restore_enemy_animation);
    ADD_PRE(LeafExecute, begin_ice_arrow_actor_execute);
    ADD_PRE(LeafExecute, prepare_lens_poe_execute);
    ADD_POST(LeafExecute, reveal_lens_poe_after_execute);
    ADD_POST(LeafExecute, hold_ice_arrow_target_in_place);
    ADD_POST(LeafExecute, end_ice_arrow_actor_execute);
    ADD_PRE(CollisionSet, suppress_ice_frozen_enemy_attack);
    ADD_PRE(SearchActorAngleY, hide_truth_masked_player_angle_from_enemy);
    ADD_PRE(SearchActorDistance, hide_truth_masked_player_from_enemy);
    ADD_PRE(SearchActorDistance2, hide_truth_masked_player_from_enemy);
    ADD_PRE(SearchActorDistanceXZ, hide_truth_masked_player_from_enemy);
    ADD_PRE(SearchActorDistanceXZ2, hide_truth_masked_player_from_enemy);
    ADD_PRE(SeenActorAngleY, hide_truth_masked_player_seen_angle_from_enemy);
    ADD_PRE(DrawItemNum, hide_secondary_assigned_vanilla_ammo_count);
    ADD_POST(CollisionSet, restore_ice_frozen_enemy_attack);
    ADD_PRE(ProcCoLavaReturnInit, delay_goron_lava_return);
    ADD_PRE(CheckRestartRoom, suppress_water_surface_restart);
    ADD_PRE(CheckSceneChange, suppress_submerged_water_exit);
    ADD_PRE(ModelCalc, offset_sit_model_forward_before_calc);
    ADD_POST(SettingTevStruct, apply_ice_arrow_shader_after_tev_setup);
    ADD_POST(ModelCalc, update_hover_magic_after_model_calc);
    ADD_POST(ItemExecute, collect_nearby_rupees);
    ADD_PRE(GlowSphereExecute, collect_nearby_clawshot_orbs);
    return MOD_OK;
}

#undef ADD_PRE
#undef ADD_POST

}  // namespace

bool wheel_is_sit_seated() {
    return g_sitActionState == SitActionState::Seated;
}

bool wheel_is_shield_surf_board(daAlink_c* alink, fopAc_ac_c* ride) {
    return g_shieldSurfActive && !g_shieldSurfIsIceLeaf &&
           alink != nullptr && alink->checkBoardRide() &&
           ride != nullptr && fopAcM_GetID(ride) == g_shieldSurfBoardId;
}

bool wheel_is_armos_hammer_active() {
    return wheel_is_armos_hammer_active_internal();
}

void features_draw_remote_armos_hammer(uint8_t playerId, const float matrix[12]) {
    draw_remote_armos_hammer_internal(playerId, matrix);
}

bool features_is_spawned_hawk(uint32_t id) {
    return is_spawned_hawk_internal(id);
}

bool wheel_is_dark_link_active() {
    return g_darkLinkVisualActive;
}

bool wheel_is_giant_mask_active() {
    const auto* link = daAlink_getAlinkActorClass();
    return g_giantMaskActive && link != nullptr && !link->checkWolf();
}

uint16_t wheel_pvp_sword_damage_percent(u32 attackType) {
    constexpr u32 kSwordAttackTypes =
        AT_TYPE_NORMAL_SWORD | AT_TYPE_MASTER_SWORD;
    if ((attackType & kSwordAttackTypes) == 0) {
        return 100;
    }

    int64_t multiplier = 100;
    const auto* link = daAlink_getAlinkActorClass();
    if (link == nullptr || link->checkWolf()) {
        return 100;
    }
    if (g_darkLinkVisualActive) {
        multiplier = darkLinkDamageMultiplierPercent();
    } else if (wheel_is_giant_mask_active()) {
        multiplier = giantMaskDamageMultiplierPercent();
    } else {
        return 100;
    }
    return static_cast<uint16_t>(std::clamp<int64_t>(multiplier, 0, 2000));
}

void wheel_dark_link_remote_draw_begin(J3DModel* model, bool active) {
    g_darkLinkRemoteDrawModel = active ? model : nullptr;
}

void wheel_dark_link_remote_draw_end() {
    g_darkLinkRemoteDrawModel = nullptr;
}

extern "C" {
MOD_EXPORT ModResult mod_initialize(ModError* error) {
    g_giantMaskActive = false;
    g_giantMaskPhaseRequested = false;
    g_giantMaskArchiveLoaded = false;
    g_giantMaskLoadFailed = false;
    g_giantMaskPhase = {};
    g_giantMaskModel = nullptr;
    g_darkLinkVisualActive = false;
    g_shieldSurfActive = false;
    g_shieldSurfStartPending = false;
    g_shieldSurfDismountPending = false;
    g_shieldSurfBoardId = fpcM_ERROR_PROCESS_ID_e;
    g_shieldSurfStartWaitFrames = 0;
    g_manualCuccoGlideActive = false;
    g_manualGlideRetiring = false;
    g_manualGlideActorId = fpcM_ERROR_PROCESS_ID_e;
    reset_manual_jump_input_state();
    g_iceArrowSelected = false;
    g_lensOfTruthActive = false;
    g_lensVisionActivated = false;
    g_iceArrowFrozenTargets.fill({});
    g_dekuNutBombIds.fill(fpcM_ERROR_PROCESS_ID_e);
    g_sitActionState = SitActionState::Inactive;
    g_sitMusicActiveTrack = 0;
    g_sitMusicAreaFadePending = false;
    g_sitMusicAreaFadeFrames = 0;
    g_sitMusicAreaFadeTimeoutReported = false;
    g_sitMusicLastStartedTrack = 0;
    g_sitMusicReportedOverrideTrack = 0;
    g_sitMusicLastLoggedSelection = -1;
    g_sitMusicConfigReadFailureReported = false;
    g_sitMusicAudioManagerUnavailableReported = false;
    g_sitMusicReportedFailureTrack = 0;
    g_sitMusicWaveFailureTrack = 0;
    g_secondaryItemInputHandled = false;
    g_jumpButtonConfigReadFailureReported = false;
    g_manualJumpHeightConfigReadFailureReported = false;
    g_glideItemConfigReadFailureReported = false;
    g_hoverPlaneLowerBindingConfigReadFailureReported = false;
    g_hoverPlaneLowerBindingWasHeld = false;
    g_secondaryRingInterpolationErrorLogged = false;

    ConfigVarDesc config = CONFIG_VAR_DESC_INIT;
    config.name = "mapOpenDirection";
    config.type = CONFIG_VAR_INT;
    config.default_int = kWheelOpensDown;
    ModResult result = svc_config->register_var(mod_ctx, &config, &g_wheelOpenDirection);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register secondary wheel direction setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "swordFlourishEnabled";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_swordFlourishEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register sword put-away flourish setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "secondaryWheelBindingEnabled";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = true;
    result = svc_config->register_var(mod_ctx, &config, &g_secondaryWheelBindingEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register secondary wheel binding setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "zItemSlotEnabled";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_zItemSlotEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Z item slot setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "darkLinkDamageMultiplier";
    config.type = CONFIG_VAR_INT;
    config.default_int = kDarkLinkDamageMultiplierDefaultPercent;
    result = svc_config->register_var(mod_ctx, &config, &g_darkLinkDamageMultiplier);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Dark Link damage multiplier setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "giantMaskDamageMultiplier";
    config.type = CONFIG_VAR_INT;
    config.default_int = kGiantMaskDamageMultiplierDefaultPercent;
    result = svc_config->register_var(mod_ctx, &config, &g_giantMaskDamageMultiplier);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Giant's Mask damage multiplier setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "manualJumpEnabled";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = true;
    result = svc_config->register_var(mod_ctx, &config, &g_manualJumpEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register manual jump setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "jumpButton";
    config.type = CONFIG_VAR_INT;
    config.default_int = kJumpButtonDefault;
    result = svc_config->register_var(mod_ctx, &config, &g_jumpButton);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register jump button setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "manualJumpHeightPercent";
    config.type = CONFIG_VAR_INT;
    config.default_int = kManualJumpDefaultHeightPercent;
    result = svc_config->register_var(mod_ctx, &config, &g_manualJumpHeightPercent);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register manual jump height setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "disableVanillaAutoJump";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_disableVanillaAutoJump);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register vanilla auto-jump setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "sprintEnabled";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_sprintEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register sprint setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "sprintSpeedPercent";
    config.type = CONFIG_VAR_INT;
    config.default_int = kSprintSpeedDefaultPercent;
    result = svc_config->register_var(mod_ctx, &config, &g_sprintSpeedPercent);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register sprint speed setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "wolfSprintEnabled";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_wolfSprintEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Wolf sprint setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "wolfSprintSpeedPercent";
    config.type = CONFIG_VAR_INT;
    config.default_int = kSprintSpeedDefaultPercent;
    result = svc_config->register_var(mod_ctx, &config, &g_wolfSprintSpeedPercent);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Wolf sprint speed setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "sprintStaminaEnabled";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_sprintStaminaEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register sprint stamina setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "sprintStaminaDrain";
    config.type = CONFIG_VAR_INT;
    config.default_int = kSprintStaminaDrainDefault;
    result = svc_config->register_var(mod_ctx, &config, &g_sprintStaminaDrain);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register sprint stamina drain setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "sprintStaminaRecovery";
    config.type = CONFIG_VAR_INT;
    config.default_int = kSprintStaminaRecoveryDefault;
    result = svc_config->register_var(mod_ctx, &config, &g_sprintStaminaRecovery);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register sprint stamina recovery setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "glideEnabled";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_glideEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register glide setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "glideItem";
    config.type = CONFIG_VAR_INT;
    config.default_int = kGlideItemDefault;
    result = svc_config->register_var(mod_ctx, &config, &g_glideItem);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register glide item setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "revalisGaleEnabled";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_revalisGaleEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Revali's Gale setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "galeHeightPercent";
    config.type = CONFIG_VAR_INT;
    config.default_int = kGaleHeightDefaultPercent;
    result = svc_config->register_var(mod_ctx, &config, &g_galeHeightPercent);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Gale height setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "galeChargeCapacity";
    config.type = CONFIG_VAR_INT;
    config.default_int = kGaleChargeCapacityDefault;
    result = svc_config->register_var(mod_ctx, &config, &g_galeChargeCapacity);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Gale charge capacity setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "galeRecoverySeconds";
    config.type = CONFIG_VAR_INT;
    config.default_int = kGaleRecoveryDefaultSeconds;
    result = svc_config->register_var(mod_ctx, &config, &g_galeRecoverySeconds);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Gale recovery setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "aimMode";
    config.type = CONFIG_VAR_INT;
    config.default_int = kAimModeDefault;
    result = svc_config->register_var(mod_ctx, &config, &g_aimMode);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register bow aim mode setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "aimMovementEnabled";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = true;
    result = svc_config->register_var(mod_ctx, &config, &g_aimMovementEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register bow aim movement setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "hawkeyeMidairBowAimEnabled";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(
        mod_ctx, &config, &g_hawkeyeMidairBowAimEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register midair Hawkeye bow aim setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "cinemaZoomPercent";
    config.type = CONFIG_VAR_INT;
    config.default_int = kCinemaZoomDefaultPercent;
    result = svc_config->register_var(mod_ctx, &config, &g_cinemaZoomPercent);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Cinema zoom setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "thirdPersonReticleOffsetY";
    config.type = CONFIG_VAR_INT;
    config.default_int = kThirdPersonReticleDefaultY;
    result = svc_config->register_var(mod_ctx, &config, &g_thirdPersonReticleOffsetY);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register third-person reticle offset setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "bulletTimeMode";
    config.type = CONFIG_VAR_INT;
    config.default_int = kBulletTimeModeDefault;
    result = svc_config->register_var(mod_ctx, &config, &g_bulletTimeMode);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Bullet Time setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "bulletTimeArrowDamageMultiplier";
    config.type = CONFIG_VAR_INT;
    config.default_int = kBulletTimeArrowDamageDefaultPercent;
    result = svc_config->register_var(
        mod_ctx, &config, &g_bulletTimeArrowDamageMultiplier);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Bullet Time arrow damage setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "flurryRushEnabled";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_flurryRushEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Flurry Rush setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "ootStyleHeartsEnabled";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_ootStyleHeartsEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register OOT hearts setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "enemySpawnerProfile";
    config.type = CONFIG_VAR_INT;
    config.default_int = 1;
    result = svc_config->register_var(mod_ctx, &config, &g_enemySpawnerProfile);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register enemy spawner profile setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "infiniteLavaWalk";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_infiniteLavaWalk);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register infinite lava walk setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "twilightWard";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_twilightWard);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Twilight Ward setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "hawkOneShotDamage";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_hawkOneShotDamage);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Hawk Grass damage setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "hoverBootsWaterWalking";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_hoverBootsWaterWalking);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Hover Boots water walking setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "hoverPlaneLowerBinding";
    config.type = CONFIG_VAR_INT;
    config.default_int = kHoverPlaneLowerDefaultBinding;
    result = svc_config->register_var(mod_ctx, &config, &g_hoverPlaneLowerBinding);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Hover Boots lower-plane binding setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "shieldSurfRBoost";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_shieldSurfRBoost);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Shield Surf R-button speed boost setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "iceLeafRBoost";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_iceLeafRBoost);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Ice Leaf R-button speed boost setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "clawshotUnevenSurfaceStick";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_clawshotUnevenSurfaceStick);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Clawshot uneven-surface setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "lanternIgnitesEnemies";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_lanternIgnitesEnemies);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Lantern enemy ignition setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "indoorMovementUnrestricted";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_indoorMovementUnrestricted);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register indoor movement setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "castleTownFirstPerson";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_castleTownFirstPerson);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Castle Town first-person setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "clawshotAnySurface";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_clawshotAnySurface);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Clawshot any-surface setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "rupeeMagnetEnabled";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_rupeeMagnetEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Rupee Magnet setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "rupeeMagnetClawshotOrbs";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_rupeeMagnetClawshotOrbs);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Rupee Magnet clawshot-orb setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "rupeeMagnetRadiusPercent";
    config.type = CONFIG_VAR_INT;
    config.default_int = kRupeeMagnetDefaultRadiusPercent;
    result = svc_config->register_var(mod_ctx, &config, &g_rupeeMagnetRadiusPercent);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Rupee Magnet radius setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "sitMusicTrack";
    config.type = CONFIG_VAR_INT;
    config.default_int = 0;
    result = svc_config->register_var(mod_ctx, &config, &g_sitMusicTrack);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Sit music setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "sitHealingEnabled";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_sitHealingEnabled);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Sit healing setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "disableSpinnerSurfaceBounce";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_disableSpinnerSurfaceBounce);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Spinner surface bounce setting");
        return result;
    }

    config = CONFIG_VAR_DESC_INIT;
    config.name = "spinnerAttackAlwaysActive";
    config.type = CONFIG_VAR_BOOL;
    config.default_bool = false;
    result = svc_config->register_var(mod_ctx, &config, &g_spinnerAttackAlwaysActive);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Spinner attack hitbox setting");
        return result;
    }

    int64_t sitMusicTrack = 0;
    result = svc_config->get_int(mod_ctx, g_sitMusicTrack, &sitMusicTrack);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to read Sit music setting");
        return result;
    }
    if (sitMusicTrack != 0) {
        svc_log->warn(mod_ctx, "reset legacy Sit music selection to File Select");
        result = svc_config->set_int(mod_ctx, g_sitMusicTrack, 0);
        if (result != MOD_OK) {
            svc_log->error(mod_ctx, "failed to reset invalid Sit music selection");
            return result;
        }
    }

    result = svc_save->observe_saves(
        mod_ctx, clear_secondary_items_for_new_save, restore_secondary_items_for_save,
        persist_secondary_items_for_save, nullptr, &g_secondarySaveObserver);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to observe game save lifecycle");
        return result;
    }
    restore_secondary_items_for_save(mod_ctx, 0, nullptr);

    result = register_fairy_companion_settings();
    if (result != MOD_OK) {
        set_mod_init_error(error, result, "Fairy Companion settings initialization failed");
        return result;
    }

    result = fire_arrows::initialize();
    if (result != MOD_OK) {
        set_mod_init_error(error, result, "Fire Arrows hook initialization failed");
        return result;
    }

    UiModsPanelDesc panel = UI_MODS_PANEL_DESC_INIT;
    panel.build = build_panel;
    result = svc_ui->register_mods_panel(mod_ctx, &panel);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register the secondary wheel settings panel");
        return result;
    }

    UiMenuTabDesc menuTab = UI_MENU_TAB_DESC_INIT;
    menuTab.label = "Robin's W&T";
    menuTab.on_selected = open_robin_settings_window;
    result = svc_ui->register_menu_tab(mod_ctx, &menuTab, &g_robinMenuTab);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Robin's W&T menu entry");
        return result;
    }

    menuTab.label = "Credits";
    menuTab.on_selected = open_combined_credits;
    result = svc_ui->register_menu_tab(mod_ctx, &menuTab, &g_creditsMenuTab);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register Credits menu entry");
        return result;
    }

    result = install_hooks(error);
    if (result != MOD_OK) {
        return result;
    }
    result = initialize_dark_link_visual_hooks(error);
    if (result != MOD_OK) {
        set_mod_init_error(error, result, "Dark Link visual hook initialization failed");
        return result;
    }
    result = coop_mod_initialize(error);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to initialize co-op runtime");
        coop_mod_shutdown(error);
        shutdown_dark_link_visual_hooks();
        return result;
    }
    initialize_fairy_companion_actor();
    dark_link_veil::initialize_dark_link_veil();
    svc_log->info(mod_ctx, "secondary inventory wheel initialized");
    (void)error;
    return MOD_OK;
}

MOD_EXPORT ModResult mod_update(ModError*) {
    if (mDoCPd_c::getTrigX(PAD_1) == 0 && mDoCPd_c::getTrigY(PAD_1) == 0) {
        g_secondaryItemInputHandled = false;
    }
    if (g_hoverBootsTransitionPending) {
        auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
        if (alink == nullptr || alink->mProcID != daAlink_c::PROC_BOOTS_EQUIP) {
            g_hoverBootsTransitionPending = false;
        }
    }
    dark_link_veil::update_dark_link_veil();
    update_sit_action();
    update_sit_music();
    update_goron_lava_contact_timer();
    update_hover_boots();
    recover_shield_surf_state(
        static_cast<daAlink_c*>(dComIfGp_getLinkPlayer()));
    update_hover_landing_recovery(
        static_cast<daAlink_c*>(dComIfGp_getLinkPlayer()));
    update_manual_cucco_glide(
        static_cast<daAlink_c*>(dComIfGp_getLinkPlayer()));
    manual_glider_visual::update(
        static_cast<daAlink_c*>(dComIfGp_getLinkPlayer()));
    update_hawk_grass_resource();
    update_ice_keese_resource();
    update_spawned_hawk_cleanup();
    update_lens_of_truth_poes();
    update_truth_mask_resource();
    update_fairy_companion();
    update_goron_mask();
    update_giant_mask();
    update_armos_hammer();
    fire_arrows::update();
    enemy_spawner::update();
    oot_hearts::update();
    bow_aim::update_bullet_time_for_player(
        static_cast<daAlink_c*>(dComIfGp_getLinkPlayer()));
    return coop_mod_update(nullptr);
}

MOD_EXPORT ModResult mod_shutdown(ModError*) {
    flurry_rush::shutdown();
    enemy_spawner::shutdown();
    oot_hearts::shutdown();
    bow_aim::stop_bow_bullet_time();
    shutdown_fairy_companion();
    g_revaliGaleVisual.release();
    g_revaliGaleVisualOwner = nullptr;
    JKR_DELETE(g_secondaryZHudLanternMeter);
    manual_glider_visual::shutdown();
    shutdown_dark_link_visual_hooks();
    dark_link_veil::shutdown_dark_link_veil();
    const ModResult coopResult = coop_mod_shutdown(nullptr);
    g_giantMaskActive = false;
    if (g_armosHammerActive) {
        auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
        if (alink != nullptr && !alink->checkWolf()) {
            set_armos_hammer_sword_visible(alink, true);
        } else {
            g_armosHammerSwordVisibility = {};
        }
        forget_armos_hammer_hand_matrix();
        g_armosHammerActive = false;
    }
    release_manual_cucco_glide(static_cast<daAlink_c*>(dComIfGp_getLinkPlayer()));
    if (g_secondarySaveObserver != 0) {
        const ModResult result =
            svc_save->unobserve_saves(mod_ctx, g_secondarySaveObserver);
        if (result != MOD_OK) {
            svc_log->error(mod_ctx, "failed to unregister game save observer");
        }
        g_secondarySaveObserver = 0;
    }
    g_darkLinkVisualActive = false;
    if (g_sitActionState == SitActionState::Seated) {
        auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
        if (alink != nullptr && !alink->checkEventRun() &&
            alink->mProcID == daAlink_c::PROC_WAIT)
        {
            restore_sit_shape_angles(alink);
            alink->mSinkShapeOffset = 0.0f;
            alink->setSingleAnimeBase(daAlink_c::ANM_WAIT);
        }
    }
    stop_sit_music_track();
    g_sitActionState = SitActionState::Inactive;
    if (g_robinSettingsWindow != 0) {
        svc_ui->window_close(mod_ctx, g_robinSettingsWindow);
        g_robinSettingsWindow = 0;
    }
    if (g_robinMenuTab != 0) {
        svc_ui->unregister_menu_tab(mod_ctx, g_robinMenuTab);
        g_robinMenuTab = 0;
    }
    if (g_creditsMenuTab != 0) {
        svc_ui->unregister_menu_tab(mod_ctx, g_creditsMenuTab);
        g_creditsMenuTab = 0;
    }
    if (g_hawkGrassPhaseRequested) {
        dComIfG_resDelete(&g_hawkGrassPhase, "J_Tobi");
    }
    if (g_iceKeeseHeap != nullptr) {
        mDoExt_destroyExpHeap(g_iceKeeseHeap);
        g_iceKeeseHeap = nullptr;
        g_iceKeeseModel = nullptr;
    }
    if (g_iceKeesePhaseRequested) {
        dComIfG_resDelete(&g_iceKeesePhase, "E_ib");
    }
    if (g_truthMaskHeap != nullptr) {
        mDoExt_destroyExpHeap(g_truthMaskHeap);
        g_truthMaskHeap = nullptr;
        g_truthMaskModel = nullptr;
    }
    if (g_truthMaskPhaseRequested) {
        dComIfG_resDelete(&g_truthMaskPhase, "N_gD_mskF");
    }
    if (g_goronMaskHeap != nullptr) {
        mDoExt_destroyExpHeap(g_goronMaskHeap);
        g_goronMaskHeap = nullptr;
        g_goronMaskModel = nullptr;
    }
    if (g_goronMaskPhaseRequested) {
        dComIfG_resDelete(&g_goronMaskPhase, "N_gD_mskT");
    }
    if (g_giantMaskHeap != nullptr) {
        mDoExt_destroyExpHeap(g_giantMaskHeap);
        g_giantMaskHeap = nullptr;
        g_giantMaskModel = nullptr;
    }
    if (g_giantMaskPhaseRequested) {
        dComIfG_resDelete(&g_giantMaskPhase, "N_gD_mskF");
    }
    if (g_goronMaskHudIconHeap != nullptr) {
        mDoExt_destroyExpHeap(g_goronMaskHudIconHeap);
        g_goronMaskHudIconHeap = nullptr;
        g_goronMaskHudIcon = nullptr;
    }
    g_goronMaskHudIconLoadFailed = false;
    g_goronMaskHudIconApplied = false;
    g_goronMaskPhase = {};
    g_goronMaskPhaseRequested = false;
    g_goronMaskArchiveLoaded = false;
    g_goronMaskLoadFailed = false;
    g_giantMaskPhase = {};
    g_giantMaskPhaseRequested = false;
    g_giantMaskArchiveLoaded = false;
    g_giantMaskLoadFailed = false;
    g_hawkGrassPhase = {};
    g_iceKeesePhase = {};
    g_hawkGrassPhaseRequested = false;
    g_hawkGrassArchiveLoaded = false;
    g_iceKeesePhaseRequested = false;
    g_iceKeeseArchiveLoaded = false;
    g_iceKeeseLoadFailed = false;
    g_pendingHawkGrassUse = nullptr;
    g_pendingHawkGrassButton = 0;
    forget_secondary_ring_pose(g_secondaryRing);
    g_secondaryRing = nullptr;
    g_openingSecondaryRing = false;
    g_secondaryHudOwner = nullptr;
    g_secondaryHudAssignmentsInitialized = false;
    g_secondaryHudRefreshPending.fill(false);
    set_goron_tunic_active(false);
    g_goronLavaContactFrames = 0;
    g_hoverBootsEquipped = false;
    g_darkLinkVisualActive = false;
    g_iceArrowSelected = false;
    g_lensOfTruthActive = false;
    g_truthMaskActive = false;
    g_goronMaskActive = false;
    g_goronMaskInputBlockFrames = 0;
    update_lens_of_truth_poes();
    g_iceArrowFrozenTargets.fill({});
    g_secondaryItemInputHandled = false;
    g_hoverGroundHeightValid = false;
    g_hoverLandingSequenceActive = false;
    g_hoverAtSurface = false;
    clear_hover_water_ground_plane();
    g_hoverLandingFrames = 0;
    g_hoverRippleEmitter = 0;
    stop_hover_magic_effects();
    g_primaryRingOpen = false;
    g_pendingPrimaryItemSync = false;
    g_deletingSecondaryRing = false;
    g_hoverBootsTransitionPending = false;
    clear_tunic_wheel_textures();
    g_keyWaitDepth = 0;
    g_mapInputDepth = 0;
    svc_log->info(mod_ctx, "secondary inventory wheel unloaded");
    return coopResult;
}
}

bool fairy_companion_midna_is_out() {
    const u8 appearance = maFairyCompanion_c::getSelectedAppearance();
    return maFairyCompanion_c::isCalled() &&
        (appearance == FAIRY_APPEARANCE_MIDNA ||
            appearance == FAIRY_APPEARANCE_MIDNA_REGULAR);
}

bool fairy_companion_midna_is_regular() {
    return maFairyCompanion_c::getSelectedAppearance() ==
        FAIRY_APPEARANCE_MIDNA_REGULAR;
}
