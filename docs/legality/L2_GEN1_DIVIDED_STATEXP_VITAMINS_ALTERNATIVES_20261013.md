# L2 Gen I — Divided battle Stat Experience, vitamins, and original route alternatives

**Lane:** PR #295 (`feature/legality-l2-breeding-20261012`), draft only; no production analyzer, MAIN, or coordinator changes. Preserve pre-existing Gen II–IX commits.

## Source-pinned retail rules

- **Original source:** `pret/pokered@af519899719f0754965776faac0e836a3b906e6d`.
- `engine/battle/experience.asm`, original Git blob `f9f47f82573771b108df6db4dacecb634a156a24`, routines `DivideExpDataByNumMonsGainingExp` and `GainExperience/.gainStatExpLoop`. Each enemy base-stat byte is integer-divided by the number of eligible recipients **before** awarding one of the five 16-bit Stat Experience values. Retail saturation is **65,535**. For this tranche, the original recipient count must be independently attested in **1–6**, and source must independently attest that Exp. All was absent. Unknown count or Exp. All state means **Unknown**, not a guessed one-recipient award.
- `engine/items/item_effects.asm`, original Git blob `c4ce13de6fb6c7036e94e20b845dd6c5ade8951e`, `ItemUseMedicine/.useVitamin`: HP Up, Protein, Iron, Carbos, Calcium each target one original Stat Experience word and add 10 to its **high byte**, i.e. **+2,560**, leaving its low byte unchanged. A successful use requires the high byte **less than 100**, or Stat Experience below 25,600. At or above that high-byte threshold the vitamin has no effect and is not consumed. An independently attested *successful* use under the blocked condition contradicts the candidate event transcript. Mere absence of item-use evidence remains Unknown.
- Species base-stat table reused directly from PokeBank NX `src/Integration/Gen1/Gen1PersonalData.inc`, source blob `399462b48b08cdd9c6bb3f7d398304147fb19cb1` and PKSM-Core `aa22d7a4f87c0351baf7da5962ba5acd01039a7c`. No modern Pokémon personal-data substitution. PK1 has **five** original Stat Experience fields; no native breeding, modern IV/EV fields, nature, gender, abilities or held items.

## Owned engineering changes

- Extended `Gen1OriginalStatExpBattleHistory.h/.cpp` to accept independently attested original participant counts and distinguish five vitamin events, retaining existing single-recipient witnesses and monotonic original event/source IDs. A battle without attested valid count, original defeated identity, eligible recipients, or known absence of Exp. All remains Unknown. A vitamin without witnessed item/consumption and per-event after snapshot remains Unknown.
- `Gen1OriginalStatExpRouteAlternatives.h`: bounded eight candidate original histories; independently identified route IDs; a separately supported original route remains viable even when another history contradicts or lacks evidence. An exhaustive candidate-local contradiction never automatically becomes global Invalid.
- New `test_legality_l2_gen1_divided_battle_vitamins.cpp`: 151 species × three R/B/Y game namespaces × 1–6 eligible participants × six initial Stat Experience states, saturation and truncation, all five vitamins and 25,599/25,600 cutoff, invalid count and missing source controls, and multi-event chronology.
- New `test_legality_l2_gen1_stat_exp_alternative_routes.cpp`: valid original-history rescue versus contradictory/missing evidence, eight-route capacity boundary, malformed/duplicate route IDs, exhaustive-coverage controls, and candidate-local contradictions.
- Existing `test_legality_l2_gen1_original_stat_exp_battles.cpp` retained and retested. All three are registered in the L2 native runner and exact-head Gen I sanitizer workflow. L2 now has **24 native programs**; final Stage7 gate requires **172** executions (24 × 4 native plus 19 × 4 separate original-source programs).

## Tests actually executed

With the available isolated source mirror and a narrowly scoped PK1 reader/species shim, the following three C++ test programs compiled with **GCC and Clang, normal and ASan/UBSan**, `-std=c++20 -Wall -Wextra -Wpedantic -Werror`:

| Regression | Assertions per build | Passes |
|---|---:|---:|
| Existing Gen I original solo-battle transcript | 137,715 | 4/4 |
| New divided battle/vitamin evidence | 163,890 | 4/4 |
| New alternative-original-route evidence | 393,216 | 4/4 |
| **Total** | **694,821 per configuration** | **12/12; 2,779,284 checks** |

Sanitizers ran with `-fsanitize=address,undefined -fno-omit-frame-pointer`, `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1`. The tests used the project's original five Gen I base-stat columns reconstructed from the pinned source, while a reduced test-only PK1 reader shim substitutes for linking the full original parser. Thus these are **independently compiled regression checks**, **not** a passed full repository build or acceptance run.

## Acceptance and remaining Gen I work

Full `bash tests/test_legality_l2_run_all.sh` and `bash tests/test_legality_l2_stage7_acceptance.sh` **have not executed here**: GitHub cloning is unavailable in this container (DNS resolution). The exact-head GitHub Actions jobs must pass using the real checkout. Unsupported original Exp. All distribution, unverified historical battle/recipient records, Time Capsule migration history, source-authenticated original vitamin inventory/usage logs, and original PKHeX.Core host-oracle differential remain **Unknown/incomplete**. No source save mutation is performed. Existing coordinator PR #298 is not modified.
