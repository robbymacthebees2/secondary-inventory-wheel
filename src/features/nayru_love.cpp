namespace nayru_love {
namespace {

DEFINE_HOOK(&daAlink_c::checkDamageAction, NayruLoveDamageAction);

constexpr u16 kBarrierDurationFrames = 10 * 30;
constexpr u16 kCooldownDurationFrames = 20 * 30;
constexpr size_t kBarrierEmitterCount = 12;

const GXColor kBreakPrimColor = {220, 248, 255, 255};
const GXColor kBreakEnvColor = {53, 146, 255, 255};
const GXColor kBarrierPrimColor = {145, 215, 255, 255};
const GXColor kBarrierEnvColor = {28, 72, 196, 255};
const JGeometry::TVec3<f32> kBarrierEmitterScale{1.35f, 1.35f, 1.35f};

struct BarrierEmitter {
    u32 key = 0;
    cXyz velocity{};
    cXyz lastPosition{};
    bool hasLastPosition = false;
};

bool g_active = false;
fpc_ProcID g_ownerId = fpcM_ERROR_PROCESS_ID_e;
u16 g_activeFrames = 0;
u16 g_cooldownFrames = 0;
bool g_cooldownConfigReadFailureReported = false;
std::array<BarrierEmitter, kBarrierEmitterCount> g_barrierEmitters{};

bool cooldown_disabled() {
    bool disabled = false;
    if (svc_config->get_bool(mod_ctx, g_nayruLoveCooldownDisabled, &disabled) != MOD_OK) {
        if (!g_cooldownConfigReadFailureReported) {
            svc_log->error(mod_ctx, "failed to read Nayru's Love cooldown setting");
            g_cooldownConfigReadFailureReported = true;
        }
        return false;
    }
    g_cooldownConfigReadFailureReported = false;
    return disabled;
}

void stop_emitters() {
    for (BarrierEmitter& slot : g_barrierEmitters) {
        if (JPABaseEmitter* emitter = dComIfGp_particle_getEmitter(slot.key);
            emitter != nullptr)
        {
            emitter->setParticleCallBackPtr(nullptr);
            emitter->setUserWork(0);
            emitter->stopDrawParticle();
        }
        slot = {};
    }
}

void stop_barrier(bool startCooldown) {
    stop_emitters();
    g_active = false;
    g_ownerId = fpcM_ERROR_PROCESS_ID_e;
    g_activeFrames = 0;
    const bool disableCooldown = cooldown_disabled();
    if (startCooldown && !disableCooldown) {
        g_cooldownFrames = kCooldownDurationFrames;
    } else if (disableCooldown) {
        g_cooldownFrames = 0;
    }
}

void draw_barrier(daAlink_c* player) {
    constexpr f32 kTau = 6.28318530717958647692f;
    constexpr std::array<f32, 3> kRingHeights = {35.0f, 85.0f, 135.0f};
    constexpr std::array<f32, 3> kRingRadii = {34.0f, 54.0f, 34.0f};
    const f32 elapsed = static_cast<f32>(g_activeFrames) * 0.035f;

    for (size_t index = 0; index < g_barrierEmitters.size(); ++index) {
        const size_t ring = index / 4;
        const f32 direction = ring == 1 ? -1.0f : 1.0f;
        const f32 angle = elapsed * direction +
            static_cast<f32>(index % 4) * (kTau / 4.0f);
        cXyz position = player->current.pos;
        position.x += kRingRadii[ring] * std::sin(angle);
        position.y += kRingHeights[ring];
        position.z += kRingRadii[ring] * std::cos(angle);

        BarrierEmitter& slot = g_barrierEmitters[index];
        slot.velocity = slot.hasLastPosition
            ? position - slot.lastPosition
            : cXyz(0.0f, 0.0f, 0.0f);
        slot.key = dComIfGp_particle_setColor(
            slot.key, ID_ZF_J_FIRE02_FIRE, &position, &player->tevStr,
            &kBarrierPrimColor, &kBarrierEnvColor, 0.0f, 0xFF, nullptr,
            nullptr, nullptr, -1, nullptr);
        dComIfGp_particle_levelEmitterOnEventMove(slot.key);
        if (JPABaseEmitter* emitter = dComIfGp_particle_getEmitter(slot.key);
            emitter != nullptr)
        {
            emitter->setParticleCallBackPtr(dPa_control_c::getParticleTracePCB());
            emitter->setUserWork(reinterpret_cast<uintptr_t>(&slot.velocity));
            emitter->setGlobalScale(kBarrierEmitterScale);
        }
        slot.lastPosition = position;
        slot.hasLastPosition = true;
    }
}

void play_break_effect(daAlink_c* player, const cXyz& position) {
    dComIfGp_particle_setColor(
        0, ID_AK_SN_HITSHIELDLIGHT00, &position, &player->tevStr,
        &kBreakPrimColor, &kBreakEnvColor, 0.0f, 0xFF, nullptr, nullptr,
        nullptr, -1, nullptr);
}

}  // namespace

HookAction before_damage_action(ModContext*, void* args, void* retval, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (!g_active || player == nullptr ||
        fopAcM_GetID(player) != g_ownerId || player->mDamageTimer != 0)
    {
        return HOOK_CONTINUE;
    }

    cXyz hitPosition = player->current.pos;
    bool wasHit = false;
    for (dCcD_Cyl& body : player->mTgCyls) {
        if (!body.ChkTgHit()) {
            continue;
        }
        if (!wasHit) {
            if (const cXyz* hit = body.GetTgHitPosP(); hit != nullptr) {
                hitPosition = *hit;
            }
            wasHit = true;
        }
        body.ResetTgHit();
    }
    if (!wasHit) {
        return HOOK_CONTINUE;
    }

    stop_barrier(true);
    play_break_effect(player, hitPosition);
    *static_cast<BOOL*>(retval) = FALSE;
    return HOOK_SKIP_ORIGINAL;
}

void update(ModContext*, void*, void*, void*) {
    if (g_cooldownFrames > 0) {
        --g_cooldownFrames;
    }
    if (!g_active) {
        return;
    }

    auto* player = static_cast<daAlink_c*>(fopAcM_SearchByID(g_ownerId));
    if (player == nullptr) {
        stop_barrier(false);
        return;
    }
    if (player->checkEventRun() || dComIfGp_event_runCheck() ||
        dComIfGp_isEnableNextStage())
    {
        stop_barrier(true);
        return;
    }
    if (dComIfGp_isPauseFlag()) {
        return;
    }

    draw_barrier(player);
    if (++g_activeFrames >= kBarrierDurationFrames) {
        stop_barrier(true);
        svc_log->info(mod_ctx, "Nayru's Love faded");
    }
}

}  // namespace nayru_love

void begin_nayru_love_use(daAlink_c* player) {
    using namespace nayru_love;
    if (player == nullptr) {
        return;
    }
    if (g_active) {
        if (g_ownerId == fopAcM_GetID(player)) {
            stop_barrier(true);
            svc_log->info(mod_ctx, "Nayru's Love deactivated");
        }
        return;
    }
    if (player->checkWolf() || player->checkEventRun()) {
        return;
    }
    if (!cooldown_disabled() && g_cooldownFrames != 0) {
        player->seStartSystem(Z2SE_SYS_ERROR);
        svc_log->info(mod_ctx, "Nayru's Love is recharging");
        return;
    }

    g_active = true;
    g_ownerId = fopAcM_GetID(player);
    g_activeFrames = 0;
    svc_log->info(mod_ctx, "Nayru's Love activated");
}
