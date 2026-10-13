# L3 Stage 3 — original Gen III game-specific joint method source

Date: 2026-10-12. Isolated existing branch `feature/legality-l3-moves-transfer-20261012`, draft PR #296. **No merge** into coordinator #298, accepted legality, MAIN or the other L1/L2/L4/L5 lanes. No saved Pokémon or source save mutation.

## The production gap actually addressed

Earlier L3 stages already pinned the *complete* **386-species level-up tables** for original Ruby, Sapphire, Emerald, FireRed and LeafGreen (including FR/LG conditional Deoxys and Dugtrio source rows), plus all **386 species × 58 numbered TM/HM original compatibility bitmaps**. Those source tables alone could not establish that **one Pokémon** retained four specific moves through the required level offers, an exact numbered item teaching and an original move-relearner visit.

**New code:** `include/Legality/Gen3GbaJointMethodHistoryCore.h` is an audit-safe, pure constexpr C++20 four-slot replay model. It requires an externally attested original snapshot, same Pokémon/game/species continuity, every player action and each individual original teaching/service receipt. It enforces mandatory ordered level-up offers (including same-level offers), exact TM vs HM item numbers/moves, species-specific compatibility, original level-bounded relearner menu, same-game move deletions, four-slot replacement, nonempty/no-duplicate move records, and complete coverage through observed level.

**New production binding:** `include/Legality/Gen3GbaJointMethodHistory.h` uses the accepted `Gen3GbaVersionedLevelUpTimeline` and `Gen3GbaOriginalMachineData` for the caller's exact Gen III cartridge. Before a source catalog may contribute a candidate-consistent result, it compares every source offer (original order, native move ID and level) against the **independently pinned per-game, per-species `Gen3GbaOriginalSourceFingerprints` manifest**, with exact-game/species FNV and 58-bit source bitmap bounds. A Ruby source cannot silently be reused as FireRed's Charmander or LeafGreen's Deoxys. Machine compatibility is source-method possibility, not a real item receipt.

The core returns `Unknown` on missing source identity, unsupported service, absent historical receipt, unknown parent/evolution/egg/trade stages, missing complete chronology, or an insufficiently pinned input. A verified contradiction applies **only to one hypothetical history**. Aggregated alternatives cannot be contradicted when any actual history is still unknown, absent or unenumerated; nothing in this module sets global `Invalid`, produces an original player receipt, or writes any save.

## Test programs and execution

- `tests/test_legality_l3_gen3_joint_core.cpp`: 9,268 adversarial candidate/source packet assertions per execution, five original game identities, machine-number substitution, missing receipts, skipped level offers, Ruby-versus-FireRed Charmander, valid Bulbasaur level-up→TM06 Toxic→move relearner, incomplete histories and alternative-history fail-closed semantics. **Locally executed on exact committed core/test Git blobs** in GCC and Clang, both `-O2` normal and `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer` sanitizer builds. All **4/4 PASS**.
- `tests/test_legality_l3_gen3_joint_native.cpp`: production-joined proof gate across **all 1,930 game/species pairs**, 19,865 original ordered level-up source records, **111,940 exact numbered machine species/game candidate checks** (44,635 method-compatible; 67,305 method-contradicted) and per-level original relearner availability checks. Refuses wrong/absent original source fingerprints and rejects fabricated original claims. The test is **registered in L3's full runner, but it has not been executed against the full production headers** because a complete local clone is unavailable. Its expected counts are independent original-source totals, not a passing runtime log.

### Commands actually executed for the standalone program

```sh
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -Iinclude tests/test_legality_l3_gen3_joint_core.cpp -o /tmp/l3-stage3-gcc
clang++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O2 -Iinclude tests/test_legality_l3_gen3_joint_core.cpp -o /tmp/l3-stage3-clang
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude tests/test_legality_l3_gen3_joint_core.cpp -o /tmp/l3-stage3-gcc-asan
clang++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Iinclude tests/test_legality_l3_gen3_joint_core.cpp -o /tmp/l3-stage3-clang-asan
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 /tmp/l3-stage3-gcc-asan
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 /tmp/l3-stage3-clang-asan
```

The standalone compiler used only the exact committed audit-owned core header/test bytes; the production binding and native source-index test were not mocked into a false real-header PASS. The local container cannot `git ls-remote` the complete GitHub repository because DNS resolution fails. Existing GitHub Actions are queued, not completed. **Acceptance remains blocked.**

The existing `tests/test_legality_l3_run_all.sh` now registers **23 standalone production/source suites** (normal plus ASan/UBSan, no skips). The existing isolated L3 workflow now watches the new `Gen3GbaJoint*.h` files and runs the 23-suite runner under GCC and Clang. The official exact-head gate is:

```sh
L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 \
  CXX=g++ bash tests/test_legality_l3_run_all.sh
L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 \
  CXX=clang++ bash tests/test_legality_l3_run_all.sh
```

Then coordinator must combine **only newer L3 commits** with its own forward-only head and independently run complete host/RSE, source-identity and all normal+sanitizer gates. Do not merge on the strength of local synthetic tests or paper evidence.

## Original source limitations and next stage

Original learnset/code revisions already pinned by L3:
`pret/pokeruby@63a8cbf0016b351a4e68f7036fa0b77e23d2f2c1`,
`pret/pokeemerald@5eff78649e7170a877b961ef0b3da13b81a16038`,
`pret/pokefirered@c75f352304d529f6ba92d4f74b9cf8b5c3810788`.

The 386-species level-up *source method* and TM/HM *compatibility* sets are complete for these selected retail source revisions, **not a proof of all obtainable current four-move combinations**. Historic parent/egg, trade, evolution, repeated TM consumables, Move Deleter/relearner access, real item receipts, event/tutor delivery, cartridge visits and full alternative source histories remain generally absent from a detached PK3. The new candidate replay requires external attestation for all such modeled steps; missing ones stay Unknown.

Stage 4 next: evolve-species history and unusual acquisition pathways (Shedinja, TM/HM, move deletion, versioned tutors and service visits). Keep later Pal Park and Gen IV work isolated to their stages.
