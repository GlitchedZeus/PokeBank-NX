# Round 10 hardware retest checklist: PokeBank-NX-Gen1-5-Candidate-3634d1e6.nro

**Status: NOT DEVICE TESTED.** This is a plain Markdown file. Fill it in with any text editor, on any device,
offline. Nothing here resets, expires or needs a network. Save often; partial results are fine.

Round 10 is the round 9 combined candidate plus two **Gen IV Bag corrections** (branch
`fix/gen4-bag-remove-compaction`, from the independent QA campaigns): **Remove** now closes the gap the way
Diamond/Pearl, Platinum and HeartGold/SoulSilver do, so later items no longer vanish from the in-game Bag; **Add**
puts a new Berry or TM/HM in item-number order like the games. Round 9 itself is the **combined candidate** from `integration/gen15-combined`: the round 6 and 7 fixes, the round 8
safety repairs, the PKSE October 2026 fixes and features (standalone Pokémon files, Top Up / Add Missing, folder
favourites, recovery files view, the real-save runner), the multilingual naming fixes, the Japanese Gen I/II writers
and kana keyboard (Japanese saves stay read-only), and the owner's Gen I–V UI fixes. Earlier candidates keep their
own checklists (`RETEST_e88957f2.md`, `RETEST_6ec5ccb9.md`, `RETEST_d6a20a2a.md`). Nothing is merged to main.

This checklist is for the bound round 10 candidate `3634d1e6` (docs/qa/CANDIDATE_UNDER_AUDIT.env).
Test only the file whose SHA-256 matches the table. Every other build is not this candidate.

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-3634d1e6.nro` |
| Code SHA | `3634d1e6f3f216160db162fd953d82ede45f7af0` |
| Tree SHA | `9b242ddf13981d45c4fd535e65c8e1da037835d4` |
| Build run | https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40775834570 (run `40775834570`) |
| Artifact | `PokeBank-NX-Gen1-5-Candidate-3634d1e6` (id `12854482976`) |
| NRO SHA-256 | `b6cd0b89e25c83627558b4d3d70949431c40947c3c8cd26fe46b67bb2c9e8cd9` |
| NRO size | 177159857 bytes |
| QA-F9 audit run / job | (recorded by the coordinator after the audit passes) |

## How this round is split

1. **Group H — owner hardware testing (this is your part).** Seventeen short checks with your own ordinary Gen I–V
   saves on your Switch. Round 10 hardware acceptance is decided on Group H only.
2. **Group A — automated compatibility and safety testing (the development team's part).** Japanese saves,
   padded emulator formats, non-English Switch releases, unreadable Banks, RTC wrappers, special items and
   deliberately corrupted data. These run on synthetic fixtures in CI and on the host, not on your Switch.
   The evidence is in `docs/qa/ROUND8_AUTOMATED_COMPATIBILITY_MATRIX.md` and the Group A table below. **You do not need these saves.**

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
5. Please do not upload private saves to GitHub. If a real sample would help, the team will ask for a
   specific one.

Host and CI results never fill in a box here.

Summary (fill in at the end): PASS ____ / FAIL ____ / NOT TESTED / NO FIXTURE ____

---

## Group H: owner hardware testing (ordinary saves)

### H01 · Install · Install and open
- **Steps:** Copy this NRO to the SD card (back up the SD card first). Check its SHA-256 against the table. Open it from the Homebrew Menu.
- **Expected:** The app opens to Product Home. Each Pokémon game you have and each emulator save it already knew appears once. Nothing asks to write a save.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H02 · Open saves · One save per generation
- **Steps:** Open one ordinary save for each generation you have (I, II, III, IV, V). Look at the trainer card, the party and one box. Leave without changing anything.
- **Expected:** Names, money, badges, party and boxes match the game. Leaving asks nothing, because nothing changed.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H03 · Bag Help · Bag Help fits
- **Steps:** Open the Bag in a Gen I–III game and in a Gen IV or V game. Press − for Help.
- **Expected:** Both show the same "Inventory Controls" window, sized to its text. No line runs past the window edge.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H04 · Top Up · Top Up / Add Missing
- **Steps:** In any save: Bag → ZR → choose Top up, This pocket, Amount 50 → Preview changes. Read the list, press B. Then Preview again and press A. Open Review.
- **Expected:** The preview lists each change ("Potion 3 → 50") and each skip with a reason; Key Items and HMs never change. B changes nothing. A stages the changes and Review lists them.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H05 · Export file · Export a Pokémon to a file
- **Steps:** On any Pokémon: A → Export to file. Export the same Pokémon a second time.
- **Expected:** The status names a new file under switch/PokeBank-NX/pokemon/genN. The second export gets " (2)" in its name. Nothing in the save changes (Review stays empty).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H06 · Import file · Import that file into an empty slot
- **Steps:** In a save of the same generation: an empty box slot → A → Import from file → open the gen folder → pick the file from H05. Open Review.
- **Expected:** The Pokémon appears in the slot as a staged change and Review lists it. B in the browser goes up a folder and closes at the start. The original save file is unchanged.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H07 · Gen V Create · Gen V Create asks for a Met Location
- **Steps:** In a Black, White, Black 2 or White 2 save: an empty box slot → Add → Create. Look at Met Location, then press X (Keep) before choosing one. Then pick a location and Keep.
- **Expected:** Met Location reads "Choose a location" and Keep is refused until one is picked. After picking, Keep works.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H08 · Gen V Gender · Gen V Gender changes inline
- **Steps:** Edit (or Create) a Gen V Pokémon whose species can be either gender. Select Gender and press A.
- **Expected:** Gender flips straight away with no "Change PID?" window; the status line says the PID changed. A again flips it back.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H09 · Gen V Trainer · Gen V Trainer Gender and Pokédex
- **Steps:** Open the Gen V Trainer page. Select Gender, press A. Open Review. Then select the Pokédex row (below the divider) and press A.
- **Expected:** Gender is a card you can change and Review lists "Trainer / Gender". The Pokédex sits with the read-only rows and A opens its list.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H10 · Gen I Play Time · Gen I Play Time
- **Steps:** Open a Red, Blue or Yellow Trainer page. Edit Play Time to a new value. Open Review.
- **Expected:** Play Time is editable (hours up to 255) and Review lists the change.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H11 · Search · L/R while searching
- **Steps:** Open any species or item search and type a few letters. Press L, then R.
- **Expected:** L deletes the last letter. R does nothing. Neither moves the screen behind the search.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H12 · Recovery files · Recovery files view
- **Steps:** Home → + → Backups on an emulator game → X (Recovery files). Read the list. Select an older backup with A, press Y, then B on the confirmation.
- **Expected:** Nothing is selected at first. The newest copy of each save and the Bank are shown as protected. The confirmation lists the files; B keeps everything.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H13 · Japanese saves · Japanese saves stay read-only (only if you have one)
- **Steps:** Open a Japanese Gen I or Gen II save if you have one.
- **Expected:** It opens, names show in kana (not blank boxes), and nothing can be edited. If you have no Japanese save, mark NOT TESTED / NO FIXTURE.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H14 · Export Copy · Export Copy still separate
- **Steps:** Make one ordinary change → Review → Export Copy. Look at your original save afterwards.
- **Expected:** A new export file appears; the original save is unchanged.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H15 · Rounds 6-8 · Earlier rounds smoke check
- **Steps:** Repeat a few favourite checks from rounds 6, 7 and 8.
- **Expected:** Nothing behaves worse than the round 7 / round 8 candidates. Note any difference in Comment.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

### H16 · Gen IV Bag · Remove keeps later items visible
- **Steps:** Open a Diamond, Pearl, Platinum, HeartGold or SoulSilver save with at least three items in one pocket. Bag → that pocket → select the FIRST (or a middle) item → Remove → confirm. Review → Export Copy. Load the export in the game or an emulator (optional) and open that Bag pocket.
- **Expected:** In the app the removed item is gone and every later item is still listed, in the same order. In the game (if you load the export) the pocket shows the same items: none are missing after the removed one.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H17 · Gen IV Bag · Berries and TM/HM added in number order
- **Steps:** In a Gen IV save: Bag → Berries → Add a Berry that comes BEFORE some you already have (for example Cheri Berry). Then TMs & HMs → Add a TM with a low number. Also Add an item to the Items pocket.
- **Expected:** The new Berry and TM appear in their item-number position, not at the end; the other Berries and TMs keep their order. The Items pocket item is added at the end (the games do not sort that pocket).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

---

## Group A: automated compatibility and safety testing (no owner action)

These cases need saves or releases you are not expected to own. The development team tests them with
synthetic fixtures under GCC, GCC AddressSanitizer/UBSan and Clang, with successful and deliberately failing
cases. **Synthetic evidence shows the app's logic handles these inputs; it does not prove real Switch or
emulator compatibility.** Each row's evidence class (synthetic / public fixture / real save) is recorded in
`docs/qa/ROUND8_AUTOMATED_COMPATIBILITY_MATRIX.md`. Every row is NOT TESTED / NO FIXTURE on hardware unless
you happen to have such a save and choose to report it in the Comment column; that never blocks acceptance.

| Scenario | Repair | Hardware status | Comment (optional) |
| --- | --- | --- | --- |
| Bank missing, unreadable/corrupt, truncated, damaged, newer layout, interrupted save | Q01, S01–S02 | NOT TESTED / NO FIXTURE | |
| Gen II battery saves padded to 128 KiB (0x00 / 0xFF fill) | Q03 | NOT TESTED / NO FIXTURE | |
| Japanese Crystal 32 KiB opens read-only; Japanese Gen II editing refused | Q02, Q08 | NOT TESTED / NO FIXTURE | |
| Non-English FireRed/LeafGreen Switch releases (10 localized title ids) discovered once each | Q04 | NOT TESTED / NO FIXTURE | |
| Gen III emulator saves with an RTC footer (mGBA / VBA-M), Bag and Trainer editors | e13ced8e | NOT TESTED / NO FIXTURE | |
| Gen III torn or signature-free save slots | Q05 | NOT TESTED / NO FIXTURE | |
| FireRed/LeafGreen Meteorite (280) and unreleased held items | Q06, b80137df | NOT TESTED / NO FIXTURE | |
| Gen II/III Bag byte fidelity (only the edited slot and checksum change) | Q07 | NOT TESTED / NO FIXTURE | |
| Gen III Egg names; Gen V species change keeps PID gender | 9c275755, 2ab98df6 | NOT TESTED / NO FIXTURE | |
| Japanese Gen I/II writers (edit, Create, Clone, Release, move, trainer, Bag, .pk1/.pk2) — built but locked read-only in the app (`JapaneseEditingGate.h`) | Oct 28 | NOT TESTED / NO FIXTURE | Real-save evidence: the owner's Japanese kit (Red, Blue, Yellow, Gold, Silver, Crystal 64 KiB, padded Gold): unedited round trip byte-identical; edits change only the named bytes |
| Japanese Gen III / IV / V saves (Ruby, Sapphire, Emerald, FireRed, LeafGreen, Pearl, Black 2, White) | Oct 28 | NOT TESTED / NO FIXTURE | Real-save evidence: unedited round trip byte-identical (Gen III / IV); strict parse (Gen V); Top Up plan + apply reparses |
| Standalone .pk1–.pk5 / .ek3–.ek5 files, including Japanese .pk1/.pk2 and PKHeX test files | Feature A | NOT TESTED / NO FIXTURE | |
| Gen III slot with an unused species index is never overwritten (Create, Clone, Move, Import) | Oct 28 | NOT TESTED / NO FIXTURE | |
| Gen I Exp. All is an ordinary item; settings file hardening; Gen IV/V full pockets | PKSE Oct | NOT TESTED / NO FIXTURE | |
| Gen IV Bag pocket with a pre-existing hole, ghost or filler record: Remove and Add refused with a message, save unchanged | Round 10 | NOT TESTED / NO FIXTURE | Synthetic DP/Pt/HGSS fixtures (`test_gen4_bag_remove_pack`) |
| Gen IV Bag Remove / Add byte fidelity (only the pocket's moved slots and the General checksum change) | Round 10 | NOT TESTED / NO FIXTURE | Real-save evidence: Japanese Pearl kit save, every stack Removed and re-Added in memory; Berries/TM byte-identical to the original |
