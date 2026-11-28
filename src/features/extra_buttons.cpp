ModResult store_secondary_assignment_sets(ModContext* ctx) {
    SecondaryAssignmentSaveData assignments{};
    assignments.active = g_secondarySelectedItems;
    assignments.alternate = g_secondaryAlternateSelectedItems;
    return svc_save->set_blob(
        ctx, kSecondaryAssignmentsBlobName, &assignments, sizeof(assignments));
}

void swap_secondary_assignment_sets() {
    std::swap(g_secondarySelectedItems, g_secondaryAlternateSelectedItems);

    if (g_secondaryRing != nullptr) {
        for (int button = 0; button < 2; ++button) {
            const u8 slot = g_secondarySelectedItems[button];
            g_secondaryRing->setSelectItem(
                button, secondaryDisplayItemForSlot(slot));
            const auto found = std::find(
                g_secondaryWheelSlotIds.begin(),
                g_secondaryWheelSlotIds.end(), slot);
            const u8 wheelSlot = found == g_secondaryWheelSlotIds.end()
                ? dItemNo_NONE_e
                : static_cast<u8>(found - g_secondaryWheelSlotIds.begin());
            if (button == 0) {
                g_secondaryRing->mXButtonSlot = wheelSlot;
            } else {
                g_secondaryRing->mYButtonSlot = wheelSlot;
            }
        }
    }

    if (!secondaryTunicAssigned(0) && !secondaryTunicAssigned(1)) {
        g_goronTunicEquipped = false;
        g_goronLavaContactFrames = 0;
    }
    if (!secondaryHoverBootsAssigned(0) && !secondaryHoverBootsAssigned(1)) {
        disable_hover_boots();
    }

    if (g_secondaryHudAssignmentsInitialized) {
        g_secondaryHudAssignments = g_secondarySelectedItems;
        for (int button = 0; button < 2; ++button) {
            g_secondaryHudRefreshPending[button] = true;
        }
    }
}

bool extra_buttons_swap_requested() {
    if (g_twilitItemsActive || g_secondaryExtraButtonsSwapHandled ||
        !mDoCPd_c::getHoldR(PAD_1) || mDoCPd_c::getTrigZ(PAD_1) == 0)
    {
        return false;
    }

    g_secondaryExtraButtonsSwapHandled = true;
    return true;
}

HookAction swap_secondary_assignments_on_key_wait(
    ModContext*, void*, void*, void*)
{
    if (g_secondaryRing != nullptr || g_primaryRingOpen) {
        return HOOK_CONTINUE;
    }
    if (extra_buttons_swap_requested()) {
        swap_secondary_assignment_sets();
    }
    return HOOK_CONTINUE;
}

void swap_secondary_assignments_on_ring_wait(
    ModContext*, void* args, void*, void*)
{
    auto* ring = mods::arg<dMenu_Ring_c*>(args, 0);
    if (ring == nullptr) {
        return;
    }
    for (int index = 0; index < 4; ++index) {
        if (ring->field_0x674[index] != 0) {
            return;
        }
    }
    if (extra_buttons_swap_requested()) {
        swap_secondary_assignment_sets();
    }
}
