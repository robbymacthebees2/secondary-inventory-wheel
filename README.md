# RobinWheelAndTweaks

Made by RobinTheSoundsmith. This Dusklight code mod adds a second item wheel with 24 mod-owned item slots and a separate Goron Tunic slot. It reuses the vanilla item-wheel presentation while handling slot identity, selection, movement, and rendering independently of vanilla inventory indexes. Hawk Grass is provided in the first secondary slot and Hover Boots in the second; both can be assigned to X or Y.

## Credits, ports, and asset provenance

This is a mod pack, not an all-original project. The credits below consolidate the provenance recorded in the source, resource metadata, in-game Credits tab, and release notes for [all published releases from v1.0.1 through v3.9.5](https://github.com/robbymacthebees2/secondary-inventory-wheel/releases), including hotfixes. The current `main` source tree is smaller than those historical release packages: it contains one standalone visual asset, while older releases bundled additional models, textures, and integrated mod code.

### Features and code adapted from other mods

| Original project | Feature or code used in this mod |
| --- | --- |
| [Dawnlight](https://github.com/BeZide93/dawnlight) by BeZide93 | Dark Link lighting, red-eye and aura visuals; Manual Jump, Disable Auto Jump, glide, Sprint, bow/camera aiming and selective Bullet Time; Dual Wield combat/equipment; Skip Intro's reference preset; Flurry Rush; and the Enemy Spawner. The Dawnlight Gale Boomerang model and tornado sound also provide Revali's Gale gust effects. The release includes Dawnlight's CC0 1.0 notice; Dual Wield and Skip Intro were ported with permission. |
| [Fairy Companion](https://github.com/AgustinFuac/fairy-companion) v1.2.1 by AgustinFuac | Companion follow, color, light, and toggle behavior, ported with permission. Fairy abilities are not included. Midna companion appearances are this mod's additions. |
| [Fire Arrows Standalone](https://github.com/baxlek/Fire-Arrows-Standalone) v1.2.2 by Baxlek | Fire-arrow behavior that combines the Lantern and Bow, ported with permission. |
| [Unrestricted Items](https://github.com/baxlek/Unrestricted-Items) by Baxlek | Unrestricted item use, including FPV in Castle Town, movement in houses, and unrestricted Clawshots, plus related restricted-area and underwater item-use behavior. |
| [Lazy Tweaks](https://github.com/kamilink7/dusklight) by kamilink7 | Difficulty and combat options: sword, bow, and Wolf Link damage; progressive sword combos and defense; incoming damage/instant death; Zora Armor insulation and Wolf Link durability; heart, arrow, and rupee drops; death rupee taxes; hitstop; parrying; manual/crouch shielding; and dodge/jump-strike cancels. Its MIT notice is preserved in the historical release source. See also kamilink7's [Mod API port](https://github.com/kamilink7/lazytweaks-modapi). |
| [Crests of Courage](https://github.com/remiafterdark/crests-of-courage) by remiafterdark | Integrated co-op networking and remote-player/puppet code. Its source is MIT-licensed. Related credits include Fimmel (puppet/model-loading code), thyherooftime (Hyrule Online base and page), Dragonberri (Hyrule Online page UI), and the Dusklight team (port and SDK). The release source contains the full license and credits. |

These are adaptations within the combined mod; they do not imply that the original authors endorse this project. Compatibility features that merely interoperate with another mod are not represented as ports.

### Third-party models, textures, and other art in historical releases

| Asset or mod | Creator and source |
| --- | --- |
| Dark Link; Hero of Time; Hero of the Sky; Hero of the Wild; Hyrule Warriors; Ordon Warrior; Ordon Warrior (No Hat) | Dark Link by **LoadingError**; the other listed models by **SkilarBabcock**. These Crests of Courage model assets were included with permission. |
| Linkle model | **Ditrey**; concept art by **ThenMichael**; modeling help from **SkilarBabcock**. Included with permission through Crests of Courage. |
| Hero of Time voice pack | **SkilarBabcock**. |
| N64 Hero of Time outfit, equipment, and Epona model | **PxExYxTxOxN**. The historical release metadata does not name a source page or license. |
| Young Link face/body variants | **madnessgod**, [GameBanana mod 724984](https://gamebanana.com/mods/724984), ported with permission. |
| KePa - Wolf Link TP human-form outfits | **XModxGodX**, [GameBanana mod 723967](https://gamebanana.com/mods/723967), ported with permission. |
| Twilight Wolf | Credited in its model metadata to **Misc** (`com.misc_custom_twilight_wolf`); ported with permission. No source URL is recorded in the release metadata. |
| Zora Link and Zora Link (Alt Eyes) | Model and textures by **SkilarBabcock**, with fixes by **Oxygno**; [GameBanana mod 735675](https://gamebanana.com/mods/735675). |
| Subi Link black-hair and red-eyes textures | **Subi**, [GameBanana mod 677266](https://gamebanana.com/mods/677266); adapted for the Ordon outfit and used with permission. |
| OOT Style Epona skin | **HugAMuffin**, [GameBanana mod 683072](https://gamebanana.com/mods/683072), included with permission. |
| Twilight Epona skin | **Dusk_Rain_GFX**, [GameBanana mod 678152](https://gamebanana.com/mods/678152), included with permission. |
| Nightwalker Epona skin | **i12bp8**, [GameBanana mod 721556](https://gamebanana.com/mods/721556), included with permission. |
| OOT Style Hearts textures | **HugAMuffin**, included with permission; the source mod URL is not recorded in the release metadata. |
| Dawnlight Custom Glider texture | **BeZide**, [Dawnlight Custom Glider](https://github.com/BeZide93/DawnlightCustomGlider), used with permission. The glider model geometry was not taken from that mod. |
| `res/hover_boots_wings.bti` | Contributed to this repository by [@eigalum-oss](https://github.com/eigalum-oss) in [the commit that added it](https://github.com/robbymacthebees2/secondary-inventory-wheel/commit/2eb877f2ffba069bf41d51799d9c5b0ca8d03272). The original source mod/artist and a separate reuse license are not identified in the repository history. |

**Unresolved provenance:** the `Concept Art Wolf v2` texture set appears in the historical release resources, but its model metadata and repository history do not identify its original author, source mod, or permission. This README does not guess at that credit. The source mod for the Hover Boots icon is likewise undocumented; the contributor who added it is credited above, but that does not establish the original artist or reuse rights.

### Game assets and licenses

The mod also uses *The Legend of Zelda: Twilight Princess* content owned by **Nintendo**. This includes the vanilla wheel/HUD presentation, game icons, models, animations, effects, and audio loaded from game data, as well as game-derived resources included in some historical packages (for example, extracted/ported character and equipment models and the Deku Nut/slingshot-nut assets). The releases are unofficial fan works; Nintendo does not endorse them, and this repository does not claim ownership of Nintendo's content.

Co-op voice chat uses **Opus** by Xiph.Org and contributors under its BSD license. The required notice is included with historical release credits. The repository's MIT license covers only the project's own code; it does not relicense third-party or Nintendo assets. Follow each asset's stated terms, and obtain permission from its creator before reuse where applicable.

## Repository contents

- `src/mod.cpp` contains the mod implementation.
- `mod.json` contains the Dusklight mod manifest.
- `CMakeLists.txt` defines the mod target for Dusklight's CMake build.
- `res/` is the mod's resource directory; it contains the contributor-provided winged Hover Boots icon.

## Building

This mod is built from a Dusklight source checkout, which provides the SDK, headers, libraries, and `add_mod` CMake function. Place this directory at `mods/secondary_inventory_wheel` and ensure the checkout's top-level `CMakeLists.txt` includes:

```cmake
add_subdirectory(mods/secondary_inventory_wheel)
```

Configure Dusklight as usual, then build the `secondary_inventory_wheel_package` target. The build produces `secondary_inventory_wheel.dusk`.

## Slot model

The secondary wheel has 24 mod-owned item slots (`0x80` through `0x97`) plus a dedicated Goron Tunic slot (`0xFE`). Slot IDs and custom item IDs (`0xFB` through `0xFD`) are private to the mod and are never written into the game's vanilla X/Y item assignments. The two secondary assignments are saved independently in the mod configuration. Assigning a secondary item to X or Y leaves that button's vanilla assignment intact; while a secondary item is assigned, it takes precedence for that button's HUD icon and activation. Assigning a vanilla item to that same button from the vanilla wheel clears its private secondary assignment so the vanilla item immediately takes over. Hawk Grass uses its private item ID in the first slot and displays the game's stock Hawk Grass icon. Its assigned X/Y action enters the game's grass-whistle flow, retaining the native reed melody and hawk sound effects. After its attack and return flight finish, the Hawk called by modded Hawk Grass is removed after five seconds in idle with Link outside the Hawk interaction. Hover Boots use their private item ID in the second slot and a custom boots-with-wings icon in the wheel, X/Y preview, and HUD. The X/Y HUD icon is rendered directly from the mod resource, without using a vanilla item ID to load or scale it. Assign them to X or Y and press that button to toggle airwalking immediately; the Iron Boots equip animation plays on ground and in the air but does not gate the Hover Boots state change or modify vanilla Iron Boots. While active, the magnetic-boots particles follow Link’s foot joints immediately, including while he is still standing on solid ground. When Link walks or jumps off solid ground, Hover Boots keep him at the height where he left it and show a ripple beneath him. While airborne, the mod prevents vanilla auto-fall/ledge actions from replacing the current player action, including idle movement states, while allowing other mods' jump actions to run. At the hover plane it uses a one-shot landing animation and returns Link to movement without requiring a background polygon. The mod does not add an A-button jump. The normal A-button put-away behavior for vanilla held items is unchanged. After updating from older versions, assign secondary items again in the secondary wheel; the old implementation mirrored those assignments into vanilla item state.

The mod settings remain available in the Mods panel and can also be opened directly from the menu bar's **Robin's W&T** entry. The menu lets players choose which Left/Down D-pad direction opens the secondary wheel, walk on lava indefinitely with the Goron Tunic (without sinking or triggering the five-second lava return), toggle Twilight Ward, enable one-hit damage for attacks from the Hawk called by the secondary Hawk Grass, and toggle Hover Boots water walking. Twilight Ward is off by default and, while Link wears Magic Armor, normal and heavy hits still deal their usual damage but do not interrupt Link, stun him, or knock him back. Water walking is off by default and only applies while the secondary-wheel Hover Boots are equipped; it keeps movement on a flat water-level plane instead of using submerged floor slopes, drops, or ice-slip movement. It also suppresses submerged slide-polygon and slope checks and clears the vanilla ground-step offsets while the water plane is active. Enabling water walking replaces airwalking, so players can choose which Hover Boots behavior they prefer. With the one-hit option enabled, player targets take 20 hearts of damage. The modded Hawk's attack is classified as a thrown object, normal sword hit, and bomb for compatibility with enemy-specific damage handlers, including mounted Bulblins.

The menu also provides Rupee Magnet as an optional feature rather than a wheel item. Its radius defaults to 100% (300 game units, matching the game's existing rupee-return distance) and can be set from 50% (150 units) to 300% (900 units) in 25% steps. Lava walking, Twilight Ward, Hawk one-hit damage, Hover Boots water walking, and Rupee Magnet are off by default.

The secondary wheel's close behavior matches the vanilla wheel, including restoring background music and sound-effect levels when the wheel closes.

## License

Released under the MIT License. See [`LICENSE`](LICENSE).
