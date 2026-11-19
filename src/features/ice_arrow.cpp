void clear_ice_arrow_target(IceArrowFrozenTarget& target, fopAc_ac_c* actor) {
    if (target.shaderColorCaptured && actor != nullptr &&
        target.actorId == fopAcM_GetID(actor))
    {
        actor->tevStr.TevColor.r = target.originalTevColor.r;
        actor->tevStr.TevColor.g = target.originalTevColor.g;
        actor->tevStr.TevColor.b = target.originalTevColor.b;
    }

    if (actor != nullptr && target.actorId == fopAcM_GetID(actor)) {
        actor->gravity = target.originalGravity;
        actor->maxFallSpeed = target.originalMaxFallSpeed;
    }

    target.active = false;
    target.shaderColorCaptured = false;
    target.showIceEffect = false;
    target.model = nullptr;
    target.framesRemaining = 0;
}

bool is_ice_arrow_enemy_dead(fopAc_ac_c* actor) {
    if (actor == nullptr || fopAcM_GetGroup(actor) != fopAc_ENEMY_e) {
        return true;
    }
    const auto* enemy = static_cast<fopEn_enemy_c*>(actor);
    return actor->health <= 0 || enemy->checkDeadFlg();
}

void register_ice_arrow_target(
    fopAc_ac_c* actor, u16 framesRemaining, bool showIceEffect)
{
    if (actor == nullptr || fopAcM_GetGroup(actor) != fopAc_ENEMY_e ||
        is_ice_arrow_enemy_dead(actor))
    {
        return;
    }

    IceArrowFrozenTarget* target = nullptr;
    for (auto& entry : g_iceArrowFrozenTargets) {
        if (entry.active && entry.actorId == fopAcM_GetID(actor)) {
            target = &entry;
            break;
        }
        if (!entry.active && target == nullptr) {
            target = &entry;
        }
    }

    if (target == nullptr) {
        target = &g_iceArrowFrozenTargets[0];
        for (auto& entry : g_iceArrowFrozenTargets) {
            if (entry.framesRemaining < target->framesRemaining) {
                target = &entry;
            }
        }
    }

    if (target->active && target->actorId != fopAcM_GetID(actor)) {
        fopAc_ac_c* replacedActor = fopAcM_SearchByID(target->actorId);
        clear_ice_arrow_target(*target, replacedActor);
    }

    if (!target->active) {
        target->actorId = fopAcM_GetID(actor);
        target->originalTevColor = actor->tevStr.TevColor;
        target->shaderColorCaptured = true;
        target->originalGravity = actor->gravity;
        target->originalMaxFallSpeed = actor->maxFallSpeed;
        target->showIceEffect = showIceEffect;
    }
    target->actorId = fopAcM_GetID(actor);
    target->model = actor->model;
    target->frozenPosition = actor->current.pos;
    target->frozenAngle = actor->current.angle;
    target->frozenShapeAngle = actor->shape_angle;
    target->framesRemaining = framesRemaining;
    target->showIceEffect = target->showIceEffect || showIceEffect;
    target->active = true;
    if (showIceEffect) {
        dComIfGp_particle_set(0, ID_AK_JN_ENEMYFREEZE00, &actor->current.pos, &actor->tevStr);
    }
}

bool is_ice_arrow_actor_frozen(const fopAc_ac_c* actor) {
    if (actor == nullptr) {
        return false;
    }
    if (fopAcM_GetGroup(actor) != fopAc_ENEMY_e || actor->health <= 0 ||
        static_cast<const fopEn_enemy_c*>(actor)->checkDeadFlg())
    {
        return false;
    }

    const fpc_ProcID actorId = fopAcM_GetID(actor);
    for (const auto& target : g_iceArrowFrozenTargets) {
        if (target.active && target.actorId == actorId) {
            return true;
        }
    }
    return false;
}

bool release_dead_ice_arrow_actor(fpc_ProcID actorId) {
    for (auto& target : g_iceArrowFrozenTargets) {
        if (!target.active || target.actorId != actorId) {
            continue;
        }

        fopAc_ac_c* actor = fopAcM_SearchByID(actorId);
        if (!is_ice_arrow_enemy_dead(actor)) {
            return false;
        }

        clear_ice_arrow_target(target, actor);
        return true;
    }
    return false;
}

void hold_ice_arrow_target_in_place(ModContext*, void* args, void*, void*) {
    auto* leaf = mods::arg<leafdraw_class*>(args, 0);
    if (leaf == nullptr) {
        return;
    }

    auto* actor = reinterpret_cast<fopAc_ac_c*>(leaf);
    const fpc_ProcID actorId = fopAcM_GetID(actor);
    for (auto& target : g_iceArrowFrozenTargets) {
        if (!target.active || target.actorId != actorId) {
            continue;
        }

        if (is_ice_arrow_enemy_dead(actor)) {
            clear_ice_arrow_target(target, actor);
            return;
        }

        actor->current.pos = target.frozenPosition;
        actor->current.angle = target.frozenAngle;
        actor->old = actor->current;
        actor->shape_angle = target.frozenShapeAngle;
        actor->speed.set(0.0f, 0.0f, 0.0f);
        actor->speedF = 0.0f;
        actor->gravity = 0.0f;
        return;
    }
}

HookAction begin_ice_arrow_actor_execute(ModContext*, void* args, void*, void*) {
    auto* leaf = mods::arg<leafdraw_class*>(args, 0);
    constexpr u32 kTrackedDepth =
        sizeof(g_iceArrowPreviousExecutingActorIds) /
        sizeof(g_iceArrowPreviousExecutingActorIds[0]);
    if (g_iceArrowActorExecuteDepth < kTrackedDepth) {
        g_iceArrowPreviousExecutingActorIds[g_iceArrowActorExecuteDepth] =
            g_iceArrowExecutingActorId;
        if (leaf != nullptr) {
            g_iceArrowExecutingActorId =
                fopAcM_GetID(reinterpret_cast<fopAc_ac_c*>(leaf));
        }
    }
    ++g_iceArrowActorExecuteDepth;
    return HOOK_CONTINUE;
}

void end_ice_arrow_actor_execute(ModContext*, void*, void*, void*) {
    if (g_iceArrowActorExecuteDepth == 0) {
        return;
    }
    --g_iceArrowActorExecuteDepth;
    if (g_iceArrowActorExecuteDepth <
        sizeof(g_iceArrowPreviousExecutingActorIds) /
            sizeof(g_iceArrowPreviousExecutingActorIds[0]))
    {
        g_iceArrowExecutingActorId =
            g_iceArrowPreviousExecutingActorIds[g_iceArrowActorExecuteDepth];
    }
}

HookAction suppress_ice_frozen_enemy_attack(ModContext*, void* args, void*, void*) {
    auto* collider = mods::arg<cCcD_Obj*>(args, 0);
    if (collider == nullptr || !collider->ChkAtSet() ||
        !is_ice_arrow_actor_frozen(collider->GetAc()))
    {
        return HOOK_CONTINUE;
    }

    collider->OffAtSetBit();
    g_iceArrowSuppressedAttackCollider = collider;
    return HOOK_CONTINUE;
}

HookAction hide_secondary_assigned_vanilla_ammo_count(
    ModContext*, void* args, void*, void*)
{
    auto* meterDraw = mods::arg<dMeter2Draw_c*>(args, 0);
    const u8 button = mods::arg<u8>(args, 1);
    auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    if (meterDraw == nullptr || button >= 2 ||
        alink == nullptr || alink->checkWolf() || !secondaryItemAssigned(button))
    {
        return HOOK_CONTINUE;
    }

    for (auto* digit : meterDraw->mpItemNumTex[button]) {
        if (digit != nullptr) {
            digit->setAlpha(0);
        }
    }
    return HOOK_SKIP_ORIGINAL;
}

void restore_ice_frozen_enemy_attack(ModContext*, void* args, void*, void*) {
    auto* collider = mods::arg<cCcD_Obj*>(args, 0);
    if (collider != nullptr && collider == g_iceArrowSuppressedAttackCollider) {
        collider->OnAtSetBit();
        g_iceArrowSuppressedAttackCollider = nullptr;
    }
}

bool is_ice_arrow_target_model(J3DModel* model) {
    if (model == nullptr) {
        return false;
    }

    for (auto& target : g_iceArrowFrozenTargets) {
        if (!target.active || target.model != model) {
            continue;
        }

        fopAc_ac_c* actor = fopAcM_SearchByID(target.actorId);
        if (is_ice_arrow_enemy_dead(actor)) {
            clear_ice_arrow_target(target, actor);
            continue;
        }
        target.model = actor->model;
        return target.model == model;
    }

    return false;
}

HookAction update_ice_arrow_freezes(ModContext*, void* args, void*, void*) {
    if (mods::arg<daAlink_c*>(args, 0) == nullptr) {
        return HOOK_CONTINUE;
    }

    for (auto& target : g_iceArrowFrozenTargets) {
        if (!target.active) {
            continue;
        }

        fopAc_ac_c* actor = fopAcM_SearchByID(target.actorId);
        if (is_ice_arrow_enemy_dead(actor) || target.framesRemaining <= 1) {
            clear_ice_arrow_target(target, actor);
            continue;
        }

        target.model = actor->model;
        --target.framesRemaining;
    }
    return HOOK_CONTINUE;
}

HookAction apply_bow_combo_arrow_selection(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (g_secondaryRing != nullptr) {
        return HOOK_SKIP_ORIGINAL;
    }
    if (player == nullptr ||
        (!g_iceArrowSelected && !g_lightArrowSelected) ||
        player->checkCanoeSlider())
    {
        return HOOK_CONTINUE;
    }

    const bool bowType =
        player->mEquipItem == dItemNo_BOW_e ||
        player->mEquipItem == dItemNo_BOMB_ARROW_e;
    if (!bowType) {
        return HOOK_CONTINUE;
    }

    player->setItemActionButtonStatus(BUTTON_STATUS_SWITCH);
    if (!player->arrowChangeTrigger()) {
        return HOOK_CONTINUE;
    }

    if (g_lightArrowSelected) {
        set_light_arrow_selected(player, false);
    } else {
        set_ice_arrow_selected(player, false);
    }
    return HOOK_SKIP_ORIGINAL;
}

void set_bow_combo_arrow_type_after_creation(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (player == nullptr) {
        return;
    }

    fopAc_ac_c* actor = player->mItemAcKeep.getActor();
    if (actor != nullptr && fopAcM_GetName(actor) == fpcNm_ARROW_e) {
        auto* arrow = static_cast<daArrow_c*>(actor);
        if (g_iceArrowSelected) {
            arrow->mArrowType = kIceArrowType;
        }
    }
}

constexpr u16 kIceArrowPlatformLifetimeFrames = 15 * 30;
constexpr size_t kIceArrowPlatformCount = 16;
constexpr f32 kIceArrowPlatformScale = 0.4f;

struct IceArrowWaterPlatform {
    fpc_ProcID actorId = fpcM_ERROR_PROCESS_ID_e;
    fpc_ProcID arrowId = fpcM_ERROR_PROCESS_ID_e;
    u16 framesRemaining = 0;
};

std::array<IceArrowWaterPlatform, kIceArrowPlatformCount> g_iceArrowWaterPlatforms{};

bool spawn_ice_arrow_water_platform(const cXyz& position, daArrow_c* arrow) {
    size_t slotIndex = g_iceArrowWaterPlatforms.size();
    for (size_t i = 0; i < g_iceArrowWaterPlatforms.size(); ++i) {
        if (g_iceArrowWaterPlatforms[i].actorId == fpcM_ERROR_PROCESS_ID_e &&
            g_iceArrowWaterPlatforms[i].arrowId == fpcM_ERROR_PROCESS_ID_e)
        {
            slotIndex = i;
            break;
        }
    }
    if (slotIndex == g_iceArrowWaterPlatforms.size()) {
        svc_log->warn(mod_ctx, "Ice Arrow platform limit reached; wait for one to melt");
        return false;
    }

    const csXyz uprightAngle(0, 0, 0);
    const fpc_ProcID actorId = fopAcM_create(
        fpcNm_Obj_Ice_s_e, 0u, &position, fopAcM_GetRoomNo(arrow),
        &uprightAngle, nullptr, -1);
    if (actorId == fpcM_ERROR_PROCESS_ID_e) {
        svc_log->error(mod_ctx, "Failed to create Ice Arrow water platform");
        return false;
    }

    g_iceArrowWaterPlatforms[slotIndex] = {
        actorId, fopAcM_GetID(arrow), kIceArrowPlatformLifetimeFrames};
    return true;
}

void scale_ice_arrow_water_platform(ModContext*, void* args, void*, void*) {
    auto* platform = mods::arg<daObjIce_s_c*>(args, 0);
    if (platform == nullptr || platform->mModel == nullptr) {
        return;
    }

    const fpc_ProcID actorId = fopAcM_GetID(platform);
    const bool isIceArrowPlatform = std::any_of(
        g_iceArrowWaterPlatforms.begin(), g_iceArrowWaterPlatforms.end(),
        [actorId](const IceArrowWaterPlatform& entry) {
            return entry.actorId == actorId;
        });
    if (!isIceArrowPlatform) {
        return;
    }

    const cXyz platformScale(
        kIceArrowPlatformScale, kIceArrowPlatformScale,
        kIceArrowPlatformScale);
    platform->mModel->setBaseScale(platformScale);
    platform->field_0x5c8.set(
        kIceArrowPlatformScale, kIceArrowPlatformScale,
        kIceArrowPlatformScale);
    for (size_t row = 0; row < 3; ++row) {
        for (size_t column = 0; column < 3; ++column) {
            platform->mBgMtx[row][column] *= kIceArrowPlatformScale;
        }
    }
}

void create_ice_arrow_water_platform_on_execute(
    ModContext*, void* args, void*, void*)
{
    auto* arrow = mods::arg<daArrow_c*>(args, 0);
    if (arrow == nullptr || arrow->mArrowType != kIceArrowType ||
        arrow->checkWait())
    {
        return;
    }

    const fpc_ProcID arrowId = fopAcM_GetID(arrow);
    const bool platformAlreadyCreated = std::any_of(
        g_iceArrowWaterPlatforms.begin(), g_iceArrowWaterPlatforms.end(),
        [arrowId](const IceArrowWaterPlatform& platform) {
            return platform.arrowId == arrowId;
        });
    if (platformAlreadyCreated) {
        return;
    }

    cXyz waterQuery = arrow->current.pos;
    if (!fopAcM_wt_c::waterCheck(&waterQuery)) {
        return;
    }

    const f32 waterY = fopAcM_wt_c::getWaterY();
    if (waterQuery.y >= waterY) {
        return;
    }

    waterQuery.y = waterY;
    spawn_ice_arrow_water_platform(waterQuery, arrow);
}

void update_ice_arrow_water_platforms(ModContext*, void*, void*, void*) {
    if (dComIfGp_isPauseFlag()) {
        return;
    }

    for (IceArrowWaterPlatform& platform : g_iceArrowWaterPlatforms) {
        if (platform.arrowId != fpcM_ERROR_PROCESS_ID_e &&
            fopAcM_SearchByID(platform.arrowId) == nullptr)
        {
            platform.arrowId = fpcM_ERROR_PROCESS_ID_e;
        }
        if (platform.actorId == fpcM_ERROR_PROCESS_ID_e) {
            if (platform.arrowId == fpcM_ERROR_PROCESS_ID_e) {
                platform = {};
            }
            continue;
        }
        fopAc_ac_c* actor = fopAcM_SearchByID(platform.actorId);
        if (platform.framesRemaining <= 1) {
            if (actor != nullptr) {
                fopAcM_delete(actor);
            }
            platform.actorId = fpcM_ERROR_PROCESS_ID_e;
            platform.framesRemaining = 0;
            if (platform.arrowId == fpcM_ERROR_PROCESS_ID_e) {
                platform = {};
            }
            continue;
        }
        --platform.framesRemaining;
    }
}

void clear_ice_arrow_water_platforms() {
    for (IceArrowWaterPlatform& platform : g_iceArrowWaterPlatforms) {
        if (platform.actorId != fpcM_ERROR_PROCESS_ID_e) {
            if (fopAc_ac_c* actor = fopAcM_SearchByID(platform.actorId);
                actor != nullptr)
            {
                fopAcM_delete(actor);
            }
        }
        platform = {};
    }
}

constexpr u8 kLightArrowLinkHandMarker = 0xA7;
constexpr int kTrackedLightArrows = 32;
constexpr f32 kLightArrowLightPower = 250.0f;
constexpr f32 kLightArrowLightFluctuation = 1.0f;
constexpr f32 kLightArrowBossLightRefDistance = 0.5f;
constexpr GXColor kLightArrowEffectPrimColor = {0xFF, 0xFF, 0xA0, 0xFF};
constexpr GXColor kLightArrowEffectEnvColor = {0xFF, 0xD0, 0x20, 0xFF};
constexpr GXColorS10 kLightArrowLightColor = {0xFF, 0xD8, 0x60, 0xFF};
constexpr GXColor kLightArrowBossLightColor = {0xFF, 0xD8, 0x60, 0xFF};

struct LightArrowLight {
    fpc_ProcID actorId = fpcM_ERROR_PROCESS_ID_e;
    LIGHT_INFLUENCE influence{};
    bool used = false;
    bool active = false;
};

std::array<LightArrowLight, kTrackedLightArrows> g_lightArrowLights{};

LightArrowLight* get_light_arrow_light(daArrow_c* arrow) {
    const fpc_ProcID actorId = fopAcM_GetID(arrow);
    LightArrowLight* freeSlot = nullptr;
    for (auto& slot : g_lightArrowLights) {
        if (slot.used && slot.actorId == actorId) {
            return &slot;
        }
        if (!slot.used && freeSlot == nullptr) {
            freeSlot = &slot;
        }
    }

    if (freeSlot == nullptr) {
        return nullptr;
    }

    freeSlot->actorId = actorId;
    freeSlot->influence.mColor = kLightArrowLightColor;
    freeSlot->influence.mPow = kLightArrowLightPower;
    freeSlot->influence.mFluctuation = kLightArrowLightFluctuation;
    freeSlot->used = true;
    freeSlot->active = false;
    return freeSlot;
}

void stop_light_arrow_emitter(u32& emitterId) {
    JPABaseEmitter* emitter = dComIfGp_particle_getEmitter(emitterId);
    if (emitter != nullptr) {
        emitter->setParticleCallBackPtr(nullptr);
        emitter->setUserWork(0);
        emitter->stopDrawParticle();
    }
    emitterId = 0;
}

void stop_light_arrow_effects(daArrow_c* arrow) {
    if (arrow == nullptr) {
        return;
    }

    stop_light_arrow_emitter(arrow->field_0x964);
    stop_light_arrow_emitter(arrow->field_0x968);
    for (auto& slot : g_lightArrowLights) {
        if (slot.used && slot.actorId == fopAcM_GetID(arrow)) {
            if (slot.active) {
                dKy_plight_cut(&slot.influence);
            }
            slot.used = false;
            slot.active = false;
            slot.actorId = fpcM_ERROR_PROCESS_ID_e;
        }
    }
}

void cleanup_light_arrow_lights(ModContext*, void*, void*, void*) {
    for (auto& slot : g_lightArrowLights) {
        if (slot.used && fopAcM_SearchByID(slot.actorId) == nullptr) {
            if (slot.active) {
                dKy_plight_cut(&slot.influence);
            }
            slot.used = false;
            slot.active = false;
            slot.actorId = fpcM_ERROR_PROCESS_ID_e;
        }
    }
}

bool has_light_arrow_boss_light_at(const cXyz& position) {
    for (int i = 1; i < 6; ++i) {
        const auto& light = g_env_light.field_0x0c18[i];
        if (light.field_0x26 == 1 && light.mPos.x == position.x &&
            light.mPos.y == position.y && light.mPos.z == position.z)
        {
            return true;
        }
    }
    return false;
}

HookAction initialize_light_arrow_on_first_execute(
    ModContext*, void* args, void*, void*)
{
    auto* arrow = mods::arg<daArrow_c*>(args, 0);
    auto* player = daAlink_getAlinkActorClass();
    if (!g_lightArrowSelected || arrow == nullptr || player == nullptr ||
        player->mItemAcKeep.getActor() != arrow ||
        arrow->mArrowType != daArrow_c::ARROW_TYPE_NORMAL)
    {
        return HOOK_CONTINUE;
    }

    arrow->mArrowType = daArrow_c::ARROW_TYPE_LIGHT;
    arrow->field_0x941 = kLightArrowLinkHandMarker;
    arrow->field_0x946 = 6;
    arrow->field_0x94c = ID_ZF_J_FIRE01_SPARK;
    arrow->field_0x688.SetAtType(
        arrow->field_0x688.GetAtType() | AT_TYPE_MASTER_SWORD);
    arrow->setLightChargeEffect(0);
    return HOOK_CONTINUE;
}

void update_light_arrow_visual_effects(ModContext*, void* args, void*, void*) {
    auto* arrow = mods::arg<daArrow_c*>(args, 0);
    if (arrow == nullptr) {
        return;
    }

    const bool isModLightArrow =
        arrow->field_0x941 == kLightArrowLinkHandMarker || projectiles_is_remote(arrow);
    if (arrow->field_0x93f != 0) {
        if (isModLightArrow) {
            stop_light_arrow_effects(arrow);
        }
        return;
    }
    if (!isModLightArrow || arrow->mpModel == nullptr ||
        arrow->mArrowType != daArrow_c::ARROW_TYPE_LIGHT)
    {
        return;
    }

    arrow->setSmokePos();
    const f32 scale = arrow->checkWait() ? 1.0f / 3.0f : 1.0f;
    const JGeometry::TVec3<f32> effectScale(scale, scale, scale);
    const u8 alpha = 0xFF;
    arrow->field_0x964 = dComIfGp_particle_setColor(
        arrow->field_0x964, ID_ZF_J_FIRE00_GLOW, &arrow->field_0x9cc,
        &arrow->tevStr, &kLightArrowEffectPrimColor, &kLightArrowEffectEnvColor,
        0.0f, alpha, &arrow->shape_angle, nullptr, nullptr, -1, nullptr);
    dComIfGp_particle_levelEmitterOnEventMove(arrow->field_0x964);
    arrow->field_0x968 = dComIfGp_particle_setColor(
        arrow->field_0x968, ID_ZF_J_FIRE01_SPARK, &arrow->field_0x9cc,
        &arrow->tevStr, &kLightArrowEffectPrimColor, &kLightArrowEffectEnvColor,
        0.0f, alpha, &arrow->shape_angle, nullptr, nullptr, -1, nullptr);
    dComIfGp_particle_levelEmitterOnEventMove(arrow->field_0x968);

    for (u32 emitterId : {arrow->field_0x964, arrow->field_0x968}) {
        JPABaseEmitter* emitter = dComIfGp_particle_getEmitter(emitterId);
        if (emitter != nullptr) {
            emitter->setParticleCallBackPtr(dPa_control_c::getParticleTracePCB());
            emitter->setUserWork(reinterpret_cast<uintptr_t>(&arrow->speed));
            emitter->setGlobalScale(effectScale);
        }
    }

    if (LightArrowLight* slot = get_light_arrow_light(arrow); slot != nullptr) {
        slot->influence.mPosition = arrow->field_0x9cc;
        if (!slot->active) {
            dKy_plight_set(&slot->influence);
            slot->active = true;
        }
    }

    GXColor bossLightColor = kLightArrowBossLightColor;
    const int lightSet = dKy_BossLight_set(
        &arrow->field_0x9cc, &bossLightColor, kLightArrowBossLightRefDistance, 0);
    if (lightSet == 0 && !has_light_arrow_boss_light_at(arrow->field_0x9cc) &&
        dKy_Indoor_check() &&
        g_env_light.field_0x0c18[0].field_0x26 == 0)
    {
        dKy_WolfEyeLight_set(&arrow->field_0x9cc, 0.0f, 0.0f, 50.0f,
                             &bossLightColor, 1.0f, 0, 3);
    }
}

HookAction use_link_hand_for_held_light_arrow(ModContext*, void* args, void*, void*) {
    auto* arrow = mods::arg<daArrow_c*>(args, 0);
    if (arrow == nullptr ||
        arrow->field_0x941 != kLightArrowLinkHandMarker ||
        arrow->mArrowType != daArrow_c::ARROW_TYPE_LIGHT)
    {
        return HOOK_CONTINUE;
    }

    arrow->mArrowType = daArrow_c::ARROW_TYPE_NORMAL;
    return HOOK_CONTINUE;
}

void restore_light_arrow_type_after_keep_matrix(ModContext*, void* args, void*, void*) {
    auto* arrow = mods::arg<daArrow_c*>(args, 0);
    if (arrow != nullptr &&
        arrow->field_0x941 == kLightArrowLinkHandMarker &&
        arrow->mArrowType == daArrow_c::ARROW_TYPE_NORMAL)
    {
        arrow->mArrowType = daArrow_c::ARROW_TYPE_LIGHT;
    }
}

void draw_ice_keese_on_held_arrow(ModContext*, void* args, void*, void*) {
    auto* arrow = mods::arg<daArrow_c*>(args, 0);
    const bool remoteIceArrow = arrow != nullptr && projectiles_is_remote(arrow);
    if ((!g_iceArrowSelected && !remoteIceArrow) || g_iceKeeseModel == nullptr || arrow == nullptr ||
        arrow->mpModel == nullptr || arrow->mArrowType != kIceArrowType ||
        !arrow->checkWait() || arrow->field_0x93f != 0 || arrow->field_0x942 != 0)
    {
        return;
    }

    mDoMtx_stack_c::copy(arrow->mpModel->getBaseTRMtx());
    mDoMtx_stack_c::transM(0.0f, 0.0f, kIceKeeseArrowTipOffset);
    mDoMtx_stack_c::scaleM(kIceKeeseArrowScale, kIceKeeseArrowScale,
                           kIceKeeseArrowScale);
    g_iceKeeseModel->setBaseTRMtx(mDoMtx_stack_c::get());

    dKy_tevstr_c tevStr{};
    g_env_light.settingTevStruct(0, &arrow->current.pos, &tevStr);
    g_env_light.setLightTevColorType_MAJI(g_iceKeeseModel, &tevStr);
    mDoExt_modelUpdateDL(g_iceKeeseModel);
}

HookAction register_ice_arrow_hit(ModContext*, void* args, void*, void*) {
    auto* arrow = mods::arg<daArrow_c*>(args, 0);
    auto* actor = mods::arg<fopAc_ac_c*>(args, 2);
    auto* targetInfo = mods::arg<dCcD_GObjInf*>(args, 3);
    if (arrow != nullptr && arrow->mArrowType == kIceArrowType &&
        targetInfo != nullptr && !targetInfo->ChkTgShield() &&
        !targetInfo->ChkTgArrowThrough())
    {
        register_ice_arrow_target(actor, kIceArrowFreezeFrames, true);
    }
    return HOOK_CONTINUE;
}
HookAction freeze_ice_arrow_target_animation(ModContext*, void* args, void*, void*) {
    auto* morf = mods::arg<mDoExt_morf_c*>(args, 0);
    if (morf == nullptr) {
        return HOOK_CONTINUE;
    }

    if (release_dead_ice_arrow_actor(g_iceArrowExecutingActorId)) {
        return HOOK_CONTINUE;
    }

    bool actorIsFrozen = false;
    for (const auto& target : g_iceArrowFrozenTargets) {
        if (target.active && target.actorId == g_iceArrowExecutingActorId) {
            actorIsFrozen = true;
            break;
        }
    }

    return actorIsFrozen || is_ice_arrow_target_model(morf->mpModel)
               ? HOOK_SKIP_ORIGINAL
               : HOOK_CONTINUE;
}

void apply_ice_arrow_shader_after_tev_setup(ModContext*, void* args, void*, void*) {
    auto* tevStr = mods::arg<dKy_tevstr_c*>(args, 3);
    if (tevStr == nullptr) {
        return;
    }

    for (const auto& target : g_iceArrowFrozenTargets) {
        if (!target.active) {
            continue;
        }

        fopAc_ac_c* actor = fopAcM_SearchByID(target.actorId);
        if (!target.showIceEffect || actor == nullptr || &actor->tevStr != tevStr) {
            continue;
        }

        daAlink_c* player = daAlink_getAlinkActorClass();
        if (player != nullptr) {
            tevStr->TevColor.r = player->getFreezeR();
            tevStr->TevColor.g = player->getFreezeG();
            tevStr->TevColor.b = player->getFreezeB();
        }
        return;
    }
}

#define ADD_PRE(HookType, callback)                                                                \
    do {                                                                                           \
        const ModResult hookResult = mods::hook::add_pre<HookType>(callback);                      \
        if (hookResult != MOD_OK) {                                                                \
            svc_log->error(mod_ctx, "failed to install secondary wheel hook " #HookType);         \
            set_mod_init_error(error, hookResult, "Secondary wheel hook " #HookType " unavailable"); \
            return hookResult;                                                                     \
        }                                                                                          \
    } while (false)

#define ADD_POST(HookType, callback)                                                               \
    do {                                                                                           \
        const ModResult hookResult = mods::hook::add_post<HookType>(callback);                     \
        if (hookResult != MOD_OK) {                                                                \
            svc_log->error(mod_ctx, "failed to install secondary wheel hook " #HookType);         \
            set_mod_init_error(error, hookResult, "Secondary wheel hook " #HookType " unavailable"); \
            return hookResult;                                                                     \
        }                                                                                          \
    } while (false)

DEFINE_HOOK(&dMenu_Ring_c::openExplain, OpenExplain);
DEFINE_HOOK(&dMenu_Ring_c::setNameString, SetNameString);
DEFINE_HOOK(&dMenu_Ring_c::setSelectItem, RingSetSelectItem);
DEFINE_HOOK(&dMenu_Ring_c::setJumpItem, RingSetJumpItem);
DEFINE_HOOK(&dMenu_Ring_c::setActiveCursor, RingSetActiveCursor);
DEFINE_HOOK(&dMenu_Ring_c::setSelectItemForce, RingSetSelectItemForce);
DEFINE_HOOK(&dMenu_Ring_c::isMixItemOn, RingIsMixItemOn);
DEFINE_HOOK(&dMenu_Ring_c::isMixItemOff, RingIsMixItemOff);
DEFINE_HOOK(&dMenu_Collect2D_c::changeClothe, CollectChangeClothe);
DEFINE_HOOK(&dMeter2Draw_c::changeTextureItemXY, ChangeTextureItemXY);
DEFINE_HOOK(&dMeter2Draw_c::setButtonIconMidonaAlpha, MeterMidnaAlpha);
DEFINE_HOOK(&dMeter2Draw_c::drawItemNum, DrawItemNum);
DEFINE_HOOK(&dMeter2_c::moveButtonXY, MeterMoveButtonXY);
DEFINE_HOOK(&daSpinner_c::reflectAccept, SpinnerReflectAccept);
DEFINE_HOOK(&dMeter2Info_readItemTexture, ReadItemTexture);
DEFINE_HOOK(&daAlink_c::setWaterY, SetWaterY);
DEFINE_HOOK(&daAlink_c::setRoomInfo, SetRoomInfo);
DEFINE_HOOK(&daAlink_c::setStickData, SetStickData);
DEFINE_HOOK(&daAlink_c::checkSetItemTrigger, CheckSetItemTrigger);
DEFINE_HOOK(&daAlink_c::checkItemChangeFromButton, CheckItemChangeFromButton);
DEFINE_HOOK(&daAlink_c::midnaTalkTrigger, MidnaTalkTrigger);
DEFINE_HOOK(&mDoCPd_c::read, PlayerPadRead);
DEFINE_HOOK(&daAlink_c::checkGrabRooster, CheckGrabRooster);
DEFINE_HOOK(&daAlink_c::boardCommon, BoardCommon);
DEFINE_HOOK(&daAlink_c::checkSnowCode, CheckSnowCode);
DEFINE_HOOK(&daAlink_c::checkShieldDraw, CheckShieldDraw);
DEFINE_HOOK(&daAlink_c::checkNextActionBow, CheckNextActionBow);
DEFINE_HOOK(&daAlink_c::checkNotItemSinkLimit, CheckNotItemSinkLimit);
DEFINE_HOOK(&daAlink_c::setBodyAngleXReadyAnime, SetBodyAngleXReadyAnime);
#if TARGET_PC
DEFINE_HOOK(&daAlink_c::checkAimContext, CheckAimContext);
#endif
DEFINE_HOOK(&daAlink_c::rideGetOff, RideGetOff);
DEFINE_HOOK(&daAlink_c::checkWaitAction, CheckWaitAction);
DEFINE_HOOK(&daAlink_c::procDamageInit, ProcDamageInit);
DEFINE_HOOK(&daAlink_c::procCoLargeDamageInit, ProcCoLargeDamageInit);
DEFINE_HOOK(&daAlink_c::procLargeDamageUpInit, ProcLargeDamageUpInit);
DEFINE_HOOK(&daAlink_c::procCoLargeDamageWallInit, ProcCoLargeDamageWallInit);
DEFINE_HOOK(&daAlink_c::procCoPolyDamageInit, ProcCoPolyDamageInit);
DEFINE_HOOK(&daAlink_c::procCoElecDamageInit, ProcCoElecDamageInit);
DEFINE_HOOK(&daAlink_c::procGuardBreakInit, ProcGuardBreakInit);
DEFINE_HOOK(&daAlink_c::procHorseDamageInit, ProcHorseDamageInit);
DEFINE_HOOK(&daAlink_c::procSwimDamageInit, ProcSwimDamageInit);
DEFINE_HOOK(&daAlink_c::autoGroundHit, AutoGroundHit);
DEFINE_HOOK(&daAlink_c::getGroundAngle, GetGroundAngle);
DEFINE_HOOK(&daAlink_c::getSlidePolygon, GetSlidePolygon);
DEFINE_HOOK(&daAlink_c::checkSlope, CheckSlope);
DEFINE_HOOK(&daAlink_c::setStepsOffset, SetStepsOffset);
DEFINE_HOOK(&daAlink_c::iceSlipBgCheck, IceSlipBgCheck);
DEFINE_HOOK(&daAlink_c::swimOutAfter, SwimOutAfter);
DEFINE_HOOK(&daAlink_c::checkAutoJumpAction, CheckAutoJumpAction);
DEFINE_HOOK(&daAlink_c::procLand, ProcLand);
DEFINE_HOOK(&daAlink_c::procHangStartInit, ProcHangStartInit);
DEFINE_HOOK(&daAlink_c::procHangFallStartInit, ProcHangFallStartInit);
DEFINE_HOOK(&daAlink_c::checkLandAction, CheckLandAction);
DEFINE_HOOK(&daAlink_c::setSandShapeOffset, SetSandShapeOffset);
DEFINE_HOOK(&daAlink_c::checkWaterPolygonUnder, CheckWaterPolygonUnder);
DEFINE_HOOK(&daAlink_c::checkPolyDamage, CheckPolyDamage);
DEFINE_HOOK(&daAlink_c::setHeavyBoots, SetHeavyBoots);
DEFINE_HOOK_SYMBOL(
    "cBgS::GetTriPla", bool(const cBgS*, const cBgS_PolyInfo*, cM3dGPla*),
    ShieldSurfGetTriPla);
DEFINE_HOOK(&daSpinner_c::setReflectAngle, SpinnerSetReflectAngle);
DEFINE_HOOK(&daAlink_c::voiceStart, HawkGrassVoiceStart);
DEFINE_HOOK(&dMenu_Ring_c::_move, RingMove);
DEFINE_HOOK(&dMenu_Ring_c::_draw, RingDraw);
DEFINE_HOOK(&dMenu_Ring_c::isOpen, RingIsOpen);
DEFINE_HOOK(&dMenu_Ring_c::isMoveEnd, RingIsMoveEnd);
DEFINE_HOOK(&dMenu_Ring_c::isClose, RingIsClose);
DEFINE_HOOK(&dMw_c::key_wait_proc, KeyWait);
DEFINE_HOOK(&dMw_c::dMw_ring_create, RingCreate);
DEFINE_HOOK(&dMw_c::dMw_ring_delete, RingDelete);
DEFINE_HOOK(&dMw_DOWN_TRIGGER, DownTrigger);
DEFINE_HOOK(&dMw_LEFT_TRIGGER, LeftTrigger);
DEFINE_HOOK(&dMeterMap_c::meter_map_move, MapMeterMove);
DEFINE_HOOK(&dMeterMap_c::keyCheck, MapKeyCheck);
DEFINE_HOOK(&dMeterMap_c::ctrlShowMap, MapShow);
DEFINE_HOOK(&daAlink_c::checkRestartRoom, CheckRestartRoom);
DEFINE_HOOK(&daAlink_c::procCoLavaReturnInit, ProcCoLavaReturnInit);
DEFINE_HOOK(&daAlink_c::checkSceneChange, CheckSceneChange);
DEFINE_HOOK(&daAlink_c::procAutoJump, ProcAutoJump);
DEFINE_HOOK(&daAlink_c::execute, PlayerExecute);
DEFINE_HOOK(&daAlink_c::offWolfEyeUp, OffWolfEyeUp);
DEFINE_HOOK(&daObjIceLeaf_c::execute, IceLeafExecute);
DEFINE_HOOK(&daObjIceLeaf_c::draw, IceLeafDraw);
DEFINE_HOOK(&daAlink_c::draw, PlayerDraw);
DEFINE_HOOK(&daAlink_c::makeArrow, MakeArrow);
DEFINE_HOOK(&daAlink_c::changeArrowType, ChangeArrowType);
DEFINE_HOOK(&daArrow_c::setKeepMatrix, ArrowKeepMatrix);
DEFINE_HOOK(&daObjIce_s_c::Execute, IcePlatformExecute);
DEFINE_HOOK(&daAlink_c::modelCalc, ModelCalc);
DEFINE_HOOK(&daArrow_c::atHitCallBack, ArrowAtHitCallBack);
DEFINE_HOOK(&daArrow_c::draw, ArrowDraw);
DEFINE_HOOK(&daArrow_c::arrowShooting, ArrowShooting);
DEFINE_HOOK(&mDoExt_morf_c::frameUpdate, MorfFrameUpdate);
DEFINE_HOOK(&fpcLf_Execute, LeafExecute);
DEFINE_HOOK(&cCcS::Set, CollisionSet);
DEFINE_HOOK(&daAlink_c::modelDraw, ModelDraw);
DEFINE_HOOK(&J3DMaterial::makeDisplayList, MaterialMakeDisplayList);
DEFINE_HOOK(&J3DMaterial::diff, MaterialDiff);
DEFINE_HOOK(&J3DShapeDraw::draw, ShapeDraw);
DEFINE_HOOK(&daAlink_c::damageMagnification, DamageMagnification);
DEFINE_HOOK(&daAlink_c::setSwordAtParam, SwordAtParam);
DEFINE_HOOK(&daAlink_c::setCylAtParam, CylinderAtParam);
DEFINE_HOOK(static_cast<void (dScnKy_env_light_c::*)(J3DModelData*, dKy_tevstr_c*)>(
                &dScnKy_env_light_c::setLightTevColorType_MAJI),
            LightTevColorTypeMaji);
DEFINE_HOOK(&dScnKy_env_light_c::settingTevStruct, SettingTevStruct);
DEFINE_HOOK(&daAlink_c::setWaterDropColor, SetWaterDropColor);
DEFINE_HOOK(&daItem_c::_daItem_execute, ItemExecute);
DEFINE_HOOK(&daGlwSph_c::execute, GlowSphereExecute);
