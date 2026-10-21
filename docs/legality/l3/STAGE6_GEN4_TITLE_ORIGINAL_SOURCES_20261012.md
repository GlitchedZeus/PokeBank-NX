# L3 Stage 6 — original Gen IV title-specific acquisition-method sources

**Date:** 2026-10-12. **Lane:** `feature/legality-l3-moves-transfer-20261012`, existing draft PR #296. **No merge** into accepted branch, MAIN, coordinator PR #298, L2/L4 event/breeding lanes, or production `src/Legality/Legality.cpp`.

## Original source reconstruction: implemented methods versus unknown methods

This stage adds **source-derived retail data**, not new guesses about recorded PK4 player histories.

- **Diamond / Pearl**, `pret/pokediamond@b623abef1b4dfe204ca50aab42447ac00c134290`: `files/poketool/personal/wotbl.json`, `personal.json`, `include/constants/{species,moves}.h`. National species 1–493 matched by **actual original source names**, and their **6,527 ordered original level-up offers** were converted from exact move names to native numeric IDs. All 493 normal-species rows' TM01–TM92/HM01–HM08 compatibility were transcribed: **17,575 original species-machine compatible methods**.
- **HeartGold / SoulSilver**, `pret/pokeheartgold@53dded53802b237d39fabfd8ee396fe9cdc8aa3e`: `files/poketool/personal/personal.json` and `files/fielddata/wazaoshie/waza_oshie.json`. Independently name-joined normal National species 1–493; **17,632 source-compatible machine methods** and **5,165 species-to-tutor source methods** across **52 different original native tutor move identities**. **Important:** the game's `files/poketool/personal/wotbl.narc` is a *binary* NARC in this source. It was not decoded/compared, therefore **HGSS level-up chronology and native move-relearner methods remain UNKNOWN**, not imported from Diamond or Platinum.
- **Platinum**, `pret/pokeplatinum@fe1ea418a7364c93daf0d07ae55e195452d8803c`: retains L3's original **twelve** pinned normal-form species level-up / TM/HM compatibility / tutor lists in `Gen4PlatinumNativeMethodData.h`, with the **twelve** independently identified numbered TM/HM source mappings in `Gen4PlatinumPinnedMachines.h`. All *other* Platinum species-level sources and unpinned Platinum item identities remain UNKNOWN.
- **Source anomaly preserved:** Diamond/Pearl's original **Banette #354** row has level-8 Screech before level-5 Night Shade. The source array is **not reordered or fabricated**; `completeOrderedOffers(354)==false` deliberately withholds positive four-slot level chronology for Banette, while retaining its original machine method bitmap. The exact source anomaly was independently detected by traversing the original GitHub snapshot.
- **Forms:** only explicitly identified *normal forms* from National #1–493 are modeled in the broad Diamond/Pearl/HGSS tables; alternate Deoxys / Wormadam / other forms and original form transitions remain UNKNOWN without exact independently sourced form-stage data. Events, breeding parents and egg transfers remain L2/L4/Stage 7 responsibilities.

**New data:** `include/Legality/Gen4NativeOriginalSourceData.h` is a generated, immutable constexpr 493-species source catalog (~114 KB). The above counts were obtained by parsing the original pinned source files via GitHub and joining names, not by inventing a species-wide compatible move pool.

## History verifier and adapter

**New C++20 core** `include/Legality/Gen4NativeOriginalHistoryCore.h`: reconstructs the same hypothetical Pokémon's ordered four-slot history through an explicitly attested Gen IV game, mandatory original same-level decisions, a separately attested game visit, original source machine-item identity and compatible species, original tutor/relearner methods where supported, deletion, and immutable original/observed snapshots. Source-unknown methods, unsupported special forms, skipped unknown level-up stages, discarded historical player choices or unattested source-game visits stay **Unknown**. A contradicted candidate is *never* global Invalid. All steps are preflighted before a move contradiction: a later unsupported original tutor cannot be hidden behind an earlier local negative.

**Production-source adapter** `include/Legality/Gen4NativeOriginalHistory.h`: original source title enum is **never unified across games**. Diamond and Pearl get their full source-derived packed offers and bitmaps. HGSS gets the full source-derived machine and 52-tutor availability with explicit Unknown level-up. Platinum gets only its twelve truly pinned species; its different HM source bitmap positions are normalized to the shared TM01–92 / HM01–08 index, without changing any retail source data. Unsupported title/source combinations do not silently substitute Platinum facts.

**Important precision limit:** DP/HGSS original numbered item-to-native move mappings have **not been independently verified in this stage**, so even a positive species-machine bit **cannot by itself certify an exact TM/HM teaching step**: the engine deliberately returns Unknown until the original title's item→move identity is pinned. Platinum's existing twelve pinned source item mappings remain available. Original teacher/service access, item receipts, and player selections must *always* be independently witnessed.

## Executed tests versus queued tests

**Actually compiled and executed**, using the exact final committed core/test source bytes:

```sh
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -O2 tests/test_legality_l3_gen4_native_core.cpp -o /tmp/l3-stage6-gcc
clang++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -O2 tests/test_legality_l3_gen4_native_core.cpp -o /tmp/l3-stage6-clang
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer tests/test_legality_l3_gen4_native_core.cpp -o /tmp/l3-stage6-gcc-asan
clang++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer tests/test_legality_l3_gen4_native_core.cpp -o /tmp/l3-stage6-clang-asan
```

Both optimized and both sanitizer binaries **PASS**, **112,002 assertion/scenario groups each** (**448,008 executions total**). The sanitizer processes use `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1`. The local source uses a strictly modeled test-only catalog for the independent core; this is **not** an integrated real-header test.

**New real-header original-data regression**, `tests/test_legality_l3_gen4_native_origins.cpp`, is **registered, NOT YET RUN against actual repository original source headers**. On a real checkout it must verify all 493 DP rows, all 493 HGSS source-method rows, all 12 Platinum source rows, 6,527 original DP offers, 17,575 DP and 17,632 HGSS species-machine bits, 5,165 HGSS tutors, selected cross-title histories, Banette fail-closed and form/unsupported-source Unknown. No claimed green gate for this real-header program.

The complete repository is not directly cloneable in the current environment: `git ls-remote https://github.com/GlitchedZeus/PokeBank-NX.git HEAD` failed with `Could not resolve host: github.com`. GitHub's connected repository API was available for retrieving and committing individual source files, but it did not supply the full working tree to run the complete L3 runner.

## Required full acceptance

The L3-owned runner now registers **29 full C++ suites**; for each compiler, it executes each suite's normal C++20 binary and ASan/UBSan binary (58 per compiler, **116 full-program executions** in the complete GCC+Clang matrix). The original Emerald data Git blob remains pinned and unmodified.

```bash
L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 CXX=g++ bash tests/test_legality_l3_run_all.sh
L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 CXX=clang++ bash tests/test_legality_l3_run_all.sh
```

Native full-data compilation, full host/RSE regression, CI exact-head matrix and coordinator combined sanitizers **remain BLOCKED until actually executed successfully**. Do not imply completion from only the standalone core or original source-row reconstruction.

**Stage 7 next:** L2 breeding parent/egg and L4 events move-source witness boundaries, native PK1–PK4 end-to-end scenarios, normal/sanitizer gates, and final coordinator integration handoff with exact SHA, all blockers and source-limited Unknown. **No merge by L3.**
