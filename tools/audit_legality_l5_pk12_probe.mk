# L5 independent coordinator-run PK1/PK2 source-free Invalid regression.
# Include AFTER coordinator-owned Makefile.host without editing its contents.
# make -f Makefile.host -f tools/audit_legality_l5_pk12_probe.mk l5-pk12-baseline
# make -f Makefile.host -f tools/audit_legality_l5_pk12_probe.mk l5-pk12-baseline-sanitize
# After coordinator fixes production native-generation predicates:
# make -f Makefile.host -f tools/audit_legality_l5_pk12_probe.mk l5-pk12-fixed
L5_PK12_NATIVE := tests/fixtures/legality_l5_pk12_sourcefree_native_probe.cpp \
 $(filter-out tests/test_gen34_ball_domain_analyzer.cpp,$(GEN34_BALL_ANALYZER_SOURCES))
L5_PK12_EXE := $(HOST_BUILD)/legality_l5_pk12_sourcefree
L5_PK12_SAN := $(HOST_BUILD)/legality_l5_pk12_sourcefree_sanitize
.PHONY: l5-pk12-baseline l5-pk12-baseline-sanitize l5-pk12-fixed l5-pk12-fixed-sanitize
$(L5_PK12_EXE): $(L5_PK12_NATIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(L5_PK12_NATIVE) -o $@
$(L5_PK12_SAN): $(L5_PK12_NATIVE)
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(L5_PK12_NATIVE) -o $@
l5-pk12-baseline: $(L5_PK12_EXE)
	./$(L5_PK12_EXE) --baseline
l5-pk12-baseline-sanitize: $(L5_PK12_SAN)
	ASAN_OPTIONS=detect_leaks=0 ./$(L5_PK12_SAN) --baseline
l5-pk12-fixed: $(L5_PK12_EXE)
	./$(L5_PK12_EXE) --after-fix
l5-pk12-fixed-sanitize: $(L5_PK12_SAN)
	ASAN_OPTIONS=detect_leaks=0 ./$(L5_PK12_SAN) --after-fix
