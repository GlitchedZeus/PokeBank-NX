# L2 B12 / B13 — Diamond and Pearl original retail daycare assembly

**Date:** 2026-10-12 · **Scope:** Gen IV Diamond/Pearl only · **Review status:** Source-pinned, independently transcribed and synthetic conditional-stage test coverage; full original retail binary execution and all CI gates NOT YET validated.

## Original-source discovery and identity

The narrow `arm9/src/daycare.c` only contains daycare state accessors, but **the original egg algorithms ARE located** in the pinned Diamond/Pearl overlay assembly:

- Repository: **pret/pokediamond** revision `b623abef1b4dfe204ca50aab42447ac00c134290`.
- Primary source: [`arm9/overlays/05/asm/ov05_021EC458.s`](https://github.com/pret/pokediamond/blob/b623abef1b4dfe204ca50aab42447ac00c134290/arm9/overlays/05/asm/ov05_021EC458.s) Git blob `e084c052388870e0d3e984379b1bb23e1a911790` (2,939 inspected lines).
- RNG source: [`arm9/src/math_util.c`](https://github.com/pret/pokediamond/blob/b623abef1b4dfe204ca50aab42447ac00c134290/arm9/src/math_util.c) `LCRandom` at line 553, `PRandom` at line 561, `MTRandom` at line 578.
- Daycare state accessor: [`arm9/src/daycare.c`](https://github.com/pret/pokediamond/blob/b623abef1b4dfe204ca50aab42447ac00c134290/arm9/src/daycare.c) `Save_Daycare_GetEggPID` / `Save_Daycare_MasudaCheck`, blob `6b8cf712dc0924189c7e81e256a48975f84be6fb`.
- Common Diamond/Pearl repo contains `pokediamond.us.sha1` and `pokepearl.us.sha1`. **The common original overlay is source evidence for the pinned repo, not proof that every regional retail binary was independently executed**. Source SHA-256, assembly hash-to-ROM attestation, game-version/region verification and legal licensing remain L5 independent QA follow-ups.
- Upstream [research #252 RNG-003](https://github.com/GlitchedZeus/PokeBank-NX/blob/fc0225e6351af62365f29200300323e38f52c8fe/docs/legality/research/packets/RNG-003.md) explicitly lacked independent DP execution. These findings add original *DP assembly* source, but do not relabel its HGSS/Platinum comparisons as DP retail execution.

## B12: independent Diamond/Pearl IV inheritance

Original `ov05_021EC9D4`, source lines **688–832**, calls `ov05_021EC99C`, lines **654–685**.

1. Available six stat IDs are initialized to `[0,1,2,3,4,5]` in the original stack buffer.
2. Select three stats via consecutive `LCRandom` high-16 rolls modulo `6,5,4`.
3. The ROM assembly passes **iteration index i**, rather than the selected stat index, to `ov05_021EC99C`; that routine marks the specified buffer position `0xff` and compacts into the front, allowing the same stat to be selected multiple times. The direct DP source establishes this even though Platinum independently has a similar bug; the DP implementation is **NOT copied from Platinum**.
4. Three more `LCRandom` draws select physical daycare slot `roll & 1`; move-father and move-mother roles do not reorder slots.
5. Original D/P stage consumes **six** LCRNG calls. No historical Power-item selector appears in this original D/P function. Overwrites of repeated stat indices are applied in sequence, and the last donor wins.
6. Stage RNG: `seed = seed*0x41C64E6D+0x6073` modulo 2^32, output `seed>>16`. Rewind of a separately independently known final boundary uses `0xEEB9EB65` exactly six times. This does **not** reverse a child IV array into a unique seed.

New `Gen4DiamondPearlEggIVStage.h` matches the original assembly control flow and is separately restricted to `diamond_nds` / `pearl_nds`. Tests `test_legality_l2_dp_egg_iv.cpp` contain original-assembly hand-emulated fixed vectors at seeds 0,1,2,0x12345678,0xffffffff for both source games, 4,096-seed bounded loop per game, repeating-stat case, hard candidate-local mismatches and explicit missing-parent/order/pre-inheritance/RNG **Unknown** statuses. The vectors are **synthetic independent assembly emulations**, not 4,096 original retail executable comparisons.

## B13: independent Diamond/Pearl pending PID and collection

Original `ov05_021EC8A8`, lines **529–610**:

- Identify female candidate from original gender snapshots; if Ditto is present, the Ditto donor wins. The assembly contains an otherwise unreachable two-Ditto selection path, but **independent original daycare eligibility rejects two Ditto**, so the conditional engine explicitly rejects that pair even when a caller incorrectly asserts eligibility. No speculative two-Ditto egg history can be reported compatible.
- Selected donor must hold original creation-time Everstone, item **229**. A separate `LCRandom` high16 roll less than `0x7FFF` activates nature inheritance; equality `0x7FFF` does not. A present-day parent's held item cannot establish the original donation.

Original `ov05_021EC944`, lines **612–652**:

- If nature inheritance is not active, use **one** pending `MTRandom` 32-bit PID.
- If active, check consecutive 32-bit MT outputs against the donor's stored original PID nature (`pid%25`), disallowing PID=0 as the accepted nature-match branch. Reject and retry, with the original loop's **2,401st MT output** retained even if it is still wrong. It is incorrect to impose an invented 64-draw cap or treat a bounded incomplete MT transcript as global Invalid.

Original `ov05_Daycare_CreateEgg`, lines **1482–1530**:

- Read stored pending PID; original `Save_Daycare_MasudaCheck` compares the physical two parents' **original language bytes** (not guessed parent nations).
- If language differs and the pending PID is not already shiny for the **original full trainer ID**, perform up to **four** `PRandom(pid) = pid*1812433253+1` transformations, stopping on shiny; otherwise keep the pending PID.
- This is an egg collection stage, not interchangeable with the earlier MT creation stage. Original source-stage tokens and true initial MT/LCRNG transcripts are not persisted on a detached egg.

New `Gen4DiamondPearlEggPidHistory.h` preserves these stages and requires original parent snapshots, eligible ordinary daycare pair, original physical deposit order, original held items, source time/event identity shared across parent/MT/collection witnesses, true LCRNG entry state, consecutive exact MT output transcript, original language and trainer-ID values, and original unhatched egg PID. Candidate-local statuses distinguish conditional PID match, mismatch, consumed-count contradiction and genuinely missing information.

Tests `test_legality_l2_dp_egg_pid.cpp` include both games, the strict 0x7FFE/0x7FFF boundary, female/Ditto donor selection, rejected double-Ditto/no-valid-donor pair hypotheses, one-MT PID, multiple rejected PIDs, 2400/2401 boundary and fallback, original 1812433253 PRandom international four-reroll path, initial shiny skip, differing trainer/languages unavailable, time token mismatch and candidate-local negatives.

## B12 + B13 same original immutable native egg

New `Gen4DiamondPearlNativeEggHistory.h` checks original encrypted **PK4** parent and child byte identity, checksum-valid strict immutable `Pokemon4ReadOnly` records from a sourced DP save container, both original parent snapshots, original native unhatched egg, source game identity and same independent daycare event token. It derives historical PID/IV *output observation* and parent IV/PID/gender from the **attested original encrypted snapshots only**.

It deliberately never infers original parent held items, language, initial random child IVs, LCRNG/MT stage boundaries or chronology from modern PK4 fields. Each separately sourced hypothesis must supply those values. If either IV or PID candidate is locally contradicted, the result is `LocalCandidateContradiction` — **not whole-Pokémon Invalid**. If any required historical evidence is missing, the joint history remains unresolved.

`tests/test_legality_l2_dp_native_egg_history.cpp` constructs **synthetic** original-format checksum-valid encrypted DP PK4 parents and an unhatched child, verifies joint IV + PID history match, independently changes the proposed seed and MT transcript to generate only candidate-local contradictions, tests unknown provenance and corrupt encryption, and checks the original source input bytes remain unchanged. **Synthetic records are not preserved authentic eggs**.

## Outstanding evidence and integration gates

- The pinned DP assembly is inspected, but not independently compiled/executed against matching original retail ARM binary. L5 should verify byte identity to Diamond **and** Pearl region revisions, run a separate original-assembly emulator/compiler or controlled original-ROM execution, record SHA-256 source pins and compare random vectors/MT attempt counts to the independently authored C++ helpers.
- L5 should supply original observed paired saved daycare parent + egg snapshots and MT/LCRNG stage timing evidence if available. A detached PK4 is fundamentally incapable of reconstructing erased held items, stage RNG, true parents or original egg collection chronology.
- Shared host Makefile/workflows and production `src/Legality/Legality.cpp` are coordinator-owned; the three new DP `tests/test_legality_l2_*` sources are not yet auto-wired to standard CI. Need exact-head normal compilation with `-Werror`, full host, focused RSE, ASan/UBSan, staged forward-only accepted ancestry reconciliation, and independent QA before acceptance.
- This branch contains no source-save or staged Pokémon mutation, automatic fix, whole-Pokémon Invalid, MAIN/#103 integration, original retail function body, distributed ROM/archived event or licensing change.
