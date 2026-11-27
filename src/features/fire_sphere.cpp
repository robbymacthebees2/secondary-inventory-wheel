namespace fire_sphere {

constexpr u16 kDamagePulseInterval = kFireDamagePulseInterval;
constexpr size_t kEmitterCount = 12;
constexpr f32 kSphereRadius = 115.0f;
constexpr f32 kSphereCenterHeight = 75.0f;
constexpr f32 kFlameScale = 1.8f;
constexpr u8 kBlastDamage = kFireDamageAmount;
constexpr f32 kBlastKnockback = 12.0f;

const GXColor kPrimColor = {255, 196, 64, 255};
const GXColor kEnvColor = {255, 72, 8, 255};

const dCcD_SrcSph kDinFireAttackSphereSource = {
    {
        {0, {{AT_TYPE_NORMAL_SWORD, 3, 0x1A},
             {0xD8FBFDFF, 5}, 0x73}},
        {dCcD_SE_NONE, 3, 1, dCcD_MTRL_FIRE, {1}},
        {dCcD_SE_NONE, 6, 0, 0, {0}},
        {0},
    },
    {{{0.0f, 0.0f, 0.0f}, 40.0f}},
};

const dCcD_SrcSph kDinFireWorldSphereSource = {
    {
        {0, {{AT_TYPE_LANTERN_SWING, 3, 0x1A}, {0xD8FBFDFF, 5}, 0x73}},
        {dCcD_SE_NONE, 3, 1, dCcD_MTRL_FIRE, {1}},
        {dCcD_SE_NONE, 6, 0, 0, {0}},
        {0},
    },
    {{{0.0f, 0.0f, 0.0f}, 40.0f}},
};

struct FlameEmitter {
    u32 key = 0;
    cXyz velocity{};
    cXyz lastPosition{};
    bool hasLastPosition = false;
};

std::array<FlameEmitter, kEmitterCount> g_emitters{};
fpc_ProcID g_ownerId = fpcM_ERROR_PROCESS_ID_e;
u16 g_elapsedFrames = 0;

dCcD_Stts g_damageStts{};
dCcD_Sph g_damageSphere{};
fpc_ProcID g_damageSourceOwnerId = fpcM_ERROR_PROCESS_ID_e;
dCcD_Stts g_worldStts{};
dCcD_Sph g_worldSphere{};

void initialize_damage_sphere(daAlink_c* player) {
    const fpc_ProcID playerId = fopAcM_GetID(player);
    if (g_damageSourceOwnerId == playerId) {
        return;
    }

    g_damageStts.Init(0xFF, 0xFF, player);
    g_damageSphere.Set(kDinFireAttackSphereSource);
    g_damageSphere.SetStts(&g_damageStts);
    g_worldStts.Init(0xFF, 0xFF, player);
    g_worldSphere.Set(kDinFireWorldSphereSource);
    g_worldSphere.SetStts(&g_worldStts);
    g_damageSourceOwnerId = playerId;
}

void apply_blast(daAlink_c* player) {
    if (player == nullptr) {
        return;
    }
    dCcS* collisionServer = dComIfG_Ccsp();
    if (collisionServer == nullptr) {
        return;
    }

    initialize_damage_sphere(player);
    cXyz center = player->current.pos;
    center.y += kSphereCenterHeight;
    g_damageSphere.SetAtType(AT_TYPE_NORMAL_SWORD);
    g_damageSphere.SetAtAtp(kBlastDamage);
    g_damageSphere.SetAtMtrl(dCcD_MTRL_FIRE);
    g_damageSphere.SetAtHitMark(1);
    g_damageSphere.SetR(kSphereRadius);
    g_damageSphere.SetC(center);
    cXyz knockback(0.0f, kBlastKnockback, 0.0f);
    g_damageSphere.SetAtVec(knockback);
    g_damageSphere.ResetAtHit();
    g_damageSphere.OnAtSetBit();
    collisionServer->Set(&g_damageSphere);
    collisionServer->SetMass(&g_damageSphere, 1);

    g_worldSphere.SetAtType(AT_TYPE_LANTERN_SWING);
    g_worldSphere.SetAtAtp(kBlastDamage);
    g_worldSphere.SetAtMtrl(dCcD_MTRL_FIRE);
    g_worldSphere.SetAtHitMark(1);
    g_worldSphere.SetR(kSphereRadius);
    g_worldSphere.SetC(center);
    g_worldSphere.ResetAtHit();
    g_worldSphere.OffAtVsEnemyBit();
    g_worldSphere.OnAtSetBit();
    collisionServer->Set(&g_worldSphere);
    collisionServer->SetMass(&g_worldSphere, 1);
}

void stop_emitter(FlameEmitter& slot) {
    JPABaseEmitter* emitter = dComIfGp_particle_getEmitter(slot.key);
    if (emitter != nullptr) {
        emitter->setParticleCallBackPtr(nullptr);
        emitter->setUserWork(0);
        emitter->stopDrawParticle();
    }
    slot = {};
}

void stop_effect() {
    g_damageSphere.OffAtSetBit();
    g_worldSphere.OffAtSetBit();
    for (FlameEmitter& emitter : g_emitters) {
        stop_emitter(emitter);
    }
    g_elapsedFrames = 0;
    g_ownerId = fpcM_ERROR_PROCESS_ID_e;
}

void start_effect(daAlink_c* player) {
    stop_effect();
    g_ownerId = fopAcM_GetID(player);
    g_elapsedFrames = 0;
}

void update_effect(daAlink_c* player) {
    if (dComIfGs_getOil() == 0) {
        stop_effect();
        return;
    }
    if (player->mpHIO == nullptr) {
        svc_log->error(mod_ctx, "Could not read Lantern oil consumption settings");
        stop_effect();
        return;
    }

    dComIfGp_setItemOilCount(
        -player->mpHIO->mItem.mLantern.m.mNormalOilLoss);
    const f32 elapsed = static_cast<f32>(g_elapsedFrames);
    if (g_elapsedFrames % kDamagePulseInterval == 0) {
        apply_blast(player);
    }
    const f32 bloom = std::min(elapsed / 18.0f, 1.0f);
    for (size_t i = 0; i < g_emitters.size(); ++i) {
        const size_t ring = i / 4;
        const size_t point = i % 4;
        const f32 angle = elapsed * (ring == 1 ? -0.12f : 0.1f) +
                          static_cast<f32>(point) * 1.5707963f +
                          static_cast<f32>(ring) * 0.7f;
        const f32 verticalOffset =
            (static_cast<f32>(ring) - 1.0f) * 68.0f * bloom;
        const f32 ringRadius = (ring == 1 ? kSphereRadius : 93.0f) * bloom;
        cXyz position = player->current.pos;
        position.x += ringRadius * cM_ssin(static_cast<s16>(angle * 10430.378f));
        position.y += kSphereCenterHeight + verticalOffset;
        position.z += ringRadius * cM_scos(static_cast<s16>(angle * 10430.378f));

        FlameEmitter& slot = g_emitters[i];
        slot.velocity = slot.hasLastPosition
            ? position - slot.lastPosition
            : cXyz(0.0f, 0.0f, 0.0f);
        slot.key = dComIfGp_particle_setColor(
            slot.key, ID_ZF_J_FIRE02_FIRE, &position, &player->tevStr,
            &kPrimColor, &kEnvColor, 0.0f, 0xFF, nullptr, nullptr, nullptr,
            -1, nullptr);
        dComIfGp_particle_levelEmitterOnEventMove(slot.key);
        if (JPABaseEmitter* emitter = dComIfGp_particle_getEmitter(slot.key);
            emitter != nullptr)
        {
            emitter->setParticleCallBackPtr(dPa_control_c::getParticleTracePCB());
            emitter->setUserWork(reinterpret_cast<uintptr_t>(&slot.velocity));
            emitter->setGlobalScale(
                JGeometry::TVec3<f32>(kFlameScale, kFlameScale, kFlameScale));
        }
        slot.lastPosition = position;
        slot.hasLastPosition = true;
    }

    ++g_elapsedFrames;
}

void update(ModContext*, void* args, void*, void*) {
    if (g_ownerId == fpcM_ERROR_PROCESS_ID_e) {
        return;
    }

    auto* effectPlayer = static_cast<daAlink_c*>(
        fopAcM_SearchByID(g_ownerId));
    if (effectPlayer == nullptr) {
        stop_effect();
        return;
    }
    update_effect(effectPlayer);
}

void clear_after_scene_change(ModContext*, void*, void* retval, void*) {
    if (retval == nullptr || *static_cast<int*>(retval) == 0) {
        return;
    }

    stop_effect();
}

}  // namespace fire_sphere

void begin_fire_sphere_use(daAlink_c* player) {
    using namespace fire_sphere;
    if (player == nullptr) {
        return;
    }
    if (g_ownerId != fpcM_ERROR_PROCESS_ID_e) {
        if (g_ownerId == fopAcM_GetID(player)) {
            stop_effect();
            svc_log->info(mod_ctx, "Din's Fire deactivated");
        }
        return;
    }
    if (player->checkWolf() || player->checkEventRun()) {
        return;
    }
    if (player->mGrabItemAcKeep.getActor() != nullptr) {
        svc_log->warn(mod_ctx, "Put away the held item before using Din's Fire");
        return;
    }
    if (dComIfGs_getOil() == 0) {
        player->seStartSystem(Z2SE_SYS_ERROR);
        svc_log->warn(mod_ctx, "Din's Fire requires Lantern oil");
        return;
    }

    start_effect(player);
    svc_log->info(mod_ctx, "Din's Fire activated");
}
