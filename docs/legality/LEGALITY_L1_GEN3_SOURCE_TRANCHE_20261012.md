# L1 second major tranche — E06 E08 E09 E10 (bounded original Gen III sources)

**Source point:** accepted `feature/legality-engine-gen1-4-20260929@d29eada5c087e201e00c749703f1a2de3ddfadeb` was live as this tranche started; lane base remains the previously accepted `0dccf2e10289164ad0d065985dcb29f14fd2fb35`. The coordinator alone may reconcile all newer accepted ancestry; never reset/rebase backward.

## Source, evidence and licensing

- PKHeX pinned `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`, source resources `encounter_r/s/fr/lg.pkl` from `EncounterArea3` via NX's accepted generated `Gen3SafariEncounterData.inc` and `Gen3SafariMethodHEvidence.h`. **R/S and FR/LG original decompilation original-game encounters still require Q06 original-source multiset verification.** Ruby original source pin `pret/pokeruby@5784633ce4ef7ade1a7f2d2d0c288e3d5e6cdd7f`; FR/LG `pret/pokefirered@037335f4c725d7c9aecdac87066f2002b4bd7e14`. Pinning repositories does not mean this tranche has independently reproduced every encounter call path.
- PKHeX `Encounters3Colo.Shadow`, `Encounters3XD.Shadow`, `TeamLockResult`, `MethodFinder`; accepted NX `Gen3ColoShadowEncounterData.inc` (80 rows), `Gen3XdShadowEncounterData.inc` (90 rows), `Gen3ColoShadowTeamLockData.inc` and `Gen3XdShadowTeamLockData.inc`. Research `#252@fc0225e6351af62365f29200300323e38f52c8fe` `ENCOUNTER-001` and `RNG-001`. GameCube original retail disassembly source/full team reconstruction is NOT yet independently proven at the level Q04 requires, so label these candidate checks pinned-PKHeX **grade B**, not confirmed game provenance grade A.
- All additions are independently written wrapper assertions around existing accepted source tables/headers. No upstream event binaries or decompiled functions copied. PKHeX GPL-3.0 vs NX AGPL-3.0 import requires source-specific licensing review before any new bulk table imports (none occur here).

## E06 + E08 — four exact GBA games, source-row first

`include/Legality/Gen3SafariIndexedOriginalHistory.h` binds exact Ruby/Sapphire/FireRed/LeafGreen **original game** to a single independently attested original Safari encounter-area row (retains `areaIndex` duplicates), original species/form, met level/location, Safari Ball, six original IVs, recorded PID, separately attested no-lead Method-1/2/3/4 family, and same original pre-PID seed. Ruby/Sapphire use Hoenn Safari-block alternatives and FireRed/LeafGreen do not. Historical source area/rod and lead must be asserted externally; PK3 records cannot infer original area after the fact.

New `tests/test_legality_l1_safari_indexed_original.cpp` has **22 concrete reproducible R/S/FR/LG vectors**: Grass/Surf/Old Rod/Good Rod/Super Rod × four games, plus Rock Smash × R/S. Positive frame and origin area assertions, swapped exact-game row as Unknown provenance, ball/source species/known seed candidate-local contradictions, and missing original save/row/lead/ball/PID-IV as explicit Unknown. Ruby identical observed tuple can represent distinct physical area 180/181; duplicates intentionally retained.

**Unfinished E06/E08:** ordinary non-Safari areas, all source-specific swarms/fishing/non-no-lead families, R/S roaming source caveats, FR/LG roaming variants and transfer/evolution history. Source table is deliberately Safari-only; no unconditional gamewide completeness claim.

## E09 + E10 — original Colosseum/XD before RNG selection

`include/Legality/Gen3GameCubeIndexedShadowHistory.h` requires independently attested **Colosseum versus XD original title and exact original shadow row**. Although PK3 saves both titles under `originVersion=15`, Colosseum ordinary shadows require standard XDRNG and source team set, with *no* XD anti-shiny fallback. XD shadows require original fateful/ball/shiny restrictions, exact XD row, independently attested original trainer TID/SID, optional Miror B. rebattle witness, XD-specific anti-shiny reroll and recursive source team. SearchLimit and missing historical fields are Unknown, never global Invalid.

New `tests/test_legality_l1_gamecube_indexed_shadow.cpp` includes pinned source-native **Colosseum Makuhita** (`0xC252FEBA`, level30/location5, nontrivial team lock), ordinary Bayleef/duplicate Murkrow team-source identity checks, **XD Teddiursa** (`0xB7951831`, trainer 12345/35598, one anti-shiny reroll, fixed Poke Ball), and **XD Mawile** (`0x049F2F05`, trainer12345/51882, full team and separately witnessed Miror B. rebattle alternative). Negative source row, PID/IV, ball, fateful and trainer constraints; unknown capture source, original game, rebattle and team search budget.

**Unfinished E09/E10:** any original GameCube release/transfer/console provenance, Colosseum e-Reader rows, source-specific team-lock ambiguities, XD Poké Spots, non-shadow fixed gifts/trades, post-snag evolution/Shadow purification timeline. These helpers prove a *supplied* capture hypothesis, not that an arbitrary detached PK3 actually came from one console.

## Tests, ownership and merge hazards

- Two new owned headers and two new `tests/test_legality_l1_*.cpp` suites, registered in the owned standalone runner `bash tests/test_legality_l1_run.sh`; **six total native suites** now attempted in both O2/Werror and ASan/UBSan passes, without touching shared Makefiles/workflows.
- The runner still needs execution in an actual exact-head checkout and per-step logs; remote connector read/write cannot execute native GCC. Existing GitHub host workflow runs pre-existing tests only. **No CI green claims for new suites until explicitly run**.
- Main coordinator must integrate source headers/tests only after exact-head full host, RSE and ASan/UBSan checks and forward-only preservation of every newer accepted SHA. Do not change `src/Legality/Legality.cpp`, global ledgers, or accepted legality; this PR is draft and never self-merges. Original-save writes, staged editing, Gen V, Master Vault, UI/touch and MAIN all untouched.
