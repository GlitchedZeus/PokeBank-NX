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
