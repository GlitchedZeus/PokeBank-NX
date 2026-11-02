# PokeBank NX — Pokémon Cry Audio Audit — 2026-09-02

Purpose: record the cry/audio source discovered for the future Summary / View Pokémon experience without confusing technical availability with redistribution rights.

Primary tracker: **#35 — Add Pokémon cry playback to Summary / View screens**.

This document does **not** authorize bundling third-party Pokémon cry audio into public PokeBank NX releases.

## Product goal

A deliberate `View Pokémon` / Summary open should feel like a Pokédex entry:

```text
open Pokémon overview
        ↓
render Summary + Pokémon visual
        ↓
play the Pokémon cry once
```

Cry playback is presentation only. It must never alter Pokémon/save/Vault data and must never block navigation or back/cancel behavior.

## Primary reference — PokeAPI/cries

Repository:

```text
https://github.com/PokeAPI/cries
```

GitHub reported repository size at audit time: roughly 23 MB.

The README states:

- cries cover Pokémon from Generations 1 through 9;
- files use `.ogg`;
- cry filenames are mapped to PokéAPI Pokémon IDs;
- two collections exist:

```text
cries/pokemon/latest/
cries/pokemon/legacy/
```

The README shows `latest` extending through high PokéAPI form/entity IDs and `legacy` through 649.

Observed individual `latest` files are small: sampled entries are generally on the order of roughly 10–20 KB, though the full corpus should be measured before any packaging decision.

## Important rights/licensing distinction

The repository's `LICENSE` begins by stating:

```text
All audio files contents within are Copyright The Pokémon Company.
```

The same repository says it is distributed under CC0 1.0 Universal.

Do **not** treat that as blanket clearance to redistribute the Pokémon cry recordings inside the public PokeBank NX repository or `.nro`.

CC0 can only waive rights held by the affirmer; the license text itself also disclaims responsibility for clearing rights owned by other persons. Because the repository explicitly identifies the audio as The Pokémon Company's copyrighted content, public bundling requires a separate rights decision.

Classification:

```text
CRY COVERAGE / FORMAT / ID-MAPPING REFERENCE — HIGH VALUE
DEVELOPER-LOCAL TEST PACK — HIGH VALUE
OPTIONAL USER-INSTALLED PACK FORMAT INSPIRATION — HIGH VALUE
PUBLIC BUNDLED CRY PACK — NOT APPROVED WITHOUT RIGHTS REVIEW
```

## Recommended runtime architecture

Keep cry lookup independent of one provider:

```text
Pokémon entity
    ↓
CryKey {
  species
  form/variant where cry actually differs
  provider style
}
    ↓
local cry resolver
    ↓
OGG/audio decoder
    ↓
non-blocking playback
```

Do not make PokeBank NX depend on an online PokeAPI request at runtime.

Potential optional SD-card layout:

```text
/switch/PokeBank-NX/assets/audio/cries/<provider>/manifest.json
/switch/PokeBank-NX/assets/audio/cries/<provider>/latest/...
/switch/PokeBank-NX/assets/audio/cries/<provider>/legacy/...
```

A local manifest should eventually record:

```text
provider/version
CryKey
filename
format
size
SHA-256
source/attribution metadata
```

## UX requirements

### Automatic play

Play the cry **once** when the user deliberately opens Summary/View.

Do not play cries merely because focus moves across Pokémon in a box/Vault grid. Rapid cursor movement must remain quiet.

### Replay

Summary should expose a deliberate replay action, for example a small speaker/cry action or context control. Final controller mapping should be chosen together with #26 so it does not conflict with Summary tabs or compatibility actions.

### Audio controls

Settings should eventually include:

```text
Pokémon Cries: On / Off
Cry Volume: 0–100%   (if separate volume is practical)
```

Persist the choice.

If a broader application sound-volume model is later introduced, cries should integrate cleanly instead of inventing a second conflicting settings system.

### Playback safety / quality

- playback must be asynchronous/non-blocking;
- opening another Pokémon stops or replaces the previous cry rather than stacking audio;
- leaving Summary can stop/fade the cry;
- missing audio must never crash or stall the UI;
- malformed/unreadable audio should degrade to `Cry unavailable` or silent fallback;
- no network delay or network failure path;
- respect future accessibility/reduced-sensory preferences;
- do not auto-play repeatedly during fast L/R Summary paging without a short debounce/replacement policy.

## `latest` vs `legacy`

The source repository names these collections `latest` and `legacy`, but this audit does not independently establish exactly which game-generation/audio revision each file corresponds to.

Therefore first implementation should use one explicit default provider/style whose semantics are verified.

A later optional preference may be considered:

```text
Cry style
- Modern / latest
- Legacy where available
```

Origin-aware cry selection is only appropriate after the mapping between game generation and cry set is verified. Do not infer it solely from the directory names.

## Form handling

PokéAPI uses IDs above the National Dex range for various forms/variants. Multiple form IDs may share identical cry blobs.

PokeBank NX should **not** adopt PokéAPI form IDs as canonical save-format truth. Instead:

1. resolve the Pokémon using PokeBank NX's own species/form model;
2. map that canonical entity to a cry provider key;
3. allow aliases when multiple forms legitimately share one cry;
4. test known differing cries/forms explicitly.

## Native Switch engineering spike

Before implementation is claimed:

- identify existing native audio facilities already present in PKSE/PokeBank NX;
- choose a practical `.ogg`/Vorbis decoder path or documented conversion strategy;
- confirm licensing of any decoder dependency;
- render/play one cry on physical Switch;
- measure decode latency and memory;
- test handheld and docked audio;
- test repeated Summary open/close;
- test fast L/R Pokémon paging;
- test missing/corrupt cry files;
- test mute/volume persistence.

If decoding `.ogg` natively would add disproportionate complexity, a build/local-pack conversion step to another supported compressed format may be evaluated, but the provider manifest should remain format-aware.

## Relationship to visual Summary

The eventual polished screen can combine #9, #25 and #35:

```text
┌─────────────────────────────────────┐
│ Pikachu                    🔊 Replay │
│                                     │
│        [large Pokémon visual]       │
│                                     │
│ Lv. 72   ♂   SHINY                  │
│ Nature      Jolly                   │
│ Ability     Static                  │
│ Ball        [icon]                  │
│ Item        [icon]                  │
│ Origin      Pokémon FireRed         │
│ Location    Master Vault            │
│                                     │
│ Summary  Moves  Stats  Origin       │
└─────────────────────────────────────┘
```

Opening this screen may play the cry once when enabled.

## Scheduling

Do **not** start cry integration during Session 2.6.

Current critical path remains:

```text
#23 safety audit
#24 PLA crash hardening
preserve #13/#16 UI + #19 analog work
        ↓
replacement exact .nro
        ↓
second physical device test
```

Cry playback belongs with the later professional Summary / polish work after the core safety/device gate.