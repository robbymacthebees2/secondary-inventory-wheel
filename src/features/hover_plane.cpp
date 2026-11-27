void reset_hover_plane_raise_input_state() {
    g_hoverPlaneRaiseLeftStickWasHeld = false;
    g_hoverPlaneRaiseRightStickWasHeld = false;
    g_hoverPlaneRaiseLeftBumperWasHeld = false;
    g_hoverPlaneRaiseLeftStickTriggered = false;
    g_hoverPlaneRaiseRightStickTriggered = false;
    g_hoverPlaneRaiseLeftBumperTriggered = false;
    g_hoverPlaneRaiseJumpActive = false;
    g_hoverPlaneRaiseJumpHasAscended = false;
    g_hoverPlaneRaiseJumpOnBoard = false;
    g_hoverPlaneRaiseJumpPeakY = 0.0f;
}

void update_hover_plane_raise_input_edges() {
    const u32 extendedButtons = JUTGamePad::mPadStatus[PAD_1].extButton;
    const bool leftStickHeld = (extendedButtons & PAD_BUTTON_LEFT_STICK) != 0;
    const bool rightStickHeld =
        (extendedButtons & PAD_BUTTON_RIGHT_STICK) != 0 ||
        PADGetNativeButtonPressed(PAD_1) == kNativeRightStickButton;
    const bool leftBumperHeld =
        PADGetNativeButtonPressed(PAD_1) == kNativeLeftShoulderButton;
    g_hoverPlaneRaiseLeftStickTriggered =
        leftStickHeld && !g_hoverPlaneRaiseLeftStickWasHeld;
    g_hoverPlaneRaiseRightStickTriggered =
        rightStickHeld && !g_hoverPlaneRaiseRightStickWasHeld;
    g_hoverPlaneRaiseLeftBumperTriggered =
        leftBumperHeld && !g_hoverPlaneRaiseLeftBumperWasHeld;
    g_hoverPlaneRaiseLeftStickWasHeld = leftStickHeld;
    g_hoverPlaneRaiseRightStickWasHeld = rightStickHeld;
    g_hoverPlaneRaiseLeftBumperWasHeld = leftBumperHeld;
}

bool hover_plane_raise_binding_triggered(int64_t binding, int* buttonMask) {
    *buttonMask = 0;
    switch (binding) {
    case 0:
        *buttonMask = daAlink_c::BTN_A;
        return mDoCPd_c::getTrigA(PAD_1) != 0;
    case 1:
        *buttonMask = daAlink_c::BTN_B;
        return mDoCPd_c::getTrigB(PAD_1) != 0;
    case 2:
        *buttonMask = daAlink_c::BTN_L;
        return g_hoverPlaneRaiseLeftBumperTriggered;
    case 3:
        *buttonMask = daAlink_c::BTN_R;
        return mDoCPd_c::getTrigLockR(PAD_1) != 0;
    case 4:
        *buttonMask = daAlink_c::BTN_Z;
        return mDoCPd_c::getTrigZ(PAD_1) != 0;
    case 5:
        return g_hoverPlaneRaiseLeftStickTriggered;
    case 6:
        return g_hoverPlaneRaiseRightStickTriggered;
    case 7:
        *buttonMask = daAlink_c::BTN_L;
        return mDoCPd_c::getTrigL(PAD_1) != 0;
    default:
        return false;
    }
}

void update_hover_plane_raise_jump(daAlink_c* alink) {
    if (!g_hoverPlaneRaiseJumpActive) {
        return;
    }

    const int expectedProc = g_hoverPlaneRaiseJumpOnBoard
        ? daAlink_c::PROC_BOARD_JUMP : daAlink_c::PROC_AUTO_JUMP;
    if (alink == nullptr || !g_hoverBootsEquipped || alink->checkEventRun() ||
        alink->checkModeFlg(daAlink_c::MODE_SWIMMING) ||
        alink->checkNoResetFlg0(daAlink_c::FLG0_WATER_IN_MOVE) ||
        alink->mProcID != expectedProc)
    {
        g_hoverPlaneRaiseJumpActive = false;
        g_hoverPlaneRaiseJumpHasAscended = false;
        g_hoverPlaneRaiseJumpOnBoard = false;
        return;
    }

    g_hoverPlaneRaiseJumpPeakY =
        std::max(g_hoverPlaneRaiseJumpPeakY, alink->current.pos.y);
    if (alink->speed.y > 0.0f) {
        g_hoverPlaneRaiseJumpHasAscended = true;
        return;
    }
    if (!g_hoverPlaneRaiseJumpHasAscended) {
        return;
    }

    g_hoverGroundHeight = g_hoverPlaneRaiseJumpPeakY;
    g_hoverGroundHeightValid = true;
    g_hoverAtSurface = false;
    g_hoverLandingSequenceActive = false;
    g_hoverLandingFrames = 0;
    g_hoverPlaneRaiseJumpActive = false;
    g_hoverPlaneRaiseJumpHasAscended = false;
    g_hoverPlaneRaiseJumpOnBoard = false;
}

bool try_raise_hover_plane(daAlink_c* alink, bool shieldSurfing) {
    if (!hoverBootsAirwalkingEnabled() || !g_hoverBootsEquipped ||
        !g_hoverGroundHeightValid || alink == nullptr || alink->checkEventRun() ||
        alink->checkModeFlg(daAlink_c::MODE_SWIMMING) ||
        alink->checkNoResetFlg0(daAlink_c::FLG0_WATER_IN_MOVE) ||
        (shieldSurfing && !g_shieldSurfHoverPlaneActive))
    {
        return false;
    }

    if (shieldSurfing) {
        if (alink->mProcID != daAlink_c::PROC_BOARD_RIDE &&
            alink->mProcID != daAlink_c::PROC_BOARD_WAIT)
        {
            return false;
        }
        if (alink->procBoardJumpInit(0.0f, FALSE) == 0) {
            return false;
        }
    } else {
        if (!is_hover_boots_hovering(alink)) {
            return false;
        }
        if (g_hoverLandingSequenceActive &&
            alink->mProcID == daAlink_c::PROC_LAND)
        {
            alink->procMoveInit();
        }
        if (alink->mProcID != daAlink_c::PROC_WAIT &&
            alink->mProcID != daAlink_c::PROC_MOVE)
        {
            return false;
        }

        const f32 horizontalMomentum = alink->mNormalSpeed;
        const s16 momentumAngle = alink->current.angle.y;
        if (alink->procAutoJumpInit(0) == 0) {
            return false;
        }
        alink->mNormalSpeed = horizontalMomentum;
        alink->current.angle.y = momentumAngle;
    }

    g_hoverPlaneRaiseJumpActive = true;
    g_hoverPlaneRaiseJumpHasAscended = false;
    g_hoverPlaneRaiseJumpOnBoard = shieldSurfing;
    g_hoverPlaneRaiseJumpPeakY = alink->current.pos.y;
    g_hoverAtSurface = false;
    g_hoverLandingSequenceActive = false;
    g_hoverLandingFrames = 0;
    return true;
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

bool update_hover_plane_lowering(daAlink_c* alink) {
    const bool held = hoverPlaneLowerBindingHeld(hoverPlaneLowerBinding());
    const bool pressed = held && !g_hoverPlaneLowerBindingWasHeld;
    g_hoverPlaneLowerBindingWasHeld = held;
    if (g_hoverPlaneRaiseJumpActive) {
        return false;
    }

    const bool shieldSurfing = g_shieldSurfActive && alink != nullptr &&
                               alink->checkBoardRide();
    if (!pressed || !hoverBootsAirwalkingEnabled() || !g_hoverBootsEquipped ||
        !g_hoverGroundHeightValid || alink == nullptr ||
        (shieldSurfing && !g_shieldSurfHoverPlaneActive) ||
        (!shieldSurfing && alink->mLinkAcch.ChkGroundHit()) ||
        alink->checkEventRun() ||
        alink->checkModeFlg(daAlink_c::MODE_SWIMMING) ||
        alink->checkNoResetFlg0(daAlink_c::FLG0_WATER_IN_MOVE))
    {
        return false;
    }

    f32 vanillaJumpHeight = 0.0f;
    if (!get_vanilla_standing_jump_height(alink, &vanillaJumpHeight)) {
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
    } else {
        if (g_hoverAtSurface) {
            alink->procFallInit(1, alink->mpHIO->mAutoJump.m.mFallInterpolation);
        }
        const auto& jump = alink->mpHIO->mAutoJump.m;
        alink->setSpecialGravity(jump.mGravity, jump.mMaxFallSpeed, TRUE);
        alink->speed.y = std::min(alink->speed.y, -0.5f);
    }

    g_hoverGroundHeight -= vanillaJumpHeight;
    g_hoverGroundHeightValid = true;
    g_hoverLandingSequenceActive = false;
    g_hoverAtSurface = false;
    g_hoverLandingFrames = 0;
    return true;
}

void update_secondary_assigned_item_input(ModContext*, void* args, void*, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink == nullptr) {
        return;
    }

    recover_shield_surf_state(alink);
    update_hover_plane_raise_input_edges();

    int raiseButtonMask = 0;
    const bool raiseHeld = hover_plane_raise_binding_triggered(
        hoverPlaneRaiseBinding(), &raiseButtonMask);
    const bool shieldSurfing =
        g_shieldSurfActive && alink->checkBoardRide();
    if (raiseHeld && try_raise_hover_plane(alink, shieldSurfing)) {
        alink->mItemTrigger &= static_cast<u8>(~raiseButtonMask);
        return;
    }
    if (update_hover_plane_lowering(alink)) {
        return;
    }

    if (alink->checkWolf()) {
        return;
    }
    if (g_twilitItemsActive) {
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

    if (pressedButtons == 0 || g_secondaryItemInputHandled) {
        return;
    }

    const int button = (pressedButtons & daAlink_c::BTN_X) != 0 ? 0 : 1;
    const u8 itemId = secondaryItemIdForSlot(g_secondarySelectedItems[button]);
    g_secondaryItemInputHandled = true;
    use_secondary_item(alink, itemId, button);
}

bool use_secondary_item(daAlink_c* alink, u8 itemId, int button) {
    if (alink == nullptr || alink->checkWolf() || button < 0 || button > 2) {
        return false;
    }

    switch (itemId) {
    case kSitItemId:
        if (g_sitActionState == SitActionState::Inactive) {
            if (!can_start_sit_action(alink)) {
                return false;
            }
            request_sit_action(alink);
        } else if (g_sitActionState == SitActionState::Seated) {
            begin_stand_action(alink);
        } else {
            return false;
        }
        return true;
    case kGoronTunicItemId:
        g_goronTunicEquipped = !g_goronTunicEquipped;
        g_goronLavaContactFrames = 0;
        svc_log->info(mod_ctx, g_goronTunicEquipped ? "Goron Tunic equipped" :
                                                     "Goron Tunic unequipped");
        return true;
    case kShieldSurfItemId:
        request_shield_surf_toggle(alink);
        return true;
    case kHawkGrassItem:
        begin_secondary_hawk_grass_use(alink, static_cast<u8>(button));
        return true;
    case kHoverBootsItem:
        begin_hover_boots_toggle(alink);
        return true;
    case kDarkLinkItem:
        g_darkLinkFormActive = !g_darkLinkFormActive;
        play_dark_link_transition_effect(alink, g_darkLinkFormActive);
        svc_log->info(mod_ctx, g_darkLinkFormActive ? "Dark Link Form activated" :
                                                     "Dark Link Form deactivated");
        return true;
    case kIceArrowItem:
        set_ice_arrow_selected(alink, !g_iceArrowSelected);
        return true;
    case kLensOfTruthItem:
        g_lensOfTruthActive = !g_lensOfTruthActive;
        svc_log->info(
            mod_ctx, g_lensOfTruthActive ? "Lens of Truth activated" :
                                          "Lens of Truth deactivated");
        return true;
    case kTruthMaskItem:
        g_truthMaskActive = !g_truthMaskActive;
        svc_log->info(mod_ctx, g_truthMaskActive ? "Stone Mask equipped" :
                                                   "Stone Mask removed");
        return true;
    case kDinFireItem:
        begin_fire_sphere_use(alink);
        return true;
    case kNayruLoveItem:
        begin_nayru_love_use(alink);
        return true;
    case kGiantMaskItem:
        if (g_hoverBootsEquipped && !g_giantMaskActive) {
            svc_log->warn(
                mod_ctx, "Giant's Mask cannot be activated while Hover Boots are active");
            return false;
        }
        g_giantMaskActive = !g_giantMaskActive;
        svc_log->info(
            mod_ctx, g_giantMaskActive ? "Giant's Mask equipped" :
                                         "Giant's Mask removed");
        return true;
    default:
        return false;
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
