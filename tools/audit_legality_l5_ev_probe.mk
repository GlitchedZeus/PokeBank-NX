# L5 standalone native PK3/PK4 analyzer EV differential probe.
# Add alongside, not inside, the coordinator-owned Makefile.host graph:
#   make -f Makefile.host -f tools/audit_legality_l5_ev_probe.mk l5-ev-baseline
#   make -f Makefile.host -f tools/audit_legality_l5_ev_probe.mk l5-ev-baseline-sanitize
# Coordinator AFTER production fix:
#   make -f Makefile.host -f tools/audit_legality_l5_ev_probe.mk l5-ev-fixed
#   make -f Makefile.host -f tools/audit_legality_l5_ev_probe.mk l5-ev-fixed-sanitize
#
# Source does NOT modify save data. Synthetic PK3+encrypted PK4.
# Uses the accepted analyzer linker graph; no vendor edits or shared CI edit.
L5_EV_PROBE_SRC := tests/fixtures/legality_l5_ev_native_analyzer_probe.cpp \
    $(filter-out tests/test_gen34_ball_domain_analyzer.cpp,$(GEN34_BALL_ANALYZER_SOURCES))
L5_EV_BIN := $(HOST_BUILD)/audit_legality_l5_ev_native
L5_EV_BIN_SAN := $(HOST_BUILD)/audit_legality_l5_ev_native_sanitize
.PHONY: l5-ev-baseline l5-ev-baseline-sanitize l5-ev-fixed l5-ev-fixed-sanitize
$(L5_EV_BIN): $(L5_EV_PROBE_SRC) include/Legality/Legality.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) -Wno-unused-parameter -Iinclude $(L5_EV_PROBE_SRC) -o $@
$(L5_EV_BIN_SAN): $(L5_EV_PROBE_SRC) include/Legality/Legality.h
	@mkdir -p $(HOST_BUILD)
	$(CXX) $(CXXFLAGS) $(SANITIZE_FLAGS) -Wno-unused-parameter -Iinclude $(L5_EV_PROBE_SRC) -o $@
l5-ev-baseline: $(L5_EV_BIN)
	./$(L5_EV_BIN) --baseline
l5-ev-baseline-sanitize: $(L5_EV_BIN_SAN)
	ASAN_OPTIONS=detect_leaks=0 ./$(L5_EV_BIN_SAN) --baseline
l5-ev-fixed: $(L5_EV_BIN)
	./$(L5_EV_BIN) --after-fix
l5-ev-fixed-sanitize: $(L5_EV_BIN_SAN)
	ASAN_OPTIONS=detect_leaks=0 ./$(L5_EV_BIN_SAN) --after-fix
