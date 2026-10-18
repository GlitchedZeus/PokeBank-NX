# PKHeX disagreement review, Generations I–V

The owner asked for an independent review of every place where PokeBank NX deliberately
disagrees with PKHeX, with one rule: **a rule needs original-game evidence before
PokeBank NX may call a Pokémon definitively Invalid.** This document reviews each
disagreement against that rule. Where the evidence is missing, the rule is downgraded
to a Warning (Incomplete), and the code, a test and the differential tools change with it.

## What was read

| | Pin |
|---|---|
| PokeBank NX base | `7356b3835d2d97eed270cae2d86300ece9b55f26` (branch `claude/gen1-5-legality-campaign-w9mkom`). Line numbers below are from this commit, except `Gen5Legality.cpp`, which is quoted after the change in this review. |
| PKHeX.Core | `kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722`, read-only at `/home/user/ext/pkhex`. History from a blobless clone of `kwsch/PKHeX` at that commit. |
| pret pokered | `af519899719f0754965776faac0e836a3b906e6d` |
| pret pokeyellow | `e89ead154b9968aa50eed9328ff2b38b6c194382` |
| pret pokegold | `ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8` |
| pret pokecrystal | `3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6` |
| pret pokeplatinum | `1dc13dcea40c04ab53f1f607de46023dc409ef20` |
| pret pokeheartgold | `d2355244ac42b3c6e40d8c36aa3564ee8706dea4` |

Gen V has no decompilation. `pret/pokeblack`, `pret/pokeblack2` and `pret/pokewhite` do
not exist as public repositories (`git ls-remote` asks for credentials), and a web search
found no pret project for Black/White; pret's DS projects are Platinum and
HeartGold/SoulSilver. Bulbapedia and Project Pokémon could not be reached
from this environment: the proxy refused them (HTTP 403) and the fetch tool could not
resolve them. GitHub's issue API for `kwsch/PKHeX` was also refused. The only Gen V
sources available were PKHeX's code and its git history, and they are secondary.

The disagreements were found by searching the code (`PKHeX` in comments, every
`Severity::Invalid` site in `Legality.cpp`, every non-definitive `GB::Problem`), and by
reading the allow-lists of the three PKHeX differentials:

- `tools/pkhex_gb_diff/compare.py`: the `DELIBERATE` and `UNCERTAIN_REASONS` lists;
- `tools/pkhex_gen5_diff/run.sh`: the `stricter` allow-list, removed in this review;
- `tools/pkhex_gen34_diff`: no allow-list at all at the time. Item 14 (added later) gave it one
  entry, `DELIBERATE` in `summarize.py`, checked by a predicate over the record bytes.

## Summary

Direction words:

- **stricter**: PokeBank NX says Invalid where PKHeX says VALID.
- **more lenient**: PokeBank NX says No problems, or only Warning/Incomplete, where
  PKHeX says INVALID.
- **warns more**: PokeBank NX says Incomplete where PKHeX says VALID. This is not Invalid.

| # | Disagreement | PokeBank NX code | Direction | PKHeX | Original-game evidence | Recommendation | Changed |
|---|---|---|---|---|---|---|---|
| 1 | GB: the stored level must be the level the EXP gives (PK1, PK2) | `GBLegality.cpp:1936-1945`, `:1947-1958`, `:1960-1982` | stricter | `PKM.cs:389`, `LevelVerifier.cs:46-59` | **Found.** pokered, pokecrystal: every path that writes EXP also writes the level (§1) | Keep | No |
| 2 | GB: GS Ball Celebi in Japanese Crystal | `GBLegality.cpp:1226-1247` | more lenient | `Encounters2.cs:120`, `EncounterEnumerator2.cs:167-178` | pokecrystal `maps/IlexForest.asm:466`. The Japanese Mobile System GB distribution is secondary | Keep | No |
| 3 | GB: no word filter on nicknames | none (no filter) | more lenient | `NicknameVerifier.cs:68-74`, `WordFilterSettings.cs:14` | No filter in pokered or pokecrystal `engine/menus/naming_screen.asm` | Keep | No |
| 4 | Gen I: order of moves learned across an evolution | `GBLegality.cpp:710-722`, `:1638-1643` (Warning) | warns more | `LearnGroup1.cs:15-21`, `:35-36` (never runs for PK1) | Partial: pokered has no move relearner. The Stadium 2 reminder is known only from PKHeX and Bulbapedia | Keep (Warning) | No |
| 5 | GB: Mew or Celebi from an unrecorded distribution | `GBLegality.cpp:1815-1826` (Warning) | more lenient | `EncounterEnumerator1.cs:122-131` (`Encounters1GBEra.Gifts`), `Encounters2GBEra` | Event list completeness unproven. The Mew glitch is secondary (Bulbapedia) | Keep (Warning) | No |
| 6 | Korean Gold/Silver: a record that only a Korean–Western trade explains | `GBLegality.cpp:2143-2166` (Warning) | more lenient (PKHeX INVALID) / warns more (PKHeX VALID) | `ParseSettings.cs:58,66` (no Crystal, no reminder for Korean), `:68` (Odd Egg allowed for Korean), `EncounterEnumerator2.cs:30,100,168`, `GBRestrictions.cs:129-130` | Secondary only (Bulbapedia, "Trade"). Hatching sets the trainer: pokegold `engine/pokemon/breeding.asm:297-310` | Keep (Warning) | No |
| 7 | Korean Gold/Silver: met data | `GBLegality.cpp:2079-2093` (Warning) | more lenient / warns more | `EncounterEnumerator2.cs:30` (ignores met data) | pokegold `engine/pokemon/move_mon.asm:238-243,301-306` writes zero. pokecrystal `engine/pokemon/caught_data.asm:163-240` writes met data. The Korean ROM is not disassembled | Keep (Warning) | No |
| 8 | Korean trainer name with characters outside the Hangul pages | `GBLegality.cpp:2094-2111` (Warning) | more lenient | `TrainerNameVerifier.cs:151-157` (`G1CharOT`) | None (Korean naming screen not disassembled) | Keep (Warning) | No |
| 9 | GB: battle stat experience next to a zero stat | `GBLegality.cpp:1479-1503` (Warning) | more lenient | `EffortValueVerifier.cs:80-96`, which calls itself a "simple sanity check" | pokered `engine/battle/experience.asm:294-325` floors each base stat by the number of recipients, so a stat can gain 0 | Keep (Warning) | No |
| 10 | Gen IV: the stored party level must be the level the EXP gives | `Legality.cpp:2760-2772`, through `Pokemon4ReadOnlyView.cpp:30-34` | stricter | `PKM.cs:389`, `LevelVerifier.cs:46-59`. No legality code in PKHeX reads `Stat_Level` | **Found.** pokeplatinum, pokeheartgold: every write of a party Pokémon's EXP is followed by a level recalculation (§10) | Keep | No |
| 11 | Gen V: hidden-ability flag with a slot 1/2 ability on a hidden-ability-only template | `Gen5Legality.cpp:700-722` | was stricter, now warns more | `AbilityVerifier.cs:225-226` (VALID on `AbilityNumber == 4`) | **None found**, either way (§11) | **Downgrade to Warning** | **Yes** |
| 12 | Gen V: a nickname over 10 or an OT over 7 units (no 0xFFFF terminator in the buffer) is Invalid | `Gen5Legality.cpp`, `internal()` text-buffer rule | stricter only in PKHeX's exemptions | `Legal.cs:244-261` gives the same limits for Gen <= 5 (OT 7, nickname 10) and `TrainerNameVerifier.cs:49-53` / `NicknameVerifier.cs:179-184` flag longer names; PKHeX exempts fixed-trainer templates and traded eggs (`TrainerNameVerifier.cs:80-95`) and makes a foreign species-name length Fishy | **Format only.** The PK5 layout stores the OT in 8 and the nickname in 11 UTF-16 units including the terminator (PKHeX `PK5.cs`), so an exempt name still fits the limit; no record PKHeX builds reaches the difference (Gen V differential, 4 seeds) | Keep; revisit if a real record ever reaches it | No |
| 13 | Gen V: a capsule transfer location (30010-30013) on a non-fateful Gen IV Celebi or beast | `Gen5Legality.cpp`, transfer location rule | was stricter, now warns more | `TransferVerifier.VerifyTransferLegalityG4` accepts it; only `PK5.GetTransferMetLocation4` limits it to fateful records | **None found** (no Gen V decomp) (§13) | **Downgrade to Warning** | **Yes** |
| 14 | Gen IV: a native non-egg record whose D/P met-location field is not what Pt/HG/SS write for its Pt/HG/SS met location | `Gen4Legality.cpp`, `Analyzer::verifyMisc` | stricter | `G4PKM.cs:229-237` reads the Pt/HG/SS field first; `MiscVerifierG4.cs:35-45` compares the two only for Pal Park | **Found.** pokeplatinum, pokeheartgold: every met-location write goes through one setter that writes both fields (§14). Not for My Pokémon Ranch (Wii, no decomp) | Keep; Warning for Ranch records | New rule |

No other Invalid that PKHeX would not issue was found:

- **Gen III/IV.** The differential, run during this review (seed 1, 3 mutations, 79,847
  records, box format), has 0 records that PKHeX accepts and PokeBank NX calls Invalid.
- **Gen V.** The differential (seed 1, 4 mutations, 40,030 records) had exactly one such
  record, row 11. It has none after the change (§11 gives the numbers).
- **Gen I/II.** The differentials (`run.sh … 20000 1 1` and `… 2`) have 0 differing
  records. Their only class of records PKHeX accepts and PokeBank NX calls Invalid is
  row 1: 230 PK1 and 198 PK2 records. The rest of their allow-lists are rows 2 and 4–9,
  none of them Invalid.

Rows 1 and 10 are the only stricter rules left. Both rest on decompiled game code.
Row 14, added later, is a third; it rests on decompiled game code too.

## 1. GB level against EXP (keep)

PokeBank NX:

- `levelExpProblem` (`GBLegality.cpp:1936-1945`) reports "Level N does not match EXP".
- `formatProblems` (`:1947-1958` for PK1, `:1960-1982` for non-egg PK2) reports it as a
  definitive problem.

PKHeX derives the level from EXP (`PKM.cs:389`: `CurrentLevel => Experience.GetLevel(EXP, …)`).
`LevelVerifier.cs:46-59` only compares that derived level with the met level and the
level-100 EXP total. No file under `PKHeX.Core/Legality` reads `Stat_Level`, and the GB
stat calculation does the same (`GBPKM.cs:229`: "recalculate instead of checking
Stat_Level"). The EXP-above-total half of the rule agrees with PKHeX (`LevelEXPTooHigh`);
only the stored-level comparison is stricter.

Evidence that the cartridges keep the level and EXP in step:

- **Battle, Gen I.** pokered `engine/battle/experience.asm:119-142` caps EXP at the
  level-100 total. Lines 154-168 then set the party level from `CalcLevelFromExperience`.
- **Battle, Gen II.** pokecrystal `engine/battle/core.asm:7161-7187` caps EXP, and line
  7192 (`CalcLevel`) through line 7207 sets the level.
- **Rare Candy.** pokered `engine/items/item_effects.asm:1332-1356` and pokecrystal
  `engine/items/item_effects.asm:1277-1305` both refuse level 100. Otherwise they set the
  level to level+1 and the EXP to that level's total.
- **Gen I PC.**
  - Party to box: `_MoveMon` (pokered `engine/pokemon/add_mon.asm:416-427`) copies the
    party level into the box level byte.
  - Box or Day Care to party: lines 496-507 recompute the level from EXP.
  - Caught straight into a box: `SendNewMonToBox` (`engine/items/item_effects.asm:2774-2795`)
    writes the box level and the EXP of the same level.
- **Gen I Day Care.** pokered `scripts/Daycare.asm:62-92` computes the level from EXP,
  caps EXP at the level-100 total and stores that level. Withdrawal (line 168,
  `DAYCARE_TO_PARTY`) goes through the `_MoveMon` recompute.
- **Gen II Day Care.** `RetrieveMonFromDayCareMan/Lady` (pokecrystal
  `engine/pokemon/move_mon.asm:774-803`) take the level from `CalcLevel`
  (`engine/pokemon/breedmon_level_growth.asm:1-27`), which stops at 100
  (`engine/pokemon/experience.asm:1-10`). `RetrieveBreedmon` (`move_mon.asm:805-900`)
  writes that level and resets EXP to its total, which is the documented "might lose
  experience" bug.
- **Time Capsule, Gen I to II.** pokecrystal `engine/link/link.asm:1119-1126` copies the
  Gen I party level. It skips the Gen I box-level byte.
- **Evolution** changes the species, not the level or EXP. Every evolution keeps the
  growth rate:
  - 122 evolution entries in pokecrystal `data/pokemon/evos_attacks.asm`;
  - 72 entries in pokered `data/pokemon/evos_moves.asm`.

  Each was checked against `data/pokemon/base_stats/*.asm`: 0 mismatches.
- **Gen I to Gen II** keeps the growth rate too: all 151 species have the same growth rate
  in pokered and pokecrystal `data/pokemon/base_stats/*.asm`. A Time Capsule level that
  was right in Gen I is still right in Gen II.

The one place where a level and its EXP disagree is a Pokémon still in the Day Care:

- Gen I (pokered `scripts/Daycare.asm:206`, `.leaveMonInDayCare`);
- Gen II (stale `wBreedMon*Level`).

PokeBank NX never analyzes such a record:

- `Pokemon1ReadOnly` and `Pokemon2ReadOnly` are only built from party and box slots
  (`src/Legacy/RBYReadOnlyTrainer.cpp:125,145,186`, `src/Legacy/GSCReadOnlyTrainer.cpp:96,155,179`);
- the Gen I/II save readers do not read the Day Care.

Party records read level byte 0x21 and box records byte 0x03
(`src/Integration/Gen1/Gen1ReadOnlySave.cpp:255`), matching the game's own use above.

## 2. GS Ball Celebi (keep)

The candidate is a Japanese PK2 Celebi with Crystal met data: Ilex Forest, level 30.

- The shrine script is in the cartridge: pokecrystal `maps/IlexForest.asm:466`,
  `loadwildmon CELEBI, 30`.
- In the international ROM, only the Virtual Console hook sets `sGSBallFlag`
  (`engine/menus/save.asm:164-174`). `BattleTowerAction_GSBall`
  (`engine/events/battle_tower/battle_tower.asm:1179-1185`) reads that flag.
- That the flag was set in Japanese Crystal through the Mobile System GB
  (2001–2002) rests on secondary sources (Serebii, Bulbapedia). The Japanese ROM is not
  disassembled.

The difference is lenient and produces no Invalid, so the evidence rule does not apply.
Nothing changes.

## 3. Word filter (keep)

PKHeX applies its word filter to GB nicknames unless a setting turns it off
(`WordFilterSettings.cs:14`, `NicknameVerifier.cs:68-74`). The Game Boy naming screens
contain no filter:

- `grep -i "filter|profan"` finds nothing in pokered or pokecrystal
  `engine/menus/naming_screen.asm`;
- PKHeX's own `WordFilter` is a PKHeX setting.

Lenient. No change.

## 4–9. Non-definitive GB rules (keep as Warnings)

Each of these rules is already a Warning (`Problem::definitive == false`). A Warning
makes the report Incomplete with Partial encounter and event coverage
(`Legality.cpp:244-269`), and never Invalid. None of them is an Invalid without
evidence, so the owner's rule is already met. The evidence for each is in the summary
table.

Row 6 is worth a note: PokeBank NX "warns more" than PKHeX on Korean Odd Eggs and Korean
met data that PKHeX accepts. This is Incomplete, not Invalid.

`docs/legality/GB_LEGALITY_VERDICTS.md` lists rows 4–8 but not row 9.
`docs/legality/GEN1_5_CAMPAIGN_CHECKPOINT.md` lists row 9 under its known gaps.

## 10. Gen IV party level against EXP (keep)

This is a stricter rule that the documentation does not list. It is inherited from PKSE
(`25f248ab`, "chore: import PKSE 1.1.3 source"):

```cpp
if (pk.exp() > maxExp || (pk.level() != 0 && pk.level() != expLevel))
    add(r, Severity::Invalid, … "Level N does not match EXP (expected M)");
```

It is in `Legality.cpp:2760-2772`.

- **Gen IV.** `Pokemon4ReadOnlyView::level()` (`Pokemon4ReadOnlyView.cpp:30-34`) returns
  the stored party level of a party record. A Gen IV party record whose level differs
  from its EXP is therefore Invalid here, and VALID in PKHeX, which never reads
  `Stat_Level` in its legality code.
- **Gen III.** `Pokemon3FRLG::level()` derives the level from EXP
  (`Pokemon3FRLG.cpp:45-48`), so the rule never fires there.
- **The Gen III/IV differential** builds box records only (`gen34_diff.cpp:87,94`:
  `SIZE_STORED3_FRLG`, `SIZE_STORED4`), so it never exercised this rule.

Evidence that Gen IV keeps a party Pokémon's level in step with its EXP:

- **The level-update functions.**
  - pokeplatinum `src/pokemon.c:555-562` (`Pokemon_CalcLevelAndStats`) and `:3501-3525`
    (`Pokemon_ShouldLevelUp`, which also caps EXP at the level-100 total).
  - pokeheartgold `src/pokemon.c:2762-2776` (`Pokemon_TryLevelUp`).
- **Box to party** recomputes the level:
  - pokeplatinum `Pokemon_FromBoxPokemon`, `src/pokemon.c:4031-4054`;
  - pokeheartgold `CopyBoxPokemonToPokemon`, `src/pokemon.c:3228-3245`.
- **Every write of a party Pokémon's EXP** is followed by a level update:
  - Rare Candy: pokeplatinum `src/item_use_pokemon.c:291-294`, pokeheartgold
    `src/use_item_on_mon.c:296`;
  - Day Care withdrawal: pokeplatinum `src/overlay005/daycare.c:147-187`, pokeheartgold
    `src/get_egg.c:118-150`;
  - battle EXP: pokeplatinum `src/battle/battle_script.c:9775`, then the
    `Pokemon_ShouldLevelUp` loop at `:9825`; pokeheartgold
    `src/battle/battle_command.c:6085`, then `:6136`;
  - level caps for battle facilities and link battles. These change copies of the party
    and recalculate the level:
    - pokeplatinum `src/field_battle_data_transfer.c:306-309`,
      `src/overlay104/battle_castle.c:126-130`, `src/overlay104/ov104_0223A0C4.c:911-915`,
      `src/overlay104/frscrcmd_battle_arcade.c:145-150`,
      `src/overlay104/battle_arcade.c:319-322` and `:1246-1249`,
      `src/applications/frontier/battle_castle/opponent_app.c:2707-2719`;
    - pokeheartgold `src/battle/battle_setup.c:285-288` and `:352-355`,
      `src/overlay_25.c:234-238`, `src/frontier/frontier_cmd_arcade.c:95-105`.

  These are every party-EXP write that `grep` finds in either decompilation
  (`Pokemon_SetValue`/`SetMonData`/`AddMonData` with `MON_DATA_EXPERIENCE`); the
  other writes are to box records, which store no level.

Diamond/Pearl was not read separately; pokediamond shares this code base. This rule is
stricter, and it rests on decompiled original code: keep.

Known gap: a stored party level of 0 or above 100 falls back to the EXP level
(`Pokemon4ReadOnlyView.cpp:31`) and is not reported.

## 11. Gen V hidden-ability flag with a slot 1/2 ability (downgraded)

### What PokeBank NX did

`Gen5Analyzer::abilities()` (`Gen5Legality.cpp:700-722`) skips the PID check when the
matched template has a fixed ability and is of the current species (line 704). Otherwise,
with the hidden-ability flag set (PK5 `0x42` bit 0), it required the stored ability to be
the species' hidden ability, and reported "Hidden ability flag is set but the ability is
not the hidden ability" as Invalid.

The matched template is the first candidate without failures, in table order
(`encounterGen5`). The verdict on the same record therefore depended on which template
came first.

The differential (seed 1, 4 mutations) produced one such record, `m9008-ability`:

- a Scizor with the flag set and Technician;
- it fits the Entree Forest templates of both Scyther and Scizor;
- PokeBank NX matched Scyther first (an evolved template), and so reported Invalid;
- PKHeX matched Scizor's own template and reported VALID (`EncounterStatic5Entree:212`,
  `Ability:AbilityFlag`, from the oracle's `probe`).

Had PokeBank NX matched Scizor first, line 704 would have skipped the check, as PKHeX does.

### Why PKHeX accepts it

`VerifyAbility345` (`AbilityVerifier.cs:162-184`) sends a fixed-ability template to
`VerifyFixedAbility` (`:186-235`):

1. Line 199: the flag must agree with `OnlyHidden`.
2. Lines 202-220: if the Pokémon evolved, `CheckMatch` → `GetPIDAbilityMatch`
   (`:419-469`). That path does require the stored ability to be the hidden one.
3. Line 222: otherwise, VALID when the stored ability is in the template's slot.
4. **Lines 225-226: otherwise, VALID when `AbilityNumber == 1 << slot`.**

   For `OnlyHidden` that is just `AbilityNumber == 4`, and `PK5.AbilityNumber` is
   `HiddenAbility ? 4 : …` (`PK5.cs:58`). Nothing compares the stored ability with the
   hidden slot.

The history explains the shortcut:

- It began as "skip the mismatch check when `AbilityNumber == EncounterAbility`" in
  `e29cf2a903e44287b90a9289a6efd796f9f33be6` (2018-06-23, "Rework secondary check flow").
- It became `if (pkm.AbilityNumber == EncounterAbility) return VALID;` in
  `8e950e83b3030cd103076a370d1f65a036849c3a` (2018-07-02, "Rewrite ability verification").
- `9cde2915959ea6b424fc520dc581590b3afec69a` (2022-01-08) rewrote it as `1 << GetSingleValue()`.

No commit or comment in that history speaks of a hidden-ability flag with a slot 1/2
ability. In this reviewer's reading, the acceptance is a side effect of a shortcut
written for fixed slot 1/2 templates, not a claim that the games produce such a record.
That is an inference from PKHeX's code, not a documented reason.

PKHeX rejects the same fields on every other path:

- evolved: lines 202-220;
- eggs and wild slots: `VerifyAbility5`, `:360-369`, then `CheckMatch`;
- after transfer, format 6+: lines 47-54.

PKHeX's model of Poké Transporter takes the flag as authoritative and gives the hidden
ability (`PK5.cs:552-556`). The GitHub issue tracker could not be searched from here.

### Could a Gen V game produce it?

| Mechanism | Finding | Kind of evidence |
|---|---|---|
| Ability Capsule | Does not exist before Gen VI. PKHeX models it from format 6 (`CanAbilityCapsule`, `AbilityVerifier.cs:483-488`), and never to or from the hidden slot (`:472-481`) | Secondary (PKHeX; Bulbapedia "Ability Capsule", not reachable here) |
| Poké Transfer (Gen IV → V) | Gen IV has no flag. Gen III/IV-origin PK5 with the flag set is Invalid in both programs (`Gen5Legality.cpp:694`, `AbilityVerifier.cs:421-422`) | Secondary |
| Entree Forest / Dream World, Dream Radar, Hidden Grotto | Give the hidden ability. No account of any of them giving the flag with a slot 1/2 ability was found | None either way |
| Evolution in Gen V | PKHeX assumes the new hidden ability follows the flag. Not verifiable without a decompilation | Secondary |
| Form change (Reveal Glass, DNA Splicers…) | No account found | None either way |
| A Gen V glitch | None found | None |

No original-game evidence was found that the games can produce this record, and none
that they cannot. Under the owner's rule it cannot be definitively Invalid.

### Change

In `Gen5Legality.cpp:708-717`, when the flag is set, the ability is not the hidden one
and the matched template is `OnlyHidden`, the analyzer now:

- adds a Warning: "Hidden ability flag is set but the ability is not the hidden ability
  (not shown impossible in Gen V; not treated as Invalid)";
- marks internal coverage Partial, so the report is Incomplete.

The change is local to that rule. Bred Pokémon with the same fields stay Invalid,
because PKHeX agrees there (`VerifyAbility5` → `CheckMatch` → `GetPIDAbilityMatch`).
That is not a disagreement.

Other changes:

- `tools/pkhex_gen5_diff/run.sh` drops its `stricter` allow-list. Any record PKHeX
  accepts and PokeBank NX calls Invalid now fails the run.
- `tests/test_legality_gen5.cpp` adds three cases:
  1. `m9008-ability` gives a Warning with no Invalid, and the same report in encrypted
     form.
  2. The same Scizor with Light Metal (135), its hidden ability, has no Invalid and no
     Warning.
  3. `m14877-ability` (a Relicanth egg, flag set, Swift Swim; PKHeX `AbilityMismatchPID`)
     is still Invalid with the same text.

  The first case fails against the old rule; this was run, and the old rule reported
  "Matches … (Scyther)" plus Invalid.

Differential, `tools/pkhex_gen5_diff/run.sh <PKHeX> 1 4`:

- **Same records, old code against new.** One oracle output (40,030 records) was judged
  by the code at the base commit and by the changed code.
  - Old code: 18,922 VALID/INCOMPLETE, 1 VALID/INVALID (`m9008-ability`),
    21,080 INVALID/INVALID, 27 INVALID/INCOMPLETE.
  - New code: 18,923 VALID/INCOMPLETE, 0 VALID/INVALID, 21,080 INVALID/INVALID,
    27 INVALID/INCOMPLETE.

  Exactly one output line changed: `m9008-ability`, from INVALID to INCOMPLETE. No
  record PKHeX rejects lost its Invalid.
- **The run script after the change:** 18,936 VALID/INCOMPLETE, 21,067 INVALID/INVALID,
  27 INVALID/INCOMPLETE, then "PASS: no false Invalid".

The oracle does not produce the same records on every run with the same seed. Three runs
of seed 1 gave 18,923, 18,933 and 18,936 PKHeX-valid records. This is why the old and new
code were compared on one saved output.

Still open, for the owner to decide:

- When the template is of the current species, both programs stay silent on this
  combination (line 704; PKHeX line 225). Gen V reports are always Incomplete (internal
  coverage is Partial), so this never reads "No problems found". Adding a Warning there
  would raise those records' severity; this review was told not to make any rule
  stricter, so it was not done.
- The bred case is Invalid in both programs, but it lacks Gen V original-game evidence
  too. It is not a PKHeX disagreement, so it was left as it is.

## 12. Gen V name length (keep, found when merging the Gen V text work)

The Gen V text work (merged after this review) documented one more theoretical difference:
the analyzer reads a name only up to the 0xFFFF terminator and calls a nickname over 10 or an
OT over 7 units Invalid. PKHeX applies the same limits to Gen <= 5 (`Legal.GetMaxLengthOT`,
`GetMaxLengthNickname`) but skips the OT check for fixed-trainer templates and traded eggs
and only warns (Fishy) for a foreign species name within that language's length. Every
exempt name comes from the game's own buffer, which the PK5 format bounds at 8 OT units and
11 nickname units including the terminator, so a game-made exempt name is still within the
limit. No PKHeX-built record reached the difference in the Gen V differential (seeds 1, 2, 7,
11). It stays as is; a real record that reaches it would be the evidence to revisit.

## 13. Gen V capsule transfer location on a non-event Celebi or beast (downgraded)

Found by the Gen V differential in CI on `2877d2a5` (record `m110571-metlocation`: a
SoulSilver static Raikou, not fateful, transferred with met location 30012).

- **PokeBank NX did:** accept 30010/30011 (Celebi) and 30012/30013 (Raikou, Entei, Suicune)
  only on a fateful Gen IV-origin record, and call anything else "not a Poke Transfer
  location" (Invalid).
- **PKHeX:** `TransferVerifier.VerifyTransferLegalityG4` accepts those locations on any Gen
  IV-origin Celebi or beast, fateful or not. Only its own PK4-to-PK5 converter
  (`PK5.GetTransferMetLocation4`) limits them to fateful records; that models Poke Transfer
  but is not original-game evidence, and there is no Gen V disassembly to settle it.
- **Change:** a non-fateful Celebi or beast at its capsule location is now a Warning
  (Incomplete). Every location PKHeX rejects stays Invalid: any other location, a capsule
  location on another species, and any capsule location on a Gen III origin (PKHeX
  `VerifyTransferLegalityG3` requires 30001). The CI record is now a fixture in
  `tests/test_legality_gen5.cpp` (`transfer-crown-nonfateful`).

## 14. Gen IV met-location fields that disagree (keep, found by the title matrix)

Found by the 20-title matrix (`TITLE_TEST_MATRIX.md`, Gen IV rows): a Platinum Giratina-Origin
with 117 in the D/P met-location field, and HeartGold and SoulSilver records with 178 there, read
"No problems found". PKHeX calls them VALID too.

- **The games.** A PK4 has two met-location fields: the D/P field (0x80) and the Pt/HG/SS field
  (0x46). Platinum and HG/SS write a met location only through one setter, which writes both: a
  place D/P know (1-111, 2000-2010, 3000-3076; pokeplatinum `src/unk_02017038.c:38-45`,
  pokeheartgold `asm/unk_02017FAC.s:68-97`) goes into both, and any other place puts Faraway Place
  (3002) in the D/P field (pokeplatinum `src/pokemon.c:1732-1740`, pokeheartgold
  `src/pokemon.c:1286-1292`). Neither decomp writes either field anywhere else: every other write
  is `Pokemon_SetValue`/`BoxPokemon_SetValue` (Platinum) or `SetMonData`/`SetBoxMonData` (HG/SS)
  through that setter, including gifts and events (pokeplatinum `src/unk_02092494.c:749-900`).
  The games read the D/P field unless it holds 3002 (pokeplatinum `src/pokemon.c:1168-1174`,
  pokeheartgold `src/pokemon.c:826-827`). D/P never write the Pt/HG/SS field, and a non-egg
  Pokémon's met location is not rewritten later.
- **Left out.** Pal Park records (PKHeX's own rule covers them, `MiscVerifierG4.cs:35-45`) and
  egg-origin records (hatching in D/P rewrites the D/P field alone). A gift received in D/P keeps
  the template's Pt/HG/SS field, and all 590 Gen IV event templates in PKHeX's database
  (`mgdb/wc4.pkl`, decoded) hold 0 there, so the rule does not reach them. My Pokémon Ranch
  writes its gifts on the Wii, which has no decomp, so a Ranch record only gets a Warning.
- **Real records.** All 29 PK4 files in PKHeX's test suite (`Tests/PKHeX.Core.Tests/Legality`)
  satisfy the rule: the Platinum and HG/SS ones hold either the same place in both fields or 3002
  with the HG/SS place, and the D/P ones hold 0 in the Pt/HG/SS field.
- **PKHeX.** It reads the Pt/HG/SS field first whenever it is not 0 (`G4PKM.cs:229-237`), so it
  judges the encounter by that field alone. Its setter writes both fields the way the games do
  (`G4PKM.cs:238-258`), so every record it builds agrees. Only its mutation of the Pt/HG/SS field
  alone (`metlocext` in the Gen III/IV oracle) builds records this rule rejects.
- **Change.** Invalid, with the fixture `met-location-dp-field` in `tests/test_legality_gen4.cpp`
  and the three matrix rows. The Gen III/IV differential lists these records as "deliberately
  stricter", and only when `met_location_fields` in `tools/pkhex_gen34_diff/summarize.py`
  confirms from the record bytes that the two fields disagree; any other record PKHeX accepts
  and PokeBank NX calls Invalid still fails the run. The CI set of runs
  (seeds 1-3; mutation corpus with Gen III and IV, grown Gen IV corpus, grown Gen III corpus) has
  0 false Invalid and 0 false Valid. It lists 112 (mutation) and 109 (grown) records under this
  rule, every one a `metlocext` mutation, on D, P, Pt, HG and SS records.

## 15-A. Colosseum/XD team lock search: the CPU-shiny reroll frame (more lenient, Gen III GameCube lane)

Found while porting `TeamLockResult.GetSingleLock` (`TeamLockResult.cs:86-129`) for the
shadow team locks of `Gen3Legality.cpp` (`TeamLockSearch::singleLock`).

- **PKHeX.** In the reroll loop, a frame whose shiny value equals the forced CPU shiny value
  (`sv == RCSV`, `:118-123`) sets `forcedOT = true` and `continue`s without advancing the frame
  counter, so the same frame is re-tested forever: PKHeX would never return from that branch.
  In practice the branch is unreachable for the published templates (RCSV is only forced by
  a prior `GetAllLocks` lock, and the differential corpora never enter it), which is why PKHeX
  does not hang on real records.
- **The games.** Colosseum and XD have no decompilation; the only source for the lock
  structure is PKHeX. The reroll frame itself is the CPU trainer's anti-shiny reroll, the same
  event `GetAllLocks` (`:131-178`) handles by moving on to the next frame.
- **Change.** The port treats the frame as `GetAllLocks` does and advances (`start += 2`). This
  can only find a seed where PKHeX would not terminate; it never produces a new Invalid. A
  budget (`CxdGuard`, 400000 steps per lock search) bounds the whole search; reaching it is
  reported Unknown (16-A). Verified against the PKHeX oracle on 3914/3928/3908 GameCube records
  (seeds 1-3) with 0 false Invalid and 0 false Valid.

## 16-A. Colosseum/XD RNG searches are bounded (Unknown instead of a hang, Gen III GameCube lane)

- **PKHeX.** `MethodCXD.IsValidNameScreenEndSeed` (`MethodCXD.cs:644-677`) recurses without a
  depth limit, `TryGetSeedTrainerID` (`:680-693`) and `TryGetSeedStarterColo` (`:571-612`) loop
  until a seed is found, and the team lock search (15-A) is unbounded.
- **Change.** `Gen3Legality.cpp` caps them (`CxdGuard`: name-screen recursion depth 256,
  100000 steps for the trainer-ID and Colosseum starter seeds, 400000 steps per lock search).
  A capped search reports "RNG search reached its limit" (Warning, Unknown), and every Invalid
  finding made under a cap is demoted to a Warning: a cap can never add an Invalid. No record of
  the seed 1-3 mutation and grown corpora, the 70 constructed fixtures or the fuzz loops reached
  a cap, so this is a guard, not a disagreement about any real record.

Also checked while porting, not disagreements: `TeamLockResult.VerifyNPC` (`:180-212`)
enumerates its team stack top first with `pos` counting down, which pairs the k-th pushed frame
with `Locks[k]`; the port replicates that pairing exactly (`TeamLockSearch::verifyNpc`). A GBA
wild record relabelled with version 15 is Invalid here ("Unable to match an encounter from the
origin game") because PKHeX's `EncounterGenerator3GC` finds no template for it (EncInvalid);
the earlier Warning was the analyzer's own scope limit, not a PKHeX verdict.

## Not disagreements (checked)

These Invalid sites match a PKHeX rule:

| PokeBank NX | PKHeX |
|---|---|
| Colosseum/XD female OT, `Legality.cpp:325` (Pal Park PK4) and `Gen3Legality.cpp` `verifyCxd` (PK3) | `CXDVerifier.cs:20` (`G3OTGender`) |
| Korean Shaymin, `:1335` | `EncounterVerifier.cs:33` |
| Korean hatch at Seabreak Path / Flower Paradise, `:1585` | `EncounterVerifier.cs:211` |
| Pal Park split-ball fields, `:1491` | `MiscVerifierG4.cs:72-94` |
| Gen IV PID ability slot, `:2027` | `CheckMatch`. Every PKHeX Gen IV fixed-ability template (DPPt Ranch gifts, HGSS trades) has its fixed slot equal to its PID's slot |
| Gen V hidden-ability flag on a Gen III/IV origin, `Gen5Legality.cpp:694` | `AbilityVerifier.cs:421-422` |
| GB egg stat experience, `Legality.cpp:1140` | `EffortValueVerifier.cs:17-20` (`EffortEgg`) |
| GB duplicate moves, empty trainer name | `LearnVerifier.cs:90` (`MoveResult.Duplicate`), `TrainerNameVerifier.cs:30` (`OTShort`) |

`docs/legality/GB_LEGALITY_VERDICTS.md:14` still says `GB::checkGen2` is not wired into
the app. At the base commit it is (`7356b383`, `Legality.cpp:1029`, `analyzeNativeGen2`).
That text is out of date, but it is not a disagreement. It was left alone because it is
outside this review.
