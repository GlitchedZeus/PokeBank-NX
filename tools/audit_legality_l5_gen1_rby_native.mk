# L5-only actual-production Gen I R/B/Y source-context differential.
# Include AFTER full Makefile.host on either accepted baseline or coordinator checkout.
# L5 source fixture lives in parent repository, not in either production checkout.
L5_G1_QA_CPP := ../tests/fixtures/legality_l5_gen1_rby_actual_native.cpp
L5_G1_QA_GRAPH := $(filter-out tests/test_gen34_ball_domain_analyzer.cpp,$(GEN34_BALL_ANALYZER_SOURCES))
L5_G1_VARIANT ?= baseline
L5_G1_BUILD := $(HOST_BUILD)/l5_gen1_rby_$(L5_G1_VARIANT)
L5_G1_SAN_BUILD := $(HOST_BUILD)/l5_gen1_rby_$(L5_G1_VARIANT)_san
.PHONY: l5-gen1-rby l5-gen1-rby-san
$(L5_G1_BUILD): $(L5_G1_QA_CPP) $(L5_G1_QA_GRAPH)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(L5_G1_QA_CPP) $(L5_G1_QA_GRAPH) -o $@
$(L5_G1_SAN_BUILD): $(L5_G1_QA_CPP) $(L5_G1_QA_GRAPH)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(L5_G1_QA_CPP) $(L5_G1_QA_GRAPH) -o $@
l5-gen1-rby: $(L5_G1_BUILD)
	./$(L5_G1_BUILD) --$(L5_G1_VARIANT)
l5-gen1-rby-san: $(L5_G1_SAN_BUILD)
	ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./$(L5_G1_SAN_BUILD) --$(L5_G1_VARIANT)

# Coordinator-only cross-generation retained checks; do not change source logic.
L5_G1_GEN2_SOURCE := ../tests/fixtures/legality_l5_pk12_sourcefree_native_probe.cpp
L5_G1_GEN2_BUILD := $(HOST_BUILD)/l5_gen2_4_neighbor_guard
L5_G1_GEN2_SAN_BUILD := $(HOST_BUILD)/l5_gen2_4_neighbor_guard_san
.PHONY: l5-gen2-4-guard l5-gen2-4-guard-san
$(L5_G1_GEN2_BUILD): $(L5_G1_GEN2_SOURCE) $(L5_G1_QA_GRAPH)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(L5_G1_GEN2_SOURCE) $(L5_G1_QA_GRAPH) -o $@
$(L5_G1_GEN2_SAN_BUILD): $(L5_G1_GEN2_SOURCE) $(L5_G1_QA_GRAPH)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(L5_G1_GEN2_SOURCE) $(L5_G1_QA_GRAPH) -o $@
l5-gen2-4-guard: $(L5_G1_GEN2_BUILD)
	./$(L5_G1_GEN2_BUILD) --after-fix
l5-gen2-4-guard-san: $(L5_G1_GEN2_SAN_BUILD)
	ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./$(L5_G1_GEN2_SAN_BUILD) --after-fix

# A new independence gate for all original Gen I National Dex species.
# Use actual Gen1ReadOnlySave.cpp for exact internal ID mapping; remove any
# earlier graph appearance before appending to prevent duplicate definitions.
L5_G1_ALLDEX_CPP := ../tests/fixtures/legality_l5_gen1_all_151_sourcefree_native.cpp
L5_G1_ALLDEX_GRAPH := $(filter-out tests/test_gen34_ball_domain_analyzer.cpp src/Integration/Gen1/Gen1ReadOnlySave.cpp,$(GEN34_BALL_ANALYZER_SOURCES)) \
 src/Integration/Gen1/Gen1ReadOnlySave.cpp
L5_G1_ALLDEX_NORMAL := $(HOST_BUILD)/l5_gen1_all_151_sourcefree
L5_G1_ALLDEX_SAN := $(HOST_BUILD)/l5_gen1_all_151_sourcefree_san
.PHONY: l5-gen1-all151 l5-gen1-all151-san
$(L5_G1_ALLDEX_NORMAL): $(L5_G1_ALLDEX_CPP) $(L5_G1_ALLDEX_GRAPH)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(L5_G1_ALLDEX_CPP) $(L5_G1_ALLDEX_GRAPH) -o $@
$(L5_G1_ALLDEX_SAN): $(L5_G1_ALLDEX_CPP) $(L5_G1_ALLDEX_GRAPH)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(L5_G1_ALLDEX_CPP) $(L5_G1_ALLDEX_GRAPH) -o $@
l5-gen1-all151: $(L5_G1_ALLDEX_NORMAL)
	./$(L5_G1_ALLDEX_NORMAL)
l5-gen1-all151-san: $(L5_G1_ALLDEX_SAN)
	ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./$(L5_G1_ALLDEX_SAN)
