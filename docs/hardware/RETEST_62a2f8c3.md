# Hardware retest (round 3): PokeBank-NX-Gen1-5-Candidate-62a2f8c3

**Status: NOT DEVICE TESTED.**
- This build is not hardware accepted until the owner tests this exact file on a Switch.
- Only the owner records acceptance.
- The identity values below come from the QA-F9 exact-binary audit.

| Identity | Value |
| --- | --- |
| NRO file | `PokeBank-NX-Gen1-5-Candidate-62a2f8c3.nro` |
| Code SHA | `62a2f8c34476062ece344ef2188ab6b385369a7d` |
| Tree SHA | `8459b9a1b5c12666170c772c3ad2a17d7703f23f` |
| Build run | https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/39227970775 (run `39227970775`, success) |
| Artifact | `PokeBank-NX-Gen1-5-Candidate-62a2f8c3` (id `12175187129`) |
| NRO SHA-256 | `0c7a6fb01af2a1356676beedf3aec9da79842a499c7c63833fb683f42f332078` |
| NRO size | 176029361 bytes |
| QA-F9 audit run / job | https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/39233872565 (job `117998117020`): `NRO AUDIT: PASS` |

QA-F9 checked the following, on the binding commit `887552e` (docs only) of #307:
- `BUILD_IDENTITY.txt`, `SHA256SUMS.txt` and the NRO bytes all agree: `APPLICATION_SHA=62a2f8c3…`, `SIZE_BYTES=176029361`, `DEVICE_ACCEPTED=false`, `STATUS=NOT DEVICE TESTED`.
- The embedded commit is `62a2f8c3` (POKEVAULT_GIT_COMMIT).
- The NACP is `PokeBank NX 0.1.0-alpha`.
- The packaged RomFS has 4267 files (digest `bc09c152ac4232a2`).
- The cross-generation contracts in **candidate mode** report `0 contract(s) failing` across all 32 contracts, with no XFAIL left.

Only this artifact is the candidate. Later docs-only commits on #307 (such as this file) also trigger the
candidate workflow. Those artifacts are **not** the candidate.

**Before testing**
1. Check that the downloaded file's SHA-256 matches the value above.
2. For any input problem, use the input trace from [RETEST_7fea1797.md](RETEST_7fea1797.md) §0:
   - create `sdmc:/switch/PokeBank-NX/config/input_trace.enable`;
   - reproduce the problem;
   - send `sdmc:/switch/PokeBank-NX/logs/input-trace.log`.

**How to record results**
- Mark each box PASS / FAIL / NOT VERIFIED, and add notes for anything that is not PASS.
- "Host evidence" means automated host testing of this candidate's source only. It is never device acceptance.

Baselines:
- [ROUND1_RESULTS_7fea1797.md](ROUND1_RESULTS_7fea1797.md), 42 PASS.
- [ROUND2_RESULTS_e10f307f.md](ROUND2_RESULTS_e10f307f.md), 6 more PASS.

## Group A: previously accepted owner passes (regression check)

Host status on this candidate: `tests/test_hw_round1_regression_gate.py` PASS (138 guards), with all gate mutations PASS.

These 48 items were owner PASS on hardware: 42 on `7fea1797` and 6 on `e10f307f`.
- `tests/test_hw_round1_regression_gate.py` guards every one of them on every PR. Its mutation test is `tests/test_qa_hw_round1_gate_mutations.py`.
- On the round-3 candidate, confirm they still hold.
- A quick pass is enough unless something looks different.

| Result | ID | Accepted behaviour | Round | Gate guard (host) |
| --- | --- | --- | --- | --- |
| [ ] | C01 | A selects and advances normally | 1 | `test_gen2_workspace_layers`, `test_home_input_guard`, `test_game_hub_contract.py` |
| [ ] | C02 | B never quits without warning | 1 | `test_home_input_guard`, CG07, `test_game_hub_contract.py` |
| [ ] | C03 | Launch requires confirmation | 1 | CG07, `test_game_hub_contract.py` |
| [ ] | C04 | L/R behave correctly | 1 | `test_home_input_guard`, `test_game_hub_contract.py`, `test_home_card_provider_contract.py` |
| [ ] | C05 | Gen II Create A/B fixed | 1 | `test_gen2_workspace_layers` |
| [ ] | C06 | Popups no longer activate background controls | 1 | CG08, CG11, `test_shared_editor_unit`, `test_gen2_workspace_layers` |
| [ ] | C07 | Backup deletion requires Y confirmation | 1 | `test_home_input_guard`, `test_touch_appwide_surface_contract.py`, `test_touch_modal_list_contract.py`, `test_game_hub_contract.py` |
| [ ] | C08 | Pokémon Release / Remove requires Y confirmation | 1 | `test_shared_editor_unit`, CG06, `test_touch_pokemon_editor_direct_rows_contract.py` |
| [ ] | C09 | Second controller works properly | 1 | source pin |
| [ ] | M01 | Gen I Moves panel properly aligned | 1 | `test_gen2_moves_panel`, `test_shared_editor_unit`, `test_gen1_pokemon_editor_ui_contract` |
| [ ] | M02 | Gen II Moves panel matches Gen I | 1 | `test_gen2_moves_panel`, `test_shared_editor_unit` |
| [ ] | M03 | Gen III Moves panel matches | 1 | `test_gen3_hardware_surface_contract`, `test_shared_editor_unit` |
| [ ] | M04 | Gen IV Moves panel matches | 1 | `test_gen4_hardware_surface_contract`, `test_shared_editor_unit` |
| [ ] | M05 | Gen V Moves panel matches | 1 | `test_shared_editor_unit` |
| [ ] | M06 | Move chooser consistent across generations | 1 | `test_move_picker_presentation`, `test_shared_editor_unit`, `test_gen2_moves_panel` |
| [ ] | M07 | Move PP editing and PP Ups work | 1 | `test_gsc_pokemon_session`, `test_gen1_edit_draft`, `test_gen3_shared_pokemon_session`, `test_gen4_mutable_pokemon` |
| [ ] | M08 | Gen I hidden-row appearance fixed | 1 | `test_gen1_list_fit`, `test_gen1_pokemon_editor_ui_contract` |
| [ ] | B05 | Bikes, rods, Key Items and other unique items limited to one | 1 | CG09, `test_item_quantity_policy`, `test_shared_bag_quantity` |
| [ ] | B07 | Removing items requires confirmation | 1 | `test_shared_editor_unit`, `test_gen5_bag_screen` |
| [ ] | B08 | Bag changes appear in Review and exported copy | 1 | `test_gen1_export_transaction`, `test_gen5_bag_screen`, `test_qa_export_contracts`, `test_qa_matrix` |
| [ ] | E01 | Make Shiny removed from quick Actions | 1 | CG01, `test_classic_pokemon_action_menu`, `test_gen4_hardware_surface_contract` |
| see B | E02 | Actions menu order consistent (**changed this round: re-check in Group B**) | 1 | `test_classic_pokemon_action_menu`, `test_shared_editor_contract`, `test_gen4_hardware_surface_contract`, `test_qa_matrix` |
| [ ] | E04 | Rows 1/15 counters removed | 1 | CG02, `test_gen1_list_fit`, `test_shared_editor_unit` |
| [ ] | E05 | Green A Select / red B Cancel footers consistent | 1 | CG05, `test_shared_editor_unit` |
| [ ] | E06 | Shiny shows simply Yes / No | 1 | CG01b, `test_gen2_inline_fields`, `test_shared_editor_unit`, source pin |
| [ ] | E07 | Pokérus cycles inline with A | 1 | `test_gen2_inline_fields`, `test_shared_editor_unit` |
| [ ] | E08 | Gender editing works without a huge picker | 1 | `test_gen2_inline_fields`, `test_gen4_hardware_surface_contract` |
| [ ] | E09 | Gen III language cycles inline | 1 | `test_shared_editor_unit` |
| [ ] | E10 | Y immediately rerolls DVs / IVs / EVs | 1 | CG06, `test_gen12_dv_reroll`, `test_gen2_workspace_layers`, `test_shared_editor_unit` |
| [ ] | E12 | Gen I Create defaults to Pikachu Level 5 | 1 | `test_rby_pokemon_editor`, `test_qa_matrix` |
| [ ] | E13 | Gen III PID-linked fields and Unown editing work | 1 | `test_shared_stat_reroll`, `test_gen3_staged_pokemon_editor`, `test_gen3_pid_search_fail_closed` |
| [ ] | L01 | Gen II DATA panel text stays inside its box | 1 | `test_gsc_workspace_render`, `test_gen2_moves_panel` |
| [ ] | L02 | Crystal location picker is compact and readable | 1 | `test_gen2_location_picker`, CG04 |
| [ ] | L03 | Gen III location picker cleaned up | 1 | CG04, `test_shared_editor_unit` |
| [ ] | L04 | Gen IV/V location windows use clean layout | 1 | `test_shared_editor_unit`, source pin |
| [ ] | L05 | No editor panel cursor or row overlaps | 1 | `test_shared_editor_unit`, `test_gen2_moves_panel` |
| [ ] | N01 | First scroll press responds immediately | 1 | `test_navigation_repeat`, CG10 |
| [ ] | N02 | Holding direction scrolls quickly and consistently | 1 | `test_navigation_repeat`, CG10 |
| [ ] | N03 | Touch scrolling and joystick focus behave properly | 1 | CG08, `test_touch_modal_list_contract.py`, `test_touch_appwide_surface_contract.py` |
| [ ] | N04 | Long lists and keyboard repeat feel smooth | 1 | CG10, `test_navigation_repeat` |
| [ ] | S02 | Gen V party, boxes and trainer display correctly | 1 | `test_gen5_party_box_parity`, `test_gen5_trainer`, `test_qa_matrix` |
| [ ] | S05 | Original saves stay unchanged | 1 | `test_qa_export_contracts`, `test_qa_matrix` |
| [ ] | B01 | Gen I Add Item workflow improved (round 2 PASS) | 2 | CG12, `test_bag_add_flow_gen15`, `test_qa_matrix` |
| [ ] | B02 | Gen II Add Item workflow improved (round 2 PASS) | 2 | CG12, `test_bag_add_flow_gen15`, `test_qa_matrix` |
| [ ] | B03 | Gen III item editing still works correctly (round 2 PASS) | 2 | CG12, `test_bag_add_flow_gen15`, `test_qa_matrix` |
| [ ] | B06 | Stackable item quantity editing works (round 2 PASS) | 2 | CG09, CG12, `test_bag_add_flow_gen15`, `test_item_quantity_policy` |
| [ ] | S01 | Gen V matches the shared Gen I-IV editor: PP / PP Ups captions (round 2 PASS) | 2 | `test_shared_editor_unit` |
| [ ] | S03 | Gen V Create / Clone / Move / Release: left panel, parity (round 2 PASS) | 2 | CG16, CG17, `test_gen5_create_edit_parity` |

## Group B: new in round 3 (check on device)

| Result | ID | What should happen now | Host evidence (contract / test) | Landed on #307 at |
| --- | --- | --- | --- | --- |
| [ ] | E02 (re-check) | Actions menu order. The accepted order already names Origin & Legality: View, Edit, Clone, Move where supported, Release, Origin & Legality, Review, Cancel.<br>**Gen V** now shows Origin & Legality too, including on created, cloned and edited slots.<br>**Gen I** Box Actions now offer **Move**, which starts the Boxes carry, for parity with Gen II–V (R2-CHECK-009). | Hardware gate E02: pins the new Gen V list, plus the Gen I box sheet (`test_gen12_packed_move.cpp:470-474`; gate mutation `round 3 E02 Gen I box sheet loses Move`). Matrix `actions`: exact lists | 7ed1cc16 (Gen V); 9d4b8b7 (Gen I, C) |
| [ ] | R2-CHECK-009 | In the **Boxes view**, Gen I Box Actions → **Move** picks the Pokémon up in the shared Boxes carry. Y / A drops it, B cancels. Review still lists the staged move. Finding: Gen I was missing the Move route that Gen II–V have, so this is a routing repair, not a new mechanic. The read-only Storage pane does not offer Move. | `test_gen12_packed_move` (C `1dcf2cc3`): sheet order with and without Move; Move → Y drop stages one verified move; B leaves the box byte-identical; both live dispatchers route through `beginGen1FromAction`. The Storage pane keeps the old list (`test_classic_pokemon_action_menu`). | 11940524 / 9d4b8b7 (C) |
| [ ] | R2-BUG-008 | Gen III Met Location picker. For a Ruby / Sapphire / Emerald Pikachu Lv. 5 met at Lv. 5, the picker first offers **"keep stored"**, which keeps the record's own location and changes nothing. The Safari Zone template (Lv. 25–27) is still offered, labelled as not matching the stored met level, and it is not focused as if it were the origin. | `tests/test_gen3_encounter_keep_stored.cpp` (Makefile.host.gen14) | 5949b15 (C `479170d`) |
| [ ] | R2-BUG-001 | The Trainer page shows **A: Edit** whenever Name / Money are editable, including Gen I on a read-only source. | CG18 + CG18b | 11940524 (C `cccec27b`) |
| [ ] | R2-BUG-002 | Gen III and Gen IV Trainer Name / Money edits stage, show in Review, and appear in the exported copy. The source is unchanged. | CG24, `tests/test_gen3_trainer_edit.cpp`, Gen IV staged Trainer editor | 7ed1cc16 |
| [ ] | R2-BUG-003 | Gen V Trainer page uses the Gen I–IV layout: selectable cards (Name, Money, Play Time, Pokédex), a divider, then compact read-only rows. No scrolling. | `tests/test_gen5_create_edit_parity.cpp`, `tests/test_gen5_readonly_ui_contract.py` | 7ed1cc16 |
| [ ] | R2-BUG-004 | Changing a Pokémon's species replaces its moves with the new species' level-up moves for that game and level, in every generation. | CG19, `tests/test_species_change_moves_gen15.cpp` | 8a84128e (R) |
| [ ] | R2-BUG-005 / Part 9 #6 | TM/HM rows show the machine's move, with type, power, accuracy and PP. Gen V HM01–06 read **HM**, not TM93–98. Gen V Feathers have icons. | CG25, `tests/test_machine_moves_gen15.cpp` | 7ed1cc16 (I data); 9d4b8b7 (P render) |
| [ ] | R2-BUG-006 | Bag Add opens a compact ~4-column item grid that fits the screen, in every generation. | CG23, `tests/test_item_grid_picker.cpp` | 9d4b8b7 (P) |
| [ ] | R2-BUG-007 | The move chooser shows coloured type badges, like the summary page. Hidden Power and Curse may show text. | CG22, `tests/test_move_type_badges.cpp` | 9d4b8b7 (P) |
| [ ] | R2-BUG-012 | Interactive popups (value lists, Move chooser, Held Item grid, species picker, confirmations) use the same boxed rows as Pokémon Actions. | CG20 (Gen III–V), CG20b (Gen I/II + Classic inventory), CG20c (species picker), `tests/test_shared_boxed_rows.cpp` | 7ed1cc16 (S); 9d4b8b7 (P species picker) |
| [ ] | R2-BUG-013 | Help windows and instructions show coloured controller glyphs, not plain "A:" letters. This includes the Gen I–III Inventory Help. | CG21, `tests/test_glyph_instruction.cpp`, `tests/test_classic_inventory_ui_model.cpp` | aeccd13; 678627f (I, inventory help) |
| [ ] | Part 9 #1 | Gen V Clone / Move / Release stay available on created, cloned and edited Pokémon. Move uses the shared carry (Y). | `tests/test_gen5_staged_slot_actions.cpp`, matrix `actions` | 7ed1cc16 |
| [ ] | Part 9 #3 | Trainer / Game Browser header names the real emulator (DraStic, melonDS, RetroArch, mGBA…), never "EXTERNAL" / "RETROARCH" / a raw label. | CG26, `tests/test_session_provider_names.cpp` | aeccd13 |
| [ ] | Part 9 #4 | A melonDS / DeSmuME save launches the matching RetroArch core. When the core can't be proven, the dialog says so. | CG27 | 7ed1cc16 |
| [ ] | Part 9 #8 | Gen IV Storage view, save pane: Y tap / hold carries Pokémon, the drop lands at the visible cursor, and the carried Pokémon is drawn. | `tests/test_gen4_storage_carry.cpp` | 7ed1cc16 |
| [ ] | Part 9 #9 | Re-confirming the same profile keeps the focused game. The Games-browser profile avatar is tappable. "L/R Switch profile" help shows only with 2+ profiles. | CG28, `tests/test_session_provider_names.cpp` | aeccd13 |
| [ ] | S06 host probe | (Host only: an exported `edited.*` copy with one corrupted checksummed byte is rejected by the strict reparse, in all 20 games.) No device step; see Group C S06. | matrix `export.checksum` | 9d4b8b7 (QA matrix) |

Investigations R2-CHECK-010/011 are listed once their lanes report a code change.
R2-CHECK-010 already changes PC Items catalogs: R/S no longer offer Devon Goods / Letter / Orbs, and Emerald no longer offers HMs.

## Group C: still NOT VERIFIED (not failures)

Detailed steps and host evidence: [PHASE13_UNVERIFIED_STEPS_ROUND3.md](PHASE13_UNVERIFIED_STEPS_ROUND3.md).

| Result | ID | Item | Host evidence |
| --- | --- | --- | --- |
| [ ] | B04 | Gen IV / V Add Item match the shared flow | `test_bag_add_flow_gen15`, CG12, matrix `bag-add` |
| [ ] | E03 | Origin & Legality heading in Gen I–V (Gen V now has the row, Part 9 #1) | CG03 |
| [ ] | E11 | Rerolls keep the requested Shiny / Gender | CG06, `test_reroll_mechanics_gen15`, matrix `reroll` (20/20) |
| [ ] | L06 | Party / summary taps open the right Actions screen | `test_party_summary_tap_routing`, CG11 |
| [ ] | S04 | Gen V Review / Discard / Export | export contracts, `test_gen5_review_discard_export_trace` |
| [ ] | S06 | An exported edited save loads in an emulator. Copy it yourself into a separate save slot; the app never writes emulator saves. | matrix `export` + `export.checksum=detects-corruption` + `immutability` (20/20). Host only, no emulator claim. |
