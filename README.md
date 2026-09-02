<p align="center">
<img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is an offline Pokémon storage, collection, transfer, and save-management project for Nintendo Switch homebrew.

The project started from the PKSE codebase, but the goal is broader: one controller-first Switch application for browsing supported Pokémon saves, keeping a permanent local Master Vault, organizing Pokémon into named Banks, tracking provenance, building Living Dex collections, and safely moving compatible Pokémon between games and generations.

> **Alpha warning:** live installed-game save writing is not an approved current feature. Installed game sources stay read-only until an individual adapter passes explicit backup, staged-write, validation, readback, rollback, and physical-device safety gates.

---

# Current status — September 2, 2026

Development branch:

```text
feature/pokebank-playable
```

Writable repository:

```text
GlitchedZeus/PokeBank-NX
```

PKSE remains **upstream only**. PokeBank NX changes must never be pushed to the original PKSE repository.

## First physical Switch milestone — COMPLETE

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

Short physical matrix:

| Area | Result |
|---|---|
| Boot | **PASS** |
| D-pad navigation | **PASS** |
| Left Stick | **FAIL** |
| A → Pokémon Action Sheet | **PASS** |
| B / Cancel | **PASS** |
| L / R | **PASS** in exercised context |
| ZL / ZR | **PASS** in exercised context |
| `+` Options / More | **PASS** |
| `-` Help / Controls | **PASS** |
| OLED Black | **PASS** |
| Dark | **PASS** |
| Light | **PASS** |
| Theme persistence | **PASS** |
| Party | **PASS** |
| Boxes | **PASS** |
| Storage | **PASS** |
| Visible PokeBank NX identity | **FAIL / INCOMPLETE** |

First report: [`docs/DEVICE_TEST_REPORT_2026-09-01.md`](docs/DEVICE_TEST_REPORT_2026-09-01.md).

Issue **#8 is complete** because the physical-test milestone happened. A device test can still be a partial pass.

## Extended first-device torture test

The same exact `3be4de6b...` binary received a longer hardware pass.

Extended report: [`docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md`](docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md).

New evidence:

```text
5 minutes idle                     PASS
10 minutes normal browsing         PASS
5 repeated relaunches              PASS
Action Sheet heavy open/close      PASS
held D-pad                         PASS
Left Stick single taps             FAIL — no input/action
Left Stick held                    FAIL — no input/action
Left Stick diagonal                FAIL — no input/action
one older Legends Arceus save      REPRODUCIBLE CRASH
```

The extended test also proved inherited PKSE mutation controls are still user-reachable in game-save views:

```text
Release
Create Pokémon
Menu / Move / Multi
editable Pokémon view
apply/save-style change flow
```

Inherited/app `Storage` is writable/persistent on hardware.

Important evidence boundary:

```text
LIVE INSTALLED SAVE WRITE: NOT PROVEN
USER-REACHABLE MUTATION UI: PROVEN
APP STORAGE PERSISTENCE: PROVEN
```

The final external verification of launching the original Pokémon title and proving whether its installed save changed was not completed, so the project does **not** claim a confirmed live-save write regression. The mutation paths are still a blocker because current-alpha installed-game browsing is supposed to be read-only at the user-facing level.

Tracked by **#23**. The old Legends Arceus crash is tracked by **#24**.

---

# Current application checkpoint

Session 2.5 published:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

GitHub host CI passed on that source.

Reported implementation includes:

```text
real libnx Left Stick position handling
analog deadzone + hysteresis
single-tap + held analog navigation
Select Game / Backups / Party / Boxes / Storage analog integration
visible PokeBank NX header/app identity
PokeBank NX chrome/cards
shared Options / Help visual treatment
matching Action Sheet styling
PokeBank NX NRO/window identity
```

This source has **not** been physically tested.

The extended first-device findings changed the handoff plan. We no longer simply package `361c6f55...`: the next coding session must preserve that UI/input work while first auditing the mutation paths and hardening the PLA crash path. If application code changes, the replacement `.nro` must be tied to a **new exact application-source SHA**.

Current HIGH prompt:

[`docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md`](docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md)

---

# Verification vocabulary

PokeBank NX deliberately separates:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

Physical results are recorded as:

```text
DEVICE TESTED — PASS
DEVICE TESTED — PARTIAL PASS
DEVICE TESTED — FAIL
```

A source commit, CI pass, remembered filename, or buildable `.nro` is never automatically device-tested.

---

# Current blockers before device test #2

## #23 — inherited mutation UI safety audit

Trace these paths to their final persistence target:

```text
Release
Create Pokémon
Move / Multi
Edit / apply / save
unsaved-changes flow
legacy/app Storage move/import
reachable Save / Commit / Restore / Inject paths
```

Classify each as:

```text
LIVE INSTALLED SAVE WRITE
BACKUP/STAGED SAVE WRITE
APP-OWNED STORAGE WRITE
IN-MEMORY ONLY
DISABLED / UNREACHABLE
UNKNOWN — NEEDS BLOCKING
```

Unsafe installed-source mutations must not remain reachable merely because a lower-level guard may reject the final write.

## #24 — old / malformed Legends Arceus crash

Valid supported PLA saves should open. Old, malformed, unsupported, truncated, or unexpected saves must return a useful read-only error and safe navigation instead of crashing or being silently repaired/written.

## #19 — Left Stick navigation

The first hardware build receives **no Left Stick navigation input at all**. `361c6f55...` contains the reported real-libnx fix, but physical single-tap, held-repeat and diagonal testing is still required.

## #13 / #16 — visible PokeBank NX identity

The replacement build must preserve the PokeBank NX shell/chrome/Options/Help/Action Sheet styling and remove obvious normal-path PKSE product identity.

---

# Product model

## Installed game saves

Installed game saves are currently **read-only sources**.

PokeBank NX can browse/import from them while the parsers, Vault, conversion engines, staged saves and per-game write adapters are being proven.

## Legacy/app Storage

The inherited Storage area is already writable/persistent. It is **not automatically the future Master Vault**.

Issue **#27** tracks its exact persistence behavior and whether it becomes a migration source, compatibility area, or is retired after Vault v1.

## Master Vault

The planned Master Vault is the permanent game-independent storage/provenance layer:

```text
immutable raw Pokémon payloads
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

One entity may be referenced in multiple organizational views without becoming several independent playable clones.

---

# Copy, Move, and Clone

Long-term semantics are explicit:

```text
COPY
= intentionally keep the source active and create a destination representation

MOVE
= relocate the active Pokémon; source stops being active only after destination success is verified

CLONE
= deliberately create a duplicate with clone provenance
```

Desired end-state:

```text
Game A  →  Master Vault / Bank  →  Game B
```

A real Move should feel like Pokémon Bank/HOME relocation. If Pikachu is moved from FireRed into the Vault, Pikachu is no longer active in FireRed. If it is later moved from the Vault into Violet, Violet becomes the active location.

PokeBank NX may retain immutable archival/provenance/rollback evidence behind the scenes, but historical records are **not another active playable Pikachu**.

True Move is **later, not now**. Issue **#20** blocks it on independently approved per-game write adapters.

Specification: [`docs/TRANSFER_MODEL.md`](docs/TRANSFER_MODEL.md).

---

# Controller-first UI

## Pokémon Action Sheet — COMPLETE foundation

Press **A** on an occupied Pokémon:

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

Opening/navigating/canceling the sheet remained stable during heavy hardware repetition. Installed-source `Edit` behavior is now covered by #23 because the inherited editable view is still reachable.

## Intended controller contract

| Control | Intended behavior |
|---|---|
| D-pad | precise navigation |
| Left Stick | navigation + held repeat |
| A | Select/Open; Pokémon → Action Sheet |
| B | Back/Cancel |
| X | Filter/Search/context |
| Y | Sort/View/secondary context |
| L/R | previous/next box, Pokémon, or nearby tab |
| ZL/ZR | larger jumps / major navigation |
| `+` | contextual More/Options |
| `-` | Help/Controls/Screen Info |
| Right Stick | optional fast scroll / secondary pane / later Summary model rotation |

Extended hardware testing exposed inherited differences such as L/R account switching, inactive ZL/ZR in one context, X Dex-sort and Y Menu/Move/Multi. Issue **#26** tracks normalization.

Full contract: [`docs/CONTROLS.md`](docs/CONTROLS.md).

## Summary / Pokémon visuals

Issue **#9** tracks professional Summary/provenance. Issue **#25** tracks adding an actual Pokémon visual to the view. High-quality sprites/artwork are sufficient; optional 3D model rotation is only a later nice-to-have if technically and legally practical.

## Themes

```text
OLED Black
Dark
Light
```

All three and persistence passed physical hardware testing.

Visual direction: [`docs/UI_STYLE_GUIDE.md`](docs/UI_STYLE_GUIDE.md).

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

GBA FireRed/LeafGreen and Switch FireRed/LeafGreen are intentionally separate identities.

Detection, read parsing, conversion, staging, live writing and physical validation are separate capabilities.

See [`docs/GAME_SUPPORT_MATRIX.md`](docs/GAME_SUPPORT_MATRIX.md).

---

# Full roadmap to v1.0

Canonical detailed roadmap: [`docs/V1_ROADMAP.md`](docs/V1_ROADMAP.md)  
Master release tracker: **#29 — Track PokeBank NX v1.0 roadmap and release gates**.

The checklist below is intentionally visible in the README so the repository homepage answers both **what have we done?** and **what is left?**

## Foundation / playable shell

- [x] Repository recovery + Git/upstream safety discipline
- [x] Stable 23-game identity registry
- [x] Low-level live installed-save write hard lock
- [x] Controller-first Pokémon Action Sheet (#2)
- [x] HOME-style controller/theme foundation
- [x] First exact physical Switch `.nro` test (#8)
- [x] Extended first-build torture test/report
- [ ] **IN PROGRESS** — visible PokeBank NX shell physical acceptance (#13/#16)
- [ ] **IN PROGRESS** — full Left Stick physical navigation (#19)
- [ ] **IN PROGRESS** — inherited mutation safety audit (#23)
- [ ] **IN PROGRESS** — old/malformed PLA crash hardening (#24)
- [ ] Exact second `.nro` packaged, hashed, preserved and physically tested
- [ ] Final controller semantics/hints normalized (#26)

## Historical Pokémon/save engine

- [ ] PKSM-Core Gen III `PK3` / `Sav3` spike and integration decision (#4)
- [ ] FireRed / LeafGreen GBA production read adapter
- [ ] Ruby / Sapphire / Emerald read adapters
- [ ] Gen I Red/Blue/Yellow read adapters
- [ ] Gen II Gold/Silver/Crystal read adapters
- [ ] RetroArch/legacy save discovery + manual fallback (#6)
- [ ] malformed/truncated/checksum rejection across historical adapters

## Validation / regression tooling

- [ ] Golden Pokémon/save fixture corpus (#17)
- [ ] malformed/corrupt fixture variants
- [ ] PKHeX host Oracle: inspect / legality / convert (#5)
- [ ] versioned machine-readable cross-engine comparisons
- [ ] untouched-save round-trip expectations where the format permits them

## Master Vault / Banks

- [ ] Master Vault v1 immutable entities (#3)
- [ ] stable Vault IDs + SHA-256
- [ ] origin + active-location + history separation
- [ ] parent/derived provenance
- [ ] transaction journal + crash recovery
- [ ] rebuildable metadata/search indexes
- [ ] named Banks over Vault IDs (#3)
- [ ] legacy writable Storage classification/migration (#27)
- [ ] optional future PKSMBANK importer if useful

## Collection / Summary / quality-of-life

- [ ] professional Summary + provenance (#9)
- [ ] Pokémon sprite/artwork/render support (#25)
- [ ] search/filter
- [ ] Box Quick Jump
- [ ] Favorites
- [ ] Recently Viewed / Recently Added / Continue
- [ ] Vault-driven Pokédex (#7)
- [ ] Living Dex
- [ ] Shiny Living Dex
- [ ] meaningful forms/gender distinctions
- [ ] Alpha/Shiny Alpha where applicable

## Modern Switch read support

Validate individually before any writes (#11):

- [ ] Switch FireRed / LeafGreen
- [ ] Let's Go Pikachu / Eevee
- [ ] Sword / Shield
- [ ] Brilliant Diamond / Shining Pearl
- [ ] Legends: Arceus
- [ ] Scarlet / Violet
- [ ] Legends: Z-A
- [ ] unsupported/malformed/version-mismatch saves fail gracefully

## Conversion / transfer engine

- [ ] compatibility result: `SUPPORTED / SUPPORTED_WITH_CHANGES / UNSUPPORTED / UNKNOWN`
- [ ] historical → later Pokémon conversion paths
- [ ] parent/derived provenance on conversion
- [ ] PKHeX comparison where available
- [ ] `.pk*` export
- [ ] staged destination-save representation without live write (#10)
- [ ] dedicated Game ↔ Vault/Bank Transfer Workspace
- [ ] one-Pokémon selection
- [ ] multi-select
- [ ] whole-box operations where safe/supported

## Legality / editing / generation / events

- [ ] legality-aware Vault editing
- [ ] `Make Shiny` obeys shiny locks/rules
- [ ] conservative `UNKNOWN != LEGAL`
- [ ] encounter-driven legal-generation path where supported
- [ ] generated/edited entities retain provenance
- [ ] Mystery Gift / Wondercard workflows
- [ ] historical event database/provenance plan
- [ ] advanced cross-generation compatibility/change preview

## Staged writes / approved live writes / real Move

- [ ] generic backup + fingerprint + staged-save transaction architecture
- [ ] checksum/container repair + reparse validation
- [ ] exact unintended-change comparison
- [ ] durable rollback/recovery state
- [ ] first **individually approved** live-write adapter
- [ ] additional adapters approved one-by-one
- [ ] real `Game → Vault/Bank MOVE` (#20)
- [ ] real `Vault/Bank → Game MOVE` (#20)
- [ ] Copy remains separate
- [ ] Clone remains separate
- [ ] one/multi/whole-box Move uses the same transaction safety model

## Official HOME bridge workflow

- [ ] supported game-save bridge workflow documented/tested where applicable
- [ ] preserve genuine HOME tracker/history
- [ ] no private HOME/Nintendo protocol impersonation
- [ ] no credential extraction or tracker forging
- [ ] no guaranteed-ban-safety claims

## Native `.nro` reliability / diagnostics / accessibility

Tracked by #21 / [`docs/NRO_QUALITY_ROADMAP.md`](docs/NRO_QUALITY_ROADMAP.md):

- [ ] persistent device-test artifact automation (#15)
- [ ] Diagnostics screen + privacy-safe diagnostic export
- [ ] Applet/constrained-memory detection
- [ ] privacy-safe crash/error logs
- [ ] clear READ ONLY / VAULT / STAGED / active-location badges
- [ ] Vault storage-health / Verify Vault / rebuild index
- [ ] interrupted-transaction recovery UI
- [ ] virtualized huge Vault/Dex grids
- [ ] bounded artwork/sprite caches
- [ ] lazy raw-entity loading
- [ ] intentional missing-resource fallbacks
- [ ] text-size option
- [ ] Reduced Motion
- [ ] color-independent focus
- [ ] optional original UI sounds
- [ ] optional restrained rumble

## Final product identity / scale / release

- [ ] final splash/startup stages tied to real work (#16)
- [ ] final title/icon/NACP metadata
- [ ] no normal-path PKSE product identity
- [ ] visible version + source SHA
- [ ] large synthetic Vault performance soak
- [ ] handheld + docked readability pass
- [ ] sleep/resume/controller-reconnect pass
- [ ] malformed-save corpus torture pass
- [ ] release-candidate transaction recovery torture test
- [ ] exact release `.nro`, source SHA, size and SHA-256 preserved
- [ ] advertised support matrix matches actual verified capability
- [ ] release notes distinguish read-only vs approved live-write adapters
- [ ] **v1.0 tag/release**

## Current critical path

```text
#23 mutation safety audit
#24 PLA crash hardening
+ preserve #19 analog fix
+ preserve #13/#16 PokeBank NX UI
        ↓
freeze new exact application source if changed
        ↓
host tests + ASan/UBSan + native build
        ↓
replacement .nro + SHA-256
        ↓
PHYSICAL DEVICE TEST #2
        ↓
#4 PKSM-Core PK3/Sav3
        ↓
Gen III read adapter
        ↓
Master Vault + Banks
        ↓
Summary / Oracle / retro + modern reads
        ↓
conversion / Dex / legality / events
        ↓
staged writes
        ↓
per-game approved live writes
        ↓
true Move
        ↓
release hardening
        ↓
v1.0
```

---

# NRO quality / reliability plan

Detailed backlog: [`docs/NRO_QUALITY_ROADMAP.md`](docs/NRO_QUALITY_ROADMAP.md). Tracking issue: **#21**.

Recommended balance after the second UI/device milestone:

```text
70% core functionality
20% hardware validation / bug fixing
10% polish / infrastructure
```

Quality work is pulled in when its dependent subsystem exists rather than becoming one giant blocking rewrite.

---

# Safety model

**LIVE INSTALLED-GAME SAVE WRITING IS NOT AN APPROVED CURRENT FEATURE.**

Known lower-level protections include:

- safe/backup destination posture;
- generic save API cannot request title injection;
- low-level restore rejects live-title writes before mounting save data;
- legacy `injectToGame=1` is disabled/rewritten.

The extended hardware test proved inherited mutation UI exists above those guards, so #23 traces the actual call chains instead of assuming the low-level lock is sufficient.

Future live writes are **per adapter**, not one global switch.

Required shape:

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

# Upstream / reference stack

PokeBank NX checks mature Pokémon homebrew/research projects before rebuilding difficult infrastructure from scratch.

| Project | Role |
|---|---|
| [PKSE](https://github.com/kiasta/PKSE) | Original native Switch foundation; **upstream only** |
| [PKSM-Core](https://github.com/FlagBrew/PKSM-Core) | Native C++ historical Pokémon/save engine candidate |
| [PKSM](https://github.com/FlagBrew/PKSM) | Mature Bank, backup, migration, event and PKSM-Core integration reference |
| [Pokémon Chest](https://github.com/Universal-Team/pkmn-chest) | Gen III–V Bank and second Nintendo-platform PKSM-Core integration reference |
| [PHBank](https://github.com/gocario/PHBank) | Historical Game-PC ↔ offline-Bank transfer UX reference |
| [PHBankGBC](https://github.com/0xb01u/PHBankGBC) | Secondary Gen I/II save-layout reference; verify independently |
| [PKHeX](https://github.com/kwsch/PKHeX) | Primary correctness/reference implementation |
| [PKHeX-Plugins / Auto Legality](https://github.com/santacrab2/PKHeX-Plugins) | Encounter-driven legality/generation reference |
| [pkHouse](https://github.com/Insektaure/pkHouse) | Modern Switch save-behavior reference |
| [pkDex](https://github.com/Insektaure/pkDex) | Pokédex UX/data-organization reference |
| [PKForge](https://github.com/sofianeelhor/PKForge) | Vault/provenance/transaction architecture reference |

Pinned revisions/licenses/classifications: [`docs/UPSTREAM_AUDIT.md`](docs/UPSTREAM_AUDIT.md).  
Additional bank-project audit: [`docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`](docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md).

PKSM/Chest/PHBank do **not** replace the Master Vault design. They give us proven ideas around parsing, backups, migrations, Game↔Bank interaction and PKSM-Core integration.

---

# Development start point

Read in this order:

1. [`PROJECT_STATUS.md`](PROJECT_STATUS.md)
2. [`docs/V1_ROADMAP.md`](docs/V1_ROADMAP.md)
3. [`docs/NEXT_SESSION_PLAN.md`](docs/NEXT_SESSION_PLAN.md)
4. [`docs/PROJECT_MAP.md`](docs/PROJECT_MAP.md)
5. [`docs/BUILD_RECORD.md`](docs/BUILD_RECORD.md)
6. [`docs/DEVICE_TEST_REPORT_2026-09-01.md`](docs/DEVICE_TEST_REPORT_2026-09-01.md)
7. [`docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md`](docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md)
8. [`docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md`](docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md)
9. [`docs/SESSION_RUNBOOK.md`](docs/SESSION_RUNBOOK.md)
10. [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
11. [`docs/SAVE_SAFETY.md`](docs/SAVE_SAFETY.md)
12. current issue-specific spec/prompt

Latest work log: [`docs/SESSION_LOG_2026-09-02.md`](docs/SESSION_LOG_2026-09-02.md).

---

## License / attribution

PokeBank NX is distributed under the repository's **GNU Affero General Public License v3**. See [`LICENSE`](LICENSE).

The project began from PKSE and preserves required upstream/license attribution. External projects retain their own licenses/notices; direct reuse decisions must be reviewed and recorded rather than assumed from a GitHub link.

---

## Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks are property of their respective owners.

Unofficial fan-made homebrew project • © 2026 PokeBank NX Contributors