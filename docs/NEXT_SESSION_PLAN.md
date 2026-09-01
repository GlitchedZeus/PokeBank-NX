# PokeBank NX — Next Session Plan

Target date: 2026-09-02  
Status: NEXT EXECUTION PLAN

This page is intentionally short. `PROJECT_STATUS.md` remains the authoritative verified status; this is the recommended split for the next two available coding blocks.

---

# Session A — primary coding block

## Objective

Produce the next **playable, device-testable** PokeBank NX milestone instead of disappearing into a large subsystem.

## Start

1. Fetch `GlitchedZeus/PokeBank-NX`.
2. Checkout `feature/pokebank-playable`.
3. Read:
   - `PROJECT_STATUS.md`
   - `docs/SESSION_RUNBOOK.md`
   - `docs/CONTROLS.md`
   - `docs/UI_FLOW.md`
   - `docs/UI_STYLE_GUIDE.md`
   - Issue #2
   - Issue #13
4. Preserve any valid work newer than the documented remote state.
5. Run host tests/sanitizers.
6. Build baseline native `.nro` before large edits when the devkitPro environment is available.

## Work order

### 1. Finish Issue #2 — Pokémon Action Sheet

A on an occupied Pokémon must open:

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

Opening/focusing/canceling performs no mutation.

Unavailable features may be visibly disabled or return a clear not-yet-supported result.

### 2. Build only the reusable UI pieces needed to make #2 clean

Prefer reusable components instead of one-off styling:

```text
Theme tokens
Focus style
Panel/card primitive
Modal/action-sheet primitive
Bottom controller hint bar
```

Support or scaffold:

```text
OLED Black
Dark
Light
```

Do not spend the entire session restyling every inherited screen before #2 works.

### 3. Apply HOME-like control conventions

Required for this milestone:

```text
D-pad / Left Stick = navigation
A                  = select/open
B                  = cancel/back
L/R                = box/tab navigation where relevant
X/Y                = contextual and labeled
+                  = Options/More where available
-                  = Help/Controls where available
```

The action sheet itself only needs the controls that make sense; do not overload the modal.

### 4. Produce fresh `.nro`

Before ending Session A:

```text
host tests PASS
sanitizers PASS
native .nro BUILDS
git diff --check PASS
PROJECT_STATUS updated
commit/push/remote SHA verified
```

Record:

```text
source commit
.nro filename
size
SHA-256
```

Most importantly: **preserve/provide the actual `.nro` for physical testing.**

Do not leave the only binary in temporary `/mnt/data` storage.

---

# Between sessions — physical Switch test

Use `docs/DEVICE_TEST_CHECKLIST.md`.

For the UI/action-sheet pass, specifically test:

```text
launch
D-pad navigation
Left Stick navigation
A opens action sheet
B cancels action sheet
no instant mutation
L/R box navigation
bottom button hints
focus visibility
OLED/Dark/Light readability if all are present
+ behavior
- Help behavior
```

Capture:

```text
screenshot/photo
exact buttons pressed
expected behavior
actual behavior
crash/error code if any
```

Only the exact tested build can become DEVICE TESTED.

---

# Session B — lunch-break block

## First priority: hardware feedback

If Session A was physically tested and problems were found:

1. reproduce from report;
2. fix the smallest coherent set;
3. host tests + sanitizers;
4. rebuild `.nro`;
5. push/verify;
6. provide replacement test build if time permits.

Do **not** ignore a device crash in order to start architecture work.

## If the device-test build is stable

Begin Issue #4:

```text
PKSM-Core Gen III integration spike
```

Focus narrowly on:

```text
PK3
Sav3
FireRed / LeafGreen GBA
read-only parse
box/party extraction
round-trip test strategy
adapter/dependency cost
```

Read:

```text
docs/PKSM_CORE_INTEGRATION.md
docs/UPSTREAM_AUDIT.md
docs/SAVE_SAFETY.md
```

Do not enable live writes.

The goal for a short Session B is an evidence-based integration decision and/or smallest working read-only spike, not a complete Gen I–VIII engine migration.

---

# Do not do tomorrow

Unless required to fix the current milestone, avoid starting all of these at once:

```text
full Master Vault implementation
full Pokédex
full legality engine
full event archive
full cross-generation transfer pipeline
live save writes
entire UI redesign in one commit
```

Those are already specified/tracked and can follow after the app has another physically testable checkpoint.

---

# Compact Session A prompt

```text
POKEBANK NX — NEXT PLAYABLE MILESTONE

Open/fetch GlitchedZeus/PokeBank-NX and checkout feature/pokebank-playable.
PKSE is upstream only; never push PokeBank NX changes there.

Read PROJECT_STATUS.md, docs/SESSION_RUNBOOK.md, docs/CONTROLS.md,
docs/UI_FLOW.md, docs/UI_STYLE_GUIDE.md, Issue #2 and Issue #13.

Run baseline host tests/sanitizers and native .nro build.

Primary task: finish the controller-first A-button Pokémon action sheet.
A on a Pokémon opens a deliberate menu and never instant-mutates.

Use Pokémon HOME-like controls and build only the reusable visual pieces needed
for this milestone: semantic theme tokens, strong focus state, action-sheet/modal,
and controller hint bar. Target OLED Black, Dark and Light themes.

+ must have a useful contextual Options/More action; on Pokémon Summary the
target behavior is Compatible Games / Transfer Compatibility.
- must open Help / Controls / Screen Info.

Keep live game-save writing hard disabled.

Before session end:
test -> sanitize -> build .nro -> git diff --check -> update PROJECT_STATUS.md
-> commit -> push -> verify remote SHA.

Preserve/provide the actual .nro and record filename, size, SHA-256 and source commit
so it can be tested on a physical Switch.

Do not start a huge later subsystem until this playable milestone is saved.
Start working immediately.
```
