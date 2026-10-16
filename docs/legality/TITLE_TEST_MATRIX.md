# Per-title legality test matrix

This page lists the acceptance test for each of the 20 mainline titles from Generations I to V.
The test is `tests/test_legality_title_matrix.cpp`. It covers all 20 titles.

Run it on its own:

```
make -f Makefile.host build-host/test_legality_title_matrix
./build-host/test_legality_title_matrix       # one line per case
./build-host/test_legality_title_matrix -v    # also prints every report
```

It also runs in `make -f Makefile.host host-test` and `host-sanitize` (`Makefile.host.title-matrix`).
It prints one line per case: title, kind, case, expected verdict, actual verdict, PASS, FAIL or BUG.
It exits non-zero if any case fails.

A BUG row is a known engine bug.
The row keeps the verdict the evidence predicts, and the engine still gives the documented wrong verdict.
A BUG row does not fail the run, but it is counted and listed under Findings.
When the bug is fixed, the row passes and prints a note to drop the mark.
If the engine gives any other verdict, the row fails.

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

Gen IV record and save cases run `Legality::analyzePk4` on the bytes the production entity decrypted, with the same rule.
They also check what the save reader does with the record: it shows a record that passes `Pokemon4ReadOnly::valid()` and hides one that does not.

Gen V records are analyzed twice: once decrypted and once in their stored (encrypted) form.
Both must give the same report.

## Which path the app calls

| Generation | Path under test |
|---|---|
| Gen I | Synthetic 32 KiB save, then `Gen1::parse(bytes, title)`, then `Pokemon::Pokemon1ReadOnly`, then `Legality::analyze(pk, pk.getGameGroup(), metadata().sourceGameId)`. This is the call in `src/UI/Modals/Gen1PokemonDetailsModal.cpp:72`. |
| Gen II | Synthetic 32 KiB save, then `Gen2::parse(bytes, title)`, then `Pokemon::Pokemon2ReadOnly`, then the same `Legality::analyze` call. This is the call in `src/UI/Modals/Gen2PokemonDetailsModal.cpp:235`. |
| Gen III | Synthetic 128 KiB save, then `Integration::Gen3::parse(bytes, sourceGame)` (the on-device reader, `PKSMGen3NativeAdapter.cpp`), then `FRLGReadOnlyTrainer::create` as in `src/UI/UI.cpp:393`, then box 1 slot 1 (a `Pokemon3FRLG`) through `Legality::analyze(pk, pk.getGameGroup(), metadata().sourceGameId)`. This is the call in `src/UI/Modals/PokemonDetailsModal.cpp:65`. Record cases put the stored (encrypted) PK3 into `Pokemon3FRLG` and make the same call with the exact source game id. |
| Gen IV | Synthetic 512 KiB save, then `Gen4ReadOnlySave::parse(bytes, layout, gameId)` for every layout, as `openAssignedSource` does (`src/Integration/Gen4/Gen4AssignedSource.cpp:31-54`). Exactly one layout must validate, and the assignment must match. Then `Gen4ReadOnlyTrainer::create` as in `src/UI/UI.cpp:447`. Box 1 slot 1 is read as `boxRecord` does (`src/UI/Gen4SharedPokemonSurface.inc:163-181`: the staged editor's copy when there is one). Then `Pokemon4ReadOnlyView` and `Legality::analyze(view, view.getGameGroup(), sourceGameId)`. This is the call in `src/UI/Gen4SharedPokemonSurface.inc:1862-1863`. Record cases put the stored (encrypted) PK4 into `Pokemon4ReadOnly` and make the same call. |
| Gen V | `Legality::analyzePk5(bytes, Pk5Input)` on a 136-byte PK5 record. The app on this branch has no PK5 save reader yet: it is in PR #307. So the app does not call `analyzePk5` yet, and the test calls the engine API directly. |

Every Gen I and Gen II case goes end to end, from save bytes to verdict.
The source game ids are the ones the parsers write: `red_gb`, `blue_gb`, `yellow_gb`, `gold_gbc`, `silver_gbc`, `crystal_gbc`.

Gen III has 13 cases from save bytes: one positive and one trade per title, a Bad Egg, a Colosseum/XD record and a save opened as the wrong family.
The other Gen III cases are record cases. The ids are `ruby_gba`, `sapphire_gba`, `emerald_gba`, `firered_gba`, `leafgreen_gba`.

Gen IV has 13 cases from save bytes: one positive per title, three trades, a Rotom-Wash, a Bad Egg in a box, a save with a broken Storage CRC, and two saves assigned to the wrong game.
The other Gen IV cases are record cases. The ids are `diamond_nds`, `pearl_nds`, `platinum_nds`, `heartgold_nds`, `soulsilver_nds`.
Each synthetic save has one General and one Storage block in partition 0, with the games' footer (save counter, size, magic 0x20060623 and CRC-16/CCITT). Partition 1 is erased flash.

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
- **Diamond, Pearl, Platinum, HeartGold, SoulSilver.** The PK4 record has a version byte at 0x5F: 7 HeartGold, 8 SoulSilver, 10 Diamond, 11 Pearl, 12 Platinum. Pal Park keeps the Gen III value (1-5, or 15 for Colosseum/XD).
  - The Gen IV analyzer matches encounters for that game. So a Pokemon traded in from another Gen IV game reads the same in any Gen IV save, and the same bytes with another version can be Invalid.
  - The record also shows which family wrote it. Platinum and HG/SS write the met location twice: the real value at 0x46, and at 0x80 either the same value or 3002 when D/P have no such place. D/P write only 0x80. HG/SS write a second ball byte at 0x86. The Apricorn and Sport Balls exist only there, with a Poke Ball at 0x83.
  - The save's title picks the personal table. D/P have no Rotom, Giratina or Shaymin forms. So a Platinum Rotom-Wash or Giratina-Origin read in a D/P save is Invalid.
  - The save bytes tell three families apart by their block layout. D/P saves do not say which of the two they are: the file's assignment picks the title. HG/SS saves hold a ROM code (7 or 8) at Trainer + 0x1C, and it must match the assignment. A save that does not match its assignment is refused.
- **Black, White, Black 2, White 2.** The record has a version byte at 0x5F: 20 White, 21 Black, 22 White 2, 23 Black 2.
  - Encounter matching uses it, so the same bytes can be legal in one title and Invalid in its pair.
  - A Pokemon transferred from Gen III or IV keeps its old version byte. Nothing in the record says which Gen V game received it.
  - The Gen IV analyzer's origin analysis matches a transfer's original Gen III or IV encounter and moves, so a transfer can read "No problems found" in any Gen V title.

## The matrix

Verdicts are written as: NP = "No problems found", Inv = Invalid, Inc = Incomplete, Rej = parser rejects the save (error name in brackets), BUG = a known engine bug row (see Findings).

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
| Diamond | NDS save, `Gen4ReadOnlySave::parse` (D/P layout), Gen4ReadOnlyTrainer, Pokemon4ReadOnlyView, `analyze(..., "diamond_nds")` | Murkrow L11, Eterna Forest, a Diamond-only night slot, from save bytes: NP. Dialga L47, Spear Pillar: NP | Level Ball in the HG/SS ball byte: Inv ("Ball (#18) is not possible for this encounter", "HG/SS ball value does not match the D/P/Pt ball value") | Box record with a bad checksum (a Bad Egg): Rej (selected Gen IV PK4 failed strict validation). Record cut to 135 bytes (`analyzePk4`): Inv ("PK4 record must be 136 or 236 bytes", structure not read) | Colosseum Umbreon through Pal Park: Inc (Warning "Pal Park transfer from Colosseum/XD: the Colosseum/XD encounter is not verified") | Murkrow met at L12 in Eterna Forest (Diamond has L10-11): Inv ("No matching Gen IV encounter for this species, level, location, ball and PID/IV") | Same Murkrow with version Pearl: Inv. Pearl Misdreavus traded into a Diamond save: NP. Platinum Rotom-Wash read in a Diamond save: Inv ("Form 2 is out of range") | 9,040 records, 0 false Valid, 0 false Invalid. Grown: 14,384, 0 false Invalid |
| Pearl | NDS save, `Gen4ReadOnlySave::parse` (D/P layout), Gen4ReadOnlyTrainer, Pokemon4ReadOnlyView, `analyze(..., "pearl_nds")` | Misdreavus L11, Eterna Forest, a Pearl-only night slot, from save bytes: NP. Palkia L47, Spear Pillar: NP | Palkia with the fateful flag: Inv ("Fateful encounter flag is set on a non-event encounter") | One stored data bit flipped (a Bad Egg): Inc, and the reader hides it. Species 494: Inv ("Species #494 does not exist in Generation IV") | XD Eevee through Pal Park: Inc | Dustox L12, Eterna Forest (Pearl slots 10-11): NP | Same Misdreavus with version Diamond: Inv. Platinum Giratina-Origin read in a Pearl save: Inv ("Form 1 is out of range"). D/P save assigned to Platinum: Rej (assigned game/family does not match) | 8,708 records, 0 false Valid, 0 false Invalid. Grown: 14,844, 0 false Invalid |
| Platinum | NDS save, `Gen4ReadOnlySave::parse` (Pt layout), Gen4ReadOnlyTrainer, Pokemon4ReadOnlyView, `analyze(..., "platinum_nds")` | Giratina-Origin L47, Distortion World, from save bytes: NP. FireRed Bulbasaur through Pal Park: NP | Pal Park Bulbasaur with met location 16: Inv ("A Generation III Pokemon must have been met at Pal Park") | Origin game 0: Inv ("Origin game #0 cannot produce a PK4 (not a Gen III or Gen IV game)"). Record padded to 237 bytes (`analyzePk4`): Inv (structure not read) | Event Arceus that knows Draco Meteor: Inc (Warning "Draco Meteor: Arceus can learn it only from the Pt/HG/SS tutor while holding a Draco Plate") | Rotom met at L15 in the Old Chateau (the script gives L20): Inv. Giratina-Origin with 117 in the D/P met-location field: BUG (expected Inv, engine NP) | Rotom-Wash from save bytes: NP. Turnback Cave Giratina L47 with version Diamond (D/P give L70): Inv. HeartGold Spinarak traded into a Platinum save: NP | 8,525 records, 0 false Valid, 0 false Invalid. Grown: 14,364, 0 false Invalid |
| HeartGold | NDS save, `Gen4ReadOnlySave::parse` (HG/SS layout, ROM code 7), Gen4ReadOnlyTrainer, Pokemon4ReadOnlyView, `analyze(..., "heartgold_nds")` | Spinarak L2, Route 30, a HeartGold-only night slot, from save bytes: NP | Great Ball at 0x83 and Poke Ball at 0x86: Inv ("HG/SS ball value does not match the D/P/Pt ball value") | Storage block with a bad CRC: Rej (No validated Gen IV General/Storage layout) | Colosseum Shadow Quilava through Pal Park: Inc | Ho-Oh L45, Bell Tower: NP. Ho-Oh L45 with version SoulSilver (SoulSilver gives L70): Inv. Spinarak with 178 in the D/P met-location field: BUG (expected Inv, engine NP) | Spinarak with version SoulSilver: Inv. Spinarak in a Level Ball: NP. Pokewalker Pikachu: NP. SoulSilver save (ROM code 8) assigned to HeartGold: Rej (assigned game/family does not match) | 12,411 records, 0 false Valid, 0 false Invalid. Grown: 16,484, 0 false Invalid |
| SoulSilver | NDS save, `Gen4ReadOnlySave::parse` (HG/SS layout, ROM code 8), Gen4ReadOnlyTrainer, Pokemon4ReadOnlyView, `analyze(..., "soulsilver_nds")` | Ledyba L3, Route 30, a SoulSilver-only morning slot, from save bytes: NP | Lugia with Shiny Leaf byte 32: Inv ("Shiny Leaf value is not obtainable") | Sanity word 4 (a Bad Egg): Inc, and the reader hides it | XD Shadow Ledyba through Pal Park: Inc | Lugia L45, Whirl Islands: NP. Lugia L45 with version HeartGold (HeartGold gives L70): Inv. Ledyba with 178 in the D/P met-location field: BUG (expected Inv, engine NP) | Ledyba with version HeartGold: Inv. Sport Ball Caterpie from the Bug-Catching Contest: NP. Pokewalker Pikachu with version Platinum: Inv. Platinum Rotom traded into a SoulSilver save: NP | 12,315 records, 0 false Valid, 0 false Invalid. Grown: 16,292, 0 false Invalid |
| Black | `analyzePk5` (no app reader yet) | Cottonee L14, Pinwheel Forest, Black-only slot: NP. Diamond Caterpie, transferred: NP (Info "Gen III/IV origin matches a Gen IV wild encounter (Caterpie)") | Fateful-encounter flag set: Inv ("Fateful encounter flag is set on a non-event encounter") | Record cut to 135 bytes: Inc (Warning "Not a 136-byte or 220-byte PK5 record", structure not read) | XD Shadow Butterfree, transferred: Inc (Warning "Colosseum / XD origin: the original encounter and moves are not checked") | Platinum Caterpie in a Dream Ball: Inv ("The Dream Ball did not exist in Gen IV") | Same Cottonee with version byte White: Inv ("No Gen V encounter gives Cottonee at met location 33"). N's Purrloin with version byte Black: Inv | 16,466 records, 0 false Valid, 0 false Invalid |
| White | `analyzePk5` (no app reader yet) | Petilil L14, Pinwheel Forest, White-only slot: NP. FireRed Weedle, transferred: NP | Met level 13 on a level-14 slot: Inv ("Closest Gen V encounter ... does not match") | Stored form with one encrypted byte flipped: Inv ("PK5 checksum does not match its data") | Sapphire egg on a TID/SID with no Ruby/Sapphire seed: Inc (Warning, PKHeX Fishy) | HeartGold Caterpie in a Dream Ball: Inv ("The Dream Ball did not exist in Gen IV") | Same Petilil with version byte Black: Inv ("No Gen V encounter gives Petilil at met location 33") | 13,793 records, 0 false Valid, 0 false Invalid |
| Black 2 | `analyzePk5` (no app reader yet) | Cottonee L15, Castelia City, Black 2-only slot: NP. HeartGold Caterpie, transferred: NP | Knows Surf: Inv ("Surf cannot be learned by this Pokemon in Gen V") | Stored form with two encrypted blocks swapped: Inv (checksum). Decrypted blocks A and B swapped: Inv (move ids above 559 and more) | LeafGreen Caterpie on no reachable Gen III RNG frame: Inc (Warning, PKHeX Fishy) | Platinum Caterpie in a Sport Ball: Inv ("Ball 24 exists only in HeartGold / SoulSilver") | Same Cottonee with version byte White 2: Inv. N's Purrloin (a B2W2 gift): NP | 20,768 records, 0 false Valid, 0 false Invalid |
| White 2 | `analyzePk5` (no app reader yet) | Petilil L15, Castelia City, White 2-only slot: NP. LeafGreen Caterpie, transferred: NP | N's sparkle flag set: Inv ("N's sparkle flag is set on a Pokemon that is not N's") | Species id 0xFFFF: Inv ("Species 65535 is above the Gen V maximum 649"). Nickname and OT all U+0001: Inv (names too long) | Colosseum Shadow Bayleef, transferred: Inc | SoulSilver Caterpie in a Sport Ball (Bug-Catching Contest): NP | Same Petilil with version byte Black 2: Inv. N's Purrloin with version byte White 2: NP | 18,795 records, 0 false Valid, 0 false Invalid |

Cases per title (175 in all, 3 of them BUG rows):

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
| Diamond | 2 | 1 | 2 | 1 | 1 | 3 | 10 |
| Pearl | 2 | 1 | 2 | 1 | 1 | 3 | 10 |
| Platinum | 2 | 1 | 2 | 1 | 2 | 3 | 11 |
| HeartGold | 1 | 1 | 1 | 1 | 3 | 4 | 11 |
| SoulSilver | 1 | 1 | 1 | 1 | 3 | 4 | 11 |
| Black | 2 | 1 | 1 | 1 | 1 | 2 | 8 |
| White | 2 | 1 | 1 | 1 | 1 | 1 | 7 |
| Black 2 | 2 | 1 | 2 | 1 | 1 | 2 | 9 |
| White 2 | 2 | 1 | 2 | 1 | 1 | 2 | 9 |

## Evidence for each case

Pins: pret pokered `af519899719f0754965776faac0e836a3b906e6d`, pokeyellow `e89ead154b9968aa50eed9328ff2b38b6c194382`, pokegold `ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8`, pokecrystal `3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6`, pokeruby `5784633ce4ef7ade1a7f2d2d0c288e3d5e6cdd7f`, pokeemerald `731ad5bfd6e6f265508d0efcca0ba42f9dcf5881`, pokefirered `037335f4c725d7c9aecdac87066f2002b4bd7e14`, pokediamond `b623abef1b4dfe204ca50aab42447ac00c134290`, pokeplatinum `1dc13dcea40c04ab53f1f607de46023dc409ef20`, pokeheartgold `d2355244ac42b3c6e40d8c36aa3564ee8706dea4`. PKHeX `kwsch/PKHeX@6501f0ab46e8f8ca048539dbaf8cae8cb104e722`.

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

### Diamond, Pearl, Platinum, HeartGold, SoulSilver

The base records come from the PKHeX Gen III/IV corpus (`tools/pkhex_gen34_diff`, seed 1, Gen IV). PKHeX calls each one VALID with the personal table of the save that holds it. The test names each record's corpus id.
The four Colosseum/XD records come from PKHeX's Colosseum/XD encounter list. The oracle built each one and converted it the way Pal Park does (CK3 or XK3, then PK3, then PK4). PKHeX calls each one VALID.

Every edited record was also checked with the oracle's probe, with the personal table of the save:

- PKHeX agrees with every Invalid and every "No problems found" in the matrix. In the three BUG rows PKHeX also says VALID.
- PKHeX calls the Arceus Draco Meteor record INVALID. The engine keeps it Incomplete on purpose: the tutor checks the Pokemon's current type, and Arceus is a Dragon type while it holds the Draco Plate.
- PKHeX calls the sanity-word record VALID, because its legality check does not read the sanity word. The games show that record as a Bad Egg, so the engine keeps it Incomplete.

The evidence for each title:

- **Diamond and Pearl positive, original-game and title-specific.** pokediamond `arm9/src/map_header.c:20` picks the Diamond or Pearl encounter file by `GAME_VERSION`. Line 287 gives Eterna Forest `narc_0008`.
  - `files/fielddata/encountdata/d_enc_data/narc_0008.bin` holds 198 (Murkrow) at 0x74 and 0x78. These are the night species for grass slots 2 and 3. `p_enc_data/narc_0008.bin` holds 200 (Misdreavus) there.
  - Slot 2 (0x14) is L11 and slot 3 (0x1C) is L10. pokeplatinum `src/overlay006/wild_encounters.c:182-191` puts the night species into those two slots, using the layout in `include/overlay006/wild_encounters.h:31-48`.
  - Slots 10 and 11 are L12. They hold 267 (Beautifly) in Diamond and 269 (Dustox) in Pearl.
  - So Murkrow L11 is a Diamond-only record, and Murkrow met at L12 is not possible. Misdreavus L11 is Pearl-only, and Dustox L12 is legal in Pearl.
  - `include/config.h:9-10` sets Diamond to 10 and Pearl to 11. `arm9/src/pokemon.c:291` writes that value as the record's version.
- **Dialga and Palkia.** Both are L47 at Spear Pillar (location 51, pokediamond `include/constants/map_sections.h:55`). They are PKHeX corpus records.
- **Forms in a D/P save.** pokediamond `arm9/src/pokemon.c:3400-3413` (`ResolveMonForm`) has personal data for the Deoxys and Wormadam forms only. pokeplatinum `include/constants/forms.h:87` makes Rotom-Wash form 2.
  - pokeplatinum `src/overlay005/ov5_021F6454.c:451-509` turns Giratina and Rotom back to their base forms and takes back the Griseous Orb. `res/field/scripts/scripts_pokemon_center_2f_common.s:409` runs it before the Union Room.
  - So a Rotom-Wash or a Giratina-Origin cannot reach a D/P save by trade. Both read Invalid there.
- **Platinum positive, original-game and title-specific.** pokeplatinum `res/field/scripts/scripts_distortion_world_giratina_room.s:69` is `StartGiratinaOriginBattle SPECIES_GIRATINA, 47`.
  - `scripts_turnback_cave_giratina_room.s:35` is `StartLegendaryBattle SPECIES_GIRATINA, 47`. D/P give Giratina at L70 there (PKHeX `Encounters4DPPt.cs:57`, against line 90 for Platinum). So the L47 Giratina with version Diamond is Invalid.
  - `scripts_old_chateau_back_middle_west_room.s:24` is `StartWildBattle SPECIES_ROTOM, 20`. So a Rotom met at L15 is Invalid.
  - `scripts_rotoms_room.s` changes Rotom's form in Platinum. So a Rotom-Wash in a Platinum save is legal.
- **The D/P met-location field (the BUG rows).** pokeplatinum `src/pokemon.c:1733-1739` writes the met location to 0x46. It writes 3002 (`FATEFUL_ENCOUNTER_LOCATION`, line 59) to 0x80, unless D/P know that place.
  - `src/unk_02017038.c:38-45` lists the places D/P know: 1-111, 2000-2010 and 3000-3076. Distortion World is 117 (`res/text/location_names.json:473-474`).
  - pokeheartgold `src/pokemon.c:1286-1292` does the same with `METLOC_FARAWAY_PLACE` (3002, `include/constants/map_sections.h:258`). `asm/unk_02017FAC.s:68-97` lists the places D/P know. Route 30 is 178 (`map_sections.h:182`).
  - So a Platinum record with 117 at 0x80, or an HG/SS record with 178 there, cannot come from the games. The engine and PKHeX both accept it.
- **Platinum unknown.** pokeplatinum `res/field/scripts/scripts_route_210_grandma_wilma_house.s:81-82` checks that the Pokemon's current type is Dragon. Lines 103-106 and 117 then teach Draco Meteor.
  - Arceus is a Dragon type only while it holds the Draco Plate. The record does not show what it held then.
- **Pal Park.** Pal Park is location 55 (pokediamond `include/constants/map_sections.h:59`). PKHeX's `TransferVerifier.VerifyTransferLegalityG3` requires it for a Gen III origin. So met location 16 is Invalid.
- **HeartGold and SoulSilver positive, original-game and title-specific.** pokeheartgold `files/fielddata/encountdata/gs_enc_data.json:512-532` is Route 30.
  - Slot 0 is L2 in HeartGold and L3 in SoulSilver.
  - At night HeartGold has Spinarak there and SoulSilver has Hoothoot. In the morning HeartGold has Pidgey and SoulSilver has Ledyba.
  - `files/fielddata/script/scr_seq/scr_seq_0021_D17R0110.s:57-72` makes Ho-Oh L45 when the version is HeartGold (7) and L70 otherwise.
  - `scr_seq_0104_D40R0107.s:69-84` makes Lugia L70 in HeartGold and L45 otherwise.
  - So Ho-Oh L45 with version SoulSilver is Invalid, and so is Lugia L45 with version HeartGold.
- **HG/SS balls.** pokeheartgold `src/pokemon.c:1298-1307` writes an Apricorn Ball or the Sport Ball (17-24, `include/constants/balls.h:25-33`) to 0x86, with a Poke Ball at 0x83. Other balls go to both bytes.
  - So a Level Ball in an HG/SS record is legal. A Level Ball byte in a Diamond record is not, and neither are two different normal balls.
  - The Bug-Catching Contest has Caterpie at L7-18 (`files/data/mushi/mushi_encount.csv:2`) in National Park (207, `map_sections.h:211`).
- **Pokewalker.** Met location 233 is the Pokewalker (pokeheartgold `include/constants/map_sections.h:237`). Platinum's location list ends at 125 (pokeplatinum `res/text/location_names.json:505`). So the Pokewalker Pikachu with version Platinum is Invalid.
- **Malformed.** The games show a record that fails its checksum or sanity word as a Bad Egg. The save reader hides it, and the engine never certifies it.
- **Trades.** Each trade case puts another Gen IV game's record into a save. Each reads "No problems found", because the record's origin game decides.

### Black, White, Black 2, White 2

Gen V has no disassembly. No pret project for Black, White, Black 2 or White 2 exists. Bulbapedia and Project Pokémon cannot be reached from this environment (`docs/legality/PKHEX_DISAGREEMENT_REVIEW.md`). So no original-game source exists here for native Gen V records.

- **Positive, negative and title-specific records.** These come from PKHeX's generator (`tools/pkhex_gen5_diff`, `oracle generate 1 4`). PKHeX calls each base record VALID.
  - The slots are version-exclusive in the Gen V encounter data (`Gen5LegalityData`): Cottonee in Pinwheel Forest only in Black, Petilil only in White, and the Castelia City slots only in Black 2 or White 2.
  - Their moves are the four latest level-up moves at that level.
  - This evidence is PKHeX-derived, not original-game.
- **Original-game cases use the Gen IV decompilations instead.** They judge Pokemon that came to Gen V by Poke Transfer.
  - pokeplatinum `generated/items.txt:2-17` lists Platinum's balls: Master Ball to Cherish Ball. It has no Dream Ball and no Sport Ball.
  - pokeheartgold `include/constants/items.h:270-285` lists the same 16 balls. Lines 913-921 add the Apricorn balls, the Sport Ball (499) and the Park Ball. No Gen IV game has a Dream Ball.
  - So a Platinum or HeartGold Pokemon in a Dream Ball is Invalid. A Platinum Pokemon in a Sport Ball is Invalid.
  - The Bug-Catching Contest has Caterpie at L7-18 (pokeheartgold `files/data/mushi/mushi_encount.csv:2`), and every contest throw is a Sport Ball (`src/battle/battle_controller_player.c:1865-1873`). So the SoulSilver Caterpie in a Sport Ball, transferred at L15, reads "No problems found".
  - These cases do not depend on the Gen V title. The record does not say which Gen V game received it.
- **Transfers.** The Gen IV analyzer's origin analysis (`Gen4::analyzeOrigin5`) matches the original Gen III or IV encounter and the moves of a transfer, as PKHeX does for a format-5 record. The four transferred Caterpie and Weedle records come from PKHeX's generator, and PKHeX calls each VALID. They read "No problems found" with an Info naming the matched encounter.
- **Unknown cases.** These transfers are still not decided here, so they read Incomplete, never "No problems found". PKHeX calls each VALID.
  - Colosseum and XD encounters (XD Shadow Butterfree, Colosseum Shadow Bayleef) are not modelled: Warning "Colosseum / XD origin: the original encounter and moves are not checked".
  - Two PKHeX "fishy" results stay Warnings: a Sapphire egg whose TID/SID PKHeX finds no Ruby/Sapphire seed for (`Trainer:TrainerIDNoSeed`), and a LeafGreen Caterpie whose Method 1 seed reaches no frame that gives its slot and level (`PID:EncConditionBadRNGFrame`).

## PKHeX differentials

`dotnet` 10.0.112 was available, so all four differentials were run here.

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

### Gen IV: `tools/pkhex_gen34_diff/run.sh`

```
tools/pkhex_gen34_diff/run.sh /home/user/ext/pkhex 1 3 4          # seed 1, 3 mutations, Gen IV: exit 0
tools/pkhex_gen34_diff/run.sh /home/user/ext/pkhex 1 3 4 grown    # played-forward records and Pal Park transfers: exit 0
```

The driver loads each record through `Pokemon4ReadOnlyView` and calls `Legality::analyze` with the save group and the id of the game the record was made for. That is the title, so this run splits by title.

Mutation run, 50,999 records:

| Title | Records | PKHeX VALID: NP / Inc | PKHeX INVALID: Inv / Inc | False Valid | False Invalid |
|---|---|---|---|---|---|
| Diamond | 9,040 | 4,152 / 0 | 4,888 / 0 | 0 | 0 |
| Pearl | 8,708 | 4,021 / 0 | 4,687 / 0 | 0 | 0 |
| Platinum | 8,525 | 3,984 / 0 | 4,541 / 0 | 0 | 0 |
| HeartGold | 12,411 | 5,478 / 0 | 6,933 / 0 | 0 | 0 |
| SoulSilver | 12,315 | 5,426 / 0 | 6,889 / 0 | 0 | 0 |
| All | 50,999 | 23,061 / 0 | 27,938 / 0 | 0 | 0 |

- No record failed to load.
- Base records only: Diamond 2,262 VALID and 241 INVALID, Pearl 2,179 and 241, Platinum 2,133 and 221, HeartGold 3,014 and 109, SoulSilver 2,982 and 117. Every VALID one reads "No problems found" and every INVALID one reads Invalid.

Grown run, 76,368 records:

| Title | Records | PKHeX VALID: NP / Inc | PKHeX INVALID: Inv / Inc | False Valid | False Invalid |
|---|---|---|---|---|---|
| Diamond | 14,384 | 7,197 / 36 | 7,151 / 0 | 0 | 0 |
| Pearl | 14,844 | 7,560 / 38 | 7,246 / 0 | 0 | 0 |
| Platinum | 14,364 | 7,240 / 53 | 7,071 / 0 | 0 | 0 |
| HeartGold | 16,484 | 8,193 / 63 | 8,228 / 0 | 0 | 0 |
| SoulSilver | 16,292 | 8,147 / 55 | 8,090 / 0 | 0 | 0 |
| All | 76,368 | 38,337 / 245 | 37,786 / 0 | 0 | 0 |

- The 245 PKHeX-valid records that read Incomplete are all ones PKHeX itself marks Fishy. The engine reports the same thing as a Warning. By Warning (7 records have two):
  - 126 Pal Park transfers whose PID and IVs match no reachable Gen III RNG frame;
  - 51 "PID is zero" and 33 "Secret ID is zero";
  - 28 whose Trainer ID and Secret ID Ruby/Sapphire cannot generate;
  - 14 "All IVs are equal".
- Every PKHeX-valid record that PKHeX does not mark Fishy reads "No problems found" (26,445 of 26,445).

### Gen V: `tools/pkhex_gen5_diff/run.sh`

```
tools/pkhex_gen5_diff/run.sh /home/user/ext/pkhex 1 4    # seed 1, 4 mutations per record
```

It covered 115,320 records:

| PKHeX verdict | PokeBank NX verdict | Records |
|---|---|---|
| VALID | No problems found | 55,885 |
| VALID | Incomplete | 902 |
| INVALID | Invalid | 58,070 |
| INVALID | Incomplete | 463 |

- False Valid: 0. False Invalid: 0.
- Text checks: casing 47,748 units, 0 mismatches. Word-filter lists: 0 mismatches. Word filter: 0 mismatches. Gen III/IV fixed names: 0 missing.
- Seeds 2, 7 and 11 also give 0 false Valid and 0 false Invalid (`docs/legality/GEN5_PKHEX_RULE_INVENTORY.md`).

The Gen V record carries its title, so this run can be split by title. The split below uses the version byte of each record. It covers native, in-game trade and event records:

| Title | Records | PKHeX VALID: NP / Inc | PKHeX INVALID: Inv / Inc | PKHeX-valid native / trade / event records certified NP |
|---|---|---|---|---|
| Black | 16,466 | 8,278 / 9 | 8,094 / 85 | 6,838 of 6,846 / 177 of 177 / 1,263 of 1,264 |
| White | 13,793 | 6,953 / 4 | 6,779 / 57 | 5,856 of 5,860 / 137 of 137 / 960 of 960 |
| Black 2 | 20,768 | 10,566 / 6 | 10,144 / 52 | 8,528 of 8,532 / 1,154 of 1,154 / 884 of 886 |
| White 2 | 18,795 | 9,665 / 6 | 9,074 / 50 | 8,089 of 8,094 / 675 of 675 / 901 of 902 |

Two groups of records have no Gen V title:

- **Transfers (44,815 records).** PKHeX VALID 21,300: 20,423 No problems found, 877 Incomplete. PKHeX INVALID 23,515: 23,296 Invalid, 219 Incomplete.
  - The Incomplete PKHeX-valid transfers are the 517 Colosseum / XD records and 360 records with a PKHeX "fishy" Gen III result, which stays a Warning: 314 Ruby/Sapphire eggs whose TID/SID has no Ruby/Sapphire seed, and 46 Gen III wild encounters on no reachable RNG frame.
  - Every PKHeX-invalid Gen III or IV transfer reads Invalid. The Incomplete ones are Colosseum / XD records (198) and transfers whose version byte the mutator changed to a Gen V game (21).
- **Mutated version bytes (683 records).** These are Gen V records whose version byte the mutator changed to a Gen III or IV game. All are Invalid in both.

## Findings

- **Engine bug (Gen IV): the D/P met-location field is not checked.** It is the three BUG rows. It was reported, not fixed.
  - Platinum and HG/SS write the met location to 0x46. At 0x80 they write the same value only when D/P know that place, and 3002 otherwise (pokeplatinum `src/pokemon.c:1733-1739`, pokeheartgold `src/pokemon.c:1286-1292`).
  - The engine checks the two fields against each other only for Pal Park records (`src/Legality/Gen4Legality.cpp:1031-1035`). A native Platinum or HG/SS record with the real location at 0x80 reads "No problems found". PKHeX also calls these records VALID.
  - The three records, decrypted:
    - Platinum Giratina-Origin, 117 at 0x80: `56F5966D0000AD74E7017000917554A0F2FA0100001A000200000000000000000000000000000000D201F6005101D30105050F05000000005BB08221000000000C00000000007500310133013C012B013E01330138012B01FFFF00000000000C00000000000000003E0149015701580149015601FFFF00000000001A0A100000750000042F170000`
    - HeartGold Spinarak, 178 at 0x80: `2B01F5AF00004240A7000000885BF9F106000000460F000200000000000000000000000000000000280051000000000023280000000000006E3AD23300000000020000000000B2003D013A01330138012B013C012B013501FFFF00000000000700000000000000003E0149015701580149015601FFFF00000000001A0A100000B200000402020400`
    - SoulSilver Ledyba, 178 at 0x80: `96E6AB5400005F3BA50000003023CE53150000004644000200000000000000000000000000000000210000000000000023000000000000007716311E00000000000000000000B20036012F012E0143012C012B01FFFF0000000000000000000800000000000000003E0149015701580149015601FFFF00000000001A0A100000B200000403020400`
  - The probe also found the same for HeartGold Ho-Oh with 205 (Bell Tower) at 0x80.
- **Every other original-game case gave the verdict the pret data predicts.**
  - That includes the Yellow starter Pikachu with catch rate $A3, the version-exclusive Gen I and Gen II slots, and the Crystal Odd Egg with its little-endian OT ID bytes.
  - For Gen III it includes the Ruby/Sapphire and FireRed/LeafGreen exclusives, the Emerald outbreak Seedot in Sapphire, the Navel Rock Lugia fateful flag and both Game Corner prizes.
  - For Gen IV it includes the Eterna Forest night slots, the Old Chateau Rotom, the version-dependent Ho-Oh and Lugia levels, the Turnback Cave Giratina and the Pokewalker location.
  - No test expectation was changed to match the engine.
- **A Gen IV Bad Egg reads Incomplete in the app's analysis, with no issue that says why.**
  - `Pokemon4ReadOnly` returns 0 for every field of a record that fails its checksum or sanity word (`src/Pokemon/Pokemon4ReadOnly.cpp:58`). So `Legality::analyze` sees species 0, treats it as an empty slot and returns early (`src/Legality/Legality.cpp:1120`). It never reaches the "Stored checksum is invalid" Warning (line 3251) or the Gen IV analyzer (line 3256).
  - `analyzePk4` on the same bytes reports "PK4 checksum or sanity word does not match its data (the games show it as a Bad Egg)". The matrix checks that.
  - The verdict is safe: it is never "No problems found". In the app the reader hides such a record, so the legality screen never shows it.
- **The Gen IV reader hides a Bad Egg and still opens the save.**
  - `Gen4ReadOnlyTrainer` leaves the slot empty (`src/Legacy/Gen4ReadOnlyTrainer.cpp:155-160`), and the legality screen says "selected Gen IV PK4 failed strict validation". The Gen III reader refuses the whole save instead (below).
- **Without a source game, the Gen IV analyzer still certifies.**
  - With an empty source id, `analyzePk4` uses the HG/SS personal table (`src/Legality/Gen4Legality.cpp:1383-1385`). The Diamond Murkrow then reads "No problems found", with every coverage area Complete. PKHeX does the same for a .pk4 file on its own (`PK4.cs:20` uses `PersonalTable.HGSS`).
  - The Gen III analyzer never certifies without a GBA source id. The Gen III Bank rows above show that.
  - No Gen IV record can reach the Bank on this branch: "Add to Bank..." is disabled for Gen IV (`src/UI/Gen4SharedPokemonSurface.inc:1201`). So no matrix row covers it. It matters once Gen IV records can go to the Bank.
- **The engine is more lenient than PKHeX on Draco Meteor for Arceus.** PKHeX calls it INVALID. The engine reads Incomplete, because the Platinum and HG/SS tutor accepts any Pokemon whose current type is Dragon.
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
