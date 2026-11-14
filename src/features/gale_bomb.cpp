namespace gale_bomb {
namespace {

DEFINE_HOOK(&daAlink_c::throwBoomerang, GaleBombThrowBoomerang);

}

void attach_bomb_to_boomerang(ModContext*, void* args, void*, void*) {
    auto* link = mods::arg<daAlink_c*>(args, 0);
    if (link == nullptr || link != daAlink_getAlinkActorClass() ||
        link->checkWolf() || link->checkEventRun() ||
        dComIfGp_event_runCheck() ||
        !gale_bomb_combo_selected(kZItemSelectIndex) ||
        link->mActiveBombNum >= 3)
    {
        return;
    }

    const u8 bombBag = gale_bomb_combo_bag_slot(kZItemSelectIndex);
    if (bombBag == dItemNo_NONE_e || dComIfGs_getBombNum(bombBag) == 0) {
        return;
    }

    auto* boomerang =
        static_cast<daBoomerang_c*>(link->getBoomerangActor());
    if (boomerang == nullptr ||
        fopAcM_GetName(boomerang) != fpcNm_BOOMERANG_e ||
        spawns_is_replica(boomerang))
    {
        svc_log->error(mod_ctx, "could not find the locally thrown Gale Boomerang");
        return;
    }

    cXyz bombPosition = boomerang->current.pos;
    bombPosition.y -= 30.0f;
    fopAc_ac_c* actor = dBomb_c::createNormalBombPlayer(&bombPosition);
    if (actor == nullptr) {
        svc_log->error(mod_ctx, "failed to create a bomb for the Gale Boomerang combo");
        return;
    }
    if (fopAcM_GetName(actor) != fpcNm_NBOMB_e) {
        fopAcM_delete(actor);
        svc_log->error(mod_ctx, "created actor was not a regular bomb for the Gale Boomerang combo");
        return;
    }

    auto* bomb = static_cast<daNbomb_c*>(actor);
    bomb->speedF = 0.0f;
    bomb->speed.set(0.0f, 0.0f, 0.0f);
    bomb->old.pos = bombPosition;
    if (!bomb->procBoomerangMoveInit(nullptr) ||
        fopAcM_GetParam(actor) !=
            static_cast<u32>(dBomb_c::PRM_BOMB_BOOMERANG_MOVE))
    {
        fopAcM_delete(actor);
        svc_log->error(mod_ctx, "failed to attach the Gale Boomerang combo bomb");
        return;
    }

    ++link->mActiveBombNum;
    dComIfGp_setItemBombNumCount(bombBag, -1);
    svc_log->info(mod_ctx, "Gale Boomerang launched with a regular bomb");
}

}
