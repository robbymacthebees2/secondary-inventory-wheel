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

struct FlameEmitter {
    u32 key = 0;
    cXyz velocity{};
    cXyz lastPosition{};
    bool hasLastPosition = false;
};

std::array<FlameEmitter, kEmitterCount> g_emitters{};
fpc_ProcID g_ownerId = fpcM_ERROR_PROCESS_ID_e;
u16 g_elapsedFrames = 0;

struct PlayerAttackSnapshot {
    fpc_ProcID playerId = fpcM_ERROR_PROCESS_ID_e;
    u32 type = 0;
    u8 damage = 0;
    u8 material = 0;
    u8 hitMark = 0;
    f32 radius = 0.0f;
    cXyz center{};
    cXyz vector{};
    bool atSet = false;
    bool valid = false;
};

PlayerAttackSnapshot g_playerAttackSnapshot{};

const dCcD_SrcSph kLanternAttackSphereSource = {
    {
        {0x0, {{AT_TYPE_LANTERN_SWING, 0x0, 0x1A}, {0x0, 0x0}, 0x0}},
        {dCcD_SE_NONE, 0x0, 0x0, dCcD_MTRL_FIRE, 0x0},
        {dCcD_SE_NONE, 0x0, 0x0, 0x0, 0x0},
        {0x0},
    },
    {{{0.0f, 0.0f, 0.0f}, kSphereRadius}},
};

dCcD_Stts g_lanternAttackStts{};
dCcD_Sph g_lanternAttackSphere{};
fpc_ProcID g_lanternAttackOwnerId = fpcM_ERROR_PROCESS_ID_e;

void restore_attack_state(daAlink_c* player) {
    if (!g_playerAttackSnapshot.valid || player == nullptr ||
        fopAcM_GetID(player) != g_playerAttackSnapshot.playerId)
    {
        return;
    }

    dCcD_Sph& attack = player->mAtSph;
    attack.SetAtType(g_playerAttackSnapshot.type);
    attack.SetAtAtp(g_playerAttackSnapshot.damage);
    attack.SetAtMtrl(g_playerAttackSnapshot.material);
    attack.SetAtHitMark(g_playerAttackSnapshot.hitMark);
    attack.SetR(g_playerAttackSnapshot.radius);
    attack.SetC(g_playerAttackSnapshot.center);
    attack.SetAtVec(g_playerAttackSnapshot.vector);
    if (g_playerAttackSnapshot.atSet) {
        attack.OnAtSetBit();
    } else {
        attack.OffAtSetBit();
    }
    g_playerAttackSnapshot = {};
}

void apply_blast(daAlink_c* player) {
    if (g_playerAttackSnapshot.valid) {
        restore_attack_state(player);
    }

    dCcS* collisionServer = dComIfG_Ccsp();
    if (collisionServer == nullptr) {
        svc_log->error(mod_ctx, "Could not apply Din's Fire damage");
        return;
    }

    dCcD_Sph& attack = player->mAtSph;
    const cXyz* attackVector = attack.GetAtVecP();
    g_playerAttackSnapshot.playerId = fopAcM_GetID(player);
    g_playerAttackSnapshot.type = attack.GetAtType();
    g_playerAttackSnapshot.damage = attack.GetAtAtp();
    g_playerAttackSnapshot.material = attack.GetAtMtrl();
    g_playerAttackSnapshot.hitMark = attack.GetAtHitMark();
    g_playerAttackSnapshot.radius = attack.GetR();
    g_playerAttackSnapshot.center = attack.GetC();
    g_playerAttackSnapshot.vector = attackVector == nullptr
        ? cXyz(0.0f, 0.0f, 0.0f) : *attackVector;
    g_playerAttackSnapshot.atSet = attack.ChkAtSet();
    g_playerAttackSnapshot.valid = true;

    attack.SetAtType(AT_TYPE_LANTERN_SWING);
    attack.SetAtAtp(kBlastDamage);
    attack.SetAtMtrl(dCcD_MTRL_FIRE);
    attack.SetAtHitMark(1);
    attack.SetR(kSphereRadius);
    cXyz center = player->current.pos;
    center.y += kSphereCenterHeight;
    attack.SetC(center);
    cXyz knockback(0.0f, kBlastKnockback, 0.0f);
    attack.SetAtVec(knockback);
    attack.OnAtSetBit();
    collisionServer->Set(&attack);
    collisionServer->SetMass(&attack, 1);
}

void apply_lantern_blast(daAlink_c* player) {
    if (player == nullptr) {
        return;
    }

    const fpc_ProcID playerId = fopAcM_GetID(player);
    if (g_lanternAttackOwnerId != playerId) {
        g_lanternAttackStts.Init(0xFF, 0xFF, player);
        g_lanternAttackSphere.Set(kLanternAttackSphereSource);
        g_lanternAttackSphere.SetStts(&g_lanternAttackStts);
        g_lanternAttackSphere.ResetAtHit();
        g_lanternAttackOwnerId = playerId;
    }

    dCcS* collisionServer = dComIfG_Ccsp();
    if (collisionServer == nullptr) {
        svc_log->error(mod_ctx, "Could not apply Lantern flame damage");
        return;
    }

    dCcD_Sph& attack = g_lanternAttackSphere;
    attack.SetAtType(AT_TYPE_LANTERN_SWING);
    attack.SetAtAtp(kBlastDamage);
    attack.SetAtMtrl(dCcD_MTRL_FIRE);
    attack.SetAtHitMark(1);
    attack.SetR(kSphereRadius);
    cXyz center = player->current.pos;
    center.y += kSphereCenterHeight;
    attack.SetC(center);
    cXyz knockback(0.0f, kBlastKnockback, 0.0f);
    attack.SetAtVec(knockback);
    attack.OnAtSetBit();
    collisionServer->Set(&attack);
    collisionServer->SetMass(&attack, 1);
}

void clear_lantern_blast() {
    g_lanternAttackSphere.OffAtSetBit();
    g_lanternAttackOwnerId = fpcM_ERROR_PROCESS_ID_e;
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
    if (g_playerAttackSnapshot.valid) {
        auto* player = static_cast<daAlink_c*>(
            fopAcM_SearchByID(g_playerAttackSnapshot.playerId));
        restore_attack_state(player);
    }
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

HookAction restore_player_attack(ModContext*, void* args, void*, void*) {
    restore_attack_state(mods::arg<daAlink_c*>(args, 0));
    return HOOK_CONTINUE;
}

void clear_after_scene_change(ModContext*, void*, void* retval, void*) {
    if (retval == nullptr || *static_cast<int*>(retval) == 0) {
        return;
    }

    stop_effect();
    clear_lantern_blast();
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
