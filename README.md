<p align="center">
<img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is an offline Pokémon storage, collection, transfer, and save-management project for Nintendo Switch homebrew.

The project started from the PKSE codebase, but the goal is broader: one controller-first Switch application for browsing supported Pokémon saves, keeping a permanent local Master Vault, organizing Pokémon into named Banks, tracking provenance, building Living Dex collections, and safely moving compatible Pokémon between games and generations.

> **Alpha warning:** direct live installed-game save writing is intentionally hard disabled. Current development builds are read-only against installed game saves unless an explicitly documented future adapter says otherwise.

---

## Current status — September 2026

Development branch:

```text
feature/pokebank-playable
```

PKSE remains **upstream only**. PokeBank NX changes must never be pushed to the original PKSE repository.

### First physical Switch milestone — COMPLETE

The first exact recorded PokeBank NX `.nro` has been physically run on Nintendo Switch hardware.

```text
Application source:
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a

Artifact:
PokeBank-NX-UI-Theme-3be4de6.nro

Size:
9,707,957 bytes

SHA-256:
df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a

Result:
DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

Physical results:

| Area | Result |
|---|---|
| Boot | **PASS** |
| D-pad navigation | **PASS** |
| Left Stick + held repeat | **FAIL** |
| A → Pokémon Action Sheet | **PASS** |
| B / Cancel | **PASS** |
| L / R | **PASS** |
| ZL / ZR | **PASS** |
| `+` Options / More | **PASS** |
| `-` Help / Controls | **PASS** |
| OLED Black | **PASS** |
| Dark | **PASS** |
| Light | **PASS** |
| Theme persistence | **PASS** |
| Party | **PASS** |
| Boxes | **PASS** |
| Storage | **PASS** |
| Crashes | **NONE OBSERVED** |
| Visible PokeBank NX identity | **FAIL / INCOMPLETE** |

Permanent report: [`docs/DEVICE_TEST_REPORT_2026-09-01.md`](docs/DEVICE_TEST_REPORT_2026-09-01.md).

Issue **#8 is complete** because the first exact physical test happened. The failures were converted into follow-up work rather than being hidden.

---

## Current Session 2.5 application checkpoint

The follow-up visible-shell/physical-input session successfully pushed this application source before running out of usage:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

GitHub host CI passed on this exact source.

Reported implementation includes:

```text
real libnx Left Stick position handling
analog deadzone + hysteresis
held analog navigation repeat
Select Game / Backups / Party / Boxes / Storage analog integration
visible PokeBank NX header/app identity
PokeBank NX chrome/cards
shared Options / Help visual treatment
matching Action Sheet styling
PokeBank NX NRO/window identity
```

The coding session ended before the clean **exact-source** rebuild, final `.nro` hash/preservation, and second physical handoff.

Therefore:

```text
APPLICATION SOURCE: PUBLISHED
GITHUB HOST CI: PASS
EXACT REPLACEMENT NRO: PENDING PACKAGE
SECOND DEVICE TEST: NOT DONE
```

Next prompt:

[`docs/PROMPT_SESSION2_5_FINISH.md`](docs/PROMPT_SESSION2_5_FINISH.md)

The replacement device artifact must be built from exact application source `361c6f55...` unless a real source regression requires a new application-source checkpoint.

---

## Verification vocabulary

PokeBank NX deliberately separates:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

For physical builds, the result is also recorded:

```text
DEVICE TESTED — PASS
DEVICE TESTED — PARTIAL PASS
DEVICE TESTED — FAIL
```

A build being physically tested does not mean every tested feature passed.

---

# Product model

## Game saves

Installed game saves are currently treated as **read-only sources**.

That lets PokeBank NX safely browse and import Pokémon while parsers, the Master Vault, conversion engines, staged-save validation, and per-game write adapters are being proven.

## Master Vault

The **Master Vault** is the permanent game-independent storage/provenance layer.

Core principles:

```text
immutable raw Pokemon payloads
stable Vault IDs
SHA-256 integrity
origin/source provenance
active/current location separate from origin
parent/derived lineage
transaction journal + crash recovery
named Banks as organization/reference layers
rebuildable search/metadata indexes
```

Specification: [`docs/MASTER_VAULT_SPEC.md`](docs/MASTER_VAULT_SPEC.md).

## Banks

Banks organize Vault entities without blindly duplicating raw payloads.

Examples:

```text
Living Dex
Shiny Living Dex
Favorites
Events
Competitive
Gen III
Scarlet / Violet
```

A Pokémon can appear in organizational views without silently becoming several independent playable clones.

---

# Copy, Move, and Clone

The long-term product semantics are explicit:

```text
COPY
= intentionally keep the source active and create a destination representation

MOVE
= relocate the active Pokemon; the source stops being active only after the destination is safely created, written, read back, and verified

CLONE
= deliberately create a duplicate with clone provenance
```

Desired end-state:

```text
Game A  ->  Master Vault / Bank  ->  Game B
```

A true Move should behave like a real transfer: if Pikachu is moved from FireRed to the Vault, it is no longer active in FireRed; if later moved from the Vault into Violet, it becomes active in Violet and is no longer active in the Vault/Bank placement.

For safety, PokeBank NX may retain immutable archival/provenance/rollback records behind the scenes. Those records are historical evidence, **not another active playable Pikachu**.

### Current alpha behavior

True Move is **not enabled now**.

Current installed game saves stay read-only, so early Game → Vault behavior uses safe Copy/import semantics.

True Move is tracked by issue **#20** and is blocked on independently validated per-game write adapters.

Specification: [`docs/TRANSFER_MODEL.md`](docs/TRANSFER_MODEL.md).

---

# Controller-first UI

## Pokémon Action Sheet

Issue #2 is complete.

Pressing **A** on an occupied Pokémon opens a deliberate Action Sheet:

```text
View Pokémon
Add to Master Vault
Add to Bank…
Transfer to Game…
Edit
Clone
Make Shiny
Legality & Provenance
Cancel
```

Opening/navigating the sheet, pressing B, or selecting Cancel performs no mutation. The Action Sheet behavior passed the first physical Switch test.

## Controller contract

| Control | Behavior |
|---|---|
| D-pad | precise navigation |
| Left Stick | navigation + held repeat |
| A | Select / Open; Pokémon → Action Sheet |
| B | Back / Cancel |
| X | Filter / Search / context |
| Y | Sort / View / secondary context |
| L / R | previous/next box, Pokémon, or nearby tab |
| ZL / ZR | larger jumps / major navigation |
| `+` | contextual More / Options |
| `-` | Help / Controls / Screen Info |
| Right Stick | optional fast-scroll / secondary pane |

Full controller contract: [`docs/CONTROLS.md`](docs/CONTROLS.md).

## Themes

```text
OLED Black
Dark
Light
```

All three themes and persistence passed the first physical Switch test.

Visual direction: [`docs/UI_STYLE_GUIDE.md`](docs/UI_STYLE_GUIDE.md).

---

# NRO quality / reliability roadmap

PokeBank NX also tracks cross-cutting native-app improvements so core features do not outgrow the shell around them.

Detailed roadmap: [`docs/NRO_QUALITY_ROADMAP.md`](docs/NRO_QUALITY_ROADMAP.md)  
Tracking issue: **#21**.

Planned items include:

```text
Diagnostics screen + privacy-safe diagnostic export
Applet/constrained-memory detection and graceful cache reduction
real startup stages instead of a fake loading timer
privacy-safe error/crash logging
clear READ ONLY / VAULT / STAGED / active-location badges
Vault recovery UI + storage-health verification
search/filter
Box Quick Jump
Continue / Recently Viewed / Recently Added
Favorites
text-size options
Reduced Motion
color-independent focus
optional original UI sounds
optional restrained rumble
virtualized large Vault/Pokedex grids
bounded artwork/sprite caches
lazy raw-entity loading
intentional missing-resource fallbacks
rebuildable metadata/search indexes
safe-operation progress and rollback UX
```

These are implemented incrementally so they do not block the core Pokémon engines.

Recommended balance after the second UI/device milestone:

```text
70% core functionality
20% hardware validation / bug fixing
10% polish / infrastructure
```

---

# Safety

**LIVE INSTALLED-GAME SAVE WRITING IS HARD DISABLED.**

Current protections include:

- safe/backup destination posture;
- generic save API cannot request title injection;
- low-level restore rejects live-title writes before mounting save data;
- legacy `injectToGame=1` is disabled/rewritten;
- Action Sheet/controller navigation cannot bypass the hard lock.

Future live writes are **per adapter**, not one global switch.

Before a game can participate in true Move, its adapter must prove the relevant pipeline:

```text
read
backup + fingerprint
stage cloned save
apply intended mutation
repair checksums/containers
reparse + validate
write
read back
verify exact intended change
retain rollback/recovery state
```

If verification fails, abort/rollback rather than guessing.

See [`docs/SAVE_SAFETY.md`](docs/SAVE_SAFETY.md).

---

# Target game identities

PokeBank NX currently tracks 23 stable release/platform identities.

### Game Boy / Game Boy Color

- Red
- Blue
- Yellow
- Gold
- Silver
- Crystal

### Game Boy Advance

- Ruby
- Sapphire
- Emerald
- FireRed
- LeafGreen

### Nintendo Switch

- FireRed
- LeafGreen
- Let's Go, Pikachu!
- Let's Go, Eevee!
- Sword
- Shield
- Brilliant Diamond
- Shining Pearl
- Legends: Arceus
- Scarlet
- Violet
- Legends: Z-A

GBA FireRed/LeafGreen and Switch FireRed/LeafGreen are intentionally separate stable identities.

Detection, parsing, conversion, staging, live writing, and physical validation are separate capabilities.

See [`docs/GAME_SUPPORT_MATRIX.md`](docs/GAME_SUPPORT_MATRIX.md).

---

# Roadmap

## Immediate

```text
finish/package exact 361c6f55 replacement .nro
        ↓
second physical Switch test
        ↓
fix blocking device regression if needed
```

## Next deep engineering

Issue #4 — PKSM-Core Gen III spike:

```text
PK3
Sav3
FireRed / LeafGreen GBA
read-only parsing
party / box extraction
active save-slot / sectors
checksum / encryption behavior
untouched round-trip strategy
integration decision
```

Ready MAX prompt: [`docs/PROMPT_SESSION3_PKSM_CORE.md`](docs/PROMPT_SESSION3_PKSM_CORE.md).

## Core product order

```text
#3  Master Vault v1 + named Banks
#9  Professional Summary + provenance
#6  RetroArch discovery + read-only Gen I-III adapters
#11 Modern Switch adapter validation
#5  PKHeX Oracle
#7  Vault-driven Pokedex / Living Dex
#10 Conversion / transfer without live writes
#20 True Move after individually proven safe-write adapters
```

Supporting:

```text
#15 persistent .nro artifacts
#16 final branding/startup/icon/NACP
#17 golden Pokemon/save fixtures
#21 NRO diagnostics/reliability/performance/QoL
```

Detailed dependency map: [`docs/PROJECT_MAP.md`](docs/PROJECT_MAP.md).

---

# Upstream / reference stack

PokeBank NX checks mature Pokémon research before rebuilding difficult infrastructure from scratch.

| Project | Role |
|---|---|
| [PKSE](https://github.com/kiasta/PKSE) | Original native Switch foundation; **upstream only** |
| [PKSM-Core](https://github.com/FlagBrew/PKSM-Core) | Native C++ historical Pokémon/save engine candidate |
| [PKHeX](https://github.com/kwsch/PKHeX) | Primary correctness/reference implementation |
| [PKHeX-Plugins / Auto Legality](https://github.com/santacrab2/PKHeX-Plugins) | Encounter-driven legality/generation reference |
| [pkHouse](https://github.com/Insektaure/pkHouse) | Modern Switch save-behavior reference |
| [pkDex](https://github.com/Insektaure/pkDex) | Pokédex UX/data-organization reference |
| [PKForge](https://github.com/sofianeelhor/PKForge) | Vault/provenance/transaction architecture reference |

Pinned revisions, licenses, and reuse classifications: [`docs/UPSTREAM_AUDIT.md`](docs/UPSTREAM_AUDIT.md).

---

# Development start point

Read in this order:

1. [`PROJECT_STATUS.md`](PROJECT_STATUS.md)
2. [`docs/NEXT_SESSION_PLAN.md`](docs/NEXT_SESSION_PLAN.md)
3. [`docs/BUILD_RECORD.md`](docs/BUILD_RECORD.md)
4. [`docs/DEVICE_TEST_REPORT_2026-09-01.md`](docs/DEVICE_TEST_REPORT_2026-09-01.md)
5. [`docs/PROMPT_SESSION2_5_FINISH.md`](docs/PROMPT_SESSION2_5_FINISH.md)
6. [`docs/SESSION_RUNBOOK.md`](docs/SESSION_RUNBOOK.md)
7. [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
8. [`docs/SAVE_SAFETY.md`](docs/SAVE_SAFETY.md)
9. the current issue-specific spec/prompt

---

## License / attribution

PokeBank NX is distributed under the repository's **GNU Affero General Public License v3**. See [`LICENSE`](LICENSE).

The project began from PKSE and preserves required upstream/license attribution. External projects referenced during development retain their own licenses and notices; reuse classifications are tracked in `docs/UPSTREAM_AUDIT.md`.

---

## Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks are property of their respective owners.

Unofficial fan-made homebrew project • © 2026 PokeBank NX Contributors
