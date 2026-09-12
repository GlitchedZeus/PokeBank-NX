# Generation II GSC permanent host gates.
GEN2_ADAPTER_SOURCES := tests/test_gsc_gen2_adapter.cpp \
	src/Integration/Gen2/Gen2ReadOnlySave.cpp
GSC_INVENTORY_SOURCES := tests/test_gsc_inventory.cpp \
	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp
GSC_PERSONAL_SOURCES := tests/test_gsc_gen2_personal.cpp \
	src/Integration/Gen2/Gen2PersonalData.cpp
GSC_DISCOVERY_SOURCES := tests/test_gsc_discovery.cpp \
	src/Legacy/RetroArchGSCDiscovery.cpp src/Integration/Gen2/Gen2ReadOnlySave.cpp \
	src/Utils/SHA256.cpp
GSC_SOURCE_BROWSER_SOURCES := tests/test_gsc_source_browser.cpp \
	src/Legacy/GSCSourceBrowser.cpp src/Legacy/LegacySourceBindings.cpp \
	src/Integration/Gen2/Gen2ReadOnlySave.cpp src/Games/GameIdentity.cpp
GSC_BRIDGE_SOURCES := tests/test_gsc_readonly_bridge.cpp \
	src/Legacy/GSCReadOnlyTrainer.cpp src/Pokemon/Pokemon2ReadOnly.cpp \
	src/Integration/Gen2/Gen2ReadOnlySave.cpp src/Integration/Gen2/Gen2ReadOnlyInventory.cpp \
	src/Integration/Gen2/Gen2PersonalData.cpp src/Pokemon/BaseStatsGen89.cpp \
	src/Names/SpeciesNames.cpp src/Utils/StringHelpers.cpp
GSC_BRIDGE_FLAGS := -Wno-unused-parameter
GSC_UI_RULES_SOURCES := tests/test_gsc_ui_rules.cpp

# The accepted shared RetroArch catalog now imports the separately validated Gen II scanner as a
# third typed payload. Any pre-GSC host target that links RetroArchFRLGDiscovery.cpp must therefore
# link these new dependencies too; its test source/expectations remain otherwise unchanged.
GSC_RUNTIME_DISCOVERY_SOURCES := src/Legacy/RetroArchGSCDiscovery.cpp \
	src/Integration/Gen2/Gen2ReadOnlySave.cpp
RETROARCH_FRLG_SOURCES += $(GSC_RUNTIME_DISCOVERY_SOURCES)
RSE_HOST_SOURCES += $(GSC_RUNTIME_DISCOVERY_SOURCES)
RSE_NATIVE_SOURCES += $(GSC_RUNTIME_DISCOVERY_SOURCES)
# Makefile.host.base defines these native-slice targets before this extension is included and their
# recipes link via $^. Add the Gen II runtime dependencies to the already-defined prerequisite lists
# so both normal and sanitizer native-slice links receive the same configured catalog dependencies.
$(HOST_BUILD)/test_rse_gen3_native_slice: $(GSC_RUNTIME_DISCOVERY_SOURCES)
$(HOST_BUILD)/test_rse_gen3_native_slice_sanitize: $(GSC_RUNTIME_DISCOVERY_SOURCES)
GSC_RUNTIME_CATALOG_SOURCES := tests/test_gsc_runtime_catalog.cpp \
	$(filter-out tests/test_retroarch_frlg_discovery.cpp,$(RETROARCH_FRLG_SOURCES)) \
	src/Legacy/FRLGSourceBrowser.cpp src/Legacy/LegacySourceBindings.cpp
GSC_RUNTIME_CATALOG_FLAGS := -Wno-unused-parameter

GSC_HOST_TESTS := $(HOST_BUILD)/test_gsc_gen2_adapter \
	$(HOST_BUILD)/test_gsc_inventory \
	$(HOST_BUILD)/test_gsc_gen2_personal \
	$(HOST_BUILD)/test_gsc_discovery \
	$(HOST_BUILD)/test_gsc_source_browser \
	$(HOST_BUILD)/test_gsc_readonly_bridge \
	$(HOST_BUILD)/test_gsc_runtime_catalog \
	$(HOST_BUILD)/test_gsc_ui_rules
GSC_SANITIZE_TESTS := $(HOST_BUILD)/test_gsc_gen2_adapter_sanitize \
	$(HOST_BUILD)/test_gsc_inventory_sanitize \
	$(HOST_BUILD)/test_gsc_gen2_personal_sanitize \
	$(HOST_BUILD)/test_gsc_discovery_sanitize \
	$(HOST_BUILD)/test_gsc_source_browser_sanitize \
	$(HOST_BUILD)/test_gsc_readonly_bridge_sanitize \
	$(HOST_BUILD)/test_gsc_runtime_catalog_sanitize \
	$(HOST_BUILD)/test_gsc_ui_rules_sanitize

# The core host recipe is defined in Makefile.host.base. Extend both its runtime loop variables
# and its prerequisite graph so normal/sanitizer invocations build and execute the GSC tests.
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
