# Coordinator L3-to-production native encrypted Platinum PK4 source evidence.
# Use with the complete coordinator Makefile.host include graph (not isolated mocks).
L5_PT_NATIVE_SOURCES := tests/fixtures/legality_l5_platinum_native_analyzer_probe.cpp \
 $(filter-out tests/test_gen34_ball_domain_analyzer.cpp,$(GEN34_BALL_ANALYZER_SOURCES))
L5_PT_NATIVE_EXEC := $(HOST_BUILD)/legality_l5_platinum_native
L5_PT_NATIVE_SAN := $(HOST_BUILD)/legality_l5_platinum_native_sanitize
.PHONY: l5-platinum-native l5-platinum-native-sanitize
$(L5_PT_NATIVE_EXEC): $(L5_PT_NATIVE_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(L5_PT_NATIVE_SOURCES) -o $@
$(L5_PT_NATIVE_SAN): $(L5_PT_NATIVE_SOURCES)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(L5_PT_NATIVE_SOURCES) -o $@
l5-platinum-native: $(L5_PT_NATIVE_EXEC)
	./$(L5_PT_NATIVE_EXEC)
l5-platinum-native-sanitize: $(L5_PT_NATIVE_SAN)
	ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ./$(L5_PT_NATIVE_SAN)
