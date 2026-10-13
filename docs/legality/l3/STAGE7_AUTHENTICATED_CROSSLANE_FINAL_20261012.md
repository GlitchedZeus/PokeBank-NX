# L3 Stage 7/7 — authenticated move-history boundary and coordinator handoff

Scope: existing `feature/legality-l3-moves-transfer-20261012`, PR #296, L3 only. Candidate hypotheses, never global Pokémon Invalid or save mutations. This stage adds no production `Legality.cpp` integration and does not merge coordinator PR #298.

## Implemented L3 API

`include/Legality/Gen14AuthenticatedMoveHistory.h` provides a read-only C++20 `constexpr` evaluator and candidate-alternative aggregator. All data is caller-supplied: it does not guess or manufacture history from an isolated PK1–PK4 record. Every `Snapshot` is an independently verified immutable physical record with native generation-specific species/move limits, candidate/individual ID and unique snapshot ID. Every step binds a full before/after snapshot, original game, exact current title, move/slot, action, source method and receipt. Histories are rejected as `Unknown` unless a complete previously source-checked L3 chronology, all intermediate decisions and immutable identities are attested.

### L2 breeding contract — external *facts*, not L3 authority to invent history

For `Method::InheritedEgg`, an L2-produced `BreedingEvidence` must contain a unique evidence ID, original egg snapshot ID, both different authenticated parent snapshot IDs, exact native egg source version, candidate/individual identities, species/form, original move identity and exact acquisition level. All four flags must be true: `originalEggVerified`, `parentsIndependentlyVerified`, `inheritanceMethodSourceVerified`, `sameEggIndividualVerified`. Original breeding game must support breeding (Gen II onward). Evidence is tied either to one L3 `Learn` step with the same `crossLaneEvidenceId` **or to an independently attested inherited move already occupying `originSlot` in the earliest immutable egg snapshot** (`originMoveClaim=true`), never both; orphan/duplicated/wrong-parent/foreign-species/wrong-version evidence causes **Unknown**. A native PK snapshot alone authenticates NONE of these claims.

### L4 event contract — exact template/distribution, not a global move allowlist

For `Method::DistributedEvent`, an L4-produced `EventEvidence` must provide unique evidence ID, nonzero source distribution and template IDs, independently authenticated original encounter snapshot ID, exact source game, species/form, candidate/individual and move identity, an independently checked valid level interval, plus independent `distributionIndependentlyVerified`, `templateIndependentlyVerified`, `templateMethodMoveVerified` and `originalEncounterVerified`. A single authentic distribution can carry multiple listed moves, but each claim has its own evidence ID and must share template and recipient identity. Event-exclusive moves already present at original distribution may be source-verified using `originMoveClaim=true`, exact `originSlot` and immutable `originalSnapshotId`; do not forge a later Move Tutor teaching step. Unknown template/method and missing event proofs produce **Unknown**, not global Invalid.

### Cross-generation chronology and anti-splicing

The evaluator checks source/target generation species/move bounds, no gap or duplicate moves, immutable snapshot identity across each transition, no skipped/interleaved snapshot reuse, no species/form drift without an explicit source-checked evolution step, monotonic levels, unique consumable TM/HM receipts, source-version-specific method verification and bound event/egg claims. It replays four-slot insertions/replacements/deletions and exact before/after slots, enforces Gen I/II Time Capsule move/species boundary, Gen III→IV Pal Park old-HM exclusion and *unchanged immediate-arrival slots*, plus separately attested postarrival Gen IV learning. Every step is preflighted before replay: an unsupported later source cannot turn an early hypothetical contradiction into a false definitive result. Candidate alternatives cannot mix different IDs; unenumerated or unresolved alternatives retain Unknown. `globalInvalidAuthorized` is hardwired `false`.

**Essential integration rule:** `sourceSpecificChronologyPassed` must be set ONLY if the existing Stage 1–6 original-source engine independently accepted the exact candidate's full required level-up offers and all source title methods/receipts. A boolean supplied by a detached PK record is never sufficient evidence. This Stage 7 component checks authenticated cross-lane links/slot integrity; it is NOT a replacement for the source-ordered move acquisition engines and cannot validate real Pokémon independently. L2 and L4 must provide their own independently authenticated witnesses and must never be changed by the coordinator to make a path pass.

## Executed tests and scope

Two new `tests/test_legality_l3_authenticated_{crosslane,multigen}.cpp` standalone C++20 suites, synthetic witnesses **explicitly not native parsed records, genuine event distributions or original title method tables**. Each one was compiled/executed with:

- GCC `g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -O2 ...`
- Clang `clang++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -O2 ...`
- GCC and Clang `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`, `ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 UBSAN_OPTIONS=halt_on_error=1`

Each of four compiler/build variants passed both programs: **360,000 synthetic source-boundary assertions + 360,000 synthetic multigen assertions = 720,000 per variant**; eight successfully executed binaries in total. Regression cases include wrong/missing/duplicated L2/L4 evidence and already-present native origin slots; contradictory method/identity/source binding, event move and level; tampered immutable move slots; false interleaved histories, TM receipt gaps, Time Capsule and Pal Park boundary behavior, source visits, evolution and alternative history aggregation.

Full L3 runner is expanded from 29 to **31** C++20 suites, 62 native/sanitizer executions per compiler, **124 total** for GCC+Clang. The full original-header/native parser runner was **not executed** because direct `git` checkout cannot resolve `github.com`; only the connected GitHub API provided files. The original `Gen3EmeraldLevelUpData.inc` Git blob remains `3e53c1c951876a26cace5dde060cc9ff2c35d0a7` in the live Stage 6 tree. Exact source-data SHA verified as unchanged; no regenerated source facts were invented in Stage 7.

## Full acceptance blockers and order

1. Keep PR #296 OPEN/DRAFT and do not merge. Re-fetch coordinator #298 and all accepted/working heads before any integration; forward-only incorporate *L3-owned* commits after last imported L3 checkpoint. Preserve newer L1/L2/L4/QoL/Main work. Do not blindly merge the L3 branch over the coordinator.
2. Independently verify source data/licensing and branch conflict freedom. Missing originals: HGSS binary NARC level-up data; full Platinum level-up (twelve species only pinned); Gen IV DP/HGSS original item→move mappings; broad Gen III original versioned records outside independently pinned subset; unmodeled special forms, tutor/reminder/event/breeding services. Return Unknown when not proved.
3. Integrate independently finished L2 breeding and L4 event witnesses through precisely specified structs; add actual native PK1–PK4 recorded paired snapshots with authenticated same-individual lineage. Detached PK files never invent original parentage/distributions/receipts. Stage 7 synthetic snapshots are NOT real save-parser coverage.
4. Run on exact forward-only combined HEAD (not the old successful SHA):

```bash
L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 CXX=g++ bash tests/test_legality_l3_run_all.sh
L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 CXX=clang++ bash tests/test_legality_l3_run_all.sh
```

Both now require 31 normal + 31 sanitizer programs. Then run complete host + RSE + full host ASan/UBSan, production Switch NRO build, source licensing/review and hardware no-write safety. Check GitHub Actions exact new head; prior queued workflows cannot validate subsequent commits. Any failed or unexecuted full integration gate means **DO NOT ACCEPT / DO NOT MERGE**.

Stage 7 concludes this isolated lane's assigned program; it does not start Gen V, Master Vault, UI changes or Stage 8.
