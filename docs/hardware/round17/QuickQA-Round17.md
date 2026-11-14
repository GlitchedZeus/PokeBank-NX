# PokeBank NX Round 17 Hardware-Only QuickQA

**Status: NOT DEVICE TESTED.** Focused candidate retest. This is **not** the complete 20-game release acceptance required by [#313](https://github.com/GlitchedZeus/PokeBank-NX/issues/313).

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-95d0579b.nro` |
| NRO SHA-256 | `86648d4b500eb4953296560c785193a96527379bb012374fffe01c157950462a` |
| NRO size | `177,290,929 bytes` |
| Source commit | `95d0579b307e6af4b423d17232d1d1c55e68ffcf` |
| Source tree | `87459a0ef5a30365c89d3231563e59f7177f9590` |
| Build run / artifact | `43571489940 / 14104911047` |
| Replaces | `231e8497 (never device tested) and 11ea9790 (failed on hardware)` |
| Status | `NOT DEVICE TESTED` |

Use the interactive HTML version (same items) to record results offline and export JSON. Mark each check PASS, FAIL or NOT TESTED; NOT TESTED is never a failure.

## 1. Things only a Switch can show

### H01 · The app starts and Home draws correctly
- **Game:** None
- **Get there:** Homebrew Menu (or your forwarder)
- **Do:** Start PokeBank NX handheld. Look at Home: game cards, artwork, text, footer. Then dock the Switch and look again.
- **Expected:** No crash or black screen. Nothing clipped, overlapping or missing in handheld or docked mode.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### H02 · Your real saves are found
- **Game:** Every Gen I-V game you have saves for
- **Get there:** Product Home
- **Do:** Do NOT delete anything in sdmc:/switch/PokeBank-NX first. Scroll through the game cards.
- **Expected:** Each game you have a save for has a card. Comment with any game whose save is on the card but not found, and where the file is (emulator and folder).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### H03 · Save Instances opens a real save (the Round 15 failure)
- **Game:** Pokémon Yellow or Blue (RetroArch, Gambatte folder)
- **Get there:** Product Home → that game's card
- **Do:** Press A. If Save Instances appears, pick your save with A.
- **Expected:** The game opens. If Home later says 'Save choices are kept until PokeBank NX closes; they could not be stored [...]', copy the text in [...] into the comment.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### H04 · The choice survives closing the app
- **Game:** The game from H03, plus one Gen III, one Gen IV and one Gen V game
- **Get there:** HOME menu
- **Do:** Open each game once (pick its save if asked). Close PokeBank NX completely. Start it again and press A on each.
- **Expected:** Every one opens directly, with no chooser.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### H05 · Touch works on the real screen
- **Game:** Any game
- **Get there:** Boxes
- **Do:** Tap a Pokémon. Tap an empty slot. Drag across four Pokémon and lift. Tap outside a pop-up.
- **Expected:** Taps hit what you touched (not a neighbour). The drag selects the group. A tap outside closes the pop-up.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### H06 · Controller: stick, D-pad and held buttons
- **Game:** Any game
- **Get there:** Boxes, then Items
- **Do:** Move with the left stick, then with the D-pad. Hold Down for two seconds in a long list. Try a detached Joy-Con or Pro Controller if you have one.
- **Expected:** Stick and D-pad do the same thing. Holding repeats smoothly and never skips or sticks.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### H07 · RetroArch launch and quit (R04, evidence only)
- **Game:** Pokémon Red
- **Get there:** Red card → launch the game
- **Do:** Launch Red from PokeBank NX, then quit RetroArch. Note where you land and any error code. Copy sdmc:/switch/PokeBank-NX/runtime/last-launch.txt and any new file in sdmc:/atmosphere/crash_reports/.
- **Expected:** Not fixed in this build. Comment: where you landed, the error code, how you start PokeBank NX (Homebrew Menu / Album / forwarder), and attach last-launch.txt.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### H08 · An exported copy loads in the emulator
- **Game:** Any game
- **Get there:** Edit one item amount → + Review → Export Copy
- **Do:** Copy the exported save over a COPY of your emulator save (never the original) and start the game in the emulator.
- **Expected:** The game loads the exported save and shows your change. Your original save is unchanged (same size and date).
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

## 2. New fixes to confirm on the Switch

### N01 · Legacy Storage: Save bank changes? can be answered
- **Game:** Any Gen I-IV game
- **Get there:** Open the game → Legacy Storage
- **Do:** Keep the cursor on the LEFT (game save) side. Tap the bank box name on the right and rename it. Press B. Then press Down and A (Discard changes).
- **Expected:** 'Save bank changes?' appears and your answer is taken: you leave Legacy Storage. No Pokémon Actions or Empty Slot menu opens on top of the question.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### N02 · Profile B cannot see Profile A's backups
- **Game:** Any game assigned on Profile A
- **Get there:** Profile A: game card → + → Backups → Create backup now
- **Do:** Switch to Profile B. On the same card press A (chooser), then + (Backups). Then press X (Recovery files).
- **Expected:** Profile B's Backups and Recovery files do not list Profile A's backup. Back on Profile A, the backup is still listed.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### N03 · A file named for the other version is refused
- **Game:** Pokémon Red (or Gold, Ruby, FireRed)
- **Get there:** Red card → chooser → file browser
- **Do:** Pick a save file whose name says the sibling version (for Red: a file named '... Blue Version ...').
- **Expected:** Refused: 'the file is named as a Pokemon Blue save; choose it from that game's card'. A file with a neutral name (for example save1.srm) can still be assigned.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

### N04 · Gen V type badges
- **Game:** Pokémon Black or White 2
- **Get there:** Edit a Ninjask, a Charizard and a Gengar
- **Do:** Look at the type badges under the portrait.
- **Expected:** Ninjask Bug/Flying, Charizard Fire/Flying, Gengar Ghost/Poison.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**

## 3. If anything failed

### D01 · Send the diagnostics
- **Game:** Any failing game
- **Get there:** Settings → Enable Debug Logging (ON), then repeat the failing step
- **Do:** Copy sdmc:/switch/PokeBank-NX/logs/debug_*.log and list the files in sdmc:/switch/PokeBank-NX/config (names and sizes).
- **Expected:** The log has an 'Assignment storage: ...' line and a line naming the failing step. Attach both. Never delete the config files.
- **Result:** [ ] PASS  [ ] FAIL  [ ] NOT TESTED
- **Comment:**
