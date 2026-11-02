# PokeBank NX host test rules: localization.mk
# Consolidated from the original Makefile.host.* fragments in their EXACT include order.
# Do not modify host-test recipes or weaken coverage while reorganizing infrastructure.

# ---- BEGIN original Makefile.host.pokemon-file-ui ----
# Feature A UI (owner Oct 28): the Pokemon file browser model. Included by Makefile.host after the
# Feature A codec fragments.
POKEMON_FILE_BROWSER_SOURCES := tests/test_pokemon_file_browser.cpp src/PokemonFile/NativePokemonFile.cpp src/Encryption/Encryption3FRLG.cpp src/Encryption/Encryption4.cpp
HOST_TESTS += $(HOST_BUILD)/test_pokemon_file_browser
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_pokemon_file_browser_sanitize
host-test: $(HOST_BUILD)/test_pokemon_file_browser
host-sanitize: $(HOST_BUILD)/test_pokemon_file_browser_sanitize
$(HOST_BUILD)/test_pokemon_file_browser: $(POKEMON_FILE_BROWSER_SOURCES) include/UI/PokemonFileBrowser.h include/Utils/FolderPreferences.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(POKEMON_FILE_BROWSER_SOURCES) -o $@
$(HOST_BUILD)/test_pokemon_file_browser_sanitize: $(POKEMON_FILE_BROWSER_SOURCES) include/UI/PokemonFileBrowser.h include/Utils/FolderPreferences.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(POKEMON_FILE_BROWSER_SOURCES) -o $@

# Feature D (owner Oct 28): opt-in real-save round trip (POKEBANK_REAL_SAVES; saves never committed).
REAL_SAVE_ROUNDTRIP_SOURCES := tests/test_real_save_roundtrip.cpp $(sort $(filter-out tests/%,$(POKEMON_FILE_GEN12_SOURCES) $(PKFILE_GEN3_SOURCES) $(PKFILE_GEN4_SOURCES)))
HOST_TESTS += $(HOST_BUILD)/test_real_save_roundtrip
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_real_save_roundtrip_sanitize
host-test: $(HOST_BUILD)/test_real_save_roundtrip
host-sanitize: $(HOST_BUILD)/test_real_save_roundtrip_sanitize
$(HOST_BUILD)/test_real_save_roundtrip: $(REAL_SAVE_ROUNDTRIP_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST $(REAL_SAVE_ROUNDTRIP_SOURCES) -o $@
$(HOST_BUILD)/test_real_save_roundtrip_sanitize: $(REAL_SAVE_ROUNDTRIP_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST $(REAL_SAVE_ROUNDTRIP_SOURCES) -o $@
# ---- END original Makefile.host.pokemon-file-ui ----

# ---- BEGIN original Makefile.host.recovery-view ----
# Owner item #10: recovery management (Save Backups -> X "Recovery files"). The pure safety policy
# (include/UI/RecoveryManagementModel.h) and the file-system layer (include/UI/RecoveryManagementFs.h)
# on a private mkdtemp tree. The real-screen test lives in Makefile.host.host-ui (GCC-only block).
RECOVERY_VIEW_MODEL_TEST := $(HOST_BUILD)/test_recovery_management_model
RECOVERY_VIEW_MODEL_SANITIZE := $(HOST_BUILD)/test_recovery_management_model_sanitize
RECOVERY_VIEW_MODEL_INPUTS := tests/test_recovery_management_model.cpp include/UI/RecoveryManagementModel.h
RECOVERY_VIEW_FS_TEST := $(HOST_BUILD)/test_recovery_management_fs
RECOVERY_VIEW_FS_SANITIZE := $(HOST_BUILD)/test_recovery_management_fs_sanitize
RECOVERY_VIEW_FS_SOURCES := tests/test_recovery_management_fs.cpp src/Utils/SHA256.cpp
RECOVERY_VIEW_FS_INPUTS := $(RECOVERY_VIEW_FS_SOURCES) include/UI/RecoveryManagementModel.h \
	include/UI/RecoveryManagementFs.h include/UI/SourceBackupClassification.h \
	include/Source/ImmutableSourceSnapshot.h

HOST_TESTS += $(RECOVERY_VIEW_MODEL_TEST) $(RECOVERY_VIEW_FS_TEST)
HOST_SANITIZE_TESTS += $(RECOVERY_VIEW_MODEL_SANITIZE) $(RECOVERY_VIEW_FS_SANITIZE)
host-test: $(RECOVERY_VIEW_MODEL_TEST) $(RECOVERY_VIEW_FS_TEST)
host-sanitize: $(RECOVERY_VIEW_MODEL_SANITIZE) $(RECOVERY_VIEW_FS_SANITIZE)

$(RECOVERY_VIEW_MODEL_TEST): $(RECOVERY_VIEW_MODEL_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_recovery_management_model.cpp -o $@

$(RECOVERY_VIEW_MODEL_SANITIZE): $(RECOVERY_VIEW_MODEL_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_recovery_management_model.cpp -o $@

$(RECOVERY_VIEW_FS_TEST): $(RECOVERY_VIEW_FS_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(RECOVERY_VIEW_FS_SOURCES) -o $@

$(RECOVERY_VIEW_FS_SANITIZE): $(RECOVERY_VIEW_FS_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(RECOVERY_VIEW_FS_SOURCES) -o $@
# ---- END original Makefile.host.recovery-view ----

# ---- BEGIN original Makefile.host.jp-gen12 ----
# Japanese Gen I / II editing (owner Oct 28). Writers and text are tested here; the UI keeps Japanese
# saves read-only until the owner opens include/Integration/Common/JapaneseEditingGate.h.
# Real-save evidence is opt-in: POKEBANK_JP_FIXTURES=<folder with saves/ and pokemon/> (never committed).
# Included by Makefile.host after the Feature A fragments.

JP_KANA_SOURCES := tests/test_gen12_kana_text.cpp src/Integration/Gen1/Gen1ReadOnlySave.cpp src/Integration/Gen2/Gen2ReadOnlySave.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp src/Names/SpeciesNames.cpp
JP_GEN12_HOST_TESTS := $(HOST_BUILD)/test_gen12_kana_text
JP_GEN12_SANITIZE_TESTS := $(HOST_BUILD)/test_gen12_kana_text_sanitize

HOST_TESTS += $(JP_GEN12_HOST_TESTS)
HOST_SANITIZE_TESTS += $(JP_GEN12_SANITIZE_TESTS)
host-test: $(JP_GEN12_HOST_TESTS)
host-sanitize: $(JP_GEN12_SANITIZE_TESTS)

$(HOST_BUILD)/test_gen12_kana_text: $(JP_KANA_SOURCES) include/Utils/Gen12KanaText.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(JP_KANA_SOURCES) -o $@
$(HOST_BUILD)/test_gen12_kana_text_sanitize: $(JP_KANA_SOURCES) include/Utils/Gen12KanaText.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(JP_KANA_SOURCES) -o $@
# ---- END original Makefile.host.jp-gen12 ----

# ---- BEGIN original Makefile.host.jp-gen1 ----
# Japanese Gen I save writing (owner Oct 28): the staged Gen I Pokemon editor (8 x 30, kana names,
# 59-byte .pk1) and the kana Trainer name. The app keeps Japanese saves read-only until the owner opens
# include/Integration/Common/JapaneseEditingGate.h; this fragment tests the writers themselves.
# Real-save evidence is opt-in: POKEBANK_JP_FIXTURES=<folder with saves/ and pokemon/> (never committed).
# Included by Makefile.host after Makefile.host.jp-gen12.

JP_GEN1_WRITER_SOURCES := tests/test_gen1_japanese_writer.cpp $(sort \
	src/PokemonFile/NativePokemonFile.cpp src/Encryption/Encryption3FRLG.cpp src/Encryption/Encryption4.cpp \
	src/Integration/Gen1/Gen1StagedPokemonEditor.cpp src/Integration/Gen1/Gen1PackedMove.cpp \
	src/Integration/Gen1/Gen1BattleStats.cpp src/Integration/Gen1/Gen1ReadOnlySave.cpp \
	src/Integration/Gen1/Gen1StagedInventoryEditor.cpp src/Integration/Gen1/Gen1ReadOnlyInventory.cpp \
	src/Integration/Gen1/Gen1ExportTransaction.cpp src/Integration/Common/VerifiedStagedExport.cpp \
	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp src/Inventory/ClassicInventoryCatalog.cpp \
	src/Inventory/ItemQuantityPolicy.cpp src/Names/ItemNames.cpp src/Utils/SHA256.cpp \
	src/Pokemon/Experience.cpp src/Names/SpeciesNames.cpp src/Names/MoveNames.cpp \
	src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp)
JP_GEN1_WRITER_HEADERS := include/Integration/Gen1/Gen1StorageLayout.h include/Integration/Gen1/Gen1StagedPokemonEditor.h \
	include/Integration/Gen1/Gen1StagedTrainerEditor.h include/Integration/Gen1/Gen12CreateReview.h \
	include/Integration/Gen1/Gen1ExportTransaction.h include/Integration/Gen1/Gen1StagedInventoryEditor.h \
	include/Utils/Gen12KanaText.h include/PokemonFile/NativePokemonFile.h src/Integration/Gen1/Gen1PersonalData.inc
JP_GEN1_HOST_TESTS := $(HOST_BUILD)/test_gen1_japanese_writer
JP_GEN1_SANITIZE_TESTS := $(HOST_BUILD)/test_gen1_japanese_writer_sanitize

HOST_TESTS += $(JP_GEN1_HOST_TESTS)
HOST_SANITIZE_TESTS += $(JP_GEN1_SANITIZE_TESTS)
host-test: $(JP_GEN1_HOST_TESTS)
host-sanitize: $(JP_GEN1_SANITIZE_TESTS)

$(HOST_BUILD)/test_gen1_japanese_writer: $(JP_GEN1_WRITER_SOURCES) $(JP_GEN1_WRITER_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(JP_GEN1_WRITER_SOURCES) -o $@
$(HOST_BUILD)/test_gen1_japanese_writer_sanitize: $(JP_GEN1_WRITER_SOURCES) $(JP_GEN1_WRITER_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(JP_GEN1_WRITER_SOURCES) -o $@
# ---- END original Makefile.host.jp-gen1 ----

# ---- BEGIN original Makefile.host.jp-gen2 ----
# Japanese Gold / Silver / Crystal staged writing (owner Oct 28). The writer and its tests live here;
# the app keeps Japanese saves read-only until the owner opens
# include/Integration/Common/JapaneseEditingGate.h (GSCReadOnlyTrainer consults it).
# Real-save evidence is opt-in: POKEBANK_JP_FIXTURES=<folder with saves/ and pokemon/> (never committed).
# Included by Makefile.host after Makefile.host.jp-gen12.

JP_GEN2_WRITER_SOURCES := tests/test_gen2_japanese_writer.cpp \
	src/Integration/Gen2/Gen2StagedEditor.cpp src/Integration/Gen2/Gen2PackedMove.cpp \
	src/Integration/Gen2/Gen2UniversalInventory.cpp src/Integration/Gen2/Gen2ReadOnlySave.cpp \
	src/Integration/Gen2/Gen2ReadOnlyInventory.cpp src/Integration/Gen2/Gen2PersonalData.cpp \
	src/PokemonFile/NativePokemonFile.cpp src/Encryption/Encryption3FRLG.cpp src/Encryption/Encryption4.cpp \
	src/Inventory/ClassicInventoryCatalog.cpp src/Inventory/ItemQuantityPolicy.cpp \
	src/Pokemon/Experience.cpp src/Names/SpeciesNames.cpp src/Names/MoveNames.cpp src/Names/ItemNames.cpp \
	src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp
JP_GEN2_WRITER_HEADERS := include/Integration/Gen2/Gen2StagedEditor.h include/Utils/Gen12KanaText.h \
	include/PokemonFile/NativePokemonFile.h tests/fixtures/gsc_pokemon_fixture.h
JP_GEN2_HOST_TESTS := $(HOST_BUILD)/test_gen2_japanese_writer
JP_GEN2_SANITIZE_TESTS := $(HOST_BUILD)/test_gen2_japanese_writer_sanitize

HOST_TESTS += $(JP_GEN2_HOST_TESTS)
HOST_SANITIZE_TESTS += $(JP_GEN2_SANITIZE_TESTS)
host-test: $(JP_GEN2_HOST_TESTS)
host-sanitize: $(JP_GEN2_SANITIZE_TESTS)

$(HOST_BUILD)/test_gen2_japanese_writer: $(JP_GEN2_WRITER_SOURCES) $(JP_GEN2_WRITER_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude -Itests $(JP_GEN2_WRITER_SOURCES) -o $@
$(HOST_BUILD)/test_gen2_japanese_writer_sanitize: $(JP_GEN2_WRITER_SOURCES) $(JP_GEN2_WRITER_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude -Itests $(JP_GEN2_WRITER_SOURCES) -o $@
# ---- END original Makefile.host.jp-gen2 ----
