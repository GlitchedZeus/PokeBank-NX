# MOVE-002 — Time Capsule acquisition is a directional history constraint

Research ID: **MOVE-002**  
Affected generations/games: I–II retail RB/Y and GS/C; Korean and console policy must remain explicit  
Audited application revision: `fecfbca9b0323578fd423f5d90486ddb2d0da72e`  
Estimated complexity: **High**  
Evidence confidence: **B**, subject to the precise scope below.

## Current implementation and wiring

`Gen12TimeCapsuleEvidence` and analyzer reporting already distinguish present eligibility from historical compatibility. Gen I/II compatibility tables are wired. The remaining move-history gap is combination closure, not a missing Time Capsule feature. Read `tests/test_legality_legacy_move_context.cpp` alongside the native Gen I/II source tests before extending coverage.

## Authoritative sources

- **PK-GROUP1**: [PKHeX.Core/Legality/LearnSource/Group/LearnGroup1.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Group/LearnGroup1.cs); `public bool Check`; grade B.
- **PK-GROUP2**: [PKHeX.Core/Legality/LearnSource/Group/LearnGroup2.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Group/LearnGroup2.cs); `public bool Check`; grade B.
- **PK-SOURCE2C**: [PKHeX.Core/Legality/LearnSource/Sources/LearnSource2C.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Sources/LearnSource2C.cs); `GetCanLearn`; grade B.
- **PK-SOURCE2Stadium**: [PKHeX.Core/Legality/LearnSource/Sources/LearnSource2Stadium.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Sources/LearnSource2Stadium.cs); `GetCanLearn`; grade B.
- **PK-SOURCE1RB**: [PKHeX.Core/Legality/LearnSource/Sources/LearnSource1RB.cs](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/LearnSource/Sources/LearnSource1RB.cs); `GetCanLearn`; grade B.
- **C-TIMECAPSULE**: [engine/link/time_capsule.asm](https://github.com/pret/pokecrystal/blob/3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6/engine/link/time_capsule.asm); ``; grade A.
- **NX-ANALYZE**: [src/Legality/Legality.cpp](https://github.com/GlitchedZeus/PokeBank-NX/blob/fecfbca9b0323578fd423f5d90486ddb2d0da72e/src/Legality/Legality.cpp); `Report analyze`; grade B.

## Implementation brief and exact conditions

Track whether a candidate requires a Gen II → Gen I visit. For that boundary, every carried move must fit Gen I; later Gen II moves can coexist only if independently reacquired after return. A move that is available solely as an initial or inherited egg move cannot simply be assumed relearnable. Pinned `LearnGroup2.Check` explicitly invalidates already-parsed >165 initial/egg-source results when it must fall back through Gen I. Its control flow and settings matter: do not copy the numeric rule as a global ban on Gen II moves.

Crystal tutor sources and Stadium 2 reminders must be separate acquisition edges. `LearnGroup2` skips Crystal and Stadium for Korean records. `LearnSource2Stadium` uses its own `reminder_stad2.pkl` and source eligibility, not all level-up moves indiscriminately. `LearnSource1RB.GetIsTutor` includes GB-era Stadium Surf for Pikachu/Raichu when the relevant policy is enabled.

## Inputs / lost evidence

Use PK1 catch-rate/held-item evidence, PK2 language, format, current moves, species and possible evolution stages. Do not invent a stored “visited Stadium” bit. PK1/PK2 generally cannot establish the complete sequence of past trades. A currently ineligible PK2 can still have visited Gen I before learning later moves.

## Tests and expected reporting

- Positive boundary unit test: all carried moves ≤165 and valid Gen I species; eligibility only.
- Positive historical unit test: obtain a Gen I-only move, return, then obtain a Gen II level-up/TM move; do not reject solely because current moves exceed 165.
- Negative candidate test: history requires carrying a >165 egg-only move across the Time Capsule with no later reacquisition witness. Reject the **candidate**.
- Unknown: Korean/console policy or an unresolved event route; remain Incomplete.

These are scenario specifications, not proven species-specific combinations. No complete event/parent search has established a particular four-move retail specimen here. The next engineering task is to produce concrete species fixtures from the full source tables before adding a hard Invalid.

## Smallest correct integration / acceptance

Extend the MOVE-001 witness model with `must_delete_on_edge` and `can_reacquire_after_edge`; feed existing Time Capsule evidence into it. Require positive native PK1 and PK2 round-trip cases, a later-learned-move case, and a candidate-only contradiction. Complexity is high because catch-rate ambiguity and policy exceptions survive. Present eligibility remains separate from historical legality.


## License and reuse restrictions

This packet is independently written implementation guidance. Linked PKHeX implementation is GPLv3 reference material. Retail decompilations and extracted game resources have unresolved reuse rights: **LICENSE REVIEW REQUIRED** before copying code/data into production. Research harnesses read external source locally; no retail function bodies or binary datasets are committed here. Synthetic vectors are not preserved original Pokémon.
