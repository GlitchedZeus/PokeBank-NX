# Round 14 hardware retest checklist (template)

**Status: NOT DEVICE TESTED.** This is a plain Markdown file. Fill it in with any text editor, on any device,
offline. Save often; partial results are fine.

Round 14 is the repair candidate for your Round 13 results on `9c301c48` (15 PASS / 12 FAIL,
`docs/hardware/round13/OWNER_RESULTS_9c301c48.json`). It is built from `fix/round13-gen15-repairs`. It is
**Gen I–V only**: no Gen VI–IX, no Vault and no legality engine. Nothing is merged to main, and `9c301c48`
stays frozen.

Every step below is written to be done in order, one button at a time. Each item says which Round 13 result
it re-tests. Your Round 13 numbering (H01–H13, R01–R14) is used, so you can compare directly.

When the coordinator binds the round 14 candidate, this template is copied to `RETEST_<sha>.md` with the exact
NRO identity filled in, and uploaded as a CI artifact. Until then the identity table is blank on purpose.

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-<sha>.nro` |
| Code SHA | (filled in at binding) |
| NRO SHA-256 | (filled in at binding) |
| QA-F9 audit run / job | (filled in at binding) |

## How to fill it in

1. Back up the SD card first. Check the NRO's SHA-256 matches the table. Write it here: `________________`
2. For each item, put an `x` in one box: **PASS**, **FAIL** (say what happened in Comment) or **NOT TESTED / NO
   FIXTURE** (you skipped it or have no save for it; never a FAIL).
3. Never overwrite an emulator or Switch save: every edit exports a separate copy.

Summary (fill in at the end): PASS ____ / FAIL ____ / NOT TESTED / NO FIXTURE ____

---

## Group H: Round 13 failures (each one fixed in a shared component)

### H01 · No save · A game with no save never opens the editor
- **Round 13:** H01 (FAIL) and R02 (FAIL).
- **Steps:**
  1. Make sure there is **no** Pokémon White save on the SD card.
  2. Open PokeBank. Move to the Pokémon White card.
  3. Press **A**.
  4. Press **B**.
  5. Press **+**.
  6. Press **B**.
  7. Repeat steps 2–6 on the White 2 card.
  8. Optional: on the save-file screen, press **+** and look at the list, then press **B**.
  9. Now pick your real White save on that screen and press **A**.
- **Expected:** Step 3 shows "Choose the save file for Pokémon White" (folders and files), never Party, Boxes or
  the editor. Step 4 returns to Home with nothing opened. Step 5 shows the same save-file screen (not the
  "Game Workspace" menu). Step 8 shows that game's Backups. Step 9 opens the game normally; afterwards A on White
  opens straight away and + opens the Game Workspace menu.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H02 · Met Location · Pikachu and other species show an explained list
- **Round 13:** H05 (FAIL "pikachu has weird met locations") and R06 (FAIL Gen V "all the same").
- **Steps:**
  1. Open **Black** (or White). Boxes → empty slot → A → **Add Pokémon** → **Pikachu**, level **5**.
  2. Open **Met Location** and read the grey caption line at the top of the list.
  3. Press B. Change the species to **Pidove**, level **5**. Open Met Location again and read the caption.
  4. Choose **Other location (unverified)…** and look at the first rows.
  5. Press B twice. Set the level to **30**. Open Met Location again.
  6. Optional: Ruby Pikachu Lv 5 and Lv 25, HeartGold Pikachu Lv 5.
- **Expected:** Step 2 says "Not in this game's wild / gift data; markers only" (Black/White have no Pikachu).
  Step 3 says "No catch at Lv. 5; 2 need Lv. 8+ (unverified list)". Step 4 lists the "level too low" places
  first (Route 3 first). Step 5 shows Route 3 as a normal (verified) row. Ruby Pikachu Lv 5 says 1 place needs
  Lv. 25+; at Lv 25 the Safari Zone is a verified row.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H03 · Met level · No verified catch above the Pokémon's level
- **Round 13:** H06 (FAIL, no note).
- **Steps:**
  1. Diamond (or any Gen IV): Add Pokémon → **Pidgeotto**, level **5** → Met Location.
  2. Look for Route 229 in the main list, then inside **Other location (unverified)…**.
  3. Press B. Set the level to **55**. Open Met Location again.
- **Expected:** At Lv 5, Route 229 is **not** in the main list; it is inside "Other location" marked "level too
  low". At Lv 55 it is a normal row.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H04 · Top Up · The screen says exactly how many fit, and lists what was added
- **Round 13:** H07 (FAIL "only adds some of the items").
- **Steps:**
  1. Ruby or Emerald: open **Items** (Bag). Press **ZR**.
  2. Choose **Bag + PC Items**, then go to **Preview**.
  3. Read the first highlighted lines.
  4. Scroll the list.
  5. Press **A**.
  6. Press **B**, then **+** (Review).
- **Expected:** Step 3 reads like "Items: 30 native slots, 5 used. Only 25 fit; N missing items won't fit" (the
  game's own pocket size). Step 4 lists every missing item once: added, "added to PC Items instead", or with a
  reason ("won't fit", "Key Item", "only one allowed"). No item is listed twice. Step 5 shows a **Result** list of
  what was staged and what was not. Step 6 shows the same items in Review.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H05 · TM/HM · Top Up from the TM/HM tab adds TMs and HMs
- **Round 13:** H08 (FAIL, no note).
- **Steps:**
  1. Red or Blue: open Items. Go to the **TM/HM** tab.
  2. Press **ZR** → **All Bag** → Preview. Read the first line.
  3. Press **A**, then **B**, then **+**.
  4. Gen II–V: repeat on the TM/HM pocket.
- **Expected:** In Gen I the first line says the Bag has **20 slots shared by all categories** and how many fit;
  the items being added are **TMs/HMs** (not Potions). In Gen II–V every missing TM and HM is added (HMs once).
  Only Key Items are skipped, with the reason shown.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H06 · Names by language · Exactly what to do
- **Round 13:** H10 (FAIL "don't know how to test").
- **Steps:**
  1. Open **Platinum** (or any Gen IV). Boxes → empty slot → A → Add Pokémon → **Pikachu**. Do **not** type a
     nickname.
  2. Find the **Language** row. Press A on it to change it to **Japanese**. Look at the name at the top left.
  3. Change Language to **Korean**. Look at the name again.
  4. Change Language back to **English**.
  5. Now do steps 1–4 in **Black** (Gen V) and in **FireRed** (Gen III; Gen III has no Korean).
- **Expected:** The name at the top follows the language: ピカチュウ (Japanese), 피카츄 (Korean, Gen IV/V),
  PIKACHU (English). PID, level and stats do not change. Gen III does not offer Korean.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H07 · Footers · Same buttons, same words in every game
- **Round 13:** R08 (FAIL "Gen 4 footer says Edit Quantity … Gen 5 has almost none").
- **Steps:**
  1. In **Red, Gold, FireRed, Platinum and Black**: open **Items** and highlight any item. Read the footer.
  2. Press **X** (Add item). Read the footer. Press **X** again and watch the subtitle.
  3. Press **B**. Press **−** (Help) and read it. Press B.
  4. Open **Party**, read the footer. Open **Trainer**, read the footer.
  5. Change one item amount, press **B** back to the game's main menu, press **+**.
- **Expected:** Step 1 is identical everywhere: D-pad/Stick Navigate, A Edit Amount, X Add Item, Y Remove Item,
  L/R Category, ZR Top Up, + Review, − Help, B Back. Step 2 shows Navigate, Add / Select, Cancel, Page, Sort, and
  X changes the sort (Black too). Help uses the same words. Party and Trainer footers end with B Back and − Help.
  Step 5 opens Review, and B returns to the main menu.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H08 · Crystal Create · A Met Location is needed before Add
- **Round 13:** R09 (FAIL "I didn't need to add a met location").
- **Steps:**
  1. **Crystal**: Boxes → empty slot → A → **Add Pokémon** → any species. Pick one move.
  2. Press **B**, then **A** (Keep / Add).
  3. Press B to keep editing. Open **Location** in DETAILS, pick any place, press A.
  4. Press **B**, then **A**.
  5. Repeat steps 1–2 in **Gold or Silver**.
- **Expected:** Step 2 says "Choose a Met Location before Add" and nothing is added. Step 4 adds the Pokémon.
  Gold/Silver add without asking (they store no met data).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H09 · Name safety · Unsupported characters are refused, never cut
- **Round 13:** R14 (FAIL keyboards) — partly. The shared multilingual keyboard is the next step; this round
  fixes the data-safety part.
- **Steps:**
  1. **FireRed** (English save): open an **Egg** in Boxes (or any Pokémon) → Edit → **OT**.
  2. Go to the **?123** page and type `A×B`. Press OK.
  3. Open Review.
  4. Edit the OT again, type `BOB`, press OK. Review → Export Copy.
  5. Save → New backup → name it `Run #2`. Then try `Run 2`.
- **Expected:** Step 2 is refused with a message; Review (step 3) shows no OT change. Step 4 stores `BOB` exactly.
  Step 5: `Run #2` is refused with a reason ("letters, numbers, space, - and _ only"); `Run 2` works.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

---

## Group N: new behaviour in this build (from your Nov 2 decisions)

### N01 · Touch carry · One tap places, outside tap cancels (Gen I–V)
- **Steps:** In each generation: Boxes → drag a finger across 2×2 Pokémon and lift (they are picked up). Tap a
  **box arrow**. Tap one cell in a **full** box. Tap one **empty** cell. Pick up again and tap an empty margin
  **outside** the grid. Pick up again and press **−**, then tap outside Help to close it (Gen III–V).
- **Expected:** The box arrow changes box and keeps the carry. A full box shows the same refusal as A. An empty
  cell places the group with no popup. An outside tap returns the group with no popup. Closing Help never
  places or cancels.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### N02 · Species change · An existing Pokémon keeps its level
- **Steps:** Edit an existing Lv 50 **Rattata** (Gen III–V) or **Pikachu** (Gen I/II). Change its species to
  **Magikarp** (Lapras in Gen V). Read the status line. Keep → Review → Export Copy → reopen the export.
- **Expected:** It stays **Lv 50** and says "Kept Lv. 50; EXP 125000 -> 156250 …". Review shows only the EXP
  change. After reopening it is still Lv 50. Pokérus and Met Location are unchanged.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### N03 · Eggs and Gen IV markers · Only real egg and trade histories are verified
- **Steps:** Ruby: Add Pokémon → **Pikachu** Lv 25 → Met Location. Then **Mewtwo** → Met Location → Other.
  Diamond: Add Pokémon → **Haunter** Lv 35 → Met Location; then set Lv 30.
- **Expected:** Ruby Pikachu shows "Route 117 · Hatched Day-Care egg … via Pichu" as a normal row. Mewtwo's egg
  row is only under Other, saying it never hatches. Diamond Haunter Lv 35 shows the trade row as a normal row;
  at Lv 30 it moves to Other, marked "level too low".
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### N04 · Gift, static and event Pokémon · Ticket and event encounters say "access unverified"
- **Steps:** Crystal: Add Pokémon → **Eevee** Lv 20 → Met Location. Emerald: **Lugia** Lv 70 → Met Location.
  Black: **Victini** Lv 15 → Met Location. HeartGold: **Chikorita** Lv 5 → Met Location, then look at the "Met
  evidence" line in the editor.
- **Expected:** Crystal Eevee shows "Goldenrod City — Gift · Lv. 20". Emerald Lugia shows "Navel Rock … access
  unverified" and the editor/Review say **Incomplete**. Black Victini says the same (Liberty Pass). HeartGold
  Chikorita shows "New Bark Town — Gift Lv. 5" and the evidence line says "single source (PKHeX)".
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

---

## Group P: smoke check of your Round 13 PASS items

### P01 · Earlier PASS items still work
- **Round 13:** H02, H03, H04, H09, H11, H12, H13, R01, R03, R05, R07, R10, R11, R12, R13 (all PASS).
- **Steps:** Briefly repeat a few: Gen V Y moves in Boxes; no popups when moving or cloning; Gen I/II packed
  clone has one cursor; Gen III language change keeps stats; export message disappears when you open something
  else; Review / Discard / Export Copy; controller reconnect; Unown rerolls keep the letter.
- **Expected:** They still behave as they did in Round 13.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

---

## Still not in this build (no boxes)

| Item | State |
| --- | --- |
| Shared multilingual keyboard (Japanese / Korean / Latin per game, R14) | Next step; this build only makes names safe |
| RetroArch Gen I–III crash on quit / return to PokeBank (R04) | Launcher work, not started |
| Move to Vault, Vault deposit / withdraw / search, cross-generation transfer, in-save box rename | v1.0 blockers, missing |
| Event distributions (Wonder Cards), Pokéwalker, Dream World / Dream Radar data | No data; always unverified |

## Group A: automated testing (no owner action)

All of these pass on the bound commit: 295 host test binaries (GCC), ASan/UBSan, Clang, the Round 8
compatibility matrix, classic inventory, QA contracts and matrix, 47 Python test files (including the
hardware regression gate with the Round 11 guards and the 94-item recheck), cross-gen candidate checks and the
15-file real-save round trip. **Synthetic evidence shows the logic handles these inputs; it does not prove real
Switch behaviour.** Every row is NOT TESTED / NO FIXTURE on hardware.
