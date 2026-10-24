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
        ('pin', 'src/UI/SharedPokemonEditorSurface.inc', 'inline constexpr int kMetLocationPickerWidth = 704;', 'Gen III-V met-location picker width: owner Round 5 NEW-10 "too wide" (was 900, 7fea1797); every row fits unclipped (test_met_location_fit)'),
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
    for pid, (title, guards) in {**GUARDS, **GUARDS_ROUND2, **GUARDS_ROUND3}.items():
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
        for pid, (title, guards) in {**GUARDS, **GUARDS_ROUND2, **GUARDS_ROUND3}.items():
            print(f"{pid} {title}")
            for g in guards:
                print(f"      {g[0]:4} {' | '.join(str(x) for x in g[1:3])}")
    every = {**GUARDS, **GUARDS_ROUND2, **GUARDS_ROUND3}
    n = sum(len(g) for _, g in every.values())
    label = (f"{len(GUARDS)} round 1 + {len(GUARDS_ROUND2)} round 2 + {len(GUARDS_ROUND3)} round 3 owner PASS items; "
             f"all {ROUND3_PASS_COUNT} round 3 PASS guarded")
    if errors:
        print(f"Hardware-accepted regression gate: FAIL ({len(errors)} problem(s), {label}, {n} guards)")
        for e in errors:
            print(f"  {e}")
        return 1
    print(f"Hardware-accepted regression gate: PASS ({label}, {n} guards, every item guarded on every PR)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
