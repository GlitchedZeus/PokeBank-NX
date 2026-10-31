# Generation I Pokémon Editor — Device Acceptance

Date: 2026-09-14

## Accepted physical candidate

The project owner physically tested and accepted the Generation I reusable Pokémon-editor foundation on a real Nintendo Switch.

```text
Application source: 69668bc81629228ef25c1bdada7c7ce1aed9b666
Application tree:   b0ec6d42a426b4bba4bff87ad7eb948ccafc4fe9
Branch:             feature/pokemon-editor-ui-foundation-final-20260913
PR:                 #66
NRO:                PokeBank-NX-Gen1-UX4-Retest-69668bc8.nro
NRO bytes:          161451761
NRO SHA-256:        3ab11f7ba6938bbab5f7cbbf192d819532ce94f09bc7788a3bb0d8f6217f3763
Artifact:           Gen1-UX4-Retest-Candidate
Artifact ID:        10333053333
Artifact digest:    sha256:f2019d188e6c4ac862def9e7837cd88824d649e9868de8f5e0caf1c6924a5b63
```

## Validation

Exact-head GitHub Actions validation for `69668bc81629228ef25c1bdada7c7ce1aed9b666`:

```text
34804727643 — PokeBank NX Host Tests — SUCCESS
34804727647 — Gen I Cleanup3 Focused Gates — SUCCESS
34804727640 — Gen I Editor Candidate Gate — SUCCESS
34804727732 — Gen I Cleanup3 Candidate Gate — SUCCESS
```

The first Host Tests attempt was externally cancelled during sanitizer execution after ordinary host/regression tests had passed; the same workflow was rerun against the exact same application SHA and completed successfully. No source change was required.

Validated gates include focused Gen I editor/safety regressions, source immutability, full permanent host suite, ASan, UBSan, complete RomFS recovery, Gen I normal/shiny sprite coverage, devkitA64 compile, final NRO link, embedded source identity, embedded RomFS verification and exact artifact packaging.

## Hardware acceptance

The final physical pass accepted the reusable Gen I editor and its safety/control follow-up. The remaining question from hardware testing concerned whether very low Gen I DVs could legitimately produce the displayed level-5 Venusaur battle stats. They can: with base stats `80/82/83/80/100`, DVs `HP 12 (derived), Atk 1, Def 1, Spe 8, Spc 2`, level 5 and zero Stat Exp, the authentic Gen I formulas produce exactly `HP 24 / Atk 13 / Def 13 / Spe 13 / Spc 15`, matching the device display.

Accepted editor behavior includes the capability-driven PKSE-style three-panel workspace, transactional existing-Pokémon Edit exit, session-local discard to the exact pre-edit staged state, Y DV randomization in `0..15`, L/R panel navigation, semantic move-compatibility colors, truthful Gen I supplemental data, generation-correct shiny presentation, real Gen I summary stats, five-axis radar, picker ownership fixes, unfocusable calculated Stat cells and staged-only source-safe editing.

## Permanent safety status

```text
GEN I BOXED STAGED POKÉMON EDITOR: DEVICE TESTED / DEVICE ACCEPTED
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE RETROARCH WRITES: HARD DISABLED
LIVE INSTALLED-GAME WRITES: HARD DISABLED
LIVE OTHER-EMULATOR WRITES: HARD DISABLED
PARTY EDIT: DEFERRED
```

Physical acceptance authorizes this staged editor milestone only. It does not authorize any live writeback path.