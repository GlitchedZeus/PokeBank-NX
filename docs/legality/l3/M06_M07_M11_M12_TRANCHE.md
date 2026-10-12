# L3 tranche 1 — source-scoped four-slot chronology (M06 M07 M11 M12; M17 boundary)

Accepted base: d29eada5c087e201e00c749703f1a2de3ddfadeb (the previously read 0dccf2e1 advanced two commits; current branch preserves them). The coordinator owns the master catalog and central analyzer.

## Bounded engineering, not completed parity

- **M06 PARTIAL:** separate Ruby, Sapphire, Emerald, FireRed and LeafGreen original-game identities and source-attested per-game move offers; a 4-slot insertion, replacement or decline applies to ONE explicitly supplied original-species candidate. All-species per-version GBA level-up resources are still missing; an Emerald-only resource must not be silently applied to RS/FRLG.
- **M07 PARTIAL:** exactly matched game and verified method for LevelUp/TM/HM/Tutor/Reminder; Move Deleter and same-generation game visits are explicit, ordered actions. Broad compatibility cannot verify teaching.
- **M11 PARTIAL:** explicitly distinguishes Diamond/Pearl, Platinum and HeartGold/SoulSilver post-transfer game identity. A retained Gen III HM cannot cross the independently verified Pal Park edge; the same move can be independently reacquired later in Gen IV. No full tutor/reminder table imported.
- **M12 PARTIAL:** fixed 32-step 4-slot candidate path replay and eight alternative paths. Inconsistent or unidentified routes return Unresolved unless all bounded candidate histories are explicitly enumerated; never global Invalid.
- **M17 regression:** preserves accepted Pal Park helper/semantics; no edits to the existing accepted Pal Park source.

## Original retail source references and exact facts

| Source identity | Pinned source path and symbol | Independently checked fact |
|---|---|---|
| Ruby/Sapphire | [pokeruby@63a8cbf/src/pokemon_1.c](https://github.com/pret/pokeruby/blob/63a8cbf0016b351a4e68f7036fa0b77e23d2f2c1/src/pokemon_1.c), GiveMoveToBoxMon / MonTryLearningNewMove | ordered moves, four slots, duplicate avoidance |
| Ruby/Sapphire | [pokeruby level_up_learnsets.h](https://github.com/pret/pokeruby/blob/63a8cbf0016b351a4e68f7036fa0b77e23d2f2c1/src/data/pokemon/level_up_learnsets.h), gBulbasaurLevelUpLearnset | level 7 Leech Seed, 10 Vine Whip, 46 SolarBeam |
| Emerald | [pokeemerald@5eff786/src/pokemon.c](https://github.com/pret/pokeemerald/blob/5eff78649e7170a877b961ef0b3da13b81a16038/src/pokemon.c), GiveMoveToBoxMon / MonTryLearningNewMove | four-slot learning; multiple offers per level |
| Emerald | [pokeemerald level_up_learnsets.h](https://github.com/pret/pokeemerald/blob/5eff78649e7170a877b961ef0b3da13b81a16038/src/data/pokemon/level_up_learnsets.h), sBulbasaurLevelUpLearnset | level 7 Leech Seed, 10 Vine Whip |
| FireRed/LeafGreen | [pokefirered@c75f352/src/pokemon.c](https://github.com/pret/pokefirered/blob/c75f352304d529f6ba92d4f74b9cf8b5c3810788/src/pokemon.c), GiveMoveToBoxMon / MonTryLearningNewMove | four-slot learning and replacement |
| FireRed/LeafGreen | [pokefirered level_up_learnsets.h](https://github.com/pret/pokefirered/blob/c75f352304d529f6ba92d4f74b9cf8b5c3810788/src/data/pokemon/level_up_learnsets.h), sBulbasaurLevelUpLearnset | same examined level 7/10 pair |
| Gen I–IV | [research #252 MOVE-001](https://github.com/GlitchedZeus/PokeBank-NX/blob/fc0225e6351af62365f29200300323e38f52c8fe/docs/legality/research/packets/MOVE-001.md) | candidate-level method and history, not pool membership |
| Gen III→IV | [research #252 TRANSFER-001](https://github.com/GlitchedZeus/PokeBank-NX/blob/fc0225e6351af62365f29200300323e38f52c8fe/docs/legality/research/packets/TRANSFER-001.md) | irreversible Gen III HM transfer exclusion / later Gen IV reacquisition |

All test records are original synthetic conditional histories, **not demonstrated actual specimens**. No wholesale third-party tables or source files copied; see master REFERENCE_FREEZE for provenance/license clearance before shipping expanded data.

## New independent native tests and executable commands

New tests/test_legality_l3_move_chronology.cpp contains positive, candidate-contradiction and Unknown controls spanning 5 GBA source identities, moves with open/full slots, game travel, unverified/wrong-version methods, deletion/reminder, level-46 decision, Pal Park later Surf reacquisition, and candidate alternative aggregation.

Local commands:
    g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude tests/test_legality_l3_move_chronology.cpp -o /tmp/l3_test && /tmp/l3_test
    g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude tests/test_legality_l3_move_chronology.cpp -o /tmp/l3_san && ASAN_OPTIONS=detect_leaks=0 /tmp/l3_san

Existing host suite sees compile-time smoke guards through the owned Gen14CandidateMoveHistory.h include. **Coordinator must register the independent L3 test target in shared Makefiles/workflows**; L3 cannot claim CI executed the new standalone tests until then.

## Missing initial/history stage fields

A detached PK3/PK4 does not retain the original four-slot initial snapshot, all offered level-up decisions, games visited, exact tutor/TM/reminder use, trades, or pre-/post-Pal Park snapshots. The new model explicitly requires an original candidate ID, source game and species, verified initial snapshot, same-Pokémon/species continuity, complete step record, exact method-source evidence per step, and transfer witnesses. Missing fields return Unresolved, not global Invalid. Evolution/species ancestry is separate M13 and is not inferred.

**L5 evidence request:** for M06/Q05, independently source, map and license-audit full 386-species Ruby/Sapphire and FireRed/LeafGreen ordered level-up and original initial moves; for M07, versioned TM/HM/tutor/reminder eligibility and exact move acquisition constraints (existing broad pool conflates methods); for M11, DPPt/HGSS game-specific tutor/reminder versions and verified Gen III retained-vs-Gen IV relearned methods; for M12, independently attested exhaustive candidate-space bounds and real native stage fixtures. Do not materialize unsupported provenance from game labels.

Status: coherent **partial** M06/M07/M11/M12, preserving M17. Not a completed PKHeX parity claim; central analyzer, Makefiles, MAIN and accepted branch untouched.
