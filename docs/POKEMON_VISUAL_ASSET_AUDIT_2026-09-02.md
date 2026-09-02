# PokeBank NX — Pokémon Visual Asset Audit — 2026-09-02

Purpose: record visual/model sources that may help the Summary / View Pokémon experience without confusing technical availability with redistribution rights.

Primary tracker: **#25 — Add Pokémon render/model support to Summary and View screens**.

This audit does **not** authorize bundling third-party Pokémon artwork/models into public PokeBank NX releases. Each source has separate code/license/asset-rights considerations.

## Design goal

PokeBank NX should be able to resolve a Pokémon entity to an appropriate visual using canonical data:

```text
species
form
shiny
sex/gender where visually relevant
regional/mega/gmax/special state where representable
```

The visual resolver should be independent of any one asset pack.

Preferred runtime hierarchy:

```text
cleared/project-owned bundled visual
        ↓
optional user-installed local visual pack
        ↓
optional provider-specific local pack
        ↓
intentional fallback silhouette/icon + species/form/name
```

The app remains fully usable when no optional Pokémon visual pack is present.

---

## 1. Project Pokémon Sprite Index

Reference:

```text
https://projectpokemon.org/home/docs/spriteindex_148/3d-models-generation-1-pok%C3%A9mon-r90/
```

Observed value:

- 3D-model GIF index pages across generations;
- HOME sprite collections across multiple generations;
- historical GB/NDS sprite sets;
- later-game/Switch-era visual sets;
- useful form/style/naming coverage reference.

Classification:

```text
COVERAGE / FORM / DEVELOPER-LOCAL REFERENCE
```

Do not assume hosting implies redistribution permission.

---

## 2. tdmalone/pokecss-media

Repository:

```text
https://github.com/tdmalone/pokecss-media
```

State: archived.

Repository size reported by GitHub: roughly 235 MB.

README describes it as a compilation of Pokémon 3D-model-based sprites, menu icons, items, symbols and glyphs, aimed mainly at sixth-generation-and-later media.

Useful structure includes:

```text
graphics/pokemon/ani-front/
graphics/pokemon/ani-front-shiny/
graphics/pokemon/ani-back/
graphics/pokemon/ani-back-shiny/
graphics/pokemon/front/
graphics/pokemon/front-shiny/
graphics/pokemon/icons-*/
```

Animated front GIFs observed are often on the order of tens to hundreds of KB per Pokémon/form.

### Rights/licensing

The repository README explicitly distinguishes its code/custom graphics from the game-extracted graphics:

- code/custom glyphs are MIT-licensed;
- game graphics remain Nintendo/Creatures/GAME FREAK property;
- the maintainer frames bundled game graphics as fan/fair-use material and says reuse is at the user's legal risk.

Therefore the MIT license is **not** a blanket redistribution license for the Pokémon sprites themselves.

### PokeBank NX classification

```text
STATIC/ANIMATED VISUAL REFERENCE
OPTIONAL DEVELOPER-LOCAL TEST PACK CANDIDATE
NOT APPROVED FOR PUBLIC BUNDLING WITHOUT ASSET-RIGHTS REVIEW
```

### Technical value

The GIF/static-image route may be dramatically easier than realtime 3D on Switch:

```text
Summary opens
    ↓
resolver chooses species/form/shiny key
    ↓
load one small image/GIF on demand
    ↓
cache a bounded number
```

This is a realistic middle ground if an asset set with clear redistribution rights is found or the user supplies a local pack.

---

## 3. Pokémon 3D API organization

Organization:

```text
https://github.com/Pokemon-3D-api
```

Relevant current repositories:

```text
Pokemon-3D-api/assets
Pokemon-3D-api/api-server
Pokemon-3D-api/Showcase
```

The user's older link `Sudhanshu-Ambastha/Pokemon-3D-api` is archived and now points users to this organization.

### 3D assets repository

Repository:

```text
https://github.com/Pokemon-3D-api/assets
```

GitHub reports the repository around 1.47 GB.

README reports an automated pipeline that:

```text
downloads raw .glb models from Sketchfab
uses Draco geometry compression
resizes textures to 1024×1024
converts textures to WebP
stores optimized .glb files
```

Observed model categories include:

```text
regular
shiny
mega
megaShiny
gmax
alolan
hisuian
galar
primal
origin
fusion
multiform
shadow
special/experimental variants
```

The README's current coverage table reports approximately:

```text
971 / 1028 regular forms
150 / 1028 shiny forms
56 mega entries
17 / 17 Hisuian
12 / 18 Alolan
11 / 20 Galarian
plus gmax/primal/fusion/origin/multi/special categories
```

Coverage is useful but incomplete, especially shiny/regional/special forms.

### File-size implications

Observed optimized `.glb` files vary enormously. Examples from the current regular directory include roughly:

```text
~33 KB
~39 KB
~0.5 MB
~2.5 MB
~5 MB
~8.5 MB
~10.9 MB
```

Therefore bundling every model in the `.nro` or normal app package is not practical.

A realtime-3D path would also require native Switch support for the model stack actually used by these files, including GLB/glTF parsing, Draco-compressed geometry, WebP textures, materials, animations and rendering. The existing web project uses browser-oriented `<model-viewer>` / Three.js patterns; those are not drop-in native Switch components.

### Metadata API

`Pokemon-3D-api/api-server` includes `server/json/MergedOpt.json` mapping National Dex IDs to model/form records, for example:

```json
{
  "id": 3,
  "forms": [
    {"name":"Venusaur","formName":"regular"},
    {"name":"Shiny Venusaur","formName":"shiny"},
    {"name":"Mega Venusaur","formName":"mega"},
    {"name":"Mega Shiny Venusaur","formName":"megaShiny"},
    {"name":"G-Max Venusaur","formName":"gmax"}
  ]
}
```

This is useful as a **form/naming/coverage reference**, but PokeBank NX should not depend on the online REST API. PokeBank NX is offline-first; any runtime mapping should be local and versioned.

### History stability warning

The assets repository pipeline intentionally squashes/force-pushes history after a small commit count. Do not assume a long-lived Git commit SHA will remain fetchable forever.

If a developer-local asset snapshot is ever used for reproducible testing, record an independent manifest containing filenames, source URLs where known, file sizes and SHA-256 values.

### Rights/licensing

The repository has an MIT license file, but its README says the 3D models are property of Nintendo/Creatures/GAME FREAK.

The current API-server copyright notice makes the distinction explicit:

```text
MIT applies to project code.
MIT does not grant rights to Pokémon intellectual property/models.
```

Therefore:

```text
API/server code: MIT candidate if useful
Pokémon 3D models: NOT MIT-cleared by that code license
```

Do not bundle the models into PokeBank NX merely because the repository contains `LICENSE`.

### PokeBank NX classification

```text
FORM/COVERAGE/MODEL-PIPELINE REFERENCE — HIGH VALUE
DEVELOPER-LOCAL 3D EXPERIMENT SOURCE — HIGH VALUE
OPTIONAL USER-INSTALLED MODEL PACK FORMAT INSPIRATION — HIGH VALUE
PUBLIC MODEL BUNDLE — NOT APPROVED WITHOUT RIGHTS REVIEW
ONLINE API RUNTIME DEPENDENCY — DO NOT USE
```

---

## 4. Sudhanshu-Ambastha/Pokemon-3D-api

Repository:

```text
https://github.com/Sudhanshu-Ambastha/Pokemon-3D-api
```

State: archived/moved to `Pokemon-3D-api` organization.

The legacy README describes the previous JSON API/model viewer and documents the same broad form categories and optimization strategy. It is useful for historical documentation only; new technical references should prefer the organization repositories.

Its copyright notice explicitly says its MIT license applies to code, not Pokémon IP/models.

Classification:

```text
HISTORICAL REFERENCE ONLY
```

---

# Recommended PokeBank NX visual plan

## Stage A — solve View Pokémon without realtime 3D

Implement the resolver first, independent of assets:

```text
VisualKey {
  species
  form
  shiny
  genderVisualVariant
  style/provider
}
```

Provide a safe fallback so Summary never has blank dead space.

Then support small/large visuals as separate classes:

```text
GRID ICON
SUMMARY RENDER
```

Use lazy loading + bounded cache.

## Stage B — optional local visual packs

Design a documented SD-card pack format, for example:

```text
/switch/PokeBank-NX/assets/pokemon/<provider>/manifest.json
/switch/PokeBank-NX/assets/pokemon/<provider>/icons/...
/switch/PokeBank-NX/assets/pokemon/<provider>/summary/...
```

Manifest should carry:

```text
provider/version
asset key
species/form/shiny/gender mapping
filename
format
size
SHA-256
optional attribution/source metadata
```

PokeBank NX should not auto-download copyrighted packs.

## Stage C — animated 2D / prerendered 3D

Before native realtime 3D, evaluate animated GIF/APNG/WebP or pre-rendered sequences as a lower-complexity option.

This can deliver a lively HOME-like Summary without needing a full model renderer.

## Stage D — optional realtime 3D experiment

Only after core app/Vault work is stable, spike one local `.glb` model on Switch and measure:

```text
library/dependency cost
GLB/glTF parser
Draco decode support
WebP texture decode
GPU/render path
animation support
load time
RAM/VRAM
Applet Mode behavior
handheld/docked performance
```

Do not promise realtime 3D until one model has physically rendered on Switch with acceptable resource use.

If native 3D is viable, Right Stick rotation can be reconsidered under #25.

---

# Current recommendation

For v1 product planning:

```text
DEFAULT: high-quality 2D/static or prerendered Summary visual
OPTIONAL: animated local visual
EXPERIMENTAL LATER: realtime 3D
```

This gives PokeBank NX a polished `View Pokémon` screen without making the whole project depend on a multi-gigabyte copyrighted model archive or a new rendering engine.

Do not start this during Session 2.6; safety/crash/second-device work remains the immediate blocker.
