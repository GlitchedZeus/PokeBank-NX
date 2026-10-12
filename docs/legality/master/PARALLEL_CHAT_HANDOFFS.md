# Five self-contained prompts for five new development chats

**Usage:** Open five separate new ChatGPT chats inside PokeBank NX and copy each matching block below as the opening message. Each chat independently fetches live GitHub; do not assume it knows work from this conversation. The master tracker is [#285](https://github.com/GlitchedZeus/PokeBank-NX/issues/285), and its machine-readable [90-task catalog](WORK_CATALOG.csv) is the authoritative assignment map. The main coordination chat controls shared integration, accepted branch and core analyzer wiring.

## Chat 1: Encounters and wild RNG — issue #286

```text
PokeBank NX — MAJOR Gen I–IV legality development lane L1: Encounters and wild RNG.
Repository: GlitchedZeus/PokeBank-NX. Your dedicated issue: https://github.com/GlitchedZeus/PokeBank-NX/issues/286; master issue #285.
First: read docs/legality/master/README.md, WORK_CATALOG.csv and REFERENCE_FREEZE.md from the audit/legality-master-parallel-plan-20261012 GitHub branch. Review accepted Gen I–IV legality source, and research PR #252's pinned packets before any work.
YOUR TASKS: L1 catalog rows E01–E18. FIRST MAJOR TRANCHE: E07 E11 E13 E15.
Start from the CURRENT LIVE accepted legality branch feature/legality-engine-gen1-4-20260929. Re-fetch all heads/PRs/CI, preserve every newer commit, never reset/rebase backward or force-push. Your personal development branch name is feature/legality-l1-encounters-rng-20261012. Before branch creation, re-check existing branch/PR names and dependencies; NEVER overwrite an already advancing branch. Do not work on another lane's source.
OWNED FILES / NEW TESTS: source-linked wild/static encounter helper headers (Gen1Encounter, Gen2Wild/Static, Gen3Safari/MethodH/Colo/XD encounters, Gen4Wild/Static/Lead), new tests prefixed tests/test_legality_l1_*.
RESTRICTIONS: Never edit ordinary egg PID/IV, source event archive, four-slot move histories, shared analyzer or shared host Makefiles. Only the coordinator is allowed to change src/Legality/Legality.cpp, shared host Makefiles/workflows and accepted branch, and to reconcile all-green lane PRs. MAIN remains untouched. Umbrella legality PR #103 must NOT merge into MAIN without explicit permission. No source-save/staged-Pokémon writes, auto-fixes, Gen V, Master Vault or Product UI/touch.
Each session MUST be a MAJOR practical tranche, not 1–2 microfixes: complete a coherent set of ready catalog IDs with actual source-backed work, original-game-specific evidence and new native positive/negative/Unknown tests. Run exact-head full host, focused RSE and ASan/UBSan CI. Document exact source refs, commits, coverage and missing original-stage fields, using one meaningful lane PR per major tranche. Do NOT self-merge.
If evidence is missing, document exact blockers in issue #286, ask L5 for source packet and work on the NEXT ready task in the SAME session. Unknown/unverified history stays Incomplete; never turn a local contradictory candidate or bounded search into global Invalid. Send the main coordination chat the PR URL, head SHA, all CI gates, catalog IDs and merge hazards when ready.
```

## Chat 2: Breeding and egg PID/IV — issue #287

```text
PokeBank NX — MAJOR Gen I–IV legality development lane L2: Breeding and egg PID/IV.
Repository: GlitchedZeus/PokeBank-NX. Your dedicated issue: https://github.com/GlitchedZeus/PokeBank-NX/issues/287; master issue #285.
First: read docs/legality/master/README.md, WORK_CATALOG.csv and REFERENCE_FREEZE.md from the audit/legality-master-parallel-plan-20261012 GitHub branch. Review accepted Gen I–IV legality source, and research PR #252's pinned packets before any work.
YOUR TASKS: L2 catalog rows B01–B18. FIRST MAJOR TRANCHE: B12 B13 B15 B16.
Start from the CURRENT LIVE accepted legality branch feature/legality-engine-gen1-4-20260929. Re-fetch all heads/PRs/CI, preserve every newer commit, never reset/rebase backward or force-push. Your personal development branch name is feature/legality-l2-breeding-20261012. Before branch creation, re-check existing branch/PR names and dependencies; NEVER overwrite an already advancing branch. Do not work on another lane's source.
OWNED FILES / NEW TESTS: Gen*Egg*, Gen*Breeding*, Gen4NativeBreeding*, Gen4Platinum*, new tests prefixed tests/test_legality_l2_*.
RESTRICTIONS: Do not duplicate already accepted wild work #282 or pending/integrated #283 Platinum PID; no wild lead or original event archive edits. Only the coordinator is allowed to change src/Legality/Legality.cpp, shared host Makefiles/workflows and accepted branch, and to reconcile all-green lane PRs. MAIN remains untouched. Umbrella legality PR #103 must NOT merge into MAIN without explicit permission. No source-save/staged-Pokémon writes, auto-fixes, Gen V, Master Vault or Product UI/touch.
Each session MUST be a MAJOR practical tranche, not 1–2 microfixes: complete a coherent set of ready catalog IDs with actual source-backed work, original-game-specific evidence and new native positive/negative/Unknown tests. Run exact-head full host, focused RSE and ASan/UBSan CI. Document exact source refs, commits, coverage and missing original-stage fields, using one meaningful lane PR per major tranche. Do NOT self-merge.
If evidence is missing, document exact blockers in issue #287, ask L5 for source packet and work on the NEXT ready task in the SAME session. Unknown/unverified history stays Incomplete; never turn a local contradictory candidate or bounded search into global Invalid. Send the main coordination chat the PR URL, head SHA, all CI gates, catalog IDs and merge hazards when ready.
```

## Chat 3: Moves and historical transfers — issue #288

```text
PokeBank NX — MAJOR Gen I–IV legality development lane L3: Moves and historical transfers.
Repository: GlitchedZeus/PokeBank-NX. Your dedicated issue: https://github.com/GlitchedZeus/PokeBank-NX/issues/288; master issue #285.
First: read docs/legality/master/README.md, WORK_CATALOG.csv and REFERENCE_FREEZE.md from the audit/legality-master-parallel-plan-20261012 GitHub branch. Review accepted Gen I–IV legality source, and research PR #252's pinned packets before any work.
YOUR TASKS: L3 catalog rows M01–M18. FIRST MAJOR TRANCHE: M06 M07 M11 M12.
Start from the CURRENT LIVE accepted legality branch feature/legality-engine-gen1-4-20260929. Re-fetch all heads/PRs/CI, preserve every newer commit, never reset/rebase backward or force-push. Your personal development branch name is feature/legality-l3-moves-transfer-20261012. Before branch creation, re-check existing branch/PR names and dependencies; NEVER overwrite an already advancing branch. Do not work on another lane's source.
OWNED FILES / NEW TESTS: Gen14CandidateMove*, Gen*JointMove*, EmeraldLevelUp*/ShedinjaEvolutionMove*, Gen*PalPark*/PostPalPark*, new tests prefixed tests/test_legality_l3_*.
RESTRICTIONS: Do not change original parent PID/IV, encounter RNG, event-card source or central analyzer. Only the coordinator is allowed to change src/Legality/Legality.cpp, shared host Makefiles/workflows and accepted branch, and to reconcile all-green lane PRs. MAIN remains untouched. Umbrella legality PR #103 must NOT merge into MAIN without explicit permission. No source-save/staged-Pokémon writes, auto-fixes, Gen V, Master Vault or Product UI/touch.
Each session MUST be a MAJOR practical tranche, not 1–2 microfixes: complete a coherent set of ready catalog IDs with actual source-backed work, original-game-specific evidence and new native positive/negative/Unknown tests. Run exact-head full host, focused RSE and ASan/UBSan CI. Document exact source refs, commits, coverage and missing original-stage fields, using one meaningful lane PR per major tranche. Do NOT self-merge.
If evidence is missing, document exact blockers in issue #288, ask L5 for source packet and work on the NEXT ready task in the SAME session. Unknown/unverified history stays Incomplete; never turn a local contradictory candidate or bounded search into global Invalid. Send the main coordination chat the PR URL, head SHA, all CI gates, catalog IDs and merge hazards when ready.
```

## Chat 4: Events and metadata — issue #289

```text
PokeBank NX — MAJOR Gen I–IV legality development lane L4: Events and metadata.
Repository: GlitchedZeus/PokeBank-NX. Your dedicated issue: https://github.com/GlitchedZeus/PokeBank-NX/issues/289; master issue #285.
First: read docs/legality/master/README.md, WORK_CATALOG.csv and REFERENCE_FREEZE.md from the audit/legality-master-parallel-plan-20261012 GitHub branch. Review accepted Gen I–IV legality source, and research PR #252's pinned packets before any work.
YOUR TASKS: L4 catalog rows V01–V18. FIRST MAJOR TRANCHE: V07 V08 V10 V11.
Start from the CURRENT LIVE accepted legality branch feature/legality-engine-gen1-4-20260929. Re-fetch all heads/PRs/CI, preserve every newer commit, never reset/rebase backward or force-push. Your personal development branch name is feature/legality-l4-events-meta-20261012. Before branch creation, re-check existing branch/PR names and dependencies; NEVER overwrite an already advancing branch. Do not work on another lane's source.
OWNED FILES / NEW TESTS: Gen*Event*, Gen4UnifiedEvent*, Gen4RawText*, Gen4ContestRibbon*, event source attestation, new tests prefixed tests/test_legality_l4_*.
RESTRICTIONS: No redistributing event Pokémon binaries, original game ROMs, source encounter tables, egg PID or shared analyzer edits. Only the coordinator is allowed to change src/Legality/Legality.cpp, shared host Makefiles/workflows and accepted branch, and to reconcile all-green lane PRs. MAIN remains untouched. Umbrella legality PR #103 must NOT merge into MAIN without explicit permission. No source-save/staged-Pokémon writes, auto-fixes, Gen V, Master Vault or Product UI/touch.
Each session MUST be a MAJOR practical tranche, not 1–2 microfixes: complete a coherent set of ready catalog IDs with actual source-backed work, original-game-specific evidence and new native positive/negative/Unknown tests. Run exact-head full host, focused RSE and ASan/UBSan CI. Document exact source refs, commits, coverage and missing original-stage fields, using one meaningful lane PR per major tranche. Do NOT self-merge.
If evidence is missing, document exact blockers in issue #289, ask L5 for source packet and work on the NEXT ready task in the SAME session. Unknown/unverified history stays Incomplete; never turn a local contradictory candidate or bounded search into global Invalid. Send the main coordination chat the PR URL, head SHA, all CI gates, catalog IDs and merge hazards when ready.
```

## Chat 5: Research and independent QA — issue #290

```text
PokeBank NX — MAJOR Gen I–IV legality development lane L5: Research and independent QA.
Repository: GlitchedZeus/PokeBank-NX. Your dedicated issue: https://github.com/GlitchedZeus/PokeBank-NX/issues/290; master issue #285.
First: read docs/legality/master/README.md, WORK_CATALOG.csv and REFERENCE_FREEZE.md from the audit/legality-master-parallel-plan-20261012 GitHub branch. Review accepted Gen I–IV legality source, and research PR #252's pinned packets before any work.
YOUR TASKS: L5 catalog rows Q01–Q18. FIRST MAJOR TRANCHE: Q01 Q02 Q03 Q06 Q09.
Start from the CURRENT LIVE accepted legality branch feature/legality-engine-gen1-4-20260929. Re-fetch all heads/PRs/CI, preserve every newer commit, never reset/rebase backward or force-push. Your personal development branch name is audit/legality-l5-research-qa-20261012. Before branch creation, re-check existing branch/PR names and dependencies; NEVER overwrite an already advancing branch. Do not work on another lane's source.
OWNED FILES / NEW TESTS: docs/legality/research/** in independent branch, new tools/audit_legality_l5_* scripts, source/fixture manifests; no production C++ files.
RESTRICTIONS: Research only. Do not edit production verifier logic, shared Makefiles, sibling tests or move accepted branch. Only the coordinator is allowed to change src/Legality/Legality.cpp, shared host Makefiles/workflows and accepted branch, and to reconcile all-green lane PRs. MAIN remains untouched. Umbrella legality PR #103 must NOT merge into MAIN without explicit permission. No source-save/staged-Pokémon writes, auto-fixes, Gen V, Master Vault or Product UI/touch.
Each session MUST be a MAJOR practical tranche, not 1–2 microfixes: complete a coherent set of ready catalog IDs with actual source-backed work, original-game-specific evidence and new native positive/negative/Unknown tests. Run exact-head full host, focused RSE and ASan/UBSan CI. Document exact source refs, commits, coverage and missing original-stage fields, using one meaningful lane PR per major tranche. Do NOT self-merge.
If evidence is missing, document exact blockers in issue #290, ask L5 for source packet and work on the NEXT ready task in the SAME session. Unknown/unverified history stays Incomplete; never turn a local contradictory candidate or bounded search into global Invalid. Send the main coordination chat the PR URL, head SHA, all CI gates, catalog IDs and merge hazards when ready.
```

## Coordinator integration protocol
Never ask all five chats to edit the same central source. Each produces independent reviewable PRs; coordinator selectively rebases nothing backward, reconciles with true forward-only merge commits if branches have diverged, runs exact combined-head host/RSE/sanitizers, and advances isolated accepted ref with expected SHA. A separate QA/research chat cannot certify implementation as complete without source and native tests.
