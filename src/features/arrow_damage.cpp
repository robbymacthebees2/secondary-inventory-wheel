namespace arrow_damage {
namespace {

DEFINE_HOOK(&at_power_check, ArrowDamageAttackPower);

bool s_configReadFailureReported = false;

int64_t multiplier_percent() {
    int64_t value = kBulletTimeArrowDamageDefaultPercent;
    if (g_bulletTimeArrowDamageMultiplier == 0 ||
        svc_config->get_int(
            mod_ctx, g_bulletTimeArrowDamageMultiplier, &value) != MOD_OK)
    {
        if (!s_configReadFailureReported) {
            svc_log->warn(mod_ctx, "could not read Bullet Time arrow damage setting");
            s_configReadFailureReported = true;
        }
        return kBulletTimeArrowDamageDefaultPercent;
    }

    s_configReadFailureReported = false;
    return std::clamp(
        value, kBulletTimeArrowDamageMinPercent,
        kBulletTimeArrowDamageMaxPercent);
}

}

void apply_modifier(ModContext*, void* args, void*, void*) {
    auto* attack = mods::arg<dCcU_AtInfo*>(args, 0);
    auto* link = daAlink_getAlinkActorClass();
    if (!bow_aim::bullet_time_active() || attack == nullptr ||
        attack->mpActor != link || attack->mpCollider == nullptr ||
        attack->mHitType != HIT_TYPE_ARROW || attack->mAttackPower == 0)
    {
        return;
    }

    const u32 percent = static_cast<u32>(multiplier_percent());
    attack->mAttackPower = static_cast<u16>(std::min<u32>(
        (static_cast<u32>(attack->mAttackPower) * percent + 99U) / 100U,
        0xFFFFU));
}

}
