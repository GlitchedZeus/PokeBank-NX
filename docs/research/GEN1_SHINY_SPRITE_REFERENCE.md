# Generation I Shiny Sprite Reference

Cleanup #3 does not invent a second art pipeline and does not recolor normal sprites at runtime.

## Existing approved pipeline

PokeBank NX already mirrors Pokémon HOME artwork through `tools/gen_hdsprites.py` from the pinned `PokeAPI/sprites` commit:

`8dfa3d97e953caaafaafd4963eff7621811af08e`

The generator mirrors `sprites/pokemon/other/home` and stores:

- `<species>.png` — normal HOME render
- `<species>s.png` — shiny HOME render
- form/female variants where upstream supplies them

The generated RomFS is reproducible and recovered by the existing candidate/release workflow before the native build.

## Runtime path

`SpriteManager::getSprite(species, shiny)` and `getIconSprite(species, shiny)` already include the shiny bit in their cache key and probe the generated HOME render before falling back to the bundled lower-resolution sprite path.

Cleanup #3 therefore reuses the existing cache for:

- Create/Edit live artwork;
- normal + shiny dual Species-picker preview;
- read-only Pokémon View;
- occupied Clone destination icons.

A missing single asset returns `nullptr` and the UI renders a text/empty fallback rather than crashing.

## Gen I coverage gate

`tools/check_device_assets.py` now requires `1s.png` through `151s.png` in the recovered `romfs/sprites/pokemon_hd/` directory before a Cleanup #3 device-test artifact may be packaged.

This is intentionally stricter than the older representative `at least one shiny` check because the owner must be able to preview Normal and Shiny for every supported Gen I species.
