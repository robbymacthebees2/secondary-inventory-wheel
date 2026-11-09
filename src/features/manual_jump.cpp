bool g_manualJumpPendingR = false;
bool g_manualJumpLeftStickWasHeld = false;
bool g_manualJumpRightStickWasHeld = false;
bool g_manualJumpLeftStickTriggered = false;
bool g_manualJumpRightStickTriggered = false;
bool g_manualJumpLeftBumperWasHeld = false;
bool g_manualJumpLeftBumperTriggered = false;
bool g_manualJumpUiVisibilityFailureReported = false;

void reset_manual_jump_input_state() {
    g_manualJumpPendingR = false;
    g_manualJumpLeftStickWasHeld = false;
    g_manualJumpRightStickWasHeld = false;
    g_manualJumpLeftStickTriggered = false;
    g_manualJumpRightStickTriggered = false;
    g_manualJumpLeftBumperWasHeld = false;
    g_manualJumpLeftBumperTriggered = false;
}

bool manual_jump_l1_ui_blocked() {
    bool documentVisible = false;
    if (svc_ui->is_any_document_visible(mod_ctx, &documentVisible) != MOD_OK) {
        if (!g_manualJumpUiVisibilityFailureReported) {
            svc_log->warn(mod_ctx, "could not check Dusklight UI visibility for L1 manual jump");
            g_manualJumpUiVisibilityFailureReported = true;
        }
        return true;
    }

    g_manualJumpUiVisibilityFailureReported = false;
    return documentVisible;
}

bool start_manual_cucco_glide(daAlink_c* alink) {
    if (alink->mProcID == daAlink_c::PROC_FALL) {
        if (!alink->procAutoJumpInit(1)) {
            return false;
        }
        alink->speed.y = 0.0f;
    } else if (alink->mProcID != daAlink_c::PROC_AUTO_JUMP) {
        return false;
    }

    auto* cucco = alink->mGrabItemAcKeep.getActor();
    const bool isGoldCucco =
        cucco != nullptr && fopAcM_GetName(cucco) == fpcNm_NI_e &&
        static_cast<ni_class*>(cucco)->checkGold();
    if (isGoldCucco) {
        alink->mMaxSpeed = 30.0f;
        alink->field_0x3478 = -10.0f;
    } else if (alink->checkStageName("F_SP115")) {
        alink->mMaxSpeed = 27.0f;
        alink->field_0x3478 = -10.0f;
    } else {
        alink->mMaxSpeed = alink->mpHIO->mAutoJump.m.mCuccoJumpMaxSpeed;
        alink->field_0x3478 = alink->mpHIO->mAutoJump.m.mCuccoFallMaxSpeed;
    }

    alink->mProcVar2.field_0x300c = 1;
    alink->field_0x30a0 = 0x1800;
    alink->setSpecialGravity(-1.0f, alink->field_0x3478, FALSE);
    return true;
}

void restore_manual_glide_bow_slow_time();

void clear_manual_glide_bow_aim(daAlink_c* alink) {
    if (!g_manualGlideBowAiming) {
        return;
    }

    if (alink != nullptr) {
        dComIfGp_offCameraAttentionStatus(alink->field_0x317c, 0x10);
        alink->mBodyAngle.y = g_manualGlideBowBodyAngleY;
    }
    dComIfGp_clearPlayerStatus0(0, 0x2000);
    g_manualGlideBowAiming = false;
    restore_manual_glide_bow_slow_time();
    if (alink != nullptr) {
        alink->setBowSight();
    }
}

void release_manual_cucco_glide(daAlink_c* alink) {
    g_manualCuccoGlideActive = false;
    clear_manual_glide_bow_aim(alink);
    alink->mProcVar2.field_0x300c = 0;
    alink->setSpecialGravity(alink->mpHIO->mAutoJump.m.mGravity,
                             alink->mpHIO->mAutoJump.m.mMaxFallSpeed, TRUE);
    alink->resetUpperAnime(daAlink_c::UPPER_1, 3.0f);
    alink->field_0x30a0 = 0;
    alink->onModeFlg(4);
}

HookAction release_manual_cucco_glide_on_a(
    ModContext*, void* args, void*, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (g_manualCuccoGlideActive && alink != nullptr &&
        alink == dComIfGp_getLinkPlayer() &&
        (alink->mItemTrigger & daAlink_c::BTN_A) != 0)
    {
        alink->mItemTrigger &= static_cast<u8>(~daAlink_c::BTN_A);
        release_manual_cucco_glide(alink);
    }
    return HOOK_CONTINUE;
}

HookAction emulate_manual_cucco_glide_check(
    ModContext*, void* args, void* retval, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (g_manualCuccoGlideActive && alink != nullptr &&
        alink == dComIfGp_getLinkPlayer() &&
        alink->mProcID == daAlink_c::PROC_AUTO_JUMP &&
        !alink->mLinkAcch.ChkGroundHit())
    {
        *static_cast<BOOL*>(retval) = TRUE;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

HookAction suspend_manual_glide_bow_movement_input(
    ModContext*, void* args, void*, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (g_manualGlideBowAiming && alink != nullptr &&
        alink == dComIfGp_getLinkPlayer())
    {
        g_manualGlideBowMoveValue = alink->mMoveValue;
        alink->mMoveValue = 0.0f;
    }
    return HOOK_CONTINUE;
}

void update_manual_glide_bow_aim(ModContext*, void* args, void*, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink == nullptr || alink != dComIfGp_getLinkPlayer()) {
        return;
    }

    if (g_manualGlideBowAiming) {
        alink->mMoveValue = g_manualGlideBowMoveValue;
    }

    const bool aimingBow = g_manualCuccoGlideActive &&
        alink->mProcID == daAlink_c::PROC_AUTO_JUMP &&
        alink->checkBowAndSlingItem(alink->mEquipItem) &&
        alink->checkBowAnime();
    if (!aimingBow) {
        clear_manual_glide_bow_aim(alink);
        return;
    }

    if (!g_manualGlideBowAiming) {
        g_manualGlideBowBodyAngleY = alink->mBodyAngle.y;
        g_manualGlideBowAiming = true;
        dComIfGp_onCameraAttentionStatus(alink->field_0x317c, 0x10);
        alink->setSubjectMode();
    }

    const s16 facingAngle = alink->shape_angle.y;
    alink->shape_angle.y = alink->field_0x310c;
    alink->mBodyAngle.y = g_manualGlideBowBodyAngleY;
    alink->setBodyAngleToCamera();
    const s16 aimingAngle = alink->shape_angle.y;
    alink->shape_angle.y = facingAngle;
    alink->mBodyAngle.y = static_cast<s16>(aimingAngle - facingAngle);
    alink->field_0x310c = aimingAngle;
    alink->setBowSight();
}

HookAction enable_manual_glide_arrow_aim(
    ModContext*, void*, void*, void*) {
    auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    if (g_manualGlideBowAiming && alink != nullptr &&
        !dComIfGp_checkPlayerStatus0(0, 0x200000))
    {
        dComIfGp_setPlayerStatus0(0, 0x200000);
        g_manualGlideArrowAimStatusAdded = true;
    }
    return HOOK_CONTINUE;
}

void restore_manual_glide_arrow_aim(ModContext*, void*, void*, void*) {
    if (g_manualGlideArrowAimStatusAdded) {
        dComIfGp_clearPlayerStatus0(0, 0x200000);
        g_manualGlideArrowAimStatusAdded = false;
    }
}

HookAction allow_manual_glide_bow_pitch(
    ModContext*, void*, void* retval, void*) {
    if (g_manualGlideBowAiming) {
        *static_cast<BOOL*>(retval) = FALSE;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

HookAction preserve_manual_glide_bow_pitch(
    ModContext*, void* args, void*, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (g_manualGlideBowAiming && alink != nullptr &&
        alink == dComIfGp_getLinkPlayer() &&
        alink->mProcID == daAlink_c::PROC_AUTO_JUMP)
    {
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

#if TARGET_PC
HookAction enable_manual_glide_bow_aim_context(
    ModContext*, void*, void* retval, void*) {
    if (g_manualGlideBowAiming) {
        *static_cast<bool*>(retval) = true;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}
#endif

void update_manual_cucco_glide(daAlink_c* alink) {
    if (g_manualCuccoGlideActive &&
        manualJumpBinding() == kManualJumpDisabled &&
        alink != nullptr && !alink->checkEventRun() && !alink->checkWolf() &&
        alink->mProcID == daAlink_c::PROC_AUTO_JUMP)
    {
        release_manual_cucco_glide(alink);
    }

    if (g_manualCuccoGlideActive &&
        (alink == nullptr || alink->checkEventRun() || alink->checkWolf() ||
         alink->mLinkAcch.ChkGroundHit() ||
         alink->mProcID != daAlink_c::PROC_AUTO_JUMP ||
         alink->mProcVar2.field_0x300c == 0))
    {
        g_manualCuccoGlideActive = false;
        clear_manual_glide_bow_aim(alink);
    }
}

void update_manual_glide_bow_slow_time() {
    if (!g_manualCuccoGlideActive || !g_manualGlideBowAiming) {
        restore_manual_glide_bow_slow_time();
        return;
    }

    const int64_t slowdownPercent = glideBowSlowTimePercent();
    if (slowdownPercent == 0) {
        restore_manual_glide_bow_slow_time();
        return;
    }

    if (!g_manualGlideSlowTimeScaleActive) {
        g_manualGlidePreviousTimeScale = aurora_get_timescale();
        g_manualGlideSlowTimeScaleActive = true;
    }

    const f32 timeScale =
        g_manualGlidePreviousTimeScale *
        (1.0f - static_cast<f32>(slowdownPercent) / 100.0f);
    aurora_set_timescale(timeScale);
}

void restore_manual_glide_bow_slow_time() {
    if (g_manualGlideSlowTimeScaleActive) {
        aurora_set_timescale(g_manualGlidePreviousTimeScale);
        g_manualGlideSlowTimeScaleActive = false;
    }
}

bool manual_jump_binding_triggered(int64_t binding, bool pendingRPress) {
    switch (binding) {
    case 1:
        return mDoCPd_c::getTrigA(PAD_1) != 0;
    case 2:
        return mDoCPd_c::getTrigB(PAD_1) != 0;
    case 3:
        return g_manualJumpLeftBumperTriggered && !manual_jump_l1_ui_blocked();
    case 4:
        return pendingRPress || mDoCPd_c::getTrigLockR(PAD_1) != 0;
    case 5:
        return mDoCPd_c::getTrigZ(PAD_1) != 0;
    case 6:
        return g_manualJumpLeftStickTriggered;
    case 7:
        return g_manualJumpRightStickTriggered;
    case 8:
        return mDoCPd_c::getTrigL(PAD_1) != 0;
    default:
        return false;
    }
}

int manual_jump_binding_button_mask(int64_t binding) {
    switch (binding) {
    case 1:
        return daAlink_c::BTN_A;
    case 2:
        return daAlink_c::BTN_B;
    case 4:
        return daAlink_c::BTN_R;
    case 5:
        return daAlink_c::BTN_Z;
    case 8:
        return daAlink_c::BTN_L;
    default:
        return 0;
    }
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

bool update_hover_plane_lowering(daAlink_c* alink, bool pendingRPress) {
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
        shieldSurfing && jumpBinding != kManualJumpDisabled &&
        manual_jump_binding_triggered(jumpBinding, pendingRPress);
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
               alink->mProcID == daAlink_c::PROC_WOLF_WAIT_SLIP ||
               alink->mProcID == daAlink_c::PROC_WOLF_WAIT_TURN ||
               alink->mProcID == daAlink_c::PROC_WOLF_TIRED_WAIT;
    }

    return alink->mProcID == daAlink_c::PROC_WAIT ||
           alink->mProcID == daAlink_c::PROC_MOVE;
}

bool try_manual_jump(daAlink_c* alink, bool pendingRPress = false) {
    if (alink == nullptr || g_secondaryRing != nullptr || g_primaryRingOpen ||
        g_sitActionState != SitActionState::Inactive || g_shieldSurfActive ||
        alink->checkEventRun() || alink->checkRideOn())
    {
        return false;
    }
    const int64_t binding = manualJumpBinding();
    if (binding == kManualJumpDisabled) {
        return false;
    }
    const bool wolf = alink->checkWolf();
    const bool wolfGrounded = wolf && manual_jump_ground_action_ready(alink);

    int buttonMask = 0;
    bool triggered = false;
    switch (binding) {
    case 1:
        buttonMask = daAlink_c::BTN_A;
        triggered = mDoCPd_c::getTrigA(PAD_1) != 0;
        break;
    case 2:
        buttonMask = daAlink_c::BTN_B;
        triggered = mDoCPd_c::getTrigB(PAD_1) != 0;
        break;
    case 3:
        buttonMask = daAlink_c::BTN_L;
        triggered = g_manualJumpLeftBumperTriggered && !manual_jump_l1_ui_blocked();
        break;
    case 4:
        buttonMask = daAlink_c::BTN_R;
        triggered = pendingRPress || mDoCPd_c::getTrigLockR(PAD_1) != 0;
        break;
    case 5:
        buttonMask = daAlink_c::BTN_Z;
        triggered = mDoCPd_c::getTrigZ(PAD_1) != 0;
        break;
    case 6:
        triggered = g_manualJumpLeftStickTriggered;
        break;
    case 7:
        triggered = g_manualJumpRightStickTriggered;
        break;
    case 8:
        buttonMask = daAlink_c::BTN_L;
        triggered = mDoCPd_c::getTrigL(PAD_1) != 0;
        break;
    default:
        return false;
    }

    if (!triggered) {
        return false;
    }

    if (!pendingRPress && buttonMask == daAlink_c::BTN_R &&
        alink->mLinkAcch.ChkGroundHit() &&
        manual_jump_ground_action_ready(alink))
    {
        // Let vanilla R interactions choose Link's action before treating the press as a jump.
        g_manualJumpPendingR = true;
        return false;
    }

    if (g_manualCuccoGlideActive) {
        if (buttonMask != daAlink_c::BTN_A) {
            alink->mItemTrigger &= static_cast<u8>(~buttonMask);
            release_manual_cucco_glide(alink);
        }
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
        if (g_hoverBootsEquipped) {
            if (!hoverBootsAirwalkingEnabled() ||
                alink->checkModeFlg(daAlink_c::MODE_SWIMMING) ||
                alink->checkNoResetFlg0(daAlink_c::FLG0_WATER_IN_MOVE))
            {
                return false;
            }

            g_hoverGroundHeight = alink->current.pos.y;
            g_hoverGroundHeightValid = true;
            g_hoverLandingSequenceActive = false;
            g_hoverAtSurface = false;
            g_hoverLandingFrames = 0;
            alink->mItemTrigger &= static_cast<u8>(~buttonMask);
            return true;
        }

        if (alink->mProcID != daAlink_c::PROC_FALL &&
            alink->mProcID != daAlink_c::PROC_AUTO_JUMP)
        {
            return false;
        }

        if (alink->checkGrabGlide()) {
            if (!start_manual_cucco_glide(alink)) {
                return false;
            }
        } else {
            g_manualCuccoGlideActive = true;
            if (!start_manual_cucco_glide(alink)) {
                g_manualCuccoGlideActive = false;
                return false;
            }
        }

        alink->mLinkAcch.ClrGroundHit();
        alink->mItemTrigger &= static_cast<u8>(~buttonMask);
        return true;
    }

    if (!manual_jump_ground_action_ready(alink)) {
        return false;
    }

    const f32 horizontalMomentum = alink->mNormalSpeed;
    const s16 momentumAngle = alink->current.angle.y;
    const int jumpStarted = wolf ? alink->procWolfAutoJumpInit(0)
                                 : alink->procAutoJumpInit(0);
    if (!jumpStarted) {
        return false;
    }

    const f32 heightScale = std::sqrt(
        static_cast<f32>(manualJumpHeightPercent()) / 100.0f);
    alink->speed.y *= heightScale;
    alink->mNormalSpeed = horizontalMomentum;
    alink->current.angle.y = momentumAngle;
    if (hovering) {
        g_hoverLandingSequenceActive = false;
        g_hoverAtSurface = false;
        g_hoverLandingFrames = 0;
    }
    alink->mLinkAcch.ClrGroundHit();
    alink->mItemTrigger &= static_cast<u8>(~buttonMask);
    return true;
}

void update_secondary_assigned_item_input(ModContext*, void* args, void*, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink == nullptr) {
        return;
    }

    recover_shield_surf_state(alink);
    const bool pendingRPress = g_manualJumpPendingR;
    g_manualJumpPendingR = false;
    const u32 extendedButtons = JUTGamePad::mPadStatus[PAD_1].extButton;
    const bool leftStickHeld = (extendedButtons & PAD_BUTTON_LEFT_STICK) != 0;
    const bool rightStickHeld =
        (extendedButtons & PAD_BUTTON_RIGHT_STICK) != 0 ||
        PADGetNativeButtonPressed(PAD_1) == kNativeRightStickButton;
    const bool leftBumperHeld =
        PADGetNativeButtonPressed(PAD_1) == kNativeLeftShoulderButton;
    g_manualJumpLeftStickTriggered = leftStickHeld && !g_manualJumpLeftStickWasHeld;
    g_manualJumpRightStickTriggered = rightStickHeld && !g_manualJumpRightStickWasHeld;
    g_manualJumpLeftBumperTriggered = leftBumperHeld && !g_manualJumpLeftBumperWasHeld;
    g_manualJumpLeftStickWasHeld = leftStickHeld;
    g_manualJumpRightStickWasHeld = rightStickHeld;
    g_manualJumpLeftBumperWasHeld = leftBumperHeld;
    if (update_hover_plane_lowering(alink, pendingRPress)) {
        return;
    }

    if (pendingRPress && manualJumpBinding() == 4 &&
        alink->mLinkAcch.ChkGroundHit() &&
        manual_jump_ground_action_ready(alink) &&
        try_manual_jump(alink, true))
    {
        return;
    }

    if (try_manual_jump(alink)) {
        return;
    }

    if (alink->checkWolf()) {
        return;
    }

    const bool xTriggered = mDoCPd_c::getTrigX(PAD_1) != 0;
    const bool yTriggered = mDoCPd_c::getTrigY(PAD_1) != 0;
    if (!xTriggered && !yTriggered) {
        return;
    }

    const u8 pressedButtons = static_cast<u8>(
        (secondaryItemAssigned(0) && xTriggered
             ? daAlink_c::BTN_X : 0) |
        (secondaryItemAssigned(1) && yTriggered
             ? daAlink_c::BTN_Y : 0));
    alink->mItemTrigger &= static_cast<u8>(~pressedButtons);
    alink->mItemButton &= static_cast<u8>(~pressedButtons);

    if (g_secondaryRing != nullptr || g_primaryRingOpen || alink->checkEventRun()) {
        g_secondaryItemInputHandled = true;
        return;
    }

    if (g_manualCuccoGlideActive &&
        alink->mProcID == daAlink_c::PROC_AUTO_JUMP)
    {
        auto* heldActor = alink->mGrabItemAcKeep.getActor();
        if (heldActor != nullptr &&
            fopAcM_GetName(heldActor) == fpcNm_NBOMB_e)
        {
            for (int button = 0; button < 2; ++button) {
                const bool triggered = button == 0 ? xTriggered : yTriggered;
                if (!triggered) {
                    continue;
                }

                const int itemId = secondaryItemAssigned(button)
                    ? secondaryItemIdForSlot(g_secondarySelectedItems[button])
                    : dComIfGp_getSelectItem(button);
                if (alink->checkBombItem(itemId)) {
                    heldActor->speedF = 0.0f;
                    heldActor->speed.y = 0.0f;
                    alink->freeGrabItem();
                    const u8 buttonMask =
                        button == 0 ? daAlink_c::BTN_X : daAlink_c::BTN_Y;
                    alink->mItemTrigger &= static_cast<u8>(~buttonMask);
                    g_secondaryItemInputHandled = true;
                    return;
                }
            }
        }
    }

    if (pressedButtons == 0) {
        return;
    }

    if (g_secondaryItemInputHandled) {
        return;
    }

    const int button = (pressedButtons & daAlink_c::BTN_X) != 0 ? 0 : 1;
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
        g_goronTunicEquipped = !g_goronTunicEquipped;
        g_goronLavaContactFrames = 0;
        svc_log->info(mod_ctx, g_goronTunicEquipped ? "Goron Tunic equipped" :
                                                     "Goron Tunic unequipped");
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
        g_darkLinkFormActive = !g_darkLinkFormActive;
        play_dark_link_transition_effect(alink, g_darkLinkFormActive);
        svc_log->info(mod_ctx, g_darkLinkFormActive ? "Dark Link Form activated" :
                                                     "Dark Link Form deactivated");
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
