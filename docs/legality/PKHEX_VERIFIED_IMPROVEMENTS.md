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
- Colosseum / XD-origin records, PKHeX "fishy" Gen III results and the capsule-location Celebi /
  beasts remain Incomplete; see the inventory's "Still Incomplete".
