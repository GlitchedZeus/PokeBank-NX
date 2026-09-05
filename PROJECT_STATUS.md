# PokeBank NX Project Status

Last updated: 2026-09-05

## Post-0ea98cc1 device feedback — static Pokémon artwork correction

The exact `0ea98cc1...` device build physically renders the packaged HOME artwork successfully and
its quality was accepted, but its inherited sine-driven idle effect failed visual acceptance:

```text
POKEMON ARTWORK:                         PASS
ARTWORK QUALITY:                        GOOD
ARTIFICIAL IDLE / BREATHING MOTION:     FAIL / REMOVE
```

PokeBank NX now routes the Pokémon details modal, selected Box preview and Party preview through a
reusable static contain renderer. It preserves the source aspect ratio and has no time input, bob,
width modulation or height modulation. The `SpriteManager` resolver/cache and HD-to-legacy fallback
remain unchanged. A pure host regression suite covers square, wide, tall and invalid sprite layout.
This correction is **IMPLEMENTED / HOST TESTED / NRO BUILDS**. Its exact replacement artifact is:

```text
Application source: 59895efc1f70974fb8c7ba8895f83c9688f27b5c
Commit: ui: render Pokemon artwork without fake idle motion
Embedded version / short SHA: 0.1.0-alpha / 59895efc
Artifact: PokeBank-NX-Static-Render-59895efc.nro
Size: 155117481 bytes
SHA-256: d85284030a7d7bef7dce73daf80089c440f011313ff423026693d35920c4c83c
Host tests (9 suites): PASS
ASan/UBSan: PASS
git diff --check: PASS
Native build: PASS (clean exact-source build)
Asset preflight: PASS (3260 HD sprites)
Embedded RomFS comparison: PASS (3281 files)
Device tested: NO
```

Status: **READY FOR STATIC-RENDER DEVICE TEST / NOT DEVICE TESTED**.

Permanent feedback record: `docs/DEVICE_TEST_SPRITE_MOTION_FEEDBACK_2026-09-05.md`.

## Session 2.6 — READY FOR SECOND DEVICE TEST / NOT DEVICE TESTED

- Application source: `0ea98cc1a9f9dfc2b17abc33e944caa4aa9de915`
- Commit: `safety: lock installed-source UI and harden PLA reads`
- Embedded version / short SHA: `0.1.0-alpha` / `0ea98cc1`
- Artifact: `PokeBank-NX-Second-Device-0ea98cc1.nro`
- Size: **155117481 bytes**
- SHA-256: `4c220bdf1736fb626e97c30b4ceb89fb7da7a4f24bce17c1dd36d25017478f28`
- Host tests (8 suites), ASan/UBSan, `git diff --check`: **PASS**
- Exact-source `make clean` / `make -j1`: **NRO BUILDS**
- Asset preflight: **PASS**; 3260 HD sprites, 18 type icons, 3 fonts.
- Independent embedded RomFS comparison: **PASS**, all 3281 files match source assets byte-for-byte.
- GitHub source verified on `feature/pokebank-playable`; CI run `33839339713`: **PASS**.
- Checklist: `docs/DEVICE_TEST_CHECKLIST_SECOND_2026-09-02.md`; hardware fields remain blank.

The next task is Will's physical acceptance of this exact artifact, especially the old PLA save,
installed-source UI guards, analog input and rendered artwork. Do not merge to main or start later
engines before acceptance. Later documentation commits do not change this binary's source identity.

### Source milestone

Read `docs/SESSION2_6_SAFETY_IMPLEMENTATION.md` for the current source-state/persistence audit.
Installed-source browsing is enforced read-only; explicit backup workspaces retain inherited
editing. Legacy Storage is app-owned, not Master Vault. The low-level live-write lock is unchanged.
PLA container/layout/record validation and the shared file-reader allocation fix are IMPLEMENTED
and HOST TESTED with ASan/UBSan. Existing analog/theme/action-sheet tests pass.
The exact old failing PLA save is unavailable: #24 remains open pending physical retest.
Exact artifact identity is recorded above and in `docs/BUILD_RECORD.md`. This source is NOT DEVICE
TESTED. Do not merge to main before acceptance.

This is the authoritative verified-state handoff for coding sessions. Source commits, CI/build evidence and physical-device evidence are deliberately tracked separately.

## Project identity

- Product: **PokeBank NX**
- Version: `0.1.0-alpha`
- Repository: `GlitchedZeus/PokeBank-NX`
- Writable remote: `origin`
- Upstream-only remote: `upstream` (`kiasta/PKSE`)
- Development branch: `feature/pokebank-playable`
- Live installed-game save writing policy: **HARD DISABLED / NOT AN APPROVED CURRENT FEATURE**
- Canonical v1.0 roadmap: `docs/V1_ROADMAP.md`
- Master v1.0 tracking issue: **#29**

## Important application-source checkpoints

```text
c618bd5e44381635f92c17fc7b36c594b64aaa40   hard-lock live game save writes
82a0779a5143cca0690d0c7068946d84ebe9f107   controller Pokémon Action Sheet
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a   first physically tested controls/theme source
361c6f551496470db305948d702944c6ed9889c1   Session 2.5 visible PokeBank shell + physical stick source
```

The feature branch contains later documentation/research/tooling commits. **Branch HEAD is not automatically application-source identity.**

`361c6f55...` is the useful current UI/analog application checkpoint, but first-build hardware testing exposed safety/crash blockers. If #23/#24 change application code, the second-device artifact must use a **new application-source checkpoint** rather than being mislabeled as `361c6f55...`.

## Verification vocabulary

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

`DEVICE TESTED` means a human physically ran the exact recorded binary/hash. A tested build may still be a partial pass or fail.

---

# First exact physical Switch milestone — COMPLETE

Exact tested artifact:

```text
Application source:
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a

Artifact:
PokeBank-NX-UI-Theme-3be4de6.nro

Size:
9,707,957 bytes

SHA-256:
df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a

Physical status:
DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

Original short pass:

```text
BOOT                     PASS
D-PAD                    PASS
LEFT STICK               FAIL
A ACTION SHEET           PASS
B / CANCEL               PASS
L / R                    PASS in exercised context
ZL / ZR                  PASS in exercised context
+                        PASS
-                        PASS
OLED BLACK               PASS
DARK                     PASS
LIGHT                    PASS
THEME PERSISTENCE        PASS
PARTY                    PASS
BOXES                    PASS
STORAGE                  PASS
VISIBLE POKEBANK NX UI   FAIL / INCOMPLETE
```

Permanent reports:

```text
docs/DEVICE_TEST_REPORT_2026-09-01.md
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md
```

Issue #8 is complete because the physical-test milestone occurred. The failures remain tracked separately.

---

# Extended / follow-up first-device hardware evidence

The longer torture passes were run on the **same exact `3be4de6b...` binary**.

## Stability

```text
5 minutes idle                        PASS
10 minutes normal browsing            PASS
5 repeated relaunches                 PASS
Action Sheet ~100 open/close          PASS / no lag / no stuck input
HOME -> return                        PASS
sleep -> wake                         PASS
Joy-Con/controller reconnect          PASS
handheld                              PASS
one older Legends Arceus save         REPRODUCIBLE CRASH
docked                                NOT TESTED
```

The PLA crash is tracked by #24.

## Left Stick failure clarified

The old build receives **no navigation input from Left Stick at all**:

```text
single tap Up/Down/Left/Right  FAIL — no input/action
hold Up/Down/Left/Right        FAIL — no input/action
diagonal                       FAIL — no input/action
```

Held D-pad directions pass and repeat predictably.

Issue #19 tracks full physical Left Stick navigation. `361c6f55...` contains the reported source fix but is not device-tested.

## User-reachable inherited mutation UI

Hardware testing physically reached:

```text
X on Pokémon        Release path
Y in Boxes          Menu / Move / Multi
Y on Pokémon        grab/move behavior
A on empty slot     Create Pokémon path
Action Sheet Edit   inherited editable view
```

The tester changed an IV, applied it, reopened it, then restored the original value and applied again.

These are real backup/in-memory mutation controls and remain inconsistent with the current user-facing read-only alpha contract even though the installed title write path is hard-blocked.

---

# Sep 3 Arbok / Storage safety clarification

The exact physically observed Arbok sequence is now clarified:

```text
installed Pokémon Legends: Z-A source
        ↓ automatic backup
open Z-A BACKUP representation
        ↓
move Arbok from backup into inherited app Storage
        ↓
return to main menu
        ↓
open another supported game's backup/session
        ↓
open Storage
        ↓
Arbok is still present
```

The tester reports the **original installed Z-A save remained unchanged** during the exercised flow.

Therefore current truth is:

```text
LIVE INSTALLED SAVE WRITE OBSERVED:      NO
ORIGINAL INSTALLED SAVE CHANGED:         NO — tester-reported in exercised flow
USER-REACHABLE MUTATION UI:              PROVEN
BACKUP/IN-MEMORY SAVE MUTATION:          PROVEN
APP-OWNED STORAGE PERSISTENCE:           PROVEN
CROSS-GAME STORAGE VISIBILITY:           PROVEN
MASTER VAULT:                            NOT IMPLEMENTED
TRUE MOVE:                               NOT IMPLEMENTED
```

Current transfer shape:

```text
INSTALLED GAME — read/backup only
        ↓
MUTABLE BACKUP REPRESENTATION
        ↓
APP-OWNED LEGACY STORAGE (`PKSEBANK` / `bank.dat`)
        ↓
OTHER GAME'S LOADED BACKUP REPRESENTATION
```

The inherited UI appears capable of placing a compatible stored Pokémon into another game's loaded backup representation. Do not infer exhaustive conversion/persistence coverage from this one hardware path.

The Arbok event is best classified as a **persistent copy/import into app-owned legacy Storage from a mutable backup representation**, not product-level true Move and not a live installed-save write.

Issue #27 tracks legacy Storage versus future Master Vault/Banks. Issue #23 remains a blocker because Release/Create/Move/Edit/save-style UI is still confusingly reachable above the low-level hard lock.

---

# Other first-build hardware observations

## Summary / View sample

Four Pokémon were sampled:

```text
Bulbasaur
Alolan Meowth
Mewtwo
Mew
```

Reported:

```text
View opens        PASS
level             PASS
gender            PASS
shiny display     PASS
B return          PASS
nickname          N/A in samples
```

The `SPECIES` checklist wording was ambiguous to the tester. Future Summary should display both explicit Species and Type.

The tested `View Pokémon` lacked its expected Pokémon image/render. Source audit later found the inherited PKSE sprite pipeline uses generated gitignored RomFS assets. #25 tracks visuals and #37 gates device-test builds on required assets.

Future desired View/Pokédex additions include Pokémon visual, cry, and short Pokédex/location text. #9/#25/#35/#7 track those areas.

## Theme/readability follow-up

```text
OLED Black              PASS
Dark                    PASS
Light                   PASS
Theme persistence       PASS
Action Sheet all themes PASS
```

Tester feedback:

- bottom controller/button hints need more contrast in OLED Black and Dark;
- Dark could be slightly darker;
- Light is liked substantially as-is;
- OLED Black currently gives the preferred Action Sheet look.

This is polish input for #13/#21/#26, not a reason to delay the safety/crash gate.

## Discovery

Explicitly sampled native Switch cards included Shining Diamond and Let's Go Pikachu, both opening without duplicate/wrong-save behavior.

The tester reported no observed FireRed/LeafGreen platform mix-up and no non-Pokémon false positives in the exercised set.

Legacy GB/GBC/GBA saves stored under RetroArch were not automatically discovered. That is expected unfinished issue #6 work, not a current native-Switch regression.

---

# Session 2.5 visible-shell / analog application source

Published application source:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

GitHub host CI passed on this exact source.

Reported implementation:

```text
real libnx Left Stick position handling
analog deadzone + hysteresis
single-tap + sustained held-stick navigation
analog navigation in Select Game / Backups / Party / Boxes / Storage
visible PokeBank NX header/app identity
PokeBank NX branded chrome/cards
shared Options / Help visual treatment
matching Action Sheet styling
PokeBank NX NRO/window identity
```

Current status:

```text
IMPLEMENTED
GITHUB HOST CI PASS
NOT DEVICE TESTED
```

The interrupted coding session did not finish the final clean exact-source artifact packaging/preservation.

Because the first-device testing found #23/#24 blockers, the next session must preserve this work but perform the safety/crash fixes before handing over the second artifact.

---

# Current critical blockers — Session 2.6

Use:

```text
docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md
```

Required hardware evidence input now includes:

```text
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md
docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md
```

## #23 — mutation UI safety / source-state clarity

Known source/audit shape:

```text
Release/Create/Move/Edit   mutate in-memory/backup representation
normal Save Changes        Working Backup / New Backup
legacy Storage             app-owned persistent PKSEBANK
low-level live restore     hard-blocked before mounting installed save while policy disabled
```

Session 2.6 should verify this against current source, fill gaps, then make the UI unambiguous:

```text
INSTALLED SOURCE — READ ONLY
BACKUP/STAGED COPY — MUTABLE ONLY WHEN EXPLICIT
LEGACY STORAGE — APP-OWNED COMPATIBILITY STORAGE
MASTER VAULT — FUTURE
LIVE GAME WRITE — LATER / PER-ADAPTER
```

While live installed-save writing remains disabled, unsafe installed-source Release/Create/Move/Edit behavior must not be reachable or misleading.

## #24 — Legends Arceus defensive failure

Required behavior:

```text
valid supported save            opens
old/unsupported/malformed save  useful read-only error / safe return
parser failure                  never crashes app
failing save                    never auto-written/repaired
```

If the exact old save is unavailable in the coding runtime, add source-grounded defensive handling/tests and keep #24 open for physical retest.

## Preserve #19/#13/#16

Do not throw away the Session 2.5 analog/UI source while fixing safety/crash issues.

## #37 — device visual asset gate

Before the replacement device build, run the existing sprite/RomFS asset generation and `tools/check_device_assets.py` preflight so an otherwise working Summary renderer is not shipped without its required assets.

---

# Required second-device build pipeline

If #23/#24 changes application source:

```text
IMPLEMENT
→ HOST TEST
→ ASan/UBSan
→ git diff --check
→ NATIVE BUILD
→ COMMIT APPLICATION SOURCE
→ PUSH origin/feature/pokebank-playable
→ VERIFY REMOTE SHA
→ CLEAN REBUILD FROM EXACT NEW SHA
→ DEVICE ASSET PREFLIGHT
→ RECORD/HASH/PRESERVE .NRO
→ UPDATE DOCS/ISSUES
→ HANDOFF AS READY FOR SECOND DEVICE TEST
```

Do not change source and still label the artifact `361c6f55`.

Preferred packaging helper:

```bash
python tools/package_device_build.py PokeBankNX.nro --label Second-Device --zip
```

Before physical handoff require:

```text
HOST TESTS PASS
ASan/UBSan PASS
git diff --check PASS
NRO BUILDS
live-write hard lock intact
mutation paths classified
installed vs backup vs legacy Storage UI state clear
unsafe installed-source mutation UI blocked
PLA failure path hardened as far as evidence allows
device asset preflight PASS
```

Then the user physically tests the exact replacement hash.

---

# Current verification table

| Area | State | Evidence / notes |
|---|---|---|
| Repository recovery | COMPLETE | recovered/published foundation |
| Stable game identity registry | HOST TESTED | 23 current source IDs; expanded target catalog planned |
| Low-level live installed-save hard lock | HOST TESTED / NRO BUILDS / PHYSICAL SUPPORTING EVIDENCE | no live write observed in clarified Arbok flow |
| User-reachable inherited mutation UI | DEVICE TESTED — PRESENT / UI FIX REQUIRED | #23 |
| Legacy/app Storage persistence | DEVICE TESTED — PASS | Arbok persisted across switching game contexts, #27 |
| Legacy Storage cross-game visibility | DEVICE TESTED — PASS | backup -> Storage -> another game session |
| Controller-first Action Sheet | HOST TESTED / NRO BUILDS / DEVICE TESTED | #2 |
| D-pad | DEVICE TESTED — PASS | held repeat works |
| Left Stick on first build | DEVICE TESTED — FAIL | no input at all |
| Left Stick source fix | IMPLEMENTED / HOST CI PASS / RETEST PENDING | `361c6f55`, #19 |
| `+` / `-` | DEVICE TESTED — PASS in exercised contexts | Options/Help |
| OLED / Dark / Light | DEVICE TESTED — PASS | all three |
| Theme persistence | DEVICE TESTED — PASS | restart/sleep confirmed |
| HOME/sleep/controller reconnect | DEVICE TESTED — PASS | Sep 3 follow-up |
| Handheld readability/focus | DEVICE TESTED — PASS | docked pending |
| Party / Boxes / Storage browsing | DEVICE TESTED — PASS WITH CAVEATS | inherited mutation UI exposed |
| sampled Summary data | DEVICE TESTED — PASS/PARTIAL | data good; visual missing |
| Pokémon visual Summary | SOURCE PIPELINE EXISTS / FIRST ARTIFACT MISSING VISUAL | #25/#37 |
| old PLA save handling | DEVICE TESTED — FAIL | reproducible crash, #24 |
| visible PokeBank NX shell | IMPLEMENTED / HOST CI PASS / DEVICE RETEST PENDING | #13 |
| final startup/icon/NACP | PARTIAL / PLANNED | #16 |
| Master Vault v1 | SPECIFIED / NOT IMPLEMENTED | #3 |
| True Move | SPECIFIED / LATER | #20 |
| PKSM-Core Gen III | AUDITED / NEXT DEEP PHASE AFTER DEVICE GATE | #4 |
| Retro legacy discovery | PLANNED | #6; RetroArch saves not yet auto-detected |
| modern Switch adapters | PLANNED | #11 |
| PKHeX Oracle | SPECIFIED | #5 |
| Vault-driven Pokédex | SPECIFIED | #7 |
| Pokémon cries | PLANNED | #35 |
| NRO quality/reliability | BACKLOG / ROADMAP | #21 |
| artifact automation | TOOLING ADDED / broader durable automation tracked | #15 |
| device asset gate | TOOLING/DOCS ADDED | #37 |
| golden corpus | PLANNED | #17 |
| v1.0 master roadmap | PUBLISHED | `docs/V1_ROADMAP.md`, #29 |

---

# Product transfer semantics

```text
COPY  = intentionally keep source active and create destination representation
MOVE  = relocate active Pokémon; source stops being active only after verified destination success
CLONE = deliberate duplicate with clone provenance
```

Desired end-state:

```text
Game A → Master Vault / Bank → Game B
```

For safety, immutable archival/provenance/rollback evidence may remain behind the scenes without being presented as another active playable copy.

**Current alpha does not authorize true Move or live installed-save writing.**

The physically observed backup→legacy Storage→other-backup behavior is a useful UX proof-of-concept, not implementation of product-level Move.

True Move is issue #20 and is gated per game adapter by `docs/SAVE_SAFETY.md`.

---

# Master Vault principles

```text
immutable raw entity bytes
stable Vault ID
SHA-256
origin/source provenance
active/current location separate from origin
parent/derived lineage
transaction journal + crash recovery
named Banks as references/organization
rebuildable metadata/search indexes
```

Archival history must never be confused with an active duplicate.

See `docs/MASTER_VAULT_SPEC.md`.

Legacy `PKSEBANK` Storage is not Master Vault; #27 tracks migration/compatibility treatment.

---

# External reference/reuse state

Primary audit: `docs/UPSTREAM_AUDIT.md`.

Additional audits:

```text
docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md
docs/POKEMON_VISUAL_ASSET_AUDIT_2026-09-02.md
docs/PKSE_SPRITE_PIPELINE_AUDIT_2026-09-02.md
```

Inspected references include:

```text
FlagBrew/PKSM
Universal-Team/pkmn-chest
gocario/PHBank
0xb01u/PHBankGBC
PokeAPI/sprites
msikma/PokéSprite
Pokemon-3D-api assets/API
Project Pokémon sprite index
```

High-value conclusions:

- full PKSM gives BankFile/versioning/corruption/backups/event/app integration references beyond PKSM-Core;
- Pokémon Chest is another Nintendo-platform PKSM-Core integration example and Game↔Bank UX reference;
- PHBank strongly informs a future Transfer Workspace and one/multi/whole-box operations;
- PHBankGBC is secondary Gen I/II reference only and must be independently verified;
- inherited PKSE already has an offline RomFS Pokémon sprite pipeline using pinned PokeAPI HOME-style assets generated at build time;
- visual/media redistribution rights must be handled separately from repository code licenses.

These do **not** replace the PokeBank NX Master Vault/provenance design.

---

# Roadmap / task order

Canonical full v1.0 plan:

```text
docs/V1_ROADMAP.md
GitHub issue #29
```

Immediate order:

```text
#23 safety/UI contract + #24 PLA hardening
+ preserve #19 analog + #13/#16 shell
+ #37 asset gate
        ↓
replacement exact .nro
        ↓
physical Switch test #2
        ↓
#4 PKSM-Core Gen III spike
        ↓
Gen III read adapter
        ↓
#3 Master Vault + Banks
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

Do not skip the current physical safety/input/UI gate to start PKSM-Core.

---

# Documentation map

Read first:

```text
README.md
PROJECT_STATUS.md
docs/V1_ROADMAP.md
docs/NEXT_SESSION_PLAN.md
docs/PROJECT_MAP.md
docs/BUILD_RECORD.md
docs/DEVICE_TEST_REPORT_2026-09-01.md
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md
docs/MUTATION_SAFETY_STATIC_AUDIT_2026-09-02.md
docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md
docs/SESSION_LOG_2026-09-02.md
```

Core contracts:

- `docs/SESSION_RUNBOOK.md`
- `docs/CONTROLS.md`
- `docs/UI_FLOW.md`
- `docs/UI_STYLE_GUIDE.md`
- `docs/ARCHITECTURE.md`
- `docs/SAVE_SAFETY.md`
- `docs/MASTER_VAULT_SPEC.md`
- `docs/TRANSFER_MODEL.md`
- `docs/POKEDEX_SPEC.md`
- `docs/NRO_QUALITY_ROADMAP.md`
- `docs/DEVICE_BUILD_ASSET_GATE.md`
- `docs/DEVICE_ARTIFACT_PACKAGING.md`
- `docs/UPSTREAM_AUDIT.md`
- `docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`
- `docs/PKSM_CORE_INTEGRATION.md`

---

# Next exact instruction

Use HIGH reasoning:

```text
Open GlitchedZeus/PokeBank-NX.

Read PROJECT_STATUS.md,
docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md, and
docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md.

Then execute:

docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md

exactly.

Start working immediately.
```

Do not begin PKSM-Core until the replacement device-test gate is complete/stable enough.
