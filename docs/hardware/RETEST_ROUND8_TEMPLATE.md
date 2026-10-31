# Round 8 hardware retest checklist (template)

**Status: NOT DEVICE TESTED.** This is a plain Markdown file. Fill it in with any text editor, on any device,
offline. Nothing here resets, expires or needs a network. Save often; partial results are fine.

Round 8 is the **safety-fix candidate** from `wip/pkse-1.2.x-gen15-audit` (the PKSE 1.2.x audit and the Q01–Q09
repairs, see `docs/audits/PKSE_UPSTREAM_REPAIR_CAMPAIGN_2026-10-27.md`). It holds no multilingual work. The audited
round 7 candidate (`e88957f2`) is unchanged and keeps its own checklist, `RETEST_e88957f2.md`.

When the coordinator binds the round 8 candidate, this template is copied to `RETEST_<sha>.md` with the exact
NRO identity filled in, and uploaded as a CI artifact. Until then the identity table is blank on purpose.

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-<sha>.nro` |
| Code SHA | (filled in at binding) |
| NRO SHA-256 | (filled in at binding) |
| QA-F9 audit run / job | (filled in at binding) |

## How this round is split

1. **Group H — owner hardware testing (this is your part).** Ten short checks with your own ordinary Gen I–V
   saves on your Switch. Round 8 hardware acceptance is decided on Group H only.
2. **Group A — automated compatibility and safety testing (the development team's part).** Japanese saves,
   padded emulator formats, non-English Switch releases, unreadable Banks, RTC wrappers, special items and
   deliberately corrupted data. These run on synthetic fixtures in CI and on the host, not on your Switch.
   The evidence is in `docs/qa/ROUND8_AUTOMATED_COMPATIBILITY_MATRIX.md`. **You do not need these saves.**

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
- **Expected:** The app opens to Product Home. Each Pokémon game you have on the Switch, and each emulator save it already knew, appears once. Nothing asks to write a save.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H02 · Navigation · Moving around
- **Steps:** Use the D-pad, left stick and touch on Product Home, a save's main menu, Pokémon, Bag and Trainer. Press B to go back each time. Open Help (−) and Quick Games (Y) and close them.
- **Expected:** Every screen responds to all three inputs. B always goes back one step. Nothing freezes or exits unexpectedly.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H03 · Open saves · Open one save per generation you own
- **Steps:** Open one ordinary save for each generation you have (I, II, III, IV, V). Look at the trainer card, the party and one box. Leave without changing anything.
- **Expected:** Names, money, badges, party and boxes match what the game shows. Leaving asks nothing, because nothing changed.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H04 · Bank · Bank storage
- **Steps:** Open Storage. Put one Pokémon into the Bank, then save the Bank. Close the app completely and open it again. Open Storage.
- **Expected:** The Pokémon is still in the Bank after the restart. No warning appears.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H05 · Bag · Ordinary Bag editing
- **Steps:** In any save you own: Bag → change one item's quantity → change it back to the original number → open Review. Then change it to a new number and open Review again.
- **Expected:** After changing it back, Review shows no pending change. After the real change, Review lists exactly that one item.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H06 · Pokémon · Ordinary Pokémon editing
- **Steps:** Pick any Pokémon. Change its nickname, level and held item → Keep → open Review.
- **Expected:** Review lists exactly those three changes, in words. Other values are unchanged.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H07 · Name limit · Gen II OT name length
- **Steps:** In a Gold, Silver or Crystal save, edit a Pokémon's OT name.
- **Expected:** The keyboard accepts up to 7 characters and no more.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H08 · Discard · Discard leaves the save untouched
- **Steps:** Make any change, then choose Discard (Y on the exit prompt).
- **Expected:** Reopening the save shows the original values. No file is written.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H09 · Export Copy · Export Copy writes a separate file
- **Steps:** Make one change → Review → Export Copy. Note the export's file name. Then look at your original save (date, size, and if you can, its SHA-256).
- **Expected:** A new export file appears. The original save is unchanged. Optional: the export loads in your emulator and shows the change.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED / NO FIXTURE
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### H10 · Round 7 · Round 7 smoke check
- **Steps:** Repeat a few of your favourite round 7 checks.
- **Expected:** Nothing behaves worse than the round 7 candidate. Note anything different in Comment.
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
