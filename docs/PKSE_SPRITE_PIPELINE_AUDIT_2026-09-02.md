# PokeBank NX — inherited PKSE sprite pipeline audit — 2026-09-02

Purpose: document how the inherited PKSE code obtains and renders Pokémon images, and what PokeBank NX should reuse instead of reinventing.

Primary relationship: **#25 — Pokémon render/model support** and **#9 — professional Summary**.

## Finding

PKSE does **not** fetch Pokémon artwork from the internet while the `.nro` is running.

It uses a build/developer-time download step and packages local PNG assets into RomFS. Runtime image lookup is completely local/offline.

This pipeline is already present on `feature/pokebank-playable` because PokeBank NX inherited it.

## Build-time HD sprite generation

Tool:

```text
tools/gen_hdsprites.py
```

The script mirrors the Pokémon HOME render tree from a **pinned** `PokeAPI/sprites` commit:

```text
PokeAPI/sprites
sprites/pokemon/other/home/
```

Pinned source in the currently inspected script:

```text
8dfa3d97e953caaafaafd4963eff7621811af08e
```

The script downloads 512×512 HOME-style PNGs and downsizes them to 256×256 with Lanczos before placing them under:

```text
romfs/sprites/pokemon_hd/
```

Local naming convention:

```text
<stem>.png      normal
<stem>s.png     shiny
<stem>f.png     female
<stem>fs.png    shiny female
```

Base species use National Dex IDs. Alternate forms commonly use PokéAPI 10000+ IDs; some special families use name-keyed stems.

The script deliberately mirrors the complete upstream HOME tree rather than maintaining a narrow hand-selected list, then validates that all base species expected by its current `DEX_MAX` are present.

Important: the HD sprite generation script requires internet **only on the development/build machine**. The Switch application itself does not use this network path.

## Current Makefile behavior

The project sets:

```text
ROMFS := romfs
```

so everything present in the local `romfs/` directory at build time is embedded into the `.nro` RomFS.

`make all` currently downloads type icons and fonts, but the Makefile explicitly says HD Pokémon sprites are **not** downloaded automatically. The developer must run:

```text
python tools/gen_hdsprites.py
```

before building if the local `romfs/sprites/pokemon_hd/` pack is not already present.

The repository `.gitignore` currently ignores:

```text
/romfs/
/data/
```

Therefore a clean Git checkout does **not** prove that Pokémon image assets are available for the resulting `.nro`. Asset presence is a local build-environment fact and must be explicitly checked/recorded.

## Runtime rendering

Core implementation:

```text
include/UI/SpriteManager.h
src/UI/SpriteManager.cpp
```

Runtime flow:

```text
Pokémon species/form/shiny
        ↓
FormSpriteMapping
        ↓
SpriteManager
        ↓
romfs:/sprites/pokemon_hd/<key>.png
        ↓ fallback
romfs:/sprites/pokemon/<key>.png
        ↓
stb_image decode
        ↓
NanoVG/OpenGL renderer
```

There is **no HTTP/network call** in the runtime sprite-loading path.

`SpriteManager::loadSprite()` builds a `romfs:/...` path and decodes it with `stbi_load`.

The normal sprite lookup prefers the 256px HD HOME render and falls back to a smaller 96px sprite directory if available. Forms that have no individual asset fall back again to the base species.

## Form resolver

File:

```text
src/Pokemon/FormSpriteMapping.cpp
```

It already maps many species/form combinations to PokéAPI form sprite IDs and contains special handling for families such as:

```text
Deoxys
Rotom
Giratina
Kyurem
Meloetta
Zygarde
regional forms
Pikachu caps
Totem forms
Mega/form variants
Ogerpon
Terapagos
and others
```

This is valuable existing engineering and should be reused/refactored rather than replaced by display-name filename guessing.

PokeBank NX should eventually generalize this into the canonical VisualKey/provider architecture described in `POKEMON_VISUAL_ASSET_AUDIT_2026-09-02.md` while preserving the proven mapping work.

## Existing memory management

`SpriteManager` already has a Pokémon sprite LRU cache with an approximately:

```text
48 MiB decoded-pixel budget
```

A 256×256 RGBA image is approximately 256 KiB decoded, so the cache budget is intended to retain roughly 190 such images before eviction pressure.

The renderer is notified before an evicted pixel buffer is freed so it can invalidate the GPU texture associated with that buffer address.

This is useful infrastructure for PokeBank NX and directly supports the bounded-cache goal in #21/#25.

## Existing Summary/box integration

The inherited source already calls `SpriteManager::getSprite(...)` from several UI locations, including:

```text
PartyPokemonPanel
BoxPokemonPanel
PokemonDetailsModal
```

The exact first physically tested source `3be4de6b...` already contained code in `PokemonDetailsModal.cpp` that asks `SpriteManager` for a species/form/shiny sprite and draws it at about 150×150 on the details screen.

Therefore the physical observation of a missing/blank Pokémon render is **not evidence that the UI lacks render code**. It strongly suggests that the required sprite file was unavailable to that binary, that form resolution failed, or that a packaging/rendering path failed.

## Important build-size clue / next verification

Because `/romfs/` is ignored by Git and HD assets are generated manually, future exact device artifacts must record whether Pokémon visual assets were present when the `.nro` was built.

For the next appropriate build session, verify before packaging:

```text
romfs/sprites/pokemon_hd/ exists?
expected test species files exist?
normal + shiny files present?
form mapping target exists?
RomFS actually included in final .nro?
SpriteManager startup/test load succeeds?
```

At minimum test known species such as:

```text
25 Pikachu
1 Bulbasaur
386 Deoxys + form
479 Rotom + form
regional form
shiny example
female visual variant where applicable
```

Do not call the artwork problem fixed until an exact `.nro` physically displays the expected image on Switch.

## Product recommendation

Do **not** replace the existing native sprite renderer for v1.

Recommended evolution:

```text
CURRENT INHERITED PATH
PokeAPI HOME PNG → build-time local RomFS → SpriteManager → NanoVG

        ↓ refactor

POKEBANK NX VISUAL PROVIDER
canonical VisualKey
        ↓
RomFS provider / optional SD-card provider
        ↓
existing SpriteManager-style lazy decoding + bounded cache
        ↓
Summary / Boxes / Vault / Pokédex
```

This gets PokeBank NX polished static/HOME-style visuals much sooner than introducing realtime GLB/3D rendering.

Realtime 3D remains optional/later under #25.

## Rights reminder

Technical inheritance does not resolve asset copyright.

The existing PKSE generator downloads HOME render imagery from `PokeAPI/sprites`, but the PokeBank NX public-release policy remains:

```text
technical availability != redistribution permission
```

For developer/device-test builds, record the provider/source snapshot used. For a public release, decide separately whether visual files can be bundled or should be supplied as an optional user-installed pack.

## Scheduling

Do not derail Session 2.6's primary safety/crash work.

However, because the next exact `.nro` packaging step already needs to inspect RomFS, it is reasonable for that session to **report** whether the inherited sprite assets are present and whether a simple local SpriteManager load succeeds. If fixing packaging is trivial and does not weaken safety or consume the session, it may be included; otherwise leave #25 open for the dedicated Summary/visual milestone.