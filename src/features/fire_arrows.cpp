// Ported from Fire Arrows Standalone v1.2.2 by Baxlek with permission.
namespace fire_arrows {

using IsMixItemOn = ::RingIsMixItemOn;
using IsMixItemOff = ::RingIsMixItemOff;
using SetActiveCursor = ::RingSetActiveCursor;
using ArrowShooting = ::ArrowShooting;
using AlinkMakeArrow = ::MakeArrow;
using ChangeArrowType = ::ChangeArrowType;

static ConfigVarHandle g_terrainLightingVar = 0;
static bool g_terrainLightingReadFailureReported = false;

static bool terrainLightingEnabled() {
    bool enabled = false;
    const ModResult result = svc_config->get_bool(mod_ctx, g_terrainLightingVar, &enabled);
    if (result != MOD_OK) {
        if (!g_terrainLightingReadFailureReported) {
            svc_log->error(mod_ctx, "failed to read Fire Arrow terrain lighting setting");
            g_terrainLightingReadFailureReported = true;
        }
        return false;
    }
    g_terrainLightingReadFailureReported = false;
    return enabled;
}

ModResult add_settings(UiElementHandle panel) {
    UiControlDesc control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Fire Arrow Terrain Lighting";
    control.help_rml = "Whether fire arrows cast dynamic light onto nearby terrain.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_terrainLightingVar;
    return svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
}

static constexpr u8 NO_MIX_ITEM = 0xFF;

DEFINE_HOOK(&dMenu_Ring_c::setMixItem, SetMixItem);
DEFINE_HOOK(&daArrow_c::execute, ArrowExecute);
DEFINE_HOOK(&dMeter2Draw_c::drawKanteraMeter, DrawKanteraMeter);
DEFINE_HOOK(&daAlink_c::setBowReadyAnime, SetBowReadyAnime);

static constexpr HookOptions kDisguiseHookOptions = {
    sizeof(HookOptions), 1'000'000, HOOK_REPLACE_CONFLICT, nullptr};

static u8 g_disguisedItemSlot = NO_MIX_ITEM;

static void disguiseLanternPre(dMenu_Ring_c* menu) {
    if (menu == nullptr || isSecondaryRing(menu) || menu->mCurrentSlot >= MAX_ITEM_SLOTS) {
        return;
    }
    u8 slot = menu->mItemSlots[menu->mCurrentSlot];
    if (slot >= MAX_ITEM_SLOTS) {
        return;
    }

    if (dComIfGs_getItem(slot, false) == dItemNo_KANTERA_e) {
        dComIfGs_setItem(slot, dItemNo_HAWK_EYE_e);
        g_disguisedItemSlot = slot;
    }
}

static void restoreLanternPost() {
    if (g_disguisedItemSlot != NO_MIX_ITEM) {
        dComIfGs_setItem(g_disguisedItemSlot, dItemNo_KANTERA_e);
        g_disguisedItemSlot = NO_MIX_ITEM;
    }
}

static HookAction on_set_mix_item_pre(ModContext*, void* args, void*, void*) {
    disguiseLanternPre(mods::arg<dMenu_Ring_c*>(args, 0));
    return HOOK_CONTINUE;
}

static void on_set_mix_item_post(ModContext*, void* args, void*, void*) {
    restoreLanternPost();

    mods::arg<dMenu_Ring_c*>(args, 0)->setJumpItem(false);
}

static HookAction on_is_mix_item_on_pre(ModContext*, void* args, void*, void*) {
    disguiseLanternPre(mods::arg<dMenu_Ring_c*>(args, 0));
    return HOOK_CONTINUE;
}

static void on_is_mix_item_on_post(ModContext*, void*, void*, void*) {
    restoreLanternPost();
}

static HookAction on_is_mix_item_off_pre(ModContext*, void* args, void*, void*) {
    disguiseLanternPre(mods::arg<dMenu_Ring_c*>(args, 0));
    return HOOK_CONTINUE;
}

static void on_is_mix_item_off_post(ModContext*, void*, void*, void*) {
    restoreLanternPost();
}

static HookAction on_set_active_cursor_pre(ModContext*, void* args, void*, void*) {
    disguiseLanternPre(mods::arg<dMenu_Ring_c*>(args, 0));
    return HOOK_CONTINUE;
}

static void on_set_active_cursor_post(ModContext*, void*, void*, void*) {
    restoreLanternPost();
}

static bool checkBowLanternComboInSlot(int selectItemIdx) {
    if (dComIfGp_getSelectItem(selectItemIdx) != dItemNo_BOW_e) {
        return false;
    }

    u8 partnerSlot = dComIfGs_getSelectItemIndex(selectItemIdx);
    return dComIfGs_getItem(partnerSlot, false) == dItemNo_KANTERA_e;
}

static bool checkBowLanternComboInExtraSlot(int selectItemIdx) {
    if (dComIfGs_getMixItemIndex(selectItemIdx) != SLOT_4) {
        return false;
    }

    u8 partnerSlot = dComIfGs_getSelectItemIndex(selectItemIdx);
    return dComIfGs_getItem(partnerSlot, false) == dItemNo_KANTERA_e;
}

static bool checkBowLanternCombo() {
    for (int selectItemIdx = 0; selectItemIdx < SELECT_ITEM_NUM; selectItemIdx++) {
        if (checkBowLanternComboInSlot(selectItemIdx)) {
            return true;
        }
    }

    for (int selectItemIdx = SELECT_ITEM_NUM; selectItemIdx < MAX_SELECT_ITEM; selectItemIdx++) {
        if (checkBowLanternComboInExtraSlot(selectItemIdx)) {
            return true;
        }
    }

    return false;
}

static bool g_fireArrowModeOn = true;

static HookAction on_change_arrow_type_pre(ModContext*, void* args, void*, void*) {
    daAlink_c* link = mods::arg<daAlink_c*>(args, 0);

    if (!checkBowLanternCombo()) {
        return HOOK_CONTINUE;
    }

    if (link->checkCanoeSlider() || (!g_fireArrowModeOn && dComIfGs_getOil() == 0)) {
        return HOOK_SKIP_ORIGINAL;
    }

    link->setItemActionButtonStatus(BUTTON_STATUS_SWITCH);

    if (link->arrowChangeTrigger()) {
        g_fireArrowModeOn = !g_fireArrowModeOn;

        if (link->mItemAcKeep.getActor() != NULL) {
            link->deleteArrow();
            link->makeArrow();
            link->setBowReloadAnime();
        }
    }

    return HOOK_SKIP_ORIGINAL;
}

static HookAction on_set_bow_ready_anime_pre(ModContext*, void* args, void*, void*) {
    daAlink_c* link = mods::arg<daAlink_c*>(args, 0);

    if (checkBowLanternCombo() && !link->checkBowAnime()) {
        g_fireArrowModeOn = dComIfGs_getOil() != 0;
    }

    return HOOK_CONTINUE;
}

static HookAction on_draw_kantera_meter_pre(ModContext*, void* args, void*, void*) {
    u8 button = mods::arg<u8>(args, 1);
    f32& alpha = mods::arg_ref<f32>(args, 2);

    if (alpha == 0.0f && button < SELECT_ITEM_NUM && checkBowLanternComboInSlot(button)) {
        alpha = 1.0f;
    }

    return HOOK_CONTINUE;
}

static const dCcD_SrcCps l_igniteCpsSrc = {
    {
        {0x0, {{AT_TYPE_LANTERN_SWING, 0x0, 0x1A}, {0x0, 0x0}, 0x0}},
        {dCcD_SE_NONE, 0x0, 0x0, dCcD_MTRL_FIRE, 0x0},
        {dCcD_SE_NONE, 0x0, 0x0, 0x0, 0x0},
        {0x0},
    },
    {
        {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 50.0f}
    }
};

static constexpr GXColorS10 FIRE_ARROW_LIGHT_COLOR = {0xBC, 0x66, 0x42, 0xFF};
static constexpr f32 FIRE_ARROW_LIGHT_FLUCTUATION = 1.0f;
static constexpr f32 FIRE_ARROW_LIGHT_POW = 250.0f;

static constexpr GXColor FIRE_ARROW_BOSSLIGHT_COLOR = {0xBC, 0x66, 0x42, 0xFF};
static constexpr f32 FIRE_ARROW_BOSSLIGHT_REF_DIST = 0.5f;

static bool hasBossLightAt(const cXyz& pos) {
    for (int i = 1; i < 6; i++) {
        const auto& light = g_env_light.field_0x0c18[i];
        if (light.field_0x26 == 1 && light.mPos.x == pos.x && light.mPos.y == pos.y &&
            light.mPos.z == pos.z)
        {
            return true;
        }
    }
    return false;
}

struct TrackedFireArrow {
    daArrow_c* arrow = nullptr;
    fpc_ProcID arrowId = fpcM_ERROR_PROCESS_ID_e;
    u32 particleKey = 0;
    u32 glowParticleKey = 0;
    cXyz velocity = {0.0f, 0.0f, 0.0f};
    dCcD_Cps igniteCps;
    dCcD_Stts igniteStts;
    bool igniteActive = false;
    bool hitActorHandled = false;
    LIGHT_INFLUENCE light;
    bool lightActive = false;
};

static constexpr int MAX_TRACKED_FIRE_ARROWS = 8;
static TrackedFireArrow g_fireArrows[MAX_TRACKED_FIRE_ARROWS];
static int g_nextFireArrowSlot = 0;

static void initFireArrowSlot(TrackedFireArrow& slot, daArrow_c* arrow, fpc_ProcID id) {
    if (slot.lightActive) {
        dKy_plight_cut(&slot.light);
        slot.lightActive = false;
    }

    slot.arrow = arrow;
    slot.arrowId = id;
    slot.particleKey = 0;
    slot.glowParticleKey = 0;
    slot.igniteActive = false;
    slot.hitActorHandled = false;
    slot.igniteStts.Init(10, 0xff, arrow);
    slot.igniteCps.Set(l_igniteCpsSrc);
    slot.igniteCps.SetStts(&slot.igniteStts);
}

static void trackFireArrow(daArrow_c* arrow) {
    fpc_ProcID id = fpcM_GetID(arrow);

    for (TrackedFireArrow& slot : g_fireArrows) {
        if (slot.arrow == arrow) {
            if (slot.arrowId != id) {
                initFireArrowSlot(slot, arrow, id);
            }
            return;
        }
    }

    TrackedFireArrow& slot = g_fireArrows[g_nextFireArrowSlot];
    g_nextFireArrowSlot = (g_nextFireArrowSlot + 1) % MAX_TRACKED_FIRE_ARROWS;
    initFireArrowSlot(slot, arrow, id);
}

static void activateFireArrowIgnition(daArrow_c* arrow) {
    for (TrackedFireArrow& slot : g_fireArrows) {
        if (slot.arrow == arrow) {
            slot.igniteActive = true;
            return;
        }
    }
}

static bool isArrowStationary(daArrow_c* arrow) {
    return arrow->mProcFunc == &daArrow_c::procWait ||
           arrow->mProcFunc == &daArrow_c::procBGStop ||
           arrow->mProcFunc == &daArrow_c::procActorStop ||
           arrow->mProcFunc == &daArrow_c::procActorControllStop;
}

static constexpr s16 BURNING_ACTOR_DURATION = 150;

static const JGeometry::TVec3<f32> BURN_EFFECT_SCALE(3.0f, 3.0f, 3.0f);

struct BurningActor {
    fpc_ProcID actorId = fpcM_ERROR_PROCESS_ID_e;
    u32 particleKeyA = 0;
    u32 particleKeyB = 0;
    cXyz velocity = {0.0f, 0.0f, 0.0f};
    s16 timer = 0;
    LIGHT_INFLUENCE light;
    bool lightActive = false;
};

static void cutBurningActorLight(BurningActor& slot) {
    if (slot.lightActive) {
        dKy_plight_cut(&slot.light);
        slot.lightActive = false;
    }
}

static constexpr int MAX_BURNING_ACTORS = 8;
static BurningActor g_burningActors[MAX_BURNING_ACTORS];
static int g_nextBurningActorSlot = 0;

static constexpr f32 EYEPOS_MAX_PLAUSIBLE_DIST = 300.0f;
static constexpr f32 FALLBACK_ANCHOR_HEIGHT = 40.0f;

static cXyz getBurnAnchorPos(fopAc_ac_c* actor) {
    if (actor->eyePos.abs(actor->current.pos) <= EYEPOS_MAX_PLAUSIBLE_DIST) {
        return actor->eyePos;
    }
    return actor->current.pos + cXyz(0.0f, FALLBACK_ANCHOR_HEIGHT, 0.0f);
}

static void igniteBurningActor(fopAc_ac_c* actor) {
    fpc_ProcID id = fpcM_GetID(actor);

    for (int i = 0; i < MAX_BURNING_ACTORS; i++) {
        BurningActor& slot = g_burningActors[i];
        if (slot.timer != 0 && slot.actorId == id) {
            slot.timer = BURNING_ACTOR_DURATION;
            return;
        }
    }

    for (int i = 0; i < MAX_BURNING_ACTORS; i++) {
        BurningActor& slot = g_burningActors[i];
        if (slot.timer == 0) {
            slot.actorId = id;
            slot.particleKeyA = 0;
            slot.particleKeyB = 0;
            slot.timer = BURNING_ACTOR_DURATION;
            return;
        }
    }

    BurningActor& slot = g_burningActors[g_nextBurningActorSlot];
    g_nextBurningActorSlot = (g_nextBurningActorSlot + 1) % MAX_BURNING_ACTORS;
    cutBurningActorLight(slot);
    slot.actorId = id;
    slot.particleKeyA = 0;
    slot.particleKeyB = 0;
    slot.timer = BURNING_ACTOR_DURATION;
}

static void updateBurningActors() {
    for (BurningActor& slot : g_burningActors) {
        if (slot.timer == 0) {
            continue;
        }

        fopAc_ac_c* actor = fopAcM_SearchByID(slot.actorId);
        if (actor == NULL) {
            cutBurningActorLight(slot);
            slot.actorId = fpcM_ERROR_PROCESS_ID_e;
            slot.timer = 0;
            continue;
        }

        slot.timer--;
        slot.velocity = actor->speed;

        if (JPABaseEmitter* emitterA = dComIfGp_particle_getEmitter(slot.particleKeyA);
            emitterA != NULL && emitterA->isEnableDeleteEmitter()) {
            slot.particleKeyA = 0;
        }
        if (JPABaseEmitter* emitterB = dComIfGp_particle_getEmitter(slot.particleKeyB);
            emitterB != NULL && emitterB->isEnableDeleteEmitter()) {
            slot.particleKeyB = 0;
        }

        cXyz pos = getBurnAnchorPos(actor);
        slot.particleKeyA =
            dComIfGp_particle_set(slot.particleKeyA, ID_ZI_J_LK_BURNS_A, &pos, NULL, NULL);
        slot.particleKeyB =
            dComIfGp_particle_set(slot.particleKeyB, ID_ZI_J_LK_BURNS_B, &pos, NULL, NULL);

        for (u32 key : {slot.particleKeyA, slot.particleKeyB}) {
            JPABaseEmitter* emitter = dComIfGp_particle_getEmitter(key);
            if (emitter != NULL) {
                emitter->setParticleCallBackPtr(dPa_control_c::getParticleTracePCB());
                emitter->setUserWork((uintptr_t)&slot.velocity);
                emitter->setGlobalScale(BURN_EFFECT_SCALE);
            }
        }

        slot.light.mPosition = pos;
        if (!slot.lightActive) {
            slot.light.mColor = FIRE_ARROW_LIGHT_COLOR;
            slot.light.mPow = FIRE_ARROW_LIGHT_POW;
            slot.light.mFluctuation = FIRE_ARROW_LIGHT_FLUCTUATION;
            dKy_plight_set(&slot.light);
            slot.lightActive = true;
        }

        if (slot.timer == 0) {
            cutBurningActorLight(slot);
            slot.actorId = fpcM_ERROR_PROCESS_ID_e;
        }
    }
}

static constexpr f32 NOCKED_FLAME_SCALE = 1.0f / 3.0f;
static constexpr f32 GLOW_SCALE_FACTOR = 0.5f;

static void updateFireArrowEffect(daArrow_c* arrow) {
    for (TrackedFireArrow& slot : g_fireArrows) {
        if (slot.arrow == arrow) {
            arrow->setSmokePos();

            slot.velocity = isArrowStationary(arrow) ? cXyz(0.0f, 0.0f, 0.0f) : arrow->speed;
            slot.particleKey =
                dComIfGp_particle_set(slot.particleKey, ID_ZF_J_FIRE02_FIRE,
                                      &arrow->field_0x9cc, &arrow->shape_angle, NULL);

            JPABaseEmitter* emitter = dComIfGp_particle_getEmitter(slot.particleKey);
            f32 scale = arrow->mProcFunc == &daArrow_c::procWait ? NOCKED_FLAME_SCALE : 1.0f;
            if (emitter != NULL) {
                emitter->setParticleCallBackPtr(dPa_control_c::getParticleTracePCB());
                emitter->setUserWork((uintptr_t)&slot.velocity);
                emitter->setGlobalScale(JGeometry::TVec3<f32>(scale, scale, scale));
            }

            slot.glowParticleKey =
                dComIfGp_particle_set(slot.glowParticleKey, ID_ZF_J_FIRE00_GLOW,
                                      &arrow->field_0x9cc, &arrow->shape_angle, NULL);

            JPABaseEmitter* glowEmitter = dComIfGp_particle_getEmitter(slot.glowParticleKey);
            if (glowEmitter != NULL) {
                glowEmitter->setParticleCallBackPtr(dPa_control_c::getParticleTracePCB());
                glowEmitter->setUserWork((uintptr_t)&slot.velocity);
                f32 glowScale = scale * GLOW_SCALE_FACTOR;
                glowEmitter->setGlobalScale(JGeometry::TVec3<f32>(glowScale, glowScale, glowScale));
            }

            slot.light.mPosition = arrow->field_0x9cc;
            if (!slot.lightActive) {
                slot.light.mColor = FIRE_ARROW_LIGHT_COLOR;
                slot.light.mPow = FIRE_ARROW_LIGHT_POW;
                slot.light.mFluctuation = FIRE_ARROW_LIGHT_FLUCTUATION;
                dKy_plight_set(&slot.light);
                slot.lightActive = true;
            }

            if (terrainLightingEnabled()) {
                GXColor bossLightColor = FIRE_ARROW_BOSSLIGHT_COLOR;
                int lightSet = dKy_BossLight_set(&arrow->field_0x9cc, &bossLightColor,
                                                 FIRE_ARROW_BOSSLIGHT_REF_DIST, 0);
                if (lightSet == 0 && !hasBossLightAt(arrow->field_0x9cc) && dKy_Indoor_check() &&
                    g_env_light.field_0x0c18[0].field_0x26 == 0)
                {
                    // The shared Wolf Eye slot still illuminates rooms with no free BossLight slots.
                    dKy_WolfEyeLight_set(&arrow->field_0x9cc, 0.0f, 0.0f, 50.0f,
                                         &bossLightColor, 1.0f, 0, 3);
                }
            }

            if (slot.igniteActive) {
                static_cast<cM3dGCps*>(&slot.igniteCps)->Set(arrow->old.pos, arrow->current.pos, 50.0f);
                slot.igniteCps.CalcAtVec();

                if (!slot.igniteCps.ChkAtSet()) {
                    slot.igniteCps.OnAtSetBit();
                }
                dComIfG_Ccsp()->Set(&slot.igniteCps);
            }

            if (!slot.hitActorHandled && arrow->field_0x688.ChkAtHit()) {
                fopAc_ac_c* hitActor = arrow->field_0x688.GetAtHitAc();
                if (hitActor != NULL) {
                    slot.hitActorHandled = true;

                    if (fopAcM_GetGroup(hitActor) == fopAc_ENEMY_e) {
                        igniteBurningActor(hitActor);
                    }
                }
            }

            return;
        }
    }
}

static void updateFireArrows() {
    for (TrackedFireArrow& slot : g_fireArrows) {
        if (slot.lightActive && fopAcM_SearchByID(slot.arrowId) == NULL) {
            dKy_plight_cut(&slot.light);
            slot.lightActive = false;
        }
    }
}

static HookAction on_arrow_shooting_pre(ModContext*, void* args, void*, void*) {
    daArrow_c* arrow = mods::arg<daArrow_c*>(args, 0);

    daAlink_c* link = daAlink_getAlinkActorClass();
    if (link == nullptr || !checkBowLanternCombo() || !g_fireArrowModeOn ||
        dComIfGs_getOil() == 0 || arrow->mArrowType != daArrow_c::ARROW_TYPE_NORMAL)
    {
        return HOOK_CONTINUE;
    }

    dComIfGp_setItemOilCount(-link->mpHIO->mItem.mLantern.m.mShakeOilLoss);

    arrow->field_0x688.SetAtMtrl(dCcD_MTRL_FIRE);

    trackFireArrow(arrow);
    activateFireArrowIgnition(arrow);

    Z2GetAudioMgr()->seStart(Z2SE_OBJ_ARROW_SHOT_FIRE, &arrow->current.pos, 0, 0, 1.0f, 1.0f, -1.0f,
                             -1.0f, 0);

    return HOOK_CONTINUE;
}

static void on_alink_make_arrow_post(ModContext*, void* args, void*, void*) {
    daAlink_c* link = mods::arg<daAlink_c*>(args, 0);
    if (!checkBowLanternCombo() || !g_fireArrowModeOn || dComIfGs_getOil() == 0) {
        return;
    }

    fopAc_ac_c* actor = link->mItemAcKeep.getActor();
    if (actor == NULL || fopAcM_GetName(actor) != fpcNm_ARROW_e) {
        return;
    }

    daArrow_c* arrow = static_cast<daArrow_c*>(actor);
    if (arrow->mArrowType != daArrow_c::ARROW_TYPE_NORMAL) {
        return;
    }

    arrow->field_0x688.SetAtMtrl(dCcD_MTRL_FIRE);
    trackFireArrow(arrow);
}

static void on_arrow_execute_post(ModContext*, void* args, void*, void*) {
    daArrow_c* arrow = mods::arg<daArrow_c*>(args, 0);
    if (arrow->field_0x688.GetAtMtrl() == dCcD_MTRL_FIRE) {
        updateFireArrowEffect(arrow);
    }
}

ModResult initialize() {
    ConfigVarDesc terrainLightingDesc = CONFIG_VAR_DESC_INIT;
    terrainLightingDesc.name = "terrainLighting";
    terrainLightingDesc.type = CONFIG_VAR_BOOL;
    terrainLightingDesc.default_bool = true;
    ModResult configResult =
        svc_config->register_var(mod_ctx, &terrainLightingDesc, &g_terrainLightingVar);
    if (configResult != MOD_OK) {
        svc_log->error(mod_ctx, "failed to register terrainLighting config var");
        return configResult;
    }

    ModResult result = mods::hook::add_pre<SetMixItem>(on_set_mix_item_pre, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install pre hook on_set_mix_item_pre");
        return result;
    }

    result = mods::hook::add_post<SetMixItem>(on_set_mix_item_post, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install post hook on_set_mix_item_post");
        return result;
    }

    result = mods::hook::add_pre<IsMixItemOn>(on_is_mix_item_on_pre, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install pre hook on_is_mix_item_on_pre");
        return result;
    }

    result = mods::hook::add_post<IsMixItemOn>(on_is_mix_item_on_post, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install post hook on_is_mix_item_on_post");
        return result;
    }

    result = mods::hook::add_pre<IsMixItemOff>(on_is_mix_item_off_pre, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install pre hook on_is_mix_item_off_pre");
        return result;
    }

    result = mods::hook::add_post<IsMixItemOff>(on_is_mix_item_off_post, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install post hook on_is_mix_item_off_post");
        return result;
    }

    result = mods::hook::add_pre<SetActiveCursor>(on_set_active_cursor_pre, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install pre hook on_set_active_cursor_pre");
        return result;
    }

    result =
        mods::hook::add_post<SetActiveCursor>(on_set_active_cursor_post, &kDisguiseHookOptions);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install post hook on_set_active_cursor_post");
        return result;
    }

    result = mods::hook::add_pre<ArrowShooting>(on_arrow_shooting_pre);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install pre hook on_arrow_shooting_pre");
        return result;
    }

    result = mods::hook::add_post<ArrowExecute>(on_arrow_execute_post);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install post hook on_arrow_execute_post");
        return result;
    }

    result = mods::hook::add_post<AlinkMakeArrow>(on_alink_make_arrow_post);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install post hook on_alink_make_arrow_post");
        return result;
    }

    result = mods::hook::add_pre<DrawKanteraMeter>(on_draw_kantera_meter_pre);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install pre hook on_draw_kantera_meter_pre");
        return result;
    }

    result = mods::hook::add_pre<ChangeArrowType>(on_change_arrow_type_pre);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install pre hook on_change_arrow_type_pre");
        return result;
    }

    result = mods::hook::add_pre<SetBowReadyAnime>(on_set_bow_ready_anime_pre);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install pre hook on_set_bow_ready_anime_pre");
        return result;
    }

    svc_log->info(mod_ctx, "fire_arrows initialized");
    return MOD_OK;
}

void update() {
    updateBurningActors();
    updateFireArrows();
}

void shutdown() {}

}
