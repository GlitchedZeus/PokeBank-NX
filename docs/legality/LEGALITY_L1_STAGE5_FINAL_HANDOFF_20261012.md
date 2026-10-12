# L1 final major Stage 5 — original encounter evidence contract and coordinator handoff

**Date:** 2026-10-12. **Owner lane:** PR #293 `feature/legality-l1-encounters-rng-20261012`, **DRAFT, not merged**. This handoff completes the five assigned L1 engineering stages but **does not authorize production adoption**, an Invalid verdict, any mainline merge, or acceptance of untested combined branches.

## Stage 5 implementation (actual executable code)

### 1. Typed source-stage evidence boundary — `include/Legality/Gen14OriginalEvidenceBridge.h`

Read-only explicit C++20 status mapping across **all 12 independent source helpers delivered in Stages 1–4**:

- Gen I source grass/fishing.
- Gen II fishing, original Headbutt/contest, swarms, roaming route adjacency.
- Gen III wild source catalog, original wild RNG stage, original roamer PID-IV frame.
- GameCube Colosseum/XD native origin provenance.
- HGSS original physical Headbutt tree; D/P/Pt Honey Tree acquisition timer and trainer-dependent Munchlax tree; Method J/K original nature/PID stage.

Each supported helper enum maps to exactly one of:

1. **Unknown**: unattested, corrupt, cross-game, unsupported, bounded search exhausted; default for any future unrecognized enum.
2. **CandidateLocalContradiction**: contradiction **only of an independently witnessed candidate source/stage**, never a verdict about a Pokémon or all acquisition paths.
3. **SourceOnly**: species/slot/rod/tree/timer/world compatibility only, **no proof of captured creature or RNG transcript**.
4. **RngStageOnly**: separately witnessed original LCRNG/PID/IV/nature stage only, **no proof of original source or complete encounter**.
5. **ConditionalNative**: independently claimed Colosseum/XD scene joined to valid native-origin PID/OT/species fields, but **not original archived full source chain or global legitimacy**.

No implicit integer-to-status inference, no `globalInvalid` mapping, and no production `Legality.cpp` hook. A valid known historical stage is never promoted to a universal legality decision.

### 2. Bounded, allocation-free alternative-route evidence ledger — `include/Legality/Gen14OriginalHistoryLedger.h`

The bridge's evidence flows through an entirely diagnostic C++20 ledger supporting **up to 256 stage witnesses and 64 source alternatives**, allowing multiple individually witnessed stages per candidate route. It tracks the same creature root, independently attested original game and original save, physical route identity, source/RNG/native stage distinction, conflicting or duplicate stage reports, negative selected-route contradictions, and explicitly unresolved egg/gift/trade/event/transfer branches.

**Enforced constraints:** rejected missing/unattested original creature roots; candidate row/root/source-game mismatches; duplicate per-route stage evidence; conflicts across original game/save identity for the same alleged route; invalid enum bytes; stage/evidence family mismatch (in particular a Gen IV candidate cannot impersonate the only currently supported GameCube native conditional witness); >256 witnesses and >64 candidates. Alternative original source games in separate independently recorded routes remain allowed.

Return labels are deliberately `UnknownMissingRoot`, `UnknownMalformedInput`, `UnknownCapacity`, `UnknownAlternativeHistories`, `CandidateLocalContradictionsOnly`, `SourceOrStageCompatibilityOnly`, `ConditionalOriginalCandidateOnly`. **There is NO output or path for globally Legal or Invalid**; both booleans permanently remain false. Even 64 independently contradictory source routes are never treated as exhaustive across all Gen I–IV egg/event/gift/trade/transfer and special histories.

**Safe adoption proposal:** coordinator may first wire only a read-only developer/source-inspection diagnostic displaying stage labels and missing provenance. Do not use ledger results to affect ordinary user-facing Invalid/Legal, Save Inject, native record mutation, or eligibility gates. Coordinator, not L1 author, owns any actual production integration change.

## Actual executed tests, normal and sanitizer

Environment: locally mirrored identical GitHub helper/test source bytes, GCC **14.2.0**, C++20. Both profiles **actually compiled and executed**, no unsupported green claims:

- Normal: `g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude [-Itests]`.
- Sanitized: `g++ -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror -Iinclude [-Itests] -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer` with `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

### Four new Stage 5 suites

| Suite | Verified native and sanitizer coverage |
|---|---|
| `test_legality_l1_stage5_status_bridge.cpp` | **12** source helper enum families, **1,536** corrupt enum-byte tests plus known foreign, unverified, and success maps |
| `test_legality_l1_stage5_ledger.cpp` | 256-witness / 64-route bounded capacity, cross-source alternative status, missing root/save/game/method provenance, duplicate route-stage and unsupported family/stage controls |
| `test_legality_l1_stage5_permutations.cpp` | **15,625** alternative-route category combinations (`5^6`), duplicate stages and conflicting original save identity; all results tested to never claim globally Legal/Invalid |
| `test_legality_l1_stage5_real_sources.cpp` | **268** Gen I version-qualified positives and opposing real source negatives, **145** Gen II fishing physical rows, **306** Gen III original wild physical rows, real Gen IV Method J original RNG witness, original HGSS tree, original GameCube Colo/XD native-origin stage and mixed alternate route outcomes |

**New Stage 5 tests 4/4 normal PASS + 4/4 ASan/UBSan PASS.**

### Prior Stages 1–4: 12 exact-source standalone suites rerun

All **12/12 normal PASS + 12/12 ASan/UBSan PASS**, rerun this Stage 5 session, not merely credited from previous turns:

```
test_legality_l1_gen1_wild_fishing
test_legality_l1_gen2_fishing
test_legality_l1_gen2_roamer_routes
test_legality_l1_gen2_swarms
test_legality_l1_gen2_tree_contest
test_legality_l1_gen3_roamer_frame
test_legality_l1_gen3_wild_catalog
test_legality_l1_gen3_wild_rng
test_legality_l1_stage4_hgss_tree
test_legality_l1_stage4_honey
test_legality_l1_stage4_method_jk
test_legality_l1_stage4_native_pk3_pk4
```

Every one of these 12 prior test file Git blobs was re-fetched from GitHub at the Stage 5 branch and compared to local compiled exact `git hash-object`; all **12/12 IDENTICAL**, plus their underlying Stage 1–4 source helper blobs matched their previously independently verified source blob identities. The six new source/test GitHub blob SHAs were also each checked **identical** against exact local compiled originals:

```
109129a281954db966fba3f08267ec92a92cb031 include/Legality/Gen14OriginalEvidenceBridge.h
25d70c5303b0a92b9cf8230ede26e944b3d513f3 include/Legality/Gen14OriginalHistoryLedger.h
1ebace268b3ab10329380e39123425890b94bdd7 tests/test_legality_l1_stage5_ledger.cpp
46082fcdf14096185675d86f7269611ddd4e4583 tests/test_legality_l1_stage5_permutations.cpp
a6dbc87699216458cd447ff892ef6adb41cc801c tests/test_legality_l1_stage5_real_sources.cpp
e4180ee923bf3c25e332bb4691b8d9da05358086 tests/test_legality_l1_stage5_status_bridge.cpp
```

**Actual local result: 16 standalone suites × 2 profiles = 32/32 executed and PASS.**

### Exact-head integration gates not passed here

`tests/test_legality_l1_run.sh` now registers **29 suites × 2 profiles = 58 required complete L1 executions**. The remaining **13 older L1 suites × 2 profiles = 26 executions were NOT rerun locally at this branch HEAD**. Full repository checkout could not be obtained because GitHub DNS access is blocked in the local test container (GitHub connector source access still works). The new 16 suites ran on a mirror of the exact tested code, **not** the full checkout with all dependencies.

The mandatory coordinator acceptance sequence remains:

1. Fetch freshest L1 PR #293 and integration PR #298 author heads. Preserve all newer work; never reset/rebase backward/force-push/merge author PRs.
2. Execute all **29 L1 suites** in both GCC and ASan/UBSan on **the exact chosen combined SHA/tree**, with `L1_EXPECT_SHA` and `L1_EXPECT_TREE` enabled.
3. Run full host, focused RSE regression, original format save fixtures, native *production* PK3/PK4 encrypted parsers and missing-bitness/corrupt-checksum guards, and integrated ASan/UBSan on exact combined HEAD.
4. Check all cross-lane event/breeding/move/QA flags, conflicting acquisition paths and real Gen I–IV historical source coverage with the L5 QA lane; specifically require counterexamples for all proposed **production Invalid** decisions, rejecting any purported exhaustive universe that omits a legal trade/event/egg/transfer or future-generation evolution.
5. Only after all actual gates PASS may the coordinator accept an explicitly informational diagnostic adapter. L1 author does not approve global Invalid, merge author PRs or modify production.

### Remaining coverage blockers

Historical source originals still lack complete all-map physical slot coverage for each G1–G4 title, event and transfer archive/chain proof, original full RNG consumption from source encounter through stored creature, real legal Gen III GameCube locked shadow PID-IV chains, HGSS Safari block evolution, full Radar chain and polished contest/honey alternatives, and original save-container chain-of-custody. Native encrypted PK3/PK4 synthetic roundtrips are not archived original retail captures or full production parser verification.

This stage is **complete as an engineering contribution**, not complete as the full Pokémon legality engine. All source-save immutable safeguards, no accidental global Invalid, no MAIN, accepted legality, shared CI, coordinator PR #298 or other-lane modifications remain active.
