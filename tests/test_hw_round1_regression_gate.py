#!/usr/bin/env python3
"""Hardware-accepted regression gate: the owner's 42 round 1 PASS items on 7fea1797 and the 6 round 2
PASS items on e10f307f (B01, B02, B03, B06, S01, S03; docs/hardware/ROUND2_RESULTS_e10f307f.md) are
frozen behaviour.

docs/hardware/ROUND1_RESULTS_7fea1797.md records the owner's sheet (54 items; 42 PASS). Every PASS ID
(C01-C09, M01-M08, B05/B07/B08, E01/E02/E04-E10/E12/E13, L01-L05, N01-N04, S02/S05) is mapped below
to the automated checks that guard it. The gate fails when:

  - the mapping is not exactly the owner's 42 PASS IDs;
  - an item has no guard that runs on every pull request;
  - a guard's file or its pinned literal is gone (the check was deleted or rewritten away);
  - a C++ guard is no longer built and run by `make -f Makefile.host host-test` (or `qa`), which
    .github/workflows/host-tests.yml / host-tests-clang.yml / app-qa-contracts.yml run on every PR;
  - a cross-generation contract (tests/test_qa_cross_gen_contract.py) that guards an item fails;
  - a Python contract guard fails (they are run here, so they gate even where their own workflow's
    branch filter would skip them);
  - a source pin (behaviour with no host-testable seam, e.g. C09 pad setup) changes.

A failing guard is never edited here to make the gate pass: fix the regression, or, when a change to a
frozen item is intended, get the coordinator's and owner's agreement and update the mapping with it.

Usage: python3 tests/test_hw_round1_regression_gate.py [--root DIR] [--list]
"""
from __future__ import annotations

import argparse
import re
import importlib.util
import subprocess
import sys
from pathlib import Path

DEFAULT_ROOT = Path(__file__).resolve().parent.parent
RESULTS_DOC = "docs/hardware/ROUND1_RESULTS_7fea1797.md"

# Guard kinds:
#   ("host", test_basename, path, literal): C++ test built and run by `make -f Makefile.host host-test`
#   ("qa",   test_basename, path, literal): C++ test built and run by `make -f Makefile.host qa`
#   ("cg",   contract_id):                  tests/test_qa_cross_gen_contract.py contract, must PASS
#   ("py",   path, literal):                Python contract, run by this gate, must exit 0
#   ("pin",  path, literal, why):           source pin for behaviour with no host-testable seam
G = list[tuple]
# Keyed by the owner's own IDs (docs/hardware/ROUND1_RESULTS_7fea1797.md, the owner's sheet): the 42 PASS.
GUARDS: dict[str, tuple[str, G]] = {
    'C01': ('A selects and advances normally', [
        ('host', 'test_gen2_workspace_layers', 'tests/test_gen2_workspace_layers.cpp', 'L::Action::EditField'),
        ('host', 'test_home_input_guard', 'tests/test_home_input_guard.cpp', 'Q::decide(Q::kButtonA) == Q::Decision::Stay'),
        ('py', 'tests/test_game_hub_contract.py', '"A", "OPEN"'),
    ]),
    'C02': ('B never quits without warning', [
        ('host', 'test_home_input_guard', 'tests/test_home_input_guard.cpp', 'if (bit == 3) assert(d == Q::Decision::Quit);'),
        ('cg', 'CG07-single-exit-and-launch-site'),
        ('py', 'tests/test_game_hub_contract.py', 'origin=home-root-b'),
    ]),
    'C03': ('Launch requires confirmation', [
        ('cg', 'CG07-single-exit-and-launch-site'),
        ('py', 'tests/test_game_hub_contract.py', 'openLaunchDecision(LeaveKind::LaunchTitle'),
    ]),
    'C04': ('L/R behave correctly', [
        ('host', 'test_home_input_guard', 'tests/test_home_input_guard.cpp', '// R, L, ZL, ZR'),
        ('py', 'tests/test_game_hub_contract.py', 'L/R must switch the selected game'),
        ('py', 'tests/test_home_card_provider_contract.py', 'The fast L/R path is a lookup only'),
    ]),
    'C05': ('Gen II Create A/B fixed', [
        ('host', 'test_gen2_workspace_layers', 'tests/test_gen2_workspace_layers.cpp', 'L::Action::AskExit'),
        ('host', 'test_gen2_workspace_layers', 'tests/test_gen2_workspace_layers.cpp', 'L::Action::ContinueEditing'),
    ]),
    'C06': ('Popups no longer activate background controls', [
        ('cg', 'CG08-modal-footer-and-touch-layer'),
        ('cg', 'CG11-no-generic-modern-ui-in-gen1-5'),
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'modal owns the touch layer and its footer: '),
        ('host', 'test_gen2_workspace_layers', 'tests/test_gen2_workspace_layers.cpp', 'void everyOverlapHasOneOwner'),
        ('py', 'tests/test_game_hub_contract.py', 'g_touchGlyphHits.clear();'),
    ]),
    'C07': ('Backup deletion requires Y confirmation', [
        ('host', 'test_home_input_guard', 'tests/test_home_input_guard.cpp', 'void quitDecisionTable()'),
        ('py', 'tests/test_touch_appwide_surface_contract.py', 'deleteDeleteBtn'),
        ('py', 'tests/test_touch_modal_list_contract.py', 'delete_modal = backup_code.split'),
        ('py', 'tests/test_game_hub_contract.py', 'Deleting an export record needs the explicit-Y decision'),
    ]),
    'C08': ('Pokémon Release / Remove requires Y confirmation', [
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'Y+B cancels'),
        ('cg', 'CG06-reroll-without-confirmation'),
        ('py', 'tests/test_touch_pokemon_editor_direct_rows_contract.py', 'Classic release: missing Y-only guard'),
    ]),
    'C09': ('Second controller works properly', [
        ('pin', 'src/UI/UI.cpp', 'padConfigureInput(1, HidNpadStyleSet_NpadStandard);', 'pad setup accepted on hardware (7fea1797)'),
        ('pin', 'src/UI/UI.cpp', 'padInitializeDefault(&pad);', 'pad setup accepted on hardware (7fea1797)'),
    ]),
    'M01': ('Gen I Moves panel properly aligned', [
        ('host', 'test_gen2_moves_panel', 'tests/test_gen2_moves_panel.cpp', 'Gen I touch button equals the focus outline'),
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'moves text inside panel: '),
        ('host', 'test_gen1_pokemon_editor_ui_contract', 'tests/test_gen1_pokemon_editor_ui_contract.cpp', '"\\"Legality\\""'),
    ]),
    'M02': ('Gen II Moves panel matches Gen I', [
        ('host', 'test_gen2_moves_panel', 'tests/test_gen2_moves_panel.cpp', 'touch row equals focus outline'),
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'moves text inside panel: '),
    ]),
    'M03': ('Gen III Moves panel matches', [
        ('host', 'test_gen3_hardware_surface_contract', 'tests/test_gen3_hardware_surface_contract.cpp', 'SharedMovesPanel::draw(fb, movesLayout'),
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'moves text inside panel: '),
    ]),
    'M04': ('Gen IV Moves panel matches', [
        ('host', 'test_gen4_hardware_surface_contract', 'tests/test_gen4_hardware_surface_contract.cpp', 'SharedMovesPanel::draw(fb, movesLayout'),
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'moves text inside panel: '),
    ]),
    'M05': ('Gen V Moves panel matches', [
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'GameVersion::BW, GameVersion::B2W2}) {'),
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'moves text inside panel: '),
    ]),
    'M06': ('Move chooser consistent across generations', [
        ('host', 'test_move_picker_presentation', 'tests/test_move_picker_presentation.cpp', 'SharedMovesPanel::drawChooserRow('),
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'chooser text inside modal'),
        ('host', 'test_gen2_moves_panel', 'tests/test_gen2_moves_panel.cpp', 'chooser shows the native Gen I/II type'),
    ]),
    'M07': ('Move PP editing and PP Ups work', [
        ('host', 'test_gsc_pokemon_session', 'tests/test_gsc_pokemon_session.cpp', 'PP edits use the same session methods as the UI'),
        ('host', 'test_gen1_edit_draft', 'tests/test_gen1_edit_draft.cpp', 'e.ppUps = std::array<uint8_t, 4>{3, 1, 0, 0};'),
        ('host', 'test_gen3_shared_pokemon_session', 'tests/test_gen3_shared_pokemon_session.cpp', 'request.ppUps[1] == 2'),
        ('host', 'test_gen4_mutable_pokemon', 'tests/test_gen4_mutable_pokemon.cpp', 'setPPUps(1, 3)'),
        ('host', 'test_gen5_pk5_readonly', 'tests/test_gen5_pk5_readonly.cpp', 'Field::MovePPUps'),
    ]),
    'M08': ('Gen I hidden-row appearance fixed', [
        ('host', 'test_gen1_list_fit', 'tests/test_gen1_list_fit.cpp', 'Gen1ListFit::wholeRows('),
        ('host', 'test_gen1_pokemon_editor_ui_contract', 'tests/test_gen1_pokemon_editor_ui_contract.cpp', '"\\"Legality\\""'),
    ]),
    'B05': ('Bikes, rods, Key Items and other unique items limited to one', [
        ('cg', 'CG09-item-quantity-policy'),
        ('host', 'test_item_quantity_policy', 'tests/test_item_quantity_policy.cpp', 'CHECK(r.maximum == 1);'),
        ('host', 'test_shared_bag_quantity', 'tests/test_shared_bag_quantity.cpp', 'assert(cap == 1'),
    ]),
    'B07': ('Removing items requires confirmation', [
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'if (itemRemoveConfirmActive) {'),
        ('host', 'test_gen5_bag_screen', 'tests/test_gen5_bag_screen.cpp', 'Y = Remove with confirmation'),
    ]),
    'B08': ('Bag changes appear in Review and exported copy', [
        ('host', 'test_gen1_export_transaction', 'tests/test_gen1_export_transaction.cpp', 'inventory->pendingChanges().size()'),
        ('host', 'test_gen5_bag_screen', 'tests/test_gen5_bag_screen.cpp', 'bagReviewChanges()'),
        ('qa', 'test_qa_export_contracts', 'tests/test_qa_export_contracts.cpp', '"gen1-pokemon+bag"'),
        ('qa', 'test_qa_matrix', 'tests/test_qa_matrix_gen3.cpp', 'exported Bag lacks Potion x7'),
        ('qa', 'test_qa_matrix', 'tests/test_qa_matrix_gen5.cpp', 'Bag review row differs'),
    ]),
    'E01': ('Make Shiny removed from quick Actions', [
        ('cg', 'CG01-no-make-shiny-quick-action'),
        ('host', 'test_classic_pokemon_action_menu', 'tests/test_classic_pokemon_action_menu.cpp', 'lacks(legacy, "Make Non-Shiny");'),
        ('host', 'test_gen4_hardware_surface_contract', 'tests/test_gen4_hardware_surface_contract.cpp', '"\\"Make Shiny\\""'),
    ]),
    'E02': ('Actions menu order consistent', [
        ('host', 'test_classic_pokemon_action_menu', 'tests/test_classic_pokemon_action_menu.cpp', '"View", "Edit", "Clone", "Release", "Origin & Legality"'),
        # Round 3 (R2-CHECK-009): the Gen I Boxes-view sheet adds Move in the accepted E02 slot; the
        # read-only Storage pane above keeps its list. Lane C's test pins both box sheets exactly.
        ('host', 'test_gen12_packed_move', 'tests/test_gen12_packed_move.cpp', 'std::vector<std::string>{"View", "Edit", "Clone", "Move", "Release", legality, "Cancel"}'),
        ('host', 'test_gen12_packed_move', 'tests/test_gen12_packed_move.cpp', '"Review Pending Changes", "Cancel"}));'),
        ('host', 'test_shared_editor_contract', 'tests/test_shared_editor_contract.cpp', 'void actionMenuContract()'),
        ('host', 'test_gen4_hardware_surface_contract', 'tests/test_gen4_hardware_surface_contract.cpp', '== "Origin & Legality"'),
        ('qa', 'test_qa_matrix', 'tests/test_qa_matrix_gen5.cpp', '{A::View, A::Edit, A::Clone, A::Move, A::Remove, A::LegalityProvenance, A::Close}'),
    ]),
    'E04': ('Rows 1/15 counters removed', [
        ('cg', 'CG02-no-row-counters'),
        ('host', 'test_gen1_list_fit', 'tests/test_gen1_list_fit.cpp', '"\\"Rows \\" + std::to_string("'),
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'no Rows N-M/T counter'),
    ]),
    'E05': ('Green A Select / red B Cancel footers consistent', [
        ('cg', 'CG05-picker-footer-helper'),
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'no ad-hoc A Choose footers'),
    ]),
    'E06': ('Shiny shows simply Yes / No', [
        ('cg', 'CG01b-shiny-editing-still-present'),
        ('host', 'test_gen2_inline_fields', 'tests/test_gen2_inline_fields.cpp', 'toggleShiny(screen, p); return true;'),
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'FI::Shiny'),
        ('pin', 'include/UI/SharedEditorPresentation.h', 'constexpr const char* yesNo(bool value) noexcept { return value ? "Yes" : "No"; }', 'shared Yes/No text'),
        ('pin', 'src/UI/Gen2UnifiedPokemonWorkspace.inc', '{"Shiny", p.shiny ? "Yes" : "No"}', 'Gen II Shiny row'),
        ('pin', 'src/UI/Gen3SharedPokemonSurface.inc', '{"Shiny", p.shiny ? "Yes" : "No"}', 'Gen III Shiny row'),
        ('pin', 'src/UI/Gen1PokemonDetailsPresentation.cpp', '"Shiny", p.shiny ? "Yes" : "No"', 'Gen I Shiny row'),
    ]),
    'E07': ('Pokérus cycles inline with A', [
        ('host', 'test_gen2_inline_fields', 'tests/test_gen2_inline_fields.cpp', 'SP::nextPokerusByte(raw, original)'),
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'P::pokerusState(0x13) == P::PokerusState::Infected'),
    ]),
    'E08': ('Gender editing works without a huge picker', [
        ('host', 'test_gen2_inline_fields', 'tests/test_gen2_inline_fields.cpp', 'chooseGender(screen, p);'),
        ('host', 'test_gen4_hardware_surface_contract', 'tests/test_gen4_hardware_surface_contract.cpp', 'genderUsesInlineToggle(Shared::Generation::Gen4)'),
    ]),
    'E09': ('Gen III language cycles inline', [
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'Gen III language ids'),
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'Gen III Language inline'),
    ]),
    'E10': ('Y immediately rerolls DVs / IVs / EVs', [
        ('cg', 'CG06-reroll-without-confirmation'),
        ('host', 'test_gen12_dv_reroll', 'tests/test_gen12_dv_reroll.cpp', 'lacks(text, "reroll.confirming");'),
        ('host', 'test_gen2_workspace_layers', 'tests/test_gen2_workspace_layers.cpp', 'Y re-rolls at once: one press, one roll'),
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'Gen IV/V Y re-roll'),
    ]),
    'E12': ('Gen I Create defaults to Pikachu Level 5', [
        ('host', 'test_rby_pokemon_editor', 'tests/test_rby_pokemon_editor.cpp', 'assert(defaults.species == 25 && defaults.level == 5);'),
        ('qa', 'test_qa_matrix', 'tests/test_qa_matrix_gen12.cpp', 'BoxPokemonCreate{} -> #25 L5'),
    ]),
    'E13': ('Gen III PID-linked fields and Unown editing work', [
        ('host', 'test_shared_stat_reroll', 'tests/test_shared_stat_reroll.cpp', 'Unown letter survives the PK3 encrypt/decrypt round trip'),
        ('host', 'test_gen3_staged_pokemon_editor', 'tests/test_gen3_staged_pokemon_editor.cpp', 'unown.form = 7;'),
        ('host', 'test_gen3_pid_search_fail_closed', 'tests/test_gen3_pid_search_fail_closed.cpp', 'Gen3PidSearch::find'),
    ]),
    'L01': ('Gen II DATA panel text stays inside its box', [
        ('host', 'test_gsc_workspace_render', 'tests/test_gsc_workspace_render.cpp', 'keeps all data text inside the data pane'),
        ('host', 'test_gen2_moves_panel', 'tests/test_gen2_moves_panel.cpp', 'text above the data pane: '),
    ]),
    'L02': ('Crystal location picker is compact and readable', [
        ('host', 'test_gen2_location_picker', 'tests/test_gen2_location_picker.cpp', 'locationPickerGeometry(count)'),
        ('cg', 'CG04-no-developer-wording'),
    ]),
    'L03': ('Gen III location picker cleaned up', [
        ('cg', 'CG04-no-developer-wording'),
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'Gen III: no PKHeX source tags or raw ids'),
    ]),
    'L04': ('Gen IV/V location windows use clean layout', [
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'Gen IV/V location shows no raw id'),
        ('pin', 'src/UI/SharedPokemonEditorSurface.inc', 'inline constexpr int kMetLocationPickerWidth = 620;', 'October 24 owner-directed compact Gen III-V location width; source context and scrolling retained (hardware retest pending)'),
    ]),
    'L05': ('No editor panel cursor or row overlaps', [
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'focus above data pane'),
        ('host', 'test_gen2_moves_panel', 'tests/test_gen2_moves_panel.cpp', 'touch row equals focus outline'),
    ]),
    'N01': ('First scroll press responds immediately', [
        ('host', 'test_navigation_repeat', 'tests/test_navigation_repeat.cpp', '// immediate first move'),
        ('cg', 'CG10-single-navigation-repeat-engine'),
    ]),
    'N02': ('Holding direction scrolls quickly and consistently', [
        ('host', 'test_navigation_repeat', 'tests/test_navigation_repeat.cpp', 'static_assert(delay >= 200 && delay <= 300'),
        ('cg', 'CG10-single-navigation-repeat-engine'),
    ]),
    'N03': ('Touch scrolling and joystick focus behave properly', [
        ('cg', 'CG08-modal-footer-and-touch-layer'),
        ('py', 'tests/test_touch_modal_list_contract.py', 'delete_modal = backup_code.split'),
        ('py', 'tests/test_touch_appwide_surface_contract.py', 'deleteDeleteBtn'),
    ]),
    'N04': ('Long lists and keyboard repeat feel smooth', [
        ('cg', 'CG10-single-navigation-repeat-engine'),
        ('host', 'test_navigation_repeat', 'tests/test_navigation_repeat.cpp', 'static_assert(delay >= 200 && delay <= 300'),
    ]),
    'S02': ('Gen V party, boxes and trainer display correctly', [
        ('host', 'test_gen5_party_box_parity', 'tests/test_gen5_party_box_parity.cpp', 'void retailEncryptedBlankSlots('),
        ('host', 'test_gen5_trainer', 'tests/test_gen5_trainer.cpp', 'void hubRoutes()'),
        ('qa', 'test_qa_matrix', 'tests/test_qa_matrix_gen5.cpp', 'Gen IV origin Eevee'),
    ]),
    'S05': ('Original saves stay unchanged', [
        ('qa', 'test_qa_export_contracts', 'tests/test_qa_export_contracts.cpp', '"C1 source file changed "'),
        ('qa', 'test_qa_matrix', 'tests/test_qa_matrix_gen12.cpp', 'row.check("immutability"'),
        ('qa', 'test_qa_matrix', 'tests/test_qa_matrix_gen5.cpp', '"immutability"'),
    ]),
}

OWNER_PASS = ['C01', 'C02', 'C03', 'C04', 'C05', 'C06', 'C07', 'C08', 'C09', 'M01', 'M02', 'M03', 'M04', 'M05', 'M06', 'M07', 'M08', 'B05', 'B07', 'B08', 'E01', 'E02', 'E04', 'E05', 'E06', 'E07', 'E08', 'E09', 'E10', 'E12', 'E13', 'L01', 'L02', 'L03', 'L04', 'L05', 'N01', 'N02', 'N03', 'N04', 'S02', 'S05']
OWNER_NOT_PASS = ['B01', 'B02', 'B03', 'B04', 'B06', 'E03', 'E11', 'L06', 'S01', 'S03', 'S04', 'S06']


# Round 2 (owner on e10f307f, docs/hardware/ROUND2_RESULTS_e10f307f.md): the six round 1 FAILs that the
# owner retested and passed are now hardware-accepted and frozen like the 42 above.
ROUND2_DOC = "docs/hardware/ROUND2_RESULTS_e10f307f.md"
GUARDS_ROUND2: dict[str, tuple[str, G]] = {
    'B01': ('Gen I Add Item workflow improved (round 2 PASS)', [
        ('cg', 'CG12-single-bag-add-flow'),
        ('host', 'test_bag_add_flow_gen15', 'tests/test_bag_add_flow_gen15.cpp', 'CHECK(pick.outcome == BagAdd::PickOutcome::StageNew && pick.quantity == 1);'),
        ('qa', 'test_qa_matrix', 'tests/test_qa_matrix_gen12.cpp', 'r2cell(row, "bag-add",'),
    ]),
    'B02': ('Gen II Add Item workflow improved (round 2 PASS)', [
        ('cg', 'CG12-single-bag-add-flow'),
        ('host', 'test_bag_add_flow_gen15', 'tests/test_bag_add_flow_gen15.cpp', 'CHECK(pick.outcome == BagAdd::PickOutcome::StageNew && pick.quantity == 1);'),
        ('qa', 'test_qa_matrix', 'tests/test_qa_matrix_gen12.cpp', 'r2cell(row, "bag-add",'),
    ]),
    'B03': ('Gen III item editing still works correctly (round 2 PASS)', [
        ('cg', 'CG12-single-bag-add-flow'),
        ('host', 'test_bag_add_flow_gen15', 'tests/test_bag_add_flow_gen15.cpp', 'CHECK(pick.outcome == BagAdd::PickOutcome::StageNew && pick.quantity == 1);'),
        ('qa', 'test_qa_matrix', 'tests/test_qa_matrix_gen3.cpp', 'guarded("bag-add",'),
    ]),
    'B06': ('Stackable item quantity editing works (round 2 PASS)', [
        ('cg', 'CG09-item-quantity-policy'),
        ('cg', 'CG12-single-bag-add-flow'),
        ('host', 'test_bag_add_flow_gen15', 'tests/test_bag_add_flow_gen15.cpp', 'BagAdd::RowOutcome::FixedQuantity : BagAdd::RowOutcome::EditQuantity));'),
        ('host', 'test_item_quantity_policy', 'tests/test_item_quantity_policy.cpp', 'CHECK(r.maximum == 1);'),
    ]),
    'S01': ('Gen V matches the shared Gen I-IV editor: PP / PP Ups captions (round 2 PASS)', [
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'S01: PP and PP Ups captions are at least 20 px apart'),
        ('host', 'test_shared_editor_unit', 'tests/test_shared_editor_unit.cpp', 'S01: PP values never reach the move name column'),
    ]),
    'S03': ('Gen V Create / Clone / Move / Release: left panel, parity (round 2 PASS)', [
        ('cg', 'CG16-shared-field-state'),
        ('cg', 'CG17-exact-source-game-header'),
        ('host', 'test_gen5_create_edit_parity', 'tests/test_gen5_create_edit_parity.cpp', 'S03 label fits the column: '),
        ('host', 'test_gen5_create_edit_parity', 'tests/test_gen5_create_edit_parity.cpp', 'S03 Gen V row '),
    ]),
}
ROUND2_PASS = list(GUARDS_ROUND2)
ROUND2_NOT_VERIFIED = ['B04', 'E03', 'E11', 'L06', 'S04', 'S06']

# Round 3 (owner on 62a2f8c3, docs/hardware/ROUND3_RESULTS_62a2f8c3.md): 70 PASS. The 48 above that the owner
# passed again stay guarded as they are; these are the round 3 PASS items that had no guard yet. Each pins
# behaviour that is in the code today (a test that runs on every PR, or an enforced cross-gen contract).
# Round 1 items the owner FAILED in round 3 (C01, C07, M04, M05, B08, E02, E05, S05) keep their round 1
# guards: those pin what was accepted; the new failures are round 3 contracts (XFAIL until fixed).
ROUND3_DOC = "docs/hardware/ROUND3_RESULTS_62a2f8c3.md"
GUARDS_ROUND3: dict[str, tuple[str, G]] = {
    'B04': ('Gen IV/V Add Item (round 3 PASS)', [
        ('cg', 'CG12-single-bag-add-flow'),
        ('host', 'test_bag_add_flow_gen15', 'tests/test_bag_add_flow_gen15.cpp', 'CHECK(G4::gen4BagItemAllowed(layout, pocket, c.id));'),
        ('qa', 'test_qa_matrix', 'tests/test_qa_matrix_gen4.cpp', 'cell(m, ver, "bag-add",'),
        ('qa', 'test_qa_matrix', 'tests/test_qa_matrix_gen5.cpp', 'runCell(m, v, "bag-add", [&] { return bagAdd(c); });'),
    ]),
    'S04': ('Gen V Review / Discard / Export (round 3 PASS)', [
        ('host', 'test_gen5_review_discard_export_trace', 'tests/test_gen5_review_discard_export_trace.cpp', 'T + "Export asks for confirmation");'),
        ('host', 'test_gen5_review_discard_export_trace', 'tests/test_gen5_review_discard_export_trace.cpp', 'T + "Discard All asks for confirmation");'),
        ('host', 'test_gen5_review_discard_export_trace', 'tests/test_gen5_review_discard_export_trace.cpp', 'T + "cancel Review/Export/Discard keeps every staged byte");'),
    ]),
    'R2-MENUS': ('Compact selection windows (round 3 PASS)', [
        ('cg', 'CG13-shared-menu-geometry'),
        ('host', 'test_shared_menu_geometry', 'tests/test_shared_menu_geometry.cpp', '"SaveDiscard rule = the accepted 560 px dialog and its two buttons");'),
    ]),
    'R2-SEARCH': ('Species live search (round 3 PASS)', [
        ('cg', 'CG15-shared-species-search'),
        ('host', 'test_species_search_gen15', 'tests/test_species_search_gen15.cpp', 'assert(Search::searchKey("Mr. Mime") == "MRMIME");'),
    ]),
    'R2-GOLD': ('Read-only field colours (round 3 PASS)', [
        ('cg', 'CG16-shared-field-state'),
    ]),
    'R2-LAUNCH': ('Game launch confirmation (round 3 PASS)', [
        ('cg', 'CG07-single-exit-and-launch-site'),
        ('py', 'tests/test_game_hub_contract.py', 'def require(cond: bool, message: str) -> None:'),
    ]),
    'R2-SCROLLBAR': ('Touch scrollbar dragging (round 3 PASS)', [
        ('cg', 'CG14-shared-draggable-scrollbar'),
        ('host', 'test_shared_scrollbar', 'tests/test_shared_scrollbar.cpp', 'assert(r.consumed && !r.changed && o.bar.dragging);'),
    ]),
    'R2-MOVECHOOSER': ('Extra rows in the Move list (round 3 PASS)', [
        ('host', 'test_shared_menu_geometry', 'tests/test_shared_menu_geometry.cpp', 'move.frame.w == 760 && move.frame.h == 520 && move.visibleRows == 10'),
    ]),
    'R2-CRYSTAL': ('Crystal Met Time (round 3 PASS)', [
        ('qa', 'test_qa_matrix', 'tests/test_qa_matrix_gen12.cpp', 'r2cell(row, "met-time",'),
    ]),
    'R2-GEN5HEADER': ('Gen V exact-game title (round 3 PASS)', [
        ('cg', 'CG17-exact-source-game-header'),
        ('host', 'test_gen5_create_edit_parity', 'tests/test_gen5_create_edit_parity.cpp', 'std::string("S03 Create header names the game: ") + id);'),
    ]),
    'R2-BUG-001': ('Trainer Edit button (round 3 PASS)', [
        ('cg', 'CG18-trainer-edit-footer'),
        ('cg', 'CG18b-trainer-footer-not-overridden'),
    ]),
    'R2-BUG-003': ('Gen V Trainer clutter (round 3 PASS)', [
        ('host', 'test_gen5_create_edit_parity', 'tests/test_gen5_create_edit_parity.cpp', '"R2-BUG-003 Trainer page: editable cards, divider, identity rows, language detail");'),
    ]),
    'R2-BUG-004': ('Species change resets moves (round 3 PASS)', [
        ('cg', 'CG19-species-change-resets-moves'),
        ('host', 'test_species_change_moves_gen15', 'tests/test_species_change_moves_gen15.cpp', 'require(moves(t, 25, 5) == (std::array<uint16_t, 4>{84, 45, 0, 0}), "Gen I Pikachu Lv5 = Create default");'),
    ]),
    'R2-BUG-005': ('TM/HM move information (round 3 PASS)', [
        ('cg', 'CG25-gen5-machine-and-feather-art'),
        ('host', 'test_machine_moves_gen15', 'tests/test_machine_moves_gen15.cpp', 'CHECK(Names::machineDisplayLabel(GameVersion::B2W2, 420, Names::getItemName(420)).rfind("HM01", 0) == 0);'),
    ]),
    'R2-BUG-006': ('Compact item grid (round 3 PASS)', [
        ('cg', 'CG23-bag-add-grid-picker'),
        ('host', 'test_item_grid_picker', 'tests/test_item_grid_picker.cpp', 'require(g.visibleRows >= 1 && g.visibleRows <= Grid::kMaxRows, "1..10 visible rows");'),
    ]),
    'R2-BUG-007': ('Coloured Move types (round 3 PASS)', [
        ('cg', 'CG22-move-chooser-type-badges'),
        ('host', 'test_move_type_badges', 'tests/test_move_type_badges.cpp', 'require(b.typeId == f.type, "badge is the move\'s own type sprite");'),
    ]),
    'R2-BUG-012': ('All clickable popup rows boxed (round 3 PASS)', [
        ('cg', 'CG20-shared-boxed-modal-rows'),
        ('cg', 'CG20b-shared-boxed-modal-rows-gen12-classic'),
        ('cg', 'CG20c-shared-boxed-species-picker'),
        ('host', 'test_shared_boxed_rows', 'tests/test_shared_boxed_rows.cpp', '"reference focus colour is the yellow PartyNumber");'),
    ]),
    'R2-BUG-013': ('Coloured controller buttons in Help (round 3 PASS)', [
        ('cg', 'CG21-help-lines-use-button-glyphs'),
        ('host', 'test_glyph_instruction', 'tests/test_glyph_instruction.cpp', 'assert(text(GI::helpLine("A   Open or show Pokémon actions")) == "[A]  Open or show Pokémon actions");'),
    ]),
    'R2-BUG-008': ('Ruby Pikachu location investigation (round 3 PASS)', [
        ('host', 'test_gen3_encounter_keep_stored', 'tests/test_gen3_encounter_keep_stored.cpp', 'assert(rows.size() == 2 && rows[0] == Picker::kKeepStoredRow && rows[1] == 0);'),
    ]),
    'R2-CHECK-009': ('Yellow Move Pokemon reachable (round 3 PASS)', [
        ('host', 'test_gen12_packed_move', 'tests/test_gen12_packed_move.cpp', 'std::vector<std::string>{"View", "Edit", "Clone", "Move", "Release", legality, "Cancel"}'),
    ]),
    'R2-CHECK-010': ('Sapphire Super Rod pocket warning (round 3 PASS)', [
        ('host', 'test_bag_add_flow_gen15', 'tests/test_bag_add_flow_gen15.cpp', 'CHECK(BagAdd::needsKeyItemWarning(QP::quantityRule(c.game, c.pocket, c.id)) == c.warns);'),
        ('host', 'test_bag_add_flow_gen15', 'tests/test_bag_add_flow_gen15.cpp', 'cases.push_back({{"PC Super Rod", QP::Pocket::PCItems, 264, true, 1}, InventoryPouch::PCItems});'),
    ]),
    'NEW-01': ('Help closes with B (round 3 PASS)', [
        ('cg', 'CG29-help-closes-with-b'),
    ]),
    'AUD-02': ('Species sprites and restrictions (round 3 PASS)', [
        ('cg', 'CG15-shared-species-search'),
        ('host', 'test_shared_species_picker_surface_contract', 'tests/test_shared_species_picker_surface_contract.cpp', '"Shared Gen I/II/III species-picker modal chrome contract: PASS\\n"'),
    ]),
    'AUD-03': ('Emulator names (round 3 PASS)', [
        ('cg', 'CG26-trainer-provider-display-name'),
        ('host', 'test_home_card_providers', 'tests/test_home_card_providers.cpp', '"Home cards: real providers Gen I-V, measured labels, fingerprinted snapshot "'),
        ('host', 'test_session_provider_names', 'tests/test_session_provider_names.cpp', '"Part 9: each provider (mGBA, RetroArch + DeSmuME core, melonDS, DraStic, Switch title) named "'),
    ]),
    'AUD-05': ('Quick View touch (round 3 PASS)', [
        ('host', 'test_touch_scroll_state', 'tests/test_touch_scroll_state.cpp', '"touch-scroll runtime geometry: PASS\\n"'),
        ('py', 'tests/test_touch_appwide_surface_contract.py', 'print(f"touch app-wide surface inventory: PASS ({len(SURFACES)} surface families)")'),
    ]),
    'AUD-06': ('Item sprites (round 3 PASS)', [
        ('cg', 'CG25-gen5-machine-and-feather-art'),
        ('py', 'tests/test_item_sprite_coverage.py', 'print("Generation I-IV item catalog sprite resolution: PASS (zero unresolved native item labels)")'),
    ]),
    'AUD-07': ('Gen V assigned to profile (round 3 PASS)', [
        ('host', 'test_gen5_source_e2e', 'tests/test_gen5_source_e2e.cpp', '"original immutable, stale refused, .dsv backed up verbatim PASS\\n"'),
        ('host', 'test_gen5_readonly_save', 'tests/test_gen5_readonly_save.cpp', '"Gen V persistent copies: 4 titles x 2 profiles, rollback, relink, isolation and immutability PASS\\n"'),
    ]),
    'AUD-08': ('Gen IV Y multi-move (round 3 PASS)', [
        ('host', 'test_gen4_hardware_surface_contract', 'tests/test_gen4_hardware_surface_contract.cpp', '"Gen IV shared editor hardware surface contract PASS\\n"'),
        ('host', 'test_gen4_storage_carry', 'tests/test_gen4_storage_carry.cpp', '"Gen IV Storage save-pane Move / Y carry (Part 9 item 8, "'),
    ]),
    'AUD-09': ('Home Profile navigation (round 3 PASS)', [
        ('cg', 'CG28-same-profile-keeps-game-focus'),
        ('py', 'tests/test_game_hub_contract.py', 'require(\'drawNavBar(fb, {{"L/R", "Change Game"}, {"A", "Open"}, {"Y", "Quick Games"}\' in source,'),
    ]),
    'AUD-10': ('Region backgrounds included (round 3 PASS)', [
        ('py', 'tests/test_qa_regressions_assets.py', 'print(f"PASS  QA-F1: all {len(referenced)} source-referenced RomFS assets are in the packaged manifest")'),
    ]),
}
ROUND3_PASS_COUNT = 70

# Round 5 (owner on the round 4 candidate c60d4da0, Oct 24; docs/hardware/OCT24_REPAIRS_AND_RETEST.md,
# "Historical owner status mapping"): 27 PASS, all of them round 3 FAILs. Each guard below already held on
# c60d4da0, the exact source the owner tested, and holds on the head under test, except two that follow
# later owner-directed work: CG40 now pins the docked Held Item keyboard that replaced the accepted live
# search prompt (Oct 24 request), and test_stats_label_neutral_contract was added after c60d4da0 (its
# CG43 / pin companions held there). Round 1 items the owner
# passed again (C07, M04, B08, E02, E05) keep their round 1 guards and gain these; the 6 Oct 24 FAILs and
# the 4 unmarked items are round 5 contracts (XFAIL until fixed). Nature colours stay green / red (NEW-09).
ROUND5_DOC = "docs/hardware/OCT24_REPAIRS_AND_RETEST.md"
ROUND5_PASS_COUNT = 27
GUARDS_ROUND5: dict[str, tuple[str, G]] = {
    'E03': ('Origin & Legality wording and one truthful panel (Oct 24 PASS)', [
        ('cg', 'CG03-origin-legality-heading'),
        ('cg', 'CG48-origin-legality-truthful-state'),
        ('cg', 'CG49-one-origin-legality-panel'),
    ]),
    'L06': ('Touch Party / summary opens Actions (Oct 24 PASS)', [
        ('host', 'test_party_summary_tap_routing', 'tests/test_party_summary_tap_routing.cpp', 'assert(B4::SummaryPanelTouchId == 2000);'),
        ('cg', 'CG33-party-opens-actions'),
    ]),
    'R2-TRAINER': ('Gen I Trainer Name / Money reach Review (Oct 24 PASS)', [
        ('cg', 'CG30-trainer-page-opens-review'),
    ]),
    'C07': ('Backup browser actions and deletion (Oct 24 PASS)', [
        ('cg', 'CG59-backup-browser-actions'),
        ('cg', 'CG60-no-backup-on-unchanged-open'),
    ]),
    'M04': ('Gen IV move Category spacing (Oct 24 PASS)', [
        ('cg', 'CG52-gen45-move-category-spacing'),
    ]),
    'B08': ('Export outcome visible (Oct 24 PASS)', [
        ('cg', 'CG58-export-outcome-visible'),
    ]),
    'E02': ('Pokemon Actions present in order (Oct 24 PASS)', [
        ('cg', 'CG33-party-opens-actions'),
    ]),
    'E05': ('Footer glyphs and Help close on B (Oct 24 PASS)', [
        ('cg', 'CG36-footer-hints-have-glyphs'),
        ('cg', 'CG29-help-closes-with-b'),
    ]),
    'R2-BUG-002': ('Gen III/IV Trainer Review and staged exit (Oct 24 PASS)', [
        ('cg', 'CG30-trainer-page-opens-review'),
        ('cg', 'CG31-classic-exit-offers-review'),
    ]),
    'R2-CHECK-011': ('Black 2 location choices are context-backed (Oct 24 PASS)', [
        ('cg', 'CG53-gen45-met-location-context'),
        ('host', 'test_gen45_encounter_context', 'tests/test_gen45_encounter_context.cpp', 'assert(pikachu[25] && pikachu[172] && !pikachu[26]);'),
    ]),
    'NEW-02': ('No Touch:Back footer, touch Back kept (Oct 24 PASS)', [
        ('cg', 'CG37-no-touch-back-footer'),
    ]),
    'NEW-03': ('Bag Remove / Key Item warning glyphs, A commits (Oct 24 PASS)', [
        ('cg', 'CG38-bag-confirm-glyphs-a-commits'),
    ]),
    'NEW-04': ('Gen II PC Move completes (Oct 24 PASS)', [
        ('cg', 'CG56-gen2-carry-refresh-without-finalize'),
        ('host', 'test_gen2_box_move_presentation', 'tests/test_gen2_box_move_presentation.cpp', 'assert(editor->packedMoveActive());'),
    ]),
    'NEW-05': ('Species keyboard beside the list (Oct 24 PASS)', [
        ('cg', 'CG39-species-search-keyboard-beside-list'),
    ]),
    'NEW-06': ('Held Item search with Sort (Oct 24 PASS)', [
        ('cg', 'CG40-held-item-picker-search'),
    ]),
    'NEW-07': ('Crystal OT Gender cycles inline (Oct 24 PASS)', [
        ('cg', 'CG41-gen2-ot-gender-inline'),
    ]),
    'NEW-08': ('Ball picker scrollbar gutter (Oct 24 PASS)', [
        ('cg', 'CG42-ball-picker-scrollbar-gutter'),
    ]),
    'NEW-09': ('Nature stat colours: raised green, lowered red (Oct 24 PASS)', [
        ('cg', 'CG43-nature-stat-colours'),
        ('host', 'test_stats_label_neutral_contract', 'tests/test_stats_label_neutral_contract.cpp', 'assert(stats.find("NatureStats::statColor(A::nature(*raw), row") != std::string::npos);'),
        ('host', 'test_stats_label_neutral_contract', 'tests/test_stats_label_neutral_contract.cpp', 'assert(nature.find("Colors::Success") != std::string::npos);'),
        ('host', 'test_stats_label_neutral_contract', 'tests/test_stats_label_neutral_contract.cpp', 'assert(nature.find("Colors::Error") != std::string::npos);'),
        ('pin', 'include/UI/NatureStatPresentation.h', 'return modifier > 0 ? Colors::Success : modifier < 0 ? Colors::Error : normal;', 'owner Oct 24: keep Nature-based green/red stat colours'),
    ]),
    'NEW-11': ('Selected species reads name first (Oct 24 PASS)', [
        ('cg', 'CG45-selected-species-name-first'),
    ]),
    'NEW-12': ('Gen IV/V location choices are context-backed (Oct 24 PASS)', [
        ('cg', 'CG53-gen45-met-location-context'),
        ('host', 'test_gen45_encounter_context', 'tests/test_gen45_encounter_context.cpp', 'assert(walrein[365] && walrein[364] && walrein[363] && !walrein[362]);'),
    ]),
    'NEW-13': ('Ability picker shows names and effects (Oct 24 PASS)', [
        ('cg', 'CG46-ability-names-not-slots'),
    ]),
    'NEW-15': ('Create defaults IVs / DVs 0 (Oct 24 PASS)', [
        ('cg', 'CG55-create-defaults-zero-ivs-dvs'),
    ]),
    'NEW-16': ('Gen V Home A opens the assigned save (Oct 24 PASS)', [
        ('cg', 'CG57-gen5-home-a-opens-assigned-save'),
    ]),
    'NEW-17': ('Gen V Key Items / TMs editable (Oct 24 PASS)', [
        ('cg', 'CG34-gen5-key-tm-pockets-editable'),
    ]),
    'NEW-18': ('Classic Trainer Play Time and Pokedex (Oct 24 PASS)', [
        ('cg', 'CG47-classic-trainer-playtime-pokedex'),
    ]),
    'NEW-19': ('Gen I-IV staged exit menu (Oct 24 PASS)', [
        ('cg', 'CG31-classic-exit-offers-review'),
    ]),
    'NEW-20': ('Gen I-III Party Actions popup (Oct 24 PASS)', [
        ('cg', 'CG33-party-opens-actions'),
    ]),
}


# Round 6 (owner on the round 5 candidate 1ca2faec, Oct 25; docs/hardware/ROUND5_RESULTS_1ca2faec.md):
# 31 PASS. 21 were already Oct 24 PASS (GUARDS_ROUND5); the 10 below are new. Several are being
# restyled in round 6 at the owner's request (7A Gen II Special layout, 7B Gen V move label, 7C Language
# toast, P2 Trainer layout, P3/P4 Review entry and Help), so these guards pin BEHAVIOUR (calculation,
# staging, inline cycling, native offsets, immutability), never the old visuals. Every guard held on
# 1ca2faec except the Safari Ball / Berry Juice held-item refusal, which is the owner's Oct 25 decision
# (PKHeX ItemStorage3RS.Unreleased, fix 15c77226, on #307 at a5db3368). The Oct 24 PASS items
# the owner failed on Oct 25 (L06, R2-TRAINER, B08, E05, R2-CHECK-011, NEW-18) keep their guards: those pin
# what was accepted, and the new failures are round 6 contracts (CG69+, XFAIL until fixed).
ROUND6_DOC = "docs/hardware/ROUND5_RESULTS_1ca2faec.md"
ROUND6_PASS_COUNT = 31
GUARDS_ROUND6: dict[str, tuple[str, G]] = {
    'R2-SPECIAL': ('Gen II one Special DV / Stat Exp, separate calculated SpA and SpD (Oct 25 PASS)', [
        ('host', 'test_gen2_special_stat', 'tests/test_gen2_special_stat.cpp', 'assert(battle[4] == spa && battle[5] == spd && spa != spd);'),
        ('cg', 'CG51-gen2-special-not-slashed'),
    ]),
    'R2-REVIEWMOVE': ('Gen III Create: every caught Ball incl. Safari, released held items only, named Review (Oct 25 PASS)', [
        ('host', 'test_gen3_staged_pokemon_editor', 'tests/test_gen3_staged_pokemon_editor.cpp', 'assert(persisted && persisted->heldItem==held && persisted->ball==ball);'),
        ('host', 'test_gen3_staged_pokemon_editor', 'tests/test_gen3_staged_pokemon_editor.cpp', 'assert(!releasedHeldItem(5) && !releasedHeldItem(44));'),
        ('cg', 'CG32-review-rows-readable-names'),
    ]),
    'M05': ('Gen V species- and form-specific move pools (Oct 25 PASS)', [
        ('host', 'test_gen5_move_compatibility', 'tests/test_gen5_move_compatibility.cpp', 'assert(bwDirect==55 && b2Direct==64);'),
        ('host', 'test_gen5_move_compatibility', 'tests/test_gen5_move_compatibility.cpp', 'assert(C::direct(game,235,0,459)); // Smeargle is the genuine Sketch exception.'),
    ]),
    'S05': ('Original saves never change; backups open through a copy (Oct 25 PASS)', [
        ('qa', 'test_qa_matrix', 'tests/test_qa_matrix_gen12.cpp', 'row.check("immutability", qacontract::fileState(source) == sourceBefore,'),
        ('cg', 'CG61-switch-backup-opens-staged-copy'),
        ('cg', 'CG60-no-backup-on-unchanged-open'),
    ]),
    'NEW-10': ('Met Location rows fit the compact window (Oct 25 PASS)', [
        ('host', 'test_met_location_fit', 'tests/test_met_location_fit.cpp', 'assert(widest <= kTextRoom);'),
        ('cg', 'CG62-met-location-rows-fit'),
    ]),
    'NEW-14': ('Gen V Language cycles inline as a staged native edit (Oct 25 PASS)', [
        ('host', 'test_gen5_language_cycle', 'tests/test_gen5_language_cycle.cpp', 'assert(branch.find("stage(screen, Field::Language, 0, PokeBank::UIModel::Gen5LanguageCycle::next(p.language()))")!=std::string::npos);'),
        ('host', 'test_gen5_language_cycle', 'tests/test_gen5_language_cycle.cpp', 'assert(branch.find("postStatus(")==std::string::npos);'),
        ('host', 'test_gen5_language_cycle', 'tests/test_gen5_language_cycle.cpp', 'assert(branch.find("openPicker(")==std::string::npos);'),
        ('cg', 'CG54-gen5-language-editable'),
    ]),
    'R2-EMUBACKUP': ('Backups: newest first, A opens the exact copy, original protected (Oct 25 PASS)', [
        ('host', 'test_emulator_backup_access', 'tests/test_emulator_backup_access.cpp', 'assert(B::deleteRecord(l, 0) == B::Refusal::OriginalSource);'),
        ('cg', 'CG59-backup-browser-actions'),
        ('cg', 'CG61-switch-backup-opens-staged-copy'),
    ]),
    'C01': ('A selects and advances; assigned Gen V save opens directly (Oct 25 PASS)', [
        ('cg', 'CG57-gen5-home-a-opens-assigned-save'),
        ('host', 'test_home_input_guard', 'tests/test_home_input_guard.cpp', 'Q::decide(Q::kButtonA) == Q::Decision::Stay'),
    ]),
    'P3-TRAINER': ('Trainer Play Time / Gender staged at native offsets, Review, Export only (Oct 25 PASS)', [
        ('cg', 'CG64-trainer-playtime-gender-staged'),
        ('host', 'test_gen4_staged_pokemon_editor', 'tests/test_gen4_staged_pokemon_editor.cpp', 'const bool allowed = i == base + 0x18 || (i >= base + 0x22 && i < base + 0x26) ||'),
        ('host', 'test_gen3_trainer_edit', 'tests/test_gen3_trainer_edit.cpp', 'const bool allowed = i == block + 0x08 || (i >= block + 0x0E && i < block + 0x12) ||'),
        ('host', 'test_gsc_readonly_bridge', 'tests/test_gsc_readonly_bridge.cpp', 'assert(edited[clock]==0x02&&edited[clock+1]==0x05&&edited[clock+2]==42&&edited[clock+3]==9);'),
    ]),
    '4D-HELP': ('Help never claims a control that does nothing there (Oct 25 PASS)', [
        ('cg', 'CG63-help-names-real-controls'),
        ('host', 'test_classic_help_accuracy', 'tests/test_classic_help_accuracy.cpp', 'assert(lines.size() >= 6 && lines.size() <= 10);'),
    ]),
}


# Round 7 (owner on the round 6 candidate 26186464, Oct 26; docs/hardware/ROUND6_RESULTS_26186464.md):
# 49 PASS of 59. The sheet numbers its rows (A01-D03). The 30 Group A passes are earlier owner IDs that
# keep their existing guards (round 1-6 tables); A22 (R2-SPECIAL) FAILED and keeps its earlier guards,
# which pin the Special calculation, not its placement (being moved, lane C). The 19 B / C / D passes
# are guarded here by their sheet number. Trainer layout, Gen II Special placement, editor typography
# and Help wording are being restyled in round 7, so every guard pins behaviour (staging, native rules,
# input routing, immutability), never visuals. Owner's Oct 26 rule: "A PASS does not close the request".
ROUND7_DOC = "docs/hardware/ROUND6_RESULTS_26186464.md"
ROUND7_PASS_COUNT = 49
GUARDS_ROUND7: dict[str, tuple[str, G]] = {
    'B01': ('L06: Gen III/V Pokerus and Held Item activation (Oct 26 PASS)', [
        ('cg', 'CG69-pokerus-editable-gen3-gen5'),
        ('cg', 'CG70-held-item-tap-commits-any-row'),
        ('host', 'test_held_item_input', 'tests/test_held_item_input.cpp', 'assert(t.tap(b.x + b.w / 2, b.y + b.h / 2).choose && t.committed == a.committed);'),
        ('host', 'test_ui_editor_input_host', 'tests/test_ui_editor_input_host.cpp', 'check(d.target(kGen3DetailTouchBase + kPokerusRow) != nullptr, "Gen III Pokérus row scrolled into view");'),
    ]),
    'B02': ('R2-TRAINER: no Gold / Silver gender overlap, supported Trainer edits kept (Oct 26 PASS)', [
        ('cg', 'CG72-gold-silver-trainer-no-gender'),
        ('cg', 'CG64-trainer-playtime-gender-staged'),
        ('host', 'test_classic_trainer_page', 'tests/test_classic_trainer_page.cpp', 'assert(countField(rows, Field::Gender) == (genderStored ? 1 : 0)); // no Gen I / Gold / Silver gender'),
    ]),
    'B03': ('B08: direct Bag Review (Oct 26 PASS)', [
        ('cg', 'CG73-review-entry-visible-gen1-5'),
        ('cg', 'CG58-export-outcome-visible'),
    ]),
    'C02': ('7B: Gen V directly learnable move reads OK (Oct 26 PASS)', [
        ('host', 'test_ui_editor_input_host', 'tests/test_ui_editor_input_host.cpp', 'check(ok && !learnable, "Gen V Moves panel marks Thunder Shock OK, not Learnable");'),
    ]),
    'C03': ('7C: Gen V Language cycles inline, no toast (Oct 26 PASS)', [
        ('cg', 'CG79-no-gen5-language-toast'),
        ('host', 'test_gen5_language_cycle', 'tests/test_gen5_language_cycle.cpp', 'assert(branch.find("postStatus(")==std::string::npos);'),
    ]),
    'C04': ('7D: tap outside closes non-destructive popups, no tap-through (Oct 26 PASS)', [
        ('cg', 'CG78-tap-outside-dismisses-non-destructive'),
        ('host', 'test_popup_dismiss', 'tests/test_popup_dismiss.cpp', 'assert(s.pickerOpen && s.dismissals == 0);      // contact alone never closes it'),
        ('host', 'test_popup_dismiss', 'tests/test_popup_dismiss.cpp', 'assert(s.underneathActivations == 0);'),
    ]),
    'C05': ('7E: save safety: originals protected, backups through a copy (Oct 26 PASS)', [
        ('cg', 'CG61-switch-backup-opens-staged-copy'),
        ('cg', 'CG60-no-backup-on-unchanged-open'),
        ('host', 'test_emulator_backup_access', 'tests/test_emulator_backup_access.cpp', 'assert(B::deleteRecord(l, 0) == B::Refusal::OriginalSource);'),
    ]),
    'C07': ('7G: Bag Review discoverable (Oct 26 PASS)', [
        ('cg', 'CG73-review-entry-visible-gen1-5'),
    ]),
    'C08': ('Gen III held-item policy: no unreleased held items, Safari as a caught Ball (Oct 26 PASS)', [
        ('cg', 'CG71-gen3-held-item-vs-caught-ball'),
        ('host', 'test_gen3_staged_pokemon_editor', 'tests/test_gen3_staged_pokemon_editor.cpp', 'assert(!releasedHeldItem(5) && !releasedHeldItem(44));'),
    ]),
    'C10': ('Tap-through: a popup consumes its taps (Oct 26 PASS)', [
        ('cg', 'CG78-tap-outside-dismisses-non-destructive'),
        ('cg', 'CG08-modal-footer-and-touch-layer'),
    ]),
    'C11': ('View-only notice: View explains, never stages or prompts (Oct 26 PASS)', [
        ('host', 'test_ui_editor_input_host', 'tests/test_ui_editor_input_host.cpp', 'check(HostUi::promptCount == 0, "Gen III View never opens a prompt");'),
        ('host', 'test_ui_editor_input_host', 'tests/test_ui_editor_input_host.cpp', 'check(!editor || editor->pendingChanges().empty(), "Gen III View stages nothing");'),
    ]),
    'C12': ('Gen V Pokerus Keep stages the toggled mode, keeps the strain (Oct 26 PASS)', [
        ('host', 'test_ui_editor_input_host', 'tests/test_ui_editor_input_host.cpp', 'check(staged() == 0x20, "Gen V Keep stages Cured, keeping the stored strain (got " + std::to_string(staged()) + ")");'),
    ]),
    'C13': ('Create Review lists the created details (Oct 26 PASS)', [
        ('cg', 'CG74-create-review-details'),
    ]),
    'C14': ('Gen II Create starts with native level-up moves (Oct 26 PASS)', [
        ('host', 'test_species_change_moves_gen2', 'tests/test_species_change_moves_gen2.cpp', 'require((fresh.moves == std::array<uint8_t, 4>{84, 45, 0, 0}), "Pikachu Lv 5: Thunder Shock, Growl");'),
    ]),
    'C15': ('Stored unreleased held item is labelled, kept (Oct 26 PASS)', [
        ('cg', 'CG71-gen3-held-item-vs-caught-ball'),
        ('host', 'test_ui_editor_input_host', 'tests/test_ui_editor_input_host.cpp', 'check(staged && staged->heldItem == 5, "Keep leaves the stored Safari Ball held item in place");'),
    ]),
    'C16': ('No draft carried across saves (Oct 26 PASS)', [
        ('host', 'test_ui_editor_input_host', 'tests/test_ui_editor_input_host.cpp', 'check(trainerB->stagedPokemon()->pendingChanges().empty(), "save B: nothing from save A is staged");'),
    ]),
    'C17': ('Gen I-III export refuses a changed original (Oct 26 PASS)', [
        ('host', 'test_emulator_backup_access', 'tests/test_emulator_backup_access.cpp', 'assert(!Guard::unchangedSinceOpen(snapshot, error) && error.find("changed since it was opened") != std::string::npos);'),
    ]),
    'D01': ('AUD-01: one shared Gen I-V Review (Oct 26 PASS: do not redesign)', [
        ('cg', 'CG35-gen5-review-matches-gen4'),
        ('cg', 'CG65-gen1-review-shared-presenter-only'),
        ('cg', 'CG67-standalone-review-returns-underneath'),
        ('host', 'test_gen12_review_parity', 'tests/test_gen12_review_parity.cpp', 'assert(Discard::step(s, false, false, false) == Outcome::NotHandled);'),
    ]),
    'D02': ('P1-GEN1: Gen I Review presentation (Oct 26 PASS)', [
        ('cg', 'CG65-gen1-review-shared-presenter-only'),
        ('cg', 'CG68-gen1-review-swipe-one-page'),
    ]),
}


def all_guards():
    """Every (pid, title, guards) entry; an item passed again in a later round keeps every round's guards."""
    for table in (GUARDS, GUARDS_ROUND2, GUARDS_ROUND3, GUARDS_ROUND5, GUARDS_ROUND6, GUARDS_ROUND7):
        for pid, (title, guards) in table.items():
            yield pid, title, guards

PR_GATING_KINDS = {"host", "qa", "cg", "py"}  # "pin" alone is not a behavioural guard (except P09)
PIN_ONLY_ALLOWED = {"C09": "no host seam: libnx pad input does not run on the host"}


def make_dry_run(root: Path, target: str) -> str:
    out = subprocess.run(["make", "-f", "Makefile.host", "-n", "-B", target], cwd=root,
                         capture_output=True, text=True)
    if out.returncode != 0:
        raise SystemExit(f"`make -f Makefile.host -n {target}` failed (is vendor/PKSM-Core checked out? "
                         f"git submodule update --init --recursive vendor/PKSM-Core):\n{out.stderr[-2000:]}")
    return out.stdout


def load_cross_gen(root: Path):
    spec = importlib.util.spec_from_file_location("cross_gen", DEFAULT_ROOT / "tests/test_qa_cross_gen_contract.py")
    mod = importlib.util.module_from_spec(spec)
    sys.modules["cross_gen"] = mod
    spec.loader.exec_module(mod)
    sources, _ = mod.load_sources(root)
    return {r.cid: r for r in mod.contracts(sources)}


def check(root: Path, run_python: bool = True) -> list[str]:
    errors: list[str] = []
    dry = {"host": make_dry_run(root, "host-test"), "qa": make_dry_run(root, "qa")}
    cg = load_cross_gen(root)
    ran_py: dict[str, int] = {}
    text_cache: dict[str, str | None] = {}

    def text(rel: str) -> str | None:
        if rel not in text_cache:
            p = root / rel
            text_cache[rel] = p.read_text(encoding="utf-8", errors="replace") if p.is_file() else None
        return text_cache[rel]

    if len(OWNER_PASS) != 42 or list(GUARDS) != OWNER_PASS or set(OWNER_PASS) & set(OWNER_NOT_PASS):
        errors.append(f"mapping must cover exactly the owner's 42 PASS IDs, has {len(GUARDS)}")
    # The mapping must agree with the owner's sheet as recorded (PASS rows end in "| PASS |").
    sheet = text(RESULTS_DOC)
    if sheet is None:
        errors.append(f"{RESULTS_DOC} is missing")
    else:
        rows = re.findall(r"^\| ([CMBELNS]\d\d) \|.*\| ([^|]+?) \|\s*$", sheet, re.M)
        sheet_pass = [i for i, res in rows if res.strip() == "PASS"]
        if len(rows) != 54 or sheet_pass != OWNER_PASS:
            errors.append(f"{RESULTS_DOC}: owner sheet has {len(rows)} rows / PASS {sheet_pass}, "
                          f"gate maps {OWNER_PASS}")
    # Round 2: the owner's six retested passes, exactly as recorded, never overlapping round 1 or the
    # items still unverified.
    r2 = text(ROUND2_DOC)
    if r2 is None:
        errors.append(f"{ROUND2_DOC} is missing")
    else:
        rows2 = re.findall(r"^\| ([CMBELNS]\d\d) \|.*\| ([^|]+?) \|\s*$", r2, re.M)
        r2_pass = [i for i, res in rows2 if res.strip() == "PASS"]
        r2_nv = [i for i, res in rows2 if res.strip() == "NOT VERIFIED"]
        if r2_pass != ROUND2_PASS or r2_nv != ROUND2_NOT_VERIFIED:
            errors.append(f"{ROUND2_DOC}: PASS {r2_pass} / NOT VERIFIED {r2_nv}, gate maps {ROUND2_PASS}")
    if set(ROUND2_PASS) & (set(OWNER_PASS) | set(ROUND2_NOT_VERIFIED)) or \
            not set(ROUND2_PASS) <= set(OWNER_NOT_PASS):
        errors.append("round 2 accepted IDs must be round 1 non-PASS items, disjoint from the unverified ones")
    # Round 3: every owner PASS on the round 3 sheet is guarded (round 1 / round 2 mapping or GUARDS_ROUND3),
    # and GUARDS_ROUND3 holds only round 3 PASS items not already guarded.
    r3 = text(ROUND3_DOC)
    if r3 is None:
        errors.append(f"{ROUND3_DOC} is missing")
    else:
        rows3 = re.findall(r"^- \*\*([A-Z0-9-]+) — (PASS|FAIL|NOT TESTED) — ", r3, re.M)
        r3_pass = [i for i, res in rows3 if res == "PASS"]
        guarded = set(GUARDS) | set(GUARDS_ROUND2) | set(GUARDS_ROUND3)
        if len(rows3) != 110 or len(r3_pass) != ROUND3_PASS_COUNT:
            errors.append(f"{ROUND3_DOC}: {len(rows3)} rows / {len(r3_pass)} PASS, gate expects 110 / {ROUND3_PASS_COUNT}")
        unguarded = [i for i in r3_pass if i not in guarded]
        if unguarded:
            errors.append(f"{ROUND3_DOC}: owner PASS item(s) with no guard: {unguarded}")
        stray = [i for i in GUARDS_ROUND3 if i not in r3_pass or i in GUARDS or i in GUARDS_ROUND2]
        if stray:
            errors.append(f"GUARDS_ROUND3 holds item(s) that are not new round 3 PASS: {stray}")
    # Round 5: GUARDS_ROUND5 is exactly the owner's Oct 24 PASS list, in the recorded order.
    r5 = text(ROUND5_DOC)
    if r5 is None:
        errors.append(f"{ROUND5_DOC} is missing")
    else:
        m5 = re.search(r"^- PASS: (.*)$", r5, re.M)
        r5_pass = [i.strip().rstrip(".") for i in m5.group(1).split(",")] if m5 else []
        if len(r5_pass) != ROUND5_PASS_COUNT or list(GUARDS_ROUND5) != r5_pass:
            errors.append(f"{ROUND5_DOC}: owner Oct 24 PASS {r5_pass}, gate maps {list(GUARDS_ROUND5)}")
        if r3 is not None:
            r3_state = dict(re.findall(r"^- \*\*([A-Z0-9-]+) — (PASS|FAIL|NOT TESTED) — ", r3, re.M))
            not_r3_fail = [i for i in r5_pass if r3_state.get(i) != "FAIL"]
            if not_r3_fail:
                errors.append(f"{ROUND5_DOC}: Oct 24 PASS item(s) that were not round 3 FAILs: {not_r3_fail}")
    # Round 6: every Oct 25 PASS is guarded; GUARDS_ROUND6 is exactly those not already Oct 24 PASS, in
    # the recorded order, and every one was an Oct 24 FAIL or unmarked item.
    r6 = text(ROUND6_DOC)
    if r6 is None:
        errors.append(f"{ROUND6_DOC} is missing")
    else:
        m6 = re.search(r"^### PASS \((\d+)\)\s*\n\s*\n(.+)$", r6, re.M)
        r6_pass = [i.strip().rstrip(".") for i in m6.group(2).split(",")] if m6 else []
        if len(r6_pass) != ROUND6_PASS_COUNT or (m6 and int(m6.group(1)) != ROUND6_PASS_COUNT):
            errors.append(f"{ROUND6_DOC}: {len(r6_pass)} Oct 25 PASS items, gate expects {ROUND6_PASS_COUNT}")
        new6 = [i for i in r6_pass if i not in GUARDS_ROUND5]
        if list(GUARDS_ROUND6) != new6:
            errors.append(f"{ROUND6_DOC}: new Oct 25 PASS {new6}, gate maps {list(GUARDS_ROUND6)}")
        if r5 is not None:
            # P3-TRAINER and 4D-HELP are round 5 owner requests, not Oct 24 sheet rows.
            prior = [i.strip().rstrip(".") for line in re.findall(r"^- (?:FAIL|UNMARKED / TRACKED): (.*)$", r5, re.M)
                     for i in line.split(",")]
            not_open = [i for i in GUARDS_ROUND6 if i not in prior and i not in ("P3-TRAINER", "4D-HELP")]
            if not_open:
                errors.append(f"GUARDS_ROUND6 item(s) that were neither Oct 24 FAIL / unmarked nor round 5 requests: {not_open}")
    # Round 7: every Oct 26 PASS row is guarded. Group A rows by their earlier owner ID (round 1-6 tables);
    # B / C / D rows by their sheet number in GUARDS_ROUND7, which holds exactly those, in order.
    r7 = text(ROUND7_DOC)
    if r7 is None:
        errors.append(f"{ROUND7_DOC} is missing")
    else:
        rows7 = re.findall(r"^\| (?:\*\*)?([ABCD]\d\d)(?:\*\*)? \| (?:\*\*)?([^|*]+?)(?:\*\*)? \| "
                           r"(?:\*\*)?(PASS|FAIL|NOT TESTED)(?:\*\*)? \|", r7, re.M)
        pass7 = [(n, i) for n, i, res in rows7 if res == "PASS"]
        if len(rows7) != 59 or len(pass7) != ROUND7_PASS_COUNT:
            errors.append(f"{ROUND7_DOC}: {len(rows7)} rows / {len(pass7)} PASS, gate expects 59 / {ROUND7_PASS_COUNT}")
        earlier = set(GUARDS) | set(GUARDS_ROUND2) | set(GUARDS_ROUND3) | set(GUARDS_ROUND5) | set(GUARDS_ROUND6)
        unguarded = [f"{n} {i}" for n, i in pass7 if n.startswith("A") and i not in earlier]
        if unguarded:
            errors.append(f"{ROUND7_DOC}: Oct 26 Group A PASS item(s) with no earlier guard: {unguarded}")
        want7 = [n for n, _ in pass7 if not n.startswith("A")]
        if list(GUARDS_ROUND7) != want7:
            errors.append(f"{ROUND7_DOC}: Oct 26 B / C / D PASS rows {want7}, gate maps {list(GUARDS_ROUND7)}")
    for pid, title, guards in all_guards():
        kinds = {g[0] for g in guards}
        if not kinds & PR_GATING_KINDS and pid not in PIN_ONLY_ALLOWED:
            errors.append(f"{pid} {title}: no guard that runs on every PR")
        for g in guards:
            kind = g[0]
            if kind in ("host", "qa"):
                _, test, rel, lit = g
                body = text(rel)
                if body is None:
                    errors.append(f"{pid}: guard file {rel} is gone")
                elif lit not in body:
                    errors.append(f"{pid}: {rel} no longer contains {lit!r}")
                target = "host-test" if kind == "host" else "qa"
                if test not in dry[kind]:
                    errors.append(f"{pid}: {test} is not built/run by `make -f Makefile.host {target}`")
            elif kind == "cg":
                r = cg.get(g[1])
                if r is None:
                    errors.append(f"{pid}: cross-gen contract {g[1]} no longer exists")
                elif r.violations:
                    errors.append(f"{pid}: {g[1]} fails: {r.violations[0]}")
            elif kind == "py":
                _, rel, lit = g
                body = text(rel)
                if body is None:
                    errors.append(f"{pid}: guard file {rel} is gone")
                    continue
                if lit not in body:
                    errors.append(f"{pid}: {rel} no longer contains {lit!r}")
                if run_python and rel not in ran_py:
                    ran_py[rel] = subprocess.run([sys.executable, rel], cwd=root, capture_output=True).returncode
                if ran_py.get(rel, 0) != 0:
                    errors.append(f"{pid}: {rel} fails")
            elif kind == "pin":
                _, rel, lit, why = g
                body = text(rel)
                if body is None or lit not in body:
                    errors.append(f"{pid}: source pin changed ({why}): {rel} no longer contains {lit!r}")
            else:
                errors.append(f"{pid}: unknown guard kind {kind}")
    return errors


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--root", type=Path, default=DEFAULT_ROOT)
    ap.add_argument("--list", action="store_true", help="print every item with its guards")
    args = ap.parse_args(argv[1:])
    errors = check(args.root.resolve())
    if args.list:
        for pid, title, guards in all_guards():
            print(f"{pid} {title}")
            for g in guards:
                print(f"      {g[0]:4} {' | '.join(str(x) for x in g[1:3])}")
    n = sum(len(g) for _, _, g in all_guards())
    label = (f"{len(GUARDS)} round 1 + {len(GUARDS_ROUND2)} round 2 + {len(GUARDS_ROUND3)} round 3 + "
             f"{len(GUARDS_ROUND5)} Oct 24 + {len(GUARDS_ROUND6)} Oct 25 + {len(GUARDS_ROUND7)} Oct 26 owner PASS items; all "
             f"{ROUND3_PASS_COUNT} round 3, {ROUND5_PASS_COUNT} Oct 24, {ROUND6_PASS_COUNT} Oct 25 and {ROUND7_PASS_COUNT} Oct 26 "
             f"PASS guarded")
    if errors:
        print(f"Hardware-accepted regression gate: FAIL ({len(errors)} problem(s), {label}, {n} guards)")
        for e in errors:
            print(f"  {e}")
        return 1
    print(f"Hardware-accepted regression gate: PASS ({label}, {n} guards, every item guarded on every PR)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
