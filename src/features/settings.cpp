ModResult build_panel(ModContext*, UiElementHandle panel, void*, ModError*) {
    static const char* kWheelDirectionOptions[] = {"Left", "Down"};
    static const char* kJumpButtonOptions[] = {"R", "L (LB)", "R3", "L3"};
    static const char* kGlideItemOptions[] = {"Cucco", "Glider"};
    static const char* kAimModeOptions[] = {"Vanilla", "3rd Person", "Cinema"};
    static const char* kBulletTimeModeOptions[] = {"Off", "Always", "BOTW"};
    static const char* kHoverPlaneLowerBindingOptions[] = {"L1", "L2", "L3", "R1", "R2", "R3"};
    static const char* kFairyColorOptions[] = {
        "Navi (Blue)", "Tatl (Orange)", "Tael (Purple)", "Green"};
    static const char* kFairyToggleBindingOptions[] = {
        "A", "LB", "R", "R3", "L3", "R+A", "R+LB", "R+R3", "R+L3"};
    initialize_sit_music_options();
    UiControlDesc control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_DROPDOWN;
    control.label = "Fairy Companion Color";
    control.help_rml = "Choose the color of the flying fairy companion.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_fairyCompanionColor;
    control.options = kFairyColorOptions;
    control.option_count = sizeof(kFairyColorOptions) / sizeof(kFairyColorOptions[0]);
    ModResult result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_DROPDOWN;
    control.label = "Fairy Companion Toggle Button";
    control.help_rml =
        "Press the selected button to toggle the fairy companion out or tucked away. "
        "R+A, R+LB, R+R3, and R+L3 are modifier combinations. "
        "A only toggles while Link is stationary with no weapon or item drawn.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_fairyCompanionToggleBinding;
    control.options = kFairyToggleBindingOptions;
    control.option_count =
        sizeof(kFairyToggleBindingOptions) / sizeof(kFairyToggleBindingOptions[0]);
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Fairy Companion Light";
    control.help_rml =
        "Enable the fairy's colored light, which illuminates nearby areas indoors and at night.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_fairyCompanionLight;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_DROPDOWN;
    control.label =
        "D-pad direction that opens the secondary inventory wheel (Down while Z is enabled)";
    control.help_rml =
        "The other Left/Down direction opens the map unless the Z item slot is enabled; then "
        "Midna uses Left and the map remains on Right. In the secondary wheel, press X or Y on "
        "the Goron Tunic or Hover Boots to equip or unequip it. Hover Boots hold Link at the "
        "height where he left the ground and emit a ripple while airborne. Optional water "
        "walking is available in this settings panel. Secondary X/Y assignments are private "
        "and take precedence over vanilla items on that button. Goron Tunic can only be "
        "activated while wearing the Hero's Tunic; while active, it keeps that tunic equipped "
        "and temporarily colors its cap, body, and skirt bright red. The wheel uses Down "
        "while the Z item slot is enabled, reserving Left for Midna; the map remains on Right.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_wheelOpenDirection;
    control.options = kWheelDirectionOptions;
    control.option_count = 2;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
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
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Enable Z item slot";
    control.help_rml =
        "Use Z for a third item assignment from either inventory wheel. The Z slot can "
        "activate vanilla items or any assignable secondary-wheel item. When enabled, "
        "Midna moves to D-pad Left, the secondary wheel opens with Down, and the map remains "
        "on Right. Disable "
        "Dawnlight's Z Item Slot and other third-item "
        "providers to avoid competing Z controls. Off by default.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_zItemSlotEnabled;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_NUMBER;
    control.label = "Dark Link sword damage multiplier";
    control.help_rml =
        "Set the damage dealt by Link's sword attacks while Dark Link is active. "
        "The default is 200%; fractional damage is rounded up. Set to 0% to disable "
        "sword damage, or choose any value up to 1000%.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_darkLinkDamageMultiplier;
    control.min = kDarkLinkDamageMultiplierMinPercent;
    control.max = kDarkLinkDamageMultiplierMaxPercent;
    control.step = 1;
    control.suffix = "%";
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Manual Jump";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_manualJumpEnabled;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_DROPDOWN;
    control.label = "Jump Button";
    control.help_rml =
        "Selects the manual jump button. R uses the game's normal R binding; L (LB) uses the "
        "left shoulder button rather than the camera/target trigger. R3 and L3 use stick clicks.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_jumpButton;
    control.options = kJumpButtonOptions;
    control.option_count = sizeof(kJumpButtonOptions) / sizeof(kJumpButtonOptions[0]);
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_NUMBER;
    control.label = "Jump Height";
    control.help_rml = "Set manual jump height from 100% (normal) to 500%.";
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
    control.label = "Disable Auto Jump";
    control.help_rml =
        "Prevent human and wolf Link from automatically jumping when running off a ledge. "
        "Walking off retains forward momentum while ordinary falls and ledge grabs remain "
        "available. Puzzle jumps, forced jumps, and event jumps are unchanged. Works independently "
        "of the Manual Jump setting. Off by default.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_disableVanillaAutoJump;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Sprint";
    control.help_rml =
        "Tap the game's Roll button and release it within a short window to roll. Hold it while "
        "running past the window to sprint. Earned speed carries into manual jumps. Off by default.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_sprintEnabled;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_NUMBER;
    control.label = "Sprint Speed";
    control.help_rml =
        "Set human sprint speed from 100% to 300% of normal run speed. Sprint animation cadence "
        "increases by half of the speed bonus.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_sprintSpeedPercent;
    control.min = 100;
    control.max = 300;
    control.step = 5;
    control.suffix = "%";
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Wolf Sprint";
    control.help_rml =
        "Hold the game's Dash button while moving as wolf Link to keep dash speed. Earned speed "
        "carries into Wolf jumps. Off by default.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_wolfSprintEnabled;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_NUMBER;
    control.label = "Wolf Sprint Speed";
    control.help_rml = "Set Wolf sprint speed from 100% to 300% of the native Dash speed.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_wolfSprintSpeedPercent;
    control.min = 100;
    control.max = 300;
    control.step = 5;
    control.suffix = "%";
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Sprint Stamina";
    control.help_rml =
        "Enable the Wheel's green stamina bar for Sprint and Wolf Sprint. It drains while sprinting "
        "and recovers when sprinting stops. When Twilit Essentials stamina is enabled, that system "
        "is used instead and this bar is hidden. Off by default.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_sprintStaminaEnabled;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_NUMBER;
    control.label = "Sprint Stamina Drain";
    control.help_rml = "Wheel stamina points drained per second while Sprint or Wolf Sprint is active.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_sprintStaminaDrain;
    control.min = 0;
    control.max = 20;
    control.step = 1;
    control.suffix = " /sec";
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_NUMBER;
    control.label = "Sprint Stamina Recovery";
    control.help_rml = "Wheel stamina points recovered per second while not sprinting.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_sprintStaminaRecovery;
    control.min = 1;
    control.max = 100;
    control.step = 1;
    control.suffix = " /sec";
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Glide";
    control.help_rml =
        "Press the jump button again while airborne to deploy the selected glide item. "
        "The item is put away when Link lands.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_glideEnabled;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_DROPDOWN;
    control.label = "Glide Item";
    control.help_rml = "Choose the native Cucco or Dawnlight's textured cloth Glider.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_glideItem;
    control.options = kGlideItemOptions;
    control.option_count = sizeof(kGlideItemOptions) / sizeof(kGlideItemOptions[0]);
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Revali's Gale";
    control.help_rml =
        "Hold the jump button through a manual jump and landing, stay crouched for one second, "
        "then release to launch Revali's Gale. The feature starts with three charges and recovers "
        "one charge at a time.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_revalisGaleEnabled;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_NUMBER;
    control.label = "Gale Height";
    control.help_rml =
        "Additional jump height for Revali's Gale, added to Jump Height. Default 500%; "
        "Jump Height 200% plus Gale Height 500% gives 700% total.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_galeHeightPercent;
    control.min = kGaleHeightMinPercent;
    control.max = kGaleHeightMaxPercent;
    control.step = 10;
    control.suffix = "%";
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_NUMBER;
    control.label = "Gale Charges";
    control.help_rml = "Maximum Revali's Gale charges. The default capacity is three.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_galeChargeCapacity;
    control.min = 1;
    control.max = kGaleChargeCapacityMax;
    control.step = 1;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_NUMBER;
    control.label = "Gale Recovery Time";
    control.help_rml = "Seconds required to recover each Gale charge. The default is 120 seconds.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_galeRecoverySeconds;
    control.min = 1;
    control.max = kGaleRecoveryMaxSeconds;
    control.step = 1;
    control.suffix = " sec";
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_DROPDOWN;
    control.label = "Aim Mode";
    control.help_rml =
        "Vanilla uses the original bow camera. 3rd Person keeps Link visible while aiming. "
        "Cinema uses the close over-the-shoulder aim camera.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_aimMode;
    control.options = kAimModeOptions;
    control.option_count = sizeof(kAimModeOptions) / sizeof(kAimModeOptions[0]);
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_TOGGLE;
    control.label = "Aim Movement";
    control.help_rml =
        "Allow movement while aiming. In Vanilla aim, movement uses the left stick and bow "
        "direction uses the right stick.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_aimMovementEnabled;
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_NUMBER;
    control.label = "Cinema Zoom";
    control.help_rml = "Adjust the Cinema aim camera zoom. 100% is the default view.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_cinemaZoomPercent;
    control.min = kCinemaZoomMinPercent;
    control.max = kCinemaZoomMaxPercent;
    control.step = 5;
    control.suffix = "%";
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_NUMBER;
    control.label = "3rd Person Reticle Y";
    control.help_rml =
        "Move the 3rd Person bow reticle vertically. Positive values move it up.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_thirdPersonReticleOffsetY;
    control.min = kThirdPersonReticleMinY;
    control.max = kThirdPersonReticleMaxY;
    control.step = 1;
    control.suffix = " deg";
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    control = UI_CONTROL_DESC_INIT;
    control.kind = UI_CONTROL_DROPDOWN;
    control.label = "Bullet Time";
    control.help_rml =
        "While aiming with the Bow in midair, Link's horizontal movement slows to 10% and "
        "vertical movement to 50%; enemies and NPCs slow to 10% and arrows to 20%. Always "
        "activates immediately; BOTW requires clearance above twice the native standing-jump "
        "height. Lasts up to five seconds.";
    control.binding = UI_BINDING_CONFIG_VAR;
    control.config_var = g_bulletTimeMode;
    control.options = kBulletTimeModeOptions;
    control.option_count =
        sizeof(kBulletTimeModeOptions) / sizeof(kBulletTimeModeOptions[0]);
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
    result = svc_ui->pane_add_control(mod_ctx, panel, &control, nullptr);
    if (result != MOD_OK) {
        return result;
    }

    return fire_arrows::add_settings(panel);
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
        "<p>Dark Link visual, body-aura mesh, and shaders based on Dawnlight by BeZide93 "
        "(CC0 1.0; license included in the package)</p>"
        "<p>Manual jump, Disable Auto Jump, glide, and Sprint features based on Dawnlight by "
        "BeZide93 (CC0 1.0; license "
        "included in the package)</p>"
        "<p>Bow aiming and selective Bullet Time based on Dawnlight by BeZide93 (CC0 1.0; "
        "license included in the package)</p>"
        "<p>Fairy companion follow, color, and light features ported from Fairy Companion by "
        "AgustinFuac with permission; fairy abilities are omitted.</p>"
        "<p>Dawnlight Custom Glider texture by BeZide, used with permission.</p>"
        "<p>Hero of Time, Hero of the Sky, Hero of the Wild, Hyrule Warriors, Ordon Warrior, and "
        "Ordon Warrior (No Hat) - SkilarBabcock</p>"
        "<p>Hero of Time voice pack - SkilarBabcock</p>"
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
