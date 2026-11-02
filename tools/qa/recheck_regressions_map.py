"""Round 13 recheck map: the 94 Hardware Accepted issues whose code changed after their accepted
candidate (docs/qa/reconciliation trace data: status "Hardware Accepted", rechecked_needed true).

For each issue: the candidate the owner accepted it on, the files that IMPLEMENT the accepted
behaviour (read from the code paths, not the title), the automated test(s) that exercise those files
on the current code and assert the accepted behaviour, and whether a later owner request superseded
part or all of it. tools/qa/gen_recheck_regressions.py adds the git history (accepted..BASE) and writes
docs/qa/RECHECK_REGRESSIONS.md. Never flips an issue to accepted: every row stays marked for
re-verification on hardware.

status: "recheck"   -> Hardware Accepted (recheck required)
        "partial"   -> Hardware Accepted (recheck required); part superseded by owner request
        "superseded"-> superseded by owner request (accepted behaviour deliberately changed; not pinned)
        "scope"     -> Hardware Accepted (recheck required); outside the Gen I-V scope, indirect coverage only
tests:  existing tests  [(path, assertion)]
new:    tests added by the Round 13 recheck lane for this issue [(path, assertion)]
extra:  the existing tests already exercised the changed path; `new` adds a further check
"""

BASE = "6010bdd3"

NEW_MET = "tests/test_recheck_met_language.cpp"
NEW_UI = "tests/test_ui_recheck_regressions_host.cpp"
NEW_G5 = "tests/test_gen5_party_box_parity.cpp"
NEW_LIFE = "tests/test_recheck_lifecycle_contract.py"
GATE = "tests/test_hw_round1_regression_gate.py"
CG = "tests/test_qa_cross_gen_contract.py"
CARRY_UI = "tests/test_ui_clone_carry_host.cpp"

MAP = {
    # ---------------------------------------------------------------- other / process
    "PBX-OTH-001": dict(acc="3be4de6b", files=[
        "include/Safety/WritePolicy.h", "src/Integration/Common/VerifiedStagedExport.cpp",
        "include/UI/ClassicSourceGuard.h", "src/Integration/Gen4/Gen4ExportTransaction.cpp"], tests=[
        ("tests/test_write_policy.cpp", "LiveGame destination refused while LIVE_SAVE_WRITES_ENABLED is false"),
        ("tests/test_qa_export_contracts.cpp", "C1: source bytes unchanged for every export path incl. Gen IV (qa-contracts)"),
        ("tests/test_ci_repository_safety_contract.py", "repository safety contract (live writes stay locked)")]),
    "PBX-OTH-014": dict(acc="3634d1e6", files=["src", "include"], tests=[
        (GATE, "per-ID guards for every owner PASS R1..Oct 26 (300+ guards, host + CG + literal)")],
        note="Smoke item: every changed file counts. The per-issue rows below are the targeted part."),
    # ---------------------------------------------------------------- help / footers / controls
    "PBX-CTL-006": dict(acc="69668bc8", files=[
        "src/UI/Gen1PokemonEditorOverlay.cpp", "src/UI/Gen1PokemonEditorOverlayUX.inc",
        "src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc", "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc",
        "include/UI/Gen1PokemonEditorUIContract.h"], tests=[
        ("tests/test_gen1_view_navigation.cpp", "Gen I View/Edit: L/R change section, A edits, B backs out"),
        ("tests/test_gen1_pokemon_editor_ui_contract.cpp", "five logical sections and section starts (L/R targets)")]),
    "PBX-CTL-007": dict(acc="cfb70357", status="partial", files=[
        "include/UI/ClassicBrowseFooter.h", "src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc",
        "src/UI/Gen2SharedSurfaceParity.inc", "src/UI/ClassicPackedMoveOverlay.inc",
        "include/UI/SharedPokemonEditorContract.h"], tests=[
        ("tests/test_classic_browse_footer.cpp", "one Gen I-V browse footer order (Gen I/II parity)"),
        ("tests/test_footer_help_handlers.py", "every footer token (A / X) has a handler on that page")],
        superseded="R11 X-IMPEXP (owner Extra 4, dd69bc93): X = Export / Import on the box grid, never Add; "
                   "the accepted 'X Stage Add' wording is not pinned. Gen I/II A/X parity still is."),
    "PBX-CTL-010": dict(acc="3634d1e6", files=[
        "include/UI/InventoryHelp.h", "include/UI/ClassicHelpPresentation.h", "src/UI/TrainerViewScreenBase.inc",
        "src/UI/ClassicInventoryOverlay.cpp", "include/UI/GlyphInstruction.h"], tests=[
        ("tests/test_classic_help_accuracy.cpp", "each Help claim (incl. Inventory Controls) tied to the code that handles it"),
        ("tests/test_ui_layout_host.cpp", "real draw: one footer bar, Help window text fits, Gen I-V pages"),
        ("tests/test_footer_help_handlers.py", "footer/Help key tokens have handlers"),
        ("tests/test_glyph_instruction.cpp", "coloured controller badges")]),
    "PBX-CTL-014": dict(acc="26186464", files=["include/UI/ClassicHelpPresentation.h", "include/UI/ReviewEntry.h"], tests=[
        ("tests/test_classic_help_accuracy.cpp", "'+ Review' named only where + opens Review (Gen V Boxes too since R12-001)"),
        ("tests/test_review_entry.cpp", "exactly one Review key per screen, Gen I-V")]),
    # ---------------------------------------------------------------- launch / quit / lifecycle
    "PBX-LQT-001": dict(acc="3634d1e6", files=["src/main.cpp", "src/UI/UI.cpp", "src/UI/SaveSelectScreen.cpp"], tests=[
        ("tests/test_home_input_guard.cpp", "only Y quits from Product Home")], new=[
        (NEW_LIFE, "an exit request returns from the Product Home loop (no rebuild, no extra frame); every frame loop gated")],
        note="Boot / relaunch itself is device-only."),
    "PBX-LQT-002": dict(acc="3be4de6b", files=["src/UI/UI.cpp", "src/main.cpp"], new=[
        (NEW_LIFE, "every frame loop is gated on appletMainLoop() (HOME / sleep / exit), one standard pad, no applet hooks")],
        note="HOME / sleep / Joy-Con reconnect remain device-only; the contract pins the structure they rely on."),
    "PBX-LQT-003": dict(acc="0760334f", files=["src/UI/SaveSelectScreen.cpp", "include/UI/QuickGamesDrawerMotion.h"], tests=[
        ("tests/test_game_hub_contract.py", "Quick Games A opens through refreshHubSelectionFromCache / refreshHubPreview(false): no I/O"),
        ("tests/test_ui_quick_games_host.cpp", "real Product Home: Y opens Quick Games, taps inside/outside behave")]),
    "PBX-LQT-006": dict(acc="0760334f", files=["src/UI/GameLauncher.cpp", "include/UI/GameLaunchModel.h",
                                               "include/UI/GameLauncher.h"], tests=[
        ("tests/test_retroarch_launch_resolution.cpp", "cartridge header identity, Red vs FireRed guards"),
        ("tests/test_game_launch_model.cpp", "argv tokenisation / direct handoff plan")]),
    "PBX-LQT-007": dict(acc="62a2f8c3", files=["include/UI/HomeQuitConfirm.h", "src/UI/SaveSelectScreen.cpp",
                                               "include/UI/LeaveDecisionLayout.h"], tests=[
        ("tests/test_home_input_guard.cpp", "only Y quits"),
        ("tests/test_leave_decision_layout.cpp", "two-button B Cancel / Y Quit dialog fits"),
        ("tests/test_game_hub_contract.py", "single exit site behind the decision (CG07)")]),
    "PBX-LQT-008": dict(acc="62a2f8c3", files=["src/UI/SaveSelectScreen.cpp", "include/UI/LeaveDecisionLayout.h"], tests=[
        ("tests/test_leave_decision_layout.cpp", "launch / quit notices and long game names fit"),
        ("tests/test_game_hub_contract.py", "CG07 single launch site behind Y")]),
    # ---------------------------------------------------------------- touch
    "PBX-TCH-001": dict(acc="3634d1e6", files=["src/UI/TouchInput.cpp", "include/UI/TouchScroll.h",
                                               "include/UI/SharedPopupDismiss.h", "src/UI/SaveSelectScreen.cpp",
                                               "src/UI/TrainerViewScreenBase.inc"], tests=[
        ("tests/test_touch_modal_list_contract.py", "contact and release on the same hit rect; modal lists"),
        ("tests/test_touch_appwide_surface_contract.py", "app-wide touch surfaces"),
        ("tests/test_touch_scroll_state.cpp", "horizontal swipe never owns a vertical list"),
        ("tests/test_gen5_box_touch.cpp", "Gen V box: first tap focuses, second opens Actions")]),
    "PBX-TCH-002": dict(acc="62a2f8c3", files=["src/UI/SaveSelectScreen.cpp", "include/UI/QuickGamesDrawerMotion.h",
                                               "include/UI/TouchScroll.h"], tests=[
        ("tests/test_touch_scroll_state.cpp", "swipe direction ownership / momentum"),
        ("tests/test_quick_games_drawer_motion.cpp", "drawer motion settles"),
        ("tests/test_ui_quick_games_host.cpp", "tap outside closes, tap inside keeps the drawer")]),
    "PBX-TCH-003": dict(acc="26186464", files=["src/UI/ClassicPartyActions.inc", "include/UI/ClassicPartyActions.h",
                                               "src/UI/TrainerViewScreenBase.inc"], tests=[
        ("tests/test_party_summary_tap_routing.cpp", "Party / summary-panel tap opens the native Actions popup"),
        ("tests/test_classic_party_actions.cpp", "Gen I-III Party popup rows and dispatch")]),
    # ---------------------------------------------------------------- install / home / settings / search
    "PBX-HOM-001": dict(acc="3634d1e6", files=["include/UI/CardScenery.h", "include/UI/RoundedArtwork.h",
                                               "src/UI/SaveSelectScreen.cpp"], tests=[
        ("tests/test_ui_home_card_scenery_host.cpp", "scenery painted as rounded shapes concentric with the card"),
        ("tests/test_home_card_scenery_contract.py", "HOME card scenery contract")]),
    "PBX-SET-001": dict(acc="3be4de6b", files=["include/UI/Common.h", "include/Utils/Settings.h", "src/Utils/Settings.cpp"], tests=[
        ("tests/test_theme.cpp", "every theme palette readable; theme persists through settings parse")]),
    "PBX-SRC-001": dict(acc="62a2f8c3", files=["include/UI/SharedSpeciesPicker.h", "include/UI/SharedSearchInput.h"], tests=[
        ("tests/test_species_search_gen15.cpp", "prefix filter narrows per character, Backspace widens, Gen I-V")]),
    "PBX-SRC-002": dict(acc="26186464", files=["include/UI/SharedSpeciesPicker.h"], tests=[
        ("tests/test_shared_species_picker_surface_contract.cpp", "one picker surface: sprite / list / keyboard regions"),
        (CG, "CG39 species keyboard docked beside the list")]),
    # ---------------------------------------------------------------- visuals / editor / stats / view / language
    "PBX-VIS-007": dict(acc="69668bc8", files=["include/UI/SharedSpeciesPicker.h",
                                               "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc"], tests=[
        ("tests/test_shared_species_picker_surface_contract.cpp", "picker draws one sprite in the current shiny state")]),
    "PBX-VIS-012": dict(acc="62a2f8c3", files=["include/UI/SharedSpeciesPicker.h", "src/UI/Gen5SharedReadOnlySurface.inc"], tests=[
        ("tests/test_shared_species_picker_surface_contract.cpp", "single sprite / shiny caption, Gen V range")]),
    "PBX-EDT-001": dict(acc="3634d1e6", files=["src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc",
                                               "src/UI/Gen2PokemonEditorFoundation.inc", "src/UI/Gen3SharedPokemonSurface.inc",
                                               "src/UI/Gen4SharedPokemonSurface.inc", "src/UI/Gen5SharedReadOnlySurface.inc"], tests=[
        ("tests/test_ui_gen2_create_review_host.cpp", "real screen: staged edits; Export keeps the source bytes unchanged"),
        ("tests/test_qa_export_contracts.cpp", "C1 source unchanged / C6 Discard All restores"),
        (CG, "staged-only edit contracts")]),
    "PBX-EDT-002": dict(acc="3634d1e6", files=["src/UI/Gen1PokemonEditorOverlayFoundation.inc",
                                               "src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc",
                                               "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc", "include/UI/EditFeedback.h"], tests=[
        ("tests/test_edit_feedback.cpp", "routine edits post no toast (EditFeedback::Kind incl. R12 Carry)"),
        ("tests/test_gen12_dv_reroll.cpp", "Gen I Y re-roll on the workspace")]),
    "PBX-EDT-003": dict(acc="26186464", files=["include/UI/ClassicPartyActions.h", "src/UI/ClassicPartyActions.inc"], tests=[
        ("tests/test_classic_party_actions.cpp", "Gen I-III Party View / Edit routed to the native staged editors"),
        ("tests/test_classic_party_review_path.cpp", "Party edit reaches Review")]),
    "PBX-EDT-006": dict(acc="3634d1e6", files=["src/UI/Gen5SharedReadOnlySurface.inc",
                                               "include/Integration/Gen5/Gen5SharedScreenState.h"], tests=[
        ("tests/test_gen5_create_edit_parity.cpp", "Gen V rows / labels fit; no duplicated PID-linked rows"),
        ("tests/test_ui_editor_input_host.cpp", "real screen: Gen V Pokerus / Language / Moves rows act inline"),
        (CG, "Gen V shared editor contract")]),
    "PBX-EDT-010": dict(acc="62a2f8c3", files=["src/UI/Gen2PokemonEditorFoundation.inc", "include/UI/EditFeedback.h"], tests=[
        ("tests/test_edit_feedback.cpp", "Shiny applied posts no toast"),
        ("tests/test_ui_editor_input_host.cpp", "real screen: Pokerus toggles on A and on a tap, Gen II-V")]),
    "PBX-STA-001": dict(acc="3634d1e6", files=["include/Pokemon/Pokemon1ReadOnly.h", "src/UI/Panels/BoxPokemonPanel.cpp",
                                               "include/UI/BoxSummaryRadar.h"], tests=[
        ("tests/test_gen1_radar_render.cpp", "Gen I radar layout / scale"),
        ("tests/test_box_summary_radar.cpp", "per-generation radar route incl. Gen I special")]),
    "PBX-STA-003": dict(acc="62a2f8c3", files=["src/UI/Panels/BoxPokemonPanel.cpp", "include/UI/BoxSummaryRadar.h"], tests=[
        ("tests/test_box_summary_radar.cpp", "Gen V boxed quick view uses calculated stats, never 0")]),
    "PBX-STA-005": dict(acc="62a2f8c3", files=["include/UI/EditFeedback.h", "include/UI/SharedStatReroll.h",
                                               "include/UI/Gen12DVReroll.h"], tests=[
        ("tests/test_edit_feedback.cpp", "re-roll posts no toast"),
        (CG, "CG06 no confirmation on re-roll")]),
    "PBX-VEW-001": dict(acc="3634d1e6", files=["src/UI/TrainerViewScreenBase.inc", "src/UI/Panels/BoxPokemonPanel.cpp",
                                               "src/UI/Panels/PartyPokemonPanel.cpp"], tests=[
        ("tests/test_ui_layout_host.cpp", "real draw of Party and Boxes for a Gen I, II, III, IV and V save"),
        ("tests/test_party_summary_tap_routing.cpp", "Party / Boxes selection routing")]),
    "PBX-LNG-002": dict(acc="26186464", files=["src/UI/Gen5SharedReadOnlySurface.inc", "include/UI/Gen5LanguageCycle.h",
                                               "include/Integration/Gen5/Gen5SharedScreenState.h"], tests=[
        ("tests/test_gen5_language_cycle.cpp", "1,2,3,4,5,7,8 wrap; no postStatus in the branch"),
        ("tests/test_ui_editor_input_host.cpp", "real screen: A / tap cycles Gen V Language, no toast, no prompt"),
        ("tests/test_gen5_party_box_parity.cpp", "R12-009: each Language step keeps PID / IVs / EVs")]),
    "PBX-LNG-009": dict(acc="26186464", files=["include/UI/SharedPokemonEditorContract.h", "include/UI/SharedEditorPresentation.h",
                                               "src/UI/Gen3SharedPokemonSurface.inc", "src/UI/Gen5SharedReadOnlySurface.inc",
                                               "src/UI/Gen2PokemonEditorFoundation.inc"], tests=[
        ("tests/test_gen3_staged_pokemon_editor.cpp", "R12-008: Gen III Language edit touches only language / names")], new=[
        (NEW_MET, "Language hidden for Gen I / II, shown (stored byte) for Gen III-V"),
        (NEW_UI, "real screen: no Language row in any Gen II Create panel")]),
    # ---------------------------------------------------------------- create / species / moves / encounters
    "PBX-CRT-001": dict(extra=True, acc="3634d1e6", files=["src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc",
                                               "src/UI/Gen2PokemonEditorFoundation.inc", "include/UI/Gen4SharedPokemonSession.h",
                                               "include/UI/CreateRequiredFields.h"], tests=[
        ("tests/test_species_change_history_policy.cpp", "CreateRequiredFields refusal leaves draft / staged copy unchanged"),
        ("tests/test_ui_gen2_create_review_host.cpp", "Gen II Create stages into the working copy, source bytes unchanged")], new=[
        (NEW_UI, "Gen II Create -> Keep / Add: staged change, source save bytes unchanged")]),
    "PBX-CRT-002": dict(acc="3634d1e6", files=["src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc",
                                               "src/UI/Gen2PokemonEditorFoundation.inc",
                                               "include/Integration/Gen5/Gen5SharedScreenState.h"], tests=[
        ("tests/test_ui_gen2_create_review_host.cpp", "Create opens the one workspace, draft persists through Review"),
        ("tests/test_gen5_party_box_parity.cpp", "Gen V Create from an empty slot through the shared surface")]),
    "PBX-SPC-001": dict(acc="cfb70357", files=["include/UI/SharedSpeciesPicker.h", "src/UI/Gen2PokemonPickerOverlay.inc",
                                               "src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc",
                                               "include/UI/Gen1PokemonEditorUIContract.h"], tests=[
        ("tests/test_gen1_pokemon_editor_ui_contract.cpp", "Gen I rows read '001 - Bulbasaur'")], new=[
        (NEW_UI, "real screen: Gen II picker rows read 'NNN - Name' ('025 - Pikachu'); B leaves the draft")]),
    "PBX-MOV-001": dict(acc="3634d1e6", files=["include/UI/SpeciesChangeMoves.h",
                                               "include/Integration/Gen5/Gen5MoveCompatibility.h",
                                               "src/UI/Gen5SharedReadOnlySurface.inc"], tests=[
        ("tests/test_gen5_move_compatibility.cpp", "BW / B2W2 species pools"),
        ("tests/test_species_change_moves_gen15.cpp", "learnable predicates per generation (keepLearnable)")]),
    "PBX-CRT-003": dict(extra=True, acc="cfb70357", files=["src/UI/Gen2PokemonEditorFoundation.inc",
                                               "src/UI/TrainerViewScreenGSCOverlay.inc"], tests=[
        ("tests/test_ui_gen2_create_review_host.cpp", "Gen II Create on a RetroArch save stages and is listed in Review")], new=[
        (NEW_UI, "Create is not refused as 'RetroArch source is read-only'; the Create is a staged change")]),
    "PBX-CRT-004": dict(acc="cfb70357", files=["src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc",
                                               "src/UI/Gen1PokemonEditorOverlay.cpp",
                                               "src/Integration/Gen1/Gen1StagedPokemonEditor.cpp"], tests=[
        ("tests/test_rby_pokemon_editor.cpp", "Gen I Create defaults and packed append"),
        ("tests/test_gen12_packed_move.cpp", "Gen I stageAdd lands at the packed append slot")]),
    "PBX-CRT-005": dict(acc="cfb70357", files=["src/UI/Gen2PokemonEditorFoundation.inc"], tests=[
        ("tests/test_gsc_editor_surface_contract.cpp", "Gen II Add calls closeToBoxAfterAdd (literal)")], new=[
        (NEW_UI, "real screen: after Add the box grid shows the new slot selected; no Actions sheet")]),
    "PBX-CRT-006": dict(acc="3634d1e6", files=["src/UI/Gen4SharedPokemonSurface.inc", "include/UI/Gen4SharedPokemonSession.h",
                                               "src/Integration/Gen4/Gen4StagedPokemonEditor.cpp"], tests=[
        ("tests/test_species_change_moves_gen4.cpp", "Gen IV Create draft (createBoxDraft) species change / moves"),
        (CARRY_UI, "real screen: Gen IV createBoxDraft + stageCreateBoxPokemon used as fixtures")],
        note="The trace cited tests/test_gen4_create_species_change_history.cpp, which does not exist."),
    "PBX-MOV-002": dict(acc="62a2f8c3", files=["include/UI/SharedMovesPanel.h", "include/UI/MovePickerPresentation.h",
                                               "src/UI/Gen4SharedPokemonSurface.inc", "src/UI/Gen5SharedReadOnlySurface.inc"], tests=[
        ("tests/test_shared_editor_unit.cpp", "Moves panel + chooser layout bounds, generation-correct move facts"),
        (GATE, "M01-M07 per-ID guards")]),
    "PBX-MET-001": dict(acc="26186464", status="superseded", files=[
        "include/Integration/Encounter/EncounterContext45.h", "include/Integration/Encounter/MetLocationPolicy.h"], tests=[
        ("tests/test_gen45_encounter_context.cpp", "R12-004 default list: verified rows + markers, then the unverified list")],
        superseded="R12-004 / R12-005 (owner Nov 1 message E0117, Round 12 H08 / H14): the accepted 'preserve "
                   "encounter filtering' list became verified rows + markers + 'Other location (unverified)...'. "
                   "The old list is not pinned."),
    "PBX-MET-002": dict(acc="62a2f8c3", files=["src/UI/Gen2UnifiedPokemonWorkspace.inc", "include/UI/Gen2FieldSelector.h",
                                               "include/UI/Gen2NativePresentation.h"], tests=[
        ("tests/test_crystal_met_time_inline.cpp", "A cycles Crystal Met Time in place, skips disallowed times")]),
    "PBX-LEG-001": dict(acc="26186464", files=["include/UI/SharedEditorPresentation.h", "src/UI/Modals/PokemonDetailsModal.cpp",
                                               "src/UI/Gen5SharedReadOnlySurface.inc", "include/UI/HistoryConsistencyFacts.h"], tests=[
        (CG, "CG03 Origin & Legality heading; CG48 no green clean verdict in Gen I-V"),
        ("tests/test_shared_editor_unit.cpp", "shared editor rows: Origin / legality rows read-only"),
        (GATE, "E03 GUARDS_ROUND5")]),
    "PBX-SPC-003": dict(acc="3634d1e6", files=["include/UI/Gen3SharedPokemonSession.h", "src/Pokemon/Pokemon4Mutable.cpp",
                                               "include/Integration/Gen5/Gen5SharedScreenState.h",
                                               "include/UI/Gen2PokemonSession.h"], tests=[
        ("tests/test_species_change_moves_gen2.cpp", "Crystal species change, both Met lists walked (R12 updated)"),
        ("tests/test_species_change_moves_gen3.cpp", "Gen III 1..386 only, native reconciliation (R12 updated)"),
        ("tests/test_species_change_moves_gen4.cpp", "Gen IV PID-linked reconciliation with rollback"),
        ("tests/test_species_change_moves_gen5.cpp", "Gen V speciesRefusal for non-native species")]),
    "PBX-CRT-007": dict(extra=True, acc="3634d1e6", files=["include/Integration/Gen5/Gen5SharedScreenState.h",
                                               "include/Integration/Gen5/Gen5NativeCreate.h",
                                               "include/Integration/Gen5/Gen5CreateValidation.h"], tests=[
        ("tests/test_gen5_party_box_parity.cpp", "retail encrypted blank slots recognised; Create -> Met Location -> Keep -> export"),
        ("tests/test_gen5_create_edit_parity.cpp", "Gen V Create header / rows")], new=[
        (NEW_G5, "recheckCreateStartsAtZero5: default Create in an empty BW / B2W2 slot -> Keep -> export reparses")]),
    "PBX-CRT-008": dict(acc="62a2f8c3", files=["src/UI/Gen5SharedReadOnlySurface.inc"], tests=[
        ("tests/test_gen5_create_edit_parity.cpp", "S03 labels fit the column; locked rows gold")]),
    "PBX-MET-003": dict(acc="62a2f8c3", files=["include/UI/Gen2PokemonPickerModel.h", "src/UI/Gen2PokemonPickerOverlay.inc",
                                               "src/UI/Gen2UnifiedPokemonWorkspace.inc"], tests=[
        ("tests/test_gen2_location_picker.cpp", "proportional geometry; every R12 row fits; catch templates names only")], new=[
        (NEW_MET, "every row of the R12 default + unverified Crystal lists is a name (no raw id / tag)")]),
    "PBX-MOV-003": dict(acc="62a2f8c3", files=["include/UI/Gen1ListFit.h", "src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc",
                                               "src/UI/Gen1PokemonEditorOverlayUX.inc"], tests=[
        ("tests/test_gen1_list_fit.cpp", "only whole rows above the status line are drawn; focus never hidden"),
        ("tests/test_gen1_pokemon_editor_ui_contract.cpp", "Gen I moves rows (M08 guard)")]),
    "PBX-CRT-010": dict(acc="62a2f8c3", files=["src/UI/Gen2PokemonEditorFoundation.inc", "src/UI/Gen2SharedPokemonSurface.inc"], tests=[
        ("tests/test_ui_gen2_create_review_host.cpp", "real screen: B asks, A keeps; + Review / B back; Y never Review"),
        (GATE, "C05 per-ID guard")]),
    "PBX-MET-004": dict(acc="62a2f8c3", files=["include/UI/Gen3EncounterPickerModel.h", "include/UI/ExactGameMetLocations.h",
                                               "src/UI/Gen3SharedPokemonSurface.inc"], tests=[
        ("tests/test_gen3_exact_game_met_locations.cpp", "exact-origin native rows; names unique per list")], new=[
        (NEW_MET, "every row of the R12 default + unverified Gen III lists: no (RSE)/(FRLG) tag or raw id")]),
    "PBX-MET-005": dict(acc="26186464", files=["include/UI/Gen3EncounterPickerModel.h", "src/UI/SharedPokemonEditorSurface.inc",
                                               "src/UI/Gen3SharedPokemonSurface.inc", "include/UI/SharedMenuGeometry.h"], tests=[
        ("tests/test_met_location_fit.cpp", "620 px window; every R12 row (verified / level-blocked / unverified) fits"),
        ("tests/test_met_location_rows_distinct.cpp", "no two visible Gen IV / V rows identical")]),
    "PBX-MOV-004": dict(acc="62a2f8c3", files=["src/UI/Gen5SharedReadOnlySurface.inc", "include/UI/SharedMovesPanel.h"], tests=[
        ("tests/test_shared_editor_unit.cpp", "S01: shared Moves panel PP / PP Ups captions within bounds (Gen V uses it)")]),
    "PBX-CRT-011": dict(acc="62a2f8c3", files=["src/UI/Gen5SharedReadOnlySurface.inc"], tests=[
        ("tests/test_gen5_create_edit_parity.cpp", "Create header names the exact game")]),
    "PBX-SPC-004": dict(acc="3634d1e6", files=["include/UI/SpeciesChangeMoves.h", "include/UI/Gen2PokemonSession.h",
                                               "include/UI/Gen3SharedPokemonSession.h", "include/UI/Gen4SharedPokemonSession.h",
                                               "src/UI/Gen5SharedReadOnlySurface.inc"], tests=[
        ("tests/test_species_change_moves_gen1.cpp", "Thunder Shock cleared, Growl kept with PP / PP Ups, nothing added"),
        ("tests/test_species_change_moves_gen2.cpp", "Gen II keepLearnable, no refill (R12 updated)"),
        ("tests/test_species_change_moves_gen3.cpp", "Gen III keepLearnable, no refill (R12 updated)"),
        ("tests/test_species_change_moves_gen4.cpp", "Gen IV keepLearnable, no refill"),
        ("tests/test_species_change_moves_gen5.cpp", "Gen V keepLearnable, no refill")]),
    "PBX-CRT-012": dict(acc="26186464", files=["include/Integration/Gen1/Gen1StagedPokemonEditor.h",
                                               "include/Integration/Gen2/Gen2StagedEditor.h",
                                               "include/Integration/Gen5/Gen5SharedScreenState.h",
                                               "include/Integration/Gen5/Gen5NativeCreate.h",
                                               "src/UI/Gen2PokemonEditorFoundation.inc"], tests=[
        (CG, "CG55 Create defaults DVs / IVs 0 (literal)")], new=[
        (NEW_G5, "recheckCreateStartsAtZero5: Gen V draft and exported record IVs 0 / EVs 0; neighbours untouched"),
        (NEW_UI, "real screen: Gen II created Pokemon has DVs 0; the existing Pokemon is untouched")]),
    "PBX-CRT-013": dict(acc="26186464", files=["src/Integration/Gen3/Gen3StagedPokemonEditor.cpp",
                                               "include/UI/Gen3SharedPokemonSession.h", "src/UI/Gen3SharedPokemonSurface.inc"], tests=[
        ("tests/test_gen3_staged_pokemon_editor.cpp", "Create x every ball 1..12 x held items -> persisted after Keep / reparse"),
        (GATE, "R2-REVIEWMOVE guards (named Review values)")]),
    "PBX-MOV-008": dict(acc="26186464", files=["src/UI/Gen5SharedReadOnlySurface.inc"], tests=[
        ("tests/test_ui_editor_input_host.cpp", "real screen: Gen V Moves panel marks Thunder Shock OK, not Learnable")]),
    "PBX-CRT-017": dict(acc="cfb70357", files=["src/UI/Gen2PokemonEditorFoundation.inc", "include/UI/TransientStatusScope.h",
                                               "src/UI/TrainerViewScreenBase.inc"], tests=[
        ("tests/test_gsc_editor_surface_contract.cpp", "'added to Box' literal in the Add path")], new=[
        (NEW_UI, "real screen: toast is exactly 'PIKACHU added to Box 1', drawn over the box after R12-010 scoping")]),
    # ---------------------------------------------------------------- bag / pc items
    "PBX-BAG-005": dict(acc="62a2f8c3", files=["src/UI/ClassicInventoryOverlay.cpp", "include/UI/SharedBagAddFlow.h"], tests=[
        ("tests/test_bag_add_flow_gen15.cpp", "Add stages x1 with no keypad; existing stack focused; invalid ids refused"),
        ("tests/test_shared_bag_quantity.cpp", "quantity editor bounds"),
        ("tests/test_item_quantity_policy.cpp", "per-game caps (CG09)")]),
    "PBX-BAG-006": dict(acc="62a2f8c3", files=["src/UI/Gen4SharedPokemonSurface.inc", "src/UI/Gen5SharedReadOnlySurface.inc",
                                               "include/Integration/Gen5/Gen5BagScreen.h"], tests=[
        ("tests/test_bag_add_flow_gen15.cpp", "Gen IV stageBagAdd / Gen V pickAdd at x1"),
        ("tests/test_gen5_bag_screen.cpp", "Gen V Add / Remove / quantity screen model"),
        ("tests/test_gen4_bag_remove_pack.cpp", "Gen IV add / remove packing")]),
    "PBX-BAG-010": dict(acc="26186464", files=["include/Integration/Gen5/Gen5BagScreen.h", "src/UI/Gen5SharedReadOnlySurface.inc"], tests=[
        ("tests/test_gen5_bag_screen.cpp", "Key Item / TM add / remove, unique cap 1, story warning"),
        ("tests/test_gen5_bag.cpp", "native SAV5 pockets")]),
    "PBX-PCI-001": dict(acc="50dac31f", files=["src/Inventory/ClassicInventoryCatalog.cpp", "src/UI/ClassicInventoryOverlay.cpp",
                                               "include/UI/BagTopUpModel.h"], tests=[
        ("tests/test_classic_inventory_catalog.cpp", "PC Items catalogs per game"),
        ("tests/test_bag_top_up_adapters.cpp", "Gen I / II / III PC Items state through the real adapters (R12 updated)")]),
    # ---------------------------------------------------------------- trainer
    "PBX-TRN-001": dict(acc="26186464", files=["include/Integration/Gen3/Gen3StagedTrainerEditor.h",
                                               "include/UI/ClassicTrainerPage.h", "src/UI/TrainerViewScreenBase.inc"], tests=[
        ("tests/test_gen3_trainer_edit.cpp", "FRLG / RSE trainer fields read, stage, round-trip")]),
    "PBX-TRN-002": dict(acc="26186464", files=["include/Integration/Gen2/Gen2StagedEditor.h", "include/UI/ClassicTrainerPage.h",
                                               "src/UI/Gen2TrainerSessionOverlay.inc"], tests=[
        ("tests/test_trainer_playtime_gender_rows.cpp", "Gen II Play Time / Crystal Gender rows editable with native writers")]),
    "PBX-TRN-004": dict(acc="3634d1e6", files=["include/Integration/Gen5/Gen5TrainerScreen.h",
                                               "include/Integration/Gen5/Gen5TrainerName.h"], tests=[
        ("tests/test_gen5_trainer.cpp", "rows, editability, Up / Down wrap"),
        ("tests/test_gen5_trainer_footer.cpp", "A: Edit only on editable rows")]),
    "PBX-TRN-005": dict(acc="26186464", files=["include/Integration/Gen1/Gen1StagedTrainerEditor.h",
                                               "include/UI/ClassicTrainerEditability.h", "include/UI/ClassicTrainerPage.h"], tests=[
        ("tests/test_gen1_trainer_edit.cpp", "Gen I name / money stage + export"),
        ("tests/test_classic_trainer_page.cpp", "rows / editability; A: Edit where editable")]),
    "PBX-TRN-006": dict(acc="26186464", files=["include/UI/ClassicTrainerSession.h", "include/UI/ClassicStagedExit.h",
                                               "include/UI/ClassicTrainerPage.h"], tests=[
        ("tests/test_gen3_trainer_edit.cpp", "Gen III name / money stage, Review, export"),
        (GATE, "GUARDS_ROUND5 R2-TRAINER / R2-BUG-002")]),
    "PBX-TRN-007": dict(acc="c60d4da0", files=["include/UI/ClassicTrainerPage.h"], tests=[
        ("tests/test_native_trainer_badges.cpp", "native badges incl. Johto / Kanto (qa-contracts)"),
        ("tests/test_classic_trainer_page.cpp", "Play Time / Pokedex rows")]),
    "PBX-TRN-008": dict(acc="26186464", files=["include/Integration/Gen2/Gen2StagedEditor.h",
                                               "include/Integration/Gen3/Gen3StagedTrainerEditor.h",
                                               "include/Integration/Gen4/Gen4StagedTrainerEditor.h", "include/UI/ClassicTrainerPage.h"], tests=[
        ("tests/test_trainer_playtime_gender_rows.cpp", "editable rows only with native writers"),
        ("tests/test_native_trainer_badges.cpp", "badges (qa-contracts)")]),
    "PBX-TRN-009": dict(acc="26186464", files=["include/UI/ClassicTrainerPage.h"], tests=[
        ("tests/test_classic_trainer_page.cpp", "no Gender row for Gold / Silver / Gen I")]),
    "PBX-TRN-011": dict(acc="3634d1e6", files=["include/Integration/Gen5/Gen5TrainerScreen.h",
                                               "include/Integration/Gen5/Gen5TrainerName.h"], tests=[
        ("tests/test_gen5_trainer.cpp", "Gender editable / focusable, 'Female' after stage")]),
    # ---------------------------------------------------------------- backups / export / review
    "PBX-BKP-002": dict(acc="26186464", files=["src/UI/BackupSelectionScreen.cpp", "include/UI/HomeQuitConfirm.h",
                                               "src/UI/SaveSelectScreen.cpp", "include/UI/SourceBackupClassification.h"], tests=[
        ("tests/test_emulator_backup_access.cpp", "original emulator file never deletable; only app export records"),
        ("tests/test_gen5_backup_listing_host.cpp", "Gen V: delete removes exactly one record; refuses last copy")]),
    "PBX-BKP-003": dict(acc="3634d1e6", files=["src/Integration/Common/VerifiedStagedExport.cpp",
                                               "src/Integration/Gen2/Gen2ExportTransaction.cpp",
                                               "src/Integration/Gen4/Gen4ExportTransaction.cpp", "include/UI/ClassicSourceGuard.h"], tests=[
        ("tests/test_qa_export_contracts.cpp", "C1-C6: source bytes unchanged for every export path (qa-contracts)"),
        ("tests/test_classic_verified_export.cpp", "Gen I / II exact original backup, fault cleanup")]),
    "PBX-BKP-004": dict(acc="26186464", files=["src/UI/SaveSelectScreen.cpp", "include/UI/SourceBackupClassification.h",
                                               "include/UI/SourceBackupOpen.h", "include/UI/EmulatorBackupLayout.h"], tests=[
        ("tests/test_emulator_backup_access.cpp", "export records listed, originals read-only"),
        ("tests/test_backup_snapshot_open.cpp", "A opens the exact immutable historical copy"),
        ("tests/test_emulator_backup_layout.cpp", "full-screen layout, newest first")]),
    "PBX-BKP-006": dict(acc="26186464", files=["include/UI/SourceBackupClassification.h",
                                               "src/Integration/Common/VerifiedStagedExport.cpp"], tests=[
        ("tests/test_gen5_backup_listing_host.cpp", "G1: Gen V exports in Save Backups, profile scoped"),
        ("tests/test_qa_export_contracts.cpp", "G2-G5 export contracts")]),
    "PBX-EXP-001": dict(acc="3634d1e6", files=["src/Integration/Common/VerifiedStagedExport.cpp",
                                               "src/Integration/Gen2/Gen2ExportTransaction.cpp",
                                               "src/Integration/Gen4/Gen4ExportTransaction.cpp",
                                               "src/Integration/Gen5/Gen5WorkingCopyExport.cpp"], tests=[
        ("tests/test_qa_export_contracts.cpp", "C1-C6 incl. never overwrite, no partial output"),
        ("tests/test_classic_verified_export.cpp", "Gen I / II original backup + checksum-valid edited save"),
        ("tests/test_gen5_export_validation.cpp", "Gen V candidates strict-reparse")]),
    "PBX-REV-002": dict(acc="26186464", files=["src/UI/Gen1PokemonEditorOverlayUX.inc", "src/UI/Gen1PokemonEditorOverlay.cpp",
                                               "include/UI/SharedEditorChrome.h", "include/UI/SharedReviewPresentation.h"], tests=[
        ("tests/test_gen12_review_parity.cpp", "Gen I uses the shared paged Review")]),
    "PBX-REV-003": dict(acc="62a2f8c3", status="partial", files=["src/UI/Gen5SharedReadOnlySurface.inc", "include/UI/ReviewEntry.h"], tests=[
        ("tests/test_gen5_review_discard_export_trace.cpp", "W / B / W2 / B2 Review / Discard / Export"),
        ("tests/test_review_entry.cpp", "Gen V Boxes open Review with + (R12-001)")],
        superseded="R12-001 (owner H03 / H16): Gen V Boxes Review key moved from Y to +; reachability is still pinned."),
    "PBX-REV-004": dict(acc="3634d1e6", files=["include/UI/Gen12ReviewDiscardConfirm.h", "include/UI/SharedEditorChrome.h",
                                               "src/Integration/Gen1/Gen1StagedPokemonEditor.cpp",
                                               "src/Integration/Gen2/Gen2StagedEditor.cpp"], tests=[
        ("tests/test_qa_export_contracts.cpp", "C6 Discard All restores; later export refused"),
        ("tests/test_gen12_review_parity.cpp", "two-step Discard confirmation")]),
    "PBX-REV-006": dict(acc="26186464", files=["include/UI/SharedReviewPresentation.h",
                                               "src/Integration/Gen4/Gen4ExportTransaction.cpp"], tests=[
        ("tests/test_gen3_staged_pokemon_editor.cpp", "Review rows carry named values (ball / item names)"),
        (CG, "CG32 Review rows readable names")]),
    "PBX-REV-008": dict(acc="26186464", files=["include/UI/ReviewEntry.h", "include/UI/SharedReviewPresentation.h"], tests=[
        ("tests/test_gen12_review_parity.cpp", "shared Review presenter"),
        ("tests/test_classic_party_review_path.cpp", "Party review path"),
        ("tests/test_review_entry.cpp", "one Review key per screen")]),
    "PBX-REV-010": dict(acc="26186464", files=["include/UI/ReviewEntry.h", "src/UI/ClassicInventoryOverlay.cpp"], tests=[
        ("tests/test_review_entry.cpp", "Bag opens Review with + (Y stays Remove)"),
        (CG, "CG73 Review entry visible Gen I-V")]),
    "PBX-OPN-004": dict(acc="0760334f", status="scope", files=["src/UI/SaveSelectScreen.cpp", "src/Save/GetSaveFileContents.cpp"], tests=[
        ("tests/test_backup_namespace_contract.cpp", "native titles (e.g. 'Pokemon Sword') get their own backup root (indirect)"),
        ("tests/test_modern_trainer_blank_slots.cpp", "modern native save parsing (indirect)")],
        note="Native Gen VIII / IX titles are outside the Gen I-V scope (owner: no Gen VI-IX work): recorded with "
             "indirect coverage only, not extended."),
    "PBX-FIL-001": dict(acc="3634d1e6", files=["src/UI/PokemonFileMenus.inc", "include/UI/PokemonFileMenus.h",
                                               "include/UI/PokemonFileDialogModel.h", "src/UI/PokemonFileDialog.cpp",
                                               "include/UI/SharedPokemonEditorContract.h"], tests=[
        ("tests/test_pokemon_file_gen12.cpp", "import into the first empty packed slot, refuse wrong-gen, non-mutating export"),
        ("tests/test_ui_pokemon_file_menus_host.cpp", "real screen: X Export / Import, ' (2)' names, Review lists the import")]),
    # ---------------------------------------------------------------- actions / bank / carry / release
    "PBX-ACT-001": dict(acc="26186464", files=["include/UI/NarrowPokemonActionMenu.h", "include/UI/ClassicGenericUiGuard.h",
                                               "include/UI/SharedPokemonEditorContract.h", "src/UI/TrainerViewScreenBase.inc"], tests=[
        ("tests/test_classic_pokemon_action_menu.cpp", "Gen I / II sheet rows are real commands in shared order"),
        ("tests/test_action_sheet_model.cpp", "generic sheet close / navigate (Gen VI+ only)"),
        (CARRY_UI, "real screen: Actions -> Clone / Move; B closes; no Actions reopen")]),
    "PBX-ACT-003": dict(acc="62a2f8c3", files=["include/UI/NarrowPokemonActionMenu.h", "include/UI/ClassicPokemonActionMenu.h"], tests=[
        ("tests/test_classic_pokemon_action_menu.cpp", "no Make Shiny row"),
        ("tests/test_gen4_hardware_surface_contract.cpp", "Gen IV Actions vocabulary has no Make Shiny")]),
    "PBX-ACT-004": dict(acc="d0a45909", files=["include/UI/ClassicPokemonActionMenu.h", "include/UI/NarrowPokemonActionMenu.h",
                                               "include/UI/SharedMenuGeometry.h"], tests=[
        ("tests/test_classic_pokemon_action_menu.cpp", "Gen I / II rows and order, empty-slot menu"),
        ("tests/test_gen4_hardware_surface_contract.cpp", "Gen IV narrow menu")]),
    "PBX-ACT-007": dict(acc="26186464", files=["include/UI/ClassicPartyActions.h", "src/UI/ClassicPartyActions.inc",
                                               "src/UI/TrainerViewScreenBase.inc"], tests=[
        ("tests/test_classic_party_actions.cpp", "Party popup rows / dispatch (View, Edit, Origin, Review, Cancel)"),
        ("tests/test_classic_party_review_path.cpp", "Party review path")]),
    "PBX-BNK-001": dict(acc="3be4de6b", files=["include/Trainer/Bank.h", "src/Trainer/Bank.cpp",
                                               "include/Trainer/BankLoadPolicy.h", "src/UI/Panels/StoragePanel.cpp"], tests=[
        ("tests/test_bank_load_policy.cpp", "LoadOutcome classification over real files"),
        ("tests/test_bank_recovery_contract.cpp", "interrupted save recovery, fail-closed writes"),
        ("tests/test_bank_format_policy.cpp", "box-count migration policy")]),
    "PBX-CRY-002": dict(acc="62a2f8c3", status="partial", files=["include/UI/ClassicCarryRules.h", "src/UI/ClassicPackedMoveOverlay.inc",
                                                                  "src/UI/Gen4SharedPokemonSurface.inc"], tests=[
        (CARRY_UI, "real screen Gen IV: Y pickup, B restores exact staged bytes, A places, between boxes"),
        ("tests/test_gen4_storage_carry.cpp", "Gen IV Y multi-move (AUD-08)"),
        ("tests/test_gen4_hardware_surface_contract.cpp", "Gen IV carry display")],
        superseded="R11 H23 (owner: A places, 6f62b69e) and R12-002 (no pickup / place toasts): Y-to-place and "
                   "carry toasts are not pinned."),
    "PBX-CRY-004": dict(acc="62a2f8c3", status="partial", files=["src/UI/ClassicPackedMoveOverlay.inc",
                                                                  "src/Integration/Gen1/Gen1PackedMove.cpp",
                                                                  "include/UI/ClassicCarryRules.h"], tests=[
        ("tests/test_gen12_packed_move.cpp", "Gen I sheet lists Move; Move -> drop stages one verified move; B byte-identical"),
        (CARRY_UI, "real screen Gen I: Y pickup, packed landing slot, A places, B cancels")],
        superseded="R11 H23 (A places) and R12-001/002/003 (Y keys, no routine toasts, one packed focus)."),
    "PBX-CRY-005": dict(acc="3634d1e6", status="partial", files=["src/UI/ClassicPackedMoveOverlay.inc",
                                                                  "src/Integration/Gen2/Gen2PackedMove.cpp",
                                                                  "include/UI/ClassicCarryRules.h"], tests=[
        ("tests/test_gen12_packed_move.cpp", "Gen II packed move within / between boxes, cancel restores"),
        ("tests/test_gen2_box_move_presentation.cpp", "pickup refreshes the grid mid-carry (NEW-04)"),
        (CARRY_UI, "real screen Gen II: pickup, landing slot, A places between boxes, B cancels")],
        superseded="R12-001/002/003 (owner H03 / H04): carry keys, toasts and packed focus changed; not pinned."),
    "PBX-REL-002": dict(acc="62a2f8c3", files=["src/UI/ClassicReleaseActionFix.inc", "src/UI/Gen3SharedPokemonSurface.inc"], tests=[
        ("tests/test_touch_pokemon_editor_direct_rows_contract.py", "classic Release keeps a Y-only guard (literal)"),
        ("tests/test_shared_editor_unit.cpp", "Release confirmation: Y confirms, B cancels"),
        (GATE, "C08 guard")], new=[
        (NEW_UI, "real screen Gen II: Actions -> Release; A releases nothing, B keeps the Pokemon, only Y stages it")]),
}
