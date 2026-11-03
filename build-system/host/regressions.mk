# PokeBank NX host test rules: regressions.mk
# Consolidated from the original Makefile.host.* fragments in their EXACT include order.
# Do not modify host-test recipes or weaken coverage while reorganizing infrastructure.

# ---- BEGIN original Makefile.host.gen14 ----
# Gen I-IV completion campaign (Thread C) host tests.
GEN1_EXPORT_SOURCES := tests/test_gen1_export_transaction.cpp $(GEN1_POKEMON_COMMON) \
	src/Integration/Gen1/Gen1ExportTransaction.cpp src/Integration/Common/VerifiedStagedExport.cpp \
	src/Integration/Gen1/Gen1PackedMove.cpp src/Utils/SHA256.cpp
HOST_TESTS += $(HOST_BUILD)/test_gen1_export_transaction
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen1_export_transaction_sanitize
host-test: $(HOST_BUILD)/test_gen1_export_transaction
host-sanitize: $(HOST_BUILD)/test_gen1_export_transaction_sanitize
$(HOST_BUILD)/test_gen1_export_transaction: $(GEN1_EXPORT_SOURCES) src/Integration/Gen1/Gen1PersonalData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN1_EXPORT_SOURCES) -o $@
$(HOST_BUILD)/test_gen1_export_transaction_sanitize: $(GEN1_EXPORT_SOURCES) src/Integration/Gen1/Gen1PersonalData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN1_EXPORT_SOURCES) -o $@

# Gen I/II shared-editor contract fixes: two-step Review discard, modal exit footer, picker entry.
GEN12_REVIEW_PARITY_SOURCES := tests/test_gen12_review_parity.cpp
HOST_TESTS += $(HOST_BUILD)/test_gen12_review_parity
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen12_review_parity_sanitize
host-test: $(HOST_BUILD)/test_gen12_review_parity
host-sanitize: $(HOST_BUILD)/test_gen12_review_parity_sanitize
$(HOST_BUILD)/test_gen12_review_parity: $(GEN12_REVIEW_PARITY_SOURCES) include/UI/Gen12ReviewDiscardConfirm.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN12_REVIEW_PARITY_SOURCES) -o $@
$(HOST_BUILD)/test_gen12_review_parity_sanitize: $(GEN12_REVIEW_PARITY_SOURCES) include/UI/Gen12ReviewDiscardConfirm.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN12_REVIEW_PARITY_SOURCES) -o $@

# Gen II Actions -> Clone dialog ownership (ClassicReleaseActionFix.inc).
GEN2_CLONE_FLOW_SOURCES := tests/test_gen2_clone_flow.cpp
HOST_TESTS += $(HOST_BUILD)/test_gen2_clone_flow
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_clone_flow_sanitize
host-test: $(HOST_BUILD)/test_gen2_clone_flow
host-sanitize: $(HOST_BUILD)/test_gen2_clone_flow_sanitize
$(HOST_BUILD)/test_gen2_clone_flow: $(GEN2_CLONE_FLOW_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN2_CLONE_FLOW_SOURCES) -o $@
$(HOST_BUILD)/test_gen2_clone_flow_sanitize: $(GEN2_CLONE_FLOW_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN2_CLONE_FLOW_SOURCES) -o $@

# Gen I/II Y Random DVs confirmation.
GEN12_REROLL_SOURCES := tests/test_gen12_dv_reroll.cpp src/Integration/Gen2/Gen2PersonalData.cpp
GEN12_REROLL_DEPS := include/UI/Gen12DVReroll.h src/UI/Gen2UnifiedPokemonWorkspace.inc \
	src/UI/Gen2PokemonEditorFoundation.inc src/UI/Gen1PokemonEditorOverlayFoundation.inc
HOST_TESTS += $(HOST_BUILD)/test_gen12_dv_reroll
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen12_dv_reroll_sanitize
host-test: $(HOST_BUILD)/test_gen12_dv_reroll
host-sanitize: $(HOST_BUILD)/test_gen12_dv_reroll_sanitize
$(HOST_BUILD)/test_gen12_dv_reroll: $(GEN12_REROLL_SOURCES) $(GEN12_REROLL_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN12_REROLL_SOURCES) -o $@
$(HOST_BUILD)/test_gen12_dv_reroll_sanitize: $(GEN12_REROLL_SOURCES) $(GEN12_REROLL_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN12_REROLL_SOURCES) -o $@

# Gen III PID gender accessor (PokemonRecord::gender), cross-checked against PKSM personal3.
GEN3_GENDER_SOURCES := tests/test_gen3_gender.cpp src/Pokemon/PersonalInfoTable.cpp
HOST_TESTS += $(HOST_BUILD)/test_gen3_gender
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen3_gender_sanitize
host-test: $(HOST_BUILD)/test_gen3_gender
host-sanitize: $(HOST_BUILD)/test_gen3_gender_sanitize
$(HOST_BUILD)/test_gen3_gender: $(GEN3_GENDER_SOURCES) include/Integration/Gen3/Gen3Gender.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN3_GENDER_SOURCES) -o $@
$(HOST_BUILD)/test_gen3_gender_sanitize: $(GEN3_GENDER_SOURCES) include/Integration/Gen3/Gen3Gender.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN3_GENDER_SOURCES) -o $@

# Gen I View navigation (read-only, focusable).
GEN1_VIEW_NAV_SOURCES := tests/test_gen1_view_navigation.cpp
HOST_TESTS += $(HOST_BUILD)/test_gen1_view_navigation
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen1_view_navigation_sanitize
host-test: $(HOST_BUILD)/test_gen1_view_navigation
host-sanitize: $(HOST_BUILD)/test_gen1_view_navigation_sanitize
$(HOST_BUILD)/test_gen1_view_navigation: $(GEN1_VIEW_NAV_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN1_VIEW_NAV_SOURCES) -o $@
$(HOST_BUILD)/test_gen1_view_navigation_sanitize: $(GEN1_VIEW_NAV_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN1_VIEW_NAV_SOURCES) -o $@

# Gen II Pokerus / Crystal Met Time / OT Gender selectors.
GEN2_FIELD_SELECTOR_SOURCES := tests/test_gen2_field_selector.cpp
HOST_TESTS += $(HOST_BUILD)/test_gen2_field_selector
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_field_selector_sanitize
host-test: $(HOST_BUILD)/test_gen2_field_selector
host-sanitize: $(HOST_BUILD)/test_gen2_field_selector_sanitize
$(HOST_BUILD)/test_gen2_field_selector: $(GEN2_FIELD_SELECTOR_SOURCES) include/UI/Gen2FieldSelector.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN2_FIELD_SELECTOR_SOURCES) -o $@
$(HOST_BUILD)/test_gen2_field_selector_sanitize: $(GEN2_FIELD_SELECTOR_SOURCES) include/UI/Gen2FieldSelector.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN2_FIELD_SELECTOR_SOURCES) -o $@

# Gen I-III picker paging parity with the shared editor.
GEN13_PAGING_SOURCES := tests/test_gen13_picker_paging.cpp
HOST_TESTS += $(HOST_BUILD)/test_gen13_picker_paging
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen13_picker_paging_sanitize
host-test: $(HOST_BUILD)/test_gen13_picker_paging
host-sanitize: $(HOST_BUILD)/test_gen13_picker_paging_sanitize
$(HOST_BUILD)/test_gen13_picker_paging: $(GEN13_PAGING_SOURCES) include/UI/Gen13PickerPaging.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN13_PAGING_SOURCES) -o $@
$(HOST_BUILD)/test_gen13_picker_paging_sanitize: $(GEN13_PAGING_SOURCES) include/UI/Gen13PickerPaging.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN13_PAGING_SOURCES) -o $@

# Gen I Edit working-copy draft.
GEN1_EDIT_DRAFT_SOURCES := tests/test_gen1_edit_draft.cpp $(GEN1_POKEMON_COMMON) src/Integration/Gen1/Gen1PackedMove.cpp
HOST_TESTS += $(HOST_BUILD)/test_gen1_edit_draft
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen1_edit_draft_sanitize
host-test: $(HOST_BUILD)/test_gen1_edit_draft
host-sanitize: $(HOST_BUILD)/test_gen1_edit_draft_sanitize
$(HOST_BUILD)/test_gen1_edit_draft: $(GEN1_EDIT_DRAFT_SOURCES) include/UI/Gen1EditDraft.h src/Integration/Gen1/Gen1PersonalData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN1_EDIT_DRAFT_SOURCES) -o $@
$(HOST_BUILD)/test_gen1_edit_draft_sanitize: $(GEN1_EDIT_DRAFT_SOURCES) include/UI/Gen1EditDraft.h src/Integration/Gen1/Gen1PersonalData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN1_EDIT_DRAFT_SOURCES) -o $@

# Gen II Create/Edit workspace: one input owner per frame, footer == action (hardware a0633b6d).
GEN2_WORKSPACE_LAYERS_SOURCES := tests/test_gen2_workspace_layers.cpp
GEN2_WORKSPACE_LAYERS_DEPS := include/UI/Gen2WorkspaceLayers.h include/UI/Gen2FieldSelector.h \
	src/UI/Gen2UnifiedPokemonWorkspace.inc \
	src/UI/Gen2PokemonEditorFoundation.inc src/UI/Gen2HardwareFinalFix.inc
HOST_TESTS += $(HOST_BUILD)/test_gen2_workspace_layers
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_workspace_layers_sanitize
host-test: $(HOST_BUILD)/test_gen2_workspace_layers
host-sanitize: $(HOST_BUILD)/test_gen2_workspace_layers_sanitize
$(HOST_BUILD)/test_gen2_workspace_layers: $(GEN2_WORKSPACE_LAYERS_SOURCES) $(GEN2_WORKSPACE_LAYERS_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN2_WORKSPACE_LAYERS_SOURCES) -o $@
$(HOST_BUILD)/test_gen2_workspace_layers_sanitize: $(GEN2_WORKSPACE_LAYERS_SOURCES) $(GEN2_WORKSPACE_LAYERS_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN2_WORKSPACE_LAYERS_SOURCES) -o $@

# Gen I/II Pokemon Actions sheet: real commands only, shared order (owner campaign phase 3).
CLASSIC_ACTION_MENU_SOURCES := tests/test_classic_pokemon_action_menu.cpp
CLASSIC_ACTION_MENU_DEPS := include/UI/ClassicPokemonActionMenu.h include/UI/NarrowPokemonActionMenu.h \
	include/UI/SharedPokemonEditorContract.h include/UI/Gen1PokemonEditorUIContract.h \
	include/UI/PokemonViewActions.h src/UI/ClassicReleaseActionFix.inc src/UI/Gen2SharedSurfaceParity.inc \
	src/UI/Gen2SharedPokemonSurface.inc src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc \
	src/UI/TrainerViewScreenGSCOverlay.inc
HOST_TESTS += $(HOST_BUILD)/test_classic_pokemon_action_menu
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_classic_pokemon_action_menu_sanitize
host-test: $(HOST_BUILD)/test_classic_pokemon_action_menu
host-sanitize: $(HOST_BUILD)/test_classic_pokemon_action_menu_sanitize
$(HOST_BUILD)/test_classic_pokemon_action_menu: $(CLASSIC_ACTION_MENU_SOURCES) $(CLASSIC_ACTION_MENU_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(CLASSIC_ACTION_MENU_SOURCES) -o $@
$(HOST_BUILD)/test_classic_pokemon_action_menu_sanitize: $(CLASSIC_ACTION_MENU_SOURCES) $(CLASSIC_ACTION_MENU_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(CLASSIC_ACTION_MENU_SOURCES) -o $@

# Gen II inline Shiny / Pokerus / Gender (owner campaign phase 10).
GEN2_INLINE_FIELDS_SOURCES := tests/test_gen2_inline_fields.cpp
GEN2_INLINE_FIELDS_DEPS := include/UI/Gen2NativePresentation.h src/UI/Gen2UnifiedPokemonWorkspace.inc \
	src/UI/Gen2PokemonEditorFoundation.inc src/UI/Gen2HardwareFinalFix.inc
HOST_TESTS += $(HOST_BUILD)/test_gen2_inline_fields
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_inline_fields_sanitize
host-test: $(HOST_BUILD)/test_gen2_inline_fields
host-sanitize: $(HOST_BUILD)/test_gen2_inline_fields_sanitize
$(HOST_BUILD)/test_gen2_inline_fields: $(GEN2_INLINE_FIELDS_SOURCES) $(GEN2_INLINE_FIELDS_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN2_INLINE_FIELDS_SOURCES) -o $@
$(HOST_BUILD)/test_gen2_inline_fields_sanitize: $(GEN2_INLINE_FIELDS_SOURCES) $(GEN2_INLINE_FIELDS_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN2_INLINE_FIELDS_SOURCES) -o $@

# Crystal Met Location chooser (owner campaign phase 11).
GEN2_LOCATION_PICKER_SOURCES := tests/test_gen2_location_picker.cpp $(GEN2_STAGED_EDITOR_COMMON)
GEN2_LOCATION_PICKER_DEPS := include/UI/Gen2PokemonPickerModel.h src/UI/Gen2PokemonPickerOverlay.inc
HOST_TESTS += $(HOST_BUILD)/test_gen2_location_picker
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_location_picker_sanitize
host-test: $(HOST_BUILD)/test_gen2_location_picker
host-sanitize: $(HOST_BUILD)/test_gen2_location_picker_sanitize
$(HOST_BUILD)/test_gen2_location_picker: $(GEN2_LOCATION_PICKER_SOURCES) $(GEN2_LOCATION_PICKER_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN2_LOCATION_PICKER_SOURCES) -o $@
$(HOST_BUILD)/test_gen2_location_picker_sanitize: $(GEN2_LOCATION_PICKER_SOURCES) $(GEN2_LOCATION_PICKER_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN2_LOCATION_PICKER_SOURCES) -o $@

# Gen I list editors: whole rows above the status line, no "Row x/y" counters (phases 1/5).
GEN1_LIST_FIT_SOURCES := tests/test_gen1_list_fit.cpp
HOST_TESTS += $(HOST_BUILD)/test_gen1_list_fit
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen1_list_fit_sanitize
host-test: $(HOST_BUILD)/test_gen1_list_fit
host-sanitize: $(HOST_BUILD)/test_gen1_list_fit_sanitize
$(HOST_BUILD)/test_gen1_list_fit: $(GEN1_LIST_FIT_SOURCES) include/UI/Gen1ListFit.h src/UI/Gen1PokemonEditorOverlayUX.inc src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN1_LIST_FIT_SOURCES) -o $@
$(HOST_BUILD)/test_gen1_list_fit_sanitize: $(GEN1_LIST_FIT_SOURCES) include/UI/Gen1ListFit.h src/UI/Gen1PokemonEditorOverlayUX.inc src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN1_LIST_FIT_SOURCES) -o $@

GEN2_MOVES_PANEL_SOURCES := tests/test_gen2_moves_panel.cpp $(GEN2_STAGED_EDITOR_COMMON) src/Integration/Gen1/Gen1StagedPokemonEditor.cpp src/Integration/Gen1/Gen1ReadOnlySave.cpp src/Integration/Gen1/Gen1BattleStats.cpp
GEN2_MOVES_PANEL_DEPS := include/UI/Gen2MovesPanel.h include/UI/ClassicMovesPanel.h include/UI/SharedMovesPanel.h include/UI/SharedMoveFacts.h src/UI/Gen1PokemonEditorFoundationHardwareFix.inc src/UI/Gen1PokemonDetailsPresentation.cpp src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc
HOST_TESTS += $(HOST_BUILD)/test_gen2_moves_panel
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_moves_panel_sanitize
host-test: $(HOST_BUILD)/test_gen2_moves_panel
host-sanitize: $(HOST_BUILD)/test_gen2_moves_panel_sanitize
$(HOST_BUILD)/test_gen2_moves_panel: $(GEN2_MOVES_PANEL_SOURCES) $(GEN2_MOVES_PANEL_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN2_MOVES_PANEL_SOURCES) -o $@
$(HOST_BUILD)/test_gen2_moves_panel_sanitize: $(GEN2_MOVES_PANEL_SOURCES) $(GEN2_MOVES_PANEL_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN2_MOVES_PANEL_SOURCES) -o $@

GEN2_SPECIAL_STAT_SOURCES := tests/test_gen2_special_stat.cpp src/Integration/Gen2/Gen2PersonalData.cpp src/Pokemon/Experience.cpp
GEN2_SPECIAL_STAT_DEPS := include/UI/Gen2WorkspacePresentation.h include/Integration/Gen2/Gen2BattleStats.h src/UI/Gen2UnifiedPokemonWorkspace.inc src/UI/Gen2HardwareFinalFix.inc
HOST_TESTS += $(HOST_BUILD)/test_gen2_special_stat
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_special_stat_sanitize
host-test: $(HOST_BUILD)/test_gen2_special_stat
host-sanitize: $(HOST_BUILD)/test_gen2_special_stat_sanitize
$(HOST_BUILD)/test_gen2_special_stat: $(GEN2_SPECIAL_STAT_SOURCES) $(GEN2_SPECIAL_STAT_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN2_SPECIAL_STAT_SOURCES) -o $@
$(HOST_BUILD)/test_gen2_special_stat_sanitize: $(GEN2_SPECIAL_STAT_SOURCES) $(GEN2_SPECIAL_STAT_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN2_SPECIAL_STAT_SOURCES) -o $@

CRYSTAL_MET_TIME_SOURCES := tests/test_crystal_met_time_inline.cpp $(GEN2_STAGED_EDITOR_COMMON)
CRYSTAL_MET_TIME_DEPS := include/UI/Gen2FieldSelector.h include/UI/Gen2NativePresentation.h src/UI/Gen2UnifiedPokemonWorkspace.inc tests/fixtures/gsc_pokemon_fixture.h
HOST_TESTS += $(HOST_BUILD)/test_crystal_met_time_inline
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_crystal_met_time_inline_sanitize
host-test: $(HOST_BUILD)/test_crystal_met_time_inline
host-sanitize: $(HOST_BUILD)/test_crystal_met_time_inline_sanitize
$(HOST_BUILD)/test_crystal_met_time_inline: $(CRYSTAL_MET_TIME_SOURCES) $(CRYSTAL_MET_TIME_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(CRYSTAL_MET_TIME_SOURCES) -o $@
$(HOST_BUILD)/test_crystal_met_time_inline_sanitize: $(CRYSTAL_MET_TIME_SOURCES) $(CRYSTAL_MET_TIME_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(CRYSTAL_MET_TIME_SOURCES) -o $@

GEN1_TRAINER_EDIT_SOURCES := tests/test_gen1_trainer_edit.cpp $(GEN1_POKEMON_COMMON) \
	src/Integration/Gen1/Gen1ExportTransaction.cpp src/Integration/Common/VerifiedStagedExport.cpp \
	src/Integration/Gen1/Gen1PackedMove.cpp src/Utils/SHA256.cpp
GEN1_TRAINER_EDIT_DEPS := include/Integration/Gen1/Gen1StagedTrainerEditor.h include/UI/ClassicTrainerSession.h include/UI/ClassicTrainerEditability.h include/UI/InventoryUIContract.h \
	include/Integration/Gen1/Gen1ExportTransaction.h src/UI/TrainerViewScreenBase.inc \
	src/UI/Gen1PokemonEditorOverlayFoundation.inc src/UI/Gen2TrainerSessionOverlay.inc src/UI/Gen1PokemonEditorOverlayUX.inc
HOST_TESTS += $(HOST_BUILD)/test_gen1_trainer_edit
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen1_trainer_edit_sanitize
host-test: $(HOST_BUILD)/test_gen1_trainer_edit
host-sanitize: $(HOST_BUILD)/test_gen1_trainer_edit_sanitize
$(HOST_BUILD)/test_gen1_trainer_edit: $(GEN1_TRAINER_EDIT_SOURCES) $(GEN1_TRAINER_EDIT_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN1_TRAINER_EDIT_SOURCES) -o $@
$(HOST_BUILD)/test_gen1_trainer_edit_sanitize: $(GEN1_TRAINER_EDIT_SOURCES) $(GEN1_TRAINER_EDIT_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN1_TRAINER_EDIT_SOURCES) -o $@

CLASSIC_STAT_FIELD_SOURCES := tests/test_classic_stat_field_state.cpp $(GEN2_STAGED_EDITOR_COMMON) \
	src/Integration/Gen1/Gen1StagedPokemonEditor.cpp src/Integration/Gen1/Gen1ReadOnlySave.cpp src/Integration/Gen1/Gen1BattleStats.cpp
CLASSIC_STAT_FIELD_DEPS := include/UI/ClassicStatFieldState.h include/UI/SharedFieldState.h \
	src/UI/Gen1PokemonEditorFoundationHardwareFix.inc src/UI/Gen1PokemonDetailsPresentation.cpp \
	src/UI/Gen2UnifiedPokemonWorkspace.inc src/UI/Gen2HardwareFinalFix.inc src/UI/Modals/Gen2PokemonDetailsModal.cpp \
	include/UI/ClassicFieldColor.h src/UI/Gen1PokemonEditorOverlayFoundation.inc src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc \
	src/UI/Gen2PokemonEditorFoundation.inc
HOST_TESTS += $(HOST_BUILD)/test_classic_stat_field_state
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_classic_stat_field_state_sanitize
host-test: $(HOST_BUILD)/test_classic_stat_field_state
host-sanitize: $(HOST_BUILD)/test_classic_stat_field_state_sanitize
$(HOST_BUILD)/test_classic_stat_field_state: $(CLASSIC_STAT_FIELD_SOURCES) $(CLASSIC_STAT_FIELD_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(CLASSIC_STAT_FIELD_SOURCES) -o $@
$(HOST_BUILD)/test_classic_stat_field_state_sanitize: $(CLASSIC_STAT_FIELD_SOURCES) $(CLASSIC_STAT_FIELD_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(CLASSIC_STAT_FIELD_SOURCES) -o $@

# R2-BUG-002: Gen III staged Trainer Name / Money (Ruby / Sapphire / Emerald / FireRed / LeafGreen)
# through the real verified Gen III export and strict reparse.
GEN3_TRAINER_EDIT_SOURCES := tests/test_gen3_trainer_edit.cpp \
	src/Integration/Gen3/Gen3StagedPokemonEditor.cpp \
	src/Integration/Gen3/Gen3ExportTransaction.cpp src/Integration/Common/VerifiedStagedExport.cpp src/Utils/SHA256.cpp \
	src/Integration/Gen3/Gen3StagedInventoryEditor.cpp src/Integration/Gen3/PKSMGen3NativeAdapter.cpp \
	src/Integration/Gen3/FRLGReadModel.cpp src/Pokemon/Pokemon3FRLG.cpp src/Pokemon/BaseStatsGen89.cpp \
	src/Pokemon/PersonalInfoTable.cpp src/Pokemon/Experience.cpp src/Encryption/Encryption3FRLG.cpp \
	src/Inventory/ClassicInventoryCatalog.cpp src/Inventory/ItemQuantityPolicy.cpp \
	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp \
	src/Names/ItemNames.cpp src/Names/MoveNames.cpp src/Names/MoveInfo.cpp \
	src/Names/SpeciesNames.cpp src/Names/NatureNames.cpp src/Names/AbilityNames.cpp src/Names/LocationNames.cpp \
	src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp
GEN3_TRAINER_EDIT_DEPS := include/Integration/Gen3/Gen3StagedTrainerEditor.h include/Integration/Gen3/Gen3ExportTransaction.h \
	include/UI/ClassicTrainerEditability.h
HOST_TESTS += $(HOST_BUILD)/test_gen3_trainer_edit
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen3_trainer_edit_sanitize
host-test: $(HOST_BUILD)/test_gen3_trainer_edit
host-sanitize: $(HOST_BUILD)/test_gen3_trainer_edit_sanitize
$(HOST_BUILD)/test_gen3_trainer_edit: $(GEN3_TRAINER_EDIT_SOURCES) $(GEN3_TRAINER_EDIT_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST -Iinclude $(GEN3_TRAINER_EDIT_SOURCES) -o $@
$(HOST_BUILD)/test_gen3_trainer_edit_sanitize: $(GEN3_TRAINER_EDIT_SOURCES) $(GEN3_TRAINER_EDIT_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST -Iinclude $(GEN3_TRAINER_EDIT_SOURCES) -o $@

# R2-BUG-008: Gen III Met Location picker keeps a stored origin the encounter templates do not cover.
GEN3_ENCOUNTER_KEEP_DEPS := include/UI/Gen3EncounterPickerModel.h include/Integration/Encounter/EncounterGuardrails.h \
	include/Integration/Encounter/EncounterGuardrailsData.inc
HOST_TESTS += $(HOST_BUILD)/test_gen3_encounter_keep_stored
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen3_encounter_keep_stored_sanitize
host-test: $(HOST_BUILD)/test_gen3_encounter_keep_stored
host-sanitize: $(HOST_BUILD)/test_gen3_encounter_keep_stored_sanitize
$(HOST_BUILD)/test_gen3_encounter_keep_stored: tests/test_gen3_encounter_keep_stored.cpp $(GEN3_ENCOUNTER_KEEP_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_gen3_encounter_keep_stored.cpp -o $@
$(HOST_BUILD)/test_gen3_encounter_keep_stored_sanitize: tests/test_gen3_encounter_keep_stored.cpp $(GEN3_ENCOUNTER_KEEP_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_gen3_encounter_keep_stored.cpp -o $@

# R2-CHECK-009: Gen I/II packed moves, including Gen I Box Actions -> Move, now in host-test (the
# focused CI workflow builds the same sources).
GEN12_PACKED_MOVE_SOURCES := tests/test_gen12_packed_move.cpp \
	src/Integration/Gen1/Gen1StagedPokemonEditor.cpp src/Integration/Gen1/Gen1PackedMove.cpp \
	src/Integration/Gen1/Gen1ReadOnlySave.cpp src/Integration/Gen2/Gen2StagedEditor.cpp \
	src/Integration/Gen2/Gen2PackedMove.cpp src/Integration/Gen2/Gen2ReadOnlySave.cpp \
	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp src/Integration/Gen2/Gen2PersonalData.cpp \
	src/Pokemon/Experience.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp \
	src/Names/SpeciesNames.cpp \
	src/Names/MoveNames.cpp
GEN12_PACKED_MOVE_DEPS := include/UI/Gen1PokemonEditorUIContract.h include/UI/NarrowPokemonActionMenu.h \
	src/UI/ClassicPackedMoveOverlay.inc src/UI/ClassicReleaseActionFix.inc \
	src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc src/UI/Gen1PokemonEditorOverlayUX.inc
HOST_TESTS += $(HOST_BUILD)/test_gen12_packed_move
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen12_packed_move_sanitize
host-test: $(HOST_BUILD)/test_gen12_packed_move
host-sanitize: $(HOST_BUILD)/test_gen12_packed_move_sanitize
$(HOST_BUILD)/test_gen12_packed_move: $(GEN12_PACKED_MOVE_SOURCES) $(GEN12_PACKED_MOVE_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -I. -Iinclude $(GEN12_PACKED_MOVE_SOURCES) -o $@
$(HOST_BUILD)/test_gen12_packed_move_sanitize: $(GEN12_PACKED_MOVE_SOURCES) $(GEN12_PACKED_MOVE_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -I. -Iinclude $(GEN12_PACKED_MOVE_SOURCES) -o $@

# NEW-04: Gen II PC box Move through the real GSCReadOnlyTrainer presentation, finalize and export.
GEN2_BOX_MOVE_SOURCES := tests/test_gen2_box_move_presentation.cpp \
	src/Legacy/GSCReadOnlyTrainer.cpp src/Pokemon/Pokemon2ReadOnly.cpp \
	src/Integration/Gen2/Gen2ReadOnlySave.cpp src/Integration/Gen2/Gen2ReadOnlyInventory.cpp \
	src/Integration/Gen2/Gen2StagedEditor.cpp src/Integration/Gen2/Gen2PackedMove.cpp \
	src/Integration/Gen2/Gen2ExportTransaction.cpp src/Utils/SHA256.cpp \
	src/Integration/Gen2/Gen2PersonalData.cpp src/Pokemon/Experience.cpp src/Pokemon/BaseStatsGen89.cpp \
	src/Pokemon/PersonalInfoTable.cpp src/Names/SpeciesNames.cpp src/Names/ItemNames.cpp \
	src/Names/NatureNames.cpp src/Names/AbilityNames.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp \
	src/Names/MoveNames.cpp
GEN2_BOX_MOVE_DEPS := include/Legacy/GSCReadOnlyTrainer.h include/Integration/Gen2/Gen2StagedEditor.h \
	tests/fixtures/gsc_pokemon_fixture.h
HOST_TESTS += $(HOST_BUILD)/test_gen2_box_move_presentation
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_box_move_presentation_sanitize
host-test: $(HOST_BUILD)/test_gen2_box_move_presentation
host-sanitize: $(HOST_BUILD)/test_gen2_box_move_presentation_sanitize
$(HOST_BUILD)/test_gen2_box_move_presentation: $(GEN2_BOX_MOVE_SOURCES) $(GEN2_BOX_MOVE_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -I. -Iinclude $(GEN2_BOX_MOVE_SOURCES) -o $@
$(HOST_BUILD)/test_gen2_box_move_presentation_sanitize: $(GEN2_BOX_MOVE_SOURCES) $(GEN2_BOX_MOVE_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -I. -Iinclude $(GEN2_BOX_MOVE_SOURCES) -o $@

# Round 5: Party edit -> staged exit -> Review Changes (real GSC adapter), Bag -> shared Review, swipe paging.
PARTY_REVIEW_SOURCES := tests/test_classic_party_review_path.cpp $(filter-out tests/test_gen2_box_move_presentation.cpp,$(GEN2_BOX_MOVE_SOURCES))
PARTY_REVIEW_DEPS := include/UI/ClassicReviewOrigin.h include/UI/ClassicStagedExit.h include/UI/SharedReviewTouch.h \
	include/UI/SharedReviewPresentation.h src/UI/ClassicInventoryOverlay.cpp $(GEN2_BOX_MOVE_DEPS)
HOST_TESTS += $(HOST_BUILD)/test_classic_party_review_path
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_classic_party_review_path_sanitize
host-test: $(HOST_BUILD)/test_classic_party_review_path
host-sanitize: $(HOST_BUILD)/test_classic_party_review_path_sanitize
$(HOST_BUILD)/test_classic_party_review_path: $(PARTY_REVIEW_SOURCES) $(PARTY_REVIEW_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -I. -Iinclude $(PARTY_REVIEW_SOURCES) -o $@
$(HOST_BUILD)/test_classic_party_review_path_sanitize: $(PARTY_REVIEW_SOURCES) $(PARTY_REVIEW_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -I. -Iinclude $(PARTY_REVIEW_SOURCES) -o $@

# Round 6 P2: Gen I-IV Trainer Info rows / layout in the Gen V design (no overlap, no invented gender).
CLASSIC_TRAINER_PAGE_DEPS := include/UI/ClassicTrainerPage.h include/UI/ClassicTrainerSession.h \
	include/Integration/Common/NativeBadges.h src/UI/TrainerViewScreenBase.inc src/UI/Gen2HardwareFinalFix.inc
HOST_TESTS += $(HOST_BUILD)/test_classic_trainer_page
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_classic_trainer_page_sanitize
host-test: $(HOST_BUILD)/test_classic_trainer_page
host-sanitize: $(HOST_BUILD)/test_classic_trainer_page_sanitize
$(HOST_BUILD)/test_classic_trainer_page: tests/test_classic_trainer_page.cpp $(CLASSIC_TRAINER_PAGE_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_classic_trainer_page.cpp -o $@
$(HOST_BUILD)/test_classic_trainer_page_sanitize: tests/test_classic_trainer_page.cpp $(CLASSIC_TRAINER_PAGE_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_classic_trainer_page.cpp -o $@
# ---- END original Makefile.host.gen14 ----

# ---- BEGIN original Makefile.host.qa ----
# Thread 4 application QA: parser fuzzing, export-safety contracts and known-failure repros.
#
# Opt-in targets only; nothing here is added to host-test / host-sanitize, so the default suite and
# its accepted test graph are unchanged.
#
#   make -f Makefile.host qa-fuzz-smoke        deterministic mutation runner, plain + ASan/UBSan
#   make -f Makefile.host qa-contracts         export-safety / source-immutability contracts (+ ASan/UBSan)
#   make -f Makefile.host qa-libfuzzer         clang libFuzzer binaries (CXX=clang++)
#   make -f Makefile.host qa-libfuzzer-run     time-boxed libFuzzer run (QA_FUZZ_SECONDS per target)
#   make -f Makefile.host qa-tools-test        tools/qa NRO audit regression + hardware round 1 gate
#   make -f Makefile.host qa-known-failures    reproductions of OPEN defects (expected to fail until fixed)
#   make -f Makefile.host qa                   fuzz smoke + contracts + tools (what CI gates on)
#
# Production sources are compiled once per toolchain variant into a static archive; each fuzz or
# contract binary links only what it uses. The native Gen III adapter is the one the Switch app
# compiles (POKEBANK_GEN3_SELECTIVE_PORT_TEST selects it on the host, as the existing suites do).

QA_TAG := $(if $(HOST_CXX_IS_CLANG),clang,gcc)
QA_BUILD := $(HOST_BUILD)/qa-$(QA_TAG)
QA_FLAGS := -Wno-unused-parameter -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST -Iinclude -Itests -Ivendor/PKSM-Core/include
QA_FUZZ_ITERATIONS ?= 25
QA_FUZZ_SECONDS ?= 60
QA_FUZZ_SMOKE_SECONDS ?= 45
QA_LIBFUZZER_FLAGS := -fsanitize=fuzzer,address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer -O1 -g

QA_CORE_SOURCES := $(sort \
	src/Integration/Common/VerifiedStagedExport.cpp \
	src/Integration/Gen1/Gen1StagedPokemonEditor.cpp src/Integration/Gen1/Gen1BattleStats.cpp \
	src/Integration/Gen1/Gen1ReadOnlySave.cpp src/Integration/Gen1/Gen1StagedInventoryEditor.cpp \
	src/Integration/Gen1/Gen1ReadOnlyInventory.cpp src/Integration/Gen1/Gen1ExportTransaction.cpp \
	src/Integration/Gen2/Gen2StagedEditor.cpp src/Integration/Gen2/Gen2UniversalInventory.cpp \
	src/Integration/Gen2/Gen2ReadOnlySave.cpp src/Integration/Gen2/Gen2ReadOnlyInventory.cpp \
	src/Integration/Gen2/Gen2PersonalData.cpp src/Integration/Gen2/Gen2ExportTransaction.cpp \
	src/Integration/Gen3/Gen3StagedPokemonEditor.cpp src/Integration/Gen3/Gen3ExportTransaction.cpp \
	src/Integration/Gen3/Gen3StagedInventoryEditor.cpp src/Integration/Gen3/PKSMGen3NativeAdapter.cpp \
	src/Integration/Gen3/FRLGReadModel.cpp \
	src/Integration/Gen4/Gen4ReadOnlySave.cpp src/Integration/Gen4/Gen4AssignedSource.cpp \
	src/Integration/Gen4/Gen4SourceDiscovery.cpp src/Integration/Gen4/Gen4StagedPokemonEditor.cpp \
	src/Integration/Gen4/Gen4ExportTransaction.cpp src/Names/MachineDisplay.cpp src/Names/TMMoves.cpp \
	src/Integration/Gen5/Gen5WorkingCopyExport.cpp src/Integration/Gen5/Gen5SourceDiscovery.cpp \
	src/Integration/Gen5/Gen5AssignedSource.cpp \
	src/Legacy/LegacySourceBindings.cpp src/Legacy/Gen5ReadOnlyTrainer.cpp src/Legacy/Gen4ReadOnlyTrainer.cpp \
	src/Legacy/RetroArchFRLGDiscovery.cpp src/Legacy/RetroArchRBYDiscovery.cpp src/Legacy/RetroArchGSCDiscovery.cpp \
	src/Legacy/FRLGSourceBrowser.cpp src/Legacy/FRLGReadOnlyTrainer.cpp \
	src/Integration/Gen1/Gen1MoveCompatibility.cpp src/Integration/Gen2/Gen2MoveCompatibility.cpp \
	src/Integration/Gen2/Gen2PackedMove.cpp \
	src/Games/GameIdentity.cpp \
	src/Pokemon/Pokemon3FRLG.cpp src/Pokemon/BaseStatsGen89.cpp src/Pokemon/PersonalInfoTable.cpp \
	src/Pokemon/Experience.cpp src/Pokemon/Pokemon4ReadOnly.cpp src/Pokemon/Pokemon4ReadOnlyView.cpp \
	src/Pokemon/PersonalRecord4.cpp src/Pokemon/PersonalInfo4DP.cpp src/Pokemon/PersonalInfo4PT.cpp \
	src/Pokemon/PersonalInfo4HGSS.cpp src/Pokemon/Pokemon4Mutable.cpp \
	src/Pokemon/Pokemon1ReadOnly.cpp src/Pokemon/Pokemon2ReadOnly.cpp \
	src/Encryption/Encryption3FRLG.cpp src/Encryption/Encryption4.cpp \
	src/Inventory/ClassicInventoryCatalog.cpp src/Inventory/ItemQuantityPolicy.cpp \
	src/Names/ItemNames.cpp src/Names/MoveNames.cpp src/Names/MoveInfo.cpp src/Names/SpeciesNames.cpp \
	src/Names/NatureNames.cpp src/Names/AbilityNames.cpp src/Names/LocationNames.cpp src/Names/ItemPresence.cpp \
	src/Names/MovePresence.cpp \
	src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp src/Utils/SHA256.cpp \
	vendor/PKSM-Core/source/utils/crypto_checksums.cpp)

QA_FUZZ_TARGETS := fuzz_gen1_sav fuzz_gen2_sav fuzz_gen3_sav fuzz_gen4_sav fuzz_gen5_sav \
	fuzz_pk_records fuzz_gen5_bag fuzz_source_containers fuzz_gen5_edits fuzz_launch_plan fuzz_home_cache
QA_HEADERS := tests/fuzz_support.h tests/fuzz_synthetic_saves.h tests/test_qa_support.h include/UI/GameLaunchModel.h runtime/return_host/source/return_host_policy.h \
	$(wildcard include/Integration/*/*.h include/Pokemon/*.h include/Encryption/*.h include/Utils/*.h include/Source/*.h)

# variant -> extra compile flags
QA_VARIANT_FLAGS_plain := $(CXXFLAGS)
QA_VARIANT_FLAGS_san := $(CXXFLAGS) $(SANITIZE_FLAGS)
QA_VARIANT_FLAGS_fuzz := $(filter-out -O2,$(CXXFLAGS)) -fsanitize=fuzzer-no-link,address,undefined -fno-omit-frame-pointer -O1 -g

define QA_VARIANT
QA_OBJS_$(1) := $$(patsubst %.cpp,$(QA_BUILD)/$(1)/obj/%.o,$(QA_CORE_SOURCES))
$(QA_BUILD)/$(1)/obj/%.o: %.cpp
	@mkdir -p $$(dir $$@)
	$$(CXX) $$(QA_VARIANT_FLAGS_$(1)) $$(QA_FLAGS) -MMD -MP -c $$< -o $$@
# Header dependencies: a header change must rebuild every object that includes it (a stale object
# archived into libqacore.a once produced an ASan new-delete-type-mismatch).
-include $$(QA_OBJS_$(1):.o=.d)
$(QA_BUILD)/$(1)/libqacore.a: $$(QA_OBJS_$(1))
	@rm -f $$@
	ar rcs $$@ $$^
endef
$(foreach v,plain san fuzz,$(eval $(call QA_VARIANT,$(v))))

# Deterministic runners (any compiler).
$(QA_BUILD)/plain/%: tests/%.cpp $(QA_BUILD)/plain/libqacore.a $(QA_HEADERS)
	$(CXX) $(QA_VARIANT_FLAGS_plain) $(QA_FLAGS) $< $(QA_BUILD)/plain/libqacore.a -o $@
$(QA_BUILD)/san/%: tests/%.cpp $(QA_BUILD)/san/libqacore.a $(QA_HEADERS)
	$(CXX) $(QA_VARIANT_FLAGS_san) $(QA_FLAGS) $< $(QA_BUILD)/san/libqacore.a -o $@
# libFuzzer binaries (clang only).
$(QA_BUILD)/fuzz/%: tests/%.cpp $(QA_BUILD)/fuzz/libqacore.a $(QA_HEADERS)
	$(CXX) $(filter-out -O2,$(CXXFLAGS)) $(QA_LIBFUZZER_FLAGS) -DPOKEBANK_QA_LIBFUZZER $(QA_FLAGS) $< $(QA_BUILD)/fuzz/libqacore.a -o $@

# The Gen III seed is the accepted pinned FR/LG fixture, generated by the existing host target.
QA_GEN3_FIXTURE := $(GEN3_FIXTURE)
export POKEBANK_QA_GEN3_FIXTURE := $(QA_GEN3_FIXTURE)

# Committed synthetic regression inputs (tests/fixtures/qa/corpus/<target>), replayed every run.
QA_REGRESSION = $$(test -d tests/fixtures/qa/corpus/$(1) && echo tests/fixtures/qa/corpus/$(1))

QA_FUZZ_PLAIN := $(addprefix $(QA_BUILD)/plain/,$(QA_FUZZ_TARGETS))
QA_FUZZ_SAN := $(addprefix $(QA_BUILD)/san/,$(QA_FUZZ_TARGETS))
QA_FUZZ_LIBFUZZER := $(addprefix $(QA_BUILD)/fuzz/,$(QA_FUZZ_TARGETS))

.PHONY: qa qa-fuzz-build qa-fuzz-smoke qa-fuzz-long qa-libfuzzer qa-libfuzzer-run qa-contracts \
	qa-known-failures qa-tools-test qa-seeds qa-matrix qa-clean

qa-fuzz-build: $(QA_FUZZ_PLAIN) $(QA_FUZZ_SAN) $(QA_GEN3_FIXTURE)

# Every target, both builds, every strategy, with and without checksum forging.
qa-fuzz-smoke: qa-fuzz-build
	@set -e; for t in $(QA_FUZZ_TARGETS); do \
		$(QA_BUILD)/plain/$$t --iterations $(QA_FUZZ_ITERATIONS) --seconds $(QA_FUZZ_SMOKE_SECONDS) $(call QA_REGRESSION,$$t); \
		ASAN_OPTIONS=$(ASAN_OPTIONS) UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
			$(QA_BUILD)/san/$$t --iterations $(QA_FUZZ_ITERATIONS) --seconds $(QA_FUZZ_SMOKE_SECONDS) $(call QA_REGRESSION,$$t); \
	done

# Longer deterministic soak (nightly): more rounds and a different RNG stream per run.
QA_FUZZ_RNG ?= 0x5EED0C7A11
qa-fuzz-long: qa-fuzz-build
	@set -e; for t in $(QA_FUZZ_TARGETS); do \
		ASAN_OPTIONS=$(ASAN_OPTIONS) UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
			$(QA_BUILD)/san/$$t --iterations 100000 --seconds $(QA_FUZZ_SECONDS) --rng-seed $(QA_FUZZ_RNG) $(call QA_REGRESSION,$$t); \
	done

qa-libfuzzer: $(QA_FUZZ_LIBFUZZER)

# Seed corpora for libFuzzer come from the same synthetic builders.
qa-seeds: $(QA_FUZZ_PLAIN) $(QA_GEN3_FIXTURE)
	@set -e; for t in $(QA_FUZZ_TARGETS); do \
		$(QA_BUILD)/plain/$$t --write-seeds $(QA_BUILD)/corpus/$$t; \
	done

qa-libfuzzer-run: qa-libfuzzer qa-seeds
	@set -e; mkdir -p $(QA_BUILD)/artifacts; for t in $(QA_FUZZ_TARGETS); do \
		echo "== libFuzzer $$t ($(QA_FUZZ_SECONDS)s)"; \
		ASAN_OPTIONS=$(ASAN_OPTIONS) UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 \
			$(QA_BUILD)/fuzz/$$t -max_total_time=$(QA_FUZZ_SECONDS) -max_len=600000 -rss_limit_mb=4096 \
			-timeout=30 -print_final_stats=1 -artifact_prefix=$(QA_BUILD)/artifacts/$$t- \
			$(QA_BUILD)/corpus/$$t $(call QA_REGRESSION,$$t); \
	done

QA_CONTRACT_TESTS := test_qa_export_contracts test_qa_regressions test_native_trainer_badges
qa-contracts: $(addprefix $(QA_BUILD)/plain/,$(QA_CONTRACT_TESTS)) $(addprefix $(QA_BUILD)/san/,$(QA_CONTRACT_TESTS)) $(QA_GEN3_FIXTURE)
	@set -e; for t in $(QA_CONTRACT_TESTS); do \
		$(QA_BUILD)/plain/$$t; \
		ASAN_OPTIONS=$(ASAN_OPTIONS) UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 $(QA_BUILD)/san/$$t; \
	done

# Gen I-V acceptance matrix: 20 versions x 26 cells (5 round 2 cells, some per-generation) on real
# production paths (docs/qa/GEN1_5_MATRIX.md).
# Fails on any FAIL / MISSING / DUPLICATE cell; Known defects are reproduced in qa-known-failures.
QA_MATRIX_SOURCES := tests/test_qa_matrix.cpp tests/test_qa_matrix_gen12.cpp tests/test_qa_matrix_gen3.cpp \
	tests/test_qa_matrix_gen4.cpp tests/test_qa_matrix_gen5.cpp
QA_MATRIX_EXTRA_SOURCES := src/Legacy/RBYReadOnlyTrainer.cpp src/UI/ClassicInventoryUIModel.cpp \
    src/Legacy/BackupReadOnlyTrainer.cpp src/Legacy/GSCReadOnlyTrainer.cpp tests/test_backup_session_trace.cpp
$(QA_BUILD)/plain/test_qa_matrix: $(QA_MATRIX_SOURCES) $(QA_MATRIX_EXTRA_SOURCES) tests/test_qa_matrix.h tests/test_qa_matrix_round2.h \
		$(QA_BUILD)/plain/libqacore.a $(QA_HEADERS)
	$(CXX) $(QA_VARIANT_FLAGS_plain) $(QA_FLAGS) $(QA_MATRIX_SOURCES) $(QA_MATRIX_EXTRA_SOURCES) \
		$(QA_BUILD)/plain/libqacore.a -o $@
$(QA_BUILD)/san/test_qa_matrix: $(QA_MATRIX_SOURCES) $(QA_MATRIX_EXTRA_SOURCES) tests/test_qa_matrix.h tests/test_qa_matrix_round2.h \
		$(QA_BUILD)/san/libqacore.a $(QA_HEADERS)
	$(CXX) $(QA_VARIANT_FLAGS_san) $(QA_FLAGS) $(QA_MATRIX_SOURCES) $(QA_MATRIX_EXTRA_SOURCES) \
		$(QA_BUILD)/san/libqacore.a -o $@
qa-matrix: $(QA_BUILD)/plain/test_qa_matrix $(QA_BUILD)/san/test_qa_matrix $(QA_GEN3_FIXTURE)
	$(QA_BUILD)/plain/test_qa_matrix --markdown $(QA_BUILD)/GEN1_5_MATRIX.generated.md >/dev/null
	ASAN_OPTIONS=$(ASAN_OPTIONS) UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 $(QA_BUILD)/san/test_qa_matrix >/dev/null

# Open-defect reproductions. Kept OUT of qa / host-test so CI stays honest; each case exits
# non-zero while its defect is open. See docs/qa/FINDINGS.md.
# Fixed findings move to test_qa_regressions(.cpp|_assets.py), which run inside the qa gate.
qa-known-failures: $(QA_BUILD)/san/test_qa_known_failures
	@status=0; \
	ASAN_OPTIONS=$(ASAN_OPTIONS) $(QA_BUILD)/san/test_qa_known_failures || status=1; \
	python3 tests/test_qa_known_failures_assets.py || status=1; \
	exit $$status

qa-tools-test:
	python3 tests/test_qa_nro_audit.py
	python3 tests/test_qa_regressions_assets.py
	python3 tests/test_hw_round1_regression_gate.py
	python3 tests/test_qa_hw_round1_gate_mutations.py
	python3 tests/test_qa_round5_contract_mutations.py
	python3 tests/test_qa_make_retest.py
	python3 tests/test_qa_manual_workflow_staleness.py
	python3 tests/test_qa_host_header_deps.py
	python3 tools/qa/manual_workflow_staleness.py

qa: qa-fuzz-smoke qa-contracts qa-matrix qa-tools-test

qa-clean:
	rm -rf $(QA_BUILD)
# ---- END original Makefile.host.qa ----

# ---- BEGIN original Makefile.host.item-quantity-policy ----
# Thread I: generation/game-aware item quantity policy (pure query API, no UI).
ITEM_QUANTITY_POLICY_TEST := $(HOST_BUILD)/test_item_quantity_policy
ITEM_QUANTITY_POLICY_SANITIZE := $(HOST_BUILD)/test_item_quantity_policy_sanitize
ITEM_QUANTITY_POLICY_SOURCES := tests/test_item_quantity_policy.cpp \
    src/Inventory/ItemQuantityPolicy.cpp src/Inventory/ClassicInventoryCatalog.cpp \
    src/Names/ItemNames.cpp src/Names/MoveNames.cpp src/Integration/Gen2/Gen2ReadOnlyInventory.cpp
ITEM_QUANTITY_POLICY_INPUTS := $(ITEM_QUANTITY_POLICY_SOURCES) \
    include/Inventory/ItemQuantityPolicy.h include/Inventory/ClassicInventoryCatalog.h \
    include/Integration/Gen4/Gen4BagCatalog.h include/Integration/Gen5/Gen5Bag.h

HOST_TESTS += $(ITEM_QUANTITY_POLICY_TEST)
HOST_SANITIZE_TESTS += $(ITEM_QUANTITY_POLICY_SANITIZE)
host-test: $(ITEM_QUANTITY_POLICY_TEST)
host-sanitize: $(ITEM_QUANTITY_POLICY_SANITIZE)

.PHONY: host-item-quantity-policy
host-item-quantity-policy: $(ITEM_QUANTITY_POLICY_TEST)
	./$(ITEM_QUANTITY_POLICY_TEST)

$(ITEM_QUANTITY_POLICY_TEST): $(ITEM_QUANTITY_POLICY_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(ITEM_QUANTITY_POLICY_SOURCES) -o $@

$(ITEM_QUANTITY_POLICY_SANITIZE): $(ITEM_QUANTITY_POLICY_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(ITEM_QUANTITY_POLICY_SOURCES) -o $@

# Round 2 B01-B06/B04: the ONE shared Gen I-V Bag Add flow against every real staged writer.
BAG_ADD_FLOW_TEST := $(HOST_BUILD)/test_bag_add_flow_gen15
BAG_ADD_FLOW_SANITIZE := $(HOST_BUILD)/test_bag_add_flow_gen15_sanitize
BAG_ADD_FLOW_SOURCES := tests/test_bag_add_flow_gen15.cpp \
    src/Inventory/ItemQuantityPolicy.cpp src/Inventory/ClassicInventoryCatalog.cpp \
    src/Names/ItemNames.cpp src/Names/MoveNames.cpp src/Names/SpeciesNames.cpp \
    src/Integration/Gen1/Gen1ReadOnlyInventory.cpp src/Integration/Gen1/Gen1ReadOnlySave.cpp \
    src/Integration/Gen1/Gen1StagedInventoryEditor.cpp \
    src/Integration/Gen2/Gen2StagedEditor.cpp src/Integration/Gen2/Gen2UniversalInventory.cpp \
    src/Integration/Gen2/Gen2ReadOnlySave.cpp src/Integration/Gen2/Gen2ReadOnlyInventory.cpp \
    src/Integration/Gen2/Gen2PersonalData.cpp src/Pokemon/Experience.cpp \
    src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp \
    src/Integration/Gen3/Gen3StagedInventoryEditor.cpp \
    src/Legacy/Gen5ReadOnlyTrainer.cpp
BAG_ADD_FLOW_INPUTS := $(BAG_ADD_FLOW_SOURCES) include/UI/SharedBagAddFlow.h \
    include/Inventory/ItemQuantityPolicy.h include/UI/Gen4BagQuantity.h \
    include/Integration/Gen4/Gen4BagCatalog.h include/Integration/Gen5/Gen5BagScreen.h \
    include/Integration/Gen5/Gen5Bag.h include/UI/SharedMenuGeometry.h

HOST_TESTS += $(BAG_ADD_FLOW_TEST)
HOST_SANITIZE_TESTS += $(BAG_ADD_FLOW_SANITIZE)
host-test: $(BAG_ADD_FLOW_TEST)
host-sanitize: $(BAG_ADD_FLOW_SANITIZE)

.PHONY: host-bag-add-flow
host-bag-add-flow: $(BAG_ADD_FLOW_TEST)
	./$(BAG_ADD_FLOW_TEST)

$(BAG_ADD_FLOW_TEST): $(BAG_ADD_FLOW_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(BAG_ADD_FLOW_SOURCES) -o $@

$(BAG_ADD_FLOW_SANITIZE): $(BAG_ADD_FLOW_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(BAG_ADD_FLOW_SOURCES) -o $@

# Round 3 R2-BUG-005: per-game TM/HM -> move table and that generation's move facts (Lane P renders).
MACHINE_MOVES_TEST := $(HOST_BUILD)/test_machine_moves_gen15
MACHINE_MOVES_SANITIZE := $(HOST_BUILD)/test_machine_moves_gen15_sanitize
MACHINE_MOVES_SOURCES := tests/test_machine_moves_gen15.cpp src/Inventory/MachineMoves.cpp \
    src/Inventory/ItemQuantityPolicy.cpp src/Inventory/ClassicInventoryCatalog.cpp \
    src/Names/ItemNames.cpp src/Names/MoveNames.cpp src/Names/TMMoves.cpp src/Names/MachineDisplay.cpp \
    src/Integration/Gen2/Gen2ReadOnlyInventory.cpp
MACHINE_MOVES_INPUTS := $(MACHINE_MOVES_SOURCES) include/Inventory/MachineMoves.h \
    include/Names/MoveBattleStats.h include/Names/MoveBattleStats5.h include/Names/MoveTypes34.h \
    include/Names/TMMoves.h include/Names/MachineDisplay.h include/UI/ClassicMachineArtwork.h \
    include/UI/ItemSpriteKey.h tools/pinned_item_sprite_names.txt

HOST_TESTS += $(MACHINE_MOVES_TEST)
HOST_SANITIZE_TESTS += $(MACHINE_MOVES_SANITIZE)
host-test: $(MACHINE_MOVES_TEST)
host-sanitize: $(MACHINE_MOVES_SANITIZE)

$(MACHINE_MOVES_TEST): $(MACHINE_MOVES_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(MACHINE_MOVES_SOURCES) -o $@

$(MACHINE_MOVES_SANITIZE): $(MACHINE_MOVES_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(MACHINE_MOVES_SOURCES) -o $@
# ---- END original Makefile.host.item-quantity-policy ----

# ---- BEGIN original Makefile.host.pickers ----
# Lane P: shared draggable scrollbar component and live species search (header-only, host fakes).
SHARED_SCROLLBAR_TEST := $(HOST_BUILD)/test_shared_scrollbar
SHARED_SCROLLBAR_SANITIZE := $(HOST_BUILD)/test_shared_scrollbar_sanitize
SHARED_SCROLLBAR_SOURCES := tests/test_shared_scrollbar.cpp
SHARED_SCROLLBAR_INPUTS := $(SHARED_SCROLLBAR_SOURCES) include/UI/SharedScrollbar.h \
    tests/touch_fakes/UI/TouchInput.h
SPECIES_SEARCH_TEST := $(HOST_BUILD)/test_species_search_gen15
SPECIES_SEARCH_SANITIZE := $(HOST_BUILD)/test_species_search_gen15_sanitize
SPECIES_SEARCH_SOURCES := tests/test_species_search_gen15.cpp src/Names/SpeciesNames.cpp
SPECIES_SEARCH_INPUTS := $(SPECIES_SEARCH_SOURCES) include/UI/SharedSpeciesPicker.h \
    include/UI/SharedSearchInput.h include/UI/SharedScrollbar.h include/UI/SharedMenuGeometry.h

MOVE_TYPE_BADGES_TEST := $(HOST_BUILD)/test_move_type_badges
MOVE_TYPE_BADGES_SANITIZE := $(HOST_BUILD)/test_move_type_badges_sanitize
MOVE_TYPE_BADGES_SOURCES := tests/test_move_type_badges.cpp src/Names/MoveNames.cpp
MOVE_TYPE_BADGES_INPUTS := $(MOVE_TYPE_BADGES_SOURCES) include/UI/ClassicTypeBadges.h \
    include/UI/SharedMovesPanel.h include/UI/SharedMoveFacts.h

ITEM_GRID_TEST := $(HOST_BUILD)/test_item_grid_picker
ITEM_GRID_SANITIZE := $(HOST_BUILD)/test_item_grid_picker_sanitize
ITEM_GRID_SOURCES := tests/test_item_grid_picker.cpp src/Names/MoveNames.cpp
ITEM_GRID_INPUTS := $(ITEM_GRID_SOURCES) include/UI/SharedItemGridPicker.h include/UI/ClassicTypeBadges.h \
    include/UI/SharedMenuGeometry.h include/UI/SharedMoveFacts.h include/UI/SharedScrollbar.h

# NEW-06: Held Item search (ItemPickerSearch.h) and its Gen II / III / IV-V wiring.
ITEM_SEARCH_TEST := $(HOST_BUILD)/test_item_picker_search
ITEM_SEARCH_SANITIZE := $(HOST_BUILD)/test_item_picker_search_sanitize
ITEM_SEARCH_INPUTS := tests/test_item_picker_search.cpp include/UI/ItemPickerSearch.h include/UI/ItemPickerSort.h \
    src/UI/Gen2PokemonEditorFoundation.inc src/UI/Gen3SharedPokemonSurface.inc \
    src/UI/SharedPokemonEditorSurface.inc include/UI/SharedEditorPresentation.h

# NEW-13: Ability names + short effects (AbilityEffects.h) and the Gen III / IV / V wiring.
ABILITY_EFFECTS_TEST := $(HOST_BUILD)/test_ability_effects
ABILITY_EFFECTS_SANITIZE := $(HOST_BUILD)/test_ability_effects_sanitize
ABILITY_EFFECTS_SOURCES := tests/test_ability_effects.cpp src/Names/AbilityNames.cpp
ABILITY_EFFECTS_INPUTS := $(ABILITY_EFFECTS_SOURCES) include/UI/AbilityEffects.h include/UI/SharedMenuGeometry.h \
    src/UI/Gen3SharedPokemonSurface.inc src/UI/Gen4SharedPokemonSurface.inc \
    src/UI/Gen5SharedReadOnlySurface.inc src/UI/SharedPokemonEditorSurface.inc

# Round 5 NEW-10: every Gen III-V Met Location row fits the compact 620 px window unclipped.
MET_LOCATION_FIT_TEST := $(HOST_BUILD)/test_met_location_fit
MET_LOCATION_FIT_SANITIZE := $(HOST_BUILD)/test_met_location_fit_sanitize
MET_LOCATION_FIT_SOURCES := tests/test_met_location_fit.cpp src/Names/LocationNames.cpp src/Names/SpeciesNames.cpp
MET_LOCATION_FIT_INPUTS := $(MET_LOCATION_FIT_SOURCES) include/UI/SharedMenuGeometry.h \
    include/Integration/Encounter/EncounterContext45.h include/Integration/Encounter/EncounterContext45Data.inc \
    include/Integration/Encounter/EncounterGuardrails.h include/Names/LocationDisplayNames.h \
    src/UI/SharedPokemonEditorSurface.inc src/UI/Gen3SharedPokemonSurface.inc

# Round 5 4D: Gen I-V Help pages name only working controls.
HELP_ACCURACY_TEST := $(HOST_BUILD)/test_classic_help_accuracy
HELP_ACCURACY_SANITIZE := $(HOST_BUILD)/test_classic_help_accuracy_sanitize
HELP_ACCURACY_INPUTS := tests/test_classic_help_accuracy.cpp include/UI/ClassicHelpPresentation.h \
    src/UI/Gen5SharedReadOnlySurface.inc src/UI/ClassicInventoryOverlay.cpp src/UI/TrainerViewScreenBase.inc \
    include/UI/ClassicTrainerSessionInput.h src/UI/ClassicPackedMoveOverlay.inc

# Round 5 6B: Gen IV / V event-year labels only where the native PKHeX name carries the year.
EVENT_YEAR_TEST := $(HOST_BUILD)/test_event_year_source
EVENT_YEAR_SANITIZE := $(HOST_BUILD)/test_event_year_source_sanitize
EVENT_YEAR_SOURCES := tests/test_event_year_source.cpp src/Names/LocationNames.cpp
EVENT_YEAR_INPUTS := $(EVENT_YEAR_SOURCES) include/Names/LocationDisplayNames.h include/Names/LocationNames.h

# Round 6 L06: Gen II-V Held Item grid input (tap and A commit the same item; taps never lost).
HELD_ITEM_INPUT_TEST := $(HOST_BUILD)/test_held_item_input
HELD_ITEM_INPUT_SANITIZE := $(HOST_BUILD)/test_held_item_input_sanitize
HELD_ITEM_INPUT_INPUTS := tests/test_held_item_input.cpp include/UI/SharedHeldItemInput.h \
    include/UI/SharedHeldItemPicker.h include/UI/TouchScroll.h include/UI/TouchGesture.h \
    include/UI/ItemPickerSearch.h include/UI/ItemPickerSort.h tests/touch_fakes/UI/TouchInput.h \
    src/UI/Gen2PokemonEditorFoundation.inc src/UI/Gen3SharedPokemonSurface.inc \
    src/UI/SharedPokemonEditorSurface.inc include/UI/Gen2PokemonEditorRules.h \
    include/Integration/Gen5/Gen5EditorFields.h

# Round 6 7D: tap outside a non-destructive editor picker closes it like B, with no tap-through.
POPUP_DISMISS_TEST := $(HOST_BUILD)/test_popup_dismiss
POPUP_DISMISS_SANITIZE := $(HOST_BUILD)/test_popup_dismiss_sanitize
POPUP_DISMISS_INPUTS := tests/test_popup_dismiss.cpp include/UI/SharedPopupDismiss.h \
    include/UI/SharedEditorChrome.h tests/touch_fakes/UI/TouchInput.h \
    src/UI/Gen2PokemonEditorFoundation.inc src/UI/Gen3SharedPokemonSurface.inc \
    src/UI/SharedPokemonEditorSurface.inc

# Round 6 P5: no two identical rows in any Gen IV / V Met Location list (POKÉPARK / Festa / twins).
MET_ROWS_DISTINCT_TEST := $(HOST_BUILD)/test_met_location_rows_distinct
MET_ROWS_DISTINCT_SANITIZE := $(HOST_BUILD)/test_met_location_rows_distinct_sanitize
MET_ROWS_DISTINCT_SOURCES := tests/test_met_location_rows_distinct.cpp src/Names/LocationNames.cpp src/Names/SpeciesNames.cpp
MET_ROWS_DISTINCT_INPUTS := $(MET_ROWS_DISTINCT_SOURCES) include/Names/LocationDisplayNames.h \
    include/Integration/Encounter/EncounterContext45.h include/Integration/Encounter/EncounterContext45Data.inc \
    include/Integration/Gen5/Gen5EncounterLocations.h src/UI/Gen4SharedPokemonSurface.inc \
    src/UI/Gen5SharedReadOnlySurface.inc src/UI/SharedPokemonEditorSurface.inc

# Round 7 B05: Gen IV / V event, transfer, trade and egg met locations say what pinned tables show.
SPECIAL_MET_NOTES_TEST := $(HOST_BUILD)/test_special_met_location_notes
SPECIAL_MET_NOTES_SANITIZE := $(HOST_BUILD)/test_special_met_location_notes_sanitize
SPECIAL_MET_NOTES_SOURCES := tests/test_special_met_location_notes.cpp src/Names/LocationNames.cpp
SPECIAL_MET_NOTES_INPUTS := $(SPECIAL_MET_NOTES_SOURCES) include/Names/SpecialMetLocationNotes.h \
    include/Names/LocationDisplayNames.h include/Integration/Encounter/EncounterContext45.h \
    include/UI/SharedMenuGeometry.h src/UI/SharedPokemonEditorSurface.inc

# tests/test_touch_scroll_state.cpp (TouchScroll.h, Lane P) was only built by the touch-input
# workflow; QA Part 9 found no host Makefile ran it. It runs in host-test / host-sanitize here.
TOUCH_SCROLL_STATE_TEST := $(HOST_BUILD)/test_touch_scroll_state
TOUCH_SCROLL_STATE_SANITIZE := $(HOST_BUILD)/test_touch_scroll_state_sanitize
TOUCH_SCROLL_STATE_INPUTS := tests/test_touch_scroll_state.cpp include/UI/TouchScroll.h \
    include/UI/TouchGesture.h tests/touch_fakes/UI/TouchInput.h

HOST_TESTS += $(SHARED_SCROLLBAR_TEST) $(SPECIES_SEARCH_TEST) $(MOVE_TYPE_BADGES_TEST) $(ITEM_GRID_TEST) $(TOUCH_SCROLL_STATE_TEST) $(ITEM_SEARCH_TEST) $(ABILITY_EFFECTS_TEST) $(MET_LOCATION_FIT_TEST) $(HELP_ACCURACY_TEST) $(EVENT_YEAR_TEST) $(HELD_ITEM_INPUT_TEST) $(POPUP_DISMISS_TEST) $(MET_ROWS_DISTINCT_TEST) $(SPECIAL_MET_NOTES_TEST)
HOST_SANITIZE_TESTS += $(SHARED_SCROLLBAR_SANITIZE) $(SPECIES_SEARCH_SANITIZE) $(MOVE_TYPE_BADGES_SANITIZE) $(ITEM_GRID_SANITIZE) $(TOUCH_SCROLL_STATE_SANITIZE) $(ITEM_SEARCH_SANITIZE) $(ABILITY_EFFECTS_SANITIZE) $(MET_LOCATION_FIT_SANITIZE) $(HELP_ACCURACY_SANITIZE) $(EVENT_YEAR_SANITIZE) $(HELD_ITEM_INPUT_SANITIZE) $(POPUP_DISMISS_SANITIZE) $(MET_ROWS_DISTINCT_SANITIZE) $(SPECIAL_MET_NOTES_SANITIZE)
host-test: $(SHARED_SCROLLBAR_TEST) $(SPECIES_SEARCH_TEST) $(MOVE_TYPE_BADGES_TEST) $(ITEM_GRID_TEST) $(TOUCH_SCROLL_STATE_TEST) $(ITEM_SEARCH_TEST) $(ABILITY_EFFECTS_TEST) $(MET_LOCATION_FIT_TEST) $(HELP_ACCURACY_TEST) $(EVENT_YEAR_TEST) $(HELD_ITEM_INPUT_TEST) $(POPUP_DISMISS_TEST) $(MET_ROWS_DISTINCT_TEST) $(SPECIAL_MET_NOTES_TEST)
host-sanitize: $(SHARED_SCROLLBAR_SANITIZE) $(SPECIES_SEARCH_SANITIZE) $(MOVE_TYPE_BADGES_SANITIZE) $(ITEM_GRID_SANITIZE) $(TOUCH_SCROLL_STATE_SANITIZE) $(ITEM_SEARCH_SANITIZE) $(ABILITY_EFFECTS_SANITIZE) $(MET_LOCATION_FIT_SANITIZE) $(HELP_ACCURACY_SANITIZE) $(EVENT_YEAR_SANITIZE) $(HELD_ITEM_INPUT_SANITIZE) $(POPUP_DISMISS_SANITIZE) $(MET_ROWS_DISTINCT_SANITIZE) $(SPECIAL_MET_NOTES_SANITIZE)

$(ITEM_SEARCH_TEST): $(ITEM_SEARCH_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_item_picker_search.cpp -o $@

$(ITEM_SEARCH_SANITIZE): $(ITEM_SEARCH_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_item_picker_search.cpp -o $@

$(ABILITY_EFFECTS_TEST): $(ABILITY_EFFECTS_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(ABILITY_EFFECTS_SOURCES) -o $@

$(ABILITY_EFFECTS_SANITIZE): $(ABILITY_EFFECTS_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(ABILITY_EFFECTS_SOURCES) -o $@

$(MET_LOCATION_FIT_TEST): $(MET_LOCATION_FIT_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(MET_LOCATION_FIT_SOURCES) -o $@

$(MET_LOCATION_FIT_SANITIZE): $(MET_LOCATION_FIT_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(MET_LOCATION_FIT_SOURCES) -o $@

$(HELP_ACCURACY_TEST): $(HELP_ACCURACY_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_classic_help_accuracy.cpp -o $@

$(HELP_ACCURACY_SANITIZE): $(HELP_ACCURACY_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_classic_help_accuracy.cpp -o $@

$(EVENT_YEAR_TEST): $(EVENT_YEAR_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(EVENT_YEAR_SOURCES) -o $@

$(EVENT_YEAR_SANITIZE): $(EVENT_YEAR_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(EVENT_YEAR_SOURCES) -o $@

$(TOUCH_SCROLL_STATE_TEST): $(TOUCH_SCROLL_STATE_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Itests/touch_fakes -Iinclude tests/test_touch_scroll_state.cpp -o $@

$(TOUCH_SCROLL_STATE_SANITIZE): $(TOUCH_SCROLL_STATE_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Itests/touch_fakes -Iinclude tests/test_touch_scroll_state.cpp -o $@

$(ITEM_GRID_TEST): $(ITEM_GRID_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(ITEM_GRID_SOURCES) -o $@

$(ITEM_GRID_SANITIZE): $(ITEM_GRID_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(ITEM_GRID_SOURCES) -o $@

$(MOVE_TYPE_BADGES_TEST): $(MOVE_TYPE_BADGES_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(MOVE_TYPE_BADGES_SOURCES) -o $@

$(MOVE_TYPE_BADGES_SANITIZE): $(MOVE_TYPE_BADGES_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(MOVE_TYPE_BADGES_SOURCES) -o $@

.PHONY: host-pickers
host-pickers: $(SHARED_SCROLLBAR_TEST) $(SPECIES_SEARCH_TEST)
	./$(SHARED_SCROLLBAR_TEST)
	./$(SPECIES_SEARCH_TEST)
	./$(MOVE_TYPE_BADGES_TEST)
	./$(ITEM_GRID_TEST)
	./$(TOUCH_SCROLL_STATE_TEST)
	./$(ITEM_SEARCH_TEST)
	./$(ABILITY_EFFECTS_TEST)

$(SPECIES_SEARCH_TEST): $(SPECIES_SEARCH_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(SPECIES_SEARCH_SOURCES) -o $@

$(SPECIES_SEARCH_SANITIZE): $(SPECIES_SEARCH_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(SPECIES_SEARCH_SOURCES) -o $@

$(SHARED_SCROLLBAR_TEST): $(SHARED_SCROLLBAR_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Itests/touch_fakes -Iinclude $(SHARED_SCROLLBAR_SOURCES) -o $@

$(SHARED_SCROLLBAR_SANITIZE): $(SHARED_SCROLLBAR_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Itests/touch_fakes -Iinclude $(SHARED_SCROLLBAR_SOURCES) -o $@

$(HELD_ITEM_INPUT_TEST): $(HELD_ITEM_INPUT_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Itests/touch_fakes -Iinclude tests/test_held_item_input.cpp -o $@

$(HELD_ITEM_INPUT_SANITIZE): $(HELD_ITEM_INPUT_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Itests/touch_fakes -Iinclude tests/test_held_item_input.cpp -o $@

$(POPUP_DISMISS_TEST): $(POPUP_DISMISS_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Itests/touch_fakes -Iinclude tests/test_popup_dismiss.cpp -o $@

$(POPUP_DISMISS_SANITIZE): $(POPUP_DISMISS_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Itests/touch_fakes -Iinclude tests/test_popup_dismiss.cpp -o $@

$(MET_ROWS_DISTINCT_TEST): $(MET_ROWS_DISTINCT_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(MET_ROWS_DISTINCT_SOURCES) -o $@

$(MET_ROWS_DISTINCT_SANITIZE): $(MET_ROWS_DISTINCT_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(MET_ROWS_DISTINCT_SOURCES) -o $@

$(SPECIAL_MET_NOTES_TEST): $(SPECIAL_MET_NOTES_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(SPECIAL_MET_NOTES_SOURCES) -o $@

$(SPECIAL_MET_NOTES_SANITIZE): $(SPECIAL_MET_NOTES_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(SPECIAL_MET_NOTES_SOURCES) -o $@
# ---- END original Makefile.host.pickers ----

# ---- BEGIN original Makefile.host.reroll-mechanics ----
# Thread R (phase 10 / owner E11): shiny and non-shiny re-roll mechanics, Gen I-V, through the
# real DV / PID / record code. Shininess never yields to another edit; DV and PID links are kept.
REROLL_MECHANICS_TEST := $(HOST_BUILD)/test_reroll_mechanics_gen15
REROLL_MECHANICS_SANITIZE := $(HOST_BUILD)/test_reroll_mechanics_gen15_sanitize
REROLL_MECHANICS_SOURCES := tests/test_reroll_mechanics_gen15.cpp \
	src/Integration/Gen2/Gen2PersonalData.cpp \
	src/Pokemon/Pokemon3FRLG.cpp \
	src/Encryption/Encryption3FRLG.cpp \
	src/Pokemon/BaseStatsGen89.cpp \
	src/Pokemon/PersonalInfoTable.cpp \
	src/Pokemon/Experience.cpp \
	src/Pokemon/Pokemon4Mutable.cpp \
	src/Pokemon/Pokemon4ReadOnly.cpp \
	src/Pokemon/PersonalRecord4.cpp \
	src/Pokemon/PersonalInfo4DP.cpp \
	src/Pokemon/PersonalInfo4PT.cpp \
	src/Pokemon/PersonalInfo4HGSS.cpp \
	src/Encryption/Encryption4.cpp \
	src/Names/SpeciesNames.cpp \
	src/Names/ItemPresence.cpp \
	src/Names/MoveInfo.cpp \
	src/Names/MovePresence.cpp \
	src/Names/ItemNames.cpp \
	src/Names/NatureNames.cpp \
	src/Names/AbilityNames.cpp \
	src/Utils/StringHelpers.cpp \
	src/Utils/HelperUtilities.cpp
REROLL_MECHANICS_INPUTS := $(REROLL_MECHANICS_SOURCES) \
	include/Conversion/Gen3PidSearch.h include/UI/Gen12DVReroll.h include/UI/SharedStatReroll.h \
	include/Integration/Gen1/Gen1Shiny.h include/Integration/Gen5/Gen5PidCoordination.h

HOST_TESTS += $(REROLL_MECHANICS_TEST)
HOST_SANITIZE_TESTS += $(REROLL_MECHANICS_SANITIZE)
host-test: $(REROLL_MECHANICS_TEST)
host-sanitize: $(REROLL_MECHANICS_SANITIZE)

.PHONY: host-reroll-mechanics
host-reroll-mechanics: $(REROLL_MECHANICS_TEST)
	./$(REROLL_MECHANICS_TEST)

$(REROLL_MECHANICS_TEST): $(REROLL_MECHANICS_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(REROLL_MECHANICS_SOURCES) -o $@

$(REROLL_MECHANICS_SANITIZE): $(REROLL_MECHANICS_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(REROLL_MECHANICS_SOURCES) -o $@
# ---- END original Makefile.host.reroll-mechanics ----

# ---- BEGIN original Makefile.host.species-change-moves ----
# Thread R, round 3 (R2-BUG-004): a species change resets all four moves to the new species'
# native level-up moves, Gen I-V (20 games). Data + algorithm + apply-path contracts, then one
# real record round trip per generation (reusing each generation's existing source list).
SPECIES_MOVES_DEPS := include/Pokemon/SpeciesMoveDefaults.h include/Pokemon/LevelUpLearnsets15Data.inc include/UI/Gen1SpeciesChangeMoves.h \
	include/UI/SpeciesChangeMoves.h

SPECIES_MOVES_DATA_SOURCES := tests/test_species_change_moves_gen15.cpp
SPECIES_MOVES_GEN2_SOURCES := tests/test_species_change_moves_gen2.cpp $(GEN2_STAGED_EDITOR_COMMON) \
	src/Integration/Gen2/Gen2MoveCompatibility.cpp src/Integration/Gen1/Gen1MoveCompatibility.cpp

SPECIES_MOVES_GEN1_SOURCES := tests/test_species_change_moves_gen1.cpp $(GEN1_POKEMON_COMMON) \
	src/Integration/Gen1/Gen1MoveCompatibility.cpp

SPECIES_MOVES_GEN3_SOURCES := tests/test_species_change_moves_gen3.cpp \
	$(filter-out tests/test_gen3_staged_pokemon_editor.cpp,$(GEN3_POKEMON_EDITOR_SOURCES))

SPECIES_MOVES_GEN4_SOURCES := tests/test_species_change_moves_gen4.cpp \
	$(filter-out tests/test_gen4_staged_pokemon_editor.cpp,$(GEN4_STAGED_EDITOR_SOURCES))

SPECIES_MOVES_TESTS := $(HOST_BUILD)/test_species_change_moves_gen15 $(HOST_BUILD)/test_species_change_moves_gen1 \
	$(HOST_BUILD)/test_species_change_moves_gen2 $(HOST_BUILD)/test_species_change_moves_gen3 \
	$(HOST_BUILD)/test_species_change_moves_gen4 $(HOST_BUILD)/test_species_change_moves_gen5
SPECIES_MOVES_SANITIZE := $(addsuffix _sanitize,$(SPECIES_MOVES_TESTS))
HOST_TESTS += $(SPECIES_MOVES_TESTS)
HOST_SANITIZE_TESTS += $(SPECIES_MOVES_SANITIZE)
host-test: $(SPECIES_MOVES_TESTS)
host-sanitize: $(SPECIES_MOVES_SANITIZE)

.PHONY: host-species-change-moves
host-species-change-moves: $(SPECIES_MOVES_TESTS)
	@set -e; for t in $(SPECIES_MOVES_TESTS); do $$t; done

$(HOST_BUILD)/test_species_change_moves_gen15: $(SPECIES_MOVES_DATA_SOURCES) $(SPECIES_MOVES_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(SPECIES_MOVES_DATA_SOURCES) -o $@
$(HOST_BUILD)/test_species_change_moves_gen15_sanitize: $(SPECIES_MOVES_DATA_SOURCES) $(SPECIES_MOVES_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(SPECIES_MOVES_DATA_SOURCES) -o $@

$(HOST_BUILD)/test_species_change_moves_gen2: $(SPECIES_MOVES_GEN2_SOURCES) $(SPECIES_MOVES_DEPS) include/UI/Gen2PokemonSession.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude -Itests $(SPECIES_MOVES_GEN2_SOURCES) -o $@
$(HOST_BUILD)/test_species_change_moves_gen2_sanitize: $(SPECIES_MOVES_GEN2_SOURCES) $(SPECIES_MOVES_DEPS) include/UI/Gen2PokemonSession.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude -Itests $(SPECIES_MOVES_GEN2_SOURCES) -o $@

$(HOST_BUILD)/test_species_change_moves_gen1: $(SPECIES_MOVES_GEN1_SOURCES) $(SPECIES_MOVES_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(SPECIES_MOVES_GEN1_SOURCES) -o $@
$(HOST_BUILD)/test_species_change_moves_gen1_sanitize: $(SPECIES_MOVES_GEN1_SOURCES) $(SPECIES_MOVES_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(SPECIES_MOVES_GEN1_SOURCES) -o $@

$(HOST_BUILD)/test_species_change_moves_gen3: $(SPECIES_MOVES_GEN3_SOURCES) $(SPECIES_MOVES_DEPS) include/UI/Gen3SharedPokemonSession.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST -Iinclude $(SPECIES_MOVES_GEN3_SOURCES) -o $@
$(HOST_BUILD)/test_species_change_moves_gen3_sanitize: $(SPECIES_MOVES_GEN3_SOURCES) $(SPECIES_MOVES_DEPS) include/UI/Gen3SharedPokemonSession.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST -Iinclude $(SPECIES_MOVES_GEN3_SOURCES) -o $@

$(HOST_BUILD)/test_species_change_moves_gen4: $(SPECIES_MOVES_GEN4_SOURCES) $(SPECIES_MOVES_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(SPECIES_MOVES_GEN4_SOURCES) -o $@
$(HOST_BUILD)/test_species_change_moves_gen4_sanitize: $(SPECIES_MOVES_GEN4_SOURCES) $(SPECIES_MOVES_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(SPECIES_MOVES_GEN4_SOURCES) -o $@

$(HOST_BUILD)/test_species_change_moves_gen5: tests/test_species_change_moves_gen5.cpp $(SPECIES_MOVES_DEPS) \
	include/Integration/Gen5/Gen5SharedScreenState.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_species_change_moves_gen5.cpp src/Names/SpeciesNames.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp -o $@
$(HOST_BUILD)/test_species_change_moves_gen5_sanitize: tests/test_species_change_moves_gen5.cpp $(SPECIES_MOVES_DEPS) \
	include/Integration/Gen5/Gen5SharedScreenState.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_species_change_moves_gen5.cpp src/Names/SpeciesNames.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp -o $@
# ---- END original Makefile.host.species-change-moves ----

# ---- BEGIN original Makefile.host.round4-r ----
# Lane R, round 4 (owner Round 3 hardware results 62a2f8c3): R2-REROLL Gen II Unown letter / sprite,
# NEW-15 Create IV/DV defaults, R2-SPECIAL Gen II Special layout, NEW-09 nature +/- stats,
# NEW-14 Gen V Language. One host binary per repair; each is also built by host-sanitize.
R4R_GEN2_SOURCES := $(GEN2_STAGED_EDITOR_COMMON) src/Integration/Gen2/Gen2MoveCompatibility.cpp \
	src/Integration/Gen1/Gen1MoveCompatibility.cpp

R4R_TESTS := $(HOST_BUILD)/test_gen2_unown_letter
R4R_SANITIZE := $(addsuffix _sanitize,$(R4R_TESTS))
HOST_TESTS += $(R4R_TESTS)
HOST_SANITIZE_TESTS += $(R4R_SANITIZE)
host-test: $(R4R_TESTS)
host-sanitize: $(R4R_SANITIZE)

$(HOST_BUILD)/test_gen2_unown_letter: tests/test_gen2_unown_letter.cpp $(R4R_GEN2_SOURCES) include/UI/Gen12DVReroll.h \
	include/UI/Gen2PokemonSession.h include/UI/Gen2PokemonPickerModel.h tests/fixtures/gsc_pokemon_fixture.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude -Itests tests/test_gen2_unown_letter.cpp $(R4R_GEN2_SOURCES) -o $@
$(HOST_BUILD)/test_gen2_unown_letter_sanitize: tests/test_gen2_unown_letter.cpp $(R4R_GEN2_SOURCES) include/UI/Gen12DVReroll.h \
	include/UI/Gen2PokemonSession.h include/UI/Gen2PokemonPickerModel.h tests/fixtures/gsc_pokemon_fixture.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude -Itests tests/test_gen2_unown_letter.cpp $(R4R_GEN2_SOURCES) -o $@

# Round 6 P6: Gen II Unown letter selector (exhaustive DV space; stored form, sprite key, Review).
R6R_TESTS := $(HOST_BUILD)/test_gen2_unown_letter_selector
R6R_SANITIZE := $(addsuffix _sanitize,$(R6R_TESTS))
HOST_TESTS += $(R6R_TESTS)
HOST_SANITIZE_TESTS += $(R6R_SANITIZE)
host-test: $(R6R_TESTS)
host-sanitize: $(R6R_SANITIZE)

$(HOST_BUILD)/test_gen2_unown_letter_selector: tests/test_gen2_unown_letter_selector.cpp $(R4R_GEN2_SOURCES) \
	include/UI/Gen2UnownLetter.h include/UI/Gen12DVReroll.h include/UI/Gen2PokemonSession.h tests/fixtures/gsc_pokemon_fixture.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude -Itests tests/test_gen2_unown_letter_selector.cpp $(R4R_GEN2_SOURCES) -o $@
$(HOST_BUILD)/test_gen2_unown_letter_selector_sanitize: tests/test_gen2_unown_letter_selector.cpp $(R4R_GEN2_SOURCES) \
	include/UI/Gen2UnownLetter.h include/UI/Gen12DVReroll.h include/UI/Gen2PokemonSession.h tests/fixtures/gsc_pokemon_fixture.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude -Itests tests/test_gen2_unown_letter_selector.cpp $(R4R_GEN2_SOURCES) -o $@

# Round 6 E05: every footer / Help key maps to a real input handler (static contract, with mutations).
.PHONY: round6-footer-help-contract
qa-tools-test: round6-footer-help-contract
round6-footer-help-contract:
	python3 tests/test_footer_help_handlers.py

# Round 6 E05: Gen I-IV browse footer order (one model, Review slot off by default).
R6R_FOOTER := $(HOST_BUILD)/test_classic_browse_footer
HOST_TESTS += $(R6R_FOOTER)
HOST_SANITIZE_TESTS += $(R6R_FOOTER)_sanitize
host-test: $(R6R_FOOTER)
host-sanitize: $(R6R_FOOTER)_sanitize
$(R6R_FOOTER): tests/test_classic_browse_footer.cpp include/UI/ClassicBrowseFooter.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_classic_browse_footer.cpp -o $@
$(R6R_FOOTER)_sanitize: tests/test_classic_browse_footer.cpp include/UI/ClassicBrowseFooter.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_classic_browse_footer.cpp -o $@
# ---- END original Makefile.host.round4-r ----

# ---- BEGIN original Makefile.host.round4-codex ----
HOST_TESTS += $(HOST_BUILD)/test_classic_staged_exit
HOST_TESTS += $(HOST_BUILD)/test_classic_party_actions
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_classic_party_actions_sanitize
host-test: $(HOST_BUILD)/test_classic_party_actions
host-sanitize: $(HOST_BUILD)/test_classic_party_actions_sanitize
$(HOST_BUILD)/test_classic_party_actions: tests/test_classic_party_actions.cpp src/UI/ClassicPartyActions.inc $(CLASSIC_INVENTORY_CATALOG_LINK_SOURCES) src/Names/ItemNames.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_classic_party_actions.cpp $(CLASSIC_INVENTORY_CATALOG_LINK_SOURCES) src/Names/ItemNames.cpp -o $@
$(HOST_BUILD)/test_classic_party_actions_sanitize: tests/test_classic_party_actions.cpp src/UI/ClassicPartyActions.inc $(CLASSIC_INVENTORY_CATALOG_LINK_SOURCES) src/Names/ItemNames.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_classic_party_actions.cpp $(CLASSIC_INVENTORY_CATALOG_LINK_SOURCES) src/Names/ItemNames.cpp -o $@
HOST_TESTS += $(HOST_BUILD)/test_gen2_party_edit
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_party_edit_sanitize
host-test: $(HOST_BUILD)/test_gen2_party_edit
host-sanitize: $(HOST_BUILD)/test_gen2_party_edit_sanitize
$(HOST_BUILD)/test_gen2_party_edit: tests/test_gen2_party_edit.cpp $(GEN2_STAGED_EDITOR_COMMON) tests/fixtures/gsc_pokemon_fixture.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_gen2_party_edit.cpp $(GEN2_STAGED_EDITOR_COMMON) -o $@
$(HOST_BUILD)/test_gen2_party_edit_sanitize: tests/test_gen2_party_edit.cpp $(GEN2_STAGED_EDITOR_COMMON) tests/fixtures/gsc_pokemon_fixture.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_gen2_party_edit.cpp $(GEN2_STAGED_EDITOR_COMMON) -o $@
HOST_TESTS += $(HOST_BUILD)/test_gen45_encounter_context
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen45_encounter_context_sanitize
host-test: $(HOST_BUILD)/test_gen45_encounter_context
host-sanitize: $(HOST_BUILD)/test_gen45_encounter_context_sanitize
$(HOST_BUILD)/test_gen45_encounter_context: tests/test_gen45_encounter_context.cpp src/Names/LocationNames.cpp include/Integration/Encounter/EncounterContext45.h include/Integration/Encounter/EncounterContext45Data.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_gen45_encounter_context.cpp src/Names/LocationNames.cpp -o $@
$(HOST_BUILD)/test_gen45_encounter_context_sanitize: tests/test_gen45_encounter_context.cpp src/Names/LocationNames.cpp include/Integration/Encounter/EncounterContext45.h include/Integration/Encounter/EncounterContext45Data.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_gen45_encounter_context.cpp src/Names/LocationNames.cpp -o $@
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_classic_staged_exit_sanitize
HOST_TESTS += $(HOST_BUILD)/test_nature_stat_effect
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_nature_stat_effect_sanitize
host-test: $(HOST_BUILD)/test_nature_stat_effect
host-sanitize: $(HOST_BUILD)/test_nature_stat_effect_sanitize
$(HOST_BUILD)/test_nature_stat_effect: tests/test_nature_stat_effect.cpp include/UI/NatureStatEffect.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $< -o $@
$(HOST_BUILD)/test_nature_stat_effect_sanitize: tests/test_nature_stat_effect.cpp include/UI/NatureStatEffect.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $< -o $@
.PHONY: round4-contract-mutations
qa-tools-test: round4-contract-mutations
round4-contract-mutations:
	python3 tests/test_gen45_encounter_generator.py
	python3 tests/test_round4_backup_contract_mutations.py
	python3 tests/test_round4_shared_contract_mutations.py
host-test: $(HOST_BUILD)/test_classic_staged_exit
host-sanitize: $(HOST_BUILD)/test_classic_staged_exit_sanitize
$(HOST_BUILD)/test_classic_staged_exit: tests/test_classic_staged_exit.cpp include/UI/ClassicStagedExit.h include/UI/ClassicTrainerSession.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $< -o $@
$(HOST_BUILD)/test_classic_staged_exit_sanitize: tests/test_classic_staged_exit.cpp include/UI/ClassicStagedExit.h include/UI/ClassicTrainerSession.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $< -o $@
# October 24 backup provenance and physical-snapshot safety (pure host POSIX).
HOST_TESTS += $(HOST_BUILD)/test_oct24_picker_help_events
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_oct24_picker_help_events_sanitize
host-test: $(HOST_BUILD)/test_oct24_picker_help_events
host-sanitize: $(HOST_BUILD)/test_oct24_picker_help_events_sanitize
$(HOST_BUILD)/test_oct24_picker_help_events: tests/test_oct24_picker_help_events.cpp src/Names/LocationNames.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Itests/host-switch -Iinclude $^ -o $@
$(HOST_BUILD)/test_oct24_picker_help_events_sanitize: tests/test_oct24_picker_help_events.cpp src/Names/LocationNames.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Itests/host-switch -Iinclude $^ -o $@
HOST_TESTS += $(HOST_BUILD)/test_backup_snapshot_open
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_backup_snapshot_open_sanitize
host-test: $(HOST_BUILD)/test_backup_snapshot_open
host-sanitize: $(HOST_BUILD)/test_backup_snapshot_open_sanitize
$(HOST_BUILD)/test_backup_snapshot_open: tests/test_backup_snapshot_open.cpp src/Utils/SHA256.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_backup_snapshot_open_sanitize: tests/test_backup_snapshot_open.cpp src/Utils/SHA256.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@
# ---- END original Makefile.host.round4-codex ----
