# PokeBank NX — Companion / Move Lab / Map & Guide Research

Date: 2026-09-03

Status: **POST-v1 RESEARCH / DO NOT IMPLEMENT DURING SESSION 2.6**

This document records a future product direction discovered while reviewing Pokédex UI projects, battle engines, emulator automation, game-script tools, link-cable implementations, and mined asset collections.

The core idea is to let PokeBank NX eventually become more than a bank/save manager without polluting the v1.0 critical path:

```text
Pokémon Summary / Pokédex
        +
Move Lab
        +
Save-aware Guide
        +
Map / encounter assistant
        +
Android emulator companion later
```

The first implementation should remain read-only/analytical. Live telemetry, overlays, RNG tooling, and external hardware links are separate later capabilities.

---

## 1. Enhanced Summary / Pokédex

The existing professional Summary work can eventually grow into a mini encyclopedia for the selected Pokémon.

Potential sections:

```text
SUMMARY
- species / form / gender / shiny
- type / ability / nature
- height / weight
- Pokédex text
- cry / replay
- origin + provenance

WHERE FOUND
- games in which the species/form is obtainable
- encounter areas
- encounter method
- version exclusivity
- legendary/event notes

EVOLUTION
- chain
- level/item/trade/location/special requirements

MOVESET
- currently known moves
- every move learnable in the selected game
- level-up / TM / tutor / egg / reminder / event source

HISTORY
- Vault provenance
- transfer/conversion path
- HOME compatibility where relevant
```

### UI references

#### skydoves/Pokedex

High-value Android architecture/UI reference for the future Android client:

- Kotlin;
- MVVM + repository pattern;
- Room local database;
- offline-first data layer;
- modularization;
- polished animated detail screens.

Classification:

```text
ANDROID ARCHITECTURE / SUMMARY UX REFERENCE — HIGH VALUE
NOT A POKÉMON SAVE OR CORRECTNESS ENGINE
```

#### TheAlphamerc/flutter_pokedex

Polished Pokédex card/detail layout with abilities, moves and power-oriented presentation.

Classification:

```text
VISUAL / NAVIGATION REFERENCE — MEDIUM VALUE
```

#### iampawan/PokemonApp

Small Flutter Pokémon app focused on attractive transitions/animations.

Classification:

```text
ANIMATION / UI REFERENCE — LOW-MEDIUM VALUE
```

#### alik0211/pokedex

Simple React/Redux list project.

Classification:

```text
GENERAL UI REFERENCE — LOW VALUE
```

#### Naereen/badges

Not Pokémon functionality. It is a README badge catalog only.

Possible later use: public-release README/CI/download/license badges.

Classification:

```text
README POLISH ONLY
```

---

## 2. Pokédex / game-data sources

### veekun/pokedex

A Python library + large extracted game-data database used by veekun.

Useful concepts include normalized relationships for:

```text
species/forms
moves
abilities
evolution
encounters
locations
versions
items
text
```

The project describes itself as languishing and contains older game-derived data, so it should not become the sole current correctness source.

Recommended role:

```text
DATA-SCHEMA / RELATIONSHIP REFERENCE — HIGH VALUE
SUPPLEMENTAL LEGACY DATA ORACLE — MEDIUM-HIGH VALUE
NOT PRIMARY MODERN CORRECTNESS SOURCE
```

Primary correctness remains PKHeX/PKSM-Core plus versioned game-specific sources such as pret, pk3DS, pkNX, UPR-ZX and other already-audited references.

### pk3DS

Already audited. Strong Gen VI/VII game-data oracle for encounters, personal data, moves, evolution, trainers and game text. Useful for Summary `Where Found`, Move Lab, and future guides.

### Poke Transporter GB

Already audited. Important for historical Gen I/II → III transfer research and PCCS conversion policy; not a Pokédex UI source.

---

## 3. Move Lab

Future concept:

```text
VIEW POKÉMON
  ↓
MOVE LAB
  ↓
selected game / generation
  ↓
all learnable moves
```

For each move:

```text
name
type
category
power
accuracy
PP
priority
learn method
learn level/TM/tutor/etc.
effect summary
generation/game availability
compatibility warnings
```

Potential filters:

```text
CURRENT MOVES
LEVEL-UP
TM/HM
TUTOR
EGG
EVENT
TRANSFER-ONLY
LEGAL IN THIS GAME
```

### pkmn/engine

`pkmn/engine` is a low-level, performance-focused battle simulation engine intended for tooling, embedded systems and AI.

Current important limitation at audit time:

```text
Generation I / II battle engines: current Stage 1 work
Generation III / IV: planned Stage 2
newer generations: longer-term scope
```

It exposes a C API and is MIT licensed, making it interesting for future isolated experiments.

Possible use:

```text
move mechanics oracle
battle-effect simulation
damage/choice experimentation
small interactive battle preview
host-side validation
```

It does **not** provide official battle animations or a ready-made graphical battle scene.

Classification:

```text
BATTLE MECHANICS / SIMULATION REFERENCE — VERY HIGH VALUE
FUTURE OPTIONAL ENGINE SPIKE — HIGH VALUE
MOVE ANIMATION ASSET SOURCE — NO
```

### Move animation preview

PokeBank NX could still build a custom preview system without shipping thousands of official animation assets.

Preferred architecture:

```text
Move
  ↓
MoveVisualKey
  ↓
base template
  - slash
  - projectile
  - beam
  - burst
  - impact
  - status pulse
  - weather
  - heal
  - stat effect
  ↓
Type/effect styling
  ↓
small per-move overrides where worthwhile
```

Example:

```text
Flamethrower → fire projectile/beam template
Thunderbolt  → electric strike template
Surf         → water wave template
Swords Dance → status/buff template
```

The purpose would be a fun Pokédex-style visualization, not a claim of reproducing the exact official battle animation.

---

## 4. PokeMiners/pogo_assets

This repository is extremely large and contains mined Pokémon GO assets, including:

```text
2D images
3D assets
Pokémon move sounds
music
UI sounds
text
```

The repository explicitly states that the content belongs to The Pokémon Company/Niantic and is provided for educational purposes.

Use:

```text
MOVE/SOUND/VISUAL RESEARCH — HIGH VALUE
STYLE / COVERAGE REFERENCE — HIGH VALUE
PUBLIC POKEBANK NX ASSET BUNDLE — NOT APPROVED
FULL REPO DOWNLOAD/BUNDLE — DO NOT USE
```

Do not make PokeBank NX depend on or redistribute mined GO assets without a separate rights review.

---

## 5. Save-aware Guide Mode

This is more realistic than a live map and can be useful even on Switch.

Given a loaded backup/save, PokeBank NX can eventually inspect supported progress values and produce contextual help:

```text
CURRENT / LAST KNOWN AREA
STORY PROGRESS
BADGES / KEY FLAGS
AVAILABLE LEGENDARIES
MISSED / AVAILABLE ITEMS
PUZZLES
ENCOUNTER TABLES
NEXT OPTIONAL OBJECTIVES
```

Example:

```text
REGI GUIDE — EMERALD

Sealed Chamber unlocked: YES
Relicanth requirement:      YES
Wailord requirement:        YES
Regirock chamber:           AVAILABLE
Regice chamber:             NOT COMPLETED
Registeel chamber:          NOT COMPLETED

[View Regirock puzzle]
[View Regice puzzle]
[Show locations]
```

This should be a transparent rule engine derived from game data/progress flags, not an opaque AI guess.

### Poryscript

Poryscript is a higher-level scripting language compiling to the scripts used by `pokeemerald`, `pokefirered`, and `pokeruby` decompilation projects. It models:

```text
flags
variables
map scripts
movement
NPC/dialogue control flow
conditional progression
```

Poryscript itself is not a save parser or guide database. Its value is understanding/normalizing Gen III event-script semantics and potentially helping tooling turn pret scripts into machine-readable guide/progression rules.

Classification:

```text
GEN III SCRIPT / PROGRESSION REFERENCE — HIGH VALUE
DIRECT RUNTIME DEPENDENCY — UNNECESSARY
```

---

## 6. Live emulator companion — proof of feasibility

### 40Cakes/pokebot-gen3

This is the strongest new reference in this batch for the future Android/Thor companion idea.

It runs on `libmgba` and reads live emulator state. Its local HTTP API exposes, among other things:

```text
/player
/player_avatar
  map bank / map ID / X/Y

/map
  current map
  player position
  tile data

/map_encounters
  normal + effective encounters

/event_flags
  current game progress flags

/opponent
  current/recent encounter

/encounter_log
/shiny_log
/encounter_rate

/party
/pokemon_storage
/pokedex
/items
```

Important architecture lesson: the HTTP server does not read emulator memory asynchronously at arbitrary times. It queues reads to the emulator/main thread after a frame to avoid inconsistent/garbage state.

This is directly relevant to a future Android companion running alongside an emulator.

Potential PokeBank NX Android display:

```text
┌───────────────────────────────┐
│ ROUTE 119                     │
│ Player: ●                     │
│                               │
│ Grass encounters              │
│ Zigzagoon   30%               │
│ ...                           │
│                               │
│ Shiny odds: 1 / 8192          │
│ Encounters this hunt: 2,481   │
│                               │
│ Nearby objectives             │
│ □ Feebas hunt                 │
│ □ Fortree Gym                 │
└───────────────────────────────┘
```

Classification:

```text
EMULATOR TELEMETRY / MAP ARCHITECTURE — VERY HIGH VALUE
GEN III MEMORY/SYMBOL REFERENCE — HIGH VALUE
AUTOMATION CODE FOR CORE BANK APP — DO NOT PORT BLINDLY
```

---

## 7. What a “shiny map” can and cannot mean

### Save-only

A normal save file can potentially tell PokeBank NX things such as:

```text
player location/progress
outbreak/event state when persisted
encounter areas
species availability
shiny charm / relevant progress
```

It generally cannot tell us that a future random encounter at coordinate X will be shiny.

For older random-encounter games, shininess is typically determined when the Pokémon is generated; there is not a permanent shiny creature waiting on a world-map tile.

Therefore the safe/default feature should be:

```text
SHINY HUNT MAP
- where the species appears
- encounter rate
- encounter method
- shiny odds for that game/context
- hunt counter/statistics
- relevant prerequisites
```

### Live emulator telemetry

With emulator integration, additional read-only information can be available in real time:

```text
player X/Y
current map
current encounters
current opponent
runtime event flags
hunt statistics
```

### RNG prediction

Predicting future shiny encounters from PRNG state is a separate advanced capability. It should not be confused with a normal map/guide and should not be required for Companion Mode.

### Modern Switch live overworld detection

Finding currently spawned shiny Pokémon while a Switch game is actively running would require game/version-specific **live runtime telemetry**, not merely parsing the save file.

That likely means a future optional overlay/sysmodule/external telemetry component and should remain far outside v1.

---

## 8. Android / AYN Thor companion direction

This fits naturally with `docs/V2_PLATFORM_VISION.md`.

Potential progression:

```text
v1.x Switch
- enhanced offline Summary/Pokédex
- static Where Found / learnset data
- save-aware guide where reliable

v2 Android
- full Vault client
- emulator save sources
- Companion screen
- emulator live map where supported
- encounter/shiny hunt statistics
- puzzle/legendary guide
- Move Lab with richer presentation

v2.x+
- LAN/USB link with Switch
- additional emulator adapters
- richer battle simulation
```

The Android implementation should use explicit per-emulator capability adapters rather than assuming arbitrary access to another app's private memory/files.

`skydoves/Pokedex` is a useful architectural example for an offline-first native Android data/UI layer.

---

## 9. Physical legacy link possibility

### kbembedded/Flipper-Zero-Game-Boy-Pokemon-Trading

This project implements real Gen I/II Game Boy link trading through Flipper Zero GPIO and documents the trade state machine/serial protocol.

It supports trading Pokémon to/from Red/Blue/Yellow/Gold/Silver/Crystal and demonstrates that a modern programmable device can impersonate the other end of a genuine Game Boy link trade.

Potential very-late PokeBank NX idea:

```text
Original GB/GBC cartridge
        ↕ real link cable
Flipper/ESP32/USB adapter
        ↕
PokeBank NX Android/Desktop
        ↓
Master Vault
```

This could preserve old cartridge Pokémon through an actual trade interface instead of requiring the user to dump a save first.

Classification:

```text
GEN I/II LINK PROTOCOL REFERENCE — VERY HIGH VALUE
FUTURE LEGACY HARDWARE BRIDGE — INTERESTING
V1 / V2 CORE REQUIREMENT — NO
```

Do not let this hardware side project delay save-file support.

---

## 10. Proposed feature boundaries

### Good near-term post-v1 additions

```text
Enhanced Summary/Pokédex
Where Found
Evolution requirements
Game-specific learnset
Move details
Static encounter maps
Save-aware legendary/puzzle checklist
```

### Good Android v2 additions

```text
Emulator source adapters
Live map for supported emulators
Encounter tables
hunt counters
current opponent
progress-aware guide
Move Lab
```

### Far-later / experimental

```text
Switch live-game overlay/sysmodule
runtime shiny-spawn detection
RNG prediction
physical GB/GBC link hardware
full graphical battle simulator
```

---

## 11. Safety / correctness rules

1. Companion telemetry should be **read-only by default**.
2. A guide should say `UNKNOWN` rather than invent progress when an adapter cannot reliably interpret a flag.
3. Static encounter/learnset data must be versioned by game and verified against strong references.
4. Do not claim a shiny is at a location unless actual runtime evidence supports that claim.
5. Do not turn live-memory writes/automation into a dependency of the core bank.
6. Keep optional emulator/runtime adapters outside the Master Vault correctness layer.
7. Never overwrite the original archived Pokémon merely to support a preview/simulation.
8. Do not redistribute mined official game assets without rights review.

---

## 12. Reference ranking from this batch

```text
40Cakes/pokebot-gen3
  → VERY HIGH: live emulator map/progress/encounter architecture

pkmn/engine
  → VERY HIGH: battle mechanics/simulation; currently Gen I/II focus

veekun/pokedex
  → HIGH: normalized Pokémon/game-data relationships

kbembedded/Flipper-Zero-Game-Boy-Pokemon-Trading
  → HIGH: genuine Gen I/II link protocol / future hardware bridge

skydoves/Pokedex
  → HIGH: future Android offline-first architecture + detail UX

huderlem/poryscript
  → HIGH: Gen III event/map-script understanding

PokeMiners/pogo_assets
  → HIGH research value, NOT approved redistribution source

TheAlphamerc/flutter_pokedex
  → MEDIUM: visual Pokédex reference

iampawan/PokemonApp
  → LOW-MEDIUM: animation/UI inspiration

alik0211/pokedex
  → LOW: simple web UI reference

Naereen/badges
  → README polish only
```

Already-audited `pk3DS` and `Poke_Transporter_GB` remain useful in their existing roles.

---

## Current project rule

This research must not alter the Session 2.6 mission or second-device-test gate.

Current priority remains:

```text
#23 safety UI
#24 PLA crash hardening
#19 Left Stick
#13/#16 PokeBank NX shell
#37 asset packaging
→ exact replacement .nro
→ physical device test #2
```

Only after the core bank is stable should Companion/Move Lab/Guide work be scheduled.