# PKHeX Gen IV (PK4) legality rule inventory

Reference for the Gen IV analyzer (`src/Legality/Gen4Legality*.cpp`, `include/Legality/Gen4Legality.h`).
It is a port of PKHeX.Core at kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722 (GPL-3.0) for
PK4 records whose origin is Diamond, Pearl, Platinum, HeartGold or SoulSilver, or Ruby, Sapphire,
Emerald, FireRed or LeafGreen through Pal Park (Colosseum/XD through Pal Park is judged by a Gen III
GameCube judge, section 6). The tables it reads (`Gen4LegalityData.{h,cpp}`) are generated from
that checkout by `tools/gen_legality_gen4_data/run.sh` (`--check` verifies the committed output).
Decomp evidence cites pret/pokeplatinum, pret/pokeheartgold and pret/pokediamond as checked out
under `/home/user/ext` (paths are given from each repository's root). PKHeX line numbers are those
of the pinned checkout.

The analyzer only reads a decrypted 136-byte (PC, "stored") or 236-byte (party) record and returns
a `Legality::Report`. It never writes.

## 1. Invocation path (every rule below is reached through it)

```
Legality::analyze(pk, context)                 src/Legality/Legality.cpp:1084
  -> applyGen4Analyzer(r, pk, exactSourceGameId)   Legality.cpp:3256 (just before the final return)
     -> Pokemon4ReadOnly::decryptedBytes()         src/Legality/Gen4LegalityHook.cpp:55-62
        (136 or 236 bytes; anything else is analyze()'s structure problem)
     -> analyzePk4(data, exactSourceGameId, &evidence)   src/Legality/Gen4Legality.cpp:1752
        -> analyzePk4(..., &Gen4::judgeCxdOrigin)        Gen4Legality.cpp:1776 (reads the record,
           chooses the save's personal table: D/P, Pt or HG/SS; cartKnown = one of the five ids)
           -> Gen4::Analyzer(pk, cart, cxd, cartKnown).run()   Gen4Legality.cpp:1615
              -> findVerifiedEncounter() (:513) then the verify* functions in UpdateChecks order
     -> the hook merges every Invalid/Warning/Info once, certifies only a clean report, and
        yields the older "audited pool" move Warning when the analyzer verified the move (:35-53)
```

"P" in the tables below means exactly this path; the function column names the last step. A rule
whose status is **Implemented** is reached on every production analysis of a native PK4 and has a
positive and a negative test in `tests/test_legality_gen4.cpp` (fixture column: the `kCases`
names, or the section of hand-built records). Severities: Invalid, Warning (PKHeX Fishy; the
record is Incomplete and never certified), Info. "Save" in the data column means the exact source
game id of the save that holds the record (the only save context the engine receives); the save's
language, trainer name and IDs are not passed, so a rule needing them cannot be decided here.

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

## 2. Structure, decoding and dispatch

| Rule | Games | PKHeX | Data | Function | Expected | Fixtures | Status |
|---|---|---|---|---|---|---|---|
| Record size 136 or 236, checksum over 0x08-0x87, sanity word | all | `PK4.cs` (`Valid`: `Sanity == 0 && ChecksumValid`), `PokeCrypto.GetCHK` | record | `read` (:48), `checksum` (:41), `analyzePk4` (:1748-1775) | Wrong size: not analyzed (structure is `analyze()`'s). Checksum or sanity mismatch: Warning, never certified (the games load it as a Bad Egg) | section 3 "malformed" (`junk`, `empty`, sanity word) | Implemented |
| Origin game must be a Gen III or Gen IV version (1-5, 7-12, 15) | all | `EncounterGenerator4/3` yield nothing for another version | record | `analyzePk4` (:1768-1773) | Invalid "cannot produce a PK4" | section 3 (`version` 0, 6, 9, 13, 14, 16, 20, 255) | Implemented |
| Species 1-493 | all | `EncounterGenerator4` (`PersonalTable.HGSS` range) | record | `analyzePk4` | Invalid "does not exist in Generation IV" | section 3 (species 494), `m16273-species` | Implemented |
| Personal table of the save (D/P lack the Platinum forms and have other ability/gender data) | all | `LegalityAnalysis(pk, PersonalTable)` as the oracle does | save | `analyzePk4` (:1776-1779), `run` (:1621) | Form count, abilities and gender ratio come from the save's game; HG/SS when the save is unknown | `m50933-form` (Rotom 3 in Pt), `m50989-form`, fixture driver "479-04 Fan dp formchange egg" (Rotom 4 in D/P: Invalid) | Implemented |
| Party record (236 bytes) vs stored record (136 bytes) | all | `LegalityAnalysis.SlotOrigin` / `IsStoredSlot` (`LegalityAnalysis.cs:67,87`) | record size | `read` (:51 `party`), section 5 rules | Only the three stored-record rules of section 5 depend on it | section 4c | Implemented |

## 3. Encounter matching (EncounterFinder, EncounterVerifier, EvolutionVerifier)

| Rule | Games | PKHeX | Data | Function | Expected | Fixtures | Status |
|---|---|---|---|---|---|---|---|
| Wild slot (grass, surf, rod, Honey tree, Great Marsh, Safari Zone, Trophy Garden, Headbutt, Bug Catching Contest, swarm, radar, GBA dual-slot, Ruins of Alph Unown, Solaceon Ruins) with level range, location, Method J/K frame, lead (Cute Charm, Pressure, Suction Cups, Static/Magnet Pull, synchronize) | D P Pt HG SS | `EncounterGenerator4`, `EncounterEnumerator4`, `EncounterSlot4`, `MethodJ`/`MethodK`, `LeadFinder.GetLeadInfo4`, `EncounterVerifier.VerifyEncounter` | record | `Gen4LegalityEncounters.cpp` `slotMatchExact` (:78), `slotRating` (:94), `slotFixedBall` (:104); `Gen4LegalityRng.cpp` `analyzePidIv` (:438); `findVerifiedEncounter` (:513), `verifyEncounter` (:391) | No template: Invalid (encounter). InvalidFrame on a Gen IV slot: Invalid; on a Gen III slot: Warning (`FramePatternSettings`) | `slot-type0..14-*` (15 slot types), `m102-metlocation`, `m1038-metlocation`, `h41946-metlevel`, `m49-pidlowbit`, `m693-pidlowbit` | Implemented |
| Static encounters incl. roamers (roamer IV bits), fateful statics, starters, gifts, fossils, Spiky-eared Pichu, Korean Shaymin lockout (`EncounterVerifier.cs:33`) | D P Pt HG SS | `EncounterStatic4`, `EncounterVerifier.VerifyEncounterStatic`, `RoamerVerifier` logic in `MethodFinder` | record | `makeTemplate` (:591), `roamerMatch` (`Gen4LegalityRng.cpp:229`), `verifyEncounter` | Invalid when no static matches level/location/ball/fateful/shiny lock | `static`, `static-fateful`, `static-roamer`, `static-egg`, `m11-fateful`, `m34-fateful` | Implemented |
| Pokewalker (course, species, gender from PID, level, met 233, Poke Ball) | HG SS | `EncounterStatic4Pokewalker`, `MethodFinder.GetPokewalkerMatch` | record | `walkerMatchExact` (:159), `walkerRating` (:175), `pokewalkerMatch` (`Gen4LegalityRng.cpp:397`) | Invalid outside the course list or with a non-Pokewalker PID | `walker` | Implemented |
| In-game trades (fixed PID, IVs, gender, OT, nickname, language swap, item, met level) | D P Pt HG SS | `EncounterTrade4PID`, `EncounterVerifier.VerifyEncounterTrade`, `NicknameVerifier.VerifyTrade4` | record | `tradeMatchExact` (:182), `verifyTrade4Names` (:747) | Invalid on any fixed field mismatch | `trade`, section 3 trade names | Implemented |
| My Pokémon Ranch gifts (Hayley: OT, TID, location 3000 or Sinnoh towns, fixed PID/IVs) | D P (Pt via the Japanese update) | `EncounterTrade4RanchGift`, `Encounters4DPPt.RanchGifts` | record | `ranchMatchExact` (:202) | Matched: Warning (Ranch is Wii software with no decomp; provenance unsupported), never Invalid from the match alone | `ranch` | Implemented (Warning, see `PKHEX_DISAGREEMENT_REVIEW.md` 14) |
| Mystery Gift cards (PCD/PGT: species, level, moves, OT, TID/SID, ball, ribbons, fateful, PID type, shiny, Manaphy egg, Ranger Manaphy egg) | D P Pt HG SS | `PCD`, `PGT`, `EncounterGenerator4` (MGDB `wc4.pkl`), `MysteryGiftVerifier` is dead code (no caller in the checkout) | record | `pcdMatchExact` (:217), `pcdRating` (:248), `isRangerManaphy` (:252), `isG4ManaphyPidValid` (:263), `mg4Match` (`Gen4LegalityRng.cpp:270`) | Invalid when no card matches; fateful without any card: Invalid (`run` :1655) | `pcd`, `pgt`, `m2449-egglocation`, `m50908-move`, `m50928-contest` | Implemented |
| Eggs (Day-Care Couple 2000, traded egg 2002, Pt/HG/SS egg field pair, egg moves, PID bit 15, hatch level 1 / met level 0, hatched egg location rules, Korean Seabreak lockout `EncounterVerifier.cs:211`) | D P Pt HG SS | `EncounterEgg4`, `EncounterVerifier.VerifyEncounterEgg`, `EggStateLegality`, `MoveBreed4` | record | `verifyEgg4Hatched` (:381), `moveBreed4Validate` (`Gen4LegalityMoves.cpp:534`), `verifyEncounter` | Invalid for a non-egg species, wrong hatch level, bad egg location, unbreedable move set | `egg-hatched`, `egg-unhatched`, `m3-egg`, `m54-egg`, `m2579-egg`, `m81-egglocation`, `h30959-egglocation`, `h28429-move`, `h31717-move` | Implemented |
| Pal Park (Gen III origin: slot, static, trade, gift, NY/JP/Colo gifts, egg3; Method H frames; met 55; level >= met) | R S E FR LG via D P Pt HG SS | `EncounterGenerator3`, `MethodH`, `CommonEvent3Checker`, `MystryMew`, `ChannelJirachi`, `TransferVerifier.VerifyTransferLegalityG3` | record | `Gen4LegalityPalPark.cpp` `searchEncounters3` (:829/:838), `matchRating3` (:900), `slot3MatchExact` (:504) ... `colo3MatchExact` (:607); `run` (:1650-1652) | Invalid when no Gen III template matches or met location is not 55 | `p0`, `p4`, `p8`, `p9320`, `p10120`, `p28412`, `h33229-move`, section 4b (met 56) | Implemented |
| Colosseum/XD origin through Pal Park (version 15) | Colo XD via D P Pt HG SS | `EncounterGenerator3` Orre templates (`Encounters3Colo`, `Encounters3XD`), `CXDVerifier`, `RibbonRules.GetValidRibbonStateNational` (IShadow3) | record + judge | `findCxdEncounter` (:613), `judgeCxdOrigin` (`Gen4LegalityPalPark.cpp`, integration point) | Judge Matched: template rules (ball, fateful, shiny, moves, National ribbon = shadow) as for any Gen III origin. NoMatch: Invalid. Undecided or no judge (production today): Warning "the Colosseum/XD encounter is not verified", EXP/fateful/ribbon/move rules that need the template are skipped, never certified | section 4b | Implemented: production passes `judgeCxdOrigin`, which runs `Gen3Cxd::judge` (section 6). Undecided judge answers stay Incomplete |
| Evolution chain reaches the species from the encounter (level, item, trade, friendship, form-specific evolutions) | all | `EvolutionVerifier`, `EvolutionTree.Evolves4/Evolves3` | record | `verifyEvolution` (:471), `evolutionHistory` (`Gen4LegalityMoves.cpp:179`) | Invalid when the encounter species cannot evolve into the record's species at its level | `m16273-species`, section 3 | Implemented |
| Encounter-level sanity: current level >= met level, level within slot range, met level of a hatched egg | all | `EncounterVerifier`, `LevelVerifier` | record | `isWithinEncounterRange` (:370), `verifyLevel` (:984) | Invalid | `h41946-metlevel`, `m106-exp`, `m514-exp`, `m2533-exp` | Implemented |

## 4. Verifiers (`LegalityAnalysis.UpdateChecks`, `LegalityAnalysis.cs:315-362`, in order)

| Rule | Games | PKHeX | Data | Function | Expected | Fixtures | Status |
|---|---|---|---|---|---|---|---|
| PID: Wurmple evolution, egg PID bit 15 (`EncounterGenerator4` egg), Day Care PID, shiny locks, PCD PID type, Unown PID form (Gen III), zero PID | all | `PIDVerifier.cs` (`Verify`, `VerifyEC...` not for format 4), `MethodFinder` | record | `verifyPid` (:669) | Invalid as PKHeX; PID 0 Warning | `m34288-pid`, `m49-pidlowbit`, `m693-pidlowbit` | Implemented |
| Language ID 1-8 (Korean only for a Gen IV origin), language 6 never | all | `LanguageVerifier.cs:1-27` (`Legal.GetMaxLanguageID`) | record | `verifyLanguage` (:708) | Invalid | `m82-language`, `m371-language`, `m1873-language` | Implemented |
| Korean GTS sanitized trainer tag (Ahn, Han, Jeong, Hwang) feeding the nickname check | D P Pt HG SS | `LanguageVerifier.cs:44-54` (`Gen4GlobalTradeRules.IsRomanizedKoreanTrainerName`), `NicknameVerifier.cs:265-275` | record | `verifyLanguage` (:727-730, `gtsSanitized_`), `verifyNickname` | Info tag only; nickname rule uses it | section 3 nickname cases | Implemented |
| Korean GTS lockout: a Korean record in a non-Korean save (or the reverse) must have crossed the GTS (no egg, no Classic Ribbon, sanitized OT) | D P Pt HG SS | `LanguageVerifier.cs:56-80` (`CheckGTS`, `ParseSettings.ActiveTrainer`) | save language | - | Needs the save's language, which the engine does not receive; PKHeX without a trainer returns at :65 | - | Missing (not decidable from record + save id) |
| Nickname: species name match when not nicknamed, name length by language, word filter (NX then 3DS regex subsets), egg nickname "Egg" by language, trade nicknames, Gen III origin names | all | `NicknameVerifier.cs`, `WordFilter.cs`, `SpeciesName` | record | `verifyNickname` (:793), `verifyNicknameEgg` (:734), `verifyTrade4Names` (:747), `Gen4LegalityText.cpp` | Invalid as PKHeX; nickname equal to another species' name and a foreign length Warning; an unsupported regex atom never matches (more lenient) | section 3 nickname/word-filter cases, `trade` | Implemented |
| Trainer name: empty, too long for the language, word filter, Gen III/IV character set, trash after terminator | all | `TrainerNameVerifier.cs` (`Verify`, `VerifyOTName`, :152 Korean) | record | `verifyTrainerName` (:882) | Invalid; a suspicious OT Warning | section 3 OT cases | Implemented |
| TID/SID: Ruby/Sapphire TrainerIDNoSeed (`TrainerIDVerifier.cs`), zero / equal / placeholder IDs | all | `TrainerIDVerifier.cs` | record | `verifyTrainerId` (:917) | Invalid for R/S no-seed (Warning for a hatched Gen III egg); zero SID, TID == SID and 12345/54321 Warning | `m194-tid`, fixture driver (SID 0 fixtures: Incomplete) | Implemented |
| IVs: fixed-IV templates, all IVs equal, hyper training N/A | all | `IndividualValueVerifier.cs` | record | `verifyIvs` (:939) | Invalid as PKHeX; all equal Warning | `m26-iv` | Implemented |
| EVs: total <= 510, each <= 255 (`VerifyGainedEVs34`), EVs with no EXP gained, egg EVs 0, all equal | all | `EffortValueVerifier.cs` (:17-20 egg, :50-53 Fishy) | record | `verifyEvs` (:950) | Invalid as PKHeX; all EVs equal Warning; EffortEXPIncreased and Effort2Remaining Info (section 7) | `m6-ev`, `h28449-ev`, `h28518-ev`, section 4b (EVs 1530) | Implemented |
| Level: EXP within the level's bracket, level 100 EXP cap, level >= met level, egg level 1 (met 0) | all | `LevelVerifier.cs` | record | `verifyLevel` (:984) | Invalid; LevelEXPThreshold Info (section 7) | `m106-exp`, `m514-exp`, `m2533-exp`, `m206-exp` (Valid) | Implemented |
| Ribbons: Gen III event/unique/common sets (Pal Park), Gen IV unique (Sinnoh Champ, Legend, Record...) and common (contest ranks by contest stats, Footprint, Alert...), event ribbons only from a matching card, National ribbon iff shadow (Colo/XD), Country/Champion Battle/Regional/National (Gen IV events only) | all | `RibbonVerifier.cs`, `RibbonVerifierCommon3/4.cs`, `RibbonVerifierEvent3.cs`, `RibbonRules.cs` (`GetValidRibbonStateNational`) | record | `verifyRibbons` (:1021) | Invalid; skipped while the Colo/XD origin is undecided | `m94-ribbon`, section 4b (National ribbon vs shadow) | Implemented |
| Ability: slot matches PID bit (Gen IV), pre-capsule Gen III rules, Gen III transfer ability (a species with one Gen III ability keeps it; evolved in Gen IV must match), species' two abilities from the save's table, fixed-ability templates; a Colosseum/XD origin may hold either of two Gen III abilities (`AbilityVerifier.cs:273-274`; Pal Park reads the stored ability bit, pokeplatinum `src/main_menu/gba_pokemon.c:759-783`) and is decided before its encounter is known (the Gen III chain top is the record's own species) | all | `AbilityVerifier.cs:237-295` (`VerifyAbilityPreCapsule`, `VerifyAbilityGen3Transfer`) | record + save table | `verifyAbility` (:1116), `abilityCanMismatch3` (:1152) | Invalid | `m10-ability`, `m49232-ability`, section 4b (XD Eevee, Colosseum/XD and Emerald Doduo) | Implemented |
| Ball: template fixed ball, species/ball breeding rules (Gen IV inheritance: male/genderless Poke Ball), Safari/Sport/Park Ball per encounter, Master Ball rules, HG/SS vs D/P/Pt ball field split | all | `BallVerifier.cs`, `BallContext4` | record | `verifyBall` (:1259), `fixedBall` (`Gen4LegalityEncounters.cpp:430`), `fixedBall3` (`Gen4LegalityPalPark.cpp:813`) | Invalid | `m5-ball`, `m147-ballhgss`, section 4b (fixedBall) | Implemented |
| Form: in range for the save's table (Unown 26/28, Mothim 3 exceptions), Unown letter from PID (Gen III), Giratina Origin iff Griseous Orb, Arceus form = Plate, Rotom egg form 0, battle-only forms (Castform, Cherrim, Deoxys forms outside range are N/A) | all | `FormVerifier.cs` (`VerifyForm`, :150-154 Shaymin) | record + save table | `verifyForm` (:1284; Shaymin :1308-1321) | Invalid; Shaymin Sky in a stored record: Warning (section 5) | `m1-form`, `m50933-form`, `m50989-form`, section 3 Arceus, section 4c Shaymin | Implemented |
| Misc, eggs: hatch counter <= species cycles, no contest stats, no Gen III egg in Gen IV, egg mood 0, shiny leaf 0 | all | `MiscVerifier.cs` (`Eggs.Verify`), `EggVerifier` | record | `verifyMisc` (:1329-1336, :1338) | Invalid; egg mood Warning | `m3-egg`, `m54-egg`, `m2579-egg`, `m50928-contest` | Implemented |
| Misc G4: Pal Park met-location field pair (`VerifyMetLocation3`, :35-45), shiny leaf (:60-67), split ball (:69-94) | D P Pt HG SS | `MiscVerifierG4.cs` | record | `verifyMisc` (:1345-1351 Pal Park pair and shiny leaf, :1373-1383 ball fields), `verifyBall` | Invalid | `m79-shinyleaf`, `m50889-metlocext`, `p*` Pal Park fixtures | Implemented |
| Misc G4: walking mood (:115-134) | HG SS | `MiscVerifierG4.cs` | record + save | `verifyMisc` (:1338 egg, :1339-1344 stored record) | Warning as PKHeX (Fishy) | section 4c | Implemented |
| Met-location field pair of a native record (Pt/HG/SS write both fields through one setter) | D P Pt HG SS | none (PKHeX reads the Pt/HG/SS field first, `G4PKM.cs:229-237`) | record | `verifyMisc` (:1352-1371) | Invalid when the D/P field is not what Pt/HG/SS write for the Pt/HG/SS field (non-egg, non-Pal-Park, Pt/HG/SS field set); Warning for a Ranch gift | `met-location-dp-field` (section 3), title-matrix Gen IV rows | Implemented, deliberately stricter (`PKHEX_DISAGREEMENT_REVIEW.md` 14; `PKHEX_VERIFIED_IMPROVEMENTS.md` Gen IV) |
| Fateful encounter flag: set iff the template is fateful (statics, cards, Ranger Manaphy, Gen III events, Orre shadow/XD) | all | `MiscFatefulEncounterVerifier.cs` | record | `verifyFateful` (:1402) | Invalid; skipped while the Colo/XD origin is undecided | `m11-fateful`, `m34-fateful`, section 4b | Implemented |
| Encounter details (met level/location vs template, egg location vs hatched state) | all | `MiscEncounterDetailsVerifier.cs` | record | `verifyEncounter`, `verifyEgg4Hatched`, `verifyMisc` | Invalid | `m102-metlocation`, `m81-egglocation` | Implemented |
| Pokerus: strain/days combinations (days <= strain % 4 + 1, strain 0 with days) | all | `MiscPokerusVerifier.cs` | record | `verifyMisc` (:1385-1387) | Invalid | `m50-pokerus` | Implemented |
| Dates: met date and egg date validity, egg date only with an egg location, met date for a hatched egg | all | `MiscDateVerifier.cs` | record | `verifyMisc` (:1388-1399) | Invalid | section 3 date cases | Implemented |
| Misc evolution (`MiscEvolutionVerifier`: form argument / Gen VII+ data) | - | `MiscEvolutionVerifier.cs` | - | - | Format >= 7 only | - | N/A for a PK4 |
| Scale (`MiscScaleVerifier`) | - | - | - | - | Format >= 8 only | - | N/A |
| Move PP: PP Ups 0-3, no PP Ups on an empty slot or Sketch, PP <= healed maximum; egg PP = base PP and no PP Ups | all | `MovePPVerifier.cs:11-69, 80-88` | record | `verifyMovePP` (:1426) | Invalid | `m7-pp`, `m34134-pp`, `h28438-pp`, section 4b (PP 99) | Implemented |
| Move PP below the healed maximum in a stored record (`GetPermittedStatePP` OnlyHealed for a Box/GTS/BattleBox PK4) | all | `MovePPVerifier.cs:110-143` | record size | `verifyMovePP` (:1446-1456) | Warning (section 5) | section 4c | Implemented (Warning where PKHeX-with-slot says Invalid) |
| Gender: from PID and the save's gender ratio, fixed-gender templates, genderless species, Gen III gender bits | all | `GenderVerifier.cs` | record + save table | `verifyGender` (:1459), `gift3MatchGender` (`Gen4LegalityPalPark.cpp:425`) | Invalid | `m47-gender`, `m2953-gender`, `m12590-gender` | Implemented |
| Held item: Gen IV item range (<= 467), unreleased/key items, egg holds nothing, Griseous Orb outside Platinum (`ItemVerifier` with `Legal.HeldItems_Pt/HGSS`) | all | `ItemVerifier.cs` | record | `verifyItem` (:1492) | Invalid | `m170-item`, `h28473-item` | Implemented |
| Contest stats: sheen formula vs stats (Gen III and Gen IV Poffin/Pokeblock granting), contest stats on an egg, contest ribbons vs stats | all | `ContestStatVerifier.cs`, `ContestStatGranting.cs`, `ContestStatGrantingSheen.cs` | record | `verifyContest` (:1499) | Invalid | `m71-contest`, `m50928-contest` | Implemented |
| Markings: Gen IV has 6 marking bits; bits 6-7 unused | all | `MarkingVerifier.cs` | record | `verifyMarking` (:1589) | Invalid | section 3 marking case | Implemented |
| Ground tile (encounter type byte): allowed set per template (`GroundTileVerifier`, format 4 only, `LegalityAnalysis.cs:338`) | D P Pt HG SS | `GroundTileVerifier.cs`, `GroundTileAllowed` | record | `verifyGroundTile` (:1593), `groundTileAllowed` (`Gen4LegalityEncounters.cpp:422`) | Invalid | section 3 ground tile cases | Implemented |
| Slot type (`SlotTypeVerifier`, :340: a Pokemon in a slot that cannot hold it, e.g. an egg in the Battle Box) | - | `SlotTypeVerifier.cs` | slot type | - | PKHeX uses the slot origin; a PK4 save has no such slot restriction (Gen IV Battle Box does not exist; party/box both hold eggs) | - | N/A |
| History/Memory/ConsoleRegion/Medal/HyperTraining/Trash/Mark (:344-362) | - | - | - | - | Format >= 6/7/8 only | - | N/A for a PK4 |
| Learn verification: every current move learnable along the evolution chain (level-up per game table, TM/HM, type tutors, HG/SS special tutors, enhanced tutors, egg moves via the Day Care, Sketch, event/Pokewalker moves, Gen III sources for a Pal Park record), duplicate moves, empty move 1 | all | `LearnVerifier.cs`, `LearnGroup4.cs`, `LearnGroup3.cs`, `LearnSource4DP/Pt/HGSS`, `MoveBreed4` | record | `verifyMoveResults` (:1604), `Gen4LegalityMoves.cpp` (`learnGroup4Check` :421, `verifyBaseMoves` :492, `moveBreed4Validate` :534) | Invalid; Arceus with Draco Meteor Warning (section 7); skipped while the Colo/XD origin is undecided | `m133-move`, `m213-move`, `m363-move`, `m654-move`, `m13919-move`, `m25929-move`, `m50908-move`, `h28429-move`, `h31717-move`, `h33229-move`, section 4b | Implemented |
| TransferVerifier.VerifyTransferLegalityG3: a Gen III origin was met at Pal Park (55) | R S E FR LG Colo XD | `TransferVerifier.cs` | record | `run` (:1650-1652) | Invalid | `p*` fixtures, section 4b (met 56) | Implemented |
| TransferVerifier.VerifyTransferLegalityG4: format-5 (Poke Transfer) rules | - | `TransferVerifier.cs` | PK5 | `run5` (:1675) via the Gen V analyzer | Lane C's path, documented in `GEN5_PKHEX_RULE_INVENTORY.md` | `tests/test_legality_gen5.cpp` | Implemented (Gen V lane) |
| IsPotentiallyMysteryGift: a fateful record with no matching template and another Invalid gets the EventGift hint | D P Pt HG SS | `LegalityAnalysis.cs` (`IsPotentiallyMysteryGift`) | record | `run` (:1654-1657) | Invalid "Fateful encounter without a matching Mystery Gift" | `m34-fateful` | Implemented |
| MysteryGiftVerifier.VerifyGift | - | `MysteryGiftVerifier.cs` | - | - | No caller in the pinned checkout (dead code) | - | N/A |

## 5. Stored-record rules (slot origin)

PKHeX applies three rules only when it knows the slot the record came from
(`LegalityAnalysis.IsStoredSlot`, `LegalityAnalysis.cs:87`: `SlotOrigin == type || SlotOrigin is
Ignore`). Without a slot origin (the default, and what the Gen III/IV oracle uses) it treats every
record as if it could be a party record and never fires them. Here a 136-byte record is by
construction a PC record (the Gen IV save reader builds party entities from 236-byte slots,
`src/Integration/Gen4/Gen4ReadOnlySave.cpp:240`), so the analyzer can apply them. Each is a
**Warning**, not Invalid: every in-game store path in the decomps behaves as PKHeX assumes, but the
D/P and HG/SS PC box applications are not in their decomps, and every game runs the Wii link (My
Pokémon Ranch, Pokémon Battle Revolution) from the module `data/eoo.dat` (pokeplatinum
`src/main_menu/main_menu.c:1317`, pokeheartgold `src/application/main_menu/main_menu.c:1503`),
which no decomp covers. A Warning leaves the record Incomplete, never certified, as the verdict
policy requires for an unsupported provenance; it never invalidates.

| Rule | Games | PKHeX | Here | Evidence | Fixtures |
|---|---|---|---|---|---|
| Move PP below the healed maximum in a stored (136-byte) record | D P Pt HG SS | `MovePPVerifier.cs:110-143`: Box/GTS/BattleBox PK4 is OnlyHealed, Invalid `MovePPExpectHealed` | Warning per move, `verifyMovePP` (:1446-1456); party records and eggs untouched | Every PC store function heals PP: pokeplatinum `src/pc_boxes.c:59-110` (callers: box UI `src/applications/pc_boxes/box_app_manager.c:3952-4017`, GTS `src/gts_application/screens/network_handler.c:1284-1328`, catching show `src/scrcmd_catching_show.c:70`, battle capture `src/battle/battle_script.c:10748`); pokeheartgold `src/pokemon_storage_system.c:54-100` (`src/overlay_bug_contest.c:227`, `src/scrcmd_12.c:68`, `src/battle/battle_command.c:7025`); pokediamond `arm9/src/pokemon_storage_system.c:46-90` (`arm9/src/scrcmd_13.c:73`). D/P and HG/SS box UI and `eoo.dat`: no decomp | section 4c |
| Shaymin Sky Forme in a stored record | Pt HG SS (D/P: form out of range already) | `FormVerifier.cs:150-154`: Invalid `FormParty` for format <= 6 | Warning, `verifyForm` (:1308-1321): Platinum text when the save is Platinum, "cannot be verified" otherwise | Platinum reverts Shaymin in every PC store function (`src/pc_boxes.c:59-110`), the Day Care (`src/overlay005/daycare.c:112`), the GTS deposit (`src/gts_application/screens/deposit.c:994-998`) and the trade room (`src/applications/trade_room/application.c:623`). HG/SS revert only party Shaymin (pokeheartgold `src/pokemon.c:3574-3610` time/freeze, `src/get_egg.c:93` Day Care, `src/field/scrcmd_pokemon_misc.c:469-470`); `src/pokemon_storage_system.c:54-100` does not touch the form | section 4c |
| Walking mood not 0 in a stored record of an HG/SS save | HG SS | `MiscVerifierG4.cs:115-134`: Fishy `G4PartnerMoodZero` when `ActiveTrainer is SAV4HGSS` | Warning, `verifyMisc` (:1339-1344), only when the save id is heartgold_nds/soulsilver_nds (`cartKnown_`); D/P/Pt saves and party records untouched | HG/SS zero the mood when a Pokemon becomes the walking lead (pokeheartgold `src/follow_mon.c:1946-1992`) and on Day Care entry (`src/get_egg.c:91`); the mood byte is copied by trades and never written by D/P/Pt | section 4c |

The 236-byte party record of the same bytes gets none of these (section 4c tests both sizes
through `analyzePk4` and through `Legality::analyze`). In the Gen III/IV differential these
Warnings turn PKHeX-VALID records with PP mutated below the maximum into Incomplete (not false
Invalid); the certification rate therefore reads lower than before and is reported as such.

## 6. Colosseum/XD origin integration point (version 15)

`Gen4::judgeCxdOrigin(const CxdOriginInput&)` at the end of `src/Legality/Gen4LegalityPalPark.cpp`
runs Lane A's `Gen3Cxd::judge` on the fields Pal Park keeps (original level bounded by the met
level, as PKHeX's `EvolutionGroup3.Devolve` bounds it; the ability bit only when the Gen III species
has two abilities, since Pal Park picks the ability from that bit, pokeplatinum
src/main_menu/gba_pokemon.c:759-783). `analyzePk4` (:1776) calls the
judge only for version 15 and hands its `CxdOriginJudgement` to the analyzer, whose
`findCxdEncounter` (:613) turns a `Matched` judgement into an `EncKind::Cxd3` template (species,
form, level range, fixed ball, fateful, shiny, moves, fixed trainer, shadow) and runs the same
Gen III-origin rules as any Pal Park record (`matchRating3`, `isFixedTrainer3`, `verifyEncounter`,
`evolutionHistory3`, `verifyMoves3`, `verifyEvolution`, ribbons with National iff shadow,
fateful, ball). `NoMatch` is Invalid with the judge's reasons; `Undecided`/`Unavailable` is the
Warning "Pal Park transfer from Colosseum/XD: the Colosseum/XD encounter is not verified" with the
template-dependent rules skipped. The judge's generic TID/SID heuristics are dropped from its
warnings because the Gen IV trainer check already reports them. The 4-argument `analyzePk4`
overload takes any `CxdOriginJudge`; a null judge (tests only) leaves every version-15 record
Incomplete.

## 7. Severity policy and deliberate differences

Every difference in the analyzer is more lenient than PKHeX except one, which is stricter and
rests on decomp evidence (`PKHEX_DISAGREEMENT_REVIEW.md` item 14):

| Case | PKHeX | Here | Evidence |
|---|---|---|---|
| A native non-egg record whose D/P met-location field is not what Pt/HG/SS write for its Pt/HG/SS met location (the same place if D/P know it, else Faraway Place 3002) | VALID: it reads the Pt/HG/SS field first (G4PKM.cs:229-237) and compares the two only for Pal Park (MiscVerifierG4.cs:35-45) | Invalid; Warning for a My Pokemon Ranch record | Pt and HG/SS write both fields through one setter: pokeplatinum src/pokemon.c:1732-1740 with src/unk_02017038.c:38-45; pokeheartgold src/pokemon.c:1286-1292 with asm/unk_02017FAC.s:68-97; every write path is mapped in review item 14. Every PKHeX Gen IV event template leaves the Pt/HG/SS field at 0. The Ranch is Wii software with no decomp |

The more lenient differences:

| Case | PKHeX | Here | Evidence |
|---|---|---|---|
| Fishy results other than the ones below | Fishy, still VALID | Warning: the record is Incomplete, never No problems found | PKHeX's own grading |
| EffortEXPIncreased (levels gained, no EVs) | Fishy (EffortValueVerifier.cs:50-51) | Info | Rare Candy and the Day Care add EXP and no EVs: pokeplatinum src/item_use_pokemon.c:291-293, src/overlay005/daycare.c:221-224; pokeheartgold src/use_item_on_mon.c:294-296, src/get_egg.c:147-150 |
| LevelEXPThreshold (EXP exactly at a level's minimum) | Fishy (LevelVerifier.cs:56-57) | Info | A Rare Candy adds exactly the EXP to the next level: pokeplatinum src/item_use_pokemon.c:291-293 with src/pokemon.c:2313-2319; pokeheartgold src/use_item_on_mon.c:294-296 |
| Effort2Remaining (EV total 508) | Fishy (EffortValueVerifier.cs:52-53) | Info | Battle EV gain stops only at 510: pokeplatinum src/battle/battle_script.c:10237, include/constants/pokemon.h:15-16; pokeheartgold include/constants/pokemon.h:9 |
| Arceus that knows Draco Meteor | Invalid (PersonalInfo4.SpecialTutorDracoMeteor omits Arceus) | Warning | The Pt and HG/SS tutors check the party Pokemon's current type, and Arceus takes its Plate's type: pokeplatinum res/field/scripts/scripts_route_210_grandma_wilma_house.s:79-81 and 129-132, src/pokemon.c:1221-1222; pokeheartgold files/fielddata/script/scr_seq/scr_seq_0948_T30R0601.s:251-252, src/pokemon.c:862-863. The other Ultimate-move tutors are species-based (pokeplatinum res/field/scripts/scripts_route_228_north_house.s:96) and unchanged |
| Checksum or sanity word does not match the data | not checked by the legality analysis | Warning, never certified | The games load such a record as a Bad Egg |
| Colosseum/XD origin (version 15) without a judge | judged with the Orre encounter tables | Warning, encounter not verified; template-dependent rules skipped | Section 6: the Orre judge is Lane A's; the consumer is in place |
| Stored-record rules (section 5) | Invalid / Fishy with a slot origin; nothing without one | Warning | Section 5 |
| A word-filter pattern outside the supported regular-expression subset | matched | never matches | Can only make the filter more lenient |

## 8. Older Gen IV checks in Legality::analyze

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
| Arceus Draco Meteor | learnable | PKHeX: not learnable; the games: learnable while holding a Draco Plate | section 7 (the analyzer follows the games, as a Warning) |

Where the audited pool is too narrow, the yield above removes the false Warning once the analyzer
has verified the move; where it is too wide (Wormadam's cloaks, Head Smash) the analyzer's own
Invalid applies, as in PKHeX.

## 9. Differential test

`tools/pkhex_gen34_diff/run.sh PKHEX_CHECKOUT SEED MUTATIONS 34|4 base|grown` compares every
verdict with PKHeX (base: every Gen IV encounter template plus single-field mutations; grown:
PKHeX-valid records played forward, traded between carts and moved through Pal Park, plus
mutations). It fails on any false Invalid (PKHeX VALID, Invalid here), false Valid (PKHeX INVALID,
No problems found here) or unloadable record. The one exception is the stricter rule in section 7:
its records are listed as "deliberately stricter" and pass only when a byte-level predicate
(`met_location_fields` in `summarize.py`) confirms the two fields disagree. The oracle emits every
PK4 as its 136 stored bytes without a slot origin (`oracle/Program.cs:416-430`), so the section 5
Warnings count there as "PKHeX VALID, ours Incomplete", never as false Invalid. CI runs seeds 1-3
for both corpora (`.github/workflows/legality-gen34-pkhex-diff.yml`).

## 10. Remaining gaps

| Gap | Why it cannot be decided here | What would decide it |
|---|---|---|
| Korean GTS lockout (`LanguageVerifier.CheckGTS`) | needs the save's language | pass the save's language through `Legality::Context` |
| Colosseum/XD origin templates | the Orre judge is Lane A's | wire `judgeCxdOrigin` (section 6) |
| Stored-record rules as Invalid | D/P and HG/SS box UI and `data/eoo.dat` are not in any decomp | a witness for those store paths (then PKHeX parity would allow Invalid for the PP rule) |
| My Pokémon Ranch gifts | Wii software, no decomp | stays a Warning |
