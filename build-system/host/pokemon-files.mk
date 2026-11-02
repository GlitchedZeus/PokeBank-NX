# PokeBank NX host test rules: pokemon-files.mk
# Consolidated from the original Makefile.host.* fragments in their EXACT include order.
# Do not modify host-test recipes or weaken coverage while reorganizing infrastructure.

# ---- BEGIN original Makefile.host.pkse-october ----
# PKSE October 2026 upstream campaign (owner Oct 28): inherited-bug regressions and new features.
# Included by Makefile.host after Makefile.host.multilingual.

PLA_RESEARCH_SOURCES := tests/test_pla_research_counts.cpp src/Trainer/Trainer8LA.cpp src/Trainer/Trainer.cpp src/Encryption/Encryption8LA.cpp src/Encryption/Encryption.cpp src/Pokemon/Pokemon8LA.cpp src/Pokemon/PLADexTable.cpp src/Pokemon/PersonalInfoTable.cpp src/Pokemon/BaseStatsGen89.cpp src/Pokemon/Experience.cpp src/Save/Block.cpp src/Utils/SHA256.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp src/Utils/Logger.cpp src/Names/AbilityNames.cpp src/Names/ItemNames.cpp src/Names/NatureNames.cpp src/Names/SpeciesNames.cpp src/Enums/SCTypeCode.cpp
PKSE_OCTOBER_HOST_TESTS := $(HOST_BUILD)/test_pla_research_counts
PKSE_OCTOBER_SANITIZE_TESTS := $(HOST_BUILD)/test_pla_research_counts_sanitize

HOST_TESTS += $(PKSE_OCTOBER_HOST_TESTS)
HOST_SANITIZE_TESTS += $(PKSE_OCTOBER_SANITIZE_TESTS)
host-test: $(PKSE_OCTOBER_HOST_TESTS)
host-sanitize: $(PKSE_OCTOBER_SANITIZE_TESTS)

$(HOST_BUILD)/test_pla_research_counts: $(PLA_RESEARCH_SOURCES) include/Trainer/Trainer8LA.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude -Ivendor/PKSM-Core/include -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST $(PLA_RESEARCH_SOURCES) -o $@
$(HOST_BUILD)/test_pla_research_counts_sanitize: $(PLA_RESEARCH_SOURCES) include/Trainer/Trainer8LA.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude -Ivendor/PKSM-Core/include -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST $(PLA_RESEARCH_SOURCES) -o $@

LGPE_BLANK_SOURCES := tests/test_lgpe_blank_slots.cpp src/Trainer/Trainer7LGPE.cpp src/Trainer/Trainer.cpp src/Encryption/Encryption7LGPE.cpp src/Encryption/Encryption.cpp src/Pokemon/Pokemon7LGPE.cpp  src/Pokemon/PersonalInfoTable.cpp src/Pokemon/BaseStatsGen89.cpp src/Pokemon/Experience.cpp src/Save/Block.cpp src/Utils/SHA256.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp src/Utils/Logger.cpp src/Names/AbilityNames.cpp src/Names/ItemNames.cpp src/Names/NatureNames.cpp src/Names/SpeciesNames.cpp src/Enums/SCTypeCode.cpp src/Pokemon/BaseStatsGen7.cpp
PKSE_OCTOBER_HOST_TESTS += $(HOST_BUILD)/test_lgpe_blank_slots
PKSE_OCTOBER_SANITIZE_TESTS += $(HOST_BUILD)/test_lgpe_blank_slots_sanitize
HOST_TESTS += $(HOST_BUILD)/test_lgpe_blank_slots
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_lgpe_blank_slots_sanitize
host-test: $(HOST_BUILD)/test_lgpe_blank_slots
host-sanitize: $(HOST_BUILD)/test_lgpe_blank_slots_sanitize
$(HOST_BUILD)/test_lgpe_blank_slots: $(LGPE_BLANK_SOURCES) include/Trainer/Trainer7LGPE.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude -Ivendor/PKSM-Core/include -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST $(LGPE_BLANK_SOURCES) -o $@
$(HOST_BUILD)/test_lgpe_blank_slots_sanitize: $(LGPE_BLANK_SOURCES) include/Trainer/Trainer7LGPE.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude -Ivendor/PKSM-Core/include -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST $(LGPE_BLANK_SOURCES) -o $@

PKSE_OCTOBER_HOST_TESTS += $(HOST_BUILD)/test_settings_file_parse
HOST_TESTS += $(HOST_BUILD)/test_settings_file_parse
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_settings_file_parse_sanitize
host-test: $(HOST_BUILD)/test_settings_file_parse
host-sanitize: $(HOST_BUILD)/test_settings_file_parse_sanitize
$(HOST_BUILD)/test_settings_file_parse: tests/test_settings_file_parse.cpp include/Utils/SettingsFile.h include/Utils/PokeBankPaths.h src/Utils/Settings.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_settings_file_parse.cpp -o $@
$(HOST_BUILD)/test_settings_file_parse_sanitize: tests/test_settings_file_parse.cpp include/Utils/SettingsFile.h include/Utils/PokeBankPaths.h src/Utils/Settings.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_settings_file_parse.cpp -o $@

PKSE_OCTOBER_HOST_TESTS += $(HOST_BUILD)/test_pkse_october_contracts
HOST_TESTS += $(HOST_BUILD)/test_pkse_october_contracts
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_pkse_october_contracts_sanitize
host-test: $(HOST_BUILD)/test_pkse_october_contracts
host-sanitize: $(HOST_BUILD)/test_pkse_october_contracts_sanitize
$(HOST_BUILD)/test_pkse_october_contracts: tests/test_pkse_october_contracts.cpp src/UI/TrainerViewScreenBase.inc include/UI/TrainerViewScreenBase.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) tests/test_pkse_october_contracts.cpp -o $@
$(HOST_BUILD)/test_pkse_october_contracts_sanitize: tests/test_pkse_october_contracts.cpp src/UI/TrainerViewScreenBase.inc include/UI/TrainerViewScreenBase.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) tests/test_pkse_october_contracts.cpp -o $@

BULK_ITEM_PLAN_SOURCES := tests/test_bulk_item_plan.cpp src/Inventory/ClassicInventoryCatalog.cpp src/Inventory/ItemQuantityPolicy.cpp src/Names/ItemNames.cpp src/Names/MoveNames.cpp src/Integration/Gen2/Gen2ReadOnlyInventory.cpp
PKSE_OCTOBER_HOST_TESTS += $(HOST_BUILD)/test_bulk_item_plan
HOST_TESTS += $(HOST_BUILD)/test_bulk_item_plan
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_bulk_item_plan_sanitize
host-test: $(HOST_BUILD)/test_bulk_item_plan
host-sanitize: $(HOST_BUILD)/test_bulk_item_plan_sanitize
$(HOST_BUILD)/test_bulk_item_plan: $(BULK_ITEM_PLAN_SOURCES) include/Inventory/BulkItemPlan.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(BULK_ITEM_PLAN_SOURCES) -o $@
$(HOST_BUILD)/test_bulk_item_plan_sanitize: $(BULK_ITEM_PLAN_SOURCES) include/Inventory/BulkItemPlan.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(BULK_ITEM_PLAN_SOURCES) -o $@

BULK_ITEM_APPLY_SOURCES := tests/test_bulk_item_apply.cpp src/Inventory/ClassicInventoryCatalog.cpp src/Inventory/ItemQuantityPolicy.cpp src/Names/ItemNames.cpp src/Names/MoveNames.cpp src/Integration/Gen2/Gen2ReadOnlyInventory.cpp src/Integration/Gen1/Gen1ReadOnlyInventory.cpp src/Integration/Gen1/Gen1ReadOnlySave.cpp src/Integration/Gen1/Gen1StagedInventoryEditor.cpp
PKSE_OCTOBER_HOST_TESTS += $(HOST_BUILD)/test_bulk_item_apply
HOST_TESTS += $(HOST_BUILD)/test_bulk_item_apply
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_bulk_item_apply_sanitize
host-test: $(HOST_BUILD)/test_bulk_item_apply
host-sanitize: $(HOST_BUILD)/test_bulk_item_apply_sanitize
$(HOST_BUILD)/test_bulk_item_apply: $(BULK_ITEM_APPLY_SOURCES) include/Inventory/BulkItemPlan.h include/Inventory/BulkItemApply.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude -Itests $(BULK_ITEM_APPLY_SOURCES) -o $@
$(HOST_BUILD)/test_bulk_item_apply_sanitize: $(BULK_ITEM_APPLY_SOURCES) include/Inventory/BulkItemPlan.h include/Inventory/BulkItemApply.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude -Itests $(BULK_ITEM_APPLY_SOURCES) -o $@

NATIVE_POKEMON_FILE_SOURCES := tests/test_native_pokemon_file.cpp src/PokemonFile/NativePokemonFile.cpp src/Encryption/Encryption3FRLG.cpp src/Encryption/Encryption4.cpp
PKSE_OCTOBER_HOST_TESTS += $(HOST_BUILD)/test_native_pokemon_file
HOST_TESTS += $(HOST_BUILD)/test_native_pokemon_file
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_native_pokemon_file_sanitize
host-test: $(HOST_BUILD)/test_native_pokemon_file
host-sanitize: $(HOST_BUILD)/test_native_pokemon_file_sanitize
$(HOST_BUILD)/test_native_pokemon_file: $(NATIVE_POKEMON_FILE_SOURCES) include/PokemonFile/NativePokemonFile.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(NATIVE_POKEMON_FILE_SOURCES) -o $@
$(HOST_BUILD)/test_native_pokemon_file_sanitize: $(NATIVE_POKEMON_FILE_SOURCES) include/PokemonFile/NativePokemonFile.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(NATIVE_POKEMON_FILE_SOURCES) -o $@

PKSE_OCTOBER_HOST_TESTS += $(HOST_BUILD)/test_folder_preferences
HOST_TESTS += $(HOST_BUILD)/test_folder_preferences
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_folder_preferences_sanitize
host-test: $(HOST_BUILD)/test_folder_preferences
host-sanitize: $(HOST_BUILD)/test_folder_preferences_sanitize
$(HOST_BUILD)/test_folder_preferences: tests/test_folder_preferences.cpp include/Utils/FolderPreferences.h include/Utils/SettingsFile.h src/Utils/Settings.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_folder_preferences.cpp -o $@
$(HOST_BUILD)/test_folder_preferences_sanitize: tests/test_folder_preferences.cpp include/Utils/FolderPreferences.h include/Utils/SettingsFile.h src/Utils/Settings.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_folder_preferences.cpp -o $@

PKSE_OCTOBER_HOST_TESTS += $(HOST_BUILD)/test_search_shoulder_keys
HOST_TESTS += $(HOST_BUILD)/test_search_shoulder_keys
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_search_shoulder_keys_sanitize
host-test: $(HOST_BUILD)/test_search_shoulder_keys
host-sanitize: $(HOST_BUILD)/test_search_shoulder_keys_sanitize
$(HOST_BUILD)/test_search_shoulder_keys: tests/test_search_shoulder_keys.cpp include/UI/SharedSearchInput.h include/UI/SharedSpeciesPicker.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude tests/test_search_shoulder_keys.cpp -o $@
$(HOST_BUILD)/test_search_shoulder_keys_sanitize: tests/test_search_shoulder_keys.cpp include/UI/SharedSearchInput.h include/UI/SharedSpeciesPicker.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude tests/test_search_shoulder_keys.cpp -o $@
# ---- END original Makefile.host.pkse-october ----

# ---- BEGIN original Makefile.host.pkse-october-pid ----
# PKSE 620c5f6 review "PID, shiny and gender preservation" (owner directive): Gen III / IV / V
# IV / EV edits, unchanged re-stages, Discard and serialization through the production
# staged-editing routes keep PID, shininess, gender, nature and ability.
PKSE_OCTOBER_PID_HOST_TESTS := $(HOST_BUILD)/test_gen345_pid_preservation_routes
PKSE_OCTOBER_PID_SANITIZE_TESTS := $(HOST_BUILD)/test_gen345_pid_preservation_routes_sanitize

HOST_TESTS += $(PKSE_OCTOBER_PID_HOST_TESTS)
HOST_SANITIZE_TESTS += $(PKSE_OCTOBER_PID_SANITIZE_TESTS)
host-test: $(PKSE_OCTOBER_PID_HOST_TESTS)
host-sanitize: $(PKSE_OCTOBER_PID_SANITIZE_TESTS)

GEN345_PID_PRESERVATION_SOURCES := tests/test_gen345_pid_preservation_routes.cpp \
	src/Integration/Gen3/Gen3StagedPokemonEditor.cpp src/Integration/Gen3/Gen3StagedInventoryEditor.cpp \
	src/Integration/Gen3/PKSMGen3NativeAdapter.cpp src/Integration/Gen3/FRLGReadModel.cpp \
	src/Pokemon/Pokemon3FRLG.cpp src/Pokemon/BaseStatsGen89.cpp src/Pokemon/PersonalInfoTable.cpp \
	src/Encryption/Encryption3FRLG.cpp src/Inventory/ClassicInventoryCatalog.cpp src/Inventory/ItemQuantityPolicy.cpp \
	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp \
	src/Integration/Gen4/Gen4StagedPokemonEditor.cpp src/Integration/Gen4/Gen4ExportTransaction.cpp \
	src/Integration/Gen4/Gen4ReadOnlySave.cpp src/Integration/Common/VerifiedStagedExport.cpp \
	src/Pokemon/Pokemon4ReadOnlyView.cpp src/Pokemon/Pokemon4Mutable.cpp src/Pokemon/Pokemon4ReadOnly.cpp \
	src/Pokemon/PersonalRecord4.cpp src/Pokemon/PersonalInfo4DP.cpp src/Pokemon/PersonalInfo4PT.cpp \
	src/Pokemon/PersonalInfo4HGSS.cpp src/Encryption/Encryption4.cpp \
	src/Integration/Gen5/Gen5WorkingCopyExport.cpp \
	src/Pokemon/Experience.cpp src/Names/ItemNames.cpp src/Names/ItemPresence.cpp src/Names/MoveNames.cpp \
	src/Names/MoveInfo.cpp src/Names/MovePresence.cpp src/Names/MachineDisplay.cpp src/Names/TMMoves.cpp \
	src/Names/LocationNames.cpp src/Names/AbilityNames.cpp src/Names/NatureNames.cpp src/Names/SpeciesNames.cpp \
	src/Utils/SHA256.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp
GEN345_PID_PRESERVATION_DEPS := $(GEN345_PID_PRESERVATION_SOURCES) \
	include/Integration/Gen3/Gen3StagedPokemonEditor.h include/UI/Gen3SharedPokemonSession.h \
	include/Integration/Gen4/Gen4StagedPokemonEditor.h include/UI/Gen4SharedPokemonSession.h \
	include/Pokemon/Pokemon4Mutable.h include/Integration/Gen4/Gen4ExportTransaction.h \
	$(wildcard include/Integration/Gen5/*.h)

$(HOST_BUILD)/test_gen345_pid_preservation_routes: $(GEN345_PID_PRESERVATION_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST -Iinclude $(GEN345_PID_PRESERVATION_SOURCES) -o $@

$(HOST_BUILD)/test_gen345_pid_preservation_routes_sanitize: $(GEN345_PID_PRESERVATION_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST -Iinclude $(GEN345_PID_PRESERVATION_SOURCES) -o $@
# ---- END original Makefile.host.pkse-october-pid ----

# ---- BEGIN original Makefile.host.pkfile-gen12 ----
# Feature A (owner Oct 28): standalone .pk1 / .pk2 import and export in the Gen I / Gen II staged editors.
# Included by Makefile.host after Makefile.host.pkse-october.

POKEMON_FILE_GEN12_SOURCES := tests/test_pokemon_file_gen12.cpp $(sort \
	src/PokemonFile/NativePokemonFile.cpp src/Encryption/Encryption3FRLG.cpp src/Encryption/Encryption4.cpp \
	src/Integration/Gen1/Gen1StagedPokemonEditor.cpp src/Integration/Gen1/Gen1PackedMove.cpp \
	src/Integration/Gen1/Gen1BattleStats.cpp src/Integration/Gen1/Gen1ReadOnlySave.cpp \
	src/Integration/Gen1/Gen1StagedInventoryEditor.cpp src/Integration/Gen1/Gen1ReadOnlyInventory.cpp \
	src/Integration/Gen2/Gen2StagedEditor.cpp src/Integration/Gen2/Gen2PackedMove.cpp \
	src/Integration/Gen2/Gen2ReadOnlySave.cpp src/Integration/Gen2/Gen2ReadOnlyInventory.cpp \
	src/Integration/Gen2/Gen2PersonalData.cpp \
	src/Inventory/ClassicInventoryCatalog.cpp src/Inventory/ItemQuantityPolicy.cpp \
	src/Pokemon/Experience.cpp src/Names/SpeciesNames.cpp src/Names/MoveNames.cpp src/Names/ItemNames.cpp \
	src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp)
POKEMON_FILE_GEN12_HEADERS := include/PokemonFile/NativePokemonFile.h \
	include/Integration/Gen1/Gen1StagedPokemonEditor.h include/Integration/Gen2/Gen2StagedEditor.h \
	src/Integration/Gen1/Gen1PersonalData.inc tests/fixtures/gsc_pokemon_fixture.h
PKFILE_GEN12_HOST_TESTS := $(HOST_BUILD)/test_pokemon_file_gen12
PKFILE_GEN12_SANITIZE_TESTS := $(HOST_BUILD)/test_pokemon_file_gen12_sanitize

HOST_TESTS += $(PKFILE_GEN12_HOST_TESTS)
HOST_SANITIZE_TESTS += $(PKFILE_GEN12_SANITIZE_TESTS)
host-test: $(PKFILE_GEN12_HOST_TESTS)
host-sanitize: $(PKFILE_GEN12_SANITIZE_TESTS)

$(HOST_BUILD)/test_pokemon_file_gen12: $(POKEMON_FILE_GEN12_SOURCES) $(POKEMON_FILE_GEN12_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude -Itests $(POKEMON_FILE_GEN12_SOURCES) -o $@
$(HOST_BUILD)/test_pokemon_file_gen12_sanitize: $(POKEMON_FILE_GEN12_SOURCES) $(POKEMON_FILE_GEN12_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude -Itests $(POKEMON_FILE_GEN12_SOURCES) -o $@
# ---- END original Makefile.host.pkfile-gen12 ----

# ---- BEGIN original Makefile.host.pkfile-gen3 ----
# Feature A (standalone Pokemon files), Gen III slice: .pk3 / .ek3 import into an empty staged box
# slot and non-mutating export, for R/S/E/FR/LG. Included by Makefile.host after
# Makefile.host.pkse-october-pid.

PKFILE_GEN3_SOURCES := tests/test_pokemon_file_gen3.cpp \
	src/Integration/Gen3/Gen3StagedPokemonEditor.cpp src/Integration/Gen3/Gen3StagedInventoryEditor.cpp \
	src/Integration/Gen3/PKSMGen3NativeAdapter.cpp src/Integration/Gen3/FRLGReadModel.cpp \
	src/PokemonFile/NativePokemonFile.cpp src/Encryption/Encryption3FRLG.cpp src/Encryption/Encryption4.cpp \
	src/Pokemon/Pokemon3FRLG.cpp src/Pokemon/BaseStatsGen89.cpp src/Pokemon/PersonalInfoTable.cpp \
	src/Pokemon/Experience.cpp src/Inventory/ClassicInventoryCatalog.cpp src/Inventory/ItemQuantityPolicy.cpp \
	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp src/Names/ItemNames.cpp src/Names/MoveNames.cpp \
	src/Names/MoveInfo.cpp src/Names/SpeciesNames.cpp src/Names/NatureNames.cpp src/Names/AbilityNames.cpp \
	src/Names/LocationNames.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp
PKFILE_GEN3_HEADERS := include/Integration/Gen3/Gen3StagedPokemonEditor.h include/PokemonFile/NativePokemonFile.h \
	tests/fuzz_synthetic_saves.h
PKFILE_GEN3_HOST_TESTS := $(HOST_BUILD)/test_pokemon_file_gen3
PKFILE_GEN3_SANITIZE_TESTS := $(HOST_BUILD)/test_pokemon_file_gen3_sanitize

HOST_TESTS += $(PKFILE_GEN3_HOST_TESTS)
HOST_SANITIZE_TESTS += $(PKFILE_GEN3_SANITIZE_TESTS)
host-test: $(PKFILE_GEN3_HOST_TESTS)
host-sanitize: $(PKFILE_GEN3_SANITIZE_TESTS)

$(HOST_BUILD)/test_pokemon_file_gen3: $(PKFILE_GEN3_SOURCES) $(PKFILE_GEN3_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST -Iinclude -Itests $(PKFILE_GEN3_SOURCES) -o $@
$(HOST_BUILD)/test_pokemon_file_gen3_sanitize: $(PKFILE_GEN3_SOURCES) $(PKFILE_GEN3_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST -Iinclude -Itests $(PKFILE_GEN3_SOURCES) -o $@
# ---- END original Makefile.host.pkfile-gen3 ----

# ---- BEGIN original Makefile.host.pkfile-gen4 ----
# Feature A (owner Oct 28): standalone .pk4 / .ek4 import into the Gen IV staged editor and
# plaintext PK4 export. Self-contained: builds with
#   make -f Makefile.host.base -f Makefile.host.pkfile-gen4 HOST_BUILD=<dir> <dir>/test_pokemon_file_gen4
# (and <dir>/test_pokemon_file_gen4_sanitize). Included by Makefile.host after Makefile.host.pkse-october.

PKFILE_GEN4_SOURCES := tests/test_pokemon_file_gen4.cpp \
	src/PokemonFile/NativePokemonFile.cpp src/Encryption/Encryption3FRLG.cpp \
	src/Integration/Gen4/Gen4StagedPokemonEditor.cpp \
	src/Integration/Gen4/Gen4ExportTransaction.cpp src/Integration/Common/VerifiedStagedExport.cpp \
	src/Pokemon/Pokemon4ReadOnlyView.cpp src/Names/ItemNames.cpp src/Names/MoveNames.cpp src/Names/MachineDisplay.cpp \
	src/Names/TMMoves.cpp src/Names/LocationNames.cpp src/Names/AbilityNames.cpp src/Names/NatureNames.cpp \
	src/Utils/SHA256.cpp \
	src/Integration/Gen4/Gen4ReadOnlySave.cpp \
	src/Pokemon/Pokemon4Mutable.cpp src/Pokemon/Pokemon4ReadOnly.cpp \
	src/Pokemon/PersonalRecord4.cpp src/Pokemon/PersonalInfo4DP.cpp \
	src/Pokemon/PersonalInfo4PT.cpp src/Pokemon/PersonalInfo4HGSS.cpp \
	src/Encryption/Encryption4.cpp src/Pokemon/Experience.cpp \
	src/Names/SpeciesNames.cpp src/Names/ItemPresence.cpp src/Names/MoveInfo.cpp src/Names/MovePresence.cpp \
	src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp
PKFILE_GEN4_DEPS := $(PKFILE_GEN4_SOURCES) include/PokemonFile/NativePokemonFile.h \
	include/Integration/Gen4/Gen4StagedPokemonEditor.h include/Integration/Gen4/Gen4ExportTransaction.h \
	include/Pokemon/Pokemon4Mutable.h include/Pokemon/Pokemon4ReadOnly.h include/Names/Gen4HeldItemCatalog.h \
	include/Encryption/Encryption4.h tests/fuzz_synthetic_saves.h
PKFILE_GEN4_TEST := $(HOST_BUILD)/test_pokemon_file_gen4
PKFILE_GEN4_SANITIZE := $(HOST_BUILD)/test_pokemon_file_gen4_sanitize

HOST_TESTS += $(PKFILE_GEN4_TEST)
HOST_SANITIZE_TESTS += $(PKFILE_GEN4_SANITIZE)
host-test: $(PKFILE_GEN4_TEST)
host-sanitize: $(PKFILE_GEN4_SANITIZE)

$(PKFILE_GEN4_TEST): $(PKFILE_GEN4_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude -Itests $(PKFILE_GEN4_SOURCES) -o $@
$(PKFILE_GEN4_SANITIZE): $(PKFILE_GEN4_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude -Itests $(PKFILE_GEN4_SOURCES) -o $@
# ---- END original Makefile.host.pkfile-gen4 ----

# ---- BEGIN original Makefile.host.pkfile-gen5 ----
# Feature A (owner Oct 28): standalone .pk5 / .ek5 files for Black/White and Black 2/White 2.
# Import into the staged workspace only, independent Review / export-candidate re-verification,
# and non-mutating export. Included by Makefile.host after Makefile.host.pkse-october.

PKFILE_GEN5_SOURCES := tests/test_pokemon_file_gen5.cpp src/PokemonFile/NativePokemonFile.cpp src/Encryption/Encryption3FRLG.cpp src/Encryption/Encryption4.cpp src/Names/SpeciesNames.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp
PKFILE_GEN5_HEADERS := include/Integration/Gen5/Gen5PokemonFile.h include/Integration/Gen5/Gen5ImportRecord.h include/Integration/Gen5/Gen5StagedPokemonWorkspace.h include/Integration/Gen5/Gen5SharedReview.h include/Integration/Gen5/Gen5StagedSaveCandidate.h include/Integration/Gen5/Gen5StagedPokemonRecord.h include/Integration/Gen5/Gen5ReviewRows.h include/PokemonFile/NativePokemonFile.h
PKFILE_GEN5_HOST_TESTS := $(HOST_BUILD)/test_pokemon_file_gen5
PKFILE_GEN5_SANITIZE_TESTS := $(HOST_BUILD)/test_pokemon_file_gen5_sanitize

HOST_TESTS += $(PKFILE_GEN5_HOST_TESTS)
HOST_SANITIZE_TESTS += $(PKFILE_GEN5_SANITIZE_TESTS)
host-test: $(PKFILE_GEN5_HOST_TESTS)
host-sanitize: $(PKFILE_GEN5_SANITIZE_TESTS)

$(HOST_BUILD)/test_pokemon_file_gen5: $(PKFILE_GEN5_SOURCES) $(PKFILE_GEN5_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(PKFILE_GEN5_SOURCES) -o $@
$(HOST_BUILD)/test_pokemon_file_gen5_sanitize: $(PKFILE_GEN5_SOURCES) $(PKFILE_GEN5_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(PKFILE_GEN5_SOURCES) -o $@
# ---- END original Makefile.host.pkfile-gen5 ----
