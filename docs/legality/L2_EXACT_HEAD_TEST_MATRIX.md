# L2 exact-head test acceptance matrix — Gen I–IV legality

> **POST-FREEZE STATUS (2026-10-12):** The frozen L2 SHA `cdc128a4...` has all 11 programs **PASS** in coordinator integration PR #298 at `c91b53e...` [Actions #38287166355](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38287166355), normal and ASan/UBSan, with complete source checkout. The current L2 branch has **four revised alternative-history helpers and six updated C++ suites** after that frozen head. Those six changed suites have **NOT** yet been compiled/run on a fresh merged exact head. An ordinary L2 branch Host Tests result cannot establish the special lane-matrix gate. See [post-freeze audit](L2_POST_FREEZE_BREEDING_INTEGRATION_AUDIT_20261012.md). The historical entries below preserve the earlier standalone/exact-blob audit and must not be read as a current-head CI certification.\n\n
**As of 2026-10-12; draft #295; own L2 branch only. This is a test runner and status ledger, not a claim that the full legality engine has been accepted.**

## Why this matters

Draft [PR #295](https://github.com/GlitchedZeus/PokeBank-NX/pull/295) added **11 distinct** `tests/test_legality_l2_*.cpp` files. Coordinator-owned `Makefile.host*` and `.github/workflows/host-tests.yml` were intentionally untouched, so queued/green ordinary Host Tests runs **do not establish that these 11 binaries were built**. This document separates exact executed tests from unexecuted test source and makes a repeatable all-suite handoff for the coordinator.

### Lane-owned reproducible runner

```bash
# From a complete exact-head repo checkout, with accepted original
# Gen34EggMoveData.inc and native Pokemon4 + Encryption4 source:
bash tests/test_legality_l2_run_all.sh

# Run Clang independently
CXX=clang++ bash tests/test_legality_l2_run_all.sh

# Limit to selected test name, or normal pass only:
L2_TEST_FILTER=gen3_joint_pid_iv_moves bash tests/test_legality_l2_run_all.sh
L2_MODES=normal bash tests/test_legality_l2_run_all.sh
```

The runner is designed to compile all 11 tests separately with C++20 `-Wall -Wextra -Wpedantic -Werror`; for native encrypted PK4 fixtures, it links against the **actual** `src/Pokemon/Pokemon4Mutable.cpp`, `Pokemon4ReadOnly.cpp`, `src/Encryption/Encryption4.cpp`, original Gen IV personal-info tables, names and utilities specified by the accepted `GEN4_MUTABLE_SOURCES` host target (rather than stubbed parser/encryption code). It creates a unique temporary directory, removes binaries afterwards, warns on tracked dirty checkout and prints the SHA supplied by `git rev-parse HEAD`. `L2_MODES` defaults to normal **and** ASan/UBSan with leak detection and halt on error.

**Coordinator must still register targets inside shared host graph** and independently validate full host, focused RSE regression, sanitizers, strict native fixtures, provenance/Unknown and original retail evidence. A standalone runner does not replace those CI gates.

### Actual executed vs pending current tranche validation

| L2 test CPP | Test type | Status verified in local container |
|---|---|---|
| `test_legality_l2_gen3_egg_pid_creation.cpp` | B07 five-game retail PID/Everstone chronology | **PASS, exact Git blob** (GCC normal + GCC ASan/UBSan + Clang normal + Clang ASan/UBSan; **20,508** seeded/fixed controls per run) |
| `test_legality_l2_gen3_joint_egg_pid_iv.cpp` | B07 same original egg and parent pair PID+IV | **PASS, exact Git blob** (same four compiler/mode combinations; **5,120** seeded histories + **50** provenance/Unknown checks per run) |
| `test_legality_l2_gen3_iv_attested_alternatives.cpp` | B04/B05/B06/B11/B18 original physical IV alternatives | **PRE-FREEZE PASS in #298, but this CPP changed after freeze: NEW merged-head normal + ASan/UBSan REQUIRED** |
| `test_legality_l2_gen3_joint_pid_iv_moves.cpp` | B08 same parent PID+IV+four-slot move | **PASS, exact-source GCC/Clang normal + ASan/UBSan; 2,560 histories + 76 guards; full accepted Gen III egg-move VALUES with a locally regenerated table representation** |
| `test_legality_l2_gen3_pid_iv_moves_alternatives.cpp` | B18 triple-stage alternative parent universe | **PRE-FREEZE PASS in #298, but this CPP changed after freeze: NEW merged-head normal + ASan/UBSan REQUIRED** |
| `test_legality_l2_dp_egg_iv.cpp` | B12 original DP overlay IV stage | **PASS, exact-source GCC/Clang normal + ASan/UBSan; 8,202 seeded/fixed cases** |
| `test_legality_l2_dp_egg_pid.cpp` | B13 original DP pending PID, Everstone and Masuda | **PASS, exact-source GCC/Clang normal + ASan/UBSan; 49 source-assembly controls** |
| `test_legality_l2_dp_native_egg_history.cpp` | B12/B13 native encrypted original PK4 | **PRE-FREEZE PASS in #298, but this CPP changed after freeze: NEW merged-head normal + ASan/UBSan REQUIRED** |
| `test_legality_l2_dp_alternative_egg_histories.cpp` | B18 DP bounded original-stage PID+IV alternatives | **PRE-FREEZE PASS in #298, but this CPP changed after freeze: NEW merged-head normal + ASan/UBSan REQUIRED** |
| `test_legality_l2_gen4_iv_candidate_set.cpp` | B16 HGSS/Platinum Power-item historical IV hypotheses | **PRE-FREEZE PASS in #298, but this CPP changed after freeze: NEW merged-head normal + ASan/UBSan REQUIRED** |
| `test_legality_l2_gen4_native_iv_history.cpp` | B16 native encrypted HGSS original parents + egg | **PRE-FREEZE PASS in #298, but this CPP changed after freeze: NEW merged-head normal + ASan/UBSan REQUIRED** |

**Verifiable executed Git blob identity** (the source files on GitHub and local test files were compared with `git hash-object`):
- `include/Legality/Gen3EggPIDCreationHistory.h`: `69a147b006630fb599702527d85a7b47c83bd783`.
- `include/Legality/Gen3JointEggPIDIVHistory.h`: `b500f2d9bbeeeb04bd48391a3374ed264a568a7e`.
- accepted `include/Legality/Gen3EggIVInheritanceStage.h`: `23581c067cb831187824beb5fd4beb1ade3b91ae`.
- `tests/test_legality_l2_gen3_egg_pid_creation.cpp`: `0ebda9c01b3b5978e2b7c541a3c9f334522e2125`.
- `tests/test_legality_l2_gen3_joint_egg_pid_iv.cpp`: `d9d577f240e885fcca9710471e1e744150b81e61`.

Executed from the local staged exact-file subset using `g++` and `clang++` (C++20 `-Werror`, O2 normally, O1 -g plus `-fsanitize=address,undefined -fno-omit-frame-pointer` for sanitized); each binary printed PASS and returned success. **44 successful executions across eleven exact-blob test suites**. The container has no direct DNS access to clone GitHub; this did not yield a full checkout. Both native encrypted-PK4 suites are also executed; see the explicit temporary unrelated-header/linker limitations in the 11/11 handoff. The accepted Gen III egg-move dataset was reconstructed with the full original 973 move bits and 184 pre-evolution links, but the local data header bytes differ from the repository's generated 397,765-character file. All other used accepted headers match their Git blobs. For every source/test blob and the precise reconstruction/CI limitations, read [the completed 11/11 integration handoff](L2_11_OF_11_VALIDATION_INTEGRATION_HANDOFF_20261012.md) (with the previous [9/11 phase evidence](L2_EXACT_SOURCE_9_OF_11_VALIDATION_20261012.md) retained).

### Priorities to unblock acceptance

1. Coordinator forward-reconcile current accepted branch and all advancing sibling legality lanes. Re-fetch heads and never reset/rebase backward or force-push.
2. From a clean exact combined head, run GCC and Clang `tests/test_legality_l2_run_all.sh`, fix errors on L2-owned files only, then add all 11 targets to coordinator-owned build. Use the exact accepted generated egg-move table, not a hand-made stub.
3. Run accepted full host, focused RSE regression and ASan/UBSan in coordinator-owned CI, retain log+commit SHA evidence.
4. Independent L5 compare against original retail binaries and true originally witnessed parent/egg stages. Synthetic same-egg input and exhaustive declared daycare routes are **not** global Pokémon legality proof.
5. No edits to source saves, no auto-fixes, no MAIN or umbrella #103 integration, no Gen V/Master Vault, and no self-merge of draft #295.
