# PokeBank NX Project Status

Last updated: 2026-09-02

This file is the authoritative verified-state handoff for coding sessions. GitHub source/build evidence and physical device evidence are tracked separately; a build may be physically tested and still contain failures.

## Project identity

- Product: **PokeBank NX**
- Version: `0.1.0-alpha`
- Repository: `GlitchedZeus/PokeBank-NX`
- Writable remote: `origin`
- Upstream-only remote: `upstream` (`kiasta/PKSE`)
- Development branch: `feature/pokebank-playable`
- Live installed-game save writing: **HARD DISABLED**

## Important application/source checkpoints

```text
c618bd5e44381635f92c17fc7b36c594b64aaa40   hard-lock live game save writes
82a0779a5143cca0690d0c7068946d84ebe9f107   controller Pokemon Action Sheet
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a   first physically tested controls/theme source
361c6f551496470db305948d702944c6ed9889c1   Session 2.5 visible PokeBank shell + physical stick source
```

The branch may contain later **documentation-only** commits. For the second device-test artifact, the required application source remains `361c6f55...` unless a genuine source regression forces a new application-source checkpoint.

## Verification vocabulary

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

`DEVICE TESTED` means a human physically ran the exact recorded binary/hash. It does not imply every tested capability passed.

---

# Milestone 1 — first physical Switch hardware test complete

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

Physical results:

```text
BOOT                     PASS
D-PAD                    PASS
LEFT STICK + HOLD        FAIL
A ACTION SHEET           PASS
B / CANCEL               PASS
L / R                    PASS
ZL / ZR                  PASS
+                        PASS
-                        PASS
OLED BLACK               PASS
DARK                     PASS
LIGHT                    PASS
THEME PERSISTENCE        PASS
PARTY                    PASS
BOXES                    PASS
STORAGE                  PASS
CRASHES                  NONE
VISIBLE POKEBANK NX UI   FAIL / INCOMPLETE
```

Permanent report: `docs/DEVICE_TEST_REPORT_2026-09-01.md`.

Issue #8 is complete because the first exact physical-test milestone happened. The failures became follow-up issues rather than being hidden.

---

# Current application source — Session 2.5

A later HIGH session implemented the visible-shell and physical analog-input work and successfully pushed the application source before running out of usage:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

GitHub host CI passed on that exact commit.

Reported implementation includes:

```text
real libnx Left Stick position handling
analog deadzone + hysteresis adapter
sustained held-stick repeat path
analog navigation in Select Game / Backups / Party / Boxes / Storage
visible PokeBank NX header/app identity
PokeBank NX branded chrome/cards
shared Options / Help visual treatment
matching Action Sheet styling
PokeBank NX NRO/window identity
```

The interrupted session reported host tests, ASan/UBSan, and a native integration build passing before the application-source checkpoint. However, it stopped before the required clean rebuild from exact source `361c6f55`, final artifact hash, preservation, and second hardware handoff.

Therefore current truth is:

```text
APPLICATION SOURCE: PUBLISHED
GITHUB HOST CI: PASS
EXACT REPLACEMENT NRO: PENDING CLEAN PACKAGE
SECOND DEVICE TEST: NOT DONE
```

Use `docs/PROMPT_SESSION2_5_FINISH.md` next. Do not redo the implementation unless verification finds a real regression.

---

## Current verification table

| Area | State | Evidence / notes |
|---|---|---|
| Repository recovery | COMPLETE | recovered/published foundation |
| Stable game identity registry | HOST TESTED | 23 unique release/platform IDs |
| Live installed-save write policy | HOST TESTED / NRO BUILDS / DEVICE TESTED | hard lock remains mandatory |
| Controller-first Action Sheet | HOST TESTED / NRO BUILDS / DEVICE TESTED | first physical build PASS |
| D-pad | DEVICE TESTED — PASS | `3be4de6b` |
| Held Left Stick repeat | SOURCE FIX PUBLISHED / SECOND DEVICE TEST PENDING | `361c6f55`; issue #19 |
| L/R + ZL/ZR | DEVICE TESTED — PASS | `3be4de6b` |
| `+` / `-` | DEVICE TESTED — PASS | `3be4de6b` |
| OLED Black / Dark / Light | DEVICE TESTED — PASS | all three exercised |
| Theme persistence | DEVICE TESTED — PASS | restart confirmed |
| Party / Boxes / Storage | DEVICE TESTED — PASS | no crash reported |
| Visible PokeBank NX shell | SOURCE PUBLISHED / SECOND DEVICE TEST PENDING | `361c6f55`; issue #13 |
| Final branding/startup/icon/NACP | PARTIAL / PLANNED | issue #16 |
| Master Vault v1 | SPECIFIED / NOT IMPLEMENTED | issue #3 |
| True Move Game <-> Vault <-> Game | SPECIFIED / LATER | issue #20; blocked on safe live-write adapters |
| PKSM-Core Gen III | AUDITED / NEXT MAX | issue #4 |
| RetroArch Gen I-III adapters | PLANNED | issue #6 |
| Modern Switch adapter validation | PLANNED | issue #11 |
| PKHeX Oracle | SPECIFIED | issue #5 |
| Vault-driven Pokédex | SPECIFIED | issue #7 |
| NRO diagnostics/reliability/QoL | BACKLOG | issue #21 + `docs/NRO_QUALITY_ROADMAP.md` |
| Artifact automation | PLANNED | issue #15 |
| Golden test corpus | PLANNED | issue #17 |

---

## Product transfer semantics

The long-term product behavior is now explicit:

```text
COPY = intentionally keep source and create destination representation
MOVE = relocate the active Pokemon; source stops being active only after destination verification
CLONE = intentional duplicate with clone provenance
```

Desired end-state:

```text
Game A -> Master Vault / Bank -> Game B
```

A true Move should feel like Pokémon Bank/HOME-style relocation: after success, the Pokémon is active in the destination and gone from the source location.

For safety, PokeBank NX may retain immutable archival/provenance records and rollback evidence behind the scenes. Those records are not another active playable copy.

**Current alpha remains COPY/import + read-only game saves.** True Move is later work under issue #20 and may be enabled only per game adapter after staged write, backup, rollback, checksum/container repair, reparse, readback, and physical-device safety gates pass.

See `docs/TRANSFER_MODEL.md`.

---

## Master Vault principles

The Vault remains the permanent, game-independent safety/provenance layer:

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

Archival history must not be confused with an active duplicate after a future true Move.

See `docs/MASTER_VAULT_SPEC.md`.

---

## Current safety posture

**LIVE INSTALLED-GAME SAVE WRITING IS HARD DISABLED.**

Current protections include:

1. safe/backup destination posture;
2. generic save API cannot request title injection;
3. low-level restore rejects live-title writes before mounting save data;
4. legacy `injectToGame=1` is disabled/rewritten;
5. controller/menu navigation cannot bypass the hard lock.

Do not weaken this for UI demos, transfer prototypes, or true-Move previews.

---

## Current task order

### Priority 1 — HIGH — finish/package Session 2.5

Use:

```text
docs/PROMPT_SESSION2_5_FINISH.md
```

Build/package exact application source:

```text
361c6f551496470db305948d702944c6ed9889c1
```

Required result:

```text
host tests PASS
ASan/UBSan PASS
git diff --check PASS
clean native build PASS
actual replacement .nro preserved
filename + size + SHA-256 recorded
READY FOR SECOND DEVICE TEST
NOT DEVICE TESTED
```

### Priority 2 — physical Switch test #2

Verify:

```text
visible PokeBank NX identity
obvious PKSE branding removed from normal path
Left Stick + held repeat
all previously passing controls/themes
Party / Boxes / Storage
no crashes
no live-write regression
```

### Priority 3 — MAX — PKSM-Core Gen III spike

Only after the replacement UI/input build is stable enough on hardware.

Use `docs/PROMPT_SESSION3_PKSM_CORE.md`.

Target issue #4:

```text
PK3
Sav3
FireRed / LeafGreen GBA
read-only parsing
party/box extraction
active save-slot/sector behavior
round-trip strategy
integration/dependency decision
```

### Product order after PKSM-Core

```text
#3  Master Vault v1 + Banks
#9  Professional Summary + provenance
#6  RetroArch / read-only Gen I-III adapters
#11 Modern Switch adapter validation
#5  PKHeX Oracle
#7  Vault-driven Pokédex / Living Dex
#10 Conversion / transfer without live writes
#20 True Move semantics after verified write adapters
```

Supporting work:

```text
#15 persistent .nro artifacts
#16 final branding/startup/icon/NACP
#17 golden fixtures
#21 diagnostics/reliability/performance/QoL
```

---

## NRO quality roadmap

`docs/NRO_QUALITY_ROADMAP.md` captures planned high-value improvements including:

```text
Diagnostics screen + privacy-safe export
Applet/constrained-memory detection
real startup stages
privacy-safe crash/error logs
clear READ ONLY / Vault / staged badges
Vault recovery + storage health
search/filter
Box Quick Jump
Continue / Recently Viewed / Recently Added
Favorites
Reduced Motion
optional original UI sounds / restrained rumble
text-size options
color-independent focus
virtualized large grids
bounded artwork/sprite caches
intentional missing-resource fallbacks
metadata/index rebuildability
safe-operation progress UX
```

Use an approximate implementation balance after the second UI/device milestone:

```text
70% core functionality
20% hardware validation / bugs
10% polish / infrastructure
```

---

## Documentation map

Start with:

```text
docs/PROJECT_MAP.md
docs/NEXT_SESSION_PLAN.md
docs/BUILD_RECORD.md
docs/DEVICE_TEST_REPORT_2026-09-01.md
docs/PROMPT_SESSION2_5_FINISH.md
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
- `docs/NRO_QUALITY_ROADMAP.md`
- `docs/UPSTREAM_AUDIT.md`
- `docs/PKSM_CORE_INTEGRATION.md`
- `docs/PKHEX_ORACLE.md`
- `docs/BUILD_RECORD.md`

## Permanent rule

Implement -> host test -> sanitize -> native build -> record exact application SHA/artifact hash -> physical test -> record exact pass/fail result.

Never confuse a later documentation commit with the application source used to build a device artifact.