# PokeBank NX — Project Map

Last updated: 2026-09-02

This page is the short navigation/index for verified state, active prompts, hardware reports, and GitHub work.

## Read first

1. [`../PROJECT_STATUS.md`](../PROJECT_STATUS.md) — authoritative verified state
2. [`DEVICE_TEST_REPORT_2026-09-01.md`](DEVICE_TEST_REPORT_2026-09-01.md) — first physical Switch hardware result
3. [`NEXT_SESSION_PLAN.md`](NEXT_SESSION_PLAN.md) — current execution order
4. [`PROMPT_SESSION2_5_VISUAL_SHELL.md`](PROMPT_SESSION2_5_VISUAL_SHELL.md) — **HIGH**, next visible-shell/input-fix session
5. [`PROMPT_SESSION3_PKSM_CORE.md`](PROMPT_SESSION3_PKSM_CORE.md) — **MAX**, PKSM-Core after replacement hardware pass
6. [`SESSION_RUNBOOK.md`](SESSION_RUNBOOK.md) — implementation/test/build/push discipline
7. [`CONTROLS.md`](CONTROLS.md) — controller contract
8. [`UI_FLOW.md`](UI_FLOW.md) — safe UI/action semantics
9. [`UI_STYLE_GUIDE.md`](UI_STYLE_GUIDE.md) — PokeBank NX visual contract
10. [`ARCHITECTURE.md`](ARCHITECTURE.md) — module boundaries
11. [`UPSTREAM_AUDIT.md`](UPSTREAM_AUDIT.md) — external project/reuse map

## First physical hardware milestone

Exact tested binary:

```text
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Artifact: PokeBank-NX-UI-Theme-3be4de6.nro
Size: 9,707,957 bytes
SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
Result: DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

Passed on physical Switch:

```text
Boot
D-pad
A Action Sheet
B/Cancel
L/R
ZL/ZR
+
-
OLED Black
Dark
Light
Theme persistence
Party
Boxes
Storage
No crashes
```

Failed/incomplete:

```text
Held Left Stick repeat      FAIL
Visible PokeBank NX shell   FAIL / INCOMPLETE
```

Issue #8 is closed because the physical-test milestone itself is complete.

## Completed milestones

### #2 — Controller-first Pokémon Action Sheet — CLOSED

Source:

```text
82a0779a5143cca0690d0c7068946d84ebe9f107
```

Its behavior was successfully exercised on physical hardware through the combined Session 2 build.

### #8 — First exact physical PokeBank NX `.nro` test — CLOSED

The exact report is `DEVICE_TEST_REPORT_2026-09-01.md`.

## Current active work

### 1. #13 — HOME-style controls and PokeBank NX UI shell — REOPENED

The semantic theme/control foundation exists and many parts passed hardware testing, but the application still looks substantially like PKSE.

Next goal:

```text
visibly PokeBank NX at boot/Select Game
PokeBank NX header/app identity
obvious PKSE product branding removed from tested path
existing OLED/Dark/Light system visibly applied
Options/Help/Action Sheet share one PokeBank NX visual language
preserve controller/safety behavior
```

Use:

```text
docs/PROMPT_SESSION2_5_VISUAL_SHELL.md
```

### 2. #19 — Fix held Left Stick navigation repeat — OPEN

Physical Switch result on `3be4de6b`:

```text
D-pad                PASS
Left Stick + HOLD    FAIL
```

Fix native analog/deadzone/sustained-repeat behavior and produce a replacement `.nro`.

### 3. #16 — PokeBank NX branding/startup/NRO metadata — OPEN / ELEVATED

Physical testing confirmed user-visible PKSE branding/logo is still present.

For the next milestone, only the visible branding pieces needed to make the top-level app clearly PokeBank NX are required. Full real startup/loading-stage work may continue later.

### 4. #4 — PKSM-Core Gen III integration spike — NEXT MAX ENGINEERING

Start only after the Session 2.5 replacement build is stable enough on hardware.

Use:

```text
docs/PROMPT_SESSION3_PKSM_CORE.md
```

Narrow target:

```text
PK3
Sav3
FireRed / LeafGreen GBA
read-only parse
box/party extraction
round-trip strategy
integration/dependency decision
```

### 5. #3 — Master Vault v1 + named Banks

Immutable raw entities, stable Vault IDs, SHA-256, provenance, atomic transactions, logical Bank references.

### 6. #9 — Professional Summary + provenance

Generation-aware Summary/Origin/Legality/Provenance presentation.

### 7. #6 — RetroArch discovery + read-only Gen I-III adapters

Depends on PKSM-Core decision.

### 8. #11 — Modern Switch adapter audit

Validate inherited handlers against PKHeX and pkHouse reference behavior.

### 9. #5 — Host-side PKHeX Oracle

Correctness/legality/conversion comparison utility.

### 10. #7 — Vault-driven Pokédex / Living Dex

Vault remains authoritative for ownership.

### 11. #10 — Conversion/transfer without live writes

Derived Vault entities + staged/exported destination representations first.

## Supporting issues

```text
#15 persistent device-test .nro artifacts
#17 reproducible Pokémon/save golden test corpus
```

## Architecture / UX docs

- [`ARCHITECTURE.md`](ARCHITECTURE.md)
- [`CONTROLS.md`](CONTROLS.md)
- [`UI_FLOW.md`](UI_FLOW.md)
- [`UI_STYLE_GUIDE.md`](UI_STYLE_GUIDE.md)
- [`MASTER_VAULT_SPEC.md`](MASTER_VAULT_SPEC.md)
- [`SAVE_SAFETY.md`](SAVE_SAFETY.md)
- [`TRANSFER_MODEL.md`](TRANSFER_MODEL.md)
- [`POKEDEX_SPEC.md`](POKEDEX_SPEC.md)
- [`PKHEX_ORACLE.md`](PKHEX_ORACLE.md)

## Integration / research

- [`PKSM_CORE_INTEGRATION.md`](PKSM_CORE_INTEGRATION.md)
- [`PKHOUSE_REFERENCE.md`](PKHOUSE_REFERENCE.md)
- [`GAME_SUPPORT_MATRIX.md`](GAME_SUPPORT_MATRIX.md)
- [`UPSTREAM_AUDIT.md`](UPSTREAM_AUDIT.md)

## Testing / release

- [`DEVICE_TEST_CHECKLIST.md`](DEVICE_TEST_CHECKLIST.md)
- [`DEVICE_TEST_REPORT_2026-09-01.md`](DEVICE_TEST_REPORT_2026-09-01.md)
- [`BUILD_RECORD.md`](BUILD_RECORD.md)
- [`RELEASE_CHECKLIST.md`](RELEASE_CHECKLIST.md)
- [`.github/workflows/host-tests.yml`](../.github/workflows/host-tests.yml)

## Near-term execution loop

```text
HIGH: #13 + #19 + visible portion of #16
        ↓
host tests + sanitizers + native build
        ↓
new exact .nro + source SHA + binary SHA-256
        ↓
second physical Switch test
        ↓
fix any blocking regression
        ↓
MAX: #4 PKSM-Core PK3/Sav3 spike
```

## Permanent safety reminder

Live installed-game save writing remains hard disabled until an explicitly named adapter passes the gates in `SAVE_SAFETY.md`.

Do not weaken the lock to make UI, transfer, editor, or integration demos appear complete.