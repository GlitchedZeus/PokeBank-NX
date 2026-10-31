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

PK6, PK7, PK8, PB8 and PK9 have rules today. PB7/PA8/PA9 are accepted and reported Incomplete
with structure coverage None until their decoders and rules are wired in.

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
bytes and nicknames/word filter, Bank/Transporter history beyond the PID and met-location
rules, fateful flags and Super Training.

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
per encounter, ribbons and marks, memories, trash bytes, nicknames/word filter, Pokémon GO
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
encounter, fateful flags, egg locations, ribbons, memories, trash bytes and nicknames.

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
ribbons and marks, memories, trash bytes, nicknames, and HOME history (pre-DLC learnset limits,
height/weight 0/0 after a Legends: Arceus visit).

## Differential evidence

`tools/pkhex_gen67_diff/run.sh PKHEX [SEED] [MUTATIONS]` builds the oracle against PKHeX,
checks the generated tables byte for byte, generates a corpus and runs `gen67_diff.cpp`.
It fails on any false Invalid (PKHeX valid, NX Invalid) or false Valid (PKHeX invalid, NX
"No problems found"). Results on 2026-10-31 (local, .NET 10.0.114, g++ 13.3):

| Format | Seed / mutations | Records | PKHeX valid | PKHeX invalid | NX Invalid of PKHeX-invalid | False Invalid | False Valid |
|---|---|---:|---:|---:|---:|---:|---:|
| PK6/PK7 | 1 / 4 | 88,012 | 42,094 | 45,918 | 36,883 (80.3%) | 0 | 0 |
| PK6/PK7 | 2 / 8 | 155,118 | 63,201 | 91,917 | 74,064 (80.6%) | 0 | 0 |
| PK6/PK7 | 3 / 8 | 156,298 | 63,436 | 92,862 | 74,892 (80.6%) | 0 | 0 |
| PK6/PK7 | 4 / 4 | 89,523 | 42,686 | 46,837 | 37,809 (80.7%) | 0 | 0 |
| PK8 | 1 / 4 | 46,387 | 19,987 | 26,400 | 20,366 (77.1%) | 0 | 0 |
| PK8 | 2 / 4 | 45,298 | 19,779 | 25,519 | 19,615 (76.9%) | 0 | 0 |
| PK8 | 3 / 8 | 81,530 | 29,728 | 51,802 | 39,815 (76.9%) | 0 | 0 |
| PK8 | 4 / 4 | 45,489 | 19,817 | 25,672 | 19,709 (76.8%) | 0 | 0 |
| PB8 | 1 / 4 | 44,334 | 18,431 | 25,903 | 21,392 (82.6%) | 0 | 0 |
| PB8 | 2 / 4 | 44,643 | 18,787 | 25,856 | 21,320 (82.5%) | 0 | 0 |
| PB8 | 3 / 8 | 77,736 | 26,371 | 51,365 | 42,100 (82.0%) | 0 | 0 |
| PB8 | 4 / 4 | 45,193 | 18,952 | 26,241 | 21,493 (81.9%) | 0 | 0 |
| PK9 | 1 / 4 | 39,685 | 18,653 | 21,032 | 17,351 (82.5%) | 0 | 0 |
| PK9 | 2 / 4 | 39,929 | 18,565 | 21,364 | 17,626 (82.5%) | 0 | 0 |
| PK9 | 3 / 8 | 69,293 | 27,200 | 42,093 | 35,065 (83.3%) | 0 | 0 |
| PK9 | 4 / 4 | 39,077 | 18,337 | 20,740 | 17,131 (82.6%) | 0 | 0 |

PK8 runs use `run.sh PKHEX SEED MUTATIONS 8`, PB8 runs `8b` and PK9 runs `9`.
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
encounter, learnset-only moves, nickname language; largest PK8 groups: no matching
encounter, fateful flags, Gigantamax per encounter, nickname language, PID type per
encounter, Hyper Training level 50–99, ball per encounter, egg locations; largest PB8
groups: no matching encounter, ball per encounter, fateful flags, nickname language, PID = EC
on BD/SP natives (fixed-EC WB8 gifts need the encounter), egg locations; largest PK9 groups:
no matching encounter, ball per encounter, fateful flags, nickname language, Tera type per
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
