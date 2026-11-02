# PokeBank NX host test rules: inventory.mk
# Consolidated from the original Makefile.host.* fragments in their EXACT include order.
# Do not modify host-test recipes or weaken coverage while reorganizing infrastructure.

# ---- BEGIN original Makefile.host.bag-top-up ----
# Top Up / Add Missing (owner Oct 28, Feature B; adapted from PKSE d2ad323 "Add All Items").
# The Gen I-V adapters (include/Inventory/BulkItemAdapters*.h) over the REAL staged Bag editors on
# synthetic saves, plus the shared SETUP / PREVIEW model (include/UI/BagTopUpModel.h). The real
# TrainerViewScreen flow (ZR -> preview -> B / A -> Review) is tests/test_ui_bag_top_up_host.cpp in
# Makefile.host.host-ui. Opt-in real saves: POKEBANK_REAL_SAVES=<folder> (never copied into the repo)
# runs tests/test_bag_top_up_real_saves.cpp against every Gen III / IV / V save found there.
# Included by Makefile.host after Makefile.host.jp-gen2.

BAG_TOP_UP_COMMON_SOURCES := $(sort \
	src/Inventory/ItemQuantityPolicy.cpp src/Inventory/ClassicInventoryCatalog.cpp \
	src/Integration/Gen1/Gen1ReadOnlyInventory.cpp src/Integration/Gen1/Gen1ReadOnlySave.cpp \
	src/Integration/Gen1/Gen1StagedInventoryEditor.cpp \
	src/Integration/Gen2/Gen2StagedEditor.cpp src/Integration/Gen2/Gen2PackedMove.cpp \
	src/Integration/Gen2/Gen2UniversalInventory.cpp src/Integration/Gen2/Gen2ReadOnlySave.cpp \
	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp src/Integration/Gen2/Gen2PersonalData.cpp \
	src/Integration/Gen3/Gen3StagedInventoryEditor.cpp \
	src/PokemonFile/NativePokemonFile.cpp src/Encryption/Encryption3FRLG.cpp \
	$(filter-out tests/test_gen4_staged_pokemon_editor.cpp,$(GEN4_STAGED_EDITOR_SOURCES)))
BAG_TOP_UP_HEADERS := include/Inventory/BulkItemPlan.h include/Inventory/BulkItemApply.h \
	include/Inventory/BulkItemAdaptersClassic.h include/Inventory/BulkItemAdaptersNds.h \
	include/UI/BagTopUpModel.h tests/fuzz_synthetic_saves.h \
	include/Integration/Gen1/Gen1StagedInventoryEditor.h include/Integration/Gen2/Gen2StagedEditor.h \
	include/Integration/Gen3/Gen3StagedInventoryEditor.h include/Integration/Gen5/Gen5Bag.h

BAG_TOP_UP_TESTS := $(HOST_BUILD)/test_bag_top_up_adapters $(HOST_BUILD)/test_bag_top_up_real_saves
BAG_TOP_UP_SANITIZE_TESTS := $(addsuffix _sanitize,$(BAG_TOP_UP_TESTS))

HOST_TESTS += $(BAG_TOP_UP_TESTS)
HOST_SANITIZE_TESTS += $(BAG_TOP_UP_SANITIZE_TESTS)
host-test: $(BAG_TOP_UP_TESTS)
host-sanitize: $(BAG_TOP_UP_SANITIZE_TESTS)

.PHONY: host-bag-top-up
host-bag-top-up: $(BAG_TOP_UP_TESTS)
	./$(HOST_BUILD)/test_bag_top_up_adapters
	./$(HOST_BUILD)/test_bag_top_up_real_saves

$(HOST_BUILD)/test_bag_top_up_%: tests/test_bag_top_up_%.cpp $(BAG_TOP_UP_COMMON_SOURCES) $(BAG_TOP_UP_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude -Itests $< $(BAG_TOP_UP_COMMON_SOURCES) -o $@
$(HOST_BUILD)/test_bag_top_up_%_sanitize: tests/test_bag_top_up_%.cpp $(BAG_TOP_UP_COMMON_SOURCES) $(BAG_TOP_UP_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude -Itests $< $(BAG_TOP_UP_COMMON_SOURCES) -o $@
# ---- END original Makefile.host.bag-top-up ----

# ---- BEGIN original Makefile.host.pokemon-file-menus ----
# Feature A UI (owner Oct 28): Pokemon Actions -> Import from file / Export to file. The dialog's pure
# input model and its SD-card half (listing, size-capped read, durable export) in a private mkdtemp
# tree. The real-screen tests live in Makefile.host.host-ui (GCC-only harness).
POKEMON_FILE_DIALOG_SOURCES := tests/test_pokemon_file_dialog_model.cpp src/PokemonFile/NativePokemonFile.cpp \
	src/Encryption/Encryption3FRLG.cpp src/Encryption/Encryption4.cpp src/Utils/DurableFile.cpp
POKEMON_FILE_DIALOG_HEADERS := include/UI/PokemonFileDialogModel.h include/UI/PokemonFileIo.h \
	include/UI/PokemonFileBrowser.h include/Utils/FolderPreferences.h
HOST_TESTS += $(HOST_BUILD)/test_pokemon_file_dialog_model
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_pokemon_file_dialog_model_sanitize
host-test: $(HOST_BUILD)/test_pokemon_file_dialog_model
host-sanitize: $(HOST_BUILD)/test_pokemon_file_dialog_model_sanitize
$(HOST_BUILD)/test_pokemon_file_dialog_model: $(POKEMON_FILE_DIALOG_SOURCES) $(POKEMON_FILE_DIALOG_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(POKEMON_FILE_DIALOG_SOURCES) -o $@
$(HOST_BUILD)/test_pokemon_file_dialog_model_sanitize: $(POKEMON_FILE_DIALOG_SOURCES) $(POKEMON_FILE_DIALOG_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(POKEMON_FILE_DIALOG_SOURCES) -o $@
# ---- END original Makefile.host.pokemon-file-menus ----

# ---- BEGIN original Makefile.host.machine ----
# Global game-aware machine-name presentation gates.
MACHINE_DISPLAY_SOURCES := tests/test_machine_display.cpp \
	src/Names/MachineDisplay.cpp src/Names/TMMoves.cpp src/Names/MoveNames.cpp

MACHINE_HOST_TESTS := $(HOST_BUILD)/test_machine_display
MACHINE_SANITIZE_TESTS := $(HOST_BUILD)/test_machine_display_sanitize

HOST_TESTS += $(MACHINE_HOST_TESTS)
HOST_SANITIZE_TESTS += $(MACHINE_SANITIZE_TESTS)
host-test: $(MACHINE_HOST_TESTS)
host-sanitize: $(MACHINE_SANITIZE_TESTS)

$(HOST_BUILD)/test_machine_display: $(MACHINE_DISPLAY_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $^ -o $@

$(HOST_BUILD)/test_machine_display_sanitize: $(MACHINE_DISPLAY_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $^ -o $@


# UI-only sorting never mutates a native game item ID or staged source.
ITEM_PICKER_SORT_SRC := tests/test_item_picker_sort.cpp
ITEM_PICKER_SORT_TEST := $(HOST_BUILD)/test_item_picker_sort
ITEM_PICKER_SORT_SANITIZE := $(HOST_BUILD)/test_item_picker_sort_sanitize
HOST_TESTS += $(ITEM_PICKER_SORT_TEST)
HOST_SANITIZE_TESTS += $(ITEM_PICKER_SORT_SANITIZE)
host-test: $(ITEM_PICKER_SORT_TEST)
host-sanitize: $(ITEM_PICKER_SORT_SANITIZE)

$(ITEM_PICKER_SORT_TEST): $(ITEM_PICKER_SORT_SRC) include/UI/ItemPickerSort.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $< -o $@

$(ITEM_PICKER_SORT_SANITIZE): $(ITEM_PICKER_SORT_SRC) include/UI/ItemPickerSort.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $< -o $@
# ---- END original Makefile.host.machine ----
