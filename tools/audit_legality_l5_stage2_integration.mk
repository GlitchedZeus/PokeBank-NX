# Coordinator-owned integration adapter for latest L5 Stage 2 native matrix.
# Include AFTER Makefile.host and tools/audit_legality_l5_stage2_native.mk.
# The L5 lane's baseline target intentionally tests its pre-F3/F4/F5 source;
# the coordinator at PR #298 MUST execute --after-f5 instead.
.PHONY: l5-stage2-after-f5-sanitize
l5-stage2-after-f5-sanitize: $(L5_STAGE2_SAN)
	ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 ./$(L5_STAGE2_SAN) --after-f5
