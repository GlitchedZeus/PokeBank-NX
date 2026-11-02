# PokeBank NX host test rules: ui.mk
# Consolidated from the original Makefile.host.* fragments in their EXACT include order.
# Do not modify host-test recipes or weaken coverage while reorganizing infrastructure.

# ---- BEGIN original Makefile.host.host-ui ----
# Host UI harness (Round 6 L06): tests that drive the REAL TrainerViewScreen::update / draw on the
# host with scripted controller and touch input (tests/host-ui). The app's own sources are built
# into a static archive against the scriptable libnx stub (tests/host-ui/switch.h); the linker pulls
# only what a test references. Platform-only sources (framebuffer, keyboard applet, account /
# filesystem utilities, the top-level screens) are replaced by tests/host-ui/HostUiStubs.cpp.
# Application objects are built with -w: the app's warnings are gated by its own native build; the
# tests themselves use the full host CXXFLAGS.
# The application itself is GCC-only (devkitA64): several headers name `Trainer::Trainer` /
# `Pokemon::...` while a global using-directive is in scope, which Clang reports as ambiguous and
# GCC accepts. The harness therefore links the app only when the host compiler is GCC; under Clang
# this file adds no targets and the rest of the host suite is unchanged.
HOST_UI_COMPILER_IS_CLANG := $(shell $(CXX) --version 2>/dev/null | grep -qi clang && echo yes)
ifneq ($(HOST_UI_COMPILER_IS_CLANG),yes)
HOST_UI_EXCLUDED := src/main.cpp src/UI/UI.cpp src/UI/BackupSelectionScreen.cpp \
	src/UI/PKSEFramebuffer.cpp src/UI/PKSEFramebufferTouchTransform.cpp src/UI/SystemIcons.cpp \
	src/Utils/FileUtilities.cpp src/Utils/Keyboard.cpp src/Utils/PhysicalMoveAudit.cpp \
	src/Integration/Gen3/PKSMGen3Adapter.cpp
HOST_UI_APP_SOURCES := $(filter-out $(HOST_UI_EXCLUDED),$(shell find src -name '*.cpp' | sort))
HOST_UI_INCLUDES := -Iinclude -Itests/host-ui
HOST_UI_APP_FLAGS := -std=c++20 -O1 -w -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST
HOST_UI_DIR := $(HOST_BUILD)/host-ui
HOST_UI_OBJECTS := $(patsubst src/%.cpp,$(HOST_UI_DIR)/obj/%.o,$(HOST_UI_APP_SOURCES))
HOST_UI_SANITIZE_OBJECTS := $(patsubst src/%.cpp,$(HOST_UI_DIR)/asan/%.o,$(HOST_UI_APP_SOURCES))
HOST_UI_ARCHIVE := $(HOST_UI_DIR)/libpokebank-hostui.a
HOST_UI_SANITIZE_ARCHIVE := $(HOST_UI_DIR)/libpokebank-hostui-asan.a
HOST_UI_STUBS := tests/host-ui/HostUiStubs.cpp

$(HOST_UI_DIR)/obj/%.o: src/%.cpp tests/host-ui/switch.h
	@mkdir -p $(dir $@)
	$(CXX) $(HOST_UI_APP_FLAGS) $(HOST_UI_INCLUDES) -c $< -o $@
# GCC rejects Gen5PersonalTypes.h's constexpr `info==nullptr` static_asserts whenever UBSan
# instruments the TU, so the one object that includes it (the composite device UI TU) is built with
# AddressSanitizer only; every other application object gets the full ASan + UBSan flags.
HOST_UI_SANITIZE := $(SANITIZE_FLAGS)
$(HOST_UI_DIR)/asan/UI/TrainerViewScreenCompositeOverlay.o: HOST_UI_SANITIZE := -fsanitize=address -fno-omit-frame-pointer -O1
$(HOST_UI_DIR)/asan/%.o: src/%.cpp tests/host-ui/switch.h
	@mkdir -p $(dir $@)
	$(CXX) $(HOST_UI_APP_FLAGS) $(HOST_UI_SANITIZE) -w $(HOST_UI_INCLUDES) -c $< -o $@
$(HOST_UI_ARCHIVE): $(HOST_UI_OBJECTS)
	@rm -f $@
	ar rcs $@ $^
$(HOST_UI_SANITIZE_ARCHIVE): $(HOST_UI_SANITIZE_OBJECTS)
	@rm -f $@
	ar rcs $@ $^

# L06: Gen III / Gen V Pokérus and Gen III Held Item through the real input path (A and touch).
HOST_UI_EDITOR_TEST := $(HOST_BUILD)/test_ui_editor_input_host
HOST_UI_EDITOR_SANITIZE := $(HOST_BUILD)/test_ui_editor_input_host_sanitize
HOST_TESTS += $(HOST_UI_EDITOR_TEST)
HOST_SANITIZE_TESTS += $(HOST_UI_EDITOR_SANITIZE)
host-test: $(HOST_UI_EDITOR_TEST)
host-sanitize: $(HOST_UI_EDITOR_SANITIZE)
$(HOST_UI_EDITOR_TEST): tests/test_ui_editor_input_host.cpp tests/fuzz_synthetic_saves.h $(HOST_UI_STUBS) tests/host-ui/HostUi.h $(HOST_UI_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_editor_input_host.cpp $(HOST_UI_STUBS) $(HOST_UI_ARCHIVE) -o $@
$(HOST_UI_EDITOR_SANITIZE): tests/test_ui_editor_input_host.cpp tests/fuzz_synthetic_saves.h $(HOST_UI_STUBS) tests/host-ui/HostUi.h $(HOST_UI_SANITIZE_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_editor_input_host.cpp $(HOST_UI_STUBS) $(HOST_UI_SANITIZE_ARCHIVE) -o $@

# Round 6 L06 / 7D (lane P): Gen III Held Item tap and A, and outside taps, through the real screen.
HOST_UI_HELD_TEST := $(HOST_BUILD)/test_ui_held_item_host
HOST_UI_HELD_SANITIZE := $(HOST_BUILD)/test_ui_held_item_host_sanitize
HOST_TESTS += $(HOST_UI_HELD_TEST)
HOST_SANITIZE_TESTS += $(HOST_UI_HELD_SANITIZE)
host-test: $(HOST_UI_HELD_TEST)
host-sanitize: $(HOST_UI_HELD_SANITIZE)
$(HOST_UI_HELD_TEST): tests/test_ui_held_item_host.cpp $(HOST_UI_STUBS) tests/host-ui/HostUi.h $(HOST_UI_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_held_item_host.cpp $(HOST_UI_STUBS) $(HOST_UI_ARCHIVE) -o $@
$(HOST_UI_HELD_SANITIZE): tests/test_ui_held_item_host.cpp $(HOST_UI_STUBS) tests/host-ui/HostUi.h $(HOST_UI_SANITIZE_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_held_item_host.cpp $(HOST_UI_STUBS) $(HOST_UI_SANITIZE_ARCHIVE) -o $@

# Round 6 E05 (lane R): Gen I-V Game Browser pages and Help through the real TrainerViewScreen::draw;
# the HostUi layout recorder asserts one footer and no overlapping visible text.
HOST_UI_LAYOUT_TEST := $(HOST_BUILD)/test_ui_layout_host
HOST_UI_LAYOUT_SANITIZE := $(HOST_BUILD)/test_ui_layout_host_sanitize
HOST_UI_LAYOUT_INPUTS := tests/test_ui_layout_host.cpp tests/host-ui/HostUi.h tests/host-ui/HostUiFixtures.h \
	tests/fuzz_synthetic_saves.h tests/fixtures/gsc_pokemon_fixture.h $(HOST_UI_STUBS)
HOST_TESTS += $(HOST_UI_LAYOUT_TEST)
HOST_SANITIZE_TESTS += $(HOST_UI_LAYOUT_SANITIZE)
host-test: $(HOST_UI_LAYOUT_TEST)
host-sanitize: $(HOST_UI_LAYOUT_SANITIZE)
$(HOST_UI_LAYOUT_TEST): $(HOST_UI_LAYOUT_INPUTS) $(HOST_UI_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_layout_host.cpp $(HOST_UI_STUBS) $(HOST_UI_ARCHIVE) -o $@
$(HOST_UI_LAYOUT_SANITIZE): $(HOST_UI_LAYOUT_INPUTS) $(HOST_UI_SANITIZE_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_layout_host.cpp $(HOST_UI_STUBS) $(HOST_UI_SANITIZE_ARCHIVE) -o $@
# Round 6 7D (lane P): Gen I Species and Gen II Move pickers close on an outside tap, real screen.
HOST_UI_CLASSIC_DISMISS_TEST := $(HOST_BUILD)/test_ui_classic_picker_dismiss_host
HOST_UI_CLASSIC_DISMISS_SANITIZE := $(HOST_BUILD)/test_ui_classic_picker_dismiss_host_sanitize
HOST_UI_CLASSIC_DISMISS_INPUTS := tests/test_ui_classic_picker_dismiss_host.cpp tests/host-ui/HostUi.h \
	tests/fuzz_synthetic_saves.h tests/fixtures/gsc_pokemon_fixture.h $(HOST_UI_STUBS)
HOST_TESTS += $(HOST_UI_CLASSIC_DISMISS_TEST)
HOST_SANITIZE_TESTS += $(HOST_UI_CLASSIC_DISMISS_SANITIZE)
host-test: $(HOST_UI_CLASSIC_DISMISS_TEST)
host-sanitize: $(HOST_UI_CLASSIC_DISMISS_SANITIZE)
$(HOST_UI_CLASSIC_DISMISS_TEST): $(HOST_UI_CLASSIC_DISMISS_INPUTS) $(HOST_UI_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_classic_picker_dismiss_host.cpp $(HOST_UI_STUBS) $(HOST_UI_ARCHIVE) -o $@
$(HOST_UI_CLASSIC_DISMISS_SANITIZE): $(HOST_UI_CLASSIC_DISMISS_INPUTS) $(HOST_UI_SANITIZE_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_classic_picker_dismiss_host.cpp $(HOST_UI_STUBS) $(HOST_UI_SANITIZE_ARCHIVE) -o $@

# Round 7 C04 (lane P): Quick Games and Product Home Help close on an outside tap, through the
# real SaveSelectScreen (Product Home builds on the host with no account or saves, switch.h).
HOST_UI_QUICK_GAMES_TEST := $(HOST_BUILD)/test_ui_quick_games_host
HOST_UI_QUICK_GAMES_SANITIZE := $(HOST_BUILD)/test_ui_quick_games_host_sanitize
HOST_UI_QUICK_GAMES_INPUTS := tests/test_ui_quick_games_host.cpp tests/host-ui/HostUi.h \
	tests/host-ui/HostUiProductHomeStubs.cpp $(HOST_UI_STUBS)
HOST_TESTS += $(HOST_UI_QUICK_GAMES_TEST)
HOST_SANITIZE_TESTS += $(HOST_UI_QUICK_GAMES_SANITIZE)
host-test: $(HOST_UI_QUICK_GAMES_TEST)
host-sanitize: $(HOST_UI_QUICK_GAMES_SANITIZE)
$(HOST_UI_QUICK_GAMES_TEST): $(HOST_UI_QUICK_GAMES_INPUTS) $(HOST_UI_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_quick_games_host.cpp $(HOST_UI_STUBS) tests/host-ui/HostUiProductHomeStubs.cpp $(HOST_UI_ARCHIVE) -o $@
$(HOST_UI_QUICK_GAMES_SANITIZE): $(HOST_UI_QUICK_GAMES_INPUTS) $(HOST_UI_SANITIZE_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_quick_games_host.cpp $(HOST_UI_STUBS) tests/host-ui/HostUiProductHomeStubs.cpp $(HOST_UI_SANITIZE_ARCHIVE) -o $@


# Round 7 B07 / C09 (lane R): Gen II Unown Letter grid, Shiny questions and Reroll through the real
# TrainerViewScreen input (Silver save with an Unown in Box 2).
HOST_UI_UNOWN_TEST := $(HOST_BUILD)/test_ui_unown_letter_host
HOST_UI_UNOWN_SANITIZE := $(HOST_BUILD)/test_ui_unown_letter_host_sanitize
HOST_UI_UNOWN_INPUTS := tests/test_ui_unown_letter_host.cpp tests/host-ui/HostUi.h tests/fixtures/gsc_pokemon_fixture.h \
	include/UI/Gen2UnownLetter.h $(HOST_UI_STUBS)
HOST_TESTS += $(HOST_UI_UNOWN_TEST)
HOST_SANITIZE_TESTS += $(HOST_UI_UNOWN_SANITIZE)
host-test: $(HOST_UI_UNOWN_TEST)
host-sanitize: $(HOST_UI_UNOWN_SANITIZE)
$(HOST_UI_UNOWN_TEST): $(HOST_UI_UNOWN_INPUTS) $(HOST_UI_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_unown_letter_host.cpp $(HOST_UI_STUBS) $(HOST_UI_ARCHIVE) -o $@
$(HOST_UI_UNOWN_SANITIZE): $(HOST_UI_UNOWN_INPUTS) $(HOST_UI_SANITIZE_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_unown_letter_host.cpp $(HOST_UI_STUBS) $(HOST_UI_SANITIZE_ARCHIVE) -o $@


# Round 8 automated QA: the real SaveSelectScreen title discovery against a scripted console (one
# account, synthetic save-data entries for the twelve FireRed/LeafGreen Switch releases plus noise).
HOST_UI_DISCOVERY_TEST := $(HOST_BUILD)/test_ui_switch_title_discovery_host
HOST_UI_DISCOVERY_SANITIZE := $(HOST_BUILD)/test_ui_switch_title_discovery_host_sanitize
HOST_UI_DISCOVERY_INPUTS := tests/test_ui_switch_title_discovery_host.cpp tests/host-ui/HostUi.h \
	tests/host-ui/switch.h tests/host-ui/HostUiProductHomeStubs.cpp $(HOST_UI_STUBS)
HOST_TESTS += $(HOST_UI_DISCOVERY_TEST)
HOST_SANITIZE_TESTS += $(HOST_UI_DISCOVERY_SANITIZE)
host-test: $(HOST_UI_DISCOVERY_TEST)
host-sanitize: $(HOST_UI_DISCOVERY_SANITIZE)
$(HOST_UI_DISCOVERY_TEST): $(HOST_UI_DISCOVERY_INPUTS) $(HOST_UI_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_switch_title_discovery_host.cpp $(HOST_UI_STUBS) tests/host-ui/HostUiProductHomeStubs.cpp $(HOST_UI_ARCHIVE) -o $@
$(HOST_UI_DISCOVERY_SANITIZE): $(HOST_UI_DISCOVERY_INPUTS) $(HOST_UI_SANITIZE_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_switch_title_discovery_host.cpp $(HOST_UI_STUBS) tests/host-ui/HostUiProductHomeStubs.cpp $(HOST_UI_SANITIZE_ARCHIVE) -o $@

# Owner item #10: Recovery files (Backups -> X) through the real SaveSelectScreen: review, select,
# confirm and remove exactly one older backup in a private mkdtemp sdmc:/ tree (synthetic files).
HOST_UI_RECOVERY_TEST := $(HOST_BUILD)/test_ui_recovery_files_host
HOST_UI_RECOVERY_SANITIZE := $(HOST_BUILD)/test_ui_recovery_files_host_sanitize
HOST_UI_RECOVERY_INPUTS := tests/test_ui_recovery_files_host.cpp tests/host-ui/HostUi.h \
	tests/host-ui/switch.h tests/host-ui/HostUiProductHomeStubs.cpp $(HOST_UI_STUBS) \
	include/UI/RecoveryManagementModel.h include/UI/RecoveryManagementFs.h
HOST_TESTS += $(HOST_UI_RECOVERY_TEST)
HOST_SANITIZE_TESTS += $(HOST_UI_RECOVERY_SANITIZE)
host-test: $(HOST_UI_RECOVERY_TEST)
host-sanitize: $(HOST_UI_RECOVERY_SANITIZE)
$(HOST_UI_RECOVERY_TEST): $(HOST_UI_RECOVERY_INPUTS) $(HOST_UI_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_recovery_files_host.cpp $(HOST_UI_STUBS) tests/host-ui/HostUiProductHomeStubs.cpp $(HOST_UI_ARCHIVE) -o $@
$(HOST_UI_RECOVERY_SANITIZE): $(HOST_UI_RECOVERY_INPUTS) $(HOST_UI_SANITIZE_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_recovery_files_host.cpp $(HOST_UI_STUBS) tests/host-ui/HostUiProductHomeStubs.cpp $(HOST_UI_SANITIZE_ARCHIVE) -o $@
# Owner Oct 28: the keyboard's Japanese Gen I / II name pages through the real KeyboardState.
HOST_UI_KANA_TEST := $(HOST_BUILD)/test_ui_kana_keyboard_host
HOST_UI_KANA_SANITIZE := $(HOST_BUILD)/test_ui_kana_keyboard_host_sanitize
HOST_TESTS += $(HOST_UI_KANA_TEST)
HOST_SANITIZE_TESTS += $(HOST_UI_KANA_SANITIZE)
host-test: $(HOST_UI_KANA_TEST)
host-sanitize: $(HOST_UI_KANA_SANITIZE)
$(HOST_UI_KANA_TEST): tests/test_ui_kana_keyboard_host.cpp include/Utils/Gen12KanaText.h $(HOST_UI_STUBS) tests/host-ui/HostUi.h $(HOST_UI_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_kana_keyboard_host.cpp $(HOST_UI_STUBS) $(HOST_UI_ARCHIVE) -o $@
$(HOST_UI_KANA_SANITIZE): tests/test_ui_kana_keyboard_host.cpp include/Utils/Gen12KanaText.h $(HOST_UI_STUBS) tests/host-ui/HostUi.h $(HOST_UI_SANITIZE_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_kana_keyboard_host.cpp $(HOST_UI_STUBS) $(HOST_UI_SANITIZE_ARCHIVE) -o $@
# Owner Oct 28 (Feature B): Top Up / Add Missing through the real Gen III / IV / V Bag screens
# (ZR -> SETUP -> numpad -> PREVIEW rows inside the modal -> B stages nothing -> A stages -> Review).
HOST_UI_BAG_TOP_UP_TEST := $(HOST_BUILD)/test_ui_bag_top_up_host
HOST_UI_BAG_TOP_UP_SANITIZE := $(HOST_BUILD)/test_ui_bag_top_up_host_sanitize
HOST_UI_BAG_TOP_UP_INPUTS := tests/test_ui_bag_top_up_host.cpp tests/host-ui/HostUi.h tests/host-ui/HostUiFixtures.h \
	tests/fuzz_synthetic_saves.h $(HOST_UI_STUBS)
HOST_TESTS += $(HOST_UI_BAG_TOP_UP_TEST)
HOST_SANITIZE_TESTS += $(HOST_UI_BAG_TOP_UP_SANITIZE)
host-test: $(HOST_UI_BAG_TOP_UP_TEST)
host-sanitize: $(HOST_UI_BAG_TOP_UP_SANITIZE)
$(HOST_UI_BAG_TOP_UP_TEST): $(HOST_UI_BAG_TOP_UP_INPUTS) $(HOST_UI_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_bag_top_up_host.cpp $(HOST_UI_STUBS) $(HOST_UI_ARCHIVE) -o $@
$(HOST_UI_BAG_TOP_UP_SANITIZE): $(HOST_UI_BAG_TOP_UP_INPUTS) $(HOST_UI_SANITIZE_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_bag_top_up_host.cpp $(HOST_UI_STUBS) $(HOST_UI_SANITIZE_ARCHIVE) -o $@
# Feature A UI (owner Oct 28): Pokemon Actions -> Import from file / Export to file, Gen I-V, through
# the real TrainerViewScreen in a private mkdtemp sdmc:/ tree (synthetic saves and files only).
HOST_UI_FILE_MENUS_TEST := $(HOST_BUILD)/test_ui_pokemon_file_menus_host
HOST_UI_FILE_MENUS_SANITIZE := $(HOST_BUILD)/test_ui_pokemon_file_menus_host_sanitize
HOST_UI_FILE_MENUS_INPUTS := tests/test_ui_pokemon_file_menus_host.cpp tests/host-ui/HostUi.h tests/host-ui/HostUiFixtures.h \
	tests/fuzz_synthetic_saves.h tests/fixtures/gsc_pokemon_fixture.h include/UI/PokemonFileIo.h \
	include/UI/PokemonFileDialogModel.h $(HOST_UI_STUBS)
HOST_TESTS += $(HOST_UI_FILE_MENUS_TEST)
HOST_SANITIZE_TESTS += $(HOST_UI_FILE_MENUS_SANITIZE)
host-test: $(HOST_UI_FILE_MENUS_TEST)
host-sanitize: $(HOST_UI_FILE_MENUS_SANITIZE)
$(HOST_UI_FILE_MENUS_TEST): $(HOST_UI_FILE_MENUS_INPUTS) $(HOST_UI_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_pokemon_file_menus_host.cpp $(HOST_UI_STUBS) $(HOST_UI_ARCHIVE) -o $@
$(HOST_UI_FILE_MENUS_SANITIZE): $(HOST_UI_FILE_MENUS_INPUTS) $(HOST_UI_SANITIZE_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_pokemon_file_menus_host.cpp $(HOST_UI_STUBS) $(HOST_UI_SANITIZE_ARCHIVE) -o $@
# Owner Oct 28: Japanese Gen I / II name prompts open the kana keyboard; other saves are unchanged.
HOST_UI_JP_NAMES_TEST := $(HOST_BUILD)/test_ui_japanese_name_prompts_host
HOST_UI_JP_NAMES_SANITIZE := $(HOST_BUILD)/test_ui_japanese_name_prompts_host_sanitize
HOST_TESTS += $(HOST_UI_JP_NAMES_TEST)
HOST_SANITIZE_TESTS += $(HOST_UI_JP_NAMES_SANITIZE)
host-test: $(HOST_UI_JP_NAMES_TEST)
host-sanitize: $(HOST_UI_JP_NAMES_SANITIZE)
$(HOST_UI_JP_NAMES_TEST): tests/test_ui_japanese_name_prompts_host.cpp include/UI/ClassicJapaneseNames.h $(HOST_UI_STUBS) tests/host-ui/HostUi.h $(HOST_UI_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_japanese_name_prompts_host.cpp $(HOST_UI_STUBS) $(HOST_UI_ARCHIVE) -o $@
$(HOST_UI_JP_NAMES_SANITIZE): tests/test_ui_japanese_name_prompts_host.cpp include/UI/ClassicJapaneseNames.h $(HOST_UI_STUBS) tests/host-ui/HostUi.h $(HOST_UI_SANITIZE_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_japanese_name_prompts_host.cpp $(HOST_UI_STUBS) $(HOST_UI_SANITIZE_ARCHIVE) -o $@
endif
# ---- END original Makefile.host.host-ui ----

# ---- BEGIN original Makefile.host.review-entry ----
# Round 6 (B08 / P1-RETURN): Review entry and return through the REAL TrainerViewScreen, Gen I-V
# (tests/host-ui harness from Makefile.host.host-ui, which must be included first).
# Links the application archive, so GCC-only like Makefile.host.host-ui.
ifneq ($(HOST_UI_COMPILER_IS_CLANG),yes)
HOST_UI_REVIEW_TEST := $(HOST_BUILD)/test_ui_review_entry_host
HOST_UI_REVIEW_SANITIZE := $(HOST_BUILD)/test_ui_review_entry_host_sanitize
HOST_TESTS += $(HOST_UI_REVIEW_TEST)
HOST_SANITIZE_TESTS += $(HOST_UI_REVIEW_SANITIZE)
host-test: $(HOST_UI_REVIEW_TEST)
host-sanitize: $(HOST_UI_REVIEW_SANITIZE)
$(HOST_UI_REVIEW_TEST): tests/test_ui_review_entry_host.cpp $(HOST_UI_STUBS) tests/host-ui/HostUi.h $(HOST_UI_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_review_entry_host.cpp $(HOST_UI_STUBS) $(HOST_UI_ARCHIVE) -o $@
$(HOST_UI_REVIEW_SANITIZE): tests/test_ui_review_entry_host.cpp $(HOST_UI_STUBS) tests/host-ui/HostUi.h $(HOST_UI_SANITIZE_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_review_entry_host.cpp $(HOST_UI_STUBS) $(HOST_UI_SANITIZE_ARCHIVE) -o $@

# Round 7 (B08 (1)): Gen II Create -> Review is drawn and visible (Silver / Crystal, Unown and Pikachu).
HOST_UI_GEN2_CREATE_REVIEW := $(HOST_BUILD)/test_ui_gen2_create_review_host
HOST_UI_GEN2_CREATE_REVIEW_SANITIZE := $(HOST_BUILD)/test_ui_gen2_create_review_host_sanitize
HOST_TESTS += $(HOST_UI_GEN2_CREATE_REVIEW)
HOST_SANITIZE_TESTS += $(HOST_UI_GEN2_CREATE_REVIEW_SANITIZE)
host-test: $(HOST_UI_GEN2_CREATE_REVIEW)
host-sanitize: $(HOST_UI_GEN2_CREATE_REVIEW_SANITIZE)
$(HOST_UI_GEN2_CREATE_REVIEW): tests/test_ui_gen2_create_review_host.cpp $(HOST_UI_STUBS) tests/host-ui/HostUi.h $(HOST_UI_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_gen2_create_review_host.cpp $(HOST_UI_STUBS) $(HOST_UI_ARCHIVE) -o $@
$(HOST_UI_GEN2_CREATE_REVIEW_SANITIZE): tests/test_ui_gen2_create_review_host.cpp $(HOST_UI_STUBS) tests/host-ui/HostUi.h $(HOST_UI_SANITIZE_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_ui_gen2_create_review_host.cpp $(HOST_UI_STUBS) $(HOST_UI_SANITIZE_ARCHIVE) -o $@

# Round 7 (7E G1): Gen V exports in Save Backups (real Gen5WorkingCopyExport records, profile scoped).
HOST_UI_GEN5_BACKUPS := $(HOST_BUILD)/test_gen5_backup_listing_host
HOST_UI_GEN5_BACKUPS_SANITIZE := $(HOST_BUILD)/test_gen5_backup_listing_host_sanitize
HOST_TESTS += $(HOST_UI_GEN5_BACKUPS)
HOST_SANITIZE_TESTS += $(HOST_UI_GEN5_BACKUPS_SANITIZE)
host-test: $(HOST_UI_GEN5_BACKUPS)
host-sanitize: $(HOST_UI_GEN5_BACKUPS_SANITIZE)
$(HOST_UI_GEN5_BACKUPS): tests/test_gen5_backup_listing_host.cpp include/UI/SourceBackupClassification.h include/UI/SourceBackupOpen.h $(HOST_UI_STUBS) $(HOST_UI_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_gen5_backup_listing_host.cpp $(HOST_UI_STUBS) $(HOST_UI_ARCHIVE) -o $@
$(HOST_UI_GEN5_BACKUPS_SANITIZE): tests/test_gen5_backup_listing_host.cpp include/UI/SourceBackupClassification.h include/UI/SourceBackupOpen.h $(HOST_UI_STUBS) $(HOST_UI_SANITIZE_ARCHIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter $(HOST_UI_INCLUDES) -Itests -DPOKEBANK_GEN3_SELECTIVE_PORT_TEST tests/test_gen5_backup_listing_host.cpp $(HOST_UI_STUBS) $(HOST_UI_SANITIZE_ARCHIVE) -o $@
endif
# ---- END original Makefile.host.review-entry ----

# ---- BEGIN original Makefile.host.pkse-upstream ----
# PKSE 1.2.x upstream-parity regressions (docs/audits/PKSE_1_2_X_GEN15_GAP_MATRIX.md).
PKSE_UPSTREAM_HOST_TESTS := \
	$(HOST_BUILD)/test_bank_load_policy \
	$(HOST_BUILD)/test_gen5_species_pid_gender \
	$(HOST_BUILD)/test_bank_fault_injection \
	$(HOST_BUILD)/test_gen2_inventory_byte_fidelity \
	$(HOST_BUILD)/test_gen3_inventory_byte_fidelity \
	$(HOST_BUILD)/test_frlg_switch_releases \
	$(HOST_BUILD)/test_gen3_sector_signature_policy

PKSE_UPSTREAM_SANITIZE_TESTS := \
	$(HOST_BUILD)/test_bank_load_policy_sanitize \
	$(HOST_BUILD)/test_gen5_species_pid_gender_sanitize \
	$(HOST_BUILD)/test_bank_fault_injection_sanitize \
	$(HOST_BUILD)/test_gen2_inventory_byte_fidelity_sanitize \
	$(HOST_BUILD)/test_gen3_inventory_byte_fidelity_sanitize \
	$(HOST_BUILD)/test_frlg_switch_releases_sanitize \
	$(HOST_BUILD)/test_gen3_sector_signature_policy_sanitize

HOST_TESTS += $(PKSE_UPSTREAM_HOST_TESTS)
HOST_SANITIZE_TESTS += $(PKSE_UPSTREAM_SANITIZE_TESTS)
host-test: $(PKSE_UPSTREAM_HOST_TESTS)
host-sanitize: $(PKSE_UPSTREAM_SANITIZE_TESTS)

BANK_LOAD_POLICY_SOURCES := tests/test_bank_load_policy.cpp src/Utils/DurableFile.cpp

$(HOST_BUILD)/test_bank_load_policy: $(BANK_LOAD_POLICY_SOURCES) include/Trainer/BankLoadPolicy.h include/Utils/DurableFile.h src/Trainer/Bank.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(BANK_LOAD_POLICY_SOURCES) -o $@

$(HOST_BUILD)/test_bank_load_policy_sanitize: $(BANK_LOAD_POLICY_SOURCES) include/Trainer/BankLoadPolicy.h include/Utils/DurableFile.h src/Trainer/Bank.cpp
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(BANK_LOAD_POLICY_SOURCES) -o $@

GEN5_SPECIES_PID_GENDER_DEPS := tests/test_gen5_species_pid_gender.cpp $(wildcard include/Integration/Gen5/*.h)

$(HOST_BUILD)/test_gen5_species_pid_gender: $(GEN5_SPECIES_PID_GENDER_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_gen5_species_pid_gender.cpp src/Names/SpeciesNames.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp -o $@

$(HOST_BUILD)/test_gen5_species_pid_gender_sanitize: $(GEN5_SPECIES_PID_GENDER_DEPS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_gen5_species_pid_gender.cpp src/Names/SpeciesNames.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp -o $@

# Q01: the production Bank linked with the real DurableFile and a fault-injecting readAllBytes.
BANK_FAULT_INJECTION_SOURCES := tests/test_bank_fault_injection.cpp src/Trainer/Bank.cpp src/Utils/DurableFile.cpp src/Pokemon/Pokemon3FRLG.cpp src/Pokemon/Pokemon7LGPE.cpp src/Pokemon/Pokemon8SWSH.cpp src/Pokemon/Pokemon8BDSP.cpp src/Pokemon/Pokemon8LA.cpp src/Pokemon/Pokemon9SV.cpp src/Pokemon/Pokemon9LZA.cpp src/Pokemon/BaseStatsGen7.cpp src/Pokemon/BaseStatsGen89.cpp src/Pokemon/PersonalInfoTable.cpp src/Pokemon/PokemonTypes.cpp src/Pokemon/LearnsetTable.cpp src/Pokemon/Experience.cpp src/Pokemon/FormInfo.cpp src/Encryption/Encryption.cpp src/Save/Block.cpp src/Enums/SCTypeCode.cpp src/Encryption/Encryption3FRLG.cpp src/Encryption/Encryption7LGPE.cpp src/Encryption/Encryption8SWSH.cpp src/Encryption/Encryption8BDSP.cpp src/Encryption/Encryption8LA.cpp src/Encryption/Encryption9SV.cpp src/Encryption/Encryption9LZA.cpp src/Names/ItemNames.cpp src/Names/ItemPresence.cpp src/Names/MoveInfo.cpp src/Names/SpeciesNames.cpp src/Names/NatureNames.cpp src/Names/AbilityNames.cpp src/Utils/HelperUtilities.cpp src/Utils/StringHelpers.cpp src/Utils/SHA256.cpp

$(HOST_BUILD)/test_bank_fault_injection: $(BANK_FAULT_INJECTION_SOURCES) include/Trainer/Bank.h include/Trainer/BankLoadPolicy.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(BANK_FAULT_INJECTION_SOURCES) -o $@

$(HOST_BUILD)/test_bank_fault_injection_sanitize: $(BANK_FAULT_INJECTION_SOURCES) include/Trainer/Bank.h include/Trainer/BankLoadPolicy.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(BANK_FAULT_INJECTION_SOURCES) -o $@

# Q07: Gen II / Gen III Bag edits are byte-surgical (native list and slot semantics).
GEN2_INVENTORY_FIDELITY_SOURCES := tests/test_gen2_inventory_byte_fidelity.cpp src/Integration/Gen2/Gen2StagedEditor.cpp src/Integration/Gen2/Gen2UniversalInventory.cpp src/Integration/Gen2/Gen2ReadOnlySave.cpp src/Integration/Gen2/Gen2ReadOnlyInventory.cpp src/Integration/Gen2/Gen2PersonalData.cpp src/Inventory/ClassicInventoryCatalog.cpp src/Inventory/ItemQuantityPolicy.cpp src/Names/ItemNames.cpp src/Names/MoveNames.cpp src/Names/SpeciesNames.cpp src/Pokemon/Experience.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp
GEN3_INVENTORY_FIDELITY_SOURCES := tests/test_gen3_inventory_byte_fidelity.cpp src/Integration/Gen3/Gen3StagedInventoryEditor.cpp \
	src/Inventory/ClassicInventoryCatalog.cpp src/Inventory/ItemQuantityPolicy.cpp src/Names/ItemNames.cpp \
	src/Names/MoveNames.cpp src/Integration/Gen2/Gen2ReadOnlyInventory.cpp

$(HOST_BUILD)/test_gen2_inventory_byte_fidelity: $(GEN2_INVENTORY_FIDELITY_SOURCES) include/Integration/Gen2/Gen2NativeItemList.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN2_INVENTORY_FIDELITY_SOURCES) -o $@

$(HOST_BUILD)/test_gen2_inventory_byte_fidelity_sanitize: $(GEN2_INVENTORY_FIDELITY_SOURCES) include/Integration/Gen2/Gen2NativeItemList.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN2_INVENTORY_FIDELITY_SOURCES) -o $@

$(HOST_BUILD)/test_gen3_inventory_byte_fidelity: $(GEN3_INVENTORY_FIDELITY_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN3_INVENTORY_FIDELITY_SOURCES) -o $@

$(HOST_BUILD)/test_gen3_inventory_byte_fidelity_sanitize: $(GEN3_INVENTORY_FIDELITY_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN3_INVENTORY_FIDELITY_SOURCES) -o $@

# Q04: all twelve FireRed/LeafGreen Switch releases, exact identities and backup namespaces.
FRLG_SWITCH_RELEASES_SOURCES := tests/test_frlg_switch_releases.cpp src/Games/GameIdentity.cpp

$(HOST_BUILD)/test_frlg_switch_releases: $(FRLG_SWITCH_RELEASES_SOURCES) include/Enums/GameVersion.h include/Games/GameIdentity.h include/Utils/MoveTransactionProduction.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(FRLG_SWITCH_RELEASES_SOURCES) -o $@

$(HOST_BUILD)/test_frlg_switch_releases_sanitize: $(FRLG_SWITCH_RELEASES_SOURCES) include/Enums/GameVersion.h include/Games/GameIdentity.h include/Utils/MoveTransactionProduction.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(FRLG_SWITCH_RELEASES_SOURCES) -o $@

# Q05: Gen III sector signature kept, matching the games' own loader (pret pokefirered src/save.c).
$(HOST_BUILD)/test_gen3_sector_signature_policy: tests/test_gen3_sector_signature_policy.cpp include/Integration/Gen3/Gen3SaveValidation.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_gen3_sector_signature_policy.cpp -o $@

$(HOST_BUILD)/test_gen3_sector_signature_policy_sanitize: tests/test_gen3_sector_signature_policy.cpp include/Integration/Gen3/Gen3SaveValidation.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_gen3_sector_signature_policy.cpp -o $@

# Round 8 automated compatibility matrix (docs/qa/ROUND8_AUTOMATED_COMPATIBILITY_MATRIX.md): build and
# run every matrix test, plain and AddressSanitizer/UBSan, then the registration guard.
ROUND8_MATRIX_TESTS := $(shell sed -n '/round8-matrix-tests:begin/,/round8-matrix-tests:end/p' docs/qa/ROUND8_AUTOMATED_COMPATIBILITY_MATRIX.md | grep '^test_')
.PHONY: round8-compat-matrix
round8-compat-matrix: $(addprefix $(HOST_BUILD)/,$(ROUND8_MATRIX_TESTS)) $(addsuffix _sanitize,$(addprefix $(HOST_BUILD)/,$(ROUND8_MATRIX_TESTS)))
	@set -e; for t in $^; do echo "== $$t"; $$t; done
	python3 tests/test_round8_compat_matrix.py
# ---- END original Makefile.host.pkse-upstream ----

# ---- BEGIN original Makefile.host.multilingual ----
# Multilingual Gen I-V naming (feature/multilingual-gen15). See docs/multilingual/.
MULTILINGUAL_HOST_TESTS := \
	$(HOST_BUILD)/test_default_species_names \
	$(HOST_BUILD)/test_gen5_language_default_name

MULTILINGUAL_SANITIZE_TESTS := \
	$(HOST_BUILD)/test_default_species_names_sanitize \
	$(HOST_BUILD)/test_gen5_language_default_name_sanitize

HOST_TESTS += $(MULTILINGUAL_HOST_TESTS)
HOST_SANITIZE_TESTS += $(MULTILINGUAL_SANITIZE_TESTS)
host-test: $(MULTILINGUAL_HOST_TESTS)
host-sanitize: $(MULTILINGUAL_SANITIZE_TESTS)

DEFAULT_SPECIES_NAMES_SOURCES := tests/test_default_species_names.cpp src/Names/SpeciesNames.cpp \
	src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp

$(HOST_BUILD)/test_default_species_names: $(DEFAULT_SPECIES_NAMES_SOURCES) include/Names/DefaultSpeciesName.h include/Utils/Gen3Text.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(DEFAULT_SPECIES_NAMES_SOURCES) -o $@

$(HOST_BUILD)/test_default_species_names_sanitize: $(DEFAULT_SPECIES_NAMES_SOURCES) include/Names/DefaultSpeciesName.h include/Utils/Gen3Text.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(DEFAULT_SPECIES_NAMES_SOURCES) -o $@

GEN5_LANGUAGE_DEFAULT_NAME_SOURCES := tests/test_gen5_language_default_name.cpp src/Names/SpeciesNames.cpp \
	src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp

$(HOST_BUILD)/test_gen5_language_default_name: $(GEN5_LANGUAGE_DEFAULT_NAME_SOURCES) $(wildcard include/Integration/Gen5/*.h) include/Names/DefaultSpeciesName.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(GEN5_LANGUAGE_DEFAULT_NAME_SOURCES) -o $@

$(HOST_BUILD)/test_gen5_language_default_name_sanitize: $(GEN5_LANGUAGE_DEFAULT_NAME_SOURCES) $(wildcard include/Integration/Gen5/*.h) include/Names/DefaultSpeciesName.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(GEN5_LANGUAGE_DEFAULT_NAME_SOURCES) -o $@
# ---- END original Makefile.host.multilingual ----
