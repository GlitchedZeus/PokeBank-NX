# L2 major tranche — Gen III original daycare IV alternatives (B04/B05/B06/B11/B18)

**Status:** committed conditional source-stage checking on dedicated L2 draft PR #295. Standalone new-test compilation and exact-head GitHub full host/sanitizers remain **UNVERIFIED**. These are source-backed synthetic stage hypotheses, not native retail eggs or completed whole-Pokémon legality checks.

## Original source, pinned source identity, licensing

This tranche composes the already-accepted source-backed six-LCRNG-draw replay in `include/Legality/Gen3EggIVInheritanceStage.h`. It does **not** copy/paste retail code or change the accepted implementation.

| Original game | Frozen primary original source (pinned by research PR #252 `SOURCE_REGISTRY.json`) | SHA-256 of source file supplied by research #252 | Retail stat removal |
|---|---|---|---|
| Ruby/Sapphire | [pret/pokeruby@5784633ce4ef7ade1a7f2d2d0c288e3d5e6cdd7f `src/daycare.c` `InheritIVs` lines 406–467](https://github.com/pret/pokeruby/blob/5784633ce4ef7ade1a7f2d2d0c288e3d5e6cdd7f/src/daycare.c) | `f4aafb22b8d55f5fe90c930fb84a5c4c468e6bf590d34a1eceddc304c676eb63` | remove stat **value** used as list position |
| Emerald | [pret/pokeemerald@731ad5bfd6e6f265508d0efcca0ba42f9dcf5881 `src/daycare.c` `InheritIVs` lines 527–597 (`BUGFIX` disabled)](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/daycare.c) | `b20e1aef476abbd91b4882cca5dbe03002a0473c3a963bbadfce9dc7c8984964` | remove **loop index i**, not selected IV |
| FireRed/LeafGreen | [pret/pokefirered@037335f4c725d7c9aecdac87066f2002b4bd7e14 `src/daycare.c` `InheritIVs` lines 791–850](https://github.com/pret/pokefirered/blob/037335f4c725d7c9aecdac87066f2002b4bd7e14/src/daycare.c) | `3509b363c4cadd7bb9de83fe288b49a8decc76d53d8b600c81b557ba052173ba` | remove selected stat **value** |

Source grades: original source family pinned by PR #252 research packet RNG-001; research record states 24 independent **synthetic** RNG-stage vectors (8 Emerald, 8 Ruby, 8 FireRed) plus source comparisons. This tranche *does not claim* original ROM retail readout, a matched original native PK3 source parent+egg fixture, source licensing approval or source-export rights. No retro retail binary/asset is included.

## Engineering

- `include/Legality/Gen3EggIVAttestedAlternatives.h`: up to 16 explicitly identified, independently attested **one-source-egg** historical IV stage alternatives across Ruby, Sapphire, Emerald, FireRed and LeafGreen only. Never combine game-specific stage algorithms. Supports independently supplied original child observed IVs, original event ID, historical parent pair ID, physical deposit-order parent identities, original parent IV snapshots, original child's **pre-inheritance random six IVs**, six-draw known **final** LCRNG seed and separate source stage token identifying the same original deposited parent state.
- Every route must independently attest all required historical inputs. A detached PK3 has none of the seed or original deposition stage data; a present parent collection cannot retroactively prove original parent snapshot timing or physical deposit order.
- `OneConditionallyConsistentStage` requires one fully coherent exact replay matching observed original egg IVs. `UnknownCandidateProvenance` includes unverified originals, mismatched source event, stage token or physical donor-pair identity, duplicate/zero parent IDs and incomplete seed attestation. Invalid hypothetical IV domain remains candidate input `UnresolvedStageHistory`; malformed observed IV domain separately identifies invalid *supplied original-child input*, not an analyzer verdict.
- A fully attested known mismatch can exclude that specific route. `AllEnumeratedIVStageHypothesesContradicted` means all **supplied** routes were locally contradicted AND the caller separately attested exhaustive **IV-stage hypothesis** enumeration. It is **NOT** globally Illegal/Invalid, does not prove all possible parents, breeding events, gift distributions or transfer histories were considered.
- Two physical deposited parent slots remain physical slots. The helper does NOT automatically reorder father/mother roles, reverse slots, substitute present-day parents, merge move history with some other egg's IV donors or claim original chronology from PID/IV.
- No original or staged save mutation, automatic legality fix, or global legality analyzer attachment.

## New tests (not yet project CI registered)

`tests/test_legality_l2_gen3_iv_attested_alternatives.cpp`:
- 4,096 synthetic entry RNG seeds **for each of five games** = **20,480** seeded conditional positive / alternate-route / local-negative / omitted-exhaustiveness scenarios.
- Reversing physical parent order without silently reassigning original RNG parent slots.
- Independent missing-proof controls: archived original egg, 32-bound IV domain, source-stage flags, creation time event, physical deposit order, donated pair identity, repeated IDs, missing final RNG boundary, malformed hypothetical parent IV, identical archive route IDs, overcapacity and no candidate.
- An alternative attested matching parent history rescues a contradicted same-egg stage route. Unknown if historical records do not prove original stage.
- Tests reuse accepted Gen III `Gen3EggIVInheritanceStage.h` and its **previously independently pinned** 24 stage vectors. The 20,480 seed cases are synthetic source-method confirmations, **not** 20,480 independently attested eggs or original ROM executions.

## CI and merge gates

At time of this tranche, dedicated L2 branch had diverged from the newer accepted legality branch due to accepted Platinum #283. No reset, rebase backward or force push allowed. Coordinator alone handles forward-only reconciliation preserving newest accepted/sibling heads.

All seven new `tests/test_legality_l2_*` sources are independent lane-owned test files. **They remain unregistered in coordinator-owned shared host Makefiles and workflows** and thus standard GitHub Host Tests—even if green—would not prove execution of these new tests. Coordinator must register them, compile strict C++20 with correct dependencies, run exact-head full host, focused RSE and ASan/UBSan, and get independent L5 source/retail binary QA before integrating.

B04/B05/B06 refer to source-game IV-stage input, **not a complete original egg PID/IV history**. B11 physical slot identity is partial for IV history, not complete move-parent source linkage. B18 alternatives here cover only GBA source-stage IV histories, not exhaustive breeding chains or Gen II/G4.
