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
  - The Day Care sets the level from EXP when a Pokémon is withdrawn, and caps (Gen I,
    pokered `scripts/Daycare.asm`) or resets (Gen II, pokecrystal `RetrieveBreedmon`,
    pret `docs/bugs_and_glitches.md` "might lose experience") the EXP to that level. A
    Pokémon still in the Day Care carries a stale level and, in both generations, EXP that
    can pass the level-100 total; PokeBank NX's save parsers do not read the Day Care
    slot, and a record exported from it would be judged by these rules.
  - PKHeX rejects EXP above the level-100 total (`LevelEXPTooHigh`) but derives the level
    from EXP, so it does not compare the two: a stored level the EXP does not give is a
    deliberate difference.
- **GS Ball Celebi.** Japanese Crystal's Mobile System GB event (27 January 2001 –
  14 December 2002; Serebii, Bulbapedia) gave the GS Ball. Kurt takes it to the Ilex Forest
  shrine, which summons a wild level-30 Celebi (pokecrystal `maps/IlexForest.asm`). A
  Japanese PK2 Celebi with that met data is legal. PKHeX models this Celebi for Virtual
  Console only (`Encounters2.CelebiVC`): in the Western cartridges only the Virtual
  Console hook sets the flag (`sGSBallFlag`).

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
  the total);
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

Seeds 1–3, 50,000 records each:

| | PK1 | PK2 |
|---|---|---|
| Records | 150,000 | 150,000 |
| Matching legal | 109,865 | 104,661 |
| Matching invalid | 38,741 | 42,282 |
| **Differing** | **0** | **0** |
| Deliberately uncertain: Mew/Celebi from an unrecorded event (PKHeX INVALID) | 66 | 107 |
| Deliberately uncertain: Korean, needs a Korean–Western trade (PKHeX INVALID / VALID) | – | 777 / 152 |
| Deliberately uncertain: Korean met data (PKHeX INVALID / VALID) | – | 472 / 63 |
| Deliberately uncertain: two of the reasons above (PKHeX INVALID) | – | 16 |
| Deliberately uncertain: Korean trainer name outside the Hangul pages (PKHeX INVALID) | – | 82 |
| Deliberately different: stored level not the level of the EXP (ours INVALID, PKHeX VALID) | 1,328 | 1,352 |
| Deliberately different: GS Ball Celebi (ours VALID, PKHeX INVALID) | – | 36 |

Every one of the 300,000 records has a verdict from both programs. The structural kinds
all agree with PKHeX where PKHeX decides: duplicate moves, EXP 0 and EXP over the
level-100 total are Invalid for both (450 records per format); the two legal boundaries
are legal for both (180 per format). The 150 Korean
records PKHeX accepts but that only a trade explains are mostly Crystal Odd Eggs: PKHeX
allows the Odd Egg for Korean records (`ParseSettings.AllowGen2OddEgg` excludes only
Japanese) while it rules out every other Crystal source. No record of these seeds is
uncertain through the evolution-order rule: the 60 `evo-order` histories (two moves
learned in an order the Gen I games alone cannot produce) are all legal for both
programs, through Stadium 2's reminder.

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
| Incomplete for the non-definitive rules above | Invalid, or valid (Korean Odd Egg, Korean met data) | see the Uncertain columns |

## Not covered yet

- **Gen II in the app:** waits on independent verification of the Gen I/II code.
- **Events:** GB-era distributions beyond PKHeX's `event2`/Gen I event tables. Japanese
  Mobile System GB content other than the GS Ball is not modelled; a non-Mew/Celebi
  Pokémon from an unrecorded event is Invalid, as in PKHeX.
- **Glitches:** glitch encounters (the Mew glitch, Old Man/MissingNo., Celebi or
  Time Capsule glitches) are not modelled; only Mew's case is softened (Incomplete).
- **Japanese Red/Green/Blue:** encounters and in-game trades follow PKHeX's tables (the
  Red/Green slots and Japanese Blue's own slots and trades). Any other Japanese-version
  difference is only as complete as those tables.
- **Name character sets:** checked only for Korean trainer names (unconfirmed). Names are
  otherwise judged by length, since callers decode them.
- **Korean edge cases:** a PK1 holding a Korean Gold/Silver Pokémon (Korean → Western
  Red/Blue/Yellow) is not modelled.
- **Day Care slots:** a Pokémon still in the Day Care (stale level, EXP possibly past the
  level-100 total) is not read by the save parsers; exported on its own, it would be
  judged Invalid by the level/EXP rules.
