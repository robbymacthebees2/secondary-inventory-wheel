bool g_shieldSurfHoverPlaneGroundInjected = false;
bool g_shieldSurfHoverPlaneGroundFindWasSet = false;
bool g_shieldSurfHoverPlaneGroundLandingWasSet = false;
bool g_shieldSurfHoverPlaneGroundAwayWasSet = false;
f32 g_shieldSurfHoverPlanePreviousGroundHeight = -G_CM3D_F_INF;
cBgS_PolyInfo g_shieldSurfPreviousGroundPolygon;

void delete_shield_surf_board() {
    if (g_shieldSurfBoardId != fpcM_ERROR_PROCESS_ID_e) {
        auto* board = static_cast<daObjIceLeaf_c*>(
            fopAcM_SearchByID(g_shieldSurfBoardId));
        if (board != nullptr) {
            board->setMode(daObjIceLeaf_c::MODE_RIDE_e);
            board->setAction(daObjIceLeaf_c::ACT_DEAD_e);
            board->offRide();
        }
        fopAcM_delete(g_shieldSurfBoardId);
        g_shieldSurfBoardId = fpcM_ERROR_PROCESS_ID_e;
    }
}

bool is_shield_surf_board_process(int processId) {
    return processId >= daAlink_c::PROC_BOARD_RIDE &&
           processId <= daAlink_c::PROC_BOARD_CUT_TURN;
}

bool has_shield_surf_surface(const daAlink_c* alink) {
    return alink != nullptr &&
           (alink->mLinkAcch.ChkGroundHit() || is_hover_boots_hovering(alink));
}

bool is_shield_surf_on_hover_plane(const daAlink_c* alink) {
    const bool hoverPlaneActive =
        g_shieldSurfActive ? g_shieldSurfHoverPlaneActive : g_hoverBootsEquipped;
    return alink != nullptr && hoverBootsAirwalkingEnabled() &&
           hoverPlaneActive && g_hoverGroundHeightValid &&
           !alink->checkEventRun() &&
           !alink->checkModeFlg(daAlink_c::MODE_SWIMMING) &&
           std::abs(alink->current.pos.y - g_hoverGroundHeight) <= 1.0f;
}

void release_shield_surf_player_state(daAlink_c* alink) {
    if (alink == nullptr) {
        return;
    }

    fopAc_ac_c* rideActor = alink->getRideActor();
    const bool ridingTrackedBoard =
        rideActor != nullptr && g_shieldSurfBoardId != fpcM_ERROR_PROCESS_ID_e &&
        fopAcM_GetID(rideActor) == g_shieldSurfBoardId;
    const bool staleBoardRide =
        g_shieldSurfActive && alink->checkBoardRide() &&
        (rideActor == nullptr || is_shield_surf_board_process(alink->mProcID));
    if (ridingTrackedBoard)
    {
        auto* board = static_cast<daObjIceLeaf_c*>(rideActor);
        board->setMode(daObjIceLeaf_c::MODE_RIDE_e);
        board->setAction(daObjIceLeaf_c::ACT_DEAD_e);
        board->offRide();
    }
    if (ridingTrackedBoard || staleBoardRide) {
        alink->mRideAcKeep.clearData();
        alink->mRideStatus = 0;
        alink->offModeFlg(daAlink_c::MODE_RIDING);
        alink->offNoResetFlg1(static_cast<daPy_py_c::daPy_FLG1>(0x1800));
        alink->attention_info.field_0xa = 10;
        alink->shape_angle.x = 0;
        alink->shape_angle.z = 0;
    }
    delete_shield_surf_board();
}

void end_shield_surf_state(daAlink_c* alink) {
    release_shield_surf_player_state(alink);
    g_shieldSurfActive = false;
    g_shieldSurfStartPending = false;
    g_shieldSurfStartWaitFrames = 0;
    g_shieldSurfDismountPending = false;
    g_shieldSurfHoverPlaneActive = false;
    g_shieldSurfPlaneRaisePending = false;
    g_hoverBootsTransitionPending = false;
    g_hoverLandingSequenceActive = false;
    g_hoverLandingFrames = 0;
    if (!g_hoverBootsEquipped) {
        disable_hover_boots();
    }
}

bool is_riding_shield_surf_board(daAlink_c* alink) {
    if (alink == nullptr || g_shieldSurfBoardId == fpcM_ERROR_PROCESS_ID_e ||
        !is_shield_surf_board_process(alink->mProcID) || !alink->checkBoardRide())
    {
        return false;
    }

    fopAc_ac_c* rideActor = alink->getRideActor();
    return rideActor != nullptr &&
           fopAcM_GetID(rideActor) == g_shieldSurfBoardId &&
           fopAcM_SearchByID(g_shieldSurfBoardId) != nullptr;
}

void recover_shield_surf_state(daAlink_c* alink) {
    if (alink == nullptr) {
        return;
    }

    if (g_shieldSurfActive) {
        if (!is_riding_shield_surf_board(alink)) {
            end_shield_surf_state(alink);
        }
        return;
    }

    if (!g_shieldSurfStartPending &&
        g_shieldSurfBoardId != fpcM_ERROR_PROCESS_ID_e)
    {
        end_shield_surf_state(alink);
    }
}

void reset_shield_surf_state_for_save_change() {
    auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    if (g_shieldSurfActive || g_shieldSurfStartPending ||
        g_shieldSurfBoardId != fpcM_ERROR_PROCESS_ID_e)
    {
        end_shield_surf_state(alink);
    } else {
        g_shieldSurfDismountPending = false;
        g_shieldSurfHoverPlaneActive = false;
        g_shieldSurfPlaneRaisePending = false;
        g_shieldSurfStartWaitFrames = 0;
    }
}

HookAction prevent_shield_surf_collision_knockoff(
    ModContext*, void* args, void* retval, void*)
{
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    const int damageType = mods::arg<int>(args, 1);
    if (retval == nullptr || damageType != -4 || !g_shieldSurfActive ||
        !is_riding_shield_surf_board(alink))
    {
        return HOOK_CONTINUE;
    }

    int wallIndex = -1;
    const s16 reverseTravelAngle = static_cast<s16>(alink->current.angle.y + 0x8000);
    for (int i = 0; i < 3; ++i) {
        if (alink->mAcchCir[i].ChkWallHit() &&
            cLib_distanceAngleS(
                reverseTravelAngle, alink->mAcchCir[i].GetWallAngleY()) <= 0x2000)
        {
            wallIndex = i;
            break;
        }
    }
    if (wallIndex < 0) {
        return HOOK_CONTINUE;
    }

    const s16 wallAngle = alink->mAcchCir[wallIndex].GetWallAngleY();
    const f32 normalX = cM_ssin(wallAngle);
    const f32 normalZ = cM_scos(wallAngle);
    const f32 travelX = cM_ssin(alink->current.angle.y);
    const f32 travelZ = cM_scos(alink->current.angle.y);
    const s16 tangentA = static_cast<s16>(wallAngle + 0x4000);
    const s16 tangentB = static_cast<s16>(wallAngle - 0x4000);
    const f32 tangentAX = cM_ssin(tangentA);
    const f32 tangentAZ = cM_scos(tangentA);
    const f32 tangentBX = cM_ssin(tangentB);
    const f32 tangentBZ = cM_scos(tangentB);
    const f32 tangentADot = travelX * tangentAX + travelZ * tangentAZ;
    const f32 tangentBDot = travelX * tangentBX + travelZ * tangentBZ;
    const bool chooseTangentA = tangentADot >= tangentBDot;
    const f32 tangentDot = chooseTangentA ? tangentADot : tangentBDot;
    const s16 tangentAngle = chooseTangentA ? tangentA : tangentB;
    const f32 tangentSpeed = alink->mNormalSpeed * std::max(0.0f, tangentDot);
    const f32 separationX = normalX * 10.0f;
    const f32 separationZ = normalZ * 10.0f;

    alink->current.angle.y = tangentAngle;
    alink->shape_angle.y = tangentAngle;
    alink->mNormalSpeed = tangentSpeed;
    alink->speed.x = tangentSpeed * cM_ssin(tangentAngle);
    alink->speed.z = tangentSpeed * cM_scos(tangentAngle);
    alink->current.pos.x += separationX;
    alink->current.pos.z += separationZ;
    alink->old.pos.x += separationX;
    alink->old.pos.z += separationZ;
    alink->offModeFlg(daAlink_c::MODE_JUMP);
    *static_cast<int*>(retval) = 0;
    return HOOK_SKIP_ORIGINAL;
}

void request_shield_surf_toggle(daAlink_c* alink) {
    recover_shield_surf_state(alink);
    if (g_shieldSurfActive) {
        g_shieldSurfDismountPending = true;
        return;
    }
    if (g_shieldSurfStartPending) {
        g_shieldSurfStartPending = false;
        g_shieldSurfStartWaitFrames = 0;
        g_shieldSurfHoverPlaneActive = false;
        g_shieldSurfPlaneRaisePending = false;
        delete_shield_surf_board();
        svc_log->info(mod_ctx, "Shield Surf start canceled");
        return;
    }
    if (alink == nullptr || alink->checkWolf() || alink->checkEventRun() ||
        alink->checkRideOn() || !has_shield_surf_surface(alink))
    {
        svc_log->warn(
            mod_ctx, "Shield Surf requires Link on walkable ground or a stable Hover Boots "
                     "plane and not riding");
        return;
    }
    if (dComIfGs_getSelectEquipShield() == dItemNo_NONE_e ||
        alink->mShieldModel == nullptr)
    {
        svc_log->warn(mod_ctx, "Shield Surf requires an equipped shield");
        return;
    }

    g_shieldSurfStartPending = true;
    g_shieldSurfStartWaitFrames = 0;
    g_shieldSurfHoverPlaneActive =
        hoverBootsAirwalkingEnabled() && g_hoverBootsEquipped &&
        g_hoverGroundHeightValid;
    g_shieldSurfPlaneRaisePending = false;
    svc_log->info(mod_ctx, "Shield Surf start requested");
}

void process_shield_surf_transition(ModContext*, void* args, void*, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink == nullptr) {
        return;
    }

    if (g_shieldSurfActive) {
        if (!is_riding_shield_surf_board(alink)) {
            recover_shield_surf_state(alink);
            svc_log->info(mod_ctx, "Shield Surf ended after Link left its board");
        } else if (g_shieldSurfDismountPending) {
            g_shieldSurfDismountPending = false;
            alink->setJumpMode();
            alink->procBackJumpInit(0);
        }
        return;
    }
    if (!g_shieldSurfStartPending) {
        return;
    }
    if (alink->checkWolf() || alink->checkEventRun() || alink->checkRideOn() ||
        !has_shield_surf_surface(alink) ||
        dComIfGs_getSelectEquipShield() == dItemNo_NONE_e ||
        alink->mShieldModel == nullptr)
    {
        g_shieldSurfStartPending = false;
        g_shieldSurfHoverPlaneActive = false;
        delete_shield_surf_board();
        svc_log->warn(mod_ctx, "Shield Surf start canceled because Link is no longer ready");
        return;
    }

    if (g_shieldSurfBoardId == fpcM_ERROR_PROCESS_ID_e) {
        cXyz spawnPosition = alink->current.pos;
        g_shieldSurfBoardId = fopAcM_create(
            fpcNm_Obj_IceLeaf_e, 0x100FF, &spawnPosition,
            fopAcM_GetRoomNo(alink), &alink->shape_angle, nullptr, -1);
        if (g_shieldSurfBoardId == fpcM_ERROR_PROCESS_ID_e) {
            g_shieldSurfStartPending = false;
            g_shieldSurfHoverPlaneActive = false;
            svc_log->error(mod_ctx, "Failed to create the native Ice Leaf board actor");
            return;
        }
        return;
    }

    auto* board = static_cast<daObjIceLeaf_c*>(
        fopAcM_SearchByID(g_shieldSurfBoardId));
    if (board != nullptr) {
        board->setMode(daObjIceLeaf_c::MODE_RIDE_e);
        board->setAction(daObjIceLeaf_c::ACT_DEAD_e);
        board->current.pos = alink->current.pos;
        board->shape_angle = alink->shape_angle;
    }
    if (board == nullptr || board->mpModel == nullptr) {
        if (++g_shieldSurfStartWaitFrames >= 300) {
            g_shieldSurfStartPending = false;
            g_shieldSurfHoverPlaneActive = false;
            delete_shield_surf_board();
            svc_log->error(mod_ctx, "Timed out loading the native Shield Surf board actor");
        }
        return;
    }

    board->setMode(daObjIceLeaf_c::MODE_RIDE_e);
    board->setAction(daObjIceLeaf_c::ACT_DEAD_e);
    if (!alink->procBoardWaitInit(board) || !alink->checkBoardRide()) {
        g_shieldSurfStartPending = false;
        g_shieldSurfHoverPlaneActive = false;
        delete_shield_surf_board();
        svc_log->warn(mod_ctx, "The game rejected the Shield Surf board process");
        return;
    }

    board->setBaseMtx();
    g_shieldSurfStartPending = false;
    g_shieldSurfActive = true;
    g_shieldSurfPlaneRaisePending = false;
    svc_log->info(mod_ctx, "Shield Surf started using Link's equipped shield");
}

HookAction allow_shield_surf_on_any_ground(ModContext*, void* args, void* retval, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (g_shieldSurfActive && alink != nullptr && alink->checkBoardRide()) {
        *static_cast<bool*>(retval) = true;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

HookAction suppress_player_shield_draw_during_surf(
    ModContext*, void* args, void* retval, void*)
{
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (g_shieldSurfActive && alink != nullptr && alink->checkBoardRide()) {
        *static_cast<bool*>(retval) = false;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

HookAction suppress_bow_action_during_shield_surf(
    ModContext*, void* args, void* retval, void*)
{
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (g_shieldSurfActive && alink != nullptr && alink->checkBoardRide()) {
        *static_cast<int*>(retval) = 0;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

HookAction keep_shield_surf_board_in_ride_mode(
    ModContext*, void* args, void*, void*)
{
    auto* board = mods::arg<daObjIceLeaf_c*>(args, 0);
    if (board == nullptr || fopAcM_GetID(board) != g_shieldSurfBoardId) {
        return HOOK_CONTINUE;
    }

    board->setMode(daObjIceLeaf_c::MODE_RIDE_e);
    board->setAction(daObjIceLeaf_c::ACT_DEAD_e);
    return HOOK_CONTINUE;
}

HookAction prepare_shield_surf_hover_plane_board_common(
    ModContext*, void* args, void*, void*)
{
    g_shieldSurfHoverPlaneGroundInjected = false;
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (g_shieldSurfActive && alink != nullptr && alink->checkBoardRide() &&
        alink->speed.y <= 0.0f && is_shield_surf_on_hover_plane(alink) &&
        g_shieldSurfGroundPolygonValid)
    {
        alink->current.pos.y = g_hoverGroundHeight;
        alink->speed.y = 0.0f;
        g_shieldSurfPreviousGroundPolygon.SetPolyInfo(alink->mLinkAcch.m_gnd);
        alink->mLinkAcch.m_gnd.SetPolyInfo(g_shieldSurfGroundPolygon);
        g_shieldSurfHoverPlanePreviousGroundHeight =
            alink->mLinkAcch.m_ground_h;
        g_shieldSurfHoverPlaneGroundFindWasSet =
            alink->mLinkAcch.ChkGroundFind();
        g_shieldSurfHoverPlaneGroundLandingWasSet =
            alink->mLinkAcch.ChkGroundLanding();
        g_shieldSurfHoverPlaneGroundAwayWasSet =
            alink->mLinkAcch.ChkGroundAway();
        alink->mLinkAcch.m_ground_h = g_hoverGroundHeight;
        alink->mLinkAcch.SetGroundHit();
        alink->mLinkAcch.SetGroundFind();
        alink->mLinkAcch.ClrGroundLanding();
        alink->mLinkAcch.ClrGroundAway();
        g_shieldSurfHoverPlaneGroundInjected = true;
    }
    return HOOK_CONTINUE;
}

HookAction flatten_shield_surf_hover_plane_polygon(
    ModContext*, void* args, void* retval, void*)
{
    if (!g_shieldSurfHoverPlaneGroundInjected) {
        return HOOK_CONTINUE;
    }

    auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    const auto* polygon = mods::arg<const cBgS_PolyInfo*>(args, 1);
    if (alink == nullptr || !alink->checkBoardRide() ||
        polygon == nullptr ||
        polygon->GetBgIndex() != alink->mLinkAcch.m_gnd.GetBgIndex() ||
        polygon->GetPolyIndex() != alink->mLinkAcch.m_gnd.GetPolyIndex())
    {
        return HOOK_CONTINUE;
    }

    auto* plane = mods::arg<cM3dGPla*>(args, 2);
    if (plane == nullptr) {
        return HOOK_CONTINUE;
    }

    plane->mNormal = cXyz(0.0f, 1.0f, 0.0f);
    plane->mD = -g_hoverGroundHeight;
    *static_cast<bool*>(retval) = true;
    return HOOK_SKIP_ORIGINAL;
}

HookAction prevent_spinner_surface_bounce(ModContext*, void* args, void*, void*) {
    auto* spinner = mods::arg<daSpinner_c*>(args, 0);
    if (!spinnerSurfaceBounceDisabled() || spinner == nullptr ||
        spinner->checkPathMoveNow() != nullptr ||
        (!spinner->mAcch.ChkWallHit() && !spinner->mAcch.ChkGroundHit()))
    {
        return HOOK_CONTINUE;
    }

    const s16 currentAngle = spinner->current.angle.y;
    const s16 surfaceAngle = spinner->field_0xa80;
    const f32 directionX = cM_ssin(currentAngle);
    const f32 directionZ = cM_scos(currentAngle);
    const f32 normalX = cM_ssin(surfaceAngle);
    const f32 normalZ = cM_scos(surfaceAngle);
    const f32 normalSpeed = directionX * normalX + directionZ * normalZ;
    const f32 slideX = directionX - normalSpeed * normalX;
    const f32 slideZ = directionZ - normalSpeed * normalZ;

    if (slideX * slideX + slideZ * slideZ < 0.001f) {
        const s16 positiveTangent = static_cast<s16>(surfaceAngle + 0x4000);
        const s16 negativeTangent = static_cast<s16>(surfaceAngle - 0x4000);
        const s16 positiveDelta = static_cast<s16>(positiveTangent - currentAngle);
        const s16 negativeDelta = static_cast<s16>(negativeTangent - currentAngle);
        const bool choosePositive =
            std::abs(static_cast<int>(positiveDelta)) <=
            std::abs(static_cast<int>(negativeDelta));
        spinner->current.angle.y = choosePositive ? positiveTangent : negativeTangent;
    } else {
        spinner->current.angle.y = cM_atan2s(slideX, slideZ);
    }

    if (spinner->mAcch.ChkWallHit()) {
        const f32 separationX = 6.0f * normalX;
        const f32 separationZ = 6.0f * normalZ;
        spinner->current.pos.x += separationX;
        spinner->current.pos.z += separationZ;
        spinner->old.pos.x += separationX;
        spinner->old.pos.z += separationZ;
    }

    return HOOK_SKIP_ORIGINAL;
}

HookAction keep_spinner_attack_always_active(
    ModContext*, void*, void* retval, void*)
{
    if (!spinnerAttackAlwaysActive()) {
        return HOOK_CONTINUE;
    }

    *static_cast<bool*>(retval) = true;
    return HOOK_SKIP_ORIGINAL;
}

void restore_shield_surf_hover_plane_board_common(
    ModContext*, void* args, void*, void*)
{
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink == nullptr) {
        return;
    }

    if (g_shieldSurfHoverPlaneGroundInjected) {
        if (!g_shieldSurfHoverPlaneGroundFindWasSet) {
            alink->mLinkAcch.ClrGroundFind();
        }
        if (!g_shieldSurfHoverPlaneGroundLandingWasSet) {
            alink->mLinkAcch.ClrGroundLanding();
        }
        if (!g_shieldSurfHoverPlaneGroundAwayWasSet) {
            alink->mLinkAcch.ClrGroundAway();
        }
        alink->mLinkAcch.ClrGroundHit();
        alink->mLinkAcch.m_ground_h = g_shieldSurfHoverPlanePreviousGroundHeight;
        alink->mLinkAcch.m_gnd.SetPolyInfo(g_shieldSurfPreviousGroundPolygon);
        g_shieldSurfHoverPlaneGroundInjected = false;
    }
}

void boost_shield_surf_speed(ModContext*, void* args, void*, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    const BOOL canMove = mods::arg<BOOL>(args, 1);
    if (!g_shieldSurfActive || !shieldSurfRBoostEnabled() || !canMove ||
        alink == nullptr || !alink->checkBoardRide() ||
        mDoCPd_c::getHoldR(PAD_1) == 0)
    {
        return;
    }

    cLib_chaseF(
        &alink->mNormalSpeed,
        alink->mMaxSpeed,
        daAlinkHIO_board_c0::m.mPushAccel);
}

HookAction draw_shield_surf_on_native_board(
    ModContext*, void* args, void* retval, void*)
{
    auto* board = mods::arg<daObjIceLeaf_c*>(args, 0);
    if (board == nullptr || fopAcM_GetID(board) != g_shieldSurfBoardId) {
        return HOOK_CONTINUE;
    }

    *static_cast<int*>(retval) = 1;
    if (!g_shieldSurfActive || board->mpModel == nullptr) {
        return HOOK_SKIP_ORIGINAL;
    }

    auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    if (alink == nullptr || !alink->checkBoardRide() ||
        dComIfGs_getSelectEquipShield() == dItemNo_NONE_e ||
        alink->mShieldModel == nullptr)
    {
        return HOOK_SKIP_ORIGINAL;
    }

    J3DModel* shield = alink->mShieldModel;
    Mtx originalMatrix;
    MTXCopy(shield->getBaseTRMtx(), originalMatrix);
    mDoMtx_stack_c::copy(board->mpModel->getBaseTRMtx());
    mDoMtx_stack_c::XrotM(0x4000);
    mDoMtx_stack_c::ZrotM(0x8000);
    Mtx surfMatrix;
    MTXCopy(mDoMtx_stack_c::get(), surfMatrix);
    surfMatrix[1][3] += kShieldSurfBoardRaise;
    shield->setBaseTRMtx(surfMatrix);

    dKy_tevstr_c tevStr{};
    g_env_light.settingTevStruct(0, &board->current.pos, &tevStr);
    g_env_light.setLightTevColorType_MAJI(shield, &tevStr);
    mDoExt_modelUpdateDL(shield);

    shield->setBaseTRMtx(originalMatrix);
    return HOOK_SKIP_ORIGINAL;
}
