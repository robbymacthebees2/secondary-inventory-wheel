# Robin's Wheel and Tweaks 1.5.1

## What's changed

- Added Twilit Essentials Items service v1.1 integration for all 12 mod items, with the legacy secondary wheel retained as a fallback.
- Added custom icons for Din's Fire, Nayru's Love, and Giant's Mask.
- Updated Ice Arrow behavior to create native water ice platforms and select or deselect the bow combo.
- Improved Hover Boots jump behavior and Extra Buttons compatibility.
- Added a multi-platform release workflow that combines the supported builds into one `.dusk` bundle.

## Twilit Essentials requirement

The Modded item page requires a Twilit Essentials provider that exports `com.dusklight.twilit_essentials.items` with the compatible v1.1 API. Twilit Essentials 3.1.3 does not export this service. Until a supporting update is installed and Dusklight is restarted, the Modded page will not appear; use the legacy wheel as a fallback if enabled.
