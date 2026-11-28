struct OcarinaNoteInput {
    u32 button;
    u8 midiNote;
};

constexpr std::array<OcarinaNoteInput, 5> kOcarinaNoteInputs = {{
    {PAD_BUTTON_A, 62},      // D4
    {PAD_BUTTON_UP, 74},     // D5
    {PAD_BUTTON_DOWN, 65},   // F4
    {PAD_BUTTON_LEFT, 71},   // B4
    {PAD_BUTTON_RIGHT, 69},  // A4
}};
constexpr std::array<u32, 6> kSongOfHealingSequence = {{
    PAD_BUTTON_LEFT,
    PAD_BUTTON_RIGHT,
    PAD_BUTTON_DOWN,
    PAD_BUTTON_LEFT,
    PAD_BUTTON_RIGHT,
    PAD_BUTTON_DOWN,
}};
constexpr u32 kOcarinaCapturedButtons =
    PAD_BUTTON_A | PAD_BUTTON_UP | PAD_BUTTON_DOWN | PAD_BUTTON_LEFT |
    PAD_BUTTON_RIGHT;
constexpr u32 kOcarinaMidiProgram = 0xF4;  // JASTrack's triangle-wave oscillator.
constexpr u32 kOcarinaMidiNoteId = 1;
constexpr u8 kOcarinaNoteVelocity = 0x7F;
constexpr u8 kOcarinaNoteDurationFrames = 10;

struct OcarinaSequencePauseState {
    JAISeq* sequence;
    u32 soundId;
    bool wasPaused;
};

JASTrack g_ocarinaMidiTrack;
JAISoundHandle g_ocarinaSuccessCueHandle;
std::vector<OcarinaSequencePauseState> g_ocarinaSequencePauseStates;
bool g_ocarinaActive = false;
bool g_ocarinaAudioErrorLogged = false;
bool g_ocarinaPresentationActive = false;
bool g_ocarinaStartAnimationComplete = false;
bool g_ocarinaCameraActive = false;
bool g_ocarinaMusicPaused = false;
bool g_ocarinaMusicPauseWarningLogged = false;
daAlink_c* g_pendingOcarinaStart = nullptr;
u8 g_pendingOcarinaButton = 0;
u8 g_ocarinaNoteFramesRemaining = 0;
u8 g_songOfHealingProgress = 0;
OSTime g_songOfHealingDeadline = 0;
bool g_songOfHealingPending = false;

Z2SoundMgr* get_game_sound_manager() {
    // The JASGlobalInstance singleton is module-local; use the game's exported audio pointer.
    auto* audioMgr = Z2GetAudioMgr();
    return audioMgr != nullptr ? &audioMgr->mSoundMgr : nullptr;
}

void remember_and_pause_ocarina_sequence(JAISeq* sequence) {
    const u32 soundId = sequence->getID();
    const auto found = std::find_if(
        g_ocarinaSequencePauseStates.begin(),
        g_ocarinaSequencePauseStates.end(),
        [sequence, soundId](const OcarinaSequencePauseState& state) {
            return state.sequence == sequence && state.soundId == soundId;
        });
    if (found == g_ocarinaSequencePauseStates.end()) {
        g_ocarinaSequencePauseStates.push_back(
            {sequence, soundId, sequence->isPaused()});
    }
    sequence->pause(true);
}

void pause_ocarina_sequences_except(JAISeq* allowedSequence) {
    auto* soundMgr = get_game_sound_manager();
    if (soundMgr == nullptr) {
        return;
    }

    auto* sequenceManager = soundMgr->getSeqMgr();
    for (auto* link = sequenceManager->getSeqList()->getFirst();
         link != nullptr;
         link = link->getNext())
    {
        auto* sequence = link->getObject();
        if (sequence == allowedSequence) {
            sequence->pause(false);
        } else {
            remember_and_pause_ocarina_sequence(sequence);
        }
    }
    sequenceManager->pause(false);
}

void restore_ocarina_sequences() {
    auto* soundMgr = get_game_sound_manager();
    if (soundMgr == nullptr) {
        g_ocarinaSequencePauseStates.clear();
        return;
    }

    auto* sequenceManager = soundMgr->getSeqMgr();
    sequenceManager->pause(false);
    for (const auto& state : g_ocarinaSequencePauseStates) {
        for (auto* link = sequenceManager->getSeqList()->getFirst();
             link != nullptr;
             link = link->getNext())
        {
            auto* sequence = link->getObject();
            if (sequence == state.sequence && sequence->getID() == state.soundId) {
                sequence->pause(state.wasPaused);
                break;
            }
        }
    }
    g_ocarinaSequencePauseStates.clear();
}

void reset_ocarina_song_progress() {
    g_songOfHealingProgress = 0;
}

void play_ocarina_song_success_cue() {
    auto* soundMgr = get_game_sound_manager();
    if (soundMgr == nullptr) {
        svc_log->warn(mod_ctx, "Ocarina could not play menu_select.bms: audio manager unavailable");
        return;
    }

    auto* sequenceManager = soundMgr->getSeqMgr();
    pause_ocarina_sequences_except(nullptr);
    if (g_ocarinaSuccessCueHandle) {
        g_ocarinaSuccessCueHandle->stop(0);
        g_ocarinaSuccessCueHandle.releaseSound();
    }

    if (!soundMgr->startSound(
            Z2BGM_MENU_SELECT, &g_ocarinaSuccessCueHandle, nullptr))
    {
        sequenceManager->pause(true);
        svc_log->warn(mod_ctx, "Ocarina could not start menu_select.bms");
        return;
    }

    pause_ocarina_sequences_except(g_ocarinaSuccessCueHandle->asSeq());
    svc_log->info(mod_ctx, "Ocarina started menu_select.bms as the song success cue");
}

void play_song_of_healing() {
    const u16 life = dComIfGs_getLife();
    const u16 maxLife = dComIfGs_getMaxLifeGauge();
    if (life < maxLife) {
        dComIfGp_setItemLifeCount(static_cast<f32>(maxLife - life), 1);
        svc_log->info(mod_ctx, "Song of Healing restored Link to maximum health");
    } else {
        svc_log->info(mod_ctx, "Song of Healing played at maximum health");
    }
}

void record_ocarina_song_note(u32 noteButtons) {
    if (g_songOfHealingPending) {
        return;
    }

    if (noteButtons == kSongOfHealingSequence[g_songOfHealingProgress]) {
        ++g_songOfHealingProgress;
        if (g_songOfHealingProgress == kSongOfHealingSequence.size()) {
            reset_ocarina_song_progress();
            play_ocarina_song_success_cue();
            g_songOfHealingDeadline = OSGetTime() + OSSecondsToTicks(1);
            g_songOfHealingPending = true;
        }
        return;
    }

    g_songOfHealingProgress =
        noteButtons == kSongOfHealingSequence[0] ? 1 : 0;
}

void initialize_ocarina() {
    if (g_ocarinaSuccessCueHandle) {
        g_ocarinaSuccessCueHandle->stop(0);
        g_ocarinaSuccessCueHandle.releaseSound();
    }
    g_ocarinaSequencePauseStates.clear();
    g_ocarinaMidiTrack.setBankNumber(0);
    g_ocarinaMidiTrack.setProgNumber(kOcarinaMidiProgram);
    g_ocarinaActive = false;
    g_ocarinaAudioErrorLogged = false;
    g_ocarinaPresentationActive = false;
    g_ocarinaStartAnimationComplete = false;
    g_ocarinaCameraActive = false;
    g_ocarinaMusicPaused = false;
    g_ocarinaMusicPauseWarningLogged = false;
    g_pendingOcarinaStart = nullptr;
    g_pendingOcarinaButton = 0;
    g_ocarinaNoteFramesRemaining = 0;
    g_songOfHealingDeadline = 0;
    g_songOfHealingPending = false;
    reset_ocarina_song_progress();
}

void pause_ocarina_music() {
    auto* soundMgr = get_game_sound_manager();
    if (soundMgr == nullptr) {
        if (!g_ocarinaMusicPauseWarningLogged) {
            svc_log->warn(mod_ctx, "Ocarina could not pause game music: audio manager unavailable");
            g_ocarinaMusicPauseWarningLogged = true;
        }
        return;
    }

    const bool cuePlaying =
        g_ocarinaSuccessCueHandle &&
        !g_ocarinaSuccessCueHandle->status_.isDead();
    const bool wasPaused = g_ocarinaMusicPaused;
    if (cuePlaying) {
        pause_ocarina_sequences_except(g_ocarinaSuccessCueHandle->asSeq());
    } else {
        soundMgr->getSeqMgr()->pause(true);
    }
    soundMgr->getStreamMgr()->pause(true);
    g_ocarinaMusicPaused = true;
    g_ocarinaMusicPauseWarningLogged = false;
    if (!wasPaused) {
        svc_log->info(mod_ctx, "Ocarina paused sequence and stream music managers");
    }
}

void maintain_ocarina_music_pause() {
    pause_ocarina_music();
}

void resume_ocarina_music() {
    if (!g_ocarinaMusicPaused) {
        return;
    }

    auto* soundMgr = get_game_sound_manager();
    if (soundMgr != nullptr) {
        if (g_ocarinaSuccessCueHandle) {
            g_ocarinaSuccessCueHandle->stop(0);
            g_ocarinaSuccessCueHandle.releaseSound();
        }
        restore_ocarina_sequences();
        soundMgr->getStreamMgr()->pause(false);
        svc_log->info(mod_ctx, "Ocarina resumed sequence and stream music managers");
    } else {
        svc_log->warn(mod_ctx, "Ocarina could not resume game music: audio manager unavailable");
    }
    g_ocarinaMusicPaused = false;
}

void stop_ocarina_note() {
    if (g_ocarinaNoteFramesRemaining != 0) {
        g_ocarinaMidiTrack.noteOff(kOcarinaMidiNoteId, 0);
        g_ocarinaNoteFramesRemaining = 0;
    }
}

void stop_ocarina_mode() {
    g_pendingOcarinaStart = nullptr;
    g_pendingOcarinaButton = 0;
    reset_ocarina_song_progress();
    if (!g_ocarinaActive) {
        stop_ocarina_note();
        resume_ocarina_music();
        return;
    }

    stop_ocarina_note();
    g_ocarinaActive = false;
    resume_ocarina_music();
    svc_log->info(mod_ctx, "Ocarina mode ended");
}

void exit_ocarina_mode(daAlink_c* alink) {
    stop_ocarina_mode();
    if (!g_ocarinaPresentationActive || alink == nullptr) {
        return;
    }

    g_ocarinaPresentationActive = false;
    g_ocarinaStartAnimationComplete = false;
    if (g_ocarinaCameraActive) {
        if (auto* camera = dCam_getBody()) {
            camera->EndEventCamera(fopAcM_GetID(alink));
        }
        g_ocarinaCameraActive = false;
    }
    if (alink->mProcID == daAlink_c::PROC_GRASS_WHISTLE_WAIT) {
        alink->resetUpperAnime(daAlink_c::UPPER_2, -1.0f);
        alink->resetFacePriAnime();
        alink->procWaitInit();
    }
}

bool activate_ocarina_whistle(daAlink_c* alink, u8 button) {
    if (alink == nullptr || alink->checkWolf() || alink->checkEventRun() ||
        (alink->mProcID != daAlink_c::PROC_WAIT &&
         alink->mProcID != daAlink_c::PROC_MOVE))
    {
        return false;
    }

    alink->commonProcInit(daAlink_c::PROC_GRASS_WHISTLE_WAIT);
    alink->keepItemData();
    alink->setHorseWhistleModel();
    alink->setSingleAnimeBase(daAlink_c::ANM_GRASS_WHISTLE_START);
    alink->setUpperAnimeBaseMorf(
        alink->getMainBckData(daAlink_c::ANM_GRASS_WHISTLE_START)->m_underID,
        -1.0f);
    alink->setFacePriAnime(daAlink_c::ANM_GRASS_WHISTLE_START);
    if (auto* camera = dCam_getBody()) {
        g_ocarinaCameraActive =
            camera->StartEventCamera(12, fopAcM_GetID(alink), nullptr) != 0;
    }
    alink->mProcVar0.field_0x3008 = 0;
    alink->mProcVar2.field_0x300c = 3;
    alink->mProcVar3.field_0x300e = -1;
    alink->mProcVar4.field_0x3010 = 0;
    alink->mProcVar5.field_0x3012 = 0;
    alink->mMsgClassID = fpcM_ERROR_PROCESS_ID_e;
    alink->field_0x3198 = -1;
    alink->field_0x32cc = 0x532;
    alink->mNormalSpeed = 0.0f;
    alink->speedF = 0.0f;
    alink->current.angle.y = alink->shape_angle.y;
    alink->field_0x2f98 = 4;
    dComIfGp_setPlayerStatus1(0, 0x4000000);
    alink->mSelectItemId = button;
    g_ocarinaPresentationActive = true;
    g_ocarinaStartAnimationComplete = false;
    g_ocarinaActive = true;
    pause_ocarina_music();
    svc_log->info(mod_ctx, "Ocarina ready: A and D-pad play notes; B exits");
    return true;
}

bool start_ocarina_whistle(daAlink_c* alink, u8 button) {
    if (alink == nullptr || alink->checkWolf() || alink->checkEventRun() ||
        (alink->mProcID != daAlink_c::PROC_WAIT &&
         alink->mProcID != daAlink_c::PROC_MOVE))
    {
        return false;
    }

    g_pendingOcarinaStart = alink;
    g_pendingOcarinaButton = button;
    svc_log->info(mod_ctx, "Ocarina activation queued");
    return true;
}

void activate_pending_ocarina(daAlink_c* alink) {
    if (alink == nullptr || alink != g_pendingOcarinaStart) {
        return;
    }

    if (alink->checkWolf() || alink->checkEventRun()) {
        g_pendingOcarinaStart = nullptr;
        g_pendingOcarinaButton = 0;
        svc_log->warn(mod_ctx, "Ocarina activation was cancelled by a player event");
        return;
    }

    if (alink->mProcID != daAlink_c::PROC_WAIT &&
        alink->mProcID != daAlink_c::PROC_MOVE)
    {
        return;
    }

    const u8 button = g_pendingOcarinaButton;
    g_pendingOcarinaStart = nullptr;
    g_pendingOcarinaButton = 0;
    if (!activate_ocarina_whistle(alink, button)) {
        svc_log->warn(mod_ctx, "Ocarina activation was cancelled before player update ended");
    }
}

void activate_pending_ocarina_after_player_execute(
    ModContext*, void* args, void*, void*)
{
    activate_pending_ocarina(mods::arg<daAlink_c*>(args, 0));
}

void activate_pending_ocarina_before_player_input(ModContext*, void*, void*, void*) {
    activate_pending_ocarina(
        static_cast<daAlink_c*>(dComIfGp_getLinkPlayer()));
}

void play_ocarina_note(u8 midiNote) {
    stop_ocarina_note();
    if (g_ocarinaMidiTrack.noteOn(
            kOcarinaMidiNoteId, midiNote, kOcarinaNoteVelocity) == 0)
    {
        if (!g_ocarinaAudioErrorLogged) {
            svc_log->warn(mod_ctx, "JAS could not start an Ocarina test note");
            g_ocarinaAudioErrorLogged = true;
        }
        return;
    }

    g_ocarinaAudioErrorLogged = false;
    g_ocarinaNoteFramesRemaining = kOcarinaNoteDurationFrames;
    svc_log->info(mod_ctx, "Ocarina oscillator note started");
}

void handle_ocarina_input(ModContext*, void*, void*, void*) {
    if (!g_ocarinaActive) {
        return;
    }

    auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    if (alink == nullptr || alink->checkWolf() ||
        (alink->checkEventRun() && !g_ocarinaCameraActive))
    {
        exit_ocarina_mode(alink);
        return;
    }

    alink->setDoStatus(BUTTON_STATUS_BLOW);
    alink->setGrassCancelStatus(BUTTON_STATUS_BACK);
    auto& pad = mDoCPd_c::getCpadInfo(PAD_1);
    auto* gamePad = mDoCPd_c::getGamePad(PAD_1);
    const u32 triggered = pad.mPressedButtonFlags |
        (gamePad != nullptr ? gamePad->mButton.mTrigger & PAD_BUTTON_LEFT : 0);
    if ((triggered & PAD_BUTTON_B) != 0) {
        pad.mPressedButtonFlags &= ~PAD_BUTTON_B;
        pad.mButtonFlags &= ~PAD_BUTTON_B;
        alink->mItemTrigger &= static_cast<u8>(~daAlink_c::BTN_B);
        alink->mItemButton &= static_cast<u8>(~daAlink_c::BTN_B);
        exit_ocarina_mode(alink);
    } else {
        const u32 noteButtons = triggered & kOcarinaCapturedButtons;
        if (noteButtons != 0 && (noteButtons & (noteButtons - 1)) == 0) {
            for (const auto& note : kOcarinaNoteInputs) {
                if ((noteButtons & note.button) != 0) {
                    record_ocarina_song_note(noteButtons);
                    play_ocarina_note(note.midiNote);
                    break;
                }
            }
        } else if (noteButtons != 0) {
            reset_ocarina_song_progress();
        }
    }

    constexpr u32 kPauseButton = PAD_BUTTON_START;
    pad.mPressedButtonFlags &= kPauseButton;
    pad.mButtonFlags &= kPauseButton;
    pad.mMainStickPosX = 0.0f;
    pad.mMainStickPosY = 0.0f;
    pad.mMainStickValue = 0.0f;
    pad.mMainStickAngle = 0;
    pad.mCStickPosX = 0.0f;
    pad.mCStickPosY = 0.0f;
    pad.mCStickValue = 0.0f;
    pad.mCStickAngle = 0;
    pad.mTriggerLeft = 0.0f;
    pad.mTriggerRight = 0.0f;
    pad.mAnalogA = 0.0f;
    pad.mAnalogB = 0.0f;
    if (gamePad != nullptr) {
        gamePad->mButton.mButton &= ~PAD_BUTTON_LEFT;
        gamePad->mButton.mTrigger &= ~PAD_BUTTON_LEFT;
        gamePad->mButton.mRelease &= ~PAD_BUTTON_LEFT;
        gamePad->mButton.mRepeat &= ~PAD_BUTTON_LEFT;
    }
    JUTGamePad::mPadStatus[PAD_1].button &= ~PAD_BUTTON_LEFT;
}

HookAction update_ocarina_whistle_wait(ModContext*, void* args, void* retval, void*) {
    if (!g_ocarinaActive) {
        return HOOK_CONTINUE;
    }

    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink == nullptr) {
        return HOOK_CONTINUE;
    }

    alink->setDoStatus(BUTTON_STATUS_BLOW);
    alink->setGrassCancelStatus(BUTTON_STATUS_BACK);
    if (!g_ocarinaStartAnimationComplete &&
        alink->checkAnmEnd(&alink->mUpperFrameCtrl[2]))
    {
        alink->setSingleAnimeBaseMorf(daAlink_c::ANM_GRASS_WHISTLE_PLAY, 0.0f);
        alink->resetUpperAnime(daAlink_c::UPPER_2, -1.0f);
        g_ocarinaStartAnimationComplete = true;
    }

    if (retval != nullptr) {
        *static_cast<int*>(retval) = 1;
    }
    return HOOK_SKIP_ORIGINAL;
}

void update_ocarina() {
    if (g_ocarinaActive) {
        maintain_ocarina_music_pause();
    }

    if (g_ocarinaActive) {
        auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
        if (alink == nullptr || alink->checkWolf() ||
            (alink->checkEventRun() && !g_ocarinaCameraActive))
        {
            exit_ocarina_mode(alink);
        } else if (alink->mProcID != daAlink_c::PROC_GRASS_WHISTLE_WAIT) {
            exit_ocarina_mode(alink);
        }
    }

    if (g_ocarinaNoteFramesRemaining > 0 &&
        --g_ocarinaNoteFramesRemaining == 0)
    {
        g_ocarinaMidiTrack.noteOff(kOcarinaMidiNoteId, 0);
    }

    if (g_songOfHealingPending && OSGetTime() >= g_songOfHealingDeadline) {
        g_songOfHealingPending = false;
        g_songOfHealingDeadline = 0;
        play_song_of_healing();
    }
}

HookAction suppress_ocarina_damage(ModContext*, void* args, void* retval, void*) {
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (!g_ocarinaActive || alink == nullptr || retval == nullptr) {
        return HOOK_CONTINUE;
    }

    *static_cast<int*>(retval) = 0;
    return HOOK_SKIP_ORIGINAL;
}

DEFINE_HOOK(&mDoCPd_c::read, CpadRead);
DEFINE_HOOK(&daAlink_c::setStickData, SetStickDataOcarina);
DEFINE_HOOK(&daAlink_c::procGrassWhistleWait, OcarinaGrassWhistleWait);
DEFINE_HOOK(&daAlink_c::checkDamageAction, OcarinaCheckDamageAction);

void cancel_ocarina_after_scene_change(ModContext*, void*, void* retval, void*) {
    if (retval == nullptr || *static_cast<int*>(retval) == 0) {
        return;
    }

    exit_ocarina_mode(static_cast<daAlink_c*>(dComIfGp_getLinkPlayer()));
}
