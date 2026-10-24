# Re-roll mechanics, Generation I–V (owner item E11)

What a "Random DVs" (Gen I/II) or "Reroll IVs/EVs" (Gen III–V) press, and the Shiny / Gender /
Nature / Ability edits that re-derive a PID, can and cannot do in each generation. Every rule
below is read from the pinned sources, not invented:

- pret `pokered` / `pokecrystal` disassemblies (Gen I/II): `CheckShininess`, `GetGender`,
  `GetUnownLetter`, `CalcMonStats`.
- pret `pokeemerald` (Gen III): `IsShinyOtIdPersonality`, `GetNature`,
  `GetGenderFromSpeciesAndPersonality`, `GET_UNOWN_LETTER`, `CalculateMonStats`,
  `GetEvolutionTargetSpecies` (`EVO_LEVEL_SILCOON` / `EVO_LEVEL_CASCOON`).
- PKHeX `6501f0ab` (GPL-3.0, cited, never copied): `ShinyUtil`, `EntityGender.GetFromPID`,
  `PKM.PIDAbility`, `EntityPID.GetUnownForm3`, `WurmpleUtil`, `PK4` / `PK5` layouts,
  `EffortValues.Max510`.

Code under this document: `include/UI/Gen12DVReroll.h`, `include/UI/SharedStatReroll.h`,
`include/Conversion/Gen3PidSearch.h`, `include/Integration/Gen1/Gen1Shiny.h`,
`include/Integration/Gen5/Gen5PidCoordination.h`, `src/Pokemon/Pokemon3FRLG.cpp`
(`rerollPID` / `rerollPreservingShiny`), `src/Pokemon/Pokemon4Mutable.cpp` (`rerollPid`).
Test: `tests/test_reroll_mechanics_gen15.cpp` (100+ re-rolls per shiny and non-shiny example in
every generation, through the real record code).

## The owner's question: can a shiny and a non-shiny have the same stats?

**Generation I and II: no.** Shininess *is* a stat pattern. A Gen II Pokemon is shiny exactly
when its Defense, Speed and Special DVs are 10 and its Attack DV is one of 2, 3, 6, 7, 10, 11,
14, 15 (`CheckShininess`). Those DVs also feed the stat formula, so every shiny of a species at
a given level has one of only eight DV spreads, and a non-shiny can never share one of them.
The HP DV packs the low bit of each of the four, Attack highest (pokecrystal
`CalcMonStatC`). Defense, Speed and Special are 10 (1010b, low bit 0) for a shiny, so a shiny's
HP DV is 8 when its Attack DV is odd (3, 7, 11, 15) and 0 when it is even (2, 6, 10, 14). A Gen I Pokemon stores no shiny flag at all; it is shown
shiny after Time Capsule when its DVs match that same rule, and the editor applies the rule to
Gen I DVs for exactly that reason (`Gen1Shiny.h`).

**Generation III, IV and V: yes, and it is correct.** Shininess is decided by the Personality
value and the trainer: `(TID ^ SID ^ PID.high16 ^ PID.low16) < 8` (`IsShinyOtIdPersonality`;
PKHeX `ShinyUtil`). IVs live in their own 32-bit word (five bits each) and EVs in six bytes,
separate from the PID (PK3 IV32 / PK4 and PK5 IV32 at 0x38; `SharedStatReroll.h` cites the
offsets). Two Pokemon with identical IVs, EVs, level and nature have identical stats whether
or not either is shiny. There is no "certain amount of points" a shiny must have: a 0-IV shiny
and a 31-IV non-shiny are both ordinary, format-valid records, and so are two with the exact
same stats. The test applies the very same IV/EV sets to a shiny and a non-shiny record in each
of Gen III, IV and V and checks both stay what they were.

So the owner's expectation is right for Gen I/II and not for Gen III–V. The app's Gen III–V
"Reroll IVs/EVs" changing stats without touching Shiny is the format behaving as designed, not
a defect; the Gen I/II "Random DVs" keeping a shiny on one of its eight spreads is the same
rule from the other side.

## Generation I

- Stored: four DVs (Attack, Defense, Speed, Special), 0–15; HP DV derived. No gender, no
  shiny flag, no Unown.
- Shiny presentation after Time Capsule: the Gen II rule on those DVs.
- Random DVs: re-rolls all four; a shiny stays on one of the eight shiny spreads (only the
  Attack DV can move), a non-shiny never lands on one. Stat EXP is untouched; stats follow
  `CalcMonStats` from the new DVs.
- Possible: everything above. Not possible: a shiny with any other Defense / Speed / Special
  DV, or a shiny with an odd Attack-bit-1 pattern (Attack 0, 1, 4, 5, 8, 9, 12, 13).

## Generation II

- Shiny: the DV pattern above (`CheckShininess`).
- Gender: the Attack DV against the species ratio (`GetGender`; female when Attack DV ≤
  ratio >> 4, so 1:7-male species are female only on Attack 0–1, 3:1 on 0–3, 1:1 on 0–7,
  7:1-female on 0–11). Fixed-ratio species ignore the DV.
- Unown letter: bits 1–2 of each DV, `((Atk & 6) << 5 | (Def & 6) << 3 | (Spd & 6) << 1 |
  (Spc & 6) >> 1) / 10` (`GetUnownLetter`).
- Random DVs keeps the shiny state, the gender and the Unown letter. When no other DV set
  satisfies all three, the editor reports it and changes nothing.
- Possible and impossible, by rule, not by the app:
  - A shiny female of a 1:7-male species (Eevee, starters, Snorlax, …) **cannot exist**: every
    shiny Attack DV is ≥ 2 and female needs 0 or 1.
  - A shiny female of a 3:1 species has Attack 2 or 3 only; 1:1 has 2, 3, 6, 7; 7:1-female has
    2, 3, 6, 7, 10, 11.
  - A shiny male of a 7:1-female species has Attack 14 or 15 only.
  - A shiny Unown is always **I** or **V** (Attack 2, 3, 10, 11 → I; 6, 7, 14, 15 → V), because
    the other three DVs are pinned at 10.

## Generation III

PID-derived and re-derived together by every Shiny / Gender / Nature / Ability edit
(`Pokemon3FRLG::rerollPID`):

- Nature `PID % 25`; gender `(PID & 0xFF) < ratio`; ability slot `PID bit 0` (only pinned for
  species with two different abilities, as PKHeX's `AbilityVerifier` asks); shiny
  `TID ^ SID ^ high ^ low < 8`.
- Unown letter: four 2-bit PID fields, `% 28` (`GET_UNOWN_LETTER`). Always preserved;
  `setUnownLetter` is the only caller that changes it.
- Wurmple line (265–269): `(PID >> 16) % 10 < 5` → Silcoon, else Cascoon
  (`EVO_LEVEL_SILCOON` / `EVO_LEVEL_CASCOON`; PKHeX `WurmpleUtil`). Preserved across every
  trait edit so a Silcoon or Cascoon record never contradicts its own PID.
- IVs/EVs (Y reroll) are independent of the PID: IV 0–31 each, EV 0–255 each, total ≤ 510.
  Stats follow `CalculateMonStats` (party records store them; box records compute on load).
  Moves, PP and PP Ups are never touched by any re-roll.

How the PID is found: it is **constructed**, not filtered. The low half is drawn inside the
wanted gender's byte range with the ability bit (and the Unown letter bits) already set; a
shiny high half is `low ^ TSV ^ k` with `k < 8`. Nature and the Wurmple value are the only
retries. The search therefore fails only when the format has no such PID, and then the
record is left exactly as it was. Shininess never yields to another edit. A request the
record already satisfies (Shiny on a shiny, the current nature) changes nothing.

Possible and impossible:

- Every nature × gender × ability × shiny combination a species can have is reachable from
  any record, including the former worst case (shiny female of an 87.5%-male dual-ability
  species such as Snorlax changing nature).
- A shiny Unown can only show the letters its trainer's TSV allows: with `high = low ^ TSV ^
  k`, PID bits 24–25 are forced to `(bits 8–9) ^ (TSV bits 8–9)`, so some letters have no
  shiny PID for a given TID/SID. The editor refuses those letters (record unchanged) instead
  of searching forever; the test enumerates every shiny PID for the fixture TSV and checks
  the editor reaches exactly that letter set.
- Fixed-gender species accept only their own gender.

Previously (before this phase) the search walked a linear-congruential sequence and kept the
first PID that happened to fit, with a budget of 8 million steps; the hardest real cases
exhausted it about one time in ten and `rerollPreservingShiny` then retried **without** the
shiny constraint, so a nature edit could quietly un-shiny the Pokemon. That retry is gone.

## Generation IV

- Same PID links as Gen III for nature, gender, ability slot (bit 0), shininess and the
  Wurmple line (PKHeX `PKM.PIDAbility` for Gen III/IV origins). The Unown letter is a stored
  form (byte 0x40 bits 3–7), **not** PID-derived, so it is untouched by PID edits.
- The stored gender bits (0x40 bits 1–2) are rewritten to match the new PID; the test reads
  them back through `Pokemon4ReadOnly` and checks they agree.
- `Pokemon4Mutable::rerollPid` uses the same construction; a satisfied request keeps the PID
  (so a TID edit that does not change shininess no longer rewrites the PID). Previously the
  same filtered walk refused about one edit in five on the Snorlax case.
- IVs/EVs separate from the PID (IV32 at 0x38); party stats follow the Gen IV formula (same
  as Gen III) and are refreshed after every IV/EV or nature change; PP untouched.

## Generation V

- Shiny `TID ^ SID ^ high ^ low < 8` (PKHeX PK5 PSV/TSV). Nature is a **stored byte** (0x41),
  gender is the PID low byte with the stored bits kept in step, the normal ability slot is
  PID bit 16 for Gen V origins and the hidden-ability flag is byte 0x42 (PKHeX `PK5`,
  `PKM.PIDAbility`).
- `withShiny5` changes only PID bits 17–31 (making shiny: high = low ^ TID ^ SID with bit 16
  kept; making non-shiny: flip bit 19), so gender, ability, nature, IVs/EVs, PP and form stay.
  Deterministic: Keep and Review replay to identical bytes. `keepShinyForIds5` restores the
  previous shiny state after a TID / SID edit.
- A record whose origin is Gen III/IV (`earlierGenerationOrigin5`) still carries the old
  PID → nature / ability links; the Gen V editor refuses PID edits on it (`Gen5StagedPokemonRecord`)
  rather than break them. The test checks the refusal leaves the PID untouched.
- Known limit: the Wurmple-line evolution value (`(PID >> 16) % 10`) is **not** preserved by
  the Gen V shiny toggle; a single-bit change cannot always keep it (223 of 65 536 high halves
  have no such bit), and a wider change was not made in this phase. It only matters to
  legality of a Silcoon / Cascoon / Beautifly / Dustox record, which is PR #314's area.

## What no re-roll claims

Structural validity only. A PID that satisfies these links is a well-formed record; whether
such a Pokemon could be encountered (Method-1 IV/PID correlation, event PIDs, RNG frames)
is legality, handled elsewhere (PR #314) and never asserted by the reroll code or this test.
