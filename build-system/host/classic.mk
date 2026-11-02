# PokeBank NX host test rules: classic.mk
# Consolidated from the original Makefile.host.* fragments in their EXACT include order.
# Do not modify host-test recipes or weaken coverage while reorganizing infrastructure.

# ---- BEGIN original Makefile.host.gsc ----
# Generation II GSC permanent host gates.
GEN2_ADAPTER_SOURCES := tests/test_gsc_gen2_adapter.cpp \
	src/Integration/Gen2/Gen2ReadOnlySave.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp \
	src/Integration/Gen2/Gen2StagedEditor.cpp src/Integration/Gen2/Gen2UniversalInventory.cpp \
	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp src/Integration/Gen2/Gen2PersonalData.cpp \
	src/Inventory/ClassicInventoryCatalog.cpp src/Inventory/ItemQuantityPolicy.cpp src/Names/ItemNames.cpp \
	src/Names/MoveNames.cpp src/Names/SpeciesNames.cpp src/Pokemon/Experience.cpp
GSC_INVENTORY_SOURCES := tests/test_gsc_inventory.cpp \
	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp
GSC_PERSONAL_SOURCES := tests/test_gsc_gen2_personal.cpp \
	src/Integration/Gen2/Gen2PersonalData.cpp
GSC_MOVE_COMPAT_SOURCES := tests/test_gsc_move_compatibility.cpp \
	src/Integration/Gen2/Gen2MoveCompatibility.cpp \
	src/Integration/Gen1/Gen1MoveCompatibility.cpp
GSC_DISCOVERY_SOURCES := tests/test_gsc_discovery.cpp \
	src/Legacy/RetroArchGSCDiscovery.cpp src/Integration/Gen2/Gen2ReadOnlySave.cpp \
	src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp src/Utils/SHA256.cpp
GSC_SOURCE_BROWSER_SOURCES := tests/test_gsc_source_browser.cpp \
	src/Legacy/GSCSourceBrowser.cpp src/Legacy/LegacySourceBindings.cpp \
	src/Integration/Gen2/Gen2ReadOnlySave.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp src/Games/GameIdentity.cpp
GSC_BRIDGE_SOURCES := tests/test_gsc_readonly_bridge.cpp \
	src/Legacy/GSCReadOnlyTrainer.cpp src/Pokemon/Pokemon2ReadOnly.cpp \
	src/Integration/Gen2/Gen2ReadOnlySave.cpp src/Integration/Gen2/Gen2ReadOnlyInventory.cpp \
	src/Integration/Gen2/Gen2StagedEditor.cpp \
	src/Integration/Gen2/Gen2PersonalData.cpp src/Pokemon/Experience.cpp src/Pokemon/BaseStatsGen89.cpp src/Pokemon/PersonalInfoTable.cpp \
	src/Names/SpeciesNames.cpp src/Names/ItemNames.cpp src/Names/NatureNames.cpp \
	src/Names/AbilityNames.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp \
	src/Names/MoveNames.cpp
GSC_BRIDGE_FLAGS := -Wno-unused-parameter
GSC_UI_RULES_SOURCES := tests/test_gsc_ui_rules.cpp \
	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp
GSC_PASSIVE_VIEW_CONTRACT_SOURCES := tests/test_passive_pokemon_view_contract.cpp
GSC_EDITOR_SURFACE_CONTRACT_SOURCES := tests/test_gsc_editor_surface_contract.cpp
GSC_NATIVE_PRESENTATION_SOURCES := tests/test_gsc_native_presentation.cpp
GSC_EXPORT_TRANSACTION_SOURCES := tests/test_gsc_export_transaction.cpp \
	src/Integration/Gen2/Gen2ExportTransaction.cpp \
	src/Integration/Gen2/Gen2StagedEditor.cpp \
	src/Integration/Gen2/Gen2ReadOnlySave.cpp \
	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp \
	src/Integration/Gen2/Gen2PersonalData.cpp \
	src/Pokemon/Experience.cpp src/Names/SpeciesNames.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp src/Utils/SHA256.cpp \
	src/Names/MoveNames.cpp

GSC_RUNTIME_DISCOVERY_SOURCES := src/Legacy/RetroArchGSCDiscovery.cpp \
	src/Integration/Gen2/Gen2ReadOnlySave.cpp
RETROARCH_FRLG_SOURCES += $(GSC_RUNTIME_DISCOVERY_SOURCES)
RSE_HOST_SOURCES += $(GSC_RUNTIME_DISCOVERY_SOURCES)
RSE_NATIVE_SOURCES += $(GSC_RUNTIME_DISCOVERY_SOURCES)
$(HOST_BUILD)/test_rse_gen3_native_slice: $(GSC_RUNTIME_DISCOVERY_SOURCES)
$(HOST_BUILD)/test_rse_gen3_native_slice_sanitize: $(GSC_RUNTIME_DISCOVERY_SOURCES)
GSC_RUNTIME_CATALOG_SOURCES := tests/test_gsc_runtime_catalog.cpp \
	$(filter-out tests/test_retroarch_frlg_discovery.cpp,$(RETROARCH_FRLG_SOURCES)) \
	src/Legacy/FRLGSourceBrowser.cpp src/Legacy/LegacySourceBindings.cpp
GSC_RUNTIME_CATALOG_FLAGS := -Wno-unused-parameter

GSC_HOST_TESTS := $(HOST_BUILD)/test_gsc_gen2_adapter \
	$(HOST_BUILD)/test_gsc_inventory \
	$(HOST_BUILD)/test_gsc_gen2_personal \
	$(HOST_BUILD)/test_gsc_move_compatibility \
	$(HOST_BUILD)/test_gsc_discovery \
	$(HOST_BUILD)/test_gsc_source_browser \
	$(HOST_BUILD)/test_gsc_readonly_bridge \
	$(HOST_BUILD)/test_gsc_runtime_catalog \
	$(HOST_BUILD)/test_gsc_ui_rules \
	$(HOST_BUILD)/test_passive_pokemon_view_contract \
	$(HOST_BUILD)/test_gsc_editor_surface_contract \
	$(HOST_BUILD)/test_gsc_native_presentation \
	$(HOST_BUILD)/test_gsc_export_transaction
GSC_SANITIZE_TESTS := $(HOST_BUILD)/test_gsc_gen2_adapter_sanitize \
	$(HOST_BUILD)/test_gsc_inventory_sanitize \
	$(HOST_BUILD)/test_gsc_gen2_personal_sanitize \
	$(HOST_BUILD)/test_gsc_move_compatibility_sanitize \
	$(HOST_BUILD)/test_gsc_discovery_sanitize \
	$(HOST_BUILD)/test_gsc_source_browser_sanitize \
	$(HOST_BUILD)/test_gsc_readonly_bridge_sanitize \
	$(HOST_BUILD)/test_gsc_runtime_catalog_sanitize \
	$(HOST_BUILD)/test_gsc_ui_rules_sanitize \
	$(HOST_BUILD)/test_passive_pokemon_view_contract_sanitize \
	$(HOST_BUILD)/test_gsc_editor_surface_contract_sanitize \
	$(HOST_BUILD)/test_gsc_native_presentation_sanitize \
	$(HOST_BUILD)/test_gsc_export_transaction_sanitize

HOST_TESTS += $(GSC_HOST_TESTS)
HOST_SANITIZE_TESTS += $(GSC_SANITIZE_TESTS)
host-test: $(GSC_HOST_TESTS)
host-sanitize: $(GSC_SANITIZE_TESTS)

$(HOST_BUILD)/test_gsc_gen2_adapter: $(GEN2_ADAPTER_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_gen2_adapter_sanitize: $(GEN2_ADAPTER_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_inventory: $(GSC_INVENTORY_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_inventory_sanitize: $(GSC_INVENTORY_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_gen2_personal: $(GSC_PERSONAL_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_gen2_personal_sanitize: $(GSC_PERSONAL_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_move_compatibility: $(GSC_MOVE_COMPAT_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_move_compatibility_sanitize: $(GSC_MOVE_COMPAT_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_discovery: $(GSC_DISCOVERY_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_discovery_sanitize: $(GSC_DISCOVERY_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_source_browser: $(GSC_SOURCE_BROWSER_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_source_browser_sanitize: $(GSC_SOURCE_BROWSER_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_readonly_bridge: $(GSC_BRIDGE_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(GSC_BRIDGE_FLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_readonly_bridge_sanitize: $(GSC_BRIDGE_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(GSC_BRIDGE_FLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_runtime_catalog: $(GSC_RUNTIME_CATALOG_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(GSC_RUNTIME_CATALOG_FLAGS) -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_runtime_catalog_sanitize: $(GSC_RUNTIME_CATALOG_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(GSC_RUNTIME_CATALOG_FLAGS) $(SANITIZE_FLAGS) -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_ui_rules: $(GSC_UI_RULES_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_ui_rules_sanitize: $(GSC_UI_RULES_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_passive_pokemon_view_contract: $(GSC_PASSIVE_VIEW_CONTRACT_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_passive_pokemon_view_contract_sanitize: $(GSC_PASSIVE_VIEW_CONTRACT_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_editor_surface_contract: $(GSC_EDITOR_SURFACE_CONTRACT_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_editor_surface_contract_sanitize: $(GSC_EDITOR_SURFACE_CONTRACT_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_native_presentation: $(GSC_NATIVE_PRESENTATION_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_native_presentation_sanitize: $(GSC_NATIVE_PRESENTATION_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_export_transaction: $(GSC_EXPORT_TRANSACTION_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gsc_export_transaction_sanitize: $(GSC_EXPORT_TRANSACTION_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@
# ---- END original Makefile.host.gsc ----

# ---- BEGIN original Makefile.host.editor ----
# Classic staged-save editor gates. Kept separate from the accepted read-only Gen I/II/III graphs.
GEN2_STAGED_EDITOR_COMMON := \
	src/Integration/Gen2/Gen2StagedEditor.cpp \
	src/Integration/Gen2/Gen2ReadOnlySave.cpp \
	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp \
	src/Integration/Gen2/Gen2PersonalData.cpp \
	src/Names/SpeciesNames.cpp \
	src/Pokemon/Experience.cpp \
	src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp \
	src/Names/MoveNames.cpp

GEN2_STAGED_EDITOR_SOURCES := tests/test_gsc_staged_editor.cpp $(GEN2_STAGED_EDITOR_COMMON)
GEN2_POKEMON_EDITOR_SOURCES := tests/test_gsc_pokemon_editor.cpp $(GEN2_STAGED_EDITOR_COMMON)

EDITOR_HOST_TESTS := $(HOST_BUILD)/test_gsc_staged_editor $(HOST_BUILD)/test_gsc_pokemon_editor
EDITOR_SANITIZE_TESTS := $(HOST_BUILD)/test_gsc_staged_editor_sanitize $(HOST_BUILD)/test_gsc_pokemon_editor_sanitize

HOST_TESTS += $(EDITOR_HOST_TESTS)
HOST_SANITIZE_TESTS += $(EDITOR_SANITIZE_TESTS)
host-test: $(EDITOR_HOST_TESTS)
host-sanitize: $(EDITOR_SANITIZE_TESTS)

$(HOST_BUILD)/test_gsc_staged_editor: $(GEN2_STAGED_EDITOR_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@

$(HOST_BUILD)/test_gsc_staged_editor_sanitize: $(GEN2_STAGED_EDITOR_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@

$(HOST_BUILD)/test_gsc_pokemon_editor: $(GEN2_POKEMON_EDITOR_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@

$(HOST_BUILD)/test_gsc_pokemon_editor_sanitize: $(GEN2_POKEMON_EDITOR_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@

GEN1_POKEMON_COMMON := src/Integration/Gen1/Gen1StagedPokemonEditor.cpp \
    src/Integration/Gen1/Gen1BattleStats.cpp \
    src/Integration/Gen1/Gen1ReadOnlySave.cpp src/Integration/Gen1/Gen1StagedInventoryEditor.cpp \
    src/Integration/Gen1/Gen1ReadOnlyInventory.cpp src/Pokemon/Experience.cpp src/Names/SpeciesNames.cpp \
    src/Inventory/ClassicInventoryCatalog.cpp src/Inventory/ItemQuantityPolicy.cpp src/Names/ItemNames.cpp src/Names/MoveNames.cpp \
    src/Integration/Gen2/Gen2ReadOnlyInventory.cpp
GEN1_POKEMON_SOURCES := tests/test_rby_pokemon_editor.cpp $(GEN1_POKEMON_COMMON)
HOST_TESTS += $(HOST_BUILD)/test_rby_pokemon_editor
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_rby_pokemon_editor_sanitize
host-test: $(HOST_BUILD)/test_rby_pokemon_editor
host-sanitize: $(HOST_BUILD)/test_rby_pokemon_editor_sanitize
$(HOST_BUILD)/test_rby_pokemon_editor: $(GEN1_POKEMON_SOURCES) src/Integration/Gen1/Gen1PersonalData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN1_POKEMON_SOURCES) -o $@
$(HOST_BUILD)/test_rby_pokemon_editor_sanitize: $(GEN1_POKEMON_SOURCES) src/Integration/Gen1/Gen1PersonalData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN1_POKEMON_SOURCES) -o $@

# Exact five-stat Generation I battle-stat preview math used by the draft and staged Summary radar.
GEN1_BATTLE_STAT_SOURCES := tests/test_gen1_battle_stats.cpp src/Integration/Gen1/Gen1BattleStats.cpp src/Integration/Gen1/Gen1StagedPokemonEditor.cpp src/Integration/Gen1/Gen1ReadOnlySave.cpp src/Pokemon/Experience.cpp src/Names/SpeciesNames.cpp src/Names/MoveNames.cpp src/Inventory/ClassicInventoryCatalog.cpp src/Inventory/ItemQuantityPolicy.cpp src/Names/ItemNames.cpp src/Integration/Gen1/Gen1ReadOnlyInventory.cpp src/Integration/Gen2/Gen2ReadOnlyInventory.cpp
HOST_TESTS += $(HOST_BUILD)/test_gen1_battle_stats
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen1_battle_stats_sanitize
host-test: $(HOST_BUILD)/test_gen1_battle_stats
host-sanitize: $(HOST_BUILD)/test_gen1_battle_stats_sanitize
$(HOST_BUILD)/test_gen1_battle_stats: $(GEN1_BATTLE_STAT_SOURCES) src/Integration/Gen1/Gen1PersonalData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN1_BATTLE_STAT_SOURCES) -o $@
$(HOST_BUILD)/test_gen1_battle_stats_sanitize: $(GEN1_BATTLE_STAT_SOURCES) src/Integration/Gen1/Gen1PersonalData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN1_BATTLE_STAT_SOURCES) -o $@

# Host-testable controller/action contract for the Switch-only Gen I overlay. This permanently proves
# empty vs occupied action availability and that draft navigation/edit/cancel never requests mutation.
HOST_TESTS += $(HOST_BUILD)/test_gen1_pokemon_editor_ui_contract
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen1_pokemon_editor_ui_contract_sanitize
host-test: $(HOST_BUILD)/test_gen1_pokemon_editor_ui_contract
host-sanitize: $(HOST_BUILD)/test_gen1_pokemon_editor_ui_contract_sanitize
$(HOST_BUILD)/test_gen1_pokemon_editor_ui_contract: tests/test_gen1_pokemon_editor_ui_contract.cpp include/UI/Gen1PokemonEditorUIContract.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_gen1_pokemon_editor_ui_contract.cpp -o $@
$(HOST_BUILD)/test_gen1_pokemon_editor_ui_contract_sanitize: tests/test_gen1_pokemon_editor_ui_contract.cpp include/UI/Gen1PokemonEditorUIContract.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_gen1_pokemon_editor_ui_contract.cpp -o $@

# RBY bridge owns the staged Pokemon sidecar; accepted read-only tests must link its native
# dependencies in addition to the already-linked classic inventory sidecar.
$(HOST_BUILD)/test_rby_readonly_bridge $(HOST_BUILD)/test_rby_readonly_bridge_sanitize: src/Integration/Gen1/Gen1StagedPokemonEditor.cpp src/Integration/Gen1/Gen1BattleStats.cpp src/Pokemon/Experience.cpp src/Names/MoveNames.cpp

GEN2_SESSION_SOURCES := tests/test_gsc_pokemon_session.cpp $(GEN2_STAGED_EDITOR_COMMON) src/Integration/Gen2/Gen2MoveCompatibility.cpp src/Integration/Gen1/Gen1MoveCompatibility.cpp
HOST_TESTS += $(HOST_BUILD)/test_gsc_pokemon_session
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gsc_pokemon_session_sanitize
host-test: $(HOST_BUILD)/test_gsc_pokemon_session
host-sanitize: $(HOST_BUILD)/test_gsc_pokemon_session_sanitize
$(HOST_BUILD)/test_gsc_pokemon_session: $(GEN2_SESSION_SOURCES) include/UI/Gen2PokemonSession.h include/UI/Gen2PokemonPickerModel.h include/UI/Gen2NativePresentation.h tests/fixtures/gsc_pokemon_fixture.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN2_SESSION_SOURCES) -o $@
$(HOST_BUILD)/test_gsc_pokemon_session_sanitize: $(GEN2_SESSION_SOURCES) include/UI/Gen2PokemonSession.h include/UI/Gen2PokemonPickerModel.h include/UI/Gen2NativePresentation.h tests/fixtures/gsc_pokemon_fixture.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN2_SESSION_SOURCES) -o $@

# Permanent Gen II shared-editor parity gate. Gen II battle-stat math is header-only, so this uses
# the proven session source graph; the explicit header dependency below keeps parity behavior tied
# to the exact six-stat implementation in every normal and sanitizer host suite.
GEN2_SHARED_EDITOR_PARITY_SOURCES := tests/test_gsc_shared_editor_parity.cpp \
	$(GEN2_STAGED_EDITOR_COMMON) \
	src/Integration/Gen2/Gen2MoveCompatibility.cpp \
	src/Integration/Gen1/Gen1MoveCompatibility.cpp
HOST_TESTS += $(HOST_BUILD)/test_gsc_shared_editor_parity
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gsc_shared_editor_parity_sanitize
host-test: $(HOST_BUILD)/test_gsc_shared_editor_parity
host-sanitize: $(HOST_BUILD)/test_gsc_shared_editor_parity_sanitize
$(HOST_BUILD)/test_gsc_shared_editor_parity: $(GEN2_SHARED_EDITOR_PARITY_SOURCES) include/Integration/Gen2/Gen2BattleStats.h include/UI/SharedPokemonEditorContract.h include/UI/ExactSaveCapabilities.h include/UI/Gen2PokemonSession.h include/UI/Gen2PokemonPickerModel.h include/UI/Gen2NativePresentation.h include/UI/BattleStatRadarModel.h tests/fixtures/gsc_pokemon_fixture.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN2_SHARED_EDITOR_PARITY_SOURCES) -o $@
$(HOST_BUILD)/test_gsc_shared_editor_parity_sanitize: $(GEN2_SHARED_EDITOR_PARITY_SOURCES) include/Integration/Gen2/Gen2BattleStats.h include/UI/SharedPokemonEditorContract.h include/UI/ExactSaveCapabilities.h include/UI/Gen2PokemonSession.h include/UI/Gen2PokemonPickerModel.h include/UI/Gen2NativePresentation.h include/UI/BattleStatRadarModel.h tests/fixtures/gsc_pokemon_fixture.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN2_SHARED_EDITOR_PARITY_SOURCES) -o $@


# Generation III one-shared-editor exact capability/provider contract.
GEN3_EXACT_PROVIDER_SOURCES := tests/test_gen3_exact_format_editor_provider.cpp
HOST_TESTS += $(HOST_BUILD)/test_gen3_exact_format_editor_provider
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen3_exact_format_editor_provider_sanitize
host-test: $(HOST_BUILD)/test_gen3_exact_format_editor_provider
host-sanitize: $(HOST_BUILD)/test_gen3_exact_format_editor_provider_sanitize
$(HOST_BUILD)/test_gen3_exact_format_editor_provider: $(GEN3_EXACT_PROVIDER_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gen3_exact_format_editor_provider_sanitize: $(GEN3_EXACT_PROVIDER_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@

# Staged boxed-PK3 editor. Uses the accepted native Gen III reader plus PokeBank-owned
# exact-format mutation/serialize/reparse logic; the caller's source vector is never written.
GEN3_POKEMON_EDITOR_SOURCES := tests/test_gen3_staged_pokemon_editor.cpp 	src/Integration/Gen3/Gen3StagedPokemonEditor.cpp 	src/Integration/Gen3/Gen3ExportTransaction.cpp src/Integration/Common/VerifiedStagedExport.cpp src/Utils/SHA256.cpp 	src/Integration/Gen3/Gen3StagedInventoryEditor.cpp 	src/Integration/Gen3/PKSMGen3NativeAdapter.cpp 	src/Integration/Gen3/FRLGReadModel.cpp 	src/Pokemon/Pokemon3FRLG.cpp src/Pokemon/BaseStatsGen89.cpp 	src/Pokemon/PersonalInfoTable.cpp src/Pokemon/Experience.cpp 	src/Encryption/Encryption3FRLG.cpp 	src/Inventory/ClassicInventoryCatalog.cpp src/Inventory/ItemQuantityPolicy.cpp 	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp 	src/Names/ItemNames.cpp src/Names/MoveNames.cpp src/Names/MoveInfo.cpp 	src/Names/SpeciesNames.cpp src/Names/NatureNames.cpp src/Names/AbilityNames.cpp 	src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp src/Names/LocationNames.cpp

HOST_TESTS += $(HOST_BUILD)/test_gen3_staged_pokemon_editor
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen3_staged_pokemon_editor_sanitize
host-test: $(HOST_BUILD)/test_gen3_staged_pokemon_editor
host-sanitize: $(HOST_BUILD)/test_gen3_staged_pokemon_editor_sanitize
$(HOST_BUILD)/test_gen3_staged_pokemon_editor: $(GEN3_POKEMON_EDITOR_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST -Iinclude $^ -o $@
$(HOST_BUILD)/test_gen3_staged_pokemon_editor_sanitize: $(GEN3_POKEMON_EDITOR_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST -Iinclude $^ -o $@


# Host-testable Generation III shared editor draft/session contract. Draft browsing and field edits
# stay UI-owned until explicit Keep/Add delegates to the staged PK3 backend.
GEN3_SHARED_SESSION_SOURCES := tests/test_gen3_shared_pokemon_session.cpp src/Pokemon/Experience.cpp src/Pokemon/PersonalInfoTable.cpp src/Names/MoveInfo.cpp
HOST_TESTS += $(HOST_BUILD)/test_gen3_shared_pokemon_session
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen3_shared_pokemon_session_sanitize
host-test: $(HOST_BUILD)/test_gen3_shared_pokemon_session
host-sanitize: $(HOST_BUILD)/test_gen3_shared_pokemon_session_sanitize
$(HOST_BUILD)/test_gen3_shared_pokemon_session: $(GEN3_SHARED_SESSION_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN3_SHARED_SESSION_SOURCES) -o $@
$(HOST_BUILD)/test_gen3_shared_pokemon_session_sanitize: $(GEN3_SHARED_SESSION_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN3_SHARED_SESSION_SOURCES) -o $@

# Hardware-retest routing, shared renderer and scroll/controller contracts.
HOST_TESTS += $(HOST_BUILD)/test_gen3_hardware_surface_contract
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen3_hardware_surface_contract_sanitize
host-test: $(HOST_BUILD)/test_gen3_hardware_surface_contract
host-sanitize: $(HOST_BUILD)/test_gen3_hardware_surface_contract_sanitize
$(HOST_BUILD)/test_gen3_hardware_surface_contract: tests/test_gen3_hardware_surface_contract.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@
$(HOST_BUILD)/test_gen3_hardware_surface_contract_sanitize: tests/test_gen3_hardware_surface_contract.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@

HOST_TESTS += $(HOST_BUILD)/test_gen3_shell_render
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen3_shell_render_sanitize
host-test: $(HOST_BUILD)/test_gen3_shell_render
host-sanitize: $(HOST_BUILD)/test_gen3_shell_render_sanitize
$(HOST_BUILD)/test_gen3_shell_render: tests/test_gen3_shell_render.cpp include/UI/SharedPokemonShell.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_gen3_shell_render.cpp -o $@
$(HOST_BUILD)/test_gen3_shell_render_sanitize: tests/test_gen3_shell_render.cpp include/UI/SharedPokemonShell.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_gen3_shell_render.cpp -o $@

# Header extraction must rebuild accepted Gen II renderer regression binaries as well.
$(HOST_BUILD)/test_gsc_workspace_render $(HOST_BUILD)/test_gsc_workspace_render_sanitize: include/UI/SharedPokemonShell.h

# Populated and Empty outer move-focus regressions exercise both real session models.
$(HOST_BUILD)/test_gsc_pokemon_session $(HOST_BUILD)/test_gsc_pokemon_session_sanitize $(HOST_BUILD)/test_gen3_shared_pokemon_session $(HOST_BUILD)/test_gen3_shared_pokemon_session_sanitize: tests/fixtures/main_workspace_move_focus.h include/UI/SharedPokemonEditorContract.h
# ---- END original Makefile.host.editor ----

# ---- BEGIN original Makefile.host.cleanup3 ----
# Gen I UX Cleanup #3 focused host gates. Kept in a separate make fragment so
# the already-accepted Cleanup #2 host graph remains intact.

GEN1_CLEANUP3_MOVE_SOURCES := \
	tests/test_gen1_move_compatibility.cpp \
	src/Integration/Gen1/Gen1MoveCompatibility.cpp

HOST_TESTS += $(HOST_BUILD)/test_gen1_move_compatibility
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen1_move_compatibility_sanitize
host-test: $(HOST_BUILD)/test_gen1_move_compatibility
host-sanitize: $(HOST_BUILD)/test_gen1_move_compatibility_sanitize

$(HOST_BUILD)/test_gen1_move_compatibility: $(GEN1_CLEANUP3_MOVE_SOURCES) src/Integration/Gen1/Gen1MoveCompatibilityData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN1_CLEANUP3_MOVE_SOURCES) -o $@

$(HOST_BUILD)/test_gen1_move_compatibility_sanitize: $(GEN1_CLEANUP3_MOVE_SOURCES) src/Integration/Gen1/Gen1MoveCompatibilityData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN1_CLEANUP3_MOVE_SOURCES) -o $@

GEN1_CLEANUP3_SHINY_SOURCES := tests/test_gen1_shiny.cpp

HOST_TESTS += $(HOST_BUILD)/test_gen1_shiny
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen1_shiny_sanitize
host-test: $(HOST_BUILD)/test_gen1_shiny
host-sanitize: $(HOST_BUILD)/test_gen1_shiny_sanitize

$(HOST_BUILD)/test_gen1_shiny: $(GEN1_CLEANUP3_SHINY_SOURCES) include/Integration/Gen1/Gen1Shiny.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN1_CLEANUP3_SHINY_SOURCES) -o $@

$(HOST_BUILD)/test_gen1_shiny_sanitize: $(GEN1_CLEANUP3_SHINY_SOURCES) include/Integration/Gen1/Gen1Shiny.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN1_CLEANUP3_SHINY_SOURCES) -o $@

GEN1_CLEANUP3_UX_SOURCES := tests/test_gen1_pokemon_editor_ux3_contract.cpp

HOST_TESTS += $(HOST_BUILD)/test_gen1_pokemon_editor_ux3_contract
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen1_pokemon_editor_ux3_contract_sanitize
host-test: $(HOST_BUILD)/test_gen1_pokemon_editor_ux3_contract
host-sanitize: $(HOST_BUILD)/test_gen1_pokemon_editor_ux3_contract_sanitize

$(HOST_BUILD)/test_gen1_pokemon_editor_ux3_contract: $(GEN1_CLEANUP3_UX_SOURCES) include/UI/Gen1PokemonEditorUX3Contract.h include/UI/NavigationRepeat.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN1_CLEANUP3_UX_SOURCES) -o $@

$(HOST_BUILD)/test_gen1_pokemon_editor_ux3_contract_sanitize: $(GEN1_CLEANUP3_UX_SOURCES) include/UI/Gen1PokemonEditorUX3Contract.h include/UI/NavigationRepeat.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN1_CLEANUP3_UX_SOURCES) -o $@

# End-to-end staged semantics for the new kid-facing shiny/move UX: a shiny Add
# must survive strict finalize/reparse and Clone, while unrelated edits preserve an
# existing weird source move instead of normalizing it through the picker model.
GEN1_CLEANUP3_STAGED_SOURCES := \
	tests/test_gen1_cleanup3_staged_semantics.cpp \
	$(GEN1_POKEMON_COMMON) \
	src/Integration/Gen1/Gen1MoveCompatibility.cpp

HOST_TESTS += $(HOST_BUILD)/test_gen1_cleanup3_staged_semantics
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen1_cleanup3_staged_semantics_sanitize
host-test: $(HOST_BUILD)/test_gen1_cleanup3_staged_semantics
host-sanitize: $(HOST_BUILD)/test_gen1_cleanup3_staged_semantics_sanitize

$(HOST_BUILD)/test_gen1_cleanup3_staged_semantics: $(GEN1_CLEANUP3_STAGED_SOURCES) src/Integration/Gen1/Gen1MoveCompatibilityData.inc src/Integration/Gen1/Gen1PersonalData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN1_CLEANUP3_STAGED_SOURCES) -o $@

$(HOST_BUILD)/test_gen1_cleanup3_staged_semantics_sanitize: $(GEN1_CLEANUP3_STAGED_SOURCES) src/Integration/Gen1/Gen1MoveCompatibilityData.inc src/Integration/Gen1/Gen1PersonalData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN1_CLEANUP3_STAGED_SOURCES) -o $@

# Permanent three-panel / generation-capability foundation contract. This is deliberately
# host-only: later-generation capability fixtures prove layout scalability without enabling
# unfinished Gen III serializers or live-write paths.
POKEMON_EDITOR_FOUNDATION_SOURCES := tests/test_pokemon_editor_foundation_contract.cpp

HOST_TESTS += $(HOST_BUILD)/test_pokemon_editor_foundation_contract
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_pokemon_editor_foundation_contract_sanitize
host-test: $(HOST_BUILD)/test_pokemon_editor_foundation_contract
host-sanitize: $(HOST_BUILD)/test_pokemon_editor_foundation_contract_sanitize

$(HOST_BUILD)/test_pokemon_editor_foundation_contract: $(POKEMON_EDITOR_FOUNDATION_SOURCES) include/UI/PokemonEditorFoundationContract.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(POKEMON_EDITOR_FOUNDATION_SOURCES) -o $@

$(HOST_BUILD)/test_pokemon_editor_foundation_contract_sanitize: $(POKEMON_EDITOR_FOUNDATION_SOURCES) include/UI/PokemonEditorFoundationContract.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(POKEMON_EDITOR_FOUNDATION_SOURCES) -o $@

# Production Gen I presentation + recording-framebuffer radar regression gates.
GEN1_SUMMARY_SOURCES := tests/test_gen1_summary_presentation.cpp $(GEN1_POKEMON_COMMON) \
    src/Pokemon/Pokemon1ReadOnly.cpp src/Pokemon/BaseStatsGen89.cpp src/Pokemon/PersonalInfoTable.cpp \
    src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp src/Names/NatureNames.cpp src/Names/AbilityNames.cpp
HOST_TESTS += $(HOST_BUILD)/test_gen1_summary_presentation $(HOST_BUILD)/test_gen1_radar_render
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen1_summary_presentation_sanitize $(HOST_BUILD)/test_gen1_radar_render_sanitize
host-test: $(HOST_BUILD)/test_gen1_summary_presentation $(HOST_BUILD)/test_gen1_radar_render
host-sanitize: $(HOST_BUILD)/test_gen1_summary_presentation_sanitize $(HOST_BUILD)/test_gen1_radar_render_sanitize
$(HOST_BUILD)/test_gen1_summary_presentation: $(GEN1_SUMMARY_SOURCES) include/UI/Gen1PokemonPresentation.h include/UI/BattleStatRadarModel.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(GEN1_SUMMARY_SOURCES) -o $@
$(HOST_BUILD)/test_gen1_summary_presentation_sanitize: $(GEN1_SUMMARY_SOURCES) include/UI/Gen1PokemonPresentation.h include/UI/BattleStatRadarModel.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(GEN1_SUMMARY_SOURCES) -o $@
$(HOST_BUILD)/test_gen1_radar_render: tests/test_gen1_radar_render.cpp include/UI/StatsRadar.h include/UI/BattleStatRadarModel.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_gen1_radar_render.cpp -o $@
$(HOST_BUILD)/test_gen1_radar_render_sanitize: tests/test_gen1_radar_render.cpp include/UI/StatsRadar.h include/UI/BattleStatRadarModel.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_gen1_radar_render.cpp -o $@
# ---- END original Makefile.host.cleanup3 ----

# ---- BEGIN original Makefile.host.standalone ----
STANDALONE_HOST_TESTS := \
	$(HOST_BUILD)/test_pokebank_paths \
	$(HOST_BUILD)/test_clean_install_runtime \
	$(HOST_BUILD)/test_runtime_dependency_contract

STANDALONE_SANITIZE_TESTS := \
	$(HOST_BUILD)/test_pokebank_paths_sanitize \
	$(HOST_BUILD)/test_clean_install_runtime_sanitize \
	$(HOST_BUILD)/test_runtime_dependency_contract_sanitize

HOST_TESTS += $(STANDALONE_HOST_TESTS)
HOST_SANITIZE_TESTS += $(STANDALONE_SANITIZE_TESTS)
host-test: $(STANDALONE_HOST_TESTS)
host-sanitize: $(STANDALONE_SANITIZE_TESTS)

$(HOST_BUILD)/test_pokebank_paths: tests/test_pokebank_paths.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@

$(HOST_BUILD)/test_clean_install_runtime: tests/test_clean_install_runtime.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@

$(HOST_BUILD)/test_runtime_dependency_contract: tests/test_runtime_dependency_contract.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@

$(HOST_BUILD)/test_pokebank_paths_sanitize: tests/test_pokebank_paths.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@

$(HOST_BUILD)/test_clean_install_runtime_sanitize: tests/test_clean_install_runtime.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@

$(HOST_BUILD)/test_runtime_dependency_contract_sanitize: tests/test_runtime_dependency_contract.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@
# ---- END original Makefile.host.standalone ----

# ---- BEGIN original Makefile.host.shared-editor ----
# One-Pokemon-editor architecture contract.
SHARED_EDITOR_SOURCES := tests/test_shared_pokemon_editor_contract.cpp
SHARED_EDITOR_HOST_TEST := $(HOST_BUILD)/test_shared_pokemon_editor_contract
SHARED_EDITOR_SANITIZE_TEST := $(HOST_BUILD)/test_shared_pokemon_editor_contract_sanitize

HOST_TESTS += $(SHARED_EDITOR_HOST_TEST)
HOST_SANITIZE_TESTS += $(SHARED_EDITOR_SANITIZE_TEST)
host-test: $(SHARED_EDITOR_HOST_TEST)
host-sanitize: $(SHARED_EDITOR_SANITIZE_TEST)

$(SHARED_EDITOR_HOST_TEST): $(SHARED_EDITOR_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@

$(SHARED_EDITOR_SANITIZE_TEST): $(SHARED_EDITOR_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@

SHARED_EDITOR_HEADING_SOURCES := tests/test_shared_editor_heading_contract.cpp
SHARED_EDITOR_HEADING_TEST := $(HOST_BUILD)/test_shared_editor_heading_contract
SHARED_EDITOR_HEADING_SANITIZE := $(HOST_BUILD)/test_shared_editor_heading_contract_sanitize

HOST_TESTS += $(SHARED_EDITOR_HEADING_TEST)
HOST_SANITIZE_TESTS += $(SHARED_EDITOR_HEADING_SANITIZE)
host-test: $(SHARED_EDITOR_HEADING_TEST)
host-sanitize: $(SHARED_EDITOR_HEADING_SANITIZE)

$(SHARED_EDITOR_HEADING_TEST): $(SHARED_EDITOR_HEADING_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@

$(SHARED_EDITOR_HEADING_SANITIZE): $(SHARED_EDITOR_HEADING_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@

# Issue #71 pre-Gen-III architecture freeze guard. It is intentionally
# product-neutral: no Gen III editor implementation or mutation path is linked.
PRE_GEN3_REUSE_GUARD_SOURCES := tests/test_pre_gen3_reuse_guard.cpp
PRE_GEN3_REUSE_GUARD_TEST := $(HOST_BUILD)/test_pre_gen3_reuse_guard
PRE_GEN3_REUSE_GUARD_SANITIZE := $(HOST_BUILD)/test_pre_gen3_reuse_guard_sanitize

HOST_TESTS += $(PRE_GEN3_REUSE_GUARD_TEST)
HOST_SANITIZE_TESTS += $(PRE_GEN3_REUSE_GUARD_SANITIZE)
host-test: $(PRE_GEN3_REUSE_GUARD_TEST)
host-sanitize: $(PRE_GEN3_REUSE_GUARD_SANITIZE)

$(PRE_GEN3_REUSE_GUARD_TEST): $(PRE_GEN3_REUSE_GUARD_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@

$(PRE_GEN3_REUSE_GUARD_SANITIZE): $(PRE_GEN3_REUSE_GUARD_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@

GSC_RENDER_SOURCES := tests/test_gsc_workspace_render.cpp src/Integration/Gen2/Gen2PersonalData.cpp \
    $(GEN1_POKEMON_COMMON) src/Pokemon/PersonalInfoTable.cpp
GSC_RENDER_TEST := $(HOST_BUILD)/test_gsc_workspace_render
GSC_RENDER_SANITIZE := $(HOST_BUILD)/test_gsc_workspace_render_sanitize
HOST_TESTS += $(GSC_RENDER_TEST)
HOST_SANITIZE_TESTS += $(GSC_RENDER_SANITIZE)
host-test: $(GSC_RENDER_TEST)
host-sanitize: $(GSC_RENDER_SANITIZE)
$(GSC_RENDER_TEST): $(GSC_RENDER_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Itests/host-switch -Iinclude $(GSC_RENDER_SOURCES) -o $@
$(GSC_RENDER_SANITIZE): $(GSC_RENDER_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Itests/host-switch -Iinclude $(GSC_RENDER_SOURCES) -o $@


# Shared Gen I-IV move picker presentation: numbered rows plus exact-era
# Accuracy / Power / base PP metadata from pinned retail-decomp tables.
MOVE_PICKER_PRESENTATION_SOURCES := tests/test_move_picker_presentation.cpp src/Names/MoveNames.cpp
MOVE_PICKER_PRESENTATION_TEST := $(HOST_BUILD)/test_move_picker_presentation
MOVE_PICKER_PRESENTATION_SANITIZE := $(HOST_BUILD)/test_move_picker_presentation_sanitize

HOST_TESTS += $(MOVE_PICKER_PRESENTATION_TEST)
HOST_SANITIZE_TESTS += $(MOVE_PICKER_PRESENTATION_SANITIZE)
host-test: $(MOVE_PICKER_PRESENTATION_TEST)
host-sanitize: $(MOVE_PICKER_PRESENTATION_SANITIZE)

$(MOVE_PICKER_PRESENTATION_TEST): $(MOVE_PICKER_PRESENTATION_SOURCES) include/UI/MovePickerPresentation.h include/Names/MoveBattleStats.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(MOVE_PICKER_PRESENTATION_SOURCES) -o $@

$(MOVE_PICKER_PRESENTATION_SANITIZE): $(MOVE_PICKER_PRESENTATION_SOURCES) include/UI/MovePickerPresentation.h include/Names/MoveBattleStats.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(MOVE_PICKER_PRESENTATION_SOURCES) -o $@


# Compact, shared native Nature picker used by Gen III and IV editors.
NATURE_PICKER_TEST := $(HOST_BUILD)/test_nature_picker_layout
NATURE_PICKER_SANITIZE := $(HOST_BUILD)/test_nature_picker_layout_sanitize

HOST_TESTS += $(NATURE_PICKER_TEST)
HOST_SANITIZE_TESTS += $(NATURE_PICKER_SANITIZE)
host-test: $(NATURE_PICKER_TEST)
host-sanitize: $(NATURE_PICKER_SANITIZE)

$(NATURE_PICKER_TEST): tests/test_nature_picker_layout.cpp include/UI/NaturePickerLayout.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $< -o $@

$(NATURE_PICKER_SANITIZE): tests/test_nature_picker_layout.cpp include/UI/NaturePickerLayout.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $< -o $@

# Owner's 14-point shared Pokemon editor UI contract, run for Gen I-V layouts
# (and the Gen V adapter's native data); Gen VI-IX add their generation here.
SHARED_EDITOR_CONTRACT_TEST := $(HOST_BUILD)/test_shared_editor_contract
SHARED_EDITOR_CONTRACT_SANITIZE := $(HOST_BUILD)/test_shared_editor_contract_sanitize
SHARED_EDITOR_CONTRACT_INPUTS := tests/test_shared_editor_contract.cpp \
    include/UI/SharedPokemonEditorWorkspace.h include/UI/SharedPokemonEditorContract.h \
    include/UI/NarrowPokemonActionMenu.h include/Integration/Gen5/Gen5EditorFields.h \
    include/Integration/Gen5/Gen5SharedScreenState.h include/Integration/Gen5/Gen5DisplayStats.h \
    include/Names/MoveBattleStats5.h src/UI/SharedPokemonEditorSurface.inc \
    src/UI/Gen4SharedPokemonSurface.inc src/UI/Gen5SharedReadOnlySurface.inc

HOST_TESTS += $(SHARED_EDITOR_CONTRACT_TEST)
HOST_SANITIZE_TESTS += $(SHARED_EDITOR_CONTRACT_SANITIZE)
host-test: $(SHARED_EDITOR_CONTRACT_TEST)
host-sanitize: $(SHARED_EDITOR_CONTRACT_SANITIZE)

$(SHARED_EDITOR_CONTRACT_TEST): $(SHARED_EDITOR_CONTRACT_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Itests/host-switch -Iinclude tests/test_shared_editor_contract.cpp src/Names/SpeciesNames.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp -o $@

$(SHARED_EDITOR_CONTRACT_SANITIZE): $(SHARED_EDITOR_CONTRACT_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Itests/host-switch -Iinclude tests/test_shared_editor_contract.cpp src/Names/SpeciesNames.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp -o $@

# Shared Gen I-V editor unit: Moves panel, Move chooser, picker footer, inline toggles and the
# DETAILS viewport, plus source contracts that the real Gen III-V surfaces use them.
SHARED_EDITOR_UNIT_SOURCES := tests/test_shared_editor_unit.cpp src/Names/MoveNames.cpp src/Names/LocationNames.cpp \
	src/Inventory/ItemQuantityPolicy.cpp src/Inventory/ClassicInventoryCatalog.cpp src/Names/ItemNames.cpp \
	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp
SHARED_EDITOR_UNIT_TEST := $(HOST_BUILD)/test_shared_editor_unit
SHARED_EDITOR_UNIT_SANITIZE := $(HOST_BUILD)/test_shared_editor_unit_sanitize

HOST_TESTS += $(SHARED_EDITOR_UNIT_TEST)
HOST_SANITIZE_TESTS += $(SHARED_EDITOR_UNIT_SANITIZE)
host-test: $(SHARED_EDITOR_UNIT_TEST)
host-sanitize: $(SHARED_EDITOR_UNIT_SANITIZE)

$(SHARED_EDITOR_UNIT_TEST): $(SHARED_EDITOR_UNIT_SOURCES) include/UI/SharedMovesPanel.h \
	include/UI/SharedMoveFacts.h include/UI/SharedEditorPresentation.h include/UI/SharedPokemonShell.h include/UI/SharedDataPaneLayout.h \
	include/Names/MoveTypes34.h include/Names/LocationDisplayNames.h src/UI/Gen3SharedPokemonSurface.inc src/UI/SharedPokemonEditorSurface.inc \
	src/UI/Gen4SharedPokemonSurface.inc src/UI/Gen5SharedReadOnlySurface.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(SHARED_EDITOR_UNIT_SOURCES) -o $@

$(SHARED_EDITOR_UNIT_SANITIZE): $(SHARED_EDITOR_UNIT_SOURCES) include/UI/SharedMovesPanel.h \
	include/UI/SharedMoveFacts.h include/UI/SharedEditorPresentation.h include/UI/SharedPokemonShell.h include/UI/SharedDataPaneLayout.h \
	include/Names/MoveTypes34.h include/Names/LocationDisplayNames.h src/UI/Gen3SharedPokemonSurface.inc src/UI/SharedPokemonEditorSurface.inc \
	src/UI/Gen4SharedPokemonSurface.inc src/UI/Gen5SharedReadOnlySurface.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(SHARED_EDITOR_UNIT_SOURCES) -o $@

# Round 2: one geometry per menu kind (SharedMenuGeometry.h) and one field-state contract
# (SharedFieldState.h), checked against every real Gen I-V species / item / location name with the
# bundled font's advances (NunitoMetrics.h).
SHARED_MENU_GEOMETRY_SOURCES := tests/test_shared_menu_geometry.cpp src/Names/SpeciesNames.cpp \
	src/Names/ItemNames.cpp src/Names/LocationNames.cpp
SHARED_MENU_GEOMETRY_INPUTS := $(SHARED_MENU_GEOMETRY_SOURCES) include/UI/SharedMenuGeometry.h \
	include/UI/SharedFieldState.h include/UI/NunitoMetrics.h include/Names/LocationDisplayNames.h \
	include/UI/SharedPokemonEditorContract.h
SHARED_MENU_GEOMETRY_TEST := $(HOST_BUILD)/test_shared_menu_geometry
SHARED_MENU_GEOMETRY_SANITIZE := $(HOST_BUILD)/test_shared_menu_geometry_sanitize

HOST_TESTS += $(SHARED_MENU_GEOMETRY_TEST)
HOST_SANITIZE_TESTS += $(SHARED_MENU_GEOMETRY_SANITIZE)
host-test: $(SHARED_MENU_GEOMETRY_TEST)
host-sanitize: $(SHARED_MENU_GEOMETRY_SANITIZE)

$(SHARED_MENU_GEOMETRY_TEST): $(SHARED_MENU_GEOMETRY_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(SHARED_MENU_GEOMETRY_SOURCES) -o $@

$(SHARED_MENU_GEOMETRY_SANITIZE): $(SHARED_MENU_GEOMETRY_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(SHARED_MENU_GEOMETRY_SOURCES) -o $@

# S03: Gen V Create/Edit parity -- one DETAILS label/value column for Gen III-V, no clipped
# species values, same rows as Gen III/IV, exact game in the header, per-row field state.
GEN5_PARITY_SOURCES := tests/test_gen5_create_edit_parity.cpp src/Names/SpeciesNames.cpp src/Names/LocationNames.cpp
GEN5_PARITY_INPUTS := $(GEN5_PARITY_SOURCES) include/UI/SharedEditorPresentation.h \
	include/UI/SharedMenuGeometry.h include/UI/ClassicGameContext.h include/UI/SharedPokemonShell.h \
	include/Integration/Gen5/Gen5EditorFields.h src/UI/Gen3SharedPokemonSurface.inc \
	src/UI/Gen4SharedPokemonSurface.inc src/UI/Gen5SharedReadOnlySurface.inc src/UI/SharedPokemonEditorSurface.inc
GEN5_PARITY_TEST := $(HOST_BUILD)/test_gen5_create_edit_parity
GEN5_PARITY_SANITIZE := $(HOST_BUILD)/test_gen5_create_edit_parity_sanitize

HOST_TESTS += $(GEN5_PARITY_TEST)
HOST_SANITIZE_TESTS += $(GEN5_PARITY_SANITIZE)
host-test: $(GEN5_PARITY_TEST)
host-sanitize: $(GEN5_PARITY_SANITIZE)

$(GEN5_PARITY_TEST): $(GEN5_PARITY_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN5_PARITY_SOURCES) -o $@

$(GEN5_PARITY_SANITIZE): $(GEN5_PARITY_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN5_PARITY_SOURCES) -o $@

# R2-BUG-012: one boxed-row style for every interactive Gen I-V modal (SharedBoxedRows.h).
SHARED_BOXED_ROWS_INPUTS := tests/test_shared_boxed_rows.cpp include/UI/SharedBoxedRows.h include/UI/ScreenChrome.h \
	src/UI/SharedPokemonEditorSurface.inc src/UI/Gen3SharedPokemonSurface.inc src/UI/Gen5SharedReadOnlySurface.inc \
	include/UI/SharedHeldItemPickerPresentation.h
SHARED_BOXED_ROWS_TEST := $(HOST_BUILD)/test_shared_boxed_rows
SHARED_BOXED_ROWS_SANITIZE := $(HOST_BUILD)/test_shared_boxed_rows_sanitize

HOST_TESTS += $(SHARED_BOXED_ROWS_TEST)
HOST_SANITIZE_TESTS += $(SHARED_BOXED_ROWS_SANITIZE)
host-test: $(SHARED_BOXED_ROWS_TEST)
host-sanitize: $(SHARED_BOXED_ROWS_SANITIZE)

$(SHARED_BOXED_ROWS_TEST): $(SHARED_BOXED_ROWS_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_shared_boxed_rows.cpp -o $@

$(SHARED_BOXED_ROWS_SANITIZE): $(SHARED_BOXED_ROWS_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_shared_boxed_rows.cpp -o $@

# Phase 6: Y re-rolls IVs/EVs through the real PK3 / PK4 / PK5 record code; Shiny, Gender,
# Nature and Ability never change (shiny/non-shiny drafts, gender-ratio edge cases).
SHARED_REROLL_SOURCES := tests/test_shared_stat_reroll.cpp \
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
SHARED_REROLL_TEST := $(HOST_BUILD)/test_shared_stat_reroll
SHARED_REROLL_SANITIZE := $(HOST_BUILD)/test_shared_stat_reroll_sanitize

HOST_TESTS += $(SHARED_REROLL_TEST)
HOST_SANITIZE_TESTS += $(SHARED_REROLL_SANITIZE)
host-test: $(SHARED_REROLL_TEST)
host-sanitize: $(SHARED_REROLL_SANITIZE)

$(SHARED_REROLL_TEST): $(SHARED_REROLL_SOURCES) include/UI/SharedStatReroll.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(SHARED_REROLL_SOURCES) -o $@

$(SHARED_REROLL_SANITIZE): $(SHARED_REROLL_SOURCES) include/UI/SharedStatReroll.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(SHARED_REROLL_SOURCES) -o $@

# Phase 2: Gen IV / V bags query QuantityPolicy (one per-item rule), capped by the native writer.
SHARED_BAG_QUANTITY_SOURCES := tests/test_shared_bag_quantity.cpp src/Inventory/ItemQuantityPolicy.cpp \
	src/Inventory/ClassicInventoryCatalog.cpp src/Names/ItemNames.cpp src/Names/MoveNames.cpp \
	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp
SHARED_BAG_QUANTITY_TEST := $(HOST_BUILD)/test_shared_bag_quantity
SHARED_BAG_QUANTITY_SANITIZE := $(HOST_BUILD)/test_shared_bag_quantity_sanitize

HOST_TESTS += $(SHARED_BAG_QUANTITY_TEST)
HOST_SANITIZE_TESTS += $(SHARED_BAG_QUANTITY_SANITIZE)
host-test: $(SHARED_BAG_QUANTITY_TEST)
host-sanitize: $(SHARED_BAG_QUANTITY_SANITIZE)

$(SHARED_BAG_QUANTITY_TEST): $(SHARED_BAG_QUANTITY_SOURCES) include/Inventory/ItemQuantityPolicy.h \
	src/UI/Gen4SharedPokemonSurface.inc src/UI/Gen5SharedReadOnlySurface.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(SHARED_BAG_QUANTITY_SOURCES) -o $@

$(SHARED_BAG_QUANTITY_SANITIZE): $(SHARED_BAG_QUANTITY_SOURCES) include/Inventory/ItemQuantityPolicy.h \
	src/UI/Gen4SharedPokemonSurface.inc src/UI/Gen5SharedReadOnlySurface.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(SHARED_BAG_QUANTITY_SOURCES) -o $@

# QA Round 3 Part 9 item 8: the Gen IV Storage save pane uses the shared Y carry (pickup,
# drop and drawn carry stay in that pane; Y tap / hold multi-select as in Boxes).
GEN4_STORAGE_CARRY_INPUTS := tests/test_gen4_storage_carry.cpp include/UI/ClassicCarryGrid.h \
	include/UI/ClassicPackedMultiSelect.h src/UI/ClassicPackedMoveOverlay.inc src/UI/Panels/StoragePanel.cpp \
	src/UI/TrainerViewScreenCompositeOverlay.cpp
GEN4_STORAGE_CARRY_TEST := $(HOST_BUILD)/test_gen4_storage_carry
GEN4_STORAGE_CARRY_SANITIZE := $(HOST_BUILD)/test_gen4_storage_carry_sanitize

HOST_TESTS += $(GEN4_STORAGE_CARRY_TEST)
HOST_SANITIZE_TESTS += $(GEN4_STORAGE_CARRY_SANITIZE)
host-test: $(GEN4_STORAGE_CARRY_TEST)
host-sanitize: $(GEN4_STORAGE_CARRY_SANITIZE)

$(GEN4_STORAGE_CARRY_TEST): $(GEN4_STORAGE_CARRY_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_gen4_storage_carry.cpp -o $@

$(GEN4_STORAGE_CARRY_SANITIZE): $(GEN4_STORAGE_CARRY_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_gen4_storage_carry.cpp -o $@

# Owner Round 4 correction: keep green/red Nature effect hints in View/Edit STATS and Nature picker.
STATS_LABEL_NEUTRAL_TEST := $(HOST_BUILD)/test_stats_label_neutral_contract
STATS_LABEL_NEUTRAL_SANITIZE := $(HOST_BUILD)/test_stats_label_neutral_contract_sanitize
HOST_TESTS += $(STATS_LABEL_NEUTRAL_TEST)
HOST_SANITIZE_TESTS += $(STATS_LABEL_NEUTRAL_SANITIZE)
host-test: $(STATS_LABEL_NEUTRAL_TEST)
host-sanitize: $(STATS_LABEL_NEUTRAL_SANITIZE)
$(STATS_LABEL_NEUTRAL_TEST): tests/test_stats_label_neutral_contract.cpp src/UI/SharedPokemonEditorSurface.inc include/UI/NatureStatEffect.h include/UI/NatureStatPresentation.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $< -o $@
$(STATS_LABEL_NEUTRAL_SANITIZE): tests/test_stats_label_neutral_contract.cpp src/UI/SharedPokemonEditorSurface.inc include/UI/NatureStatEffect.h include/UI/NatureStatPresentation.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $< -o $@

# P3 (Round 5): the classic Trainer page's Play Time / Gender rows (native staged writers only).
TRAINER_PT_GENDER_INPUTS := tests/test_trainer_playtime_gender_rows.cpp include/UI/ClassicTrainerSession.h \
	include/UI/ClassicTrainerSessionInput.h src/UI/TrainerViewScreenBase.inc src/UI/Gen2TrainerSessionOverlay.inc \
	src/UI/Gen3SharedPokemonSurface.inc src/UI/Gen4SharedPokemonSurface.inc
TRAINER_PT_GENDER_TEST := $(HOST_BUILD)/test_trainer_playtime_gender_rows
TRAINER_PT_GENDER_SANITIZE := $(HOST_BUILD)/test_trainer_playtime_gender_rows_sanitize
HOST_TESTS += $(TRAINER_PT_GENDER_TEST)
HOST_SANITIZE_TESTS += $(TRAINER_PT_GENDER_SANITIZE)
host-test: $(TRAINER_PT_GENDER_TEST)
host-sanitize: $(TRAINER_PT_GENDER_SANITIZE)
$(TRAINER_PT_GENDER_TEST): $(TRAINER_PT_GENDER_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $< -o $@
$(TRAINER_PT_GENDER_SANITIZE): $(TRAINER_PT_GENDER_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $< -o $@
# ---- END original Makefile.host.shared-editor ----

# ---- BEGIN original Makefile.host.hardware-fixes ----
# Hardware-derived classic presentation regressions added after physical Switch retest.
# Reuse the proven Gen I editor source graph and the Gen II personal table so this is
# executable data coverage, not only source-text coverage.
CLASSIC_PICKER_TYPE_SOURCES := tests/test_classic_picker_type_contract.cpp \
	$(GEN1_POKEMON_COMMON) \
	src/Integration/Gen2/Gen2PersonalData.cpp \
	src/Pokemon/PersonalInfoTable.cpp
CLASSIC_PICKER_TYPE_TEST := $(HOST_BUILD)/test_classic_picker_type_contract
CLASSIC_PICKER_TYPE_SANITIZE := $(HOST_BUILD)/test_classic_picker_type_contract_sanitize

HOST_TESTS += $(CLASSIC_PICKER_TYPE_TEST)
HOST_SANITIZE_TESTS += $(CLASSIC_PICKER_TYPE_SANITIZE)
host-test: $(CLASSIC_PICKER_TYPE_TEST)
host-sanitize: $(CLASSIC_PICKER_TYPE_SANITIZE)

$(CLASSIC_PICKER_TYPE_TEST): $(CLASSIC_PICKER_TYPE_SOURCES) src/Integration/Gen1/Gen1PersonalData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(CLASSIC_PICKER_TYPE_SOURCES) -o $@

$(CLASSIC_PICKER_TYPE_SANITIZE): $(CLASSIC_PICKER_TYPE_SOURCES) src/Integration/Gen1/Gen1PersonalData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(CLASSIC_PICKER_TYPE_SOURCES) -o $@

ENCOUNTER_GUARDRAILS_SOURCES := tests/test_encounter_guardrails.cpp src/Names/LocationNames.cpp
ENCOUNTER_GUARDRAILS_TEST := $(HOST_BUILD)/test_encounter_guardrails
ENCOUNTER_GUARDRAILS_SANITIZE := $(HOST_BUILD)/test_encounter_guardrails_sanitize

HOST_TESTS += $(ENCOUNTER_GUARDRAILS_TEST)
HOST_SANITIZE_TESTS += $(ENCOUNTER_GUARDRAILS_SANITIZE)
host-test: $(ENCOUNTER_GUARDRAILS_TEST)
host-sanitize: $(ENCOUNTER_GUARDRAILS_SANITIZE)

$(ENCOUNTER_GUARDRAILS_TEST): $(ENCOUNTER_GUARDRAILS_SOURCES) include/Integration/Encounter/EncounterGuardrailsData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(ENCOUNTER_GUARDRAILS_SOURCES) -o $@

$(ENCOUNTER_GUARDRAILS_SANITIZE): $(ENCOUNTER_GUARDRAILS_SOURCES) include/Integration/Encounter/EncounterGuardrailsData.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(ENCOUNTER_GUARDRAILS_SOURCES) -o $@

# Audit-phase custody regression. This source-contract test intentionally failed on the
# accepted pre-audit checkpoint because returnHeldToOrigin() could clear an unplaced mon.
STORAGE_CUSTODY_TEST := $(HOST_BUILD)/test_storage_custody_contract
STORAGE_CUSTODY_SANITIZE := $(HOST_BUILD)/test_storage_custody_contract_sanitize

HOST_TESTS += $(STORAGE_CUSTODY_TEST)
HOST_SANITIZE_TESTS += $(STORAGE_CUSTODY_SANITIZE)
host-test: $(STORAGE_CUSTODY_TEST)
host-sanitize: $(STORAGE_CUSTODY_SANITIZE)

$(STORAGE_CUSTODY_TEST): tests/test_storage_custody_contract.cpp src/UI/TrainerViewScreenBase.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_storage_custody_contract.cpp -o $@

$(STORAGE_CUSTODY_SANITIZE): tests/test_storage_custody_contract.cpp src/UI/TrainerViewScreenBase.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_storage_custody_contract.cpp -o $@

# AUDIT-037: group pickup must roll backend custody back if presentation refresh fails.
CLASSIC_PACKED_RECOVERY_TEST := $(HOST_BUILD)/test_classic_packed_move_recovery_contract
CLASSIC_PACKED_RECOVERY_SANITIZE := $(HOST_BUILD)/test_classic_packed_move_recovery_contract_sanitize

HOST_TESTS += $(CLASSIC_PACKED_RECOVERY_TEST)
HOST_SANITIZE_TESTS += $(CLASSIC_PACKED_RECOVERY_SANITIZE)
host-test: $(CLASSIC_PACKED_RECOVERY_TEST)
host-sanitize: $(CLASSIC_PACKED_RECOVERY_SANITIZE)

$(CLASSIC_PACKED_RECOVERY_TEST): tests/test_classic_packed_move_recovery_contract.cpp src/UI/ClassicPackedMoveOverlay.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) tests/test_classic_packed_move_recovery_contract.cpp -o $@

$(CLASSIC_PACKED_RECOVERY_SANITIZE): tests/test_classic_packed_move_recovery_contract.cpp src/UI/ClassicPackedMoveOverlay.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) tests/test_classic_packed_move_recovery_contract.cpp -o $@

# Audit-phase malformed-input regression: BDSP must reject a short flat blob before
# any fixed-offset parser touches PARTY_COUNT / MyStatus / hash fields.
BDSP_LAYOUT_GUARD_TEST := $(HOST_BUILD)/test_bdsp_layout_guard
BDSP_LAYOUT_GUARD_SANITIZE := $(HOST_BUILD)/test_bdsp_layout_guard_sanitize

HOST_TESTS += $(BDSP_LAYOUT_GUARD_TEST)
HOST_SANITIZE_TESTS += $(BDSP_LAYOUT_GUARD_SANITIZE)
host-test: $(BDSP_LAYOUT_GUARD_TEST)
host-sanitize: $(BDSP_LAYOUT_GUARD_SANITIZE)

$(BDSP_LAYOUT_GUARD_TEST): tests/test_bdsp_layout_guard.cpp include/Save/BDSPReadValidation.h src/Trainer/Trainer8BDSP.cpp src/Utils/MD5.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_bdsp_layout_guard.cpp src/Utils/MD5.cpp -o $@

$(BDSP_LAYOUT_GUARD_SANITIZE): tests/test_bdsp_layout_guard.cpp include/Save/BDSPReadValidation.h src/Trainer/Trainer8BDSP.cpp src/Utils/MD5.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_bdsp_layout_guard.cpp src/Utils/MD5.cpp -o $@

# Audit-phase Bank compatibility regression: a newer/larger Bank may be inspected
# but must become non-writable; invalid box-count headers fail closed.
BANK_FORMAT_POLICY_TEST := $(HOST_BUILD)/test_bank_format_policy
BANK_FORMAT_POLICY_SANITIZE := $(HOST_BUILD)/test_bank_format_policy_sanitize

HOST_TESTS += $(BANK_FORMAT_POLICY_TEST)
HOST_SANITIZE_TESTS += $(BANK_FORMAT_POLICY_SANITIZE)
host-test: $(BANK_FORMAT_POLICY_TEST)
host-sanitize: $(BANK_FORMAT_POLICY_SANITIZE)

$(BANK_FORMAT_POLICY_TEST): tests/test_bank_format_policy.cpp include/Trainer/BankFormatPolicy.h src/Trainer/Bank.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_bank_format_policy.cpp -o $@

$(BANK_FORMAT_POLICY_SANITIZE): tests/test_bank_format_policy.cpp include/Trainer/BankFormatPolicy.h src/Trainer/Bank.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_bank_format_policy.cpp -o $@

# Audit-phase recovery evidence regression: preserving a new unreadable Bank must never
# delete a previously preserved casualty.
BANK_RECOVERY_CONTRACT_TEST := $(HOST_BUILD)/test_bank_recovery_contract
BANK_RECOVERY_CONTRACT_SANITIZE := $(HOST_BUILD)/test_bank_recovery_contract_sanitize

HOST_TESTS += $(BANK_RECOVERY_CONTRACT_TEST)
HOST_SANITIZE_TESTS += $(BANK_RECOVERY_CONTRACT_SANITIZE)
host-test: $(BANK_RECOVERY_CONTRACT_TEST)
host-sanitize: $(BANK_RECOVERY_CONTRACT_SANITIZE)

$(BANK_RECOVERY_CONTRACT_TEST): tests/test_bank_recovery_contract.cpp src/Trainer/Bank.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_bank_recovery_contract.cpp -o $@

$(BANK_RECOVERY_CONTRACT_SANITIZE): tests/test_bank_recovery_contract.cpp src/Trainer/Bank.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_bank_recovery_contract.cpp -o $@

# Audit-phase durable persistence primitive: exercise real host file replacement and
# rollback before Bank / mutable save writers depend on it.
DURABLE_FILE_TEST := $(HOST_BUILD)/test_durable_file
DURABLE_FILE_SANITIZE := $(HOST_BUILD)/test_durable_file_sanitize
DURABLE_FILE_SOURCES := tests/test_durable_file.cpp src/Utils/DurableFile.cpp

HOST_TESTS += $(DURABLE_FILE_TEST)
HOST_SANITIZE_TESTS += $(DURABLE_FILE_SANITIZE)
host-test: $(DURABLE_FILE_TEST)
host-sanitize: $(DURABLE_FILE_SANITIZE)

$(DURABLE_FILE_TEST): $(DURABLE_FILE_SOURCES) include/Utils/DurableFile.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(DURABLE_FILE_SOURCES) -o $@

$(DURABLE_FILE_SANITIZE): $(DURABLE_FILE_SOURCES) include/Utils/DurableFile.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(DURABLE_FILE_SOURCES) -o $@


# A09: mutable backup workspaces must not overwrite their authoritative files in place.
BACKUP_WORKSPACE_DURABILITY_TEST := $(HOST_BUILD)/test_backup_workspace_durability
BACKUP_WORKSPACE_DURABILITY_SANITIZE := $(HOST_BUILD)/test_backup_workspace_durability_sanitize

HOST_TESTS += $(BACKUP_WORKSPACE_DURABILITY_TEST)
HOST_SANITIZE_TESTS += $(BACKUP_WORKSPACE_DURABILITY_SANITIZE)
host-test: $(BACKUP_WORKSPACE_DURABILITY_TEST)
host-sanitize: $(BACKUP_WORKSPACE_DURABILITY_SANITIZE)

$(BACKUP_WORKSPACE_DURABILITY_TEST): tests/test_backup_workspace_durability.cpp src/Save/GetSaveFileContents.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_backup_workspace_durability.cpp -o $@

$(BACKUP_WORKSPACE_DURABILITY_SANITIZE): tests/test_backup_workspace_durability.cpp src/Save/GetSaveFileContents.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_backup_workspace_durability.cpp -o $@

# A08: mutable backup workspaces are namespaced by Switch profile + exact game identity.
BACKUP_NAMESPACE_TEST := $(HOST_BUILD)/test_backup_namespace_contract
BACKUP_NAMESPACE_SANITIZE := $(HOST_BUILD)/test_backup_namespace_contract_sanitize
BACKUP_NAMESPACE_SOURCES := tests/test_backup_namespace_contract.cpp src/Games/GameIdentity.cpp

HOST_TESTS += $(BACKUP_NAMESPACE_TEST)
HOST_SANITIZE_TESTS += $(BACKUP_NAMESPACE_SANITIZE)
host-test: $(BACKUP_NAMESPACE_TEST)
host-sanitize: $(BACKUP_NAMESPACE_SANITIZE)

$(BACKUP_NAMESPACE_TEST): $(BACKUP_NAMESPACE_SOURCES) include/Utils/PokeBankPaths.h src/Utils/FileUtilities.cpp src/UI/BackupSelectionScreen.cpp src/UI/UI.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(BACKUP_NAMESPACE_SOURCES) -o $@

$(BACKUP_NAMESPACE_SANITIZE): $(BACKUP_NAMESPACE_SOURCES) include/Utils/PokeBankPaths.h src/Utils/FileUtilities.cpp src/UI/BackupSelectionScreen.cpp src/UI/UI.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(BACKUP_NAMESPACE_SOURCES) -o $@


# A03: conversion/save-normalization must prepare a separate destination
# candidate and keep the carried original immutable until slot commit.
STORAGE_CONVERSION_CUSTODY_TEST := $(HOST_BUILD)/test_storage_conversion_custody_contract
STORAGE_CONVERSION_CUSTODY_SANITIZE := $(HOST_BUILD)/test_storage_conversion_custody_contract_sanitize

HOST_TESTS += $(STORAGE_CONVERSION_CUSTODY_TEST)
HOST_SANITIZE_TESTS += $(STORAGE_CONVERSION_CUSTODY_SANITIZE)
host-test: $(STORAGE_CONVERSION_CUSTODY_TEST)
host-sanitize: $(STORAGE_CONVERSION_CUSTODY_SANITIZE)

$(STORAGE_CONVERSION_CUSTODY_TEST): tests/test_storage_conversion_custody_contract.cpp include/UI/StoragePlacementCommit.h include/UI/TrainerViewScreenBase.h src/UI/TrainerViewScreenBase.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_storage_conversion_custody_contract.cpp -o $@

$(STORAGE_CONVERSION_CUSTODY_SANITIZE): tests/test_storage_conversion_custody_contract.cpp include/UI/StoragePlacementCommit.h include/UI/TrainerViewScreenBase.h src/UI/TrainerViewScreenBase.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_storage_conversion_custody_contract.cpp -o $@


# A04a: durable destination-first Move transaction journal/recovery core.
MOVE_TRANSACTION_TEST := $(HOST_BUILD)/test_move_transaction_journal
MOVE_TRANSACTION_SANITIZE := $(HOST_BUILD)/test_move_transaction_journal_sanitize
MOVE_TRANSACTION_SOURCES := tests/test_move_transaction_journal.cpp src/Utils/MoveTransaction.cpp src/Utils/DurableFile.cpp

HOST_TESTS += $(MOVE_TRANSACTION_TEST)
HOST_SANITIZE_TESTS += $(MOVE_TRANSACTION_SANITIZE)
host-test: $(MOVE_TRANSACTION_TEST)
host-sanitize: $(MOVE_TRANSACTION_SANITIZE)

$(MOVE_TRANSACTION_TEST): $(MOVE_TRANSACTION_SOURCES) include/Utils/MoveTransaction.h include/Utils/DurableFile.h include/Utils/PokeBankPaths.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(MOVE_TRANSACTION_SOURCES) -o $@

$(MOVE_TRANSACTION_SANITIZE): $(MOVE_TRANSACTION_SOURCES) include/Utils/MoveTransaction.h include/Utils/DurableFile.h include/Utils/PokeBankPaths.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(MOVE_TRANSACTION_SOURCES) -o $@


# A04b: production Bank <-> PokeBank-owned single-file workspace true-Move integration.
MOVE_TRANSACTION_PRODUCTION_TEST := $(HOST_BUILD)/test_move_transaction_production
MOVE_TRANSACTION_PRODUCTION_SANITIZE := $(HOST_BUILD)/test_move_transaction_production_sanitize
MOVE_TRANSACTION_PRODUCTION_SOURCES := tests/test_move_transaction_production.cpp src/Games/GameIdentity.cpp src/Utils/MoveTransaction.cpp src/Utils/DurableFile.cpp

HOST_TESTS += $(MOVE_TRANSACTION_PRODUCTION_TEST)
HOST_SANITIZE_TESTS += $(MOVE_TRANSACTION_PRODUCTION_SANITIZE)
host-test: $(MOVE_TRANSACTION_PRODUCTION_TEST)
host-sanitize: $(MOVE_TRANSACTION_PRODUCTION_SANITIZE)

$(MOVE_TRANSACTION_PRODUCTION_TEST): $(MOVE_TRANSACTION_PRODUCTION_SOURCES) include/Utils/MoveTransactionProduction.h include/UI/TrainerViewScreenBase.h src/UI/TrainerViewScreenBase.inc src/UI/UI.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(MOVE_TRANSACTION_PRODUCTION_SOURCES) -o $@

$(MOVE_TRANSACTION_PRODUCTION_SANITIZE): $(MOVE_TRANSACTION_PRODUCTION_SOURCES) include/Utils/MoveTransactionProduction.h include/UI/TrainerViewScreenBase.h src/UI/TrainerViewScreenBase.inc src/UI/UI.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(MOVE_TRANSACTION_PRODUCTION_SOURCES) -o $@

# Physical durability gate: restart-safe DurableFile promotion recovery plus the complete
# Move-engine interruption/idempotence matrix used before the Switch FAT32 test.
MOVE_TRANSACTION_FAULT_MATRIX_TEST := $(HOST_BUILD)/test_move_transaction_fault_matrix
MOVE_TRANSACTION_FAULT_MATRIX_SANITIZE := $(HOST_BUILD)/test_move_transaction_fault_matrix_sanitize
MOVE_TRANSACTION_FAULT_MATRIX_SOURCES := tests/test_move_transaction_fault_matrix.cpp src/Utils/MoveTransaction.cpp src/Utils/DurableFile.cpp

HOST_TESTS += $(MOVE_TRANSACTION_FAULT_MATRIX_TEST)
HOST_SANITIZE_TESTS += $(MOVE_TRANSACTION_FAULT_MATRIX_SANITIZE)
host-test: $(MOVE_TRANSACTION_FAULT_MATRIX_TEST)
host-sanitize: $(MOVE_TRANSACTION_FAULT_MATRIX_SANITIZE)

$(MOVE_TRANSACTION_FAULT_MATRIX_TEST): $(MOVE_TRANSACTION_FAULT_MATRIX_SOURCES) include/Utils/MoveTransaction.h include/Utils/DurableFile.h include/Utils/PokeBankPaths.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(MOVE_TRANSACTION_FAULT_MATRIX_SOURCES) -o $@

$(MOVE_TRANSACTION_FAULT_MATRIX_SANITIZE): $(MOVE_TRANSACTION_FAULT_MATRIX_SOURCES) include/Utils/MoveTransaction.h include/Utils/DurableFile.h include/Utils/PokeBankPaths.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(MOVE_TRANSACTION_FAULT_MATRIX_SOURCES) -o $@

# F07 / N01: Gen III PID search exhaustion must fail closed. Cross-game true Move
# remains locked until conversion fidelity is proven route-by-route.
GEN3_PID_SEARCH_TEST := $(HOST_BUILD)/test_gen3_pid_search_fail_closed
GEN3_PID_SEARCH_SANITIZE := $(HOST_BUILD)/test_gen3_pid_search_fail_closed_sanitize
GEN3_PID_SEARCH_SOURCES := tests/test_gen3_pid_search_fail_closed.cpp
HOST_TESTS += $(GEN3_PID_SEARCH_TEST)
HOST_SANITIZE_TESTS += $(GEN3_PID_SEARCH_SANITIZE)
host-test: $(GEN3_PID_SEARCH_TEST)
host-sanitize: $(GEN3_PID_SEARCH_SANITIZE)

$(GEN3_PID_SEARCH_TEST): $(GEN3_PID_SEARCH_SOURCES) include/Conversion/Gen3PidSearch.h src/Conversion/Convert.cpp include/Conversion/Convert.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN3_PID_SEARCH_SOURCES) -o $@

$(GEN3_PID_SEARCH_SANITIZE): $(GEN3_PID_SEARCH_SOURCES) include/Conversion/Gen3PidSearch.h src/Conversion/Convert.cpp include/Conversion/Convert.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN3_PID_SEARCH_SOURCES) -o $@

# F05-F13 conversion fidelity / generated golden fixture policy.
CONVERSION_FIDELITY_GOLDEN_TEST := $(HOST_BUILD)/test_conversion_fidelity_golden
CONVERSION_FIDELITY_GOLDEN_SANITIZE := $(HOST_BUILD)/test_conversion_fidelity_golden_sanitize
CONVERSION_FIDELITY_GOLDEN_SOURCES := tests/test_conversion_fidelity_golden.cpp src/Utils/SHA256.cpp
HOST_TESTS += $(CONVERSION_FIDELITY_GOLDEN_TEST)
HOST_SANITIZE_TESTS += $(CONVERSION_FIDELITY_GOLDEN_SANITIZE)
host-test: $(CONVERSION_FIDELITY_GOLDEN_TEST)
host-sanitize: $(CONVERSION_FIDELITY_GOLDEN_SANITIZE)

$(CONVERSION_FIDELITY_GOLDEN_TEST): $(CONVERSION_FIDELITY_GOLDEN_SOURCES) include/Conversion/Fidelity.h include/Conversion/Gen3PidSearch.h include/Conversion/Convert.h src/Conversion/Convert.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(CONVERSION_FIDELITY_GOLDEN_SOURCES) -o $@

$(CONVERSION_FIDELITY_GOLDEN_SANITIZE): $(CONVERSION_FIDELITY_GOLDEN_SOURCES) include/Conversion/Fidelity.h include/Conversion/Gen3PidSearch.h include/Conversion/Convert.h src/Conversion/Convert.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(CONVERSION_FIDELITY_GOLDEN_SOURCES) -o $@

# F05-F13: production converter entity goldens. These exercise native encrypted bytes,
# source immutability, conversion, destination serialization, checksum and reparse.
CONVERSION_ENTITY_GOLDEN_TEST := $(HOST_BUILD)/test_conversion_entity_golden
CONVERSION_ENTITY_GOLDEN_SANITIZE := $(HOST_BUILD)/test_conversion_entity_golden_sanitize
CONVERSION_ENTITY_GOLDEN_SOURCES := tests/test_conversion_entity_golden.cpp \
	src/Conversion/Convert.cpp \
	src/Pokemon/Pokemon3FRLG.cpp src/Pokemon/Pokemon7LGPE.cpp \
	src/Pokemon/Pokemon8SWSH.cpp src/Pokemon/Pokemon8BDSP.cpp src/Pokemon/Pokemon8LA.cpp \
	src/Pokemon/Pokemon9SV.cpp src/Pokemon/Pokemon9LZA.cpp \
	src/Pokemon/BaseStatsGen7.cpp src/Pokemon/BaseStatsGen89.cpp src/Pokemon/PersonalInfoTable.cpp \
	src/Pokemon/PokemonTypes.cpp src/Pokemon/LearnsetTable.cpp src/Pokemon/Experience.cpp src/Pokemon/FormInfo.cpp \
	src/Encryption/Encryption.cpp src/Save/Block.cpp src/Enums/SCTypeCode.cpp \
	src/Encryption/Encryption3FRLG.cpp src/Encryption/Encryption7LGPE.cpp \
	src/Encryption/Encryption8SWSH.cpp src/Encryption/Encryption8BDSP.cpp src/Encryption/Encryption8LA.cpp \
	src/Encryption/Encryption9SV.cpp src/Encryption/Encryption9LZA.cpp \
	src/Names/ItemNames.cpp src/Names/ItemPresence.cpp src/Names/MoveInfo.cpp src/Names/SpeciesNames.cpp \
	src/Names/NatureNames.cpp src/Names/AbilityNames.cpp \
	src/Utils/HelperUtilities.cpp src/Utils/StringHelpers.cpp src/Utils/SHA256.cpp
HOST_TESTS += $(CONVERSION_ENTITY_GOLDEN_TEST)
HOST_SANITIZE_TESTS += $(CONVERSION_ENTITY_GOLDEN_SANITIZE)
host-test: $(CONVERSION_ENTITY_GOLDEN_TEST)
host-sanitize: $(CONVERSION_ENTITY_GOLDEN_SANITIZE)

$(CONVERSION_ENTITY_GOLDEN_TEST): $(CONVERSION_ENTITY_GOLDEN_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(CONVERSION_ENTITY_GOLDEN_SOURCES) -o $@

$(CONVERSION_ENTITY_GOLDEN_SANITIZE): $(CONVERSION_ENTITY_GOLDEN_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(CONVERSION_ENTITY_GOLDEN_SOURCES) -o $@


# F13 persisted conversion evidence: exact loss acknowledgement, provenance binding,
# cross-game retirement gating, corruption/version handling and recovery semantics.
CONVERSION_EVIDENCE_TEST := $(HOST_BUILD)/test_conversion_evidence
CONVERSION_EVIDENCE_SANITIZE := $(HOST_BUILD)/test_conversion_evidence_sanitize
CONVERSION_EVIDENCE_SOURCES := tests/test_conversion_evidence.cpp \
	src/Conversion/RouteEvidence.cpp src/Utils/MoveTransaction.cpp src/Utils/DurableFile.cpp

HOST_TESTS += $(CONVERSION_EVIDENCE_TEST)
HOST_SANITIZE_TESTS += $(CONVERSION_EVIDENCE_SANITIZE)
host-test: $(CONVERSION_EVIDENCE_TEST)
host-sanitize: $(CONVERSION_EVIDENCE_SANITIZE)

$(CONVERSION_EVIDENCE_TEST): $(CONVERSION_EVIDENCE_SOURCES) include/Conversion/RouteEvidence.h include/Conversion/Fidelity.h include/Utils/MoveTransaction.h include/Utils/DurableFile.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(CONVERSION_EVIDENCE_SOURCES) -o $@

$(CONVERSION_EVIDENCE_SANITIZE): $(CONVERSION_EVIDENCE_SOURCES) include/Conversion/RouteEvidence.h include/Conversion/Fidelity.h include/Utils/MoveTransaction.h include/Utils/DurableFile.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(CONVERSION_EVIDENCE_SOURCES) -o $@


# Hardware-smoke follow-up: Gen I / II / III species selection must share one quiet modal shell.
SHARED_SPECIES_PICKER_SURFACE_TEST := $(HOST_BUILD)/test_shared_species_picker_surface_contract
SHARED_SPECIES_PICKER_SURFACE_SANITIZE := $(HOST_BUILD)/test_shared_species_picker_surface_contract_sanitize
SHARED_SPECIES_PICKER_SURFACE_SOURCES := tests/test_shared_species_picker_surface_contract.cpp

HOST_TESTS += $(SHARED_SPECIES_PICKER_SURFACE_TEST)
HOST_SANITIZE_TESTS += $(SHARED_SPECIES_PICKER_SURFACE_SANITIZE)
host-test: $(SHARED_SPECIES_PICKER_SURFACE_TEST)
host-sanitize: $(SHARED_SPECIES_PICKER_SURFACE_SANITIZE)

$(SHARED_SPECIES_PICKER_SURFACE_TEST): $(SHARED_SPECIES_PICKER_SURFACE_SOURCES) \
		include/UI/SharedSpeciesPicker.h src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc \
		src/UI/Gen2PokemonPickerOverlay.inc src/UI/Gen2SharedSurfaceParity.inc \
		src/UI/Gen3SharedPokemonSurface.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(SHARED_SPECIES_PICKER_SURFACE_SOURCES) -o $@

$(SHARED_SPECIES_PICKER_SURFACE_SANITIZE): $(SHARED_SPECIES_PICKER_SURFACE_SOURCES) \
		include/UI/SharedSpeciesPicker.h src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc \
		src/UI/Gen2PokemonPickerOverlay.inc src/UI/Gen3SharedPokemonSurface.inc
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(SHARED_SPECIES_PICKER_SURFACE_SOURCES) -o $@
# ---- END original Makefile.host.hardware-fixes ----
