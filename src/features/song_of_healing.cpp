namespace song_of_healing {
namespace {

enum class Phase {
    Idle,
    Transforming,
    Performing,
    Extending,
};

constexpr u16 kActionTimeoutFrames = 480;
constexpr u16 kHowlExtensionFrames = 60;

Phase g_phase = Phase::Idle;
daAlink_c* g_owner = nullptr;
u16 g_actionFrames = 0;
u16 g_howlExtensionFrames = 0;

void clear_state(bool stopAudio) {
    auto* player = daAlink_getAlinkActorClass();
    if (player != nullptr) {
        if (stopAudio && player->checkWolf() &&
            player->mProcID == daAlink_c::PROC_WOLF_HOWL)
        {
            player->procWolfWaitInit();
        }
        Z2WolfHowlMgr* howlMgr = player->getWolfHowlMgrP();
        if (howlMgr != nullptr) {
            if (stopAudio && howlMgr->mWolfHowlHandle) {
                howlMgr->mWolfHowlHandle->stop(30);
            }
            howlMgr->setCorrectCurve(Z2WOLFHOWL_NONE);
        }
    }

    g_phase = Phase::Idle;
    g_owner = nullptr;
    g_actionFrames = 0;
    g_howlExtensionFrames = 0;
}

bool can_transform(daAlink_c* player) {
    daMidna_c* midna = daAlink_c::getMidnaActor();
    const char* stage = dComIfGp_getStartStageName();
    return player != nullptr && !player->checkEventRun() &&
        !dComIfGp_event_runCheck() && !dComIfGp_isEnableNextStage() &&
        player->mLinkAcch.ChkGroundHit() && !player->checkSwimUp() &&
        !player->checkMagneBootsOn() &&
        player->mEquipItem != dItemNo_IRONBALL_e &&
        midna != nullptr && midna->checkMetamorphoseEnableBase() &&
        (stage == nullptr || std::strcmp(stage, "R_SP161") != 0);
}

bool start_howl(daAlink_c* player) {
    Z2WolfHowlMgr* howlMgr = player->getWolfHowlMgrP();
    if (howlMgr == nullptr) {
        return false;
    }

    howlMgr->resetState();
    howlMgr->setCorrectCurve(Z2WOLFHOWL_HEALINGSONG);
    player->procWolfHowlInit(0);
    if (player->mProcID != daAlink_c::PROC_WOLF_HOWL) {
        howlMgr->setCorrectCurve(Z2WOLFHOWL_NONE);
        return false;
    }

    g_phase = Phase::Performing;
    g_actionFrames = 0;
    g_howlExtensionFrames = 0;
    return true;
}

bool restart_howl(daAlink_c* player) {
    player->procWolfHowlInit(0);
    return player->mProcID == daAlink_c::PROC_WOLF_HOWL;
}

void maintain_howl_audio(daAlink_c* player) {
    Z2WolfHowlMgr* howlMgr = player->getWolfHowlMgrP();
    Z2SoundStarter* soundStarter = Z2GetSoundStarter();
    if (howlMgr != nullptr && soundStarter != nullptr) {
        soundStarter->startLevelSound(
            Z2SE_WL_V_HOWL, &howlMgr->mWolfHowlHandle, nullptr);
    }
}

void finish_song(daAlink_c* player) {
    clear_state(true);
    coop_song_of_healing_heal_room();
    svc_log->info(mod_ctx, "Song of Healing restored the Co-op room's health");
}

}  // namespace

void request(daAlink_c* player) {
    if (player == nullptr || g_phase != Phase::Idle) {
        return;
    }

    if (player->checkWolf()) {
        if (!start_howl(player)) {
            player->seStartSystem(Z2SE_SYS_ERROR);
        } else {
            g_owner = player;
        }
        return;
    }

    if (!can_transform(player)) {
        player->seStartSystem(Z2SE_SYS_ERROR);
        svc_log->info(mod_ctx, "Song of Healing requires a permitted Wolf Link transformation");
        return;
    }

    g_owner = player;
    g_phase = Phase::Transforming;
    g_actionFrames = 0;
    player->procCoMetamorphoseInit();
    if (player->mProcID != daAlink_c::PROC_METAMORPHOSE &&
        player->mProcID != daAlink_c::PROC_METAMORPHOSE_ONLY)
    {
        clear_state(false);
        player->seStartSystem(Z2SE_SYS_ERROR);
        svc_log->warn(mod_ctx, "Song of Healing transformation did not start");
    }
}

void update() {
    if (g_phase == Phase::Idle) {
        return;
    }

    auto* player = daAlink_getAlinkActorClass();
    if (player == nullptr || player != g_owner ||
        dComIfGp_isEnableNextStage())
    {
        clear_state(true);
        return;
    }

    if (++g_actionFrames >= kActionTimeoutFrames) {
        clear_state(true);
        player->seStartSystem(Z2SE_SYS_ERROR);
        svc_log->warn(mod_ctx, "Song of Healing action timed out");
        return;
    }

    if (g_phase == Phase::Transforming) {
        if (player->checkWolf() &&
            player->mProcID != daAlink_c::PROC_METAMORPHOSE &&
            player->mProcID != daAlink_c::PROC_METAMORPHOSE_ONLY)
        {
            if (!start_howl(player)) {
                clear_state(false);
                player->seStartSystem(Z2SE_SYS_ERROR);
                svc_log->warn(mod_ctx, "Song of Healing howl did not start");
            }
        } else if (!player->checkWolf() &&
                   player->mProcID != daAlink_c::PROC_METAMORPHOSE &&
                   player->mProcID != daAlink_c::PROC_METAMORPHOSE_ONLY)
        {
            clear_state(false);
            player->seStartSystem(Z2SE_SYS_ERROR);
            svc_log->warn(mod_ctx, "Song of Healing transformation was interrupted");
        }
        return;
    }

    if (!player->checkWolf()) {
        clear_state(true);
        return;
    }

    if (player->mProcID == daAlink_c::PROC_WOLF_HOWL) {
        maintain_howl_audio(player);
    }

    if (g_phase == Phase::Performing) {
        if (player->mProcID == daAlink_c::PROC_WOLF_HOWL) {
            return;
        }

        g_phase = Phase::Extending;
        g_howlExtensionFrames = kHowlExtensionFrames;
        if (!restart_howl(player)) {
            finish_song(player);
        }
        return;
    }

    if (g_howlExtensionFrames > 0) {
        --g_howlExtensionFrames;
        if (g_howlExtensionFrames == 0) {
            finish_song(player);
        } else if (player->mProcID != daAlink_c::PROC_WOLF_HOWL &&
                   !restart_howl(player))
        {
            finish_song(player);
        }
    }
}

void cancel() {
    if (g_phase != Phase::Idle) {
        clear_state(true);
    }
}

}  // namespace song_of_healing

void request_song_of_healing(daAlink_c* player) {
    song_of_healing::request(player);
}

void update_song_of_healing() {
    song_of_healing::update();
}

void cancel_song_of_healing() {
    song_of_healing::cancel();
}
