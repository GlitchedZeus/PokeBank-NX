# L5 Stage 3 full-checkout-only PK3/PK4 semantic quarantine.
# Include AFTER Makefile.host; do not modify shared coordinator Makefiles.
# Tests the actual Pokemon3FRLG and immutable Pokemon4ReadOnly classes.
L5_STAGE3_ENTITY_SOURCES := tests/fixtures/legality_l5_stage3_native_entity_boundary.cpp \
 $(filter-out tests/test_gen34_ball_domain_analyzer.cpp,$(GEN34_BALL_ANALYZER_SOURCES))
L5_STAGE3_ENTITY_NORMAL := $(HOST_BUILD)/legality_l5_stage3_entity
L5_STAGE3_ENTITY_SAN := $(HOST_BUILD)/legality_l5_stage3_entity_san
.PHONY: l5-stage3-entity l5-stage3-entity-sanitize
$(L5_STAGE3_ENTITY_NORMAL): $(L5_STAGE3_ENTITY_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(L5_STAGE3_ENTITY_SOURCES) -o $@
$(L5_STAGE3_ENTITY_SAN): $(L5_STAGE3_ENTITY_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(L5_STAGE3_ENTITY_SOURCES) -o $@
l5-stage3-entity: $(L5_STAGE3_ENTITY_NORMAL)
	./$(L5_STAGE3_ENTITY_NORMAL)
l5-stage3-entity-sanitize: $(L5_STAGE3_ENTITY_SAN)
	ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./$(L5_STAGE3_ENTITY_SAN)
