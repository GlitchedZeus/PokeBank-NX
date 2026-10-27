# Round 7 hardware retest checklist (template)

**Status: NOT DEVICE TESTED.** This is a plain Markdown file. Fill it in with any text editor, on any device,
offline. Nothing here resets, expires or needs a network. Save often; partial results are fine.

When the coordinator binds the round 7 candidate, this template is copied to `RETEST_<sha>.md` with the exact
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

Baseline: [ROUND6_RESULTS_26186464.md](ROUND6_RESULTS_26186464.md), 49 PASS / 9 FAIL / 1 NOT TESTED.
Host and CI results never fill in a box here.

Summary (fill in at the end): PASS ____ / FAIL ____ / NOT TESTED ____

---

## Group A: your 49 round 6 passes (regression check)

A quick pass is enough unless something looks different. Each title gives the round 6 checklist number.
Items marked *(restyled in round 7)* get a requested visual change in Group B or C; here, check only that
the behaviour still works.
### A01 · E03 · Origin & Legality wording (round 6 A01)
- **Steps:** In one save per generation (I–V), open a Pokémon → Actions → Origin & Legality.
- **Expected:** The row and panel say "Origin & Legality". The state is honest (never "Legality OK" for an unchecked record).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A02 · C07 · Backup deletion is protected (round 6 A02)
- **Steps:** Open Backups. Try to delete a backup, then the original save entry.
- **Expected:** Deleting a backup asks first (Y confirms, B cancels). The original save cannot be deleted.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A03 · M04 · Gen IV move Category spacing (round 6 A03)
- **Steps:** Gen IV save → Pokémon → Moves.
- **Expected:** The Category element does not crowd the type badge or the Power / Accuracy section.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A04 · E02 · Pokémon Actions order (round 6 A04)
- **Steps:** Open Actions from Boxes and from Party in each generation.
- **Expected:** The same order everywhere; Party shows View / Edit / Origin & Legality / Cancel.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A05 · R2-BUG-002 · Gen III/IV Trainer Name and Money reach Review (round 6 A05) *(restyled in round 7)*
- **Steps:** Gen III and Gen IV save → Trainer → change Name and Money → open Review → leave.
- **Expected:** Both changes are listed by name; leaving offers the four staged-exit actions.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A06 · NEW-02 · No "Touch: Back" footer (round 6 A06)
- **Steps:** Open View / Overview and a few windows.
- **Expected:** No "Touch: Back" text; tapping Back still works.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A07 · NEW-03 · Bag Remove and Key Item warning buttons (round 6 A07)
- **Steps:** Bag → select an item → Remove; also a Key Item.
- **Expected:** Coloured A / B glyph buttons inside the dialog; A removes, B cancels; no repeat in the footer.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A08 · NEW-04 · Gen II PC Move (round 6 A08)
- **Steps:** Gold / Silver / Crystal → Boxes → move a Pokémon between slots and between boxes.
- **Expected:** Pick up, drop and cancel all complete; Review lists the move.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A09 · NEW-05 · Species keyboard beside the list (round 6 A09)
- **Steps:** Create or Edit → Species → open the search keyboard.
- **Expected:** The keyboard sits under the species art; the whole list stays visible.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A10 · NEW-06 · Held Item search and Sort (round 6 A10)
- **Steps:** Held Item picker → − (Search) → type POKE, then clear; X to sort.
- **Expected:** Docked keyboard beside the results; live filtering; Sort works; B closes the keys.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A11 · NEW-07 · Crystal OT Gender inline (round 6 A11)
- **Steps:** Crystal → Edit a Pokémon → OT Gender → press A.
- **Expected:** It cycles inline; no three-option popup.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A12 · NEW-08 · Ball picker scrollbar gutter (round 6 A12)
- **Steps:** Edit → Ball.
- **Expected:** Rows stop before the scrollbar; the bar has its own space.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A13 · NEW-09 · Nature stat colours (round 6 A13)
- **Steps:** Gen III–V → a Pokémon with a non-neutral Nature → Stats; then open the Nature picker.
- **Expected:** The raised stat is green and the lowered stat red, in the stats and the picker's +10% / −10% line.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A14 · NEW-11 · Selected species name first (round 6 A14)
- **Steps:** Select a species.
- **Expected:** The left panel reads "Pikachu" or "Pikachu #25", never "25 Pikachu"; lists keep their numbers.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A15 · NEW-12 · Gen IV/V location choices (round 6 A15)
- **Steps:** Gen IV / V → Met Location for a wild-catchable species (e.g. Walrein).
- **Expected:** Wild data for the species and its ancestry first; other native histories marked unverified; nothing legitimate refused.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A16 · NEW-13 · Ability names and effects (round 6 A16)
- **Steps:** Gen III–V → Ability picker.
- **Expected:** Ability names with full effects, never "Slot 1/2"; slot restrictions respected.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A17 · NEW-15 · Create defaults (round 6 A17)
- **Steps:** Create a Pokémon in Gen I/II and in Gen III–V.
- **Expected:** DVs 0 (Gen I/II) and IVs 0 (Gen III–V); existing Pokémon unchanged.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A18 · NEW-16 · Gen V Home A opens the assigned save (round 6 A18)
- **Steps:** HOME → Gen V card → A.
- **Expected:** The assigned save opens directly, with no second save chooser.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A19 · NEW-17 · Gen V Key Items and TMs editable (round 6 A19)
- **Steps:** Gen V → Bag → Key Items and TMs & HMs → Add / Remove.
- **Expected:** Staged Add / Remove works, with the Key Item warning; no blanket view-only footer.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A20 · NEW-19 · Gen I–IV staged exit menu (round 6 A20)
- **Steps:** Stage any change → leave with B.
- **Expected:** A Review, B Continue, Y Discard & Exit, X Export Copy.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A21 · NEW-20 · Gen I–III Party Actions popup (round 6 A21)
- **Steps:** Party → A or tap a Pokémon.
- **Expected:** View / Edit / Origin & Legality / Cancel, not straight to View.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A22 · R2-REVIEWMOVE · Gen III Create with Safari Ball (round 6 A23)
- **Steps:** Gen III (FRLG and one RSE) → Create Pikachu → Ball: Safari → Keep → Review → Export → reopen the export.
- **Expected:** Safari is accepted as the caught Ball; Review names the values; the export reopens with them.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A23 · M05 · Gen V move pools *(restyled in round 6: 7B)* (round 6 A24)
- **Steps:** Gen V → Pikachu → Moves; also Wormadam, Rotom, B2W2 Kyurem.
- **Expected:** Species / form pools only (not 559 moves); Learnable / Transfer / Prior form / Preserved meanings intact.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A24 · S05 · Original saves never change (round 6 A25)
- **Steps:** Open a save, edit, export, discard; reopen the original.
- **Expected:** The original is unchanged; the edit exists only in the export.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A25 · NEW-10 · Met Location rows fit (round 6 A26)
- **Steps:** Gen III–V → Met Location; scroll the whole list.
- **Expected:** Every row shows in full, with no "…"; row 1 is the stored location.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A26 · NEW-14 · Gen V Language inline *(restyled in round 6: 7C)* (round 6 A27)
- **Steps:** Gen V → Edit → Language → A, and a completed tap.
- **Expected:** It cycles inline through supported languages, stays staged and appears by name in Review.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A27 · R2-EMUBACKUP · Backups list and open (round 6 A28)
- **Steps:** Open Backups (emulator and Switch). Open an older copy, edit Money, export, discard, reopen the copy.
- **Expected:** Full-screen list, newest first; A opens that exact copy as a staged session; the copy keeps its Money.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A28 · C01 · A selects and advances (round 6 A29)
- **Steps:** Use A across HOME, Game Browser and editors.
- **Expected:** A selects and advances normally; the assigned Gen V save opens directly.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A29 · P3-TRAINER · Trainer Play Time and Gender *(restyled in round 6: P2)* (round 6 A30) *(restyled in round 7)*
- **Steps:** Gen II–IV → Trainer → edit Play Time; Crystal / Gen III / Gen IV → Gender → Review → Export → reopen.
- **Expected:** Changes are staged, named in Review, and present only in the export; no Gender in Gen I or Gold / Silver.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A30 · 4D-HELP · Help names real controls *(restyled in round 6: P4)* (round 6 A31) *(restyled in round 7)*
- **Steps:** Open Help on the Pokémon, Items and Trainer pages in each generation.
- **Expected:** Every control named does that action there; no "+ Review" where + does something else.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A31 · L06 · Pokérus and Held Item selection (round 6 B01) *(restyled in round 7)*
- **Steps:**
  1. Gen III and Gen V: Edit → Pokérus; tap its row (also after scrolling) and press A.
  2. Gen III: Held Item picker → choose an item with A, then with a tap, at the top, middle and bottom of the list.
- **Expected:** Pokérus can be changed in Gen III and V as in Gen II / IV; a tap does what A does and a swipe only scrolls. Every Held Item choice is applied with A and with a tap, at any list position.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A32 · R2-TRAINER · Gold / Silver Trainer layout (round 6 B02) *(restyled in round 7)*
- **Steps:** Gold and Silver → Trainer; also Crystal.
- **Expected:** No overlapping rows; no "Male" label under another row; no Gender row in Gold / Silver.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A33 · B08 · Bag Review is discoverable (round 6 B03) *(restyled in round 7)*
- **Steps:** Bag in each generation → find Review without using + or the exit menu.
- **Expected:** A visible, labelled Review control in the Bag that opens Review and returns to the Bag.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### A34 · 7B · Gen V "Learnable" label (round 6 C02)
- **Steps:** Gen V → Moves → move chooser.
- **Expected:** "Learnable" uses the same OK treatment as Gen I–IV.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
### A35 · 7C · No Gen V Language toast (round 6 C03)
- **Steps:** Gen V → Edit → Language → A.
- **Expected:** The value changes inline with no extra toast.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
### A36 · 7D · Tap outside closes non-destructive popups (round 6 C04)
- **Steps:** Open a picker, Help, or a details popup; tap outside it. Then open a Discard / Delete / Remove confirmation and tap outside.
- **Expected:** Non-destructive popups close, and the tap does not also press what was underneath. Destructive confirmations stay open.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
### A37 · 7E · Save safety and backups (round 6 C05)
- **Steps:** Review the Backups screens and the export flow against PKSE habits.
- **Expected:** Nothing overwrites a source save; backups are app-owned, dated and protected.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
### A38 · 7G · Bag Review discoverability (round 6 C07) *(restyled in round 7)*
- **Steps:** Same as B03.
- **Expected:** Review is reachable from the Bag without hunting.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
### A39 · Held-item policy · Gen III held-item policy (Safari Ball / Berry Juice) (round 6 C08)
- **Steps:**
  1. Gen III → Create or Edit → Held Item → look for Safari Ball and Berry Juice.
  2. Ball → Safari.
  3. If you have a save whose Pokémon already holds one, open it.
- **Expected:** Safari Ball and Berry Juice cannot be chosen as held items; Safari is still a valid caught Ball; an existing record holding one is kept and labelled honestly; the Bag is unaffected.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
### A40 · Tap-through · Tap-outside: no tap-through (round 6 C10)
- **Steps:** Over the Boxes grid, open a popup and tap outside it on a Pokémon slot.
- **Expected:** The popup closes and the slot underneath is not opened or moved.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
### A41 · View-only notice · View mode explains instead of doing nothing (L06 follow-up) (round 6 C11)
- **Steps:** Gen III, IV and V → open a Pokémon in **View** (not Edit) → press A on Pokérus, then tap Held Item. Also press A on Origin or PID.
- **Expected:** Pokérus / Held Item say "View only: choose Edit to change …". Origin / PID say "… is read-only for this Pokémon". Nothing is staged. (Gen I/II use different screens and are not covered.)
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
### A42 · Gen V Pokérus · Gen V Pokérus on a Pokémon that already has it (L06) (round 6 C12)
- **Steps:** Gen V → Edit a Pokémon that already has Pokérus (infected or cured) → change Pokérus, cycle it back and forth → Keep → Review.
- **Expected:** Keep is accepted, Review names the Pokérus state in words, not raw numbers like "2 -> 1".
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
### A43 · Create Review · Create Review shows the new Pokémon's details (P3) (round 6 C13)
- **Steps:** In each generation, Create a Pokémon → Keep → open Review.
- **Expected:** A header "Add Pokémon: Empty → <Species> Lv n" followed by named rows: Moves, OT / ID, and where the game stores them Held Item, Ball, Nature, Gender, Ability, Language, Met. Only changed IVs/EVs/DVs, Shiny, Pokérus, Friendship appear. Gen I/II never show Nature, Ability or Ball.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
### A44 · Gen II Create moves · Gen II Create starts with level-up moves (round 6 C14)
- **Steps:** Gold, Silver or Crystal → Create → Pikachu Lv 5.
- **Expected:** Moves start as Thunder Shock and Growl (with PP), as in Gen I and III–V, and Review lists them.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
### A45 · Unreleased label · Stored unreleased held item is labelled, not deleted (round 6 C15)
- **Steps:** Gen III → open a Pokémon that already holds Safari Ball or Berry Juice (if you have one).
- **Expected:** The item shows with "(unreleased)" and is kept; it is not offered as a new choice for other Pokémon.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
### A46 · No cross-save draft · Switching saves never carries a draft over (round 6 C16)
- **Steps:** Open save A → start editing a Pokémon (do not Keep) → leave the save with Discard & Exit → open save B → open its Boxes.
- **Expected:** Save B shows no editor, picker or draft from save A; Review of B lists only B's own changes.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
### A47 · Source guard · Gen I–III export refuses a changed original (round 6 C17)
- **Steps:** Open a Gen I–III emulator save in PokeBank → make an edit → (optional, advanced) change the original save file outside the app → Export Copy.
- **Expected:** If the original changed since it was opened, the export is refused with a message; otherwise it exports normally. The original is never modified.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
### A48 · AUD-01 · Gen I–V Review consistency (round 6 D01)
- **Steps:** Stage a named move or item and a Money change in each generation; open Review.
- **Expected:** The same pages, names and before / after values in every generation; X Export Copy, Y Discard All, B Back.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
### A49 · P1-GEN1 · Gen I Review presentation (round 6 D02)
- **Steps:** Gen I → stage changes → Review; then Y Discard All, and B.
- **Expected:** Only the shared Review panel and its confirmation; no old panel or footer on top; a swipe turns one page.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
- **Screenshots sent separately:**
---

## Group B: your 9 round 6 failures (repaired in round 7)

B08 was two separate defects; they are B06 and B07 here.

### B01 · R2-SPECIAL · Gen II Special in the lower-middle STATS area (round 6 A22 / C01)
- **Steps:** Gold, Silver or Crystal → Edit a Pokémon → STATS panel. Compare with a Gen I Pokémon's STATS panel.
- **Expected:** The calculated Special Attack and Special Defense are NOT squeezed into the numeric column. They sit in the lower-middle of the centre STATS panel, presented like Gen I's calculated Special. There is still one Special DV and one Special Stat Exp; both calculated values update when you change them.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### B02 · E05 · Inventory Help and glyphs (round 6 B04)
- **Steps:** Gen I, II and III Bag → − (Help). Then Gen IV and Gen V Bag → Help.
- **Expected:** No "X Add remains available" line. Where + opens Review, Help says + is Review (not Options). Gen V Bag Help is titled "Inventory Controls", not "Game Browser Controls". Every row shows its button as a coloured glyph.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### B03 · R2-CHECK-011 · Event and Met Location descriptions (round 6 B05)
- **Steps:** Black 2 / White 2 → Met Location → scroll through Pokémon Movie, Festa, POKÉPARK, Worlds, WCS, VGE, Pokémon Center, Entralink and transfer rows.
- **Expected:** Each row's description says what that choice is (event, year or place) and how it differs from its neighbours. Rows no longer all read "Event / special location; unverified". Where the source cannot be confirmed, the row says so honestly. Encounter filtering and the compact window are unchanged.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### B04 · NEW-18 · Gen I–IV Trainer matches Gen V (round 6 B06 / C06)
- **Steps:** Open Trainer in Gen I, II, III and IV, side by side with Gen V.
- **Expected:** The same design: neutral (not yellow) values, consistent row backgrounds, no patched-in boxes. The Y Review Changes button is narrow and overlaps no row. Supported edits (Name, Money, Play Time, Gender where stored) still work; Gen I and Gold / Silver show no Gender.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### B05 · R2-REROLL · Gen II Unown letter, Shiny, DVs and Reroll (round 6 B07 / C09)
- **Steps:** Gen II → Create or Edit an Unown. Try each: Shiny on, Shiny off, choose a letter (shiny and not shiny), press Y Reroll several times.
- **Expected:** Nothing changes silently and nothing does nothing. When an action must change the letter or Shiny (only I and V can be shiny), the screen says so or asks first. Choosing a letter says whether the result will be shiny. Reroll visibly changes the DVs (and keeps the letter when it can). The letter, Shiny, DVs and sprite always agree.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### B06 · P1-RETURN (1) · Gen II Create → Review is visible (round 6 B08, defect 1)
- **Steps:** Gold, Silver or Crystal → Create an Unown (and another species) → open Review from the Create screen → press B.
- **Expected:** Review is drawn in full: no invisible screen, nothing that looks frozen. B returns to the Create screen with the draft intact.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### B07 · P1-RETURN (2) · One Review key per screen (round 6 B08, defect 2)
- **Steps:** In Party, Boxes, Trainer, Bag, Create and Edit, in each generation, read the footer, then press + and Y.
- **Expected:** Each screen opens Review with exactly the key its footer shows (Y where Y is free). + does not secretly open Review on Party or Trainer. B returns to the screen you came from.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

---

## Group C: requests and late changes

### C01 · Bag + and Help agree (round 6 B03 follow-up)
- **Steps:** Each generation's Bag: press +, then open Help.
- **Expected:** + opens Review, and Help describes + as Review. No screen offers + as both Options and Review.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### C02 · Gen V export appears in Save Backups (round 6 C12 follow-up, G1)
- **Steps:** Black / White / B2W2 → edit something → Export Copy → open Save Backups.
- **Expected:** The exported copy is listed like the other generations' exports, newest first. A opens it as a staged copy; the original and the export are never overwritten. Exports from other profiles or games are not shown or opened.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### C03 · Editor typography is the same in Gen I–V
- **Steps:** Open View and Edit for a Pokémon in each generation.
- **Expected:** DETAILS, STATS, MOVES, Origin & Legality and field labels use the same heading style as the shared Gen IV/V editor (smaller coloured section labels). Nature green/red stays.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### C04 · Quick Games closes on a tap outside (round 6 C04 follow-up)
- **Steps:** HOME → open Quick Games → tap outside the panel, including on a game card underneath.
- **Expected:** The panel closes, and the card underneath is not opened. Other ordinary popups do the same; destructive confirmations do not.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### C05 · Unown confirmations
- **Steps:** Gen II Unown: choose a letter that cannot be shiny while Shiny is on; turn Shiny on for a non-I/V letter.
- **Expected:** A confirmation explains the change (letter or Shiny) before anything is applied; B cancels and nothing changes.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### C06 · Gen V Trainer: Review button overlaps nothing
- **Steps:** Black, White, Black 2 or White 2 → Trainer. Look at the bottom rows (Badges, Game language) and the Y Review Changes button.
- **Expected:** The button sits below the last row; Badges and the Game language line are fully visible and nothing overlaps.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### C07 · Gen I/II "Origin & Legality" panel
- **Steps:** Red/Blue/Yellow and Gold/Silver/Crystal → open a Pokémon in View.
- **Expected:** The provenance panel is titled "Origin & Legality" (not "GEN I DATA" / "GEN II DATA"), in the same heading style as Gen III–V, and the title is not repeated inside the panel.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### C08 · Create Review rows look the same in Gen I–V
- **Steps:** In each generation, Create a Pokémon with moves → Keep → Review.
- **Expected:** A header "<slot> / Add Pokémon: Empty → <Species> Lv n", then named rows (Moves, OT / TID (/ SID), and the fields the game stores). No empty "Moves: (none)" rows; Gen III shows the save's trainer name as OT.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### C09 · Met Location: focused event row shows its detail line
- **Steps:** Black 2 → Edit a Pokémon → Met Location → Other histories → move through POKÉPARK, Pokémon Festa, Movie, WCS, VGE, Worlds and PC rows.
- **Expected:** Each row shows a short kind (e.g. "Event: World Championship (WCS)"); the header shows the focused row's detail with its native #id. Unknown markers say "meaning not documented"; no dates, recipients or legality are claimed. The popup stays compact.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

---

## Group D: still untested

### D01 · S06 · Export loads in an emulator (optional)
- **Steps:** Load an exported copy in an emulator as a **separate** save.
- **Expected:** The edits are present and no corruption is reported; the original is unchanged. This is optional and never means direct writing or injection.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

---

## Anything else

- **Other findings:**
- **Input trace file names:**

Every NRO is **NOT DEVICE TESTED** until the owner tests that exact file.
