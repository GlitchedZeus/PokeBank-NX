# PokeBank NX Project Status

Last updated: 2026-09-13

For the shortest engineering handoff, read `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md`.

## Project identity and safety

```text
Product: PokeBank NX
Version: 0.1.0-alpha
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream reference: kiasta/PKSE
Live installed-game writing: HARD DISABLED
Live RetroArch writing: HARD DISABLED
Live emulator-source writing: HARD DISABLED
```

`DEVICE ACCEPTED` is used only after the user physically tests the exact relevant artifact and reports the milestone working.

## Current headline

Generation I Red/Blue/Yellow, Generation II Gold/Silver/Crystal, and Generation III FireRed/LeafGreen/Ruby/Sapphire/Emerald are physically accepted for their legacy read-only paths.

The Generation II **boxed Pokémon staged editor** is now **HOST/NATIVE VERIFIED** and is awaiting hardware testing. It remains staged/export-only; no live installed-game, RetroArch, or emulator-source writeback is enabled.

```text
PHYSICAL STANDALONE TEST: PASSED
GEN II BOXED POKEMON EDITOR: HOST/NATIVE VERIFIED
GEN II EDITOR HARDWARE ACCEPTANCE: PENDING
PARTY EDIT: DEFERRED — SAFETY PROOF INCOMPLETE
JAPANESE GEN II: READ-ONLY
LIVE INSTALLED-GAME WRITES: HARD DISABLED
LIVE RETROARCH WRITES: HARD DISABLED
LIVE EMULATOR-SOURCE WRITES: HARD DISABLED
```

## Accepted legacy read baselines

| Game | State |
|---|---|
| Red GB | DEVICE TESTED / DEVICE ACCEPTED READ-ONLY |
| Blue GB | DEVICE TESTED / DEVICE ACCEPTED READ-ONLY |
| Yellow GB | DEVICE TESTED / DEVICE ACCEPTED READ-ONLY |
| Gold GBC | DEVICE TESTED / DEVICE ACCEPTED READ-ONLY |
| Silver GBC | DEVICE TESTED / DEVICE ACCEPTED READ-ONLY |
| Crystal GBC | DEVICE TESTED / DEVICE ACCEPTED READ-ONLY |
| FireRed GBA | DEVICE TESTED / DEVICE ACCEPTED READ-ONLY |
| LeafGreen GBA | DEVICE TESTED / DEVICE ACCEPTED READ-ONLY |
| Ruby GBA | DEVICE TESTED / DEVICE ACCEPTED READ-ONLY |
| Sapphire GBA | DEVICE TESTED / DEVICE ACCEPTED READ-ONLY |
| Emerald GBA | DEVICE TESTED / DEVICE ACCEPTED READ-ONLY |

Generation II trainer semantics remain:

```text
Gold Trainer gender: Male
Silver Trainer gender: Male
Crystal Trainer gender: save-derived
Gen II SID: DOES NOT EXIST
```

## Generation II staged boxed-Pokémon editor

The current staged editor preserves the source save bytes and supports boxed-Pokémon operations including:

- Species edit
- Nickname edit
- Level / EXP
- Held Item
- Moves
- PP
- PP Ups
- DVs and derived HP DV
- derived Gen II gender semantics
- Stat Experience
- OT
- TID
- Friendship
- Pokérus
- caught/met data where supported
- Make Shiny / Make Non-Shiny
- Add boxed Pokémon
- Clone boxed Pokémon
- semantic Pending Changes
- strict serialize → finalize → reparse → canonical semantic verification
- staged Trainer Name / Money / Inventory editing
- game-aware TM/HM machine move-name display without changing stored item IDs

Party browsing remains supported, but mutation of the Generation II 48-byte party battle-state structure is intentionally deferred.

### Resolved semantic defect

The editor previously exposed the semantic round-trip failure:

```text
gender expected=0 reparsed=2
```

Root cause: Generation II gender is derived from **species + Attack DV** and is not an independently stored PK2 field. The correct fix is to derive/canonicalize gender on both sides of semantic verification. The serializer was not the defect, the accepted parser was left unchanged, and validation was not weakened.

### Frozen development candidate evidence

```text
Frozen dev SHA: 4866de7fb54d3e9da884dd5ab862cb07902d3b28
Frozen dev tree: b990eeafe31a52b44a091adcebdb6a2c850e36de
Semantic normalization fix: 538c91feb01ab76bb3b102460d449c43f97727b7
Old native validation run: 34727424357
Completed native job: 103643931880
```

Frozen-dev gates:

```text
Focused PK2: PASS
Compound edit: PASS
Per-field semantic matrix: PASS
Add: PASS
Clone: PASS
Shiny: PASS
Derived gender: PASS
Trainer/Money/Inventory: PASS
Machine display: PASS
GSC bridge: PASS
Full host: PASS
ASan: PASS
UBSan: PASS
Safety invariants: PASS
Source mutation policy: PASS
Clean install: PASS
No /PKSE/ dependency: PASS
git diff --check: PASS
Device asset preflight: PASS
devkitA64: PASS
Native final link: PASS
```

### Production-candidate validation

The integrated application candidate was created from production without importing temporary CI carrier workflows. It was independently validated by GitHub Actions run `34728928845` against the exact application SHA/tree rather than the carrier commit.

```text
Integrated code candidate: fc3da981165f9d5d8a97871f21be3690ac20dc4d
Integrated code tree: dc972d147a915c074fea14e3774927dab24473c0
Production validation run: 34728928845
Native job: 103648021828 — PASS
Host job: 103648021879 — PASS
Full host: PASS
ASan/UBSan: PASS
Safety invariants: PASS
Device asset preflight: PASS
devkitA64: PASS
Native final link: PASS
```

The final production/documentation SHA and hardware-test artifact are recorded after exact-SHA packaging; hardware acceptance remains pending until the user tests that exact artifact.

## Standalone/runtime contract

PokeBank NX owns its runtime data and does not require PKSE or other external save tools to launch.

```text
PokeBank runtime root: sdmc:/switch/PokeBank-NX/
Gen II export root: sdmc:/switch/PokeBank-NX/exports/gen2/
Required /PKSE/: NO
Required PKSE.nro: NO
Required PKSM: NO
Required pkDex: NO
Required pkHouse: NO
Required JKSV: NO
Required Checkpoint: NO
Required PC PKHeX: NO
```

The native Switch edit flow must continue to show `SAVE WRITING NOT YET ENABLED` and provide discard/return behavior rather than implying installed-game writes were applied.

## Machine-name presentation

The reusable machine display system resolves machine labels using game identity + machine kind + machine number. Current routed mappings include GSC, FRLG, LGPE, Sword/Shield TM/TR, BDSP, Scarlet/Violet, and Legends Z-A. Visible labels may show forms such as `TM24 — Thunderbolt`; underlying stored item IDs remain unchanged.

## Generation I RBY physical-test history

The physically accepted corrected RBY runtime remains preserved. The exact accepted ItemsFix artifact was:

```text
Application source: 50dac31f53907143f48884681056f8d582813b76
Application tree: 1cf73ea12833e8a94a06dfaf2b9036e9059344ec
NRO: PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro
NRO bytes: 159754197
NRO SHA-256: b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b
ZIP: PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.zip
ZIP bytes: 152773612
ZIP SHA-256: f6dbdf2a76df1598e37ea0d1a771ac7a1eaa1864a29294c9101a6c6c09e2ecb6
Native/device build run: 34576781488
```

RBY opening, Trainer, Party, Boxes, Pokémon details, Items, Bag, PC Items, inventory navigation, and GB platform identity were physically accepted. Generation-appropriate Trainer fields are shown without fabricated SID/modern fields.

## Accepted Generation III baseline

```text
FireRed GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
LeafGreen GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
Ruby GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
Sapphire GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
Emerald GBA: DEVICE TESTED YES / DEVICE ACCEPTED YES
GEN III LEGACY READ-ONLY: PHYSICALLY ACCEPTED
```

Accepted RSE application source remains `a2df4c1acdb7a556808bd58a2bdbcd4fc0335954`; exact accepted RSE NRO SHA-256 remains `34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e`.

Do not reopen accepted Gen I, Gen II read-only, or Gen III behavior without new physical evidence of a defect.

## Current architecture / safety rules

- Legacy source identity remains provider/path/save-container based.
- Savestates are not canonical battery-save sources.
- Original source bytes remain untouched.
- Unknown save variants fail safely instead of being guessed writable.
- Staged/editor exports are app-owned outputs, not live source mutation.
- Live installed-game writing is HARD DISABLED.
- Live RetroArch writing is HARD DISABLED.
- Live emulator-source writing is HARD DISABLED.
- Physical acceptance of reads does not authorize live writes.
- Party editing remains deferred until its separate safety proof is complete.
- Japanese Generation II remains read-only.

## Immediate next step

Produce and retrieve the exact pushed production hardware-test package, verify local/retrieved hashes, then perform the Gold staged-editor hardware test. Do not enable live writeback during that test.
