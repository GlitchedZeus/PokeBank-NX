# PokeBank NX — Device-Build Asset Gate

Purpose: prevent a technically valid `.nro` from being handed to a physical tester while locally generated RomFS assets are missing.

This is especially important because `/romfs/` is intentionally gitignored. A clean checkout can therefore compile without the HD Pokémon renders used by the inherited `SpriteManager` / `View Pokémon` path.

## Current source of HD Pokémon renders

`tools/gen_hdsprites.py` mirrors the Pokémon HOME render tree from a pinned `PokeAPI/sprites` commit, downsizes the source 512×512 PNGs to 256×256, and writes them under:

```text
romfs/sprites/pokemon_hd/
```

The generator is build-time tooling. PokeBank NX does not need to fetch these images from the internet while running on Switch.

## Required preflight for a visual-acceptance / device-test build

From the repository root:

```bash
make types
make fonts
python tools/gen_hdsprites.py
python tools/check_device_assets.py
```

`tools/check_device_assets.py` is intentionally offline and stdlib-only. It verifies, at minimum:

```text
romfs exists
HD Pokémon sprite directory exists
all 1..1025 base-species HOME renders are present
at least one shiny render is present
alternate-form numeric render IDs are present
representative Bulbasaur/Pikachu renders exist
all 18 type sprites are present
required UI fonts are present
```

If the preflight fails, **do not hand off the build as a visual-acceptance `.nro`**. Generate/repair the missing local assets first or explicitly report the build blocked.

## Important evidence boundary

A passing asset preflight proves the expected local source files are present **before packaging**.

It does **not** by itself prove:

```text
the final .nro contains the RomFS
SpriteManager resolves the correct form
stb_image decodes the image on Switch
the Summary/View layout actually renders it correctly
```

Those remain build/runtime/device-test responsibilities.

## Second-device build requirement

For the next replacement `.nro`, record:

```text
DEVICE ASSET PREFLIGHT: PASS
PokeAPI sprite generator pinned ref
number of generated HD PNG files
HD asset directory total size
```

Then physically test at least:

```text
normal Pokémon render
shiny Pokémon render
alternate/form Pokémon render where available
missing-art fallback
View Pokémon opens/closes repeatedly without crash or large hitch
```

Do not allow this asset work to weaken or delay the current save-safety hard lock. The safety/crash gate remains higher priority than visual polish.
