DEFINE_HOOK(
    &daAlink_c::checkItemChangeFromButton, SwordFlourishItemChange);
DEFINE_HOOK(&daAlink_c::setNormalSpeedF, SetNormalSpeedF);
DEFINE_HOOK(&daAlink_c::decideCommonDoStatus, DecideCommonDoStatus);
DEFINE_HOOK(&daAlink_c::setFrontWallType, SetFrontWallType);
DEFINE_HOOK(&daAlink_c::checkRoomOnly, CheckRoomOnlyForMovement);
DEFINE_HOOK(&daAlink_c::checkNotAutoJumpStage, CheckNotAutoJumpStage);
DEFINE_HOOK(&daAlink_c::checkNoSubjectModeCamera, CheckNoSubjectModeCamera);
DEFINE_HOOK(&dCamera_c::ChangeModeOK, CastleTownChangeModeOK);
DEFINE_HOOK(&daAlink_c::checkHookshotStickBG, CheckHookshotStickBG);
DEFINE_HOOK_SYMBOL(
    "cBgS::GetTriPla", bool(const cBgS*, const cBgS_PolyInfo*, cM3dGPla*),
    ClawshotUnevenSurfaceGetTriPla);

namespace {
bool g_bypassRoomOnlyMovementRestriction = false;
}

HookAction begin_unrestricted_room_movement(ModContext*, void* args, void*, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (!indoorMovementUnrestrictedEnabled() || alink == nullptr ||
        !alink->checkRoomOnly() || g_bypassRoomOnlyMovementRestriction)
    {
        return HOOK_CONTINUE;
    }

    g_bypassRoomOnlyMovementRestriction = true;
    return HOOK_CONTINUE;
}

void allow_unrestricted_room_movement(ModContext*, void*, void* retval, void*) {
    if (g_bypassRoomOnlyMovementRestriction) {
        *static_cast<bool*>(retval) = false;
    }
}

void end_unrestricted_room_movement(ModContext*, void*, void*, void*) {
    g_bypassRoomOnlyMovementRestriction = false;
}

void allow_unrestricted_room_auto_jump(ModContext*, void*, void* retval, void*) {
    if (indoorMovementUnrestrictedEnabled() && daAlink_c::checkRoomOnly()) {
        *static_cast<bool*>(retval) = false;
    }
}

void allow_castle_town_first_person(ModContext*, void* args, void* retval, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (castleTownFirstPersonEnabled() && alink != nullptr &&
        alink->checkCastleTown())
    {
        *static_cast<bool*>(retval) = false;
    }
}

void allow_castle_town_subject_mode(ModContext*, void* args, void* retval, void*) {
    auto* camera = mods::arg<dCamera_c*>(args, 0);
    auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    if (castleTownFirstPersonEnabled() && camera != nullptr && alink != nullptr &&
        alink->checkCastleTown() && mods::arg<s32>(args, 1) == 4 &&
        !dComIfGp_evmng_cameraPlay() && !camera->chkFlag(0x20000000))
    {
        *static_cast<bool*>(retval) = true;
    }
}

void use_castle_town_subject_camera_type(
    ModContext*, void* args, void* retval, void*)
{
    auto* camera = mods::arg<dCamera_c*>(args, 0);
    auto* alink = daAlink_getAlinkActorClass();
    if (!castleTownFirstPersonEnabled() || camera == nullptr || alink == nullptr ||
        !alink->checkCastleTown() ||
        alink->mProcID != daAlink_c::PROC_SUBJECTIVITY)
    {
        return;
    }

    const s32 fieldCameraType = camera->GetCameraTypeFromCameraName("FieldS");
    if (fieldCameraType < 0xFFFF) {
        *static_cast<s32*>(retval) = fieldCameraType;
    }
}

HookAction allow_clawshots_any_surface(ModContext*, void* args, void* retval, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (!clawshotAnySurfaceEnabled() || alink == nullptr) {
        return HOOK_CONTINUE;
    }

    *static_cast<BOOL*>(retval) = TRUE;
    return HOOK_SKIP_ORIGINAL;
}

void allow_clawshot_uneven_surface_stick(
    ModContext*, void* args, void* retval, void*)
{
    auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    const auto* poly = mods::arg<const cBgS_PolyInfo*>(args, 1);
    auto* plane = mods::arg<cM3dGPla*>(args, 2);
    if (!clawshotUnevenSurfaceStickEnabled() || alink == nullptr ||
        alink->mProcID != daAlink_c::PROC_HOOKSHOT_FLY ||
        alink->mProcVar2.field_0x300c != 4 || poly != &alink->mPolyInfo2 ||
        plane == nullptr || !*static_cast<bool*>(retval))
    {
        return;
    }

    const f32 normalY = plane->mNormal.y;
    if (std::fabs(normalY) <= 0.05f || std::fabs(normalY) > 0.5f) {
        return;
    }

    plane->mNormal.y = 0.0f;
}

HookAction enable_sword_putaway_flourish(ModContext*, void* args, void*, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (!get_bool_option(g_swordFlourishEnabled, false) || alink == nullptr ||
        alink->checkWolf() || alink->checkEventRun() || !alink->checkSwordDraw() ||
        alink->checkWoodSwordEquip() || alink->checkModeFlg(0x402))
    {
        return HOOK_CONTINUE;
    }

    if (alink->mSwordFlourishTimer == 0) {
        alink->mSwordFlourishTimer = 1;
    }
    return HOOK_CONTINUE;
}

HookAction handle_hover_boots_heavy_toggle(ModContext*, void* args, void* retval, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink == nullptr || !g_hoverBootsTransitionPending ||
        alink->mProcID != daAlink_c::PROC_BOOTS_EQUIP)
    {
        return HOOK_CONTINUE;
    }

    *static_cast<int*>(retval) = 0;
    return HOOK_SKIP_ORIGINAL;
}

void begin_hover_boots_toggle(daAlink_c* alink) {
    if (alink == nullptr) {
        return;
    }

    if (g_shieldSurfActive && alink->checkBoardRide()) {
        set_hover_boots_enabled(alink, !g_hoverBootsEquipped);
        return;
    }

    set_hover_boots_enabled(alink, !g_hoverBootsEquipped);
    g_hoverBootsTransitionPending = false;

    const bool playBootsAnimation =
        !alink->mLinkAcch.ChkGroundHit() ||
        (alink->mLinkAcch.ChkGroundHit() && !alink->checkModeFlg(0x70C52)) ||
        (alink->checkMagneBootsOn() && cBgW_CheckBGround(alink->mMagneBootsTopVec.y)) ||
        alink->mProcID == daAlink_c::PROC_HANG_CLIMB;
    if (playBootsAnimation) {
        g_hoverBootsTransitionPending = true;
        const int result = alink->procBootsEquipInit();
        if (result == 0 || alink->mProcID != daAlink_c::PROC_BOOTS_EQUIP) {
            g_hoverBootsTransitionPending = false;
        }
    }
}

void begin_secondary_hawk_grass_use(daAlink_c* alink, u8 button) {
    if (alink == nullptr) {
        return;
    }

    alink->mSelectItemId = button;
    g_pendingHawkGrassUse = alink;
    g_pendingHawkGrassButton = button;
    g_hawkGrassPhaseRequested = true;
    update_hawk_grass_resource();
}

bool can_start_sit_action(const daAlink_c* alink) {
    return alink != nullptr && !alink->checkWolf() && !alink->checkEventRun() &&
           (alink->mLinkAcch.ChkGroundHit() || is_hover_boots_hovering(alink)) &&
           (alink->mProcID == daAlink_c::PROC_WAIT ||
            alink->mProcID == daAlink_c::PROC_MOVE ||
            alink->mProcID == daAlink_c::PROC_ATN_ACTOR_WAIT ||
            alink->mProcID == daAlink_c::PROC_ATN_ACTOR_MOVE);
}

namespace {
OSTime g_sitHealNextTick = 0;
bool g_sitHealTimerStarted = false;

void reset_sit_healing_timer() {
    g_sitHealTimerStarted = false;
}

void update_sit_healing() {
    const OSTime now = OSGetTime();
    if (!sitHealingEnabled()) {
        g_sitHealTimerStarted = false;
        return;
    }
    if (!g_sitHealTimerStarted) {
        g_sitHealNextTick = now + OSSecondsToTicks(4);
        g_sitHealTimerStarted = true;
        return;
    }
    if (now < g_sitHealNextTick) {
        return;
    }

    const u16 life = dComIfGs_getLife();
    const u16 maxLife = dComIfGs_getMaxLifeGauge();
    if (life < maxLife) {
        dComIfGp_setItemLifeCount(1.0f, 1);
    }
    g_sitHealNextTick = now + OSSecondsToTicks(1);
}
}  // namespace

void align_link_to_ground(daAlink_c* alink) {
    const f32 groundHeight = alink->mLinkAcch.GetGroundH();
    if (groundHeight == -G_CM3D_F_INF) {
        return;
    }

    alink->current.pos.y = groundHeight;
    if (!dComIfG_Bgsp().ChkPolySafe(alink->mLinkAcch.m_gnd)) {
        return;
    }

    cM3dGPla groundPlane;
    if (!dComIfG_Bgsp().GetTriPla(alink->mLinkAcch.m_gnd, &groundPlane) ||
        !cBgW_CheckBGround(groundPlane.mNormal.y))
    {
        return;
    }

    alink->shape_angle.x =
        fopAcM_getPolygonAngle(&groundPlane, alink->shape_angle.y);
    alink->shape_angle.z =
        -fopAcM_getPolygonAngle(&groundPlane, alink->shape_angle.y + 0x4000);
}

void restore_sit_shape_angles(daAlink_c* alink) {
    alink->shape_angle.x = g_sitOriginalShapeAngleX;
    alink->shape_angle.z = g_sitOriginalShapeAngleZ;
}

void begin_sit_action(daAlink_c* alink) {
    if (!can_start_sit_action(alink)) {
        return;
    }

    const bool hovering = is_hover_boots_hovering(alink);
    g_sitOriginalShapeAngleX = alink->shape_angle.x;
    g_sitOriginalShapeAngleZ = alink->shape_angle.z;
    alink->procWaitInit();
    alink->mNormalSpeed = 0.0f;
    alink->speedF = 0.0f;
    if (hovering) {
        alink->current.pos.y = g_hoverGroundHeight;
        alink->speed.y = 0.0f;
        alink->mSinkShapeOffset = 0.0f;
    } else {
        align_link_to_ground(alink);
    }
    alink->setSingleAnimeBase(daAlink_c::ANM_WAIT_SIT);
    g_sitActionState = SitActionState::Seated;
    reset_sit_healing_timer();
    svc_log->info(mod_ctx, "Sit action entered");
}

void begin_stand_action(daAlink_c* alink) {
    if (alink == nullptr || g_sitActionState != SitActionState::Seated) {
        return;
    }

    restore_sit_shape_angles(alink);
    alink->mSinkShapeOffset = 0.0f;
    alink->setSingleAnimeBase(daAlink_c::ANM_WAIT);
    g_sitActionState = SitActionState::Inactive;
    g_sitHealTimerStarted = false;
    svc_log->info(mod_ctx, "Sit action ended by player");
}

HookAction preserve_sit_idle_animation(ModContext*, void* args, void* retval, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (g_sitActionState == SitActionState::Inactive || alink == nullptr ||
        alink->mProcID != daAlink_c::PROC_WAIT)
    {
        return HOOK_CONTINUE;
    }

    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

void suppress_sit_stick_input(ModContext*, void* args, void*, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (g_sitActionState == SitActionState::Inactive || alink == nullptr) {
        return;
    }

    alink->mStickValue = 0.0f;
    alink->mMoveValue = 0.0f;
    alink->mNormalSpeed = 0.0f;
    alink->speedF = 0.0f;
}

void update_sit_action() {
    if (g_sitActionState == SitActionState::Inactive) {
        g_sitHealTimerStarted = false;
        return;
    }

    auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    const bool grounded = alink != nullptr && alink->mLinkAcch.ChkGroundHit();
    const bool hovering = is_hover_boots_hovering(alink);
    const bool eventRunning = alink != nullptr && alink->checkEventRun();
    if (alink == nullptr || alink->checkWolf() || eventRunning ||
        alink->mProcID != daAlink_c::PROC_WAIT || (!grounded && !hovering))
    {
        char message[128];
        std::snprintf(message, sizeof(message),
                      "Sit action ended by game state: actor=%u event=%u proc=%d grounded=%u hovering=%u",
                      static_cast<u32>(alink != nullptr),
                      static_cast<u32>(eventRunning),
                      alink != nullptr ? static_cast<int>(alink->mProcID) : -1,
                      static_cast<u32>(grounded), static_cast<u32>(hovering));
        svc_log->warn(mod_ctx, message);
        if (alink != nullptr) {
            restore_sit_shape_angles(alink);
            alink->mSinkShapeOffset = 0.0f;
        }
        g_sitActionState = SitActionState::Inactive;
        g_sitHealTimerStarted = false;
        return;
    }

    update_sit_healing();

    if (hovering) {
        alink->current.pos.y = g_hoverGroundHeight;
        alink->speed.y = 0.0f;
        alink->mSinkShapeOffset = 0.0f;
        alink->mNormalSpeed = 0.0f;
        alink->speedF = 0.0f;
        return;
    }

    if (alink->mLinkAcch.GetGroundH() == -G_CM3D_F_INF) {
        restore_sit_shape_angles(alink);
        alink->mSinkShapeOffset = 0.0f;
        g_sitActionState = SitActionState::Inactive;
        g_sitHealTimerStarted = false;
        svc_log->warn(mod_ctx, "Sit action ended: no valid ground height");
        return;
    }

    align_link_to_ground(alink);
    alink->mNormalSpeed = 0.0f;
    alink->speedF = 0.0f;
}
