# PokeBank NX — Round 11 QUICK hardware checklist (17 checks)

**Status:** NOT DEVICE TESTED — draft, not yet bound to a distinct Round 11 build.

This is the short version of the extended Round 11 worksheet. The 17 carry-forward tests are identical in scope to the tested Round 10 checklist; use the full checklist for additional Editor, Bank and safety coverage.

- NRO filename installed: `____________`
- Build commit: `____________`
- NRO SHA-256 checked: `____________`
- Testing date: `____________`
- Switch / Atmosphère version (optional): `____________`

**Last known verified Round 10 NRO only:** `PokeBank-NX-Gen1-5-Candidate-3634d1e6.nro`; SHA-256 `b6cd0b89e25c83627558b4d3d70949431c40947c3c8cd26fe46b67bb2c9e8cd9`. This is not a Round 11 build identity.

**Instructions:** Back up SD/saves; stage changes and Export Copy only. For each Hxx replace `[ ]` with `[x]` next to exactly one of PASS / FAIL / NOT TESTED. Add notes and screenshot names to failures. No hardware checkbox is filled by host CI or Codex PR #314 (not in the NRO). Upload the edited file back to ChatGPT for review.

| Count | Number |
|---|---:|
| PASS | ____ |
| FAIL | ____ |
| NOT TESTED | ____ |
| TOTAL | 17 |

---


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

## Failures to fix first

| Test ID | Game / emulator | Severity | What failed | Screenshot |
|---|---|---|---|---|
| | | | | |
| | | | | |
| | | | | |

**Anything that could lose or overwrite a save?** `____________`

**Other notes:** `____________`