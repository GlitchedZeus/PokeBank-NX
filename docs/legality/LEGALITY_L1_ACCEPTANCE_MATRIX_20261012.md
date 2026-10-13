# L1 exact-head validation and coordinator acceptance matrix (2026-10-12)

PR #293, `feature/legality-l1-encounters-rng-20261012`. Latest checkpoint when written `f910d62ba0e20206876c104329f6a3e733573896` (this acceptance document/runner hardening advances its HEAD). Accepted target last observed `d29eada5c087e201e00c749703f1a2de3ddfadeb`. **Neither is timeless: re-fetch before tests/merge.** Do not reset, rebase backwards or force-push. Do not self-merge L1/MAIN.

## What is actually checked

**Source/data checks, independent of the C++ test suite:** 31 source RBY fixed templates and 17 trade source rows indexed against accepted generated tables (81 game-permitted fixed-source combinations); 9856 packed GSC wild /107 static source counts and 36 selected index/method/game source tuples; 197 packed DPPt/HGSS static templates with 24 fixed fixture tuples; 20147 Gen IV wild source tuples with 12 Sinnoh selected method/game/slot families; 14 original GameCube fixed rows with **14/14** indexed fixture species aligned. Those are frozen source-array comparisons, **not** independently archived real specimens.

**Actual native independent executable validation in this session:** C++ reference LCRNG replay using eight GBA roamer original pre-PID seed vectors and **24** altered seed/PID/IV controls, compiled+executed under GCC 14.2 `-std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror` and `-O1 -fsanitize=address,undefined -fno-omit-frame-pointer`: **8 positive /24 negative PASS under each**. This standalone arithmetic reference does not include the repository’s `Gen3OriginalRoamerStage.h` or the full product analyzer, and therefore **does not qualify any of the 13 L1 native suites as green**. Native source testing is still mandatory.

## 13 repository-native unit suites — all still unexecuted in this session

| Suite | Catalog purpose | Normal / ASan+UBSan in actual repo |
| --- | --- | --- |
| `test_legality_l1_emerald_methodh` | E07 Emerald Method H and Synchronize original stage | Not executed / Not executed |
| `test_legality_l1_gen3_family_router` | E11 GBA, GC, roamer, Channel, BACD source-gated RNG | Not executed / Not executed |
| `test_legality_l1_hgss_special` | E13 native encrypted PK4 HGSS fishing/Headbutt/Contest/Safari | Not executed / Not executed |
| `test_legality_l1_gen4_nature_replay` | E15 129-rejection real Method J/K nature history | Not executed / Not executed |
| `test_legality_l1_safari_indexed_original` | E06/E08 exact R/S/FR/LG indexed Safari | Not executed / Not executed |
| `test_legality_l1_gamecube_indexed_shadow` | E09/E10 source-labeled Colosseum/XD shadows | Not executed / Not executed |
| `test_legality_l1_gen1_original_source` | E02/E03 RBY fixed, trade, Time Capsule and later evolution | Not executed / Not executed |
| `test_legality_l1_gen2_original_source` | E04/E05 GSC wild methods/time, gifts/roaming/egg | Not executed / Not executed |
| `test_legality_l1_gen4_static_indexed` | E14/E16 exact DPPt/HGSS gift, static, roaming, special PID class | Not executed / Not executed |
| `test_legality_l1_gen4_sinnoh_special` | E12/E16 Sinnoh Radar activation, Chain Shiny, Honey Tree ID | Not executed / Not executed |
| `test_legality_l1_original_candidate_universe` | E17/E18 conservative completeness and no global Invalid | Not executed / Not executed |
| `test_legality_l1_gen3_original_roamers` | E06/E08 original RSE/FRLG 8-bit vs full-IV source stage | Not executed / Not executed |
| `test_legality_l1_gamecube_original_fixed` | E09/E10 14 original Colo/XD fixed/gift/trade/bonus templates | Not executed / Not executed |

## Coordinator exact-head acceptance commands

At an actual git checkout of **the latest PR head**, preserve version identity and record SHA:

```bash
git fetch origin --prune
git rev-parse HEAD
bash tests/test_legality_l1_run.sh
# Then run repository-normal full host regression/CI:
make -f Makefile.host clean
make -f Makefile.host test -j2
# Required separately: existing focused RSE and product sanitizer gates
# exactly as defined in current GitHub host-tests.yml; do not infer these
# from successful isolated unit suites.
```

Important: `make -f Makefile.host` invocation must be checked against current live accepted workflow; do not assert commands above succeeded. **The current shared Makefile/host workflow does not register all thirteen new L1 test units**. L1 deliberately does not edit that shared surface. The standalone script compiles each new unit twice, and includes the actual original native PK4 parser dependency sources for the HGSS encrypted test.

Sanitizer runner now has `-fno-sanitize-recover=all`, `ASAN_OPTIONS=halt_on_error=1`, `UBSAN_OPTIONS=halt_on_error=1`, preserving the earlier `detect_leaks=0` runner environment. Sanitizer-detected undefined behavior now yields nonzero process failure, not a recoverable diagnostic with misleading success.

## Source confidence and remaining gates

Pinned research PR #252, PKHeX `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`; original retail stage sources RSE/FRLG pinned pret roamer.c where explicitly documented. **No native game/source stage archive supplied**, so fixed vector witnesses are synthetic original-stage candidate examples, not historic Pokémon authentication. All E01–E18 catalog rows remain partial or source-blocked rather than closed. Exact original encounter multiset, event/egg/tradeback/transfer routes, game-specific lead and Radar RNG histories need independent original retail proof and Q04/Q06/Q09 review; no false global Invalid.

Merge is coordinator-only on a verified up-to-date accepted legality tree, preserving newer accepted commits and rerunning final combined tests. MAIN, umbrella #103, source/emulator writes, other lanes, Gen V and Master Vault untouched.
