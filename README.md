# Robin's Wheel and Tweaks 1.9.1

A Dusklight mod for *The Legend of Zelda: Twilight Princess* with a secondary item wheel, optional movement and combat features, and integration with Twilit Essentials' Modded item page.

## Twilit Essentials item page

Version 1.9.1 registers 13 mod items through the Twilit Essentials Items service (service major 1, minor 1): Hawk Grass, Hover Boots, Dark Link Form, Ice Arrow, Lens of Truth, Stone Mask, Din's Fire, Nayru's Love, Giant's Mask, Ocarina, Shield Surf, Sit, and Goron Tunic. Assign items to X, Y, or Z from the Modded page in the vanilla item wheel or Advanced UI.

This requires a Twilit Essentials release that exports `com.dusklight.twilit_essentials.items` with the compatible v1.1 API. Twilit Essentials 3.1.3 does not include that service, so the Modded page will not appear until the provider update is installed and Dusklight is restarted. The legacy secondary wheel remains available as a fallback when enabled in the mod settings. Existing legacy-wheel assignments are not transferred to the Modded page.

## Legacy secondary wheel

- 24 secondary item slots, plus dedicated entries for Shield Surf, Sit, Goron Tunic, and Ocarina.
- Open the wheel with a configurable D-pad direction (Left or Down) and assign items to X or Y without overwriting vanilla assignments.
- If the Twilit Essentials Items service is missing, registration fails, or the integration is disabled, use the legacy wheel when its binding is enabled.

## Items and gameplay features

- Ocarina: use the item to enter a stationary, damage-protected instrument mode with the game's horse-call model and animations. The confirmed notes are D4 (A), F4 (Down), A4 (Right), B4 (Left), and D5 (Up). Playing Left, Right, Down, Left, Right, Down plays the game's `menu_select.bms` sequence; Song of Healing restores Link to maximum health one second after the cue starts. Press B to exit. Other game music sequences and streamed music are paused while Ocarina mode is active and resumed afterward. Note inputs are captured before global game shortcuts; Start remains available to open the pause menu. Notes use an original JASTrack triangle-wave tone.
- Hawk Grass uses the native whistle flow to call a Hawk; an option makes the Hawk's attacks deal one-hit damage.
- Hover Boots let Link airwalk, optionally walk on water instead, and use configurable buttons to raise or lower the hover plane. Raising the plane starts a jump and automatically follows the jump's peak.
- Ice Arrow selects or deselects the bow combo. Hits on water create a native Twilight Princess ice-platform actor using the Snowpeak model and collision; platforms last about 15 seconds and are removed on scene changes or mod shutdown.
- Din's Fire, Nayru's Love, and Giant's Mask have custom icons.
- Dark Link Form, Lens of Truth, and Stone Mask retain their existing gameplay behavior.
- Shield Surf uses Link's equipped shield and has an optional R-button speed boost.
- Sit supports optional healing and File Select music while seated. Goron Tunic enables lava walking.
- Twilight Ward lets Magic Armor hits deal damage without interrupting, stunning, or knocking Link back.
- Rupee Magnet has an adjustable collection radius and can optionally collect clawshot minigame orbs.
- Spinner options can disable surface bouncing and keep the powered-up attack hitbox active while riding.
- Vanilla auto-jump can be disabled. The former standalone manual-jump and Cucco-glide features are not included in this build.

## Extra Buttons compatibility

With the legacy wheel, R+Z swaps the secondary X/Y assignments together with Extra Buttons' native item sets. Both sets are saved with the game save, and older single-set data remains compatible. In Twilit Essentials mode, the legacy swap is disabled and the shortcut is prevented from activating an item assigned to Twilit Essentials' Z button. The Items service does not expose alternate assignment sets, so Twilit Essentials X/Y assignments remain fixed during Extra Buttons swaps.

The Items service also has no selected-item callback for showing the legacy Ice Arrow prompt.

## Bundled icon credits

The `.dusk` includes nine standalone `.bti` icons. The maintainer confirms that all nine are cleared for redistribution with this release. Repository history credits `hover_boots_wings.bti` as contributed by [@eigalum-oss](https://github.com/eigalum-oss), but does not identify its original source or artist; provenance for the other bundled icons is not recorded here. This release's redistribution permission does not grant permission to reuse the icons separately.

Twilight Princess game models, textures, audio, and other original game assets used at runtime are loaded from the player's game installation and are not bundled in this repository.

## Multi-platform builds

The release workflow builds Windows (AMD64 and ARM64), macOS (Apple Silicon and Intel), iOS (Apple Silicon), Linux (x86_64 and aarch64), and Android (aarch64), then verifies and merges the platform libraries into one `.dusk` bundle.

Local builds are platform-specific and should not be distributed as universal bundles. The GitHub Actions workflow produces the combined release package.