namespace flurry_rush {
namespace {

DEFINE_HOOK(&cCcS::Set, FlurryColliderSet);
DEFINE_HOOK(&daAlink_c::checkDamageAction, FlurryLinkDamageAction);

constexpr f32 kPerfectDodgeMargin = 45.0f;
constexpr f32 kFlurryMeleeDistance = 120.0f;
constexpr auto kFlurryDuration = std::chrono::seconds(3);

daAlink_c* s_dodgeOwner = nullptr;
u16 s_dodgeProc = daAlink_c::PROC_WAIT;
bool s_dodgeTriggered = false;
daAlink_c* s_flurryOwner = nullptr;
fopAc_ac_c* s_flurryTarget = nullptr;
std::chrono::steady_clock::time_point s_flurryStarted{};
bool s_flurryActive = false;
bool s_flurryLinkSlowed = false;
bool s_flurryMeleePositioned = false;

bool flurry_enabled() {
    return get_bool_option(g_flurryRushEnabled, false);
}

bool dodge_active(const daAlink_c* link) {
    return link != nullptr &&
        (link->mProcID == daAlink_c::PROC_SIDESTEP ||
            link->mProcID == daAlink_c::PROC_BACK_JUMP);
}

void update_dodge_attempt(daAlink_c* link) {
    if (!dodge_active(link)) {
        s_dodgeOwner = nullptr;
        s_dodgeProc = daAlink_c::PROC_WAIT;
        s_dodgeTriggered = false;
        return;
    }

    if (s_dodgeOwner != link || s_dodgeProc != link->mProcID) {
        s_dodgeOwner = link;
        s_dodgeProc = link->mProcID;
        s_dodgeTriggered = false;
    }
}

void disable_link_targets(daAlink_c* link) {
    if (link == nullptr) return;
    for (dCcD_Cyl& body : link->mTgCyls) {
        body.OffTgSetBit();
        body.ResetTgHit();
    }
    link->mAtSph.OffTgSetBit();
    link->mAtSph.ResetTgHit();
}

f32 axis_gap(f32 minA, f32 maxA, f32 minB, f32 maxB) {
    if (maxA < minB) return minB - maxA;
    if (maxB < minA) return minA - maxB;
    return 0.0f;
}

bool collider_near_link(cCcD_Obj* attack, daAlink_c* link) {
    cCcD_ShapeAttr* attackShape = attack->GetShapeAttr();
    if (attackShape == nullptr) return false;
    attackShape->CalcAabBox();
    const cM3dGAab& attackBounds = attackShape->GetWorkAab();
    const f32 marginSquared = kPerfectDodgeMargin * kPerfectDodgeMargin;

    for (dCcD_Cyl& body : link->mTgCyls) {
        if (!body.ChkTgSet() || (attack->GetAtGrp() & body.GetTgGrp()) == 0 ||
            (attack->GetAtType() & body.GetTgType()) == 0)
        {
            continue;
        }

        cCcD_ShapeAttr* bodyShape = body.GetShapeAttr();
        if (bodyShape == nullptr) continue;
        bodyShape->CalcAabBox();
        const cM3dGAab& bodyBounds = bodyShape->GetWorkAab();
        const f32 x = axis_gap(attackBounds.GetMinX(), attackBounds.GetMaxX(),
            bodyBounds.GetMinX(), bodyBounds.GetMaxX());
        const f32 y = axis_gap(attackBounds.GetMinY(), attackBounds.GetMaxY(),
            bodyBounds.GetMinY(), bodyBounds.GetMaxY());
        const f32 z = axis_gap(attackBounds.GetMinZ(), attackBounds.GetMaxZ(),
            bodyBounds.GetMinZ(), bodyBounds.GetMaxZ());
        if (x * x + y * y + z * z <= marginSquared) return true;
    }
    return false;
}

void stop_flurry() {
    s_flurryActive = false;
    s_flurryOwner = nullptr;
    s_flurryTarget = nullptr;
    s_flurryLinkSlowed = false;
    s_flurryMeleePositioned = false;
    s_flurryStarted = {};
    bow_aim::set_flurry_rush_slow_motion(nullptr, false, false);
}

void try_start_flurry(cCcD_Obj* attack) {
    if (attack == nullptr || !attack->ChkAtSet() ||
        attack->GetAtType() == AT_TYPE_0 || s_flurryActive ||
        !flurry_enabled())
    {
        return;
    }

    daAlink_c* link = daAlink_getAlinkActorClass();
    update_dodge_attempt(link);
    if (!dodge_active(link) || s_dodgeTriggered ||
        !link->checkAttentionLock())
    {
        return;
    }

    fopAc_ac_c* target = link->mTargetedActor;
    fopAc_ac_c* attacker = attack->GetAc();
    if (target == nullptr || attacker == nullptr || attacker == link ||
        !daAlink_c::checkEnemyGroup(target) ||
        (attacker != target && !daAlink_c::checkEnemyGroup(attacker)) ||
        !collider_near_link(attack, link))
    {
        return;
    }

    s_flurryOwner = link;
    s_flurryTarget = target;
    s_flurryStarted = std::chrono::steady_clock::now();
    s_flurryActive = true;
    s_flurryLinkSlowed = true;
    s_flurryMeleePositioned = false;
    s_dodgeTriggered = true;
    disable_link_targets(link);
    bow_aim::set_flurry_rush_slow_motion(link, true, true);
}

bool sword_attack_active(const daAlink_c* link) {
    if (link == nullptr || link->mEquipItem != 0x103) return false;
    switch (link->mProcID) {
    case daAlink_c::PROC_CUT_NORMAL:
    case daAlink_c::PROC_CUT_FINISH:
    case daAlink_c::PROC_CUT_FINISH_JUMP_UP:
    case daAlink_c::PROC_CUT_REVERSE:
    case daAlink_c::PROC_CUT_JUMP:
    case daAlink_c::PROC_CUT_TURN:
    case daAlink_c::PROC_CUT_DOWN:
    case daAlink_c::PROC_CUT_HEAD:
    case daAlink_c::PROC_CUT_LARGE_JUMP:
        return true;
    default:
        return false;
    }
}

void move_link_to_target(daAlink_c* link) {
    if (link == nullptr || s_flurryTarget == nullptr ||
        link->mTargetedActor != s_flurryTarget)
    {
        return;
    }

    const cXyz targetPosition = s_flurryTarget->current.pos;
    const f32 deltaX = link->current.pos.x - targetPosition.x;
    const f32 deltaZ = link->current.pos.z - targetPosition.z;
    const f32 distance = cXyz(deltaX, 0.0f, deltaZ).abs();
    if (distance <= kFlurryMeleeDistance || distance < 0.001f) return;

    const f32 scale = kFlurryMeleeDistance / distance;
    const cXyz destination(
        targetPosition.x + deltaX * scale, link->current.pos.y,
        targetPosition.z + deltaZ * scale);
    link->current.pos = destination;
    link->old.pos = destination;
    link->mCcStts.ClrCcMove();
}

void suppress_link_collider(cCcD_Obj* collider) {
    if (!s_flurryActive || s_flurryOwner == nullptr || collider == nullptr ||
        collider->GetAc() != s_flurryOwner)
    {
        return;
    }
    for (dCcD_Cyl& body : s_flurryOwner->mTgCyls) {
        if (collider == &body) {
            body.OffTgSetBit();
            body.ResetTgHit();
            return;
        }
    }
    if (collider == &s_flurryOwner->mAtSph) {
        s_flurryOwner->mAtSph.OffTgSetBit();
        s_flurryOwner->mAtSph.ResetTgHit();
    }
}

HookAction before_collider_set(ModContext*, void* args, void*, void*) {
    auto* collider = mods::arg<cCcD_Obj*>(args, 1);
    try_start_flurry(collider);
    suppress_link_collider(collider);
    return HOOK_CONTINUE;
}

HookAction before_link_damage_action(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!s_flurryActive || link == nullptr || link != s_flurryOwner) {
        return HOOK_CONTINUE;
    }
    disable_link_targets(link);
    *static_cast<BOOL*>(retval) = FALSE;
    return HOOK_SKIP_ORIGINAL;
}

}

HookAction update(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr) {
        stop_flurry();
        return HOOK_CONTINUE;
    }

    update_dodge_attempt(link);
    if (!s_flurryActive) return HOOK_CONTINUE;

    const bool targetStillLocked =
        link == s_flurryOwner && link->checkAttentionLock() &&
        link->mTargetedActor == s_flurryTarget && s_flurryTarget != nullptr;
    if (!flurry_enabled() || !targetStillLocked || link->checkEventRun() ||
        dComIfGp_event_runCheck() || dComIfGp_isPauseFlag() ||
        dComIfGp_isEnableNextStage())
    {
        stop_flurry();
        return HOOK_CONTINUE;
    }

    if (!s_flurryMeleePositioned && sword_attack_active(link)) {
        s_flurryLinkSlowed = false;
        s_flurryStarted = std::chrono::steady_clock::now();
        s_flurryMeleePositioned = true;
        bow_aim::set_flurry_rush_slow_motion(link, true, false);
        move_link_to_target(link);
    } else if (s_flurryLinkSlowed &&
        (link->mProcID == daAlink_c::PROC_BACK_JUMP_LAND ||
            link->mProcID == daAlink_c::PROC_SIDESTEP_LAND))
    {
        s_flurryLinkSlowed = false;
        s_flurryStarted = std::chrono::steady_clock::now();
        bow_aim::set_flurry_rush_slow_motion(link, true, false);
    }

    if (!s_flurryLinkSlowed &&
        std::chrono::steady_clock::now() - s_flurryStarted >= kFlurryDuration)
    {
        stop_flurry();
        return HOOK_CONTINUE;
    }

    return HOOK_CONTINUE;
}

HookAction before_collider_set_callback(ModContext* context, void* args, void* retval, void* user) {
    return before_collider_set(context, args, retval, user);
}

HookAction before_link_damage_action_callback(
    ModContext* context, void* args, void* retval, void* user)
{
    return before_link_damage_action(context, args, retval, user);
}

void shutdown() {
    stop_flurry();
    s_dodgeOwner = nullptr;
    s_dodgeProc = daAlink_c::PROC_WAIT;
    s_dodgeTriggered = false;
}

}
