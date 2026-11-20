namespace lazy_tweaks_combat {
namespace {

DEFINE_HOOK(&at_power_check, AtPowerCheck);
DEFINE_HOOK(&cc_at_check, CcAtCheck);
DEFINE_HOOK(&daAlink_c::setShieldGuard, ManualShield);
DEFINE_HOOK(&daAlink_c::procMove, CrouchShieldMove);
DEFINE_HOOK(&daAlink_c::procWait, CrouchShieldWait);
DEFINE_HOOK(&daAlink_c::procGuardAttackInit, GuardAttackInit);
DEFINE_HOOK(&daAlink_c::setSmallGuard, SmallGuard);
DEFINE_HOOK(&daAlink_c::procCutNormal, CrouchShieldStabMotion);
DEFINE_HOOK(&daAlink_c::procCutNormalInit, CrouchShieldNormalCutInit);
DEFINE_HOOK(&daAlink_c::procCutFinishInit, CrouchShieldFinishCutInit);

bool s_configReadFailureReported = false;
bool s_crouchStaminaFailureReported = false;
daAlink_c* s_combatLink = nullptr;
daAlink_c* s_crouchShieldLink = nullptr;
daAlink_c* s_crouchStabLink = nullptr;
bool s_crouchShieldActive = false;
bool s_crouchStabPoseActive = false;
daAlink_c* s_crouchShieldTurnLink = nullptr;
bool s_crouchShieldOwnsUpperTurnFlag = false;
bool s_crouchShieldOwnsUpperPitchFlag = false;
s16 s_crouchStabFacingAngle = 0;
s16 s_crouchStabMoveAngle = 0;
s16 s_crouchStabBodyYaw = 0;
s16 s_crouchStabBodyPitch = 0;
u8 s_parryWindowFrames = 0;
u8 s_riposteWindowFrames = 0;
constexpr int kCutNormalStabParam = 4;
constexpr f32 kCrouchStabStaminaCost = 1.0f;

void restore_crouch_stab_root_angles(daAlink_c* link) {
    link->shape_angle.y = s_crouchStabFacingAngle;
    link->current.angle.y = s_crouchStabFacingAngle;
    link->mMoveAngle = s_crouchStabMoveAngle;
    link->mProcVar2.field_0x300c = s_crouchStabFacingAngle;
}

void restore_crouch_stab_body_angles(daAlink_c* link) {
    link->mBodyAngle.y = s_crouchStabBodyYaw;
    link->mBodyAngle.x = s_crouchStabBodyPitch;
    link->field_0x30c8 = s_crouchStabBodyYaw >> 1;
    link->field_0x310a = s_crouchStabBodyPitch;
}

void update_crouch_shield_body_angles(daAlink_c* link) {
    const f32 stickX = mDoCPd_c::getStickX(PAD_1);
    const f32 stickY = mDoCPd_c::getStickY(PAD_1);
    const s16 targetBodyYaw = static_cast<s16>(
        std::clamp(-stickX, -1.0f, 1.0f) * 0x4000);
    const s16 targetBodyPitch = static_cast<s16>(
        std::clamp(-stickY, -1.0f, 1.0f) * 0x2000);
    cLib_addCalcAngleS(&link->mBodyAngle.y, targetBodyYaw, 4, 0xC00, 0x180);
    cLib_addCalcAngleS(&link->mBodyAngle.x, targetBodyPitch, 4, 0xC00, 0x180);
    link->field_0x30c8 = link->mBodyAngle.y >> 1;
    link->field_0x310a = link->mBodyAngle.x;
}

class CrouchStabArmAnimation final : public J3DAnmTransform {
public:
    CrouchStabArmAnimation()
        : J3DAnmTransform(0, nullptr, nullptr, nullptr),
          m_crouchPose(nullptr), m_stabPose(nullptr), m_stabFrameCtrl(nullptr) {}

    void configure(
        J3DAnmTransform* crouchPose, J3DAnmTransform* stabPose,
        const daPy_frameCtrl_c* stabFrameCtrl,
        const std::vector<u16>& swordArmJoints)
    {
        m_crouchPose = crouchPose;
        m_stabPose = stabPose;
        m_stabFrameCtrl = stabFrameCtrl;
        m_swordArmJoints = swordArmJoints;
        mFrameMax = stabPose != nullptr ? stabPose->getFrameMax() : 0;
        mAttribute = stabPose != nullptr ? stabPose->getAttribute() : 0;
    }

    void clear() {
        m_crouchPose = nullptr;
        m_stabPose = nullptr;
        m_stabFrameCtrl = nullptr;
        m_swordArmJoints.clear();
    }

    s32 getKind() const override { return 0; }

    void getTransform(u16 joint, J3DTransformInfo* transform) const override {
        for (u16 armJoint : m_swordArmJoints) {
            if (joint == armJoint && m_stabPose != nullptr) {
                if (m_stabFrameCtrl != nullptr) {
                    m_stabPose->setFrame(m_stabFrameCtrl->getFrame());
                }
                m_stabPose->getTransform(joint, transform);
                return;
            }
        }

        if (m_crouchPose != nullptr) {
            m_crouchPose->getTransform(joint, transform);
        } else if (m_stabPose != nullptr) {
            m_stabPose->setFrame(getFrame());
            m_stabPose->getTransform(joint, transform);
        }
    }

private:
    J3DAnmTransform* m_crouchPose;
    J3DAnmTransform* m_stabPose;
    const daPy_frameCtrl_c* m_stabFrameCtrl;
    std::vector<u16> m_swordArmJoints;
};

CrouchStabArmAnimation s_crouchStabArmAnimation;

int64_t damage_multiplier_percent(ConfigVarHandle setting) {
    int64_t value = kLazyTweaksDamageMultiplierDefaultPercent;
    if (setting == 0 ||
        svc_config->get_int(mod_ctx, setting, &value) != MOD_OK)
    {
        if (!s_configReadFailureReported) {
            svc_log->warn(mod_ctx, "could not read Lazy Tweaks damage settings");
            s_configReadFailureReported = true;
        }
        return kLazyTweaksDamageMultiplierDefaultPercent;
    }

    s_configReadFailureReported = false;
    return std::clamp(
        value, kLazyTweaksDamageMultiplierMinPercent,
        kLazyTweaksDamageMultiplierMaxPercent);
}

bool enabled(ConfigVarHandle setting) {
    bool value = false;
    if (setting == 0 || svc_config->get_bool(mod_ctx, setting, &value) != MOD_OK) {
        if (!s_configReadFailureReported) {
            svc_log->warn(mod_ctx, "could not read Lazy Tweaks combat settings");
            s_configReadFailureReported = true;
        }
        return false;
    }

    s_configReadFailureReported = false;
    return value;
}

bool progressive_combos_enabled() {
    return enabled(g_lazyTweaksProgressiveCombos);
}

void scale_damage(u16& power, int64_t percent) {
    if (power == 0 || percent == 100) {
        return;
    }

    u32 scaled = (static_cast<u32>(power) * static_cast<u32>(percent)) / 100U;
    if (scaled == 0 && percent != 0) {
        scaled = 1;
    }
    power = static_cast<u16>(std::min<u32>(scaled, 0xFFFFU));
}

}

void apply_damage_multipliers(ModContext*, void* args, void*, void*) {
    auto* attack = mods::arg<dCcU_AtInfo*>(args, 0);
    auto* link = daAlink_getAlinkActorClass();
    if (attack == nullptr || attack->mpActor != link || link == nullptr ||
        attack->mpCollider == nullptr || attack->mAttackPower == 0)
    {
        return;
    }

    if (attack->mHitType == HIT_TYPE_ARROW) {
        scale_damage(
            attack->mAttackPower,
            damage_multiplier_percent(g_lazyTweaksBowDamageMultiplier));
    } else if (attack->mHitType == HIT_TYPE_LINK_NORMAL_ATTACK) {
        if (link->checkWolf()) {
            if (!attack->mpCollider->ChkAtType(AT_TYPE_MIDNA_LOCK)) {
                scale_damage(
                    attack->mAttackPower,
                    damage_multiplier_percent(g_lazyTweaksWolfDamageMultiplier));
            }
        } else {
            scale_damage(
                attack->mAttackPower,
                damage_multiplier_percent(g_lazyTweaksSwordDamageMultiplier));
        }
    }
}

void apply_progressive_combo_damage(ModContext*, void* args, void*, void*) {
    if (!progressive_combos_enabled()) {
        return;
    }

    auto* attack = mods::arg<dCcU_AtInfo*>(args, 1);
    auto* link = daAlink_getAlinkActorClass();
    if (attack == nullptr || link == nullptr || link->checkWolf() ||
        attack->mHitType != HIT_TYPE_LINK_NORMAL_ATTACK)
    {
        return;
    }

    if (link->mComboCutCount == 2) {
        scale_damage(attack->mAttackPower, 120);
    } else if (link->mComboCutCount == 3) {
        scale_damage(attack->mAttackPower, 140);
    }
}

bool manual_shield_enabled() {
    return enabled(g_lazyTweaksManualShield);
}

bool try_manual_shield_attack(daAlink_c* player) {
    if (!manual_shield_enabled() || player == nullptr || player->checkWolf() ||
        (!dComIfGs_isEventBit(dSv_event_flag_c::F_0338) &&
         !player->checkNoResetFlg3(daAlink_c::FLG3_TRANING_SHIELD_ATTACK)) ||
        !player->checkGuardActionChange() || player->checkUpperReadyThrowAnime() ||
        player->checkModeFlg(0x70C52) || !player->checkShieldGet() ||
        player->checkNotBattleStage() ||
        (!player->mLinkAcch.ChkGroundHit() && !player->checkMagneBootsOn()) ||
        !mDoCPd_c::getHoldLockR(PAD_1))
    {
        return false;
    }

    player->setBStatus(BUTTON_STATUS_SHIELD_ATTACK);
    if (!mDoCPd_c::getTrigB(PAD_1)) {
        return false;
    }

    return player->procGuardAttackInit() != 0;
}

void apply_crouch_stab_pose(daAlink_c* link) {
    link->setUnderAnime(
        dRes_ID_ALANM_BCK_CROUCHDEFS_e, daAlink_c::UNDER_2,
        0.0f, 0.0f, -1, -1.0f);

    if (link->mpLinkModel == nullptr) {
        return;
    }

    const auto findJointPath = [](J3DJoint* root, u16 target) {
        std::vector<u16> path;
        const auto visit = [&](const auto& self, J3DJoint* first) -> bool {
            for (J3DJoint* joint = first; joint != nullptr; joint = joint->getYounger()) {
                path.push_back(joint->getJntNo());
                if (joint->getJntNo() == target || self(self, joint->getChild())) {
                    return true;
                }
                path.pop_back();
            }
            return false;
        };
        if (!visit(visit, root)) {
            path.clear();
        }
        return path;
    };

    auto* modelData = link->mpLinkModel->getModelData();
    if (modelData == nullptr) {
        return;
    }
    auto* skeleton = modelData->getJointTree().getRootNode();
    if (skeleton == nullptr) {
        return;
    }
    const auto swordHandPath = findJointPath(skeleton, link->mLeftHandJntNo);
    const auto swordItemPath = findJointPath(skeleton, link->mLeftItemJntNo);
    const auto shieldPath = findJointPath(skeleton, link->mRightHandJntNo);
    if (swordHandPath.empty() || shieldPath.empty()) {
        return;
    }
    const auto& swordPath =
        swordItemPath.empty() ? swordHandPath : swordItemPath;
    std::size_t commonPathLength = 0;
    while (commonPathLength < swordPath.size() &&
           commonPathLength < shieldPath.size() &&
           swordPath[commonPathLength] == shieldPath[commonPathLength])
    {
        ++commonPathLength;
    }

    std::vector<u16> swordArmJoints;
    if (commonPathLength < swordPath.size()) {
        swordArmJoints.assign(
            swordPath.begin() + commonPathLength, swordPath.end());
    } else {
        return;
    }
    auto* crouchPose = link->getNowAnmPackUnder(daAlink_c::UNDER_2);
    auto* stabPose = link->getNowAnmPackUpper(daAlink_c::UPPER_0);
    if (crouchPose == nullptr || stabPose == nullptr) {
        return;
    }
    const daPy_frameCtrl_c* stabFrameCtrl =
        stabPose == link->getNowAnmPackUnder(daAlink_c::UNDER_0)
            ? &link->mUnderFrameCtrl[0]
            : &link->mUpperFrameCtrl[0];
    const daPy_frameCtrl_c& stabFrame = *stabFrameCtrl;

    link->setFrameCtrl(
        &link->mUpperFrameCtrl[2], stabPose->getAttribute(),
        stabFrame.getStart(), stabFrame.getEnd(), stabFrame.getRate(),
        stabFrame.getFrame());
    s_crouchStabArmAnimation.configure(
        crouchPose, stabPose, stabFrameCtrl, swordArmJoints);
    link->mNowAnmPackUpper[daAlink_c::UPPER_2].setAnmTransform(
        &s_crouchStabArmAnimation);
    link->mNowAnmPackUpper[daAlink_c::UPPER_2].setRatio(1.0f);
    link->mNowAnmPackUnder[0].setRatio(0.0f);
    link->mNowAnmPackUpper[0].setRatio(0.0f);

    s_crouchStabLink = link;
    s_crouchStabPoseActive = true;
}

void clear_crouch_shield_upper_turn(daAlink_c* link) {
    if (link == s_crouchShieldTurnLink && s_crouchShieldOwnsUpperTurnFlag) {
        link->offModeFlg(daAlink_c::MODE_UNK_40000000);
    }
    if (link == s_crouchShieldTurnLink && s_crouchShieldOwnsUpperPitchFlag) {
        link->offModeFlg(daAlink_c::MODE_UNK_20000000);
    }

    if (link == s_crouchShieldTurnLink) {
        s_crouchShieldTurnLink = nullptr;
        s_crouchShieldOwnsUpperTurnFlag = false;
        s_crouchShieldOwnsUpperPitchFlag = false;
    }
}

bool crouch_stab_input_active(daAlink_c* link) {
    return link != nullptr && s_crouchShieldActive &&
        link == s_crouchShieldLink && enabled(g_lazyTweaksCrouchShield) &&
        link->mEquipItem == 0x103 && link->checkShieldGet() &&
        mDoCPd_c::getHoldLockR(PAD_1);
}

bool consume_crouch_stab_stamina() {
    const auto* service = svc_te_stamina;
    if (service == nullptr || service->header.major_version != 1 ||
        service->header.struct_size <
            offsetof(TwilitEssentialsStaminaService, try_consume) +
                sizeof(service->try_consume) ||
        service->get_state == nullptr || service->try_consume == nullptr)
    {
        return true;
    }

    TwilitEssentialsStaminaState state = TWILIT_ESSENTIALS_STAMINA_STATE_INIT;
    if (service->get_state(mod_ctx, &state) != MOD_OK ||
        state.struct_size <
            offsetof(TwilitEssentialsStaminaState, gameplay) +
                sizeof(state.gameplay))
    {
        if (!s_crouchStaminaFailureReported) {
            svc_log->warn(mod_ctx, "could not read Twilit Essentials stamina state for crouch stab");
            s_crouchStaminaFailureReported = true;
        }
        return true;
    }
    s_crouchStaminaFailureReported = false;

    if (state.enabled == 0 || state.gameplay == 0) {
        return true;
    }

    if (service->try_consume(mod_ctx, kCrouchStabStaminaCost) == MOD_OK) {
        return true;
    }
    if (service->deny != nullptr) {
        service->deny(mod_ctx);
    }
    return false;
}

HookAction force_crouch_stab_cut(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (crouch_stab_input_active(link)) {
        mods::arg_ref<int>(args, 1) = kCutNormalStabParam;
        s_crouchStabFacingAngle = link->shape_angle.y;
        s_crouchStabMoveAngle = link->mMoveAngle;
        s_crouchStabBodyYaw = link->mBodyAngle.y;
        s_crouchStabBodyPitch = link->mBodyAngle.x;
        restore_crouch_stab_root_angles(link);
        restore_crouch_stab_body_angles(link);
        if (!consume_crouch_stab_stamina()) {
            if (retval != nullptr) {
                *static_cast<int*>(retval) = 0;
            }
            return HOOK_SKIP_ORIGINAL;
        }
    }
    return HOOK_CONTINUE;
}

void apply_crouch_stab_after_cut_init(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (retval == nullptr || *static_cast<int*>(retval) == 0 ||
        !crouch_stab_input_active(link) ||
        link->mProcID != daAlink_c::PROC_CUT_NORMAL ||
        link->getCutType() != daPy_py_c::CUT_TYPE_NM_STAB)
    {
        return;
    }

    link->resetCombo(TRUE);
    apply_crouch_stab_pose(link);
    link->field_0x3480 = 0.0f;
    restore_crouch_stab_root_angles(link);
    restore_crouch_stab_body_angles(link);
    link->onModeFlg(daAlink_c::MODE_UNK_40000000);
    link->onModeFlg(daAlink_c::MODE_UNK_20000000);
    update_crouch_shield_body_angles(link);
}

HookAction replace_crouch_shield_finisher(
    ModContext*, void* args, void* retval, void*)
{
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!crouch_stab_input_active(link)) {
        return HOOK_CONTINUE;
    }

    link->resetCombo(TRUE);
    if (link->procCutNormalInit(kCutNormalStabParam) == 0) {
        if (retval != nullptr) {
            *static_cast<int*>(retval) = 0;
        }
        return HOOK_SKIP_ORIGINAL;
    }

    if (retval != nullptr) {
        *static_cast<int*>(retval) = 1;
    }
    return HOOK_SKIP_ORIGINAL;
}

void stop_crouch_stab_lunge(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (s_crouchStabPoseActive && link == s_crouchStabLink &&
        link != nullptr &&
        link->mProcID == daAlink_c::PROC_CUT_NORMAL &&
        link->getCutType() == daPy_py_c::CUT_TYPE_NM_STAB)
    {
        link->mNormalSpeed = 0.0f;
        link->field_0x3480 = 0.0f;
        restore_crouch_stab_root_angles(link);
        link->onModeFlg(daAlink_c::MODE_UNK_40000000);
        link->onModeFlg(daAlink_c::MODE_UNK_20000000);
        update_crouch_shield_body_angles(link);
    }
}

void update_crouch_stab_pose(ModContext*, void*, void*, void*) {
    if (!s_crouchStabPoseActive) {
        return;
    }

    auto* link = daAlink_getAlinkActorClass();
    if (link == s_crouchStabLink && link != nullptr &&
        link->mProcID == daAlink_c::PROC_CUT_NORMAL &&
        link->getCutType() == daPy_py_c::CUT_TYPE_NM_STAB)
    {
        return;
    }

    if (link == s_crouchStabLink && link != nullptr) {
        s_crouchStabArmAnimation.clear();
        link->mNowAnmPackUpper[daAlink_c::UPPER_2].setAnmTransform(nullptr);
        link->mNowAnmPackUnder[daAlink_c::UNDER_2].setRatio(0.0f);
        link->mNowAnmPackUpper[daAlink_c::UPPER_2].setRatio(0.0f);
        link->mNowAnmPackUnder[0].setRatio(1.0f);
        link->mNowAnmPackUpper[0].setRatio(1.0f);
    }

    s_crouchStabLink = nullptr;
    s_crouchStabPoseActive = false;
}

void apply_manual_shield(ModContext*, void* args, void*, void*) {
    if (!enabled(g_lazyTweaksManualShield)) {
        return;
    }

    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link != nullptr && !mDoCPd_c::getHoldLockR(PAD_1) &&
        !mDoCPd_c::getTrigR(PAD_1))
    {
        link->offNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);
    }
}

void maintain_crouch_block_pose(daAlink_c* link) {
    if (link == nullptr || link != s_crouchShieldLink ||
        !s_crouchShieldActive || !enabled(g_lazyTweaksCrouchShield) ||
        !link->checkShieldGet() || !mDoCPd_c::getHoldLockR(PAD_1) ||
        link->mpHIO == nullptr)
    {
        return;
    }

    link->onModeFlg(daAlink_c::MODE_UNK_40000000);
    link->onModeFlg(daAlink_c::MODE_UNK_20000000);
    update_crouch_shield_body_angles(link);
    link->setSingleAnimeBaseSpeed(
        daAlink_c::ANM_CROUCH_DEF,
        link->mpHIO->mGuard.m.mCrouchGuardAnmSpeed,
        link->mpHIO->mGuard.m.mCrouchGuardInterpolation);
    link->onNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);
}

HookAction apply_crouch_shield(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr) {
        clear_crouch_shield_upper_turn(s_crouchShieldTurnLink);
        s_crouchShieldLink = nullptr;
        s_crouchShieldActive = false;
        return HOOK_CONTINUE;
    }

    if (link != s_crouchShieldLink) {
        clear_crouch_shield_upper_turn(s_crouchShieldTurnLink);
        s_crouchShieldLink = link;
        s_crouchShieldActive = false;
    }

    if (!enabled(g_lazyTweaksCrouchShield) ||
        (link->mProcID != daAlink_c::PROC_WAIT &&
         link->mProcID != daAlink_c::PROC_MOVE) ||
        !link->checkShieldGet() || !mDoCPd_c::getHoldLockR(PAD_1) ||
        link->checkAttentionLock() || link->checkGrabAnime() ||
        link->checkUpperReadyThrowAnime() || link->wallGrabTrigger() ||
        link->checkFmChainGrabAnime() || link->checkEventRun())
    {
        clear_crouch_shield_upper_turn(link);
        s_crouchShieldActive = false;
        return HOOK_CONTINUE;
    }

    link->mNormalSpeed = 0.0f;
    if (s_crouchShieldTurnLink != link) {
        clear_crouch_shield_upper_turn(s_crouchShieldTurnLink);
        s_crouchShieldTurnLink = link;
        s_crouchShieldOwnsUpperTurnFlag =
            !link->checkModeFlg(daAlink_c::MODE_UNK_40000000);
        s_crouchShieldOwnsUpperPitchFlag =
            !link->checkModeFlg(daAlink_c::MODE_UNK_20000000);
    }
    link->onModeFlg(daAlink_c::MODE_UNK_40000000);
    link->onModeFlg(daAlink_c::MODE_UNK_20000000);
    update_crouch_shield_body_angles(link);
    link->setSingleAnimeBaseSpeed(
        daAlink_c::ANM_CROUCH_DEF,
        link->mpHIO->mGuard.m.mCrouchGuardAnmSpeed,
        link->mpHIO->mGuard.m.mCrouchGuardInterpolation);
    link->onNoResetFlg2(daPy_py_c::FLG2_UNK_8000000);
    if (!s_crouchShieldActive) {
        link->setSwordVoiceSe(Z2SE_AL_ITEM_TAKEOUT);
        s_crouchShieldActive = true;
    }

    const int direction = link->getCutDirection();
    if (link->mEquipItem == 0x103 &&
        mDoCPd_c::getTrigB(PAD_1))
    {
        if (link->checkCutTurnInput()) {
            if (direction == daAlink_c::DIR_RIGHT) {
                link->procCutTurnInit(1, 1);
            } else if (direction == daAlink_c::DIR_LEFT) {
                link->procCutTurnInit(1, 0);
            }
        } else {
            (void)link->procCutNormalInit(kCutNormalStabParam);
        }
    } else if (link->mEquipItem != 0x103 &&
               mDoCPd_c::getTrigB(PAD_1))
    {
        clear_crouch_shield_upper_turn(link);
        s_crouchShieldActive = false;
        return HOOK_CONTINUE;
    }
    if (mDoCPd_c::getTrigA(PAD_1)) {
        if (direction == daAlink_c::DIR_BACKWARD) {
            link->procBackJumpInit(0);
        } else if (direction == daAlink_c::DIR_LEFT) {
            link->procSideRollInit(2);
        } else if (direction == daAlink_c::DIR_RIGHT) {
            link->procSideRollInit(1);
        } else {
            link->procFrontRollInit();
        }
    }

    if (retval != nullptr) {
        *static_cast<int*>(retval) = 1;
    }
    return HOOK_SKIP_ORIGINAL;
}

void begin_parry_window(ModContext*, void* args, void*, void*) {
    if (!enabled(g_lazyTweaksCombinedParry)) {
        return;
    }

    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr) {
        return;
    }

    s_combatLink = link;
    s_parryWindowFrames = 10;
    s_riposteWindowFrames = 0;
}

void on_shield_hit(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    maintain_crouch_block_pose(link);

    if (!enabled(g_lazyTweaksCombinedParry)) {
        return;
    }

    if (link != nullptr && link == s_combatLink && s_parryWindowFrames != 0) {
        s_parryWindowFrames = 0;
        s_riposteWindowFrames = 30;
    }
}

void update_combat_cancels(ModContext*, void*, void*, void*) {
    auto* link = daAlink_getAlinkActorClass();
    if (link == nullptr) {
        s_combatLink = nullptr;
        s_parryWindowFrames = 0;
        s_riposteWindowFrames = 0;
        return;
    }

    if (link != s_combatLink) {
        s_combatLink = link;
        s_parryWindowFrames = 0;
        s_riposteWindowFrames = 0;
    }

    if (enabled(g_lazyTweaksCombinedParry)) {
        if (s_parryWindowFrames != 0) {
            --s_parryWindowFrames;
        }
        if (s_riposteWindowFrames != 0) {
            if (mDoCPd_c::getTrigB(PAD_1) && !link->checkWolf() &&
                !link->checkEventRun())
            {
                link->procCutFinishInit(2);
                s_riposteWindowFrames = 0;
                s_parryWindowFrames = 0;
            } else {
                --s_riposteWindowFrames;
            }
        }
    } else {
        s_parryWindowFrames = 0;
        s_riposteWindowFrames = 0;
    }

    if (link->checkWolf() || link->checkEventRun()) {
        return;
    }

    const bool inCombo = link->mComboCutCount >= 1 && link->mComboCutCount <= 3;
    const int direction = link->getCutDirection();
    if (inCombo && enabled(g_lazyTweaksDodgeCancel) &&
        (mDoCPd_c::getHoldLockR(PAD_1) || mDoCPd_c::getTrigR(PAD_1)) &&
        mDoCPd_c::getTrigA(PAD_1))
    {
        if (direction == daAlink_c::DIR_LEFT) {
            link->procSideRollInit(2);
        } else if (direction == daAlink_c::DIR_RIGHT) {
            link->procSideRollInit(1);
        } else {
            link->procFrontRollInit();
        }
        return;
    }

    if (inCombo && enabled(g_lazyTweaksJumpStrikeCancel) &&
        mDoCPd_c::getHoldA(PAD_1) &&
        !mDoCPd_c::getTrigR(PAD_1) &&
        !mDoCPd_c::getHoldLockR(PAD_1) &&
        direction == daAlink_c::DIR_NONE &&
        dComIfGs_isEventBit(dSv_event_flag_c::F_0343))
    {
        link->procCutLargeJumpChargeInit();
    }
}

}
