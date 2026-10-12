# L2 — 11/11 Standalone Suite Validation and Coordinator Integration Handoff

**Date:** 2026-10-12 · **Owner:** L2 breeding / egg PID-IV / historical-parent lane [issue #287](https://github.com/GlitchedZeus/PokeBank-NX/issues/287) · **Draft:** [PR #295](https://github.com/GlitchedZeus/PokeBank-NX/pull/295) · **Master:** [#285](https://github.com/GlitchedZeus/PokeBank-NX/issues/285).

## Disposition

**Stabilization complete for L2-owned conditional source-stage helpers, ready for coordinator review. NOT merge-approved or production-integrated.** No new breeding feature scope is being started. Every one of the **11** new lane-prefixed C++ suites has now been independently compiled and executed. **44/44 GCC/Clang normal and ASan+UBSan executions passed during this validation round** (11 suites × two compilers × two modes). These are standalone tests in isolated source-verified test workspaces; **not a clean whole-repository checkout**, no full host/RSE GitHub Actions pass, no authentic retail original-parent-and-egg archive.

Source reference frozen under `docs/legality/master/REFERENCE_FREEZE.md`; pinned original GBA daycare C, DP NDS ARM assembly, Platinum/HGSS daycare and RNG. Prior [nine-suite source/hash audit](L2_EXACT_SOURCE_9_OF_11_VALIDATION_20261012.md) is retained verbatim as historical evidence, superseded for current validation status by this file.

## Actual executed targets — evidence and limits

| L2 new test (all GCC+Clang, normal+ASan/UBSan PASS) | Git blob SHA-1 | Result |
|---|---|---|
| `test_legality_l2_gen3_egg_pid_creation.cpp` | `0ebda9c01b3b5978e2b7c541a3c9f334522e2125` | B07 20,508 synthetic Gen III PID/Everstone histories |
| `test_legality_l2_gen3_joint_egg_pid_iv.cpp` | `d9d577f240e885fcca9710471e1e744150b81e61` | B07 5,120 PID+IV stage histories + 50 provenance guards |
| `test_legality_l2_gen3_iv_attested_alternatives.cpp` | `baf39518b47c5f2b9a3f11284eb427a6646ff4ef` | B04/B05/B06 20,480 inherited-IV stage seeds |
| `test_legality_l2_gen3_joint_pid_iv_moves.cpp` | `0d4f6184ea2a17c46d26f763fd30e1a19d63cc44` | B08 2,560 original same-parent move/PID/IV + 76 controls |
| `test_legality_l2_gen3_pid_iv_moves_alternatives.cpp` | `ea65904d4e1a8080ccfc04b265d5bef06ef6d41b` | B18 2,560 alternative histories + 65 controls |
| `test_legality_l2_dp_egg_iv.cpp` | `a8aa0490adcdae8c16c4364866563835f91bb611` | B12 DP 8,202 stage/Unknown checks |
| `test_legality_l2_dp_egg_pid.cpp` | `1340c3cf23d81133202c97c1ecc5fbb11dd25204` | B13 49 MT/Everstone/international controls |
| `test_legality_l2_dp_alternative_egg_histories.cpp` | `0803574041aa4c2cf6c42c7538a0bf554f2a502a` | B18 DP 8,232 alternative route scenarios |
| `test_legality_l2_gen4_iv_candidate_set.cpp` | `348ee7d9281c4c8a28147b78ea28e2ee999a2888` | B16 HGSS/Pt 12,717 Power-item IV hypotheses |
| `test_legality_l2_dp_native_egg_history.cpp` | `4fcb8953a97de43dc803caacdd6c0cc7e94ef5d6` | **Native encrypted DP PK4 parent+egg; PASS**, checksum, corrupted records, IV/PID evidence and missing histories |
| `test_legality_l2_gen4_native_iv_history.cpp` | `de5254a7874325871599628d49e66277c9d8363b` | **Native encrypted HGSS PK4 parent+egg; PASS**, held-item history independence, negative/Unknown and source immutability |

**Native test execution this round:** The **exact byte-for-byte committed two native test CPPs and all six immediately consumed native L2/accepted legality headers** had their `git hash-object` values verified against the GitHub ref. Likewise, production `src/Pokemon/Pokemon4ReadOnly.cpp` `bc82e07fa737bf5a3fa15533bd77083b0300436a`, `include/Pokemon/Pokemon4ReadOnly.h` `436bafc06d5543d6c5d2b54f3b1620653bc145ad`, `src/Encryption/Encryption4.cpp` `406935761713db95ae738398b65ba0a37d4cf97b`, `include/Encryption/Encryption4.h` `e823d3fb2fdd2008047d79e15faea1c2a12f48a0`, and `PersonalRecord.h` `1ab506fe4a69b293945920a663ff5f1bb44a544d` were exact Git blobs. Tested against **real, unchanged production parser and encryption implementation**, not a fake PK4 parser. GCC+Clang C++20 `-Wall -Wextra -Wpedantic -Werror`, normal `-O2` and `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer` with `ASAN_OPTIONS=halt_on_error=1:detect_leaks=1`, `UBSAN_OPTIONS=halt_on_error=1` — **8/8 native executions PASS**.

**Native workspace limitation:** The isolated environment could not fetch the complete Git checkout through DNS. For unrelated, **uncalled** personal-data/text helpers and the GameVersion declarations, the temporary native build used reduced header declarations plus the original PK4 shuffle-position values and linker section garbage collection (`-ffunction-sections -fdata-sections -Wl,--gc-sections`). Those **temporary headers are NOT committed and NOT production substitutes**. Therefore this validates the exact committed L2 and real PK4/crypto implementation paths that were exercised; it does **not** prove the full production translation-unit linker graph or a complete clean checkout. These remain mandatory coordinator CI gates.

**Nine other standalone tests this session:** All **36** GCC/Clang normal/sanitizer executions pass again. Their CPP and legality headers are byte-for-byte Git-blob matched. As documented previously, the isolated Gen III move-table header was regenerated with **all 973 original Gen III egg-move membership bits and 184 nonzero Gen III pre-evolution links** from accepted `Gen34EggMoveData.inc` blob `2385e4bc189f2b784e064ac0c2977cb4c9e8325a`, not the byte-identical generated `.inc` file. No suite queried reconstructed/placeholder Gen IV egg-move tables. The exact accepted full data-file build remains a coordinator gate.

**No failing new L2 standalone tests observed.** No assertion failures, no ASan/UBSan errors in 44 executed binary runs. Ordinary GitHub PR Host Tests runs remain **queued/not verified green** and did not register these new test targets.

## Exact CI registration missing — coordinator-owned, NOT edited

The current ref was independently checked against `Makefile.host`, `Makefile.host.base`, `Makefile.host.gen4`, `.github/workflows/host-tests.yml`: **NONE registers `test_legality_l2_*.cpp`**, although the 11 files exist. The workflow checks out exact legality PR heads and executes `make -f Makefile.host host-test`, focused RSE bridge, and `host-sanitize`; passing it as written does NOT run these suites.

Coordinator checklist:
1. **Before integration:** Re-fetch **all** accepted/PR heads and run a forward-only conflict-aware reconciliation of this lane into the newest accepted legality source; preserve Platinum accepted #283 and separately advancing HGSS PID #284. No reset, backward rebase, force push, self-merge, MAIN or umbrella #103.
2. Register all **eleven** test executable targets and sanitized variants in **coordinator-owned host build** (dedicated Makefile.host.legality-l2 if desired, then explicitly include it in Makefile.host) and include them in `HOST_TESTS` / `HOST_SANITIZE_TESTS`. Use `-std=c++20` due to `std::span`, strict `-Werror` and real accepted headers.
3. Native PK4 test targets must link to the complete accepted `Makefile.host.gen4` **`GEN4_MUTABLE_SOURCES`** production dependencies (`Pokemon4Mutable`, `Pokemon4ReadOnly`, `PersonalInfo4DP/PT/HGSS`, `Encryption4`, names/utility source); **do not use temporary header reductions or garbage-collected unreferenced-method shortcuts** in final CI. The lane-owned `tests/test_legality_l2_run_all.sh` lists the source files and offers both compiler/sanitizer paths.
4. From a **clean exact candidate head** with byte-identical generated egg-move source, run `bash tests/test_legality_l2_run_all.sh` and `CXX=clang++ bash tests/test_legality_l2_run_all.sh`. Require **22/22 executions per compiler** (11 normal + 11 sanitizer), i.e. 44 total, **on that combined head**; archive logs and SHA. Then full `make -f Makefile.host host-test`, focused RSE save-open bridge, full `host-sanitize` and all required device-build gates.
5. **Production integration requires a separate coordinator-owned decision.** Helpers return **conditional history-match, candidate-local contradiction, or Unknown/Incomplete**. Attach via explicitly sourced original-game/provider/egg identity, preserving read-only source saves and immutable stage evidence. Never translate bounded candidate exhaustion, absent original daycare parents/items/RNG, unsupported origins or unverifiable completeness into whole-Pokémon `Invalid` or a definitive `Legal` verdict.

## Source and provenance blockers that tests do not close

- **Independent original retail proof:** L5 must independently compare DP `pret/pokediamond` overlay `ov05_021EC9D4/ov05_021EC944/ov05_Daycare_CreateEgg`, Ruby/Sapphire vs Emerald vs FRLG daycare PID/IV/egg-move routines, Platinum and HGSS RNG/Power-item paths against the correct retail region/binary; document frozen source hashes, licensing boundaries and confirmed behavioral equivalence.
- **Authentic original-stage witness archive:** No documented immutable original daycare parent pair, original pre-inheritance egg IVs, correct original creation/collection RNG transcript (Emerald VBlank, DP/Pt MT, HGSS LCRNG), creation-time held items, languages, original unhatched child and complete egg move chain has yet been independently attested for these new cases. The synthetic encrypted PK4 vectors validate parsing/control flow **only**. Historical provenance cannot be manufactured from a detached child PK3/PK4.
- **Partial catalog:** B04-B08/B11-B13/B16/B18 received source-specific *conditional* engineering, not exhaustive origin coverage; Gen II B01-B03, incense/special egg B09, versioned B10, gift/event/traded B17 and the cross-generational complete breeding-chain B18 remain partial or source-blocked. B14 and B15 have different accepted/sibling ownership (#283/#284). All unsupported, incomplete or non-exhaustively enumerated histories must report **Incomplete/Unknown**, not a global negative.

**Decision:** L2 implementation/testing package **ready for coordinator to review and forward integrate only after clean combined-head registration/CI and provenance policy review**. L2 development lane holds off on new feature expansion.
