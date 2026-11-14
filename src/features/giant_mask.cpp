constexpr u16 kGiantMaskModelResource = 4;
constexpr f32 kGiantMaskModelScaleX = 0.85f;
constexpr f32 kGiantMaskModelScaleY = 0.42f;
constexpr f32 kGiantMaskModelScaleZ = 0.35f;
constexpr f32 kGiantMaskFaceOffsetX = 14.0f;
constexpr f32 kGiantMaskFaceOffsetY = -8.0f;
constexpr s16 kGiantMaskFaceRotationY = 0x4000;
constexpr f32 kGiantMaskPlayerScale = 2.0f;

bool giant_mask_active_for_player(const daAlink_c* player) {
    return player != nullptr && player == daAlink_getAlinkActorClass() &&
           g_giantMaskActive && !player->checkWolf();
}

void update_giant_mask_resource() {
    if (!g_giantMaskActive || g_giantMaskArchiveLoaded || g_giantMaskLoadFailed) {
        return;
    }

    g_giantMaskPhaseRequested = true;
    const cPhs_Step phase = dComIfG_resLoad(&g_giantMaskPhase, "N_gD_mskF");
    if (phase == cPhs_COMPLEATE_e) {
        g_giantMaskArchiveLoaded = true;
        auto* modelData = static_cast<J3DModelData*>(
            dComIfG_getObjectRes("N_gD_mskF", kGiantMaskModelResource));
        if (modelData == nullptr) {
            svc_log->error(mod_ctx, "Failed to get the Giant's Mask model resource");
            g_giantMaskLoadFailed = true;
            return;
        }

        g_giantMaskHeap = JKRExpHeap::create(0x40000, mDoExt_getGameHeap(), true);
        if (g_giantMaskHeap == nullptr) {
            svc_log->error(mod_ctx, "Failed to create the Giant's Mask model heap");
            g_giantMaskLoadFailed = true;
            return;
        }

        JKRHeap* previousHeap = mDoExt_setCurrentHeap(g_giantMaskHeap);
        g_giantMaskModel =
            mDoExt_J3DModel__create(modelData, 0x80000, 0x11000084);
        mDoExt_setCurrentHeap(previousHeap);
        if (g_giantMaskModel == nullptr) {
            mDoExt_destroyExpHeap(g_giantMaskHeap);
            g_giantMaskHeap = nullptr;
            svc_log->error(mod_ctx, "Failed to create the Giant's Mask model");
            g_giantMaskLoadFailed = true;
            return;
        }
        svc_log->info(mod_ctx, "Giant's Mask model loaded from N_gD_mskF resource 4");
    } else if (phase == cPhs_ERROR_e || phase == cPhs_UNK3_e) {
        svc_log->error(mod_ctx, "Failed to load the Giant's Mask model archive");
        g_giantMaskLoadFailed = true;
    }
}

void update_giant_mask() {
    auto* player = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    if (player != nullptr && player->checkWolf()) {
        g_giantMaskActive = false;
    }
    if (g_giantMaskActive) {
        update_giant_mask_resource();
    }
}

void apply_giant_mask_player_scale(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (!giant_mask_active_for_player(player) || player->mpLinkModel == nullptr) {
        return;
    }

    MtxP matrix = player->mpLinkModel->getBaseTRMtx();
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            matrix[row][column] *= kGiantMaskPlayerScale;
        }
    }
    DCStoreRangeNoSync(matrix, sizeof(Mtx));
}

void draw_giant_mask_on_player(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (!giant_mask_active_for_player(player) || g_giantMaskModel == nullptr) {
        return;
    }

    mDoMtx_stack_c::copy(player->getHeadMtx());
    mDoMtx_stack_c::transM(
        kGiantMaskFaceOffsetX, kGiantMaskFaceOffsetY, 0.0f);
    mDoMtx_stack_c::YrotM(kGiantMaskFaceRotationY);
    mDoMtx_stack_c::scaleM(
        kGiantMaskModelScaleX, kGiantMaskModelScaleY, kGiantMaskModelScaleZ);
    g_giantMaskModel->setBaseTRMtx(mDoMtx_stack_c::get());

    dKy_tevstr_c tevStr{};
    g_env_light.settingTevStruct(0, &player->current.pos, &tevStr);
    g_env_light.setLightTevColorType_MAJI(g_giantMaskModel, &tevStr);
    mDoExt_modelUpdateDL(g_giantMaskModel);
}

void deactivate_giant_mask_after_scene_change(
    ModContext*, void*, void* retval, void*)
{
    if (retval != nullptr && *static_cast<int*>(retval) != 0) {
        g_giantMaskActive = false;
    }
}

DEFINE_HOOK(&daAlink_c::setMatrix, GiantMaskSetMatrix);
