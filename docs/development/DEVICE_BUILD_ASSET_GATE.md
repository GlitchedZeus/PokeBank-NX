# PokeBank NX — Device-Build Asset Gate

Purpose: prevent a technically valid `.nro` from being handed to a physical tester while generated RomFS assets are missing.

`/romfs/` is intentionally generated/gitignored. Project-authored fixes must never exist only there; see `docs/RECOVERY_CONTRACT.md`.

## Normal fresh-workspace recovery

Do **not** manually reconstruct the asset tree one piece at a time during a routine recovery.

Run:

```bash
python3 tools/recover_workspace.py
```

This restores:

```text
pinned HD Pokémon renders
pinned type icons
required UI fonts
tracked FireRed/LeafGreen game-card art
tracked recovery overrides
```

and then runs `tools/check_device_assets.py` automatically.

Expected success:

```text
RECOVERY COMPLETE
Generated asset preflight: PASS
```

If this deterministic path fails, report the exact failed stage. Only then use a narrower manual repair/forensic path.

## Current source of HD Pokémon renders

`tools/gen_hdsprites.py` mirrors the Pokémon HOME render tree from a pinned `PokeAPI/sprites` commit, downsizes the source 512×512 PNGs to 256×256, and writes them under:

```text
romfs/sprites/pokemon_hd/
```

The generator is build-time tooling. PokeBank NX does not need to fetch these images while running on Switch.

The verified full baseline is:

```text
HD renders: 3,260 / 3,260
base species: 1,025 / 1,025
type icons: 18 / 18
fonts: 3 / 3
embedded RomFS: 3,283 / 3,283 files
```

`recovery/RECOVERY_STATE.json` records the pinned recovery inputs and historical artifact identity.

## Offline preflight

After recovery, or whenever a build workflow modifies generated assets, run:

```bash
python3 tools/check_device_assets.py
```

The check verifies at minimum:

```text
romfs exists
HD Pokémon sprite directory exists
all 1..1025 base-species HOME renders are present
at least one shiny render is present
alternate-form numeric render IDs are present
representative Bulbasaur/Pikachu renders exist
all 18 type sprites are present
required UI fonts are present
FireRed/LeafGreen game-card art is present
```

If the preflight fails, **do not hand off the build as a visual-acceptance `.nro`**.

## Important evidence boundary

A passing asset preflight proves the expected local source files are present **before packaging**.

It does not by itself prove:

```text
the final .nro contains the RomFS
SpriteManager resolves the correct form
stb_image decodes the image on Switch
the Summary/View layout renders it correctly
```

Therefore a replacement device artifact must also verify the embedded RomFS and then be physically tested.

## Manual correction rule

A future session may not fix a sprite/form/render problem by changing only an ignored file in `romfs/` and then call the work saved.

The correction must be represented in GitHub as one of:

```text
tracked generator change
tracked form/sprite mapping change
tracked deterministic transform
legally-safe file under assets/recovery_overrides/romfs/
```

Then `tools/recover_workspace.py` must reproduce it from a clean workspace.

## Device-test record

For every replacement `.nro`, record:

```text
DEVICE ASSET PREFLIGHT: PASS
pinned PokeAPI sprite ref
number of generated HD PNG files
HD asset directory total size
embedded RomFS verification
```

Then physically test at least:

```text
normal Pokémon render
shiny Pokémon render
alternate/form Pokémon render where available
missing-art fallback
View Pokémon opens/closes repeatedly without crash or large hitch
```

Do not allow asset work to weaken save-safety locks. Live save writing remains a separate higher-risk gate.
