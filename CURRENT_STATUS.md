# PokeBank NX — Current Verified Engineering State

Last updated: **2026-09-28**

GitHub is authoritative. Re-fetch live heads before new work and preserve newer commits.

## Active development line

### PR #90 — combined Gen IV editor + v1 polish hardware candidate

State:
**OPEN / DRAFT / NOT MERGED**

Branch:
**integration/gen4-polish-hardware-20260928**

Base:
**audit/full-project-hardening-20260923**

Current exact head:
**84dae170deb2756d9b80aec32bf8ad512ce17c31**

Tree:
**7733004cd7b4aebffc3ba687ad0f7df22eea8aa8**

Underlying work:
- PR #87 — Gen IV shared staged Pokémon editor
- frozen QoL snapshot from PR #88 through **86002169d3fec073fb8454ee794c85e07b5a3d9a**
- independent-audit remediations

Current milestone:
**one combined Party + Box Gen IV hardware retest NRO with current integrated polish**

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

The first Gen IV editor candidate at:

**5e79e9f8e038f2f940070edd92a70b692df7a7b5**

was physically tested and **REJECTED** because a party-only Platinum save could not reach Edit and Party View fell through to the legacy read-only presentation.

The current combined retest candidate is:

- Application SHA: **84dae170deb2756d9b80aec32bf8ad512ce17c31**
- Tree SHA: **7733004cd7b4aebffc3ba687ad0f7df22eea8aa8**
- Artifact: **Gen4-SharedEditor-Candidate-84dae170**
- Artifact ID: **10986964856**
- NRO: **PokeBank-NX-Gen4-SharedEditor-84dae170.nro**
- NRO SHA-256: **313b6ed5f209b0fba797deee010d73b753d25294dd3d1c39f279c61df13fe6de**
- Artifact ZIP SHA-256: **50c2fd1051f0b35ab1eadb5dec3d5f5685389d5b61e45154d5ebd7bc419c93e1**

This candidate includes:
- Party + Box shared View/Edit;
- the party-only Platinum reachability fix;
- staged dirty B/+ exit protection;
- independent-audit fixes for immutable-source dirty modal lifetime, Shedinja HP, quarantined-record refresh and Gen IV sixth-stat focus;
- permanent audit regressions, including the DP/Pt/HGSS mixed General/Storage partition mutation-footprint matrix;
- integrated v1 polish through PR #88 snapshot **86002169d3fec073fb8454ee794c85e07b5a3d9a**.

Exact-head gates:
- Host Tests #1472 — **PASS**
- Host ASan/UBSan — **PASS**
- Gen IV Candidate Gate #36 — **PASS**
- v1 Polish Native #20 — **PASS**
- Packed Move #240 — **PASS**
- Packed Multi-Move #239 — **PASS**

Artifact identity and NRO hash were independently verified.

**AUTOMATED GATES: PASS**

**DEVICE ACCEPTANCE: PENDING OWNER PHYSICAL SWITCH TEST**

Later PR #88 QoL commits are intentionally deferred to the next integration cycle so this candidate remains reproducible.

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

Owner physical Switch test of the exact PR #90 NRO above.

Use the same Platinum DraStic save that previously showed one Party Pokémon and zero boxed Pokémon.

Verify:

1. Party → Piplup → A opens Generation IV Actions.
2. View uses the shared Pokémon editor presentation.
3. Edit is reachable for the Party Pokémon.
4. Stage and Keep a supported edit.
5. Visible values refresh correctly after Keep.
6. Repeated B cannot silently discard staged changes.
7. The dirty exit dialog survives a neutral frame and remains actionable.
8. DraStic source .dsv remains unchanged in this build.
9. Integrated Games/Settings/controller polish is present.
10. Themes remain readable and NRO metadata identifies PokeBank NX correctly.

Do not mark device accepted until the owner explicitly passes this exact NRO.

## Canonical project documents

- README.md — public project front page
- CURRENT_STATUS.md — exact engineering checkpoint
- PROJECT_STATUS.md — high-level project state
- docs/V1_ROADMAP.md — release roadmap
- docs/GAME_SUPPORT_MATRIX.md — support matrix
- docs/audit/ISSUE85-SAVE-INSTANCES.md — provider-neutral source architecture audit
- docs/REFERENCE_INDEX.md — upstream/reference index
