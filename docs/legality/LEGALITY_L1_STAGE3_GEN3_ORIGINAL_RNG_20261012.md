# PokeBank NX L1 — Stage 3 Gen III original encounter/RNG engineering (2026-10-12)

## Exact pinned independent retail source references

- [pret/pokeruby@5784633ce4ef7ade1a7f2d2d0c288e3d5e6cdd7f](https://github.com/pret/pokeruby/tree/5784633ce4ef7ade1a7f2d2d0c288e3d5e6cdd7f): `src/data/wild_encounters.json`, `src/wild_encounter.c` and `src/roamer.c`. Distinct original Ruby and Sapphire source entries.
- [pret/pokeemerald@731ad5bfd6e6f265508d0efcca0ba42f9dcf5881](https://github.com/pret/pokeemerald/tree/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881): `src/data/wild_encounters.json`, `src/random.c`, `src/wild_encounter.c`, `src/roamer.c`. Emerald's `PickWildMonNature` has Synchronize success/failure and optional Safari Pokéblock route; `ChooseWildMonLevel` has Pressure/Hustle/Vital Spirit maximum-level reroll.
- [pret/pokefirered@037335f4c725d7c9aecdac87066f2002b4bd7e14](https://github.com/pret/pokefirered/tree/037335f4c725d7c9aecdac87066f2002b4bd7e14): `src/data/wild_encounters.json`, `src/random.c`, `src/wild_encounter.c`, `src/roamer.c`. Original FireRed/LeafGreen source tables are separately versioned; the original `GenerateWildMon` no-lead nature path is not automatically Emerald Synchronize.
- This tranche independently implements the original GBA LCRNG `state = state * 1103515245 + 24691` with 32-bit wrap, sampled upper 16 bits. It does **not** copy original engine code or claim arbitrary current-PID reversal is a surviving original RNG transcript.

## Engineering delivered: source catalogs, lead/RNG and roamer

1. **`Gen3OriginalWildSourceStage.h`**: 33 indexed original game/map/method groups, **306 physical grass/surf/fishing source rows** from original R/S/E Routes 101, 110 and 119 and FRLG Routes 1, 12 and 19. Packed literal data retains exact version, source species, min/max level and physical slot; all 306 source rows were independently decoded and compared against the three pinned original JSON files: **306/306 match, zero mismatches**. Source-level positives are deliberately `SourceTemplateCompatibleHistoryUnverified`. A real original save, map, slot, game, level, and rod are separate provenance inputs. Incorrect *independently attested* source identity is only a selected-candidate contradiction; missing witness, foreign game and unmodeled map remain Unknown, not global Invalid. Source scope is intentionally bounded: this is **not all original Gen III maps**; Rock Smash/Feebas/Altered Cave/event/egg/transfer special histories must not be excluded by its omissions.
2. **`Gen3OriginalWildRngWitness.h`**: original slot selection (12 grass, five surf/rock, 2/3/5 rod-group slots), original bounded level roll, Emerald-only Pressure/Hustle/Vital Spirit secondary level roll, Emerald Synchronize nature-roll success/failure, and ordinary R/S/E/FRLG no-lead nature selection. Independent PRE-SLOT and PRE-NATURE original RNG states are separately required because generation routines can consume other RNG calls between them. No guessed contiguous capture transcript, no forced origin from detached native PID. Emerald Safari Pokéblock or unsupported lead path remains Unknown. Every result is a **stage only**, not matched PID/IV, encounter authenticity, or universal negative.
3. **`Gen3OriginalRoamerFrameWitness.h`**: independently replay 32-bit original pre-PID frame through both PID halves and IV words. Ruby/Sapphire/FireRed/LeafGreen truncated low-eight-IV storage is deliberately distinct from Emerald's full two-word IV storage. Requires exact original version, activated roamer, independently witnessed Emerald TV choice or FRLG starter, actual original species/IVs and original pre-PID frame seed; malformed/unknown original fields remain Unknown. The original route and activation witnesses must be validated by the existing L1 `Gen3OriginalRoamerStage.h` before integration: this standalone frame is never a proof of a complete original roaming capture.

## Real exact-source executable validation

The following three C++20 native tests were **compiled and executed twice** locally under GCC 14.2:
- Host: `g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude`.
- Sanitizers: `-O1 -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer`, `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1`, `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

| Exact committed suite | Source/stage positives | Candidate-local negatives | Missing/foreign Unknown | Normal | ASan/UBSan |
| --- | ---: | ---: | ---: | --- | --- |
| `test_legality_l1_gen3_wild_catalog.cpp` | 306 | 1,018 | 2,242 | PASS | PASS |
| `test_legality_l1_gen3_wild_rng.cpp` | 5,252 | 13,696 | 21,252 | PASS | PASS |
| `test_legality_l1_gen3_roamer_frame.cpp` | 5,120 | 15,360 | 20,480 | PASS | PASS |
| **Total** | **10,678** | **30,074** | **43,974** | **3/3** | **3/3** |

The wild RNG regression also runs 1,024 separate Emerald Synchronize nature stage inputs and witnesses **both success and failure**, with missing-history and altered-nature adversarial controls. The roamer suite enumerates 1,024 independently forward-computed original seeds **per exact GBA game**. These are **synthetic source-stage reference arithmetic fixtures**, not historic retail specimens or authentic original RNG captures.

Every committed file's Git blob identity was checked against local `git hash-object` output **after** real tests, including the expanded Synchronize suite. Exact six tested Git blobs:
- `Gen3OriginalRoamerFrameWitness.h`: `d9d8e6d3bfeffe2b1d872162c32d31329021a372`
- `Gen3OriginalWildRngWitness.h`: `55f4361f4cd2a16e574878a95b7db5c6cc0c1980`
- `Gen3OriginalWildSourceStage.h`: `bfc09aaaecf6ff18b7cfb6f419569e9f1d4b14f4`
- `test_legality_l1_gen3_roamer_frame.cpp`: `b8b62731a020fcec91dd7234d40fcab8152539d7`
- `test_legality_l1_gen3_wild_catalog.cpp`: `2f1b711c77c9ba917a949c66ad1eb2c45d32fe14`
- `test_legality_l1_gen3_wild_rng.cpp`: `54a0ca3b067d904a6be9bb55283b6414516c1b87`

The isolated L1 `tests/test_legality_l1_run.sh` now registers **21 suites × 2 profiles = 42 expected native runs**. **Only the six newly added native runs were executed at the latest L1 author head in this environment.** Full project Git checkout/compilation is blocked locally by GitHub DNS access restrictions; the previous 18 L1 suites and whole-project host/RSE/sanitizer/combined-lane tests were **not** executed here, and cannot be reported green on this SHA. The standard GitHub host workflow is a separate job and does **not** by itself run all L1 suites.

## Residual blockers and exact integration discipline

- Most original version/map source rows remain unevaluated; all altered-cave, Feebas tile, Weather Institute, special outbreak, swarm, event/gift, egg/transfer and non-NoLead histories remain candidate-universe Unknown. Indexed local failure cannot declare global Invalid.
- Source-slot existence and mathematical RNG-stage match are **not** original-save provenance, a continuous encounter transcript, or native encrypted PK3 checksum validation. Gen III original PK3/transfer/safari stage witness, actual original RNG lead route and competing encounter families require coordinator L5 review.
- Advance Stage 4 only after coordinator receives this exact stage report. No production `src/Legality/Legality.cpp`, shared Makefile/CI, accepted legality, PR #298, other lanes, or MAIN edits; PR #293 stays draft/open. Do not force-push, rebase backward, reset, merge or throw away concurrent work.
