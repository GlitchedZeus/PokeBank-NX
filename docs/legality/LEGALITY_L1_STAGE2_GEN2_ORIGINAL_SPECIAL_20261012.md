# L1 Stage 2 — Gen II original wild-method and source-history expansion (2026-10-12)

## Ownership and safety

Only isolated L1 original-source evidence headers, executable L1 test units, this source review and `tests/test_legality_l1_run.sh` were changed. No edits to central `src/Legality/Legality.cpp`, accepted legality, PR #298, other lanes, common CI, Makefiles or MAIN. PR #293 stays draft. All helpers evaluate a *separately attested original encounter source* and **never produce global Invalid**.

## Independently verified retail disassembly sources

- Gold/Silver original-source disassembly: [pret/pokegold at `ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8`](https://github.com/pret/pokegold/tree/ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8).
- Crystal original-source disassembly: [pret/pokecrystal at `3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6`](https://github.com/pret/pokecrystal/tree/3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6).
- Directly checked exact `data/wild/fish.asm`, `treemon_maps.asm`, `treemons.asm`, `bug_contest_mons.asm`, `swarm_grass.asm`, `swarm_water.asm`, and `roammon_maps.asm` in **both** pinned repositories where present.
- Important observed source differences: Gold/Silver Route 29 and Route 43 Headbutt use Canyon, while Crystal uses Route and Lake; Gold/Silver Forest lists depend on version; Crystal does not provide the Gold/Silver Mount Mortar Marill surfing swarm. Original Route 38 Gold/Silver Snubbull swarms have different species choices, including night differences. Bug Contest terminator `-1,VENOMOTH,30,40` is **not** an encounter slot.

## Four substantial additions

1. `Gen2OriginalFishingStage.h`: **all 13 fish groups** and **145 physical outcome rows** across Old/Good/Super Rod including Day/Night Corsola–Staryu split, other `time_group` references, swarm Qwilfish/Remoraid and no-swarm variation. Group, rod, physical row, historic day/night, swarm, species and level have independently attested input flags. Matching a row is `SourceSlotCompatibleWorldAndRngUnverified`, **not** fishing transcript or map-source proof. The map→fishing-group original-save state and actual generated encounter RNG remain unverified.
2. `Gen2OriginalTreeContestStage.h`: **72 versioned Headbutt physical rows**, **six original game/map→tree-set mappings**, and **10 selectable Bug Contest species rows** with original level intervals. The original tree coordinate/trainer-derived rarity and eligibility must be independently witnessed. Missing event session or tree rare/common status is Unknown.
3. `Gen2OriginalSwarmStage.h`: **150 original source rows**, providing **216 game-qualified witnesses**: Gold/Silver Yanma, Snubbull, Dunsparce and Marill grass swarms, Crystal Yanma and Dunsparce grass swarms, plus Gold/Silver-only Marill surfing. Morning/day/night physical slot differences are preserved. A present-day swarm event is not original-state evidence.
4. `Gen2OriginalRoamerRouteStage.h`: independently verified graph of **16 original Johto route nodes and 40 directed transitions**. Route 40/41 are absent from the roaming topology. An independent actual-original transition source/destination is required. A contradiction is only against a *specific witnessed transition*, never an arbitrary roaming encounter's origin.

All missing witness flags and out-of-domain source inputs return Unknown. Independently witnessed slot mismatches remain **candidate-local only** and cannot exclude alternate wild/egg/event/trade/roamer/transferred histories.

## Executed native tests — verified committed bytes

All four new self-contained C++20 test source files and their headers were compiled/executed locally with **GCC 14.2**:
- Normal: `g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude`.
- Sanitized: `g++ -std=c++20 -O1 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -Iinclude`, with `ASAN_OPTIONS=halt_on_error=1:detect_leaks=0` and `UBSAN_OPTIONS=halt_on_error=1`.

| New native suite | Source positives | Candidate-local negatives | Unknown | Foreign game checks |
| --- | ---: | ---: | ---: | ---: |
| `test_legality_l1_gen2_fishing.cpp` | 435 | 1416 | 3591 | — |
| `test_legality_l1_gen2_tree_contest.cpp` | 168 | 522 | 1170 | Additional controls |
| `test_legality_l1_gen2_swarms.cpp` | 216 | 1074 | 2160 | 234 |
| `test_legality_l1_gen2_roamer_routes.cpp` | 120 | 120 | 720 | — |
| **TOTAL** | **939** | **3132** | **7641** | **234** |

**All eight binaries PASS** (four normal + four ASan/UBSan); every new header and test was subsequently fetched from GitHub and its Git blob SHA matched `git hash-object` on the locally executed exact same files. These are explicitly *synthetic original-stage witnesses* sourced from disassembled tables, **not** archived original caught Pokémon.

Git blob SHA identity:
- `Gen2OriginalFishingStage.h` `2816754ea16537b0bca7ab7feb03fe754c688c81`
- `Gen2OriginalTreeContestStage.h` `ef6e08baac0d1453688043ad51e1b59df7b2b5ad`
- `Gen2OriginalSwarmStage.h` `8589543ca1786fc64b45daa9a4a567f41074340e`
- `Gen2OriginalRoamerRouteStage.h` `5625f2e9ada2c1c0d373041a46ac758a280cfc64`
- `test_legality_l1_gen2_fishing.cpp` `df3a2d13e36aaaba71245a06b16684ed2be20ea7`
- `test_legality_l1_gen2_tree_contest.cpp` `f03b4caaa6ed93bb85161ffc784669557375fa4d`
- `test_legality_l1_gen2_swarms.cpp` `0589dfea87f4d575f0087858d18c2ba4d85c5c7c`
- `test_legality_l1_gen2_roamer_routes.cpp` `bd7a6ea0826430abcfbf4a6882a098db65a4d9d2`

The exact current L1 standalone runner registers the four new units, for **18 suites in two profiles = 36 required executions**. The eight newly isolated C++ runs above are **not** a full 36/36 green claim. Actual full repository checkout is unavailable locally owing to GitHub DNS/network restrictions; production/native PK2 format integration and cross-lane host/RSE/sanitizer CI remain coordinator gates. The current GitHub standard host workflow is not a replacement for independently running all L1 suites.

## Still blocked or incomplete

- Historical original clock/session, swarm phone-call state, Headbutt coordinate/trainer-dependent tree rarity, fishgroup world ownership and original RNG state cannot be inferred from a detached modern PK2.
- Complete G/S/C maps, complete wild source multiset and generation/event distinctions are not proven exhaustive. Crystal caught-data alone cannot authenticate its historical origin. Current `Gen2IndexedOriginalSourceHistory.h` has separately bounded 9856 wild and 107 static rows from older accepted PKHeX-generated data; these new original retail source witnesses do not replace that dataset.
- No universal false-Invalid hard gate and no native PK2 authenticated archive. No automatic production wiring until original-history-source provenance, original capture RNG, L5 candidate-universe completeness and full combined exact-head tests are approved.
- Keep the draft PR unmerged; next assigned round Stage 3 (Gen III advanced RNG) only after this Stage 2 handoff is available to the coordinator.
