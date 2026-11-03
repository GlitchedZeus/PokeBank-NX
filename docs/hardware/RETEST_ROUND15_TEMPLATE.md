# Round 15 hardware retest checklist (template)

**Status: NOT DEVICE TESTED.** Focused Round 15 candidate retest (27 checks). It is **not** the complete 20-game release acceptance required by issue #313. The interactive offline version is `docs/hardware/round15/QuickQA-Round15.html` (same items; saves progress; exports JSON).

Round 15 is built from `fix/round15-gen15-repairs`: Round 14 repairs plus PR #338 (White 2 import/new-game gates), the Gen I–V Save Instances chooser, profile-scoped save assignment with Forget, the hard exact-game save gate, the EXP Review rows, and the B2/W2 Regi and HG/SS orb access rules. The previous candidate `d160aa42` stays frozen.

When the coordinator binds the round 15 candidate, this template is copied to `RETEST_<sha>.md` with the exact NRO identity filled in.

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-<sha>.nro` |
| Code SHA | (filled in at binding) |
| NRO SHA-256 | (filled in at binding) |
| QA-F9 audit run / job | (filled in at binding) |

Mark each check PASS, FAIL or NOT TESTED (never a failure). Back up your SD card first; original saves are never written.

## 1. Saves and profiles (P0)

### S01 · No save: A opens the save chooser, never an editor
- **Game:** Pokémon White 2 (with no White 2 save on the SD card)
- **Get there:** Product Home → White 2 card
- **Do:** Press A. Press B. Press +. Press B. Press Items on the HOME dock. Press B. Open Quick Games and choose White 2.
- **Expected:** Every route shows White 2's save chooser (Save Instances / file browser). Party, Boxes, Bag or the editor never open.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### S02 · Gen I–III first open asks for a save once
- **Game:** Pokémon Red (or any Gen I–III game you have)
- **Get there:** Product Home → Red card
- **Do:** Press A. In Save Instances pick your Red save with A. Go back Home and press A again.
- **Expected:** First time: Save Instances opens and picking your save opens the game. Second time: the game opens directly.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### S03 · Assigned save opens from every route
- **Game:** Pokémon Red (assigned in S02)
- **Get there:** Product Home → Red card
- **Do:** Press A, then B. Press +, then B. Press Items on the HOME dock, then B. Open it from Quick Games.
- **Expected:** Each route opens your Red save (the hub, the Game Workspace menu, the Bag). Never a different save.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### S04 · Forget (unassign) a save
- **Game:** Pokémon Red
- **Get there:** Red card → + → Source / Game File (or Save Instances)
- **Do:** Press − (Forget). Read the question and press Y. Back Home, press A on Red.
- **Expected:** Forget asks first; Y removes only the app's link. Red then shows the chooser again. The save file on the SD card is unchanged (same size and date).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### S05 · Profile B never sees Profile A's save
- **Game:** Pokémon Red and Pokémon Black 2
- **Get there:** Product Home, Profile A has saves assigned
- **Do:** Tap the profile icon and switch to Profile B. Press A and Items on Red. Then A and Items on Black 2.
- **Expected:** Profile B gets the chooser (or 'Matching saves belong to another profile'). Profile A's saves never open on Profile B.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### S06 · Switching back keeps Profile A's assignment
- **Game:** Pokémon Black 2
- **Get there:** Switch back to Profile A
- **Do:** Press Items on the HOME dock for Black 2. Then press A on Black 2.
- **Expected:** Items opens Black 2's Bag from A's Black 2 save (not a list of every DS save). The assignment is still there; no 'belongs to someone else' error.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### S07 · Moved or renamed save is refused, not replaced
- **Game:** Any game with an assigned save
- **Get there:** SD card, then Product Home
- **Do:** On a PC, rename the assigned save (keep a copy). Open PokeBank and press A on that game. Rename it back afterwards.
- **Expected:** The chooser opens and says the save is missing or moved. No other save is opened in its place.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### S08 · Wrong game's save is refused
- **Game:** Pokémon White and White 2 (or Black / Black 2)
- **Get there:** White card → chooser → file browser
- **Do:** In White's file browser, pick a White 2 save file.
- **Expected:** Refused with a reason; White does not open with White 2's save.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### S09 · Uninitialized emulator file is not editable
- **Game:** Any Gen IV/V game (e.g. White 2)
- **Get there:** Chooser → file browser
- **Do:** Start a new game in the emulator but quit before saving in-game, so the emulator makes an empty save. Pick that file in PokeBank.
- **Expected:** Shows 'not an initialized save'. Nothing opens and the file is unchanged.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

## 2. White 2 Create and Import

### W01 · First-boot White 2 offers no Add or Import
- **Game:** Pokémon White 2 save made before the first in-game save (no trainer yet)
- **Get there:** Boxes → empty slot
- **Do:** Press A on an empty slot. Then press X.
- **Expected:** No Add Pokémon and no Import from file. The slot explains why (no trainer yet).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### W02 · After a real save, Add and Import work
- **Game:** Pokémon White 2 after saving in-game once
- **Get there:** Boxes → empty slot
- **Do:** Press A on an empty slot (Add Pokémon is listed). Press B. Press X (Import from file).
- **Expected:** Add Pokémon is offered; X opens Import from file. A and X agree.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### W03 · Gen V Import lists only .pk5 / .ek5
- **Game:** Pokémon White 2 (or Black)
- **Get there:** Boxes → empty slot → X (Import from file)
- **Do:** Browse to a folder that also has .pk1/.pk3/.pk4 files (for example your Gen I import folder).
- **Expected:** Only .pk5 and .ek5 files are listed. Other generations' files do not appear.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

## 3. Editor and shared controls

### E01 · Footers are the same in every game
- **Game:** Red, Gold, FireRed, Platinum, Black
- **Get there:** Items (Bag) with an item highlighted
- **Do:** Read the footer in each game. Press X (Add item) and read that footer.
- **Expected:** Same words everywhere: Navigate, A Edit Amount, X Add Item, Y Remove Item, L/R Category, ZR Top Up, + Review, − Help, B Back. Add picker: Navigate, Add / Select, Cancel, Page, Sort.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### E02 · + opens Review from the game's main menu
- **Game:** Any Gen I–IV game
- **Get there:** Change one item amount, then B back to the game's main menu (hub)
- **Do:** Press +. Then press B.
- **Expected:** Review opens with your change; B returns to the main menu.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### E03 · Crystal Add needs a Met Location
- **Game:** Pokémon Crystal
- **Get there:** Boxes → empty slot → A → Add Pokémon
- **Do:** Pick a species and one move. Press B, then A (Keep / Add). Then open Location, pick a place, B, A.
- **Expected:** First A: 'Choose a Met Location before Add', nothing added. After choosing a location the Pokémon is added.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

## 4. Bag, Top Up, TMs and moves

### B01 · Top Up says how many fit
- **Game:** Pokémon Ruby or Emerald
- **Get there:** Items → ZR → Bag + PC Items → Preview
- **Do:** Read the first lines, scroll the list, press A.
- **Expected:** Lines like 'Items: 30 native slots, 5 used. Only 25 fit; N missing items won't fit'. Every item listed once with a reason. A shows a Result list.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### B02 · Gen I TM tab adds TMs
- **Game:** Pokémon Red or Blue
- **Get there:** Items → TM/HM tab → ZR → All Bag → Preview
- **Do:** Read the preview, press A, then +.
- **Expected:** The items being added are TMs/HMs, and the preview says the Bag has 20 slots shared by every category.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### B03 · Move chooser and moves still work
- **Game:** Any Gen III–V game
- **Get there:** Edit a Pokémon → Moves
- **Do:** Open a move slot, pick a move, Keep, then Review.
- **Expected:** Only learnable moves are listed; the move change shows in Review.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

## 5. EXP, species and data

### X01 · Species change keeps the level
- **Game:** Any Gen III–V game (e.g. Emerald)
- **Get there:** Edit an existing Lv 50 Rattata → Species
- **Do:** Change the species to Magikarp. Read the status line. Keep, then Review.
- **Expected:** Still Lv 50, note 'Kept Lv. 50; EXP 125000 -> 156250'. Review shows separate Species and Experience rows (no Level row).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### X02 · Gen I species change keeps the level
- **Game:** Pokémon Red
- **Get there:** Edit an existing Lv 50 Pikachu → Species
- **Do:** Change to Magikarp. Keep. Review. Export Copy.
- **Expected:** Still Lv 50; Review lists Species and Experience. Untouched Pokémon in the export are byte-identical (nothing else changed).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### X03 · Event Pokémon say access unverified
- **Game:** Pokémon Black (Victini) or Black 2 (Regirock / Registeel)
- **Get there:** Add Pokémon → Victini Lv 15 (Black) → Met Location
- **Do:** Read the Victini row and the Met evidence line. In Black 2, compare Regirock with Registeel.
- **Expected:** Victini: 'access unverified', evidence Incomplete (Liberty Pass). Black 2 Regirock is a normal row; Registeel says access unverified (Iron Key).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

## 6. Names and language

### L01 · Unsupported characters are refused
- **Game:** Pokémon FireRed (English save)
- **Get there:** Any Pokémon → Edit → OT (or Nickname)
- **Do:** On the ?123 page type A×B and press OK. Then type BOB and press OK.
- **Expected:** A×B is refused with a message and nothing changes. BOB is stored exactly.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### L02 · Name follows Language
- **Game:** Pokémon Platinum (or any Gen IV)
- **Get there:** Add Pokémon → Pikachu (no nickname) → Language
- **Do:** Change Language to Japanese, then Korean, then English.
- **Expected:** The name at the top changes to ピカチュウ, 피카츄, PIKACHU. Level and stats do not change.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

## 7. Move, Clone, Release, Review, Export

### M01 · Move and Clone
- **Game:** Any Gen I–V game
- **Get there:** Boxes
- **Do:** Y on a Pokémon, move to an empty slot, A. Then A → Clone, place the copy with A.
- **Expected:** Both land where chosen, with no pop-up; an occupied slot is never overwritten.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### M02 · Review, Discard and Export Copy
- **Game:** Any game with staged changes
- **Get there:** + Review
- **Do:** Discard (B then Y), make one change again, then Export Copy.
- **Expected:** Discard asks first and restores everything. Export Copy says 'Verified copy exported: … (original save unchanged)'.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

## 8. Touch, stick and D-pad

### T01 · Touch carry places and cancels
- **Game:** Any Gen I–V game
- **Get there:** Boxes
- **Do:** Drag a finger over 2×2 Pokémon and lift. Tap an empty cell. Pick up again and tap outside the grid.
- **Expected:** The tap places the group (no pop-up); the outside tap returns it. The box arrows only change the box.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

## 9. RetroArch quit (Needs Switch Evidence)

### R01 · RetroArch quit diagnostic
- **Game:** Pokémon Red
- **Get there:** Red card → launch the game
- **Do:** Launch Red from PokeBank, then quit RetroArch. Note where you land and any error code. Copy sdmc:/switch/PokeBank-NX/runtime/last-launch.txt and any new file in sdmc:/atmosphere/crash_reports/.
- **Expected:** Not a fix: this collects evidence. Comment with: where you landed, error code, how you start PokeBank (Homebrew Menu / Album / forwarder), and attach last-launch.txt.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
