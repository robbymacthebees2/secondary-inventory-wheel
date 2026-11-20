namespace unrestricted_items {
namespace {

DEFINE_HOOK(&daAlink_c::checkAcceptUseItemInWater, CheckAcceptUseItemInWater);
DEFINE_HOOK(&daAlink_c::checkCastleTownUseItem, CheckCastleTownUseItem);
DEFINE_HOOK(&daAlink_c::checkNotBattleStage, CheckNotBattleStage);
DEFINE_HOOK(&daAlink_c::checkItemAction, CheckItemAction);
DEFINE_HOOK(&daAlink_c::checkItemChangeFromButton, CheckItemChangeFromButton);
DEFINE_HOOK(&daAlink_c::swimDeleteItem, SwimDeleteItem);
DEFINE_HOOK(&daAlink_c::checkWaterInKandelaar, CheckWaterInKandelaar);
DEFINE_HOOK(&daAlink_c::checkKandelaarSwing, CheckKandelaarSwing);
DEFINE_HOOK(&daAlink_c::initKandelaarSwing, InitKandelaarSwing);
DEFINE_HOOK(&daAlink_c::checkNewItemChange, CheckNewItemChange);
DEFINE_HOOK(&daAlink_c::changeItemTriggerKeepProc, ChangeItemTriggerKeepProc);
DEFINE_HOOK(&daAlink_c::checkNoSubjectModeCamera, CheckNoSubjectModeCamera);
DEFINE_HOOK(&daAlink_c::checkNotHeavyBootsStage, CheckNotHeavyBootsStage);
DEFINE_HOOK(&daAlink_c::checkHookshotStickBG, CheckHookshotStickBG);
DEFINE_HOOK(&daAlink_c::checkRoomOnly, CheckRoomOnly);
DEFINE_HOOK(&daAlink_c::procGrassWhistleWait, ProcGrassWhistleWait);
DEFINE_HOOK(&daAlink_c::setLight, SetLight);
DEFINE_HOOK(&dCamera_c::ChangeModeOK, ChangeModeOK);
DEFINE_HOOK(&dCamera_c::Run, CameraRun);
DEFINE_HOOK(&dMeter2_c::alphaAnimeKantera, AlphaAnimeKantera);

enum ItemProc {
    ITEM_PROC_NONE = 0,
    ITEM_PROC_BOOTS_EQUIP = 1,
    ITEM_PROC_SET_HEAVY_BOOTS = 2,
    ITEM_PROC_BOTTLE_DRINK = 3,
    ITEM_PROC_SPINNER_READY = 4,
    ITEM_PROC_DUNGEON_WARP_READY = 5,
    ITEM_PROC_BOTTLE_OPEN = 6,
    ITEM_PROC_FISHING_FOOD = 7,
    ITEM_PROC_LANTERN_POUR = 8,
    ITEM_PROC_SUBJECTIVITY = 9,
    ITEM_PROC_PICK_PUT = 10,
    ITEM_PROC_OFF_LANTERN = 11,
    ITEM_PROC_COMMON_CHANGE_ITEM = 12,
    ITEM_PROC_BOTTLE_SWING = 13,
    ITEM_PROC_NOT_USE_ITEM = 14,
    ITEM_PROC_GRASS_WHISTLE = 15,
};

struct CameraStyleRestore {
    dCamera_c* camera;
    int cameraType;
    int playerStyle;
    int mode;
    s16 style;
};

struct CameraFlagRestore {
    dCamera_c* camera;
    u8 flags;
};

struct OilRestore {
    daAlink_c* player;
    s32 oil;
    s32 bottleOil;
    s32 currentBottleOil;
};

std::vector<CameraStyleRestore> g_cameraStyleStack;
std::vector<CameraFlagRestore> g_cameraFlagStack;
std::vector<daAlink_c*> g_lanternLightRestoreStack;
std::vector<OilRestore> g_lanternOilRestoreStack;
const HookOptions kOverrideReplaceOptions = {
    sizeof(HookOptions), INT32_MAX, HOOK_REPLACE_OVERRIDE, nullptr};

bool in_water(const daAlink_c* player) {
    return player != nullptr &&
        (player->checkNoResetFlg0(daAlink_c::FLG0_WATER_IN_MOVE) ||
         player->checkModeFlg(0x40000));
}

bool camera_stage() {
    return daAlink_c::checkStageName("F_SP116") ||
        daAlink_c::checkStageName("R_SP160");
}

bool special_no_battle_room() {
    return daAlink_c::checkRoomSpecial() ||
        (daAlink_c::checkStageName("R_SP161") &&
         !dComIfGs_isOneZoneSwitch(14, -1));
}

bool lantern_water_depth(const daAlink_c* player, f32 waterY) {
    const f32 baseY = player->checkModeFlg(0x40)
        ? player->mRightFootPos.y : player->current.pos.y;
    return waterY > baseY + 65.0f;
}

bool lantern_submerged(const daAlink_c* player) {
    return in_water(player) && player->mEquipItem == dItemNo_KANTERA_e &&
        lantern_water_depth(player, player->mWaterY);
}

bool camera_mode_is_subjective(s32 mode) {
    return mode == 4 || mode == 7 || mode == 8;
}

bool native_hookshot_camera_proc(const daAlink_c* player) {
    if (player == nullptr) {
        return false;
    }
    switch (player->mProcID) {
    case daAlink_c::PROC_HOOKSHOT_ROOF_WAIT:
    case daAlink_c::PROC_HOOKSHOT_ROOF_SHOOT:
    case daAlink_c::PROC_HOOKSHOT_ROOF_BOOTS:
    case daAlink_c::PROC_HOOKSHOT_WALL_WAIT:
    case daAlink_c::PROC_HOOKSHOT_WALL_SHOOT:
    case daAlink_c::PROC_HOOKSHOT_FLY:
        return true;
    default:
        return false;
    }
}

bool camera_type_valid(const dCamera_c* camera, int type) {
    return camera != nullptr && type >= 0 && type < camera->mCamTypeNum;
}

int camera_style_index(const dCamera_c* camera, int type, int fieldType) {
    return camera->mCamTypeData[type].field_0x18[camera->mIsWolf][0] >= 0 &&
            camera->mCamTypeData[fieldType].field_0x18[camera->mIsWolf][0] >= 0
        ? camera->mIsWolf : 0;
}

void copy_camera_style(dCamera_c* camera, int type, int fieldType, int mode) {
    if (!camera_type_valid(camera, type) ||
        !camera_type_valid(camera, fieldType) || mode < 0 || mode >= 10)
    {
        return;
    }

    const int styleIndex = camera_style_index(camera, type, fieldType);
    auto& style = camera->mCamTypeData[type].field_0x18[styleIndex][mode];
    const s16 fallback = camera->mCamTypeData[fieldType]
        .field_0x18[styleIndex][mode];
    if (fallback < 0 || style == fallback) {
        return;
    }
    g_cameraStyleStack.push_back({camera, type, styleIndex, mode, style});
    style = fallback;
}

void restore_camera_state(dCamera_c* camera) {
    while (!g_cameraFlagStack.empty() &&
           g_cameraFlagStack.back().camera == camera)
    {
        const CameraFlagRestore saved = g_cameraFlagStack.back();
        g_cameraFlagStack.pop_back();
        camera->mTagCamTool.mFlags = saved.flags;
    }
    while (!g_cameraStyleStack.empty() &&
           g_cameraStyleStack.back().camera == camera)
    {
        const CameraStyleRestore saved = g_cameraStyleStack.back();
        g_cameraStyleStack.pop_back();
        camera->mCamTypeData[saved.cameraType]
            .field_0x18[saved.playerStyle][saved.mode] = saved.style;
    }
}

int unrestricted_item_change(daAlink_c* player, u8 slot, u16 item) {
    if (player == nullptr || player->checkSpinnerRide() ||
        item == dItemNo_BOMB_BAG_LV1_e)
    {
        return ITEM_PROC_NONE;
    }

    const bool waterMode = in_water(player);
    if (waterMode && !player->checkAcceptUseItemInWater(item)) {
        return ITEM_PROC_NONE;
    }
    if (player->checkModeFlg(0x40000) && item == dItemNo_WATER_BOMB_e) {
        return ITEM_PROC_NONE;
    }

    if (player->checkModeFlg(0x400) &&
        (item == dItemNo_COPY_ROD_e || daAlink_c::checkFishingRodItem(item)))
    {
        return ITEM_PROC_NONE;
    }

    const bool itemNeedsSpecialChange =
        item == dItemNo_HVY_BOOTS_e || player->checkDungeonWarpItem(item) ||
        player->checkTradeItem(item) ||
        (player->checkBottleItem(item) && item != dItemNo_EMPTY_BOTTLE_e) ||
        item == dItemNo_SPINNER_e || item == dItemNo_POKE_BOMB_e ||
        item == dItemNo_HORSE_FLUTE_e || item == dItemNo_HAWK_EYE_e ||
        item == dItemNo_WORM_e || item == dItemNo_BEE_CHILD_e;
    if (item != dItemNo_NONE_e && itemNeedsSpecialChange &&
        (player->checkReinRide() || player->checkCanoeRide()))
    {
        if (player->checkDrinkBottleItem(item)) {
            return ITEM_PROC_BOTTLE_DRINK;
        }
        if (daAlink_c::checkOilBottleItem(item) &&
            player->checkItemSetButton(0x48) != 2)
        {
            return ITEM_PROC_LANTERN_POUR;
        }
        if (player->checkReinRide() && item == dItemNo_HAWK_EYE_e &&
            player->acceptSubjectModeChange())
        {
            return ITEM_PROC_SUBJECTIVITY;
        }
        return ITEM_PROC_NONE;
    }

    if (item != dItemNo_NONE_e && itemNeedsSpecialChange)
    {
        if (player->mLinkAcch.ChkGroundHit() &&
            !player->checkModeFlg(0x70C52))
        {
            if (item == dItemNo_SPINNER_e) {
                return ITEM_PROC_SPINNER_READY;
            }
            if (player->checkDungeonWarpItem(item)) {
                return ITEM_PROC_DUNGEON_WARP_READY;
            }
            if (item == dItemNo_HAWK_EYE_e &&
                player->acceptSubjectModeChange())
            {
                return ITEM_PROC_SUBJECTIVITY;
            }
            if (item == dItemNo_HORSE_FLUTE_e) {
                return ITEM_PROC_GRASS_WHISTLE;
            }
            if (player->checkDrinkBottleItem(item)) {
                return ITEM_PROC_BOTTLE_DRINK;
            }
            if (player->checkOpenBottleItem(item)) {
                return ITEM_PROC_BOTTLE_OPEN;
            }
            if (player->checkTradeItem(item)) {
                return ITEM_PROC_NOT_USE_ITEM;
            }
            if (item == dItemNo_POKE_BOMB_e &&
                dComIfGp_getSelectItemNum(slot) && player->field_0x2fcf < 2)
            {
                return ITEM_PROC_PICK_PUT;
            }
            if (item == dItemNo_HVY_BOOTS_e) {
                if (!player->checkBoardRide()) {
                    if ((player->mLinkAcch.ChkGroundHit() &&
                         !player->checkModeFlg(0x70C52)) ||
                        (player->checkMagneBootsOn() &&
                         cBgW_CheckBGround(player->mMagneBootsTopVec.y)) ||
                        player->mProcID == daAlink_c::PROC_HANG_CLIMB)
                    {
                        return ITEM_PROC_BOOTS_EQUIP;
                    }
                    return ITEM_PROC_SET_HEAVY_BOOTS;
                }
            }
            if (daAlink_c::checkOilBottleItem(item) &&
                player->checkItemSetButton(0x48) != 2)
            {
                return ITEM_PROC_LANTERN_POUR;
            }
            if (item == dItemNo_WORM_e || item == dItemNo_BEE_CHILD_e) {
                const u16 rod = dComIfGp_getSelectItem(
                    player->checkItemSetButton(0x108));
                if (rod == dItemNo_WORM_ROD_e ||
                    rod == dItemNo_JEWEL_WORM_ROD_e)
                {
                    return item == dItemNo_BEE_CHILD_e
                        ? ITEM_PROC_BOTTLE_DRINK : ITEM_PROC_NONE;
                }
                if (item == dItemNo_BEE_CHILD_e &&
                    (rod == dItemNo_BEE_ROD_e ||
                     rod == dItemNo_JEWEL_BEE_ROD_e))
                {
                    return ITEM_PROC_BOTTLE_DRINK;
                }
                return ITEM_PROC_FISHING_FOOD;
            }
        } else if (item == dItemNo_HVY_BOOTS_e) {
            return ITEM_PROC_NONE;
        }
        if (player->checkMagneBootsOn() &&
            player->checkDrinkBottleItem(item) &&
            cBgW_CheckBGround(player->mMagneBootsTopVec.y))
        {
            return ITEM_PROC_BOTTLE_DRINK;
        }
    }

    if (player->mEquipItem != item && item != dItemNo_NONE_e) {
        const bool noBombs = player->checkBombItem(item) &&
            !dComIfGp_getSelectItemNum(slot);
        const bool bombLimit =
            (item == dItemNo_NORMAL_BOMB_e || item == dItemNo_WATER_BOMB_e) &&
            player->mActiveBombNum >= 3;
        const bool ironBallBlocked = item == dItemNo_IRONBALL_e &&
            (!player->mLinkAcch.ChkGroundHit() ||
             player->checkModeFlg(0x70C52));
        const bool lanternBlocked = item == dItemNo_KANTERA_e &&
            (player->checkEndResetFlg1(daAlink_c::ERFLG1_UNK_4) ||
             (waterMode && !lantern_submerged(player)));
        if (noBombs || bombLimit || ironBallBlocked || lanternBlocked) {
            return ITEM_PROC_NONE;
        }
        return ITEM_PROC_COMMON_CHANGE_ITEM;
    }

    if (player->mEquipItem == item && player->mSelectItemId != slot &&
        item == dItemNo_EMPTY_BOTTLE_e)
    {
        return ITEM_PROC_BOTTLE_SWING;
    }
    return ITEM_PROC_NONE;
}

HookAction camera_run_pre(ModContext*, void* args, void*, void*) {
    auto* camera = mods::arg<dCamera_c*>(args, 0);
    if (camera == nullptr || !castleTownFirstPersonEnabled() ||
        !camera_stage())
    {
        return HOOK_CONTINUE;
    }

    auto* player = static_cast<daAlink_c*>(daPy_getPlayerActorClass());
    if (native_hookshot_camera_proc(player) &&
        (camera->mTagCamTool.mFlags & 0x10) == 0)
    {
        g_cameraFlagStack.push_back({camera, camera->mTagCamTool.mFlags});
        camera->mTagCamTool.mFlags |= 0x10;
    }

    const int fieldType = camera->GetCameraTypeFromCameraName("FieldS");
    if (fieldType < 0) {
        return HOOK_CONTINUE;
    }
    const int scopeType = camera->GetCameraTypeFromCameraName("Scope");
    const int hookWallType = camera->GetCameraTypeFromCameraName("HookWall");
    const int hookRoofType = camera->GetCameraTypeFromCameraName("HookRoof");
    const int hookActorType = camera->GetCameraTypeFromCameraName("HookActor");

    if (player != nullptr && player->mProcID == daAlink_c::PROC_HOOKSHOT_FLY) {
        copy_camera_style(camera, camera->mCurType, fieldType, 9);
        copy_camera_style(camera, camera->mMapToolType, fieldType, 9);
        if (camera->mRoomMapTool.mCameraIndex != 0xff) {
            const int roomType = camera->GetCameraTypeFromToolData(
                &camera->mRoomMapTool.mCamData);
            copy_camera_style(camera, roomType, fieldType, 9);
        }
    }
    if (camera->mCurType == scopeType || camera->mCurType == hookWallType ||
        camera->mCurType == hookRoofType || camera->mCurType == hookActorType)
    {
        return HOOK_CONTINUE;
    }
    for (const int mode : {4, 7, 8}) {
        if (camera_mode_is_subjective(mode)) {
            copy_camera_style(camera, camera->mCurType, fieldType, mode);
        }
    }
    return HOOK_CONTINUE;
}

void camera_run_post(ModContext*, void* args, void*, void*) {
    restore_camera_state(mods::arg<dCamera_c*>(args, 0));
}

HookAction check_water_in_lantern_pre(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    const f32 waterY = mods::arg<f32>(args, 1);
    return player != nullptr && player->mEquipItem == dItemNo_KANTERA_e &&
            lantern_water_depth(player, waterY) && in_water(player) &&
            player->checkNoResetFlg2(daAlink_c::FLG2_UNK_1)
        ? HOOK_SKIP_ORIGINAL : HOOK_CONTINUE;
}

void check_lantern_swing_post(ModContext*, void* args, void* retval, void*) {
    auto* player = mods::arg<const daAlink_c*>(args, 0);
    const int allowUnlit = mods::arg<int>(args, 1);
    if (player != nullptr && allowUnlit == 0 &&
        *static_cast<bool*>(retval) && lantern_submerged(player))
    {
        *static_cast<bool*>(retval) = false;
    }
}

HookAction begin_lantern_light_restore(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (player != nullptr && lantern_submerged(player) &&
        !player->checkNoResetFlg2(daAlink_c::FLG2_KANDELAAR_LIGHT_OFF))
    {
        player->onNoResetFlg2(daAlink_c::FLG2_KANDELAAR_LIGHT_OFF);
        g_lanternLightRestoreStack.push_back(player);
    }
    return HOOK_CONTINUE;
}

void end_lantern_light_restore(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (!g_lanternLightRestoreStack.empty() &&
        g_lanternLightRestoreStack.back() == player)
    {
        player->offNoResetFlg2(daAlink_c::FLG2_KANDELAAR_LIGHT_OFF);
        g_lanternLightRestoreStack.pop_back();
    }
}

HookAction save_lantern_oil(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (player != nullptr && player->mEquipItem == dItemNo_KANTERA_e &&
        lantern_submerged(player) && !player->checkEventRun())
    {
        g_lanternOilRestoreStack.push_back({
            player, dComIfGs_getOil(), dComIfGp_getItemOilCount(),
            dComIfGp_getItemNowOil()});
    }
    return HOOK_CONTINUE;
}

void restore_lantern_oil(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (g_lanternOilRestoreStack.empty() ||
        g_lanternOilRestoreStack.back().player != player)
    {
        return;
    }
    const OilRestore saved = g_lanternOilRestoreStack.back();
    g_lanternOilRestoreStack.pop_back();
    const s32 oilDelta = saved.oil - dComIfGs_getOil();
    const s32 bottleDelta = saved.bottleOil - dComIfGp_getItemOilCount();
    if (oilDelta != 0) {
        dComIfGs_setOil(saved.oil);
    }
    if (bottleDelta != 0) {
        dComIfGp_setItemOilCount(bottleDelta);
    }
    dComIfGp_setItemNowOil(saved.currentBottleOil);
}

void allow_lantern_gauge_underwater(ModContext*, void* args, void*, void*) {
    auto* meter = mods::arg<dMeter2_c*>(args, 0);
    auto* player = static_cast<daAlink_c*>(daPy_getPlayerActorClass());
    if (meter != nullptr && player != nullptr && lantern_submerged(player)) {
        meter->getMeterDrawPtr()->setAlphaKanteraAnimeMin();
        meter->getMeterDrawPtr()->setAlphaKanteraChange(true);
    } else if (meter != nullptr) {
        AlphaAnimeKantera::g_orig(meter);
    }
}

void preserve_underwater_lantern(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    const bool keepLantern = player != nullptr &&
        player->mEquipItem == dItemNo_KANTERA_e && in_water(player);
    if (player == nullptr) {
        return;
    }
    if (!player->checkHookshotItem(player->mEquipItem) && !keepLantern &&
        (player->mEquipItem != 0x103 || !player->checkBootsOrArmorHeavy()))
    {
        player->deleteEquipItem(TRUE, TRUE);
    }
    if (!keepLantern && player->checkNoResetFlg2(daAlink_c::FLG2_UNK_1)) {
        player->offKandelaarModel();
    }
}

HookAction allow_hawkeye_on_horse(ModContext*, void* args, void* retval, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    const u8 slot = mods::arg<u8>(args, 1);
    const int proc = mods::arg<int>(args, 2);
    if (player == nullptr || !player->checkReinRide() ||
        proc != ITEM_PROC_SUBJECTIVITY)
    {
        return HOOK_CONTINUE;
    }
    player->mSelectItemId = slot;
    player->procHorseSubjectivityInit();
    dComIfGp_setPlayerStatus0(0, 0x200000);
    player->seStartSystem(Z2SE_AL_HAWK_EYE_PUTON);
    *static_cast<int*>(retval) = 1;
    return HOOK_SKIP_ORIGINAL;
}

void camera_mode_allowed(ModContext*, void* args, void* retval, void*) {
    auto* camera = mods::arg<dCamera_c*>(args, 0);
    const s32 mode = mods::arg<s32>(args, 1);
    auto& allowed = *static_cast<bool*>(retval);
    if (camera == nullptr) {
        allowed = false;
        return;
    }

    allowed = ChangeModeOK::g_orig(camera, mode);
    if (!allowed && camera_mode_is_subjective(mode) &&
        castleTownFirstPersonEnabled() && camera_stage())
    {
        const int fieldType = camera->GetCameraTypeFromCameraName("FieldS");
        if (camera_type_valid(camera, fieldType) &&
            camera_type_valid(camera, camera->mCurType))
        {
            const int index = camera_style_index(
                camera, camera->mCurType, fieldType);
            allowed = camera->mCamTypeData[fieldType]
                .field_0x18[index][mode] >= 0;
        }
    }
}

}  // namespace

ModResult initialize() {
    const HookOptions* overrideOptions = &kOverrideReplaceOptions;
    ModResult hookResult = MOD_OK;
    auto install = [&hookResult](ModResult result, const char* label) {
        if (result != MOD_OK) {
            svc_log->error(mod_ctx, label);
            hookResult = result;
        }
    };

    install(mods::hook::replace<CheckAcceptUseItemInWater>(
        svc_hook, [](ModContext*, void* args, void* retval, void*) {
            auto* player = mods::arg<const daAlink_c*>(args, 0);
            auto& accepted = *static_cast<BOOL*>(retval);
            accepted = player != nullptr && in_water(player)
                ? TRUE : CheckAcceptUseItemInWater::g_orig(player,
                    mods::arg<u16>(args, 1));
        }), "Failed to relax underwater item checks");
    install(mods::hook::replace<CheckCastleTownUseItem>(
        svc_hook, [](ModContext*, void* args, void* retval, void*) {
            (void)args;
            *static_cast<bool*>(retval) = true;
        }, overrideOptions), "Failed to allow items in Castle Town");
    install(mods::hook::replace<CheckNotBattleStage>(
        svc_hook, [](ModContext*, void*, void* retval, void*) {
            if (daAlink_c::checkCastleTown() || special_no_battle_room()) {
                *static_cast<bool*>(retval) = daAlink_c::checkRoomOnly();
            } else {
                *static_cast<bool*>(retval) = CheckNotBattleStage::g_orig();
            }
        }, overrideOptions), "Failed to relax no-battle stage item restrictions");
    install(mods::hook::replace<CheckItemAction>(
        svc_hook, [](ModContext*, void* args, void* retval, void*) {
            auto* player = mods::arg<daAlink_c*>(args, 0);
            if (lazy_tweaks_combat::try_manual_shield_attack(player)) {
                *static_cast<BOOL*>(retval) = TRUE;
                return;
            }

            const bool adjustWater = player != nullptr &&
                daAlink_c::checkFishingRodItem(player->mEquipItem) &&
                player->mLinkAcch.ChkGroundHit() &&
                !player->checkNoResetFlg0(daAlink_c::FLG0_SWIM_UP);
            const f32 waterY = adjustWater ? player->mWaterY : 0.0f;
            const u8 rStatus = dComIfGp_getRStatus();
            const bool suppressNativeShieldAttack =
                player != nullptr && lazy_tweaks_combat::manual_shield_enabled() &&
                mDoCPd_c::getHoldLockR(PAD_1);
            if (adjustWater) {
                player->mWaterY = player->current.pos.y;
            }
            if (suppressNativeShieldAttack) {
                dComIfGp_setRStatus(
                    BUTTON_STATUS_SHIELD_ATTACK, BUTTON_STATUS_FLAG_NONE);
            }
            *static_cast<BOOL*>(retval) = CheckItemAction::g_orig(player);
            if (suppressNativeShieldAttack) {
                dComIfGp_setRStatus(rStatus, BUTTON_STATUS_FLAG_NONE);
            }
            if (adjustWater) {
                player->mWaterY = waterY;
            }
        }), "Failed to relax fishing-rod water restrictions");
    install(mods::hook::replace<CheckItemChangeFromButton>(
        svc_hook, [](ModContext*, void* args, void* retval, void*) {
            auto* player = mods::arg<daAlink_c*>(args, 0);
            auto& changed = *static_cast<BOOL*>(retval);
            changed = CheckItemChangeFromButton::g_orig(player);
            if (player != nullptr && !changed && player->checkModeFlg(4) &&
                !player->checkEquipAnime() &&
                !player->checkBoomerangThrowAnime() &&
                !player->checkCopyRodThrowAnime() &&
                !player->checkKandelaarSwingAnime() &&
                !player->checkCanoeRide() &&
                (!player->checkModeFlg(0x40000) ||
                 player->checkEquipHeavyBoots()) &&
                player->mEquipItem != 0x103 && player->swordTrigger() &&
                !player->checkEndResetFlg1(
                    daAlink_c::ERFLG1_SWORD_TRIGGER_NON))
            {
                player->swordEquip(TRUE);
                changed = TRUE;
            }
        }, overrideOptions), "Failed to allow sword use in restricted stages");
    install(mods::hook::replace<SwimDeleteItem>(
        svc_hook, preserve_underwater_lantern),
        "Failed to preserve underwater equipped items");
    install(mods::hook::add_pre<CheckWaterInKandelaar>(
        svc_hook, check_water_in_lantern_pre),
        "Failed to keep the Lantern lit underwater");
    install(mods::hook::add_post<CheckKandelaarSwing>(
        svc_hook, check_lantern_swing_post),
        "Failed to update underwater Lantern behavior");
    install(mods::hook::add_pre<InitKandelaarSwing>(
        svc_hook, save_lantern_oil),
        "Failed to preserve Lantern oil underwater");
    install(mods::hook::add_post<InitKandelaarSwing>(
        svc_hook, restore_lantern_oil),
        "Failed to restore Lantern oil underwater");
    install(mods::hook::replace<CheckNewItemChange>(
        svc_hook, [](ModContext*, void* args, void* retval, void*) {
            auto* player = mods::arg<daAlink_c*>(args, 0);
            const u8 slot = mods::arg<u8>(args, 1);
            *static_cast<int*>(retval) = unrestricted_item_change(
                player, slot, dComIfGp_getSelectItem(slot));
        }), "Failed to relax item-change restrictions");
    install(mods::hook::add_pre<ChangeItemTriggerKeepProc>(
        svc_hook, allow_hawkeye_on_horse),
        "Failed to allow Hawkeye while riding");
    install(mods::hook::replace<CheckNoSubjectModeCamera>(
        svc_hook, [](ModContext*, void* args, void* retval, void*) {
            auto* player = mods::arg<daAlink_c*>(args, 0);
            if (player != nullptr && castleTownFirstPersonEnabled() &&
                camera_stage())
            {
                *static_cast<bool*>(retval) = player->checkCargoCarry();
            } else {
                *static_cast<bool*>(retval) =
                    CheckNoSubjectModeCamera::g_orig(player);
            }
        }, overrideOptions), "Failed to allow first-person camera stages");
    install(mods::hook::replace<CheckNotHeavyBootsStage>(
        svc_hook, [](ModContext*, void*, void* retval, void*) {
            *static_cast<bool*>(retval) = false;
        }), "Failed to relax Heavy Boots restrictions");
    install(mods::hook::replace<CheckHookshotStickBG>(
        svc_hook, [](ModContext*, void* args, void* retval, void*) {
            auto* player = mods::arg<daAlink_c*>(args, 0);
            if (clawshotAnySurfaceEnabled()) {
                *static_cast<BOOL*>(retval) = TRUE;
            } else {
                *static_cast<BOOL*>(retval) =
                    CheckHookshotStickBG::g_orig(
                        player, mods::arg<cBgS_PolyInfo&>(args, 1));
            }
        }), "Failed to enable unrestricted Clawshots");
    install(mods::hook::replace<CheckRoomOnly>(
        svc_hook, [](ModContext*, void*, void* retval, void*) {
            *static_cast<bool*>(retval) = indoorMovementUnrestrictedEnabled()
                ? false : CheckRoomOnly::g_orig();
        }), "Failed to enable normal movement indoors");
    install(mods::hook::replace<ProcGrassWhistleWait>(
        svc_hook, [](ModContext*, void* args, void* retval, void*) {
            auto* player = mods::arg<daAlink_c*>(args, 0);
            if (player == nullptr) {
                return;
            }
            const bool suppressHorseCall =
                (player->mProcVar2.field_0x300c == 1 ||
                 player->mProcVar2.field_0x300c == 3) &&
                player->mProcVar0.field_0x3008 == 1 &&
                !player->checkNoResetFlg0(daAlink_c::FLG0_SWIM_UP);
            const s16 oldType = player->mProcVar2.field_0x300c;
            if (suppressHorseCall) {
                player->mProcVar2.field_0x300c = 0;
            }
            *static_cast<int*>(retval) = ProcGrassWhistleWait::g_orig(player);
            player->mProcVar2.field_0x300c = oldType;
        }), "Failed to suppress underwater horse calls");
    install(mods::hook::add_pre<SetLight>(
        svc_hook, begin_lantern_light_restore),
        "Failed to keep the Lantern visible underwater");
    install(mods::hook::add_post<SetLight>(
        svc_hook, end_lantern_light_restore),
        "Failed to restore underwater Lantern state");
    install(mods::hook::replace<AlphaAnimeKantera>(
        svc_hook, allow_lantern_gauge_underwater),
        "Failed to keep the Lantern gauge visible underwater");
    install(mods::hook::replace<ChangeModeOK>(
        svc_hook, camera_mode_allowed),
        "Failed to allow first-person camera modes");
    install(mods::hook::add_pre<CameraRun>(
        svc_hook, camera_run_pre),
        "Failed to adapt restricted-stage camera modes");
    install(mods::hook::add_post<CameraRun>(
        svc_hook, camera_run_post),
        "Failed to restore camera mode data");
    return hookResult;
}

}  // namespace unrestricted_items
