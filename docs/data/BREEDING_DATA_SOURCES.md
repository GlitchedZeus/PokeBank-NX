# Breeding data sources (Round 13, owner decisions 5 and 6)

Owner decision 5: "Add proper egg-group and breeding compatibility data. No generic egg histories
presented as verified." Owner decision 6: "Support Gen IV Day-Care/trade location markers where native
semantics and history are verified; distinguish unverified choices."

## Generated table: `include/Data/EggGroups.h`

Generator: `tools/gen_egg_groups.py` (`--check` verifies the committed file is reproducible).
It covers species 1-251 (Gen II), 1-386 (Gen III), 1-493 (Gen IV) and 1-649 (Gen V). Each row holds
egg group 1, egg group 2, the gender ratio and the pre-evolution in that generation.

| Role | Repository | Commit | Files |
| --- | --- | --- | --- |
| Primary | kwsch/PKHeX | `6501f0ab46e8f8ca048539dbaf8cae8cb104e722` | `PKHeX.Core/Resources/byte/personal/personal_{gs,c,rs,e,fr,lg,dp,pt,hgss,bw,b2w2}`; offsets from `PersonalInfo/Info/PersonalInfo2.cs`, `PersonalInfo3.cs`, `PersonalInfo4.cs`, `PersonalInfo5BW.cs`, `PersonalInfo5B2W2.cs` |
| Primary (pre-evolutions) | kwsch/PKHeX | `6501f0ab46e8f8ca048539dbaf8cae8cb104e722` | `PKHeX.Core/Resources/byte/evolve/evos_g4.pkl`, `evos_g5.pkl`. Gen II and Gen III use the Gen IV edges where both species are in their National Dex |
| Cross-check (Gen II) | pret/pokecrystal | `3c813b0b4d82b39bf62185aceffb76fd6026c70b` | `data/pokemon/base_stats.asm`, `data/pokemon/base_stats/*.asm`, `constants/pokemon_data_constants.asm` |
| Cross-check (Gen III) | pret/pokeemerald | `731ad5bfd6e6f265508d0efcca0ba42f9dcf5881` | `src/data/pokemon/species_info.h`, `include/constants/pokemon.h` |
| Species names (Emerald mapping) | kwsch/PKHeX | `6501f0ab46e8f8ca048539dbaf8cae8cb104e722` | `PKHeX.Core/Resources/text/other/en/text_Species_en.txt` |

The generator stops if:
- two games of the same generation disagree (G/S vs C, R/S vs E vs FR/LG, D/P vs Pt vs HG/SS, B/W vs B2/W2);
- pret/pokecrystal disagrees for any of the 251 species;
- pret/pokeemerald disagrees for any of the 386 species.

Result at these commits: all games of each generation agree, both decompilations agree, and no species
changed egg groups between Gen II and Gen V.

Egg-group numbering (shared by all three sources): 1 Monster, 2 Water 1, 3 Bug, 4 Flying, 5 Field,
6 Fairy, 7 Grass, 8 Human-Like, 9 Water 3, 10 Mineral, 11 Amorphous, 12 Water 2, 13 Ditto, 14 Dragon,
15 Undiscovered. The gender ratio uses 0 for male only, 254 for female only and 255 for genderless.

## Policy: `include/Integration/Encounter/BreedingPolicy.h`

`compatible(gen, a, b)` returns true when two Pokémon can breed together. Neither can be Undiscovered,
and two Dittos cannot breed. A Ditto can breed with any other Pokémon, and a genderless Pokémon can
breed only with Ditto. Otherwise the pair needs a shared egg group and one male and one female.

`hatchable(gen, species)` returns true when some parent that is not Undiscovered and not Ditto lays an
egg that hatches as that species. The egg hatches as the first stage of the parent's family. Some
special cases come from PKHeX `Legality/Breeding.cs`:
- Incense splits (`IsSplitBreedNotBabySpecies3` / `4`):
  - Gen III: Marill (Azurill) and Wobbuffet (Wynaut).
  - Gen IV and V add Chansey (Happiny), Mr. Mime (Mime Jr.), Snorlax (Munchlax), Sudowoodo (Bonsly),
    Mantine (Mantyke), Roselia (Budew) and Chimecho (Chingling).
- Manaphy eggs hatch as Phione.
- Nidoran♀ / Nidoran♂ and Volbeat / Illumise eggs can hatch as either species of the pair.

Hatch level is Lv. 5 in Gen II and III and Lv. 1 in Gen IV and V (PKHeX `EncounterEgg2`-`5`).
`tests/test_breeding_policy.cpp` checks every species of every generation against PKHeX's
devolve-then-exclude rule (`Breeding.IsAbleToHatchFromEgg`). Hatchable counts: Gen II 116, Gen III 181,
Gen IV 220, Gen V 289.

## Gen IV markers (PKHeX `Game/Locations/Locations.cs`, `Legality/Encounters/Verifiers/EncounterVerifier.cs`)

| Id | PKHeX name | Native field and meaning | Picker behaviour |
| --- | --- | --- | --- |
| 2000 | `Daycare4` | **Egg Location** of a Day-Care egg (`EncounterEgg4`). An unhatched egg's Met Location is 0. A hatched Pokémon's Met Location is where it hatched, with Met Level 0. | Never written as a Met Location. The row "Hatched from a Day-Care egg" writes Met Location = Day-Care hatch place (D/P/Pt 4 Solaceon Town, HG/SS 182 Route 34), Met Level 0 and Egg Location 2000. It is verified only when `BreedingPolicy::eggHistory(4, ...)` verifies the species. |
| 2001 | `LinkTrade4NPC` | Met Location of an in-game (NPC) trade (`EncounterTrade4PID`, except HG/SS Spearow and Shuckle, which record their real places) | Verified for the exact game's trades and their evolutions at or above the trade level (`InGameTrades4.h`, from `Encounters4DPPt.cs` / `Encounters4HGSS.cs`). Otherwise unverified. |
| 2002 | `LinkTrade4` | Met Location of an egg traded before it hatched; Egg Location of a hatched traded egg (`TradedEggLocation`) | Verified only for an egg record of a hatchable species. Otherwise unverified. |

None of these is "Pokémon Ranch". Ranch gifts use other locations, such as 3000 and map ids
(`EncounterTrade4RanchGift`).

The ids are encoded like PKHeX `G4PKM.MetLocation` / `EggLocation`: the D/P field (0x80 / 0x7E) holds
the id, and the extended field (0x46 / 0x44) holds it only for Platinum and HG/SS origins.
`tests/test_gen4_breeding_markers.cpp` writes each marker, then checks Review, export and strict
reparse in all five games.

## Gen III and Gen V

- **Gen III:** 253 is the Met Location of an **unhatched gift egg** (pokeemerald `METLOC_SPECIAL_EGG`;
  the game replaces it with the hatch place when the egg hatches). It is no longer a default row, and
  bred eggs never carry it. Instead, "Hatched from a Day-Care egg" writes the Day-Care hatch place
  (R/S/E 32 Route 117, FR/LG 146 Four Island) with Met Level 0. It is verified only for a species
  that hatches at the Pokémon's level.
- **Gen V:** 60002 (`Daycare5`) and 60003 (Breeder) are Egg Location values, so they are no longer
  default Met Location rows. A hatched record whose Egg Location is 60002 / 30003 / 30002 and whose
  Met Level is 0 reads as verified only for a hatchable species. Gen V has no hatched-egg write
  because there is no native Egg Location edit.
- **Crystal:** no egg marker exists, so there is nothing to change. A hatched Crystal record (level
  code 1) still reads through the existing template check.
