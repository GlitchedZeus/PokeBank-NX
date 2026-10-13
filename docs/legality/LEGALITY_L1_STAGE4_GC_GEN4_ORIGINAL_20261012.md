# L1 MAJOR Stage 4 — GameCube / Gen IV original encounter and native record evidence

**Date:** 2026-10-12. **Branch:** `feature/legality-l1-encounters-rng-20261012`, existing draft PR #293, no main/accepted/PR #298/production Legality.cpp/shared CI edits. All local contradictions apply **only to a separately witnessed original source**. A detached modern PK3/PK4 does not prove game identity, encounter method, a wild-origin transcript, or global Invalid.

## Source provenance and independent mechanics

- HGSS original Headbutt tree/Trainer ID encounter-group assignment, including 2/3/4/5-or-more designated trees, A/B/empty outcomes, Special C: [Bulbapedia reversed HGSS table](https://bulbapedia.bulbagarden.net/wiki/Headbutt_(move)/Wild_Pok%C3%A9mon), [reverse-engineered original HGSS NARC Headbutt tree file specification](https://projectpokemon.org/home/forums/topic/41693-hgss-encounter-file-specification/). Group depends on `(SID16 << 16 | TID16) % 10`, original *physical* tree list and index. Secret/Special C trees are never mistaken for regular trees. Independent full-ID example (TID 19410, SID 21243): normal first tree in 2/3/4/5-tree areas maps respectively **A/B/B/B**.
- D/P/Pt saved Honey Tree encounter state: [Project Pokémon original Gen IV Honey Tree reverse-engineering](https://projectpokemon.org/home/docs/gen-4/honey-tree-r22/). Twenty-one physical trees, original 1440-minute slather countdown, appearance only when time remaining at most 1080 and greater than zero, original group and six-row selection, plus trainer-dependent Munchlax tree index. Full-TID algorithm and 21-index met-location multiset mirror pre-existing `Gen4WildEncounter.h`, including original overlap adjustment bug. Distinct physical trees sharing the same met location are not conflated.
- J/K native forward nature-stage replay: independent original LCRNG arithmetic, `Gen4NatureReversalReplay.h` (J: `high16 / 0xA3E`; K: `high16 % 25`) and `Gen4WildRngCorrelation.h` (J and K Synchronize proc). No-lead and separately witnessed successful Synchronize are distinguished. A bounded replay miss is **Unknown**, never proof that all histories fail. PokeRadar chain-shiny nature/PID generation, mixed Synchronize success/failure, Safari lead/rejection routes are intentionally *not* certified.
- GameCube native PK3 source bridge: `Gen3GameCubeOriginalFixedHistory.h` and `Gen3GameCubeFixedEncounterEvidence.h` separate original Colosseum vs XD and fixed/gift/purified vs GBA-recipient scene. Native PK3 origin game field `(origins >> 7) & 15` at 0x46 is **15 for both** original GameCube games, and does not identify which title generated the creature. The original scene, purification and recipient stage must be witnessed separately.
- Native format tests cross-check the actual encrypted PK3 **80/100-byte** and PK4 **136/236-byte** record layouts, 24 PK3 shuffled subblock positions and all 32 PK4 block-shuffle selectors, PK3 PID⊕OT key and 16-bit word-sum, and PK4 checksum-keyed/party PID-keyed LCRNG XOR. Reference algorithms: production `Encryption3FRLG.cpp`, `Encryption4.cpp`, `Encryption.h` and [Generation IV native record layout](https://projectpokemon.org/home/docs/gen-4/pkm-structure-r65/). All native source records remain immutable; corrupted ciphertext is rejected by checksums.

## Implemented L1 files

| Purpose | Files |
|---|---|
| Original HGSS tree identity and group witness | `include/Legality/Gen4HgssOriginalTreeStage.h`, `tests/test_legality_l1_stage4_hgss_tree.cpp` |
| Original Sinnoh Honey Tree acquisition stage | `include/Legality/Gen4SinnohOriginalHoneyHistory.h`, `tests/test_legality_l1_stage4_honey.cpp` |
| Original Method J/K nature/PID forward stage | `include/Legality/Gen4OriginalJkNatureStage.h`, `tests/test_legality_l1_stage4_method_jk.cpp` |
| Original GameCube game/scene/native-origin bridge | `include/Legality/Gen3GameCubeOriginalNativeStage.h` |
| Actual PK3/PK4 native encrypted test fixtures | `tests/l1_native_encrypted_formats.h`, `tests/test_legality_l1_stage4_native_pk3_pk4.cpp` |
| Test registration, L1 only | `tests/test_legality_l1_run.sh` adds four suites; now **25 total suites × 2 profiles = 50 full L1 runs required** |

These deliberately do not provide production analyzer verdicts. All four source headers are self-contained, readable original-stage validators rather than changes to `src/Legality/Legality.cpp`.

## Executed normal and sanitizer tests

Actual **GCC 14.2** C++20 locally compiled and executed on the **exact bytes** of the nine Stage 4 helper/test files. For each suite, separately:

- Normal: `g++ -std=c++20 -Iinclude -Itests -O2 -Wall -Wextra -Wpedantic -Werror`.
- Sanitizers: `g++ -std=c++20 -Iinclude -Itests -O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer` with `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

**4/4 compiled and ran normally; 4/4 compiled and ran with both sanitizers.** No failures after correcting an initially mistaken expected HGSS Headbutt table outcome against the independent published example.

| Suite | Positive source/stage cases | Candidate-local or selected-source negatives | Unknown controls |
|---|---:|---:|---:|
| HGSS trees | 8,272 | 8,272 contradictions + 2,068 independently attested empty-tree outcomes | 62,040 |
| Sinnoh Honey | 5,376 | 13,809 | 44,027 |
| Gen IV J/K | 5,120 (plus 2,472 overlapping Sync-positive controls) | 5,120 | 28,248 |
| Encrypted PK3/PK4 | 96 PK3 + 576 PK4 | 864 | 768 |

**The exact nine Git blob hashes, verified against local `git hash-object`:**

```
d50d3437a081343dfec2cd3a91ba7f586beca4be include/Legality/Gen3GameCubeOriginalNativeStage.h
9c767ede360d503baa5831db3ce9c6eff04c9441 include/Legality/Gen4HgssOriginalTreeStage.h
ba76f9183acedd90ad97b759ee9c77601a8b22dd include/Legality/Gen4OriginalJkNatureStage.h
3b82f11c6b983768fee53240c90bd71c190a33ed include/Legality/Gen4SinnohOriginalHoneyHistory.h
be52d08176bafe2d73067046f0bb6b807ef0b047 tests/l1_native_encrypted_formats.h
f8f9c03672faae6d93b6cc871ed0f526f6627e0f tests/test_legality_l1_stage4_hgss_tree.cpp
2e031859fa3f0cfc42d9b6218e7fa34788dc0bcf tests/test_legality_l1_stage4_honey.cpp
220d53506d41defd1bbee870aad403c9e666fdad tests/test_legality_l1_stage4_method_jk.cpp
b0338e26d2d0b91a46a9bcb31db2faa8171e4fb8 tests/test_legality_l1_stage4_native_pk3_pk4.cpp
```

**Important verification boundary:** The encrypted format tests use a **self-contained, test-only independent codec** producing real ciphertext, CRC/checksum corruption and read-only roundtrips; they **do not execute the actual production native Pokemon4ReadOnly or Pokemon3FRLG C++ parser on this branch head**. The existing L1 HGSS special test exercises production PK4 but was **not re-executed at this exact head** because a complete repository checkout could not be obtained in this isolated local environment. Likewise the complete 25-suite dual-profile runner, the shared GitHub host/RSE tests, GameCube native retail archives, and integration coordinator head all remain separate mandatory gates.

## Explicit remaining source limitations / Stage 5 handoff

- Headbutt group selection still needs authenticated original physical tree list/coordinate, exact special-C tree origin, corresponding HGSS species-slot and Method K match joined with native capture source. A known empty physical tree invalidates only that tree hypothesis.
- Honey timer can be checked against independently archived original save state; a present-day saved countdown or transferred Pokémon does not prove a capture encounter. Complete slather RNG, 6-row species matches and multiple eligible Munchlax tree possibilities remain outstanding.
- Radar chain-shiny transcript, HGSS Safari blocks/day multipliers, detailed Contest retry stages, PokeWalker/world transfers, GameCube shadow-specific lock-chain and original archived PK3/PK4 save provenance are not exhaustively implemented; these stay Unknown.
- The source headers are intentionally **not connected to production legality verdicts**, and existing 21 L1 tests and full repository gates must be rerun by the integration coordinator on an exact accepted SHA. The fifth/final L1 development stage should perform a candid candidate-universe audit, safe informational adapter(s) only where actually justified, isolated full regression completion and exact-SHA coordinator handoff. Do not promote a local contradiction or missing encounter transcript into global Invalid.
