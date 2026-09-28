# PokeBank NX — Current Verified Engineering State

Last updated: **2026-09-28**

GitHub is authoritative. Re-fetch live heads before new work and preserve newer commits.

## Active development line

### PR #87 — Gen IV shared staged Pokémon editor

State:
**OPEN / DRAFT / NOT MERGED**

Branch:
**feature/gen4-shared-pokemon-editor-20260928**

Base:
**audit/full-project-hardening-20260923**

Current exact head:
**6e9f54f2a9a0cea79f14942912ba76aaca3fa0ed**

Tracking issue:
**#86 — G4-03 Gen IV shared staged Pokémon editor**

Current milestone:
**Party + boxed PK4 View/Edit through the shared Pokémon editor, correcting the first hardware-rejected route**

### Accepted base checkpoint

PR #79 branch:
**audit/full-project-hardening-20260923**

Exact accepted head:
**00ee7a6ed7ac1b5a93c43246d70c252e135acec0**

Status:
**DEVICE ACCEPTED**

Issue #85 is complete/closed.

## Historical accepted checkpoints

### Gen I-III shared editor

Application SHA:
**996e6aa40c96e4408282f3d55476dae8e64968b2**

NRO SHA-256:
**ac3f6bd03d2a6733aee509729b81b6636cabe836095715c7b575b5dc84c8076c**

Status:
**DEVICE ACCEPTED**

### Provider-neutral Save Instances

Application SHA:
**00ee7a6ed7ac1b5a93c43246d70c252e135acec0**

Tree SHA:
**b0832910df44898114a413191ebad3228bded8af**

NRO SHA-256:
**5fad07002c5074ffb3d1d2bdd91275ef29fbdf199f7db263f39c5a3a9f86ca25**

Artifact ID:
**10956604914**

Status:
**DEVICE ACCEPTED**

## Supported game state

| Area | Current state |
|---|---|
| Gen I R/B/Y | Read + staged Pokémon editing |
| Gen II G/S/C | Read + staged Pokémon editing |
| Gen III R/S/E/FRLG | Read + staged Pokémon editing |
| Gen IV D/P/Pt/HG/SS | Read foundation + Party/Box staged View/Edit in hardware-retest draft |
| Classic Inventory | Staged editing where supported |
| Save Instances | Device accepted for Gen I-IV |
| Gen IV Create | Disabled |
| Gen IV Party Pokémon editing | Staged View/Edit implemented on current draft |
| Gen IV Inventory editing | Disabled |
| Cross-game True Move | Disabled |
| Explicit external-source Inject Save | Planned in Issue #89; disabled in current builds |
| Master Vault | Not started |
| Gen V | Not started |

## G4-03 implementation state

The active Gen IV tranche reuses the existing shared editor UI rather than creating a generation-specific shell.

Implemented on PR #87:

- exact Gen IV editor capabilities for Diamond/Pearl, Platinum and HeartGold/SoulSilver;
- mutable PK4 layer built around existing Encryption4;
- boxed PK4 staged mutation;
- full 0xEC Party PK4 staged mutation;
- Storage CRC refresh for boxed edits;
- General-block CRC refresh for Party edits;
- Party level/HP/battle-stat coherence after stat-affecting edits;
- strict save reparse and exact record verification;
- rollback on failed validation;
- shared View/Edit surface;
- inline Gender toggle;
- direct Shiny toggle;
- Nature picker;
- legal Ability picker with ability names;
- numeric Level/EXP/Friendship/IV/EV editing;
- shared move editor behavior;
- joystick/D-pad parity and existing exit/discard rules;
- constrained PID-linked Nature/Gender/Shiny/Ability edits;
- repeated PID-linked edit stability;
- source-save immutability.

Still intentionally deferred:

- Gen IV Create;
- Inventory editing;
- source writeback;
- cross-game True Move.

## Current exact-head verification

The previous automated-green candidate:

**5e79e9f8e038f2f940070edd92a70b692df7a7b5**

was tested on physical Switch hardware and **REJECTED**.

Observed failure:
- the Platinum DraStic save contained one Party Pokémon and zero boxed Pokémon;
- the first milestone only exposed boxed Edit, so there was no editable target;
- Party View fell through to the older generic READ ONLY details surface instead of the shared editor.

Current corrected application head:

**6e9f54f2a9a0cea79f14942912ba76aaca3fa0ed**

Tree observed by the focused candidate gate:

**42b6f13e299f1ec5306f75844ae3f90718676a30**

Confirmed PASS on this corrected head:
- Gen IV exact-format provider;
- mutable PK4 core;
- staged Party + Box editor regression;
- shared hardware-surface routing contract;
- strict Gen IV read-only foundation;
- focused ASan/UBSan;
- real devkitA64 shared UI compile;
- Gen I/II Packed Move #225;
- Gen I/II Packed Multi-Move #224.

Still running at the latest check:
- PokeBank NX Host Tests #1449;
- Gen IV full devkitA64 compile/link/package in Candidate Gate #21.

**DEVICE ACCEPTANCE: NOT YET**

The next NRO must be built from the corrected exact head and physically retested with the same party-only Platinum save.

### Save-session backup / injection direction

Issue #89 tracks the future cross-generation model:

```text
Current emulator save
        ↓
automatic immutable PokeBank backup
        ↓
PokeBank working/staged copy
        ↓
edit + validate
        ↓
explicit Inject Save
        ↓
validated edited copy replaces emulator source
while backup remains available
```

Current builds do **not** inject. Ordinary editing still leaves emulator source bytes untouched.

## Provider support

### Gen I-III

- RetroArch
- configured mGBA battery-save directory only
- Tico roots:
  - sdmc:/tico/saves/gb
  - sdmc:/tico/saves/gbc
  - sdmc:/tico/saves/gba
- Manual where supported

No Tico DS root is authorized.

### Gen IV

- RetroArch
- DraStic cartridge backups
- melonDS
- Manual / remembered sources

DraStic `.dsv` cartridge backups are supported read only. DraStic `.dss` savestates remain unsupported.

## Permanent safety invariants

- Ordinary editing never mutates external emulator source bytes.
- Installed-game live writes remain disabled.
- Emulator Inject Save remains disabled until Issue #89 is implemented and validated.
- Any future injection must first preserve and verify an immutable backup.
- Unknown / ambiguous saves fail closed.
- Remembered sources may not silently substitute another physical file.
- A-button is never destructive by itself.
- Cross-game True Move remains locked.

## Next gate

Finish Host Tests #1449 and Gen IV Candidate Gate #21 for exact head 6e9f54f2.

If green:

1. freeze exact application/tree identity;
2. retrieve and independently hash the Actions-built NRO;
3. retest the same Platinum DraStic save with one Party Pokémon and zero boxed Pokémon;
4. verify Party A → Actions → shared View/Edit;
5. verify B-spam cannot discard staged save-session changes;
6. keep the external .dsv unchanged in this build;
7. only after hardware acceptance proceed to Create or Issue #89 injection implementation.

## Canonical project documents

- README.md — public project front page
- CURRENT_STATUS.md — exact engineering checkpoint
- PROJECT_STATUS.md — high-level project state
- docs/V1_ROADMAP.md — release roadmap
- docs/GAME_SUPPORT_MATRIX.md — support matrix
- docs/audit/ISSUE85-SAVE-INSTANCES.md — provider-neutral source architecture audit
- docs/REFERENCE_INDEX.md — upstream/reference index
