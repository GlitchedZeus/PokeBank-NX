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
| `include/Integration/Gen8/Gen8PK8.h` (PK8 codec, next milestone) | `33ecd733bfec5e9c84281e7ea45772267a8fd038` |

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

PK6 and PK7 have rules today. PB7/PK8/PB8/PA8/PK9/PA9 are accepted and reported Incomplete
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
| Virtual Console | female OT outside Crystal; met level 100 nature = EXP % 25; met level ≤ 2 nature set; VC language vs console region | `TransferVerifier.VerifyTransferLegalityG12` |
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

## Differential evidence

`tools/pkhex_gen67_diff/run.sh PKHEX [SEED] [MUTATIONS]` builds the oracle against PKHeX,
checks the generated tables byte for byte, generates a corpus and runs `gen67_diff.cpp`.
It fails on any false Invalid (PKHeX valid, NX Invalid) or false Valid (PKHeX invalid, NX
"No problems found"). Results on 2026-10-31 (local, .NET 10.0.114, g++ 13.3):

| Seed / mutations | Records | PKHeX valid | PKHeX invalid | NX Invalid of PKHeX-invalid | False Invalid | False Valid |
|---|---:|---:|---:|---:|---:|---:|
| 1 / 4 | 88,012 | 42,080 | 45,932 | 36,805 (80.1%) | 0 | 0 |
| 2 / 8 | 155,118 | 63,170 | 91,948 | 73,937 (80.4%) | 0 | 0 |
| 3 / 8 | 156,298 | 63,386 | 92,912 | 74,779 (80.5%) | 0 | 0 |
| 4 / 4 | 89,523 | 42,679 | 46,844 | 37,753 (80.6%) | 0 | 0 |

The corpus is synthetic (PKHeX builds every record from its own templates for a test
trainer); it is not a personal save and not retail evidence. The detection rate measures how
much of PKHeX's rejection these encounter-independent rules reproduce. It is not a measure of
rule completeness. The remaining ~20% need encounter, learnset, event or history data
(largest groups: fateful flags, geography consistency, egg locations, ball per encounter,
learnset-only moves, nickname language).

`tests/fixtures/gen67/pkhex_seed2_sample.txt` (1,531 records, stratified by group, mutation
and NX rule) locks each record's verdict class in `tests/test_legality_gen67.cpp`, which also
covers rule edits, the interface contract, malformed input, encrypted/decrypted equality,
input immutability and a deterministic fuzz pass. It runs under GCC and Clang, normal and
ASan/UBSan (`make -f Makefile.host … test_legality_gen67{,_sanitize}`).
