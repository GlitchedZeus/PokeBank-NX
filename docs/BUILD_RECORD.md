# PokeBank NX — Build / Artifact Record

## Static Pokémon render replacement — READY FOR DEVICE TEST

```text
Application source full SHA: 59895efc1f70974fb8c7ba8895f83c9688f27b5c
Application commit: ui: render Pokemon artwork without fake idle motion
Embedded short SHA/version: 59895efc / 0.1.0-alpha
Branch: feature/pokebank-playable
Artifact filename: PokeBank-NX-Static-Render-59895efc.nro
Artifact byte size: 155117481
Artifact SHA-256: d85284030a7d7bef7dce73daf80089c440f011313ff423026693d35920c4c83c
ZIP fallback: PokeBank-NX-Static-Render-59895efc.zip
ZIP byte size: 148511529
ZIP SHA-256: 44920db75271ff04910c6edfca2c17bdf33f738cab5d50c678468ab4f815b79d
Host tests: PASS (9 suites)
ASan/UBSan: PASS (LeakSanitizer disabled by existing host recipe)
git diff --check: PASS
Native build: PASS; clean make -j1 from exact published application SHA
Compiler: devkitA64 15.2.0
Asset preflight: PASS; 3260 HD sprites, 18 type icons, 3 fonts
Embedded RomFS byte comparison: PASS; all 3281 files
GitHub source SHA: VERIFIED on origin/feature/pokebank-playable
Device tested: NO
```

The exact `0ea98cc1...` predecessor physically passed artwork visibility and quality but failed the
artificial idle/breathing presentation. This replacement removes the sine-driven bob and independent
width/height modulation from active Pokémon preview paths. It does not replace the artwork or alter
the SpriteManager resolver/cache/fallback behavior. See
`docs/DEVICE_TEST_SPRITE_MOTION_FEEDBACK_2026-09-05.md`.

This documentation follows the application-source commit and is not the binary source identity.

---

Last updated: 2026-09-04

## Session 2.6 replacement — READY FOR SECOND DEVICE TEST

```text
Application source full SHA: 0ea98cc1a9f9dfc2b17abc33e944caa4aa9de915
Application commit: safety: lock installed-source UI and harden PLA reads
Embedded short SHA/version: 0ea98cc1 / 0.1.0-alpha
Branch: feature/pokebank-playable
Artifact filename: PokeBank-NX-Second-Device-0ea98cc1.nro
Artifact byte size: 155117481
Artifact SHA-256: 4c220bdf1736fb626e97c30b4ceb89fb7da7a4f24bce17c1dd36d25017478f28
ZIP fallback: PokeBank-NX-Second-Device-0ea98cc1.zip
Host tests: PASS (8 suites)
ASan/UBSan: PASS (LeakSanitizer disabled by existing host recipe)
git diff --check: PASS
Native build: PASS; clean make -j1 from exact application SHA
Compiler: devkitA64 15.2.0
Asset preflight: PASS
Embedded RomFS byte comparison: PASS; all 3281 files
PNG integrity: PASS; 3260 HD renders at 256x256
GitHub source SHA: VERIFIED
GitHub CI: PASS; run 33839339713, job 100918244541
Device tested: NO
```

This documentation follows the application-source commit; its own Git SHA is not the binary source.
CI: https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/33839339713

### Reproduction / packaging

The verified local `b072c2b022c0725b03a3d69f588a4101e5685daa` was preserved on a recovery branch.
Normal Git push lacked credentials, so the connected GitHub integration published the identical
tree (`7cde28c81e8f17bb83221f61387fefa89ab15f26`) as `0ea98cc1...`. The latter was fetched and
clean-built. No upstream push, force-push or main merge occurred.

```bash
export DEVKITPRO="$PWD/build-deps/devkitpro-root/opt/devkitpro"
export DEVKITA64="$DEVKITPRO/devkitA64"
export PATH="$DEVKITA64/bin:$DEVKITPRO/tools/bin:$PWD/build-deps/pkgconf-install/bin:$PATH"
export LD_LIBRARY_PATH="$PWD/build-deps/pkgconf-install/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make types fonts
python tools/gen_hdsprites.py
python tools/check_device_assets.py
make clean
make -j1
python tools/package_device_build.py PokeBankNX.nro --label Second-Device --zip
```

For the restored extracted toolchain, `/opt/devkitpro` points to the workspace toolchain; local
`pkg-config` and `libpkgconf.so.3` links point to the extracted image's existing binaries/libraries.
One generated host test binary lost its executable mode on runtime restoration; restoring that
generated file's executable bit allowed the local test run to finish. No application source changed.

Sprites use pinned PokeAPI commit `8dfa3d97e953caaafaafd4963eff7621811af08e`. Download scheduling
used 64 then 32 workers through the unchanged generator module; a timed-out batch was resumed.
All 3260 PNGs were validated. The NRO asset header and RomFS directory/file tables were parsed
using the installed libnx layouts, and every embedded payload was compared to its generated source
file. This proves packaging, not Switch rendering or resolution of the old PLA crash.

Packager overrides: **NONE**. A verified `/mnt/data/PokeBank-NX-FULL.bundle` preserves local refs.
The `.nro`, ZIP and generated manifest are separate durable downloads, not just build outputs.

---

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
Follow-up report: docs/DEVICE_TEST_FOLLOWUP_2026-09-03.md
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

### Extended + Sep 3 follow-up hardware evidence on same exact artifact

```text
5 MIN IDLE                        PASS
10 MIN NORMAL BROWSING            PASS
5 RELAUNCHES                      PASS
ACTION SHEET HEAVY OPEN/CLOSE     PASS (~100 opens/closes)
HELD D-PAD                        PASS
LEFT STICK SINGLE TAP             FAIL — no input/action
LEFT STICK HOLD                   FAIL — no input/action
LEFT STICK DIAGONAL               FAIL — no input/action
HOME / RESUME                     PASS
SLEEP / WAKE                      PASS
CONTROLLER RECONNECT              PASS
HANDHELD                          PASS
DOCKED                            NOT TESTED
ONE OLD LEGENDS ARCEUS SAVE       REPRODUCIBLE CRASH
USER-REACHABLE MUTATION UI        PRESENT
APP/LEGACY STORAGE PERSISTENCE    PRESENT
CROSS-GAME STORAGE VISIBILITY     PRESENT
ORIGINAL INSTALLED SAVE CHANGED   NO — tester-reported in exercised flow
LIVE INSTALLED SAVE WRITE         NOT OBSERVED
```

### Arbok / Storage transfer clarification

Exact tester-reported sequence:

```text
installed Z-A source
    ↓ automatic backup
open Z-A BACKUP representation
    ↓
move Arbok into inherited app Storage
    ↓
return to main menu
    ↓
open another supported game's backup/session
    ↓
open Storage
    ↓
Arbok persists
```

Interpretation:

```text
INSTALLED TITLE             unchanged in exercised physical check
BACKUP REPRESENTATION       mutable
LEGACY STORAGE              app-owned persistent PKSEBANK/bank.dat
OTHER GAME BACKUP           potential compatible destination
MASTER VAULT                not implemented
TRUE MOVE                   not implemented
```

The Arbok event is therefore a persistent backup-side copy/import into legacy Storage, not a live installed-save write and not product-level true Move.

The old foundation does demonstrate useful cross-game intermediate-bank UX for future PokeBank NX Transfer Workspace design.

### Additional Sep 3 physical observations

```text
sampled View data       PASS for Bulbasaur / Alolan Meowth / Mewtwo / Mew
Pokemon visual in View  MISSING in this artifact
theme torture           PASS
theme persistence       PASS after restart/sleep
bottom hint contrast    needs improvement in OLED/Dark
Dark theme              tester suggests slightly darker
Light theme             liked as-is
```

Important interpretation:

- **DEVICE TESTED** means the exact binary was physically run.
- It does not mean every tested capability passed.
- The Left Stick failure is broader than the first short report; issue #19 tracks no analog navigation at all on this artifact.
- One older Legends Arceus save reproducibly crashes; issue #24.
- Hardware reached inherited Release/Create/Move/Multi/Edit/apply-style actions; issue #23 requires clear installed-vs-backup UI behavior even though the low-level live-write hard lock appears intact.
- Legacy/app Storage is physically writable/persistent and is not the future Master Vault; issue #27.
- Sep 3 tester feedback supports that the installed Z-A source remained unchanged in the exercised Storage flow.
- Missing View artwork is likely related to generated gitignored RomFS resources; #25/#37 track visual presentation and device asset gating.

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

Originally this checkpoint was waiting only for a clean exact-source rebuild and artifact hash. First-build hardware testing changed the gate.

Before a second artifact is handed over, the next session must:

```text
#23 verify/classify mutation paths + block ambiguous installed-source mutation UI
#24 harden the PLA old/malformed-save crash path
preserve #19 analog source fix
preserve #13/#16 visible PokeBank NX shell/identity
#37 ensure required generated visual assets are present
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
#23  OPEN — inherited mutation UI safety/UI contract; second-device blocker
#24  OPEN — old/malformed PLA crash; second-device blocker
#16  OPEN — visible identity improved; full startup/icon/NACP remains
#25  OPEN — Pokémon visual Summary/View later
#26  OPEN — controller normalization later
#27  OPEN — legacy Storage vs Master Vault migration/clarification
#35  OPEN — Pokémon cry feature later
#37  OPEN — device build visual asset gate
```

---

## Device artifact tooling

Added before the second artifact:

```text
tools/check_device_assets.py
tools/package_device_build.py
docs/DEVICE_BUILD_ASSET_GATE.md
docs/DEVICE_ARTIFACT_PACKAGING.md
```

Preferred future device artifact flow:

```text
application source commit
        ↓
host tests + sanitizers
        ↓
native .nro build
        ↓
asset preflight
        ↓
exact-source package helper
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
Device asset preflight:
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
