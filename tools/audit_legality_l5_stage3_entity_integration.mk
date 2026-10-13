# Coordinator exact-head L5 Stage 3: actual native PK3/PK4 entity class
# quarantine, checksum validation, malformed inputs, and source immutability.
# Must be included AFTER Makefile.host. Does not infer original retail history.
L5_STAGE3_ENTITY_SRC := tests/fixtures/legality_l5_stage3_native_entity_boundary.cpp \
 $(filter-out tests/test_gen34_ball_domain_analyzer.cpp,$(GEN34_BALL_ANALYZER_SOURCES))
L5_STAGE3_ENTITY_EXEC := $(HOST_BUILD)/legality_l5_stage3_native_entities
L5_STAGE3_ENTITY_SAN := $(HOST_BUILD)/legality_l5_stage3_native_entities_san
.PHONY: l5-stage3-native-entities l5-stage3-native-entities-sanitize
$(L5_STAGE3_ENTITY_EXEC): $(L5_STAGE3_ENTITY_SRC)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(L5_STAGE3_ENTITY_SRC) -o $@
$(L5_STAGE3_ENTITY_SAN): $(L5_STAGE3_ENTITY_SRC)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(L5_STAGE3_ENTITY_SRC) -o $@
l5-stage3-native-entities: $(L5_STAGE3_ENTITY_EXEC)
	./$(L5_STAGE3_ENTITY_EXEC)
l5-stage3-native-entities-sanitize: $(L5_STAGE3_ENTITY_SAN)
	ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./$(L5_STAGE3_ENTITY_SAN)
