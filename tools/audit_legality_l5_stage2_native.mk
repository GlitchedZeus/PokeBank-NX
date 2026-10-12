# L5 Stage 2. Include after Makefile.host / supported original analyzer graph.
# Baseline is expected to reveal F3/F4 and F5 in the accepted L5 parent.
# Coordinator target requires F3/F4 guard fixes; after-f5 requires native egg stat exp fix.
L5_STAGE2_PROBE := tests/fixtures/legality_l5_stage2_gen12_native_analyzer_matrix.cpp \
 $(filter-out tests/test_gen34_ball_domain_analyzer.cpp,$(GEN34_BALL_ANALYZER_SOURCES))
L5_STAGE2_NORMAL := $(HOST_BUILD)/legality_l5_stage2_gen12
L5_STAGE2_SAN := $(HOST_BUILD)/legality_l5_stage2_gen12_san
.PHONY: l5-stage2-baseline l5-stage2-baseline-sanitize l5-stage2-coordinator l5-stage2-after-f5
$(L5_STAGE2_NORMAL): $(L5_STAGE2_PROBE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(L5_STAGE2_PROBE) -o $@
$(L5_STAGE2_SAN): $(L5_STAGE2_PROBE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(L5_STAGE2_PROBE) -o $@
l5-stage2-baseline: $(L5_STAGE2_NORMAL)
	./$(L5_STAGE2_NORMAL) --l5-baseline
l5-stage2-baseline-sanitize: $(L5_STAGE2_SAN)
	ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./$(L5_STAGE2_SAN) --l5-baseline
l5-stage2-coordinator: $(L5_STAGE2_NORMAL)
	./$(L5_STAGE2_NORMAL) --coordinator
l5-stage2-after-f5: $(L5_STAGE2_NORMAL)
	./$(L5_STAGE2_NORMAL) --after-f5
