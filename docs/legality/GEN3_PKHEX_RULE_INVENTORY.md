# PKHeX Gen III (PK3) legality rule inventory

Reference for the Gen III analyzer (`src/Legality/Gen3Legality.cpp`, `include/Legality/Gen3Legality.h`).
It is a port of PKHeX.Core at kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722 (GPL-3.0) for
Pokemon whose origin is Ruby, Sapphire, Emerald, FireRed or LeafGreen. The tables it reads
(`Gen3LegalityData.{h,cpp}`) are generated from that checkout by `tools/gen_legality_gen3_data.py`
(`--check` verifies the committed output). Decomp evidence below cites pret/pokeemerald@731ad5bf
and pret/pokefirered@037335f4.

The analyzer only reads a decrypted 80- or 100-byte record and returns a `Legality::Report`.
Production calls it once, from `Legality::analyze` (`applyGen3Analyzer`, just before the final
`return r;`), for records whose save group is FRLG (every PK3 save).

## 1. Pipeline (PKHeX class -> analyzer section)

| PKHeX.Core | Gen3Legality.cpp |
|---|---|
| `PKM/PK3.cs` accessors, `StringConverter3`, `SpeciesConverter` | `read`, `decodeString`, "Personal data, experience and simple PK3 derivations", "Strings" |
| `Legality/RNG/MethodFinder.cs` (Methods 1-4 and their Unown and roamer forms, the BACD family, Cute Charm, Channel, CXD, PokeSpot) | `analyzePidIv`, "PID/IV correlation" |
| `CommonEvent3Checker`, `MystryMew`, `ChannelJirachi`, event seed revision | "Event gift seed revision" |
| `EvolutionGroup3`, `EvolutionChain`, `EvolutionUtil` | "Evolution chains" |
| `LearnVerifier`, `LearnGroup3`, `MoveBreed3`, `LearnSource3*` | "Moves" (`verifyMoves`, `reportMoves`) |
| `EncounterSlot3`, `EncounterSlot3Swarm`, `EncounterStatic3`, `EncounterTrade3`, `EncounterGift3`, `EncounterGift3NY`, `EncounterGift3JPN`, `EncounterGift3Colo`, `EncounterEgg3` | "Encounter templates" |
| `EncounterGenerator3`, `EncounterEnumerator3` (template order, PID/IV type filter, ball filter, lead queue and deferral) | "EncounterEnumerator3", "EncounterGenerator3" |
| `MethodH`, `SlotMethodH`, `LeadFinder` (wild slot lead/frame search) | "Method H" |
| `EncounterFinder.FindVerifiedEncounter`, `EncounterVerifier`, `EvolutionVerifier` | `findEncounter`, `verifyEncounter3`, `verifyEvolution` |
| `SpeciesName.TryGetSpeciesAnyLanguage[CaseInsensitive]`, `WordFilter.IsFiltered` | "Species-name dictionaries", "Word filter" (`isWordFiltered`) |
| `LegalityAnalysis.UpdateChecks` (Legality/LegalityAnalysis.cs:315-344) | `runChecks` (same order) |

The finder loop is replicated with peek tracking: the manual-flag fallback (InvalidFrame -> Fishy,
InvalidPIDIV -> Invalid) applies only when the last-resort candidate was actually fetched, and every
Gen III template except `EncounterEgg3` is an `IEncounterMatch`, so an egg match ends the loop as it
does in PKHeX. Eager generation is equivalent to PKHeX's lazy generator because the fetch position
and the gift `Mutated` state are tracked explicitly. Seed-reversal loops that PKHeX leaves unbounded
are capped at 2^22 steps; reaching the cap is reported as Unknown (Warning), never Invalid.

## 2. Verifiers (LegalityAnalysis.UpdateChecks order)

| PKHeX verifier | Analyzer | Results (I = Invalid, F = Fishy, N = reported as Info) |
|---|---|---|
| EncounterFinder hints | `findEncounter` | EncInvalid, EncGiftNotFound, EncGiftEggEvent, EggLocationInvalid, EggMetLocationFail, EggFMetLevel_0, EncUnreleasedEMewJP, FatefulGiftMissing (I); EncConditionBadRNGFrame (F) |
| PIDVerifier (PIDEC) | `verifyPid` | PIDTypeMismatch, Wurmple evolution PID, Day Care egg PID, PIDGenderMismatch (bred egg PID bit 15), EncStaticPIDShiny / shiny lock (I); PID zero (F) |
| LanguageVerifier | `verifyLanguage` | OTLanguageShouldBeLeq_0 (I) |
| NicknameVerifier | `verifyNickname`, `verifyWordFilter` | NickLengthLong, empty nickname, characters a Gen III game cannot enter, EncTradeChangedNickname, egg name and flag (NickMatchLanguageEggFail, FirstWithLanguage), WordFilterFlaggedPattern (I); NickMatchNoOthersFail (F) |
| TrainerNameVerifier | `verifyTrainerName` | OTLong, empty OT, EncTradeChangedOT, Colosseum gift OT (I); an OT starting with "PKHeX" (F) |
| TrainerIDVerifier | `verifyTrainerId` | TrainerIDNoSeed for Ruby/Sapphire (I; F for a hatched egg, as PKHeX grades it); OT_IDs0, OT_IDEqual, OT_TID0, OT_SID0, the two well-known placeholder TID/SID pairs (F) |
| IndividualValueVerifier | `verifyIvs` | IVAllEqual_0 (F) |
| EffortValueVerifier (VerifyGainedEVs34) | `verifyEvs` | EffortEgg, EV total above 510, EffortCap100, EffortUntrainedCap_0 (I); EffortAllEqual (F); EffortEXPIncreased, Effort2Remaining (N) |
| LevelVerifier | `verifyLevel` | EggFMetLevel_0, EggEXP, LevelEXPTooHigh, LevelMetBelow (I); LevelEXPThreshold (N) |
| RibbonVerifier | `verifyRibbons` | missing / unobtainable ribbons (FirstComplex) (I) |
| AbilityVerifier | `verifyAbility` | AbilityMismatch, AbilityMismatchFlag, AbilityMismatchPID, in-game trade ability (I) |
| BallVerifier | `verifyBall` | BallEncMismatch (I) |
| MiscVerifier (+ trash bytes) | `verifyMisc`, `verifyTrash` | EggHatchCycles, EggContest, egg language, EncGiftShinyMismatch, FatefulMissing, FatefulInvalid, PokerusDaysLEQ_0, TrashBytesShouldBeEmpty, TrashBytesMissingTerminatorFinal, nickname trash (I) |
| MovePPVerifier | `verifyMovePP` | EggPPUp, EggPP, MovePPUpsTooHigh_01, PP above maximum (FirstWithTwoArguments) (I) |
| GenderVerifier, FormVerifier | (none) | PK3 stores neither: gender and the Unown letter derive from the PID, so PKHeX's checks cannot fail independently; the PID rules above cover them |
| ItemVerifier | `verifyItem` | ItemEgg, ItemUnreleased (I); Enigma Berry, see 3 |
| ContestStatVerifier | `verifyContest` | sheen above its ceiling (ContestSheenLEQ_0) or below its floor (FirstWithArgument) (I) |
| MarkVerifier | `verifyMarking` | unused marking bits (I) |
| LearnVerifier (moves) | `reportMoves` | Duplicate, EmptyInvalid, Unobtainable, UnobtainableExpect (I) |
| EvolutionVerifier | `verifyEvolution` | EvoInvalid (I) |

Every PKHeX Invalid code the differential corpus produces for Gen III (53 code families over
seeds 1-4, 1-25 mutations) is reported Invalid here, except `HeldItem:ItemUnreleased` on an
Enigma Berry (section 3).

## 3. Severity policy and deliberate differences

All differences are more lenient than PKHeX (never a new Invalid):

| Case | PKHeX | Here | Evidence |
|---|---|---|---|
| Fishy result (e.g. EncConditionBadRNGFrame, OT_IDs0, OT_SID0, TrainerIDNoSeed when Fishy, IVAllEqual_0, EffortAllEqual, NickMatchNoOthersFail) | Fishy, still VALID | Warning: the record is Incomplete, never No problems found | PKHeX's own grading; the corpus shows a one-to-one match (no Warning here without a PKHeX Fishy) |
| EffortEXPIncreased (EXP gained, no EVs) and LevelEXPThreshold (EXP exactly on a level boundary) | Fishy (EffortValueVerifier.cs:49-51, LevelVerifier.cs:57) | Info | Rare Candy sets EXP to exactly the next level's threshold and gives no EVs (pokeemerald src/pokemon.c:4918-4925, pokefirered src/pokemon.c:4164); the Day Care adds steps to EXP with no EVs (pokeemerald src/daycare.c:255-256 and 290-291, pokefirered src/daycare.c:520-521) |
| Effort2Remaining (EV total 508) | Fishy (EffortValueVerifier.cs:52-53) | Info | Battle EV gain is clamped only at 510 total / 255 per stat (pokeemerald src/pokemon.c:5988-6052 MonGainEVs, include/constants/pokemon.h:206-207; pokefirered include/constants/pokemon.h:234), so every total up to 510, 508 included, is reachable |
| Enigma Berry (item 175) | Invalid ItemUnreleased when no save is loaded (ItemVerifier.cs:32-48, EReaderBerryMatch.NoData) | Warning | The berry's data lives in the save (`gSaveBlock1Ptr->enigmaBerry`, pokeemerald src/berry.c:969-978 IsEnigmaBerryValid; pokefirered src/berry.c:984); a record alone cannot decide it |
| Stored checksum does not match the data | not checked by the legality analysis | Warning, never certified | The game marks such a record a Bad Egg on load (pokeemerald src/pokemon.c:3742-3746) |
| Internal species 0 with other data present | EncInvalid | Warning ("holds no species") | Real empty slots are all zero; anything else is a damaged record, not a judged Pokemon |
| Internal species index with no national species (unused "?" slots, >= 412) | EncInvalid | Invalid | Same as PKHeX |
| Colosseum/XD origin (version 15) or any other non-GBA origin game | judged with PKHeX's GameCube or other logic | Warning, Gen III cartridge rules not run | Not this analyzer's scope; the existing Colosseum/XD evidence in Legality.cpp still runs |
| RNG frame search reaches its cap | (unbounded) | Warning (Unknown) | Never observed in the corpus |
| A word-filter pattern the matcher cannot evaluate | regex | Warning (Unknown) | The pinned lists have none (`unsupportedWordFilterPatterns() == 0`, asserted by the tests) |
| PK3 flag bits (bad egg, has species, egg) | ignored by PKHeX | ignored | Same as PKHeX |

The word filter applies to the nickname only when it is nicknamed and passed the species, CJK and
length checks, and to the OT only for a player-OT encounter (not an in-game trade, PCNY/PCJP gift,
Colosseum gift or event with a fixed OT). PKHeX's quirk in `IsNicknameLanguageEvolution`
(a name that matches no language counts as nicknamed in every language) is replicated literally.

## 4. Certification (production merge rule)

`applyGen3Analyzer` adds the analyzer's Invalid and Warning issues to `r` (deduplicated by text)
and sets all nine coverage areas Complete only when all of these hold:

1. the analyzer found a legitimate origin, checked every area and found nothing wrong;
2. `r` itself has no Invalid and no Warning;
3. the exact source game is `ruby_gba`, `sapphire_gba`, `emerald_gba`, `firered_gba` or
   `leafgreen_gba`. An empty id (including one `Legality::analyze` sanitized because it named
   another generation) or a non-GBA id never certifies.

Otherwise `r`'s coverage is left as it was (Partial). Pal Park is not handled here.

## 5. Measurement (tools/pkhex_gen34_diff)

`run.sh PKHEX SEED MUTATIONS 3` diffs every Gen III encounter PKHeX can build plus seeded
single-field mutations; `GROW=1 run.sh PKHEX SEED GROWTHS` diffs the grown corpus
(`oracle/Grow3.cs`: battle EXP or Rare Candy, level-up moves from the origin or receiving game,
EV training, level/stone/trade/friendship evolution with the game's rename of an unnicknamed
Pokemon, held items, trades into another cartridge's save; every record stays PKHeX-VALID).
`summarize.py` fails on any false Invalid, false Valid or load failure and prints per-game rates.
The oracle's PIDs come from PKHeX's unseeded generator, so record counts vary slightly per run.

Production path (`Legality::analyze` on an encrypted `Pokemon3FRLG`), 3 mutations:

| Seed | False Invalid | False Valid | PKHeX-invalid caught | No problems found (PKHeX-valid) |
|---|---|---|---|---|
| 1 | 0 | 0 | 14325/14329 (99.97%) | 13745/14507 (94.75%) |
| 2 | 0 | 0 | 14433/14435 (99.99%) | 13598/14401 (94.42%) |
| 3 | 0 | 0 | 14343/14347 (99.97%) | 13691/14489 (94.49%) |

Per game (seed 1): R 2615/2843, S 2542/2686, E 2699/2804, FR 2966/3109, LG 2923/3065 certified;
every game 0 false Invalid and 0 false Valid. The uncaught PKHeX-invalid records are all Enigma
Berry holders (section 3).

Grown corpus, production path, 3 growths per base record (all PKHeX-VALID):

| Seed | Records | False Invalid | No problems found |
|---|---|---|---|
| 1 | 19254 | 0 | 14732 (76.51%) |
| 2 | 19256 | 0 | 14765 (76.68%) |
| 3 | 19221 | 0 | 14704 (76.50%) |

The analyzer alone (`analyzePk3`) certifies 95.8-95.9% of PKHeX-valid mutation records (every
uncertified one carries a PKHeX Fishy result) and 98.5% of grown records. A stress run with 25
mutations per encounter (187434 records) also gave 0 false Invalid and 0 false Valid.

## 6. Known gaps

- Older checks in `Legality.cpp` add Warnings of their own that block certification of records the
  analyzer certifies: "Met location/level does not match the audited encounter table" (it looks up
  the current species in the tables of the save's cartridge, so evolved Pokemon and Pokemon traded
  into another game's save warn), "Move is not native to this exact Gen III game" and "Move is
  preserved but not in the audited Gen III native/transfer pool". They account for the gap between
  the analyzer's 98.5% and production's 76.5% on the grown corpus.
- `Legality::analyze` returns before the hook when the record's species reads as 0, so an
  internal species index with no national species shows no issues (Incomplete) in production,
  while `analyzePk3` reports it Invalid as PKHeX does.
- Colosseum/XD origins are not judged by this analyzer (Warning).
