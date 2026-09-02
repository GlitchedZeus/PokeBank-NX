# PokeBank NX — Second Device Test Checklist

Prepared: 2026-09-02

Use this **only after Session 2.6 produces a new exact replacement `.nro`**.

Do not fill in the artifact identity by guessing. Copy the exact application-source SHA, filename, byte size and SHA-256 from `docs/BUILD_RECORD.md` / the Session 2.6 handoff.

## Exact artifact identity

```text
Application source full SHA: PENDING
Application source commit:   PENDING
Embedded short SHA/version:  PENDING
Artifact filename:            PENDING
Artifact size:                PENDING
Artifact SHA-256:             PENDING
Host tests:                   PENDING
ASan/UBSan:                   PENDING
Native build:                 PENDING
```

Pre-test status must read:

```text
READY FOR SECOND DEVICE TEST
NOT DEVICE TESTED
```

---

# A. Launch / product identity

```text
BOOT:                                      PASS / FAIL / NOT TESTED
VISIBLY POKEBANK NX ON NORMAL PATH:        PASS / FAIL / NOT TESTED
OBVIOUS PKSE PRODUCT BRANDING GONE:        PASS / FAIL / NOT TESTED
POKEBANK NX NRO/TITLE IDENTITY:            PASS / FAIL / NOT TESTED
NO MISSING-RESOURCE CRASH:                 PASS / FAIL / NOT TESTED
```

Notes:

---

# B. Core navigation regression

```text
D-PAD SINGLE TAPS:                         PASS / FAIL / NOT TESTED
D-PAD HELD REPEAT:                         PASS / FAIL / NOT TESTED
A SELECT/OPEN:                             PASS / FAIL / NOT TESTED
B BACK/CANCEL:                             PASS / FAIL / NOT TESTED
L/R:                                       PASS / FAIL / PARTIAL / NOT TESTED
ZL/ZR:                                     PASS / FAIL / PARTIAL / NOT TESTED
+:                                         PASS / FAIL / NOT TESTED
-:                                         PASS / FAIL / NOT TESTED
```

Notes / exact context:

---

# C. Left Stick — issue #19 acceptance

Test on Select Game and at least one Pokémon box/grid where available.

```text
LEFT STICK TAP UP:                         PASS / FAIL / NOT TESTED
LEFT STICK TAP DOWN:                       PASS / FAIL / NOT TESTED
LEFT STICK TAP LEFT:                       PASS / FAIL / NOT TESTED
LEFT STICK TAP RIGHT:                      PASS / FAIL / NOT TESTED

LEFT STICK HOLD UP:                        PASS / FAIL / NOT TESTED
LEFT STICK HOLD DOWN:                      PASS / FAIL / NOT TESTED
LEFT STICK HOLD LEFT:                      PASS / FAIL / NOT TESTED
LEFT STICK HOLD RIGHT:                     PASS / FAIL / NOT TESTED

LEFT STICK DIAGONAL STABILITY:             PASS / FAIL / NOT TESTED
DIRECTION CHANGES WHILE HELD:              PASS / FAIL / NOT TESTED
NO STUCK REPEAT AFTER RELEASE:             PASS / FAIL / NOT TESTED
NO UNCONTROLLED SKIPPING:                  PASS / FAIL / NOT TESTED
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
PARTY ACTION SHEET OPENS:                  PASS / FAIL / NOT TESTED
BOX ACTION SHEET OPENS:                    PASS / FAIL / NOT TESTED
STORAGE ACTION SHEET OPENS:                PASS / FAIL / NOT TESTED
ORDER CORRECT:                             PASS / FAIL / NOT TESTED
B/CANCEL ZERO-MUTATION:                    PASS / FAIL / NOT TESTED
20x OPEN/CLOSE STABLE:                     PASS / FAIL / NOT TESTED
NO STUCK INPUT:                            PASS / FAIL / NOT TESTED
```

Notes:

---

# E. Installed-game mutation safety — issue #23 acceptance

Use a Pokémon loaded directly from an installed-game source.

The goal is **not** to prove a dangerous write by experimenting. The goal is to prove unsafe mutation UI is not reachable/committable.

```text
RELEASE NOT REACHABLE ON INSTALLED SOURCE: PASS / FAIL / NOT TESTED
CREATE NOT REACHABLE ON INSTALLED SOURCE:  PASS / FAIL / NOT TESTED
UNSAFE MOVE/MULTI NOT REACHABLE:           PASS / FAIL / NOT TESTED
EDIT CANNOT COMMIT TO INSTALLED SOURCE:    PASS / FAIL / NOT TESTED
NO SAVE-CHANGES PATH TO INSTALLED SOURCE:  PASS / FAIL / NOT TESTED
NO CONTROLLER SHORTCUT BYPASS:             PASS / FAIL / NOT TESTED
VIEW/BROWSE REMAINS READ ONLY:             PASS / FAIL / NOT TESTED
```

If Edit is intentionally unavailable on installed sources:

```text
CLEAR NOT-YET-SUPPORTED/READ-ONLY MESSAGE: PASS / FAIL / NOT TESTED
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

Only test the user-facing behavior exposed by the replacement build.

```text
STORAGE OPENS:                             PASS / FAIL / NOT TESTED
STORAGE CLEARLY LOOKS APP-OWNED/SEPARATE:  PASS / FAIL / PARTIAL / NOT TESTED
STORAGE PERSISTENCE STILL WORKS:           PASS / FAIL / NOT TESTED
MOVING/COPYING TO STORAGE DOES NOT
SILENTLY REMOVE INSTALLED-SOURCE POKÉMON:  PASS / FAIL / NOT TESTED
```

Do not confuse legacy Storage with Master Vault; Vault is not implemented yet unless the application source explicitly says otherwise.

Notes:

---

# G. Legends Arceus crash — issue #24 acceptance

Use the exact older PLA save that crashed the first binary if available.

```text
NORMAL/OTHER PLA SAVE OPENS:               PASS / FAIL / NOT TESTED
OLD PROBLEM PLA SAVE ATTEMPTED:            YES / NO
OLD PROBLEM PLA SAVE CRASHES:              YES / NO / NOT TESTED
OLD SAVE OPENS SUCCESSFULLY:                YES / NO / NOT TESTED
OR FAILS WITH USEFUL ERROR:                 YES / NO / NOT TESTED
CAN RETURN SAFELY AFTER ERROR:              PASS / FAIL / NOT TESTED
NO AUTO-REPAIR/WRITE PROMPT:                PASS / FAIL / NOT TESTED
```

If the old save still crashes, record the exact screen/action and stop repeating unnecessary crash loops.

Notes:

---

# H. Themes / visible shell

```text
OLED BLACK:                                PASS / FAIL / NOT TESTED
DARK:                                      PASS / FAIL / NOT TESTED
LIGHT:                                     PASS / FAIL / NOT TESTED
THEME PERSISTENCE AFTER RESTART:           PASS / FAIL / NOT TESTED
OPTIONS LOOKS POKEBANK NX:                 PASS / FAIL / NOT TESTED
HELP LOOKS POKEBANK NX:                    PASS / FAIL / NOT TESTED
ACTION SHEET LOOKS POKEBANK NX:            PASS / FAIL / NOT TESTED
TEXT/FOCUS READABLE IN ALL THEMES:         PASS / FAIL / NOT TESTED
```

Notes:

---

# I. Party / Boxes / data sanity

```text
PARTY OPENS:                               PASS / FAIL / NOT TESTED
PARTY COUNT/DATA LOOKS CORRECT:            PASS / FAIL / NOT TESTED
BOXES OPEN:                                PASS / FAIL / NOT TESTED
BOX COUNT/DATA LOOKS CORRECT:              PASS / FAIL / NOT TESTED
EMPTY SLOTS SAFE:                          PASS / FAIL / NOT TESTED
BOX NAVIGATION STABLE:                     PASS / FAIL / NOT TESTED
NO POKÉMON DISAPPEARS/CHANGES BY BROWSING: PASS / FAIL / NOT TESTED
```

Notes:

---

# J. Summary / View regression

The visual-model work in #25 is later unless Session 2.6 explicitly included it (it should not).

For now verify read-only data/view behavior:

```text
VIEW OPENS:                                PASS / FAIL / NOT TESTED
SPECIES/LEVEL LOOK CORRECT:                PASS / FAIL / NOT TESTED
B RETURNS SAFELY:                          PASS / FAIL / NOT TESTED
VIEW DOES NOT MUTATE SOURCE:               PASS / FAIL / NOT TESTED
```

Notes:

---

# K. Lifecycle / controller reconnect

If practical:

```text
HOME → RETURN:                             PASS / FAIL / NOT TESTED
SLEEP → WAKE → RETURN:                     PASS / FAIL / NOT TESTED
JOY-CON/CONTROLLER DISCONNECT → RECONNECT: PASS / FAIL / NOT TESTED
INPUT WORKS AFTER RECONNECT:               PASS / FAIL / NOT TESTED
NO STUCK FOCUS/REPEAT AFTER RESUME:         PASS / FAIL / NOT TESTED
```

Notes:

---

# L. Handheld / docked

```text
HANDHELD TESTED:                           YES / NO
HANDHELD TEXT/FOCUS/HINTS READABLE:        PASS / FAIL / NOT TESTED
HANDHELD CONTENT CLIPPED:                  YES / NO / NOT TESTED

DOCKED TESTED:                             YES / NO
DOCKED TEXT/FOCUS/HINTS READABLE:          PASS / FAIL / NOT TESTED
DOCKED CONTENT CLIPPED/OVERSCAN:           YES / NO / NOT TESTED
```

Notes:

---

# M. Original game sanity check

After read-only testing, launch at least one actual Pokémon game that was browsed if practical.

```text
GAME CHECKED:
ORIGINAL SAVE LOADS:                       PASS / FAIL / NOT TESTED
PARTY STILL LOOKS CORRECT:                 PASS / FAIL / NOT TESTED
BOXES STILL LOOK CORRECT:                  PASS / FAIL / NOT TESTED
POKÉMON STILL WHERE EXPECTED:              PASS / FAIL / NOT TESTED
UNEXPECTED CHANGE:                         YES / NO / NOT TESTED
```

This is a user-level sanity check, not a byte-level proof.

Notes:

---

# N. Final result

```text
BOOT / STABILITY:                          PASS / FAIL / PARTIAL
VISIBLE POKEBANK NX SHELL:                 PASS / FAIL / PARTIAL
LEFT STICK:                                PASS / FAIL / PARTIAL
D-PAD / CORE INPUT:                        PASS / FAIL / PARTIAL
ACTION SHEET:                              PASS / FAIL / PARTIAL
INSTALLED-SOURCE SAFETY UI:                PASS / FAIL / PARTIAL
PLA OLD-SAVE HANDLING:                     PASS / FAIL / PARTIAL / NOT TESTED
LEGACY STORAGE:                            PASS / FAIL / PARTIAL / NOT TESTED
THEMES:                                    PASS / FAIL / PARTIAL
PARTY / BOXES:                             PASS / FAIL / PARTIAL
LIFECYCLE/RECONNECT:                       PASS / FAIL / PARTIAL / NOT TESTED
HANDHELD:                                  PASS / FAIL / PARTIAL / NOT TESTED
DOCKED:                                    PASS / FAIL / PARTIAL / NOT TESTED
ORIGINAL GAME SANITY:                      PASS / FAIL / PARTIAL / NOT TESTED
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