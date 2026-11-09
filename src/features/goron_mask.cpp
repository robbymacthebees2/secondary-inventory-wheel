constexpr u16 kGoronMaskModelResource = 4;
constexpr f32 kGoronMaskModelScaleX = 0.85f;
constexpr f32 kGoronMaskModelScaleY = 0.42f;
constexpr f32 kGoronMaskModelScaleZ = 0.35f;
constexpr f32 kGoronMaskFaceOffsetX = 14.0f;
constexpr f32 kGoronMaskFaceOffsetY = -8.0f;
constexpr s16 kGoronMaskFaceRotationY = 0x4000;
constexpr f32 kGoronRollSpeedMultiplier = 4.0f;
constexpr f32 kGoronRollAttackRadius = 60.0f;
constexpr int kGoronRollAttackDamage = 2;

bool goron_mask_active_for_player(daAlink_c* player) {
    return g_goronMaskActive && player != nullptr && !player->checkWolf();
}

bool goron_mask_roll_available(daAlink_c* player) {
    return goron_mask_active_for_player(player) &&
           !player->checkEventRun() &&
           g_sitActionState == SitActionState::Inactive &&
           (player->mProcID == daAlink_c::PROC_WAIT ||
            player->mProcID == daAlink_c::PROC_MOVE);
}

void update_goron_mask_resource() {
    if (!g_goronMaskActive || g_goronMaskArchiveLoaded || g_goronMaskLoadFailed) {
        return;
    }

    g_goronMaskPhaseRequested = true;
    const cPhs_Step phase = dComIfG_resLoad(&g_goronMaskPhase, "N_gD_mskT");
    if (phase == cPhs_COMPLEATE_e) {
        g_goronMaskArchiveLoaded = true;
        auto* modelData = static_cast<J3DModelData*>(
            dComIfG_getObjectRes("N_gD_mskT", kGoronMaskModelResource));
        if (modelData == nullptr) {
            svc_log->error(mod_ctx, "Failed to get the clown mask model resource");
            g_goronMaskLoadFailed = true;
            return;
        }

        g_goronMaskHeap = JKRExpHeap::create(0x100000, mDoExt_getGameHeap(), true);
        if (g_goronMaskHeap == nullptr) {
            svc_log->error(mod_ctx, "Failed to create the clown mask model heap");
            g_goronMaskLoadFailed = true;
            return;
        }

        JKRHeap* previousHeap = mDoExt_setCurrentHeap(g_goronMaskHeap);
        g_goronMaskModel =
            mDoExt_J3DModel__create(modelData, J3DMdlFlag_DifferedDLBuffer, 0x11000084);
        mDoExt_setCurrentHeap(previousHeap);
        if (g_goronMaskModel == nullptr) {
            mDoExt_destroyExpHeap(g_goronMaskHeap);
            g_goronMaskHeap = nullptr;
            svc_log->error(mod_ctx, "Failed to create the clown mask model");
            g_goronMaskLoadFailed = true;
            return;
        }

        svc_log->info(mod_ctx, "Goron Mask clown model loaded");
    } else if (phase == cPhs_ERROR_e || phase == cPhs_UNK3_e) {
        svc_log->error(mod_ctx, "Failed to load the clown mask model archive");
        g_goronMaskLoadFailed = true;
    }
}

void update_goron_mask() {
    if (g_goronMaskInputBlockFrames > 0) {
        --g_goronMaskInputBlockFrames;
    }

    auto* player = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    if (player != nullptr && player->checkWolf()) {
        g_goronMaskActive = false;
    }
    if (g_goronMaskActive) {
        update_goron_mask_resource();
    }
}

bool start_goron_mask_roll(daAlink_c* player) {
    if (!goron_mask_roll_available(player) || g_secondaryRing != nullptr ||
        g_primaryRingOpen || mDoCPd_c::getHoldB(PAD_1) == 0)
    {
        return false;
    }

    player->procFrontRollInit();
    player->mUnderFrameCtrl[0].setRate(
        player->mUnderFrameCtrl[0].getRate() * kGoronRollSpeedMultiplier);
    player->mNormalSpeed *= kGoronRollSpeedMultiplier;
    player->mAtSph.SetAtType(AT_TYPE_NORMAL_SWORD);
    player->mAtSph.SetAtHitMark(1);
    player->mAtSph.SetAtAtp(kGoronRollAttackDamage);
    player->mAtSph.SetAtMtrl(dCcD_MTRL_NONE);
    player->mAtSph.SetR(kGoronRollAttackRadius);
    player->mAtSph.OnAtSetBit();
    return true;
}

HookAction begin_goron_mask_roll(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    start_goron_mask_roll(player);
    return HOOK_CONTINUE;
}

void continue_goron_mask_roll(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    start_goron_mask_roll(player);
}

void enable_goron_roll_attack(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (!goron_mask_active_for_player(player) ||
        player->mProcID != daAlink_c::PROC_FRONT_ROLL)
    {
        return;
    }

    player->mAtSph.SetAtType(AT_TYPE_NORMAL_SWORD);
    player->mAtSph.SetAtHitMark(1);
    player->mAtSph.SetAtAtp(kGoronRollAttackDamage);
    player->mAtSph.SetAtMtrl(dCcD_MTRL_NONE);
    player->mAtSph.SetR(kGoronRollAttackRadius);
    player->mAtSph.OnAtSetBit();
    cXyz center = player->current.pos;
    center.y += 55.0f;
    player->mAtSph.SetC(center);
    cXyz attackVector(
        player->mNormalSpeed * cM_ssin(player->shape_angle.y),
        0.0f,
        player->mNormalSpeed * cM_scos(player->shape_angle.y));
    player->mAtSph.SetAtVec(attackVector);
    dComIfG_Ccsp()->Set(&player->mAtSph);
    dComIfG_Ccsp()->SetMass(&player->mAtSph, 1);
}

HookAction suppress_goron_mask_sword_trigger(
    ModContext*, void* args, void* retval, void*)
{
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (mods::arg<u8>(args, 1) == daAlink_c::BTN_B &&
        goron_mask_active_for_player(player))
    {
        *static_cast<BOOL*>(retval) = false;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

HookAction suppress_goron_mask_sword_button(
    ModContext*, void* args, void* retval, void*)
{
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (mods::arg<u8>(args, 1) == daAlink_c::BTN_B &&
        goron_mask_active_for_player(player))
    {
        *static_cast<BOOL*>(retval) = false;
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

HookAction set_goron_mask_roll_action(ModContext*, void* args, void*, void*) {
    auto* player = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    if (goron_mask_active_for_player(player)) {
        mods::arg_ref<u8>(args, 1) = 0x81;
    }
    return HOOK_CONTINUE;
}

void load_goron_mask_hud_icon() {
    if (g_goronMaskHudIcon != nullptr || g_goronMaskHudIconLoadFailed) {
        return;
    }

    ResourceBuffer icon = RESOURCE_BUFFER_INIT;
    const ModResult result =
        svc_resource->load(mod_ctx, kGoronMaskIconResource, &icon);
    if (result != MOD_OK || icon.data == nullptr || icon.size == 0) {
        svc_log->error(mod_ctx, "failed to load the Goron Mask B-button icon");
        svc_resource->free(mod_ctx, &icon);
        g_goronMaskHudIconLoadFailed = true;
        return;
    }
    if (icon.size < sizeof(ResTIMG) || icon.size > kItemTextureBufferSize) {
        svc_log->error(mod_ctx, "Goron Mask B-button icon has an invalid texture size");
        svc_resource->free(mod_ctx, &icon);
        g_goronMaskHudIconLoadFailed = true;
        return;
    }

    g_goronMaskHudIconHeap = JKRExpHeap::create(0x2000, mDoExt_getGameHeap(), true);
    if (g_goronMaskHudIconHeap == nullptr) {
        svc_log->error(mod_ctx, "failed to create the Goron Mask B-button icon heap");
        svc_resource->free(mod_ctx, &icon);
        g_goronMaskHudIconLoadFailed = true;
        return;
    }
    g_goronMaskHudIcon = static_cast<ResTIMG*>(
        g_goronMaskHudIconHeap->alloc(kItemTextureBufferSize, 0x20));
    if (g_goronMaskHudIcon == nullptr) {
        svc_log->error(mod_ctx, "failed to allocate the Goron Mask B-button icon");
        mDoExt_destroyExpHeap(g_goronMaskHudIconHeap);
        g_goronMaskHudIconHeap = nullptr;
        svc_resource->free(mod_ctx, &icon);
        g_goronMaskHudIconLoadFailed = true;
        return;
    }
    std::memset(g_goronMaskHudIcon, 0, kItemTextureBufferSize);
    std::memcpy(g_goronMaskHudIcon, icon.data, icon.size);
    svc_resource->free(mod_ctx, &icon);
    DCStoreRangeNoSync(g_goronMaskHudIcon, kItemTextureBufferSize);
}

void replace_goron_mask_b_icon(dMeter2Draw_c* meter) {
    if (meter == nullptr) {
        return;
    }
    load_goron_mask_hud_icon();
    if (g_goronMaskHudIcon == nullptr) {
        return;
    }

    const int bufferIndex = meter->field_0x76b;
    if (bufferIndex > 1) {
        return;
    }
    ResTIMG* primary = meter->mpItemBTex[bufferIndex][0];
    ResTIMG* secondary = meter->mpItemBTex[bufferIndex][1];
    if (primary == nullptr || secondary == nullptr) {
        return;
    }

    std::memcpy(primary, g_goronMaskHudIcon, kItemTextureBufferSize);
    std::memcpy(secondary, g_goronMaskHudIcon, kItemTextureBufferSize);
    DCStoreRangeNoSync(primary, kItemTextureBufferSize);
    DCStoreRangeNoSync(secondary, kItemTextureBufferSize);
    if (meter->mpItemB != nullptr) {
        static_cast<J2DPicture*>(meter->mpItemB->getPanePtr())->changeTexture(primary, 0);
        meter->mpItemB->show();
    }
    if (meter->mpItemBPane != nullptr) {
        meter->mpItemBPane->changeTexture(secondary, 0);
        meter->mpItemBPane->show();
    }
}

void show_goron_mask_roll_prompt(ModContext*, void* args, void*, void*) {
    auto* player = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
    if (meter == nullptr) {
        return;
    }

    if (!goron_mask_active_for_player(player)) {
        const u8 action = mods::arg<u8>(args, 1);
        char* actionText = meter->getActionString(
            action, 1, &meter->field_0x762);
        if (actionText != nullptr) {
            for (int index = 0; index < 5; ++index) {
                if (meter->mpBText[index] != nullptr) {
                    static_cast<J2DTextBox*>(
                        meter->mpBText[index]->getPanePtr())
                        ->setString(64, actionText);
                }
            }

            const u8 doStatus = dComIfGp_getDoStatus();
            const bool showAction =
                *actionText != '\0' && action != 0 && action != 0x2E &&
                ((doStatus == 0 ||
                  (doStatus != dComIfGp_getAStatus() &&
                   doStatus != dComIfGp_getAStatusForce())) ||
                 !meter->isEmphasisB());
            if (meter->mpTextB != nullptr) {
                if (showAction) {
                    meter->mpTextB->show();
                } else {
                    meter->mpTextB->hide();
                }
            }
        }
        if (g_goronMaskHudIconApplied) {
            meter->changeTextureItemB(dComIfGs_getSelectEquipSword());
            g_goronMaskHudIconApplied = false;
        }
        return;
    }
    if (meter->mpTextB == nullptr) {
        return;
    }

    for (int index = 0; index < 5; ++index) {
        if (meter->mpBText[index] != nullptr) {
            static_cast<J2DTextBox*>(meter->mpBText[index]->getPanePtr())
                ->setString(64, "Goron Roll");
        }
    }
    replace_goron_mask_b_icon(meter);
    g_goronMaskHudIconApplied = g_goronMaskHudIcon != nullptr;
    meter->mpTextB->show();
}

void suppress_goron_mask_sword_item_input(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (goron_mask_active_for_player(player)) {
        player->mItemTrigger &= static_cast<u8>(~daAlink_c::BTN_B);
        player->mItemButton &= static_cast<u8>(~daAlink_c::BTN_B);
    }
}

void draw_goron_mask_model(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (!g_goronMaskActive || player == nullptr || player->checkWolf() ||
        g_goronMaskModel == nullptr)
    {
        return;
    }

    mDoMtx_stack_c::copy(player->getHeadMtx());
    mDoMtx_stack_c::transM(
        kGoronMaskFaceOffsetX, kGoronMaskFaceOffsetY, 0.0f);
    mDoMtx_stack_c::YrotM(kGoronMaskFaceRotationY);
    mDoMtx_stack_c::scaleM(
        kGoronMaskModelScaleX, kGoronMaskModelScaleY, kGoronMaskModelScaleZ);
    g_goronMaskModel->setBaseTRMtx(mDoMtx_stack_c::get());

    dKy_tevstr_c tevStr{};
    g_env_light.settingTevStruct(0, &player->current.pos, &tevStr);
    g_env_light.setLightTevColorType_MAJI(g_goronMaskModel, &tevStr);
    mDoExt_modelUpdateDL(g_goronMaskModel);
}

DEFINE_HOOK(&daAlink_c::itemTriggerCheck, ItemTriggerCheck);
DEFINE_HOOK(&daAlink_c::itemButtonCheck, ItemButtonCheck);
DEFINE_HOOK(&dMeter2Draw_c::drawButtonB, DrawButtonB);
