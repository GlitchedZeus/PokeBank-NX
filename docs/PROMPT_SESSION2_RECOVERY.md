# PokeBank NX — Session 2 Recovery / Finish Prompt

Last updated: 2026-09-01

Copy/paste this into the coding agent when resuming the interrupted issue #13 session.

```text
POKEBANK NX — SESSION 2 CONTINUATION — RECOVER + FINISH UI/THEME MILESTONE

Use HIGH reasoning.

Continue the interrupted Session 2 work for:

GlitchedZeus/PokeBank-NX

Development branch:

feature/pokebank-playable

PKSE is upstream only.
NEVER push PokeBank NX changes to upstream.

Issue #2 — controller-first Pokémon Action Sheet — is COMPLETED/CLOSED.
Do not redo it except to fix a real regression.

The previous Session 2 ran out of usage before its issue #13 work was fully tested, native-built, committed, pushed, or turned into a new .nro.

--------------------------------------------------
FIRST PRIORITY — RECOVER INTERRUPTED LOCAL WORK
--------------------------------------------------

DO NOT reset, clean, checkout over, or discard local work before inspecting it.

First run:

pwd
git rev-parse --show-toplevel
git status
git status --short
git branch -avv
git log --all --oneline --decorate --graph -40
git reflog -40
git stash list
git worktree list

Look specifically for:

- preserved Session 2 local branch/worktree
- uncommitted UI/theme changes
- local commits not pushed
- relevant reflog entries
- stashes containing Session 2 changes

The previous agent explicitly reported continuing from a preserved Session 2 branch/worktree.

If the Session 2 work exists, continue it.

Only if it is genuinely absent should the missing milestone be reimplemented from the documented contracts.

Do not repeat the old repository-recovery investigation.

--------------------------------------------------
READ CURRENT SOURCE OF TRUTH
--------------------------------------------------

Read:

PROJECT_STATUS.md
docs/NEXT_SESSION_PLAN.md
docs/SESSION_RUNBOOK.md
docs/CONTROLS.md
docs/UI_FLOW.md
docs/UI_STYLE_GUIDE.md
docs/ARCHITECTURE.md
docs/SAVE_SAFETY.md
docs/BUILD_RECORD.md
GitHub issue #13

Preserve newer valid local work.

--------------------------------------------------
REPORTED INTERRUPTED SESSION 2 WORK
--------------------------------------------------

The interrupted session reported local implementation of:

- semantic OLED Black / Dark / Light theme foundation
- Select Game raised/focused cards
- context-aware typed bottom controller hints
- held D-pad / Left-Stick navigation repeat
- + no longer globally exits
- contextual + Options / Settings / compatibility / More behavior
- - read-only Help / Controls
- persisted theme cycling
- reusable focused-card helpers
- reusable modal helpers
- Action Sheet routing through shared primitives

Treat this list as a recovery target, NOT verified GitHub functionality.

Do not redo recovered code unless needed to fix tests/build/integration.

--------------------------------------------------
SESSION GOAL
--------------------------------------------------

Finish GitHub issue #13 as ONE coherent device-testable milestone:

HOME-style controls
+
OLED Black / Dark / Light theme foundation
+
reusable UI primitives
+
new .nro

Do NOT begin PKSM-Core, Master Vault, Pokédex, transfer engine, full Summary redesign, or live writes in this session.

--------------------------------------------------
CONTROL CONTRACT
--------------------------------------------------

Follow docs/CONTROLS.md.

D-pad
- precise navigation

Left Stick
- navigation
- held repeat/scroll

A
- Select / Open
- on Pokémon = completed Action Sheet

B
- Back / Cancel

X
- Filter / Search / contextual action

Y
- Sort / View / secondary contextual action

L / R
- previous / next box, Pokémon, or nearby tab as appropriate

ZL / ZR
- larger jumps / major navigation

+
- contextual More / Options
- MUST NOT globally exit the app
- on Pokémon Summary reserve/prefer Compatible Games / Transfer Compatibility

-
- Help / Controls / Screen Info
- read-only

Right Stick
- optional fast scrolling / secondary pane
- never required for core navigation

Stick clicks may remain unused.

Only show active controls in the bottom hint bar.

--------------------------------------------------
THEME CONTRACT
--------------------------------------------------

Required themes:

OLED Black
Dark
Light

Use one semantic theme layer rather than scattered per-screen dark/light conditionals.

Semantic roles should include equivalents of:

background
surface
surfaceRaised
surfaceSelected
textPrimary
textSecondary
textMuted
accentPrimary
accentSecondary
focusBorder
divider
success
warning
error
info

Persist the selected theme.

Invalid persisted theme values must fall back safely.

OLED Black:
- true/near-black main background
- dark raised surfaces
- strong readable focus

Dark:
- charcoal/navy foundation
- softer raised surfaces

Light:
- near-white/pastel Pokémon-style feel
- dark readable text
- obvious focus

Light must not simply be inverted Dark.

--------------------------------------------------
VISUAL MILESTONE SCOPE
--------------------------------------------------

Follow docs/UI_STYLE_GUIDE.md.

Finish only the reusable/currently-touched areas needed for this milestone:

1. semantic theme structure
2. focus/selection state
3. card/panel primitive
4. modal/action-sheet primitive
5. bottom controller hint bar
6. Select Game card shell
7. Action Sheet themed consistently
8. already-small Party/Boxes integration only if it does not expand the session

Do not attempt the full Pokédex, Vault, Banks, or professional Summary UI now.

--------------------------------------------------
SAFETY CONTRACT
--------------------------------------------------

LIVE INSTALLED-GAME SAVE WRITING REMAINS HARD DISABLED.

Do not weaken, bypass, rename around, or expose the safety lock.

UI/input/theme refactoring must not create a direct installed-save mutation path.

Opening menus, changing focus, changing themes, opening Help, filtering, sorting, viewing compatibility, and browsing Pokémon must remain non-mutating.

--------------------------------------------------
HOST REGRESSION TESTS
--------------------------------------------------

Add practical tests where logic can be isolated for:

- exactly OLED Black / Dark / Light theme enumeration
- persisted theme round-trip
- invalid persisted-theme fallback
- required semantic palette roles
- + maps to contextual More/Options instead of Exit
- - maps to Help/Screen Info
- held navigation repeat behaves predictably
- button hint model exposes only active bindings
- completed Action Sheet behavior remains intact
- Action Sheet B/Cancel remains non-mutating
- live-write hard lock remains enforced

Avoid brittle pixel-coordinate tests unless necessary.

--------------------------------------------------
VERIFY
--------------------------------------------------

Run:

make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
make -j1

Fix regressions rather than deleting the new architecture just to get a green build.

Smoke-check that:

- Select Game still works
- Action Sheet still opens
- B/Cancel is safe
- + does not unexpectedly exit
- - opens Help where implemented
- held D-pad/Left-Stick repeat works
- OLED Black/Dark/Light all route through the semantic palette
- theme preference persists
- bottom hints match actual behavior
- live-write lock remains untouched

--------------------------------------------------
COMMIT / PUSH / ARTIFACT
--------------------------------------------------

Update PROJECT_STATUS.md.

Commit the APPLICATION SOURCE milestone first.

Push to:

origin/feature/pokebank-playable

Verify the exact remote application-source SHA.

If a later status/artifact-record commit is needed, clearly distinguish:

APPLICATION SOURCE SHA
STATUS/DOCUMENTATION SHA

Build and preserve the actual device-test .nro.

Record and report:

filename
exact APPLICATION SOURCE SHA
file size
SHA-256
host tests
sanitizers
native build

Give me the actual downloadable .nro artifact.

Do not mark DEVICE TESTED.

--------------------------------------------------
STOP CONDITION
--------------------------------------------------

Once issue #13 is recovered/finished, tested, native-built, committed, pushed, remote-SHA verified, and the .nro is provided:

STOP.

Do not begin PKSM-Core in this session.

Report:

READY FOR DEVICE TEST

Start by finding and preserving the interrupted local Session 2 work.
```