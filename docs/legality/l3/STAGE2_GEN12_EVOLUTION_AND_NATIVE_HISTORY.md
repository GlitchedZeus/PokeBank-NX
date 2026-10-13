# L3 Stage 2 — Gen I/II original-source four-slot evolution and native snapshots

Isolated branch: `feature/legality-l3-moves-transfer-20261012`, draft PR #296. Source saves, accepted legality, MAIN, L1/L2/L4/L5, `src/Legality/Legality.cpp`, and coordinator PR #298 are out of scope.

## Original data boundaries

The new `Gen12CandidateMoveMultiStage.h` represents complete level-up lists for **five modeled Gen I species** (Bulbasaur, Ivysaur, Venusaur, Charmander, Charmeleon) and **five Gen II species** (Bulbasaur, Ivysaur, Venusaur, Cyndaquil, Quilava). Original pinned source paths:

- `pret/pokered@af519899719f0754965776faac0e836a3b906e6d/data/pokemon/evos_moves.asm`
- `pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382/data/pokemon/evos_moves.asm`
- `pret/pokegold@ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8/data/pokemon/evos_attacks.asm`
- `pret/pokecrystal@3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6/data/pokemon/evos_attacks.asm`

The corresponding source original rows were independently inspected before implementation. Blue uses the modeled Red-family lists; Silver uses the modeled Gold-family lists. This does **not** establish complete moveset legality across all 151/251 species or across unspecified stadium/tradeback/egg/event routes.

## Engineering delivered

- Single-candidate four-slot replay with original-game mandatory level-up offers, same-level ordered decisions, slot insertion/replacement and no move duplication.
- Source-backed level evolutions Bulbasaur→Ivysaur→Venusaur (Gen I/II), Charmander→Charmeleon (Gen I), Cyndaquil→Quilava (Gen II). Evolution changes the level-up table without erasing any of the four retained moves.
- Exact original numbered **Bulbasaur-only** TM06/TM08/TM10 and HM01 teaching hypotheses, never a fabricated item receipt. TM10 maps to Double-Edge in RBY and Hidden Power in GSC.
- Distinct same-generation cartridge visits, direct Gen I→II, Gen II→I Time Capsule, and verified return-to-original-Gen-II-game edges, including the ban on retaining move IDs above 165 at entry to Gen I.
- Gen II Move Deleter replay with compacted four slots only when the hypothetical original method step is externally attested. No invented Gen I deletion.
- Unknown for unsupported breeding/parent history, tutor, reminder, unmodeled game/species, malformed method packets and unsupported cross-species evolution. A contradicted *candidate* is never a global Pokémon Invalid verdict. Aggregate alternatives remain Unknown unless every actual history is independently enumerated.
- `Gen12CandidateMoveNativeSnapshot.h`: strictly copies read-only PK1 and PK2 parser-record slots and source context. A native format alone never proves an original cartridge or historical decision; `emptyWitnessSkeleton` deliberately leaves original snapshot, same-Pokémon, and chronology attestation flags false. Egg, all-empty and invalid-length/move-domain records are excluded.

## Verified local executions, 2026-10-12

The **exact final committed C++ header and test bytes** have been compared with GitHub blobs after local compilation. Normal commands used:

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -Iinclude tests/test_legality_l3_gen12_multistage.cpp -o /tmp/l3-gen12-multi
clang++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -Iinclude tests/test_legality_l3_gen12_multistage.cpp -o /tmp/l3-gen12-multi-clang
```

Both compilers also ran with `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`, `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1`, and `UBSAN_OPTIONS=halt_on_error=1`. All four multistage runs passed **9,978** scenario/assertion groups per run.

The native snapshot test passed **571** checks in each of the same four modes, but **locally used minimal exact-type synthetic PK1/PK2 record stubs** because the full repository could not be cloned (GitHub DNS failure). The committed fixture includes the **real** `Integration/Gen1/Gen1ReadOnlySave.h` and `Integration/Gen2/Gen2ReadOnlySave.h`; its real-production-header compilation is **NOT verified** until exact-head CI runs. Do not misrepresent this stub-local result as linked native parser/integration success.

## Coordinator acceptance gates

The existing L3-only runner now registers **21 real C++ suites** (normal + ASan/UBSan) per compiler. The existing L3-specific workflow covers Gen12 header changes and displays the correct 21-suite count. Neither shared CI nor production analyzer code was edited.

Run on a clean, complete PR #296 checkout, and then on the coordinator's new forward-only combined application head:

```bash
L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 CXX=g++ bash tests/test_legality_l3_run_all.sh
L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 CXX=clang++ bash tests/test_legality_l3_run_all.sh
```

**Block acceptance** until both complete runner executions, exact-head host/RSE, and combined ASan/UBSan pass. This isolated branch has not performed a production `Legality::analyze` call or original-retail specimen history proof. Do not merge into PR #298 or accepted legality from L3; coordinator must review its new commits without overwriting later other-lane work.

Remaining major limitations: full R/B/Y/G/S/C species-level and machine data, trustworthy parent/egg/evolution histories, original player TM receipts, historical Time Capsule/stadium source snapshots, tutors/reminders, and all-route completeness. All remain Unknown.
