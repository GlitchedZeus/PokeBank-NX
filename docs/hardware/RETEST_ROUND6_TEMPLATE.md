# Round 6 hardware retest checklist (template)

**Status: NOT DEVICE TESTED.** This is a plain Markdown file. Fill it in with any text editor, on any device,
offline. Nothing here resets, expires or needs a network. Save often; partial results are fine.

When the coordinator binds the round 6 candidate, this template is copied to `RETEST_<sha>.md` with the exact
NRO identity filled in, and uploaded as a CI artifact. Until then the identity table is blank on purpose.

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-<sha>.nro` |
| Code SHA | (filled in at binding) |
| NRO SHA-256 | (filled in at binding) |
| QA-F9 audit run / job | (filled in at binding) |

## How to use this file

1. Check the downloaded NRO's SHA-256 against the table before testing. Write it here: `________________`
2. Each item has the same fields. Tick one result box by replacing `[ ]` with `[x]`:
   - **Steps** and **Expected**: what to do and what you should see.
   - **Result**: `[ ] PASS  [ ] FAIL  [ ] NOT TESTED`.
   - **Game / save used**: e.g. `Crystal, emulator archive 2026-10-20`.
   - **Comment**: anything not PASS, or a request. A PASS can still carry a request.
   - **Screenshots sent separately**: the file names, e.g. `IMG_0412.jpg, IMG_0413.jpg`.
3. Never overwrite an installed or emulator source save. Every edit exports to a separate verified copy.
   S06 (loading an export in an emulator) is optional and never means direct writing or injection.
4. For an input problem, turn on the input trace: create `sdmc:/switch/PokeBank-NX/config/input_trace.enable`,
   reproduce, then send `sdmc:/switch/PokeBank-NX/logs/input-trace.log` and note its name below.

Baseline: [ROUND5_RESULTS_1ca2faec.md](ROUND5_RESULTS_1ca2faec.md), 31 PASS / 8 FAIL / 3 NOT TESTED.
Host and CI results never fill in a box here.

Summary (fill in at the end): PASS ____ / FAIL ____ / NOT TESTED ____

---

## Group A: your 31 round 5 passes (regression check)

A quick pass is enough unless something looks different. Items marked *(restyled in round 6)* have a
requested visual change in Group C; here, check only that the behaviour still works.

### A01 · E03 · Origin & Legality wording
- **Steps:** In one save per generation (I–V), open a Pokémon → Actions → Origin & Legality.
- **Expected:** The row and panel say "Origin & Legality". The state is honest (never "Legality OK" for an unchecked record).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A02 · C07 · Backup deletion is protected
- **Steps:** Open Backups. Try to delete a backup, then the original save entry.
- **Expected:** Deleting a backup asks first (Y confirms, B cancels). The original save cannot be deleted.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A03 · M04 · Gen IV move Category spacing
- **Steps:** Gen IV save → Pokémon → Moves.
- **Expected:** The Category element does not crowd the type badge or the Power / Accuracy section.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A04 · E02 · Pokémon Actions order
- **Steps:** Open Actions from Boxes and from Party in each generation.
- **Expected:** The same order everywhere; Party shows View / Edit / Origin & Legality / Cancel.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A05 · R2-BUG-002 · Gen III/IV Trainer Name and Money reach Review
- **Steps:** Gen III and Gen IV save → Trainer → change Name and Money → open Review → leave.
- **Expected:** Both changes are listed by name; leaving offers the four staged-exit actions.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A06 · NEW-02 · No "Touch: Back" footer
- **Steps:** Open View / Overview and a few windows.
- **Expected:** No "Touch: Back" text; tapping Back still works.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A07 · NEW-03 · Bag Remove and Key Item warning buttons
- **Steps:** Bag → select an item → Remove; also a Key Item.
- **Expected:** Coloured A / B glyph buttons inside the dialog; A removes, B cancels; no repeat in the footer.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A08 · NEW-04 · Gen II PC Move
- **Steps:** Gold / Silver / Crystal → Boxes → move a Pokémon between slots and between boxes.
- **Expected:** Pick up, drop and cancel all complete; Review lists the move.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A09 · NEW-05 · Species keyboard beside the list
- **Steps:** Create or Edit → Species → open the search keyboard.
- **Expected:** The keyboard sits under the species art; the whole list stays visible.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A10 · NEW-06 · Held Item search and Sort
- **Steps:** Held Item picker → − (Search) → type POKE, then clear; X to sort.
- **Expected:** Docked keyboard beside the results; live filtering; Sort works; B closes the keys.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A11 · NEW-07 · Crystal OT Gender inline
- **Steps:** Crystal → Edit a Pokémon → OT Gender → press A.
- **Expected:** It cycles inline; no three-option popup.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A12 · NEW-08 · Ball picker scrollbar gutter
- **Steps:** Edit → Ball.
- **Expected:** Rows stop before the scrollbar; the bar has its own space.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A13 · NEW-09 · Nature stat colours
- **Steps:** Gen III–V → a Pokémon with a non-neutral Nature → Stats; then open the Nature picker.
- **Expected:** The raised stat is green and the lowered stat red, in the stats and the picker's +10% / −10% line.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A14 · NEW-11 · Selected species name first
- **Steps:** Select a species.
- **Expected:** The left panel reads "Pikachu" or "Pikachu #25", never "25 Pikachu"; lists keep their numbers.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A15 · NEW-12 · Gen IV/V location choices
- **Steps:** Gen IV / V → Met Location for a wild-catchable species (e.g. Walrein).
- **Expected:** Wild data for the species and its ancestry first; other native histories marked unverified; nothing legitimate refused.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A16 · NEW-13 · Ability names and effects
- **Steps:** Gen III–V → Ability picker.
- **Expected:** Ability names with full effects, never "Slot 1/2"; slot restrictions respected.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A17 · NEW-15 · Create defaults
- **Steps:** Create a Pokémon in Gen I/II and in Gen III–V.
- **Expected:** DVs 0 (Gen I/II) and IVs 0 (Gen III–V); existing Pokémon unchanged.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A18 · NEW-16 · Gen V Home A opens the assigned save
- **Steps:** HOME → Gen V card → A.
- **Expected:** The assigned save opens directly, with no second save chooser.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A19 · NEW-17 · Gen V Key Items and TMs editable
- **Steps:** Gen V → Bag → Key Items and TMs & HMs → Add / Remove.
- **Expected:** Staged Add / Remove works, with the Key Item warning; no blanket view-only footer.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A20 · NEW-19 · Gen I–IV staged exit menu
- **Steps:** Stage any change → leave with B.
- **Expected:** A Review, B Continue, Y Discard & Exit, X Export Copy.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A21 · NEW-20 · Gen I–III Party Actions popup
- **Steps:** Party → A or tap a Pokémon.
- **Expected:** View / Edit / Origin & Legality / Cancel, not straight to View.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A22 · R2-SPECIAL · Gen II Special *(restyled in round 6: 7A)*
- **Steps:** Gen II → Edit → Stats → change the Special DV and the Special Stat Exp.
- **Expected:** One Special DV and one Stat Exp; calculated SpA and SpD both update and differ where the bases differ.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A23 · R2-REVIEWMOVE · Gen III Create with Safari Ball
- **Steps:** Gen III (FRLG and one RSE) → Create Pikachu → Ball: Safari → Keep → Review → Export → reopen the export.
- **Expected:** Safari is accepted as the caught Ball; Review names the values; the export reopens with them.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A24 · M05 · Gen V move pools *(restyled in round 6: 7B)*
- **Steps:** Gen V → Pikachu → Moves; also Wormadam, Rotom, B2W2 Kyurem.
- **Expected:** Species / form pools only (not 559 moves); Learnable / Transfer / Prior form / Preserved meanings intact.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A25 · S05 · Original saves never change
- **Steps:** Open a save, edit, export, discard; reopen the original.
- **Expected:** The original is unchanged; the edit exists only in the export.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A26 · NEW-10 · Met Location rows fit
- **Steps:** Gen III–V → Met Location; scroll the whole list.
- **Expected:** Every row shows in full, with no "…"; row 1 is the stored location.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A27 · NEW-14 · Gen V Language inline *(restyled in round 6: 7C)*
- **Steps:** Gen V → Edit → Language → A, and a completed tap.
- **Expected:** It cycles inline through supported languages, stays staged and appears by name in Review.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A28 · R2-EMUBACKUP · Backups list and open
- **Steps:** Open Backups (emulator and Switch). Open an older copy, edit Money, export, discard, reopen the copy.
- **Expected:** Full-screen list, newest first; A opens that exact copy as a staged session; the copy keeps its Money.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A29 · C01 · A selects and advances
- **Steps:** Use A across HOME, Game Browser and editors.
- **Expected:** A selects and advances normally; the assigned Gen V save opens directly.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A30 · P3-TRAINER · Trainer Play Time and Gender *(restyled in round 6: P2)*
- **Steps:** Gen II–IV → Trainer → edit Play Time; Crystal / Gen III / Gen IV → Gender → Review → Export → reopen.
- **Expected:** Changes are staged, named in Review, and present only in the export; no Gender in Gen I or Gold / Silver.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### A31 · 4D-HELP · Help names real controls *(restyled in round 6: P4)*
- **Steps:** Open Help on the Pokémon, Items and Trainer pages in each generation.
- **Expected:** Every control named does that action there; no "+ Review" where + does something else.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

---

## Group B: your 8 round 5 failures (repaired in round 6)

### B01 · L06 · Pokérus and Held Item selection
- **Steps:**
  1. Gen III and Gen V: Edit → Pokérus; tap its row (also after scrolling) and press A.
  2. Gen III: Held Item picker → choose an item with A, then with a tap, at the top, middle and bottom of the list.
- **Expected:** Pokérus can be changed in Gen III and V as in Gen II / IV; a tap does what A does and a swipe only scrolls. Every Held Item choice is applied with A and with a tap, at any list position.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### B02 · R2-TRAINER · Gold / Silver Trainer layout
- **Steps:** Gold and Silver → Trainer; also Crystal.
- **Expected:** No overlapping rows; no "Male" label under another row; no Gender row in Gold / Silver.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### B03 · B08 · Bag Review is discoverable
- **Steps:** Bag in each generation → find Review without using + or the exit menu.
- **Expected:** A visible, labelled Review control in the Bag that opens Review and returns to the Bag.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### B04 · E05 · Controller glyphs and footers
- **Steps:** Look at the footers and Help in HOME, Game Browser (Controls), Party, Boxes, Trainer, Bag, Create, Edit, Review and Backups.
- **Expected:** Every action shows a coloured glyph; every glyph shown does something there; Game Browser Help has no stale Review lines.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### B05 · R2-CHECK-011 · Gen V event and location rows distinguishable
- **Steps:** Black 2 / White 2 → Met Location → scroll to the POKÉPARK and Festa rows.
- **Expected:** No two rows read the same; repeated names carry what tells them apart (year, location id or source); unknown provenance says so.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### B06 · NEW-18 · Older Trainer cards match Gen V
- **Steps:** Trainer in Gen I, II, III and IV, next to Gen V.
- **Expected:** The same card design and read-only styling; no Gen II Gender overlap; nothing invented for Gen I or Gold / Silver.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### B07 · R2-REROLL · Gen II Unown letter, shiny and DVs agree
- **Steps:** Gen II → an Unown (try A) → toggle Shiny on and off; press Y to reroll a few times.
- **Expected:** The letter shown, the sprite and the DVs always agree. If making it shiny must change the letter, the screen says so (letter comes from the DVs).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### B08 · P1-RETURN · Review is easy to find and returns where it came from
- **Steps:** Open Review from Trainer, Create, Edit, Bag, Party and Boxes, in each generation; leave it with B.
- **Expected:** A visible Review prompt on each screen; B returns to the screen it was opened from.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

---

## Group C: requests attached to round 5 passes

### C01 · 7A · Gen II Special SpA / SpD placement
- **Steps:** Gen II → Edit → Stats.
- **Expected:** SpA and SpD are placed clearly; the values match A22.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C02 · 7B · Gen V "Learnable" label
- **Steps:** Gen V → Moves → move chooser.
- **Expected:** "Learnable" uses the same OK treatment as Gen I–IV.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C03 · 7C · No Gen V Language toast
- **Steps:** Gen V → Edit → Language → A.
- **Expected:** The value changes inline with no extra toast.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C04 · 7D · Tap outside closes non-destructive popups
- **Steps:** Open a picker, Help, or a details popup; tap outside it. Then open a Discard / Delete / Remove confirmation and tap outside.
- **Expected:** Non-destructive popups close, and the tap does not also press what was underneath. Destructive confirmations stay open.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C05 · 7E · Save safety and backups
- **Steps:** Review the Backups screens and the export flow against PKSE habits.
- **Expected:** Nothing overwrites a source save; backups are app-owned, dated and protected.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C06 · 7F · Gen V Trainer visual standard
- **Steps:** Compare all Trainer screens (same work as B06).
- **Expected:** One visual standard across generations.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C07 · 7G · Bag Review discoverability
- **Steps:** Same as B03.
- **Expected:** Review is reachable from the Bag without hunting.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C08 · Gen III held-item policy (Safari Ball / Berry Juice)
- **Steps:**
  1. Gen III → Create or Edit → Held Item → look for Safari Ball and Berry Juice.
  2. Ball → Safari.
  3. If you have a save whose Pokémon already holds one, open it.
- **Expected:** Safari Ball and Berry Juice cannot be chosen as held items; Safari is still a valid caught Ball; an existing record holding one is kept and labelled honestly; the Bag is unaffected.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C09 · Gen II Unown letter selector (optional request)
- **Steps:** Gen II → Unown → choose a letter.
- **Expected:** Choosing a letter picks a valid DV combination for it, keeping the shiny state you asked for where possible, and says when it cannot.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C10 · Tap-outside: no tap-through
- **Steps:** Over the Boxes grid, open a popup and tap outside it on a Pokémon slot.
- **Expected:** The popup closes and the slot underneath is not opened or moved.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C11 · View mode explains instead of doing nothing (L06 follow-up)
- **Steps:** Gen III, IV and V → open a Pokémon in **View** (not Edit) → press A on Pokérus, then tap Held Item. Also press A on Origin or PID.
- **Expected:** Pokérus / Held Item say "View only: choose Edit to change …". Origin / PID say "… is read-only for this Pokémon". Nothing is staged. (Gen I/II use different screens and are not covered.)
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C12 · Gen V Pokérus on a Pokémon that already has it (L06)
- **Steps:** Gen V → Edit a Pokémon that already has Pokérus (infected or cured) → change Pokérus, cycle it back and forth → Keep → Review.
- **Expected:** Keep is accepted, Review names the Pokérus state in words, not raw numbers like "2 -> 1".
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C13 · Create Review shows the new Pokémon's details (P3)
- **Steps:** In each generation, Create a Pokémon → Keep → open Review.
- **Expected:** A header "Add Pokémon: Empty → <Species> Lv n" followed by named rows: Moves, OT / ID, and where the game stores them Held Item, Ball, Nature, Gender, Ability, Language, Met. Only changed IVs/EVs/DVs, Shiny, Pokérus, Friendship appear. Gen I/II never show Nature, Ability or Ball.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C14 · Gen II Create starts with level-up moves
- **Steps:** Gold, Silver or Crystal → Create → Pikachu Lv 5.
- **Expected:** Moves start as Thunder Shock and Growl (with PP), as in Gen I and III–V, and Review lists them.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C15 · Stored unreleased held item is labelled, not deleted
- **Steps:** Gen III → open a Pokémon that already holds Safari Ball or Berry Juice (if you have one).
- **Expected:** The item shows with "(unreleased)" and is kept; it is not offered as a new choice for other Pokémon.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C16 · Switching saves never carries a draft over
- **Steps:** Open save A → start editing a Pokémon (do not Keep) → leave the save with Discard & Exit → open save B → open its Boxes.
- **Expected:** Save B shows no editor, picker or draft from save A; Review of B lists only B's own changes.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### C17 · Gen I–III export refuses a changed original
- **Steps:** Open a Gen I–III emulator save in PokeBank → make an edit → (optional, advanced) change the original save file outside the app → Export Copy.
- **Expected:** If the original changed since it was opened, the export is refused with a message; otherwise it exports normally. The original is never modified.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

---

## Group D: still untested

### D01 · AUD-01 · Gen I–V Review consistency
- **Steps:** Stage a named move or item and a Money change in each generation; open Review.
- **Expected:** The same pages, names and before / after values in every generation; X Export Copy, Y Discard All, B Back.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### D02 · P1-GEN1 · Gen I Review presentation
- **Steps:** Gen I → stage changes → Review; then Y Discard All, and B.
- **Expected:** Only the shared Review panel and its confirmation; no old panel or footer on top; a swipe turns one page.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

### D03 · S06 · Export loads in an emulator (optional)
- **Steps:** Load an exported copy in an emulator as a **separate** save.
- **Expected:** The edits are present and no corruption is reported; the original is unchanged.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**

---

## Anything else

- **Other findings:**
- **Input trace file names:**

Every NRO is **NOT DEVICE TESTED** until the owner tests that exact file.
