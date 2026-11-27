struct LensPoeState {
    fpc_ProcID actorId = fpcM_ERROR_PROCESS_ID_e;
    decltype(e_po_class::field_0x5F4) originalAlpha{};
    bool originallyBattleTargetable = false;
    bool active = false;
};

std::array<LensPoeState, 32> g_lensPoeStates{};
bool g_lensVisionActivated = false;

HookAction preserve_lens_of_truth_vision(ModContext*, void* args, void*, void*) {
    auto* player = mods::arg<daAlink_c*>(args, 0);
    if (g_lensOfTruthActive && player != nullptr && !player->checkWolf()) {
        return HOOK_SKIP_ORIGINAL;
    }
    return HOOK_CONTINUE;
}

LensPoeState* find_lens_poe_state(fpc_ProcID actorId) {
    for (auto& state : g_lensPoeStates) {
        if (state.active && state.actorId == actorId) {
            return &state;
        }
    }
    return nullptr;
}

LensPoeState* capture_lens_poe_state(e_po_class* poe) {
    const fpc_ProcID actorId = fopAcM_GetID(&poe->enemy);
    if (LensPoeState* state = find_lens_poe_state(actorId)) {
        return state;
    }

    for (auto& state : g_lensPoeStates) {
        if (state.active) {
            continue;
        }
        state.actorId = actorId;
        state.originalAlpha = poe->field_0x5F4;
        state.originallyBattleTargetable =
            (poe->enemy.attention_info.flags & fopAc_AttnFlag_BATTLE_e) != 0;
        state.active = true;
        return &state;
    }

    return nullptr;
}

void restore_lens_poe_state(LensPoeState& state) {
    auto* actor = fopAcM_SearchByID(state.actorId);
    if (actor != nullptr && fopAcM_GetName(actor) == fpcNm_E_PO_e) {
        auto* poe = reinterpret_cast<e_po_class*>(actor);
        poe->field_0x5F4 = state.originalAlpha;
        if (state.originallyBattleTargetable) {
            poe->enemy.attention_info.flags |= fopAc_AttnFlag_BATTLE_e;
        } else {
            poe->enemy.attention_info.flags &= ~fopAc_AttnFlag_BATTLE_e;
        }
    }
    state = {};
}

void reveal_lens_poe(e_po_class* poe) {
    if (capture_lens_poe_state(poe) == nullptr) {
        return;
    }

    poe->field_0x5F4 = 315;
    poe->enemy.attention_info.flags |= fopAc_AttnFlag_BATTLE_e;
}

HookAction prepare_lens_poe_execute(ModContext*, void* args, void*, void*) {
    auto* leaf = mods::arg<leafdraw_class*>(args, 0);
    if (leaf == nullptr) {
        return HOOK_CONTINUE;
    }

    auto* actor = reinterpret_cast<fopAc_ac_c*>(leaf);
    if (fopAcM_GetName(actor) != fpcNm_E_PO_e) {
        return HOOK_CONTINUE;
    }

    auto* poe = reinterpret_cast<e_po_class*>(actor);
    auto* player = daAlink_getAlinkActorClass();
    if (!g_lensOfTruthActive || player == nullptr || player->checkWolf() ||
        poe->mActionID == ACT_HOLL_DEMO)
    {
        if (LensPoeState* state = find_lens_poe_state(fopAcM_GetID(actor))) {
            restore_lens_poe_state(*state);
        }
        return HOOK_CONTINUE;
    }

    reveal_lens_poe(poe);
    return HOOK_CONTINUE;
}

void reveal_lens_poe_after_execute(ModContext*, void* args, void*, void*) {
    auto* leaf = mods::arg<leafdraw_class*>(args, 0);
    if (leaf == nullptr || !g_lensOfTruthActive) {
        return;
    }

    auto* player = daAlink_getAlinkActorClass();
    if (player == nullptr || player->checkWolf()) {
        return;
    }

    auto* actor = reinterpret_cast<fopAc_ac_c*>(leaf);
    if (fopAcM_GetName(actor) != fpcNm_E_PO_e) {
        return;
    }

    auto* poe = reinterpret_cast<e_po_class*>(actor);
    if (poe->mActionID != ACT_HOLL_DEMO &&
        find_lens_poe_state(fopAcM_GetID(actor)) != nullptr)
    {
        poe->field_0x5F4 = 315;
        poe->enemy.attention_info.flags |= fopAc_AttnFlag_BATTLE_e;
    }
}

void update_lens_of_truth_poes() {
    auto* player = daAlink_getAlinkActorClass();
    if (player != nullptr) {
        if (g_lensOfTruthActive && !player->checkWolf()) {
            if (!player->checkWolfEyeUp()) {
                player->onWolfEyeUp();
                g_lensVisionActivated = true;
            }
        } else if (g_lensVisionActivated) {
            if (!player->checkWolf() && player->checkWolfEyeUp()) {
                player->offWolfEyeUp();
            }
            g_lensVisionActivated = false;
        }
    }

    const bool shouldRestore =
        !g_lensOfTruthActive || player == nullptr || player->checkWolf();
    for (auto& state : g_lensPoeStates) {
        if (!state.active) {
            continue;
        }

        auto* actor = fopAcM_SearchByID(state.actorId);
        if (actor == nullptr || fopAcM_GetName(actor) != fpcNm_E_PO_e) {
            state = {};
        } else if (shouldRestore) {
            restore_lens_poe_state(state);
        }
    }
}
