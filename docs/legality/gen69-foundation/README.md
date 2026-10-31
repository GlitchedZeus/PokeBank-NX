# Gen VI–IX legality foundation (Lane B)

Branch `work/claude-gen69-legality-foundation-20261030`. Started 2026-10-31 under the owner's
Oct 30 mandate that lifted the Gen VI–IX freeze. Reference: PKHeX 26.10.18 at
`bfe89c74a3f5265805e364b79bc04ad38ebbc5ea`.

## Base and reuse

| Input | Head verified 2026-10-31 | Use here |
|---|---|---|
| #314 Gen I–V legality (Codex) | `eabe41f31903d2e724e25d04d18b36c26a0e340a` | **Base of this branch.** It owns the shared `Legality::Report` / `Verdict` / `CoverageSummary` model and the legality CI. Nothing in #314's files is changed except one `include` line in `Makefile.host`. |
| #309 Gen VI native | `5e6836902555da4431be16348ef21abec0358d13` | Ancestor of #311 and #312. |
| #311 Gen VII native | `2438b5d0b9294193a9de9c8f14736c59ebbb90a5` | Ancestor of #312. |
| #312 Gen VIII native | `2f065ee2b0768d01bf621f969aef01120c6231e3` | Contains #309 and #311. Five headers copied **unchanged** from this commit (below). |
| #321 Gen VI–IX research | `9b46c779f54356eb0ee4d0ec3b8c8a539422bdb6` | Read for the plan (MASTER_HANDOFF, CLAUDE_IMPLEMENTATION_ORDER, LEGALITY_RESEARCH, TEST_RESULTS). |
| #301 experimental foundation | `809a31841fc2ad43dda02d4412cb9891f29a482f` | Not merged. Its detached PK6/PK7 readers duplicate the #312 codec; the #312 codec is used instead. |

#309/#311/#312 are stacked on an old application history (merge base with #314 is
`220f18f7`, about 1,100 commits behind it). The research handoff says not to merge that
stack to obtain its files, so only the format files the analyzer needs were taken, byte for
byte (git blob ids):

| File (from #312 `2f065ee2`) | Blob |
|---|---|
| `include/Integration/Gen6/Gen6PK6.h` (PK6/PK7 crypto, checksum, field accessors) | `726c8ff22b342dd5595df194f0ff47cecc25e1a3` |
| `include/Integration/Gen6/Gen6PersonalData.h` (PersonalTable.AO base stats) | `e9f57b01e405235758f16ca0ab5e0a101ab62b7b` |
| `include/Integration/Gen7/Gen7PersonalData.h` (PersonalTable.USUM base stats) | `ebf2dc7bfc246adacce2fa5fe15a69923be08cef` |
| `include/Integration/Gen8/Gen8PK8.h` (PK8 codec) | `33ecd733bfec5e9c84281e7ea45772267a8fd038` |

No save parser, staged session, exporter or SAV7 signing code is used or changed. Save
parsing and serialization stay with the Main Development project.

## Interface

`include/Legality/ModernLegality.h`, `Legality::Modern::analyze(record, Input)`:

* `Format` (PK6, PK7, PB7, PK8, PB8, PA8, PK9, PA9) with PKHeX's stored/party sizes,
* exact `Game` (PKHeX GameVersion id), `Slot` (party/box/standalone/unknown),
* original or staged bytes, `encrypted` or decrypted,
* `Integrity` of the caller's container (feeds source-game coverage only),
* optional save `Trainer`.

It returns the same `Legality::Report` as `analyze()` (Gen I–IV) and `analyzePk5` (Gen V):
Invalid outranks everything, "No problems found" needs Complete coverage in every area, and
everything else is Incomplete. No modern area claims Complete coverage, so a record without
a contradiction is **Incomplete**, never "No problems found". The analyzer only reads the
bytes: no Pokémon object, no mutation, no save write, deterministic.

Every format (PK6, PK7, PB7, PK8, PB8, PA8, PK9 and PA9) has rules today.

## PK6 / PK7 rules

Every Invalid mirrors a PKHeX.Core verifier at the pinned commit and depends only on what the
record itself fixes (format, origin game, species/form, slot). Rule data is generated from
PKHeX (`tools/pkhex_gen67_diff/oracle dump` → `include/Legality/Gen67LegalityData.inc`) and
CI fails if it drifts.

| Area | Rule (Invalid) | PKHeX source |
|---|---|---|
| Structure | checksum mismatch; sanity flag set | `G6PKM.ChecksumValid`, `PKM.Valid` |
| Species | species above the format maximum (721 / 807) | `Legal.MaxSpeciesID_6/_7_USUM` |
| Origin | origin version no game writes; newer than the format; Virtual Console in PK6 | `EncounterFinder` (no encounter) |
| Form | out of range (Unown/Mothim/Scatterbug/Spewpa exceptions); Cosplay Pikachu outside Gen VI; Unown Gen II/III letter; Giratina/Arceus/Genesect/Silvally form vs item; Keldeo vs Secret Sword; Furfrou/Shaymin/Hoopa in a box (Gen VI); Vivillon pattern vs console region; event-only Scatterbug/Spewpa patterns; Meowstic form vs gender; Own Tempo Rockruff / Dusk Lycanroc from Sun/Moon; Rotom/Furfrou egg forms; battle-only forms | `FormVerifier`, `FormInfo`, `FormItem`, `Vivillon3DS` |
| Level | EXP above the level-100 total; level below met level | `LevelVerifier` |
| EVs | egg with EVs; total above 510; any EV above 252 | `EffortValueVerifier` |
| Moves | move above the format maximum; empty first slot; empty slot before a move; duplicates; Kyurem fusion-move interlocks | `LearnVerifier.Finalize`, `FlagFormExclusiveKyurem` |
| PP | PP Ups on an empty slot or Sketch; PP above the PP-Up maximum (except boxed Virtual Console records, left undecided); egg PP Ups; egg PP not base PP | `MovePPVerifier` |
| EC/PID | Gen III–V origin: PID must be the Poké Transporter PID for its EC; other origins: PID ≠ EC | `PIDVerifier.VerifyEC`, `PK5.GetTransferPID` |
| Gender | genderless mismatch; Gen III–V origin: PID gender (Azurill/Marill exception); fixed-gender species | `GenderVerifier`, `PKM.IsGenderValid` |
| Nature | out of range; Gen III/IV origin: EC % 25 | `PIDVerifier`, `GenderVerifier.VerifyNaturePID` |
| Ability | ability not one of the species'; ability number not 1/2/4; ability number points elsewhere | `AbilityVerifier` (format ≥ 6 part) |
| Ball | ball 0 or above the format's last ball | `BallVerifier` (no encounter) |
| Language | 0 or 6; above the origin generation's maximum; Korean VC outside Gold/Silver; Blue VC not Japanese | `LanguageVerifier`, `Legal.GetMaxLanguageID` |
| Held item | egg holding an item; item not released for the generation | `ItemVerifier`, `ItemRestrictions` |
| 3DS region | console region ≥ 7; country outside the console region | `ConsoleRegionVerifier`, `Locale3DS` |
| Transfer | Gen III/IV origin met location must be Poké Transporter's (Celebi / Crown beast exceptions) | `TransferVerifier.VerifyTransferLegalityG3/G4` |
| Virtual Console | met location not Poké Transporter's (30013 Red/Green/Blue/Yellow, 30017 Gold/Silver/Crystal); female OT outside Crystal; met level 100 nature = EXP % 25; met level ≤ 2 nature set; VC language vs console region | `TransferVerifier.VerifyVCEncounter`, `VerifyTransferLegalityG12` |
| Hyper Training (PK7) | below level 100; six perfect IVs; a trained perfect IV | `HyperTrainingVerifier` |

Warnings (not PKHeX rules, never Invalid): a PK7 in a Gen VI game or the reverse; species,
move or item above the exact game's data (X/Y below OR/AS, Sun/Moon below Ultra Sun/Ultra
Moon); party level or stats that do not match the level/IV/EV/nature/Hyper Training; same
trainer IDs as the save with another OT name or gender; a container that failed its own
integrity check.

Not implemented (keeps the report Incomplete): encounter tables, learnsets and move
chronology, relearn moves, eggs beyond the structural checks, events/Wonder Cards, balls per
encounter, IV/flawless counts, shiny locks, ribbons, memories, geolocation history, trash
bytes and nicknamed records/word filter, Bank/Transporter history beyond the PID and met-location
rules, fateful flags and Super Training.

## PB7 (Let's Go Pikachu/Eevee) rules

PB7 is always 0x104 bytes with PK6/PK7's encryption, checksum and most offsets, so it is decoded
by the existing `Gen6PK6.h` and analyzed by `src/Legality/Gen7bLegality.cpp`. Tables come from
`PersonalTable.GG` (abilities, gender, growth, forms, base stats, height, weight and
`IsPresentInGame`), `EntityContext.Gen7b` PP and held items and format-7b battle forms, dumped
by `oracle dump7b` into `Gen7bLegalityData.inc` (species ≤ 809, moves ≤ 742, balls ≤ 26). CP,
height and weight are recomputed in single precision without fused multiply-add, as PKHeX does.

| Area | Rule (Invalid) | PKHeX source |
|---|---|---|
| Origin | a version other than Let's Go Pikachu/Eevee, or GO without the GO Park met location (50) | `EncounterGenerator7GG`, `EncounterGeneratorGO` |
| Species | above 809; a species/form not present in Let's Go; a form outside the species' forms; a battle-only form | `PersonalTable.GG.IsPresentInGame`, `FormVerifier` |
| Egg | the egg flag (Let's Go has no eggs) | `EggVerifier` |
| Level | experience above the level-100 total; level below met level | `LevelVerifier` |
| EVs and AVs | any EV (Let's Go has none); an Awakening Value above 200 | `EffortValueVerifier`, `AwakenedValueVerifier` |
| Moves and PP | as PK7 (unknown move, empty first slot, gaps, duplicates, PP Ups on an empty slot, PP above its maximum) | `MovePPVerifier`, `MoveVerifier` |
| Identity | PID = EC; gender against the ratio; ability against the slots and number; nature; language | `PIDVerifier`, `GenderVerifier`, `AbilityVerifier` |
| Ball, item | ball 0 or above 26; a held item Let's Go cannot hold | `BallVerifier`, `ItemVerifier` |
| Hyper Training | below level 100; with six perfect IVs; on a perfect IV | `HyperTrainingVerifier` |
| CP | stored CP is not `PB7.CalcCP` (the partner Pikachu/Eevee excepted) | `MiscVerifierPB7.Verify` |
| Spirit / mood | not 100 for a record at rest in a box (the partner excepted; party and unknown slots stay undecided) | `MiscVerifierPB7.VerifyMoodSpirit` |
| Received date | a time outside the day, a date that does not exist, or, on an untraded non-GO record, a date other than the met date | `MiscVerifierPB7.VerifyReceivedDateTime` |
| Sizes | stored height or weight is not what its scalars give (Meltan and Melmetal excepted: fixed WB7 sizes need the gift) | `MiscVerifierPB7.VerifyCalculatedSizes` |

Not implemented for PB7: encounter tables (wild, static, trades, GO Park), learnsets, WB7 gifts,
ball per encounter, fateful flags, nicknamed records, trash bytes and memories.

## PA8 (Legends: Arceus) rules

PA8 keeps the Gen VIII encryption (PK8's LCG and block order) with 0x58-byte blocks: 0x168
bytes stored, 0x178 in the party. `src/Legality/Gen8aLegality.cpp` has its own codec (reusing
`Gen8PK8.h`'s crypt and block tables unchanged) and analyzer. Tables come from
`PersonalTable.LA` (abilities, gender, growth, forms, presence, height, weight and move shop
permissions), the move shop list, `EntityContext.Gen8a` PP and held items and format-8
battle forms, dumped by `oracle dump8a` into `Gen8aLegalityData.inc` (species ≤ 905, moves ≤
850, items ≤ 1828, balls ≤ 37).

| Area | Rule (Invalid) | PKHeX source |
|---|---|---|
| Species | above 905; a species/form Legends: Arceus does not hold; a form outside the species' forms; a battle-only form (no form-item rule: Dialga, Palkia, Giratina and Arceus change form with key items) | `TransferVerifier`, `FormVerifier` |
| Level, EVs | experience above the level-100 total; level below met level; an EV above 252 or a total above 510 | `LevelVerifier`, `EffortValueVerifier` |
| Moves and PP | unknown move, empty first slot, gaps, duplicates; any PP Up (Legends: Arceus has none); PP above its maximum | `Legal.IsPPUpAvailable`, `MovePPVerifier` |
| Identity | gender against the ratio; ability against the slots and number; nature; stat nature not the nature or a mint; ball 0 or above 37; language; held item | `GenderVerifier`, `AbilityVerifier`, `MiscVerifierHelpers.VerifyStatAlignment`, `BallVerifier`, `ItemVerifier` |
| Hyper Training | six perfect IVs; a perfect IV trained (the minimum level depends on the games visited and is not decided) | `HyperTrainingVerifier` |
| Legends: Arceus | noble flag; displayed scale not the height scalar; an Effort Level above its IV's cap | `LegendsArceusVerifier` |
| Move shop | an alpha move on a non-alpha, not masterable by the species, or not mastered; a purchased move the species cannot buy | `LegendsArceusVerifier.CheckMastery` |
| Misc | sociability, Gigantamax, Dynamax level, TR flags, fullness or enjoyment set; battle version as Sword/Shield | `MiscVerifierPA8`, `FullnessRules` |
| Sizes | a Legends: Arceus record with no HOME tracker, not an alpha and not Enamorus whose stored height or weight is not the in-game calculation | `MiscVerifierPA8.VerifyAbsoluteSizes` |

Not decided for PA8: encounters (alpha flag per encounter, alpha scalars, fixed gifts),
learnsets and mastery by level, PID = EC (fixed-EC gifts), HOME-calculated sizes (fused
multiply-add) and pre-evolution sizes, Hyper Training minimum level, ball per encounter,
nicknamed records, ribbons and memories.

## PA9 (Legends: Z-A) rules

PA9 is a 0x148/0x158-byte record with PK8's encryption (decoded with the unchanged
`Gen8PK8.h`) and PK9's internal species ids, with its own layout; `src/Legality/Gen9aLegality.cpp`
analyzes it. Tables come from `PersonalTable.ZA` (abilities, gender, growth, forms, presence),
`EntityContext.Gen9a` PP and held items and format-9 battle forms, dumped by `oracle dump9a`
into `Gen9aLegalityData.inc` (species ≤ 1000, moves ≤ 920, items ≤ 2684, balls ≤ 37).

| Area | Rule (Invalid) | PKHeX source |
|---|---|---|
| Species | above 1000; a species/form Legends: Z-A does not hold; a form outside the species' forms; a battle-only form | `TransferVerifier`, `FormVerifier` |
| Level, EVs | experience above the level-100 total; level below met level; an EV above 252 or a total above 510 | `LevelVerifier`, `EffortValueVerifier` |
| Moves and PP | an unknown or duplicated move; any PP Up; PP above its maximum, or neither full nor 0 (Z-A never uses PP; HOME writes 0) | `Legal.IsPPUpAvailable`, `Legal.IsPPUnused`, `MovePPVerifier` |
| Identity | gender against the ratio; ability number not 1, 2 or 4 (the ability itself is the encounter species' birth ability); nature; stat nature not the nature or a mint; ball 0 or above 37; language 0, 6, above 11, or Spanish (LATAM) on a non-Z-A origin; held item | `GenderVerifier`, `AbilityVerifier.VerifyBirthAbility`, `MiscVerifierHelpers.VerifyStatAlignment`, `BallVerifier`, `LanguageVerifier`, `ItemVerifier` |
| Hyper Training | six perfect IVs; a perfect IV trained | `HyperTrainingVerifier` |
| Legends: Z-A | level boost at rest; a Plus flag past the 340 used; a Plus flag for a move that no form of the species or its pre-evolutions learns by TM, level up or Plus learnset | `LegendsZAVerifier`, `LegendsZAVerifier.GetInvalidPlusMove`, `PA9.GetMovePlusFlagAnyImpossible` |
| Misc | battle version as Sword/Shield; obedience level (untraded: equals met level; traded: between met and current level); an alpha with a HOME tracker whose scalars are not 255 | `MiscVerifierPA9` |

Not decided for PA9: empty or rearranged move slots (moves can be removed through HOME and the
move menu; PKHeX checks them against the encounter's minimum moveset), encounters, the birth
ability, Plus moves per level (a flag the line can learn is not decided), alpha per encounter, PID = EC, Hyper
Training minimum level, ball per encounter, nicknamed records, ribbons and memories.

## PK8 (Sword/Shield) rules

`include/Legality/Gen8Legality.h`, `src/Legality/Gen8Legality.cpp`. The codec is the #312
`Gen8PK8.h` (unchanged). Rule data comes from `oracle dump8` →
`include/Legality/Gen8LegalityData.inc` (PersonalTable.SWSH abilities, gender ratios, form
counts, growth rates, base stats, presence in the game and TR permissions; Gen VIII base PP;
held items; battle-only forms; form items; mint natures), and CI fails if it drifts.

A PK8 can hold a record from almost every game. Its origin follows PKHeX `PKM.Generation`
(version byte, plus the met location for Pokémon GO), and a HOME side transfer is recognised
by its `LocationsHOME` met location (59994–60000: Legends: Arceus, BD/SP, Scarlet/Violet,
Switch FireRed/LeafGreen), where the version byte reads Sword or Shield.

| Area | Rule (Invalid) | PKHeX source |
|---|---|---|
| Structure | checksum mismatch; sanity flag set | `G8PKM`, `PKM.Valid` |
| Species | species above 898; species/form not present in Sword/Shield | `Legal.MaxSpeciesID_8`, `TransferVerifier.VerifyTransferLegalityG8` (`IsPresentInGame`) |
| Origin | a version no game writes into a PK8 (0, 6, 46, 50/51 without a HOME location, 53+, GO without a GO location); Legends: Z-A | `PKM.Generation` = 0 (no encounter), newer than the format |
| Form | out of range (Unown/Mothim/Scatterbug/Spewpa exceptions); Cosplay Pikachu outside Gen VI; Unown Gen II/III letter; Giratina/Arceus/Genesect/Silvally form vs item; Keldeo vs Secret Sword (forced from Gen VIII on); event-only Scatterbug/Spewpa patterns; Meowstic form vs gender; Own Tempo Rockruff / Dusk Lycanroc from Sun/Moon; Rotom/Furfrou egg forms; battle-only forms | `FormVerifier` (format 8 branches), `FormInfo`, `FormItem` |
| Level, EVs | as PK6/PK7 | `LevelVerifier`, `EffortValueVerifier` |
| Moves | move above 826; empty first slot; gap; duplicates; Kyurem fusion moves; Hoopa form vs Hyperspace Hole/Fury | `LearnVerifier.Finalize`, `FlagFormExclusiveKyurem/Hoopa` |
| PP | PP Ups on an empty slot, Sketch or Revival Blessing; PP above the PP-Up maximum; egg PP Ups; egg PP not base PP | `MovePPVerifier`, `Legal.IsPPUpAvailable` |
| EC/PID | Gen III–V origin: Poké Transporter PID; other origins: PID ≠ EC (Switch Virtual Console FR/LG and Legends: Arceus / BD/SP / HOME side transfers left undecided: HOME PID and fixed-EC gifts) | `PIDVerifier.VerifyEC` |
| Gender, ability, nature | as PK6/PK7, on PersonalTable.SWSH | `GenderVerifier`, `AbilityVerifier`, `PIDVerifier` |
| Stat nature | the stat nature is neither the nature nor a mint | `MiscVerifierHelpers.VerifyStatAlignment`, `Nature.IsMint` |
| Ball | ball 0 or above Beast Ball (Legends: Arceus balls become a Poké Ball in Sword/Shield) | `BallVerifier`, `Legal.MaxBallID_8` |
| Language | as PK6/PK7, Gen VII–IX origins up to Chinese (Traditional) | `LanguageVerifier`, `Legal.GetMaxLanguageID` |
| Held item | egg holding an item; item not released in Sword/Shield | `ItemVerifier`, `ItemRestrictions` |
| Transfer | Gen III/IV origin met location must be Poké Transporter's (Celebi / Crown beast exceptions) | `TransferVerifier.VerifyTransferLegalityG3/G4` |
| Virtual Console | met location 30013/30017; female OT outside Crystal; met level 100 nature = EXP % 25; met level ≤ 2 nature set | `TransferVerifier.VerifyVCEncounter`, `VerifyTransferLegalityG12` |
| Hyper Training | below level 50; six perfect IVs; a trained perfect IV | `HyperTrainingVerifier` |
| Sword/Shield fields | Dynamax level above 10, on an egg, or on Zacian/Zamazenta/Eternatus; battle version other than 0/Sword/Shield, or Sword/Shield on a record that originated in or side-transferred to Sword/Shield; sociability above 255 or on an egg; a TR flag the species/form cannot learn (Calyrex exception); a Sword/Shield egg without the nickname flag | `MiscVerifierPK8` (`VerifyDynamax`, `IsBattleVersionValid`, `VerifySociability`, `VerifyTechRecordFlags`), `NicknameVerifier` (`EggStateLegality.IsNicknameFlagSet`) |

Warnings (never Invalid): a PK8 in a game that does not store PK8; party level or stats that
do not match (stats use the stat nature); same trainer IDs as the save with another OT; a
container that failed its own integrity check. Hyper Training between level 50 and 99 is an
Info line: it is valid only after a Scarlet/Violet or Z-A visit, which needs the evolution
history.

Not implemented for PK8 (keeps the report Incomplete): encounter tables (wild, raids, dens,
gifts, eggs), learnsets and relearn moves, Gigantamax per encounter, HOME tracker and size
re-roll history, egg locations, fateful flags, balls per encounter, IV/shiny/PID correlations
per encounter, ribbons and marks, memories, trash bytes, nicknamed records/word filter, Pokémon GO
and Let's Go transfer details.

## PB8 (Brilliant Diamond/Shining Pearl) rules

PB8 shares the G8PKM layout and crypto with PK8, so the same analyzer runs with
`Gen8::Context::BDSP`: BD/SP tables (`PersonalTable.BDSP`, `EntityContext.Gen8b` base PP,
held items, battle-only forms and Arceus plates; species ≤ 493, moves ≤ 826, items ≤ 1822,
balls ≤ 37), generated by the same `oracle dump8` into `Gen8LegalityData.inc`. Every PK8 rule
above that is not Sword/Shield-specific applies, with these differences:

| Area | Rule (Invalid) | PKHeX source |
|---|---|---|
| Origin | a version no game writes into a PB8; Scarlet/Violet and Legends: Z-A are allowed (HOME) | `PKM.Generation` |
| Hyper Training | below level 100 | `HyperTrainingVerifier` (`Legal.GetMinLevelHyperTraining`, BD/SP) |
| BD/SP fields | sociability set; the Dpr-illegal flag set; Spinda or Nincada not from BD/SP; a Spinda with a HOME tracker; Gigantamax flag; Dynamax level; any TR byte on a non-egg; battle version rule as PK8 | `MiscVerifierPB8`, `IBattleVersion` |
| PID = EC | not decided on a BD/SP native (fixed-EC `WB8` gifts) or a Sword/Shield record with EC 0 (`WC8` HOME gifts) | `PIDVerifier.VerifyEC` |

TR learnability (Sword/Shield TRs) is not checked for PB8. Not implemented for PB8: encounter
tables (wild, Grand Underground, gifts, eggs, roamers), learnsets and TM flags, ball per
encounter, fateful flags, egg locations, ribbons, memories, trash bytes and nicknamed records.

## PK9 (Scarlet/Violet) rules

PK9 is a 0x148/0x158-byte record with PK8's encryption and most of its offsets, so the same
analyzer runs with `Gen8::Context::ScarletViolet`. It reads the fields PK9 moved: the internal
species id (`SpeciesConverter.GetNational9`, dumped as `kSpecies9National`), gender in bits 1–2
of 0x22, version 0xCE, battle version 0xCF, language 0xD5, Tera types 0x94/0x95, obedience level
0x11F, tracker 0x127 and TM flags (0x12F, DLC 0x4B). Tables come from `PersonalTable.SV`,
`EntityContext.Gen9` PP and held items, format-9 battle forms, Arceus plates and Ogerpon masks
(species ≤ 1025, moves ≤ 919, items ≤ 2557, balls ≤ 37), all from `oracle dump8`. Every
non-Sword/Shield-specific PK8 rule applies, with these differences:

| Area | Rule (Invalid) | PKHeX source |
|---|---|---|
| Origin | a version no game writes into a PK9; Scarlet/Violet, Z-A and an unhatched egg's version 0 are allowed | `PKM.Generation`, `PK9.IsUnhatchedEgg` |
| Form | Giratina-Origin ⇔ Griseous Core, Dialga-Origin ⇔ Adamant Crystal, Palkia-Origin ⇔ Lustrous Globe; Ogerpon's form ⇔ its mask; the Vivillon-pattern rule only for non-Scarlet/Violet origins | `FormVerifier` (format ≥ 9), `FormItem.GetFormOgerpon` |
| Hyper Training | below level 50 (no Info line: Scarlet/Violet trains at 50) | `HyperTrainingVerifier` |
| Tera type | an egg with an override; Terapagos with an override; Ogerpon's override not its mask type; an original type that does not exist; a record from another game without an override | `MiscVerifierPK9.VerifyTeraType`, `VerifyEncounter` |
| Obedience | untraded (no handler, Gen IX origin): obedience ≠ met level; traded: below met level or above current level | `MiscVerifierHelpers.IsObedienceLevelValid`, `PK9.IsUntraded` |
| TM flags | a flag no form of the species or of any pre-evolution can learn | `MiscVerifierPK9.VerifyTechRecordFlags` (superset of its evolution-chain check) |
| Scale | a record with a HOME tracker whose height scalar is not its scale (not for Legends: Arceus or Z-A origins) | `MiscScaleVerifier.VerifyScale` |
| Egg | a Scarlet/Violet egg without the nickname flag (Jacq's egg, location 60005, excepted) | `NicknameVerifier.VerifyNicknameEgg` |
| Battle version | as PK8 | `IBattleVersion` |

Not implemented for PK9: encounter tables (wild, Tera raids, outbreaks, gifts, eggs), learnsets,
Tera type per encounter or evolution chain, ball per encounter, fateful flags, egg locations,
ribbons and marks, memories, trash bytes, nicknamed records, and HOME history (pre-DLC learnset limits,
height/weight 0/0 after a Legends: Arceus visit).

## Encounter ball rule (every format)

`src/Legality/ModernBall.cpp`, called by all five analyzers after the ball range check, makes
`BallVerifier.VerifyBall` encounter-independent. For each format and national species the
oracle (`dumpballs`) runs `EncounterMovesetGenerator` over every game that can reach the format
and every form of the species, and unions each encounter's balls: `IFixedBall`, egg
inheritance (`BallContext6/7/HOME`, Poké Ball before Generation 6, never Master or Cherish),
Dream World balls, GO restrictions (plus Master and Strange), and the game's wild balls. Every
mystery gift of the species' line that can reach the format adds its ball, as do Shedinja's
Poké/Sport Ball and Sword/Shield's Poké Ball replacement for Legends: Arceus records. A Switch
format (PK8, PB8, PA8, PK9, PA9) checks the union over all Switch formats and Let's Go, because
HOME also moves records back to older games. A ball outside the union is Invalid; a species with
no generated encounter in the format is not decided. The masks are
`include/Legality/ModernBallData.inc`, checked byte for byte by every `run.sh` mode. Which ball a
given encounter allows (and Heavy Ball, ability-dependent inheritance, curry) still needs
encounter matching.

## Nickname rule (every format)

`src/Legality/ModernNickname.cpp`, called by all five analyzers, mirrors two parts of
`NicknameVerifier`:

* **NickLengthShort**: an empty nickname is Invalid whatever the flags.
* **VerifyUnNicknamed**: a record that is not an egg and whose nickname flag is clear must carry
  its species' name (`SpeciesName.GetSpeciesNameGeneration`). PB7, PK8, PB8, PA8, PK9 and PA9
  need the name in the record's own language; HOME's ASCII apostrophe is accepted for
  Farfetch'd and Sirfetch'd. PK6/PK7 need the record's language only for a species with no
  pre-evolution in any game that was never an egg; otherwise (an evolved record or a traded
  egg may keep another language's name) the name must match one of the game's languages.

The rule never decides a species whose evolution line includes a fixed-nickname in-game trade
or gift (`IFixedNickname`) or a fixed-language, un-nicknamed mystery gift (`ILangNick`), since
`NicknameVerifier` skips or relaxes the check for those encounters. It also leaves a PK7 name
with Chinese private glyphs and an invalid language to the other rules. The species names,
the Generation 7 Chinese renames, the language lists, those relaxed species and the
no-pre-evolution set are `include/Legality/ModernNicknameData.inc` (oracle `dumpnames`), which
every `run.sh` mode checks byte for byte. Nicknamed records (word filter, length, another
species' name) are not decided yet.

## Differential evidence

`tools/pkhex_gen67_diff/run.sh PKHEX [SEED] [MUTATIONS]` builds the oracle against PKHeX,
checks the generated tables byte for byte, generates a corpus and runs `gen67_diff.cpp`.
It fails on any false Invalid (PKHeX valid, NX Invalid) or false Valid (PKHeX invalid, NX
"No problems found"). Results on 2026-10-31 (local, .NET 10.0.114, g++ 13.3):

| Format | Seed / mutations | Records | PKHeX valid | PKHeX invalid | NX Invalid of PKHeX-invalid | False Invalid | False Valid |
|---|---|---:|---:|---:|---:|---:|---:|
| PK6/PK7 | 1 / 4 | 88,012 | 42,094 | 45,918 | 37,976 (82.7%) | 0 | 0 |
| PK6/PK7 | 2 / 8 | 155,118 | 63,201 | 91,917 | 76,342 (83.1%) | 0 | 0 |
| PK6/PK7 | 3 / 8 | 156,298 | 63,420 | 92,878 | 77,195 (83.1%) | 0 | 0 |
| PK6/PK7 | 4 / 4 | 89,523 | 42,686 | 46,837 | 38,948 (83.2%) | 0 | 0 |
| PB7 | 1 / 8 | 5,324 | 1,447 | 3,877 | 3,608 (93.1%) | 0 | 0 |
| PB7 | 2 / 4 | 3,050 | 1,102 | 1,948 | 1,823 (93.6%) | 0 | 0 |
| PB7 | 3 / 16 | 9,859 | 2,049 | 7,810 | 7,279 (93.2%) | 0 | 0 |
| PB7 | 4 / 8 | 5,287 | 1,440 | 3,847 | 3,598 (93.5%) | 0 | 0 |
| PA8 | 1 / 4 | 16,161 | 6,530 | 9,631 | 6,824 (70.9%) | 0 | 0 |
| PA8 | 2 / 4 | 15,521 | 6,361 | 9,160 | 6,491 (70.9%) | 0 | 0 |
| PA8 | 3 / 8 | 29,114 | 9,546 | 19,568 | 13,945 (71.3%) | 0 | 0 |
| PA8 | 4 / 4 | 15,706 | 6,505 | 9,201 | 6,610 (71.8%) | 0 | 0 |
| PA9 | 1 / 4 | 20,953 | 9,460 | 11,493 | 8,069 (70.2%) | 0 | 0 |
| PA9 | 2 / 4 | 21,203 | 9,446 | 11,757 | 8,208 (69.8%) | 0 | 0 |
| PA9 | 3 / 8 | 38,040 | 13,798 | 24,242 | 16,773 (69.2%) | 0 | 0 |
| PA9 | 4 / 4 | 21,136 | 9,481 | 11,655 | 8,264 (70.9%) | 0 | 0 |
| PK8 | 1 / 4 | 46,387 | 19,987 | 26,400 | 20,743 (78.6%) | 0 | 0 |
| PK8 | 2 / 4 | 45,298 | 19,779 | 25,519 | 19,971 (78.3%) | 0 | 0 |
| PK8 | 3 / 8 | 81,530 | 29,728 | 51,802 | 40,577 (78.3%) | 0 | 0 |
| PK8 | 4 / 4 | 45,489 | 19,817 | 25,672 | 20,077 (78.2%) | 0 | 0 |
| PB8 | 1 / 4 | 44,334 | 18,431 | 25,903 | 22,057 (85.2%) | 0 | 0 |
| PB8 | 2 / 4 | 44,643 | 18,787 | 25,856 | 22,023 (85.2%) | 0 | 0 |
| PB8 | 3 / 8 | 77,736 | 26,371 | 51,365 | 43,433 (84.6%) | 0 | 0 |
| PB8 | 4 / 4 | 45,193 | 18,952 | 26,241 | 22,188 (84.6%) | 0 | 0 |
| PK9 | 1 / 4 | 39,685 | 18,653 | 21,032 | 18,061 (85.9%) | 0 | 0 |
| PK9 | 2 / 4 | 39,929 | 18,565 | 21,364 | 18,374 (86.0%) | 0 | 0 |
| PK9 | 3 / 8 | 69,293 | 27,200 | 42,093 | 36,444 (86.6%) | 0 | 0 |
| PK9 | 4 / 4 | 39,077 | 18,337 | 20,740 | 17,821 (85.9%) | 0 | 0 |

PB7 runs use `run.sh PKHEX SEED MUTATIONS 7b`, PA8 runs `8a`, PA9 runs `9a`, PK8 runs `8`, PB8 runs `8b` and PK9 runs `9`.
The PB7 corpus is smaller because Let's Go has far fewer encounters; it uses more mutations per record.
The PK8 counts dropped by a few records from the previous milestone because a Sword/Shield
record with EC 0 is no longer Invalid for PID = EC: it may be a HOME gift (`WC8.IsHOMEGift`),
whose EC PKHeX does not check, and the encounter is not resolved here. A few PKHeX verdicts depend on the run date
(HOME and distribution date windows), so the PKHeX valid/invalid split can move by a few
records between days; the false-Invalid and false-Valid counts must stay zero.

The corpus is synthetic (PKHeX builds every record from its own templates for a test
trainer); it is not a personal save and not retail evidence. The detection rate measures how
much of PKHeX's rejection these encounter-independent rules reproduce. It is not a measure of
rule completeness. The remaining ~20% need encounter, learnset, event or history data
(largest PK6/PK7 groups: fateful flags, geography consistency, egg locations, ball per
encounter, learnset-only moves, nickname language for relaxed species; largest PK8 groups: no matching
encounter, fateful flags, Gigantamax per encounter, nickname language for relaxed species, PID type per
encounter, Hyper Training level 50–99, ball per encounter, egg locations; largest PB8
groups: no matching encounter, ball per encounter, fateful flags, nickname language for relaxed species, PID = EC
on BD/SP natives (fixed-EC WB8 gifts need the encounter), egg locations; largest PK9 groups:
no matching encounter, ball per encounter, fateful flags, nickname language for relaxed species, Tera type per
encounter or evolution chain, ball availability, gift matching, learnset-only moves). The driver
prints this gap list ("left Incomplete, by first PKHeX result") on every run.

`tests/fixtures/gen67/pkhex8_seed2_sample.txt` (1,632 PK8 records from `generate8 2 4`,
stratified the same way) does the same for PK8 in `tests/test_legality_gen8.cpp`
(`test_legality_gen8{,_sanitize}`), with PK8 rule edits, valid edits that must stay
undecided (mint, Dynamax 10, battle version on a Gen VII record), egg rules, structure,
interface and fuzz checks. `tests/fixtures/gen67/pkhexb8_seed2_sample.txt` (2,570 PB8 records
from `generate8b 2 4`) is checked by the same test, with PB8 field edits (sociability,
Dpr-illegal flag, Gigantamax, Dynamax, TR bytes, battle version, Hyper Training below 100,
species above 493, ball, Spinda tracker, Spinda/Nincada moved in) and edits that must stay
undecided (PID = EC on a native, EC 0 on a Sword/Shield record, battle version on a moved
record, TR bytes on an egg). `tests/fixtures/gen67/pkhex9_seed2_sample.txt` (2,202 PK9 records
from `generate9 2 4`, with up to eight unmutated records per stratum) adds PK9 edits: Tera
types, obedience level, battle version, origin, internal species id, language, Hyper Training,
ball, held item, TM flags, the Griseous Core, the Tera override of a moved record, HOME's scale
copy and egg rules (Tera override, nickname flag, Jacq's egg, version 0), plus edits that must
stay undecided. The fuzz pass mixes PK8, PB8 and PK9.

`tests/fixtures/gen67/pkhex_seed2_sample.txt` (1,531 records, stratified by group, mutation
and NX rule) locks each record's verdict class in `tests/test_legality_gen67.cpp`, which also
covers rule edits, the interface contract, malformed input, encrypted/decrypted equality,
input immutability and a deterministic fuzz pass. It runs under GCC and Clang, normal and
ASan/UBSan (`make -f Makefile.host … test_legality_gen67{,_sanitize}`).

`tests/fixtures/gen67/pkhex7b_seed2_sample.txt` (418 PB7 records from `generate7b 2 4`) is
checked by `tests/test_legality_gen7b.cpp` (`test_legality_gen7b{,_sanitize}`), with PB7 edits
(EVs, Awakening Values, CP, spirit, mood, received time and date, height and weight, origin,
GO without GO Park, species, egg, PID = EC, ball, Hyper Training, held item, checksum), edits
that must stay undecided (spirit and mood in the party, a traded record's received date, the
partner's CP and spirit), the source-game warning, the interface contract and a fuzz pass.

`tests/fixtures/gen67/pkhex8a_seed2_sample.txt` (2,420 PA8 records from `generate8a 2 4`) is
checked by `tests/test_legality_gen8a.cpp` (`test_legality_gen8a{,_sanitize}`), with the codec
round trip (stored and party), PA8 edits (EVs, PP Ups, PP, noble, scale, Effort Levels, alpha
move, move shop purchases, sociability, Gigantamax, Dynamax, TR flags, fullness, enjoyment,
battle version, natures, height, weight, species, presence, ball, language, Hyper Training,
held item, checksum, sanity), edits that must stay undecided (a permitted purchase, a mastered
alpha move on an alpha, sizes with a HOME tracker, on an alpha or on a transfer), the
source-game warning, the interface contract and a fuzz pass. Its largest PKHeX gaps are
encounter matching, alpha size and alpha flag per encounter, fateful flags, mastery without a
purchase, PID = EC and ball per encounter.

`tests/fixtures/gen67/pkhex9a_seed2_sample.txt` (2,196 PA9 records from `generate9a 2 4`) is
checked by `tests/test_legality_gen9a.cpp` (`test_legality_gen9a{,_sanitize}`), with PA9 edits
(EVs, PP Ups, used PP, level boost, Plus flags past the end, a Plus move the line cannot learn, battle version, obedience, an alpha
through HOME, natures, ability number, duplicate moves, species, ball, languages, Hyper Training,
held item, checksum, sanity) and edits that must stay undecided (0 PP, an in-range Plus flag, a
Plus move the line learns, a
Sword/Shield battle version, a native alpha's scalars, another species' ability, an empty first
slot, Spanish (LATAM) on a Z-A origin), the source-game warning and a fuzz pass. Its largest
PKHeX gaps are encounter matching, Plus moves per level, ball per encounter, nickname
language, fateful flags, Hyper Training level and PID = EC.
