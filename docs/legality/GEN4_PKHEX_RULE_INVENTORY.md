# PKHeX Gen IV (PK4) legality rule inventory

Reference for the Gen IV analyzer (`src/Legality/Gen4Legality*.cpp`, `include/Legality/Gen4Legality.h`).
It is a port of PKHeX.Core at kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722 (GPL-3.0) for
PK4 records whose origin is Diamond, Pearl, Platinum, HeartGold or SoulSilver, or Ruby, Sapphire,
Emerald, FireRed or LeafGreen through Pal Park. The tables it reads (`Gen4LegalityData.{h,cpp}`)
are generated from that checkout by `tools/gen_legality_gen4_data/run.sh` (`--check` verifies the
committed output). Decomp evidence below cites pret/pokeplatinum@1dc13dce and
pret/pokeheartgold@d2355244.

The analyzer only reads a decrypted 136- or 236-byte record and returns a `Legality::Report`.
Production calls it once, from `Legality::analyze` (`applyGen4Analyzer`, just before the final
`return r;`), for every native PK4 entity, with the exact source game of the save that holds it.

## 1. Pipeline (PKHeX class -> analyzer source)

| PKHeX.Core | Analyzer |
|---|---|
| `PKM/PK4.cs` accessors, `StringConverter4`, `SpeciesName`, `WordFilter` (NX, then 3DS) | `Gen4Legality.cpp` (`read`), `Gen4LegalityText.cpp` |
| `MethodFinder` (Method 1, BACD, Cute Charm, Channel, Chain Shiny, Pokewalker), `MethodJ`, `MethodK`, `LeadFinder.GetLeadInfo4`, `RuinsOfAlph4`, `SolaceonRuins4`, `HoneyTreeUtil`, `WurmpleUtil` | `Gen4LegalityRng.cpp` |
| `EncounterGenerator4`, `EncounterEnumerator4`; `EncounterSlot4`, `EncounterStatic4`, `EncounterStatic4Pokewalker`, `EncounterTrade4PID`, `EncounterTrade4RanchGift`, `PCD`/`PGT` (incl. Ranger Manaphy), `EncounterEgg4` | `Gen4LegalityEncounters.cpp` |
| `EncounterGenerator3` and the Gen III templates for a format-4 entity (Pal Park), `MethodH`, `CommonEvent3Checker`, `MystryMew`, `ChannelJirachi` | `Gen4LegalityPalPark.cpp` |
| `EvolutionTree.Evolves4`/`Evolves3`, `EvolutionChain`, `LearnVerifier`, `LearnGroup4`/`LearnGroup3`, `LearnSource4DP/Pt/HGSS`, `MoveBreed4`, `EvolutionRestrictions.IsValidEvolutionWithMove` | `Gen4LegalityMoves.cpp` |
| `EncounterFinder.FindVerifiedEncounter`, `EncounterVerifier`, `EvolutionVerifier`, `LegalityAnalysis.UpdateChecks` (format 4; `ParsePK3` for a Gen III origin) | `Gen4Legality.cpp` (`findVerifiedEncounter`, `run`) |

The generators are evaluated eagerly. PKHeX's finder only peeks one element ahead, so the eager
list, the ManualFlag (InvalidFrame / InvalidPIDIV) and the final wild lead are the same as in
PKHeX's lazy sequence; the ManualFlag counts only when the last-resort element was requested.

## 2. Verifiers (LegalityAnalysis.UpdateChecks order)

| PKHeX verifier | Analyzer | Severity |
|---|---|---|
| EncounterFinder hints, EncounterVerifier | `findVerifiedEncounter`, `verifyEncounter` | Invalid as PKHeX; InvalidFrame is Invalid for a Gen IV wild slot and Fishy for a Gen III one (FramePatternSettings) |
| PIDVerifier | `verifyPid` | Invalid as PKHeX (Wurmple, egg PID bit 15, Day Care PID, shiny locks, PCD PID); PID zero Fishy |
| LanguageVerifier | `verifyLanguage` | Invalid |
| NicknameVerifier | `verifyNickname`, `verifyNicknameEgg` | Invalid as PKHeX; nickname equal to a species name, a foreign nickname length Fishy |
| TrainerNameVerifier | `verifyTrainerName` | Invalid; a suspicious OT Fishy |
| TrainerIDVerifier | `verifyTrainerId` | Ruby/Sapphire TrainerIDNoSeed Invalid (Fishy for a hatched Gen III egg); zero / equal / placeholder IDs Fishy |
| IndividualValueVerifier | `verifyIvs` | Invalid as PKHeX; all IVs equal Fishy |
| EffortValueVerifier (VerifyGainedEVs34) | `verifyEvs` | Invalid as PKHeX; all EVs equal Fishy; EffortEXPIncreased and Effort2Remaining Info (section 3) |
| LevelVerifier | `verifyLevel` | Invalid as PKHeX; LevelEXPThreshold Info (section 3) |
| RibbonVerifier (Event3, Event4, Unique3, Common3, Unique4, Common4) | `verifyRibbons` | Invalid |
| AbilityVerifier (VerifyAbilityPreCapsule, VerifyAbilityGen3Transfer) | `verifyAbility` | Invalid |
| BallVerifier | `verifyBall` | Invalid |
| FormVerifier | `verifyForm` | Invalid (incl. the Unown letter of a Gen III origin) |
| MiscVerifier (eggs, MiscVerifierG4 Pal Park fields and split ball, fateful, Pokerus, dates) | `verifyMisc` | Invalid; walking mood on an egg Fishy |
| MovePPVerifier | `verifyMovePP` | Invalid |
| GenderVerifier | `verifyGender` | Invalid |
| ItemVerifier | `verifyItem` | Invalid |
| ContestStatVerifier (Gen III and Gen IV sheen formulas) | `verifyContest` | Invalid |
| MarkingVerifier, GroundTileVerifier | `verifyMarking`, `verifyGroundTile` | Invalid |
| LearnVerifier | `verifyMoveResults` | Invalid; Arceus with Draco Meteor Fishy (section 3) |
| TransferVerifier.VerifyTransferLegalityG3 | `run` | Invalid: a Gen III origin must have been met at Pal Park (55) |

## 3. Severity policy and deliberate differences

Every difference is more lenient than PKHeX (never a new Invalid):

| Case | PKHeX | Here | Evidence |
|---|---|---|---|
| Fishy results other than the ones below | Fishy, still VALID | Warning: the record is Incomplete, never No problems found | PKHeX's own grading |
| EffortEXPIncreased (levels gained, no EVs) | Fishy (EffortValueVerifier.cs:50-51) | Info | Rare Candy and the Day Care add EXP and no EVs: pokeplatinum src/item_use_pokemon.c:291-293, src/overlay005/daycare.c:221-224; pokeheartgold src/use_item_on_mon.c:294-296, src/get_egg.c:147-150 |
| LevelEXPThreshold (EXP exactly at a level's minimum) | Fishy (LevelVerifier.cs:56-57) | Info | A Rare Candy adds exactly the EXP to the next level: pokeplatinum src/item_use_pokemon.c:291-293 with src/pokemon.c:2313-2319; pokeheartgold src/use_item_on_mon.c:294-296 |
| Effort2Remaining (EV total 508) | Fishy (EffortValueVerifier.cs:52-53) | Info | Battle EV gain stops only at 510: pokeplatinum src/battle/battle_script.c:10237, include/constants/pokemon.h:15-16; pokeheartgold include/constants/pokemon.h:9 |
| Arceus that knows Draco Meteor | Invalid (PersonalInfo4.SpecialTutorDracoMeteor omits Arceus) | Warning | The Pt and HG/SS tutors check the party Pokemon's current type, and Arceus takes its Plate's type: pokeplatinum res/field/scripts/scripts_route_210_grandma_wilma_house.s:79-81 and 129-132, src/pokemon.c:1221-1222; pokeheartgold files/fielddata/script/scr_seq/scr_seq_0948_T30R0601.s:251-252, src/pokemon.c:862-863. The other Ultimate-move tutors are species-based (pokeplatinum res/field/scripts/scripts_route_228_north_house.s:96) and unchanged |
| Checksum or sanity word does not match the data | not checked by the legality analysis | Warning, never certified | The games load such a record as a Bad Egg |
| Colosseum/XD origin (version 15) | judged with the Orre encounter tables | Warning, encounter not verified | The Orre tables are not ported |
| A word-filter pattern outside the supported regular-expression subset | matched | never matches | Can only make the filter more lenient |

## 4. Older Gen IV checks in Legality::analyze

`analyze()` keeps its older Gen IV evidence (Info notes on wild slots, statics, trades, special
balls, Pal Park move stages and contest ribbon ranks). One of its Warnings yields to the analyzer:
"Move is outside the audited direct/transfer pool for this exact Gen IV game". The audited pools
(`include/Integration/Gen4/Gen4MoveCompatibility.h`, PokeAPI default formes plus pre-evolutions)
cannot hold the moves an encounter itself supplies (Mystery Gift and Pokewalker moves, Smeargle's
Sketch, a Pal Park transfer's Generation III moves). When the analyzer matched an encounter for the
record, found nothing Invalid and verified every move against that encounter and the evolution
chain, `applyGen4Analyzer` turns that Warning into an Info note naming how the move was verified;
otherwise the Warning stays. No Invalid is changed. The "Met location/level does not match the
audited encounter table" Warning is not emitted for a Gen IV save (its Gen IV branch adds Info only,
judged by the record's stored origin game).

Disagreements between the audited pools and PKHeX found by comparing every species, forme and move
(each pool unioned over D/P, Pt and HG/SS and the pre-evolution chain):

| Pool | Audited (PokeAPI) | PKHeX and the games | Evidence |
|---|---|---|---|
| Deoxys Attack/Defense/Speed formes | Normal forme only | Every forme's moves (formes change in Pt/HG/SS): Superpower, Zap Cannon; Spikes, Iron Defense, Amnesia, Counter, Mirror Coat; Agility, Extreme Speed | pokeplatinum res/pokemon/deoxys/forms/attack/data.json:44,48,114; defense/data.json:42,47-48,51-52; speed/data.json:47,50 |
| Shaymin Sky Forme | Land Forme only | Quick Attack, Air Slash, Leaf Storm, Ominous Wind, Air Cutter, Tailwind | pokeplatinum res/pokemon/shaymin/forms/sky/data.json:40,44,47,81,83; pokeheartgold files/fielddata/wazaoshie/waza_oshie.json:7733-7744 |
| Giratina Origin Forme | Altered Forme only | HG/SS tutors Tailwind and Magic Coat | pokeheartgold files/fielddata/wazaoshie/waza_oshie.json:7710,7726,7728 |
| Wormadam Sandy/Trash Cloak | Plant Cloak pool for every cloak | Each cloak's own moves (cloaks never change): Sandy Rock Blast, Harden, Fissure, Earthquake, Dig, Sandstorm, Rock Tomb, Mud-Slap, Rollout, Earth Power; Trash Mirror Shot, Metal Sound, Iron Head, Gyro Ball, Stealth Rock, Flash Cannon, Gunk Shot, Iron Defense, Magnet Rise; Plant-only moves (Razor Leaf, Growth, Leaf Storm, Solar Beam, Energy Ball, Grass Knot, Synthesis, ...) are not learnable by the other cloaks | pokeplatinum res/pokemon/wormadam/forms/sandy/data.json:42-43,49,60,62,66-67,86-87,91; trash/data.json:42-43,49,73-74,82,85,89-91; res/pokemon/wormadam/data.json:42-43,49,62,73,82,89 |
| Nosepass/Probopass Head Smash | learnable | not learnable in any Gen IV game | pokeplatinum res/pokemon/nosepass/data.json:36-51 (level-up), 99-105 (egg moves); PKHeX's D/P/Pt/HG/SS learnsets |
| Arceus Draco Meteor | learnable | PKHeX: not learnable; the games: learnable while holding a Draco Plate | section 3 (the analyzer follows the games, as a Warning) |

Where the audited pool is too narrow, the yield above removes the false Warning once the analyzer
has verified the move; where it is too wide (Wormadam's cloaks, Head Smash) the analyzer's own
Invalid applies, as in PKHeX.

## 5. Differential test

`tools/pkhex_gen34_diff/run.sh PKHEX_CHECKOUT SEED MUTATIONS 34|4 base|grown` compares every
verdict with PKHeX (base: every Gen IV encounter template plus single-field mutations; grown:
PKHeX-valid records played forward, traded between carts and moved through Pal Park, plus
mutations). It fails on any false Invalid (PKHeX VALID, Invalid here), false Valid (PKHeX INVALID,
No problems found here) or unloadable record. CI runs seeds 1-3 for both corpora
(`.github/workflows/legality-gen34-pkhex-diff.yml`).
