# Gen I / Gen II legality verdicts

`src/Legality/GBLegality.cpp` searches every Red/Blue/Yellow and Gold/Silver/Crystal
origin a PK1 or PK2 record could have (wild slots, statics and gifts, in-game trades,
eggs, recorded event distributions, Time Capsule trips both ways) and checks the
record's moves, levels, catch rate or held item, DVs, met data, PP, stat experience,
Pokérus, trainer name and nickname against that origin. The rules are ported from PKHeX.Core
(kwsch/PKHeX@6501f0ab, GB cartridge-era settings) and the data comes from the same
PKHeX resources, cross-checked against pret (pokered/pokeyellow/pokegold/pokecrystal).
Where the cartridges and PKHeX disagree, the cartridge source decides and the difference
is listed below.

Both are shown in the app. `Legality::analyze` uses `GB::checkGen1` for native PK1
records and `GB::checkGen2` for native PK2 records; `tools/pkhex_gb_diff/run.sh` checks that
the app path gives the same verdict as the search on the whole corpus.

## What each verdict means

| `GB::Status` | App report | When |
|---|---|---|
| `Legal` | **No problems found** | A legitimate origin exists and every check passes. That shows the Pokémon is possible, not where it actually came from; the report says so in an Info line. |
| `Illegal` | **Invalid** | A rule that holds for any origin fails (stat experience, PP, types, Pokérus, names), or every origin is excluded by a definitive rule. |
| `Uncertain` | **Incomplete**, with warnings | Every origin was excluded, but only by a rule PokeBank NX does not treat as definitive. The encounter and event coverage are marked Partial. |

A failed candidate never makes a record Invalid while another candidate is still open:
the search keeps the candidate that only non-definitive rules exclude. The rules that are
not definitive (`Problem::definitive == false`):

1. **Gen I evolution-order moves.** An evolved Pokémon cannot know a move its
   pre-evolution learned after the level at which it must already have evolved: Gen I
   has no move relearner. Pokémon Stadium 2's reminder is the exception. It reads a
   Pokémon straight from a Red/Blue/Yellow cartridge through the Transfer Pak and
   re-teaches level-up moves of its current species (PKHeX `LearnSource2Stadium` and its
   `reminder_stad2` table; Bulbapedia, "Game move errors", records Yellow-cart reminders).
   The rule therefore only fires when the reminder cannot restore the move either. PKHeX
   states the rule (`LearnGroup1.FlagEvolutionSlots`) but never reaches it for PK1, so no
   independent implementation checks it. The Exeggutor with Stomp and PoisonPowder that
   used to be the example is legal through the reminder.
2. **Mew and Celebi from an unrecorded distribution.** The GB-era event list is not known
   to be complete, and Red/Blue/Yellow's Mew glitch (not modelled by PKHeX) catches a Mew
   with the player's own trainer data. Matching nothing in our tables is therefore not
   proof. Rules that hold whatever the event was (Mew's catch rate, stat experience, PP,
   trainer name and nickname length) still make such a record Invalid.
3. **Korean Gold/Silver records that only a Korean–Western trade explains.** See below.
4. **Korean records with Crystal met data**, and **Korean trainer names with
   one-byte-page characters** (Latin letters, digits, space, `/`, ♂, ♀, ×). See below.
5. **A Day Care slot's level and EXP.** A stored level below the level its EXP gives, and
   EXP above the level-100 total up to `$50FFFF` (any level on a PK1, below level 100 on a
   PK2). See "Other rules".
6. **A Korean trainer name on a PK1.** It needs the Korean–Western trade of rule 3 and a
   Time Capsule trip. See "Korean Gold/Silver".
7. **Name characters.** A digit in a Western name; a Japanese trainer name outside the
   kana; ？ or ！ in a Japanese trainer name on a Gen I origin. See "Names".
8. **Name bytes with no glyph.** A byte PKHeX's font tables give no character, before the
   terminator. See "Names".
9. **Mew's and Celebi's unrecorded events** are rule 2; the GS Ball Celebi is a deliberate
   difference, not an uncertainty.

## Korean Gold/Silver

A PK2 whose trainer name starts with a Hangul page byte (1–11) is Korean, as in PKHeX
(`PK2.Korean`). `GB::Language::Korean` applies PKHeX's Korean rules where they decide:

- the Korean in-game trade names;
- no Crystal encounter or event distribution (there is no Korean Crystal, and no GB-era
  event reached Korean Gold/Silver);
- no Gen I origin;
- trainer name and nickname of at most 5 characters.

PKHeX goes further and keeps a Korean record inside Korean Gold/Silver: no Crystal
learnsets, tutor, eggs or Odd Egg, no Stadium 2 reminder and no Gen I visit
(`ParseSettings.AllowGen2Crystal`, `AllowGen2MoveReminder`, `GBRestrictions`,
`LearnGroup2`). The cartridges were not that closed. Bulbapedia ("Korean character
encoding (Generation II)", "Trade") records that Korean Gold/Silver trade with Western
Gen I and II cartridges, garbling the Hangul names; only the Virtual Console releases
refuse. Japanese and Korean cartridges cannot trade. Through such a trade:

- a Korean player's Pokémon can learn Crystal's moves;
- it can be reminded of moves by Stadium 2 on a Western cartridge;
- it can visit Red/Blue/Yellow;
- an egg from a Western Crystal (Crystal egg moves, the Odd Egg) or a Pokémon Center
  New York egg hatches with the Korean player as its trainer, since hatching sets the
  trainer (pokegold `engine/pokemon/breeding.asm`, `HatchEggs`).

That route rests on one secondary source. A record that needs it is therefore
**Incomplete**, citing the trade, not Invalid on PKHeX's rule and not legal.

Two Korean results stay Invalid whatever the trades:

- a Korean trainer on something caught or received in Crystal or Red/Blue/Yellow;
- a Korean trainer on a non-egg event gift.

Korean met data is reported as unconfirmed. Only Crystal writes met data: pokecrystal's
`SetCaughtData` family runs on catch, hatch, gift and in-game trade, never on a link
trade. Gold/Silver zero those bytes (pokegold `engine/pokemon/move_mon.asm`). The Korean
ROM is not disassembled, so this is not taken as proof. The search ignores those bytes,
as PKHeX does, and the record is at best Incomplete.

PKHeX also requires a Korean trainer name to use only Hangul-page characters
(`G1CharOT`). We have not checked that against the Korean naming screen, so it also
leaves the record Incomplete.

PokeBank NX has no Korean save support. The harness decodes Korean names with a table
generated at run time from PKHeX's `StringConverter2KOR` (`tools/pkhex_gb_diff/gb_text.py`);
nothing derived from it is committed.

## Other rules

- **Gen II egg names.** An unhatched egg keeps the name the game gave it. pokegold and
  pokecrystal write `EGG@`, and nothing renames an egg. Like PKHeX (`VerifyNicknameEgg`),
  any Gen II language's egg name is accepted: EGG, OEUF, UOVO, EI, HUEVO, タマゴ, 알.
- **No duplicate moves.** Learning a move a Pokémon already knows does nothing, so a
  record with the same move in two slots is Invalid (PKHeX `Move1:Duplicate`), for PK1
  and PK2, alongside the existing packing rules (no gap before a move, at least one move).
- **The stored level is the level the EXP gives, and EXP stops at the level-100 total.**
  Both rules apply to PK1 and PK2 (`GB::formatProblems`); unhatched eggs are judged by
  their encounter instead.
  - Battle EXP raises the level as thresholds are crossed and stops EXP at the level-100
    total (pokered `engine/battle/experience.asm`, pokecrystal `engine/battle/core.asm`).
  - Rare Candy refuses level 100 and otherwise sets EXP to the new level's total
    (`engine/items/item_effects.asm`).
  - The Day Care is the one place level and EXP part. Gen I adds one EXP per step with
    no level check and holds the top byte at `$50` (pokered
    `engine/overworld/daycare_exp.asm:1-16`); Gen II skips a level-100 Pokémon and caps the
    EXP at `MAX_DAY_CARE_EXP = $500000` (pokecrystal `engine/events/happiness_egg.asm`
    `DayCareStep`). Withdrawal recomputes the level from the EXP (pokered
    `scripts/Daycare.asm:69-83`; pokecrystal `engine/pokemon/move_mon.asm` `RetrieveBreedmon`,
    which also resets the EXP, pret `docs/bugs_and_glitches.md` "might lose experience").
    So a stored level **ahead** of its EXP is Invalid; a stored level **behind** its EXP, or
    EXP above the level-100 total up to `$50FFFF` (any level on a PK1, below 100 on a PK2),
    is **Incomplete**: only a Pokémon still in the Day Care carries it, and the save parsers
    do not read that slot. EXP past `$50FFFF`, or above the total at level 100 on a PK2, is
    Invalid.
  - PKHeX rejects EXP above the level-100 total only at level 100 (`LevelEXPTooHigh`) and
    otherwise derives the level from EXP, so it does not compare the two: the level-ahead
    rule is a deliberate difference (`PKHEX_VERIFIED_IMPROVEMENTS.md` D-1), the Day Care
    softening is review item 15-D.
- **GS Ball Celebi.** Japanese Crystal's Mobile System GB event (27 January 2001 –
  14 December 2002; Serebii, Bulbapedia) gave the GS Ball. Kurt takes it to the Ilex Forest
  shrine, which summons a wild level-30 Celebi (pokecrystal `maps/IlexForest.asm`). A
  Japanese PK2 Celebi with that met data is legal. PKHeX models this Celebi for Virtual
  Console only (`Encounters2.CelebiVC`): in the Western cartridges only the Virtual
  Console hook sets the flag (`sGSBallFlag`).

## Names

The engine takes the decoded names and, when the caller has them, the stored name bytes
(`Entity1/Entity2::otRaw`, `nickRaw`; empty means not available, and the byte rules are
skipped). The save parsers keep the stored bytes (`PokemonRecord::originalTrainerRaw`,
`nicknameRaw`, since `5028a0a3`) and `Legality::analyze` passes them on; `tools/pkhex_gb_diff`
passes both, and `production_parity.cpp` holds production to the search with the bytes and
counts the records whose verdict depends on them.

- **Lengths** (PKHeX `TrainerNameVerifier.VerifyGBOTWithinBounds`,
  `NicknameVerifier.VerifyG1NicknameWithinBounds`): trainer 7, nickname 10; 5 and 5 in
  Japanese and Korean. A Korean name is measured on its bytes (a Hangul character is two
  bytes). Empty trainer name: Invalid; an egg's nickname is the game's egg name.
- **In-game-trade marker.** `<TRAINER>` (`$5D`) is followed by the terminator: pokered
  `macros/data.asm` `dname` writes `<TRAINER>@` (PKHeX `EncounterTrade1.IsTrainerNameValid`).
  A letter after the marker is Invalid; the marker on anything but a Red/Blue/Yellow in-game
  trade is Invalid.
- **Glyphless bytes.** A byte before the terminator that PKHeX's `StringConverter1/2`
  tables give no character (the games write none; the decoders stop there, so the name
  reads shorter than it is stored) is a Warning: a garbled Korean trade or a damaged record.
  The glyphless kinds in the corpus are how PKHeX comes to judge an empty or shorter name.
- **Digits.** No disassembled naming screen offers a digit (pokered
  `data/text/alphabets.asm`; pokecrystal `data/text/name_input_chars.asm`, where only box
  names get them), and no species, trade or event name holds one: a digit in a Western
  trainer name or nickname is a Warning (the other languages' screens are not disassembled).
  PKHeX accepts digits.
- **Japanese trainer names.** PKHeX requires kana (`StringConverter1.GetIsJapanese`) and,
  for a Gen II encounter, also ？ and ！ (`StringConverter2.GetIsJapanese`). A character
  outside that is a Warning here. The two marks are judged per origin: fine with a Gen II
  origin on either format, a Warning on a Gen I origin, so a record with only Gen I origins
  is Incomplete. PKHeX judges by the first encounter it matched and does not look for
  another, so it calls a record with ？ INVALID (`G1CharOT`) when that first match is Gen I
  while a Gen II egg or slot is open (`jp-ot-mark-gen1-pick` below). Review item 16-D.
- **Korean trainer names on a PK1.** A Hangul page byte (1–11) first in the trainer name
  marks the record Korean (PKHeX `PK1.Korean`); no Korean Red/Blue/Yellow exists, so the
  record needs the Korean–Western trade above and a Time Capsule trip: Legal becomes
  Incomplete, and when no Gen II origin fits (the catch-rate byte is not a Gen II held
  item, a Gen I-only move or species) it is Invalid. PKHeX accepts such a record as a
  Red/Blue/Yellow native (`korean-pk1-origin` below). Review item 17-D.

## Evidence

`tools/pkhex_gb_diff/run.sh` builds a corpus of raw party records and runs them through
PKHeX.Core and through `GBLegality`. Every record lands in exactly one class: matching
legal, matching invalid, deliberately uncertain, deliberately different, or differing (a
failure). `compare.py` enforces the protocol and the exceptions:

- A malformed or duplicate corpus line, a malformed or duplicate verdict line, a verdict
  for an unknown record, a record without a verdict from either program, or a verdict
  word a program may not give stops the run (exit 2). The PKHeX oracle and `gb_diff`
  reject malformed corpus lines instead of skipping them.
- No exception is granted on explanatory text alone. Each is a predicate on the record's
  own bytes and on both programs' complaints: an "event only" reason needs a Mew or
  Celebi, a Korean reason a Hangul trainer name (and met data, or a one-byte-page
  character, where the reason says so), the GS Ball difference a Japanese Celebi met at
  Ilex Forest at level 30 with no other PKHeX complaint, the level/EXP difference a
  stored level the EXP does not give and no other PokeBank NX complaint.
- Every uncertain or deliberately different record is listed by id with its reason.
- `compare.py --self-test` (run by `run.sh` first) corrupts the protocol 20 ways and
  checks that each is caught.

The corpora contain:

- records built the way the games build them;
- deliberate defects: moves, levels, catch rate, types, held items, met data, PP, egg
  data, egg names, stat experience, level-100 EXP, Pokérus, over-long or empty names;
- structural defects (`struct-*`, `struct2-*`, 30 records per kind and seed, plus a share
  of the random defects): duplicate moves, EXP 0, EXP one short of the level, EXP of the
  next level, and level 100 with EXP at, one under, one over and far over the total, with
  the two boundaries that must stay legal (EXP one short of the next level, level 100 at
  the total); the Day Care states (`exp-daycare-stale`, `-99`, `-reach` at `$50FFFF`,
  `-past` at `$510000`);
- name bytes and characters (30 records per kind and seed): digits in the trainer name
  and nickname, a glyphless byte in a Western and in a Japanese name, a Japanese trainer
  name outside the kana, ？/！ with a Gen II origin (`jp-ot-mark-ok`) and with only Gen I
  origins (`jp-ot-mark-gen1`), the trade marker followed by a letter, and on PK1 the Korean
  names (`kor1-tradeback`, `kor1-rby`, `kor1-ot-one-byte`, `kor1-ot-long`);
- random records.

About a quarter of the records are Japanese, encoded with PKHeX's own
`StringConverter1/2` tables. Targeted kinds cover:

- **Gen I:**
  - evolution-order histories (`evo-order`, full histories from the lowest wild level);
  - PK1s that started in Gen II (`from-gen2`);
  - the European in-game trades (`trade-<lang>`), right and renamed.
- **Gen II:**
  - the European in-game trades;
  - GS Ball Celebi (`gsball2-*`, with wrong level, place and language);
  - Korean Gold/Silver: 20 `kor2-*` kinds of 40 records each, one rule per kind, plus a
    Korean share of the random kinds.

Seeds 1–5, 50,000 records each (`tools/pkhex_gb_diff/run.sh /home/user/ext/pkhex
/home/user/ext/pokered 50000 <seed> <gen>`; CI runs seeds 1–3). Every record has a verdict
from both programs and lands in one class:

| | PK1 | PK2 |
|---|---|---|
| Records | 250,000 | 250,000 |
| Matching legal | 181,346 | 173,259 |
| Matching invalid | 56,461 | 67,415 |
| **Differing** | **0** | **0** |
| Deliberately uncertain, total | 10,922 | 7,494 |
| – Mew/Celebi from an unrecorded event (PKHeX INVALID) | 120 | 164 (+6 with Korean met data) |
| – battle stat experience with a stat still at 0 (PKHeX INVALID) | 6,191 | 2,812 (+2 with the Korean trade) |
| – Day Care: EXP over the level-100 total within `$50FFFF` (PKHeX INVALID) | 2,320 | 366 |
| – Day Care: stored level below its EXP (PKHeX VALID) | 1,239 | 728 (+1 with the Korean trade) |
| – Korean, needs a Korean–Western trade (PKHeX INVALID / VALID) | – | 1,277 / 246 |
| – Korean met data (PKHeX INVALID / VALID) | – | 797 / 97 (+17 with the trade) |
| – Korean trainer name outside the Hangul pages (PKHeX INVALID) | – | 140 |
| – Korean trainer name on a PK1 (PKHeX INVALID / VALID) | 75 / 300 | – |
| – digit in a Western name (PKHeX VALID; 1 word-filter INVALID) | 300 | 300 |
| – glyphless byte in a name (PKHeX INVALID / VALID) | 31 / 233 | 39 / 233 |
| – Japanese trainer name outside the kana (PKHeX INVALID) | 61 | 119 |
| – ？/！ in a Japanese trainer name, only Gen I origins (PKHeX INVALID) | 52 | 150 |
| Deliberately different, total | 1,271 | 1,832 |
| – stored level ahead of its EXP (ours INVALID, PKHeX VALID) | 1,160 | 1,652 |
| – Korean-named PK1 with no Gen II origin (ours INVALID, PKHeX VALID) | 100 | – |
| – ？/！ with a Gen II origin open (ours VALID, PKHeX INVALID `G1CharOT`) | 11 | 120 |
| – GS Ball Celebi (ours VALID, PKHeX INVALID) | – | 60 |

Seeds 1–3 alone (the CI set): PK1 108,738 legal / 33,926 invalid / 0 differing; PK2 103,891 /
40,525 / 0. The previous published counts (before the Day Care, name and Korean-PK1 kinds were
added, same seeds) were PK1 109,865 / 38,741 / 0 and PK2 104,661 / 42,282 / 0; the corpus is
not the same, so the two sets are not comparable record for record.

Production parity passed on every run: PK1 50,000 of 50,000 records through `Legality::analyze`
per seed (174–177 of them with a verdict that needs the stored name bytes the parsers do not
carry yet); PK2 45,824–45,949 per seed (4,051–4,176 Korean records skipped, which the parsers do
not read; 52–56 needing the name bytes).

The structural kinds all agree with PKHeX where PKHeX decides: duplicate moves, EXP 0 and EXP
over the level-100 total at level 100 are Invalid for both; the two legal boundaries are legal
for both. The Korean records PKHeX accepts but that only a trade explains are mostly Crystal Odd
Eggs: PKHeX allows the Odd Egg for Korean records (`ParseSettings.AllowGen2OddEgg` excludes
only Japanese) while it rules out every other Crystal source. No record of these seeds is
uncertain through the evolution-order rule: the `evo-order` histories (two moves learned in an
order the Gen I games alone cannot produce) are all legal for both programs, through Stadium 2's
reminder.

`tests/test_gb_legality_gen1.cpp` (through `Legality::analyze`) and
`tests/test_gb_legality_gen2.cpp` keep PKHeX-verdict samples of every corpus kind in the
ordinary host suite. They include the uncertain and deliberate cases above, every Korean
outcome, and each record a fix in this document came from.
`tests/test_gb_legality_structural.cpp` covers the structural rules: duplicate moves in
every slot pattern, a level/EXP grid over the four Gen I/II growth rates checked against
the games' EXP formulas, a checksum-valid Red save with a level-100 Bulbasaur at, above
and below the EXP total through the production analyzer (save bytes unchanged), and native
PK2 records.

## Deliberate differences from PKHeX

| PokeBank NX | PKHeX | Records |
|---|---|---|
| The stored level must be the level the EXP gives (PK1 and PK2): battle, Rare Candy and the Day Care keep them in step. | Derives the level from EXP; does not compare | `struct*-exp-below`, `struct*-exp-next`, `struct*-exp-cap-minus1`, level defects |
| GS Ball Celebi in Japanese Crystal is legal | Invalid (Virtual Console only) | `gsball2-caught`, `gsball2-grown` |
| No word filter: Game Boy games filter no names | `WordFilterFlaggedPattern` on some random nicknames | random Latin nicknames |
| A Hangul-named PK1 with no Gen II origin (catch rate not a held item, Gen I-only move or species) is Invalid | Valid as a Red/Blue/Yellow native | `korean-pk1-origin`: `kor1-rby`, `kor1-ot-long` |
| ？/！ in a Japanese trainer name with a Gen II origin open is legal | Invalid (`G1CharOT`) when the first encounter it matched is Gen I | `jp-ot-mark-gen1-pick`: `jp-ot-mark-ok` |
| Incomplete for the non-definitive rules above | Invalid, or valid (Korean Odd Egg, Korean met data, Day Care stale level, digits, glyphless bytes, Korean PK1) | see the Uncertain columns |

## Not covered yet

- **Events:** GB-era distributions beyond PKHeX's `event2`/Gen I event tables. Japanese
  Mobile System GB content other than the GS Ball is not modelled; a non-Mew/Celebi
  Pokémon from an unrecorded event is Invalid, as in PKHeX.
- **Glitches:** glitch encounters (the Mew glitch, Old Man/MissingNo., Celebi or
  Time Capsule glitches) are not modelled; only Mew's case is softened (Incomplete).
- **Japanese Red/Green/Blue:** encounters and in-game trades follow PKHeX's tables (the
  Red/Green slots and Japanese Blue's own slots and trades). Any other Japanese-version
  difference is only as complete as those tables.
- **Name character sets:** the Japanese and Korean naming screens and the non-English
  Western screens are not disassembled, so those character rules (see "Names") are
  Warnings, never Invalid. The byte rules run on the stored name bytes in production (`Gen1ReadOnlySave`/`Gen2ReadOnlySave` keep them since `5028a0a3`).
- **Korean edge cases:** the Korean ROM is not disassembled; Korean met data, the Korean
  naming screen and the Korean–Western trade rest on PKHeX and Bulbapedia (Incomplete).
- **Day Care slots:** the save parsers do not read the Day Care slot; a record exported
  from it is judged Incomplete by the level/EXP rules, never Invalid, within the step
  counter's reach.
