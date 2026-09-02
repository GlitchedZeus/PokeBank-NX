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

`Pokemon-3D-api/api-server` includes `server/json/MergedOpt.json` mapping National Dex IDs to model/form records. This is useful as a **form/naming/coverage reference**, but PokeBank NX should not depend on the online REST API. PokeBank NX is offline-first; any runtime mapping should be local and versioned.

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

## 5. PokeAPI/sprites

Repository:

```text
https://github.com/PokeAPI/sprites
```

GitHub currently reports the repository at roughly **1.63 GB**, so the full archive must not be bundled into PokeBank NX.

The repository is exceptionally useful as a **coverage/layout reference** because its README documents a consistent tree with:

```text
sprites/pokemon/other/home/              512×512 HOME PNGs + shiny
sprites/pokemon/other/official-artwork/  475×475 PNGs + shiny
sprites/pokemon/other/showdown/           animated GIFs
sprites/pokemon/versions/                 game/generation-specific PNG/GIF sets
sprites/items/                            item sprites
```

Historical coverage documented in the repository includes Red/Blue/Yellow, Gold/Silver/Crystal, RSE/FRLG, DPPt/HGSS, BW, XY/ORAS, USUM, later icons/BDSP and Scarlet/Violet-era assets, with gender/shiny/back variants where available.

### Why it is high-value

For resolver design it gives us three useful visual classes in one predictable source:

```text
BOX/GRID        → compact game/icon sprite
SUMMARY         → HOME 512×512 or official-artwork style image
ANIMATED OPTION → Showdown/game GIF where appropriate
```

It also gives a useful naming/path model for generation-specific historical Summary styling.

### Rights/licensing

No root `LICENSE` file was found in the `PokeAPI/sprites` repository during this audit. Its README says applications may download/use the sprite archive, but that statement is not treated as a blanket copyright redistribution grant for Nintendo/TPC/Game Freak imagery.

Do not infer sprite-asset rights from the separate `PokeAPI/pokeapi` server-code license.

Classification:

```text
VISUAL COVERAGE / PATH / RESOLVER REFERENCE — VERY HIGH VALUE
DEVELOPER-LOCAL TEST SOURCE — HIGH VALUE
PUBLIC POKÉMON-ASSET BUNDLE — NOT APPROVED WITHOUT RIGHTS REVIEW
FULL ARCHIVE IN .NRO — DO NOT USE
ONLINE HOTLINK/RUNTIME DEPENDENCY — DO NOT USE
```

---

## 6. PokeAPI/pokeapi

Repository:

```text
https://github.com/PokeAPI/pokeapi
```

This is primarily a Pokémon data/API project, not a visual asset pack. Its server code carries a permissive BSD-style license with notice/attribution conditions.

Potential PokeBank NX value:

```text
species/name/form metadata cross-checks
canonical API naming reference
item/ability/move metadata reference
sprite-path relationship reference
```

PokeBank NX should remain offline-first and should **not** require the public PokéAPI service at runtime.

For correctness-critical Pokémon/save data, PKHeX/PKSM-Core and our own versioned tables remain stronger primary references. PokéAPI is supplemental metadata/reference infrastructure, not a save-format oracle.

Classification:

```text
SUPPLEMENTAL METADATA REFERENCE
NO ONLINE RUNTIME DEPENDENCY
```

---

## 7. msikma/PokéSprite

Repository:

```text
https://github.com/msikma/pokesprite
```

Project/overview:

```text
https://msikma.github.io/pokesprite/
https://msikma.github.io/pokesprite/overview/inventory.html
```

GitHub reports a much smaller repository than the giant 3D/full-history packs, roughly **25 MB**.

PokéSprite is particularly useful for **small controller UI assets and metadata structure**. Its README documents:

```text
pokemon-gen7x/   68×56 box sprites
pokemon-gen8/    68×56 box sprites
items/           32×32 inventory icons
items-outline/   32×32 Sword/Shield-style outlined icons
misc/            ribbons/body-style/etc.
data/pokemon.json
data/item-map.json
```

The Pokémon data tracks forms, aliases, visible female variants, legacy/unofficial icon flags and previous-generation fallbacks. This is exactly the kind of metadata discipline our own visual resolver needs.

The inventory overview maps internal-style item IDs to organized filenames and includes balls, medicine, held items and other categories. That makes it a strong UI/reference source for future Summary fields such as:

```text
Poké Ball
held item
ribbons/marks where matching assets exist
```

Do **not** blindly reuse its item IDs as universal save-format constants; map through PokeBank NX's generation-aware item model.

### Rights/licensing

PokéSprite explicitly states:

```text
sprite images = © Nintendo/Creatures Inc./GAME FREAK Inc.
code/other project material = MIT
```

Therefore the repository is excellent for metadata/layout/reference and local prototyping, but the sprite-image copyright is not converted into MIT by the project license.

Classification:

```text
BOX ICON / ITEM ICON / FORM-METADATA REFERENCE — VERY HIGH VALUE
OPTIONAL LOCAL PACK STRUCTURE INSPIRATION — VERY HIGH VALUE
PUBLIC GAME-SPRITE BUNDLE — NOT APPROVED WITHOUT RIGHTS REVIEW
```

---

## 8. Pokémon Database sprite archive

Reference:

```text
https://pokemondb.net/sprites
```

The site currently advertises sprite coverage from Generation 1 through Generation 9, including regular, shiny, back, form and gender variants where applicable, plus some spin-off material.

This is a strong **human-readable visual QA/reference** when checking whether our resolver chose the expected sprite/form for a Pokémon from a specific generation/game.

The site's footer states Pokémon images/names are copyrighted by Nintendo/Game Freak, and no redistribution permission for PokeBank NX was identified during this audit.

Classification:

```text
VISUAL QA / COVERAGE REFERENCE — HIGH VALUE
PUBLIC BUNDLE SOURCE — NOT APPROVED
```

Do not scrape or hotlink Pokémon Database as a runtime asset service.

---

## 9. Pokemon Infinite Fusion sprite page (coderobo)

Reference supplied:

```text
https://pokemon-infinite-fusion.coderobo.org/pokemon-sprite/
```

During this audit the page exposed no useful downloadable dataset/license information and displayed `00 Variant` in the parsed page.

More importantly, fusion sprites are not canonical main-series species/forms represented by normal PK1–PK9 entities. Treating arbitrary fan fusions as valid PokeBank NX entity visuals would corrupt our species/form model.

Classification:

```text
OUT OF SCOPE FOR CANONICAL POKÉMON VISUAL RESOLVER
NO RUNTIME USE
```

If PokeBank NX ever gains a completely separate fan/custom visualization plugin system after v1, fusion art could be reconsidered there. It should not enter the core Vault/Summary identity model.

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
ITEM/BALL/RIBBON ICON
```

Use lazy loading + bounded cache.

### Preferred resolver metadata lessons

Use the strongest structural ideas from PokeAPI/sprites and PokéSprite without coupling the app to either repository:

```text
canonical species/form key
explicit shiny flag
explicit visible-gender variant
provider/style
source generation/game where desired
alias/fallback handling
separate icon vs summary-render asset
```

## Stage B — optional local visual packs

Design a documented SD-card pack format, for example:

```text
/switch/PokeBank-NX/assets/pokemon/<provider>/manifest.json
/switch/PokeBank-NX/assets/pokemon/<provider>/icons/...
/switch/PokeBank-NX/assets/pokemon/<provider>/summary/...
/switch/PokeBank-NX/assets/items/<provider>/...
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
BOX/VAULT GRID:
compact HOME/Gen-8-style box icon resolver

SUMMARY/VIEW:
high-quality HOME-style/static or prerendered visual

DETAIL ICONS:
small Ball / held-item / ribbon / mark icons where supported

OPTIONAL:
animated local visual pack

EXPERIMENTAL LATER:
realtime 3D
```

This gives PokeBank NX a polished `View Pokémon` screen and richer Summary UI without making the whole project depend on multi-gigabyte copyrighted archives, a network service, or a new 3D rendering engine.

Do not start this during Session 2.6; safety/crash/second-device work remains the immediate blocker.
