# L3 MAJOR Stage 1 — Gen I/II source-ordered, candidate-scoped four-move histories

**Repository lane:** existing draft PR #296, `feature/legality-l3-moves-transfer-20261012`. This change is **not** production `Legality.cpp` integration or a global legality engine verdict. Never derive "Invalid" from one contradicted path.

## Original-source verification (2026-10-12)

Pinned upstream source snapshots (rather than today's generic online species move pool):

| Original version | Source pin | Independent facts used |
|---|---|---|
| Red/Blue | [pret/pokered@af519899719f0754965776faac0e836a3b906e6d](https://github.com/pret/pokered/tree/af519899719f0754965776faac0e836a3b906e6d) | `data/pokemon/evos_moves.asm`: Bulbasaur lv7 Leech Seed, lv13 Vine Whip; Charmander learnsets. `constants/item_constants.asm`: TM06 Toxic, TM08 Body Slam, TM10 Double-Edge. `data/pokemon/base_stats/bulbasaur.asm`: original machine species compatibility. |
| Yellow | [pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382](https://github.com/pret/pokeyellow/tree/e89ead154b9968aa50eed9328ff2b38b6c194382) | Independently checked `data/pokemon/evos_moves.asm` Bulbasaur level list; the new solver's represented RBY family does **not** imply other species are interchangeable. |
| Gold/Silver | [pret/pokegold@ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8](https://github.com/pret/pokegold/tree/ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8) | `data/pokemon/evos_attacks.asm`: Bulbasaur lv7 Leech Seed/lv10 Vine Whip, both lv15 PoisonPowder and Sleep Powder; Cyndaquil levels; `constants/item_constants.asm` TM identity. |
| Crystal | [pret/pokecrystal@3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6](https://github.com/pret/pokecrystal/tree/3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6) | Same independently checked pinned Bulbasaur and Cyndaquil level lists; TM10 **Hidden Power**, not RBY TM10 Double-Edge; `data/pokemon/base_stats/bulbasaur.asm` original compatible machines. |

The source-pinned implementation is in `include/Legality/Gen12CandidateMoveJointChronology.h`. It models **complete level-up tables only for Bulbasaur in RBY/GSC, Charmander in RBY, and Cyndaquil in GSC**; Bulbasaur's pinning includes a deliberately bounded original-machine sample (RBY TM06/08/10 and GSC TM06/10). Unsupported species, item numbers, methods or exact-game history are **Unknown**, not false contradictions.

## What the new four-slot verifier proves — and does NOT prove

- Replays the **same asserted Pokémon** with four actual ordered slots, legal insert/overwrite decisions, offered level-up moves that cannot be silently skipped, and two level-up offers at the same level. A version-wide move-pool union is never evidence of a feasible complete history.
- Verifies exact source-game and observed-game identity, move method and numbered machine semantics before certifying a **single proposed path**. Example: RBY TM10 produces Double-Edge (#38) but GSC TM10 produces Hidden Power (#237).
- Distinguishes within-generation visits, an independently verified **Gen I → Gen II** trip, and an independently verified **Gen II → Gen I → Gen II** round trip. The reverse gate requires a verified retail Time Capsule policy, attested Gen I species and a complete Gen I-compatible four-slot departure. A later Gen II-only move must be newly witnessed *after* return.
- Preflights all source evidence before replay so an unmodeled later method cannot turn an otherwise uncertain specimen into a hard negative. Unsupported transfers, service visits, eggs, events, ancestry/evolutions, and an unrecorded snapshot remain Unknown.
- Supports multiple independently proposed candidate paths; incomplete alternatives remain Unknown even if a separately proposed route contradicts the retail table. The `CandidateContradicted` enum applies **only to the specified hypothetical path**, not to Pokémon legality.

## Reproducible validation

`tests/test_legality_l3_gen12_joint_chronology.cpp` is a standalone strict C++20 suite with compile-time assertions and runtime assertions covering RBY and GSC source tables, same-level offers, source/version TM mapping, skipped required offers, bad slot order, lack of original snapshot, tradeback directions, Gen II-only moves at the Gen I boundary, post-return reacquisition and competing Unknown branches.

**Exact native source bytes** after Stage 1 additions were locally compiled and run successfully using GCC `-std=c++20 -Wall -Wextra -Wpedantic -Werror -O2`, GCC `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`, and Clang `-std=c++20 -Wall -Wextra -Wpedantic -Werror -O2`, with all programs reporting PASS. The GitHub blob IDs were compared to the exact locally executed header/test bytes. These are local standalone tests, **not** proof that all L3 suites or the repository-wide gates have run on the new branch.

The existing `tests/test_legality_l3_run_all.sh` is updated to **eight** independent native and **eight** sanitizer suite executions. It intentionally still requires the complete 3,947-entry accepted Emerald level-up dataset: the full eight-suite gate, full native PK1/PK2 corpus, integrated CI and source-hardware coverage are **not** claimed as passed in this stage.

## Required continuing stages and outstanding Unknown routes

1. Stage 2: verified Ruby, Sapphire, FireRed and LeafGreen per-species complete move data; do not borrow unsupported Emerald whole-species tables.
2. Stage 3: widen joint four-move ordering over explicit evolution, breeding, trade and relearner/tutor history, preserving erased trainer choices.
3. Stage 4: verify Gen III→IV Pal Park and post-transfer acquisition provenance, including HM exclusion and exact species/version method receipts.
4. Stage 5: native PK1–PK4 positive, candidate-negative and Unknown fixtures, exact-head runner and sanitizer gates, and informational-only coordinator recommendations.

**Limits:** no ordinary PK1/PK2 reveals the trainer's original move choices, visit order, source machine receipts or complete ancestry. The synthetic positive candidate witnesses are *not* independently authenticated specimen histories. Missing events, inherited moves, unmodeled species, Stadium/Korean policy and same-Pokémon evolutions continue to return Unknown. No save injection, original save edit, branch merge, coordinator PR or shared CI modification was authorized or made.
