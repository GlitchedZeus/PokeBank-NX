# Generation II GSC permanent host gates.
GEN2_ADAPTER_SOURCES := tests/test_gsc_gen2_adapter.cpp \
	src/Integration/Gen2/Gen2ReadOnlySave.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp
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
	src/Integration/Gen2/Gen2PersonalData.cpp src/Pokemon/Experience.cpp src/Pokemon/BaseStatsGen89.cpp \
	src/Names/SpeciesNames.cpp src/Names/ItemNames.cpp src/Names/NatureNames.cpp \
	src/Names/AbilityNames.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp
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
	src/Pokemon/Experience.cpp src/Names/SpeciesNames.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp src/Utils/SHA256.cpp

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

GSC_EVOLVED_WILD_SOURCES := tests/test_gen2_crystal_evolved_wild.cpp
GSC_EVOLVED_WILD_HEADERS := include/Legality/Gen2CrystalEvolvedWildEvidence.h \
	include/Legality/Gen2WildEncounter.h \
	include/Legality/Gen2WildEncounterData.inc
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

HOST_TESTS += $(HOST_BUILD)/test_gen2_crystal_evolved_wild
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_crystal_evolved_wild_sanitize
host-test: $(HOST_BUILD)/test_gen2_crystal_evolved_wild
host-sanitize: $(HOST_BUILD)/test_gen2_crystal_evolved_wild_sanitize

$(HOST_BUILD)/test_gen2_crystal_evolved_wild: $(GSC_EVOLVED_WILD_SOURCES) $(GSC_EVOLVED_WILD_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GSC_EVOLVED_WILD_SOURCES) -o $@

$(HOST_BUILD)/test_gen2_crystal_evolved_wild_sanitize: $(GSC_EVOLVED_WILD_SOURCES) $(GSC_EVOLVED_WILD_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GSC_EVOLVED_WILD_SOURCES) -o $@

# Production report test via immutable validated PK2 record wrapper.
GSC_EVOLVED_WILD_NATIVE_SOURCES := tests/test_gen2_crystal_evolved_native.cpp \
	$(filter-out tests/test_gen3_legality_context.cpp,$(GEN3_LEGALITY_CONTEXT_SOURCES))

HOST_TESTS += $(HOST_BUILD)/test_gen2_crystal_evolved_native
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_crystal_evolved_native_sanitize
host-test: $(HOST_BUILD)/test_gen2_crystal_evolved_native
host-sanitize: $(HOST_BUILD)/test_gen2_crystal_evolved_native_sanitize

$(HOST_BUILD)/test_gen2_crystal_evolved_native: $(GSC_EVOLVED_WILD_NATIVE_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $^ -o $@

$(HOST_BUILD)/test_gen2_crystal_evolved_native_sanitize: $(GSC_EVOLVED_WILD_NATIVE_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $^ -o $@

# Raw international Crystal .sav -> verified strict parser -> immutable PK2
# -> production legality report; protects source-byte immutability.
GSC_EVOLVED_RAW_PIPELINE_SOURCES := tests/test_gen2_crystal_raw_save_pipeline.cpp \
	$(filter-out tests/test_gen2_crystal_evolved_native.cpp,$(GSC_EVOLVED_WILD_NATIVE_SOURCES)) \
	src/Integration/Gen2/Gen2ReadOnlySave.cpp

HOST_TESTS += $(HOST_BUILD)/test_gen2_crystal_raw_save_pipeline
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_crystal_raw_save_pipeline_sanitize
host-test: $(HOST_BUILD)/test_gen2_crystal_raw_save_pipeline
host-sanitize: $(HOST_BUILD)/test_gen2_crystal_raw_save_pipeline_sanitize

$(HOST_BUILD)/test_gen2_crystal_raw_save_pipeline: $(GSC_EVOLVED_RAW_PIPELINE_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $^ -o $@

$(HOST_BUILD)/test_gen2_crystal_raw_save_pipeline_sanitize: $(GSC_EVOLVED_RAW_PIPELINE_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $^ -o $@

# Pinned Crystal Eevee static gift after stone/friendship evolution:
# exact caught-data positive evidence and native parsed-save regressions.
GSC_CRYSTAL_EEVEE_GIFT_SOURCES := tests/test_gen2_crystal_eevee_gift.cpp
GSC_CRYSTAL_EEVEE_GIFT_HEADERS := include/Legality/Gen2CrystalEeveeGiftEvidence.h \
	include/Legality/Gen2StaticEncounter.h \
	include/Legality/Gen2StaticEncounterData.inc

HOST_TESTS += $(HOST_BUILD)/test_gen2_crystal_eevee_gift
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_crystal_eevee_gift_sanitize
host-test: $(HOST_BUILD)/test_gen2_crystal_eevee_gift
host-sanitize: $(HOST_BUILD)/test_gen2_crystal_eevee_gift_sanitize

$(HOST_BUILD)/test_gen2_crystal_eevee_gift: $(GSC_CRYSTAL_EEVEE_GIFT_SOURCES) $(GSC_CRYSTAL_EEVEE_GIFT_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GSC_CRYSTAL_EEVEE_GIFT_SOURCES) -o $@

$(HOST_BUILD)/test_gen2_crystal_eevee_gift_sanitize: $(GSC_CRYSTAL_EEVEE_GIFT_SOURCES) $(GSC_CRYSTAL_EEVEE_GIFT_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GSC_CRYSTAL_EEVEE_GIFT_SOURCES) -o $@

# Crystal's pinned level-5 Johto starter gifts, after level-up evolution.
GSC_CRYSTAL_STARTER_GIFT_SOURCES := tests/test_gen2_crystal_starter_gift.cpp
GSC_CRYSTAL_STARTER_GIFT_HEADERS := include/Legality/Gen2CrystalStarterGiftEvidence.h \
	include/Legality/Gen2StaticEncounter.h \
	include/Legality/Gen2StaticEncounterData.inc
HOST_TESTS += $(HOST_BUILD)/test_gen2_crystal_starter_gift
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_crystal_starter_gift_sanitize
host-test: $(HOST_BUILD)/test_gen2_crystal_starter_gift
host-sanitize: $(HOST_BUILD)/test_gen2_crystal_starter_gift_sanitize

$(HOST_BUILD)/test_gen2_crystal_starter_gift: $(GSC_CRYSTAL_STARTER_GIFT_SOURCES) $(GSC_CRYSTAL_STARTER_GIFT_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GSC_CRYSTAL_STARTER_GIFT_SOURCES) -o $@

$(HOST_BUILD)/test_gen2_crystal_starter_gift_sanitize: $(GSC_CRYSTAL_STARTER_GIFT_SOURCES) $(GSC_CRYSTAL_STARTER_GIFT_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GSC_CRYSTAL_STARTER_GIFT_SOURCES) -o $@

# Crystal Dragon's Den level-15 Dratini gift via preserved evolution history.
GSC_CRYSTAL_DRATINI_GIFT_SOURCES := tests/test_gen2_crystal_dratini_gift.cpp
GSC_CRYSTAL_DRATINI_GIFT_HEADERS := include/Legality/Gen2CrystalDratiniGiftEvidence.h \
	include/Legality/Gen2StaticEncounter.h \
	include/Legality/Gen2StaticEncounterData.inc
HOST_TESTS += $(HOST_BUILD)/test_gen2_crystal_dratini_gift
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_crystal_dratini_gift_sanitize
host-test: $(HOST_BUILD)/test_gen2_crystal_dratini_gift
host-sanitize: $(HOST_BUILD)/test_gen2_crystal_dratini_gift_sanitize

$(HOST_BUILD)/test_gen2_crystal_dratini_gift: $(GSC_CRYSTAL_DRATINI_GIFT_SOURCES) $(GSC_CRYSTAL_DRATINI_GIFT_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GSC_CRYSTAL_DRATINI_GIFT_SOURCES) -o $@

$(HOST_BUILD)/test_gen2_crystal_dratini_gift_sanitize: $(GSC_CRYSTAL_DRATINI_GIFT_SOURCES) $(GSC_CRYSTAL_DRATINI_GIFT_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GSC_CRYSTAL_DRATINI_GIFT_SOURCES) -o $@

# Crystal Lake of Rage fixed Red Gyarados requires shiny DVs; native raw
# PK2 regression also verifies the parsed shiny/non-shiny distinction.
GSC_CRYSTAL_RED_GYARADOS_SOURCES := tests/test_gen2_crystal_red_gyarados.cpp
GSC_CRYSTAL_RED_GYARADOS_HEADERS := include/Legality/Gen2StaticEncounter.h \
	include/Legality/Gen2StaticEncounterData.inc
HOST_TESTS += $(HOST_BUILD)/test_gen2_crystal_red_gyarados
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_crystal_red_gyarados_sanitize
host-test: $(HOST_BUILD)/test_gen2_crystal_red_gyarados
host-sanitize: $(HOST_BUILD)/test_gen2_crystal_red_gyarados_sanitize

$(HOST_BUILD)/test_gen2_crystal_red_gyarados: $(GSC_CRYSTAL_RED_GYARADOS_SOURCES) $(GSC_CRYSTAL_RED_GYARADOS_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GSC_CRYSTAL_RED_GYARADOS_SOURCES) -o $@

$(HOST_BUILD)/test_gen2_crystal_red_gyarados_sanitize: $(GSC_CRYSTAL_RED_GYARADOS_SOURCES) $(GSC_CRYSTAL_RED_GYARADOS_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GSC_CRYSTAL_RED_GYARADOS_SOURCES) -o $@

# Crystal Tyrogue level-10 gift through Hitmonlee/Hitmonchan/Hitmontop.
GSC_CRYSTAL_TYROGUE_GIFT_SOURCES := tests/test_gen2_crystal_tyrogue_gift.cpp
GSC_CRYSTAL_TYROGUE_GIFT_HEADERS := include/Legality/Gen2CrystalTyrogueGiftEvidence.h \
	include/Legality/Gen2StaticEncounter.h \
	include/Legality/Gen2StaticEncounterData.inc

HOST_TESTS += $(HOST_BUILD)/test_gen2_crystal_tyrogue_gift
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen2_crystal_tyrogue_gift_sanitize
host-test: $(HOST_BUILD)/test_gen2_crystal_tyrogue_gift
host-sanitize: $(HOST_BUILD)/test_gen2_crystal_tyrogue_gift_sanitize

$(HOST_BUILD)/test_gen2_crystal_tyrogue_gift: $(GSC_CRYSTAL_TYROGUE_GIFT_SOURCES) $(GSC_CRYSTAL_TYROGUE_GIFT_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GSC_CRYSTAL_TYROGUE_GIFT_SOURCES) -o $@

$(HOST_BUILD)/test_gen2_crystal_tyrogue_gift_sanitize: $(GSC_CRYSTAL_TYROGUE_GIFT_SOURCES) $(GSC_CRYSTAL_TYROGUE_GIFT_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GSC_CRYSTAL_TYROGUE_GIFT_SOURCES) -o $@

# True Gen I branched Eevee ancestry through the immutable PK1 production
# legality report (not just a helper predicate).
GSC_GEN1_RATE_NATIVE_SOURCES := tests/test_gen1_catch_rate_native.cpp \
	$(filter-out tests/test_gen2_crystal_evolved_native.cpp,$(GSC_EVOLVED_WILD_NATIVE_SOURCES))
HOST_TESTS += $(HOST_BUILD)/test_gen1_catch_rate_native
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_gen1_catch_rate_native_sanitize
host-test: $(HOST_BUILD)/test_gen1_catch_rate_native
host-sanitize: $(HOST_BUILD)/test_gen1_catch_rate_native_sanitize

$(HOST_BUILD)/test_gen1_catch_rate_native: $(GSC_GEN1_RATE_NATIVE_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $^ -o $@

$(HOST_BUILD)/test_gen1_catch_rate_native_sanitize: $(GSC_GEN1_RATE_NATIVE_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $^ -o $@
