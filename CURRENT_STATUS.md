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
**5e79e9f8e038f2f940070edd92a70b692df7a7b5**

Tracking issue:
**#86 — G4-03 Gen IV shared staged Pokémon editor**

Current milestone:
**boxed PK4 View/Edit through the existing shared Pokémon editor**

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
| Gen IV D/P/Pt/HG/SS | Read foundation + boxed staged View/Edit in active draft |
| Classic Inventory | Staged editing where supported |
| Save Instances | Device accepted for Gen I-IV |
| Gen IV Create | Disabled |
| Gen IV Party mutation | Disabled |
| Gen IV Inventory editing | Disabled |
| Cross-game True Move | Disabled |
| Live external-source writes | Disabled |
| Master Vault | Not started |
| Gen V | Not started |

## G4-03 implementation state

The active Gen IV tranche reuses the existing shared editor UI rather than creating a generation-specific shell.

Implemented on PR #87:

- exact Gen IV editor capabilities for Diamond/Pearl, Platinum and HeartGold/SoulSilver;
- mutable PK4 layer built around existing Encryption4;
- boxed PK4 staged mutation;
- Storage CRC refresh and strict save reparse;
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
- Party mutation;
- Inventory editing;
- source writeback;
- cross-game True Move.

## Current exact-head verification

At application head:
**5e79e9f8e038f2f940070edd92a70b692df7a7b5**

Confirmed PASS:

- Gen IV focused PK4/staged-editor host gates;
- Gen IV focused ASan/UBSan gates;
- Gen IV shared hardware-surface contract;
- real devkitA64 shared UI compile;
- Gen I/II Packed Move regression;
- Gen I/II Packed Multi-Move regression.

Gen IV candidate workflow #11 is now fully green, including the exact devkitA64 compile/link/package job.

Exact candidate identity:

- Application SHA: **5e79e9f8e038f2f940070edd92a70b692df7a7b5**
- Tree SHA: **c636127b2e52792371e8e55abac8fe1d5cfaf99f**
- Artifact: **Gen4-SharedEditor-Candidate-5e79e9f8**
- Artifact ID: **10960797638**
- NRO: **PokeBank-NX-Gen4-SharedEditor-5e79e9f8.nro**
- NRO SHA-256: **8011ef64d269ae42331306c13692d60494fb7ea936619d61e1abc7d7906829d5**

The downloaded artifact manifest and actual NRO hash were independently verified and match.

Still running at the latest check:

- PokeBank NX Host Tests #1427.

Therefore:

**GEN IV CANDIDATE WORKFLOW: PASS**

**BROAD HOST SUITE: IN PROGRESS**

**DEVICE ACCEPTANCE: NOT YET**

Do not call PR #87 device accepted until an exact Actions-built NRO finishes all required gates and the owner passes it on physical Switch hardware.

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

- Original external saves are immutable.
- Installed-game live writes are disabled.
- RetroArch live writes are disabled.
- mGBA live writes are disabled.
- Tico live writes are disabled.
- DraStic live writes are disabled.
- melonDS live writes are disabled.
- Unknown / ambiguous saves fail closed.
- Remembered sources may not silently substitute another physical file.
- A-button is never destructive by itself.
- Cross-game True Move remains locked.

## Next gate

Finish broad Host Tests #1427, then hardware-test the exact frozen Gen IV candidate NRO.

Next sequence:

1. confirm Host Tests #1427 PASS;
2. owner hardware-test boxed Gen IV View/Edit using the exact NRO above;
3. fix only hardware-observed regressions;
4. only after Edit is device accepted, consider Gen IV Create.

## Canonical project documents

- README.md — public project front page
- CURRENT_STATUS.md — exact engineering checkpoint
- PROJECT_STATUS.md — high-level project state
- docs/V1_ROADMAP.md — release roadmap
- docs/GAME_SUPPORT_MATRIX.md — support matrix
- docs/audit/ISSUE85-SAVE-INSTANCES.md — provider-neutral source architecture audit
- docs/REFERENCE_INDEX.md — upstream/reference index
