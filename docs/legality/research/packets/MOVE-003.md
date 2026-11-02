# MOVE-003 — Shedinja copies an evolution-time moveset, not Ninjask’s lifetime pool

Research ID: **MOVE-003**
Affected generations/games: III–IV; retail independent confirmation here is Emerald
Audited application revision: `fecfbca9b0323578fd423f5d90486ddb2d0da72e`
Estimated complexity: **High**
Evidence confidence: **B**, subject to the precise scope below.

## Current implementation and wiring

The engine has dedicated Shedinja ball/evolution evidence, but the move classification branch at `Legality.cpp:1507` has no Ninjask evolution-time witness. Do not mistake `Gen4ShedinjaBallEvidence` coverage for moveset coverage. Broad pre-evolution pools alone cannot model the Ninjask sibling copy.

## Authoritative sources

- **NX-MOVE3**: [include/Integration/Gen3/Gen3LearnsetTable.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/include/Integration/Gen3/Gen3LearnsetTable.h); `Availability classify`; grade B.
- **NX-MOVE4**: [include/Integration/Gen4/Gen4MoveCompatibility.h](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/include/Integration/Gen4/Gen4MoveCompatibility.h); `Availability classify`; grade B.
- **PK-GROUP3**: [PKHeX.Core/Legality/LearnSource/Group/LearnGroup3.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Group/LearnGroup3.cs); `public bool Check`; grade B.
- **PK-GROUP4**: [PKHeX.Core/Legality/LearnSource/Group/LearnGroup4.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Group/LearnGroup4.cs); `public bool Check`; grade B.
- **E-EVO**: [src/evolution_scene.c](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/evolution_scene.c); `CreateShedinja`; grade A.
- **E-LEVEL**: [src/data/pokemon/level_up_learnsets.h](https://github.com/pret/pokeemerald/blob/731ad5bfd6e6f265508d0efcca0ba42f9dcf5881/src/data/pokemon/level_up_learnsets.h); `sNinjaskLevelUpLearnset`; grade A.

## Retail mechanic / exact exceptions

Emerald `Task_EvolutionScene` changes Nincada into Ninjask, processes new evolution-level moves, and only at EVOSTATE_END calls `CreateShedinja`, which copies the resulting Ninjask record before changing its species and clearing several fields. Thus a Shedinja can acquire Ninjask moves at the one evolution level, but cannot later learn arbitrary Ninjask level-up moves.

The pinned learnsets give Ninjask Agility (97) at 38 and Baton Pass (226) at 45. Nincada and Shedinja do not have those level-up acquisitions. This creates a concrete incompatible pair under the ordinary Emerald evolution route: one Nincada cannot evolve at both levels. **Swords Dance is a bad negative control** because an alternative tutor/TM route can make it independently obtainable.

Do not flatten the pinned PKHeX comment “one Ninjask move” into “one copied move total.” Ninjask has several level-20 acquisitions (Double Team, Fury Cutter, Screech), and ordinary learnability/tutor sources are checked first. The pinned helper grants at most one **otherwise unresolved** move. Retail copying and alternate paths must be considered together.

## Inputs / lost information

Current level, species 292, moves, candidate Nincada minimum encounter level, possible Gen III/IV visits, egg history, and evolution-level range are needed. No PK3/PK4 evolution timestamp survives. A candidate captured above a required evolution level cannot use that level's Ninjask move.

## Tests / hard-Invalid boundary

- Positive scenario: preserve Nincada to 38, evolve, retain Agility in Shedinja; compatibility expected.
- Positive scenario: preserve Nincada to 45, evolve, retain Baton Pass; compatibility expected.
- Negative **ordinary Emerald candidate**: Agility + Baton Pass requires two different evolution levels. Reject that route, not globally until all event/transfer/other sources are excluded.
- Unknown: missing encounter/evolution level range or unenumerated external sources.
- False-Invalid control: Swords Dance + Baton Pass must explore a separate tutor route; no blanket two-Ninjask-moves ban.

Selected factual levels in `evidence/selected-levelup-facts.json` are decoded from pinned PKHeX and independently agree with the inspected Emerald table. No native Shedinja specimen was generated or run through PKHeX in this tranche.

## Integration / acceptance / order

Add a sibling-evolution edge supplying moves at one feasible level; retain alternative witnesses for each move. First add the copied-move compatibility path and tests, then combination rejection scoped to a complete candidate. Require native encrypted PK3/PK4 parser→analyzer fixtures in the development PR. Estimated complexity Medium for positive evidence, High for exhaustive negative closure.


## License and reuse restrictions

This packet is independently written implementation guidance. Linked PKHeX implementation is GPLv3 reference material. Retail decompilations and extracted game resources have unresolved reuse rights: **LICENSE REVIEW REQUIRED** before copying code/data into production. Research harnesses read external source locally; no retail function bodies or binary datasets are committed here. Synthetic vectors are not preserved original Pokémon.
