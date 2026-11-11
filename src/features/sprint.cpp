DEFINE_HOOK(&daAlink_c::procMove, WheelSprintProcMove);
DEFINE_HOOK(&daAlink_c::checkMoveDoAction, WheelSprintCheckMoveDoAction);
DEFINE_HOOK(&daAlink_c::setDoubleAnime, WheelSprintSetDoubleAnime);
DEFINE_HOOK(&daAlink_c::getMainBckData, WheelSprintGetMainBckData);
DEFINE_HOOK(&daAlink_c::setSpeedAndAngleWolf, WheelWolfSprintMovement);
DEFINE_HOOK(&daAlink_c::procWolfDash, WheelWolfSprintDash);
DEFINE_HOOK(&daAlink_c::procWolfAutoJumpInit, WheelWolfSprintJump);
DEFINE_HOOK(&daAlink_c::commonProcInit, WheelSprintCommonProcInit);
DEFINE_HOOK(&dMeter2Draw_c::draw, WheelSprintStaminaMeterDraw);

constexpr f32 kSprintStaminaMaximum = 100.0f;
constexpr f32 kSprintStaminaExternalCostPerSecond = 0.90f * 30.0f;
constexpr auto kSprintRollWindow = std::chrono::milliseconds(100);

struct SprintRollWindow {
    daAlink_c* link = nullptr;
    u16 linkId = 0;
    std::chrono::steady_clock::time_point started{};
    bool active = false;
};

enum class ExternalStaminaState {
    Unavailable,
    Disabled,
    Enabled,
    Error,
};

struct SprintStaminaState {
    daAlink_c* link = nullptr;
    u16 linkId = 0;
    f32 value = kSprintStaminaMaximum;
    bool exhausted = false;
    bool sprintActive = false;
    bool wolfSprintActive = false;
    bool externalWasEnabled = false;
    std::chrono::steady_clock::time_point lastUpdate{};
};

SprintStaminaState g_sprintStamina{};
SprintRollWindow g_sprintRollWindow{};
daAlink_c* g_sprintOwner = nullptr;
daAlink_c* g_wolfSprintOwner = nullptr;

struct WolfSprintJumpScope {
    daAlink_c* link = nullptr;
    void* args = nullptr;
    f32 speed = 0.0f;
    bool carry = false;
};

thread_local std::vector<WolfSprintJumpScope> g_wolfSprintJumpScopes;

f32 sprint_stamina_maximum() {
    return kSprintStaminaMaximum;
}

void reset_sprint_stamina(daAlink_c* link) {
    g_sprintStamina = {};
    g_sprintStamina.link = link;
    g_sprintStamina.linkId = link != nullptr ? link->setID : 0;
    g_sprintStamina.value = sprint_stamina_maximum();
    g_sprintStamina.lastUpdate = std::chrono::steady_clock::now();
}

ExternalStaminaState external_stamina_state(
    TwilitEssentialsStaminaState* state = nullptr)
{
    const auto* service = svc_te_stamina;
    if (service == nullptr || service->header.major_version != 1 ||
        service->header.struct_size <
            offsetof(TwilitEssentialsStaminaService, restore) ||
        service->get_state == nullptr || service->drain == nullptr)
    {
        return ExternalStaminaState::Unavailable;
    }

    TwilitEssentialsStaminaState current = TWILIT_ESSENTIALS_STAMINA_STATE_INIT;
    if (service->get_state(mod_ctx, &current) != MOD_OK ||
        current.struct_size < TWILIT_ESSENTIALS_STAMINA_STATE_V1_0_SIZE ||
        !std::isfinite(current.current) || !std::isfinite(current.maximum) ||
        current.current < 0.0f || current.maximum <= 0.0f)
    {
        return ExternalStaminaState::Error;
    }
    if (state != nullptr) {
        *state = current;
    }
    g_sprintStamina.externalWasEnabled = current.enabled != 0;
    return current.enabled != 0 ? ExternalStaminaState::Enabled :
                                  ExternalStaminaState::Disabled;
}

bool external_sprint_source_enabled(uint32_t source) {
    const auto* service = svc_te_stamina;
    if (service == nullptr ||
        service->header.struct_size < sizeof(TwilitEssentialsStaminaService) ||
        service->get_source == nullptr)
    {
        return false;
    }
    TwilitEssentialsStaminaSourceInfo info =
        TWILIT_ESSENTIALS_STAMINA_SOURCE_INFO_INIT;
    return service->get_source(mod_ctx, source, &info) == MOD_OK &&
        info.enabled != 0;
}

f32 external_sprint_drain_rate(uint32_t source) {
    const auto* service = svc_te_stamina;
    if (service == nullptr ||
        service->header.struct_size < sizeof(TwilitEssentialsStaminaService) ||
        service->get_source == nullptr)
    {
        return kSprintStaminaExternalCostPerSecond;
    }
    TwilitEssentialsStaminaSourceInfo info =
        TWILIT_ESSENTIALS_STAMINA_SOURCE_INFO_INIT;
    if (service->get_source(mod_ctx, source, &info) != MOD_OK ||
        !std::isfinite(info.cost_multiplier) || info.cost_multiplier < 0.0f)
    {
        return -1.0f;
    }
    return info.enabled != 0 ?
        kSprintStaminaExternalCostPerSecond * info.cost_multiplier : 0.0f;
}

bool sprint_stamina_available(daAlink_c* link, bool wolf) {
    if (link == nullptr) {
        return false;
    }
    if (g_sprintStamina.link != link || g_sprintStamina.linkId != link->setID) {
        reset_sprint_stamina(link);
    }

    TwilitEssentialsStaminaState external{};
    const ExternalStaminaState provider = external_stamina_state(&external);
    if (provider == ExternalStaminaState::Enabled) {
        if (!external.gameplay) {
            return true;
        }
        if (external.exhausted || external.current <= 0.0f) {
            if (svc_te_stamina->deny != nullptr) {
                svc_te_stamina->deny(mod_ctx);
            }
            return false;
        }
        const uint32_t source = wolf ?
            TWILIT_ESSENTIALS_STAMINA_SOURCE_WOLF_SPRINT :
            TWILIT_ESSENTIALS_STAMINA_SOURCE_SPRINT;
        if (external_sprint_source_enabled(source)) {
            return false;
        }
        return true;
    }
    if (provider == ExternalStaminaState::Error) {
        return false;
    }
    if (!sprintStaminaEnabled()) {
        return true;
    }
    return !g_sprintStamina.exhausted && g_sprintStamina.value > 0.0f;
}

bool sprint_roll_window_eligible(daAlink_c* link) {
    return link != nullptr && sprintEnabled() &&
        link->mProcID == daAlink_c::PROC_MOVE &&
        link->doTrigger() && link->checkInputOnR() &&
        link->mLinkAcch.ChkGroundHit() && !link->checkWolf() &&
        !link->checkEventRun() && !dComIfGp_event_runCheck() &&
        !link->checkMagneBootsOn() && !link->getSumouMode() &&
        link->mGrabItemAcKeep.getActor() == nullptr &&
        sprint_stamina_available(link, false);
}

HookAction defer_sprint_roll_for_window(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr || dComIfGp_getDoStatus() != BUTTON_STATUS_UNK_121 ||
        !sprint_roll_window_eligible(link))
    {
        return HOOK_CONTINUE;
    }

    g_sprintRollWindow = {
        .link = link,
        .linkId = link->setID,
        .started = std::chrono::steady_clock::now(),
        .active = true,
    };
    if (retval != nullptr) {
        *static_cast<BOOL*>(retval) = FALSE;
    }
    return HOOK_SKIP_ORIGINAL;
}

bool sprint_requested(daAlink_c* link) {
    if (link == nullptr || !sprintEnabled() || !link->doButton()) {
        return false;
    }
    if (g_sprintRollWindow.active && g_sprintRollWindow.link == link &&
        g_sprintRollWindow.linkId == link->setID &&
        std::chrono::steady_clock::now() - g_sprintRollWindow.started <
            kSprintRollWindow)
    {
        return false;
    }
    const bool canSprint = link->mProcID == daAlink_c::PROC_MOVE &&
        !link->doTrigger() && link->checkInputOnR() &&
        link->mLinkAcch.ChkGroundHit() && !link->checkWolf() &&
        !link->checkEventRun() && !dComIfGp_event_runCheck() &&
        !link->checkMagneBootsOn() && !link->getSumouMode() &&
        link->mGrabItemAcKeep.getActor() == nullptr;
    return canSprint && sprint_stamina_available(link, false);
}

bool sprint_ground_movement_proc(daAlink_c* link) {
    switch (link->mProcID) {
    case daAlink_c::PROC_GRAB_WAIT:
        return !link->checkWolf() && link->checkGrabAnime() &&
            link->mGrabItemAcKeep.getActor() != nullptr;
    case daAlink_c::PROC_WAIT:
    case daAlink_c::PROC_MOVE:
    case daAlink_c::PROC_ATN_MOVE:
    case daAlink_c::PROC_ATN_ACTOR_WAIT:
    case daAlink_c::PROC_ATN_ACTOR_MOVE:
    case daAlink_c::PROC_WAIT_TURN:
    case daAlink_c::PROC_MOVE_TURN:
    case daAlink_c::PROC_WOLF_WAIT:
    case daAlink_c::PROC_WOLF_MOVE:
    case daAlink_c::PROC_WOLF_DASH:
    case daAlink_c::PROC_WOLF_WAIT_TURN:
    case daAlink_c::PROC_WOLF_ATN_AC_MOVE:
        return true;
    default:
        return false;
    }
}

bool wolf_sprint_requested(daAlink_c* link) {
    if (link == nullptr || !wolfSprintEnabled() || !link->checkWolf() ||
        link->mpHIO == nullptr || !link->doButton() ||
        !link->checkInputOnR() || !link->checkWolfDashMode() ||
        !link->mLinkAcch.ChkGroundHit() ||
        (link->mProcID != daAlink_c::PROC_WOLF_WAIT &&
         link->mProcID != daAlink_c::PROC_WOLF_MOVE &&
         link->mProcID != daAlink_c::PROC_WOLF_DASH) ||
        link->checkPlayerDemoMode() || link->checkEventRun() ||
        dComIfGp_event_runCheck() || link->checkAttentionState() ||
        link->checkSlope() || link->checkNotJumpSinkLimit() ||
        link->checkWolfGrabAnime() ||
        link->mGrabItemAcKeep.getActor() != nullptr)
    {
        return false;
    }
    return sprint_stamina_available(link, true);
}

f32 wolf_dash_speed(daAlink_c* link) {
    const auto& move = link->mpHIO->mWolf.mWlMove.m;
    if (link->checkWolfSlowDash()) {
        return move.mADashMaxSpeedSlow;
    }
    if (link->field_0x2fc7 == 2) {
        return move.mADashMaxSpeedSlow2;
    }
    return move.mADashMaxSpeed;
}

HookAction before_sprint_proc_move(ModContext*, void* args, void* retval, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    g_sprintOwner = nullptr;
    if (g_sprintRollWindow.active) {
        const bool sameLink = link == g_sprintRollWindow.link &&
            link != nullptr && link->setID == g_sprintRollWindow.linkId;
        if (!sameLink || !sprintEnabled() ||
            link->mProcID != daAlink_c::PROC_MOVE)
        {
            g_sprintRollWindow = {};
        } else {
            const auto heldFor =
                std::chrono::steady_clock::now() - g_sprintRollWindow.started;
            if (heldFor < kSprintRollWindow && link->doButton()) {
                return HOOK_CONTINUE;
            }
            const bool releasedInWindow =
                heldFor < kSprintRollWindow && !link->doButton();
            g_sprintRollWindow = {};
            if (releasedInWindow && link->mLinkAcch.ChkGroundHit() &&
                !link->checkEventRun() && !dComIfGp_event_runCheck())
            {
                if (!link->checkAttentionLock() && link->checkInputOnR()) {
                    link->shape_angle.y = link->mMoveAngle;
                }
                const int rollStarted = link->procFrontRollInit();
                if (rollStarted != 0) {
                    if (retval != nullptr) {
                        *static_cast<int*>(retval) = rollStarted;
                    }
                    return HOOK_SKIP_ORIGINAL;
                }
            }
        }
    }
    if (!sprint_requested(link)) {
        return HOOK_CONTINUE;
    }
    link->mMaxSpeed *= static_cast<f32>(sprintSpeedPercent()) / 100.0f;
    g_sprintOwner = link;
    g_sprintStamina.sprintActive = true;
    return HOOK_CONTINUE;
}

void after_sprint_proc_move(ModContext*, void*, void*, void*) {
    g_sprintOwner = nullptr;
}

bool sprint_animation_active(const daAlink_c* link) {
    return link != nullptr && g_sprintOwner == link &&
        link->mProcID == daAlink_c::PROC_MOVE;
}

HookAction adjust_sprint_animation_cadence(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!sprint_animation_active(link) || link->mMaxSpeed <= 0.0f) {
        return HOOK_CONTINUE;
    }
    const f32 speedBonus =
        std::max(0.0f, link->getMoveGroundAngleSpeedRate() - 1.0f);
    const f32 cadence = 1.0f + 0.5f * speedBonus;
    for (int layer = 0; layer < 2; ++layer) {
        const auto animation =
            mods::arg<daAlink_c::daAlink_ANM>(args, 4 + layer);
        if (animation == daAlink_c::ANM_RUN ||
            animation == daAlink_c::ANM_RUN_B)
        {
            mods::arg_ref<f32>(args, 2 + layer) *= cadence;
        }
    }
    return HOOK_CONTINUE;
}

HookAction select_sprint_run_animation(ModContext*, void* args, void*, void*) {
    const auto* link = mods::arg<const daAlink_c*>(args, 0);
    auto& animation = mods::arg_ref<daAlink_c::daAlink_ANM>(args, 1);
    if (sprint_animation_active(link) && animation == daAlink_c::ANM_RUN) {
        animation = daAlink_c::ANM_RUN_B;
    }
    return HOOK_CONTINUE;
}

f32 sprint_jump_speed_multiplier(daAlink_c* link) {
    if (!sprint_requested(link) || link->mpHIO == nullptr) {
        return 1.0f;
    }
    const f32 runSpeed = link->mpHIO->mMove.m.mMaxSpeed;
    if (runSpeed <= 0.0f) {
        return 1.0f;
    }
    return std::clamp(
        link->mNormalSpeed / runSpeed, 1.0f,
        static_cast<f32>(sprintSpeedPercent()) / 100.0f);
}

void apply_sprint_jump_speed(daAlink_c* link, f32 multiplier) {
    if (link == nullptr || multiplier <= 1.0f) {
        return;
    }
    link->mNormalSpeed *= multiplier;
    link->speedF *= multiplier;
    link->mMaxSpeed *= multiplier;
}

HookAction before_wolf_sprint_movement(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (!wolf_sprint_requested(link)) {
        if (link != nullptr && g_wolfSprintOwner == link) {
            if (link->mProcID == daAlink_c::PROC_WOLF_DASH &&
                link->mpHIO != nullptr)
            {
                link->mMaxSpeed = wolf_dash_speed(link);
            }
            g_wolfSprintOwner = nullptr;
        }
        return HOOK_CONTINUE;
    }
    link->field_0x30d0 = std::max<int>(link->field_0x30d0, 2);
    link->mMaxSpeed = wolf_dash_speed(link) *
        (static_cast<f32>(wolfSprintSpeedPercent()) / 100.0f);
    g_wolfSprintOwner = link;
    g_sprintStamina.wolfSprintActive = true;
    return HOOK_CONTINUE;
}

HookAction before_wolf_sprint_jump(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const bool carry = link != nullptr && wolfSprintEnabled() &&
        link->checkWolf() && link->checkWolfDashMode() &&
        g_wolfSprintOwner == link && sprint_ground_movement_proc(link) &&
        !link->checkPlayerDemoMode() && !link->checkEventRun() &&
        !dComIfGp_event_runCheck();
    g_wolfSprintJumpScopes.push_back(
        {link, args, carry ? link->speedF : 0.0f, carry});
    return HOOK_CONTINUE;
}

void after_wolf_sprint_jump(ModContext*, void* args, void* retval, void*) {
    if (g_wolfSprintJumpScopes.empty() ||
        g_wolfSprintJumpScopes.back().args != args)
    {
        return;
    }
    const WolfSprintJumpScope saved = g_wolfSprintJumpScopes.back();
    g_wolfSprintJumpScopes.pop_back();
    if (!saved.carry || retval == nullptr ||
        !*static_cast<int*>(retval) ||
        saved.link->mProcID != daAlink_c::PROC_WOLF_AUTO_JUMP)
    {
        return;
    }
    saved.link->mNormalSpeed = saved.speed;
    saved.link->speedF = saved.speed;
    saved.link->mMaxSpeed = std::max(saved.link->mMaxSpeed, saved.speed);
}

HookAction before_sprint_common_proc_init(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    const auto nextProc =
        mods::arg<daAlink_c::daAlink_PROC>(args, 1);
    if (g_wolfSprintOwner == link &&
        nextProc != daAlink_c::PROC_WOLF_MOVE &&
        nextProc != daAlink_c::PROC_WOLF_DASH &&
        nextProc != daAlink_c::PROC_WOLF_WAIT)
    {
        g_wolfSprintOwner = nullptr;
    }
    return HOOK_CONTINUE;
}

void update_sprint_stamina(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr || link != daAlink_getAlinkActorClass()) {
        return;
    }
    if (g_sprintStamina.link != link || g_sprintStamina.linkId != link->setID) {
        reset_sprint_stamina(link);
    }

    const bool sprintActive = g_sprintStamina.sprintActive;
    const bool wolfSprintActive = g_sprintStamina.wolfSprintActive;
    g_sprintStamina.sprintActive = false;
    g_sprintStamina.wolfSprintActive = false;

    const auto now = std::chrono::steady_clock::now();
    const f32 elapsed = std::clamp(
        std::chrono::duration<f32>(now - g_sprintStamina.lastUpdate).count(),
        0.0f, 0.25f);
    g_sprintStamina.lastUpdate = now;
    if (link->checkDeadHP() || link->checkSceneChangeAreaStart()) {
        reset_sprint_stamina(link);
        return;
    }
    if (dComIfGp_isPauseFlag() || dComIfGp_event_runCheck()) {
        return;
    }

    const ExternalStaminaState provider = external_stamina_state();
    if (provider == ExternalStaminaState::Enabled) {
        if (link->checkPlayerDemoMode()) {
            return;
        }
        f32 drainRate = 0.0f;
        if (sprintActive) {
            drainRate += external_sprint_drain_rate(
                TWILIT_ESSENTIALS_STAMINA_SOURCE_SPRINT);
        }
        if (wolfSprintActive) {
            drainRate += external_sprint_drain_rate(
                TWILIT_ESSENTIALS_STAMINA_SOURCE_WOLF_SPRINT);
        }
        if (drainRate < 0.0f) {
            if (svc_te_stamina->deny != nullptr) {
                svc_te_stamina->deny(mod_ctx);
            }
            return;
        }
        if (drainRate > 0.0f && elapsed > 0.0f &&
            svc_te_stamina->drain(mod_ctx, drainRate * elapsed) != MOD_OK &&
            svc_te_stamina->deny != nullptr)
        {
            svc_te_stamina->deny(mod_ctx);
        }
        return;
    }
    if (provider == ExternalStaminaState::Error) {
        return;
    }
    if (!sprintStaminaEnabled()) {
        g_sprintStamina.value = sprint_stamina_maximum();
        g_sprintStamina.exhausted = false;
        return;
    }

    if ((sprintActive || wolfSprintActive) && !g_sprintStamina.exhausted) {
        g_sprintStamina.value = std::max(
            0.0f, g_sprintStamina.value -
                elapsed * static_cast<f32>(sprintStaminaDrain()));
        if (g_sprintStamina.value <= 0.0f) {
            g_sprintStamina.exhausted = true;
        }
    } else {
        g_sprintStamina.value = std::min(
            sprint_stamina_maximum(),
            g_sprintStamina.value +
                elapsed * static_cast<f32>(sprintStaminaRecovery()));
        if (g_sprintStamina.value > 0.0f) {
            g_sprintStamina.exhausted = false;
        }
    }
}

struct SprintPaneState {
    CPaneMgr* pane = nullptr;
    f32 posX = 0.0f;
    f32 posY = 0.0f;
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 width = 0.0f;
    f32 height = 0.0f;
    f32 scaleX = 1.0f;
    f32 scaleY = 1.0f;
    f32 alpha = 1.0f;
    bool visible = false;
};

SprintPaneState save_sprint_pane(CPaneMgr* pane) {
    if (pane == nullptr || pane->getPanePtr() == nullptr) {
        return {};
    }
    return {
        pane,
        pane->getPosX(),
        pane->getPosY(),
        pane->getTranslateX(),
        pane->getTranslateY(),
        pane->getSizeX(),
        pane->getSizeY(),
        pane->getScaleX(),
        pane->getScaleY(),
        pane->getAlphaRate(),
        pane->getPanePtr()->isVisible(),
    };
}

void restore_sprint_pane(const SprintPaneState& state) {
    if (state.pane == nullptr || state.pane->getPanePtr() == nullptr) {
        return;
    }
    state.pane->move(state.posX, state.posY);
    state.pane->translate(state.x, state.y);
    state.pane->resize(state.width, state.height);
    state.pane->scale(state.scaleX, state.scaleY);
    state.pane->setAlphaRate(state.alpha);
    if (state.visible) {
        state.pane->getPanePtr()->show();
    } else {
        state.pane->getPanePtr()->hide();
    }
}

void draw_sprint_stamina_meter(ModContext*, void* args, void*, void*) {
    const ExternalStaminaState provider = external_stamina_state();
    if (!sprintStaminaEnabled() ||
        provider == ExternalStaminaState::Enabled ||
        (provider == ExternalStaminaState::Error &&
         g_sprintStamina.externalWasEnabled) ||
        dMeter2Info_getWindowStatus() != 0 ||
        dMeter2Info_getPauseStatus() != 0 ||
        dComIfGp_isPauseFlag() || dComIfGp_event_runCheck() ||
        dMeter2Info_isShopTalkFlag() || dMsgObject_isTalkNowCheck())
    {
        return;
    }

    auto* meter = mods::arg<dMeter2Draw_c*>(args, 0);
    if (meter == nullptr || meter->mpKanteraScreen == nullptr ||
        meter->mpMagicParent == nullptr || meter->mpMagicBase == nullptr ||
        meter->mpMagicFrameL == nullptr || meter->mpMagicMeter == nullptr ||
        meter->mpMagicFrameR == nullptr)
    {
        return;
    }

    const SprintPaneState parent = save_sprint_pane(meter->mpMagicParent);
    const SprintPaneState base = save_sprint_pane(meter->mpMagicBase);
    const SprintPaneState left = save_sprint_pane(meter->mpMagicFrameL);
    const SprintPaneState fill = save_sprint_pane(meter->mpMagicMeter);
    const SprintPaneState right = save_sprint_pane(meter->mpMagicFrameR);
    auto* fillPicture = static_cast<J2DPicture*>(meter->mpMagicMeter->getPanePtr());
    const JUtility::TColor oldBlack = fillPicture->getBlack();
    const JUtility::TColor oldWhite = fillPicture->getWhite();

    const f32 fullWidth = meter->mpMagicMeter->getInitSizeX();
    const f32 fillRatio = std::clamp(
        g_sprintStamina.value / sprint_stamina_maximum(), 0.0f, 1.0f);
    meter->mpMagicParent->getPanePtr()->show();
    meter->mpMagicBase->getPanePtr()->show();
    meter->mpMagicFrameL->getPanePtr()->show();
    meter->mpMagicMeter->getPanePtr()->show();
    meter->mpMagicFrameR->getPanePtr()->show();
    meter->mpMagicParent->setAlphaRate(1.0f);
    meter->mpMagicBase->setAlphaRate(1.0f);
    meter->mpMagicFrameL->setAlphaRate(1.0f);
    meter->mpMagicMeter->setAlphaRate(1.0f);
    meter->mpMagicFrameR->setAlphaRate(1.0f);
    meter->mpMagicParent->translate(parent.x, parent.y + 25.0f);
    meter->mpMagicMeter->setBlackWhite(
        JUtility::TColor(100, 255, 100, 255),
        JUtility::TColor(0, 210, 0, 255));
    meter->mpMagicMeter->resize(
        fullWidth * fillRatio, meter->mpMagicMeter->getInitSizeY());

    J2DGrafContext* graf = dComIfGp_getCurrentGrafPort();
    if (graf != nullptr) {
        graf->setup2D();
        meter->mpKanteraScreen->draw(0.0f, 0.0f, graf);
        graf->setup2D();
    }

    fillPicture->setBlackWhite(oldBlack, oldWhite);
    restore_sprint_pane(right);
    restore_sprint_pane(fill);
    restore_sprint_pane(left);
    restore_sprint_pane(base);
    restore_sprint_pane(parent);
}
