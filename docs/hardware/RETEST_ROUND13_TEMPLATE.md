# Round 13 hardware retest checklist (template)

**Status: NOT DEVICE TESTED.** This is a plain Markdown file. Fill it in with any text editor, on any device,
offline. Nothing here resets, expires or needs a network. Save often; partial results are fine.

Round 13 is the **Round 12 Gen I–V repair candidate** from `fix/round12-gen15-repairs`: the fixes for your
Round 12 results on `066e1db9` (`docs/hardware/round12/OWNER_RESULTS_066e1db9.json`, issues R12-001 to R12-010
in `docs/hardware/round12/ISSUE_LEDGER.csv`). It is **Gen I–V only**: no Gen VI–IX, no Vault and no
legality-engine code. Nothing is merged to main. The Round 12 candidate (`066e1db9`) and its checklist stay
unchanged.

Items are mapped by **issue ID and title**, never by H-number alone: your Round 12 sheet used its own
numbering, so the "Round 12 issue" line under each item says exactly which finding it re-tests. Group R
re-checks earlier requirements that silently dropped off later checklists without ever being accepted
(`docs/qa/HISTORICAL_ROUND_1_12_AUDIT.md` §5). Unless an item says otherwise, **this build has no new fix
for a Group R item**: it records what happens today.

When the coordinator binds the round 13 candidate, this template is copied to `RETEST_<sha>.md` with the exact
NRO identity filled in, and uploaded as a CI artifact. Until then the identity table is blank on purpose.

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-<sha>.nro` |
| Code SHA | (filled in at binding) |
| NRO SHA-256 | (filled in at binding) |
| QA-F9 audit run / job | (filled in at binding) |

## How this round is split

1. **Group H — owner hardware testing of the Round 12 repairs.** One item per Round 12 issue.
2. **Group R — earlier requirements that were never accepted.** Report what happens; NOT TESTED is fine.
3. **Not in this build** — v1.0 features that are still missing. No boxes: they are listed so they are
   never forgotten.
4. **Group A — automated testing (the development team's part).** You do not need these saves.

Round 13 hardware acceptance of a Round 12 issue is decided on its Group H item only.

## How to fill in Groups H and R

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

## Group H: Round 12 repairs (ordinary saves)

### H01 · Install · Install, open and SHA-256
- **Round 12 issue:** none (identity check).
- **Steps:** Copy this NRO to the SD card (back up first). Check its SHA-256. Open it from the Homebrew Menu and open one save of each generation you have.
- **Expected:** The SHA-256 matches the table. Product Home and each save open as in Round 12.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H02 · Gen V Boxes keys · Y is Move, + is Review, same as Gen I–IV
- **Round 12 issue:** R12-001 "Gen V Boxes Y/Review vs Move controls".
- **Steps:** Gen V (Black / White / Black 2 / White 2) Boxes: press **Y** on an occupied slot, move, press **A** on an empty slot. Repeat and press **B**. Hold **Y** to multi-select a few Pokémon and place them. Press **+** with a change staged. Press **-** for Help. Compare the footer and Help with a Gen III or IV save.
- **Expected:** Y tap picks up (Move); Y hold multi-selects; A places; B cancels with nothing changed; **+ opens Review** on the Boxes. The footer reads **Y Move / Hold Multi** and **+ Review**, the same as Gen I–IV, and the Help Pokémon page has the same rows as Gen I–IV. A on an occupied slot never overwrites (refused, the carry stays). The Gen V Party keeps **Y Review**.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H03 · Quiet carry · No pop-up for pickup, place, multi-move, clone or cancel
- **Round 12 issue:** R12-002 "Routine move/clone toast and hint clutter".
- **Steps:** In each generation you have: pick up and place a Pokémon, cancel a carry with B, multi-move a group, clone and place a copy. While carrying, press Y or X. Then try to pick up from an **empty** slot, and try to drop a group where it does not fit.
- **Expected:** Pickup, place, multi-move, clone, cancel and Y/X while carrying show **no pop-up**: the footer and the faded carry are the feedback. These still show a message: **"There is no Pokemon in this slot to move"**, a group that does not fit, an occupied / protected / changed-source refusal, and any failure.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H04 · Gen I/II packed clone · One cursor, lands in the box you choose
- **Round 12 issue:** R12-003 "Gen I–II packed-box clone shows second cursor / wrong destination".
- **Steps:** Gen I and Gen II Boxes: Clone a Pokémon. Change box with **L/R** (and ZL/ZR), try the D-pad, then press A. Repeat and press B. Try a Move (Y) too. Try a box that is full.
- **Expected:** There is **one** cursor, and it sits on the exact slot where the copy will land (after the last Pokémon of the box you are viewing; for Move, the insertion point). No second cursor wanders on its own. The copy lands in the **box you chose**, never silently in the next box. A full box is refused with the capacity message. B restores everything.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H05 · Met Location list · Verified catches first, everything else under "Other location (unverified)…"
- **Round 12 issue:** R12-004 "Met-location list mixes legitimate catches with unverified fallback".
- **Steps:** Ruby (or any Gen III): Add Pokémon → Pikachu Lv. 25 → Met Location. Crystal: Add Pokémon → Arbok Lv. 10 → Met Location. Open **"Other location (unverified)…"** and pick a row from it. Look at the **Met evidence** line in the editor and in Review. Also open an existing Pokémon whose Met Location is unusual.
- **Expected:** The default list is short: the species' verified catches in that game (including its pre-evolutions, shown "via …"), the game's markers (trade / egg / event, labelled), and one last row **"Other location (unverified)…"** (Ruby Pikachu Lv. 25: 5 rows; Crystal Arbok Lv. 10: about 15). Every other location of that exact game is still reachable inside "Other location (unverified)…". A choice from there is shown as unverified on the **Met evidence** line in the editor and in Review; it is never called legal. Create still needs an explicit choice. An existing Pokémon's stored location is always in its list and is never changed.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H06 · Met level · No verified catch above the Pokémon's level
- **Round 12 issue:** R12-005 "Shown met-level / current-level contradictions".
- **Steps:** Diamond (or any Gen IV): Pidgeotto Lv. 5 → Met Location. Black 2 (or any Gen V): Nidorino Lv. 5 → Met Location. Then raise the level (for example to 55) and look again. Crystal: choose a catch row and look at Met Level.
- **Expected:** At Lv. 5 the Route 229 (Lv. 51) and Hidden Grotto (Lv. 55–60) rows are **not** in the verified list; they appear only inside "Other location (unverified)…" marked **"level too low"** (for example "Lv. 51+ via Pidgey; level too low"). At a high enough level they are verified rows again. Crystal Met Level is never above the current level.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H07 · Top Up capacity · Preview names each list's native room
- **Round 12 issue:** R12-006 "Top Up/Add Missing appears limited to ~20 items".
- **Steps:** On a save you do not mind exporting a copy of: Bag → ZR (Top Up / Add Missing). Try **This pocket**, **All Bag pockets**, and in Gen I–III **PC Items** and **Bag + PC Items**. Read the preview, then A, then open Review.
- **Expected:** The preview opens with **one capacity line per list**, for example "Balls (16 slots): 16 free, all 11 missing added". When a list is full it says so with its native size (for example "Items (20 native slots) is full"); nothing is cut off silently. "About 20" is only ever a game's real list size (Ruby/Sapphire Items, Gen II Items, Gen I's shared 20-slot Bag), and the preview says so. The Bag never holds more than the game allows.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H08 · Every TM/HM · Add Missing adds HMs; only Key Items are left out
- **Round 12 issue:** R12-007 "All eligible TM/HM choices in inventory management".
- **Steps:** Bag → ZR → Add Missing on the TM/HM pocket (Gen I: whole Bag; Gen II: TM/HM). Read the skipped reasons in the preview. Then open Review.
- **Expected:** Every missing TM **and HM** is added (HMs with quantity 1, never a second copy anywhere in Bag + PC). An HM you already keep in the PC is not added again to the Bag. Only Key Items are left out, with the reason **"Key Item (excluded: can gate story events)"**. Emerald / FireRed / LeafGreen PC lists never get HMs (the game does not allow it there).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H09 · Gen III language · PID and stats stay the same
- **Round 12 issue:** R12-008 "Gen III language may reset PID and computed stats".
- **Steps:** LeafGreen (or any Gen III): Add Pokémon → Pikachu Lv. 5. Note its PID, stats and radar. Cycle **Language** English → Japanese → English with no other input, looking each time. Discard. Repeat on an **existing** Pokémon with Edit.
- **Expected:** PID, IVs, EVs, level and stats **never change** when only Language changes, and the radar keeps its shape. In Japanese a Latin name or OT is shown in full-width letters (ＲＥＤ); that is how the game stores it. A language that cannot store the current name is skipped with a message, never applied half-way. **Korean is not offered** in Gen III ("Korean is not offered: no Generation III game was released in Korean").
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H10 · Names by language · Gen III and Gen V names follow Language
- **Round 12 issue:** R12-009 "Gen III and Gen V Japanese/Korean name glyph display inconsistent".
- **Steps:** Gen III: create an un-nicknamed Pokémon and set Language to Japanese. Gen V: Add Pokémon (no nickname) and cycle Language, including Japanese and Korean. Compare with the same thing in Gen IV.
- **Expected:** An un-nicknamed name follows Language in Gen III, IV and V: Japanese shows kana, Gen IV/V Korean shows Hangul (not boxes). A custom nickname or OT is never changed by Language. Gen III never offers Korean (see H09).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H11 · Export message · Gone when you open something else
- **Round 12 issue:** R12-010 "Stale export confirmation overlays unrelated active editor/modal".
- **Steps:** Export a Pokémon to a file (X on an occupied slot). Straight away move to another Pokémon and open its Actions, the editor, or Met Location. Then check the exported file name in the file browser.
- **Expected:** The export message names the Pokémon you exported, and it disappears as soon as you open a new menu, editor or list. It never sits on top of an unrelated Pokémon's screen. The file is the Pokémon you exported.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H12 · Safety · Review, Keep, Discard, Export Copy, occupied destinations
- **Round 12 issue:** none new (safety re-check). Also the never-tested R11 H32 "Occupied destination safety".
- **Steps:** In one save, stage several of the changes above. Open Review, Discard once (answer B, then Y), stage again, then Export Copy. Also: Import from file (X on an empty slot), then try to Move, Clone or Import onto an **occupied** slot.
- **Expected:** Review lists every staged change, with any Met evidence / Caution notes. Discard asks first and restores the original. Export Copy writes a separate verified file; the original save is unchanged. Nothing is ever placed over an occupied slot (Gen III/V refuse, Gen IV swaps, Gen I/II insert). A Japanese Gen I/II save stays read-only.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H13 · Earlier rounds · Smoke check of Round 11 and Round 12 PASS items
- **Round 12 issue:** your Round 12 PASS items.
- **Steps:** Briefly repeat a few things that passed in Round 12: HOME card corners, quit question (B Cancel / Y Quit), "-" closes the file browser, Recovery files selection, Gen II Special stats, Gen V radar, species picker Normal / Shiny.
- **Expected:** They still behave as they did in Round 12.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

---

## Group R: earlier requirements never accepted (report what happens)

### R01 · Controller · Disconnect and reconnect
- **Source:** pre-Round 1 checklists (never tested).
- **Steps:** With a save open, detach the Joy-Con (or turn a Pro Controller off), reattach / turn it on, and press a few buttons.
- **Expected:** Input works again after reconnecting; nothing is staged or lost.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### R02 · Gen V new game · A freshly started save
- **Source:** Gen I–V gate Oct 15 (FAIL, never re-tested).
- **Steps:** If you have a Black 2 / White 2 save that was only just started (or never saved past the intro), open it.
- **Expected:** It is reported as a new game with no trainer data yet, not as a damaged save and not with invented data.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### R03 · Save discovery · Moved, missing and junk saves
- **Source:** pre-Round 1 checklists (never tested).
- **Steps:** Link a save, then rename or move it on the SD card and reopen the app. Put a copy of a non-save file (or an empty file) named `.sav` where saves are found.
- **Expected:** A moved or missing save is shown as missing with a way to relink it; nothing is created in its place. A junk or truncated `.sav` is refused with a reason, never opened.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### R04 · Launch and return · Quick Games
- **Source:** Round 3 AUD-04 (NOT TESTED).
- **Steps:** Launch a Gen I–III game (RetroArch) and a Gen IV/V game (DraStic) from the app, then quit the emulator.
- **Expected:** The game starts; quitting returns to the Homebrew Menu or the app without a crash.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### R05 · Rerolls · Keep Shiny, Gender and Unown letter
- **Source:** Round 3 E11 (NOT TESTED); Round 2/6 Unown letter (last result FAIL).
- **Steps:** Make a Pokémon Shiny and female (Gen II: an Unown with a chosen letter). Reroll IVs / DVs / PID with Y several times.
- **Expected:** Shiny, gender and the Unown letter stay as you set them; the letter, Shiny, DVs and the picture agree.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### R06 · Gen V event rows · Event and location rows look different
- **Source:** Round 2 CHECK-011 / Round 7 C09 (last result FAIL).
- **Steps:** Gen V: open Met Location and scroll to the event / marker rows.
- **Expected:** Event and marker rows are clearly labelled and cannot be mistaken for map locations; the focused row explains what it is.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### R07 · Trainer screen · Same layout Gen I–V; Review button never overlaps
- **Source:** NEW-18 / 7F, Round 7 B04 / C06 (last result FAIL).
- **Steps:** Open the Trainer screen in each generation; stage a change and look at the Gen V Review button.
- **Expected:** Gen I–IV Trainer screens are laid out like Gen V, and no button overlaps a row.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### R08 · Inventory Help · Wording and button pictures Gen I–V
- **Source:** Round 6 B04 / Round 7 B02, C01 (last result FAIL).
- **Steps:** Bag → - (Help) in each generation; compare with the footer.
- **Expected:** Help and footer name the same buttons with the same pictures, including ZR Top Up / Add Missing.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### R09 · Gen II Unown · Create then Review is visible; one Review key per screen
- **Source:** P1-RETURN, Round 7 B06 / B07 (last result FAIL).
- **Steps:** Gen II: Add Pokémon → Unown → Keep → open Review.
- **Expected:** Review draws normally (never an invisible screen), and each screen shows one Review key.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### R10 · Gen V export · Shows in Save Backups
- **Source:** Round 7 C02 (never tested).
- **Steps:** Gen V: Export Copy, then open Save Backups / Recovery files.
- **Expected:** The exported copy is listed.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### R11 · Editor typography · Same text sizes Gen I–V
- **Source:** Round 7 C03 (never tested).
- **Steps:** Open the Pokémon editor in each generation.
- **Expected:** Headings, labels and values use the same sizes and fonts in every generation.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### R12 · Popups · Tap outside closes ordinary popups
- **Source:** Round 7 C04 (never tested).
- **Steps:** Open Quick Games and another ordinary (non-confirmation) popup; tap outside it.
- **Expected:** It closes. Confirmation questions (Discard, Remove) never close on an outside tap.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### R13 · Gen I/II Origin & Legality · Panel opens and reads correctly
- **Source:** Round 7 C07 (never tested).
- **Steps:** Gen I and Gen II: A on a Pokémon → Origin & Legality.
- **Expected:** The panel opens, shows what is known, and says "unverified" where it does not know; it never claims legal.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### R14 · Keyboards · Japanese / Korean entry and the search language button
- **Source:** Round 11 H27 (FAIL).
- **Steps:** Rename a Gen IV/V Pokémon using the Japanese and Korean keyboards; open a search bar and look for a language button.
- **Expected:** Report what you find. (Not repaired in this build; this records today's state for the v1.0 list.)
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

---

## Not in this build (v1.0 blockers, no test)

These are required for v1.0 and are **missing** from this NRO. They are not regressions and are not tested
here; they stay open in `docs/qa/V1_RELEASE_FEATURE_GAPS.md` until built and accepted on your Switch.

| Requirement | Source | State |
| --- | --- | --- |
| Move to Vault in every Gen I–V Actions list | R12-011; R11 H41–H43, Extra 7 | Missing Feature |
| Vault deposit, withdraw and storage (survives restart) | R12-011; R8 H04; R11 H41/H42 | Missing Feature |
| Vault Pokémon search with highlighted matches | R12-012; R11 Extra 8 | Missing Feature |
| Moving a Gen I Pokémon to a later generation (transfer path) | R11 H38 | Missing Feature |
| Renaming in-save boxes (only Legacy Storage boxes rename today) | R11 H40 | Open, routed to the Vault plan |

---

## Group A: automated testing (no owner action)

These cases are tested by the development team with synthetic fixtures and the Japanese kit saves under GCC,
GCC AddressSanitizer/UBSan and Clang. **Synthetic evidence shows the app's logic handles these inputs; it does
not prove real Switch or emulator compatibility.** Every row is NOT TESTED / NO FIXTURE on hardware unless you
choose to report it in the Comment column; that never blocks acceptance.

| Scenario | Repair | Hardware status | Comment (optional) |
| --- | --- | --- | --- |
| Gen V Boxes: Y tap / hold, A, B and + run the shared Gen I–IV carry model; group drop is one transaction keeping its shape; Help Pokémon page identical Gen I–V | R12-001 | NOT TESTED / NO FIXTURE | |
| No toast for pickup / place / multi-move / clone / cancel; refusals and failures still post | R12-002 | NOT TESTED / NO FIXTURE | |
| Gen I/II packed carry: one focus on the landing slot; L/R, ZL/ZR change box; full box refused; B restores | R12-003 | NOT TESTED / NO FIXTURE | |
| Met Location default list = verified rows (with ancestry, level-checked) + markers + one unverified row, for every species of Ruby, Sapphire, Emerald, FireRed, LeafGreen, Crystal and every Gen IV/V game; never empty; stored rows kept; Other row never written | R12-004, R12-005 | NOT TESTED / NO FIXTURE | |
| Top Up / Add Missing adds exactly min(eligible missing, native free slots) per list, every generation and scope (Red, Yellow, Crystal, Ruby, Emerald, FireRed, LeafGreen, Diamond, Platinum, HeartGold, Black, Black 2); plan → apply → export → strict reparse | R12-006, R12-007 | NOT TESTED / NO FIXTURE | |
| Gen III language cycle on all five Gen III games keeps PID, IVs, EVs and stats; staged record reparses; OT / nickname re-stored per language or refused | R12-008 | NOT TESTED / NO FIXTURE | |
| Gen V un-nicknamed Create name follows Language | R12-009 | NOT TESTED / NO FIXTURE | |
| Transient status ends when a new modal / Actions sheet / editor opens (fails on the Round 12 code, passes now) | R12-010 | NOT TESTED / NO FIXTURE | |
| Earlier Group A rows (Round 12 template) | Rounds 8–12 | NOT TESTED / NO FIXTURE | See `RETEST_ROUND12_TEMPLATE.md` |
