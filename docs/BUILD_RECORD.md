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
Exact report: docs/DEVICE_TEST_REPORT_2026-09-01.md
```

Physical test matrix:

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

Important interpretation:

- **DEVICE TESTED** means the exact binary was physically run.
- It does not mean every tested capability passed.
- Held Left Stick repeat failed and is tracked by issue #19.
- The visual product shell remained overwhelmingly inherited PKSE and is tracked by reopened issue #13 plus issue #16.
- No live installed-save write/safety regression was reported during this test.

Later Session 2 build/status documentation commit:

```text
3e2fec591fa178b51f16f2741c9f5f68a04e7a44
docs: record Session 2 device build
```

---

# SESSION 2.5 APPLICATION SOURCE CHECKPOINT — ARTIFACT PENDING

The visible-shell/physical-stick implementation reached a remote source checkpoint before the coding session ran out of usage.

```text
Version/tag: 0.1.0-alpha
Branch: feature/pokebank-playable
Application source SHA: 361c6f551496470db305948d702944c6ed9889c1
Application source commit message: ui: add visible PokeBank shell and physical stick input
GitHub host CI: PASS
Remote source: VERIFIED
```

Reported implementation at this source checkpoint includes:

```text
real libnx Left Stick position handling
analog deadzone/hysteresis adapter
sustained analog navigation repeat path
Select Game / Backups / Party / Boxes / Storage analog integration
visible PokeBank NX header/archive identity
PokeBank NX branded chrome/cards
shared Help / Options visual treatment
matching Action Sheet styling
NRO/window identity changed to PokeBank NX
```

The interrupted session reported host tests, sanitizers, and a native integration build passing before the source checkpoint. However, the required **clean exact-source rebuild + final artifact hash/preservation** had not been completed when the session ended.

Therefore the authoritative state is:

```text
Application source: PUBLISHED
GitHub host CI: PASS
Exact replacement .nro: PENDING
Replacement artifact SHA-256: PENDING
Second physical device test: NOT DONE
```

Do not invent an artifact filename/hash from a pre-checkpoint local build.

Use:

```text
docs/PROMPT_SESSION2_5_FINISH.md
```

to rebuild/package this exact source and produce the replacement device-test artifact.

---

## Device-test milestone tracking

Issue #8 is complete because the first exact recorded PokeBank NX `.nro` was physically tested.

Current follow-up:

```text
#13  OPEN — 361c6f55 source published; second device visual acceptance pending
#19  OPEN — source fix implemented; second physical stick test pending
#16  OPEN — visible identity improved; full startup/icon/NACP work remains
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