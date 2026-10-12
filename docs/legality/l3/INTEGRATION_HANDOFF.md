# L3 integration handoff — freeze feature expansion and run exact-head gates

**Lane:** L3 moves and historical transfers, [issue #288](https://github.com/GlitchedZeus/PokeBank-NX/issues/288); [draft PR #296](https://github.com/GlitchedZeus/PokeBank-NX/pull/296). **Accepted parent at validation:** `d29eada5c087e201e00c749703f1a2de3ddfadeb`. The PR's exact current head is shown in GitHub; require `git rev-parse HEAD` to match the head being accepted. Never reset/rebase backward, force-push, self-merge, edit the accepted branch or modify shared coordinator-owned analyzer/CI files from L3.

## Delivered (L3-owned paths only)

**Read-only headers:** `Gen14CandidateMoveChronology.h`, `Gen14CandidateMoveGbaLevelUpTimeline.h`, `Gen14CandidateMoveGbaStarterTables.h`, `Gen14CandidateMoveGbaNinjaTables.h`, `Gen14CandidateMoveGbaEvolutionStages.h`, `Gen14CandidateMoveShedinjaCopyStage.h`, `Gen14CandidateMoveMachineEvidence.h`, `Gen14CandidateMoveGbaRelearner.h`, `Gen14CandidateMovePalParkJointStage.h`; the existing owned `Gen14CandidateMoveHistory.h` includes new header-level static smoke checks.

**Independent seven native C++20 programs:** `tests/test_legality_l3_move_chronology.cpp`, `tests/test_legality_l3_gba_versioned_levelup.cpp`, `tests/test_legality_l3_gba_starter_evolution.cpp`, `tests/test_legality_l3_shedinja_copy.cpp`, `tests/test_legality_l3_machine_methods.cpp`, `tests/test_legality_l3_gba_relearner.cpp`, `tests/test_legality_l3_palpark_joint_stage.cpp`.

**Validation-only runner:** `tests/test_legality_l3_run_all.sh`. This does not alter shared Makefiles or workflows; it fails closed unless the *actual full* `include/Legality/Gen3EmeraldLevelUpData.inc` source is present, compiles seven binaries with `-std=c++20 -Wall -Wextra -Wpedantic -Werror`, executes all seven under normal and ASan/UBSan, and prints each test and source tree SHA. It does **not** use a mock, skip any full-data loop, mutate save files or make legality judgments. Coordinator may run:
```bash
L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 \
  bash tests/test_legality_l3_run_all.sh
```
The optional blob pin deliberately blocks unexplained Emerald table replacements. For a forward-advanced accepted branch, review the new data revision and set a consciously updated expected SHA.

## Validation ledger (2026-10-12)

| Gate | Actual result and limitation |
|---|---|
| Live accepted vs L3 ancestry | GitHub compared accepted `d29eada5...`; L3 remained **0 commits behind**, never reset, and is still a draft |
| Original pinned retail evidence | Rechecked Ruby/Sapphire `pokeruby@63a8cbf`, Emerald `pokeemerald@5eff786` and FRLG `pokefirered@c75f352`, including level-up tables, evolution-scene ordering, machine IDs and relearner selection routine. Those facts prove retail *possibilities*, not the specimen's actual actions |
| Original accepted Emerald all-species source audit | Independently parsed the full accepted generated source on GitHub: **388 offsets; 3,947 packed entries; 386 species each nonempty; maximum 17 original offers/species, 14 unique options after excluding first known move; 11 species with empty resulting relearner menus**. Numerical domain and capacity checks passed. This was a **source-data audit in JavaScript**, not execution of C++ native sweeps |
| All seven C++ suites on local subset | **PASS** using GCC strict C++20 `-Werror`, plus separate GCC ASan/UBSan for all seven, and Clang strict C++20 for all seven. Local build used an expressly named 12-species-only Emerald compile fixture. The versioned-levelup runner had *only the all-386 local sweep* disabled in a **temporary local build copy**, not GitHub. The relearner's `--sampled` option intentionally skips its all-386 loop on this local fixture |
| Exact-file reproducibility | The published `test_legality_l3_shedinja_copy.cpp` and `test_legality_l3_gba_relearner.cpp` were independently restored from their **exact GitHub blob bytes**, with `git hash-object` matching; the two accepted Pal Park helper headers and the L3 runner shell script were also byte-identical to GitHub. **Other local L3 source/test mirror files have Git blob SHA mismatches**; their subset-only passes cannot be represented as executions of every exact committed header/test combination |
| Full accepted source + **all seven exact committed** native binaries and sanitizers | **NOT EXECUTED/PASSED yet**. Local runtime cannot retrieve the full accepted 3,947-entry file; the committed validation runner correctly exits code 2 (`L3 BLOCKED: full accepted Emerald learnset source is missing`). Coordinator must execute runner in its real checkout |
| GitHub Actions exact-head host + RSE + ASan/UBSan | **PENDING**: latest exact-head workflow remains queued at the validation checkpoint. Never claim a green gate or successful merge |
| Main analyzer integration | **NOT INTEGRATED**. The accepted `src/Legality/Legality.cpp` contains older Gen I–IV move helpers but does not invoke these L3 new candidate history/relearner/Pal Park joint helpers. These are conditional evidence utilities, not yet production legality decisions |
| Hardware, complete event/transfer/exhaustive history | **NOT TESTED**; not an acceptable basis for Invalid |

The local source-subset test first exposed an expected mismatch: running the complete 386-species runner on the deliberately incomplete local mock aborts its species-count assertion. That is **not** a retail data regression or full-source pass. A second correction in the committed relearner exhaustive fixture ensures that species with one distinct move (for example Unown) keep a structurally valid after-snapshot when their menu is empty; the asserted outcome is candidate contradiction for the specific unsupported relearn attempt, not malformed slots or global Invalid.

## Coordinator-owned integration tasks — blockers before merge

1. Re-fetch and compare live accepted and L3 heads; retain every newer accepted commit. Do not replace another lane's branch or cherry-pick an obsolete head.
2. Checkout a candidate exact tree and run **`L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 bash tests/test_legality_l3_run_all.sh`**. Confirm every one of seven normal and seven sanitizer executions finishes; this includes real all-386 Emerald source sweeps.
3. Register the seven independent L3 `test_legality_l3_*.cpp` programs (or the validated all-seven shell runner) in **coordinator-owned** `Makefile.host.base` / `Makefile.host` host-test and host-sanitize graph. The accepted `test_gen14_candidate_move_history` binary compiles L3 header smoke but does **not** execute these seven `main()` programs.
4. Re-run actual exact-head **host tests, focused RSE regression, RSE ASan/UBSan and host sanitizers**. Verify the job checkout SHA equals the PR head and no source was omitted.
5. Reconcile accepted branch and sibling legality lanes on a separate coordinator-owned integration head, rerun all normal/sanitizer gates with those merged trees, and review L3 header dependencies on accepted `Gen3EmeraldLevelUpEvidence.h`, `Gen34PalParkHMIds.h`, `Gen34PalParkSnapshotTransition.h`.
6. Decide **production-call integration** separately in coordinator-owned `Legality.cpp` and relevant UI/reporting, preserving read-only guarantees, no save edits, source-version disambiguation and **Incomplete/Unknown** results whenever source/history is absent. Never promote one contradicted **hypothetical candidate** to global Invalid.
7. Review pinned upstream source redistribution/license provenance, genuine native/save evidence ownership, and final coordinator code review. L3 is NOT authorized to self-merge PR #296 or the umbrella PR.

## Unresolved factual evidence — keep Incomplete

- Ruby/Sapphire/FRLG original per-version species move acquisition remains pinned for **12 out of 386**, with 374 unmodeled species per source-game family; do not borrow Emerald's entire species table.
- Full original Gen IV Diamond/Pearl/Platinum/HGSS machine, tutor, reminder, service-access and after-Pal-Park acquisition records are not independently frozen. A made-up native witness ID inside a conditional unit test is **never** a verified original retail packet.
- Actual game-visit order, original save/party snapshots, evolving player's move choices, original Move Relearner service use, exact Pal Park arrival and Nincada's party slot/sibling-copy event are normally erased from a detached PK3/PK4.
- Other event/egg/trade/GameCube move routes, source species/form/parent history, original cross-game migration steps and **complete alternative-candidate enumeration** require L5 research and combined coordinator evaluation.
- Candidate-local `CandidateContradicted` is **not a global Invalid finding**. Absent or unsupported source/ancestry must remain Incomplete (Unresolved); the L3 helpers are read-only and no writes are enabled.

**Status:** M06/M07/M08/M11/M12/M13 remain **PARTIAL**; M17 historic Pal Park HM safety preserved. This is a coordinator review package and **feature work should pause after handoff**, not another feature-extension branch.
