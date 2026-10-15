# PKHeX Gen V (PK5) legality rule inventory

Reference for the Gen V analyzer. Read from PKHeX.Core at kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722 (GPL-3.0). Line numbers marked ~ are approximate.

```text
PKHeX.Core Gen V (PK5) LEGALITY SPEC
Checkout: /home/user/ext/pkhex, commit 6501f0ab46e8f8ca048539dbaf8cae8cb104e722. All paths below are relative to /home/user/ext/pkhex/PKHeX.Core/.

Line numbers are given where I verified them. A line written as "~N" is approximate; grep the named symbol to find it.

Severity legend:
- I = Invalid
- F = Fishy
- V = Valid (an informational check result)

All "Lgl" message keys come from LegalityCheckStrings.

======================================================================
0. TOP-LEVEL FLOW (Legality/LegalityAnalysis.cs:183-363)
======================================================================

Step 1: EncounterFinder.FindVerifiedEncounter (Legality/Encounters/Generator/EncounterFinder.cs).
- It iterates candidates from EncounterGenerator5, or from Gen4/Gen3 when the PK5 is a transfer.
- For each candidate it runs these checks in order:
  1. EncounterVerifier.
  2. Relearn (format < 6 fills all relearn results Valid).
  3. LearnVerifier on moves.
  4. Evolution verification.
- If a candidate fails and more candidates remain, it moves to the next one.
- Final sanity check: if the template's GetMatchRating is >= PartialMatch, it tries the next candidate. If none remain, it adds EncInvalid (I).
- If no encounter is found at all, it adds a hint, in this order:
  1. EggLocation == 60003 (Breeder5) and not fateful: EncGift.
  2. Fateful and EggDay != 0: EncGiftEggEvent.
  3. Fateful, or MetLocation in 40001..49999: EncGiftNotFound.
  4. Otherwise: EncInvalid.
  All four are I.
- LegalityAnalysis.IsPotentiallyMysteryGift: if the result is invalid, the PK5 is fateful and the generation is < 6, it adds FatefulGiftMissing (I).

Step 2: parse dispatch.
- PK5 of Gen5 origin: UpdateChecks only.
- Gen4 origin: ParsePK4 also adds Transfer.VerifyTransferLegalityG4.
- Gen3 origin: ParsePK3 also adds VerifyTransferLegalityG3.

UpdateChecks order:
1. PIDEC
2. LanguageIndex
3. Nickname
4. Trainer (name)
5. TrainerID
6. IVs
7. EVs
8. Level
9. Ribbon
10. AbilityValues
11. BallIndex
12. FormValues
13. MiscValues (MiscVerifier, which includes MiscVerifierPK5, fateful, Pokérus and date)
14. MovePP
15. GenderValues
16. Item
17. Contest
18. Marking
19. GroundTile (formats 4/5/6)
20. SlotType

At format < 6 it returns here. History, Memory, Consoles, etc. are not run for PK5.

======================================================================
1. PK5 STRUCTURE (PKM/PK5.cs)
======================================================================

Sizes and encryption:
- Stored size 0x88; party size 0xDC.
- Checksum (0x06) = sum of u16 words over [0x08..0x88).
- The data is encrypted and shuffled with the PID: four 0x20 blocks A, B, C, D at 0x08, 0x28, 0x48, 0x68.
- EncryptionConstant == PID.
- PSV = ((PID>>16) ^ (PID&0xFFFF)) >> 3.
- TSV = (TID ^ SID) >> 3.

Header and block A:

| Offset | Size / bits | Field |
|---|---|---|
| 0x00 | u32 | PID |
| 0x04 | u16 | Sanity |
| 0x06 | u16 | Checksum |
| 0x08 | u16 | Species |
| 0x0A | u16 | HeldItem |
| 0x0C | u16 | TID16 |
| 0x0E | u16 | SID16 |
| 0x10 | u32 | EXP |
| 0x14 | u8 | OT Friendship (the hatch counter while an egg) |
| 0x15 | u8 | Ability |
| 0x16 | u8 | Markings; bits 0-5 = circle, triangle, square, heart, star, diamond |
| 0x17 | u8 | Language |
| 0x18-0x1D | 6 x u8 | EVs: HP, ATK, DEF, SPE, SPA, SPD |
| 0x1E-0x23 | 6 x u8 | Contest stats: Cool, Beauty, Cute, Smart, Tough, Sheen |
| 0x24 | bits 0-7 | RIB0: ChampionSinnoh, Ability, AbilityGreat, AbilityDouble, AbilityMulti, AbilityPair, AbilityWorld, Alert |
| 0x25 | bits 0-7 | RIB1: Shock, Downcast, Careless, Relax, Snooze, Smile, Gorgeous, Royal |
| 0x26 | bits 0-7 | RIB2: GorgeousRoyal, Footprint, Record, Event, Legend, ChampionWorld, Birthday, Special |
| 0x27 | bits 0-3 | RIB3: Souvenir, Wishing, Classic, Premier (bits 4-7 unused) |

Block B:

| Offset | Size / bits | Field |
|---|---|---|
| 0x28, 0x2A, 0x2C, 0x2E | u16 each | Move1-4 |
| 0x30-0x33 | u8 each | PP1-4 |
| 0x34-0x37 | u8 each | PPUps1-4 |
| 0x38 | u32 | IV32: HP bits 0-4, ATK 5-9, DEF 10-14, SPE 15-19, SPA 20-24, SPD 25-29; bit 30 IsEgg; bit 31 IsNicknamed |
| 0x3C | bits 0-7 | RIB4: G3Cool, G3CoolSuper, G3CoolHyper, G3CoolMaster, G3Beauty x4 |
| 0x3D | bits 0-7 | RIB5: G3Cute x4, G3Smart x4 |
| 0x3E | bits 0-7 | RIB6: G3Tough x4, ChampionG3, Winning, Victory, Artist |
| 0x3F | bits 0-7 | RIB7: Effort, ChampionBattle, ChampionRegional, ChampionNational, Country, National, Earth, World |
| 0x40 | bits | bit 0 FatefulEncounter; bits 1-2 Gender; bits 3-7 Form |
| 0x41 | u8 | Nature (stored explicitly in Gen5) |
| 0x42 | bits | bit 0 HiddenAbility flag; bit 1 NSparkle |
| 0x43-0x47 | | unused |

Block C:

| Offset | Size / bits | Field |
|---|---|---|
| 0x48 | 22 bytes | Nickname: 11 u16 chars, 0xFFFF-terminated, max length 10 |
| 0x5E | | unused |
| 0x5F | u8 | Version (origin game) |
| 0x60 | bits 0-7 | RIB8: G4 Cool x4, G4 Beauty x4 |
| 0x61 | bits 0-7 | RIB9: G4 Cute x4, G4 Smart x4 |
| 0x62 | bits 0-3 | RIBA: G4 Tough x4 (rest unused) |
| 0x63 | | RIBB unused |
| 0x64-0x67 | | unused |

Block D:

| Offset | Size / bits | Field |
|---|---|---|
| 0x68 | 16 bytes | OT name: 8 u16 chars, max length 7 |
| 0x78, 0x79, 0x7A | u8 each | Egg date: year (minus 2000), month, day |
| 0x7B, 0x7C, 0x7D | u8 each | Met date: year (minus 2000), month, day |
| 0x7E | u16 | EggLocation |
| 0x80 | u16 | MetLocation |
| 0x82 | u8 | Pokérus: low nibble = days, high nibble = strain |
| 0x83 | u8 | Ball |
| 0x84 | bits | bits 0-6 MetLevel; bit 7 OT gender |
| 0x85 | u8 | GroundTile (Gen4 encounter type) |
| 0x86 | | unused |
| 0x87 | u8 | PokeStarFame (IsPokeStar = value > 250) |

Party data:

| Offset | Size | Field |
|---|---|---|
| 0x88 | | Status |
| 0x8C | u8 | Level |
| 0x8D | | junk byte |
| 0x8E | | current HP, MaxHP, Atk, Def, Spe, SpA, SpD (u16 each) |
| 0x9C | 0x38 bytes | Mail |
| 0xD4 | 8 bytes | junk |

Derived fields:
- AbilityNumber (PK5.cs:58) = HiddenAbility ? 4 : (1 << PIDAbility).
- PIDAbility depends on the origin version:
  - B, W, B2 or W2 origin: (PID >> 16) & 1.
  - Gen3/4 origin: PID & 1.
  - CXD origin: the ability index.

Limits (Legal.cs:41-47):

| Constant | Value |
|---|---|
| MaxSpeciesID_5 | 649 |
| MaxMoveID_5 | 559 |
| MaxItemID_5_B2W2 | 638 |
| MaxAbilityID_5 | 164 |
| MaxBallID_5 | 0x19 (Dream = 25) |
| MaxGameID_5 | B2 (23) |
| Max EV | 255 |
| EV total | 510 |

Game IDs: W = 20, B = 21, W2 = 22, B2 = 23.

No legality check reads the unused bytes or the unused ribbon bits.

======================================================================
2. PID RULES (Legality/Verifiers/PIDVerifier.cs)
======================================================================

- PID == 0: PIDZero (F).
- Nature > 24: PIDNatureMismatch (I).
  - Gen5 origin has no nature == PID % 25 requirement.
  - Gen3/4 origin requires Nature == PID % 25 (enforced in GenderVerifier, see topic 4).
- There are no EC checks for format 5 (EC == PID).
- Shiny constraint: enc.Shiny.IsValid(pk) must be true, else EncStaticPIDShiny (I).
  - IsShiny means (TID ^ SID ^ PIDhi ^ PIDlo) < 8.
  - Shiny.Always requires shiny; Shiny.Never requires not shiny.
  - AlwaysSquare requires xor == 0; AlwaysStar requires xor == 1 (Legality/Encounters/Templates/Enums/Shiny.cs).
  - FixedValue (N's Pokémon, trades) is checked by exact PID match in the template.

Gen5 fixed-gender PID rule (PIDVerifier.cs ~82-88):
- Applies to Gen5 encounters implementing IFixedGender with Gender 0 or 1, except EncounterTrade5BW and PGF.
  - Male (0): (PID & 0xFF) must not be 254 or 255.
  - Female (1): (PID & 0xFF) must not be 0.
  - Failure: EncConditionBadRNGFrame (I).
  - If the encounter is also an EncounterStatic5 with IsWildCorrelationPID, the correlation check below also runs.
- EncounterStatic5 with IsWildCorrelationPID gets the correlation check.
  - IsWildCorrelationPID = !IsRoaming && Shiny == Random && FixedBall != Poke && Ability != OnlyHidden.

EncounterSlot5:
- HiddenGrotto slot that is shiny: G5PIDShinyGrotto (I).
- Any other slot: the correlation check.

Correlation check (Legality/RNG/ClassicEra/MonochromeRNG.cs, GetBitXor):
  ((TID ^ SID) & 1) ^ (PID & 1) ^ (PID >> 31) must equal 0, else PIDTypeMismatch (I).

Wurmple: VerifyECPIDWurmple. The evolution is chosen by the EC (= PID): (EC >> 16) % 10 / 5 picks Silcoon or Cascoon. A mismatch is I.

MonochromeRNG generation, for reference:
- Shiny PID = ((low ^ TID ^ SID) << 16) | low, then the ability bit 16 is forced.
- Anti-shiny: PID ^= 0x10000000.

G5MGShiny detection (Legality/RNG/MethodFinder.cs:380-393):
- Requires low = PID & 0xFFFF <= 0xFF.
- Then PID must equal MonochromeRNG.GetShinyPID(low, (PID >> 16) & 1, TID, SID).
- It is reached through GetModified8BitMatch (lines 79-82).
- It is used only by the PGF shiny check (topic 4, Fateful).

======================================================================
3. HIDDEN ABILITY / ABILITY (Legality/Verifiers/Ability/AbilityVerifier.cs)
======================================================================

1. Ability not in personal {Ability1, Ability2, AbilityH} for the current species and form: AbilityUnexpected (I).

2. Basculin special case (lines 100-134). Applies to a PK5 Basculin with form 1 (Blue), no HA, and ability != Adaptability:
- Expected is Rock Head.
- B2/W2 eggs expect Reckless.
- B/W encounters that are not EncounterTrade5BW expect Reckless.
- Otherwise: AbilityUnexpected.

3. MysteryGift (VerifyAbilityMG, lines 298-334), using the PGF AbilityType:
- 4 (Any12H): Valid.
- 3 (Any12): AbilityNumber == 4 gives AbilityMismatchGift (I).
- 0/1/2 (fixed): AbilityNumber must equal 1 << type.
  - HA mismatch: AbilityHiddenFail.
  - Otherwise, at format < 6: AbilityMismatchPID (I).

4. Format < 6 goes to VerifyAbility345 (lines 162-184):
- If the encounter ability permission is fixed or OnlyHidden:
  - (AbilityNumber == 4) != (permission == OnlyHidden): AbilityHiddenFail (I).
  - A fixed slot goes to VerifyFixedAbility.
- Gen5 encounters go to VerifyAbility5 (lines 360-369):
  - EncounterEgg5 with AbilityNumber == 4 but no HA permission (species is in BanHidden5): AbilityHiddenUnavailable (I).
  - Otherwise CheckMatch with state MustMatch.
- CheckMatch:
  - Gen3/4 origin with the HA bit set: AbilityHiddenUnavailable (I).
  - GetPIDAbilityMatch: personal.GetAbilityAtIndex(AbilityNumber >> 1) must equal Ability, else AbilityMismatchPID (I).
  - Mapping: AbilityNumber 1 is index 0, 2 is index 1, 4 is index 2 (HA).
- Gen3-origin special rules are in lines 237-296: species with 2 abilities in Gen3, and the PID-bit / CXD ability index.

BanHidden5 (Legality/Verifiers/Ability/AbilityBreedLegality.cs) is a bitset: byte = species >> 3, bit = species & 7. These species cannot pass HA via a Gen5 egg:
1, 4, 7, 81, 92, 100, 109, 120, 128, 132, 137, 152, 155, 158, 200, 201, 236, 252, 255, 258, 287, 311, 337, 338, 343, 351, 352, 355, 358, 374, 387, 390, 393, 420, 433, 436, 479, 489, 495, 498, 501, 511, 513, 515, 538, 539, 562, 566, 570, 574, 597, 599, 602, 607, 615, 622, 627, 633, 636.

EncounterEgg5.Ability = IsHiddenPossible5(species) ? Any12H : Any12.

Template ability permissions:
- Grotto slot: OnlyHidden.
- Normal slot: Any12.
- DreamWorldEntry: OnlyHidden if PersonalTable.B2W2[species] has a distinct HA (AbilityH != Ability1), else OnlyFirst.
- Dream Radar: OnlyHidden (Rotom: Any12).
- Statics: as listed in the tables (e.g. Musharna, Darmanitan, Eevee, Deerling, Jellicent, Mandibuzz/Braviary are OnlyHidden).

Partial-hidden species (IsPartialMatchHidden): a species with HA whose current species differs from the original gives PartialMatch. The list is Metapod, Kakuna, Pupitar, Silcoon, Cascoon, Vibrava, Flygon.

======================================================================
4. BALL, LOCATIONS, EGGS, LEVELS AND MISC
======================================================================

--- Ball (Legality/Verifiers/Ball/BallVerifier.cs, BallUseLegality.cs) ---
- Ball > 25: invalid.
- The check uses EncounterOriginal. Nincada → Shedinja is special-cased.
- IFixedBall with FixedBall != None: Ball must equal it.
- Egg encounters below Gen6: Poke ball (4) only.
- EncounterStatic5Entree (Dream World): DreamWorldBalls = WildPokeBalls5 + Dream (25).
- Otherwise WildPokeBalls5 = {Master 1, Ultra 2, Great 3, Poke 4, Net 6, Dive 7, Nest 8, Repeat 9, Timer 10, Luxury 11, Premier 12, Dusk 13, Heal 14, Quick 15}. Safari (5), Cherish (16), Sport (24), Dream and the Apricorn balls are excluded.
- Dream Radar: FixedBall Dream.
- PGF: Ball must equal the gift's ball (in IsMatchExact).
- Gen3/4 transfers keep their ball; it is checked against Gen3/4 rules.
- Failure: BallEncMismatch (I).

--- Location constants (Game/Locations/Locations.cs) ---

| Name | Value |
|---|---|
| LinkTrade5 | 30003 |
| LinkTrade5NPC | 30002 (in-game trade; spin-trade egg location) |
| Daycare5 | 60002 |
| Breeder5 | 60003 |
| HatchLocation5 | 64 |
| Transfer4 (Poké Transporter) | 30001 |
| Transfer4_CelebiUnused / Used | 30010 / 30011 |
| Transfer4_CrownUnused / Used | 30012 / 30013 |
| Dream Radar | 30015 |
| Transfer3 (Pal Park) | 0x37 (Gen4 met location) |

Range and set helpers:
- Event range: 40000 < loc < 50000.
- IsEggLocationBred5 = {60002, 30003, 30002}.
- IsMetLocation5BW: loc <= 116.
- TradedEggLocation(5) = 30003.

--- Hatched Gen5 egg (EncounterVerifier.VerifyEncounterEgg5, lines 225-237) ---
- MetLevel must be 1, else EggFMetLevel_0 (I).
- EggHatchLocation5.IsValidMet5(met, version), else EggLocationInvalid (I).

EggHatchLocation5 (Legality/Encounters/Verifiers/EggHatchLocation5.cs):
- Version mask: W = 1, B = 2, W2 = 4, B2 = 8 (bit index = version - 20).
- A location is valid if location < 144 and (table[location] >> (version - 20)) & 1.

LocationPermitted5[144] table:

| Index | Value |
|---|---|
| 0-3 | 0 |
| 4-22 | 15 |
| 23 | 3 |
| 24-39 | 15 |
| 40 | 3 |
| 41-58 | 15 |
| 59 | 3 |
| 60-72 | 15 |
| 73 | 3 |
| 74-111 | 15 |
| 112 | 10 (B/B2 only, Black Gate) |
| 113 | 5 (W/W2 only, White Gate) |
| 114-116 | 15 |
| 117-137 | 12 (B2W2 only) |
| 138 | 0 |
| 139-143 | 12 |

--- Unhatched egg in PK5 (VerifyUnhatchedEgg5, lines 348-362) ---
1. MetLevel must be 1 (EggFMetLevel_0, I).
2. EggLocation in {30003, 30002}: EggLocationTradeFail (I).
3. MetLocation in {30003, 30002}: Valid (EggLocationTrade).
4. MetLocation == 0: Valid (EggUnhatched).
5. Anything else: EggLocationNone (I).

--- Egg state (Legality/Verifiers/Egg/EggVerifier.cs, EggStateLegality.cs) ---
- While an egg:
  - Friendship (hatch counter) must be in [0, PersonalInfo.HatchCycles], else EggHatchCycles (I).
  - Contest stats must be 0 (EggContest, I).
  - EVs must be 0 (EffortEgg, I).
  - HeldItem must be 0 (I).
  - PP Ups must be 0 and PP must equal base PP.
  - Ribbons: none, except event ribbons matching the gift.
  - Encounter context must be Gen5 (TransferEggVersion, I).
  - The nickname flag must be set (expected for non-Gen4 encounters) and the nickname must be the language's egg name.
- Egg met level constant: 1 for Gen5; 0 for Gen3/4.
- Hatch friendship baseline: 120 for Gen5.

--- Met level, level and EXP (Legality/Verifiers/LevelVerifier.cs) ---
- MysteryGift:
  - Gift Level must equal MetLevel unless the original met location is lost. Failure: LevelMetGift (I).
  - Gift level > current level: LevelMetGiftFail (I).
- Egg: Level == enc.LevelMin and EXP == the growth-table EXP for that level (I).
- Level 100: EXP must equal the level-100 EXP.
- Level < MetLevel: LevelMetBelow (I).
- Level not within the encounter range and EXP exactly at the level threshold: LevelEXPThreshold (F).
- IsWithinEncounterRange:
  - Gen5 origin with MetLevel != 0: Level == MetLevel.
  - Gen3/4 origin: within the encounter range. In PK5, MetLevel is the level at transfer.

Template met-level rules:
- Statics/trades/N: MetLevel == template Level.
- Slots: LevelMin <= MetLevel <= LevelMax.
- Dream Radar: MetLevel is a multiple of 5 in 5..40.
- Gen4 templates in format 5:
  - EncounterStatic4.cs:347: when Format != 4, the requirement is Level <= evo.LevelMax.
  - EncounterSlot4.cs:~137: evo.LevelMax >= LevelMin.

--- Encounter type / ground tile (Legality/Verifiers/GroundTileVerifier.cs) ---
- Byte 0x85 must be 0 unless the encounter implements IGroundTypeTile (Gen4 origin). In that case it must be in the template's allowed tile set.
- Failure: EncTypeMismatch (I).

--- Fateful (Legality/Verifiers/Misc/MiscFatefulEncounterVerifier.cs) ---
- PGF:
  - RestrictLanguage != 0 and Language differs: EncGiftLanguageNotDistributed (I). RestrictLanguage is always 0 from the pkl.
  - PIDType == 2 (always shiny): PIDIV must be G5MGShiny (MethodFinder.cs:380), unless the PK5 was traded as an egg (EggLocation, or MetLocation for an egg, in 30003/30002). Failure: PIDTypeMismatch (I). The check is at ~line 69.
  - Fateful must equal the gift's flag, which is true for PGF. Failure: FatefulMysteryMissing / FatefulInvalid (I).
- Gen5 non-gift templates: fateful must be false (FatefulInvalid, I).
- Gen4 templates with a fateful requirement need it set (FatefulMissing, I). Examples: event statics such as Darkrai/Shaymin/Arceus and the PCD/PGT gifts.

--- N's sparkle / PokéStar (Legality/Verifiers/Misc/MiscVerifierPK5.cs) ---
- PokeStarFame (0x87) != 0:
  - Egg: EggShinyPokeStar (I).
  - Encounter species Ditto: G5PokeStarMustBeZero (I).
  - fame % 25 not in {0, 5}: G5PokeStarImpossibleValue (I).
- NSparkle (0x42 bit 1):
  - Must be set iff the encounter is EncounterStatic5N.
  - Missing on N: G5SparkleRequired (I).
  - Set on anything else: G5SparkleInvalid (I).

--- Pokérus (Legality/Verifiers/Misc/MiscPokerusVerifier.cs, Pokerus.cs) ---
- Any strain 0..15 is accepted for PK5.
- Days must be <= (strain & 3) + 1, else PokerusDaysLEQ_0 (I).

--- EVs (Legality/Verifiers/EffortValueVerifier.cs) ---
- Egg with nonzero EVs: EffortEgg (I).
- Sum > 510: EffortAbove510 (I). There is no per-stat 252 cap in format 5.
- Fishy cases:
  - Sum 0 while Level > MetLevel (not within the encounter range): EffortEXPIncreased (F).
  - Sum == 508: Effort2Remaining (F).
  - All six equal and nonzero: EffortAllEqual (F).

--- Dates (Legality/Verifiers/Misc/MiscDateVerifier.cs) ---
- MetLocation != 0 requires a valid calendar date (2000 + Y, M, D). Failure: DateCalendarInvalidMet (I).
- MetLocation == 0 requires the met date to be all zero.
- The same rules apply to EggLocation and the egg date (DateCalendarInvalidEgg, I).

--- Language (Legality/Verifiers/LanguageVerifier.cs) ---
- Valid range is up to Korean (8). Language 6 is unused and invalid.
- Language 0 is invalid, except for EncounterTrade5BW while in format 5 (IsValidMissingLanguage).

--- Nickname (Legality/Verifiers/NicknameVerifier.cs) ---
- Empty name: NickLengthShort (I).
- EncounterTrade5BW in format 5:
  - Language == Japanese (1): OTLanguageShouldBe_0 (I).
  - Then the effective language is max(lang, 1) for name matching.
  - OT must equal the template's OT (EncTradeChangedOT, I).
  - Nickname must equal the template's nickname for that language (EncTradeChangedNickname, I).
- B2W2 trades: same OT/nickname checks. Yancy/Curtis have no fixed nickname.
- PGF with no nickname but the PK5 is nicknamed: EncGiftNicknamed (I).
- Nicknamed:
  - Equals another species' name: F.
  - Contains East Asian characters while the language is non-Asian (or the reverse): I.
  - Length > 10 for Western or > 5 for JPN/KOR: NickLengthLong (I).
  - More than 4 digits (generation < 6): I.
  - Word filter (if enabled): WordFilter5 over badwords_gen5. Normalization: uppercase, katakana → hiragana, fullwidth → halfwidth. It must be an exact whole-string match.
- Not nicknamed: must equal the species name in the PK5's language.
  - Exceptions: evolved, or traded eggs, may carry any language's name.
  - Gen3/4 origin uses IsMatch45 (Gen4 names; Farfetch'd apostrophe handling).

--- OT / TID (Legality/Verifiers/TrainerNameVerifier.cs, TrainerIDVerifier.cs) ---
- These apply only when the player is the OT: not a fixed-trainer template, and not a non-egg gift.
- Name:
  - Empty: OTShort (I).
  - Starts with "PKHeX": F.
  - Length > 12 for Western or > 6 for Asian: OTLong (I). There are traded-egg edge cases.
  - Word filter and the 4-digit rule also apply.
- TID/SID (all F):
  - ID32 == 0.
  - TID == SID.
  - TID == 0 or SID == 0.
  - (12345, 54321).
  - (15040, 18831).

--- Item (Legality/Verifiers/ItemVerifier.cs) ---
- Egg holding an item: I.
- Item must be in ItemStorage5 (General + Medicine + Berry) and not Unreleased {5, 16, 260-264, 492-500, 576}. Failure: ItemUnreleased (I).

--- Contest stats (Legality/Verifiers/ContestStatVerifier.cs) ---
- Gen5 origin in format < 6: all must be 0 (ContestZero, I).
- Gen3/4 origin: Gen3/4 sheen correlation rules apply.
- PGF: the stats must be >= the gift's stats.

--- Markings (Legality/Verifiers/MarkingVerifier.cs) ---
Byte 0x16 must be <= 0x3F, else MarkValueUnusedBitsPresent (I).

--- Move PP (Legality/Verifiers/MovePPVerifier.cs) ---
- PP Ups on Sketch or on an empty move slot: I.
- PP must be <= the max for its PP Ups.
- Eggs: PP Ups 0 and PP == base.
- PK5 stored in Box/GTS/BattleBox must be fully healed (OnlyHealed).

--- IVs (Legality/Verifiers/IndividualValueVerifier.cs) ---
- PGF fixed IVs: a value <= 31 must match. 0xFC-0xFE mean a count of flawless IVs. Any other value is random.
- All IVs equal and < 30: F.

--- Gender (Legality/Verifiers/GenderVerifier.cs) ---
- Genderless mismatch: GenderInvalidNone (I).
- Gen3/4/5 origin: the gender must follow the PID.
  - Expected = (PID & 0xFF) < ratio ? Female : Male.
  - Fixed ratios: 0 = all male, 254 = all female, 255 = genderless.
- Species whose gender was fixed from a dual-gender line use the original species' ratio. These are Shedinja, Wormadam, Mothim, Vespiquen, Gallade, Froslass (IsFixedGenderFromDual).
- Failure: PIDGenderMismatch (I).
- Gen3/4 origin also requires Nature == PID % 25 (PIDNatureMismatch, I).

--- Form (Legality/Verifiers/FormVerifier.cs) ---
- Form must be < the personal FormCount.
- Arceus form must equal the plate type. Transfer to PK5 sets form 0 and removes the item.
- Giratina Origin requires Griseous Orb (112).
- Genesect form must match the held drive.
- Keldeo: a mismatch is allowed for Gen5 origin.
- Battle-only forms are invalid: Darmanitan Zen (form & 1), Meloetta Pirouette (form != 0), Castform != 0, Cherrim != 0.
- Shaymin Sky (form != 0) outside the party: FormParty (I).
- Rotom egg with form != 0: I.
- Deerling/Sawsbuck form is free for Gen5 origin (IsFormChangeable).

--- Ribbons (Legality/Verifiers/Ribbons/*) ---
- Event ribbons (Classic, Wishing, Premier, Event, Birthday, Special, Souvenir, ChampionWorld/Battle/Regional/National, Country, National, Earth, World) must match the PGF's ribbons.
  - With no gift encounter they are invalid.
  - Exceptions: National (Gen3 Colosseum/XD shadow) and Earth (Gen3).
- Gen3-only ribbons: ChampionG3, Artist, Winning, Victory, and the G3 contest ribbons (cumulative tiers).
- Effort needs a Gen3 or Gen4 origin.
- Gen4-only ribbons:
  - Ability* ribbons (Battle Tower).
  - G4 contest ribbons (not Ditto, Unown, or Spiky Pichu).
  - Legend, ChampionSinnoh, the daily ribbons.
  - Cosmetic ribbons, with the Gorgeous → Royal → GorgeousRoyal prerequisite.
- Footprint: not obtainable for format < 6 without a Gen3/4 origin.
- Record: always invalid.
- Eggs: all ribbons invalid except matching gift ribbons.

======================================================================
5. GEN 3/4 → 5 TRANSFERS
======================================================================

PK4 → PK5 conversion (PKM/PK4.cs:315-383, ConvertToPK5):
- OT friendship = 70.
- Arceus: form 0, item 0.
- Items not held in BW are removed.
- Nature = PID % 25.
- Pt/HGSS extra met data (0x44-0x47) and HGSS bytes 0x86-0x87 are cleared.
- MetLocation = PK5.GetTransferMetLocation4 (PKM/PK5.cs:571-587):
  - Not fateful, or not Gen4 origin: 30001.
  - Fateful Raikou, Entei or Suicune: 30012 (Transfer4_CrownUnused).
  - Fateful Celebi: 30010 (Transfer4_CelebiUnused).
  - Otherwise: 30001.
- EggLocation is unchanged (Gen4 values). Ball is unchanged.
- MetLevel = current level at transfer.
- HMs are removed: PersonalInfo4.GetPreferredTransferHMs(hasDefog) returns the HGSS HM list if the Pokémon knows Defog, else the DPPt list.
- Shedinja gender = 2.
- Nickname and OT glyphs are re-encoded.

Verifier (Transfer.VerifyTransferLegalityG4):
- Met location must be 30001, or 30010-30013 for Celebi and the crown beasts, as appropriate.
- Species must be <= 493.

Gen4-origin egg in PK5 (EncounterVerifier.cs:191-202):
- Must not still be an egg (TransferEgg, I).
- MetLevel >= 1.
- MetLocation == 30001 (TransferEggLocationTransporter, I).

Gen3-origin egg in PK5 (EncounterVerifier.cs:166-189):
- Must not still be an egg.
- MetLevel >= 5.
- EggLocation == 0.
- MetLocation == 30001.

Gen3 origin (ParsePK3 + VerifyTransferLegalityG3):
- Pal Park (0x37) was the Gen4 met location, which the PK4→PK5 conversion overwrote with 30001.
- Gen3 ability rules (AbilityVerifier.cs 237-296) apply.
- Gen4 template matching in format 5:
  - EncounterStatic4.cs:347-350: met level is lost, so Level <= evo.LevelMax is required instead of an exact met level.
  - EncounterSlot4.cs:~128-138: the exact met-level check applies only in format 4; otherwise evo.LevelMax >= LevelMin.

Learnset: LearnGroup5 → LearnGroup4 (if the encounter is not Gen5) → LearnGroup3. EvolutionGroup5 → previous group Gen4 when origin generation < 5.

======================================================================
6. MOVE LEGALITY AND BINARY FORMATS
======================================================================

LearnGroup5 (Legality/LearnSource/Group/LearnGroup5.cs):
- For each evolution stage in the Gen5 chain, check B2W2 sources:
  - Level-up (level <= the stage's max level).
  - TM/HM bits.
  - Type tutor.
  - Special (move) tutors.
  - Enhanced tutor.
- Then BW level-up only.
- Deoxys, Giratina and Shaymin check all forms.
- EncounterEgg5 additionally allows:
  - Egg moves.
  - Inherited level-up moves.
  - Volt Tackle (344) for Pichu.
- Then the previous group (Gen4/Gen3) for transfers.
- No HM-removal check is done in the verifier; only the conversion removes HMs.

Sources:
- LearnSource5BW.cs: lvlmove_bw.pkl, BinLinker ident "51", 668 entries, indexed by personal form index.
  - TM check excludes index 94 (TM95 Snarl is B2W2-learnable only).
  - Enhanced tutor:
    - Keldeo: Secret Sword (548).
    - Meloetta: Relic Song (547).
    - Rotom forms: 1 Overheat (315), 2 Hydro Pump (56), 3 Blizzard (59), 4 Air Slash (403), 5 Leaf Storm (437).
- LearnSource5B2W2.cs: lvlmove_b2w2.pkl, ident "52", 709 entries.
- LearnSource5.cs: egg moves from eggmove_bw.pkl, ident "bw", 637 entries indexed by species. They are shared by BW and B2W2.

Other move rules:
- LearnVerifier: no duplicate moves, no gaps (a nonzero move after an empty slot), Move1 nonzero.
- Kyurem: Fusion Flare (558) only in form 1, Fusion Bolt (559) only in form 2. A fused Kyurem must not have Scary Face below format 8.
- Smeargle: any move 1..559 except Struggle (165) and Chatter (448) is valid via Sketch.
- IMoveset templates (Dream World Entree) mark their move as valid.
- LearnVerifierEgg:
  - An unhatched Gen5 egg is validated strictly by MoveBreed5.
  - A hatched egg uses LearnGroup5 plus CheckEncounterMoves.

MoveBreed5 (Legality/.../MoveBreed5.cs):
- Origin order: Base (level-up at level 1), FatherEgg, ParentLevelUp, FatherTM, then Volt Tackle last for Pichu.
- The TM list is MachineMoves[..^1], which drops the last entry (HM06 Dive, 291) and keeps TM95. This is a PKHeX quirk.
- Egg moves come from the BW table.
- Learnset form 0 uses BW for B/W origin and B2W2 otherwise.

Binary formats:

BinLinker (BinLinkerAccessor):
- [0x00] 2-byte ASCII ident.
- [0x02] u16 count.
- [0x04] (count + 1) u32 offsets. Entry i spans [off[i], off[i+1]).
- BinLinkerAccessor16 has the same layout with u16 offsets.

Learnset entry (LearnsetReader):
- n = length / 3; entry 0 is empty.
- Layout: [u16 move x n][u8 level x n].

Egg move entry: u16 move list.

evos_g5.pkl:
- Ident "g5", 650 entries indexed by species.
- Each 8-byte record: u8 method, u8 pad, u16 argument, u16 species, u8 form, u8 level.
- level-up adjustment = 1 for level-up methods.
- EvolutionTree.Evolves5 = GetViaSpecies(PersonalTable.BW).
- EvolutionGroup5 uses the B2W2 personal table.

Personal tables:
- personal_bw: 0x3C bytes per record, 668 records.
- personal_b2w2: 0x4C bytes per record, 709 records.

| Offset | Field |
|---|---|
| 0x00-0x05 | base stats |
| 0x06, 0x07 | types |
| 0x08 | catch rate |
| 0x12 | gender ratio |
| 0x13 | HatchCycles |
| 0x14 | base friendship |
| 0x15 | EXPGrowth |
| 0x18, 0x19, 0x1A | Ability1, Ability2, AbilityH |
| 0x1C | u16 FormStatsIndex |
| 0x20 | FormCount |
| 0x28 | TM/HM bitfield, 101 bits: TM01-TM95 then HM01-HM06 |
| 0x38 | type tutor byte |
| 0x3C | Driftveil tutor bits (B2W2 only) |
| 0x40 | Lentimas tutor bits (B2W2 only) |
| 0x44 | Humilau tutor bits (B2W2 only) |
| 0x48 | Nacrene tutor bits (B2W2 only) |

- Type tutor bits, in order: Grass Pledge 520, Fire Pledge 519, Water Pledge 518, Frenzy Plant 338, Blast Burn 307, Hydro Cannon 308, Draco Meteor 434, Dragon Ascent 620. The last is not Gen5; it is listed for completeness of the bit field.
- B2W2 tutor lists:
  - Driftveil (15): 450, 343, 162, 530, 324, 442, 402, 529, 340, 067, 441, 253, 009, 007, 008.
  - Lentimas (17): 277, 335, 414, 492, 356, 393, 334, 387, 276, 527, 196, 401, 399, 428, 406, 304, 231.
  - Humilau (13): 020, 173, 282, 235, 257, 272, 215, 366, 143, 220, 202, 409, 355.
  - Nacrene (15): 380, 388, 180, 495, 270, 271, 478, 472, 283, 200, 278, 289, 446, 214, 285.
- MachineMoves: PersonalInfo5BW.cs:130-145 has the 95 TM move IDs, then the HMs 015, 019, 057, 070, 127, 291.
- Form index = FormStatsIndex + form - 1 when form > 0 and form < FormCount; otherwise the species index.

======================================================================
7. ENCOUNTER DATA FORMATS AND STATIC TABLES
======================================================================

Wild areas (EncounterArea5.cs, EncounterSlot5.cs):
- Files: encounter_b/w/b2/w2.pkl, BinLinker ident "51" (BW) or "52" (B2W2).
- File sizes: b 14744, w 14896, b2 20812, w2 20784 (B has 355 areas).
- Area layout: u16 location, u8 SlotType5, u8 pad, then n slots of 4 bytes.
- Slot layout: u16 (species = value & 0x3FF, form = value >> 11), u8 min level, u8 max level.
- SlotType5: Standard 0, Grass 1, Surf 2, Super_Rod 3, Swarm 4, HiddenGrotto 5.
- Grotto slots: Shiny Never, OnlyHidden. Other slots: Shiny Random, Any12.
- IsMatchExact:
  - Met location == area location (checked by the enumerator).
  - MetLevel within [min, max].
  - Form match, except Deerling/Sawsbuck.
- GetMatchRating:
  - HA partial-hidden case: PartialMatch.
  - Combee slot where the PK5 is a Vespiquen with (EC & 0xFF) >= 0x1F (i.e. Combee would have been male): DeferredErrors.
  - HA on a non-grotto slot, or no HA on a grotto slot: Deferred.
  - Otherwise: Match.

EncounterStatic5 (Templates/Gen5/EncounterStatic5.cs):
- Fields: Version, IsRoaming, FixedBall, Species, Level, Location (byte), Ability, Form, Shiny, EggLocation, Gender (random by default).
- IsEgg = EggLocation != 0.
- IsMatchExact:
  - Egg location:
    - Non-egg template: pk.EggLocation == 0.
    - Hatched: EggLocation == the template's, or 30003/30002.
    - Unhatched: exact.
  - Met location:
    - Egg template: !pk.IsEgg, or met in {Location, 30003, 30002}.
    - Non-roamer: met == Location.
    - Roamer: bit `met` of 0b10111111111111111000000000000000 must be set, i.e. locations 15-29 and 31.
  - MetLevel == Level.
  - Fixed gender equals pk.Gender.
  - Form == evo.Form, or the form is changeable.
- IsMatchPartial (PartialMatch):
  - OnlyHidden and AbilityNumber != 4.
  - EggLocation 60002 with RelearnMove1 != 0 (format >= 6 only).
  - HA on a partial-hidden species.

EncounterStatic5N (N's Pokémon):
- Fixed PID; ID32 = 2 (TID 2, SID 0); IVs all 30.
- OT "N", or fullwidth "Ｎ" for Japanese. OT gender 0.
- Gender 0, or 2 if genderless. Version B2W2. FixedBall Poke for Zorua only.
- Requires an exact match of PID, egg location, met location, met level, form, gender, OT gender, IVs and ID32. Always rated Match.
- PIDs used: 0xFF01007F, 0xFF00003F, 0xFF00007F, 0xFF000000, 0xFF00001F. 15 entries are in Encounters5B2W2.cs.

EncounterStatic5Entree (Dream World):
- Location 75; Shiny Never; FixedBall None (Dream World ball set).
- IsMatchExact: egg location 0, met location 75, MetLevel == Level.
- PKHeX BUG (EncounterStatic5Entree.cs:94): on a fixed-gender mismatch the method returns TRUE and skips the form check. Decide whether to replicate it.
- Moves: one special move (IMoveset).
- Always rated Match; ability is handled by AbilityVerifier.

DreamWorldEntry: compact entries with up to 3 moves, expanded into one template per move.
- Ability: OnlyHidden if B2W2 personal AbilityH != Ability1, else OnlyFirst.
- Lists: DreamWorld_BW, DreamWorld_B2W2, DreamWorld_Common (Encounters5DR.cs, version Gen5).

EncounterStatic5Radar (Encounters5DR.cs, 26 entries):
- Location 30015; Ball Dream; Shiny Never.
- OnlyHidden by default; Rotom is Any12.
- Therian forms of 641/642/645 use form 1.
- MetLevel % 5 == 0 and 5 <= MetLevel <= 40.

EncounterTrade5BW:
- Location 30002; fixed PID; SID 0.
- IsMatchExact: MetLevel, IVs exact, PID, Nature, ID32, Form, OT gender, egg location.
- Language 0 is allowed while in format 5.
- PIDs:
  - Emolga 0xD400007F
  - Rotom 0x2A000000
  - Munchlax 0x6200001F
  - Petilil 0x64000000
  - Basculin 0x9400007F (form 0 in B, form 1 in W)
  - Cottonee 0x6400007E

EncounterTrade5B2W2:
- u32 ID32; Shiny Never.
- IVs are checked only if specified. Gender and nature are checked only if fixed.
- Yancy (F, TID 10303) / Curtis (M, TID 54118): lv50, HA, random gender and nature, no fixed nickname.
- OT names:
  - Female: Yancy / Brenda / Lilì / Sabine / Belinda / ルリ / 루리.
  - Male: Curtis / Julien / Dadi / Markus / Julián / テツ / 철권.

EncounterEgg5:
- Hatch location 64; EggLocation 60002; FixedBall Poke; Shiny Random.
- Ability Any12H or Any12 per BanHidden5.

Static highlights (full tables in Legality/Encounters/Data/Gen5/Encounters5BW.cs and Encounters5B2W2.cs):
- BW:
  - Starters at location 4 (Poke ball).
  - Fossils lv25 at location 7.
  - Elemental monkeys lv10 at location 32.
  - Magikarp at location 68.
  - Larvesta egg: EggLocation 60003, Location 0.
  - Musharna lv50 at location 32 and Darmanitan lv35 at location 34, both OnlyHidden.
  - Victini lv15 at location 62, Never.
  - Zorua lv10 at location 8 (gender 0) and Zoroark lv25 at location 72 (gender 1), both Never.
  - Black: Reshiram at locations 45/39 (Never); Tornadus roams lv40.
  - White: Zekrom; Thundurus roams.
- B2W2:
  - Eevee HA lv10 at location 8, gender 0, Never.
  - Deerling HA lv30 at location 19.
  - Happiny egg: EggLocation 60003.
  - Haxorus lv60 at location 147, Shiny Always.
  - B2: Gible lv1 at location 122 (Always, gender 0, Poke ball); Latios; Jellicent HA female-0 at location 71; Mandibuzz HA; Zekrom lv70.
  - W2: Dratini; Latias; Jellicent HA gender 1; Braviary HA; Reshiram.

PGF (MysteryGifts/PGF.cs; pgf.pkl = 144636 bytes = 709 records of 0xCC):

| Offset | Field |
|---|---|
| 0x00 | TID |
| 0x02 | SID |
| 0x04 | OriginGame |
| 0x08 | PID |
| 0x0C | RIB0: Country, National, Earth, World, Classic, Premier, Event, Birthday |
| 0x0D | RIB1: Special, Souvenir, Wishing, ChampionBattle, ChampionRegional, ChampionNational, ChampionWorld |
| 0x0E | Ball |
| 0x10 | Item |
| 0x12-0x18 | Moves (4 x u16) |
| 0x1A | Species |
| 0x1C | Form |
| 0x1D | Language |
| 0x1E | Nickname (11 chars) |
| 0x34 | Nature (0xFF = random) |
| 0x35 | Gender (2 = random) |
| 0x36 | AbilityType: 0/1/2 fixed, 3 = Any12, 4 = Any12H |
| 0x37 | PIDType: 0 never shiny, 1 random, 2 always shiny |
| 0x38 | EggLocation |
| 0x3A | MetLocation |
| 0x3C | MetLevel |
| 0x3D-0x42 | Contest stats |
| 0x43-0x48 | IVs: HP, ATK, DEF, SPE, SPA, SPD (0xFF = random) |
| 0x4A | OT (8 chars) |
| 0x5A | OTGender (3 = the receiving player's) |
| 0x5B | Level |
| 0x5C | IsEgg |
| 0x60 | Title |
| 0xAC-0xAE | Date |
| 0xB0 | CardID |
| 0xB3 | CardType |
| 0xB4 | Flags |

PGF notes:
- PKHeX quirk: the Level setter writes 0x5C instead of 0x5B. The getter reads 0x5B.
- RestrictLanguage and RestrictVersion are 0 in the pkl.
- Shiny mapping: PIDType 0 on a non-egg is Never; 2 on a non-egg is Always; anything else is Random.

PGF.IsMatchExact (lines 399-445):
- Non-egg gifts:
  - SID and TID must match.
  - OT name must match.
  - OT gender must match if OTGender < 3.
  - PID must match if nonzero.
  - Shiny constraint.
  - OriginGame must match.
  - Language must match if nonzero.
  - EggLocation and MetLocation must match.
- Egg gifts:
  - EggLocation must equal the gift's, or the hatched egg was traded (30003/30002).
  - An untraded PIDType 0 egg must not be shiny.
  - An unhatched egg requires format 5.
- Both:
  - Form must match.
  - MetLevel == Level.
  - Ball must match.
  - Nature must match if fixed.
  - Gender must match if != 2.
  - Each contest stat must be >= the gift's.
- Partial match: language restriction mismatch, or the version cannot receive the gift.

Legality/Restrictions/Locale/LocaleNDS5.cs holds Unity Tower country data only and is not used for PK5 legality.

======================================================================
8. ENCOUNTERGENERATOR5 SEARCH ORDER AND DEFERRAL
======================================================================

Setup (Legality/Encounters/Generator/ByGeneration/EncounterGenerator5.cs):
- Chain = EvolutionChain.GetOriginChain(pk, origin 5, context Gen5).
- Minimum level = max(1, MetLevel).

State machine (Legality/Encounters/Generator/Search/EncounterEnumerator5.cs):

1. Start (lines 162-170):
   - MetLocation == 30002 and not an egg: go to Trades.
   - Else if not fateful: go to Bred.
   - Else go to Event:
     1. MGDB_G5 (pgf.pkl).
     2. EGDB_G5 (user files).
     If anything was yielded, STOP. Otherwise go to Bred.
2. Bred:
   - Requires EggLocation in {60002, 30003, 30002} and TryGetEgg. Yield the egg.
   - Then TryGetSplit: yield the split-breed egg.
   - If EggLocation == 60002: STOP (nothing else is possible). Otherwise go to StartCaptures.
3. Trades (TradeStart):
   - Version-specific list first (TradeGift_W / B / W2 / B2), then the shared list (TradeGift_BW or TradeGift_B2W2).
   - If anything was yielded, STOP; otherwise go to StartCaptures.
4. Statics:
   - BW: StaticW or StaticB, then Encounter_BW, then DreamWorld_BW, then DreamWorld_Common.
   - B2W2: StaticW2 or StaticB2, then Encounter_B2W2_Regular, then Encounter_B2W2_N, then DreamWorld_B2W2, then Encounter_DreamRadar, then DreamWorld_Common.
5. Slots:
   - Skipped unless EncounterStateUtil.CanBeWildEncounter (EncounterStateUtil.cs:13-20): not an egg, and EggLocation == 0.
   - Iterate SlotsW / B / W2 / B2 areas where area.Location == MetLocation.
6. Fallback: if a Deferred candidate exists, yield it.

Any other version throws.

Matching loop (lines 355-422), for every template:
- Walk the chain until evo.Species == template species. If none matches, skip the template.
- If IsMatchExact fails, break (skip the template).
- rating = GetMatchRating(pk).
  - rating == Match: yield immediately.
  - Otherwise, if rating < the current deferred rating (strict), replace the deferred candidate.
- Rating order: Match 0 < Deferred 1 < DeferredErrors 2 < PartialMatch 3 < MaxNotMatch 4.

TryGetEgg:
- The lowest chain stage must reach level 1.
- (base species, form) = Evolves5.GetBaseSpeciesForm.
- If base != the devolved species and the devolved species is not IsSplitBreedNotBabySpecies4, reject.
- Must pass CanHatchAsEgg (species and form).
- The species must be present in B2W2 personal data.
- The result is EncounterEgg5(species, form, version).

TryGetSplit:
- If the egg species == the devolved species, use chain[^2].
- That species must be IsSplitBreedNotBabySpecies4.
- The result is the egg with that species.

EncounterFinder then validates candidates as described in section 0. A candidate with rating >= PartialMatch is accepted only if nothing better validates; otherwise the result is EncInvalid.

QUIRKS TO DECIDE ON:
(a) Entree gender-mismatch early return true (EncounterStatic5Entree.cs:94).
(b) PGF Level setter offset bug (affects writing only).
(c) MoveBreed5 drops HM06 Dive instead of TM95.
(d) The roamer location bitmask.
(e) 30002 accepted as an egg location (spin trade).
(f) GetTransferMetLocation4 returns the "Unused" capsule locations 30010/30012.
(g) There is no 252 EV cap in format 5.
```

## PokeBank NX: deliberate differences and known gaps

Measured with `tools/pkhex_gen5_diff/run.sh` (every Gen V encounter PKHeX can
build, plus single-field mutations).

Deliberately stricter than PKHeX: nothing. run.sh fails on every record PKHeX
accepts and PokeBank NX reports Invalid; it has no allow-list.

Known gaps (reported as Incomplete, never Invalid):

- **Hidden-ability flag with a slot 1/2 ability on a hidden-ability-only
  template** (Entree Forest / Dream World, Hidden Grotto, Dream Radar, hidden
  ability statics and trades) that the Pokemon has evolved from. This used to
  be Invalid. PKHeX accepts such a record when it matches the current species'
  template (`VerifyFixedAbility` returns VALID on `AbilityNumber == 4` alone,
  `AbilityVerifier.cs:225`), and no original-game evidence shows Gen V cannot
  produce it: there is no Gen V decompilation, and the absence of an Ability
  Capsule before Gen VI is a secondary source. It is now a Warning; see
  [PKHEX_DISAGREEMENT_REVIEW.md](PKHEX_DISAGREEMENT_REVIEW.md). Bred Pokemon
  with the flag and a slot 1/2 ability stay Invalid, as in PKHeX
  (`VerifyAbility5` → `CheckMatch` → `GetPIDAbilityMatch`).

- OT and nickname text of in-game trades and event cards, and nicknames that
  PokeBank NX cannot decide. The PKHeX name rules are ported (see "Names,
  trades, events and transfers" below); only the cases listed there under
  "Still Incomplete" remain.
- Fateful records that match no supported PGF card (`EncGiftNotFound`): the
  card list is not proven complete, so the analyzer warns. "Supported PGF" is
  defined below.
- Gen III/IV-origin PK5 records. Every Gen V-side and conversion rule PKHeX
  applies to them is checked. The original Gen III/IV encounter, and the moves
  and PID/IV correlation it fixes, are left to the Gen III/IV analyzer, so
  these records are always Incomplete (transfer coverage Partial).

## PokeBank NX: names, trades, events and transfers

This section covers the name rules, PGF provenance and the Poke Transfer
checks. Each rule follows the PKHeX verifier named in its row.
`tools/pkhex_gen5_diff/run.sh` measures them against PKHeX.Core on records
PKHeX builds itself.

### Text model

`include/Legality/Gen5Text.h` / `src/Legality/Gen5Text.cpp` port the PKHeX
text helpers the name rules need:

- `StringConverter5.LoadString`;
- the `SpeciesName` lookups (9 languages, the Gen III/IV/V name forms, egg
  names);
- `TextNormalizer` and `WordFilter5` normalisation;
- `WordFilter.IsFiltered` with PKHeX's default settings (the Switch and 3DS
  regular expressions plus the Gen V dictionary);
- `TrainerNameVerifier.ContainsTooManyNumbers`.

The tables (trade names, species names, word lists, Gen III/IV fixed names)
are generated by `tools/gen_legality_gen5_data.py` from the pinned PKHeX
resources.

.NET invariant casing, `char.IsNumber` and `char.IsWhiteSpace` are modelled
only for a fixed set of UTF-16 units (`Gen5Text::inDomain`). run.sh checks
three things against the reference:

- every unit of that set against .NET (`casemap`);
- the stored filter patterns against PKHeX (`patterns`);
- filter verdicts on sampled strings against PKHeX (`wordfilter`).

A name with a unit outside the set is a Warning ("not modelled for", internal
coverage Partial), never Invalid.

### Names: Gen V origin

| Rule | PKHeX | PokeBank NX |
|---|---|---|
| In-game trade OT for the record's language | `NicknameVerifier.VerifyTrade` → `EncTradeChangedOT` | Invalid |
| In-game trade fixed nickname for the record's language | `VerifyNickname` → `EncTradeChangedNickname` | Invalid |
| Black/White trades from Japanese games store language 0 | `VerifyEncounterTrade5` → `OTLanguageShouldBe_0` | Invalid |
| N's Pokemon have OT "N" ("Ｎ" in Japanese) | `EncounterStatic5N.IsTrainerMatch` | Invalid |
| Nicknamed event Pokemon whose card has no nickname | `IsMysteryGiftNoNickname` → `EncGiftNicknamed` | Invalid |
| Name differs from the card's own nickname | not checked (PGF is `ILangNick`, not `ILangNicknamedTemplate`) | Warning |
| Not nicknamed: species name of its language. Any language is accepted after evolving, as a traded egg, or from a fixed-language card without a nickname | `IsNotNicknameValid` → `NickMatchLanguageFail` | Invalid |
| Nicknamed with a species name (any language) | Fishy only (`NickMatchNoOthersFail`) | accepted |
| Nickname with East Asian ideographs | `VerifyUnNicknamedEncounter` → `NickInvalidChar` | Invalid |
| Japanese / Korean nickname longer than 5 (a traded egg may keep 10) | `NickLengthLong`, `GetForeignNicknameLength` | Invalid |
| Egg name for the language | `VerifyNicknameEgg` → `NickMatchLanguageEggFail` | Invalid |
| Empty nickname; empty OT of a player-OT encounter | `NickLengthShort`, `OTShort` | Invalid |
| Player OT, Japanese / Korean, longer than 6 (traded-egg edge allowed) | `OTLong`, `IsEdgeCaseLength` | Invalid |
| Player OT, Japanese / Korean, exactly 6 (name entry stops at 5) | accepted | Warning |
| Player nickname and OT: word filter; more than 4 digits | `WordFilterFlaggedPattern_01`, `WordFilterTooManyNumbers_0` | Invalid |

### Supported PGF

"Supported PGF" means one of the 709 cards in PKHeX's Mystery Gift database at
the pinned commit (`Resources/legality/mgdb/pgf.pkl`, generated into
`Gen5Data::kPgf`). A Gen V-origin fateful record is matched against those
cards. A record that matches none gets the Warning "Fateful encounter does not
match any event card in the pinned Gen V database" (event coverage Partial),
never Invalid. PKHeX reports `EncGiftNotFound`, but a database PokeBank NX
cannot prove complete is no proof that the card never existed.

For a matched card, the following are checked:

- the card's fixed data (existing rules);
- the nickname rules above;
- the OT (fixed by the card, so `TrainerNameVerifier` does not apply);
- an unhatched event egg's moves, which must equal the card's
  (`LearnVerifierEgg.VerifyFromEncounter`);
- card ribbons and contest stats (existing rules).

PKHeX's `MiscFatefulEncounterVerifier` accepts a card's shiny PID only when
`MethodFinder.Analyze` returns `G5MGShiny`, and `MethodFinder` tries the
Gen III event searches (`CommonEvent3` BACD, BACD_EA, BACD_EAX, BACD_S) first.
Their 16-bit comparisons let a real event Pokemon hit one by chance (about
1 in 65536). When the PID and IVs fit one of them, PokeBank NX reports a
Warning (PID/RNG coverage Partial) instead of deciding.

### Poke Transfer / Pal Park origin (Gen III/IV)

The original encounter is not identified, so a rule is Invalid only when no
Gen III/IV encounter can explain the record. Otherwise it is a Warning. Every
Gen III/IV-origin record also carries the Warning "the original encounter and
moves are not checked by the Gen V analyzer".

| Rule | PKHeX | PokeBank NX |
|---|---|---|
| Hidden-ability flag | `AbilityVerifier.CheckMatch` (Gen III/IV) | Invalid; Warning on a fateful Gen IV Pokemon (below) |
| Contest sheen within the range the contest stats allow | `ContestStatVerifier`, `CalculateMinimumSheen3/4`, `CalculateMaximumSheen` | Invalid; Warning for a fateful Gen IV Pokemon or a trade OT (they may start with stats) |
| Colosseum / XD OTs are male | `CXDVerifier` | Invalid |
| Gen III: no egg location; the evolution line existed in Gen III; Gen III balls only | `PK3.ConvertToPK4`, `BallVerifier` | Invalid |
| Gen IV: no Dream Ball; Apricorn and Sport Balls only in HGSS | `BallUseLegality` | Invalid |
| Spiky-eared Pichu: HGSS only, never Gen III | encounter tables | Invalid |
| Korean is not a Gen III language | `LanguageVerifier`, `GetMaxLanguageID` | Invalid |
| Mothim keeps Burmy's cloak (forms 0-2) | `FormInfo.IsValidOutOfBoundsForm` | accepted |
| Gender of Wormadam, Mothim, Vespiquen, Gallade, Froslass against the pre-evolution's PID gender | `GenderVerifier.IsValidFixedGenderFromBiGender` | Warning (it may have been caught evolved) |
| Wurmple evolution branch | `PIDVerifier.VerifyECPIDWurmple` | Warning (the encounter may not be Wurmple) |
| Nickname: species name of its language in its Gen IV or Gen V form, Farfetch'd upper-cased, any language for a traded Gen IV egg | `IsNotNicknameValid`, `IsMatch45` | Invalid; Warning when an in-game trade nickname or an evolution in another language's game could explain it |
| Nickname: ideographs, Japanese / Korean length, word filter, digits | as for Gen V | Invalid unless the name is a Gen III/IV trade name |
| OT: Japanese / Korean length, word filter, digits | `TrainerNameVerifier` | Invalid for a non-fateful Gen IV Pokemon whose OT is not a trade name; otherwise Warning (Gen III gifts, Gen IV events and in-game trades fix the OT) |
| Ribbons. Hoenn ribbons need Gen III. Battle Tower and Frontier ribbons are not for banned species. Record Ribbon never given. Contest ranks in order. Royal ribbons in order. Spiky-eared Pichu's limits | `RibbonVerifier` with `RibbonVerifierCommon3/4`, `Unique3/4` | Invalid |
| Event ribbons. Gen III: National only on Colosseum / XD or the Metang gifts; Earth from Mt. Battle; others never. Gen IV: only a card gives them | `RibbonVerifierEvent3/4`, `EncounterGift3` (`IRibbonSetEvent3`), PCD | Invalid; Warning on a fateful Gen IV Pokemon (the card is not identified) |

The in-game trade name table (`kTransferTradeNames`, 369 names) holds every
Gen III/IV trade nickname and OT in every language, the XD trade nickname and
the My Pokemon Ranch OTs. `run.sh` checks (`fixednames`) that it holds every
fixed nickname PKHeX knows, and every fixed OT of the trades that start with
contest stats (`EncounterTrade3`, `EncounterTrade4PID`).

### Decisions

- **Hidden-ability flag on a fateful Gen IV Pokemon.** PKHeX rejects the flag
  for every Gen III/IV encounter (`CheckMatch`), except an unevolved Gen IV
  card. `AbilityVerifier.VerifyAbilityPCD` compares the ability only. A PK4
  has no such flag (0x42 is unused) and `PK4.ConvertToPK5` copies the byte, so
  a real transfer has it clear. PKHeX accepts the record all the same, and
  PokeBank NX cannot tell whether the Pokemon is an unevolved card. On a
  fateful Gen IV Pokemon this rule is therefore a Warning; it stays Invalid
  everywhere else. On the measured corpora below, the base commit 7356b383
  reported 37 PKHeX-valid records Invalid for this reason.
- **Wurmple branch.** The former rule was Invalid for every Silcoon /
  Beautifly / Cascoon / Dustox whose PID chose the other branch. PKHeX checks
  only when the encounter is Wurmple; the base commit had 65 false Invalids
  of this kind on the measured corpora. It is now Invalid only when the
  encounter is Wurmple and no other template matches; otherwise it is a
  Warning.
- **Mothim forms.** Burmy's cloak stays on Mothim (PKHeX
  `FormInfo.IsValidOutOfBoundsForm`); the base commit called forms 1-2
  Invalid (1 record on the measured corpora).
- **Text buffers** (existing rule, documented here). A Gen III-V game ends a
  name with 0xFFFF, so a nickname uses at most 10 of its 11 units and an OT 7
  of its 8. PKHeX reads to the end of the buffer and allows a 12-character
  Western OT. PokeBank NX is stricter. No record PKHeX builds reaches it, so
  run.sh never meets it.
- **Hatched bred Pokemon.** PKHeX checks only that each move has a source; it
  replays the inheritance order (`MoveBreed`) only for an unhatched egg.
  PokeBank NX does the same. Moves and breeding coverage are Complete for a
  hatched Pokemon and Partial (Warning) for an unhatched bred egg.

### Still Incomplete

- Gen III/IV-origin records: the original encounter (above).
- Fateful records that match no supported PGF card.
- Names with a UTF-16 unit outside `Gen5Text::inDomain`.
- Japanese / Korean player OTs of exactly 6 characters.
- A Gen V nickname that differs from the matched card's nickname.
- A PGF shiny PID that also fits a Gen III event pattern.
- Encounter-dependent rules where several Gen V templates match and PKHeX's
  choice is not modelled: gender from a two-gender pre-evolution, and the
  Wurmple branch.
- Gen III/IV-origin names, OTs, contest sheen and event ribbons that only an
  in-game trade or an event card can explain (see the table).

### Measured

`tools/pkhex_gen5_diff/run.sh /path/to/PKHeX SEED 4` for seeds 1, 2, 7 and 11
(fresh corpora, PKHeX at the pinned commit). Each run produced:

- 0 false Invalid and 0 false Valid;
- 0 mismatches in casing (47,748 units), in the word-filter lists
  (4,533 / 3,405 / 362 entries) and in about 47,600 word-filter verdicts per
  origin;
- 0 fixed Gen III/IV names missing from the trade-name table (225 nicknames,
  339 OTs).

The corpora hold 461,340 records in all; 227,135 of them are PKHeX-valid.
Each record is in one of these groups:

- native Gen V encounters;
- in-game trades, both as the trade table and with every language substituted;
- every PGF card;
- Gen III/IV encounters converted to PK5;
- grown copies of each of the above;
- four single-field mutations of every record.

"Before" is the base commit 7356b383. Its name rules were not implemented, so
it certified nothing.

Share of PKHeX-valid records certified "No problems found":

| Origin | Game | PKHeX-valid | Before | After |
|---|---|---:|---:|---:|
| native | B | 27,439 | 0 | 27,419 (99.9%) |
| native | W | 23,712 | 0 | 23,695 (99.9%) |
| native | B2 | 34,151 | 0 | 34,123 (99.9%) |
| native | W2 | 32,219 | 0 | 32,195 (99.9%) |
| native | all | 117,521 | 0 | 117,432 (99.9%) |
| trade | B | 689 | 0 | 689 (100%) |
| trade | W | 514 | 0 | 514 (100%) |
| trade | B2 | 4,624 | 0 | 4,624 (100%) |
| trade | W2 | 2,752 | 0 | 2,752 (100%) |
| trade | all | 8,579 | 0 | 8,579 (100%) |
| event | B | 5,200 | 0 | 5,197 (99.9%) |
| event | W | 3,715 | 0 | 3,711 (99.9%) |
| event | B2 | 3,636 | 0 | 3,634 (99.9%) |
| event | W2 | 3,523 | 0 | 3,519 (99.9%) |
| event | all | 16,074 | 0 | 16,061 (99.9%) |
| transfer | R / S / E / FR / LG / CXD / D / P / Pt / HG / SS | 84,961 | 0 | 0 (by design: original encounter Partial) |
| all | | 227,135 | 0 | 142,072 (62.5%) |

The PKHeX-valid native, trade and event records left Incomplete are:

- Japanese / Korean OTs of exactly 6 characters;
- nicknames that differ from the card's nickname;
- records where several templates match and the gender fits only another
  species' PID.

Share of PKHeX-invalid records reported Invalid:

| Origin | PKHeX-invalid | Before | After |
|---|---:|---:|---:|
| native | 101,534 | 97,407 (95.9%) | 101,520 (99.99%) |
| trade | 11,571 | 9,748 (84.2%) | 11,570 (99.99%) |
| event | 26,536 | 22,258 (83.9%) | 25,526 (96.2%) |
| transfer | 94,564 | 63,435 (67.1%) | 80,118 (84.7%) |
| all | 234,205 | 192,848 (82.3%) | 218,734 (93.4%) |

Transfer records PKHeX rejects and PokeBank NX leaves Incomplete depend on the
original Gen III/IV encounter. By PKHeX result, the most frequent are:

| PKHeX result | Records |
|---|---:|
| `FatefulInvalid` | 4,406 |
| `PIDTypeMismatch` | 2,458 |
| `TrainerIDNoSeed` | 2,152 |
| `Unobtainable` (moves) | 1,856 |
| `EncInvalid` | 1,729 |
| `EncTypeMismatch` | 1,199 |
| `BallEncMismatch` | 1,182 |

Fixed-name encounters account for the word-filter, `OTLong` and
`NickMatchLanguageFail` cases, which therefore stay Warnings.
