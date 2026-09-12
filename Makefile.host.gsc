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

GSC_HOST_TESTS := $(HOST_BUILD)/test_gsc_gen2_adapter \
	$(HOST_BUILD)/test_gsc_inventory \
	$(HOST_BUILD)/test_gsc_gen2_personal \
	$(HOST_BUILD)/test_gsc_discovery \
	$(HOST_BUILD)/test_gsc_source_browser
GSC_SANITIZE_TESTS := $(HOST_BUILD)/test_gsc_gen2_adapter_sanitize \
	$(HOST_BUILD)/test_gsc_inventory_sanitize \
	$(HOST_BUILD)/test_gsc_gen2_personal_sanitize \
	$(HOST_BUILD)/test_gsc_discovery_sanitize \
	$(HOST_BUILD)/test_gsc_source_browser_sanitize

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
