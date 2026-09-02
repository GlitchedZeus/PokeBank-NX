# PokeBank NX — Next Session Plan

Last updated: 2026-09-02
Status: ACTIVE EXECUTION PLAN

`PROJECT_STATUS.md` is authoritative.

The first exact PokeBank NX `.nro` has now been physically tested on Nintendo Switch hardware. The result was a strong functional partial pass but exposed two blocking near-term issues: held Left Stick repeat fails, and the visible application still looks overwhelmingly like inherited PKSE.

Permanent physical report:

```text
docs/DEVICE_TEST_REPORT_2026-09-01.md
```

## Current position

Completed milestones:

```text
#2  Controller-first Pokémon Action Sheet
#8  First exact physical PokeBank NX .nro test
```

Exact physically tested binary:

```text
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Artifact: PokeBank-NX-UI-Theme-3be4de6.nro
Size: 9,707,957 bytes
SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
Result: DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

Hardware passes:

```text
BOOT
D-PAD
A ACTION SHEET
B/CANCEL
L/R
ZL/ZR
+
-
OLED BLACK
DARK
LIGHT
THEME PERSISTENCE
PARTY
BOXES
STORAGE
NO CRASHES
```

Hardware failures/gaps:

```text
LEFT STICK + HOLD     FAIL
VISIBLE POKEBANK NX   FAIL / INCOMPLETE
```

Tracking:

```text
#13 REOPENED — visible PokeBank NX shell incomplete
#19 OPEN — held Left Stick repeat hardware bug
#16 OPEN — branding/startup/NRO metadata; near-term visible dependency
```

---

# Block A — HIGH — Session 2.5 visible shell + device fix

Use:

```text
docs/PROMPT_SESSION2_5_VISUAL_SHELL.md
```

## Goal

Produce a second device-test `.nro` that:

1. fixes held Left Stick navigation repeat;
2. immediately looks like PokeBank NX rather than PKSE;
3. removes obvious PKSE product branding/logo from the normal tested path;
4. visibly applies the existing semantic theme/card/modal/hint primitives;
5. preserves every hardware behavior that passed;
6. preserves the live-write hard lock.

## Narrow visible scope

Prioritize:

```text
PokeBank NX header/app identity
Select Game / Home shell
background/chrome/card/focus language
bottom controller hint bar
Options modal
Help modal
Action Sheet visual integration
top-level game browser branding/chrome where small
held Left Stick repeat
```

Do not redesign every Summary/Vault/Pokédex screen in this block.

## Verification

Run:

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1
```

Then update status/docs/issues, commit application source, push, verify remote SHA, and provide a new exact `.nro` with filename/size/SHA-256.

Do not mark that replacement build `DEVICE TESTED` until it is physically run.

---

# Between Block A and Block B — second physical Switch test

The replacement build should target:

```text
BOOT                              PASS
D-PAD                             PASS
LEFT STICK + HOLD                 PASS
A/B ACTION SHEET                  PASS
L/R + ZL/ZR                       PASS
+ / -                             PASS
OLED BLACK / DARK / LIGHT         PASS
THEME PERSISTENCE                 PASS
PARTY / BOXES / STORAGE           PASS
NO CRASHES                        PASS
NO LIVE-WRITE REGRESSION          PASS
VISIBLY POKEBANK NX               PASS
OBVIOUS PKSE BRANDING REMOVED     PASS
```

Only physically exercised behaviors on the exact replacement hash become device evidence.

---

# Block B — MAX — PKSM-Core Gen III spike

Use only after the Session 2.5 replacement build is stable enough on hardware:

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
active save-slot/sector behavior
PK3 encrypt/decrypt/checksum behavior
untouched round-trip strategy
adapter/dependency decision
```

Issue #17 tracks reusable golden fixtures. Do not let fixture perfection block the smallest useful PK3/Sav3 integration decision.

No live writes during this spike.

---

# Product order after PKSM-Core

```text
#3  Master Vault v1 + named Banks
#9  Professional Summary + provenance
#6  RetroArch discovery + read-only Gen I-III adapters
#11 Modern Switch adapter validation
#5  PKHeX Oracle
#7  Vault-driven Pokédex / Living Dex
#10 Conversion / transfer without live writes
```

Supporting:

```text
#15 persistent .nro artifacts
#16 final branding/startup/NRO metadata
#17 reproducible golden fixtures
```

---

# Do not do prematurely

Do not start all of these in the same session:

```text
full Master Vault
full Pokédex
full legality engine
full event archive
all generations
live installed-save writing
full application-wide visual redesign
```

Finish the smallest coherent milestone, build it, preserve it, and physically test it.