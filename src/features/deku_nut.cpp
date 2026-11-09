constexpr f32 kDekuNutContactRadius = 110.0f;
constexpr f32 kDekuNutStunRadius = 350.0f;
constexpr u16 kDekuNutStunFrames = 5 * 30;
constexpr size_t kDekuNutTrackedBombCount = 64;
constexpr s32 kDekuNutNativeModelResource = 0x20;
constexpr u32 kDekuNutActorStateFlag = 0x80000000u;
constexpr f32 kDekuNutModelScale = 2.0f;
constexpr f32 kDekuNutHandOffsetY = 8.0f;

std::array<fpc_ProcID, kDekuNutTrackedBombCount> g_dekuNutBombIds = [] {
    std::array<fpc_ProcID, kDekuNutTrackedBombCount> ids{};
    ids.fill(fpcM_ERROR_PROCESS_ID_e);
    return ids;
}();

JKRExpHeap* g_dekuNutModelHeap = nullptr;
std::array<J3DModel*, kDekuNutTrackedBombCount> g_dekuNutModels{};
bool g_dekuNutModelLoadFailed = false;

bool is_deku_nut_bomb(fopAc_ac_c* actor) {
    if (actor == nullptr || fopAcM_GetName(actor) != fpcNm_NBOMB_e) {
        return false;
    }

    return std::find(
               g_dekuNutBombIds.begin(), g_dekuNutBombIds.end(),
               fopAcM_GetID(actor)) != g_dekuNutBombIds.end();
}

size_t deku_nut_bomb_slot(fpc_ProcID actorId) {
    const auto found = std::find(g_dekuNutBombIds.begin(), g_dekuNutBombIds.end(), actorId);
    if (found == g_dekuNutBombIds.end()) {
        return kDekuNutTrackedBombCount;
    }
    return static_cast<size_t>(found - g_dekuNutBombIds.begin());
}

void forget_deku_nut_bomb(fpc_ProcID actorId) {
    const size_t slot = deku_nut_bomb_slot(actorId);
    if (slot < kDekuNutTrackedBombCount) {
        g_dekuNutBombIds[slot] = fpcM_ERROR_PROCESS_ID_e;
    }
}

bool track_deku_nut_bomb(fopAc_ac_c* actor) {
    const fpc_ProcID actorId = fopAcM_GetID(actor);
    for (size_t slot = 0; slot < g_dekuNutBombIds.size(); ++slot) {
        if (g_dekuNutBombIds[slot] == fpcM_ERROR_PROCESS_ID_e) {
            g_dekuNutBombIds[slot] = actorId;
            return true;
        }
    }
    return false;
}

bool load_deku_nut_model() {
    if (g_dekuNutModelHeap != nullptr) {
        return true;
    }
    if (g_dekuNutModelLoadFailed) {
        return false;
    }

    auto* modelData = static_cast<J3DModelData*>(
        dComIfG_getObjectRes("Alink", kDekuNutNativeModelResource));
    if (modelData == nullptr) {
        svc_log->error(mod_ctx, "Failed to get the native slingshot nut model");
        g_dekuNutModelLoadFailed = true;
        return false;
    }

    g_dekuNutModelHeap = JKRExpHeap::create(0x100000, mDoExt_getGameHeap(), true);
    if (g_dekuNutModelHeap == nullptr) {
        svc_log->error(mod_ctx, "Failed to create a heap for the slingshot nut model");
        g_dekuNutModelLoadFailed = true;
        return false;
    }
    return true;
}

J3DModel* deku_nut_model_for_bomb(daNbomb_c* bomb) {
    if (bomb == nullptr || !load_deku_nut_model()) {
        return nullptr;
    }
    const size_t slot = deku_nut_bomb_slot(fopAcM_GetID(bomb));
    if (slot >= kDekuNutTrackedBombCount) {
        return nullptr;
    }
    if (g_dekuNutModels[slot] != nullptr) {
        return g_dekuNutModels[slot];
    }

    auto* modelData = static_cast<J3DModelData*>(
        dComIfG_getObjectRes("Alink", kDekuNutNativeModelResource));
    if (modelData == nullptr) {
        svc_log->error(mod_ctx, "Failed to get the native slingshot nut model");
        return nullptr;
    }
    JKRHeap* previousHeap = mDoExt_setCurrentHeap(g_dekuNutModelHeap);
    J3DModel* model = mDoExt_J3DModel__create(modelData, 0x80000, 0x11000084);
    mDoExt_setCurrentHeap(previousHeap);
    if (model == nullptr) {
        svc_log->error(mod_ctx, "Failed to create a model from the native slingshot nut");
        return nullptr;
    }

    g_dekuNutModels[slot] = model;
    svc_log->info(mod_ctx, "Created an isolated slingshot nut model instance");
    return model;
}

void stun_deku_nut_enemies(const cXyz& center) {
    struct StunArea {
        cXyz center;
    } area{center};

    const auto stunEnemy = [](void* actorPtr, void* contextPtr) -> int {
        auto* actor = static_cast<fopAc_ac_c*>(actorPtr);
        auto* area = static_cast<StunArea*>(contextPtr);
        if (actor == nullptr || fopAcM_GetGroup(actor) != fopAc_ENEMY_e ||
            is_ice_arrow_enemy_dead(actor))
        {
            return 0;
        }

        const f32 dx = actor->current.pos.x - area->center.x;
        const f32 dy = actor->current.pos.y - area->center.y;
        const f32 dz = actor->current.pos.z - area->center.z;
        if (dx * dx + dy * dy + dz * dz <= kDekuNutStunRadius * kDekuNutStunRadius) {
            register_ice_arrow_target(actor, kDekuNutStunFrames, false);
        }
        return 0;
    };

    fopAcIt_Executor(stunEnemy, &area);
}

void pop_deku_nut(daNbomb_c* bomb) {
    if (!is_deku_nut_bomb(bomb)) {
        return;
    }

    const fpc_ProcID actorId = fopAcM_GetID(bomb);
    const cXyz impactPosition = bomb->current.pos;
    const csXyz impactAngle(0, bomb->shape_angle.y, 0);
    bomb->mStateFlg0 &= ~kDekuNutActorStateFlag;
    forget_deku_nut_bomb(actorId);
    stun_deku_nut_enemies(impactPosition);
    dComIfGp_setHitMark(3, nullptr, &impactPosition, &impactAngle, nullptr, 0);
    Z2GetAudioMgr()->seStart(
        Z2SE_OBJ_BOMB_EXPLODE, &impactPosition, 0, 0, 0.6f, 1.4f, -1.0f, -1.0f, 0);
    fopAcM_delete(bomb);
}

bool enemy_touched_deku_nut(const cXyz& position) {
    struct ContactArea {
        const cXyz& position;
        bool hit;
    } area{position, false};

    const auto checkEnemy = [](void* actorPtr, void* contextPtr) -> int {
        auto* actor = static_cast<fopAc_ac_c*>(actorPtr);
        auto* area = static_cast<ContactArea*>(contextPtr);
        if (actor == nullptr || fopAcM_GetGroup(actor) != fopAc_ENEMY_e ||
            is_ice_arrow_enemy_dead(actor))
        {
            return 0;
        }

        const f32 dx = actor->current.pos.x - area->position.x;
        const f32 dy = actor->current.pos.y - area->position.y;
        const f32 dz = actor->current.pos.z - area->position.z;
        if (dx * dx + dy * dy + dz * dz <=
            kDekuNutContactRadius * kDekuNutContactRadius)
        {
            area->hit = true;
            return 1;
        }
        return 0;
    };

    fopAcIt_Executor(checkEnemy, &area);
    return area.hit;
}

void begin_deku_nut_use(daAlink_c* alink) {
    if (alink == nullptr || alink->checkWolf() || alink->checkEventRun()) {
        return;
    }

    fopAc_ac_c* heldActor = alink->mGrabItemAcKeep.getActor();
    if (is_deku_nut_bomb(heldActor)) {
        auto* heldBomb = static_cast<daNbomb_c*>(heldActor);
        if (g_manualCuccoGlideActive) {
            heldBomb->speedF = 0.0f;
            heldBomb->speed.y = 0.0f;
            alink->freeGrabItem();
            return;
        }

        if (!alink->procGrabThrowInit(0)) {
            svc_log->warn(mod_ctx, "Could not start the Deku Nut throw animation");
        }
        return;
    }

    if (heldActor != nullptr && !g_manualCuccoGlideActive) {
        svc_log->warn(mod_ctx, "Put away the held item before using a Deku Nut");
        return;
    }

    fopAc_ac_c* actor = dBomb_c::createNormalBombPlayer(&alink->current.pos);
    if (actor == nullptr || fopAcM_GetName(actor) != fpcNm_NBOMB_e) {
        svc_log->error(mod_ctx, "Failed to create a native bomb actor for Deku Nut");
        return;
    }

    ++alink->mActiveBombNum;
    if (!track_deku_nut_bomb(actor)) {
        svc_log->error(mod_ctx, "Deku Nut bomb tracking capacity exhausted");
        fopAcM_delete(actor);
        return;
    }

    if (g_manualCuccoGlideActive) {
        actor->speedF = 0.0f;
        actor->speed.y = 0.0f;
        return;
    }

    alink->setGrabItemActor(actor);
    alink->setGrabUpperAnime(1.0f);

    svc_log->info(mod_ctx, "Deku Nut ready; use the normal bomb throw controls");
}

HookAction handle_deku_nut_bomb_execute(ModContext*, void* args, void* retval, void*) {
    auto* bomb = mods::arg<daNbomb_c*>(args, 0);
    if (!is_deku_nut_bomb(bomb)) {
        return HOOK_CONTINUE;
    }

    bomb->mStateFlg0 |= kDekuNutActorStateFlag;
    bomb->mExTime = 0;
    if (fopAcM_checkCarryNow(bomb)) {
        return HOOK_CONTINUE;
    }

    const bool hitSurface = bomb->mAcch.ChkGroundHit() ||
                            bomb->mAcch.ChkWallHit() ||
                            bomb->mAcch.ChkRoofHit();
    if (!hitSurface && !enemy_touched_deku_nut(bomb->current.pos)) {
        return HOOK_CONTINUE;
    }

    pop_deku_nut(bomb);
    if (retval != nullptr) {
        *static_cast<int*>(retval) = 1;
    }
    return HOOK_SKIP_ORIGINAL;
}

HookAction intercept_deku_nut_bomb_explosion(ModContext*, void* args, void* retval, void*) {
    auto* bomb = mods::arg<daNbomb_c*>(args, 0);
    if (!is_deku_nut_bomb(bomb)) {
        return HOOK_CONTINUE;
    }

    pop_deku_nut(bomb);
    if (retval != nullptr) {
        *static_cast<BOOL*>(retval) = TRUE;
    }
    return HOOK_SKIP_ORIGINAL;
}

HookAction suppress_deku_nut_bomb_fuse_effects(
    ModContext*, void* args, void* retval, void*)
{
    auto* bomb = mods::arg<daNbomb_c*>(args, 0);
    if (!is_deku_nut_bomb(bomb)) {
        return HOOK_CONTINUE;
    }

    if (retval != nullptr) {
        *static_cast<bool*>(retval) = true;
    }
    return HOOK_SKIP_ORIGINAL;
}

void clear_deku_nut_bombs_after_scene_change(
    ModContext*, void*, void* retval, void*)
{
    if (retval != nullptr && *static_cast<int*>(retval) != 0) {
        g_dekuNutBombIds.fill(fpcM_ERROR_PROCESS_ID_e);
    }
}

HookAction draw_deku_nut_bomb(ModContext*, void* args, void* retval, void*) {
    auto* bomb = mods::arg<daNbomb_c*>(args, 0);
    if (!is_deku_nut_bomb(bomb) || bomb->mpModel == nullptr) {
        return HOOK_CONTINUE;
    }

    J3DModel* nutModel = deku_nut_model_for_bomb(bomb);
    if (nutModel == nullptr) {
        return HOOK_CONTINUE;
    }

    mDoMtx_stack_c::copy(bomb->mpModel->getBaseTRMtx());
    mDoMtx_stack_c::scaleM(
        kDekuNutModelScale, kDekuNutModelScale, kDekuNutModelScale);
    mDoMtx_stack_c::transM(0.0f, kDekuNutHandOffsetY, 0.0f);
    nutModel->setBaseTRMtx(mDoMtx_stack_c::get());
    g_env_light.settingTevStruct(0, &bomb->current.pos, &bomb->tevStr);
    g_env_light.setLightTevColorType_MAJI(nutModel, &bomb->tevStr);
    mDoExt_modelUpdateDL(nutModel);

    if (retval != nullptr) {
        *static_cast<int*>(retval) = 1;
    }
    return HOOK_SKIP_ORIGINAL;
}

DEFINE_HOOK(&daNbomb_c::execute, DekuNutBombExecute);
DEFINE_HOOK(&daNbomb_c::procExplodeInit, DekuNutBombExplodeInit);
DEFINE_HOOK(&daNbomb_c::checkTimerStop, DekuNutBombCheckTimerStop);
DEFINE_HOOK(&daNbomb_c::draw, DekuNutBombDraw);
