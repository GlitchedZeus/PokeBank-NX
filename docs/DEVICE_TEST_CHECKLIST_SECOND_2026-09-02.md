# PokeBank NX — Second Device Test Checklist

Prepared: 2026-09-02; exact replacement identity recorded 2026-09-04

Use this **only after Session 2.6 produces a new exact replacement `.nro`**.

Do not fill in the artifact identity by guessing. Copy the exact application-source SHA, filename, byte size and SHA-256 from `docs/BUILD_RECORD.md` / the Session 2.6 handoff.

## Exact artifact identity

```text
Application source full SHA: 0ea98cc1a9f9dfc2b17abc33e944caa4aa9de915
Application source commit:   safety: lock installed-source UI and harden PLA reads
Embedded short SHA/version:  0ea98cc1 / 0.1.0-alpha
Artifact filename:            PokeBank-NX-Second-Device-0ea98cc1.nro
Artifact size:                155117481 bytes
Artifact SHA-256:             4c220bdf1736fb626e97c30b4ceb89fb7da7a4f24bce17c1dd36d25017478f28
Host tests:                   PASS (8 suites)
ASan/UBSan:                   PASS
Native build:                 PASS (clean exact-source build)
git diff --check:              PASS
Device asset preflight:        PASS
Embedded RomFS comparison:    PASS (3281 matching files, including 3260 HD sprites)
GitHub application CI:         PASS (run 33839339713)
Device tested:                NO
```

## Additional physical artwork feedback for the exact artifact above

Recorded separately in `docs/DEVICE_TEST_SPRITE_MOTION_FEEDBACK_2026-09-05.md`:

```text
POKEMON ARTWORK:                         PASS
ARTWORK QUALITY:                        GOOD
ARTIFICIAL IDLE / BREATHING MOTION:     FAIL / REMOVE
```

This feedback belongs to the exact `0ea98cc1...` binary. Do not transfer its device-tested result
to a later static-render replacement artifact.

Pre-test status must read:

```text
READY FOR SECOND DEVICE TEST
NOT DEVICE TESTED
```

---

# A. Launch / product identity

```text
BOOT:                                      ____
VISIBLY POKEBANK NX ON NORMAL PATH:        ____
OBVIOUS PKSE PRODUCT BRANDING GONE:        ____
POKEBANK NX NRO/TITLE IDENTITY:            ____
NO MISSING-RESOURCE CRASH:                 ____
```

Notes:

---

# B. Core navigation regression

```text
D-PAD SINGLE TAPS:                         ____
D-PAD HELD REPEAT:                         ____
A SELECT/OPEN:                             ____
B BACK/CANCEL:                             ____
L/R:                                       ____
ZL/ZR:                                     ____
+:                                         ____
-:                                         ____
```

Notes / exact context:

---

# C. Left Stick — issue #19 acceptance

Test on Select Game and at least one Pokémon box/grid where available.

```text
LEFT STICK TAP UP:                         ____
LEFT STICK TAP DOWN:                       ____
LEFT STICK TAP LEFT:                       ____
LEFT STICK TAP RIGHT:                      ____

LEFT STICK HOLD UP:                        ____
LEFT STICK HOLD DOWN:                      ____
LEFT STICK HOLD LEFT:                      ____
LEFT STICK HOLD RIGHT:                     ____

LEFT STICK DIAGONAL STABILITY:             ____
DIRECTION CHANGES WHILE HELD:              ____
NO STUCK REPEAT AFTER RELEASE:             ____
NO UNCONTROLLED SKIPPING:                  ____
```

Failure behavior if any:

```text
[ ] no movement
[ ] moves once then stops
[ ] repeats too fast
[ ] skips unpredictably
[ ] gets stuck after release
[ ] diagonal chooses unstable direction
[ ] other
```

Notes:

---

# D. Action Sheet regression

Test Party, Boxes and app Storage where available.

Expected order:

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

```text
PARTY ACTION SHEET OPENS:                  ____
BOX ACTION SHEET OPENS:                    ____
STORAGE ACTION SHEET OPENS:                ____
ORDER CORRECT:                             ____
B/CANCEL ZERO-MUTATION:                    ____
20x OPEN/CLOSE STABLE:                     ____
NO STUCK INPUT:                            ____
```

Notes:

---

# E. Installed-game mutation safety — issue #23 acceptance

Select the first **Browse installed source** row for this test. The entire session, including the
Legacy Storage pane, is read-only. To test inherited mutable behavior separately, leave this
browser and deliberately select an **Edit backup workspace** row. Its changes target backup
files or app-owned `bank.dat`, never the installed title. The global **LIVE LOCKED** badge refers
to the installed-game write lock; it does not mean an explicitly selected backup is immutable.

Use a Pokémon loaded directly from an installed-game source.

The goal is **not** to prove a dangerous write by experimenting. The goal is to prove unsafe mutation UI is not reachable/committable.

```text
RELEASE NOT REACHABLE ON INSTALLED SOURCE: ____
CREATE NOT REACHABLE ON INSTALLED SOURCE:  ____
UNSAFE MOVE/MULTI NOT REACHABLE:           ____
EDIT CANNOT COMMIT TO INSTALLED SOURCE:    ____
NO SAVE-CHANGES PATH TO INSTALLED SOURCE:  ____
NO CONTROLLER SHORTCUT BYPASS:             ____
VIEW/BROWSE REMAINS READ ONLY:             ____
```

If Edit is intentionally unavailable on installed sources:

```text
CLEAR NOT-YET-SUPPORTED/READ-ONLY MESSAGE: ____
```

If any path appears capable of writing the installed save unexpectedly:

```text
STOP THAT PATH.
MARK: FAIL — SAFETY REGRESSION
DO NOT CONTINUE TRYING TO FORCE THE WRITE.
```

Notes:

---

# F. App/legacy Storage classification — issue #27

Only test the user-facing behavior exposed by the replacement build. Use an explicit BACKUP
WORKSPACE for writable Legacy Storage tests; initial INSTALLED SOURCE browsing blocks these edits.

```text
STORAGE OPENS:                             ____
STORAGE CLEARLY LOOKS APP-OWNED/SEPARATE:  ____
STORAGE PERSISTENCE STILL WORKS:           ____
MOVING/COPYING TO STORAGE DOES NOT
SILENTLY REMOVE INSTALLED-SOURCE POKÉMON:  ____
```

Do not confuse legacy Storage with Master Vault; Vault is not implemented yet unless the application source explicitly says otherwise.

Notes:

---

# G. Legends Arceus crash — issue #24 acceptance

Use the exact older PLA save that crashed the first binary if available.

```text
NORMAL/OTHER PLA SAVE OPENS:               ____
OLD PROBLEM PLA SAVE ATTEMPTED:            ____
OLD PROBLEM PLA SAVE CRASHES:              ____
OLD SAVE OPENS SUCCESSFULLY:                ____
OR FAILS WITH USEFUL ERROR:                 ____
CAN RETURN SAFELY AFTER ERROR:              ____
NO AUTO-REPAIR/WRITE PROMPT:                ____
```

If the old save still crashes, record the exact screen/action and stop repeating unnecessary crash loops.

Notes:

---

# H. Themes / visible shell

```text
OLED BLACK:                                ____
DARK:                                      ____
LIGHT:                                     ____
THEME PERSISTENCE AFTER RESTART:           ____
OPTIONS LOOKS POKEBANK NX:                 ____
HELP LOOKS POKEBANK NX:                    ____
ACTION SHEET LOOKS POKEBANK NX:            ____
TEXT/FOCUS READABLE IN ALL THEMES:         ____
```

Notes:

---

# I. Party / Boxes / data sanity

```text
PARTY OPENS:                               ____
PARTY COUNT/DATA LOOKS CORRECT:            ____
BOXES OPEN:                                ____
BOX COUNT/DATA LOOKS CORRECT:              ____
EMPTY SLOTS SAFE:                          ____
BOX NAVIGATION STABLE:                     ____
NO POKÉMON DISAPPEARS/CHANGES BY BROWSING: ____
```

Notes:

---

# J. Summary / View regression

The visual-model work in #25 is later unless Session 2.6 explicitly included it (it should not).

For now verify read-only data/view behavior:

```text
VIEW OPENS:                                ____
SPECIES/LEVEL LOOK CORRECT:                ____
B RETURNS SAFELY:                          ____
VIEW DOES NOT MUTATE SOURCE:               ____
```

Notes:

---

# K. Lifecycle / controller reconnect

If practical:

```text
HOME → RETURN:                             ____
SLEEP → WAKE → RETURN:                     ____
JOY-CON/CONTROLLER DISCONNECT → RECONNECT: ____
INPUT WORKS AFTER RECONNECT:               ____
NO STUCK FOCUS/REPEAT AFTER RESUME:         ____
```

Notes:

---

# L. Handheld / docked

```text
HANDHELD TESTED:                           ____
HANDHELD TEXT/FOCUS/HINTS READABLE:        ____
HANDHELD CONTENT CLIPPED:                  ____

DOCKED TESTED:                             ____
DOCKED TEXT/FOCUS/HINTS READABLE:          ____
DOCKED CONTENT CLIPPED/OVERSCAN:           ____
```

Notes:

---

# M. Original game sanity check

After read-only testing, launch at least one actual Pokémon game that was browsed if practical.

```text
GAME CHECKED:
ORIGINAL SAVE LOADS:                       ____
PARTY STILL LOOKS CORRECT:                 ____
BOXES STILL LOOK CORRECT:                  ____
POKÉMON STILL WHERE EXPECTED:              ____
UNEXPECTED CHANGE:                         ____
```

This is a user-level sanity check, not a byte-level proof.

Notes:

---

# N. Final result

```text
BOOT / STABILITY:                          ____
VISIBLE POKEBANK NX SHELL:                 ____
LEFT STICK:                                ____
D-PAD / CORE INPUT:                        ____
ACTION SHEET:                              ____
INSTALLED-SOURCE SAFETY UI:                ____
PLA OLD-SAVE HANDLING:                     ____
LEGACY STORAGE:                            ____
THEMES:                                    ____
PARTY / BOXES:                             ____
LIFECYCLE/RECONNECT:                       ____
HANDHELD:                                  ____
DOCKED:                                    ____
ORIGINAL GAME SANITY:                      ____
```

Crashes:

UI bugs:

Input bugs:

Safety concerns:

Data/save bugs:

Other weird shit:

Screenshots/photos taken:

---

# Completion rules

- Close #13 only if the exact replacement build is visibly accepted as PokeBank NX.
- Close #19 only if Left Stick single-tap/held/diagonal physical tests pass.
- Close #24 only if the exact old problem save is physically retested successfully or fails gracefully; source hardening alone is not physical proof.
- #23 may require both source audit evidence and physical confirmation that unsafe installed-source mutation UI is blocked.
- Do not call the build a full device PASS if important tested gates still fail.
- Update `PROJECT_STATUS.md`, `README.md`, `BUILD_RECORD.md`, `NEXT_SESSION_PLAN.md`, `PROJECT_MAP.md`, `V1_ROADMAP.md` and affected issues with the exact result.
