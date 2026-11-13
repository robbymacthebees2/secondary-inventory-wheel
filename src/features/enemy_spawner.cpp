namespace enemy_spawner {
namespace {

constexpr std::array<ProfileName, 43> kProfiles = {
    fpcNm_B_TN_e, fpcNm_E_OC_e, fpcNm_E_FZ_e,
    fpcNm_E_BA_e, fpcNm_E_BA_e, fpcNm_E_BA_e,
    fpcNm_E_TT_e, fpcNm_E_GI_e, fpcNm_NPC_GRA_e,
    fpcNm_E_ZS_e, fpcNm_B_GG_e, fpcNm_E_KK_e,
    fpcNm_E_FB_e, fpcNm_E_BS_e, fpcNm_E_BU_e,
    fpcNm_E_BU_e, fpcNm_E_BU_e, fpcNm_E_MS_e,
    fpcNm_E_WW_e, fpcNm_E_FS_e, fpcNm_E_CR_e,
    fpcNm_E_SH_e, fpcNm_E_TK_e, fpcNm_E_TK2_e,
    fpcNm_E_RD_e, fpcNm_E_DN_e, fpcNm_E_DD_e,
    fpcNm_E_MF_e, fpcNm_E_ST_e, fpcNm_E_HB_e,
    fpcNm_E_GB_e, fpcNm_E_DB_e, fpcNm_E_SF_e,
    fpcNm_E_MM_e, fpcNm_E_MM_e, fpcNm_E_KR_e,
    fpcNm_E_GE_e, fpcNm_E_AI_e, fpcNm_E_SM2_e,
    fpcNm_E_SM2_e, fpcNm_E_SM2_e, fpcNm_E_SM2_e,
    fpcNm_E_SM2_e,
};

constexpr std::array<u32, kProfiles.size()> kParameters = {
    0x000001ff, 0x00ff0000, 0,
    0xffff0f01, 0xffff1f01, 0xffff2f01,
    0x000000ff, 0x00ff01ff, 0,
    0, 0xffff0002, 0x00ff0000,
    0x00ffff00, 0x00ffff00, 0xffff0f00,
    0xffff1f00, 0xffff2f00, 0xffff0000,
    0x03000000, 0, 0,
    0x00ffff00, 0x00ff0000, 0,
    0xff000100, 0xff000000, 0xffff0000,
    0xff000000, 0xff000002, 0xff000000,
    0x00ffff00, 0xff000000, 0x0000ff00,
    0x0000ff00, 0x0000ff00, 0xffffff00,
    0x00ffff01, 0x00ff000a, 0xffff0010,
    0xffff0020, 0xffff0030, 0xffff0040,
    0xffff0060,
};

constexpr std::array<const char*, kProfiles.size()> kLabels = {
    "Darknut", "Bokoblin", "Mini Freezard",
    "Keese", "Fire Keese", "Ice Keese",
    "Tektite", "Gibdo", "Goron",
    "Staltroop", "Aeralfos", "Chilfos",
    "Freezard", "Stalchild", "Bubble",
    "Fire Bubble", "Ice Bubble", "Rat",
    "White Wolfos", "Puppet", "Bomskit",
    "Stalhound", "Water Toadpoli", "Fire Toadpoli",
    "Bulblin", "Lizalfos", "Dodongo",
    "Dynalfos", "Skulltula", "Baba Serpent",
    "Big Baba", "Deku Baba", "Stalfos",
    "Helmasaur", "Helmasaurus", "Kargarok",
    "Guay", "Armos", "Red Chu",
    "Blue Chu", "Yellow Chu", "Purple Chu",
    "Black Chu",
};

constexpr std::size_t kHelmasaurusIndex = 34;
constexpr std::size_t kDefaultProfileIndex = 1;
constexpr f32 kSpawnDistance = 300.0f;
constexpr f32 kFlyingSpawnHeight = 200.0f;

DEFINE_HOOK(&fopAcM_createChild, EnemySpawnerChild);
DEFINE_HOOK(&daE_ZS_c::executeAppear, EnemySpawnerStaltroopAppear);
DEFINE_HOOK(&daE_ZS_c::executeWait, EnemySpawnerStaltroopWait);

std::unordered_set<ActorId> s_testActors;
std::unordered_map<ActorId, u32> s_missingActorUpdates;
bool s_hooksInstalled = false;
bool s_configReadFailureReported = false;

int64_t selected_profile() {
    int64_t value = static_cast<int64_t>(kDefaultProfileIndex);
    if (g_enemySpawnerProfile == 0 ||
        svc_config->get_int(mod_ctx, g_enemySpawnerProfile, &value) != MOD_OK)
    {
        if (!s_configReadFailureReported) {
            svc_log->warn(mod_ctx, "could not read enemy spawner profile setting");
            s_configReadFailureReported = true;
        }
        return static_cast<int64_t>(kDefaultProfileIndex);
    }

    s_configReadFailureReported = false;
    return std::clamp<int64_t>(value, 0, static_cast<int64_t>(kProfiles.size() - 1));
}

bool test_actor(ActorId id) {
    return s_testActors.contains(id);
}

void track_test_actor(ActorId id) {
    s_testActors.insert(id);
    s_missingActorUpdates[id] = 0;
}

void after_create_child(ModContext*, void* args, void* retval, void*) {
    if (args == nullptr || retval == nullptr) return;
    const ActorId parent = mods::arg<ActorId>(args, 1);
    const ActorId child = *static_cast<ActorId*>(retval);
    if (child != fpcM_ERROR_PROCESS_ID_e && test_actor(parent)) {
        track_test_actor(child);
    }
}

HookAction before_staltroop_appear(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<daE_ZS_c*>(args, 0);
    if (actor != nullptr && test_actor(fopAcM_GetID(actor)) && actor->mMode == 0) {
        actor->mMode = 1;
    }
    return HOOK_CONTINUE;
}

HookAction before_staltroop_wait(ModContext*, void* args, void*, void*) {
    auto* actor = mods::arg<daE_ZS_c*>(args, 0);
    if (actor == nullptr || !test_actor(fopAcM_GetID(actor))) {
        return HOOK_CONTINUE;
    }
    if (actor->mMode == 0) {
        actor->setBck(7, 2, 3.0f, 1.0f);
        actor->mMode = 1;
    }
    cLib_addCalcAngleS2(
        &actor->current.angle.y, fopAcM_searchPlayerAngleY(actor), 8, 0x1000);
    actor->shape_angle.y = actor->current.angle.y;
    return HOOK_SKIP_ORIGINAL;
}

ModResult install_test_hooks() {
    if (s_hooksInstalled) return MOD_OK;
    ModResult result =
        mods::hook::add_post<EnemySpawnerChild>(svc_hook, after_create_child);
    if (result != MOD_OK) return result;
    result = mods::hook::add_pre<EnemySpawnerStaltroopAppear>(
        svc_hook, before_staltroop_appear);
    if (result != MOD_OK) {
        mods::hook::uninstall<EnemySpawnerChild>(svc_hook);
        return result;
    }
    result = mods::hook::add_pre<EnemySpawnerStaltroopWait>(
        svc_hook, before_staltroop_wait);
    if (result != MOD_OK) {
        mods::hook::uninstall<EnemySpawnerStaltroopAppear>(svc_hook);
        mods::hook::uninstall<EnemySpawnerChild>(svc_hook);
        return result;
    }
    s_hooksInstalled = true;
    return MOD_OK;
}

bool blocked_by_game_state() {
    if (coop_net_connected() || dComIfGp_isEnableNextStage() ||
        dComIfGp_event_runCheck() || dComIfGp_isPauseFlag())
    {
        return true;
    }

    const char* stage = dComIfGp_getStartStageName();
    return stage != nullptr && std::strcmp(stage, "D_MN09C") == 0 &&
        dComIfGp_getStartStageRoomNo() == 0;
}

bool spawn_disabled(ModContext*, void*) {
    daAlink_c* link = daAlink_getAlinkActorClass();
    return blocked_by_game_state() || link == nullptr ||
        s_manualJumpActorService == nullptr ||
        s_manualJumpActorService->create_actor == nullptr;
}

void spawn_selected(ModContext*, void*) {
    if (spawn_disabled(nullptr, nullptr)) {
        svc_log->warn(mod_ctx,
            "enemy spawner is unavailable during co-op, scene transitions, or events");
        return;
    }

    const std::size_t index = static_cast<std::size_t>(selected_profile());
    daAlink_c* link = daAlink_getAlinkActorClass();
    auto* layer = link->layer_tag.layer;
    if (layer == nullptr || layer == fpcLy_RootLayer() ||
        fpcLy_IsDeletingMesg(layer))
    {
        svc_log->warn(mod_ctx, "enemy spawner could not find Link's active scene layer");
        return;
    }

    const s16 facing = link->shape_angle.y;
    cXyz position{
        link->current.pos.x + cM_ssin(facing) * kSpawnDistance,
        link->current.pos.y,
        link->current.pos.z + cM_scos(facing) * kSpawnDistance,
    };
    cXyz probe = position;
    probe.y += 500.0f;
    dBgS_ObjGndChk ground;
    ground.SetPos(&probe);
    const f32 groundY = dComIfG_Bgsp().GroundCross(&ground);
    if (!std::isfinite(groundY) || groundY == -G_CM3D_F_INF) {
        svc_log->warn(mod_ctx, "enemy spawner could not find a floor ahead of Link");
        return;
    }

    position.y = groundY;
    const ProfileName profile = kProfiles[index];
    if (profile == fpcNm_E_BA_e || profile == fpcNm_E_BU_e ||
        profile == fpcNm_B_GG_e || profile == fpcNm_E_KR_e ||
        profile == fpcNm_E_GE_e)
    {
        position.y += kFlyingSpawnHeight;
    }
    const s16 angleZ =
        (profile == fpcNm_E_SF_e || profile == fpcNm_E_KR_e) ? 0xff : 0;
    const csXyz angle{0, static_cast<s16>(facing + 0x8000), angleZ};
    const ActorSpawnParams params{
        .parameters = kParameters[index],
        .argument = static_cast<s8>(index == kHelmasaurusIndex ? 1 : -1),
        .room_num = fopAcM_GetRoomNo(link),
        .position = {position.x, position.y, position.z},
        .angle = {angle.x, angle.y, angle.z},
        .scale = {1.0f, 1.0f, 1.0f},
        .create_function = nullptr,
    };

    const ModResult hookResult = install_test_hooks();
    if (hookResult != MOD_OK) {
        svc_log->error(mod_ctx, "failed to install enemy spawner lifecycle hooks");
        return;
    }

    struct LayerRestore {
        layer_class* previous;
        ~LayerRestore() { fpcLy_SetCurrentLayer(previous); }
    } restore{fpcLy_CurrentLayer()};
    fpcLy_SetCurrentLayer(layer);
    ActorId actorId = fpcM_ERROR_PROCESS_ID_e;
    const ModResult result = s_manualJumpActorService->create_actor(
        mod_ctx, profile, &params, &actorId);
    if (result != MOD_OK) {
        svc_log->error(mod_ctx, "failed to spawn the selected enemy");
        return;
    }
    track_test_actor(actorId);
    svc_log->info(mod_ctx, "spawned selected enemy");
}

}

ModResult add_settings(UiElementHandle panel) {
    UiControlDesc control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_DROPDOWN;
    control.label = "Enemy Spawner Profile";
    control.help_rml =
        "Choose an enemy profile to spawn 300 units ahead of Link. Spawning is disabled "
        "during Hyrule Online co-op, events, and scene transitions.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_enemySpawnerProfile;
    control.options = kLabels.data();
    control.option_count = kLabels.size();
    ModResult result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) return result;

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_BUTTON;
    control.label = "Spawn Selected Enemy";
    control.help_rml =
        "Spawn the selected enemy in Link's current scene. Test actors are removed with the "
        "scene; spawning is blocked while connected to co-op.";
    control.on_pressed = spawn_selected;
    control.is_disabled = spawn_disabled;
    return svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
}

void update() {
    for (auto it = s_missingActorUpdates.begin(); it != s_missingActorUpdates.end();) {
        if (fopAcM_SearchByID(it->first) == nullptr) {
            if (++it->second >= 60) {
                s_testActors.erase(it->first);
                it = s_missingActorUpdates.erase(it);
            } else {
                ++it;
            }
        } else {
            it->second = 0;
            ++it;
        }
    }
}

void shutdown() {
    s_testActors.clear();
    s_missingActorUpdates.clear();
    if (s_hooksInstalled) {
        mods::hook::uninstall<EnemySpawnerStaltroopWait>(svc_hook);
        mods::hook::uninstall<EnemySpawnerStaltroopAppear>(svc_hook);
        mods::hook::uninstall<EnemySpawnerChild>(svc_hook);
    }
    s_hooksInstalled = false;
    s_configReadFailureReported = false;
}

}
