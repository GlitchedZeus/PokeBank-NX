# PokeBank NX — Round 11 hardware test checklist (fillable draft)

**Status:** NOT DEVICE TESTED. **Not yet bound to a Round 11 NRO.** This is a user-fillable worksheet, not a claim that a Round 11 build exists or passed CI.

**Purpose:** Carry forward the 17 Round 10 hardware tests (H01–H17) and add 31 optional or expanded hardware checks for Gen I–V save/edit/file/Bank safety. Use it with the next actual candidate, or record Round 10 as the tested build if that is what you installed. Do not count deferred legality PR #314 as integrated.

## Candidate identity — fill this in before testing

- Date tested: `____________`
- Switch firmware / Atmosphère version (optional): `____________`
- NRO filename actually installed: `____________`
- NRO SHA-256 actually calculated: `____________`
- Git commit / build link (if known): `____________`
- Game/emulator save roots used (optional): `____________`
- Where screenshots or recordings are saved: `____________`

**Last independently identified Round 10 reference (NOT a Round 11 identity):** `PokeBank-NX-Gen1-5-Candidate-3634d1e6.nro`, SHA-256 `b6cd0b89e25c83627558b4d3d70949431c40947c3c8cd26fe46b67bb2c9e8cd9`; audited but not hardware-accepted. If you test this file, label your results as Round 10 build / Round 11 worksheet.

## How to fill this in

1. **Back up the SD card and saves first.** Never overwrite an emulator/source save; use staged changes and Export Copy. Work on dispensable Pokémon for Create/Clone/Move/Release-type operations.
2. Under each test tick exactly one checkbox, replacing `[ ]` with `[x]` for **PASS**, **FAIL**, or **NOT TESTED / NO FIXTURE**. For a feature genuinely unavailable in the tested candidate, mark NOT TESTED and explain.
3. If something fails, add the game, exact action, what happened, what should have happened, and screenshot/video filename. **BLOCKER** means potential data loss/corruption/crash; **MAJOR** means core functionality fails; **MINOR** means cosmetic/low-impact.
4. **Optional** tests are not required to accept the current Gen I–V hardware foundation. Do not fabricate malformed saves or simulate power cuts on personal data.
5. Upload the filled `.md` file back to ChatGPT. I can convert every marked FAIL into a reproducible GitHub-ready bug report and prioritize fixes.

## Quick summary — fill last

| Summary | Count |
|---|---:|
| PASS | ____ |
| FAIL | ____ |
| NOT TESTED / NO FIXTURE | ____ |
| Total | 48 |

**Critical/blocking failures:** `________________________`

**Overall impression / most annoying UI issue:** `________________________`

## Games you actually tested

| Gen | Game(s) | Emulator/provider | Save opened? | Notes |
|---|---|---|---|---|
| I | | | | |
| II | | | | |
| III | | | | |
| IV | | | | |
| V | | | | |

---

## Part A — Round 10 carry-forward (H01–H17)

### H01 · Install and open
**Area:** Core / Setup
**Steps:** Back up the SD. Record the actual NRO filename and SHA-256; launch from the Homebrew Menu.
**Expected:** App opens to Product Home without a crash, unexpected write request, or duplicate save instance.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H02 · One save per generation
**Area:** Core / Saves
**Steps:** Open one ordinary Gen I, II, III, IV and V save if available. Inspect trainer, party and box; exit untouched.
**Expected:** Names, party, boxes, badge count, money and trainer context are plausible and unchanged; unedited exit is clean.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H03 · Inventory Controls help fits
**Area:** Bag UI
**Steps:** Open Bag in a Gen I–III save and a Gen IV/V save. Press − for Help.
**Expected:** Inventory Controls dialog is legible with no clipped or overlapping lines.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H04 · Top Up and Add Missing preview
**Area:** Bag / Bulk
**Steps:** Bag → ZR → Top Up → This pocket → Amount 50 → Preview; cancel with B. Preview again and confirm with A. Open Review.
**Expected:** Preview lists changes/skips; B makes no change; A stages only allowed edits; Key Items and HMs never change.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H05 · Export same Pokémon twice
**Area:** Pokémon / Export
**Steps:** On one Pokémon use A → Export to file; repeat on same Pokémon.
**Expected:** Files appear under switch/PokeBank-NX/pokemon/genN; second has a distinct “ (2)” name; source save and Review unchanged.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H06 · Import exported Pokémon into an empty slot
**Area:** Pokémon / Import
**Steps:** On an empty slot in a save of the SAME generation, A → Import from file → select H05 file; inspect Review; try B navigation.
**Expected:** Imported Pokémon is staged and shown in Review; B navigates browser correctly; original source remains untouched.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H07 · Create requires Met Location
**Area:** Gen V
**Steps:** In BW/B2W2 create in an empty box slot. Try Keep before choosing Met Location, then select a location and Keep.
**Expected:** “Choose a location” appears; premature Keep refuses; picking a location permits staged creation.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H08 · Gender changes inline
**Area:** Gen V
**Steps:** Edit a species capable of either gender. Select Gender; press A twice.
**Expected:** Gender cycles without an extra Change PID dialog; status reports PID change; returning works.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H09 · Trainer Gender and Pokédex
**Area:** Gen V / Trainer
**Steps:** Open Trainer, change Gender, inspect Review; open Pokédex row.
**Expected:** Trainer / Gender appears in Review; Pokédex stays in its read-only section and opens its list.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H10 · Play Time editable
**Area:** Gen I / Trainer
**Steps:** Open Red/Blue/Yellow Trainer and stage a different Play Time; inspect Review.
**Expected:** Play Time allows supported hours up to 255 and appears as a staged change.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H11 · L/R while typing
**Area:** UI / Search
**Steps:** Open species/item search, enter text, press L then R.
**Expected:** L removes final character; R is inert; screen behind search does not move.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H12 · Recovery files cancellation
**Area:** Recovery UI
**Steps:** Home → + → Backups for an emulator game → X Recovery files; select an older copy, press Y, then cancel with B.
**Expected:** Newest save and Bank copies display as protected; B cancels and deletes NOTHING.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H13 · Japanese Gen I/II read-only
**Area:** Languages / Safety
**Steps:** If available, open a Japanese Gen I or II save and inspect names and available actions.
**Expected:** Kana renders; editors/writes remain locked; no original save mutation. Otherwise mark NOT TESTED.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H14 · Export Copy is separate
**Area:** Save / Export
**Steps:** Stage one safe change → Review → Export Copy. Inspect exported path and original source.
**Expected:** A new copy is produced; source save is byte-unchanged; no live overwrite.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H15 · Rounds 6–8 smoke check
**Area:** Regression
**Steps:** Repeat a few previously passed actions: navigation, open/exit, trainer, party/boxes and a harmless staged edit.
**Expected:** No regression compared with previously accepted behavior; note any difference.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H16 · Remove keeps later items visible
**Area:** Gen IV / Bag
**Steps:** In DP/Pt/HGSS Bag pocket with 3+ stacks, remove first or middle item. Inspect staged pocket; optionally Export Copy and open in emulator.
**Expected:** Removed stack gone; every later item remains visible, original order preserved; exported game sees all remaining stacks if tested.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H17 · Berry/TM/HM Add uses native ordering
**Area:** Gen IV / Bag
**Steps:** Add lower-numbered Berry and TM to Gen IV pockets, then add an ordinary Items item.
**Expected:** Berry/TM inserts in item-ID order; normal Items pocket appends; no unrelated stacks changed.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

---

## Part B — Expanded Round 11 smoke & safety coverage (H18–H48)

These are additional manual checks. Mark optional/unsupported cases NOT TESTED rather than FAIL if the feature is intentionally absent in this candidate.

### H18 · D-pad and joystick parity
**Area:** UI / Navigation
**Steps:** On Home, game selection, party, boxes and a long modal, navigate once with D-pad and once with joystick.
**Expected:** Both inputs reach all selectable controls; focus remains visible and never skips beyond list.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H19 · B closes nested menus
**Area:** UI / Modals
**Steps:** Open species, move, nature or item chooser; navigate a nested list and press B repeatedly.
**Expected:** B returns one layer at a time without hidden menus, focus traps or unintended edit.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H20 · Emulator/provider identification
**Area:** Save discovery
**Steps:** Open Save Instances for games you have on RetroArch/mGBA/Tico/DraStic/melonDS. Check provider labels and saves.
**Expected:** Correct game/provider shown, save instances distinct, no accidental duplicate or unrelated save attached.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H21 · Gen I DV/EV and shiny controls
**Area:** Gen I / Editor
**Steps:** Edit a safe copy or stage an edit; inspect DV/EV, derived HP DV and Y shiny action; discard.
**Expected:** HP DV derived correctly and read-only if intended; no malformed values; discard restores original.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H22 · DV column and Unown sprite
**Area:** Gen II / Editor
**Steps:** Open Gen II editor; inspect DV column; compare Unown form in box card, picker and detail if one exists.
**Expected:** DV row/column displays intact; Unown form/sprite consistent across views or discrepancy documented.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H23 · Move between boxes regression
**Area:** Gen II / Storage
**Steps:** On Gen II only, stage a move from an occupied box slot to an empty slot in another box; inspect both. Discard.
**Expected:** Destination holds correct Pokémon; source stage empty; both boxes remain consistent; discard restores.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H24 · Edit and discard
**Area:** Gen III / Editor
**Steps:** In Gen III stage species/held item/move/stat change on a dispensable Pokémon; inspect Review then discard.
**Expected:** Only intended fields change in preview; original Pokémon returns exactly after discard.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H25 · Nature stat colours and trainer fields
**Area:** Gen IV / Editor
**Steps:** Inspect Gen IV nature-influenced stat colours; edit OT/met level if supported; cancel.
**Expected:** Stat colours match positive/negative nature modifiers; fields do not overlap; cancel does not stage changes.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H26 · Legal move chooser list
**Area:** Gen V / Moves
**Steps:** On Pikachu or another familiar Gen V species, open move picker and inspect available moves and count.
**Expected:** Chooser is species-restricted, not the entire ~559-move global list; move labels/type/PP legible.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H27 · Language chooser A-cycles
**Area:** Gen V / Editor
**Steps:** In a Gen V Pokémon editor, focus Language and press A repeatedly.
**Expected:** Language cycles directly; a large modal does not open; displayed language is consistent. Do not commit unsafe edits.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H28 · Changing species clears incompatible moves
**Area:** Pokémon / Species
**Steps:** On a dispensable staged Pokémon, choose another in-game species; inspect resulting move slots.
**Expected:** Old invalid moves are cleared or safely reconciled; out-of-game species absent; Review shows correct changes.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H29 · Y shiny toggle
**Area:** Pokémon / Shiny
**Steps:** Where supported, press Y on a Pokémon details/editor view; inspect shiny state and Review, then discard.
**Expected:** Shiny state toggles as intended without changing unrelated data; cancel or discard restores baseline.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H30 · Create, back out, keep
**Area:** Pokémon / Create
**Steps:** Create in empty box slot, cancel before Keep; try again and Keep; inspect Review.
**Expected:** Cancel leaves slot empty; Keep stages exactly one new Pokémon; original save not written.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H31 · Clone to available slot
**Area:** Pokémon / Clone
**Steps:** On a dispensable Pokémon, stage Clone to an empty box slot; inspect source and destination; discard.
**Expected:** Both staged copies are correct; no existing slot overwritten; discard restores baseline.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H32 · Occupied destination safety
**Area:** Pokémon / Full slots
**Steps:** Attempt Import/Move/Clone toward an occupied or unavailable destination, without confirming destructive replacement.
**Expected:** App blocks or explicitly handles collision; no silent overwrite or Pokémon loss.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H33 · Remove confirmation and cancel
**Area:** Bag / Remove
**Steps:** On a save with 3+ items, choose Remove; cancel once, then confirm; inspect Review and discard.
**Expected:** Cancel changes nothing; confirmed operation edits only intended stack; discard returns Bag to original.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H34 · Add and pocket capacity
**Area:** Bag / Add
**Steps:** Add a safe regular item within capacity; if naturally full, try Add in that pocket without deleting data.
**Expected:** Valid Add stages exactly once; naturally full pocket refuses safely, without replacing any item.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H35 · Held item and TM/HM details
**Area:** Bag / Held
**Steps:** Inspect held item chooser and Bag TM/HM entries across available generations.
**Expected:** Appropriate type, power/accuracy/PP shown where supported; no clipping; selection does not alter Bag until confirmed.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H36 · Malformed pocket safe refusal (OPTIONAL)
**Area:** Gen IV / Edge
**Steps:** Only with a *synthetic, separate, deliberately malformed* Gen IV save: attempt Bag Add/Remove.
**Expected:** App refuses with a useful message and no byte changes. Never corrupt a personal save to run this.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H37 · Gen V Bag read/navigation
**Area:** Gen V / Bag
**Steps:** Open BW/B2W2 Bag pockets; browse first/middle/last stacks. Do not rely on unverified native sorting.
**Expected:** Every pre-existing stack appears correctly; navigation works; any unexpected gap/hiding documented.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H38 · Cross-generation import refusal
**Area:** Files / Boundaries
**Steps:** On a *separate staging copy*, browse a Pokémon file from the wrong generation or unsupported extension.
**Expected:** Unsupported format is refused clearly; no destination change or original-save write.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H39 · Standalone file browser selection/cancel
**Area:** Files / Repeat
**Steps:** Open file browser, enter/leave folders with A/B, cancel an import, reopen and select a valid file.
**Expected:** Back navigation works, cancel causes no staged mutation, valid file loads in its correct format.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H40 · Bank UI opens and persists safely (if supported)
**Area:** Bank / Smoke
**Steps:** If the Bank pane is available, open it, inspect boxes, then exit without changes; optionally rename a box, Save, reopen.
**Expected:** No crash/corruption; bank boxes display; supported save/reload preserves exactly the accepted change.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H41 · Bank deposit (supported format only)
**Area:** Bank / Deposit
**Steps:** On a dispensable Pokémon from a supported game group, stage a Bank deposit if UI offers it; save Bank using intended action, reopen.
**Expected:** Deposited Pokémon persists with correct species/OT/moves; original source save remains unchanged until Export Copy.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H42 · Bank withdrawal and cancellation (if supported)
**Area:** Bank / Withdraw
**Steps:** From Bank, choose an empty supported destination, preview/stage a withdrawal; cancel once, then try again and inspect Review.
**Expected:** Cancel leaves custody unchanged; accepted staged withdrawal preserves identity and never silently overwrites occupied destination.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H43 · Gen I/II/IV/V Vault availability check
**Area:** Bank / Known limitation
**Steps:** On classic saves, inspect whether the UI actually offers native Vault/Bank deposit and withdrawal; do not convert formats to force a test.
**Expected:** Record AVAILABLE or NOT IMPLEMENTED as an observation; a missing path is a tracked v1.0 gap, not a regression in this Gen I–V candidate.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H44 · Bank and save recovery display
**Area:** Recovery / Safety
**Steps:** Inspect Recovery files only; verify latest prior generation is labelled protected. Cancel any deletion.
**Expected:** No unintentional pruning; original files unchanged. Do NOT intentionally simulate power loss on hardware.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H45 · Multiple edits → Review → Discard
**Area:** Staging / Review
**Steps:** Make two safe staged edits in a disposable save/session; inspect Review; discard; reopen.
**Expected:** Review has both changes and correct counts; discard restores both changes; no source mutation.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H46 · Export Copy reload and compare (OPTIONAL)
**Area:** Save / Verify
**Steps:** Export a harmless staged change to a separate file; if comfortable, load ONLY that copy in an emulator or compare on PC.
**Expected:** Export reparses and reflects the intended change; original source remains byte-identical; no silent unrelated edits.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H47 · Touch and controls agreement
**Area:** UI / Touch
**Steps:** Open a long list and a modal; try touchscreen scrolling/selecting and controller equivalent.
**Expected:** No modal layering, double-activation, dead zones or unintended background selection.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

### H48 · Exit and relaunch after discard
**Area:** UI / Exit
**Steps:** Discard staged edits, leave editor, exit Homebrew, relaunch and reopen same save.
**Expected:** No crash; save instances load; discarded edits absent; no unsolicited save writes.
**Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
**Game / emulator:** `____________`
**What happened / notes:** `____________`
**Severity if FAIL:** [ ] BLOCKER  [ ] MAJOR  [ ] MINOR
**Screenshot / video reference (optional):** `____________`

---

## Automated / independent work — NOT hardware results

- Round 10: independent NRO build and exact-file audit were reported PASS; they cannot fill any hardware checkbox.
- API Campaign 3: independent seeded properties for `.pk1`, `.pk3`, `.pk4`, `.pk5` and Bank; these do not mean the UI or physical SD writes were hardware-tested.
- Legality PR #314: Codex reports scoped Gen I–V rule improvements and 24 workflows/49 jobs passing, **but it is on a separate branch**. It is not part of the tested NRO, and a legal verdict must not be marked as a device success without subsequent integration and validation.
- Gen I/II Japanese editing is intentionally read-only in the current Gen I–V candidate.
- The current legacy Bank supports selected native groups (FRLG and certain Gen VII–IX groups); it is **not yet equivalent to the proposed all-generations immutable Master Vault**.
- Don’t run dangerous corruption / force-power-off scenarios on the real Switch. The development team handles synthetic negative tests.

## Failure index (optional — one line per FAIL)

| Test ID | Severity | Game | One-line symptom | Screenshot |
|---|---|---|---|---|
| | | | | |
| | | | | |
| | | | | |

## Questions or extra notes for Claude / Codex / ChatGPT

`________________________________________________________________`

`________________________________________________________________`

---

End of Round 11 fillable draft. Keep this file as your complete test record.