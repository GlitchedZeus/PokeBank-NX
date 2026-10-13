# Gen I / Gen II legality verdicts

`src/Legality/GBLegality.cpp` searches every Red/Blue/Yellow and Gold/Silver/Crystal
origin a PK1 or PK2 record could have (wild slots, statics and gifts, in-game trades,
eggs, recorded event distributions, Time Capsule trips both ways) and checks the
record's moves, levels, catch rate or held item, DVs, met data, PP, stat experience,
Pokérus, trainer name and nickname against that origin. The rules are ported from PKHeX.Core
(kwsch/PKHeX@6501f0ab, GB cartridge-era settings) and the data comes from the same
PKHeX resources, cross-checked against pret (pokered/pokeyellow/pokegold/pokecrystal).

Only Gen I verdicts are shown in the app. `Legality::analyze` uses `GB::checkGen1` for
native PK1 records; `GB::checkGen2` is tested but not yet wired into the app.

## What each verdict means

| `GB::Status` | App report | When |
|---|---|---|
| `Legal` | **No problems found** | A legitimate origin exists and every check passes. That shows the Pokémon is possible, not where it actually came from; the report says so in an Info line. |
| `Illegal` | **Invalid** | A rule that holds for any origin fails (stat experience, PP, types, Pokérus, names), or every origin is excluded by a definitive rule. |
| `Uncertain` | **Incomplete**, with warnings | Every origin was excluded, but only by a rule PokeBank NX does not treat as definitive. The encounter and event coverage are marked Partial. |

There are two non-definitive rules (`Problem::definitive == false`):

1. **Gen I evolution-order moves.** For example, an Exeggutor that knows Stomp (Exeggutor
   learns it at level 28) and PoisonPowder (Exeggcute learns it at level 37) would have needed
   to be Exeggcute after it had already become Exeggutor. Gen I has no move relearner, so this
   cannot happen in the games. PKHeX states the rule (`LearnGroup1.FlagEvolutionSlots`) but
   never applies it to PK1, so no independent implementation has checked ours.
2. **Mew and Celebi from an unrecorded distribution.** The GB-era event list is not known
   to be complete, and Red/Blue/Yellow's Mew glitch (not modelled by PKHeX) catches a Mew
   with the player's own trainer data. Matching nothing in our tables is therefore not
   proof. Rules that hold whatever the event was (Mew's catch rate, stat experience, PP,
   trainer name and nickname length) still make such a record Invalid.

## Evidence

`tools/pkhex_gb_diff/run.sh` builds a corpus of raw party records, runs them through
PKHeX.Core and through `GBLegality`, and fails on any VALID/INVALID disagreement, on an
UNCERTAIN verdict without one of the two reasons above, or on a corpus record either
side skipped. The corpora contain records built the way the games build them, plus
deliberate defects (moves, levels, catch rate, types, held items, met data, PP,
egg data, stat experience, Pokérus, over-long or empty names) and random records.
About a quarter of the generated records are Japanese: 6-byte name fields encoded
with the Japanese character tables read from PKHeX's own `StringConverter1/2`
source, Japanese Blue encounters, Japanese in-game trade names, Japanese Odd Eggs
and the Japanese-only Gen II event gifts.

Seeds 1–3, 50,000 records each:

| Format | Decided records, agreement with PKHeX | Uncertain |
|---|---|---|
| PK1 | 149,860 / 149,860 | 140: Mew from an unrecorded event (PKHeX INVALID) |
| PK2 | 149,935 / 149,935 | 65: 34 Mew and 31 Celebi from an unrecorded event (PKHeX INVALID) |

The evolution-order case does not come up in these three seeds. An earlier corpus
without Japanese records produced one (an Exeggutor that knows Stomp and
PoisonPowder, PKHeX VALID). That record is kept as a fixed case in
`tests/test_gb_legality_gen1.cpp`.

`tests/test_gb_legality_gen1.cpp` (through `Legality::analyze`) and
`tests/test_gb_legality_gen2.cpp` keep PKHeX-verdict samples of every corpus kind
in the ordinary host suite, including the uncertain cases above.

Deliberate differences from PKHeX:

- A PK1's level must match its EXP. The games recompute level from EXP in battle,
  with Rare Candy and at the Day Care; PKHeX does not compare them for Gen I.
- The two uncertain cases above are Incomplete instead of PKHeX's verdict.

## Not covered yet

- Korean Gold/Silver and the non-English European languages: the corpora use
  English and Japanese text only.
- Gen II in the app: waits on independent verification of the Gen I/II code.
