# L5 Stage 3: coordinator-only native Gen II egg stat-experience differential.
# Do not register in shared CI until the coordinator reviews F5.
# Include AFTER Makefile.host. Full production Legality::analyze is linked.
L5_PK2_EGG_SOURCES := tests/fixtures/legality_l5_pk2_egg_stat_exp_analyzer_probe.cpp \
 $(filter-out tests/test_gen34_ball_domain_analyzer.cpp,$(GEN34_BALL_ANALYZER_SOURCES))
L5_PK2_EGG_NORMAL := $(HOST_BUILD)/legality_l5_pk2_egg_reference
L5_PK2_EGG_SAN := $(HOST_BUILD)/legality_l5_pk2_egg_reference_sanitize
.PHONY: l5-pk2-egg-baseline l5-pk2-egg-baseline-sanitize l5-pk2-egg-fixed l5-pk2-egg-fixed-sanitize
$(L5_PK2_EGG_NORMAL): $(L5_PK2_EGG_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(L5_PK2_EGG_SOURCES) -o $@
$(L5_PK2_EGG_SAN): $(L5_PK2_EGG_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(L5_PK2_EGG_SOURCES) -o $@
l5-pk2-egg-baseline: $(L5_PK2_EGG_NORMAL)
	./$(L5_PK2_EGG_NORMAL) --baseline
l5-pk2-egg-baseline-sanitize: $(L5_PK2_EGG_SAN)
	ASAN_OPTIONS=detect_leaks=0 ./$(L5_PK2_EGG_SAN) --baseline
l5-pk2-egg-fixed: $(L5_PK2_EGG_NORMAL)
	./$(L5_PK2_EGG_NORMAL) --after-fix
l5-pk2-egg-fixed-sanitize: $(L5_PK2_EGG_SAN)
	ASAN_OPTIONS=detect_leaks=0 ./$(L5_PK2_EGG_SAN) --after-fix
