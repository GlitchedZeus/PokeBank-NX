# Per-title legality test matrix

This page lists the acceptance test for each of the 20 mainline titles from Generations I to V.
The test is `tests/test_legality_title_matrix.cpp`. It covers the Gen I, Gen II, Gen III and Gen V titles.
The Gen IV analyzer is being rewritten, so its 5 titles are still to do.

Run it on its own:

```
make -f Makefile.host build-host/test_legality_title_matrix
./build-host/test_legality_title_matrix       # one line per case
./build-host/test_legality_title_matrix -v    # also prints every report
```

It also runs in `make -f Makefile.host host-test` and `host-sanitize` (`Makefile.host.title-matrix`).
It prints one line per case: title, kind, case, expected verdict, actual verdict, PASS or FAIL.
It exits non-zero if any case fails.

## What each kind means

| Kind | What it checks |
|---|---|
| Positive | A record a real game makes reads "No problems found". |
| Negative | A record no game can make reads Invalid, and the issue names the rule. |
| Malformed | Corrupt bytes do not crash and never read "No problems found". The test checks the exact verdict. |
| Historical-Unknown | A history the data cannot settle reads Incomplete. It is never Invalid and never "No problems found". |
| Original-game | The expected verdict comes from the game's own code or data (a pret disassembly), not from PKHeX. |
| Title-specific | The title changes the answer. |

The test also checks the following for every case:

- The verdict follows from the issues and coverage, using the same rule as `Report::verdict()`.
- The save or record bytes are the same after the analysis.
- A second analysis gives the same report.

Gen III record cases also run `Legality::analyzePk3` on the same bytes.
`Legality::analyze` must not drop an Invalid from it, and must not certify what it did not certify.

Gen V records are analyzed twice: once decrypted and once in their stored (encrypted) form.
Both must give the same report.

## Which path the app calls

| Generation | Path under test |
|---|---|
| Gen I | Synthetic 32 KiB save, then `Gen1::parse(bytes, title)`, then `Pokemon::Pokemon1ReadOnly`, then `Legality::analyze(pk, pk.getGameGroup(), metadata().sourceGameId)`. This is the call in `src/UI/Modals/Gen1PokemonDetailsModal.cpp:72`. |
| Gen II | Synthetic 32 KiB save, then `Gen2::parse(bytes, title)`, then `Pokemon::Pokemon2ReadOnly`, then the same `Legality::analyze` call. This is the call in `src/UI/Modals/Gen2PokemonDetailsModal.cpp:235`. |
| Gen III | Synthetic 128 KiB save, then `Integration::Gen3::parse(bytes, sourceGame)` (the on-device reader, `PKSMGen3NativeAdapter.cpp`), then `FRLGReadOnlyTrainer::create` as in `src/UI/UI.cpp:393`, then box 1 slot 1 (a `Pokemon3FRLG`) through `Legality::analyze(pk, pk.getGameGroup(), metadata().sourceGameId)`. This is the call in `src/UI/Modals/PokemonDetailsModal.cpp:65`. Record cases put the stored (encrypted) PK3 into `Pokemon3FRLG` and make the same call with the exact source game id. |
| Gen V | `Legality::analyzePk5(bytes, Pk5Input)` on a 136-byte PK5 record. The app on this branch has no PK5 save reader yet: it is in PR #307. So the app does not call `analyzePk5` yet, and the test calls the engine API directly. |

Every Gen I and Gen II case goes end to end, from save bytes to verdict.
The source game ids are the ones the parsers write: `red_gb`, `blue_gb`, `yellow_gb`, `gold_gbc`, `silver_gbc`, `crystal_gbc`.

Gen III has 13 cases from save bytes: one positive and one trade per title, a Bad Egg, a Colosseum/XD record and a save opened as the wrong family.
The other Gen III cases are record cases. The ids are `ruby_gba`, `sapphire_gba`, `emerald_gba`, `firered_gba`, `leafgreen_gba`.

## Where the game is recorded

- **Red, Blue, Yellow.** A PK1 record does not say which game made it. Only the save does.
  - Red and Blue use the same save layout, and nothing in the save tells them apart. The app's hint decides `red_gb` or `blue_gb`.
  - A Yellow save is told apart by its starter byte (Pikachu) and its Pikachu friendship byte.
  - The Gen I analyzer does not use the exact title. The same record gets the same verdict in a Red, Blue or Yellow save.
- **Gold, Silver.** A PK2 record from Gold or Silver does not say which game made it.
  - Gold and Silver saves have the same layout. The hint decides `gold_gbc` or `silver_gbc`.
  - The verdict comes from `GB::checkGen2`, which does not use the title. The title only adds an Info note: "Species has pinned Gold/Silver wild-slot provenance".
- **Crystal.** Crystal saves have their own layout.
  - Crystal also writes caught data into the record: time of day, met level, OT gender and met location. So a Crystal-caught record says where it was caught.
  - In a Crystal save the analyzer adds "PK2 caught-data location/level/time matches a pinned Crystal wild encounter slot".
- **Ruby, Sapphire, Emerald, FireRed, LeafGreen.** The PK3 origins word at 0x46 holds the origin game in bits 7-10: 1 Sapphire, 2 Ruby, 3 Emerald, 4 FireRed, 5 LeafGreen, 15 Colosseum/XD.
  - The Gen III analyzer matches encounters for that game, not for the save's title. So a Pokemon traded in from another GBA game reads the same in any GBA save.
  - The same bytes with another version can be Invalid. Version-exclusive encounters show this.
  - The save's title still matters in one way: without a GBA source id nothing is certified. A Bank record has no source id.
  - The save bytes tell three families apart. The word at section 0 + 0xAC is 1 on FireRed/LeafGreen, the security key on Emerald, and 0 on Ruby/Sapphire (`detectFamily` in `include/Integration/Gen3/Gen3SaveValidation.h`).
  - Ruby and Sapphire saves have the same layout, and so do FireRed and LeafGreen saves. The file's identity picks the title. A save whose family does not match that title is refused.
- **Black, White, Black 2, White 2.** The record has a version byte at 0x5F: 20 White, 21 Black, 22 White 2, 23 Black 2.
  - Encounter matching uses it, so the same bytes can be legal in one title and Invalid in its pair.
  - A Pokemon transferred from Gen III or IV keeps its old version byte. Nothing in the record says which Gen V game received it.

## The matrix

Verdicts are written as: NP = "No problems found", Inv = Invalid, Inc = Incomplete, Rej = parser rejects the save (error name in brackets).

| Title | Analyzer path the app calls | Positive | Negative | Malformed | Historical-Unknown | Original-game | Title-specific | PKHeX differential |
|---|---|---|---|---|---|---|---|---|
| Red | Gen1 parse, Pokemon1ReadOnly, `analyze(..., "red_gb")` | Route 1 Pidgey L3, Gust: NP | Pidgey knowing Spore: Inv ("Pidgey cannot learn Spore from this origin") | Checksum byte off by one: Rej (checksum_mismatch). Move id 166: Inv | Pidgey with battle stat exp {7,0,3,3,3}: Inc | Route 4 Ekans L6, Wrap/Leer, a Red-only slot: NP, origin "wild Ekans in Red (level 6+)" | Red save opened as Yellow: Rej (game_hint_mismatch) | Gen I run, not split by title (see below) |
| Blue | Gen1 parse, Pokemon1ReadOnly, `analyze(..., "blue_gb")` | Route 1 Rattata L2, Tackle/Tail Whip: NP | Rattata with Fire type bytes: Inv ("First type byte does not match Rattata") | Party level 101: Rej (invalid_party). Empty nickname: Inv ("Nickname is empty") | Rattata with battle stat exp {7,0,3,3,3}: Inc | Route 4 Sandshrew L6, Scratch, a Blue-only slot: NP, origin "wild Sandshrew in Blue (level 6+)" | Blue save opened as Red: accepted as `red_gb`, same verdict | Gen I run, not split by title |
| Yellow | Gen1 parse, Pokemon1ReadOnly, `analyze(..., "yellow_gb")` | Viridian Forest Caterpie L3, Tackle/String Shot: NP | Pikachu knowing ThunderShock twice: Inv ("Duplicate move: Thunder Shock") | Party marker and species = internal index 31 (no species): Rej (checksum_mismatch). OT bytes 01 02 03...: Inv ("Original trainer name is empty") | Caterpie with battle stat exp {7,0,3,3,3}: Inc | Starter Pikachu L5 with catch-rate byte 0xA3: NP, origin "gift/static Pikachu in Yellow (level 5)" | Safari Zone Dragonair L15, catch rate 27: NP, origin "wild Dragonair in Yellow". Yellow save opened as Red: Rej (game_hint_mismatch) | Gen I run, not split by title |
| Gold | Gen2 parse, Pokemon2ReadOnly, `analyze(..., "gold_gbc")` | Route 29 Sentret L3, Tackle: NP | Pokerus byte 0x90: Inv ("Pokerus strain 9 does not exist in Gen II") | Checksum off by one: Rej (checksum_mismatch). Move id 252: Inv | Sentret with battle stat exp {7,0,3,3,3}: Inc | Route 30 Spinarak L3, Poison Sting/String Shot, a Gold-only slot: NP | The same Spinarak in a Silver save: NP without the Gold/Silver wild note | Gen II run, not split by title |
| Silver | Gen2 parse, Pokemon2ReadOnly, `analyze(..., "silver_gbc")` | Route 29 Hoothoot L2, Tackle/Growl: NP | Holding item 7 (Bicycle): Inv ("Held item 7 cannot be held in Gold/Silver/Crystal") | Marker Hoothoot, body Sentret: Rej (invalid_party). Empty OT: Inv ("Original trainer name is empty") | Hoothoot with battle stat exp {7,0,3,3,3}: Inc | Route 30 Ledyba L3, Tackle, a Silver-only slot: NP | The same Ledyba in a Gold save: NP without the Gold/Silver wild note | Gen II run, not split by title |
| Crystal | Gen2 parse, Pokemon2ReadOnly, `analyze(..., "crystal_gbc")` | Route 29 Pidgey L2 with morning caught data: NP, plus the Crystal slot note | Pidgey L2 with met level 3: Inv ("Level is below the Crystal met level") | Save one byte short: Rej (wrong_size). Caught data FFFF: Inv | Pidgey with battle stat exp {7,0,3,3,3}: Inc | Unhatched Odd Egg Pichu, bytes from `odd_eggs.asm`: NP, origin "egg Pichu in Crystal" | Crystal save opened as Gold: Rej (game_hint_mismatch). Crystal-caught Pidgey in a Gold save: NP without either slot note | Gen II run, not split by title |
| Ruby | GBA save, `Gen3::parse(..., RubyGBA)`, FRLGReadOnlyTrainer, Pokemon3FRLG, `analyze(..., "ruby_gba")` | Zangoose L16, Route 114, a Ruby-only slot, from save bytes: NP | 255/255/10 EVs: Inv ("EV total 520 is above 510") | Box record with a bad checksum (a Bad Egg): Rej (strict Gen III adapter could not enumerate storage boxes) | Zangoose holding an Enigma Berry: Inc (Warning "Enigma Berry: legality depends on the e-Reader berry stored in the original save") | Zangoose met at L14 on Route 114 (Ruby has L15-17): Inv ("Unable to match an encounter from the origin game") | Same Zangoose with version Sapphire: Inv. Sapphire roaming Latias with version Ruby: Inv. Sapphire Seviper traded into a Ruby save: NP. Zangoose from the Bank: Inc | 5,736 records, 0 false Valid, 0 false Invalid. Grown: 3,850, 0 false Invalid |
| Sapphire | GBA save, `Gen3::parse(..., SapphireGBA)`, FRLGReadOnlyTrainer, Pokemon3FRLG, `analyze(..., "sapphire_gba")` | Seviper L16, Route 114, a Sapphire-only slot, from save bytes: NP | Knows Wrap twice: Inv ("Move 1 (Wrap) is a duplicate") | Record cut to 79 bytes (`analyzePk3`): Inc (Warning "Record is not a Gen III PK3 (79 bytes)", structure not read) | Seviper with origin Colosseum/XD: Inc (Warning "Colosseum/XD origin; Gen III cartridge rules cannot verify this Pokemon") | Seedot L3, Route 102, Bide/Harden/Leech Seed, from a record-mixed Emerald outbreak: NP | Same Seviper with version Ruby: Inv. Ruby Zangoose traded into a Sapphire save: NP. Seviper from the Bank: Inc | 5,296 records, 0 false Valid, 0 false Invalid. Grown: 3,663, 0 false Invalid |
| Emerald | GBA save, `Gen3::parse(..., EmeraldGBA)`, FRLGReadOnlyTrainer, Pokemon3FRLG, `analyze(..., "emerald_gba")` | Smeargle L40, Artisan Cave, an Emerald-only Method H slot, from save bytes: NP | Pokerus strain 0 with 5 days left: Inv ("Pokerus days remaining exceed the strain duration") | Origin game id 0: Inc (Warning "Origin game id 0 is not a Gen III cartridge") | Colosseum/XD-origin record in an Emerald save, from save bytes: Inc | Navel Rock Lugia L70 with the fateful flag: NP. Without it: Inv ("Event encounter is missing its fateful encounter flag") | Same Smeargle with version Ruby: Inv. FireRed Scyther traded into an Emerald save: NP. Ruby/Sapphire save opened as Emerald: Rej (family does not match). Smeargle from the Bank: Inc | 5,516 records, 0 false Valid, 0 false Invalid. Grown: 3,746, 0 false Invalid |
| FireRed | GBA save, `Gen3::parse(..., FireRedGBA)`, FRLGReadOnlyTrainer, Pokemon3FRLG, `analyze(..., "firered_gba")` | Scyther L25, the Celadon Game Corner prize, from save bytes: NP, with "the Gen III analyzer matched static encounter Scyther at met location 94, level 25" | Fateful flag set: Inv ("Fateful encounter flag is not possible for this encounter") | Stored checksum off by one: Inc (Warning "PK3 checksum does not match its data") | Scyther holding an Enigma Berry: Inc | Prize Scyther with met level 24: Inv ("Unable to match an encounter from the origin game") | Same Scyther with version LeafGreen: Inv. LeafGreen Bellsprout traded into a FireRed save: NP. Scyther from the Bank: Inc | 6,132 records, 0 false Valid, 0 false Invalid. Grown: 3,994, 0 false Invalid |
| LeafGreen | GBA save, `Gen3::parse(..., LeafGreenGBA)`, FRLGReadOnlyTrainer, Pokemon3FRLG, `analyze(..., "leafgreen_gba")` | Bellsprout L13, Route 5, a LeafGreen-only slot, from save bytes: NP | EXP 0x00FFFFFF: Inv ("EXP is above the level 100 maximum") | Internal species index 260: Inv ("Species index 260 is not a Gen III Pokemon") | Bellsprout with origin Colosseum/XD: Inc | Prize Pinsir L18: NP. Prize Pinsir with met level 17: Inv | Prize Pinsir with version FireRed: Inv. Bellsprout with version FireRed: Inv ("Event gift is missing its fateful encounter flag"). Emerald Smeargle traded into a LeafGreen save: NP. Bellsprout from the Bank: Inc | 6,156 records, 0 false Valid, 0 false Invalid. Grown: 3,988, 0 false Invalid |
| Diamond | pending Gen IV analyzer merge | | | | | | | |
| Pearl | pending Gen IV analyzer merge | | | | | | | |
| Platinum | pending Gen IV analyzer merge | | | | | | | |
| HeartGold | pending Gen IV analyzer merge | | | | | | | |
| SoulSilver | pending Gen IV analyzer merge | | | | | | | |
| Black | `analyzePk5` (no app reader yet) | Cottonee L14, Pinwheel Forest, Black-only slot: NP | Fateful-encounter flag set: Inv ("Fateful encounter flag is set on a non-event encounter") | Record cut to 135 bytes: Inc (Warning "Not a 136-byte or 220-byte PK5 record", structure not read) | Diamond Caterpie, transferred: Inc (Warning "Gen III/IV origin: the original encounter and moves are not checked") | Platinum Caterpie in a Dream Ball: Inv ("The Dream Ball did not exist in Gen IV") | Same Cottonee with version byte White: Inv ("No Gen V encounter gives Cottonee at met location 33"). N's Purrloin with version byte Black: Inv | 16,290 records, 0 false Valid, 0 false Invalid |
| White | `analyzePk5` (no app reader yet) | Petilil L14, Pinwheel Forest, White-only slot: NP | Met level 13 on a level-14 slot: Inv ("Closest Gen V encounter ... does not match") | Stored form with one encrypted byte flipped: Inv ("PK5 checksum does not match its data") | FireRed Weedle, transferred: Inc | HeartGold Caterpie in a Dream Ball: Inv ("The Dream Ball did not exist in Gen IV") | Same Petilil with version byte Black: Inv ("No Gen V encounter gives Petilil at met location 33") | 13,718 records, 0 false Valid, 0 false Invalid |
| Black 2 | `analyzePk5` (no app reader yet) | Cottonee L15, Castelia City, Black 2-only slot: NP | Knows Surf: Inv ("Surf cannot be learned by this Pokemon in Gen V") | Stored form with two encrypted blocks swapped: Inv (checksum). Decrypted blocks A and B swapped: Inv (move ids above 559 and more) | HeartGold Caterpie, transferred: Inc | Platinum Caterpie in a Sport Ball: Inv ("Ball 24 exists only in HeartGold / SoulSilver") | Same Cottonee with version byte White 2: Inv. N's Purrloin (a B2W2 gift): NP | 20,696 records, 0 false Valid, 0 false Invalid |
| White 2 | `analyzePk5` (no app reader yet) | Petilil L15, Castelia City, White 2-only slot: NP | N's sparkle flag set: Inv ("N's sparkle flag is set on a Pokemon that is not N's") | Species id 0xFFFF: Inv ("Species 65535 is above the Gen V maximum 649"). Nickname and OT all U+0001: Inv (names too long) | LeafGreen Caterpie, transferred: Inc | SoulSilver Caterpie in a Sport Ball: Inc, not Invalid | Same Petilil with version byte Black 2: Inv. N's Purrloin with version byte White 2: NP | 18,954 records, 0 false Valid, 0 false Invalid |

Cases per title (118 in all):

| Title | Positive | Negative | Malformed | Unknown | Original-game | Title-specific | Total |
|---|---|---|---|---|---|---|---|
| Red | 1 | 1 | 2 | 1 | 1 | 1 | 7 |
| Blue | 1 | 1 | 2 | 1 | 1 | 1 | 7 |
| Yellow | 1 | 1 | 2 | 1 | 1 | 2 | 8 |
| Gold | 1 | 1 | 2 | 1 | 1 | 1 | 7 |
| Silver | 1 | 1 | 2 | 1 | 1 | 1 | 7 |
| Crystal | 1 | 1 | 2 | 1 | 1 | 2 | 8 |
| Ruby | 1 | 1 | 1 | 1 | 1 | 4 | 9 |
| Sapphire | 1 | 1 | 1 | 1 | 1 | 3 | 8 |
| Emerald | 1 | 1 | 1 | 1 | 2 | 4 | 10 |
| FireRed | 1 | 1 | 1 | 1 | 1 | 3 | 8 |
| LeafGreen | 1 | 1 | 1 | 1 | 2 | 4 | 10 |
| Black | 1 | 1 | 1 | 1 | 1 | 2 | 7 |
| White | 1 | 1 | 1 | 1 | 1 | 1 | 6 |
| Black 2 | 1 | 1 | 2 | 1 | 1 | 2 | 8 |
| White 2 | 1 | 1 | 2 | 1 | 1 | 2 | 8 |

## Evidence for each case

Pins: pret pokered `af519899719f0754965776faac0e836a3b906e6d`, pokeyellow `e89ead154b9968aa50eed9328ff2b38b6c194382`, pokegold `ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8`, pokecrystal `3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6`, pokeruby `5784633ce4ef7ade1a7f2d2d0c288e3d5e6cdd7f`, pokeemerald `731ad5bfd6e6f265508d0efcca0ba42f9dcf5881`, pokefirered `037335f4c725d7c9aecdac87066f2002b4bd7e14`, pokeplatinum `1dc13dcea40c04ab53f1f607de46023dc409ef20`, pokeheartgold `d2355244ac42b3c6e40d8c36aa3564ee8706dea4`. PKHeX `kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722`.

The synthetic records use the games' own numbers.

- Stats use the games' formula: pokered `home/move_mon.asm` `CalcStat` and pokecrystal `engine/pokemon/move_mon.asm` `CalcMonStatC`.
- EXP is the level's total for the species' growth rate.
- Every move has its full base PP.

### Red, Blue, Yellow

All paths are in pret pokered unless the line says pokeyellow.

- **Red positive.** `data/wild/maps/Route1.asm:3` has `db 3, PIDGEY`.
  - Pidgey's only level-1 move is Gust (`data/pokemon/base_stats/pidgey.asm:13`). Sand-Attack comes at level 5 (`data/pokemon/evos_moves.asm:601`).
  - The record has type bytes NORMAL/FLYING and catch rate 255 (`data/pokemon/base_stats/pidgey.asm:6-7`), and EXP 57 (medium slow, level 3).
- **Red original-game.** `data/wild/maps/Route4.asm:6-7` has `db 6, EKANS` inside `IF DEF(_RED)`.
  - Ekans knows Wrap and Leer (`data/pokemon/base_stats/ekans.asm:13`). Poison Sting comes at level 10 (`data/pokemon/evos_moves.asm:1336`).
  - The analyzer names "wild Ekans in Red (level 6+)".
- **Blue positive.** `data/wild/maps/Route1.asm:6` has `db 2, RATTATA`.
  - Rattata knows Tackle and Tail Whip (`data/pokemon/base_stats/rattata.asm:13`). Quick Attack comes at level 7 (`data/pokemon/evos_moves.asm:1879`).
- **Blue original-game.** `data/wild/maps/Route4.asm:15-16` has `db 6, SANDSHREW` inside `IF DEF(_BLUE)`.
  - Sandshrew knows Scratch (`data/pokemon/base_stats/sandshrew.asm:13`). Sand-Attack comes at level 10 (`data/pokemon/evos_moves.asm:1193`).
- **Red and Blue negatives.**
  - Spore is not in Pidgey's learnset (`data/pokemon/evos_moves.asm:596-605`) or its TM list.
  - The type bytes must be Rattata's NORMAL/NORMAL (`data/pokemon/base_stats/rattata.asm:6`).
- **Yellow positive.** pokeyellow `data/wild/maps/ViridianForest.asm:3` has `db 3, CATERPIE`.
  - Caterpie knows Tackle and String Shot (pokeyellow `data/pokemon/base_stats/caterpie.asm:13`).
  - It learns nothing by level-up (pokeyellow `data/pokemon/evos_moves.asm:1499-1504`).
- **Yellow original-game.** pokeyellow `scripts/OaksLab.asm:1033-1040` gives the level-5 starter Pikachu.
  - It then writes `LIGHT_BALL_GSC` over its catch rate. That is $A3 (pokeyellow `constants/item_constants.asm:225`).
  - Pikachu knows ThunderShock and Growl (pokeyellow `data/pokemon/base_stats/pikachu.asm:13`).
  - Tail Whip comes at level 6 (pokeyellow `data/pokemon/evos_moves.asm:1095`).
- **Yellow title-specific.** pokeyellow `data/wild/super_rod.asm:25` gives Dragonair L15 in the Safari Zone Center.
  - Yellow's Dragonair catch rate is 27 (pokeyellow `data/pokemon/base_stats/dragonair.asm:7`). Red and Blue say 45 (pokered, same file and line).
  - It knows Wrap, Leer and Thunder Wave (pokeyellow `data/pokemon/base_stats/dragonair.asm:13`).
  - The analyzer names "wild Dragonair in Yellow (level 15+)".
- **Stat experience (Unknown).** `engine/battle/experience.asm:294` (`DivideExpDataByNumMonsGainingExp`) divides battle stat experience among the Pokemon that fought.
  - So a zero stat next to small battle gains can only happen if every one of those battles added nothing to it. The data cannot settle that.

### Gold, Silver, Crystal

All Gold and Silver paths are in pret pokegold, and the Crystal paths are in pokecrystal.

- **Gold positive.** `data/wild/johto_grass.asm:1577` (Route 29, morning) has `db 3, SENTRET`.
  - Sentret learns Tackle at 1 and Defense Curl at 5 (`data/pokemon/evos_attacks.asm:2192-2193`).
  - It is medium fast (`data/pokemon/base_stats/sentret.asm:20`).
- **Gold original-game.** `data/wild/johto_grass.asm:1603` opens `IF DEF(_GOLD)` for Route 30, and line 1621 (night) has `db 3, SPINARAK`.
  - Spinarak learns Poison Sting and String Shot at 1 (`data/pokemon/evos_attacks.asm:2276-2277`).
  - It is fast (`data/pokemon/base_stats/spinarak.asm:16`), so level 3 is 21 EXP.
- **Silver positive.** `data/wild/johto_grass.asm:1592` (Route 29, night) has `db 2, HOOTHOOT`.
  - Hoothoot learns Tackle and Growl at 1 (`data/pokemon/evos_attacks.asm:2217-2218`).
- **Silver original-game.** The `ELIF DEF(_SILVER)` branch of Route 30 has `db 3, LEDYBA` (morning) at `data/wild/johto_grass.asm:1630`.
  - Ledyba learns Tackle at 1 and Supersonic at 8 (`data/pokemon/evos_attacks.asm:2246-2247`).
- **Gold negative.** `engine/events/pokerus/pokerus.asm:43-59` picks the strain.
  - It is `(random & 7) + 1`, or 0, so 1 to 8 or 0. The days are `(strain & 3) + 1`. Strain 9 never happens.
- **Silver negative.** `data/items/attributes.asm:23-24` makes the Bicycle a `CANT_TOSS` key item.
  - `engine/items/pack.asm:241-290` gives such an item a menu without GIVE.
- **Crystal positive.** pokecrystal `data/wild/johto_grass.asm:1240` has Route 29 morning `db 2, PIDGEY`.
  - Pidgey learns Tackle at 1 and Sand-Attack at 5 (pokecrystal `data/pokemon/evos_attacks.asm:226-227`).
  - pokecrystal `engine/pokemon/caught_data.asm:163-199` writes the caught data: (time of day + 1) in the top two bits, then the level, then OT gender and the landmark.
  - Route 29 is landmark 2 (pokecrystal `constants/landmark_constants.asm:7`).
- **Crystal original-game.** pokecrystal `data/events/odd_eggs.asm:40-64` is the plain Pichu Odd Egg.
  - pokecrystal `engine/events/odd_egg.asm:40-94` copies it verbatim into the party as species EGG with OT "ODD".
  - The OT ID line is `dw 02048`. `dw` is little-endian, so the record's bytes are 00 08.
  - The test uses those exact 48 bytes. The analyzer finds "egg Pichu in Crystal" and the pinned gift encounter.

### Ruby, Sapphire, Emerald, FireRed, LeafGreen

The base records come from the PKHeX Gen III/IV corpus (`tools/pkhex_gen34_diff`, seed 1). PKHeX calls each one VALID. The test names each record's corpus id.

Every edited record was also checked with the oracle's probe (`PKHeXGen34Oracle.dll probe HEX`):

- PKHeX agrees with every Invalid and every "No problems found" in the matrix.
- PKHeX calls the Enigma Berry records INVALID (HeldItem:ItemUnreleased). The engine keeps them Incomplete on purpose: the e-Reader berry is stored in the save, not in the record.
- PKHeX also calls the Colosseum/XD, origin-0 and bad-checksum edits INVALID: they match no encounter. The engine does not judge those origins or read a record that fails its checksum, so they stay Incomplete.

The evidence for each title:

- **Ruby positive and original-game.** pokeruby `src/data/wild_encounters.json:6291-6349` is `Route114_Ruby`.
  - It has Zangoose at L16 (line 6324), L17 (6339, 6349) and L15 (6344). Nothing else in Ruby has Zangoose.
  - The template splits these rows by base label: `src/data/wild_encounters.json.txt:27-30` puts a `_Ruby` label under `#ifdef RUBY`.
  - So Zangoose L16 on Route 114 is legal, and Zangoose met at L14 there is not.
- **Ruby title-specific.** Sapphire's `Route114_Sapphire` (`wild_encounters.json:15730`) has Seviper (15762-15787) where Ruby has Zangoose. So the Zangoose record with version Sapphire is Invalid.
  - pokeruby `include/constants/species.h:1282-1286` makes the roamer Latias in Sapphire and Latios otherwise. `src/roamer.c:65` creates it at L40. So a roaming Latias with version Ruby is Invalid.
- **Sapphire positive.** `Route114_Sapphire` has Seviper at L15-17 (`wild_encounters.json:15730-15787`).
- **Sapphire original-game.** pokeemerald `src/tv.c:198-203` is an Emerald mass outbreak: Seedot L3 on Route 102 with Bide, Harden and Leech Seed.
  - pokeruby `src/tv.c:1066-1079` (`StartMassOutbreak`) copies any outbreak show in the save, including a record-mixed one, into the outbreak fields.
  - pokeruby `src/wild_encounter.c:350-361` then creates the Pokemon at that level with those moves.
  - Ruby and Sapphire's own outbreak list (`src/tv.c:89-121`) has only Surskit and Skitty, and Sapphire's wild tables have no Seedot. So this Seedot can only come from an Emerald show. The record matches it exactly.
- **Emerald positive and title-specific.** pokeemerald `src/data/wild_encounters.json:10627-10635` (`gArtisanCave_B1F`) has Smeargle at L40.
  - Artisan Cave does not exist in pokeruby: no file there names it. So the same record with version Ruby is Invalid.
  - The record's PID and IVs follow Method H, as every GBA wild Pokemon does. pokeemerald `src/wild_encounter.c:414` picks a nature first, and `src/pokemon.c:2305-2316` (`CreateMonWithNature`) draws PIDs until one has that nature. `CreateMon` then draws the IVs from the next two calls, so the report names the Method 1 spread.
- **Emerald original-game.** `data/maps/NavelRock_Bottom/scripts.inc:54` is `seteventmon SPECIES_LUGIA, 70`.
  - `asm/macros/event.inc:1990-1995` turns that into `special CreateEnemyEventMon`.
  - `src/pokemon.c:2773-2780` and `2633-2639` create the Pokemon with `MON_DATA_MODERN_FATEFUL_ENCOUNTER` set.
  - So the L70 Lugia with the fateful flag is legal, and the same Lugia without it is Invalid.
- **FireRed positive and original-game.** pokefirered `data/maps/CeladonCity_GameCorner_PrizeRoom/scripts.inc:80-89` offers Scyther in FireRed. Lines 155-156 give it at L25 (`givemon VAR_TEMP_1, 25`).
  - So the L25 prize Scyther is legal, and the same Scyther met at L24 is not.
  - The older production table has no Game Corner rows. The report keeps that as an Info note naming the analyzer's match.
- **FireRed title-specific.** The same script lines give Dratini in LeafGreen, and LeafGreen's wild tables have no Scyther. So the record with version LeafGreen is Invalid.
- **LeafGreen positive.** pokefirered `src/data/wild_encounters.json:21163-21179` (`sRoute5_LeafGreen`) has Bellsprout at L13.
  - `sRoute5_FireRed` (line 8620) has Oddish, Meowth and Pidgey there, and no Bellsprout.
  - With version FireRed, the nearest FireRed template for this record is an event gift, which it fails. PKHeX makes the same match (EncounterGift3, FatefulMissing).
- **LeafGreen original-game.** `CeladonCity_GameCorner_PrizeRoom/scripts.inc:67-78` offers Pinsir in LeafGreen and Dratini in FireRed. Lines 171-172 give Pinsir at L18.
  - So the L18 prize Pinsir is legal, a Pinsir met at L17 is not, and the record with version FireRed is not.
- **Malformed.** pokeemerald `src/pokemon.c:3742-3746` shows a record whose checksum fails as a Bad Egg. The engine reads such a record as a Warning, never as legal.
- **Trades.** Each title's save holds one Pokemon from another GBA game, and that save's trainer has a different ID. Each reads "No problems found", because the record's origin game decides.

### Black, White, Black 2, White 2

Gen V has no disassembly. No pret project for Black, White, Black 2 or White 2 exists. Bulbapedia and Project Pokémon cannot be reached from this environment (`docs/legality/PKHEX_DISAGREEMENT_REVIEW.md`). So no original-game source exists here for native Gen V records.

- **Positive, negative and title-specific records.** These come from PKHeX's generator (`tools/pkhex_gen5_diff`, `oracle generate 1 4`). PKHeX calls each base record VALID.
  - The slots are version-exclusive in the Gen V encounter data (`Gen5LegalityData`): Cottonee in Pinwheel Forest only in Black, Petilil only in White, and the Castelia City slots only in Black 2 or White 2.
  - Their moves are the four latest level-up moves at that level.
  - This evidence is PKHeX-derived, not original-game.
- **Original-game cases use the Gen IV decompilations instead.** They judge Pokemon that came to Gen V by Poke Transfer.
  - pokeplatinum `generated/items.txt:2-17` lists Platinum's balls: Master Ball to Cherish Ball. It has no Dream Ball and no Sport Ball.
  - pokeheartgold `include/constants/items.h:270-285` lists the same 16 balls. Lines 913-921 add the Apricorn balls, the Sport Ball (499) and the Park Ball. No Gen IV game has a Dream Ball.
  - So a Platinum or HeartGold Pokemon in a Dream Ball is Invalid. A Platinum Pokemon in a Sport Ball is Invalid. A SoulSilver Pokemon in a Sport Ball is fine, and stays Incomplete only because its Gen IV history is not checked.
  - These cases do not depend on the Gen V title. The record does not say which Gen V game received it.
- **Unknown cases.** The Gen V analyzer does not check a transfer's original Gen III or IV encounter or moves. So those records read Incomplete, never "No problems found".

## PKHeX differentials

`dotnet` 10.0.112 was available, so all three differentials were run here.

### Gen I and Gen II: `tools/pkhex_gb_diff/run.sh`

```
tools/pkhex_gb_diff/run.sh /home/user/ext/pkhex /home/user/ext/pokered 20000 1 1   # Gen I, exit 0
tools/pkhex_gb_diff/run.sh /home/user/ext/pkhex /home/user/ext/pokered 20000 1 2   # Gen II, exit 0
```

| | Gen I (PK1) | Gen II (PK2) |
|---|---|---|
| Records | 20,000 | 20,000 |
| Same verdict, legal | 14,975 | 15,332 |
| Same verdict, invalid | 4,355 | 3,945 |
| Differing | 0 | 0 |
| Deliberately uncertain | 440 (430 divided stat exp, 10 event-only Mew) | 513 |
| Deliberately different | 230 (level against EXP) | 210 |
| Production parity through `Legality::analyze` | 20,000: legal 14,975, illegal 4,585, uncertain 440. PASS | 18,291 (1,709 Korean skipped): legal 14,385, illegal 3,748, uncertain 158. PASS |

These numbers cannot be split by title.

- A PK1 record does not carry Red, Blue or Yellow.
- PKHeX names only the first encounter that matches.
- The corpus kinds (wild, static, trade, grown, defect...) are not per title.
- Gold and Silver records do not carry their game either.
- The Gen II corpus mixes Gold, Silver and Crystal records in the same kinds.

### Gen III: `tools/pkhex_gen34_diff/run.sh`

```
tools/pkhex_gen34_diff/run.sh /home/user/ext/pkhex 1 3 3          # seed 1, 3 mutations, Gen III: exit 0
GROW=1 tools/pkhex_gen34_diff/run.sh /home/user/ext/pkhex 1 3 3   # 3 grown copies of each valid base: exit 0
```

The driver loads each record through `Pokemon3FRLG` and calls `Legality::analyze` with the id of the game the record was made for. That is the title, so this run splits by title.

Mutation run, 28,836 records:

| Title | Records | PKHeX VALID: NP / Inc | PKHeX INVALID: Inv / Inc | False Valid | False Invalid |
|---|---|---|---|---|---|
| Ruby | 5,736 | 2,725 / 120 | 2,891 / 0 | 0 | 0 |
| Sapphire | 5,296 | 2,568 / 113 | 2,614 / 1 | 0 | 0 |
| Emerald | 5,516 | 2,710 / 95 | 2,710 / 1 | 0 | 0 |
| FireRed | 6,132 | 2,991 / 114 | 3,025 / 2 | 0 | 0 |
| LeafGreen | 6,156 | 2,941 / 134 | 3,081 / 0 | 0 | 0 |
| All | 28,836 | 13,935 / 576 | 14,321 / 4 | 0 | 0 |

- No record failed to load.
- The 4 PKHeX-invalid records that read Incomplete all hold an Enigma Berry. That is the deliberate Unknown above.
- Base records only: 7,208 are PKHeX VALID. 7,019 read "No problems found" and 189 read Incomplete (Ruby 1,401 / 32, Sapphire 1,291 / 33, Emerald 1,343 / 36, FireRed 1,494 / 39, LeafGreen 1,490 / 49).
- One base record is PKHeX INVALID: a Ruby PCNY gift Flygon whose ability bit does not match its PID. It reads Invalid.

Grown run, 19,241 records, all PKHeX VALID:

| Title | Records | NP | Inc | False Invalid |
|---|---|---|---|---|
| Ruby | 3,850 | 3,779 | 71 | 0 |
| Sapphire | 3,663 | 3,575 | 88 | 0 |
| Emerald | 3,746 | 3,681 | 65 | 0 |
| FireRed | 3,994 | 3,930 | 64 | 0 |
| LeafGreen | 3,988 | 3,935 | 53 | 0 |
| All | 19,241 | 18,900 | 341 | 0 |

### Gen V: `tools/pkhex_gen5_diff/run.sh`

```
tools/pkhex_gen5_diff/run.sh /home/user/ext/pkhex 1 4    # seed 1, 4 mutations per record
```

The run used `GEN5_DIFF_CORPUS=<saved pkhex.txt> SHOW=100000`. The large `SHOW` worked around an early exit that is now fixed (see Findings).

It covered 115,035 records:

| PKHeX verdict | PokeBank NX verdict | Records |
|---|---|---|
| VALID | No problems found | 35,417 |
| VALID | Incomplete | 21,233 |
| INVALID | Invalid | 54,534 |
| INVALID | Incomplete | 3,851 |

- False Valid: 0. False Invalid: 0.
- Text checks: casing 47,748 units, 0 mismatches. Word-filter lists: 0 mismatches. Word filter: 0 mismatches. Gen III/IV fixed names: 0 missing.

The Gen V record carries its title, so this run can be split by title. The split below uses the version byte of each record. It covers native, in-game trade and event records:

| Title | Records | PKHeX VALID: NP / Inc | PKHeX INVALID: Inv / Inc | PKHeX-valid native / trade / event records certified NP |
|---|---|---|---|---|
| Black | 16,290 | 8,239 / 9 | 7,973 / 69 | 6,862 of 6,870 / 177 of 177 / 1,200 of 1,201 |
| White | 13,718 | 6,895 / 4 | 6,763 / 56 | 5,840 of 5,844 / 130 of 130 / 925 of 925 |
| Black 2 | 20,696 | 10,549 / 5 | 10,088 / 54 | 8,557 of 8,561 / 1,137 of 1,137 / 855 of 856 |
| White 2 | 18,954 | 9,734 / 5 | 9,163 / 52 | 8,123 of 8,128 / 683 of 683 / 928 of 928 |

Two groups of records have no Gen V title:

- **Transfers (44,690 records).** PKHeX VALID 21,210: all Incomplete. PKHeX INVALID 23,480: 19,860 Invalid, 3,620 Incomplete.
- **Mutated version bytes (687 records).** These are Gen V records whose version byte the mutator changed to a Gen III or IV game. All are Invalid in both.

## Findings

- **No engine bug was found.** Every original-game case gave the verdict the pret data predicts.
  - That includes the Yellow starter Pikachu with catch rate $A3, the version-exclusive Gen I and Gen II slots, and the Crystal Odd Egg with its little-endian OT ID bytes.
  - For Gen III it includes the Ruby/Sapphire and FireRed/LeafGreen exclusives, the Emerald outbreak Seedot in Sapphire, the Navel Rock Lugia fateful flag and both Game Corner prizes.
  - No test expectation was changed to match the engine.
- **One Bad Egg makes the app refuse the whole GBA save.**
  - The strict reader stops when one box record fails its checksum (`PKSMGen3NativeAdapter.cpp:92-96`). `FRLGReadOnlyTrainer::populate` then fails (`src/Legacy/FRLGReadOnlyTrainer.cpp:190-194`), and `UI.cpp` does not open the save.
  - The games show such a record as a Bad Egg and carry on (pokeemerald `src/pokemon.c:3742-3746`, pokeruby `src/pokemon_2.c:326`).
  - This is not a legality bug: nothing is certified. It is save-reader behavior and was not changed here.
- **The Ruby and Sapphire roamers in the corpus read Incomplete.**
  - PKHeX made both roaming Latias and Latios with all IVs 0. PKHeX flags that as Fishy (IVs:IVAllEqual_0), and the engine reports the Warning "All IVs are 0".
  - So the roamers appear here only in a title-specific Invalid case.
- **"Possible origin" names the first game that fits, not the save's title.**
  - A Yellow-save Caterpie reads "wild Caterpie in Red". A Silver-save Hoothoot reads "wild Hoothoot in Gold".
  - This is correct: a Pokemon can be traded between these games. The report already says that a legitimate origin does not prove where the Pokemon came from.
- **The Gen V differential script could stop early. This is now fixed.**
  - `tools/pkhex_gen5_diff/run.sh` runs `set -o pipefail`, and it used to pipe a long listing into `head`. With a short `SHOW`, `head` closed the pipe and the script exited 141 before its false-Invalid check.
  - Campaign commit 66a15e68 replaced `head` with `awk` (lines 75, 80 and 88), so a short `SHOW` no longer ends the run.
- **The brief's title counts.** The brief said 11 Gen I/II/V titles and 9 Gen III/IV titles. The list has 10 of each, and all 20 rows are here.
