DEFINE_HOOK(&daAlink_c::procFallInit, DisableAutoJumpFallInit);
DEFINE_HOOK(&daAlink_c::procWolfFallInit, DisableAutoJumpWolfFallInit);
DEFINE_HOOK(&daNbomb_c::procExplodeInit, DeferGlideBomblingExplode);
DEFINE_HOOK(&daNbomb_c::procWait, GlideBomblingWait);

#if TARGET_PC
#include <SDL3/SDL_gamepad.h>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#endif

struct AutoJumpFlagScope {
    daAlink_c* link = nullptr;
    bool added = false;
    bool groundMovement = false;
    f32 forwardSpeed = 0.0f;
    s16 moveAngle = 0;
};

thread_local std::vector<AutoJumpFlagScope> g_autoJumpFlagScopes;

struct RunOffLedgeState {
    daAlink_c* owner = nullptr;
    u8 framesRemaining = 0;
};

constexpr u8 kRunOffLedgeAnimationFrames = 7;
RunOffLedgeState g_runOffLedgeState{};

bool disable_auto_jump_ground_movement_proc(daAlink_c* link) {
    switch (link->mProcID) {
    case daAlink_c::PROC_GRAB_WAIT:
        return !link->checkWolf() && link->checkGrabAnime() &&
            link->mGrabItemAcKeep.getActor() != nullptr;
    case daAlink_c::PROC_WAIT:
    case daAlink_c::PROC_MOVE:
    case daAlink_c::PROC_ATN_MOVE:
    case daAlink_c::PROC_ATN_ACTOR_WAIT:
    case daAlink_c::PROC_ATN_ACTOR_MOVE:
    case daAlink_c::PROC_WAIT_TURN:
    case daAlink_c::PROC_MOVE_TURN:
    case daAlink_c::PROC_WOLF_WAIT:
    case daAlink_c::PROC_WOLF_MOVE:
    case daAlink_c::PROC_WOLF_DASH:
    case daAlink_c::PROC_WOLF_WAIT_TURN:
    case daAlink_c::PROC_WOLF_ATN_AC_MOVE:
        return true;
    default:
        return false;
    }
}

HookAction before_check_auto_jump_action(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    g_autoJumpFlagScopes.push_back({link});
    if (!vanillaAutoJumpDisabled() || link == nullptr ||
        link->mMode == daPy_py_c::SMODE_WOLF_PUZZLE ||
        link->mProcID == daAlink_c::PROC_WOLF_GIANT_PUZZLE ||
        link->checkPlayerDemoMode() || link->checkEventRun() ||
        dComIfGp_event_runCheck() ||
        link->checkEndResetFlg0(daPy_py_c::ERFLG0_FORCE_AUTO_JUMP) ||
        link->checkEndResetFlg0(daPy_py_c::ERFLG0_NOT_AUTO_JUMP))
    {
        return HOOK_CONTINUE;
    }

    link->onEndResetFlg0(daPy_py_c::ERFLG0_NOT_AUTO_JUMP);
    AutoJumpFlagScope& scope = g_autoJumpFlagScopes.back();
    scope.added = true;
    scope.groundMovement = disable_auto_jump_ground_movement_proc(link);
    scope.forwardSpeed = link->speedF;
    scope.moveAngle = link->current.angle.y;
    return HOOK_CONTINUE;
}

void after_check_auto_jump_action(ModContext*, void*, void*, void*) {
    if (g_autoJumpFlagScopes.empty()) {
        return;
    }
    const AutoJumpFlagScope scope = g_autoJumpFlagScopes.back();
    g_autoJumpFlagScopes.pop_back();
    if (scope.added && scope.link != nullptr) {
        scope.link->mEndResetFlg0 &= ~daPy_py_c::ERFLG0_NOT_AUTO_JUMP;
        if (g_runOffLedgeState.owner == scope.link &&
            (scope.link->mProcID != daAlink_c::PROC_MOVE ||
             scope.link->mLinkAcch.ChkGroundHit()))
        {
            g_runOffLedgeState = {};
        }
    }
}

HookAction preserve_sprint_speed_on_ledge_fall(
    ModContext*, void* args, void* retval, void*)
{
    if (g_autoJumpFlagScopes.empty()) {
        return HOOK_CONTINUE;
    }
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const AutoJumpFlagScope& scope = g_autoJumpFlagScopes.back();
    if (link == nullptr || !scope.added || scope.link != link ||
        mods::arg<int>(args, 1) != 1 || link->mLinkAcch.ChkGroundHit() ||
        !disable_auto_jump_ground_movement_proc(link))
    {
        return HOOK_CONTINUE;
    }

    if (link->checkWolf() || is_hover_boots_hovering(link))
    {
        g_runOffLedgeState = {};
        mods::arg_ref<int>(args, 1) = 2;
        link->mNormalSpeed = scope.forwardSpeed;
        link->current.angle.y = scope.moveAngle;
        link->speed.y = 0.0f;
        return HOOK_CONTINUE;
    }
    if (link->mpHIO == nullptr || link->mProcID != daAlink_c::PROC_MOVE) {
        return HOOK_CONTINUE;
    }

    const f32 runSpeed = link->mpHIO->mMove.m.mMaxSpeed;
    if (runSpeed <= 0.0f || scope.forwardSpeed < runSpeed * 0.6f) {
        g_runOffLedgeState = {};
        return HOOK_CONTINUE;
    }

    if (g_runOffLedgeState.owner != link) {
        g_runOffLedgeState.owner = link;
        g_runOffLedgeState.framesRemaining = kRunOffLedgeAnimationFrames;
    }
    if (g_runOffLedgeState.framesRemaining > 0) {
        --g_runOffLedgeState.framesRemaining;
        const auto& jump = link->mpHIO->mAutoJump.m;
        link->setSpecialGravity(jump.mGravity, jump.mMaxFallSpeed, TRUE);
        link->speed.y = std::min(link->speed.y, -0.5f);
        if (retval != nullptr) *static_cast<int*>(retval) = 1;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

void finish_run_off_ledge_fall(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr || g_autoJumpFlagScopes.empty()) {
        return;
    }
    const AutoJumpFlagScope& scope = g_autoJumpFlagScopes.back();
    if (!scope.added || !scope.groundMovement || scope.link != link ||
        mods::arg<int>(args, 1) != 1 || link->checkWolf() ||
        is_hover_boots_hovering(link) || link->mLinkAcch.ChkGroundHit() ||
        link->mProcID != daAlink_c::PROC_FALL || link->mpHIO == nullptr)
    {
        return;
    }

    const bool running = g_runOffLedgeState.owner == link &&
        g_runOffLedgeState.framesRemaining == 0;
    if (running) {
        link->mNormalSpeed = scope.forwardSpeed;
        link->current.angle.y = scope.moveAngle;
        g_runOffLedgeState = {};
    }
    link->setSingleAnimeBaseSpeed(
        daAlink_c::ANM_DMG_FALL, 0.0f,
        link->mpHIO->mDamage.mDamFall.m.mFallAnmMorf);
    link->mProcVar2.field_0x300c = 1;
}

bool g_manualJumpPendingPress = false;
bool g_manualJumpLeftStickWasHeld = false;
bool g_manualJumpRightStickWasHeld = false;
bool g_manualJumpLeftStickTriggered = false;
bool g_manualJumpRightStickTriggered = false;
bool g_manualJumpLeftBumperWasHeld = false;
bool g_manualJumpLeftBumperTriggered = false;
bool g_droppingBomblingFromGlide = false;
bool g_warnedMissingNativeButtonPoller = false;

bool native_left_bumper_held() {
#if TARGET_PC
    using GetGamepadJoystick = SDL_Joystick* (*)(SDL_Gamepad*);
    using GetGamepadBindings = SDL_GamepadBinding** (*)(SDL_Gamepad*, int*);
    using GetJoystickButton = bool (*)(SDL_Joystick*, int);
    using FreeSdlMemory = void (*)(void*);
    struct SdlInputApi {
        GetGamepadJoystick getGamepadJoystick = nullptr;
        GetGamepadBindings getGamepadBindings = nullptr;
        GetJoystickButton getJoystickButton = nullptr;
        FreeSdlMemory freeMemory = nullptr;
    };
    static const SdlInputApi api = [] {
        SdlInputApi result;
#if defined(_WIN32)
        HMODULE module = GetModuleHandleA("SDL3.dll");
        if (module != nullptr) {
            result.getGamepadJoystick = reinterpret_cast<GetGamepadJoystick>(
                GetProcAddress(module, "SDL_GetGamepadJoystick"));
            result.getGamepadBindings = reinterpret_cast<GetGamepadBindings>(
                GetProcAddress(module, "SDL_GetGamepadBindings"));
            result.getJoystickButton = reinterpret_cast<GetJoystickButton>(
                GetProcAddress(module, "SDL_GetJoystickButton"));
            result.freeMemory = reinterpret_cast<FreeSdlMemory>(
                GetProcAddress(module, "SDL_free"));
        }
#else
        result.getGamepadJoystick = reinterpret_cast<GetGamepadJoystick>(
            dlsym(RTLD_DEFAULT, "SDL_GetGamepadJoystick"));
        result.getGamepadBindings = reinterpret_cast<GetGamepadBindings>(
            dlsym(RTLD_DEFAULT, "SDL_GetGamepadBindings"));
        result.getJoystickButton = reinterpret_cast<GetJoystickButton>(
            dlsym(RTLD_DEFAULT, "SDL_GetJoystickButton"));
        result.freeMemory = reinterpret_cast<FreeSdlMemory>(
            dlsym(RTLD_DEFAULT, "SDL_free"));
#endif
        return result;
    }();
    const s32 controllerIndex = PADGetIndexForPort(PAD_1);
    SDL_Gamepad* controller = controllerIndex >= 0
        ? PADGetSDLGamepadForIndex(static_cast<u32>(controllerIndex))
        : nullptr;
    if (controller == nullptr || api.getGamepadJoystick == nullptr ||
        api.getGamepadBindings == nullptr || api.getJoystickButton == nullptr ||
        api.freeMemory == nullptr)
    {
        if (!g_warnedMissingNativeButtonPoller) {
            svc_log->warn(mod_ctx, "could not poll the physical left-shoulder button");
            g_warnedMissingNativeButtonPoller = true;
        }
        return false;
    }

    static SDL_Gamepad* mappedController = nullptr;
    static SDL_Joystick* joystick = nullptr;
    static std::vector<int> shoulderButtons;
    if (controller != mappedController) {
        mappedController = controller;
        joystick = api.getGamepadJoystick(controller);
        shoulderButtons.clear();
        int bindingCount = 0;
        SDL_GamepadBinding** bindings =
            api.getGamepadBindings(controller, &bindingCount);
        if (bindings != nullptr) {
            for (int i = 0; i < bindingCount; ++i) {
                const SDL_GamepadBinding* binding = bindings[i];
                if (binding != nullptr &&
                    binding->input_type == SDL_GAMEPAD_BINDTYPE_BUTTON &&
                    binding->output_type == SDL_GAMEPAD_BINDTYPE_BUTTON &&
                    binding->output.button == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)
                {
                    shoulderButtons.push_back(binding->input.button);
                }
            }
            api.freeMemory(bindings);
        }
    }

    if (joystick == nullptr) {
        return false;
    }
    for (const int button : shoulderButtons) {
        if (api.getJoystickButton(joystick, button)) {
            return true;
        }
    }
#endif
    return false;
}

struct FallingBombling {
    daNbomb_c* actor = nullptr;
    u32 actorId = 0;
};

std::array<FallingBombling, 2> g_fallingBomblings{};
daAlink_c* g_manualJumpOwner = nullptr;
bool g_zSlotMidnaLeftTriggered = false;
daAlink_c* g_midnaZTriggerOwner = nullptr;
u8 g_midnaZTriggerPrevious = 0;

void reset_manual_jump_input_state() {
    g_manualJumpPendingPress = false;
    g_manualJumpLeftStickWasHeld = false;
    g_manualJumpRightStickWasHeld = false;
    g_manualJumpLeftStickTriggered = false;
    g_manualJumpRightStickTriggered = false;
    g_manualJumpLeftBumperWasHeld = false;
    g_manualJumpLeftBumperTriggered = false;
    g_manualJumpOwner = nullptr;
    bow_aim::clear_manual_jump(nullptr);
}

void capture_z_slot_midna_input(ModContext*, void*, void*, void*) {
    g_zSlotMidnaLeftTriggered =
        zItemSlotEnabled() && mDoCPd_c::getTrigLeft(PAD_1) != 0;
}

HookAction route_midna_to_dpad_left(ModContext*, void* args, void*, void*) {
    g_midnaZTriggerOwner = nullptr;
    g_midnaZTriggerPrevious = 0;
    if (!zItemSlotEnabled()) {
        return HOOK_CONTINUE;
    }

    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink == nullptr || !g_zSlotMidnaLeftTriggered) {
        return HOOK_SKIP_ORIGINAL;
    }

    g_midnaZTriggerOwner = alink;
    g_midnaZTriggerPrevious = alink->mItemTrigger & daAlink_c::BTN_Z;
    alink->mItemTrigger |= daAlink_c::BTN_Z;
    return HOOK_CONTINUE;
}

void restore_midna_z_trigger(ModContext*, void* args, void*, void*) {
    if (g_midnaZTriggerOwner == nullptr) {
        return;
    }

    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink == g_midnaZTriggerOwner && g_midnaZTriggerPrevious == 0) {
        alink->mItemTrigger &= static_cast<u8>(~daAlink_c::BTN_Z);
    }
    g_midnaZTriggerOwner = nullptr;
    g_midnaZTriggerPrevious = 0;
}

void update_manual_jump_button_input(ModContext*, void*, void*, void*) {
    const u32 extendedButtons = JUTGamePad::mPadStatus[PAD_1].extButton;
    const bool leftStickHeld = (extendedButtons & PAD_BUTTON_LEFT_STICK) != 0;
    const bool rightStickHeld = (extendedButtons & PAD_BUTTON_RIGHT_STICK) != 0;
    const bool rPressed = mDoCPd_c::getTrigLockR(PAD_1) != 0;
    const bool nativeLeftBumperHeld = native_left_bumper_held();
    const bool leftBumperHeld = nativeLeftBumperHeld;
    const bool leftBumperPressed =
        nativeLeftBumperHeld && !g_manualJumpLeftBumperWasHeld;
    bool documentVisible = false;
    if (svc_ui->is_any_document_visible(mod_ctx, &documentVisible) != MOD_OK) {
        if (!g_jumpInputVisibilityFailureReported) {
            svc_log->warn(mod_ctx, "could not check Dusklight UI visibility for manual jump input");
            g_jumpInputVisibilityFailureReported = true;
        }
        documentVisible = true;
    } else {
        g_jumpInputVisibilityFailureReported = false;
    }

    const bool blocked = dComIfGp_isPauseFlag() || documentVisible;
    if (blocked) {
        g_manualJumpPendingPress = false;
        g_manualJumpLeftStickWasHeld = leftStickHeld;
        g_manualJumpRightStickWasHeld = rightStickHeld;
        g_manualJumpLeftBumperWasHeld = leftBumperHeld;
        g_manualJumpLeftStickTriggered = false;
        g_manualJumpRightStickTriggered = false;
        g_manualJumpLeftBumperTriggered = false;
        return;
    }

    g_manualJumpLeftStickTriggered = leftStickHeld && !g_manualJumpLeftStickWasHeld;
    g_manualJumpRightStickTriggered = rightStickHeld && !g_manualJumpRightStickWasHeld;
    g_manualJumpLeftBumperTriggered = leftBumperPressed;
    bool selectedBindingPressed = false;
    switch (manualJumpBinding()) {
    case 0:
        selectedBindingPressed = rPressed;
        break;
    case 1:
        selectedBindingPressed = g_manualJumpLeftBumperTriggered;
        break;
    case 2:
        selectedBindingPressed = g_manualJumpRightStickTriggered;
        break;
    case 3:
        selectedBindingPressed = g_manualJumpLeftStickTriggered;
        break;
    default:
        break;
    }
    g_manualJumpPendingPress = g_manualJumpPendingPress || selectedBindingPressed;
    g_manualJumpLeftStickWasHeld = leftStickHeld;
    g_manualJumpRightStickWasHeld = rightStickHeld;
    g_manualJumpLeftBumperWasHeld = leftBumperHeld;
}

bool request_manual_glide(daAlink_c* alink) {
    if (alink == nullptr || !glideEnabled() || g_manualGlideRetiring ||
        g_manualGlideActorId != fpcM_ERROR_PROCESS_ID_e ||
        alink->mGrabItemAcKeep.getActor() != nullptr)
    {
        return false;
    }
    if (s_manualJumpActorService == nullptr) {
        svc_log->error(mod_ctx, "actor service is unavailable for the manual glide");
        return false;
    }

    ActorSpawnParams params{};
    params.parameters = 0;
    params.argument = -1;
    params.room_num = static_cast<int8_t>(fopAcM_GetRoomNo(alink));
    params.position = {
        alink->current.pos.x, alink->current.pos.y + 80.0f, alink->current.pos.z};
    params.angle = {-1, alink->shape_angle.y, 0};
    params.scale = {1.0f, 1.0f, 1.0f};
    ActorId actorId = fpcM_ERROR_PROCESS_ID_e;
    if (s_manualJumpActorService->create_actor(
            mod_ctx, fpcNm_NI_e, &params, &actorId) != MOD_OK ||
        actorId == fpcM_ERROR_PROCESS_ID_e)
    {
        svc_log->error(mod_ctx, "failed to create the glide Cucco actor");
        return false;
    }

    g_manualGlideActorId = static_cast<fpc_ProcID>(actorId);
    g_manualGlideLoadTicks = 0;
    return true;
}

void retire_manual_glide_actor() {
    if (g_manualGlideActorId == fpcM_ERROR_PROCESS_ID_e) {
        g_manualGlideRetiring = false;
        return;
    }

    auto* actor = fopAcM_SearchByID(g_manualGlideActorId);
    if (actor != nullptr) {
        fopAcM_delete(actor);
        g_manualGlideActorId = fpcM_ERROR_PROCESS_ID_e;
        g_manualGlideRetiring = false;
    } else if (fpcM_IsCreating(g_manualGlideActorId)) {
        g_manualGlideRetiring = true;
    } else {
        g_manualGlideActorId = fpcM_ERROR_PROCESS_ID_e;
        g_manualGlideRetiring = false;
    }
}

void release_manual_cucco_glide(daAlink_c* alink) {
    g_manualCuccoGlideActive = false;
    g_manualGlideLoadTicks = 0;
    auto* actor = g_manualGlideActorId == fpcM_ERROR_PROCESS_ID_e
        ? nullptr : fopAcM_SearchByID(g_manualGlideActorId);
    if (alink != nullptr && actor != nullptr &&
        alink->mGrabItemAcKeep.getActor() == actor)
    {
        alink->freeGrabItem();
    }
    if (alink != nullptr && alink->mpHIO != nullptr) {
        alink->mProcVar2.field_0x300c = 0;
        alink->setSpecialGravity(alink->mpHIO->mAutoJump.m.mGravity,
                                 alink->mpHIO->mAutoJump.m.mMaxFallSpeed, TRUE);
        alink->resetUpperAnime(daAlink_c::UPPER_1, 3.0f);
        alink->field_0x30a0 = 0;
        alink->onModeFlg(4);
    }
    retire_manual_glide_actor();
}

u8 item_button_mask(int button) {
    switch (button) {
    case 0:
        return daAlink_c::BTN_X;
    case 1:
        return daAlink_c::BTN_Y;
    default:
        return daAlink_c::BTN_Z;
    }
}

bool track_falling_bombling(daNbomb_c* bomb);
FallingBombling* falling_bombling(daNbomb_c* bomb);

bool drop_bomb_during_manual_glide(daAlink_c* alink, int button, u8 itemId) {
    if (alink == nullptr || !alink->checkBombItem(itemId) ||
        dComIfGp_getSelectItemNum(button) <= 0)
    {
        return false;
    }

    if ((itemId == dItemNo_NORMAL_BOMB_e || itemId == dItemNo_WATER_BOMB_e) &&
        alink->mActiveBombNum >= 3)
    {
        return false;
    }
    if (itemId == dItemNo_POKE_BOMB_e && alink->field_0x2fcf >= 2) {
        return false;
    }

    cXyz dropPosition = alink->current.pos;
    dropPosition.y -= 30.0f;
    fopAc_ac_c* bomb = nullptr;
    if (itemId == dItemNo_NORMAL_BOMB_e) {
        bomb = dBomb_c::createNormalBombPlayer(&dropPosition);
    } else if (itemId == dItemNo_WATER_BOMB_e) {
        bomb = dBomb_c::createWaterBombPlayer(&dropPosition);
    } else if (itemId == dItemNo_POKE_BOMB_e) {
        g_droppingBomblingFromGlide = true;
        bomb = dBomb_c::createInsectBombPlayer(&dropPosition);
        g_droppingBomblingFromGlide = false;
    }

    if (bomb == nullptr || fopAcM_GetName(bomb) != fpcNm_NBOMB_e) {
        svc_log->error(mod_ctx, "failed to drop a bomb from the manual glide");
        const u8 buttonMask = item_button_mask(button);
        alink->mItemTrigger &= static_cast<u8>(~buttonMask);
        alink->mItemButton &= static_cast<u8>(~buttonMask);
        g_secondaryItemInputHandled = true;
        return true;
    }

    bomb->speedF = 0.0f;
    bomb->speed.set(0.0f, 0.0f, 0.0f);
    if (itemId == dItemNo_POKE_BOMB_e) {
        auto* bombling = static_cast<daNbomb_c*>(bomb);
        const bool alreadyDeferred = falling_bombling(bombling) != nullptr;
        const bool tracked =
            alreadyDeferred || track_falling_bombling(bombling);
        bool waiting = alreadyDeferred;
        if (tracked && !alreadyDeferred) {
            bombling->offStateFlg0(daNbomb_c::FLG0_INSECT_BOMB);
            waiting = bombling->procWaitInit();
        }
        if (!tracked || !waiting) {
            svc_log->error(mod_ctx, "failed to initialize a falling Bombling");
        }
        ++alink->field_0x2fcf;
    } else {
        ++alink->mActiveBombNum;
    }
    alink->mSelectItemId = static_cast<u8>(button);
    dComIfGp_addSelectItemNum(button, -1);
    const u8 buttonMask = item_button_mask(button);
    alink->mItemTrigger &= static_cast<u8>(~buttonMask);
    alink->mItemButton &= static_cast<u8>(~buttonMask);
    g_secondaryItemInputHandled = true;
    return true;
}

bool track_falling_bombling(daNbomb_c* bomb) {
    if (bomb == nullptr) return false;
    const u32 actorId = fopAcM_GetID(bomb);
    for (FallingBombling& falling : g_fallingBomblings) {
        if (falling.actor == bomb && falling.actorId == actorId) return true;
        if (falling.actor != nullptr &&
            fopAcM_SearchByID(falling.actorId) != falling.actor)
        {
            falling = {};
        }
    }
    for (FallingBombling& falling : g_fallingBomblings) {
        if (falling.actor == nullptr) {
            falling = {bomb, actorId};
            return true;
        }
    }
    return false;
}

FallingBombling* falling_bombling(daNbomb_c* bomb) {
    if (bomb == nullptr) return nullptr;
    const u32 actorId = fopAcM_GetID(bomb);
    for (FallingBombling& falling : g_fallingBomblings) {
        if (falling.actor == bomb && falling.actorId == actorId) return &falling;
    }
    return nullptr;
}

HookAction defer_glide_bombling_explode(
    ModContext* context, void* args, void* retval, void*)
{
    auto* bomb = mods::arg<daNbomb_c*>(args, 0);
    if (bomb == nullptr ||
        fopAcM_GetParam(bomb) != dBomb_c::PRM_INSECT_BOMB_PLAYER)
    {
        return HOOK_CONTINUE;
    }

    const bool alreadyDeferred = falling_bombling(bomb) != nullptr;
    if (!g_droppingBomblingFromGlide && !alreadyDeferred) {
        return HOOK_CONTINUE;
    }
    if (!alreadyDeferred) {
        if (!track_falling_bombling(bomb)) {
            return HOOK_CONTINUE;
        }
        bomb->offStateFlg0(daNbomb_c::FLG0_INSECT_BOMB);
        if (!bomb->procWaitInit()) {
            for (FallingBombling& falling : g_fallingBomblings) {
                if (falling.actor == bomb && falling.actorId == fopAcM_GetID(bomb)) {
                    falling = {};
                    break;
                }
            }
            svc_log->error(context, "failed to initialize a falling Bombling");
            return HOOK_CONTINUE;
        }
    }
    if (retval != nullptr) *static_cast<BOOL*>(retval) = TRUE;
    return HOOK_SKIP_ORIGINAL;
}

void finish_glide_bombling_fall(ModContext*, void* args, void*, void*) {
    auto* bomb = mods::arg<daNbomb_c*>(args, 0);
    FallingBombling* falling = falling_bombling(bomb);
    if (falling == nullptr) return;

    if (bomb->checkStateFlg0(daNbomb_c::FLG0_DELETE_BOMB)) {
        *falling = {};
        return;
    }
    if (!bomb->mAcch.ChkGroundHit()) return;

    bomb->current.angle.y = bomb->shape_angle.y;
    bomb->field_0xbf0 = cXyz::BaseY;
    bomb->field_0xbfc.set(
        cM_ssin(bomb->shape_angle.y), 0.0f, cM_scos(bomb->shape_angle.y));
    bomb->field_0xc08.set(bomb->field_0xbfc.z, 0.0f, -bomb->field_0xbfc.x);
    mDoMtx_stack_c::transS(bomb->current.pos);
    mDoMtx_stack_c::YrotM(bomb->shape_angle.y);
    bomb->mpModel->setBaseTRMtx(mDoMtx_stack_c::get());
    mDoMtx_inverse(bomb->mpModel->getBaseTRMtx(), bomb->field_0xa70);

    cXyz start = bomb->current.pos + bomb->field_0xbf0 * 5.0f;
    cXyz end = bomb->current.pos - bomb->field_0xbf0 * 100.0f;
    bomb->mLineChk.Set(&start, &end, bomb);
    int lineResult = bomb->insectLineCheck();
    if (lineResult != 1) {
        start = bomb->current.pos - bomb->field_0xbf0 * 5.0f;
        end = start - bomb->field_0xbfc * 100.0f;
        bomb->mLineChk.Set(&start, &end, bomb);
        lineResult = bomb->insectLineCheck();
    }

    if (lineResult != 1) {
        *falling = {};
        bomb->procExplodeInit();
        return;
    }

    bomb->onStateFlg0(daNbomb_c::FLG0_INSECT_BOMB);
    const bool started = bomb->procInsectMoveInit();
    if (!started) {
        bomb->offStateFlg0(daNbomb_c::FLG0_INSECT_BOMB);
        svc_log->error(mod_ctx, "failed to start Bombling behavior after landing");
    }
    *falling = {};
}

void update_manual_cucco_glide(daAlink_c* alink) {
    if (g_manualGlideRetiring) {
        retire_manual_glide_actor();
        return;
    }
    if (g_manualGlideActorId == fpcM_ERROR_PROCESS_ID_e) {
        g_manualCuccoGlideActive = false;
        return;
    }

    if (alink == nullptr || alink->mpHIO == nullptr || !glideEnabled() ||
        alink->checkEventRun() || dComIfGp_event_runCheck() || alink->checkWolf() ||
        alink->mLinkAcch.ChkGroundHit() ||
        (alink->mProcID != daAlink_c::PROC_AUTO_JUMP &&
         alink->mProcID != daAlink_c::PROC_FALL))
    {
        release_manual_cucco_glide(alink);
        return;
    }

    auto* actor = fopAcM_SearchByID(g_manualGlideActorId);
    if (actor == nullptr) {
        if (++g_manualGlideLoadTicks > 90 || !fpcM_IsCreating(g_manualGlideActorId)) {
            svc_log->warn(mod_ctx, "glide Cucco did not finish loading; cancelling glide");
            release_manual_cucco_glide(alink);
        }
        return;
    }

    if (alink->mGrabItemAcKeep.getActor() != actor) {
        if (alink->mGrabItemAcKeep.getActor() != nullptr) {
            release_manual_cucco_glide(alink);
            return;
        }

        const f32 fallHeight = alink->mFallHeight;
        const f32 verticalSpeed = alink->speed.y;
        const f32 horizontalSpeed = alink->mNormalSpeed;
        alink->deleteEquipItem(TRUE, FALSE);
        if (alink->mProcID != daAlink_c::PROC_AUTO_JUMP) {
            alink->commonProcInit(daAlink_c::PROC_AUTO_JUMP);
        }
        alink->setGrabItemActor(actor);
        alink->field_0x33e4 = 38.0f;
        alink->setGrabUpperAnime(3.0f);
        alink->mMaxSpeed = alink->mpHIO->mAutoJump.m.mCuccoJumpMaxSpeed;
        alink->field_0x3478 = alink->mpHIO->mAutoJump.m.mCuccoFallMaxSpeed;
        alink->mProcVar0.field_0x3008 = 0;
        alink->mProcVar2.field_0x300c = 1;
        alink->field_0x3198 = 0x33;
        alink->field_0x2f99 = 0x20;
        alink->offModeFlg(4);
        alink->setSingleAnimeBaseSpeed(daAlink_c::ANM_JUMP_LAND, 0, 3.0f);
        alink->setUpperAnime(dRes_ID_ALANM_BCK_WALKHBS_e, daAlink_c::UPPER_1, 1, 0, -1, 3);
        alink->setSpecialGravity(-1.0f, alink->field_0x3478, FALSE);
        alink->speed.y = std::max(verticalSpeed, alink->field_0x3478);
        alink->mNormalSpeed = horizontalSpeed;
        alink->mFallHeight = fallHeight;
        actor->current.pos = alink->current.pos + cXyz(0, 80, 0);
        actor->old.pos = actor->current.pos;
        alink->setGrabItemPos();
        g_manualCuccoGlideActive = true;
    }
}

bool manual_jump_binding_triggered(int64_t binding, bool pendingPress) {
    switch (binding) {
    case 0:
        return pendingPress || mDoCPd_c::getTrigLockR(PAD_1) != 0;
    case 1:
        return pendingPress || g_manualJumpLeftBumperTriggered;
    case 2:
        return pendingPress || g_manualJumpRightStickTriggered;
    case 3:
        return pendingPress || g_manualJumpLeftStickTriggered;
    default:
        return false;
    }
}

bool manual_jump_binding_held(int64_t binding) {
    switch (binding) {
    case 0:
        return mDoCPd_c::getHoldLockR(PAD_1) != 0;
    case 1:
        return g_manualJumpLeftBumperWasHeld;
    case 2:
        return g_manualJumpRightStickWasHeld;
    case 3:
        return g_manualJumpLeftStickWasHeld;
    default:
        return false;
    }
}

int manual_jump_binding_button_mask(int64_t binding) {
    switch (binding) {
    case 0:
        return daAlink_c::BTN_R;
    default:
        return 0;
    }
}

bool manual_jump_r_action_context_active(daAlink_c* alink) {
    const u8 statuses[] = {
        dComIfGp_getDoStatus(), dComIfGp_getDoStatusForce(),
        dComIfGp_getRStatus(), dComIfGp_getRStatusForce()};
    for (const u8 status : statuses) {
        switch (status) {
        case BUTTON_STATUS_ENTER:
        case BUTTON_STATUS_GRAB:
        case BUTTON_STATUS_PULL_DOWN:
        case BUTTON_STATUS_PUSH:
        case BUTTON_STATUS_RESIST:
        case BUTTON_STATUS_STRIKE:
        case BUTTON_STATUS_PULL:
        case BUTTON_STATUS_HOLD_ON:
        case BUTTON_STATUS_UNK_123:
        case BUTTON_STATUS_UNK_150:
        case BUTTON_STATUS_UNK_153:
            return true;
        default:
            break;
        }
    }
    if (alink->mAttention != nullptr && alink->checkAttentionLock()) return true;
    if (alink->mTargetedActor != nullptr) return true;
    if (alink->checkFmChainGrabAnime()) return true;

    const u8 previousChainSlot = alink->field_0x2fa3;
    const bool chainAvailable = alink->searchFmChainPos() != 0;
    alink->field_0x2fa3 = previousChainSlot;
    if (chainAvailable) return true;

    alink->setFrontWallType();
    return alink->checkResetFlg0(daPy_py_c::RFLG0_UNK_8) != 0;
}

bool get_vanilla_standing_jump_height(daAlink_c* alink, f32* outHeight) {
    if (alink == nullptr || alink->mpHIO == nullptr || outHeight == nullptr) {
        return false;
    }
    const auto& jump = alink->mpHIO->mAutoJump.m;
    const f32 standingJumpSpeed =
        jump.mAlwaysMaxSpeedJump ? jump.mMaxJumpSpeed : jump.mMinJumpSpeed;
    const f32 verticalJumpSpeed =
        standingJumpSpeed * jump.mJumpSpeedRate * cM_ssin(jump.mJumpAngle);
    const f32 gravityMagnitude = std::abs(jump.mGravity);
    if (gravityMagnitude <= 0.0f || !std::isfinite(verticalJumpSpeed) ||
        verticalJumpSpeed <= 0.0f)
    {
        svc_log->warn(mod_ctx, "could not determine vanilla jump height for Hover Boots");
        return false;
    }

    *outHeight = verticalJumpSpeed * verticalJumpSpeed / (2.0f * gravityMagnitude);
    if (!std::isfinite(*outHeight) || *outHeight <= 0.0f) {
        svc_log->warn(mod_ctx, "could not determine vanilla jump height for Hover Boots");
        return false;
    }
    return true;
}

bool update_hover_plane_lowering(daAlink_c* alink, bool pendingJumpPress) {
    const bool held = hoverPlaneLowerBindingHeld(hoverPlaneLowerBinding());
    const bool pressed = held && !g_hoverPlaneLowerBindingWasHeld;
    g_hoverPlaneLowerBindingWasHeld = held;
    const bool shieldSurfing = g_shieldSurfActive && alink != nullptr &&
                               alink->checkBoardRide();
    if (!hoverBootsAirwalkingEnabled() || !g_hoverBootsEquipped ||
        !g_hoverGroundHeightValid || alink == nullptr ||
        (shieldSurfing && !g_shieldSurfHoverPlaneActive))
    {
        return false;
    }

    if ((!shieldSurfing && alink->mLinkAcch.ChkGroundHit()) || alink->checkEventRun() ||
        alink->checkModeFlg(daAlink_c::MODE_SWIMMING) ||
        alink->checkNoResetFlg0(daAlink_c::FLG0_WATER_IN_MOVE))
    {
        return false;
    }

    if (shieldSurfing && !pressed && !g_shieldSurfPlaneRaisePending &&
        alink->mProcID == daAlink_c::PROC_BOARD_JUMP && alink->speed.y > 0.0f)
    {
        return false;
    }

    if (shieldSurfing && g_shieldSurfPlaneRaisePending) {
        if (!pressed && alink->mProcID == daAlink_c::PROC_BOARD_JUMP &&
            alink->speed.y > 0.0f)
        {
            g_hoverGroundHeight = alink->current.pos.y;
            g_hoverGroundHeightValid = true;
            g_hoverAtSurface = false;
            g_hoverLandingSequenceActive = false;
            g_hoverLandingFrames = 0;
            return false;
        }
        g_shieldSurfPlaneRaisePending = false;
    }

    const int64_t jumpBinding = manualJumpBinding();
    const bool raisePressed =
        shieldSurfing && manualJumpEnabled() &&
        manual_jump_binding_triggered(jumpBinding, pendingJumpPress);
    if (!pressed && !raisePressed) {
        return false;
    }

    f32 vanillaJumpHeight = 0.0f;
    if (!get_vanilla_standing_jump_height(alink, &vanillaJumpHeight)) {
        return false;
    }

    if (raisePressed) {
        if (alink->mProcID == daAlink_c::PROC_BOARD_RIDE ||
            alink->mProcID == daAlink_c::PROC_BOARD_WAIT)
        {
            const int jumpStarted = alink->procBoardJumpInit(0.0f, FALSE);
            if (jumpStarted != 0) {
                g_shieldSurfPlaneRaisePending = true;
                g_hoverAtSurface = false;
                g_hoverLandingSequenceActive = false;
                g_hoverLandingFrames = 0;
                alink->mItemTrigger &= static_cast<u8>(
                    ~manual_jump_binding_button_mask(jumpBinding));
                return true;
            }
        } else if (alink->mProcID == daAlink_c::PROC_BOARD_JUMP &&
                   alink->speed.y > 0.0f)
        {
            g_hoverGroundHeight = alink->current.pos.y;
            g_shieldSurfPlaneRaisePending = true;
            alink->mItemTrigger &= static_cast<u8>(
                ~manual_jump_binding_button_mask(jumpBinding));
            return true;
        }
    }

    if (!pressed) {
        return false;
    }

    if (shieldSurfing) {
        if (alink->mProcID == daAlink_c::PROC_BOARD_RIDE ||
            alink->mProcID == daAlink_c::PROC_BOARD_WAIT)
        {
            alink->procBoardJumpInit(0.0f, TRUE);
        }
        alink->setSpecialGravity(
            alink->mpHIO->mAutoJump.m.mGravity,
            alink->mpHIO->mAutoJump.m.mMaxFallSpeed, TRUE);
        alink->speed.y = std::min(alink->speed.y, -0.5f);
        g_shieldSurfPlaneRaisePending = false;
        g_hoverGroundHeight -= vanillaJumpHeight;
        g_hoverGroundHeightValid = true;
        g_hoverLandingSequenceActive = false;
        g_hoverAtSurface = false;
        g_hoverLandingFrames = 0;
        return true;
    }

    if (g_hoverAtSurface) {
        alink->procFallInit(1, alink->mpHIO->mAutoJump.m.mFallInterpolation);
    }
    const auto& jump = alink->mpHIO->mAutoJump.m;
    alink->setSpecialGravity(jump.mGravity, jump.mMaxFallSpeed, TRUE);
    alink->speed.y = std::min(alink->speed.y, -0.5f);
    g_hoverGroundHeight -= vanillaJumpHeight;
    g_hoverLandingSequenceActive = false;
    g_hoverAtSurface = false;
    g_hoverLandingFrames = 0;
    return false;
}

bool manual_jump_ground_action_ready(const daAlink_c* alink) {
    if (alink == nullptr) {
        return false;
    }

    if (alink->checkWolf()) {
        return alink->mProcID == daAlink_c::PROC_WOLF_WAIT ||
               alink->mProcID == daAlink_c::PROC_WOLF_MOVE ||
               alink->mProcID == daAlink_c::PROC_WOLF_DASH ||
               alink->mProcID == daAlink_c::PROC_WOLF_WAIT_TURN ||
               alink->mProcID == daAlink_c::PROC_WOLF_ATN_AC_MOVE;
    }

    if (alink->mProcID == daAlink_c::PROC_GRAB_WAIT) {
        return alink->checkGrabAnime() &&
               alink->mGrabItemAcKeep.getActor() != nullptr;
    }

    return alink->mProcID == daAlink_c::PROC_WAIT ||
           alink->mProcID == daAlink_c::PROC_MOVE ||
           alink->mProcID == daAlink_c::PROC_ATN_MOVE ||
           alink->mProcID == daAlink_c::PROC_ATN_ACTOR_WAIT ||
           alink->mProcID == daAlink_c::PROC_ATN_ACTOR_MOVE ||
           alink->mProcID == daAlink_c::PROC_WAIT_TURN ||
           alink->mProcID == daAlink_c::PROC_MOVE_TURN;
}

bool manual_jump_ground_context_ready(
    daAlink_c* alink, bool hoverSurface = false, bool sprinting = false)
{
    if (alink == nullptr || !manual_jump_ground_action_ready(alink)) return false;

    auto* heldActor = alink->mGrabItemAcKeep.getActor();
    const bool humanCarrying = !alink->checkWolf() && alink->checkGrabAnime() &&
                               heldActor != nullptr;
    const bool wolfCarrying = alink->checkWolf() && alink->checkWolfGrabAnime() &&
                              heldActor != nullptr;
    const bool carrying = humanCarrying || wolfCarrying;
    return (hoverSurface || alink->mGndPolyAtt1 != 0xFF) &&
           !alink->checkFlyAtnWait() &&
           !alink->checkModeFlg(0x70C12) &&
           alink->mProcID != daAlink_c::PROC_DOOR_OPEN &&
           alink->mProcID != daAlink_c::PROC_WARP &&
           !alink->getSumouMode() && !alink->checkPlayerDemoMode() &&
           !alink->checkEventRun() && !dComIfGp_event_runCheck() &&
           (hoverSurface ||
            (!alink->checkMagneBootsFly() && !alink->checkMagneBootsOn())) &&
           !alink->checkNotJumpSinkLimit() &&
           (!alink->checkGrabAnime() || humanCarrying) &&
           (!alink->checkWolf() || !alink->checkWolfGrabAnime() || wolfCarrying) &&
           (heldActor == nullptr || carrying) &&
           (hoverSurface || alink->mLinkAcch.ChkGroundHit()) &&
           (!manual_jump_r_action_context_active(alink) || sprinting);
}

enum class RevaliGaleState : u8 {
    Idle,
    Airborne,
    Crouching,
};

void apply_manual_jump_movement(daAlink_c* alink);

struct RevaliGaleCharge {
    daAlink_c* owner = nullptr;
    RevaliGaleState state = RevaliGaleState::Idle;
    f32 launchSpeed = 0.0f;
    s16 launchAngle = 0;
    u32 crouchTicks = 0;
};

RevaliGaleCharge g_revaliGaleCharge{};
int64_t g_revaliGaleCharges = 0;
int64_t g_revaliGaleObservedCapacity = 0;
std::chrono::steady_clock::time_point g_revaliGaleNextRecovery{};

struct RevaliGaleVisual {
    JKRSolidHeap* heap = nullptr;
    J3DModel* model = nullptr;
    mDoExt_bckAnm motion;
    mDoExt_btkAnm texture;
    cXyz origin{0, 0, 0};
    int ticks = 0;
    bool readyLoop = false;
    bool failed = false;

    bool prepare() {
        if (model != nullptr) return true;
        if (failed) return false;
        auto* data = static_cast<J3DModelData*>(
            dComIfG_getObjectRes(daAlink_c::getAlinkArcName(), 0x34));
        auto* bck = static_cast<J3DAnmTransform*>(
            dComIfG_getObjectRes(daAlink_c::getAlinkArcName(), 0x13));
        auto* btk = static_cast<J3DAnmTextureSRTKey*>(
            dComIfG_getObjectRes(daAlink_c::getAlinkArcName(), 0x48));
        if (data == nullptr || bck == nullptr || btk == nullptr ||
            data->getJointNum() > 32)
        {
            return false;
        }
        heap = mDoExt_createSolidHeapFromGame(0x20000, 0x20);
        if (heap != nullptr) {
            auto* previous = mDoExt_setCurrentHeap(heap);
            model = mDoExt_J3DModel__create(data, 0, 0x11000284);
            const bool ready = model != nullptr &&
                motion.init(bck, TRUE, J3DFrameCtrl::EMode_LOOP, 1, 0, -1, false) &&
                texture.init(data, btk, TRUE, J3DFrameCtrl::EMode_LOOP, 1, 0, -1);
            mDoExt_setCurrentHeap(previous);
            if (ready) {
                mDoExt_adjustSolidHeap(heap);
                return true;
            }
            mDoExt_destroySolidHeap(heap);
            heap = nullptr;
            model = nullptr;
        }
        failed = true;
        if (svc_log != nullptr) {
            svc_log->warn(mod_ctx, "Revali's Gale: could not prepare the native wind model");
        }
        return false;
    }

    void launch(const cXyz& position) {
        stop_ready();
        origin = position;
        if (prepare()) {
            ticks = 60;
            motion.setFrame(0);
            texture.setFrame(0);
        }
    }

    void hold_ready(const cXyz& position) {
        origin = position;
        if (readyLoop || !prepare()) return;
        readyLoop = true;
        ticks = 0;
        motion.setFrame(0);
        texture.setFrame(0);
        Z2GetAudioMgr()->seStartLevel(
            Z2SE_BOOM_TORNADO, &origin, 0, 0, 1, 1, -1, -1, 0);
    }

    void stop_ready() {
        readyLoop = false;
    }

    void update() {
        if (!readyLoop && ticks <= 0) return;
        if (!readyLoop) --ticks;
        motion.play();
        texture.play();
        Z2GetAudioMgr()->seStartLevel(
            Z2SE_BOOM_TORNADO, &origin, 0, 0, 1, 1, -1, -1, 0);
    }

    void draw(daAlink_c* alink) {
        if (model == nullptr || (!readyLoop && ticks <= 0) || alink == nullptr) return;
        const f32 fade = readyLoop ? 1.0f : std::min(1.0f, ticks / 15.0f);
        const f32 height = readyLoop ? 0.1f : 2.5f;
        model->setBaseScale(cXyz(2.0f * fade, height * fade, 2.0f * fade));
        mDoMtx_stack_c::transS(
            origin.x, origin.y + 60.0f * height * fade, origin.z);
        model->setBaseTRMtx(mDoMtx_stack_c::get());
        auto* data = model->getModelData();
        J3DJointCallBack callbacks[32]{};
        for (u32 i = 0; i < data->getJointNum(); ++i) {
            auto* joint = data->getJointNodePointer(i);
            callbacks[i] = joint->getCallBack();
            joint->setCallBack(nullptr);
        }
        auto* root = data->getJointNodePointer(0);
        auto* previousCalc = root->getMtxCalc();
        const f32 previousBckFrame = motion.getBckAnm()->getFrame();
        const f32 previousBtkFrame = texture.getBtkAnm()->getFrame();
        motion.entry(data);
        texture.entry(data);
        g_env_light.setLightTevColorType_MAJI(model, &alink->tevStr);
        mDoExt_modelUpdateDL(model);
        root->setMtxCalc(previousCalc);
        motion.getBckAnm()->setFrame(previousBckFrame);
        texture.getBtkAnm()->setFrame(previousBtkFrame);
        for (u32 i = 0; i < data->getJointNum(); ++i) {
            data->getJointNodePointer(i)->setCallBack(callbacks[i]);
        }
    }

    void release() {
        stop_ready();
        ticks = 0;
        model = nullptr;
        if (heap != nullptr) mDoExt_destroySolidHeap(heap);
        heap = nullptr;
        failed = false;
    }
};

RevaliGaleVisual g_revaliGaleVisual{};
daAlink_c* g_revaliGaleVisualOwner = nullptr;

void update_revali_gale_visual(ModContext*, void* args, void*, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink != g_revaliGaleVisualOwner) {
        g_revaliGaleVisual.release();
        g_revaliGaleVisualOwner = alink;
    }
    if (alink == nullptr || alink->checkEventRun() ||
        dComIfGp_event_runCheck() || dComIfGp_isPauseFlag() ||
        dComIfGp_isEnableNextStage())
    {
        g_revaliGaleVisual.stop_ready();
        g_revaliGaleVisual.ticks = 0;
        return;
    }
    g_revaliGaleVisual.update();
}

void draw_revali_gale_visual(ModContext*, void* args, void*, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink == g_revaliGaleVisualOwner) {
        g_revaliGaleVisual.draw(alink);
    }
}

void update_revali_gale_charges() {
    const auto now = std::chrono::steady_clock::now();
    const int64_t capacity = galeChargeCapacity();
    if (g_revaliGaleObservedCapacity == 0) {
        g_revaliGaleCharges = capacity;
        g_revaliGaleObservedCapacity = capacity;
    } else if (capacity != g_revaliGaleObservedCapacity) {
        g_revaliGaleCharges = std::clamp(
            g_revaliGaleCharges + capacity - g_revaliGaleObservedCapacity,
            int64_t{0}, capacity);
        g_revaliGaleObservedCapacity = capacity;
    }

    if (g_revaliGaleCharges >= capacity) {
        g_revaliGaleNextRecovery = {};
        return;
    }
    if (g_revaliGaleNextRecovery == std::chrono::steady_clock::time_point{}) {
        g_revaliGaleNextRecovery = now +
            std::chrono::seconds(galeRecoverySeconds());
    }
    while (now >= g_revaliGaleNextRecovery && g_revaliGaleCharges < capacity) {
        ++g_revaliGaleCharges;
        g_revaliGaleNextRecovery += std::chrono::seconds(galeRecoverySeconds());
    }
    if (g_revaliGaleCharges >= capacity) {
        g_revaliGaleNextRecovery = {};
    }
}

void cancel_revali_gale(daAlink_c* alink, bool restorePose) {
    const bool wasCrouching = g_revaliGaleCharge.state == RevaliGaleState::Crouching;
    g_revaliGaleCharge = {};
    g_revaliGaleVisual.stop_ready();
    if (restorePose && wasCrouching && alink != nullptr &&
        alink->mProcID == daAlink_c::PROC_CROUCH)
    {
        alink->procWaitInit();
    }
}

bool launch_revali_gale(daAlink_c* alink, int buttonMask) {
    if (alink == nullptr || g_revaliGaleCharges <= 0 ||
        !alink->procAutoJumpInit(1))
    {
        cancel_revali_gale(alink, true);
        return false;
    }

    const cXyz origin = alink->current.pos;
    const f32 launchSpeed = g_revaliGaleCharge.launchSpeed;
    const s16 launchAngle = g_revaliGaleCharge.launchAngle;
    --g_revaliGaleCharges;
    g_revaliGaleNextRecovery = g_revaliGaleCharges < galeChargeCapacity()
        ? std::chrono::steady_clock::now() +
              std::chrono::seconds(galeRecoverySeconds())
        : std::chrono::steady_clock::time_point{};
    g_revaliGaleCharge = {};
    apply_manual_jump_movement(alink);
    alink->mNormalSpeed = launchSpeed;
    alink->speedF = launchSpeed;
    alink->mMaxSpeed = std::max(alink->mMaxSpeed, launchSpeed);
    alink->shape_angle.y = launchAngle;
    alink->current.angle.y = launchAngle;
    const f32 totalHeightPercent =
        static_cast<f32>(manualJumpHeightPercent() + galeHeightPercent());
    alink->speed.y *= std::sqrt(totalHeightPercent / 100.0f);
    alink->mItemTrigger &= static_cast<u8>(~buttonMask);
    g_manualJumpOwner = alink;
    bow_aim::mark_manual_jump_started(alink);
    g_revaliGaleVisual.launch(origin);
    return true;
}

bool update_revali_gale_charge(daAlink_c* alink) {
    constexpr u32 kGaleChargeReadyTicks = 30;
    update_revali_gale_charges();
    if (g_revaliGaleCharge.state == RevaliGaleState::Idle) {
        return false;
    }
    if (alink == nullptr || alink != g_revaliGaleCharge.owner ||
        !manualJumpEnabled() || !revalisGaleEnabled() || alink->checkWolf() ||
        alink->checkEventRun() || dComIfGp_event_runCheck() ||
        dComIfGp_isEnableNextStage() || dComIfGp_isPauseFlag())
    {
        cancel_revali_gale(alink, true);
        return false;
    }

    constexpr u32 kFaceButtons = PAD_BUTTON_A | PAD_BUTTON_B | PAD_BUTTON_X | PAD_BUTTON_Y;
    if ((mDoCPd_c::getTrig(PAD_1) & kFaceButtons) != 0) {
        cancel_revali_gale(alink, true);
        return false;
    }

    const bool held = manual_jump_binding_held(manualJumpBinding());
    if (g_revaliGaleCharge.state == RevaliGaleState::Airborne) {
        if (!held) {
            cancel_revali_gale(alink, false);
            return false;
        }
        const bool landed = alink->mLinkAcch.ChkGroundHit() &&
            (alink->mProcID == daAlink_c::PROC_AUTO_JUMP ||
                alink->mProcID == daAlink_c::PROC_FALL ||
                alink->mProcID == daAlink_c::PROC_LAND);
        if (!landed) {
            return false;
        }
        if (!alink->procCrouchInit()) {
            cancel_revali_gale(alink, false);
            return false;
        }
        g_revaliGaleCharge.state = RevaliGaleState::Crouching;
        g_revaliGaleCharge.crouchTicks = 0;
    }

    if (alink->mProcID != daAlink_c::PROC_CROUCH ||
        !alink->mLinkAcch.ChkGroundHit())
    {
        cancel_revali_gale(alink, true);
        return false;
    }

    if (g_revaliGaleCharge.crouchTicks < kGaleChargeReadyTicks) {
        ++g_revaliGaleCharge.crouchTicks;
    }
    if (g_revaliGaleCharge.crouchTicks >= kGaleChargeReadyTicks && held) {
        g_revaliGaleVisual.hold_ready(alink->current.pos);
    }
    if (!held) {
        if (g_revaliGaleCharge.crouchTicks >= kGaleChargeReadyTicks) {
            return launch_revali_gale(
                alink, manual_jump_binding_button_mask(manualJumpBinding()));
        }
        cancel_revali_gale(alink, true);
        return false;
    }

    alink->mNormalSpeed = 0.0f;
    alink->speedF = 0.0f;
    alink->speed.set(0.0f, 0.0f, 0.0f);
    alink->shape_angle.y = g_revaliGaleCharge.launchAngle;
    alink->current.angle.y = g_revaliGaleCharge.launchAngle;
    return true;
}

void set_manual_jump_direction(daAlink_c* alink) {
    if (alink->checkInputOnR()) alink->shape_angle.y = alink->mMoveAngle;
    alink->current.angle.y = alink->shape_angle.y;
}

void apply_manual_jump_movement(daAlink_c* alink) {
    if (!alink->checkInputOnR()) {
        alink->speedF = 0.0f;
        alink->mNormalSpeed = 0.0f;
    }
    alink->mLinkAcch.ClrGroundHit();
    alink->setJumpMode();
}

bool try_manual_jump(daAlink_c* alink, bool pendingJumpPress = false) {
    if (alink == nullptr || g_secondaryRing != nullptr || g_primaryRingOpen ||
        g_sitActionState != SitActionState::Inactive || g_shieldSurfActive ||
        alink->checkEventRun() || alink->checkRideOn())
    {
        return false;
    }
    if (g_manualJumpOwner == alink &&
        (!manualJumpEnabled() || alink->mLinkAcch.ChkGroundHit() ||
         (alink->mProcID != daAlink_c::PROC_AUTO_JUMP &&
          alink->mProcID != daAlink_c::PROC_FALL)))
    {
        g_manualJumpOwner = nullptr;
    }
    if (update_revali_gale_charge(alink)) {
        return true;
    }

    const int64_t binding = manualJumpBinding();
    if (!manualJumpEnabled() && !glideEnabled()) {
        return false;
    }
    const bool wolf = alink->checkWolf();
    const bool wolfGrounded = wolf && manual_jump_ground_action_ready(alink);

    const int buttonMask = manual_jump_binding_button_mask(binding);
    const bool triggered = manual_jump_binding_triggered(binding, pendingJumpPress);
    if (!triggered) {
        return false;
    }

    if (g_manualCuccoGlideActive) {
        return true;
    }

    const bool hovering = !wolf && is_hover_boots_hovering(alink);
    if (hovering && g_hoverLandingSequenceActive &&
        alink->mProcID == daAlink_c::PROC_LAND)
    {
        g_hoverLandingSequenceActive = false;
        g_hoverLandingFrames = 0;
        alink->procMoveInit();
    }

    if (!alink->mLinkAcch.ChkGroundHit() && !hovering && !wolfGrounded) {
        if (!glideEnabled()) {
            return false;
        }
        if (g_hoverBootsEquipped &&
            (!hoverBootsAirwalkingEnabled() ||
                alink->checkModeFlg(daAlink_c::MODE_SWIMMING) ||
                alink->checkNoResetFlg0(daAlink_c::FLG0_WATER_IN_MOVE)))
        {
            return false;
        }

        if (alink->mProcID != daAlink_c::PROC_FALL &&
            alink->mProcID != daAlink_c::PROC_AUTO_JUMP)
        {
            return false;
        }

        if (!request_manual_glide(alink)) {
            return false;
        }

        if (g_hoverBootsEquipped && g_hoverGroundHeightValid) {
            g_hoverGroundHeight = alink->current.pos.y;
            g_hoverLandingSequenceActive = false;
            g_hoverAtSurface = false;
            g_hoverLandingFrames = 0;
        }

        alink->mLinkAcch.ClrGroundHit();
        alink->mItemTrigger &= static_cast<u8>(~buttonMask);
        return true;
    }

    if (hovering && g_hoverLandingSequenceActive &&
        alink->mProcID == daAlink_c::PROC_LAND)
    {
        g_hoverLandingSequenceActive = false;
        g_hoverLandingFrames = 0;
        alink->procMoveInit();
    }

    const bool sprinting = !wolf && sprint_requested(alink);
    const bool hoverSurfaceJump = hovering && g_hoverBootsEquipped &&
        hoverBootsAirwalkingEnabled();
    if (!manualJumpEnabled() ||
        !manual_jump_ground_context_ready(alink, hoverSurfaceJump, sprinting))
    {
        return false;
    }

    set_manual_jump_direction(alink);
    constexpr int kSwordItem = 0x103;
    if (!wolf && alink->mGrabItemAcKeep.getActor() == nullptr &&
        alink->mEquipItem == kSwordItem &&
        (mDoCPd_c::getHoldB(PAD_1) || mDoCPd_c::getTrigB(PAD_1)))
    {
        if (!alink->procCutJumpInit(FALSE)) return false;
        apply_manual_jump_movement(alink);
        g_manualJumpOwner = nullptr;
        alink->mItemTrigger &= static_cast<u8>(~buttonMask);
        return true;
    }

    const f32 launchSpeed = std::max(0.0f, alink->mNormalSpeed);
    const s16 launchAngle = alink->shape_angle.y;
    const f32 sprintJumpMultiplier =
        wolf ? 1.0f : sprint_jump_speed_multiplier(alink);
    const int jumpStarted = wolf ? alink->procWolfAutoJumpInit(1)
                                 : alink->procAutoJumpInit(1);
    if (!jumpStarted) {
        return false;
    }

    apply_manual_jump_movement(alink);
    if (!wolf) {
        apply_sprint_jump_speed(alink, sprintJumpMultiplier);
    }
    const f32 heightScale = std::sqrt(
        static_cast<f32>(manualJumpHeightPercent()) / 100.0f);
    alink->speed.y *= heightScale;
    g_manualJumpOwner = wolf ? nullptr : alink;
    if (!wolf) {
        bow_aim::mark_manual_jump_started(alink);
    }
    if (!wolf && revalisGaleEnabled() && g_revaliGaleCharges > 0 &&
        manual_jump_binding_held(binding))
    {
        g_revaliGaleCharge.owner = alink;
        g_revaliGaleCharge.state = RevaliGaleState::Airborne;
        g_revaliGaleCharge.launchSpeed = launchSpeed;
        g_revaliGaleCharge.launchAngle = launchAngle;
    }
    if (hovering) {
        g_hoverLandingSequenceActive = false;
        g_hoverAtSurface = false;
        g_hoverLandingFrames = 0;
    }
    alink->mLinkAcch.ClrGroundHit();
    alink->mItemTrigger &= static_cast<u8>(~buttonMask);
    return true;
}

HookAction release_manual_cucco_glide_on_a(ModContext*, void* args, void* retval, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink == nullptr) return HOOK_CONTINUE;

    if (g_manualCuccoGlideActive && (alink->mItemTrigger & daAlink_c::BTN_A) != 0) {
        release_manual_cucco_glide(alink);
    }

    if (!manualJumpEnabled() || g_manualJumpOwner != alink ||
        g_manualCuccoGlideActive || alink->mProcID != daAlink_c::PROC_AUTO_JUMP ||
        !manual_jump_binding_held(manualJumpBinding()) ||
        !mDoCPd_c::getTrigB(PAD_1) || alink->mEquipItem != 0x103 ||
        alink->mGrabItemAcKeep.getActor() != nullptr)
    {
        return HOOK_CONTINUE;
    }

    set_manual_jump_direction(alink);
    if (!alink->procCutJumpInit(TRUE)) return HOOK_CONTINUE;
    apply_manual_jump_movement(alink);
    g_manualJumpOwner = nullptr;
    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

void update_secondary_assigned_item_input(ModContext*, void* args, void*, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink == nullptr) {
        return;
    }
    const bool pendingManualJumpPress = g_manualJumpPendingPress;
    g_manualJumpPendingPress = false;

    if (g_goronMaskInputBlockFrames > 0) {
        const u8 maskButtonMask = static_cast<u8>(
            (secondaryItemAssigned(0) &&
                     (secondaryItemIdForSlot(g_secondarySelectedItems[0]) ==
                          kGoronMaskItem ||
                      secondaryItemIdForSlot(g_secondarySelectedItems[0]) ==
                          kGiantMaskItem)
                 ? daAlink_c::BTN_X : 0) |
            (secondaryItemAssigned(1) &&
                     (secondaryItemIdForSlot(g_secondarySelectedItems[1]) ==
                          kGoronMaskItem ||
                      secondaryItemIdForSlot(g_secondarySelectedItems[1]) ==
                          kGiantMaskItem)
                 ? daAlink_c::BTN_Y : 0) |
            (secondaryItemAssigned(kZItemSelectIndex) &&
                     (secondaryItemIdForSlot(
                          g_secondarySelectedItems[kZItemSelectIndex]) ==
                          kGoronMaskItem ||
                      secondaryItemIdForSlot(
                          g_secondarySelectedItems[kZItemSelectIndex]) ==
                          kGiantMaskItem)
                 ? daAlink_c::BTN_Z : 0));
        if (maskButtonMask != 0) {
            alink->mItemTrigger &= static_cast<u8>(~maskButtonMask);
            alink->mItemButton &= static_cast<u8>(~maskButtonMask);
            g_secondaryItemInputHandled = true;
            return;
        }
    }

    if (alink->checkWolf() && g_pendingHawkGrassUse == alink) {
        g_pendingHawkGrassUse = nullptr;
        svc_log->warn(mod_ctx, "Cancelled pending Hawk Grass use after Link transformed");
    } else if (try_start_pending_hawk_grass_use(alink)) {
        g_secondaryItemInputHandled = true;
        return;
    }

    recover_shield_surf_state(alink);
    if (update_hover_plane_lowering(alink, pendingManualJumpPress)) {
        return;
    }

    if (try_manual_jump(alink, pendingManualJumpPress)) {
        return;
    }

    if (alink->checkWolf()) {
        return;
    }

    const bool xTriggered = mDoCPd_c::getTrigX(PAD_1) != 0;
    const bool yTriggered = mDoCPd_c::getTrigY(PAD_1) != 0;
    const bool zTriggered = zItemSlotEnabled() && mDoCPd_c::getTrigZ(PAD_1) != 0;
    if (!xTriggered && !yTriggered && !zTriggered) {
        return;
    }

    if (g_manualCuccoGlideActive &&
        alink->mProcID == daAlink_c::PROC_AUTO_JUMP)
    {
        for (int button = 0; button < kSecondaryAssignmentCount; ++button) {
            const bool triggered = button == 0 ? xTriggered
                : button == 1 ? yTriggered : zTriggered;
            if (!triggered) {
                continue;
            }
            const int itemId = secondaryItemAssigned(button)
                ? secondaryItemIdForSlot(g_secondarySelectedItems[button])
                : selected_item_for_button(button);
            if (alink->checkBowItem(itemId) || itemId == dItemNo_PACHINKO_e) {
                release_manual_cucco_glide(alink);
                if (g_manualJumpOwner != alink) {
                    g_manualJumpOwner = alink;
                    bow_aim::mark_manual_jump_started(alink);
                }
                break;
            }
        }

        if (g_manualCuccoGlideActive) {
            for (int button = 0; button < kSecondaryAssignmentCount; ++button) {
                const bool triggered = button == 0 ? xTriggered
                    : button == 1 ? yTriggered : zTriggered;
                if (!triggered) {
                    continue;
                }
                const int itemId = secondaryItemAssigned(button)
                    ? secondaryItemIdForSlot(g_secondarySelectedItems[button])
                    : selected_item_for_button(button);
                if (drop_bomb_during_manual_glide(
                        alink, button, static_cast<u8>(itemId)))
                {
                    return;
                }
            }
        }
    }

    const u8 pressedButtons = static_cast<u8>(
        (secondaryItemAssigned(0) && xTriggered
             ? daAlink_c::BTN_X : 0) |
        (secondaryItemAssigned(1) && yTriggered
             ? daAlink_c::BTN_Y : 0) |
        (secondaryItemAssigned(kZItemSelectIndex) && zTriggered
             ? daAlink_c::BTN_Z : 0));
    alink->mItemTrigger &= static_cast<u8>(~pressedButtons);
    alink->mItemButton &= static_cast<u8>(~pressedButtons);

    if (g_secondaryRing != nullptr || g_primaryRingOpen || alink->checkEventRun()) {
        g_secondaryItemInputHandled = true;
        return;
    }

    if (pressedButtons == 0) {
        if (zTriggered && !secondaryItemAssigned(kZItemSelectIndex) &&
            selected_item_for_button(kZItemSelectIndex) != dItemNo_NONE_e)
        {
            alink->mSelectItemId = kZItemSelectIndex;
        }
        return;
    }

    if (g_secondaryItemInputHandled) {
        return;
    }

    const int button = (pressedButtons & daAlink_c::BTN_X) != 0 ? 0
        : (pressedButtons & daAlink_c::BTN_Y) != 0 ? 1 : kZItemSelectIndex;
    const u8 itemId = secondaryItemIdForSlot(g_secondarySelectedItems[button]);
    g_secondaryItemInputHandled = true;
    switch (itemId) {
    case kSitItemId:
        if (g_sitActionState == SitActionState::Inactive) {
            request_sit_action(alink);
        } else if (g_sitActionState == SitActionState::Seated) {
            begin_stand_action(alink);
        }
        break;
    case kGoronTunicItemId:
        if (g_goronTunicEquipped) {
            set_goron_tunic_active(false);
            svc_log->info(mod_ctx, "Goron Tunic unequipped");
        } else if (!goron_tunic_can_activate(alink)) {
            if (alink != nullptr &&
                dComIfGs_getSelectEquipClothes() != dItemNo_WEAR_KOKIRI_e)
            {
                alink->seStartSystem(Z2SE_SYS_ERROR);
            }
            svc_log->warn(mod_ctx, "Goron Tunic requires the Hero's Tunic");
        } else {
            set_goron_tunic_active(true);
            svc_log->info(mod_ctx, "Goron Tunic equipped");
        }
        break;
    case kShieldSurfItemId:
        request_shield_surf_toggle(alink);
        break;
    case kHawkGrassItem:
        begin_secondary_hawk_grass_use(alink, static_cast<u8>(button));
        break;
    case kHoverBootsItem:
        begin_hover_boots_toggle(alink);
        break;
    case kDarkLinkItem:
        g_darkLinkVisualActive = !g_darkLinkVisualActive;
        svc_log->info(mod_ctx, g_darkLinkVisualActive ? "Dawnlight Dark Link activated" :
                                                       "Dawnlight Dark Link deactivated");
        break;
    case kLensOfTruthItem:
        if (alink->checkWolf()) {
            svc_log->warn(mod_ctx, "Lens of Truth is only available to human Link");
            break;
        }
        g_lensOfTruthActive = !g_lensOfTruthActive;
        svc_log->info(
            mod_ctx, g_lensOfTruthActive ? "Lens of Truth activated" :
                                          "Lens of Truth deactivated");
        break;
    case kTruthMaskItem:
        if (alink->checkWolf()) {
            svc_log->warn(mod_ctx, "Stone Mask is only available to human Link");
            break;
        }
        g_truthMaskActive = !g_truthMaskActive;
        svc_log->info(mod_ctx, g_truthMaskActive ? "Stone Mask equipped" :
                                                   "Stone Mask removed");
        break;
    case kGoronMaskItem:
        if (alink->checkWolf()) {
            svc_log->warn(mod_ctx, "Goron Mask is only available to human Link");
            break;
        }
        g_goronMaskActive = !g_goronMaskActive;
        svc_log->info(mod_ctx, g_goronMaskActive ? "Goron Mask equipped" :
                                                   "Goron Mask removed");
        break;
    case kGiantMaskItem:
        if (alink->checkWolf()) {
            svc_log->warn(mod_ctx, "Giant's Mask is only available to human Link");
            break;
        }
        g_giantMaskActive = !g_giantMaskActive;
        svc_log->info(mod_ctx, g_giantMaskActive ? "Giant's Mask equipped" :
                                                   "Giant's Mask removed");
        break;
    case kArmosHammerItem:
        if (alink->checkWolf()) {
            svc_log->warn(mod_ctx, "Armos Hammer is only available to human Link");
            break;
        }
        if (!g_armosHammerActive && !alink->checkSwordGet()) {
            svc_log->warn(mod_ctx, "Armos Hammer requires Link's sword to be available");
            break;
        }
        g_armosHammerActive = !g_armosHammerActive;
        if (g_armosHammerActive) {
            alink->setSwordModel();
            set_armos_hammer_sword_visible(alink, false);
        } else {
            set_armos_hammer_sword_visible(alink, true);
            forget_armos_hammer_hand_matrix();
        }
        svc_log->info(mod_ctx, g_armosHammerActive ? "Armos Hammer equipped" :
                                                     "Armos Hammer unequipped");
        break;
    case kDekuNutItem:
        begin_deku_nut_use(alink);
        break;
    default:
        break;
    }
}

void tint_goron_tunic_hud_icon(ModContext*, void* args, void*, void*) {
    if (g_secondaryIconContext != SecondaryIconContext::GoronTunic) {
        return;
    }

    const int button = mods::arg<int>(args, 1);
    if (button < 0 || button > 1) {
        return;
    }
    auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
    tint_tunic_icon(meter->mpItemXY[button]);
    tint_tunic_icon(meter->mpItemXYPane[button]);
}
