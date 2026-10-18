# PKHeX Gen I / Gen II (PK1, PK2) legality rule inventory

Reference for the Game Boy analyzer (`src/Legality/GBLegality.cpp`, `src/Legality/GBLegalityData.cpp`,
`include/Legality/GBLegality.h`, `include/Legality/GBLegalityData.h`). It is a port of PKHeX.Core at
kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722 (GPL-3.0) for Pokémon whose origin is
Red, Blue, Yellow (and Japanese Red/Green/Blue), Gold, Silver or Crystal, including Time Capsule
trips in both directions, Pokémon Stadium's gifts and Pokémon Stadium 2's move reminder. The
tables it reads are generated from that checkout by `tools/gen_gb_legality_data.py` and checked
against pret by `tools/verify_gb_legality_data.py` (`--pokered`, `--pokeyellow`). Decomp evidence
cites pret/pokered@af519899, pret/pokeyellow@e89ead15, pret/pokegold@ef0201d8 and
pret/pokecrystal@3bc8daa4, read-only under `/home/user/ext`.

The engine is read-only: it takes a `GB::Entity1` or `GB::Entity2` (the record's fields, the
decoded names and, when the caller has them, the stored name bytes) and returns a `GB::Result`
(`Status` Legal / Illegal / Uncertain, the origin found, and `Problem{area, text, definitive}`
entries). It never changes a record.

## 0. Production path

```
Legality::analyze(pk, group, sourceGame)                  src/Legality/Legality.cpp
  └─ analyzeNativeGen1  (PK1 from Gen1ReadOnlySave)        fills GB::Entity1, calls GB::checkGen1
  └─ analyzeNativeGen2  (PK2 from Gen2ReadOnlySave)        fills GB::Entity2, calls GB::checkGen2
        GB::checkGen1 / checkGen2                          src/Legality/GBLegality.cpp
          ├─ movesWellFormed, formatProblems               rules that hold for any origin
          ├─ rawNameProblems, nameCharsetProblems          name bytes / characters (any origin)
          ├─ originChain                                   evolution chain down to the lowest form
          └─ search(pk, chainOwn, chainOther, otherGen, extra)
                ├─ gen1Candidates / gen2Candidates         every encounter the chain allows
                └─ evaluate(pk, candidate, chain)          moves, levels, catch rate, DVs, met data,
                                                           names, PP, egg data for that candidate
```

`search` returns the first candidate with no problem (Legal), else keeps the candidate that only
non-definitive problems exclude (Uncertain), else Illegal. A definitive origin-independent problem
makes the record Illegal whatever the origin. `Legality.cpp` maps `definitive == true` to
**Invalid**, `definitive == false` to a **Warning** (the report reads Incomplete and the encounter
and event coverage are Partial), and a Legal result to Info lines ("Possible origin", "needs a
Time Capsule trip", "a legitimate origin exists; that does not prove where it came from").

What the production entity carries today: species, level, EXP, moves, PP and PP Ups, DVs,
trainer ID, stat experience, the decoded trainer name and nickname, the language guessed from
the decoded text (`gen1Language`: kana → Japanese, A–Z → International), and for PK1 the catch
rate and types, for PK2 the held item, friendship/hatch counter, Pokérus and caught data. It
does **not** carry the stored name bytes (`otRaw`/`nickRaw`), so the byte rules of section 2.8
are reachable only through `tools/pkhex_gb_diff` and the tests until
`docs/legality/handoff/lane-d-raw-names.patch` lands in the Integration parsers.

## 1. Pipeline (PKHeX class → GBLegality.cpp section)

| PKHeX.Core | GBLegality.cpp |
|---|---|
| `LegalityAnalysis.ParsePK1` (Legality/LegalityAnalysis.cs:217-228: Nickname, Level, Level.VerifyG1, Trainer.VerifyOTGB, MiscValues.VerifyMiscG12, MovePP, EVs, Item for PK2) | `checkGen1` / `checkGen2`, `formatProblems`, `evaluate` |
| `EncounterFinder.FindVerifiedEncounter`, `EncounterGenerator12` (Generator/ByGeneration/Lump/EncounterGenerator12.cs:25-42, `GBRestrictions.GetTradebackStatusInitial` Restrictions/GBRestrictions.cs:154-190), `EncounterEnumerator1` (trades → statics → slots → events), `EncounterEnumerator2` (trades → eggs → Odd Egg → statics → slots → events, Korean gates at lines 30, 100, 111) | `search`, `gen1Candidates`, `gen2Candidates` |
| `EncounterOrigin.GetOriginChain12`, `EvolutionGroup1` (GetPrevious = Gen II when tradeback allowed, Evolutions/EvolutionGroup/EvolutionGroup1.cs:12), `EvolutionGroup2` (GetPrevious = Gen I unless format 1, EvolutionGroup2.cs:14), `EvolutionTree.Evolves1/2` | `originChain`, `historyFrom`, `presentIn`, `preEvolution` |
| `EncounterSlot1` (Templates/Gen1/EncounterSlot1.cs:63-80), `EncounterStatic1` (:67-110), `EncounterTrade1` (:56-160), `EncounterGift1` (`event1.pkl`, Stadium and tour trainers) | `gen1Candidates`, `catchRateMatchesEncounter`, `isStadiumGiftSpecies` |
| `EncounterSlot2` (Templates/Gen2/EncounterSlot2.cs:60-80 `IsTreeAvailable`, :134-160 `IsMatchExact`), `EncounterStatic2` (:118-170, roamers :24 and :267-282, Odd Egg :38 and :122-140), `EncounterTrade2` (:98-200), `EncounterEgg2`, `EncounterGift2` (`event2.pkl`), `Encounters2.CelebiVC` | `gen2Candidates`, `isTreeAvailable`, `isRoamLocation`, `matchEggLocation`, `isOddEggTrainer`, `canHatch` |
| `LearnGroup1` (LearnSource/Group/LearnGroup1.cs:25-40 Check, :41-64 FlagEvolutionSlots, :151 HasPossiblyVisitedGen2), `LearnGroup2` (LearnGroup2.cs:14-18 GetPrevious, :23-60 Check, :85-100 egg moves, :114 Korean, :139 Stadium reminder), `LearnSource1RB`/`1YW` (TM, tutor Surf/Fly Pikachu), `LearnSource2GS`/`2C` (TM, Crystal tutor, egg moves), `LearnSource2Stadium` (:29-40 GetCanRelearn), `LearnsetStadium.CanRelearn` (Learnset/LearnsetStadium.cs:58) | "learnsets" and "move verification": `canLearn1`, `canLearn2`, `checkGen1Sources`, `checkGen2Sources`, `flagGen1Bookkeeping`, `stadiumRelearnLevel`, `checkMoves` |
| `MoveBreed2.Validate` (Legality/Moves/Breeding/MoveBreed2.cs:18), `LearnVerifierEgg` / `LearnVerifierRelearn.VerifyEggMoveset` | `validateBreed2`, `verifyBaseMoves`, `recurseOrigins`, `eggMovesProblem` |
| `LearnVerifierHistory.VerifySmeargle`, `LearnVerifier` duplicate / empty slot rules | `isSketchable2`, `movesWellFormed` |
| `LevelVerifier.Verify` (Legality/Verifiers/LevelVerifier.cs:12-56), `LevelVerifier.VerifyG1` (:77-100, met level, trade evolution) | `levelExpProblem`, `historyFrom`, `evaluate` |
| `TrainerNameVerifier.VerifyOTGB` / `VerifyGBOTWithinBounds` (TrainerNameVerifier.cs:98-166), `NicknameVerifier.VerifyG1NicknameWithinBounds` (NicknameVerifier.cs:365-387), `VerifyNicknameEgg` (:331-350), `StringConverter1/2.GetIsJapanese/GetIsEnglish`, `StringConverter2KOR.IsHangul/GetIsKorean`, `PK1.Korean`/`PK2.Korean` (PKM/PK1.cs:15, PK2.cs:15) | "stat experience, Pokerus, names" and "name bytes": `trainerNameProblem`, `nicknameProblem`, `rawNameProblems`, `nameCharsetProblems`, `hangulLead`, `koreanLength`, `glyphless` |
| `MiscG1Verifier.VerifyG1` (Verifiers/Misc/MiscG1Verifier.cs:14-39 time of day, :40-73 types, :75-110 catch rate), `GBRestrictions.IsTimeCapsuleTransferred` (GBRestrictions.cs:192-212) | `typeProblem`, `catchRateProblem`, `gen2Possible`, `evaluate` (met time) |
| `EffortValueVerifier.VerifyEVsGB` (EffortValueVerifier.cs:70-125) | `statExpProblem` |
| `MovePPVerifier.VerifyEntity` / `VerifyEgg` (MovePPVerifier.cs:22-60), `GBPKM.GetMovePP` | `ppProblem`, `maxPP` |
| `MiscVerifier.VerifyMiscG12` → Pokérus (MiscVerifier.cs:69-76) | `pokerusProblem` |
| `ItemVerifier` (ItemVerifier.cs:16-17 egg item), `ItemConverter.IsCatchRateHeldItem` (PKM/Util/Conversion/ItemConverter.cs:229) | `isHoldableGen2`, `isHeldItemRate` |
| `EggStateLegality.GetIsEggHatchCyclesValid` (Verifiers/Egg/EggStateLegality.cs:16-27), `EncounterVerifier.VerifyEncounterEgg2` / `VerifyUnhatchedEgg2` (Encounters/Verifiers/EncounterVerifier.cs:95-120), `VerifyWildEncounterGen2` (:58-66) | `evaluate` (egg block, met data, headbutt trees) |
| `LanguageVerifier` (LanguageVerifier.cs:30-39: Korean needs GD/SI, Japanese Blue needs Japanese) | `koreanCartOnly`, `languageFits`, `gen1Candidates` (Blue slots for Japanese only) |
| `TrainerIDVerifier` (TrainerIDVerifier.cs:53-58: TID 0 is Fishy) | not ported: PKHeX grades it Fishy, never Invalid; no game rule forbids TID 0 |
| `ParseSettings` (Settings/ParseSettings.cs:49-72: AllowGen1Tradeback, AllowGen2Crystal, AllowGen2MoveReminder, AllowGen2OddEgg, AllowGBEraEvents, AllowGBStadium2 = AllowEraCartGB) | cartridge-era settings throughout (tradeback on, GB-era events and Stadium 2 on, Virtual Console off) |

## 2. Rules

Unless a row says otherwise: games = all six (Japanese Blue noted as `BU`); invocation path =
`Legality::analyze → analyzeNativeGen1/2 → GB::checkGen1/2 → search → evaluate`; fuzz coverage =
`tests/test_gb_legality_fuzz.cpp` (6,000 random `Entity1` and 6,000 random `Entity2` through
`checkGen1`/`checkGen2`/`formatProblems`/`describe`, plus 3,000 random `PokemonRecord`s through
`Legality::analyze`), built and run under `-fsanitize=address,undefined`. "I" = definitive →
Invalid; "W" = non-definitive → Warning / Incomplete; "ok" = passes. Fixtures name the test file
(`gen1` = tests/test_gb_legality_gen1.cpp, `gen2` = tests/test_gb_legality_gen2.cpp, `struct` =
tests/test_gb_legality_structural.cpp, `names` = tests/test_gb_legality_names.cpp) and the corpus
kind (`tools/pkhex_gb_diff/gen_corpus.py`, `gen_corpus2.py`).

### 2.1 Encounters and origins

| Rule | Games | PKHeX | Data | Ours | Outcome | Fixtures | Status |
|---|---|---|---|---|---|---|---|
| Wild slots: grass, surf, Old/Good/Super Rod, Safari Zone (Gen I); grass, surf, rods, Rock Smash, Headbutt, Bug-Catching Contest (Gen II); the lowest level per species and version is what the chain needs | all | `EncounterSlot1.IsMatchExact` (:63-80), `EncounterSlot2.IsMatchExact` (:134-160), `Encounters1/2` slot tables (`encounter_*.pkl`) | species, level, version; PK2 met data when present | `kWildSlots`, `gen1Candidates` ("Wild slots"), `gen2Candidates`, `addWild` | ok when a slot's lowest level ≤ met/current level; I when no slot (and nothing else) fits | gen1 `wild`, `wild-jp`, `grown`; gen2 `wild2`, `grown2`; `evo-order` | Ported; slot tables verified against pokered/pokeyellow by `verify_gb_legality_data.py` |
| Yellow's own slots and catch rates; Japanese Blue's own slots (BU) only for a Japanese record | RBY, BU | `EncounterEnumerator1` (SlotBU only when `Entity.Japanese`, :102-108), `EncounterStatic1.IsMatchExact` (:73-75) | language | `gen1Candidates` (version mask, `languageFits`) | I for a non-Japanese record whose only origin is BU | gen1 `wild-jp`, `static-jp` | Ported |
| Static encounters, starters, gifts (Eevee, Lapras, Hitmons, Magikarp, Snorlax, the birds, Mewtwo; Gen II statics, Red Gyarados, roamers, Lugia/Ho-Oh, Suicune) and Game Corner prizes | all | `Encounters1.StaticRBY` (:19-60), `Encounters2.StaticGSC/GS/C` (:18-100), `EncounterStatic2.IsMatchExact` (:118-170) | species, level, version, met data | `kStatics1`, `kStatics2`, the two candidate generators | ok at the static's level (or grown from it) | gen1 `static`, `grown-static`; gen2 `static2`, `grown-static2` | Ported |
| Fossils (Omanyte, Kabuto, Aerodactyl at level 30) | RBY | in `StaticRBY` | as above | `kStatics1` | ok; a PK2 of these can also be bred in Gen II from a Time Capsule parent (`EncounterEgg2`), so a PK2 fossil has a Gen II egg origin too | gen1 `static`; names `japaneseTrainerName` (Articuno chosen because it has no egg) | Ported |
| In-game (NPC) trades, every language's fixed trainer and nickname, the Gen I trainer marker, evolved trades, GSC level minimum for a traded-back Gen I trade | all | `EncounterTrade1.IsMatchExact` (:157-170), `IsTrainerNameValid` (:70-80), `IsNicknameValid` (:56-67), `EncounterTrade2.IsMatchExact` (:98-150), `DetectLanguage` (:150-200) | names, language, TID and DVs (Gen II fixed trades), met data | `kTrades1`, `kTrades2`, `nameMatches`, `nameLanguages`, `fixedDVsMatch` | ok with the right names; a renamed trade falls back to the other origins (I when none) | gen1 `trade-<lang>`, names `tradeMarker`; gen2 the European trades, `trade-ot-bad-term` | Ported |
| Roaming beasts: any roam location (Raikou, Entei; Suicune outside Crystal's Tin Tower) | GSC | `EncounterStatic2.IsRoaming` (:24), `RoamLocations` (:267-282) | met location | `isRoamLocation`, `gen2Candidates` | ok on a roam location; I elsewhere with met data | gen2 `static2` beasts | Ported (same bitmask) |
| Headbutt trees: the trees a trainer ID can shake, per area | GSC | `EncounterSlot2.IsTreeAvailable` (:60-80), `EncounterVerifier.VerifyWildEncounterGen2` (:58-66) | TID, met location, slot type | `isTreeAvailable`, `isHeadbutt`, `gen2Candidates` (deferred then flagged) | I when the only slot is a headbutt tree the TID cannot reach | gen2 headbutt slots in `wild2` | Ported |
| Time of day: Crystal records 1–3 for caught Pokémon, 0 for gifts and in-game trades | C | `MiscG1Verifier.VerifyG1` (:21-33), `EncounterSlot2.IsMatchExact` (`Parent.Time.Contains`) | caught data | `evaluate` (`fixedTime`, `timeOk`), slot time masks | I | gen2 `defect2-time-*` | Ported |
| Crystal caught data: met level within the slot, met location, egg met level 1 and location 0 (unhatched) / ≤ 95 (hatched), beasts and Celebi keep theirs (no Time Capsule) | C | `EncounterVerifier.VerifyEncounterEgg2` / `VerifyUnhatchedEgg2` (:95-120), `EncounterSlot2.IsMatchExact`, `LevelVerifier.VerifyG1` (:84-89 `LevelMetBelow`) | caught data | `evaluate`, `matchEggLocation`, `gen2Candidates` | I | gen2 `defect2-caught-*`, `defect2-level-*`, `egg2` | Ported |
| Swarms: Gen II's swarm species are in the area tables as ordinary slots | GSC | slot tables | — | `kWildSlots` | as wild slots | gen2 `wild2` | Ported with the tables |
| Eggs: species that can hatch, level 5, the Odd Egg (fixed trainer per language, EXP 125, Dizzy Punch) | GSC, C | `EncounterEgg2`, `Breeding.IsAbleToHatchFromEgg`, `EncounterStatic2` Odd Egg (:38, :122-140), `LevelVerifier.Verify` egg branch (:29-42) | species, level, EXP, egg flag, OT, friendship | `canHatch`, `gen2Candidates` ("Daycare eggs", Odd Egg), `isOddEggTrainer`, `evaluate` egg block | I on the wrong level, EXP, hatch counter, item or trainer | gen2 `egg2`, `defect2-egg-*`, Odd Egg fixtures | Ported |
| Egg hatch counter 1..species' cycles (10 for event eggs) | GSC | `EggStateLegality.GetIsEggHatchCyclesValid` (:16-27) | friendship | `evaluate` (`maxCycles`) | I | gen2 `defect2-egg-cycles-*` | Ported |
| GB-era event distributions: Stadium gifts (OT STADIUM / スタジアム and localized, TID 2000 / 1999, gift-box catch rate), the European and Japanese Mew tours (fixed DVs 10/1/12/5, tour OTs), Gen II events (`event2.pkl`: Pokémon Center New York, Japanese distributions, event eggs with 10 cycles) | all | `Encounters1GBEra.Gifts` (`event1.pkl`), `Encounters2GBEra.Gifts` (`event2.pkl`), `EncounterGift1` (:42-180), `EncounterGift2` (:46-120), `ParseSettings.AllowGBEraEvents` | OT, TID, DVs, catch rate, met data, language | `kGifts1`, `kGifts2`, `gen1Candidates` ("GB-era event gifts"), `gen2Candidates` (events), `isEuropeTourOT`, `isJapanTourOT` | ok on a match; a Mew/Celebi matching nothing → **W** ("the event list is not known to be complete"), any other species → I | gen1 `event*`, Mew fixtures; gen2 `gift2-jp`, `defect2-event-*` | Ported; the softening is deliberate (verdicts doc rule 2) |
| GS Ball Celebi (Japanese Crystal, Ilex Forest, level 30) | C (JP) | `Encounters2.CelebiVC` (:120, Virtual Console only) | met data, language | `gen2Candidates` ("GS Ball Celebi") | ok for a Japanese record with the shrine's met data; PKHeX INVALID | gen2 `gsball2-*` | Deliberate difference (`PKHEX_VERIFIED_IMPROVEMENTS.md` D-2) |
| Korean Gold/Silver: Korean trade names, no Crystal, no events, no Gen I origin; the Korean–Western cartridge trade (Bulbapedia) opens Crystal moves, Stadium 2, Gen I and Western eggs | GD/SI (KOR) | `EncounterEnumerator2` (:30, :100, :111), `EncounterGenerator12.GenerateFilteredEncounters12BothKorean` (:40-60), `ParseSettings.AllowGen2Crystal/AllowGen2MoveReminder` (:58, :66), `LearnGroup2` (:18, :114), `LanguageVerifier` (:33-35) | Hangul lead byte (or caller's language) | `koreanCartOnly`, `Subject::westernTrade`, `checkGen2` Korean block | ok inside Korean Gold/Silver; **W** when only the trade explains it; I for a Korean trainer on a Crystal/Gen I catch or a non-egg event | gen2 `kor2-*` (20 kinds); names `koreanPk2` | Ported; the trade route is review-listed (verdicts doc) |
| Korean trainer name on a PK1 | RBY | none (`PK1.Korean` is set, PKM/PK1.cs:15, but `EncounterEnumerator1` has no Korean gate; the oracle accepts a Korean-named Rattata as `EncounterSlot1:19:RD:3`) | Hangul lead byte | `checkGen1` Korean block (Gen II origins only, `westernTrade`) | **W** when a Gen II origin and the trade explain it; I when none does | names `koreanPk1`; gen1 `kor1-*` | Beyond PKHeX; review item 17-D; production needs the hand-off patch |
| Mew glitch, Old Man / MissingNo., Celebi egg and other Time Capsule glitches | RBY, GSC | not modelled | — | not modelled; Mew's unrecorded-event softening is the only concession | W (Mew) / I (anything else that matches no origin) | gen1 Mew fixtures | **Unknown** (owner decision: glitch provenance is not an origin) |

### 2.2 Levels and experience

| Rule | Games | PKHeX | Data | Ours | Outcome | Fixtures | Status |
|---|---|---|---|---|---|---|---|
| The encounter's level ≤ the record's level; evolution history reachable from the encounter level by level-up, trade, item and friendship evolutions (Gen I and Gen II trees, the Gen II-only evolutions only on a Gen II visit) | all | `EncounterOrigin.GetOriginChain12`, `EvolutionGroup1/2`, `EvolutionTree.Evolves1/2`, `IsMatchLevel` on each template | species, level, (met level) | `originChain`, `historyFrom`, `presentIn`, `kEvolutions1/2` | I ("met at level L cannot become S by level N") | gen1 `grown*`, `evo-order`; gen2 `grown2`, `time-capsule` | Ported |
| Trade evolution left unevolved: Kadabra, Machoke, Graveler, Haunter caught as such are fine; a traded one must have evolved (no Everstone in Gen I) — PKHeX applies it only to a trade-evolution species *in Gen II format whose encounter is Gen I* | RBY→GSC | `LevelVerifier.VerifyG1` (:91-100), `IsTradeEvolutionRequired` (:105-125), `GBRestrictions.IsTradeEvolution1` (:85) | species, format | the history solver keeps Kadabra etc. reachable; Stadium 2 transfers without a trade | ok (PKHeX's exact Gen II-format case is covered by the Time Capsule candidates: a Gen I Kadabra that reached Gen II evolved on the way unless Stadium 2 moved it, which PKHeX also allows) | gen2 `time-capsule` | Ported in effect; no Invalid is raised that PKHeX does not |
| Stored level vs EXP: level ahead of its EXP is impossible; level behind its EXP or EXP above the level-100 total within `$50FFFF` is a Day Care slot | all | `LevelVerifier.Verify` (:45-51, `LevelEXPTooHigh` only at level 100; otherwise derives level from EXP) | level, EXP, species growth rate | `levelExpProblem` (`kDayCareExpMax = 0x50FFFF`), `formatProblems` | I (level ahead; EXP past `$50FFFF`; EXP over the total at level 100 on a PK2); **W** (level behind; EXP over the total within reach) | struct `levelExpGrid`, `realRedSave`, `productionPk2`; gen1 `level-exp-*`, `level-100-exp-*`; corpus `struct*-exp-*`, `exp-daycare-*` | Beyond PKHeX (D-1) and review item 15-D |
| Egg level 5 and egg EXP (Odd Egg 125) | GSC | `LevelVerifier.Verify` (:29-42), `LevelVerifier.VerifyG1` (:80-83) | level, EXP, egg flag | `formatProblems(Entity2)` (`kEggLevel`), `evaluate` egg block (`kOddEggExp`) | I | gen2 `defect2-egg-*` | Ported |
| Met level ≤ current level (Crystal) | C | `LevelVerifier.VerifyG1` (:84-89) | caught data | `evaluate` through `historyFrom(enc.level = met level)` | I | gen2 `defect2-level-*` | Ported |

### 2.3 Catch rate, held item and types

| Rule | Games | PKHeX | Data | Ours | Outcome | Fixtures | Status |
|---|---|---|---|---|---|---|---|
| PK1 catch rate = the encounter species' rate (RB or Yellow table) or a pre-evolution's, or a Gen II held item after a Time Capsule trip; gifts and Stadium boxes have their own; Gen II-only moves require a held-item rate | RBY | `MiscG1Verifier.VerifyMiscG1CatchRate` (:75-110), `GBRestrictions.IsTimeCapsuleTransferred` (:192-212), `RateMatchesEncounter` (:87-98), `GetCatchRateMatchesPreEvolution` (:104), `EncounterSlot1.IsMatchExact` (:73), `ItemConverter.IsCatchRateHeldItem` (:229), `EncounterFinder` (:142-150) | catch rate, moves, chain | `catchRateProblem`, `catchRateMatchesEncounter`, `rateMatchesEncounter`, `rateMatchesEither`, `isHeldItemRate`, `kCatchRateIsHeldItem`, `evaluate` | I | gen1 `defect-catch*`, `from-gen2`, `kor1-rby` | Ported |
| PK2 held item must be holdable in Gen II; an egg holds nothing | GSC | `ItemVerifier` (:16-17), `ItemConverter`/`Legal` Gen II held-item list | held item, egg flag | `isHoldableGen2`, `kHeldItems2`, `formatProblems(Entity2)` | I | gen2 `defect2-item-*`, `defect2-egg-item-*` | Ported |
| PK1 types match the species (RB table), Porygon's Conversion types any valid pair, Stadium 2 writes Steel on Magnemite/Magneton | RBY | `MiscG1Verifier.VerifyMiscG1Types` (:40-73), `PersonalTable1.TypeIDExists` | type1, type2 | `typeProblem`, `gen1TypeExists`, `kTypeSteel` | I | gen1 `defect-type*` | Ported |

### 2.4 DVs

| Rule | Games | PKHeX | Data | Ours | Outcome | Fixtures | Status |
|---|---|---|---|---|---|---|---|
| Shiny DVs (DEF/SPE/SPC 10, ATK in {2,3,6,7,10,11,14,15}): the Red Gyarados is always shiny; a shiny-locked static is never; Odd Egg shiny chances are not a rule | GSC | `EncounterStatic2.IsMatchExact` (:120, :156-158), `EncounterGift2.IsShiny` (:22) | DVs | `isShinyDVs`, `gen2Candidates` (`shinyAlways`, `shinyNever`), event `g.shiny` | I | gen2 `static2` Gyarados, `defect2-dv*` | Ported |
| Gender from the ATK DV (gender ratio): a fixed-DV trade's gender, egg inheritance (female parent's line), Gen I / event OT gender male | GSC | `EncounterTrade2.IsMatchExact` (:127-131), `GenderVerifier` (:58, derived in format 2 so never independently wrong), `TrainerNameVerifier.VerifyOTGB` (:106-107 `G1OTGender`) | DVs, OT gender bit | `fixedDVsMatch`, `kPersonalC[...].genderRatio` (egg solver), `evaluate` ("cannot have a female original trainer") | I | gen2 `defect2-otgender-*`, trades | Ported |
| Unown letter from the DVs; forms ≥ 26 (! ?) never in Gen II; Unown cannot take the Time Capsule | GSC | `FormVerifier` (:55), `EncounterSlot2.IsMatchExact` (:135-140) | DVs | letter derived in `Pokemon2ReadOnly`; `gen2Candidates` (Gen II-only species list) | ok (the letter is never stored, so it cannot mismatch) | gen2 Unown slots in `wild2` | Ported |
| Fixed DVs of the Mew tours (10/1/12/5) and Gen II fixed trades | all | `EncounterGift1` tours, `EncounterTrade2.IVs` (:105-131) | DVs | `kGifts1` tour DVs, `fixedDVsMatch` | I | gen1 tour fixtures; gen2 trades | Ported |

### 2.5 Stat experience and EXP gains

| Rule | Games | PKHeX | Data | Ours | Outcome | Fixtures | Status |
|---|---|---|---|---|---|---|---|
| An egg has no stat experience | GSC | `EffortValueVerifier.Verify` (:18-21 `EffortEgg`) | stat exp, egg flag | `statExpProblem(..., egg)` | I | gen2 `defect2-egg-statexp-*` | Ported |
| Battle stat experience with one stat still at 0 (PKHeX's "simple sanity check"): pokered `DivideExpDataByNumMonsGainingExp` floors each enemy base stat by the recipient count, so a stat can gain 0 | all | `EffortValueVerifier.VerifyEVsGB` (:70-125, `EffortShouldBeZero`) | stat exp | `statExpProblem` | **W** | gen1/gen2 `defect-statexp*`; corpus reason `gb-divided-stat-exp-zero` | Downgraded with pret evidence (verdicts doc) |
| Vitamins: 2,560 per vitamin, 25,600 maximum | all | `EffortValues.VitaminBoost12`, `MaxVitamins12` | stat exp | `statExpProblem` (vitamin-only pattern recognised) | ok | corpus predicate `battle_stat_exp_with_zero` | Ported |

### 2.6 Moves

| Rule | Games | PKHeX | Data | Ours | Outcome | Fixtures | Status |
|---|---|---|---|---|---|---|---|
| Move slots packed (no gap before a move), at least one move, no duplicates, move IDs within the format (165 / 251) | all | `LearnVerifier` (`Duplicate`, `EmptyInvalid`), `MoveInfo` | moves | `movesWellFormed` | I | struct `duplicateMoves`; corpus `struct*-dup-move*` | Ported |
| Encounter moves (the last four level-up moves at the encounter level, per version), level-up moves up to the current level in each form of the history, TM/HM compatibility (RB / Yellow / GS / Crystal tables), Yellow's Surf and Fly Pikachu tutor, Crystal's Flamethrower/Thunderbolt/Ice Beam tutor, trade moves (fixed), Stadium's Surfing Pikachu | all | `LearnGroup1.Check` (:25-40), `LearnGroup2.Check` (:23-60), `LearnSource1RB/1YW.GetCanLearn` (:35-55), `LearnSource2GS/2C.GetCanLearn` (LearnSource2C.cs:53-72, tutor :74-84), `GetEncounterMoves` | moves, species history, versions | `addEncounterMoves`, `gen1EncounterMoves`, `gen2EncounterMoves`, `canLearn1`, `canLearn2`, `tm1`, `tm2`, `tutorC`, `checkGen1Sources`, `checkGen2Sources` | I ("cannot learn X") | gen1/gen2 `defect-move*`, `defect2-move-*`; `defect2-move-kingler-protect` | Ported; learnsets verified against pret |
| Evolution moves (a move learned on evolving) only when the evolution happened in that generation | GSC | `LearnGroup2.Check` (:30-34, "Disallow Evolution moves if the evo is the last in the list") | history | `canLearn2(..., allowEvolutionMove)` | I | gen2 `grown2` evolved forms | Ported |
| Four-move chronology, Gen I evolution order: an evolved form cannot keep a pre-evolution's move learned after the level at which it must already have evolved — unless Pokémon Stadium 2's reminder re-taught it (it reads RBY cartridges through the Transfer Pak) | RBY | `LearnGroup1.FlagEvolutionSlots` (:41-64; stated but never reached for a PK1), `LearnSource2Stadium.GetCanRelearn` (:29-40), `LearnsetStadium.CanRelearn` (:58) | moves, history | `flagGen1Bookkeeping`, `stadiumRelearnLevel`, `kStadium2Relearn` | **W** only when the reminder cannot restore the move either | gen1 `evo-order` (all legal through the reminder); corpus reason `gen1-evolution-order` | Beyond PKHeX in effect; non-definitive by design |
| Stadium 2 move reminder (level-up moves of the current species up to its level, per cartridge learnset) for Gen I and Gen II records; not for Korean Gold/Silver inside Korea | all | `LearnGroup2` (:139), `LearnSource2Stadium` (`reminder_stad2.pkl`), `ParseSettings.AllowGen2MoveReminder` (:66) | moves, level | `stadiumRelearnLevel`, `checkGen2Sources`, `flagGen1Bookkeeping` (`kStadiumLevelUp*` sources) | ok | gen1 `evo-order`; gen2 `defect2-move-*` | Ported |
| Time Capsule move rules: a PK1 with a Gen II-only move needs a held-item catch rate; a PK2 whose origin is Gen I may carry Gen I-era moves; Korean Gold/Silver have no Time Capsule partner | all | `LearnGroup1.GetPrevious`/`HasPossiblyVisitedGen2` (:17, :151), `LearnGroup2.GetPrevious` (:14-18), `EncounterFinder` (:142-150) | moves, catch rate, language | `evaluate` (`reachable`), `checkMoves`, `mc.anyGen2` | I | gen1 `from-gen2`, `defect-catch*`; gen2 `time-capsule`, `kor2-*` | Ported; the directional candidate history is documented in `GEN12_DIRECTIONAL_TIME_CAPSULE.md` |
| Egg moves and inherited level-up moves, judged as a whole moveset in order (GS and Crystal egg-move tables; Crystal-only egg moves need Crystal; base moves of the species at level 5 first) | GSC | `MoveBreed2.Validate` (:18), `LearnVerifierRelearn.VerifyEggMoveset` (:65-100), `LearnGroup2.CheckEncounterMoves(egg)` (:85-100) | moves, species, version | `validateBreed2`, `verifyBaseMoves`, `recurseOrigins`, `baseEggMoves`, `eggMovesProblem`, `eggMove` | I | gen2 `defect2-egg-moves-*`, `egg2` | Ported |
| Smeargle: Sketch copies nearly anything | GSC | `LearnVerifierHistory.VerifySmeargle` | moves | `isSketchable2` | ok / I for the unsketchable | gen2 Smeargle slots | Ported |
| PP ≤ the move's maximum for its PP Ups (0–3), PP Ups 0 where no PP Up exists, egg PP and PP Ups fixed | all | `MovePPVerifier.VerifyEntity` (:44-60), `VerifyEgg` (:22-30), `GBPKM.GetMovePP` | PP, PP Ups, moves | `basePP`, `maxPP`, `ppProblem`, `formatProblems` | I | gen1/gen2 `defect-pp*`, `defect2-egg-pp-*` | Ported |

### 2.7 Time Capsule both ways

| Rule | Games | PKHeX | Data | Ours | Outcome | Fixtures | Status |
|---|---|---|---|---|---|---|---|
| PK1 ← Gen II: a Gen II origin for a PK1 needs a held-item catch rate, a species ≤ 151 and no Gen II-only move at the Time Capsule (the Capsule refuses them) | RBY | `EncounterGenerator12` (:25-42), `GBRestrictions.GetTradebackStatusRBY` (:172-190), `IsTimeCapsuleTransferred` (:192-212) | catch rate, moves, species | `checkGen1` (`otherGeneration`), `search` (`gen2Candidates` for chainOther), `gen2Possible`, `canVisitGen1` | ok / I | gen1 `from-gen2`; names `japaneseTrainerName` (Rattata with catch rate 83) | Ported |
| PK2 ← Gen I: a Gen I origin for a PK2 needs a species ≤ 151 (or a form of it), no egg, no Crystal met data (the Capsule wipes it), not Korean | GSC | `GBRestrictions.CanOriginateGen1` (:127-145), `EncounterGenerator2.GetEncounters` (chain), `EncounterEnumerator2` | species, egg flag, caught data, language | `checkGen2` (`fromGen1`), `search` (`gen1Candidates` for chainOwn) | ok / I | gen2 `time-capsule`, `defect2-wipe-*` | Ported |
| Gen II Steel written back onto a PK1 (Magnemite/Magneton through Stadium 2) | RBY | `MiscG1Verifier.VerifyMiscG1Types` (:66) | types | `typeProblem` (`kTypeSteel`) | ok | gen1 type fixtures | Ported |
| Directional candidate history (hypothetical Gen II → Gen I → Gen II route with time-indexed move witnesses) | GSC | — | an asserted history | `GEN12_DIRECTIONAL_TIME_CAPSULE.md` solver | Unresolved when the history is not fully specified | its own tests | Beyond PKHeX; never changes a source-free verdict |

### 2.8 Names and language

| Rule | Games | PKHeX | Data | Ours | Outcome | Fixtures | Status |
|---|---|---|---|---|---|---|---|
| Trainer name ≤ 7 characters (5 Japanese, 5 Korean), not empty; nickname ≤ 10 (5 Japanese, 5 Korean); Korean names measured on their bytes | all | `TrainerNameVerifier.VerifyGBOTWithinBounds` (:132-166), `NicknameVerifier.VerifyG1NicknameWithinBounds` (:365-387), `Legal.GetMaxLengthNickname` | decoded names, bytes when present | `trainerNameProblem`, `nicknameProblem`, `koreanLength`, `codePoints` | I | gen1/gen2 `defect-ot-long*`, `defect-nick-*`, `defect2-ot-empty-*`; names `koreanPk1`, `koreanPk2` | Ported |
| Egg nickname is the game's egg name in any Gen II language (EGG, OEUF, UOVO, EI, HUEVO, タマゴ, 알) | GSC | `NicknameVerifier.VerifyNicknameEgg` (:331-350) | nickname, egg flag | `formatProblems(Entity2)` | I | gen2 `defect2-egg-name-*` | Ported |
| Language from the record: Japanese by the 6-byte fields (the parser's region), Korean by a Hangul page lead byte (1–11) in the trainer name, else Western | all | `PK1.Japanese` (list length), `PK1.Korean`/`PK2.Korean` (:15, `StringConverter2KOR.IsHangul`: byte ≤ 0x0B) | name bytes / caller's language | `hangulLead` (1..0x0B; a 0 byte is not a page), `checkGen1`/`checkGen2` language override, `gen1Language` in Legality.cpp (decoded text) | — | names `koreanPk1`, `koreanPk2`, `glyphlessBytes`; corpus `lang` column cross-checked by compare.py | Ported; production decides from decoded text until the hand-off |
| In-game-trade marker `<TRAINER>` (`$5D`) followed by the terminator; the marker only on a Red/Blue/Yellow trade | all | `EncounterTrade1.IsTrainerNameValid` (:70-80) | OT bytes | `rawNameProblems`, `evaluate` (`tradeTrainer`) | I | names `tradeMarker`; corpus `trade-ot-bad-term`, `defect2-trade-ot-bad-term` | Ported (D-3); byte half needs the hand-off |
| A byte with no glyph in the font (PKHeX `StringConverter1/2` TableEN / TableJP NUL entries) before the terminator: the games write none, the decoders stop there | all | `StringConverter1.LoadString` (:109-133, stops at a NUL entry), the table NUL masks | name bytes | `glyphless`, `rawNameProblems` | **W** | names `glyphlessBytes`; corpus `defect-name-glyphless*`, `defect2-name-glyphless*`; reason `name-glyphless` | Beyond PKHeX (PKHeX judges the shorter name); non-definitive |
| Western names: no digit (pokered `data/text/alphabets.asm`, pokecrystal `data/text/name_input_chars.asm`; no species, trade or event name holds one) | RBY, GSC (EN screens) | none (`StringConverter1.GetIsEnglish` accepts digits) | decoded names | `nameCharsetProblems` | **W** (other languages' screens not disassembled) | names `digits`, `production`; corpus `defect-*-digit`, `defect2-*-digit`; reason `name-digit` | Beyond PKHeX; non-definitive |
| Japanese trainer name in kana (U+3000–U+30FC); ？ and ！ only from the Gen II screen, judged per origin | all | `StringConverter1.GetIsJapanese` (:22-31), `StringConverter2.GetIsJapanese` (:25-34), `TrainerNameVerifier` (:145-150 `G1CharOT`, by the first matched encounter's generation) | decoded OT, language, candidate generation | `nameCharsetProblems`, `hasJapaneseMark`, `evaluate` (Gen I candidate + marks → W) | **W** outside the kana; **W** on marks when every origin is Gen I; ok on marks with a Gen II origin (PKHeX INVALID when it matched Gen I first) | names `japaneseTrainerName`; corpus `defect*-jp-ot-charset`, `jp-ot-mark-ok`, `jp-ot-mark-gen1`; reasons `jp-ot-charset`, `jp-ot-mark-gen1`; class `jp-ot-mark-gen1-pick` | Review item 16-D |
| Korean trainer name only on the Hangul pages (`GetIsKorean`) | GD/SI (KOR) | `TrainerNameVerifier` (:152-157) | OT bytes / decoded | `checkGen2` (`oneBytePage`) | **W** (Korean naming screen not disassembled) | gen2 `kor2-ot-one-byte`; corpus reason `korean-trainer-name` | Downgraded (verdicts doc) |
| PKHeX word filter on nicknames | — | `WordFilter` (a PKHeX setting) | — | none: Game Boy games filter no names | ok | corpus class `pkhex-word-filter` | Deliberate difference |
| Japanese Blue version needs a Japanese record; Korean needs Gold/Silver | BU, KOR | `LanguageVerifier` (:33-38) | language, version | `languageFits`, `koreanCartOnly` (a PK1/PK2 stores no version; the encounter's version stands in) | I when the only origin is BU for a non-Japanese record | gen1 `wild-jp`; gen2 `kor2-*` | Ported in effect |

### 2.9 Other fields

| Rule | Games | PKHeX | Data | Ours | Outcome | Fixtures | Status |
|---|---|---|---|---|---|---|---|
| Pokérus strain 0–8 and days ≤ strain % 4 + 1 | GSC | `MiscVerifier.VerifyMiscG12` → Pokérus (:69-76), `PokerusVerifier` | Pokérus byte | `pokerusProblem`, `formatProblems(Entity2)` | I | gen2 `defect2-pokerus-*` | Ported |
| Trainer ID 0 | all | `TrainerIDVerifier` (:53-58, Fishy) | TID | not reported | ok (PKHeX never Invalid) | — | Not ported: no game rule; PKHeX itself only grades it Fishy |
| Friendship (non-egg) | GSC | none | — | none | ok | — | No rule in PKHeX or the games to port |
| Record fields the Integration parsers reject before legality (level > 100, list markers, checksums) | all | — | — | `Gen1::parse`, `Gen2::parse` (Integration) | the save is rejected or the slot skipped | fuzz section 3 | Hardened (sanitizer run) |

## 3. Severity policy and deliberate differences

| Case | PKHeX | Here | Evidence |
|---|---|---|---|
| Stored level ahead of its EXP | derives the level from EXP (VALID) | Invalid | pokered `engine/battle/experience.asm`, `engine/items/item_effects.asm`, `scripts/Daycare.asm:69-83`; pokecrystal `engine/battle/core.asm`, `engine/pokemon/move_mon.asm` `RetrieveBreedmon` (D-1) |
| Stored level behind its EXP; EXP above the level-100 total within `$50FFFF` (PK1 any level, PK2 below 100) | VALID / `LevelEXPTooHigh` | Warning (Day Care slot) | pokered `engine/overworld/daycare_exp.asm:1-16`; pokecrystal `engine/events/happiness_egg.asm:142-190` (15-D) |
| GS Ball Celebi, Japanese Crystal | INVALID (Virtual Console only) | legal | pokecrystal `maps/IlexForest.asm`; Mobile System GB dates (D-2) |
| Mew / Celebi matching no recorded distribution | `EncInvalid` | Warning | the GB-era event list is not known complete; Red/Blue/Yellow's Mew glitch |
| Korean record that only the Korean–Western cartridge trade explains; Korean met data; Korean one-byte-page trainer characters | INVALID (or VALID for the Korean Odd Egg) | Warning | Bulbapedia (secondary); the Korean ROM is not disassembled |
| Hangul-named PK1 | VALID as a Red/Blue/Yellow native | Warning with a Gen II origin and the trade; Invalid without one | no Korean Gen I game exists (17-D) |
| Battle stat experience with a 0 stat | `EffortShouldBeZero` | Warning | pokered `DivideExpDataByNumMonsGainingExp` |
| Gen I evolution-order moves the Stadium 2 reminder cannot restore | never reached for PK1 | Warning | `LearnGroup1.FlagEvolutionSlots` states it; Bulbapedia "Game move errors" for the reminder |
| Digits in Western names; glyphless bytes | VALID | Warning | pokered/pokecrystal naming screens (EN only) |
| Japanese trainer name outside the kana; ？/！ with only Gen I origins | `G1CharOT` | Warning | the Japanese screens are not disassembled (16-D) |
| ？/！ with a Gen II origin open | `G1CharOT` when the first matched encounter is Gen I | legal | PKHeX's own Gen II rule; it does not retry the encounter search (16-D) |
| Word filter | `WordFilterFlaggedPattern` | not applied | the games filter nothing |
| TID 0 | Fishy | not reported | PKHeX never Invalid |

Everything else PKHeX calls Invalid for a PK1/PK2 is Invalid here, and the differential run
fails on any record where the two disagree outside the classes above
(`tools/pkhex_gb_diff/compare.py` `DELIBERATE` and `UNCERTAIN_REASONS`; each class is gated by a
predicate on the record's own bytes and by PKHeX's own complaint codes, and the self-test proves
each predicate rejects the neighbouring cases).

## 4. Tests

| Binary (Makefile.host.gb-legality; `host-test`, `host-sanitize`) | Checks | What |
|---|---|---|
| `test_gb_legality_gen1` | 207 (21 deliberately Incomplete) | PKHeX-verdict fixtures of every Gen I corpus kind through `Legality::analyze`; the Day Care and level/EXP fixtures |
| `test_gb_legality_gen2` | 244 | PKHeX-verdict fixtures of every Gen II corpus kind, every Korean outcome, GS Ball Celebi |
| `test_gb_legality_structural` | 4,781 | duplicate moves in every slot pattern; the level/EXP grid over the four growth rates with the `$50FFFF` / `$510000` / `$FFFFFF` boundaries; a checksum-valid Red save through the production analyzer (bytes unchanged); native PK2 records |
| `test_gb_legality_names` | 169 | trade marker, glyphless bytes, Korean PK1/PK2, digits, Japanese trainer names (kana, digits, ？ per origin on both formats), the production path with decoded names |
| `test_gb_legality_fuzz` | 6 (12,000 random entities, every decoder length, random and truncated saves, 3,000 random records) | no crash, OOB or UB under ASan/UBSan; well-formed results |

All five build and pass under `-fsanitize=address,undefined -fno-omit-frame-pointer -O1`
(`make -f Makefile.host build-host/test_gb_legality_<name>_sanitize`). The GB tests run in CI
through `host-tests.yml` (`HOST_TESTS`, `HOST_SANITIZE_TESTS`); the differential runs in
`legality-gb-pkhex-diff.yml` (seeds 1–3, 50,000 records, both generations).

## 5. Differential test

`tools/pkhex_gb_diff/run.sh PKHEX POKERED COUNT SEED GEN` builds a corpus (`gen_corpus.py` for
PK1, `gen_corpus2.py` for PK2), runs it through PKHeX.Core (`oracle/`, .NET) and through
`GBLegality` with the stored name bytes (`gb_diff.cpp`), classifies every record
(`compare.py`: matching legal / matching invalid / deliberately uncertain / deliberately
different / differing; exit 1 on any differing record or unaccounted record), and then runs the
same corpus through the app's path (`PokemonRecord → Pokemon1/2ReadOnly → Legality::analyze`,
`production_parity.cpp`), which must give the origin search's verdict for every record it can
reach (Korean records are skipped, and the records whose verdict needs the name bytes are
counted). Corpus kinds and the counts of the runs are in `GB_LEGALITY_VERDICTS.md`
("Evidence").

## 6. Not modelled (Unknown)

- Glitch provenance (Mew glitch, Old Man / MissingNo., Celebi egg, Time Capsule glitches): not
  an origin; only Mew's unrecorded-event softening applies.
- Japanese Red/Green/Blue beyond PKHeX's tables (Red/Green slots, Japanese Blue's own slots
  and trades): as complete as those tables.
- Japanese and Korean naming screens and the non-English Western screens: not disassembled;
  the character rules stay Warnings.
- Korean Gold/Silver internals (met data bytes, the Korean naming screen): PKHeX's and
  Bulbapedia's word, so Incomplete.
- The Day Care slot itself: not read by the save parsers.
