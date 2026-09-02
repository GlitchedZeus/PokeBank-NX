# PokeBank NX — Session 2.5 Finish Prompt

Use **HIGH reasoning**.

Continue development of:

```text
GlitchedZeus/PokeBank-NX
```

Development branch:

```text
feature/pokebank-playable
```

PKSE is upstream only. Never push PokeBank NX changes to upstream.

## Critical continuity state

The visible-shell / physical Left Stick application source is already committed and pushed:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

GitHub host CI passed on that exact application source.

Subsequent commits on `feature/pokebank-playable` may be **documentation-only**. Do not confuse the current branch head with the application source used for the replacement device-test `.nro`.

The replacement binary must be built from exact application source:

```text
361c6f551496470db305948d702944c6ed9889c1
```

Do not redo the UI/analog implementation unless verification proves a real regression.

---

## Read first

```text
PROJECT_STATUS.md
docs/BUILD_RECORD.md
docs/DEVICE_TEST_REPORT_2026-09-01.md
docs/NEXT_SESSION_PLAN.md
docs/PROMPT_SESSION2_5_VISUAL_SHELL.md
docs/CONTROLS.md
docs/UI_FLOW.md
docs/UI_STYLE_GUIDE.md
docs/SAVE_SAFETY.md
```

Inspect issues:

```text
#13
#16
#19
```

---

## Preserve continuity

Before destructive operations:

```bash
pwd
git rev-parse --show-toplevel
git status
git status --short
git branch -avv
git log --oneline --decorate -20
git worktree list
git stash list
```

Do not reset/clean away legitimate local work.

If the branch head is later than `361c6f55` only because of documentation, that is expected.

---

## Required exact-source verification/build

Verify the application source commit exists locally/remotely.

Run host verification against the application source/worktree used for the build:

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
```

Then perform a clean native build from exact application source `361c6f55...`:

```bash
make clean
make -j1
```

or the documented equivalent required by the environment.

The final binary must embed/report the `361c6f55` source identity where the build system supports the abbreviated commit display.

Do not silently build from a later docs-only head and then label it `361c6f55`.

---

## Expected application behavior in this source checkpoint

Reported implementation includes:

```text
real libnx Left Stick position handling
analog deadzone/hysteresis
sustained held-stick repeat path
analog navigation in Select Game / Backups / Party / Boxes / Storage
visible PokeBank NX header/app identity
PokeBank NX branded chrome/cards
shared Options and Help visual treatment
matching Action Sheet styling
PokeBank NX NRO/window identity
```

Previously hardware-passing behavior must remain intact:

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
LIVE-WRITE HARD LOCK
```

---

## Package exact replacement artifact

Record the actual final `.nro`:

```text
Application source SHA:
361c6f551496470db305948d702944c6ed9889c1

Artifact filename:
<actual filename>

Embedded abbreviated SHA:
<actual value>

Size:
<bytes>

SHA-256:
<hash>
```

Use a clear device-test filename such as:

```text
PokeBank-NX-Visual-Shell-361c6f5.nro
```

if renaming the build output is appropriate.

Preserve/provide the actual `.nro`. If direct `.nro` delivery may be unreliable, additionally package a ZIP containing the `.nro` and a tiny text manifest.

Do not leave the only copy inside a temporary runtime.

---

## Update status after the exact artifact exists

Update:

```text
PROJECT_STATUS.md
docs/BUILD_RECORD.md
docs/NEXT_SESSION_PLAN.md
docs/PROJECT_MAP.md
```

Update issues #13, #16, #19 accurately.

Required issue truth:

```text
#13: keep OPEN until physical visual acceptance
#19: keep OPEN until physical held-stick acceptance
#16: keep OPEN while final splash/loading/icon/NACP work remains
```

The new binary is:

```text
HOST TESTED
NRO BUILDS
READY FOR SECOND DEVICE TEST
NOT DEVICE TESTED
```

Do not overwrite the first physical report for `3be4de6b`.

---

## Safety

**LIVE INSTALLED-GAME SAVE WRITING REMAINS HARD DISABLED.**

Do not begin true Move, live injection, Master Vault, or PKSM-Core in this continuation.

True Move is a future feature tracked by issue #20 and is intentionally blocked on adapter-specific safe-write gates.

---

## Commit/push discipline

The application-source SHA already exists and must remain:

```text
361c6f551496470db305948d702944c6ed9889c1
```

Any new commits produced by this continuation should be documentation/status/build-record changes unless an actual source regression requires a real source fix.

If application source changes, create a new application-source milestone and rebuild/hash from that new exact SHA. Do not falsely reuse `361c6f55`.

Push only to:

```text
origin/feature/pokebank-playable
```

Do not force push.

---

## Stop condition

Stop when all are true:

```text
exact-source host verification PASS
ASan/UBSan PASS
git diff --check PASS
clean native build PASS
actual .nro preserved
artifact size + SHA-256 recorded
docs/status updated
issues updated
remote docs/status SHA verified
```

Report exactly:

```text
READY FOR SECOND DEVICE TEST
```

and provide the actual artifact.

Do **not** begin PKSM-Core until the user physically tests this replacement build.
