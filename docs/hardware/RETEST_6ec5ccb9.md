# Round 8 hardware retest checklist: PokeBank-NX-Gen1-5-Candidate-6ec5ccb9.nro

**Status: NOT DEVICE TESTED.** This is a plain Markdown file. Fill it in with any text editor, on any device,
offline. Nothing here resets, expires or needs a network. Save often; partial results are fine.

Round 8 is the **safety-fix candidate** from `wip/pkse-1.2.x-gen15-audit` (the PKSE 1.2.x audit and the Q01–Q09
repairs, see `docs/audits/PKSE_UPSTREAM_REPAIR_CAMPAIGN_2026-10-27.md`). It holds no multilingual work. The audited
round 7 candidate (`e88957f2`) is unchanged and keeps its own checklist, `RETEST_e88957f2.md`.

This checklist is for the bound round 8 candidate `6ec5ccb9` (docs/qa/CANDIDATE_UNDER_AUDIT.env).
Test only the file whose SHA-256 matches the table. Every other build is not this candidate.

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-6ec5ccb9.nro` |
| Code SHA | `6ec5ccb982d3f51e42470c6b22bf4c2ea61b4b5d` |
| Tree SHA | `0106544696902cff26b938d9acdb92aa82736526` |
| Build run | https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/40605484594 (run `40605484594`) |
| Artifact | `PokeBank-NX-Gen1-5-Candidate-6ec5ccb9` (id `12779504702`) |
| NRO SHA-256 | `49d95c555636e3ed3f461c080c168d06c963415801d8ad02b0f7ae5f87eee186` |
| NRO size | 176885425 bytes |
| QA-F9 audit run / job | (recorded by the coordinator after the audit passes) |

## How to use this file

1. Check the downloaded NRO's SHA-256 against the table before testing. Write it here: `________________`
2. Each item has the same fields. Tick one result box by replacing `[ ]` with `[x]`:
   - **Steps** and **Expected**: what to do and what you should see.
   - **Result**: `[ ] PASS  [ ] FAIL  [ ] NOT TESTED`.
   - **Game / save used**: e.g. `Crystal, emulator archive 2026-10-20`.
   - **Comment**: anything not PASS, or a request. A PASS can still carry a request.
   - **Screenshots sent separately**: the file names, e.g. `IMG_0412.jpg, IMG_0413.jpg`.
3. Back up the SD card first. Never overwrite an installed or emulator source save. Every edit exports to a separate
   verified copy. Loading an export in an emulator is optional and never means direct writing or injection.
4. Skip any item you have no save for; mark it NOT TESTED. Please do not upload private saves to GitHub.

Host and CI results never fill in a box here.

Summary (fill in at the end): PASS ____ / FAIL ____ / NOT TESTED ____

---

## Group S: safety repairs in this candidate

### S01 · Q01 · Bank: fresh install
- **Steps:** Back up the SD card first. Install this NRO where no `sdmc:/switch/PokeBank-NX/banks/` folder exists (or temporarily move that folder away). Open Storage, deposit one Pokémon, save the Bank.
- **Expected:** Storage opens with no warning. The Bank saves, and reopening the app shows the deposited Pokémon.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### S02 · Q01 · Bank: unreadable file is kept, never replaced
- **Steps:** Copy any junk file over `banks/legacy-pkse/bank.dat` (keep your real Bank elsewhere). Open Storage. Try to save.
- **Expected:** Storage says the Bank could not be read and was kept as `bank.dat.unreadable.N`. Saving the Bank is refused with a reason. The junk file is byte-identical afterwards. Restoring your real Bank brings normal use back.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### S03 · Q03 · Gen II padded 128 KiB save
- **Steps:** Use a copy of a 128 KiB padded Gold/Silver/Crystal `.srm`. Open it, change money, Export Copy.
- **Expected:** The save is found and opens. The export is 128 KiB with the same padding, and opens in an emulator. The original is unchanged (same SHA-256).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### S04 · Q02 · Japanese Crystal 32 KiB is read-only
- **Steps:** Open a copy of a 32 KiB Japanese Crystal save.
- **Expected:** It opens read-only. No trainer gender is invented. Edit is refused with the Japanese read-only message.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### S05 · Q04 · Localized FireRed/LeafGreen on Switch
- **Steps:** With any non-English FireRed or LeafGreen release installed, open the game list.
- **Expected:** Each installed release shows as its own card and gets its own backup folder. Opening it never writes to the console save.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### S06 · Q07 · Gen III Bag byte fidelity
- **Steps:** Gen III save → Bag. Set an item to the same quantity; then change it and change it back.
- **Expected:** Review shows no pending change. If you export after a real change, only that slot (and the checksum) differs from the original.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### S07 · Q07 · Gen II Bag byte fidelity
- **Steps:** Gen II save → Bag. Same steps as S06.
- **Expected:** Review shows no pending change for the same value. A real change touches only that pocket list.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### S08 · Q06 · FireRed/LeafGreen Meteorite
- **Steps:** On a save that holds the Meteorite, remove it (accept the key-item warning), then add it back.
- **Expected:** Both work. Quantity 2 is refused. The export reopens with the Meteorite in Key Items.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### S09 · 9c275755 · Gen III Egg names
- **Steps:** Gen III save → open an Egg → try to change its Nickname and Language.
- **Expected:** Both edits are refused with the Egg message. The Egg keeps its name.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### S10 · 2ab98df6 · Gen V species change keeps gender consistent
- **Steps:** Gen V save → a female Pokémon whose species change would flip its gender (e.g. a female Pikachu whose PID gender byte is 31 or more → Eevee).
- **Expected:** The change is refused with the "other gender" message. A change that keeps the gender works.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### S11 · e13ced8e · Gen III emulator save with RTC footer
- **Steps:** Copy of an mGBA/VBA-M Gen III save with an RTC footer → edit Bag and Trainer → Export Copy.
- **Expected:** Both editors open the save. The export keeps the footer byte-for-byte and opens in the emulator.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
### S12 · 0489dd20 · Gen II OT name length
- **Steps:** Gen II save → Pokémon → OT name.
- **Expected:** The keyboard allows up to 7 characters; an 8th is not accepted.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**

## Group R: regression

### R01 · Round 7 · Round 7 smoke check
- **Steps:** Repeat a few of your favourite round 7 items (one save per generation: open, browse, edit, Export Copy).
- **Expected:** Nothing behaves worse than the round 7 candidate. Note anything different in Comment.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Game / save used:**
- **Comment:**
- **Screenshots sent separately:**
