# L2 B08 / B11 / B18 — Gen III PID + IV + inherited-move history binding

**Status:** source-backed, read-only **conditional** historical stage check; **DRAFT** development on PR #295. This is not a reconstructed original retail egg, global legality ruling, original-ROM-executed test, completed full project CI or a PKHeX-equivalent breeding-chain verifier.

## Source and accepted components (read before implementation)

- Ruby/Sapphire: `pret/pokeruby@5784633ce4ef7ade1a7f2d2d0c288e3d5e6cdd7f` `src/daycare.c` — original `InheritIVs`, `GetEggMoves`, egg four-slot mutation passes, `_TriggerPendingDaycareEgg`, `SetInitialEggData`. [Pinned C](https://github.com/pret/pokeruby/blob/5784633ce4ef7ade1a7f2d2d0c288e3d5e6cdd7f/src/daycare.c).
- Emerald: `pret/pokeemerald@731ad5bfd6e6f265508d0efcca0ba42f9dcf5881` `src/daycare.c`, including independently attested original physical deposited slots, move-donor roles, Everstone nature choice, per-source IV selection. [Pinned C](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/daycare.c).
- FireRed/LeafGreen: `pret/pokefirered@037335f4c725d7c9aecdac87066f2002b4bd7e14` `src/daycare.c`, including the nonexecuting/commented FRLG Everstone block. [Pinned C](https://github.com/pret/pokefirered/blob/037335f4c725d7c9aecdac87066f2002b4bd7e14/src/daycare.c).
- Exact original stage and pinned file digests: [B07 PID record](L2_GEN3_EGG_PID_ORIGINAL_HISTORY.md); [Gen III IV alternative record](L2_GEN3_IV_ALTERNATIVES.md). All source code used solely as behavioral reference; no original retail source or PKHeX implementation copied into NX.

**Accepted, unmodified NX building blocks:**
`Gen34OrdinaryEggMoveConstruction.h`, `Gen34EggMoveEvidence.h`, `Gen34EggMoveData.inc`, `Gen34SameParentEggHistory.h`, `Gen34BreedingPairEligibility.h`, `Gen3JointEggCandidateEvidence.h`, `Gen3EggIVInheritanceStage.h`. Previous work already enforces one four-move-slot father hypothesis and original source-game six LCRNG IV draws; this tranche composes them with L2's B07 original source-game PID creation without refactoring the accepted logic.

## What this batch adds

1. `include/Legality/Gen3JointEggPIDIVMoveHistory.h` joins accepted SAME-FATHER move+IV evidence with L2's B07 PID creation/collection evidence for one independently identified **original unhatched egg**. Requires original egg species and move data, original parent snapshots, independently attested identical **physical daycare deposited slots**, original event identity common to all sources, original stage tokens, original pair eligibility proof, original pending/collection RNG evidence, and (Emerald) creation-time held Everstone and VBlank-seeded secondary RNG.
2. **Father/mother are not synonym for physical deposited slot0/slot1.** The helper independently derives/validates role identity from original physical parents. Male + female: male father, female mother; female + Ditto: Ditto move father; male/genderless + Ditto: non-Ditto move father. Invalid or inconsistent role claims return `IncoherentHistoricalDonorRoles`; a move-parent pair cannot borrow some other couple's PID or IV witnesses even when the outputs look equal.
3. B07 Ruby/Sapphire and FRLG original pending 16-bit PID plus collection high-half remains distinct from Emerald's complete PID at pending time with original creation-time Everstone and two independent RNG streams; this helper does not substitute or conflate those stages.
4. Distinct results preserve `UnknownOriginalMoveEvidence`, `UnknownOriginalPIDHistory`, `UnknownOriginalIVHistory`, `MalformedOriginalIVCandidate` and candidate-local Move/IV/PID contradictions. **None of these verdicts imply whole-Pokémon Invalid**, and a detached PK3 does not prove the missing historical stages.
5. `include/Legality/Gen3EggFullHistoryAlternatives.h` checks a bounded **8 original hypothetical histories** for one same observed egg. Distinct route IDs, asserted source-egg identity, original observed species/PID/IV/moves and source game must agree. A verified stage that contradicts source observations excludes one candidate only; a separately attested consistent alternate parent history rescues it. Unverified parent/event history, incomplete proof, or lack of independently exhaustive ordinary-daycare enumeration remains Unknown. Even `AllAttestedOrdinaryDaycareRoutesContradicted` describes **only** supplied ordinary-daycare candidates, never all possible gift/event eggs, chain breeding histories, transfers or whole Pokémon legality.

## New tests and reproducibility

- `tests/test_legality_l2_gen3_joint_pid_iv_moves.cpp`: **256 seeds × 2 physical deposit orders × 5 games = 2,560** synthetic source-stage triples plus **75** independent provenance, source identity, historical father/Ditto role and missing evidence controls. Includes internally contradicted PID/IV/move candidates, malformed IV-source inputs, and missing original parents.
- `tests/test_legality_l2_gen3_pid_iv_moves_alternatives.cpp`: **512 seeds × 5 games = 2,560** candidate sets, local move-TM contradiction, unknown completeness, rescued alternate attested route, malformed/zero/duplicate IDs, all-8 finite capacity, and **65** additional Unknown/provenance controls.
- The local negative history specifically uses `verifiedMachine[0] = true` and father move `15` on Bulbasaur, changing the four-slot result under the accepted constructor without falsifying the original observed move projection. This fixes the nondeterminism of using a merely possible egg move to manufacture a contradiction.
- `tests/test_legality_l2_run_joint_gen3.sh`: lane-owned, opt-in GCC/Clang C++20 strict `-Werror` normal and ASan/UBSan standalone test runner. Requires complete checkout with accepted `Gen34EggMoveData.inc`. It does **not** modify shared Makefile, CI workflows or production legality; set `CXX=clang++` to run the second compiler.

**Truth in validation:** source and test files have been committed; the runner defines an exact-source repeatable test target but had **not** completed an exact-header build with the full accepted generated egg-move table in this session. No claim of all-green newly added L2 tests, complete repository CI or original GBA retail executable oracle. Coordinator's clean checkout and registration of all eleven new L2 tests remains essential. A local mock of the move table is not an original retail table or a substitute for that gate.

## Dependencies and caution

Only L2-owned `include/Legality/Gen3*`, `tests/test_legality_l2_*`, `docs/legality/L2_*` have been touched in this batch. No global legality analyzer, save code, MAIN, shared workflows, Makefile, Gen V, Master Vault or other lane files touched.

Coordination prerequisites: preserve accepted Gen I–IV branch's newer Platinum #283; HGSS sibling PID #284 is separate; only coordinator forward-reconciles L2 into live accepted without reset/rebase backward or force push. L5 independent original GBA original-retail executable and historical native parent+egg witness verification remains needed. Do not convert a local negative candidate into global Illegal or assert absent original-stage data was reconstructed from a stored PK3.
