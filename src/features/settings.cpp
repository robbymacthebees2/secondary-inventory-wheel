ModResult build_panel(ModContext*, UiElementHandle panel, void*, ModError*) {
    static const char* kWheelDirectionOptions[] = {"Left", "Down"};
    static const char* kHoverPlaneRaiseButtonOptions[] = {
        "A", "B", "L1", "R", "Z", "L3", "R3", "L2"};
    static const char* kHoverPlaneLowerBindingOptions[] = {"L1", "L2", "L3", "R1", "R2", "R3"};
    initialize_sit_music_options();
    UiControlDesc control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_DROPDOWN;
    control.label = "D-pad direction that opens the legacy item wheel";
    control.help_rml =
        "This applies only while the Twilit Essentials item-page integration is disabled or "
        "unavailable. The other Left/Down direction opens the map. Hover Boots hold Link at the "
        "height where he left the ground and emit a ripple while airborne. Optional water "
        "walking is available in this settings panel. The tunic color override is applied "
        "after game-level model color updates; integration with Crests' remote-player color "
        "replication is experimental.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_wheelOpenDirection;
    control.options = kWheelDirectionOptions;
    control.option_count = 2;
    ModResult result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Enable legacy D-pad item wheel binding";
    control.help_rml =
        "When disabled, Left and Down use the game's normal map and item-wheel behavior "
        "instead of opening the legacy secondary item wheel. Other mod features remain available.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_secondaryWheelBindingEnabled;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Use Twilit Essentials Modded item page";
    control.help_rml =
        "When Twilit Essentials is installed, register all 12 mod items on its Modded item page. "
        "Assign the items there; existing legacy-wheel assignments are not transferred. If the "
        "service is unavailable or registration fails, the legacy wheel remains active. Restart "
        "the game after changing this setting. Twilit Essentials assignments do not switch with "
        "Extra Buttons' alternate X/Y item sets.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_useTwilitEssentialsItems;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_DROPDOWN;
    control.label = "Raise Hover Boots plane button";
    control.help_rml =
        "While Hover Boots are active and Link is standing on the hover plane, press this button "
        "to jump. The plane automatically rises to Link's jump peak. While Shield Surfing on the "
        "Hover Boots plane, the same button starts a board jump and raises the plane to its peak. "
        "L1 uses the gamepad's L1 input; L2 uses the game's vanilla L trigger. Outside these "
        "conditions, the button keeps its normal action.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_hoverPlaneRaiseBinding;
    control.options = kHoverPlaneRaiseButtonOptions;
    control.option_count =
        sizeof(kHoverPlaneRaiseButtonOptions) / sizeof(kHoverPlaneRaiseButtonOptions[0]);
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Disable vanilla autojump";
    control.help_rml =
        "Prevent the game's automatic jump from triggering. Off by default.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_disableVanillaAutoJump;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_DROPDOWN;
    control.label = "Music while sitting";
    control.help_rml =
        "Play the File Select song while Link is seated. The area soundtrack resumes when Link "
        "stands or the Sit action ends.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_sitMusicTrack;
    control.options = g_sitMusicOptionLabels.data();
    control.option_count = g_sitMusicOptionLabels.size();
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Heal while sitting";
    control.help_rml =
        "When enabled, Link restores one quarter of a heart after sitting for four seconds, "
        "then once per second while seated. Healing uses the game's spring-healing sound and "
        "stops at the current maximum health.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_sitHealingEnabled;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Walk on lava indefinitely";
    control.help_rml =
        "When the Goron Tunic is equipped, stay at the lava surface without sinking or "
        "triggering the five-second lava return.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_infiniteLavaWalk;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Twilight Ward (Magic Armor)";
    control.help_rml =
        "When Magic Armor is worn, hits still deal damage but do not interrupt Link, stun him, "
        "or knock him back. The normal game handling remains active while Link is sinking in "
        "quicksand or lava.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_twilightWard;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Hawk Grass one-hit attacks";
    control.help_rml =
        "The Hawk called by the secondary Hawk Grass deals one-hit damage with its attacks.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_hawkOneShotDamage;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Hover Boots water walking (replaces airwalking)";
    control.help_rml =
        "When enabled, Hover Boots walk on water and no longer airwalk. Disabled by default.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_hoverBootsWaterWalking;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_DROPDOWN;
    control.label = "Lower Hover Boots plane button";
    control.help_rml =
        "While airwalking with Hover Boots, including while Shield Surfing, press this button "
        "to lower the hover plane by Link's vanilla standing-jump height. L1 uses the gamepad's "
        "L1 bumper input; L2 uses the game's vanilla L trigger. R3 is selected by default.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_hoverPlaneLowerBinding;
    control.options = kHoverPlaneLowerBindingOptions;
    control.option_count =
        sizeof(kHoverPlaneLowerBindingOptions) / sizeof(kHoverPlaneLowerBindingOptions[0]);
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Shield Surf R-button speed boost";
    control.help_rml =
        "While Shield Surfing, hold R to gradually accelerate to the vanilla Ice Leaf "
        "maximum of 70 game units per frame and maintain that speed.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_shieldSurfRBoost;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Disable Spinner surface bouncing";
    control.help_rml =
        "When enabled, the Spinner slides along walls and steep surfaces instead of reflecting "
        "its direction. Disabled by default.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_disableSpinnerSurfaceBounce;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Spinner attack hitbox always active";
    control.help_rml =
        "Keep the Spinner's powered-up attack hitbox active while riding, without performing "
        "the attack. Disabled by default.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_spinnerAttackAlwaysActive;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Rupee Magnet";
    control.help_rml = "Collect nearby rupees automatically. The radius is configurable below.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_rupeeMagnetEnabled;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Rupee Magnet collects clawshot minigame orbs";
    control.help_rml =
        "When Rupee Magnet is enabled, automatically collect nearby clawshot minigame orbs "
        "within the configured radius during an active minigame. Disabled by default.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_rupeeMagnetClawshotOrbs;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_NUMBER;
    control.label = "Rupee Magnet radius";
    control.help_rml =
        "100% is 300 game units, matching the game's existing rupee-return distance. "
        "The configurable range is 50% (150 units) to 300% (900 units).";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_rupeeMagnetRadiusPercent;
    control.min = kRupeeMagnetMinRadiusPercent;
    control.max = kRupeeMagnetMaxRadiusPercent;
    control.step = 25;
    control.suffix = "%";
    return svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
}

ModResult build_robin_settings_tab(
    ModContext* ctx, UiWindowHandle, UiElementHandle left_pane, UiElementHandle, void* user_data,
    ModError* error) {
    return build_panel(ctx, left_pane, user_data, error);
}

void on_robin_settings_window_closed(ModContext*, UiWindowHandle, void*) {
    g_robinSettingsWindow = 0;
}

void open_robin_settings_window(ModContext* ctx, void*) {
    if (g_robinSettingsWindow != 0) {
        return;
    }

    UiTabDesc tabs[] = {UI_TAB_DESC_INIT};
    tabs[0].title = "Settings";
    tabs[0].build = build_robin_settings_tab;
    UiWindowDesc desc = UI_WINDOW_DESC_INIT;
    desc.tabs = tabs;
    desc.tab_count = 1;
    desc.on_closed = on_robin_settings_window_closed;
    const ModResult result = svc_ui->window_push(ctx, &desc, &g_robinSettingsWindow);
    if (result != MOD_OK) {
        svc_log->error(ctx, "failed to open Robin's W&T settings");
    }
}
