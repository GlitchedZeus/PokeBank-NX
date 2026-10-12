# L3 — joint move-acquisition and historical-transfer validation handoff

**Purpose.** One Pokemon's four stored moves can each be present in the original game's acquisition tables while their **joint historical sequence** is impossible under a proposed, fully witnessed path. This document separates a *conditional candidate-history contradiction* from any global legality result. Preserve the original save; no mutation, automatic repair, Game V work, or fabricated historical facts.

**Source freeze at this review:** accepted legality `d29eada5c087e201e00c749703f1a2de3ddfadeb`; pre-session L3 `0b30c020f3cad1d505e175d419a52c9b786ba4d3`; integration [PR #298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298) at `c91b53e84ef75669049d159616894ec68cd4b0dd` when compared. The **current** L3 head is the head displayed in [PR #296](https://github.com/GlitchedZeus/PokeBank-NX/pull/296). Coordinator must re-fetch both heads immediately before integrating; never overwrite the newer coordinator branch.

## Concrete gap closed by L3-only source-scoped proof

Original `Gen14CandidateMoveChronology::evaluate(Path)` checked move-slot mutation and a few sampled level-up offers. Because the caller could claim `fullChronologyVerified=true` without listing **every** mandatory level-up offer, it accepted some internally consistent four-slot reconstructions that were not complete source histories. The broader `Gen3JointMoveEvidence` / `Gen4JointMoveEvidence` source-pool checks also cannot establish coexistence or teaching order.

New `include/Legality/Gen14CandidateMoveJointAcquisition.h` validates a **single unchanged-species, unchanged-GBA-cartridge history** against:

1. The exact pinned original Ruby/Sapphire/Emerald/FireRed/LeafGreen level-up offers, including mandatory automatic insertions into non-full movesets, independently witnessed decline/replacement/already-known outcomes when full, and exact source ordering of two offers at one level.
2. The chronology's witnessed action times and resultant four move slots. At a source level, a TM/HM cannot be ordered before an as-yet-unprocessed original level-up offer.
3. One independent, correct original numbered TM/HM item witness for each machine-teaching step, checked by the existing L3 `Gen14CandidateMoveMachineEvidence`; no missing, duplicated, or disconnected item witness is treated as complete.
4. An *explicitly attested* original same-species/game interval, verified original four-slot snapshot, complete level-up decisions, source identity and player choices. A PK3/PK4 move array **cannot** supply these lost facts.

**Demonstrable synthetic counterexample:** Ruby Bulbasaur level 5 with original moves {Tackle 33, Growl 45} purportedly reaches level 10 with {Tackle 33, Growl 45, Toxic 92, Hidden Power 237} from TM06 and TM10. Both TMs are independently learnable, and the generic four-slot replay returns `CandidateConsistent`. But source Ruby offers Leech Seed 73 at level 7 and Vine Whip 22 at level 10, and both would automatically occupy the two empty slots. If every stage and action is independently proven and the proposed full sequence omits those offers, the new **specific hypothetical candidate** returns `CandidateContradicted`. Other histories, including pre-existing egg moves, service visits or future game trades, have not been exhaustively excluded.

**Valid coexistence witness:** same source Bulbasaur at verified level 40 with four occupied slots {33,45,73,22} may teach TM06 Toxic 92, TM10 Hidden Power 237 and HM01 Cut 15 in the correct verified chronological order, then learn level-46 SolarBeam 76 into its fourth slot. The stated full candidate ends {92,237,15,76} and the joint source check is `CandidateConsistent`; treating the HM teaching as occurring at level 46 *before* the outstanding level-up offer contradicts the stated ordered history. It is still merely a verified **conditional route**, not proof the real Pokémon took it.

Added positive, negative, and Unknown regressions to **existing** `tests/test_legality_l3_move_chronology.cpp`; no eighth native test program or unrelated lane files. Header-level compile-time positive/negative/Unknown smokes are additionally reachable through owned `Gen14CandidateMoveHistory.h`, already included by the accepted host test, so the new algorithm is compiled using the original full Emerald data even before coordinator registers new standalone changes.

## Important legal alternatives and deliberately unresolved cases

- **Evolution timing:** target-species evolution-level offers, delayed evolution, cancellation, pre-evolution inheritance and form differences are independently modeled in `Gen14CandidateMoveGbaEvolutionStages.h`. The new uninterrupted-species joint checker returns **Unresolved** for a path that needs evolution or an unattested species stage; it must not reject that alternative.
- **Trading:** verified game visits have their own generic four-slot witness and source-specific TM/level-up methods. The new checker returns **Unresolved** for a cross-game history, rather than borrowing another game's table or inferring that trading did not happen.
- **Relearning/tutors:** source-specific `Gen14CandidateMoveGbaRelearner.h` can conditionally reconstruct the menu and four-slot choice; the new joint checker deliberately does not consume a Reminder or Tutor step until independently witnessed service/interleaving semantics are supplied. Those alternatives remain **Unresolved**, not excluded.
- **Egg inheritance/events:** the starting move array is an externally attested stage, not an original egg/parent or event archive. An independently witnessed move already present may legitimately trigger an `AlreadyKnown` offer. Without a true original native initial snapshot, parent pairing or exact event record, return **Unresolved**, not a negative acquisition judgment.
- **Shedinja:** `Gen14CandidateMoveShedinjaCopyStage.h` provides the separate one-copy origin route; never treat Shedinja as a continuing Ninjask. Delayed level-38 Agility and level-45 Baton Pass correspond to distinct candidate creation times; future learning, event or transfer alternatives are still open.
- **Pal Park:** `Gen14CandidateMovePalParkJointStage.h` and accepted `Gen34PalParkSnapshotTransition.h` check exact *observed* source/destination and pre/post snapshots. Old Gen III HMs cannot cross unchanged, but may be removed pre-transfer or learned anew in Gen IV with independently verified method evidence. The new **single GBA-stage** source checker returns Unknown on Pal Park edges; never use its narrower contradiction as a global postmigration finding.
- **Candidate sets:** even when all known hypothetical paths contradict, a complete original alternative universe has not been established. The accepted `Gen14CandidateMoveHistory` and `Gen14CandidateSetCoverage` retain source-missing routes as Incomplete. A contradicted local path **NEVER** directly maps to `Severity::Invalid`.

## Exact production call-edge proposal (coordinator-owned code; NOT implemented by L3)

Based on accepted `src/Legality/Legality.cpp` at `d29eada5`, the safe future entry points are:

| Current production edge | Safe optional extra information | Mandatory prerequisites / prohibited action |
|---|---|---|
| `Legality::analyze`, lines **1690–1740**: exact Gen I/II source move pool and source-free native joint pool | Keep existing `Gen12JointMoveEvidence` **informational lower bound**; no new L3 Gen III method claims for Gen I/II | Gen I/II original Time Capsule history, version-specific source lineage and egg/event alternatives still not reconstructed; never infer global impossibility from pool mask |
| Lines **1742–1767**: `hasExactGen3Source`, `Gen3::Learnset::classify`, warnings and move array | After immutable `sourceMoves` construction, optional source-specific **positive** `Gen3GbaVersionedLevelUpTimeline::offerAt` / `Gen14CandidateMoveGbaRelearner` source-menu *possibility* (Info); if a **separately authenticated original history witness** is provided, invoke `Gen14CandidateMoveJointAcquisition::evaluate` | Do NOT construct `Path` by setting `fullChronologyVerified=true` from `pk.move(i)`, origin byte, edit history, broad Learnset pool or current save alone. An unsuccessful hypothetical path is at most Info (“that proposed history disagrees”), NEVER Invalid; absent/wider alternatives -> Incomplete |
| Lines **1768–1787**: exact Gen IV `Gen4MoveCompatibility::classify` | Continue pure Info/Warning compatibility, optionally `Gen4JointMoveEvidence` bound, distinguishing different retail pools from actual witnessed travel | No complete D/P/Pt/HG/SS level-up, TM/HM, egg, event and tutor/reminder **historical sequence proof** in L3. Do not reuse Gen III mechanism to reject Gen IV-native routes |
| Lines **1091–1145** and **1793–1844**: structurally verified PK4 Pal Park markers/ball fields and compatibility | Existing `Gen34PalParkMoves::verifiedTransferFields`, `Gen34PalParkHMHistory` remain *native informational* constraints. For **external independently matched pre-transfer PK3 AND immediate-arrival PK4** source stages, call `Gen34PalParkSnapshotTransition`, then `Gen14CandidateMovePalParkJointStage` before optional positive Info | A lone PK4 transfer marker does **not** contain original GBA outbound moves. Never insert an invented same-individual snapshot, assert impossible HM survival from the final PK4 move array, or treat postarrival re-learning as excluded |
| Coordinator-owned reporting/coverage mapping | CandidateConsistent → Info “a provided, independently witnessed route is internally consistent”; CandidateContradicted → Info “this documented route fails source chronology, other origins unresolved”; Unresolved/Unsupported → Incomplete with missing-field reasons | Never promote a *candidate-only* contradiction to `Severity::Invalid`, auto-correct a Pokémon, or write to source saves |

Suggested **separate read-only caller contract**, owned by the coordinator, *not* inferred from a Pokémon struct:

```cpp
// Conceptual only: not a production patch and not an automatic origin claim.
struct ExternalHistoryEvidence {
    bool independentlyMatchedOriginalPokemon = false;
    bool originalStageSnapshotVerified = false;
    bool completeLevelUpDecisionsVerified = false;
    bool sameCartridgeAndSpeciesThroughoutStageVerified = false;
    // Exact source-game visits, level/move events and numbered item proof
    // must be supplied by independent original save/source data.
};
// If missing any proof, report Incomplete. Never construct a "verified"
// history from four move IDs or "same game origin" alone.
```

**What must remain standalone:** candidate-history enumeration; speculative trade, egg/event parent, GameCube origin and Shedinja sibling histories; unverified original save reconstruction; sampled Bulbasaur TM/HM item witnesses for non-Bulbasaur species; Gen IV native method-source IDs without genuine pinned original records. None are safe independent negative production validators.

## Exact-source validation and release gates

- Frozen original source revisions: `pret/pokeruby@63a8cbf0016b351a4e68f7036fa0b77e23d2f2c1`, `pret/pokeemerald@5eff78649e7170a877b961ef0b3da13b81a16038`, `pret/pokefirered@c75f352304d529f6ba92d4f74b9cf8b5c3810788`; accepted full Emerald packed table Git blob `3e53c1c951876a26cace5dde060cc9ff2c35d0a7`.
- Accepted Emerald original source: **386** species / **3,947** packed entries. Ruby/Sapphire and FireRed/LeafGreen complete same-species level-up tables are independently pinned only for **12 of 386 species**; all other **374** are Unknown. The 19 GBA TM/HM mappings are pinned specifically for Bulbasaur, not all species.
- **Completed full-data baseline at PR #298:** GitHub Actions [run #38287166355](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38287166355), job “Legality lane L3 (native + sanitizers),” executed **seven native tests plus seven ASan/UBSan** using `L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7` and ended `L3 PASS`. This validates the integrated pre-session L3 head `0b30c020` (the L3 head included in PR #298), **not** the new joint-history changes after that head.
- **New candidate-history changes need revalidation** after the coordinator incorporates the newer PR #296 head into PR #298. On an exact GitHub checkout with the accepted real data, run `L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 bash tests/test_legality_l3_run_all.sh` and confirm all seven native and sanitizer programs, full Emerald versioned-offer test and 386-species Relearner sweep. Run normal host tests, RSE host/sanitizer and whole host sanitizer again.
- The L3-specific new header has additional compile-time `static_assert` guards included by already-registered `Gen14CandidateMoveHistory.h`; GitHub Host Tests on the **new** exact L3 head can verify its new algorithm compiles even before the all-seven modified standalone suite is registered.
- Check current integration PR #298 and L3 PR #296 ancestry **both directions**, preserve all newer integration commits, and have the coordinator accept/merge only when the *combined exact head* is green. No self-merge, backward rebase, force push, CI ownership changes or any installed-game/source save writes.

**Open factual blockers:** unknown original player teaching choices, unrecorded original moves and visited cartridges, Gen IV versioned native teaching evidence, additional legal egg/event/game trade paths, direct native source-paired fixtures, licensing/redistribution review and exhaustive candidate history enumeration. Keep unsupported histories **Incomplete**. L3 M06/M07/M08/M11/M12/M13 remain **partial**; accepted M17 Pal Park HM boundary remains intact.
