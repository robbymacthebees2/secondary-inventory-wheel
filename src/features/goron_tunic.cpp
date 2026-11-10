void tint_clothing_material(J3DModelData* model, u16 index) {
    if (model == nullptr || index >= model->getMaterialNum()) {
        return;
    }

    auto* material = model->getMaterialNodePointer(index);
    auto* color = material != nullptr ? material->getTevColor(1) : nullptr;
    if (color == nullptr) {
        return;
    }

    color->r = 72;
    color->g = 0;
    color->b = 0;
}

void set_goron_tunic_active(bool active) {
    g_goronTunicEquipped = active;
    g_goronLavaContactFrames = 0;
    colors_set_goron_tunic_active(active);
}

bool goron_tunic_can_activate(daAlink_c* alink) {
    return alink != nullptr &&
           alink->getClothesChangeWaitTimer() == 0 &&
           dComIfGs_getSelectEquipClothes() == dItemNo_WEAR_KOKIRI_e &&
           !alink->checkZoraWearAbility() &&
           !alink->checkMagicArmorWearAbility() &&
           !daAlink_c::checkCasualWearFlg();
}

void enforce_goron_tunic_clothing(ModContext*, void* args, void*, void*) {
    if (!g_goronTunicEquipped) return;
    auto* alink = mods::arg<daAlink_c*>(args, 0);
    if (alink == nullptr ||
        dComIfGs_getSelectEquipClothes() == dItemNo_WEAR_KOKIRI_e)
    {
        return;
    }

    dComIfGs_setSelectEquipClothes(dItemNo_WEAR_KOKIRI_e);
    alink->setClothesChange(0);
}

HookAction prevent_goron_tunic_clothing_change(ModContext*, void* args, void*, void*) {
    if (!g_goronTunicEquipped) return HOOK_CONTINUE;
    auto* menu = mods::arg<dMenu_Collect2D_c*>(args, 0);
    if (menu == nullptr ||
        (menu->mCursorX != 4 && !(menu->mCursorX == 5 && menu->mCursorY == 2)))
    {
        return HOOK_CONTINUE;
    }

    daAlink_c* alink = daAlink_getAlinkActorClass();
    if (alink != nullptr) alink->seStartSystem(Z2SE_SYS_ERROR);
    return HOOK_SKIP_ORIGINAL;
}
