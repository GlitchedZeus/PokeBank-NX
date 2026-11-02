# L3 Major Stage 5 — full Gen III history → Pal Park → Gen IV candidate continuity

Date: 2026-10-12. Draft **PR #296** on `feature/legality-l3-moves-transfer-20261012`. Forward-only. No edit to `src/Legality/Legality.cpp`, accepted legality, MAIN, coordinator PR #298, other lanes, original saves or shared CI.

## What this stage implements

The existing L3 native Pal Park verifier already checked a one-stage original PK3 route, eight prohibited Gen III HM move IDs, the exact arrival four slots and a narrow Platinum destination. Stage 4 added multi-species / multi-GBA-cartridge candidate reconstruction, but no Pal Park verifier bound **that complete original path** to a Gen IV arrival. This stage connects those previously isolated witnesses.

- `include/Legality/Gen34PalParkMultiCartridgeCore.h`: a pure, read-only C++20 original-candidate evaluator that replays a **full Stage 4 GBA witnessed timeline** (multiple cartridges, source-specific mandatory level-up choices, original evolutions, original TM/HM/relearner and Move Deleter decisions, and **separately identified Shedinja sibling**) before admitting an exact Pal Park transfer boundary.
- Source GBA TM teaching requires **one independent, single-use physical item receipt per actual TM step**, bound to original cartridge, species, exact numbered TM, move and action index. Orphan receipts, duplicates and receipts borrowed from a different source cartridge cannot create a valid candidate. This is an evidence *requirement*, not a claim that ordinary PK3 files record such receipts.
- The exact outbound GBA four-slot snapshot must match the source replay, include no **Gen III HMs** (Cut 15, Fly 19, Surf 57, Strength 70, Flash 148, Rock Smash 249, Waterfall 127, Dive 291), and be identical to the independently verified immediate Gen IV four-slot arrival. A real source-side Move Deleter sequence can remove an HM **before** the crossing; a separately witnessed Gen IV HM teaching **after** the crossing is not incorrectly rejected.
- After arrival, the same Pokémon must preserve its candidate ID, unchanged species, level and slot chronology; unverified/non-normal forms remain `Unknown`, not silently normalized. Required exact-title Gen IV level-up offers cannot be omitted; numbered machines require correctly attested retail item numbers and compatibility, single-use TM receipts cannot be recycled, original tutor/relearner source availability must be established, move deletion compacts slots, and visits to an unmodeled Gen IV game's move source remain `Unknown` when level/method information is needed.
- `include/Legality/Gen34PalParkMultiCartridgeOriginal.h`: **replaces caller-supplied GBA catalogs** with the independently fingerprinted original 386-species source arrays already owned by L3, on each of five game titles. For Gen IV it binds only to the **twelve actually pinned original Platinum native source species** and **twelve independently pinned numbered Platinum machine identities**; D/P/HG/SS post-arrival methods are not falsely claimed complete.
- Every hard negative is a **contradicted proposed candidate history only**, never a global Invalid finding. Missing historical item receipts, original snapshots, complete alternative routes, parental/breeding/event evidence and other-generation method sources remain `Unknown`. Candidate-set contradiction requires every actual route to be externally authenticated and enumerated.

## Tests delivered and actually executed

- `tests/test_legality_l3_palpark_multicartridge_core.cpp`: **21,519 assertions/scenario groups per execution** covering 25 destination-cartridge combinations from verified candidate histories, evolving Bulbasaur→Ivysaur via Ruby/FireRed visits, original TM receipt provenance, exact Move Deleter slot continuity, direct Gen III HM exclusion, Nincada→Ninjask→Shedinja sibling identity, four-slot post-arrival learning, forced Gen IV offers, single-use TM receipts, original tutor and reminder methods, post-arrival HM teaching, unsupported title visits, many altered history packets, unknown-before-contradiction priority and competing alternative routes.
- **Actual executions (exact committed core/test bytes):** GCC 14.2 optimized `-O2` PASS; Clang 17 optimized `-O2` PASS; GCC 14.2 `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer` PASS; Clang 17 with identical sanitizer flags PASS. Strict common options `-std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude`. Sanitizer processes ran with `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1`. **86,076 passed scenario groups across the four executions**, no observed failures.
- Exact Git blobs used in these local tests and matched to GitHub after upload: core `c7c233f9298c884c42e20a2924c5004a5125d920`, test `42f9d6c9f1d44f340192a35e1281c0201b532712`. The prerequisite Stage 3 and Stage 4 core sources were exact previously committed Git blobs.

### Production-linked acceptance coverage: registered, NOT yet executed

`tests/test_legality_l3_palpark_multicartridge_original.cpp` links **actual full repository** source arrays and asserts all five original GBA game titles × 386 National species = **1,930 original source fingerprints**, **19,865 ordered original level-up offers**, **111,940 original numbered GBA machine/species/game method checks** (44,635 source-compatible bits), plus 25 direct five-by-five source/destination combinations, original Platinum TM53/tutor/relearner replay, two-game original evolution + source TM/deletion receipts and the original Nincada/Ninjask/Shedinja split. It also checks the Deoxys form gate and unpinned Platinum TM/method outcomes. This suite is **registered in the full runner but has not compiled or executed with the real source headers locally**.

The local container's `git ls-remote https://github.com/GlitchedZeus/PokeBank-NX.git HEAD` fails with `Could not resolve host: github.com`. Four focused standalone executions **do not** establish passing complete production integration.

## Acceptance gates and sequence

The existing L3-owned `tests/test_legality_l3_run_all.sh` registers **27 real standalone native programs**, each compiled in normal and ASan/UBSan modes by both GCC and Clang: **108 individual builds/executions** required by the exact-head L3 matrix. The existing **L3-specific** GitHub workflow watches both new header paths and its test/runner changes; no shared CI or coordinator workflow changed.

Required on the complete exact GitHub PR #296 head:

```bash
L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 CXX=g++ bash tests/test_legality_l3_run_all.sh
L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 CXX=clang++ bash tests/test_legality_l3_run_all.sh
```

Independently run host tests, RSE regression, sanitizer gates and original-data parity on an accepted **forward-only coordinator** integration. Until actual CI verifies the new production native suite and cross-lane builds, **acceptance stays BLOCKED**. No merges are authorized from this lane.

## Next major stages

Stage 6: expand Gen IV original Diamond/Pearl/Platinum/HeartGold/SoulSilver native source and title-specific level-up, TM/HM, tutor, relearner, breeding/event and form boundaries. Do not infer all-title equality from Platinum's 12 source-pinned species.

Stage 7: source-bounded L2 parent/egg and L4 event witnesses, native PK1–PK4 end-to-end chronology, unknown provenance preservation and final coordinator-only handoff. Continue using PR #296; no Stage 7 integration or production gating bypass before its real test results.
