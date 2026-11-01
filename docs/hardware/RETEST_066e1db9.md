# Round 12 hardware retest checklist: PokeBank-NX-Gen1-5-Candidate-066e1db9.nro

**Status: NOT DEVICE TESTED.** This is a plain Markdown file. Fill it in with any text editor, on any device,
offline. Nothing here resets, expires or needs a network. Save often; partial results are fine.

Round 12 is the **Round 11 Gen I–V repair candidate** from `fix/round11-gen15-repairs`: the fixes for the
Round 11 worksheet (`docs/hardware/ROUND11_OWNER_WORKSHEET.md`, 9 FAIL items and the extra findings), the
independent Round 11 QA findings, and your Oct 31 safety clarifications. It is **Gen I–V only**: no Gen VI–IX,
no Vault and no legality-engine code. Nothing is merged to main. The Round 10 candidate (`3634d1e6`) and its
checklist stay unchanged.

This checklist is for the bound round 12 candidate `066e1db9` (docs/qa/CANDIDATE_UNDER_AUDIT.env).
Test only the file whose SHA-256 matches the table. Every other build is not this candidate.

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-066e1db9.nro` |
| Code SHA | `066e1db9921e8b081588fb6e4ad66a739689006f` |
| Tree SHA | `6e101a8d480be7996604e0f423f3a0f986cd9a8b` |
| Build run | https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/41273304655 (run `41273304655`) |
| Artifact | `PokeBank-NX-Gen1-5-Candidate-066e1db9` (id `13070367140`) |
| NRO SHA-256 | `ba6aea09f9032fcab52b397cdd9c9d04fdf5b9b9cb4babf2bfa2f0fa7bd63c03` |
| NRO size | 177073841 bytes |
| QA-F9 audit run / job | (recorded by the coordinator after the audit passes) |

## How this round is split

1. **Group H — owner hardware testing (this is your part).** Focused checks of what Round 11 changed, with your
   own ordinary Gen I–V saves. Round 12 hardware acceptance is decided on Group H only.
2. **Group A — automated testing (the development team's part).** Synthetic and kit-save evidence, listed at
   the end. **You do not need these saves.**

## How to fill in Group H

1. Check the downloaded NRO's SHA-256 against the table before testing. Write it here: `________________`
2. Tick one result box per item by replacing `[ ]` with `[x]`:
   - **PASS**: it did what Expected says.
   - **FAIL**: it did something else. Say what in Comment.
   - **NOT TESTED / NO FIXTURE**: you skipped it, or you have no save or Pokémon for it. This is never a FAIL
     and never blocks acceptance.
3. Fill in **Game / save used**, **Comment** and **Screenshots sent separately** as you like.
4. Back up the SD card first. Never overwrite an installed or emulator source save: every edit exports to a
   separate verified copy. Loading an export in an emulator is optional and never means direct writing.
5. Please do not upload private saves to GitHub.

Host and CI results never fill in a box here.

Summary (fill in at the end): PASS ____ / FAIL ____ / NOT TESTED / NO FIXTURE ____

---

## Group H: owner hardware testing (ordinary saves)

### H01 · Install · Install, open and HOME card
- **Steps:** Copy this NRO to the SD card (back up first). Check its SHA-256. Open it from the Homebrew Menu. On Product Home, move the focus onto the selected game card and off it again, in handheld and docked mode, and with two different themes if you like.
- **Expected:** The app opens to Product Home. The region picture at the top of the game card follows the card's **rounded top corners**: no square picture corner sticks out past the card's corner or the blue focus ring, focused or not, in handheld and docked.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H02 · Quit · Leave and Launch questions
- **Steps:** On Product Home press the button that asks to quit (and, if you use it, the Launch game question). Then open a save, stage an edit and back out to the save's exit question.
- **Expected:** The quit / launch question has two plain buttons: **B Cancel** on the left (nothing happens) and **Y Quit** (or **Y Launch**). There is no A Stay button. The save exit question labels B as **Cancel**; Save & Exit / Discard / Cancel still work as before.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H03 · Move · Move: Y picks up, A places, B cancels (every generation)
- **Steps:** In one save per generation you have (I–V), on the Boxes grid pick a Pokémon up (Gen I–IV: Y; Gen V: A → Move), move to an empty slot and press **A**. Repeat once and press **B** instead. Try A on an occupied slot.
- **Expected:** A places it; B cancels with nothing changed; A on an occupied slot is refused with a message and nothing is overwritten. Y or X while carrying only shows the hint "A places it here, B cancels". The footer reads A Place / L/R Box / B Cancel.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H04 · Clone · Clone is a visual carry (every generation)
- **Steps:** Box Pokémon → A → Clone, in each generation you have. Gen V: also clone a Party Pokémon.
- **Expected:** The menu closes and you are back on the box carrying a faded "Copy". No number keypad and no menu opens. **Gen III–V:** A on any empty slot places the copy there. **Gen I/II:** boxes have no gaps, so the copy always goes **after the last Pokémon of the box you choose** (the faded copy shows exactly where), and the messages never say you can pick any empty slot. A on an occupied slot is refused; B cancels with nothing staged. The original stays where it was.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H05 · Actions · Actions order, Move row and X Export / Import
- **Steps:** A on an occupied box slot, in each generation. Then press X on an occupied slot, and X on an empty slot. A on an empty slot.
- **Expected:** Every Actions list reads View, Edit, Clone, Move, Release, Origin & Legality, Export to file, (Review), Cancel. Gen II and III now have Move on Boxes. X on an occupied slot exports it to a file; X on an empty slot imports from a file; X never opens Create. A on an empty slot still offers Add Pokémon. Gen I–III Party Actions list Review Pending Changes while changes are staged.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H06 · File browser · "-" cancels Import from file
- **Steps:** X on an empty slot (Import from file). Go into a folder, press B, then press **-**. Also open Link Game File on Product Home and press -.
- **Expected:** B goes up one folder. **-** closes the browser from anywhere (any folder, Favourites) with nothing imported or linked. The footer shows both.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H07 · Create · Create needs a Met Location choice and a move (Gen III–V)
- **Steps:** Gen III (FireRed or LeafGreen if you have one), Gen IV and Gen V: Add Pokémon on an empty slot. Pick a species that is not a wild catch in that game (FR/LG: Rayquaza or Charizard). Try Keep without choosing anything. Then open Met Location.
- **Expected:** Keep is refused with one message naming what is missing (Met Location and/or at least one move). The new draft starts with **no moves**. The Met Location list is never empty: listed catches come first (with their levels); every other location of that exact game follows, marked **"Not a listed catch; unverified"** (Trade / Egg / Event marked separately). After you choose a location and a move, Keep works. The "one move" rule is a PokeBank requirement, not a legality claim.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H08 · Crystal · Crystal Met Location list
- **Steps:** Crystal only: edit or create a Pokémon and open its Met Location / caught data.
- **Expected:** The list is never empty: catch rows first, then every Johto/Kanto landmark marked unverified. Caught data stays optional (no caught data is a normal traded Pokémon).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H09 · History · Species change keeps an existing Pokémon's history (H25)
- **Steps:** Edit an **existing** Pokémon (Gen III, IV, V or Crystal) that has Pokérus or a known Met Location. Change its species to something unrelated. Look at Origin & Legality, then Keep and open Review.
- **Expected:** Pokérus, origin game, Met Location and Met Level are **kept exactly**. If the combination now looks unusual, a **Caution** line says so (for example "Met Location is not a listed catch for <species> (history kept)") in the editor and on its Review entry. Nothing is deleted and Keep is not blocked. For a **new** (Create) draft, a species change shows a status naming what was reset (e.g. "Species changed: Pokérus cleared and Met Location needs a new choice").
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H10 · Top Up · Scope is easy to find; Gen I says its Bag is shared
- **Steps:** Bag → ZR (Top Up / Add Missing). Look at the Scope row and change it with Left/Right. Gen I–III: also try PC Items and Bag + PC Items. Gen I: Add Missing on the whole Bag when it is nearly full.
- **Expected:** SETUP opens on Scope (`< All Bag pockets >`, or in Gen I `< Whole Bag (20 slots shared) >`). Preview says how many slots were free and how many items did not fit. After A, a partial result says **"Partly staged: … N did not fit because the Bag is full (20 slots shared by every category). Nothing overflowed"**. The Bag never holds more than the game allows.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H11 · Recovery files · Selection is visible; nothing starts selected
- **Steps:** Open Recovery files from a game. Move the focus, select one removable copy, select another, deselect one. Try A on a protected copy and on a heading. Press - for Help.
- **Expected:** Focus starts on the opening game's copies. **Nothing starts selected.** A selected copy is clearly marked (coloured bar and badge), different from the focus outline. Protected copies explain why they are kept and cannot be selected; headings do nothing. The footer shows A Select / Deselect, Y Remove selected (n) only when something is selected, and B Back. Removing still asks both confirmations.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H12 · Gen II stats · Special shows Sp. Atk and Sp. Def
- **Steps:** Gen II: open a Pokémon's stats.
- **Expected:** The Special row's Calculated cell shows two numbers: Sp. Atk then Sp. Def.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H13 · Quiet edits · No toast after routine edits; Gen V Shiny inline
- **Steps:** In any generation: change the Unown letter, reroll DVs/IVs with Y, toggle Shiny. Gen V: Shiny on the PID panel.
- **Expected:** These succeed without a pop-up message (refusals and warnings still show). Gen V Shiny toggles inline like Gender, with no "Change PID?" dialog; gender, nature and ability stay the same.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H14 · Gen V names · Localized names and Korean text
- **Steps:** Gen V: open a Pokémon from a non-English game or with a non-English language (if you have one). Cycle its Language in the editor without a nickname. Any Korean Gen IV/V Pokémon: look at its name.
- **Expected:** The header shows the stored name in its own script (Japanese, Korean, accented French …), and an un-nicknamed name follows the Language. Korean text draws as Hangul, not boxes. Changing Language never changes a nickname or the OT.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H15 · Gen V quick view · Radar matches the editor
- **Steps:** Gen V Boxes: move the cursor across several Pokémon and watch the quick-view stat radar; then open one in View.
- **Expected:** The quick view uses the same radar, scale and values as the editor / View and updates as the cursor moves.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H16 · Species picker · One picture, Normal / Shiny
- **Steps:** Open the species picker in Create. Press Y (or tap the picture) to switch Normal / Shiny. Gen II: try Unown.
- **Expected:** One picture, centred, captioned NORMAL or SHINY. If shiny art is missing it says "No shiny sprite" instead of showing the normal art.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H17 · Safety · Review, Keep, Discard and Export Copy
- **Steps:** In one save, stage several of the changes above. Open Review, Discard once (answer B, then Y), stage again, then Export Copy.
- **Expected:** Review lists every staged change (including any Caution notes). Discard asks first and restores the original. Export Copy writes a separate verified file; the original save is unchanged. A Japanese Gen I/II save (if you have one) stays read-only.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H18 · Earlier rounds · Smoke check of Round 11 PASS items
- **Steps:** Briefly repeat a few things that passed in Round 11 (open each generation, edit and Keep a Pokémon, Bag Help, Gen IV Bag Remove/Add, search with L/R).
- **Expected:** They still behave as they did in Round 11.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

---

## Group A: automated testing (no owner action)

These cases are tested by the development team with synthetic fixtures and the Japanese kit saves under GCC,
GCC AddressSanitizer/UBSan and Clang. **Synthetic evidence shows the app's logic handles these inputs; it does
not prove real Switch or emulator compatibility.** Every row is NOT TESTED / NO FIXTURE on hardware unless you
choose to report it in the Comment column; that never blocks acceptance.

| Scenario | Repair | Hardware status | Comment (optional) |
| --- | --- | --- | --- |
| Met Location picker offers a choosable row for every species in Ruby, Sapphire, Emerald, FireRed, LeafGreen (1–386) and Crystal (1–251); 0 empty combinations; chosen rows survive export → reparse | QA finding 1 | NOT TESTED / NO FIXTURE | |
| Gen III / IV Create Met Location counts only an explicit pick (never "value ≠ 0") | Owner clarification 1 | NOT TESTED / NO FIXTURE | |
| Edit species change keeps Pokérus, origin, Met Location / Level byte for byte through Keep → Review → export → reparse (Gen II–V) | H25, QA finding 4 | NOT TESTED / NO FIXTURE | |
| Clone / Move into an occupied, protected or full destination is refused with no overwrite, every generation | H31, H32 | NOT TESTED / NO FIXTURE | |
| Gen I/II carry owns the staged save until place / cancel; other edits refused while carrying; Discard ends a carry | H32 | NOT TESTED / NO FIXTURE | |
| Gen I Add Missing on the 20-slot shared Bag never overflows; partial results reported | QA finding 2 | NOT TESTED / NO FIXTURE | |
| Recovery files model (randomized cases) and the H12 footer / marked rows | H12, QA finding 5 | NOT TESTED / NO FIXTURE | |
| HOME card scenery drawn as one rounded vector path (no rectangular clip); geometry checked at several card sizes | HOME card | NOT TESTED / NO FIXTURE | |
| Earlier Group A rows (Bank faults, padded Gen II, Japanese saves read-only, localized FR/LG, RTC footers, Gen IV Bag fidelity) | Rounds 8–10 | NOT TESTED / NO FIXTURE | See `RETEST_ROUND10_TEMPLATE.md` |
