# Hardware retest (round 2) — PokeBank-NX-Gen1-5-Candidate-e10f307f

**Status: NOT DEVICE TESTED.** Not hardware accepted until the owner tests this exact file on a Switch.
Only the owner records acceptance. Identity values below are from the QA-F9 exact-binary audit.

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-e10f307f.nro` |
| Code SHA | `e10f307faf7254a47429babc49093e407ed65f1a` |
| Tree SHA | `8451ec64013c8f1bdf43b0981050e2d8c6061d9d` |
| Build run | https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/39120355693 (run `39120355693`) |
| Artifact | `PokeBank-NX-Gen1-5-Candidate-e10f307f` (id `12133189538`) |
| NRO SHA-256 | `1dd518b756274cfe70d7a87b06a2e30f027389aeaddd9640f03ef3c5a2eb7ac0` |
| NRO size | 175791793 bytes |
| QA-F9 audit run / job | https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/39122863086 (job `117661839347`): NRO AUDIT: PASS |

Only this artifact is the candidate. Later docs-only commits on #307 (this file, the candidate binding)
also trigger the candidate workflow; those artifacts are **not** the candidate.

Before testing, check that the downloaded file's SHA-256 matches the value above. Round 1 baseline:
[ROUND1_RESULTS_7fea1797.md](ROUND1_RESULTS_7fea1797.md) (42/54 PASS).

**Input trace (recommended for any input problem)**, as in [RETEST_7fea1797.md](RETEST_7fea1797.md) §0:
create an empty file `sdmc:/switch/PokeBank-NX/config/input_trace.enable`, start PokeBank NX, reproduce
the problem, then send `sdmc:/switch/PokeBank-NX/logs/input-trace.log`. It records only button names,
screen tags and timings (no save data). Delete the marker file to turn it off.

Mark each box PASS / FAIL / NOT VERIFIED and send notes for anything that is not PASS. "Host evidence"
is automated host testing only; it is never device acceptance.

## 1. Round 1 FAIL — fixed this round, retest on device

| Result | ID | Owner's round 1 problem | What should happen now | Host evidence |
| --- | --- | --- | --- | --- |
| [ ] | B01 | Gen I Add Item: numpad before the item is in the bag | Bag → Add Item → pick a Potion: it appears in the pocket at **x1 at once, no numpad**. A on its row opens the quantity editor. Picking an item you already hold jumps to that row. | CG12 (no numpad on any Add path; it reproduced this defect on the round 1 code), matrix `bag-add` (Red/Blue/Yellow), `tests/test_bag_add_flow_gen15.cpp` |
| [ ] | B02 | Gen II: same as B01 | Same as B01 in Gold / Silver / Crystal. | CG12, matrix `bag-add` (G/S/C) |
| [ ] | B03 | Gen III: same as B01/B02 | Same as B01 in Ruby / Sapphire / Emerald / FireRed / LeafGreen. | CG12, matrix `bag-add` (Gen III) |
| [ ] | B06 | Stackable quantity workflow (part of B01–B03) | Quantity editing only from an inventory row; range follows the game's rule; unique items show no quantity editor. | CG09 + CG12 (`decideRow`), matrix `bag-add` |
| [ ] | S01 | Gen V PP / PP Ups text too close | In a Gen V Moves panel the PP and PP Ups captions are clearly separated, like Gen I–IV; the move chooser shows more rows. | Lane S `b1d34f9` (PP spacing, 10-row chooser) |
| [ ] | S03 | Gen V left-panel species text cut off; parity with Gen I–IV | Gen V Create / Edit / View: species and other DETAILS values are not clipped; locked rows are gold; the header names the game (e.g. "• White 2 •"). | `tests/test_gen5_create_edit_parity.cpp`, CG16, CG17 |

## 2. Round 1 NOT VERIFIED — please check (host evidence only so far)

Detailed steps: [PHASE13_UNVERIFIED_STEPS.md](PHASE13_UNVERIFIED_STEPS.md).

| Result | ID | Item | Host evidence |
| --- | --- | --- | --- |
| [ ] | B04 | Gen IV / V Add Item match the shared flow | CG12, matrix `bag-add` (Gen IV through `stageBagAdd`, Gen V through `Gen5BagScreen::pickAdd`), Gen IV Add → export → strict reparse round trip |
| [ ] | E03 | Origin & Legality heading in Gen I–V | CG03 (enforced since round 1) |
| [ ] | E11 | Rerolls keep the requested Shiny / Gender (and Gen II Unown letter) | matrix `reroll` (all 20 versions), Lane R `tests/test_reroll_mechanics_gen15.cpp`. Expected: in Gen I/II a shiny and a non-shiny can never share DVs (shininess is the DV pattern); in Gen III–V identical IVs on a shiny and a non-shiny are valid. |
| [ ] | L06 | Party / summary taps open the right Actions screen | CG11 (no generic sheet / modal in a Gen I–V session) |
| [ ] | S04 | Gen V Review / Discard / Export | Lane S `tests/test_gen5_review_discard_export_trace.cpp` (372 checks on W/B/W2/B2, no defect found). See notes in §5. |
| [ ] | S06 | Exported edited save loads in an emulator. The app never writes to emulator saves. To verify: copy `sdmc:/switch/PokeBank-NX/exports/…/edited.<ext>` into the emulator's save folder yourself (keep a backup of your own save first), load the game, check the edits. | export contract C2 (strict reparse as the same game). Device / emulator only otherwise. |

## 3. New this round (owner's requests) — check on device

| Result | Item | What should happen | Host evidence |
| --- | --- | --- | --- |
| [ ] | R2 menus (Phase 2) | Bag menus, held-item and Ball pickers, Key Item notice, Remove confirmation and box menus are compact, with no big gaps between name and sprite; the same kind of menu is the same size in every generation. | CG13 (every menu type through SharedMenuGeometry), `tests/test_shared_menu_geometry.cpp` |
| [ ] | R2-SEARCH (Phase 3) | Species picker: press X or − (or tap Search), type letters: the list narrows as you type ("C", then "CO"…); Backspace widens it again; Gen I–V. | CG15, `tests/test_species_search_gen15.cpp` |
| [ ] | R2-GOLD (Phase 4) | Gen I and Gen II, View and Edit: locked values (HP DV, calculated stats) are gold in both; editable values (including Shiny in Edit/Create) use the normal colour. Gen III–V: locked rows are gold, with no "(read-only)" labels. | CG16, `tests/test_theme.cpp` |
| [ ] | Gen II Special (Phase 4) | Gen II shows a real Special value (SpA/SpD), never "--". | matrix `special-stat` (G/S/C), `tests/test_gen2_special_stat.cpp` |
| [ ] | R2-TRAINER (Phase 5) | Gen I Trainer page: Name and Money are editable; the footer reads "A: Edit" only where a field can be edited (all generations); Money round-trips in the export. | CG18, matrix `trainer-edit` (Red/Blue/Yellow, BCD money), `tests/test_gen1_trainer_edit.cpp` |
| [ ] | Launch menu (Phase 6) | Compact Launch dialog with glyph buttons **B Back / A Stay / Y Launch**: only Y launches; A and B stay. | CG07, `tests/test_game_hub_contract.py` |
| [ ] | R2-EMUBACKUP (Phase 7) | + on an emulator game → Backups lists the app's export records; the original emulator save stays read-only; deleting a record asks first and **only Y deletes**. | `tests/test_emulator_backup_access.cpp`, `tests/test_game_hub_contract.py` (C07 guard) |
| [ ] | R2-SCROLLBAR (Phase 8) | Every scrollbar can be dragged (and the track tapped) to scroll fast. | CG14, `tests/test_shared_scrollbar.cpp` |
| [ ] | Move chooser rows (Phase 9) | The move chooser shows 1–2 more rows instead of blank space, all generations. | Lane S `b1d34f9` |
| [ ] | Rerolls (Phase 10) | As E11 above. | matrix `reroll` |
| [ ] | Crystal Met Time (Phase 11) | Crystal: A on Met Time cycles the value in place (no menu). | matrix `met-time` (Crystal), `tests/test_crystal_met_time_inline.cpp` |
| [ ] | Gen V Create header (Phase 12) | Gen V Create names the exact game at the top (e.g. "Box 1 / Slot 1 • White 2 • Source save immutable • DRAFT"), like Gen I–IV. | CG17 |
| [ ] | Review: one row per Move | A staged Move is ONE Review row in Gen III, IV and V (see §5). | Lane S `0f132eb` |

## 4. Round 1 PASS — regression check (must still pass)

All 42 are guarded on every pull request by
[`tests/test_hw_round1_regression_gate.py`](../../tests/test_hw_round1_regression_gate.py); the two marked
**device-only** have no host test that can exercise them and need the owner's eyes.

| Result | ID | Item | Owner's round 1 note |
| --- | --- | --- | --- |
| [ ] | C01 | A selects and advances normally | Home, Games, menus and editor |
| [ ] | C02 | B never quits without warning | Home shows Quit confirmation; A/B stay, Y quits |
| [ ] | C03 | Launch requires confirmation | ZR shows Launch dialog; Y launches, A/B cancel |
| [ ] | C04 | L/R behave correctly | Home changes games, no accidental save opening |
| [ ] | C05 | Gen II Create A/B fixed | A edits/selects, B backs out without wrong actions |
| [ ] | C06 | Popups no longer activate background controls | Including touchscreen and controller input |
| [ ] | C07 | Backup deletion requires Y confirmation | A alone never deletes |
| [ ] | C08 | Pokémon Release / Remove requires Y confirmation | Cancellation preserves Pokémon |
| [ ] | C09 | Second controller works properly | No A/B/R mix-ups on another Joy-Con or Pro Controller | **device-only: no host test can drive libnx pad input (pad setup pinned)**
| [ ] | M01 | Gen I Moves panel properly aligned | Four moves, PP, PP Ups and cursor |
| [ ] | M02 | Gen II Moves panel matches Gen I | Same columns and selection layout |
| [ ] | M03 | Gen III Moves panel matches | No shifted PP, clipped rows or hidden values |
| [ ] | M04 | Gen IV Moves panel matches | Same clean shared layout |
| [ ] | M05 | Gen V Moves panel matches | Same shared interface, appropriate native details |
| [ ] | M06 | Move chooser consistent across generations | Move number, name, type, power, accuracy and PP where applicable |
| [ ] | M07 | Move PP editing and PP Ups work | Correct generation-specific maximum PP |
| [ ] | M08 | Gen I hidden-row appearance fixed | All visible rows and cursor outlines fit within the panel | **device-only: guarded only by the closest list-fit / moves-panel tests**
| [ ] | B05 | Bikes, rods, Key Items and other unique items limited to one | No nonsensical quantities; TM/HM behavior respects the game |
| [ ] | B07 | Removing items requires confirmation | Y Remove, B Cancel |
| [ ] | B08 | Bag changes appear in Review and exported copy | No source-save overwrite |
| [ ] | E01 | Make Shiny removed from quick Actions | Shiny remains an editor field |
| [ ] | E02 | Actions menu order consistent | View, Edit, Clone, Move where supported, Release, Origin & Legality, Review |
| [ ] | E04 | Rows 1/15 counters removed | No text blocking the cursor |
| [ ] | E05 | Green A Select / red B Cancel footers consistent | Species, moves, item pickers and confirmations |
| [ ] | E06 | Shiny shows simply Yes / No | No DV-derived clutter; supported toggle works | Round 2: Shiny value in Edit/Create is now normal (editable) colour; gold now marks only read-only values such as HP DV and calculated stats.
| [ ] | E07 | Pokérus cycles inline with A | No unnecessary popup |
| [ ] | E08 | Gender editing works without a huge picker | Where applicable and format-valid |
| [ ] | E09 | Gen III language cycles inline | Only supported languages |
| [ ] | E10 | Y immediately rerolls DVs / IVs / EVs | No repeated confirmation dialog |
| [ ] | E12 | Gen I Create defaults to Pikachu Level 5 | Thunder Shock / Growl, correct PP |
| [ ] | E13 | Gen III PID-linked fields and Unown editing work | No unexpected nature, gender, ability or form changes |
| [ ] | L01 | Gen II DATA panel text stays inside its box | Caught history no longer overlaps the graph |
| [ ] | L02 | Crystal location picker is compact and readable | No huge popup or raw IDs |
| [ ] | L03 | Gen III location picker cleaned up | Readable names without (RSE), (FRLG) or ID numbers |
| [ ] | L04 | Gen IV/V location windows use clean layout | No clipped entries, scroll and focus work |
| [ ] | L05 | No editor panel cursor or row overlaps | Especially first and last visible rows |
| [ ] | N01 | First scroll press responds immediately | D-pad and stick |
| [ ] | N02 | Holding direction scrolls quickly and consistently | No quarter-second stall between each row |
| [ ] | N03 | Touch scrolling and joystick focus behave properly | No menus bleeding input into panels behind |
| [ ] | N04 | Long lists and keyboard repeat feel smooth | Species, moves, items and other menus |
| [ ] | S02 | Gen V party, boxes and trainer display correctly | Correct data instead of cached or blank information |
| [ ] | S05 | Original saves stay unchanged | Backup and edited copy exist separately |

## 5. Notes for this round (behaviour that changed on purpose)

- Review shows a staged Move as ONE row in Gen III, IV and V (Gen V: "Move Box a/Slot b → Box c/Slot d";
  Gen IV: "Moved <Pokémon>: …"; Gen III: "Move Pokemon: N Pokemon → …").
- Export Copy always also writes `EXPORT_MANIFEST.txt` next to the exported save.
- Shiny value in Edit/Create now uses the normal (editable) colour; gold now marks only read-only values
  (HP DV, calculated stats, PID).
- Gen IV: adding a Key Item shows a Key Item notice; A confirms the add, B cancels. This is new
  behaviour, not a regression.
- Documented limitations from round 1 still apply ([RETEST_7fea1797.md](RETEST_7fea1797.md) §5).

## Result

Report PASS / FAIL / NOT VERIFIED per box with notes and, for any failure, the input trace. This file is
the checklist only. Acceptance is recorded by the owner after testing this exact NRO.
