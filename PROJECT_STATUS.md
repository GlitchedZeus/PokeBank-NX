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
- Recovery branch: `recovery/interrupted-2026-09-01`
- Live installed-game save writing: **HARD DISABLED**

## Important application/source checkpoints

```text
3101fc0   verified recovery checkpoint
fabff21   recovered baseline/status checkpoint
c618bd5e44381635f92c17fc7b36c594b64aaa40   hard-lock live game save writes
82a0779a5143cca0690d0c7068946d84ebe9f107   controller Pokémon Action Sheet
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a   Session 2 controls/theme application source
3e2fec591fa178b51f16f2741c9f5f68a04e7a44   Session 2 build/status record
```

## Verification vocabulary

Use these states precisely:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

`DEVICE TESTED` means a human physically ran the exact recorded binary/hash. It does **not** mean every tested capability passed. Device failures must be recorded alongside the device-tested claim.

---

# MILESTONE — FIRST PHYSICAL SWITCH HARDWARE TEST COMPLETE

On 2026-09-01 local / 2026-09-02 UTC, the exact Session 2 PokeBank NX build was physically run on Nintendo Switch hardware.

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

Physical test results:

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
```

Safety/result notes:

- No crash was observed during this test.
- Action Sheet behavior worked on physical hardware.
- Party, Boxes, and Storage were physically exercised successfully.
- OLED Black, Dark, Light, and theme persistence worked on hardware.
- `+` contextual Options and `-` Help worked on hardware.
- L/R and ZL/ZR navigation worked on hardware.
- No reported live installed-save write/safety regression was observed.

Known failures / product gaps exposed by hardware:

1. **Held Left Stick navigation repeat fails on physical Switch.** Tracked by issue #19.
2. **The visible application still looks overwhelmingly like inherited PKSE.** PKSE branding/logo and inherited screen visual identity remain obvious. The new theme/control infrastructure exists, but it has not yet been applied broadly enough to make the application visibly PokeBank NX. Issue #13 is reopened and issue #16 branding work is now a near-term dependency.

This hardware pass closes issue #8 as a completed testing milestone; it does not imply issue #13 is complete.

Permanent report:

```text
docs/DEVICE_TEST_REPORT_2026-09-01.md
```

---

## Current verification table

| Area | State | Evidence / notes |
|---|---|---|
| Repository recovery | COMPLETE | recovered/published; old requested `1932cf0` not found |
| Stable game identity registry | HOST TESTED | 23 unique release/platform IDs |
| GBA/Switch FireRed + LeafGreen identity separation | HOST TESTED | distinct stable IDs |
| Live installed-save write policy | HOST TESTED / NRO BUILDS / DEVICE TESTED | no safety regression reported in first physical pass; hard lock remains mandatory |
| Controller-first Pokémon Action Sheet | HOST TESTED / NRO BUILDS / DEVICE TESTED | A opens sheet; B/Cancel safe on hardware |
| D-pad navigation | DEVICE TESTED — PASS | exact `3be4de6b` binary |
| Held Left Stick repeat | DEVICE TESTED — FAIL | issue #19 |
| L/R and ZL/ZR navigation | DEVICE TESTED — PASS | exact `3be4de6b` binary |
| Contextual `+` / read-only `-` Help | DEVICE TESTED — PASS | exact `3be4de6b` binary |
| OLED Black / Dark / Light themes | DEVICE TESTED — PASS | all three exercised on hardware |
| Theme persistence | DEVICE TESTED — PASS | restart persistence confirmed |
| Party / Boxes / Storage browsing | DEVICE TESTED — PASS | no crash reported |
| Visible PokeBank NX shell/branding | DEVICE TESTED — FAIL / INCOMPLETE | still substantially PKSE visual identity; issues #13/#16 |
| Master Vault v1 | SPECIFIED / NOT IMPLEMENTED | `docs/MASTER_VAULT_SPEC.md` |
| PKSM-Core Gen III integration | AUDITED / PLANNED | issue #4; hold until current device UI regression milestone is fixed |
| RetroArch Gen I-III adapters | PLANNED | after PKSM-Core decision |
| Modern Switch adapter validation | AUDITED / PLANNED | pkHouse/PKHeX comparison plan |
| PKHeX Oracle | SPECIFIED | issue #5 |
| Vault-driven Pokédex | SPECIFIED | issue #7 |
| Persistent `.nro` artifact workflow | PLANNED | issue #15 |
| Product branding/startup/NRO metadata | ACTIVE / PLANNED | issue #16; elevated by physical test |
| Golden save/Pokémon fixture corpus | PLANNED | issue #17 |

---

## Completed issue #2 — Controller-first Pokémon Action Sheet

Application source:

```text
82a0779a5143cca0690d0c7068946d84ebe9f107
```

The shared Party/Boxes/Storage Action Sheet remains complete. Its behavior was subsequently exercised successfully on physical hardware through the combined Session 2 binary.

Action order:

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

Opening, navigating, B, and Cancel remain non-mutating. `View Pokémon` remains read-only. Unimplemented actions remain safe/not-yet-supported.

---

## Issue #13 — REOPENED after physical test

The Session 2 source at `3be4de6b...` successfully established reusable controller/theme machinery:

- semantic OLED Black / Dark / Light palettes;
- persisted theme selection;
- typed context-aware controller hints;
- contextual `+` behavior;
- read-only `-` Help;
- reusable panels/cards/modal/focus primitives;
- Action Sheet integration;
- ZL/ZR box jumps;
- host regression coverage.

However, physical hardware testing showed the visible application is still substantially the inherited PKSE interface, including obvious PKSE branding/logo and inherited visual structure.

Therefore issue #13 is **OPEN** again. The next UI milestone is not another theme-engine rewrite; it is to apply the existing PokeBank NX primitives visibly across the top-level shell so the application is immediately identifiable as PokeBank NX.

Required next visible targets:

```text
PokeBank NX app/header identity
remove obvious PKSE logo/branding from tested path
Select Game / Home shell visibly PokeBank NX
background/chrome/card language from docs/UI_STYLE_GUIDE.md
context hint bar visibly integrated
Options / Help modal styling
Action Sheet styling
fix held Left Stick repeat (#19)
```

Do not redesign every Summary/Vault/Pokédex screen in this pass.

---

## Current safety posture

**LIVE INSTALLED-GAME SAVE WRITING IS HARD DISABLED.**

Protections include:

1. safe/backup destination posture;
2. generic save API cannot request title injection;
3. low-level restore rejects live-title writes before mounting save data;
4. legacy `injectToGame=1` is disabled/rewritten.

The first hardware test did not report a visible safety regression. This does not authorize live writes.

---

## Current task order

### Priority 1 — HIGH — visible PokeBank NX shell + physical input fix

Use:

```text
docs/PROMPT_SESSION2_5_VISUAL_SHELL.md
```

Target issues:

```text
#13 reopened — apply PokeBank NX visual shell
#19 — fix held Left Stick navigation repeat
#16 — only the user-visible branding pieces needed for this milestone
```

Produce another exact `.nro` and physically retest it.

### Priority 2 — MAX — PKSM-Core Gen III spike

Only after the replacement device build visibly reads as PokeBank NX and the held-stick failure is fixed or clearly isolated.

Use:

```text
docs/PROMPT_SESSION3_PKSM_CORE.md
```

Target issue #4:

```text
PK3
Sav3
FireRed / LeafGreen GBA
read-only parsing
box/party extraction
round-trip strategy
adapter/dependency decision
```

### Later product priorities

```text
#3  Master Vault v1 + named Banks
#9  Professional Summary + provenance
#6  RetroArch discovery + read-only Gen I-III adapters
#11 Modern Switch adapter validation
#5  PKHeX Oracle
#7  Vault-driven Pokédex / Living Dex
#10 Conversion / transfer without live writes
```

Supporting issues: #15 artifact workflow, #16 branding/startup, #17 golden fixtures.

---

## Documentation map

Start with:

```text
docs/PROJECT_MAP.md
docs/NEXT_SESSION_PLAN.md
docs/DEVICE_TEST_REPORT_2026-09-01.md
docs/PROMPT_SESSION2_5_VISUAL_SHELL.md
```

Core contracts remain:

- `docs/SESSION_RUNBOOK.md`
- `docs/CONTROLS.md`
- `docs/UI_FLOW.md`
- `docs/UI_STYLE_GUIDE.md`
- `docs/ARCHITECTURE.md`
- `docs/SAVE_SAFETY.md`
- `docs/MASTER_VAULT_SPEC.md`
- `docs/UPSTREAM_AUDIT.md`
- `docs/PKSM_CORE_INTEGRATION.md`
- `docs/PKHEX_ORACLE.md`
- `docs/BUILD_RECORD.md`
- `docs/DEVICE_TEST_CHECKLIST.md`

## Permanent rule

Implement → host test → sanitize → native build → record application SHA/artifact hash → physical test → record exact pass/fail results.

Never turn `DEVICE TESTED` into an assumption that the tested behavior passed.