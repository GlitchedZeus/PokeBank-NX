# L1 first major tranche — E07 E11 E13 E15

**Scope:** Gen I–IV isolated legality / issue #286 / parent #285. Source-only candidate helpers; no shared analyzer wiring, global Invalid reporting, mutation, workflow changes or branch acceptance.

## Exact repository dependencies / frozen evidence

- PokeBank NX accepted base: `0dccf2e10289164ad0d065985dcb29f14fd2fb35`; accepted `include/Legality/Gen3SafariMethodHEvidence.h`, `Gen3SafariEncounterData.inc`, `Gen3PidIvCorrelation.h`, `Gen3CxdPidIvCorrelation.h`, `Gen3ChannelPidIvCorrelation.h`, `Gen3BacdPidIvCorrelation.h`, `Gen4WildEncounterData.inc`, `Gen4WildRngCorrelation.h` and `Gen4NatureReversalEvidence.h`.
- Source freeze research #252: `fc0225e6351af62365f29200300323e38f52c8fe`; `docs/legality/research/packets/ENCOUNTER-001.md`, `RNG-001.md`, `RNG-002.md`, `SOURCE_REGISTRY.json`. Research stage fixtures are synthetic, not native-source certificates.
- PKHeX audited pin `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`: `PKHeX.Core/Legality/RNG/MethodFinder.cs`, `ClassicEra/Gen4/GenerateMethodJ.cs`, `GenerateMethodK.cs` and source-generated encounter tables. GPL-3.0; no upstream implementation or binary data copied by this tranche.
- HGSS retail original `pret/pokeheartgold@9d8b7591f09b65804da2fb2dfd56f320633e0d36`: `src/pokemon.c:CreateMonWithNature`, `src/field/encounter_check.c:getWildMonNature`, `FieldSystem_GenerateSafariEncounter`, `FieldSystem_GenerateBugContestEncounter_Internal`, `FieldSystem_GenerateRegularEncounter`, `EncounterSlot_WildMonSlotRoll_Headbutt`, `FieldSystem_GenerateRockSmashEncounter`, `FieldSystem_GenerateFishingEncounter`. Exactly verified retail source logic must be separated from current save-world context.
- Emerald source reference `pret/pokeemerald@731ad5bfd6e6f265508d0efcca0ba42f9dcf5881` and PKHeX pinned Method H; `Gen3SafariMethodHEvidence.h` is accepted partial positive source evidence, *not* a proof that all lead histories have been enumerated.

## Work delivered (first bounded engineering tranche)

**E07:** `Gen3MethodHOriginalStage.h` joins **one indexed, original Emerald Safari row**, original level/location/species/form, Method-1/2/3/4 PID/IV seed, and original no-lead or successful Synchronize frames. The same source row is used for the Method H frame, rather than allowing another area sharing location 57 to supply a slot. Missing historical row, lead, seed or unsupported lead paths remain Unknown. Other already-accepted Method H branches (Pressure, Static, Cute Charm, failed Sync and Safari-block alternatives) are not eliminated by this conservative new interface.

**E11:** `Gen3PidIvFamilyRouter.h` differentiates ordinary GBA Methods 1/2/3/4, truncated Ruby/Sapphire/FRLG roamer, GameCube XDRNG/CXDAnti, Channel (R/S only, exact PK3 stored origin-bit agreement), and BACD candidate correlation families with independently known game/family/trainer fields. This is *family selection*, not event-template/catch provenance proof. A failed matcher (including XD 64 anti-shiny search limit) never declares globally Invalid.

**E13:** `Gen4WildSpecialStageEvidence.h` consumes **exact HGSS packed rows** for five families: fishing, Rock Smash, Headbutt, Bug Catching Contest, Safari. It requires independent original capture-time area/method context, a known no-lead Method K route, and original PID **plus six 5-bit IVs**, all verified from the **same** independently attested Method-1 pre-PID seed before matching slot/lead. Optional `evaluateNative` now joins unchanged original checksum-valid encrypted PK4, exact HG/SS origin-version byte, source group, species/form/location/level/PID/IVs and independently attested captured-creature identity. Test suite builds separate encrypted native original HG and SS PK4 snapshots for each of five category routes, then tests a bad checksum, wrong version/source group, wrong PID, unknown native provenance and byte immutability. An unrelated PID/IV seed is candidate-local contradiction, never global Invalid. Reuses accepted row-specific frame checks and minimum-31 reroll paths; a bounded RNG miss is Unknown. Newly generated tests cover separate original HeartGold and SoulSilver rows and original-stage-positive, local row contradiction and missing-stage outcomes. Time-of-day, Safari block/date configurations, swarm activation, tree group, BCC eligibility, lead history and other conditional states must be proved independently; current save properties do not attest them.

**E15:** `Gen4NatureReversalReplay.h` replays **forward from an independently attested pre-nature RNG state**, using Method J's division and Method K's modulo nature, and bounded generated PID pairs. Explicit `SearchExhausted` is not an impossible history. Frozen RNG-002 HGSS vector: initial state `0x00000012`, nature 20, 129 rejected pairs, retained PID `0xEA0705DB`, accepted pre-PID state `0xE0D14A4F`. The legacy 128 reverse-window search exhausts; a 130-pair forward/reverse witness succeeds. This does not yet retroactively extend every existing row matcher; coordinator owns shared integration decisions.

## Coverage, evidence grade, and caveats

- Four new read-only headers; four independent native C++ translation-unit suites `tests/test_legality_l1_*.cpp`; E13 suite additionally constructs ten checksum-valid encrypted PK4 snapshots, one per original game x five special categories (synthetic historical stage, not archival catches).
- Positive/negative/Unknown targets: E07 indexed source vs distinct source and missing historical stage; E11 all five families plus wrong game/variant/trainer/IV and Unknown; E13 5 categories × HG/SS source rows plus wrong row and missing original conditions; E15 forward no-lead J and K, 129-deep HGSS vector, candidate-local incorrect PID, 128/129 resource exhaustion and original-stage Unknown.
- **Synthetic native evidence:** some samples reuse accepted source-linked Gen III PID/IV vectors, while Gen IV test witnesses are synthetic RNG states joined to pinned original game encounter-table rows. They are **not** retail-captured and no fixture demonstrates a real-world authentic Pokémon.
- **Unfinished for full catalog closure:** E07 all Emerald Method H lead and conditional Safari/fishing histories; E11 every GameCube/event/roamer method variant and original event/template feasibility; E13 full historical Safari blocks/time, Bug Contest entry and all lead/slot alternates; E15 complete extended original-stage J/K slot/lead/reversal integration and capture-time proof, including independently preserved original PID/IV, source location and original source save. Do not mark these catalog IDs fully closed or PKHeX-parity.
- **Coordinator action needed:** register the four new native test translation units in *shared* host Makefiles (L1 cannot change them); reconcile only exact-head all-green lane PR into isolated accepted, re-test combined head. Do not self-merge.

## Standalone exact-head L1 native test runner (no shared Makefile edits)

From a checked-out PR HEAD, run `bash tests/test_legality_l1_run.sh`. The runner compiles **all four** new C++ test units under `-Werror`, then recompiles and runs all four with `-fsanitize=address,undefined`. E13 links production `Pokemon4ReadOnly`, `Encryption4`, and all three Gen IV personal tables to exercise checksum-valid encrypted original HG/SS PK4 fixtures. The build outputs are isolated temporary files and cleaned up. The script cannot itself be executed via the connector; its existence is not a passing result. Coordinator must run it manually/on CI and retain exact-head logs.

## Test commands (only after coordinator adds owned test targets)

`g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude tests/test_legality_l1_gen3_family_router.cpp -o /tmp/l1-family && /tmp/l1-family`

`g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude tests/test_legality_l1_emerald_methodh.cpp -o /tmp/l1-emerald && /tmp/l1-emerald`

`g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude tests/test_legality_l1_hgss_special.cpp -o /tmp/l1-hgss && /tmp/l1-hgss`

`g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude tests/test_legality_l1_gen4_nature_replay.cpp -o /tmp/l1-nature && /tmp/l1-nature`

E13 native-PK4 test requires linkage with the existing accepted native Gen IV read-only/encryption source graph (e.g. the coordinator-owned `GEN34_BALL_ANALYZER_SOURCES` model); the simple single-file command for E13 alone is insufficient once `evaluateNative` is exercised. Repeat with `-O1 -fsanitize=address,undefined -fno-omit-frame-pointer` and `ASAN_OPTIONS=detect_leaks=0`. Full accepted suite: `make -f Makefile.host host-clean host-test host-sanitize`; focused RSE regression is owned by the shared GitHub host-tests workflow. Passing an unrelated host workflow does not prove L1's new tests have been compiled.
