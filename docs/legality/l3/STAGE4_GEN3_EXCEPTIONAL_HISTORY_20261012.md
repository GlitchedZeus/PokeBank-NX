# L3 Stage 4 — original Gen III evolution, cartridge visit, and Shedinja exceptional histories

2026-10-12 · isolated L3 draft PR #296 · branch `feature/legality-l3-moves-transfer-20261012`.

## Implementation and original-source boundary

This stage adds two L3-owned, read-only headers and two substantial native regression suites. It **does not** modify production `src/Legality/Legality.cpp`, accepted legality, MAIN, coordinator PR #298, source saves, or other lane code.

- `Gen3GbaExceptionalHistoryCore.h` composes existing, exact four-slot Stage 3 `Gen3GbaJointMethodHistoryCore` source-bound histories into a **single** witnessed Pokémon timeline. It requires full attestation of each original level-up decision, game visit, same-individual transition, evolution or sibling creation. It checks uninterrupted moves and level continuity, original source species, mandatory immediate evolved-species level-up offers **at the exact evolution level**, fully ordered replacement choices, historical numbered TM/HM receipts, move relearner and source-context move deletions. It preflights **every phase and transition** before any candidate contradiction so later unsupported tutors, lost machine receipts or unobserved services remain `Unknown`.
- Source-supported level evolutions are Bulbasaur→Ivysaur→Venusaur, Charmander→Charmeleon→Charizard, Squirtle→Wartortle→Blastoise and Nincada→Ninjask (level 20), with canceled/delayed evolution allowed at a later level. Other possible evolutionary mechanisms require separate original evidence; absence from this narrow model is not illegality.
- An authenticated **same-individual cartridge visit** changes the game-specific source catalog without importing methods from an unvisited cartridge. Claimed slot values and original identity must match exactly. A source game visit, NPC service, move deletion or numbered TM receipt cannot be fabricated from a detached PK3.
- **Shedinja is a new sibling**. Source evidence must include original Nincada→Ninjask, all three exact-level-20 Ninjask offers and their player decisions, creation after those offers, party/ball resource witnesses, a distinct sibling ID, and copied four-slot contents. Later Shedinja uses species #292's own source offers; Ninjask's later moves cannot be stitched onto it. The sibling's same-Pokémon ID cannot masquerade as Ninjask.
- `Gen3GbaExceptionalHistory.h` constructs production-bound catalogs using Stage 3's independently fingerprint-pinned original per-game/386-species tables and 58 TM/HM machine identities. All five retail GBA game identities remain distinct. These method facts are not original player acquisition receipts.
- Unsupported event, egg, tutor, transfer, or service steps remain `Unknown`; a contradicted *hypothetical* route never authorizes global `Invalid`. Candidate-set contradictions only apply to an externally complete, authenticated set of actual possibilities.

Original source lines and identities were cross-checked with pinned `pret/pokeruby@63a8cbf0016b351a4e68f7036fa0b77e23d2f2c1`, `pret/pokeemerald@5eff78649e7170a877b961ef0b3da13b81a16038`, and `pret/pokefirered@c75f352304d529f6ba92d4f74b9cf8b5c3810788`. Relevant underlying source files include `src/data/pokemon/level_up_learnsets.h`, `src/data/pokemon/tmhm_learnsets.h`, `src/evolution_scene.c` and the original move relearner functions. Emerald/FRLG tutor method compatibility and true one-time NPC flags are **not** accepted by this module: no genuine teacher visit or item receipt is encoded in ordinary PK3 data.

## Actual executed tests

The exact final committed `Gen3GbaExceptionalHistoryCore.h` blob `c5f762d88eff7e07c5aa9178c6431fbdeee1031c` and exact committed `test_legality_l3_gen3_exceptional_core.cpp` blob `11359cacaae3586360dc7d4933ccf392d22ec2ce` were locally executed with the original exact prior Stage 3 core blob `c3368b800d9ae34125ebc65278d829c26219bf66`.

| Actually executed | Result |
|---|---|
| GCC 14.2 C++20 strict, `-O2` | PASS — 10,240 adversarial groups |
| Clang 17 C++20 strict, `-O2` | PASS — 10,240 groups |
| GCC 14.2 `-O1 -g -fsanitize=address,undefined` | PASS — 10,240 groups |
| Clang 17 `-O1 -g -fsanitize=address,undefined` | PASS — 10,240 groups |

Common `-std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude`; sanitizer builds add `-fno-omit-frame-pointer` and execute with `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. No compiler warnings or sanitizer failures observed in those **four focused standalone** executions. These synthetic witness successes do **not** attest original retail player histories.

**Registered but NOT YET EXECUTED against actual production headers:** `tests/test_legality_l3_gen3_exceptional_native.cpp`, requiring real production original-source catalogs for all 1,930 game/species identities and starter/Shedinja evolution integration across the five games. A direct full checkout remains unavailable in this runtime because `git ls-remote` fails with `Could not resolve host: github.com`; GitHub Actions were queued during authoring. This native gate must not be counted as passed merely because the standalone core passed.

## Exact-head integration gate and disposition

This stage registers two new suites in the existing L3-only `tests/test_legality_l3_run_all.sh`: the total is **25 C++ programs** (25 normal and 25 sanitizer binaries per compiler). The existing L3-specific workflow watches the two headers and new test paths and runs GCC and Clang against their exact PR head; no shared workflows, other lane Makefiles, or coordinator branches were edited.

On a clean, complete exact L3 checkout:

```sh
L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 CXX=g++ bash tests/test_legality_l3_run_all.sh
L3_EXPECTED_EMERALD_BLOB=3e53c1c951876a26cace5dde060cc9ff2c35d0a7 CXX=clang++ bash tests/test_legality_l3_run_all.sh
```

Then coordinator must forward-integrate only L3's newer commits into its independently advancing PR #298 and run its full combined host, focused RSE, native/Sanitizer and cross-lane evidence matrix. Keep PR #296 draft; **all release acceptance stays blocked** until these exact-head results exist. No new Gen V or Vault work.

## Remaining Stage 5 and later blockers

Stage 5: real original Gen III→Gen IV Pal Park transfer and multi-cartridge ancestry, unchanged species/form and native records, source/destination four slots, Gen III HM removal and Gen IV post-arrival teaching. Stage 6: complete Gen IV per-title move acquisition. Stage 7: L2 breeding and L4 event cross-lane witnesses and final informational-only coordinator handoff.

Outstanding original history facts that cannot be reconstructed from one PK3: actual machine/item consumption, move tutor flags and access, forgotten decisions, eggs/parents, events, authentic evolution and cartridge visits, complete alternative histories and original retail-hardware provenance. All remain Unknown, not `Invalid`.
