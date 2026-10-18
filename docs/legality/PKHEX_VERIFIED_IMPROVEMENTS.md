# Verified improvements over PKHeX, Generations I–V

Each entry is a place where PokeBank NX decides a record PKHeX decides differently, **and**
the original game (a pret disassembly or a decomp) shows PokeBank NX is right. An entry
needs: the games, the PKHeX code (file:line at kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722),
the game evidence (file:line), what PokeBank NX now reports, the tests, and the corpus
class that keeps the difference accounted for. A difference that rests on a secondary
source or on an undisassembled screen is not an improvement; it is listed in
`PKHEX_DISAGREEMENT_REVIEW.md` as a downgrade to Incomplete.

Each generation's lane appends under its own heading. Entries are numbered within the
heading.

## Gen I / Gen II (PK1, PK2) — Lane D

Pins: pret/pokered `af519899719f0754965776faac0e836a3b906e6d`, pret/pokeyellow
`e89ead154b9968aa50eed9328ff2b38b6c194382`, pret/pokegold `ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8`,
pret/pokecrystal `3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6`.

### D-1. A stored level ahead of its EXP is impossible; EXP ahead of the level is a Day Care slot

- **Games.** Red/Blue/Yellow, Gold/Silver/Crystal; PK1 and PK2.
- **PKHeX.** `LevelVerifier.Verify` (Legality/Verifiers/LevelVerifier.cs:45-51) rejects EXP
  only when the current level is 100 and EXP is not the level-100 total
  (`LevelEXPTooHigh`); at any other level it derives the level from EXP and never compares
  the two. A level-50 record with 0 EXP is VALID to PKHeX; a level-5 record with level-30
  EXP is VALID too.
- **Evidence.** Battle EXP raises the level as thresholds are crossed and stops at the
  level-100 total (pokered `engine/battle/experience.asm`, pokecrystal
  `engine/battle/core.asm`); Rare Candy refuses level 100 and sets EXP to the new level's
  total (`engine/items/item_effects.asm`). The Day Care is the one place EXP and level part:
  Gen I adds one EXP per step with no level check and holds the top byte at `$50`
  (pokered `engine/overworld/daycare_exp.asm:1-16`); Gen II skips a level-100 Pokémon and
  caps the EXP at `MAX_DAY_CARE_EXP = $500000` (pokecrystal
  `engine/events/happiness_egg.asm:142-190` `DayCareStep`, `constants/misc_constants.asm:52`).
  On withdrawal both recompute the level from EXP (pokered `scripts/Daycare.asm:69-83`,
  pokecrystal `engine/pokemon/move_mon.asm:805` `RetrieveBreedmon`). So the stored level is
  never *ahead* of the EXP; it can be *behind* it only in a Day Care slot, where the EXP can
  also pass the level-100 total up to `$50FFFF`.
- **PokeBank NX.** `GB::levelExpProblem` (src/Legality/GBLegality.cpp): level ahead of EXP
  → Invalid; level behind EXP → Incomplete ("only a Pokemon still in the Day Care carries a
  stale level"); EXP above the level-100 total → Incomplete up to `$50FFFF` when the Day Care
  could have written it (any level on a PK1, level < 100 on a PK2), Invalid beyond that or
  at level 100 on a PK2. The Invalid half is the improvement; the Incomplete half is the
  review item 15-D.
- **Tests.** tests/test_gb_legality_structural.cpp `levelExpGrid` (four growth rates, the
  boundaries 0x50FFFF / 0x510000 / 0xFFFFFF), `realRedSave` (a checksum-valid Red save
  through `Legality::analyze`), `productionPk2`; tests/test_gb_legality_gen1.cpp
  `level-exp-ahead`, `level-exp-daycare-stale`, `level-100-exp-daycare`,
  `level-100-exp-beyond-daycare`.
- **Corpus.** `tools/pkhex_gb_diff/compare.py` class `level-exp` (ours INVALID, PKHeX VALID;
  only when the stored level is ahead of the EXP) and reasons `daycare-stale-level`,
  `daycare-exp-over-cap`; corpus kinds `struct*-exp-below`, `struct*-exp-next`,
  `struct*-exp-cap-minus1`, `exp-daycare-*`, the level defects.

### D-2. Japanese Crystal's GS Ball Celebi is a cartridge encounter

- **Games.** Japanese Crystal; PK2.
- **PKHeX.** `Encounters2.CelebiVC` (Legality/Encounters/Data/Gen2/Encounters2.cs:120) models
  the Ilex Forest Celebi for the Virtual Console release only; a GB-era Japanese Celebi with
  the shrine's met data is INVALID (`EncInvalid`, `FatefulGiftMissing`).
- **Evidence.** pokecrystal `maps/IlexForest.asm` (the shrine script summons a wild level-30
  Celebi when the GS Ball is held); the Mobile System GB distribution of the GS Ball
  (27 January 2001 – 14 December 2002; Serebii, Bulbapedia). Western cartridges only set the
  flag through the Virtual Console hook (`engine/menus/save.asm` `sGSBallFlag`), which is why
  the rule is Japanese-only.
- **PokeBank NX.** `gen2Candidates` adds the GS Ball Celebi for a Japanese PK2 with
  Ilex Forest met data at level 30 (Legal); wrong place, level or language keeps PKHeX's
  verdict.
- **Tests.** tests/test_gb_legality_gen2.cpp `gsball2-*` fixtures.
- **Corpus.** compare.py class `gs-ball-celebi` (ours VALID, PKHeX INVALID), kinds
  `gsball2-caught`, `gsball2-grown`.

### D-3. The in-game-trade trainer marker is followed by the terminator

- **Games.** Red/Blue/Yellow (the marker travels into Gold/Silver/Crystal); PK1 and PK2.
- **PKHeX.** `EncounterTrade1.IsTrainerNameValid` (Legality/Encounters/Templates/Gen1/EncounterTrade1.cs:70-80)
  requires `[0x5D, 0x50, ...]`; a record whose marker is followed by a letter matches no
  trade and is then INVALID through `G1CharOT`. PKHeX agrees; the improvement is that the
  rule is now reachable for the two byte decoders here (see the hand-off), where the decoded
  "*" alone could not tell the cases apart.
- **Evidence.** pokered `constants/charmap.asm:25` (`<TRAINER>` = `$5d`), `macros/data.asm:85`
  (`dname` pads with `@` = `$50`), `engine/events/in_game_trades.asm`, pokecrystal
  `data/events/npc_trades.asm` (`dname \9, NAME_LENGTH`).
- **PokeBank NX.** `GB::rawNameProblems` → Invalid ("the games write <TRAINER>@").
- **Tests.** tests/test_gb_legality_names.cpp `tradeMarker`.
- **Corpus.** kinds `trade-ot-bad-term`, `defect2-trade-ot-bad-term` (matching invalid).
