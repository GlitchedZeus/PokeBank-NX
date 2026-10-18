# PKHeX verified improvements and differences

Each entry names a place where PokeBank NX's verdict differs from, or was brought in line with,
PKHeX.Core at `6501f0ab46e8f8ca048539dbaf8cae8cb104e722`, and classifies it:

- **Verified improvement**: PokeBank NX is right and PKHeX is not, with original-game evidence.
- **Verified parity**: a difference that existed was removed, with the PKHeX code path and the
  measurement that showed it.
- **Unresolved difference**: the two disagree and the evidence does not settle it; the record stays
  a Warning (Incomplete), never Invalid or "No problems found".
- **Unsupported claim**: a difference that was believed to be an improvement but has no primary
  evidence; it is withdrawn or kept only as a Warning.

Line numbers are of the pinned PKHeX checkout and of this branch.


Only entries classified **Verified improvement** may be described as better than PKHeX. Rates quoted
here are measured on the differential corpora and are not completeness figures.

## Gen I-II (PK1, PK2)

Pins: pret/pokered `af519899719f0754965776faac0e836a3b906e6d`, pret/pokeyellow
`e89ead154b9968aa50eed9328ff2b38b6c194382`, pret/pokegold `ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8`,
pret/pokecrystal `3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6`.

### D-1. A stored level ahead of its EXP is impossible; EXP ahead of the level is a Day Care slot (Verified improvement)

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

### D-2. Japanese Crystal's GS Ball Celebi is a cartridge encounter (Verified improvement)

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

### D-3. The in-game-trade trainer marker is followed by the terminator (Verified improvement)

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

## Gen III

### Colosseum/XD origins judged in the Gen III analyzer (Verified parity)

- **Before.** A PK3 with version 15 got the Warning "Colosseum/XD origin; Gen III cartridge
  rules cannot verify this Pokemon" and the older GameCube evidence in `Legality.cpp`; it
  could never be certified and a relabelled GBA record was never Invalid.
- **Now.** `src/Legality/Gen3Legality.cpp` runs PKHeX's `EncounterGenerator3GC` path (shadow,
  starter, gift, trade, Poke Spot and e-Reader templates, team locks, name-screen trainer IDs,
  CXDVerifier, fateful, National/Earth Ribbon, ball, ability rules; file:line in
  `GEN3_PKHEX_RULE_INVENTORY.md` section 7) on a version-15 PK3 held in any GBA save.
- **Witnesses.** PKHeX's own test records converted to PK3 (hex in
  `tests/test_legality_gen3.cpp`, rows `pkhex-*`): Vaporeon split evolution VALID / certified;
  Shuckle `.xk3` without fateful VALID / certified; Italian HO-OH long OT VALID / certified;
  Shuckle `.ck3` without fateful INVALID / Invalid; Ultra Ball Teddiursa INVALID / Invalid;
  Tyranitar missing Winning Ribbon Fishy / Incomplete, with it INVALID / Invalid. Oracle-built
  records: 3914 (seed 1), 3928 (seed 2), 3908 (seed 3) GameCube records in five cartridges,
  0 false Invalid, 0 false Valid, 100% of PKHeX-invalid caught, 97.2-97.7% of PKHeX-valid
  certified (the rest Fishy in PKHeX too).

### Team lock search terminates (Verified parity with a guard; see PKHEX_DISAGREEMENT_REVIEW.md 15-A, 16-A)

- `TeamLockResult.GetSingleLock` has a branch that never advances (`TeamLockResult.cs:118-123`);
  the port advances, and every GameCube RNG search is budgeted (`CxdGuard`), reporting Unknown
  at the cap instead of hanging. No corpus record reached a cap. This is not a stricter rule:
  a cap demotes Invalid findings to Warnings. Classified parity because PKHeX never enters the
  branch on the published templates; the improvement is robustness, not a new verdict.

### Oracle coverage: GameCube records in every GBA cartridge (Verified parity)

- `tools/pkhex_gen34_diff/oracle/Program.cs` builds each Colosseum/XD template as CK3/XK3 ->
  PK3 with a name-screen-valid TID/SID (`CxdTrainer`) once per holding cartridge, plus
  mutations and growth. Before, the Gen III corpora had no version-15 record at all, so the
  GameCube path had no oracle measurement.

### Unresolved differences (none promoted)

- e-Reader shadows (Japanese bonus disc) carry fixed all-zero IVs; PKHeX grades them Fishy
  (IVAllEqual_0) and so do we (Incomplete). A primary source for the fixed IVs (the disc's
  data) would allow certification; none is available in this environment.
- English XD TID/SID without a name-screen seed: PKHeX Fishy (PAL copies skip the ball
  sequence, `TrainerIDVerifier.cs:86-101`); kept as a Warning. No decompilation exists to
  decide the PAL behaviour independently.

## Gen IV (Diamond, Pearl, Platinum, HeartGold, SoulSilver)


### IV-1. Pt/HG/SS write both met-location fields; a disagreeing pair was never written by a game (Verified improvement)

- **Rule.** A native (origin 7, 8, 10, 11, 12) non-egg PK4 whose Pt/HG/SS met-location field (0x46)
  is set and whose egg-location fields (0x44, 0x7E) are 0 must hold in its D/P met-location field
  (0x80) what Pt/HG/SS write for 0x46: the same value when D/P know the place (1-111, 2000-2010,
  3000-3076), Faraway Place (3002) otherwise. `src/Legality/Gen4Legality.cpp` `Analyzer::verifyMisc`
  (:1352-1371). Full write-path map: `PKHEX_DISAGREEMENT_REVIEW.md` item 14.
- **Primary source.** pokeplatinum `src/pokemon.c:1732-1740` (one `case` for
  `MON_DATA_MET_LOCATION` and `MON_DATA_MET_LOCATION_PTHGSS`: `sub_0201708C(value)` or value 0 puts
  the value into both fields, else 3002 into `MetLocation_DP` and the value into
  `MetLocation_PtHGSS`), `src/unk_02017038.c:38-45` (the D/P-known ranges); pokeheartgold
  `src/pokemon.c:1285-1294` with `LocationIsDiamondPearlCompatible`. Every met-location write in
  both decomps goes through this setter (`src/unk_02092494.c:440-590` strategies and their callers;
  `src/trainer_memo.c:703-840` and its callers), and D/P write only the D/P field.
- **Hex witness** (Gen III/IV differential, seed 1 base corpus, record `m6211-metlocext`, Pearl, a
  wild slot with 1 in the Pt/HG/SS field and 23 in the D/P field; PKHeX VALID, PokeBank NX Invalid
  "D/P met location field holds 23 where Pt/HG/SS write 1 for met location 1"):
  `6FDF3AB0000081FC3600000070912A2E093D0000460D0002000000000000000000000000000000002700370032005D001E19141900000000DA52D0170000000002000000000001003A013D0143012E013F012D013501FFFF000000000000000B00000000000000003E0149015701580149015601FFFF00000000001A0A1200001700000419070000`
- **Reproduction.** `tools/pkhex_gen34_diff/run.sh /home/user/ext/pkhex 1 3 4` lists the record
  under "DELIBERATELY STRICTER ... met-location-fields" (35 records in that run, all `metlocext`
  mutations; the predicate `met_location_fields` in `summarize.py` confirms the disagreement from the
  bytes). Unit test: `tests/test_legality_gen4.cpp` fixture `met-location-dp-field`; title matrix
  Gen IV rows (`tests/test_legality_title_matrix.cpp`). PKHeX's 29 `.pk4` test fixtures all satisfy
  the rule (fixture driver of this campaign).
- **PKHeX's verdict.** VALID: `G4PKM.cs:229-237` reads the Pt/HG/SS field whenever it is set and
  never compares the two outside Pal Park (`MiscVerifierG4.cs:35-45`).
- **Ours.** Invalid; Warning for a My Pokémon Ranch gift (the Ranch writes on the Wii, no decomp).

## Gen V (Black, White, Black 2, White 2; Poke Transfer records)

### Pokerus on an unhatched egg (Verified parity)

PokeBank NX called "Egg has Pokerus" Invalid. PKHeX accepts it: `Pokerus.IsObtainable`
(`PKHeX.Core/Editing/Pokerus.cs:21-27`) is true for every PK5, and `MiscPokerusVerifier`
(`Legality/Verifiers/Misc/MiscPokerusVerifier.cs:23-29`) checks the strain and the day count only,
with no egg case. No original-game evidence shows a Gen V egg cannot carry the virus (eggs in the
party are infected by neighbours in every game from Gen II on; there is no Gen V decompilation to
prove the contrary). The Gen V differential's new unhatched-egg corpus group (`egg`) showed the
rule as 57 false Invalids on seed 1; it was removed (`src/Legality/Gen5Legality.cpp`,
`Gen5Analyzer::eggState`). After the change: 0 false Invalid on seeds 1, 2, 7, 11, 13 and 17.

### Unhatched bred egg: move inheritance order (Verified parity)

PokeBank NX did not replay the inheritance order and left an unhatched bred egg's moves Partial
(Warning). PKHeX validates it strictly (`LearnVerifierRelearn.VerifyEggMoveset` →
`MoveBreed5.Validate`, `Legality/Moves/Breeding/MoveBreed5.cs:17-40`, base moves at `:115`, TM list
at `:152-153`, inheritance gate `Breeding.GetCanInheritMoves`, `Legality/Breeding.cs:54`). The port
is `Breed5::valid` (`src/Legality/Gen5Legality.cpp:714-803`), reached from `Gen5Analyzer::movePool`
for an unhatched egg matched to a bred template. Fixtures: `egg-b7730`, `egg-b11015`, `egg-b58750`,
`egg-b17360` (accepted), `egg-b12020`, `egg-b52180`, `egg-b28890`, `egg-m18178-move`,
`egg-m53276-move` (Invalid) in `tests/test_legality_gen5.cpp`. On the six seeds, 78,680
PKHeX-invalid and 31,475 PKHeX-valid unhatched eggs agree (4 PKHeX-invalid and 1 PKHeX-valid
stay Incomplete for unrelated reasons: word filter, Colosseum / XD).

### Kyurem fusion moves, Shedinja's ball, Combee deferral, evolution by move, boxed PP, Shaymin in a box, per-stage level caps (Verified parity)

Each was Missing or Partial in the inventory and is now ported with positive and negative
fixtures; see `GEN5_PKHEX_RULE_INVENTORY.md`, "Rules ported in this round", for the PKHeX
file:line, the PokeBank NX function and the fixtures. None is an improvement over PKHeX; each
removes a gap the differential could not see before the oracle gained the `egg`, `evomove` and
`kyurem` corpus groups.

### Colosseum/XD-origin transfers judged by the GameCube judge (Verified parity)

Pal Park (PK4) and Poke Transfer (PK5) records with version 15 now run the Gen III GameCube judge
(`Gen4::judgeCxdOrigin`, `src/Legality/Gen4LegalityPalPark.cpp`). The differential found each
place the transfer changes what PKHeX compares, and each now follows PKHeX:

- The Gen III level is bounded by the met level, the level at transfer (`EvolutionGroup3.Devolve`
  for format 4, `EvolutionGroup4.Devolve` for format 5).
- A two-ability Gen III species keeps its GameCube ability bit apart from the PID
  (`AbilityVerifier.cs:273-274`), in format 4 and format 5.
- Spanish Duking's Pokemon read "GERMいN" after Pal Park (`Encounters3Colo.TrainerNameDukingSpanish4`,
  used by `EncounterTrade3XD.IsTrainerMatch` and `EncounterGift3Colo.IsTrainerMatch`).
- Templates that differ only in the met location the transfer lost (Colosseum Shadow Suicune with
  Surf or with Hydro Pump) are tried in order, as PKHeX defers to the next encounter when the
  moves do not fit.
- A Pokemon that evolved after the transfer is matched on its Gen III pre-evolutions
  (`EvoChainsAllGens.Gen3`).

Fixtures: `transfer-cxd`, `transfer-cxd-ability`, `transfer-cxd-suicune-hydro`,
`transfer-xd-duking-spanish`, `transfer-xd-evolved-later` (`tests/test_legality_gen5.cpp`) and the
Pal Park Colosseum/XD rows of `tests/test_legality_title_matrix.cpp`.

### A Gen III Pokemon that evolved after its transfer keeps its only Gen III ability against the PID (Unresolved difference)

Example: a Shadow Pidgeotto snagged in XD, transferred at level 30 and then evolved into Pidgeot,
holding Keen Eye with PID bit 0 set (`b121630`, seed 1). PKHeX accepts it: `EvolutionGroup3.Evolve`
leaves the unreached Pidgeot stage in the Gen III history, so `VerifyAbilityGen3Transfer` sees
Pidgeot's single Gen III ability and allows the mismatch. The Gen IV games recalculate the ability
from the PID when a Pokemon evolves (pokeplatinum `src/evolution.c:436` calls `Pokemon_CalcAbility`,
`src/pokemon.c:4746-4764`), so after a Gen IV evolution the record should hold Tangled Feet. There is
no Gen V decompilation to show the same for an evolution in Black/White. PokeBank NX reports a
Warning ("PKHeX accepts it"), so the record is Incomplete, never Invalid. Evidence that would settle
it: the Gen V evolution routine, or a captured Pokemon of this history.

### PGF shiny PID that also fits a Gen III event pattern (Unresolved difference)

PKHeX's `MiscFatefulEncounterVerifier` accepts a card's shiny PID only when `MethodFinder.Analyze`
returns `G5MGShiny`, and `MethodFinder` tries the Gen III event searches (BACD and variants) first.
Their 16-bit comparisons let a real event Pokemon hit one by chance (about 1 in 65,536), and PKHeX
then reports `PIDTypeMismatch`. PokeBank NX reports a Warning (PID/RNG coverage Partial). The six
seeds hold 160 such PKHeX-invalid records (`iv` mutations of shiny cards), all left Incomplete here.
Settling it needs the Gen V shiny-PID generation of the Mystery Gift receiver (no Gen V
decompilation exists) or a captured event Pokemon whose PID fits a BACD pattern.

### Text buffer length (Unresolved difference, kept as before)

A nickname over 10 or an OT over 7 UTF-16 units before the 0xFFFF terminator is Invalid here;
PKHeX reads to the end of the buffer and skips the OT check for fixed-trainer templates
(`PKHEX_DISAGREEMENT_REVIEW.md` item 12). No PKHeX-built record reaches it on any seed. A real
record that reaches it would be the evidence to revisit.

### MoveBreed5 TM list drops HM06 Dive (quirk (c); parity kept, Unresolved difference)

`MoveBreed5.Validate` builds its inheritable TM list from `PersonalInfo5BW.MachineMoves[..^1]`
(`MoveBreed5.cs:153`); the comment says it drops TM96 Snarl, but the last entry of that table is
HM06 Dive (291), so an unhatched egg whose father passed Dive is Invalid in PKHeX. PokeBank NX
keeps parity (`Breed5::valid` uses the same list) because the differential's `egg` group would
otherwise report false Valids, and because no primary source settles whether a Gen V egg can
inherit an HM move. Evidence needed: the Gen V daycare egg-move code, or a captured unhatched egg
with Dive. Until then a real such egg would be a false Invalid in both programs; it is recorded
here so it is not mistaken for a verified rule.

### Items not claimed as improvements

- Hidden-ability flag on a fateful Gen IV origin, the Wurmple branch and Mothim's forms are
  reviewed in `PKHEX_DISAGREEMENT_REVIEW.md` items 15-C, 16-C and 17-C. None is an improvement
  over PKHeX; two are Warnings where PokeBank NX used to be stricter without evidence, and one is
  the same acceptance PKHeX has.
- PKHeX "fishy" Gen III results, an undecided Colosseum / XD judge answer and the
  capsule-location Celebi / beasts remain Incomplete; see the inventory's "Still Incomplete".
- Blue-Striped Basculin without its hidden ability matched to a B2/W2 Dream World encounter is a
  Warning, as `AbilityVerifier.IsEdgeCaseBasculin` accepts it (`PKHEX_DISAGREEMENT_REVIEW.md` item 18).
