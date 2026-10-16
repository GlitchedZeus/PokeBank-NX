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

`Legality::analyze` runs the analyzer once (`Gen3Pk3::verify`, right after the source game is
resolved) and merges that result through the same rule at the end. The `species == 0` early
return merges it too, so a PK3 whose internal species index has no National Dex number is
reported as Invalid there, as PKHeX does (`EncInvalid`). An empty PK3 slot (internal index 0) and
every other format are untouched.

### 4a. The older Gen III checks in `Legality.cpp`

Two older checks run on every PK3 that has an exact GBA source:

- the L1 encounter table (`EncounterGuardrailsData.inc`, "Met location/level does not match
  the audited encounter table");
- the pret move pools (`Gen3LearnsetTable.h`, "Move is not native ..." and "Move is preserved
  but not in the audited Gen III native/transfer pool").

Before this change they held thousands of records at Incomplete that the analyzer certifies.
The counts below come from the stored seed 1-3 corpora. "Grown" is 57,731 records, all
PKHeX-valid: the analyzer certified 56,734 and production 44,201. "Mutation" is 86,508
records: the analyzer certified 41,682 and production 41,034. The mutation figures use the base
record's PKHeX label.

| Root cause of the older Warning | Grown | Mutation | Fix |
|---|---|---|---|
| Evolved: the current species is not the encounter species, and the table was searched for the current species only | 7179 | 5 | The table is now searched for every species of the Gen III lineage (`Gen3Pk3::lineage`) |
| The origin game differs from the save cartridge: the check used `exactSourceGameId`, not the version byte | 2997 | 0 | The table is now searched in the origin game's table (`Gen4Origin::exactGen3GameId(pk.originGame())`) |
| Both of the above | 1705 | 0 | Both fixes |
| Encounter kind absent from L1: event gifts (met location 255) | 345 | 322 | Info when the analyzer certified the record (see below) |
| Encounter kind absent from L1: in-game trades (met location 254) | 154 | 115 | Info when certified |
| In-game trade that PKHeX matches at another met location (the mutated met location of a trade) | 0 | 9 | Kept as a Warning, with its own text (section 4b) |
| L1 omission, wild: mass-outbreak slots | 37 | 50 | Info when certified; decomp evidence below |
| L1 omission, static: FireRed Game Corner prizes, Ruby Electrode and Latias | 31 | 26 | Info when certified; decomp evidence below |
| Move native to the origin game but not to the cartridge ("transfer") | 75 | 0 | A move counts as native when it is native to the origin game or to the cartridge |
| Move native to another GBA game only (traded there and back) | 0 | 12 | Info when certified |
| Move outside every pret pool: event-gift moves (PCNY/PCJP/WISHMKR eggs and gifts) | 29 | 169 | Info when certified |
| Move outside every pret pool: Sketch, or a slot/egg move PKHeX accepts | 0 | 4 | Info when certified |

The Colosseum/XD or unknown version bytes have no handheld table, so the encounter check is
skipped for them. The analyzer reports such an origin as a Warning in any case.

"Info when certified" means the following. The check still runs, and its finding is kept as an
Info naming the analyzer's match, for example:

> Met location/level does not match the audited encounter table for firered_gba; the Gen III
> analyzer matched in-game trade Nidoran F at met location 254, level 5

This happens only when the analyzer certified the record: an encounter matched for this exact
record, every move and every other rule verified, and a GBA source. Any analyzer Warning or
Invalid (for example an Enigma Berry) keeps the older finding as a Warning, and
`tests/test_legality_gen3.cpp` checks this. No Invalid changed severity, and no direct
`Severity::Invalid` site was added.

After the fix, production certifies the analyzer's records on all six stored corpora:

- Grown: 56,734 of 56,734.
- Mutation: 41,668 of 41,682.
- The 14 exceptions are the in-game trades with a mutated met location (section 4b). Five of them
  had been certified before, because their species have no L1 row in their origin game.
- False Invalid and false Valid stay 0.

### 4b. L1 encounter table compared with PKHeX

The L1 table's own header names PKHeX commit 08c27668 (BinLinker `encounter_*.pkl` plus
`Encounters3RSE.cs` and `Encounters3FRLG.cs`) as its source, not pret. It was compared row by row
with the pinned PKHeX data (every slot, static and trade the analyzer uses) for each game,
species and location:

- No L1 row is missing from PKHeX.
- 458 keys differ only because L1 stores a min-max span where PKHeX has separate slot levels.
  L1 is more lenient there, so no false Warning results.
- Each case below is a PKHeX encounter that L1 lacks. In every one the decomp shows that PKHeX is
  right.

The decomp revisions used:
- pokeruby 5784633c, cloned to /home/user/ext/pokeruby for this check;
- pokeemerald 731ad5bf;
- pokefirered 037335f4.

Met locations are the `MAPSEC_*` indices in each decomp's `region_map_sections.json`.

| Game(s) | Species, met location, level | L1 | Decomp evidence |
|---|---|---|---|
| R, S | Surskit 32 (Route 117) lv 15 | has 13, 20-30 only | pokeruby src/tv.c:103-108, the R/S mass outbreak list |
| E | Surskit 17, 29, 32, 35 at lv 3, 15, 15, 28 | none | pokeruby src/tv.c:91-114. Emerald receives R/S TV shows by record mixing: pokeemerald src/record_mixing.c:262, src/tv.c:3449 and 3466-3467 (TranslateRubyShows) |
| R, S, E | Skitty 31 (Route 116) lv 15 | has 7-8 only | pokeruby src/tv.c:115-120 (R/S); Emerald by record mixing as above |
| R, S | Nuzleaf 29 (Route 114) lv 15 | R has 16-18, S has none | pokeemerald src/tv.c:204-209, the Emerald mass outbreak list. R/S receive Emerald shows: pokeruby src/record_mixing.c:88. Emerald keeps outbreaks when sanitizing shows for Ruby: pokeemerald src/tv.c:4031 and 4171-4172 |
| R, S | Seedot 32 lv 13 and 35 lv 25; S also 17 lv 3 | none | pokeemerald src/tv.c:198-221; record mixing as above |
| FR | Clefairy, Abra, Dratini, Scyther, Porygon at 94 (Celadon City), lv 8, 9, 18, 25, 26 | LG levels only | pokefirered data/maps/CeladonCity_GameCorner_PrizeRoom/scripts.inc:124, 135, 146, 156, 162 (the `.ifdef FIRERED` branches) |
| FR | Deoxys 187 (Birth Island) lv 30 | LG only | pokefirered data/maps/BirthIsland_Exterior/scripts.inc:83, with no version guard |
| R | Electrode 66 (Evil Team Hideout) lv 30 | S only | pokeruby data/scripts/static_pokemon.inc:4 and 19, used by AquaHideout_B1F and MagmaHideout_B1F (map.json:89 and 115) |
| R | Latias 73 (Southern Island) lv 50 | S Latios only | pokeruby data/maps/SouthernIsland_Interior/scripts.inc:64 and constants/version.inc:27-31 (Ruby: Latias) |
| R | Groudon 72 (Cave of Origin) lv 45 | S Kyogre only | pokeruby data/maps/CaveOfOrigin_B4F/scripts.inc:57 and constants/version.inc:21-25 |
| R | Latios, roaming, lv 40 | S Latias only | pokeruby src/roamer.c:65 and include/constants/species.h:1282-1286 (Ruby: Latios) |

Roaming legendaries in all five games:
- L1 lists one met location per roamer (16 for Hoenn, 101 for Kanto).
- The game picks the roamer's location from a list of routes (pokeruby src/roamer.c:18
  `sRoamerLocations`; pokeemerald src/roamer.c:35), and PKHeX matches any of them.
- A roamer met elsewhere is therefore the same L1 omission.

One case goes the other way, and the decomp shows that PKHeX is too lenient:

- `EncounterTrade3.IsMatchExact` matches an in-game trade without checking the met location. It
  checks the PID, IVs, trainer ID, gender and contest stats.
- Every game stores met location 254 for an in-game trade:
  - pokeemerald src/trade.c:4555 and 4577 (`METLOC_IN_GAME_TRADE` = 0xFE, from
    src/data/region_map/region_map_sections.constants.json.txt:15);
  - pokeruby src/trade.c:4948 and 4970;
  - pokefirered src/trade_scene.c:2461 and 2481.
- The met level is the level of the player's Pokemon that was traded away (the same lines), which
  PKHeX allows as well.
- When the analyzer matched an in-game trade and the met location is not 254,
  `Legality::analyze` adds the Warning "Met location N does not fit the in-game trade the Gen III
  analyzer matched ...". This holds whether or not the L1 table has rows for the species.
- It stays a Warning, never Invalid, because PKHeX accepts the record. The analyzer itself keeps
  PKHeX's verdict.

Not changed:
- The older Gen III egg-move evidence still keys on the cartridge (`eggEvidenceGameId`). It only
  adds Info, so it never blocks certification.
- The L1 table itself is not edited. It also drives editor guardrails outside legality.

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

- The older checks in `Legality.cpp` and the `species == 0` early return no longer block the
  analyzer's certification (section 4a). The only exception is the in-game trade met-location
  Warning, which is backed by the decomp (section 4b).
- Colosseum/XD origins are not judged by this analyzer (Warning).
