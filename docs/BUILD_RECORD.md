# PokeBank NX — Build / Artifact Record

Last updated: 2026-09-02

This file distinguishes application source checkpoints, documentation commits, build artifacts, and physical hardware results.

A binary is not reproducibly identified by filename alone.

---

## Recovery-era safety build

```text
Reported size: 9,695,669 bytes
SHA-256: 0cf50b659ed5c648009e10d51a75a398bc2a3e69e4cbeb99cc0b74b9643ece07
Local short source recorded at recovery: 65aa52c
Published remote safety milestone: c618bd5e44381635f92c17fc7b36c594b64aaa40
Device tested: NO
```

`65aa52c` is not available as a remote GitHub commit. Rebuild from published source rather than chasing that local recovery SHA.

---

## Action Sheet build

```text
Version/tag: 0.1.0-alpha
Branch: feature/pokebank-playable
Application source SHA: 82a0779a5143cca0690d0c7068946d84ebe9f107
Commit message: ui: add controller Pokemon action sheet
Build date: 2026-09-01 UTC
Compiler: aarch64-none-elf-g++ (devkitA64) 15.2.0
Host test result: PASS
Sanitizer result: PASS
Git diff check: PASS
Native build result: BUILDS
Artifact filename: PokeBank-NX-ActionSheet-82a0779.nro
Artifact size: 9,695,669 bytes
Artifact SHA-256: 6ff0f71c2e8f6d7fcf948a4bbc0037ba799e22bbaac433263be7cd0afac3b72b
Direct artifact device test: NO
```

Later status/documentation commit:

```text
467f4b896280498bd8c36abd01eb4a0b39995863
```

Although this exact Action-Sheet-only binary was not separately tested, the same Action Sheet functionality was later exercised successfully on physical hardware in the combined Session 2 build below.

---

# FIRST PHYSICALLY TESTED POKEBANK NX BUILD

## Session 2 HOME controls / themes build

```text
Version/tag: 0.1.0-alpha
Branch: feature/pokebank-playable
Application source SHA: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Application source commit message: fix: restore complete controller UI source
Build date: 2026-09-02 UTC
Compiler: aarch64-none-elf-g++ (devkitA64) 15.2.0
Host test result: PASS (6 suites)
Sanitizer result: PASS (ASan/UBSan)
Git diff check: PASS
Native build result: BUILDS
Embedded abbreviated commit: 3be4de6b
Artifact filename: PokeBank-NX-UI-Theme-3be4de6.nro
Artifact size: 9,707,957 bytes
Artifact SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
Persistent artifact location: ChatGPT Library /PokeVault NX/PokeBank-NX-UI-Theme-3be4de6.nro
Device tested: YES
Device result: PARTIAL PASS / KNOWN FAILURES
First report: docs/DEVICE_TEST_REPORT_2026-09-01.md
Extended report: docs/DEVICE_TEST_EXTENDED_REPORT_2026-09-02.md
```

### Original shorter physical pass

```text
BOOT                     PASS
D-PAD                    PASS
LEFT STICK + HOLD        FAIL
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
CRASHES                  NONE DURING SHORTER PASS
VISIBLE POKEBANK NX UI   FAIL / INCOMPLETE
```

### Extended hardware pass on the same exact artifact

```text
5 MIN IDLE                        PASS
10 MIN NORMAL BROWSING            PASS
5 RELAUNCHES                      PASS
ACTION SHEET HEAVY OPEN/CLOSE     PASS
HELD D-PAD                        PASS
LEFT STICK SINGLE TAP             FAIL — no input/action
LEFT STICK HOLD                   FAIL — no input/action
LEFT STICK DIAGONAL               FAIL — no input/action
ONE OLD LEGENDS ARCEUS SAVE       REPRODUCIBLE CRASH
USER-REACHABLE MUTATION UI        PRESENT
APP/LEGACY STORAGE PERSISTENCE    PRESENT
LIVE INSTALLED SAVE WRITE         NOT PROVEN
```

Important interpretation:

- **DEVICE TESTED** means the exact binary was physically run.
- It does not mean every tested capability passed.
- The Left Stick failure is broader than the first short report; issue #19 now tracks no analog navigation at all on this artifact.
- One older Legends Arceus save reproducibly crashes; issue #24.
- Extended testing physically reached inherited Release/Create/Move/Multi/Edit/apply-style actions; issue #23 requires source-level persistence classification and blocking before a second device handoff.
- Legacy/app Storage is physically writable/persistent and is not automatically the future Master Vault; issue #27.
- The extended checklist did not finish the external original-game save verification, so a live installed-save write regression is **not proven** from this test alone.

Later Session 2 build/status documentation commit:

```text
3e2fec591fa178b51f16f2741c9f5f68a04e7a44
docs: record Session 2 device build
```

---

# SESSION 2.5 APPLICATION SOURCE CHECKPOINT — SECOND ARTIFACT NOT YET FROZEN

The visible-shell/physical-stick implementation reached a remote source checkpoint:

```text
Version/tag: 0.1.0-alpha
Branch: feature/pokebank-playable
Application source SHA: 361c6f551496470db305948d702944c6ed9889c1
Application source commit message: ui: add visible PokeBank shell and physical stick input
GitHub host CI: PASS
Remote source: VERIFIED
Device tested: NO
```

Reported implementation includes:

```text
real libnx Left Stick position handling
analog deadzone/hysteresis adapter
single-tap + sustained analog navigation repeat
Select Game / Backups / Party / Boxes / Storage analog integration
visible PokeBank NX header/archive identity
PokeBank NX branded chrome/cards
shared Help / Options visual treatment
matching Action Sheet styling
NRO/window identity changed to PokeBank NX
```

The interrupted Session 2.5 run reported host tests, sanitizers, and a native integration build passing before the source checkpoint.

Originally this checkpoint was waiting only for a clean exact-source rebuild and artifact hash. The extended first-device test changed the gate.

Before a second artifact is handed over, the next session must:

```text
#23 trace/classify/block inherited installed-source mutation paths
#24 harden the PLA old/malformed-save crash path
preserve #19 analog source fix
preserve #13/#16 visible PokeBank NX shell/identity
```

Therefore current truth is:

```text
361c6f55 application source: PUBLISHED / USEFUL CHECKPOINT
GitHub host CI: PASS
Final second-device application source: NOT YET FROZEN
Exact second-device .nro: PENDING
Replacement artifact SHA-256: PENDING
Second physical device test: NOT DONE
```

If #23/#24 require source changes, create a **new application-source commit** and build/hash from that exact source. Do not modify source and still label the artifact `361c6f55`.

Use:

```text
docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md
```

---

## Device-test milestone tracking

Issue #8 is complete because the first exact recorded PokeBank NX `.nro` was physically tested.

Current blockers/follow-up:

```text
#13  OPEN — visible shell source published; second visual acceptance pending
#19  OPEN — full Left Stick source fix pending physical test
#23  OPEN — inherited mutation UI safety audit; second-device blocker
#24  OPEN — old/malformed PLA crash; second-device blocker
#16  OPEN — visible identity improved; full startup/icon/NACP remains
#25  OPEN — Pokémon visual Summary/View later
#26  OPEN — controller normalization later
#27  OPEN — legacy Storage vs Master Vault clarification
```

---

## Artifact preservation issue

Issue #15 tracks durable `.nro` preservation through GitHub Actions or prerelease artifacts.

Preferred future flow:

```text
application source commit
        ↓
host tests + sanitizers
        ↓
native .nro build
        ↓
metadata manifest / hashes
        ↓
persistent artifact
        ↓
physical device test
        ↓
exact pass/fail report
```

Artifact automation must never automatically mark a build `DEVICE TESTED`.

---

## Build record template

```text
Version/tag:
Branch:
Application source full SHA:
Application source commit message:
Build date/time:
Compiler/devkitPro environment:
Host test result:
Sanitizer result:
git diff --check result:
Native build result:
Artifact filename:
Artifact size:
Artifact SHA-256:
Persistent artifact/release location:
Device tested: YES/NO
Device result: PASS / PARTIAL PASS / FAIL
Exact device-test report/reference:
```

If a later documentation/status commit is created after the application source, record it separately.

---

## Verification rule

These are different claims:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

For device-tested builds always record the actual result beside the claim.

A remembered filename, local-only session report, successful host test, or pre-commit binary is never enough to claim a final device artifact or `DEVICE TESTED`.