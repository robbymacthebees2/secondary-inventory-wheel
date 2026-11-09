ModResult build_panel(ModContext*, UiElementHandle panel, void*, ModError*) {
    static const char* kWheelDirectionOptions[] = {"Left", "Down"};
    static const char* kManualJumpOptions[] = {
        "Disabled", "A", "B", "L1", "R", "Z", "L3", "R3", "L2"};
    static const char* kHoverPlaneLowerBindingOptions[] = {"L1", "L2", "L3", "R1", "R2", "R3"};
    initialize_sit_music_options();
    UiControlDesc control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_DROPDOWN;
    control.label = "D-pad direction that opens the secondary inventory wheel";
    control.help_rml =
        "The other Left/Down direction opens the map. In the secondary wheel, press X or Y on "
        "the Goron Tunic or Hover Boots to equip or unequip it. Hover Boots hold Link at the "
        "height where he left the ground and emit a ripple while airborne. Optional water "
        "walking is available in this settings panel. Secondary X/Y assignments are private "
        "and take precedence over vanilla items on that button. The tunic color override is applied after "
        "game-level model color updates; integration with Crests' remote-player color "
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
    control.label = "Enable D-pad item wheel binding";
    control.help_rml =
        "When disabled, Left and Down use the game's normal map and item-wheel behavior "
        "instead of opening the secondary inventory wheel. Other mod features remain available.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_secondaryWheelBindingEnabled;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_DROPDOWN;
    control.label = "Manual jump button";
    control.help_rml =
        "Press the selected button while Link or Wolf Link is grounded and walking or standing "
        "to jump. Wolf Link uses his native jump action; Link's jump preserves his current "
        "horizontal momentum without adding forward speed. "
        "While Shield Surfing on the Hover Boots plane, this button starts a board jump and "
        "raises the plane with Link. "
        "L1 uses the gamepad's L1 input; L2 uses the game's vanilla L button. "
        "Select Disabled to turn off both manual jump and manual Cucco glide; doing so during "
        "an active glide releases Link. The selected button's normal action is replaced while "
        "the jump conditions are met.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_manualJumpBinding;
    control.options = kManualJumpOptions;
    control.option_count = sizeof(kManualJumpOptions) / sizeof(kManualJumpOptions[0]);
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_NUMBER;
    control.label = "Manual jump height";
    control.help_rml =
        "Set the manual jump's peak height from 100% (normal) to 300% of normal. "
        "Press the jump button while airborne, without Hover Boots, to enter the native Cucco "
        "glide without spawning a separate Cucco actor.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_manualJumpHeightPercent;
    control.min = kManualJumpMinHeightPercent;
    control.max = kManualJumpMaxHeightPercent;
    control.step = 10;
    control.suffix = "%";
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Disable vanilla autojump";
    control.help_rml =
        "Prevent the game's automatic jump from triggering. This does not disable the mod's "
        "Manual jump button. Off by default.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_disableVanillaAutoJump;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_NUMBER;
    control.label = "Glide bow slow time";
    control.help_rml =
        "Scale the game's simulation while aiming a bow during a manual glide, affecting Link "
        "and the world together. Dusklight frame interpolation smooths movement when enabled. "
        "At 100%, simulation pauses; lower this setting in the Mods panel to resume.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_glideBowSlowTimePercent;
    control.min = kGlideBowSlowTimeMinPercent;
    control.max = kGlideBowSlowTimeMaxPercent;
    control.step = 1;
    control.suffix = "%";
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

ModResult update_glide_bow_slow_time_setting(ModContext*, void*, ModError*) {
    update_manual_glide_bow_slow_time();
    return MOD_OK;
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
    tabs[0].update = update_glide_bow_slow_time_setting;

    UiWindowDesc desc = UI_WINDOW_DESC_INIT;
    desc.tabs = tabs;
    desc.tab_count = 1;
    desc.on_closed = on_robin_settings_window_closed;
    const ModResult result = svc_ui->window_push(ctx, &desc, &g_robinSettingsWindow);
    if (result != MOD_OK) {
        svc_log->error(ctx, "failed to open Robin's W&T settings");
    }
}

void dismiss_combined_info_dialog(ModContext*, UiDialogHandle, void*) {}

void open_combined_info_dialog(
    ModContext* ctx, const char* title, const char* body, UiDialogVariant variant) {
    UiDialogAction action = UI_DIALOG_ACTION_INIT;
    action.label = "Got it";
    action.on_pressed = dismiss_combined_info_dialog;

    UiDialogDesc desc = UI_DIALOG_DESC_INIT;
    desc.title = title;
    desc.body_rml = body;
    desc.variant = variant;
    desc.actions = &action;
    desc.action_count = 1;
    UiDialogHandle dialog = 0;
    const ModResult result = svc_ui->dialog_push(ctx, &desc, &dialog);
    if (result != MOD_OK) {
        svc_log->error(ctx, "failed to open combined Wheel information");
    }
}

void open_combined_credits(ModContext* ctx, void*) {
    open_combined_info_dialog(
        ctx, "Credits",
        "<p><b>Third-party character models</b></p>"
        "<p>Dark Link - LoadingError</p>"
        "<p>Hero of Time, Hero of the Sky, Hero of the Wild, Hyrule Warriors, Ordon Warrior, and "
        "Ordon Warrior (No Hat) - SkilarBabcock</p>"
        "<p>Linkle - Ditrey; concept art by ThenMichael; modeling help from SkilarBabcock</p>"
        "<p>Zora Link and Zora Link (Alt Eyes) - model and textures by SkilarBabcock; fixes by "
        "Oxygno. Source: GameBanana mod 735675.</p>"
        "<p>The Deku Nut uses the original in-game slingshot-nut asset.</p>"
        "<p><b>Co-op source</b></p>"
        "<p>Networking and remote puppet code from Crests of Courage by remiafterdark, licensed "
        "under MIT. Contributions: Fimmel, thyherooftime, Dragonberri, and the Dusklight team. "
        "Full source license and credits are included in the package.</p>",
        UI_DIALOG_NORMAL);
}
