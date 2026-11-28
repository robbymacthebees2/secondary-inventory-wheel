struct TwilitItemEntry {
    u8 itemId;
    const char* id;
    const char* name;
    const char* description;
    const char* icon;
    u32 iconItem;
    TwilitEssentialsItemHandle handle;
    bool registered;
};

TwilitItemEntry g_twilitItems[] = {
    {kHawkGrassItem, "robin.hawk_grass", "Hawk Grass",
     "Use Hawk Grass to call a hawk.", nullptr, kHawkGrassIconTexture, 0, false},
    {kHoverBootsItem, "robin.hover_boots", "Hover Boots",
     "Toggle the Hover Boots.", kHoverBootsIconResource, 0xFFu, 0, false},
    {kDarkLinkItem, "robin.dark_link", "Dark Link Form",
     "Toggle Dark Link Form.", kDarkLinkIconResource, 0xFFu, 0, false},
    {kIceArrowItem, "robin.ice_arrow", "Ice Arrow",
     "Select or deselect Ice Arrows for the bow combo.", kIceArrowIconResource, 0xFFu, 0, false},
    {kLensOfTruthItem, "robin.lens_of_truth", "Lens of Truth",
     "Toggle the Lens of Truth.", kLensOfTruthIconResource, 0xFFu, 0, false},
    {kTruthMaskItem, "robin.stone_mask", "Stone Mask",
     "Toggle the Stone Mask.", kTruthMaskIconResource, 0xFFu, 0, false},
    {kDinFireItem, "robin.dins_fire", "Din's Fire",
     "Cast Din's Fire.", kDinFireIconResource, 0xFFu, 0, false},
    {kNayruLoveItem, "robin.nayrus_love", "Nayru's Love",
     "Toggle Nayru's Love.", kNayruLoveIconResource, 0xFFu, 0, false},
    {kGiantMaskItem, "robin.giants_mask", "Giant's Mask",
     "Toggle Giant's Mask.", kGiantMaskIconResource, 0xFFu, 0, false},
    {kOcarinaItemId, "robin.ocarina", "Ocarina",
     "Play test notes with A and the D-pad; press B to exit.", nullptr,
     kOcarinaDisplayItem, 0, false},
    {kShieldSurfItemId, "robin.shield_surf", "Shield Surf",
     "Toggle Shield Surfing.", nullptr, 0, 0, false},
    {kSitItemId, "robin.sit", "Sit",
     "Sit down or stand up.", kSitIconResource, 0xFFu, 0, false},
    {kGoronTunicItemId, "robin.goron_tunic", "Goron Tunic",
     "Toggle the Goron Tunic lava-walking effect.", nullptr, kGoronTunicDisplayItem, 0, false},
};

bool g_twilitItemButtonErrorLogged = false;

int twilit_button_for_handle(TwilitEssentialsItemHandle handle) {
    if (svc_te_items == nullptr) {
        return -1;
    }

    uint32_t button = TWILIT_ESSENTIALS_ITEM_BUTTON_NONE;
    const ModResult result =
        svc_te_items->get_item_button(mod_ctx, handle, &button);
    if (result != MOD_OK) {
        if (!g_twilitItemButtonErrorLogged) {
            svc_log->warn(mod_ctx, "Twilit Essentials could not report a mod-item button");
            g_twilitItemButtonErrorLogged = true;
        }
        return -1;
    }

    g_twilitItemButtonErrorLogged = false;
    if (button == TWILIT_ESSENTIALS_ITEM_BUTTON_NONE || button > 2) {
        return -1;
    }
    return static_cast<int>(button);
}

uint32_t use_twilit_item(void* userData) {
    auto* entry = static_cast<TwilitItemEntry*>(userData);
    if (entry == nullptr || !g_twilitItemsActive) {
        return 0;
    }

    const int button = twilit_button_for_handle(entry->handle);
    if (button < 0) {
        return 0;
    }

    if (button == 2 && mDoCPd_c::getHoldR(PAD_1) != 0 &&
        mDoCPd_c::getTrigZ(PAD_1) != 0)
    {
        return 1;
    }

    auto* alink = static_cast<daAlink_c*>(dComIfGp_getLinkPlayer());
    return use_secondary_item(alink, entry->itemId, button) ? 1u : 0u;
}

bool register_twilit_essentials_items() {
    g_twilitItemsRegistrationPending = false;
    if (!get_bool_option(g_useTwilitEssentialsItems, true)) {
        svc_log->info(mod_ctx, "Twilit Essentials item-page integration is disabled");
        return false;
    }
    if (svc_te_items == nullptr) {
        g_twilitItemsRegistrationPending = true;
        svc_log->warn(
            mod_ctx, "Twilit Essentials item API is unavailable; using the legacy item wheel");
        return false;
    }

    for (auto& entry : g_twilitItems) {
        TwilitEssentialsItemDesc desc = TWILIT_ESSENTIALS_ITEM_DESC_INIT;
        desc.id = entry.id;
        desc.name = entry.name;
        desc.description = entry.description;
        desc.icon = entry.icon;
        desc.icon_item = entry.iconItem;
        if (entry.itemId == kShieldSurfItemId) {
            desc.icon_item = shield_surf_display_item();
        }
        desc.use = use_twilit_item;
        desc.user_data = &entry;

        const ModResult result =
            svc_te_items->add_item(mod_ctx, &desc, &entry.handle);
        if (result != MOD_OK) {
            char errorMessage[128];
            std::snprintf(
                errorMessage, sizeof(errorMessage),
                "Twilit Essentials rejected item '%s' (result %d)",
                entry.id, static_cast<int>(result));
            svc_log->error(mod_ctx, errorMessage);
            unregister_twilit_essentials_items();
            return false;
        }
        entry.registered = true;
    }

    g_twilitItemsActive = true;
    g_twilitItemsRegistrationPending = false;
    svc_log->info(mod_ctx, "registered all 13 items with the Twilit Essentials item page");
    return true;
}

void unregister_twilit_essentials_items() {
    g_twilitItemsRegistrationPending = false;
    if (svc_te_items != nullptr) {
        for (auto& entry : g_twilitItems) {
            if (!entry.registered) {
                continue;
            }
            const ModResult result =
                svc_te_items->remove_item(mod_ctx, entry.handle);
            if (result != MOD_OK) {
                svc_log->error(
                    mod_ctx, "failed to remove a mod item from Twilit Essentials");
            }
            entry.registered = false;
        }
    }
    g_twilitItemsActive = false;
}

bool twilit_item_assigned_to_button(int button) {
    if (!g_twilitItemsActive || button < 0 || button > 2) {
        return false;
    }

    for (const auto& entry : g_twilitItems) {
        if (entry.registered && twilit_button_for_handle(entry.handle) == button) {
            return true;
        }
    }
    return false;
}

bool twilit_item_id_assigned_to_button(u8 itemId, int button) {
    if (!g_twilitItemsActive || button < 0 || button > 2) {
        return false;
    }

    for (const auto& entry : g_twilitItems) {
        if (entry.registered && entry.itemId == itemId) {
            return twilit_button_for_handle(entry.handle) == button;
        }
    }
    return false;
}
