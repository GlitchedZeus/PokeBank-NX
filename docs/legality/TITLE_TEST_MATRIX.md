# Per-title legality test matrix

This page lists the acceptance test for each of the 20 mainline titles from Generations I to V.
The test is `tests/test_legality_title_matrix.cpp`. It covers the Gen I, Gen II and Gen V titles.
The Gen III and Gen IV analyzers are being rewritten, so their 10 titles are still to do.

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

Gen V records are analyzed twice: once decrypted and once in their stored (encrypted) form.
Both must give the same report.

## Which path the app calls

| Generation | Path under test |
|---|---|
| Gen I | Synthetic 32 KiB save, then `Gen1::parse(bytes, title)`, then `Pokemon::Pokemon1ReadOnly`, then `Legality::analyze(pk, pk.getGameGroup(), metadata().sourceGameId)`. This is the call in `src/UI/Modals/Gen1PokemonDetailsModal.cpp:72`. |
| Gen II | Synthetic 32 KiB save, then `Gen2::parse(bytes, title)`, then `Pokemon::Pokemon2ReadOnly`, then the same `Legality::analyze` call. This is the call in `src/UI/Modals/Gen2PokemonDetailsModal.cpp:235`. |
| Gen V | `Legality::analyzePk5(bytes, Pk5Input)` on a 136-byte PK5 record. The app on this branch has no PK5 save reader yet: it is in PR #307. So the app does not call `analyzePk5` yet, and the test calls the engine API directly. |

Every Gen I and Gen II case goes end to end, from save bytes to verdict.
The source game ids are the ones the parsers write: `red_gb`, `blue_gb`, `yellow_gb`, `gold_gbc`, `silver_gbc`, `crystal_gbc`.

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
| Ruby | pending Gen III/IV analyzer merge | | | | | | | |
| Sapphire | pending Gen III/IV analyzer merge | | | | | | | |
| Emerald | pending Gen III/IV analyzer merge | | | | | | | |
| FireRed | pending Gen III/IV analyzer merge | | | | | | | |
| LeafGreen | pending Gen III/IV analyzer merge | | | | | | | |
| Diamond | pending Gen III/IV analyzer merge | | | | | | | |
| Pearl | pending Gen III/IV analyzer merge | | | | | | | |
| Platinum | pending Gen III/IV analyzer merge | | | | | | | |
| HeartGold | pending Gen III/IV analyzer merge | | | | | | | |
| SoulSilver | pending Gen III/IV analyzer merge | | | | | | | |
| Black | `analyzePk5` (no app reader yet) | Cottonee L14, Pinwheel Forest, Black-only slot: NP | Fateful-encounter flag set: Inv ("Fateful encounter flag is set on a non-event encounter") | Record cut to 135 bytes: Inc (Warning "Not a 136-byte or 220-byte PK5 record", structure not read) | Diamond Caterpie, transferred: Inc (Warning "Gen III/IV origin: the original encounter and moves are not checked") | Platinum Caterpie in a Dream Ball: Inv ("The Dream Ball did not exist in Gen IV") | Same Cottonee with version byte White: Inv ("No Gen V encounter gives Cottonee at met location 33"). N's Purrloin with version byte Black: Inv | 16,290 records, 0 false Valid, 0 false Invalid |
| White | `analyzePk5` (no app reader yet) | Petilil L14, Pinwheel Forest, White-only slot: NP | Met level 13 on a level-14 slot: Inv ("Closest Gen V encounter ... does not match") | Stored form with one encrypted byte flipped: Inv ("PK5 checksum does not match its data") | FireRed Weedle, transferred: Inc | HeartGold Caterpie in a Dream Ball: Inv ("The Dream Ball did not exist in Gen IV") | Same Petilil with version byte Black: Inv ("No Gen V encounter gives Petilil at met location 33") | 13,718 records, 0 false Valid, 0 false Invalid |
| Black 2 | `analyzePk5` (no app reader yet) | Cottonee L15, Castelia City, Black 2-only slot: NP | Knows Surf: Inv ("Surf cannot be learned by this Pokemon in Gen V") | Stored form with two encrypted blocks swapped: Inv (checksum). Decrypted blocks A and B swapped: Inv (move ids above 559 and more) | HeartGold Caterpie, transferred: Inc | Platinum Caterpie in a Sport Ball: Inv ("Ball 24 exists only in HeartGold / SoulSilver") | Same Cottonee with version byte White 2: Inv. N's Purrloin (a B2W2 gift): NP | 20,696 records, 0 false Valid, 0 false Invalid |
| White 2 | `analyzePk5` (no app reader yet) | Petilil L15, Castelia City, White 2-only slot: NP | N's sparkle flag set: Inv ("N's sparkle flag is set on a Pokemon that is not N's") | Species id 0xFFFF: Inv ("Species 65535 is above the Gen V maximum 649"). Nickname and OT all U+0001: Inv (names too long) | LeafGreen Caterpie, transferred: Inc | SoulSilver Caterpie in a Sport Ball: Inc, not Invalid | Same Petilil with version byte Black 2: Inv. N's Purrloin with version byte White 2: NP | 18,954 records, 0 false Valid, 0 false Invalid |

Cases per title (73 in all):

| Title | Positive | Negative | Malformed | Unknown | Original-game | Title-specific | Total |
|---|---|---|---|---|---|---|---|
| Red | 1 | 1 | 2 | 1 | 1 | 1 | 7 |
| Blue | 1 | 1 | 2 | 1 | 1 | 1 | 7 |
| Yellow | 1 | 1 | 2 | 1 | 1 | 2 | 8 |
| Gold | 1 | 1 | 2 | 1 | 1 | 1 | 7 |
| Silver | 1 | 1 | 2 | 1 | 1 | 1 | 7 |
| Crystal | 1 | 1 | 2 | 1 | 1 | 2 | 8 |
| Black | 1 | 1 | 1 | 1 | 1 | 2 | 7 |
| White | 1 | 1 | 1 | 1 | 1 | 1 | 6 |
| Black 2 | 1 | 1 | 2 | 1 | 1 | 2 | 8 |
| White 2 | 1 | 1 | 2 | 1 | 1 | 2 | 8 |

## Evidence for each case

Pins: pret pokered `af519899719f0754965776faac0e836a3b906e6d`, pokeyellow `e89ead154b9968aa50eed9328ff2b38b6c194382`, pokegold `ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8`, pokecrystal `3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6`, pokeplatinum `1dc13dcea40c04ab53f1f607de46023dc409ef20`, pokeheartgold `d2355244ac42b3c6e40d8c36aa3564ee8706dea4`. PKHeX `kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722`.

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

`dotnet` 10.0.112 was available, so both differentials were run here.

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

### Gen V: `tools/pkhex_gen5_diff/run.sh`

```
tools/pkhex_gen5_diff/run.sh /home/user/ext/pkhex 1 4    # seed 1, 4 mutations per record
```

The run used `GEN5_DIFF_CORPUS=<saved pkhex.txt> SHOW=100000` (see Findings).

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
  - No test expectation was changed to match the engine.
- **"Possible origin" names the first game that fits, not the save's title.**
  - A Yellow-save Caterpie reads "wild Caterpie in Red". A Silver-save Hoothoot reads "wild Hoothoot in Gold".
  - This is correct: a Pokemon can be traded between these games. The report already says that a legitimate origin does not prove where the Pokemon came from.
- **The Gen V differential script can stop early.**
  - `tools/pkhex_gen5_diff/run.sh` runs `set -o pipefail`. Line 80 pipes a long listing into `head -"${SHOW:-15}"`.
  - With a short `SHOW` (5 here), `head` closed the pipe and the script exited 141 before its false-Invalid check.
  - With `SHOW=100000` it ran to the end and passed. The default of 15 could hit the same problem.
  - This was not changed here.
- **The brief's title counts.** The brief said 11 Gen I/II/V titles and 9 Gen III/IV titles. The list has 10 of each, and all 20 rows are here.
