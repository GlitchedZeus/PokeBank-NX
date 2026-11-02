# PokeBank NX host test rules: sources.mk
# Consolidated from the original Makefile.host.* fragments in their EXACT include order.
# Do not modify host-test recipes or weaken coverage while reorganizing infrastructure.

# ---- BEGIN original Makefile.host.source-instances ----
# Provider-neutral Save Instances model gate. Generation-specific parsers are exercised by their
# existing suites; this pins the shared metadata/dedupe/order/profile contract itself.
SAVE_INSTANCE_MODEL_TEST := $(HOST_BUILD)/test_save_instance_model
SAVE_INSTANCE_MODEL_SANITIZE := $(HOST_BUILD)/test_save_instance_model_sanitize

HOST_TESTS += $(SAVE_INSTANCE_MODEL_TEST)
HOST_SANITIZE_TESTS += $(SAVE_INSTANCE_MODEL_SANITIZE)
host-test: $(SAVE_INSTANCE_MODEL_TEST)
host-sanitize: $(SAVE_INSTANCE_MODEL_SANITIZE)

$(SAVE_INSTANCE_MODEL_TEST): tests/test_save_instance_model.cpp include/Source/SaveInstance.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $< -o $@

$(SAVE_INSTANCE_MODEL_SANITIZE): tests/test_save_instance_model.cpp include/Source/SaveInstance.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $< -o $@

.PHONY: save-instance-architecture-contract
save-instance-architecture-contract:
	python3 tests/test_save_instance_architecture_contract.py

host-test: save-instance-architecture-contract
host-sanitize: save-instance-architecture-contract
# ---- END original Makefile.host.source-instances ----

# ---- BEGIN original Makefile.host.product-ui ----
# Product-shell model tests are intentionally header-only: they prove navigation and honest
# capability classification without pulling libnx/UI rendering into the host suite.
PRODUCT_UI_TESTS := $(HOST_BUILD)/test_app_shell_model $(HOST_BUILD)/test_organization_preview_model $(HOST_BUILD)/test_game_launch_model $(HOST_BUILD)/test_quick_games_drawer_motion
PRODUCT_UI_SANITIZE_TESTS := $(HOST_BUILD)/test_app_shell_model_sanitize $(HOST_BUILD)/test_organization_preview_model_sanitize $(HOST_BUILD)/test_game_launch_model_sanitize $(HOST_BUILD)/test_quick_games_drawer_motion_sanitize

HOST_TESTS += $(PRODUCT_UI_TESTS)
HOST_SANITIZE_TESTS += $(PRODUCT_UI_SANITIZE_TESTS)

# This fragment is included after Makefile.host.base defines host-test/host-sanitize. GNU make
# expands target prerequisites when the rule is parsed, so appending the variables above is not
# enough to make those late-added binaries build before the base recipe iterates HOST_TESTS.
# Add explicit prerequisite-only rules here so the global suites compile these tests first.
host-test: $(PRODUCT_UI_TESTS)
host-sanitize: $(PRODUCT_UI_SANITIZE_TESTS)

.PHONY: product-ui-test
product-ui-test: $(PRODUCT_UI_TESTS)
	@set -e; for test_bin in $(PRODUCT_UI_TESTS); do $$test_bin; done

$(HOST_BUILD)/test_app_shell_model: tests/test_app_shell_model.cpp include/UI/AppShellModel.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_app_shell_model.cpp -o $@

$(HOST_BUILD)/test_app_shell_model_sanitize: tests/test_app_shell_model.cpp include/UI/AppShellModel.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_app_shell_model.cpp -o $@

$(HOST_BUILD)/test_organization_preview_model: tests/test_organization_preview_model.cpp include/UI/OrganizationPreviewModel.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_organization_preview_model.cpp -o $@

$(HOST_BUILD)/test_organization_preview_model_sanitize: tests/test_organization_preview_model.cpp include/UI/OrganizationPreviewModel.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_organization_preview_model.cpp -o $@

$(HOST_BUILD)/test_game_launch_model: tests/test_game_launch_model.cpp include/UI/GameLaunchModel.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_game_launch_model.cpp -o $@

$(HOST_BUILD)/test_game_launch_model_sanitize: tests/test_game_launch_model.cpp include/UI/GameLaunchModel.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_game_launch_model.cpp -o $@

$(HOST_BUILD)/test_quick_games_drawer_motion: tests/test_quick_games_drawer_motion.cpp include/UI/QuickGamesDrawerMotion.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_quick_games_drawer_motion.cpp -o $@

$(HOST_BUILD)/test_quick_games_drawer_motion_sanitize: tests/test_quick_games_drawer_motion.cpp include/UI/QuickGamesDrawerMotion.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_quick_games_drawer_motion.cpp -o $@

# P0 #320: Product Home quit guard + opt-in input trace.
HOME_INPUT_GUARD_TEST := $(HOST_BUILD)/test_home_input_guard
HOME_INPUT_GUARD_SANITIZE := $(HOST_BUILD)/test_home_input_guard_sanitize
HOME_INPUT_GUARD_HEADERS := include/UI/HomeQuitConfirm.h include/UI/InputTrace.h
HOST_TESTS += $(HOME_INPUT_GUARD_TEST)
HOST_SANITIZE_TESTS += $(HOME_INPUT_GUARD_SANITIZE)
host-test: $(HOME_INPUT_GUARD_TEST)
host-sanitize: $(HOME_INPUT_GUARD_SANITIZE)

$(HOME_INPUT_GUARD_TEST): tests/test_home_input_guard.cpp $(HOME_INPUT_GUARD_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $< -o $@

$(HOME_INPUT_GUARD_SANITIZE): tests/test_home_input_guard.cpp $(HOME_INPUT_GUARD_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $< -o $@

# Round 2 phase 7: emulator-save backups (original source read-only, app-owned export records only).
EMU_BACKUP_TEST := $(HOST_BUILD)/test_emulator_backup_access
EMU_BACKUP_SANITIZE := $(HOST_BUILD)/test_emulator_backup_access_sanitize
EMU_BACKUP_SOURCES := tests/test_emulator_backup_access.cpp \
	src/Integration/Common/VerifiedStagedExport.cpp src/Utils/SHA256.cpp
EMU_BACKUP_INPUTS := $(EMU_BACKUP_SOURCES) include/UI/SourceBackupClassification.h \
	include/UI/WorkingCopyRefresh.h include/UI/ClassicSourceGuard.h include/Source/ImmutableSourceSnapshot.h \
	include/Integration/Common/VerifiedStagedExport.h
HOST_TESTS += $(EMU_BACKUP_TEST)
HOST_SANITIZE_TESTS += $(EMU_BACKUP_SANITIZE)
host-test: $(EMU_BACKUP_TEST)
host-sanitize: $(EMU_BACKUP_SANITIZE)

$(EMU_BACKUP_TEST): $(EMU_BACKUP_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(EMU_BACKUP_SOURCES) -o $@

$(EMU_BACKUP_SANITIZE): $(EMU_BACKUP_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(EMU_BACKUP_SOURCES) -o $@

# Round 6 (B08 / P1-RETURN): Review entry from every editor screen, Gen I-IV return to the screen.
REVIEW_ENTRY_TEST := $(HOST_BUILD)/test_review_entry
REVIEW_ENTRY_SANITIZE := $(HOST_BUILD)/test_review_entry_sanitize
REVIEW_ENTRY_INPUTS := tests/test_review_entry.cpp include/UI/ReviewEntry.h include/UI/ClassicBrowseReview.h \
	include/UI/ClassicReviewOrigin.h include/UI/Gen2WorkspaceLayers.h include/UI/InventoryUIContract.h
HOST_TESTS += $(REVIEW_ENTRY_TEST)
HOST_SANITIZE_TESTS += $(REVIEW_ENTRY_SANITIZE)
host-test: $(REVIEW_ENTRY_TEST)
host-sanitize: $(REVIEW_ENTRY_SANITIZE)

$(REVIEW_ENTRY_TEST): $(REVIEW_ENTRY_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude tests/test_review_entry.cpp -o $@

$(REVIEW_ENTRY_SANITIZE): $(REVIEW_ENTRY_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude tests/test_review_entry.cpp -o $@

# Round 2 phase 13 / L06: party and summary-panel taps open the native Actions sheet (Gen III-V).
L06_TAP_TEST := $(HOST_BUILD)/test_party_summary_tap_routing
L06_TAP_SANITIZE := $(HOST_BUILD)/test_party_summary_tap_routing_sanitize
L06_TAP_INPUTS := tests/test_party_summary_tap_routing.cpp include/UI/Gen4BoxTouchActions.h \
	include/Integration/Gen5/Gen5BoxTouchNavigation.h src/UI/Gen3SharedPokemonSurface.inc \
	src/UI/Gen4SharedPokemonSurface.inc src/UI/Gen5SharedReadOnlySurface.inc
HOST_TESTS += $(L06_TAP_TEST)
HOST_SANITIZE_TESTS += $(L06_TAP_SANITIZE)
host-test: $(L06_TAP_TEST)
host-sanitize: $(L06_TAP_SANITIZE)

$(L06_TAP_TEST): $(L06_TAP_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $< -o $@

$(L06_TAP_SANITIZE): $(L06_TAP_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $< -o $@

# R2-LAUNCH: the leave decision follows SharedMenuGeometry::LaunchConfirm and every line fits.
LEAVE_LAYOUT_TEST := $(HOST_BUILD)/test_leave_decision_layout
LEAVE_LAYOUT_SANITIZE := $(HOST_BUILD)/test_leave_decision_layout_sanitize
LEAVE_LAYOUT_INPUTS := tests/test_leave_decision_layout.cpp include/UI/LeaveDecisionLayout.h \
	include/UI/SharedMenuGeometry.h include/UI/NunitoMetrics.h include/UI/GameLaunchModel.h \
	include/UI/HomeQuitConfirm.h
HOST_TESTS += $(LEAVE_LAYOUT_TEST)
HOST_SANITIZE_TESTS += $(LEAVE_LAYOUT_SANITIZE)
host-test: $(LEAVE_LAYOUT_TEST)
host-sanitize: $(LEAVE_LAYOUT_SANITIZE)

$(LEAVE_LAYOUT_TEST): $(LEAVE_LAYOUT_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $< -o $@

$(LEAVE_LAYOUT_SANITIZE): $(LEAVE_LAYOUT_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $< -o $@

# CG13: the emulator backups list lays out through SharedMenuGeometry::BackupMenu and its text fits.
EMU_BACKUP_LAYOUT_TEST := $(HOST_BUILD)/test_emulator_backup_layout
EMU_BACKUP_LAYOUT_SANITIZE := $(HOST_BUILD)/test_emulator_backup_layout_sanitize
EMU_BACKUP_LAYOUT_INPUTS := tests/test_emulator_backup_layout.cpp include/UI/EmulatorBackupLayout.h src/UI/SaveSelectScreen.cpp \
	include/UI/SharedMenuGeometry.h include/UI/NunitoMetrics.h include/UI/SourceBackupClassification.h
HOST_TESTS += $(EMU_BACKUP_LAYOUT_TEST)
HOST_SANITIZE_TESTS += $(EMU_BACKUP_LAYOUT_SANITIZE)
host-test: $(EMU_BACKUP_LAYOUT_TEST)
host-sanitize: $(EMU_BACKUP_LAYOUT_SANITIZE)

$(EMU_BACKUP_LAYOUT_TEST): $(EMU_BACKUP_LAYOUT_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $< -o $@

$(EMU_BACKUP_LAYOUT_SANITIZE): $(EMU_BACKUP_LAYOUT_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $< -o $@

# R2-BUG-013: help rows and modal instructions name every controller button as a coloured badge.
GLYPH_INSTRUCTION_TEST := $(HOST_BUILD)/test_glyph_instruction
GLYPH_INSTRUCTION_SANITIZE := $(HOST_BUILD)/test_glyph_instruction_sanitize
GLYPH_INSTRUCTION_INPUTS := tests/test_glyph_instruction.cpp include/UI/GlyphInstruction.h
HOST_TESTS += $(GLYPH_INSTRUCTION_TEST)
HOST_SANITIZE_TESTS += $(GLYPH_INSTRUCTION_SANITIZE)
host-test: $(GLYPH_INSTRUCTION_TEST)
host-sanitize: $(GLYPH_INSTRUCTION_SANITIZE)

$(GLYPH_INSTRUCTION_TEST): $(GLYPH_INSTRUCTION_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $< -o $@

$(GLYPH_INSTRUCTION_SANITIZE): $(GLYPH_INSTRUCTION_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $< -o $@

# Round 3 Part 9: each provider is named in the session header / leave dialog; profile focus kept.
SESSION_PROVIDER_TEST := $(HOST_BUILD)/test_session_provider_names
SESSION_PROVIDER_SANITIZE := $(HOST_BUILD)/test_session_provider_names_sanitize
SESSION_PROVIDER_INPUTS := tests/test_session_provider_names.cpp include/Source/SaveProviderIdentity.h \
	include/Source/SaveInstance.h include/UI/LeaveDecisionLayout.h include/UI/ProfileTitleFocus.h \
	include/UI/SharedMenuGeometry.h include/UI/NunitoMetrics.h
HOST_TESTS += $(SESSION_PROVIDER_TEST)
HOST_SANITIZE_TESTS += $(SESSION_PROVIDER_SANITIZE)
host-test: $(SESSION_PROVIDER_TEST)
host-sanitize: $(SESSION_PROVIDER_SANITIZE)

$(SESSION_PROVIDER_TEST): $(SESSION_PROVIDER_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $< -o $@

$(SESSION_PROVIDER_SANITIZE): $(SESSION_PROVIDER_INPUTS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $< -o $@
# ---- END original Makefile.host.product-ui ----

# ---- BEGIN original Makefile.host.exports ----
# Verified app-owned working-copy exports shared by Gen I-IV (Integration/Common).
CLASSIC_EXPORT_SOURCES := tests/test_classic_verified_export.cpp \
	src/Integration/Common/VerifiedStagedExport.cpp \
	src/Integration/Gen1/Gen1ReadOnlySave.cpp src/Integration/Gen2/Gen2ReadOnlySave.cpp \
	src/Utils/SHA256.cpp src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp
CLASSIC_EXPORT_HEADERS := include/Integration/Common/VerifiedStagedExport.h \
	include/Integration/Common/ClassicStrictValidators.h

HOST_TESTS += $(HOST_BUILD)/test_classic_verified_export
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_classic_verified_export_sanitize
host-test: $(HOST_BUILD)/test_classic_verified_export
host-sanitize: $(HOST_BUILD)/test_classic_verified_export_sanitize

$(HOST_BUILD)/test_classic_verified_export: $(CLASSIC_EXPORT_SOURCES) $(CLASSIC_EXPORT_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(CLASSIC_EXPORT_SOURCES) -o $@

$(HOST_BUILD)/test_classic_verified_export_sanitize: $(CLASSIC_EXPORT_SOURCES) $(CLASSIC_EXPORT_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(CLASSIC_EXPORT_SOURCES) -o $@

# Optional verbatim source-file backup + container facts (Gen IV .dsv) on the shared export.
VERBATIM_EXPORT_SOURCES := tests/test_verified_export_verbatim.cpp \
	src/Integration/Common/VerifiedStagedExport.cpp src/Utils/SHA256.cpp

HOST_TESTS += $(HOST_BUILD)/test_verified_export_verbatim
HOST_SANITIZE_TESTS += $(HOST_BUILD)/test_verified_export_verbatim_sanitize
host-test: $(HOST_BUILD)/test_verified_export_verbatim
host-sanitize: $(HOST_BUILD)/test_verified_export_verbatim_sanitize

$(HOST_BUILD)/test_verified_export_verbatim: $(VERBATIM_EXPORT_SOURCES) include/Integration/Common/VerifiedStagedExport.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(VERBATIM_EXPORT_SOURCES) -o $@

$(HOST_BUILD)/test_verified_export_verbatim_sanitize: $(VERBATIM_EXPORT_SOURCES) include/Integration/Common/VerifiedStagedExport.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(VERBATIM_EXPORT_SOURCES) -o $@
# ---- END original Makefile.host.exports ----

# ---- BEGIN original Makefile.host.home-providers ----
# Thread 2H: Home card provider identity + fingerprinted current-save snapshot
# for every generation (Gen I-V). Pure model gate plus real Gen IV / Gen V
# synthetic-save gates through the same request/parse wiring as the UI.
HOME_PROVIDERS_TEST := $(HOST_BUILD)/test_home_card_providers
HOME_PROVIDERS_SANITIZE := $(HOST_BUILD)/test_home_card_providers_sanitize
HOME_PROVIDERS_HEADERS := include/Source/HomeCardSnapshot.h include/Source/SaveProviderIdentity.h \
    include/Source/SaveInstance.h include/Legacy/LegacySourceBindings.h

HOME_GEN5_TEST := $(HOST_BUILD)/test_home_card_gen5_snapshot
HOME_GEN5_SANITIZE := $(HOST_BUILD)/test_home_card_gen5_snapshot_sanitize
HOME_GEN5_SOURCES := tests/test_home_card_gen5_snapshot.cpp \
    src/Integration/Gen5/Gen5SourceDiscovery.cpp src/Utils/SHA256.cpp src/Games/GameIdentity.cpp \
    src/Integration/Gen5/Gen5AssignedSource.cpp src/Legacy/LegacySourceBindings.cpp
HOME_GEN5_HEADERS := $(HOME_PROVIDERS_HEADERS) $(wildcard include/Integration/Gen5/*.h) \
    include/Pokemon/Pokemon5ReadOnly.h include/Encryption/Encryption5.h include/Utils/CRC16.h

HOME_GEN4_TEST := $(HOST_BUILD)/test_home_card_gen4_snapshot
HOME_GEN4_SANITIZE := $(HOST_BUILD)/test_home_card_gen4_snapshot_sanitize
HOME_GEN4_SOURCES := tests/test_home_card_gen4_snapshot.cpp \
    src/Integration/Gen4/Gen4ReadOnlySave.cpp \
    src/Pokemon/Pokemon4ReadOnly.cpp src/Pokemon/Pokemon4ReadOnlyView.cpp src/Pokemon/PersonalRecord4.cpp \
    src/Pokemon/PersonalInfo4DP.cpp src/Pokemon/PersonalInfo4PT.cpp src/Pokemon/PersonalInfo4HGSS.cpp \
    src/Encryption/Encryption4.cpp src/Utils/SHA256.cpp \
    src/Integration/Gen4/Gen4AssignedSource.cpp src/Integration/Gen4/Gen4SourceDiscovery.cpp \
    src/Legacy/LegacySourceBindings.cpp src/Names/SpeciesNames.cpp src/Pokemon/Experience.cpp \
    src/Utils/StringHelpers.cpp src/Utils/HelperUtilities.cpp \
    vendor/PKSM-Core/source/utils/crypto_checksums.cpp
HOME_GEN4_HEADERS := $(HOME_PROVIDERS_HEADERS) $(wildcard include/Integration/Gen4/*.h) \
    include/Encryption/Encryption4.h

# Gen I-III Home party adapters (the same slot fields as Gen IV / Gen V).
HOME_PARTY_TEST := $(HOST_BUILD)/test_home_party_preview
HOME_PARTY_SANITIZE := $(HOST_BUILD)/test_home_party_preview_sanitize
HOME_PARTY_SOURCES := tests/test_home_party_preview.cpp src/Integration/Gen2/Gen2PersonalData.cpp
HOME_PARTY_HEADERS := $(HOME_PROVIDERS_HEADERS) include/Integration/Common/HomePartyPreview.h \
    include/Conversion/Gen3PidSearch.h include/Integration/Gen2/Gen2PersonalData.h

HOST_TESTS += $(HOME_PROVIDERS_TEST) $(HOME_GEN5_TEST) $(HOME_GEN4_TEST) $(HOME_PARTY_TEST)
HOST_SANITIZE_TESTS += $(HOME_PROVIDERS_SANITIZE) $(HOME_GEN5_SANITIZE) $(HOME_GEN4_SANITIZE) \
    $(HOME_PARTY_SANITIZE)
host-test: $(HOME_PROVIDERS_TEST) $(HOME_GEN5_TEST) $(HOME_GEN4_TEST) $(HOME_PARTY_TEST)
host-sanitize: $(HOME_PROVIDERS_SANITIZE) $(HOME_GEN5_SANITIZE) $(HOME_GEN4_SANITIZE) \
    $(HOME_PARTY_SANITIZE)

$(HOME_PARTY_TEST): $(HOME_PARTY_SOURCES) $(HOME_PARTY_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $(HOME_PARTY_SOURCES) -o $@

$(HOME_PARTY_SANITIZE): $(HOME_PARTY_SOURCES) $(HOME_PARTY_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $(HOME_PARTY_SOURCES) -o $@

$(HOME_PROVIDERS_TEST): tests/test_home_card_providers.cpp $(HOME_PROVIDERS_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Iinclude $< -o $@

$(HOME_PROVIDERS_SANITIZE): tests/test_home_card_providers.cpp $(HOME_PROVIDERS_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Iinclude $< -o $@

$(HOME_GEN5_TEST): $(HOME_GEN5_SOURCES) $(HOME_GEN5_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(HOME_GEN5_SOURCES) -o $@

$(HOME_GEN5_SANITIZE): $(HOME_GEN5_SOURCES) $(HOME_GEN5_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(HOME_GEN5_SOURCES) -o $@

$(HOME_GEN4_TEST): $(HOME_GEN4_SOURCES) $(HOME_GEN4_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude -Ivendor/PKSM-Core/include $(HOME_GEN4_SOURCES) -o $@

$(HOME_GEN4_SANITIZE): $(HOME_GEN4_SOURCES) $(HOME_GEN4_HEADERS)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude -Ivendor/PKSM-Core/include $(HOME_GEN4_SOURCES) -o $@

.PHONY: home-card-provider-contract
home-card-provider-contract:
	python3 tests/test_home_card_provider_contract.py
host-test: home-card-provider-contract
host-sanitize: home-card-provider-contract
# ---- END original Makefile.host.home-providers ----
