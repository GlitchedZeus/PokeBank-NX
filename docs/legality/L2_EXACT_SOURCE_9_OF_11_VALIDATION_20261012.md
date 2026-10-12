# L2 independent exact-source host validation — 9 of 11 test suites

**Date:** 2026-10-12. **Status:** 9 of 11 L2 tests independently compiled and executed successfully under two compilers and normal/sanitizer modes. This is a real standalone test result, **not** a complete repository host CI, not a retail-ROM differential, not a native PK4 validation, and not acceptance of the global legality engine.

**Repository:** `GlitchedZeus/PokeBank-NX`. Dedicated draft [PR #295](https://github.com/GlitchedZeus/PokeBank-NX/pull/295), [lane issue #287](https://github.com/GlitchedZeus/PokeBank-NX/issues/287), [master #285](https://github.com/GlitchedZeus/PokeBank-NX/issues/285). Original branch last re-fetched at `72f2d891fd8408170f408ee1b39e886fb94fbdf5`, accepted legality branch `d29eada5c087e201e00c749703f1a2de3ddfadeb`.

## Nine actual successful test suites

Every **individual** suite in this table was compiled and executed in **four separate configurations**: GCC 14 normal `-O2`, GCC 14 ASan+UBSan `-O1 -g`, Clang normal `-O2`, Clang ASan+UBSan `-O1 -g`. All used `-std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude`. Sanitizer runs used `-fsanitize=address,undefined -fno-omit-frame-pointer`, `ASAN_OPTIONS=halt_on_error=1:detect_leaks=1`, `UBSAN_OPTIONS=halt_on_error=1`. All **36 executions exited successfully**, with no compiler, assertion or sanitizer failures.

| Exact committed test | Git blob ID | Case counter printed per execution |
|---|---|---:|
| `test_legality_l2_gen3_egg_pid_creation.cpp` | `0ebda9c01b3b5978e2b7c541a3c9f334522e2125` | 20,508 |
| `test_legality_l2_gen3_joint_egg_pid_iv.cpp` | `d9d577f240e885fcca9710471e1e744150b81e61` | 5,120 histories + 50 controls |
| `test_legality_l2_gen3_iv_attested_alternatives.cpp` | `baf39518b47c5f2b9a3f11284eb427a6646ff4ef` | 20,480 |
| `test_legality_l2_gen3_joint_pid_iv_moves.cpp` | `0d4f6184ea2a17c46d26f763fd30e1a19d63cc44` | 2,560 histories + 76 controls |
| `test_legality_l2_gen3_pid_iv_moves_alternatives.cpp` | `ea65904d4e1a8080ccfc04b265d5bef06ef6d41b` | 2,560 histories + 65 controls |
| `test_legality_l2_dp_egg_iv.cpp` | `a8aa0490adcdae8c16c4364866563835f91bb611` | 8,202 |
| `test_legality_l2_dp_egg_pid.cpp` | `1340c3cf23d81133202c97c1ecc5fbb11dd25204` | 49 |
| `test_legality_l2_dp_alternative_egg_histories.cpp` | `0803574041aa4c2cf6c42c7538a0bf554f2a502a` | 8,232 |
| `test_legality_l2_gen4_iv_candidate_set.cpp` | `348ee7d9281c4c8a28147b78ea28e2ee999a2888` | 12,717 |

**Printed case counters sum to 80,428**, excluding separately printed control counts and unprinted per-seed assertions. That total is **not** 80,428 authentic independent retail eggs.

## Exact-source confidence and an important data caveat

All nine L2 test `.cpp` files were copied into the local test workspace, then matched byte-for-byte to their *GitHub Git blob SHA-1 IDs*, using `git hash-object`. All used source headers from the accepted and L2 branches were likewise verified by their Git blob IDs:

- Accepted: `Gen3EggIVInheritanceStage.h` `23581c067cb831187824beb5fd4beb1ade3b91ae`; `Gen34EggMoveEvidence.h` `300c4fe5d1ca4b5f8d3d66b4c72dd76e21fc90f0`; `Gen34OrdinaryEggMoveConstruction.h` `17cd8587c7455c85745a1410a3a1675b0a4b450d`; `Gen34SameParentEggHistory.h` `8df955eda060e74d12aae7d9a8385f4a5581964a`; `Gen34BreedingPairEligibility.h` `ad1da95911841c91d082db67f49b204d37bdca70`; `Gen3JointEggCandidateEvidence.h` `58ad6ab10551f5816e03316fcdcd46a19b155cf5`; `Gen4EggIVInheritanceStage.h` `11e72b655d6963b5f31f07017122e28a5a7a4996`.
- L2: `Gen3EggPIDCreationHistory.h` `69a147b006630fb599702527d85a7b47c83bd783`; `Gen3JointEggPIDIVHistory.h` `b500f2d9bbeeeb04bd48391a3374ed264a568a7e`; `Gen3EggIVAttestedAlternatives.h` `9f62980381dfcdc95f0ce4ed1f16ecccc5a220d8`; `Gen3JointEggPIDIVMoveHistory.h` `b129414e71bff37fb190fb0097c3d5fa48947043`; `Gen3EggFullHistoryAlternatives.h` `5f6635f715db870994260cec3da19de1d85f763e`; `Gen4DiamondPearlEggIVStage.h` `6b1d2a7bc2299fe7780deba849caf033fa2ff0f5`; `Gen4DiamondPearlEggPidHistory.h` `fe9ada59136bb069440733b14d93ec0f27046cb0`; `Gen4DiamondPearlEggAlternativeHistories.h` `37bd50412b70fdb280809689f518ff4e29307149`; `Gen4EggIVHistoricalCandidateSet.h` `77b87bb9d1cd322fde09998c6b6d47240d9e399e`.

**Accepted generated egg-move data distinction:** The model's container could not clone GitHub through DNS. For the **five Gen III games tested**, the accepted `Gen34EggMoveData.inc` Git blob `2385e4bc189f2b784e064ac0c2977cb4c9e8325a` was inspected via connected GitHub, and **the full 973 Gen III egg-move set bits and all 184 nonzero Gen III pre-evolution links** were encoded and checked by length/checksum, then regenerated locally into the same `kEggMovesGen3` and `kPreEvolutionGen3` C++ array interfaces. The Bulbasaur row was explicitly compared to the pinned source. The local data-file *representation* is **not byte-identical** to the accepted generated file. Other Gen IV move-table arrays were deliberately zero placeholders in this isolated workspace, and **none of these nine tests query Gen IV egg-move data**. This achieves full semantically identical Gen III move values for the tests, not proof of building the complete accepted repository. The coordinator must still compile against the unmodified generated source.

All stage-fixture cases use **synthetic independently asserted historical inputs**, not original authenticated parent+egg archival saves. Checks of a purported history are candidate-local. Incomplete history remains Unknown, never a blanket Pokémon Invalid.

## Remaining two actual unexecuted native tests

1. `tests/test_legality_l2_dp_native_egg_history.cpp`: checksum-valid encrypted original-format DP parent/egg records, original PID/IV binding.
2. `tests/test_legality_l2_gen4_native_iv_history.cpp`: HGSS original-format parent/egg encrypted PK4, historical Power-item IV hypotheses.

Both require actual `Pokemon4ReadOnly.cpp`, `Pokemon4Mutable.cpp`, `Encryption4.cpp`, Gen IV personal-info tables and the real name/utility linkage. This partial local test workspace did not have those production sources, so neither native suite was compiled or executed, and **9/11 is the correct count**. The committed `tests/test_legality_l2_run_all.sh` can run all eleven against a full checkout with those dependencies.

## Coordinator gates — still blocking merge

- Verify current live accepted/sibling heads again and forward-reconcile without discarding newer work, reset/rebase backward or force-push.
- From a **complete clean exact-head checkout**, run GCC and Clang `tests/test_legality_l2_run_all.sh` using the original unmodified `Gen34EggMoveData.inc` and production PK4/Encryption sources.
- Register all eleven `tests/test_legality_l2_*.cpp` in coordinator-owned host Makefiles/workflows; run full host, focused RSE and ASan/UBSan CI. GitHub Host Tests remained **queued** on the PR at last check and do **not** constitute verified acceptance.
- Obtain L5 independent original retail function/binary checks and authentic original-stage parent/egg provenance when such evidence exists.
- Preserve sibling HGSS PID #284 and accepted Platinum #283. No production `Legality.cpp` integration, no mainline/umbrella #103 merge, no original save writes or autofixes, no Gen V/Master Vault.
