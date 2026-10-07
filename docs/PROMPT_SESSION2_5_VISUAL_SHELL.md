POKEBANK NX — SESSION 2.5 — PHYSICAL DEVICE FIX + VISIBLE POKEBANK NX SHELL

Use HIGH reasoning.

Continue development of:

GlitchedZeus/PokeBank-NX

Development branch:

feature/pokebank-playable

PKSE is upstream only.
NEVER push PokeBank NX changes to upstream.

--------------------------------------------------
CONTEXT — FIRST PHYSICAL HARDWARE TEST IS COMPLETE
--------------------------------------------------

The exact build below has now been physically run on Nintendo Switch hardware:

Application source:
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a

Artifact:
PokeBank-NX-UI-Theme-3be4de6.nro

SHA-256:
df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a

Read first:

PROJECT_STATUS.md
docs/DEVICE_TEST_REPORT_2026-09-01.md
docs/NEXT_SESSION_PLAN.md
docs/SESSION_RUNBOOK.md
docs/CONTROLS.md
docs/UI_FLOW.md
docs/UI_STYLE_GUIDE.md
docs/ARCHITECTURE.md
docs/SAVE_SAFETY.md
docs/BUILD_RECORD.md

Inspect GitHub issues:

#13
#16
#19

Issue #8 is complete: the first exact recorded .nro has been physically tested.

Issue #13 was REOPENED after that physical test.

--------------------------------------------------
PHYSICAL TEST RESULT
--------------------------------------------------

Exact results from the tested build:

BOOT: PASS
D-PAD: PASS
LEFT STICK + HOLD: FAIL
A ACTION SHEET: PASS
B/CANCEL: PASS
L/R: PASS
ZL/ZR: PASS
+: PASS
-: PASS
OLED BLACK: PASS
DARK: PASS
LIGHT: PASS
THEME PERSISTENCE: PASS
PARTY: PASS
BOXES: PASS
STORAGE: PASS
CRASHES: NONE

Major UI finding:

The application still looks overwhelmingly like PKSE.

The user physically observed:

- PKSE visual structure remains dominant
- PKSE branding/logo is still visible
- inherited screens look essentially the same as PKSE
- the clearest evidence that this is PokeBank NX is the new + Options behavior saying PokeBank NX

This is NOT the visible product result we want.

The semantic theme/control infrastructure is useful and must be preserved, but it has not been applied broadly enough to the visible shell.

--------------------------------------------------
SESSION GOAL
--------------------------------------------------

Produce the next coherent device-test build that:

1. visibly looks like PokeBank NX immediately after launch
2. removes obvious PKSE branding/logo from the tested top-level path
3. uses the existing semantic OLED/Dark/Light system visibly
4. fixes held Left Stick navigation repeat on physical-hardware paths
5. preserves all controls/features that passed the first hardware test
6. preserves the live-write hard lock

This is a focused UI/device-fix milestone.

DO NOT start PKSM-Core in this session.

--------------------------------------------------
PRIORITY 1 — FIX ISSUE #19 LEFT STICK HOLD
--------------------------------------------------

Physical Switch result:

LEFT STICK + HOLD: FAIL

D-pad navigation passed.

Investigate the actual native input path rather than assuming the pure host repeat model proves analog behavior.

Check:

- Joy-Con/Pro Controller analog threshold handling
- deadzone thresholds
- sustained analog direction state
- whether the repeat timer receives a held direction after the first move
- whether analog is converted to an edge-only event
- diagonal jitter / dominant-axis behavior
- difference between D-pad held path and Left Stick held path

Target behavior:

- initial Left Stick direction moves once
- continued hold waits for a sensible initial delay
- then repeats predictably
- releasing stops repeat immediately
- changing direction resets/rearms sensibly
- small stick noise does not move focus
- diagonals do not flicker unpredictably between axes

Add/adjust host tests where practical, but prioritize fixing the native input path that failed on real hardware.

--------------------------------------------------
PRIORITY 2 — MAKE THE APP VISIBLY POKEBANK NX
--------------------------------------------------

Do NOT rewrite the theme engine.

Use the semantic theme/card/panel/modal/focus/hint primitives already implemented.

The goal is to APPLY them visibly.

When this new .nro boots, a tester should immediately know it is PokeBank NX rather than PKSE.

Focus this session on the top-level user-visible path.

Required visible changes where applicable:

A. APPLICATION / HEADER IDENTITY

- replace obvious user-visible PKSE product logo/name on the tested path
- display PokeBank NX identity consistently
- remove inherited PKSE branding from the main/header/select-game path
- preserve upstream/legal attribution in documentation/licenses; removing product branding does NOT mean deleting required notices

B. SELECT GAME / HOME SHELL

Follow docs/UI_STYLE_GUIDE.md.

Target:

- clear PokeBank NX header/identity
- semantic themed background
- visually distinct raised game cards
- obvious focus state
- title
- platform/source line or badge where data exists
- cleaner spacing/chrome than inherited PKSE
- controller hint bar integrated into the composition

Do not merely change colors on the old PKSE layout and call it done.

C. OPTIONS + HELP

- theme Options and Help using the shared PokeBank NX modal/panel language
- make titles/text explicitly PokeBank NX where product identity is appropriate
- keep Help read-only
- + remains contextual More/Options and must never revert to global Exit

D. ACTION SHEET

- preserve the completed Action Sheet semantics
- visibly style it through the PokeBank NX modal/focus/card language
- no mutation on open/navigation/B/Cancel

E. TOP-LEVEL GAME BROWSER CHROME

Where practical without exploding scope:

- remove obvious inherited PKSE product branding
- use semantic PokeBank NX background/header/footer/focus language
- keep Party/Boxes/Storage functional behavior intact

--------------------------------------------------
BRANDING SCOPE FOR ISSUE #16
--------------------------------------------------

Use only the user-visible branding pieces needed for this milestone.

Do not spend this whole session implementing every future startup stage.

For this pass:

- PokeBank NX visible name/identity
- remove obvious PKSE logo/name in normal tested flow
- ensure app/version/commit text identifies PokeBank NX where shown
- use an existing approved PokeBank NX asset only if it is already safely available in the repository/workspace
- if the final graphical logo/splash asset is not actually available, use a clean PokeBank NX text/wordmark treatment rather than blocking the milestone or copying copyrighted reference screenshots

Do NOT bundle Pokémon HOME screenshots or other design-reference images into the application.

Full splash/loading-stage/NACP polish can remain under issue #16 after this milestone.

--------------------------------------------------
VISUAL CONTRACT
--------------------------------------------------

Required themes remain:

OLED Black
Dark
Light

All three already passed physical testing.

DO NOT regress them.

Use semantic roles, not per-screen hard-coded theme forks.

Target visual language:

- friendly Pokémon-console utility
- clean spacing
- readable handheld text
- obvious controller focus
- card/panel hierarchy
- consistent modal language
- context-aware bottom hints
- 1280x720 first

Avoid generic neon/cyber visual styling.

Do not pixel-copy Pokémon HOME.

--------------------------------------------------
REGRESSION CONTRACT
--------------------------------------------------

The following PASSED on physical hardware and must stay working:

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

Treat regressions in these areas as blockers.

--------------------------------------------------
SAFETY
--------------------------------------------------

LIVE INSTALLED-GAME SAVE WRITING REMAINS HARD DISABLED.

Do not weaken, bypass, rename around, or expose the hard lock.

UI branding/input fixes do not need save-write code changes.

No controller shortcut may create a live installed-save mutation path.

Action Sheet opening/navigation/B/Cancel remains zero mutation.

--------------------------------------------------
TESTS
--------------------------------------------------

Run baseline before major edits:

make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
make -j1

Add/adjust regressions where practical for:

- sustained analog direction/repeat model
- neutral/deadzone behavior
- direction change reset
- + remains Options, never global Exit
- - remains Help
- theme enumeration/persistence remains safe
- Action Sheet safety remains unchanged
- live-write hard lock remains enforced

After implementation run:

make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1

--------------------------------------------------
VISUAL SELF-CHECK BEFORE FINAL BUILD
--------------------------------------------------

Before declaring success, inspect the code/render path and ask:

- Would a person who knows PKSE immediately see this is PokeBank NX?
- Is obvious PKSE product branding gone from the normal tested path?
- Does Select Game look materially different, not merely recolored?
- Are Options, Help, Action Sheet and bottom hints visibly one PokeBank NX design system?
- Do OLED Black/Dark/Light still work through the same semantic system?

If the answer is no, do not close #13.

--------------------------------------------------
REQUIRED DELIVERABLE
--------------------------------------------------

Aim for:

Held Left Stick repeat: FIXED / HOST TESTED / NRO BUILDS
Visible PokeBank NX top-level shell: IMPLEMENTED
Obvious PKSE product branding in tested path: REMOVED
Select Game visual identity: IMPLEMENTED
Options/Help styling: IMPLEMENTED
Action Sheet visual integration: IMPLEMENTED
Passed hardware behaviors: REGRESSION TESTED
Host tests: PASS
ASan/UBSan: PASS
git diff --check: PASS
Native .nro: BUILDS

Update:

PROJECT_STATUS.md
docs/BUILD_RECORD.md
docs/NEXT_SESSION_PLAN.md
docs/PROJECT_MAP.md

Update issues #13, #16 and #19 accurately.

Do NOT close #13 solely because host tests pass.

#13 should close only when the resulting visible shell is coherent enough for another physical test and the implementation satisfies its revised acceptance target.

#19 should close only after the fix is implemented/tested/buildable; physical confirmation can be recorded in the replacement device test.

--------------------------------------------------
COMMIT / PUSH / ARTIFACT
--------------------------------------------------

Commit the APPLICATION SOURCE milestone first.

Push to:

origin/feature/pokebank-playable

Verify exact remote application source SHA.

If later docs/status commits follow, clearly distinguish:

APPLICATION SOURCE SHA
STATUS/DOCUMENTATION SHA

Build and preserve the new .nro.

Give me the actual downloadable artifact.

Record:

filename
application source SHA
size
SHA-256

Do not claim the replacement build is DEVICE TESTED until I physically run that exact binary.

--------------------------------------------------
STOP CONDITION
--------------------------------------------------

STOP after the coherent visible-shell + Left Stick replacement build is:

implemented
host tested
sanitizer tested
native built
committed
pushed
remote SHA verified
artifact preserved/provided

Do NOT start PKSM-Core in this session.

Report:

READY FOR SECOND DEVICE TEST

Start working immediately.