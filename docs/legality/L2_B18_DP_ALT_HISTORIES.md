# L2 B18 — bounded Diamond/Pearl breeding-history alternatives

**Date:** 2026-10-12. **Status:** new source-backed *conditional* engineering implementation with independently exercised standalone tests, NOT an accepted whole-Pokémon legality verdict, a retail-original stage reconstruction, or completed repository-wide CI. Dedicated issue [#287](https://github.com/GlitchedZeus/PokeBank-NX/issues/287), master [#285](https://github.com/GlitchedZeus/PokeBank-NX/issues/285), draft [#295](https://github.com/GlitchedZeus/PokeBank-NX/pull/295).

## What is new

- `include/Legality/Gen4DiamondPearlEggAlternativeHistories.h`: maximum **8** independently identified candidate original daycare PID/IV histories for **one** externally verified original unhatched egg; explicit Diamond/Pearl-only dispatch. Each candidate reuses the B12/B13 egg generation rules without modifying accepted implementation or the global legality analyzer.
- `tests/test_legality_l2_dp_alternative_egg_histories.cpp`: **4,096 synthetic original IV entry seeds per game**, Diamond and Pearl; alternatives, candidate-local negative cases, incomplete evidence and identity mismatch. **8,232 counted outer-loop and focused synthetic scenarios** (not independent retail eggs; multiple assertions per seed).
- The candidate requires a nonzero unique route identity, independent source attestation, original egg creation event identity, and separately sourced evidence binding the **same physical parent pair** to both PID and IV. Merely sharing an event token is not enough to combine one pair's inherited IVs with another pair's Everstone donor, languages or original PID.
- The source event token, exact original egg PID and original child IV bytes, candidate's IV/PID child projections, physical parent pair ID and PID creation/collection tokens must agree. A mismatch in those identities is `UnknownCandidateProvenance` — **not** an impossible legitimate Pokémon.
- A fully attested candidate can conditionally match B12 **and** B13. A known contradiction excludes only that specific hypothesis and only when `allMethodsForThisCandidateIndependentlyCovered` is separately attested. If that method-proof is not complete, preserve `UnresolvedCandidateHistory`.
- If at least one candidate matches, the set reports one *conditionally consistent history* even if another proposed candidate contradicts. Duplicate/zero route IDs, out-of-range capacity, unsupported Pt/HGSS, missing historical source, missing seed, language, parent pair or egg provenance always retain an Unknown/unresolved result.
- Even `AllAttestedDaycareStageHypothesesContradicted` describes **only** the externally supplied, alleged exhaustive daycare-stage hypothesis set (and only after independent enumeration proof). It does not inspect every other breeding pair, gift/event egg, trade, transfer, time-erased history, or whole-Pokémon legality.

## Frozen original source

The stage mechanics are sourced to:

- [pret/pokediamond@b623abef1b4dfe204ca50aab42447ac00c134290](https://github.com/pret/pokediamond/tree/b623abef1b4dfe204ca50aab42447ac00c134290)
- `arm9/overlays/05/asm/ov05_021EC458.s` Git blob `e084c052388870e0d3e984379b1bb23e1a911790`, `ov05_021EC9D4` and `ov05_021EC99C` (six LCRandom draws, original D/P indexed-removal bug), `ov05_021EC8A8` and `ov05_021EC944` (Everstone MT pending egg PID), `ov05_Daycare_CreateEgg` (international collection).
- `arm9/src/math_util.c` `LCRandom`, `MTRandom`, `PRandom`; `arm9/src/daycare.c` historical Save_Daycare language/accessor.
- See the detailed [Diamond/Pearl assembly analysis](GEN4_DIAMOND_PEARL_DAYCARE_ASSEMBLY.md) and [B12/B13/B15/B16 dependency ledger](GEN4_L2_IV_HISTORICAL_WITNESSES.md).
- The original retro retail assembly has **not** been executed as a separate oracle, and no original retail save-backed exact historical parent + egg bundle was independently attested. Original source SHA-256, binary match, regional equivalence and licensing remain L5 verification needs.

## Reproducible standalone validation of exact committed new files

The **exact GitHub-current B18 header and test contents** were compared with the local testing files by their full text length and a per-character FNV-1a checksum before testing. Tests include source-compatible B12/B13 public headers; these two dependencies were reproduced locally for the standalone test, not obtained from a complete checkout.

Normal compile/run (PASS):
```sh
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -Iinclude tests/test_legality_l2_dp_alternative_egg_histories.cpp -o build-host/l2_dp_alternatives
./build-host/l2_dp_alternatives
```

GCC and Clang AddressSanitizer/UndefinedBehaviorSanitizer were separately run with `-fsanitize=address,undefined -fno-omit-frame-pointer` and `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1` (PASS). Output: `L2 B18 bounded DP PID+IV candidate histories: 8232 positive/local negative/Unknown combinations PASS`.

This **does not** establish that all previously committed five test suites ran, or that exact-head full host/focused RSE/ASan+UBSan workflow CI passed. The coordinator alone owns shared Makefiles, workflows, `src/Legality/Legality.cpp` and accepted-branch reconciliation; all six `tests/test_legality_l2_*` must be registered and executed in exact-head CI, without modifying another lane's source.

## Missing stage-proof fields and merge hazards

A PK4, even if checksum-valid, does not carry the originally deposited pair identity, original held items, MT/LCRNG transcript, parent stages, daycare event/collection chronology, or proof of an exhaustive alternative-parent enumeration. These must be supplied by independent original-stage evidence. Synthetic positive tests do **not** generate such proof.

The lane branch originated from accepted legality SHA `0dccf2e1` and currently diverges from newer `d29eada5` accepted ancestry. Coordinator re-fetches and forward-reconciles all live heads while preserving #283 Platinum work and independently authored #284 HGSS PID. No reset/rebase backward/force push, no self-merge, no MAIN/#103 integration and no source-save/staged-Pokémon writes.
