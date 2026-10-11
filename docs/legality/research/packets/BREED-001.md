# BREED-001 — Ordered egg moves and distinct ordinary/event origins

Research ID: **BREED-001**  
Affected generations/games: II–IV; independent retail details: Crystal, Emerald, Platinum, HGSS  
Audited application revision: `fecfbca9b0323578fd423f5d90486ddb2d0da72e`  
Estimated complexity: **High**  
Evidence confidence: **B**, subject to the precise scope below.

## Current implementation and wiring

`Gen34EggMoveEvidence` provides DirectEggMove or PreEvolutionEggMove membership. `Gen34EggState` checks selected structural met fields. Neither proves breeding-parent compatibility or egg move ordering. Gen III event egg modules already exist, so ordinary daycare assumptions cannot be applied to all eggs.

## Authoritative sources

- **NX-EGGMOVE**: [include/Legality/Gen34EggMoveEvidence.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/include/Legality/Gen34EggMoveEvidence.h); `classify`; grade B.
- **NX-ANALYZE**: [src/Legality/Legality.cpp](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/src/Legality/Legality.cpp); `Report analyze`; grade B.
- **PK-EGGMOVE2**: [PKHeX.Core/Legality/Moves/Breeding/MoveBreed2.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Moves/Breeding/MoveBreed2.cs); `Validate`; grade B.
- **PK-EGGMOVE3**: [PKHeX.Core/Legality/Moves/Breeding/MoveBreed3.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Moves/Breeding/MoveBreed3.cs); `Validate`; grade B.
- **PK-EGGMOVE4**: [PKHeX.Core/Legality/Moves/Breeding/MoveBreed4.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Moves/Breeding/MoveBreed4.cs); `Validate`; grade B.
- **PK-BREED**: [PKHeX.Core/Legality/Breeding.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Breeding.cs); `IsValidSpeciesBit34`; grade B.
- **C-BREED**: [engine/pokemon/breeding.asm](https://github.com/pret/pokecrystal/blob/3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6/engine/pokemon/breeding.asm); `InitEggMoves`; grade A.
- **C-DAYCARE**: [engine/events/daycare.asm](https://github.com/pret/pokecrystal/blob/3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6/engine/events/daycare.asm); `.GotDVs`; grade A.
- **E-DAYCARE**: [src/daycare.c](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/daycare.c); `InheritIVs`; grade A.
- **PT-DAYCARE**: [src/overlay005/daycare.c](https://github.com/pret/pokeplatinum/blob/637bb186e11b5ce1ee578981311c057498e57126/src/overlay005/daycare.c); `Egg_InheritIVs`; grade A.
- **HG-DAYCARE**: [src/get_egg.c](https://github.com/pret/pokeheartgold/blob/9d8b7591f09b65804da2fb2dfd56f320633e0d36/src/get_egg.c); `InheritIVs`; grade A.

## Compact implementation brief

Missing: an ordinary-egg constructor and an origin resolver that keeps event eggs separate. It matters because independently legal inherited moves need not come from a feasible parent pair, and the unhatched move order is constrained. Build the constructor before a parent-chain search. Integration: candidate-specific Egg findings and MOVE-001 encounter initial moves. Unknown parents remain existential possibilities, never automatic Invalid.

## Source-backed rule catalog

| Rule | Exact scope / source behavior | Engine consequence |
|---|---|---|
| Breeding availability | Gen I has no daycare eggs; Gen II introduces them | Do not invent native PK1 eggs; a tradeback history is a different question |
| Egg groups | Crystal/Emerald/HGSS check no-eggs groups, gender and compatible groups; Ditto is exceptional | A baby in Undiscovered can still **hatch**, although it cannot itself be a parent |
| Ditto | Two Ditto cannot breed; a eligible genderless species uses Ditto | Never treat Ditto as a universal override for no-eggs species |
| Crystal kinship | Matching Defense DV and matching low 3 Special-DV bits prevent breeding | This is a constraint on a proposed **pair**, not a standalone child record |
| Crystal inherited moves | Iterate donor's four slots; accept egg, shared level-up or compatible TM/HM route, shifting oldest on overflow | Gen II ordering differs from the multi-pass III/IV algorithm |
| III/IV ordered moves | Base moves, father egg moves, father compatible TM/HM moves, then level-up moves shared by both parents | Simulate duplicate suppression and four-slot FIFO replacement; do not sort move IDs |
| Special Volt Tackle | Emerald Pichu: Light Ball on either parent appends it last; reference allows it in IV too | Special breeding mechanic, not an ordinary egg-list entry; do not require Volt Tackle on a parent |
| Incense III | Lax → Wynaut; Sea → Azurill, otherwise Wobbuffet/Marill | Both baby and non-baby egg candidates can exist |
| Incense IV | Adds Odd/Mime Jr., Rock/Bonsly, Full/Munchlax, Wave/Mantyke, Rose/Budew, Luck/Happiny, Pure/Chingling | Maintain nine independent incense alternatives; source table in HG `get_egg.c` |
| Split species | Nidoran/Illumise families use the pending PID species-selection bit | Keep species-selection PID checks candidate-specific; reference `IsValidSpeciesBit34` |
| Manaphy | HGSS breeding species calculation maps Manaphy to Phione | Manaphy daycare egg impossible, Ranger Manaphy egg is a valid separate origin family |
| Hatch/initial levels | PKHeX EncounterEgg2/3 = 5; EncounterEgg4 = 1 | Distinguish current level, hatch level and stored met-level 0 |

The ordinary constructor is not proof of father feasibility. A full chain-breeding solver needs egg-group/gender compatibility plus father movesets that can coexist, including previous breeding generations and events. A graph over individual move reachability is insufficient: maintain sets of up to four moves or the required subset plus acquisition constraints. Bound recursion and memoize species/game/required-subset; exhaustion is Unknown.

## Inputs and information lost

PK2/3/4 give current species, moves, egg state and varying provenance; no parent identities, parental items or parental move order survive. Native PK4 retains EggLocation after hatching; PK3 has no separate persistent egg-location field. Source game containing an egg need not be its breeding game. Traded eggs and event eggs need separate candidates before strict ordinary-origin checks.

## Test vectors / positive and negative evidence

Emerald Pichu base moves are Thunder Shock 84 and Charm 204. With eligible Light Ball parents and no other inherited moves, the constructor yields `[84,204,344,0]`. The reference's ordinary unhatched-egg validator expects Volt Tackle at the end. `[344,84,204,0]` is a proposed ordinary-egg ordering negative, **not** a blanket negative on hatched Pokémon, which can reorder/change moves. `[84,204,0,0]` is a normal no-Light-Ball positive. These move scenarios are source-derived, not executed native fixtures.

Manaphy marked as an ordinary daycare candidate must fail that origin, but a Ranger egg must be checked by the accepted Manaphy module. Ditto × Ditto is an impossible pair; missing parents is Unknown; baby species in Undiscovered must not be rejected merely for its egg group.

## Acceptance criteria and implementation order

1. Enumerate ordinary and event egg origins independently.
2. Reproduce the constructor's ordering/overflow/duplicate handling per game.
3. Add native unhatched PK2/PK3/encrypted PK4 tests and hatched controls.
4. Add parent-subset feasibility only after complete input resources are registered.
5. Add hard Invalid only when all permitted origins are closed; uncertain external/event data stays Incomplete.

Independent source confirmation is strongest for the inspected Crystal/Emerald/Pt/HGSS code. DP-specific retail implementation was not downloaded; its rule assertions remain pinned-reference grade B until directly corroborated. Full egg-group datasets and chain-parent solution tables are still required.


## License and reuse restrictions

This packet is independently written implementation guidance. Linked PKHeX implementation is GPLv3 reference material. Retail decompilations and extracted game resources have unresolved reuse rights: **LICENSE REVIEW REQUIRED** before copying code/data into production. Research harnesses read external source locally; no retail function bodies or binary datasets are committed here. Synthetic vectors are not preserved original Pokémon.
