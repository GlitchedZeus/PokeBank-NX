# L5 Seven-Stage Program — Stage 2 Gen I/II Native QA

Date: 2026-10-12. Existing draft PR #294. No production, coordinator,
accepted legality, MAIN, or sibling L1–L4 branch changes.

## Executable C++ and Python implementation

- tests/fixtures/legality_l5_stage2_gen12_native_history_fuzz.cpp:
  65,536 distinct species IDs, 131,072 deterministic source/history
  combinations, exact pinned original production
  Gen12TimeCapsuleEvidence.h. Tests Gen I ancestry (including 11 later
  evolutions), egg/caught-data exclusions, current trade vs historical
  compatibility, Gen I move #165/166, Gen II #251/252, missing
  provenance, malformed moves and input immutability.
- tools/audit_legality_l5_stage2.py: checks exact Git blobs, compiles
  GCC and Clang with normal and ASan/UBSan, verifies strict output,
  includes five negative attestation controls; optionally invokes
  full actual analyzer. Header-only success never marks native analyzer
  as executed.
- tests/fixtures/legality_l5_stage2_gen12_native_analyzer_matrix.cpp:
  1,530 actual Legality::analyze synthetic PK1/PK2 contexts, including
  600 nonzero native stat-experience unhatched egg instances, all
  five stored Gen II stat-experience fields, zero slots, duplicate
  moves, format move ceilings, source label conflicts, source-free
  ability/gender, evolving Gen II species and immutable raw records.
  Actual full-production execution remains PENDING.
- tools/audit_legality_l5_stage2_native.mk: isolated build targets
  matching three source revisions: L5 baseline, intermediate F3/F4
  only and latest F3/F4/F5 coordinator. Shared Makefiles unchanged.
- .github/workflows/l5-independent-native-stage2.yml:
  new L5-specific exact-checkout Actions workflow. It demands both
  standalone and actual full analyzer normal+sanitizer passes.
  Coordinator/shared CI workflow untouched.

## Tests actually executed

Exact original source Git blob
4be5eda0dac2f43e886024d2febc07d9639abc18, and
tested fixture Git blob bf110896cbea4c0632f41ac11bd2247064c77e74.

| Test executable | Cases | Result |
|---|---:|---|
| GCC 14.2 C++20 O2 Werror | 1,507,328 | PASS |
| GCC 14.2 O1 ASan+UBSan | 1,507,328 | PASS |
| Clang 17 C++20 O2 Werror | 1,507,328 | PASS |
| Clang 17 O1 ASan+UBSan | 1,507,328 | PASS |
| Python output attestation negative controls | 5 | PASS |
| GCC -DNDEBUG assertions-disabled negative build | 1 | correctly FAILED to compile |

Total 6,029,312 actually executed helper assertions.
The suite observed 162 historically Gen I-compatible species IDs,
3,088 cases with conditional later-generation moves and 15,499
malformed-move input cases. The initial deterministic generator had
correlated branch inputs, making one positive control unreachable.
That QA-fixture bug was corrected before these successful runs;
it was not a production defect.

## Native analyzer / F3-F5 evidence boundary

The L5 accepted analyzer still reflects the historical F3/F4
source-free false-Invalid conditions and lacks F5. A separately
re-fetched coordinator commit
4258e341aed431b1ac4367e78ccec02e73e0d32c
contains explicit PK1/PK2 ability/gender capability guards and an
F5 check around Legality.cpp:620–629 for nonzero stat experience in
unhatched native PK2 eggs. This is source verification, not a claimed
native analyzer test pass.

Native analyzer execution was ATTEMPTED but BLOCKED:
the isolated local test root lacks Makefile.host, and cloning GitHub
fails DNS resolution. The deliberate native command exited nonzero
with "full source checkout missing: Makefile.host absent".
The independent exact-head Actions run was QUEUED when checked.
No 1,530-case analyzer PASS or complete normal/sanitizer CI PASS
is claimed.

The new source-only helper checks never elevate unverified acquisition
histories to global Invalid, and no original retail specimen or PKHeX
runtime evidence has been fabricated. All 336 full-parity cells stay
uncertified.

## Reproduce and coordinator handoff

On L5 branch, run:
  python3 tools/audit_legality_l5_stage2.py --self-test
  python3 tools/audit_legality_l5_stage2.py --native-analyzer

On a separate reviewed coordinator checkout, rebuild the 1,530-case
fixture against the full production analyzer and execute its
--after-f5 mode under optimized and ASan/UBSan builds. Record
exact SHA and tree for any successful evidence. Do not edit
the coordinator branch from L5; do not merge MAIN.
