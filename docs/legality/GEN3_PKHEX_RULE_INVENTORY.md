# PKHeX Gen III (PK3) legality rule inventory

Reference for the Gen III analyzer (`src/Legality/Gen3Legality.cpp`, `include/Legality/Gen3Legality.h`).
It is a port of PKHeX.Core at kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722 (GPL-3.0) for
Pokemon whose origin is Ruby, Sapphire, Emerald, FireRed or LeafGreen, and (section 7) for
Pokemon whose origin is Colosseum or XD (version 15, `GameVersion.CXD`) held in any of those
saves. The tables it reads (`Gen3LegalityData.{h,cpp}`) are generated from that checkout by
`tools/gen_legality_gen3_data.py`; the GameCube tables (`Gen3LegalityDataCxd.{h,cpp}`: shadow,
starter, gift, trade and Poke Spot templates, NPC team locks) by
`tools/gen_legality_gen3_cxd_data.py --pkhex PKHEX` (`--check` verifies the committed output of
each). Decomp evidence below cites pret/pokeemerald@731ad5bf and pret/pokefirered@037335f4.

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
| `EncounterShadow3Colo`, `EncounterShadow3XD`, `EncounterStatic3Colo` (starters), `EncounterGift3Colo` (CXD Plusle), `EncounterStatic3XD`, `EncounterTrade3XD`, `EncounterSlot3XD` / `EncounterArea3XD` (Poke Spots), `Encounters3Colo`, `Encounters3XD` | "Encounter templates" GameCube cases (`makeShadowEnc`, `makeColoStarterEnc`, `makeColoGiftCxdEnc`, `makeXdStaticEnc`, `makeXdTradeEnc`, `makeXdSlotEnc`), tables in `Gen3LegalityDataCxd.cpp` |
| `EncounterGenerator3GC`, `EncounterEnumerator3GC` (trades, Colosseum shadows, starters, gift, XD shadows, XD gifts, e-Reader, Poke Spots, deferred) | `enumerateEncountersGC`, `generateEncountersGC` |
| `LockFinder`, `TeamLockResult`, `NPCLock`, `TeamLock` (shadow team lock reversal) | `TeamLockSearch`, `shadowLockValid`, `eReaderLockValid` |
| `MethodCXD` (name screen, trainer ID, starters), `MethodPokeSpot`, `XDRNG` | `isValidNameScreenEndSeed`, `cxdTrainerIdHasSeed`, `coloStarterSeed`, `xdStarterSeed`, `pokeSpotOrigin*`, `xdrng*` |
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
| CXDVerifier (ParsePK3 runs it for Version CXD) | `verifyCxd` | XD starter Eevee PID/IV seed, female OT (I) |
| TrainerIDVerifier.VerifyTrainerID_CXD | `verifyTrainerId` | TID/SID not reachable from the name screen (I; F for an English XD encounter; skipped above English) |
| RibbonRules (National, Earth), MiscVerifier fateful for XD, BallVerifier fixed balls, AbilityVerifier CXD CanMismatch, NicknameVerifier.VerifyTrade for XD trades and the Colosseum gift, EncounterVerifier G3EReader | `verifyRibbons`, `verifyFateful`, `verifyBall`, `verifyAbility`, `verifyNickname` / `verifyTrainerName`, `findEncounter` | see section 7 |

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
| Colosseum/XD origin (version 15) | EncounterGenerator3GC and the verifiers of section 7 | the same (one verdict path: `Legality.cpp` no longer adds its own GameCube evidence for a PK3 the analyzer judged) | Section 7 |
| Other non-GBA origin game byte (0, 6-14) | EncInvalid | Warning, Gen III cartridge rules not run | Not a Gen III cartridge value; the record may be damaged rather than a judged Pokemon |
| English XD encounter whose TID/SID has no name-screen seed | Fishy (`TrainerIDVerifier.cs:86-101`, PAL copies skip the ball sequence) | Warning | PKHeX's own grading |
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

The Colosseum/XD or unknown version bytes have no handheld table, so the L1 encounter check is
skipped for them. A Colosseum/XD origin is judged by the analyzer (section 7); an unknown byte is
reported as a Warning.

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

Production path (`Legality::analyze` on an encrypted `Pokemon3FRLG`). The stored corpora are
the records of the first measurement, re-run through production before and after the section 4a
fix, so both columns judge the same records:

| Corpus | Records | False Invalid | False Valid | PKHeX-invalid caught | No problems found (PKHeX-valid), before -> after |
|---|---|---|---|---|---|
| Mutation seed 1 | 28836 | 0 | 0 | 14325/14329 (99.97%) | 13745 (94.75%) -> 13949/14507 (96.15%) |
| Mutation seed 2 | 28836 | 0 | 0 | 14433/14435 (99.99%) | 13598 (94.42%) -> 13814/14401 (95.92%) |
| Mutation seed 3 | 28836 | 0 | 0 | 14343/14347 (99.97%) | 13691 (94.49%) -> 13905/14489 (95.97%) |
| Grown seed 1 | 19254 | 0 | 0 | - | 14732 (76.51%) -> 18920/19254 (98.27%) |
| Grown seed 2 | 19256 | 0 | 0 | - | 14765 (76.68%) -> 18936/19256 (98.34%) |
| Grown seed 3 | 19221 | 0 | 0 | - | 14704 (76.50%) -> 18878/19221 (98.22%) |

Per game, No problems found among PKHeX-valid records, before -> after:

| Corpus | R | S | E | FR | LG |
|---|---|---|---|---|---|
| Mutation 1 | 91.98 -> 95.85 | 94.64 -> 96.17 | 96.26 -> 96.97 | 95.40 -> 96.11 | 95.37 -> 95.73 |
| Mutation 2 | 91.04 -> 95.09 | 93.75 -> 95.28 | 95.95 -> 96.89 | 95.54 -> 96.21 | 95.56 -> 96.08 |
| Mutation 3 | 92.07 -> 95.89 | 93.82 -> 95.59 | 95.72 -> 96.55 | 95.46 -> 96.14 | 95.20 -> 95.68 |
| Grown 1 | 76.04 -> 98.41 | 74.54 -> 97.82 | 75.11 -> 98.06 | 78.06 -> 98.56 | 78.59 -> 98.44 |
| Grown 2 | 75.94 -> 98.04 | 75.40 -> 98.50 | 76.14 -> 98.26 | 77.09 -> 98.32 | 78.66 -> 98.57 |
| Grown 3 | 75.81 -> 98.10 | 76.17 -> 97.78 | 74.91 -> 98.24 | 77.72 -> 98.27 | 77.75 -> 98.65 |

Every game has 0 false Invalid and 0 false Valid. The uncaught PKHeX-invalid records are all
Enigma Berry holders (section 3).

Production now certifies exactly the analyzer's records, apart from the 14 mutated in-game
trades of section 4a. The remaining PKHeX-valid records are uncertified because of the
analyzer's own Warnings, which mirror PKHeX Fishy results:
- no RNG frame produces the wild slot;
- all IVs equal;
- TID/SID zero, or a TID/SID pair Ruby/Sapphire cannot generate;
- a preserved move on a record that has another analyzer Warning.

Fresh `run.sh` / `GROW=1 run.sh` runs at this commit, seeds 1-3. These have new PIDs, so the
counts differ slightly:

| Corpus | Records | False Invalid | False Valid | PKHeX-invalid caught | No problems found |
|---|---|---|---|---|---|
| Mutation seed 1 | 28836 | 0 | 0 | 14317/14321 (99.97%) | 13943/14515 (96.06%) |
| Mutation seed 2 | 28836 | 0 | 0 | 14421/14423 (99.99%) | 13799/14413 (95.74%) |
| Mutation seed 3 | 28836 | 0 | 0 | 14334/14338 (99.97%) | 13893/14498 (95.83%) |
| Grown seed 1 | 19270 | 0 | 0 | - | 18918/19270 (98.17%) |
| Grown seed 2 | 19338 | 0 | 0 | - | 19006/19338 (98.28%) |
| Grown seed 3 | 19280 | 0 | 0 | - | 18951/19280 (98.29%) |

A stress run with 25 mutations per encounter (187434 records, before the section 4a fix) also
gave 0 false Invalid and 0 false Valid.

## 6. Known gaps

- The older checks in `Legality.cpp` and the `species == 0` early return no longer block the
  analyzer's certification (section 4a). The only exception is the in-game trade met-location
  Warning, which is backed by the decomp (section 4b).
- Colosseum/XD origins are judged (section 7). Remaining GameCube gaps: an e-Reader shadow
  (Japanese Colosseum bonus disc) is Incomplete because PKHeX grades its fixed all-zero IVs
  Fishy (`IndividualValueVerifier`, IVAllEqual_0); an English XD TID/SID without a name-screen
  seed is Incomplete (Fishy, section 3); a search that reaches a guard is Unknown (never seen in
  the corpora). The Gen IV (Pal Park) and Gen V (Poke Transfer) analyzers still report the
  GameCube encounter as unverified until they call `Gen3Cxd::judge`
  (`docs/legality/handoff/lane-b-gen4-palpark-cxd-judge.patch`,
  `docs/legality/handoff/lane-c-gen5-transfer-cxd-judge.patch`).

## 7. Colosseum/XD (GameCube) origins

A PK3 with version 15 came from Pokemon Colosseum or Pokemon XD through the link cable
(`CK3.ConvertToPK3`, `XK3.ConvertToPK3`: XD sets the fateful flag on everything it sends and the
National Ribbon on shadows, `XK3.cs:224-241`; Colosseum sets the National Ribbon on shadows,
`CK3.cs:243`). PKHeX judges such a record with `EncounterGenerator3GC` and the verifiers below;
the analyzer runs the same path for a version-15 PK3 in any GBA save (R/S/E/FR/LG: the holding
cartridge does not take part in a GameCube match). A GBA wild record relabelled with version 15
matches no GameCube template and is Invalid ("Unable to match an encounter from the origin
game"), as PKHeX grades it (EncInvalid). No GameCube template is an egg.

### 7a. Rules

| Rule | PKHeX | Analyzer | Behaviour | Fixtures (`tests/test_legality_gen3.cpp`) |
|---|---|---|---|---|
| Template order and filters: XD trades, Colosseum shadows, Colosseum starters, Colosseum gift (Plusle), XD shadows, XD gifts, e-Reader shadows (Japanese only; a match skips the Poke Spots), Poke Spots, deferred | `EncounterEnumerator3GC.cs:1-140`, `EncounterGenerator3GC.cs:30-87` | `enumerateEncountersGC`, `generateEncountersGC`, `findEncounter` | every compatible template in PKHeX order; species/level/location as the template demands | every `colo-*`, `xd-*`, `ereader-*`, `pokespot-*` row |
| Shadow template match (species, level, met location 128 for e-Reader, otherwise the template location; re-battle locations 59/90/91/92/113 for XD shadows) | `EncounterShadow3Colo.cs:42,81,160-182`, `EncounterShadow3XD.cs` IsMatchLocation | `isMatchExact` GameCube cases, `gcLocation`, `gcLevel` | exact for a PK3; for a transferred record (`Gen3Cxd::Input::hasOriginalMet == false`) the location is not compared and the level only needs `evo.LevelMax >= Level` | `colo-shadow-bayleef`, `xd-shadow-zapdos`, `xd-shadow-zapdos@rebattle90`, `@rebattle59` (Valid), `@loc75`, `+metlevel` (Invalid) |
| Shadow team locks: the trainer's party is regenerated backwards from the origin seed; a non-shadow prior lock allows any gap, otherwise the member sits right before, with anti-shiny reroll frames; the CPU trainer's shiny value is checked against each member; XD never yields a shiny shadow | `LockFinder.cs:7-36`, `TeamLockResult.cs:86-215` (GetSingleLock, GetAllLocks, VerifyNPC), `NPCLock.cs` | `TeamLockSearch`, `shadowLockValid` | Invalid when no permutation of the team regenerates (`ShadowLock`) | `colo-shadow-bayleef+pid`, `xd-shadow-zapdos+pid` (Invalid); all base shadows (Valid) |
| e-Reader shadow: origin seeds from `XDRNG.GetSeeds(pid)`, lock check, language forced to Japanese | `EncounterGenerator3GC.cs:77-87`, `EncounterShadow3Colo.cs:42,81`, `EncounterVerifier.cs:24` (G3EReader non-Japanese) | `eReaderLockValid`, `findEncounter` | Invalid for a non-Japanese record; the fixed all-zero IVs stay Fishy (Warning) | `ereader-togepi`, `-mareep`, `-scizor` (Fishy), `ereader-togepi+english`, `+pid`, `+fateful` (Invalid) |
| Colosseum starters (Espeon, Umbreon): TID, SID, then two PIDs rerolled until male and not shiny, IVs from the same seed | `MethodCXD.cs:510-523, 571-612`, `MethodFinder.cs:555-572` | `coloStarterPid`, `coloStarterSeed`, `coloStarterMatch` | Invalid when no seed yields TID/SID/PID/IVs | `colo-starter-espeon`, `-umbreon` (Valid), `+ivhp`, `+fateful`, `+femaleot`, `+metlevel` (Invalid) |
| XD starter Eevee: TID, SID, two fake PIDs, IVs, ability, PID | `MethodCXD.cs:614-636`, `CXDVerifier.cs:15-35` | `xdStarterSeed`, `verifyCxd` | Invalid when PID and IVs do not follow from the TID/SID | `xd-starter-eevee` (Valid), `+ivhp`, `+pid` (Invalid) |
| Trainer ID from the name screen: the name-screen end seed advances 1000 frames to the TID then SID; XD encounters exempt above English, Fishy for English | `MethodCXD.cs:644-693`, `XDRNG.cs:97-98`, `TrainerIDVerifier.cs:30-32, 86-101` | `isValidNameScreenEndSeed`, `cxdTrainerIdHasSeed`, `verifyTrainerId` | Invalid (Colosseum, Japanese XD), Fishy (English XD), skipped (other XD languages); the oracle builds every record with a valid name-screen TID/SID (`CxdTrainer`) | base rows (Valid); differential TID/SID mutations |
| Poke Spot: activation, slot from the PID, origin seed from the IVs, spot animation, level within the slot (a transferred record: template level at most the met level) | `MethodPokeSpot.cs:8-50, 63-84, 140-175, 373-410`, `EncounterEnumerator3GC.cs:106-117` (area location compared for a PK3 only) | `pokeSpotActivation`, `pokeSpotOriginPid`, `pokeSpotOriginIVs`, `pokeSpotAnimation` | Invalid when no origin seed produces the record | `pokespot-sandshrew`, `-zubat`, `-hoppip`, `-phanpy`, `-aron` (Valid), `pokespot-sandshrew+pid`, `+metlevel`, `@loc91` (Invalid) |
| XD trades and Colosseum gift: fixed OT and nickname per language (the GameCube name cut to the GBA length, 5 JPN / 7 INT) | `EncounterTrade3XD.cs` IsTrainerMatch / IsNicknameMatch, `EncounterGift3Colo.cs` IsTrainerMatch, `NicknameVerifier.cs:51-54, 356-361` | `xdFixedNameMatch`, `coloTrainerMatch`, `verifyNickname`, `verifyTrainerName` | Invalid on a changed OT or nickname | `xd-trade-meditite`, `-shuckle`, `-larvitar`, `-elekid-zaprong`, `colo-gift-plusle` (Valid), `xd-trade-shuckle+ot`, `xd-trade-elekid+nick`, `colo-gift-plusle+ot` (Invalid) |
| Fateful (obedience) flag: set on every Pokemon XD sends, never by Colosseum | `MiscVerifier.cs:15,60` (VerifyFatefulEncounter), `XK3.cs:224-241`, `CK3.cs:243` | `verifyFateful` | Invalid either way | `xd-shadow-zapdos-fateful`, `xd-gift-chikorita-fateful`, `colo-shadow-bayleef+fateful`, `colo-starter-espeon+fateful`, `ereader-togepi+fateful`, PKHeX records `pkhex-shuckle-gift-nofateful-ck3` (Invalid) and `pkhex-shuckle-nofateful-xk3` (Valid) |
| National Ribbon exactly on shadows; Earth Ribbon allowed on any Gen III visitor | `RibbonRules.cs:311-317, 333` | `verifyRibbons` | Invalid when missing on a shadow or present on a non-shadow | `colo-shadow-bayleef-national`, `xd-shadow-zapdos-national` (Invalid), `+earth` (Valid) |
| Ball: XD shadows keep their snag ball (any GBA ball but the Safari Ball), Colosseum shadows likewise, starters, gifts, trades and Poke Spots are fixed | `BallVerifier.cs` (IFixedBall, VerifyBallEquals), template `FixedBall` | `verifyBall`, `wildBall3` | Invalid | `colo-shadow-bayleef+safari`, `xd-shadow-zapdos+safari`, `xd-shadow-teddiursa+great`, `xd-gift-chikorita+master`, PKHeX `pkhex-ultraball-teddiursa` (Invalid); `+master` on shadows (Valid) |
| Ability: a two-ability species may carry either ability without matching the PID; a one-ability species must not set the bit unless it is a shadow (re-battles flip it) | `AbilityVerifier.cs:147-158, 203-247` | `verifyAbility` | Invalid | `pkhex-vaporeon-split-evo` (Valid) |
| Male OT only | `CXDVerifier.cs:20` (G3OTGender) | `verifyCxd` | Invalid | `+femaleot` variants |
| Moves: the CXD learn source is Emerald's | `LearnGroup3.cs:73` | `reportMoves`, `movesFor` | as for an Emerald origin | base rows |
| Language: the trade/gift/e-Reader fixed language, otherwise any GBA language | `LanguageVerifier`, `EncounterShadow3Colo.cs:81` | `verifyLanguage` | Invalid | `ereader-togepi+english` |
| Unbounded searches | `MethodCXD`, `TeamLockResult` loop until found | `CxdGuard` (name screen depth 256, trainer-ID 100000 steps, team lock 400000 steps) | Unknown (Warning "RNG search reached its limit"); every Invalid found under a cap is demoted to a Warning | never reached in the corpora (`PKHEX_DISAGREEMENT_REVIEW.md` 16-A) |

PKHeX test records (`PKHeX.Core.Tests/Legality/Legal/...`, converted with the oracle's
`cxdprobe convert`) are fixtures too: Vaporeon split evolution (VALID; its LevelEXPThreshold
Fishy is Info here, section 3), Shuckle `.xk3` without the fateful flag (VALID: the XK3 itself
has no flag), Italian HO-OH with a long OT (VALID, bonus disc, version Sapphire), Shuckle `.ck3`
without the fateful flag (INVALID), Ultra Ball Teddiursa (INVALID), Tyranitar with and without
the Winning Ribbon (Fishy / Invalid).

### 7b. Shared judge for later formats

`include/Legality/Gen3Cxd.h` exposes the same path as `Gen3Cxd::judge(const Input&)` for a Pal
Park PK4 or a Poke Transfer PK5: every field the caller does not know is declared unknown and is
neither proved nor disproved; `hasOriginalMet == false` applies PKHeX's Format != 3 rules
(location not compared, `EncounterShadow3Colo.cs:178-182`; level within the chain, `:173-176`;
origin chain from level 2, `EncounterOrigin.cs:73-82`; Poke Spot level at most the met level,
`MethodPokeSpot.cs:160-163`). The verdict is Invalid, Matched or Undecided (a guard reached);
Matched returns the template species, level, location and fixed moves so the caller can verify
the moves of its own format. `Legality.cpp` (`addGen3PidEvidence`) skips its older GameCube
evidence for a PK3 the analyzer judged; a Pal Park PK4 still gets it until the Gen IV analyzer
calls the judge.

### 7c. Measurement

`tools/pkhex_gen34_diff/oracle/Program.cs` now builds every Colosseum/XD template
(`GameVersion.CXD` in the origin list) as a CK3/XK3 converted to PK3 with a trainer whose
TID/SID a name screen can produce, once per GBA cartridge (R, S, E, FR, LG), plus the usual
single-field mutations; `GROW=1` grows them like any other Gen III record (CXD learns as
Emerald, `oracle/Grow3.cs`). Fresh runs at commit `0a14ffcd`, `run.sh /home/user/ext/pkhex
SEED 3 3` and `GROW=1 run.sh /home/user/ext/pkhex SEED 3`, counted by origin version 15 only:

| Corpus | All records | False Invalid | False Valid | GameCube records | PKHeX-invalid caught | No problems found (PKHeX-valid) |
|---|---|---|---|---|---|---|
| Mutation seed 1 | 32756 | 0 | 0 | 3914 | 2176/2176 (100%) | 1689/1738 (97.18%) |
| Mutation seed 2 | 32756 | 0 | 0 | 3928 | 2182/2182 (100%) | 1705/1746 (97.65%) |
| Mutation seed 3 | 32756 | 0 | 0 | 3908 | 2098/2098 (100%) | 1761/1810 (97.29%) |
| Grown seed 1 | 21910 | 0 | 0 | 2610 | - | 2573/2610 (98.58%) |
| Grown seed 2 | 21971 | 0 | 0 | 2623 | - | 2581/2623 (98.40%) |
| Grown seed 3 | 21928 | 0 | 0 | 2601 | - | 2560/2601 (98.42%) |

The 965 unmutated GameCube records of seed 1 (415 Colosseum shadows, 450 XD shadows, 45 Poke
Spot slots, 10 Colosseum starters, 20 XD gifts, 20 XD trades, 5 Plusle; each in five
cartridges) are all PKHeX-VALID; 950 are certified and the 15 e-Reader shadows are Incomplete
(IVAllEqual_0). The uncertified PKHeX-valid mutations are English XD TID/SID mutations
(Fishy TrainerIDNoSeed), e-Reader all-zero IVs, and records that carry both an older
"preserved move" Warning and one of these. The whole-corpus figures (all origins) for the same
runs: seed 1 caught 16593/16601 (the 8 uncaught are Enigma Berry holders, section 3), certified
15552/16155 (96.27%); seed 2 16612/16614, 15523/16142 (96.17%); seed 3 16400/16405,
15789/16351 (96.56%); grown seeds 1-3 certified 98.19%, 98.22%, 98.28%. The Gen IV run
(`run.sh /home/user/ext/pkhex 1 3 4`: 50999 records, caught 28001/28001, certified 22963/22998,
35 deliberately stricter met-location records) is unchanged by this work.
