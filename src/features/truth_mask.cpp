constexpr u16 kTruthMaskModelResource = 4;
constexpr f32 kTruthMaskModelScaleX = 0.85f;
constexpr f32 kTruthMaskModelScaleY = 0.42f;
constexpr f32 kTruthMaskModelScaleZ = 0.35f;
constexpr f32 kTruthMaskFaceOffsetX = 14.0f;
constexpr f32 kTruthMaskFaceOffsetY = -8.0f;
constexpr s16 kTruthMaskFaceRotationY = 0x4000;

bool truth_mask_stealth_active() {
    auto* player = daAlink_getAlinkActorClass();
    return g_truthMaskActive && player != nullptr && !player->checkWolf();
}

bool is_enemy_query_for_player(void* args) {
    auto* actor = mods::arg<fopAc_ac_c*>(args, 0);
    auto* target = mods::arg<fopAc_ac_c*>(args, 1);
    return truth_mask_stealth_active() && actor != nullptr &&
           actor->group == fopAc_ENEMY_e &&
           target == static_cast<fopAc_ac_c*>(dComIfGp_getPlayer(0));
}

HookAction hide_truth_masked_player_from_enemy(
    ModContext*, void* args, void* retval, void*)
{
    if (retval != nullptr && is_enemy_query_for_player(args)) {
        *static_cast<f32*>(retval) = 1000000000.0f;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

HookAction hide_truth_masked_player_angle_from_enemy(
    ModContext*, void* args, void* retval, void*)
{
    if (retval != nullptr && is_enemy_query_for_player(args)) {
        *static_cast<s16*>(retval) = 0x7fff;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

HookAction hide_truth_masked_player_seen_angle_from_enemy(
    ModContext*, void* args, void* retval, void*)
{
    if (retval != nullptr && is_enemy_query_for_player(args)) {
        *static_cast<s32*>(retval) = 0x7fff;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

void update_truth_mask_resource() {
    auto* player = daAlink_getAlinkActorClass();
    if (player != nullptr && player->checkWolf()) {
        g_truthMaskActive = false;
    }
    if ((!g_truthMaskActive && !g_truthMaskPhaseRequested) ||
        g_truthMaskArchiveLoaded || g_truthMaskLoadFailed)
    {
        return;
    }

    g_truthMaskPhaseRequested = true;
    const cPhs_Step phase = dComIfG_resLoad(&g_truthMaskPhase, "N_gD_mskF");
    if (phase == cPhs_COMPLEATE_e) {
        g_truthMaskArchiveLoaded = true;
        auto* modelData = static_cast<J3DModelData*>(
            dComIfG_getObjectRes("N_gD_mskF", kTruthMaskModelResource));
        if (modelData == nullptr) {
            svc_log->error(mod_ctx, "Failed to get the Stone Mask model resource");
            g_truthMaskLoadFailed = true;
            return;
        }

        g_truthMaskHeap = JKRExpHeap::create(0x40000, mDoExt_getGameHeap(), true);
        if (g_truthMaskHeap == nullptr) {
            svc_log->error(mod_ctx, "Failed to create the Stone Mask model heap");
            g_truthMaskLoadFailed = true;
            return;
        }

        JKRHeap* previousHeap = mDoExt_setCurrentHeap(g_truthMaskHeap);
        g_truthMaskModel =
            mDoExt_J3DModel__create(modelData, 0x80000, 0x11000084);
        mDoExt_setCurrentHeap(previousHeap);
        if (g_truthMaskModel == nullptr) {
            mDoExt_destroyExpHeap(g_truthMaskHeap);
            g_truthMaskHeap = nullptr;
            svc_log->error(mod_ctx, "Failed to create the Stone Mask model");
            g_truthMaskLoadFailed = true;
            return;
        }
        svc_log->info(mod_ctx, "Stone Mask model loaded");
    } else if (phase == cPhs_ERROR_e || phase == cPhs_UNK3_e) {
        svc_log->error(mod_ctx, "Failed to load the Stone Mask model archive");
        g_truthMaskLoadFailed = true;
    }
}

void draw_truth_mask_on_player(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (!truth_mask_stealth_active() || player == nullptr ||
        player->checkWolf() || g_truthMaskModel == nullptr)
    {
        return;
    }

    mDoMtx_stack_c::copy(player->getHeadMtx());
    mDoMtx_stack_c::transM(
        kTruthMaskFaceOffsetX, kTruthMaskFaceOffsetY, 0.0f);
    mDoMtx_stack_c::YrotM(kTruthMaskFaceRotationY);
    mDoMtx_stack_c::scaleM(
        kTruthMaskModelScaleX, kTruthMaskModelScaleY, kTruthMaskModelScaleZ);
    g_truthMaskModel->setBaseTRMtx(mDoMtx_stack_c::get());

    dKy_tevstr_c tevStr{};
    g_env_light.settingTevStruct(0, &player->current.pos, &tevStr);
    g_env_light.setLightTevColorType_MAJI(g_truthMaskModel, &tevStr);
    mDoExt_modelUpdateDL(g_truthMaskModel);
}

DEFINE_HOOK(&fopAcM_searchActorAngleY, SearchActorAngleY);
DEFINE_HOOK(&fopAcM_searchActorDistance, SearchActorDistance);
DEFINE_HOOK(&fopAcM_searchActorDistance2, SearchActorDistance2);
DEFINE_HOOK(&fopAcM_searchActorDistanceXZ, SearchActorDistanceXZ);
DEFINE_HOOK(&fopAcM_searchActorDistanceXZ2, SearchActorDistanceXZ2);
DEFINE_HOOK(&fopAcM_seenActorAngleY, SeenActorAngleY);
