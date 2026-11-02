# Gift / static / in-game trade encounter data: coverage and gaps

Round 13 owner decision 7: "Gift/static encounters require proper source-backed data before v1.0
acceptance. Markers alone are provisional." Decision 8: "Crystal time rows can be grouped visually,
but do not lose encounter-time distinctions. Preserve accurate ancestry labels."

Status: Fixed in Code — Awaiting Hardware. Nothing here is release-ready from automated tests alone.

## What it is

`include/Integration/Encounter/GiftStaticEncountersData.inc` (generated, 1332 rows), read through
`include/Integration/Encounter/GiftStaticEncounters.h`. Each row: game, species, exact native met
location id, met level range, kind (gift / prize / fossil / static / roaming / trade / event /
swarm / egg gift), flags (fateful encounter, N's Pokémon, Crystal "caught level unrecorded") and
form / egg location where the source fixes them.

Generator: `tools/gen_gift_static_encounters.py` (`--check` verifies the committed files). Source
files, commit SHAs and sha256 of every input: `docs/data/gift-static-encounter-sources.json`.

| Source | Commit | Used for |
|---|---|---|
| kwsch/PKHeX | `6042efa241a9925ce57e645329ea7f976deabdd3` (same pin as the Gen IV/V wild catalogue) | `Legality/Encounters/Data/Gen2..Gen5/Encounters*.cs` static / gift / trade tables; `Templates/Gen2..Gen5/Encounter{Static,Trade}*.cs` trade met location, level ranges, roamer route sets; `Game/Locations/Locations.cs` |
| pret/pokecrystal | `3c813b0b4d82b39bf62185aceffb76fd6026c70b` | every Crystal row: `maps/*.asm` (`givepoke` = gift, `loadwildmon` = static, Game Corner = prize), `data/maps/maps.asm` + `constants/landmark_constants.asm` (landmark = PKHeX location), `data/events/npc_trades.asm` (Crystal's 7 trades), `engine/pokemon/caught_data.asm` (`SetGiftMonCaughtData`: LANDMARK_GIFT 0x7E, level 0, time 0), `engine/events/shuckle.asm`, `engine/events/npc_trade.asm` |
| pret/pokefirered | `ce6406d8c2eaf46d543c8a2606de0c3cb1951982` | spot checks: Lapras (Silph Co. 7F), Eevee (Celadon Condominiums), Magikarp (Route 4), Snorlax (Route 12), Zapdos / Electrode (Power Plant); map section = met location |
| pret/pokeemerald | `731ad5bfd6e6f265508d0efcca0ba42f9dcf5881` | spot checks: Beldum (Steven's house), Castform (Weather Institute), Chikorita (Birch's lab); region map sections |
| pret/pokeruby | `5784633ce4ef7ade1a7f2d2d0c288e3d5e6cdd7f` | spot check: Beldum (Steven's house) |

The generator FAILS if a Crystal PKHeX row has no matching pret script (species, level, landmark), if
a pret script has no PKHeX row, or if a Gen III spot check disagrees.

## Coverage per game

| Game | Rows | Species | Gift | Prize | Fossil | Static | Roaming | Trade | Event | Swarm | Egg gift |
|---|---|---|---|---|---|---|---|---|---|---|---|
| Crystal | 76 | 42 | 8 | 6 | 0 | 11 | 32 | 7 | 1 | 3 | 8 |
| Ruby | 55 | 21 | 5 | 0 | 2 | 10 | 34 | 3 | 0 | 0 | 1 |
| Sapphire | 55 | 21 | 5 | 0 | 2 | 10 | 34 | 3 | 0 | 0 | 1 |
| Emerald | 100 | 31 | 8 | 0 | 2 | 11 | 68 | 4 | 6 | 0 | 1 |
| FireRed | 112 | 38 | 8 | 5 | 3 | 8 | 75 | 9 | 3 | 0 | 1 |
| LeafGreen | 112 | 38 | 8 | 5 | 3 | 8 | 75 | 9 | 3 | 0 | 1 |
| Diamond | 74 | 28 | 4 | 0 | 7 | 9 | 48 | 4 | 0 | 0 | 2 |
| Pearl | 74 | 28 | 4 | 0 | 7 | 9 | 48 | 4 | 0 | 0 | 2 |
| Platinum | 154 | 38 | 5 | 0 | 7 | 15 | 120 | 4 | 1 | 0 | 2 |
| HeartGold | 123 | 64 | 19 | 5 | 7 | 21 | 57 | 10 | 0 | 0 | 4 |
| SoulSilver | 123 | 64 | 19 | 5 | 7 | 21 | 57 | 10 | 0 | 0 | 4 |
| Black | 54 | 37 | 7 | 0 | 9 | 13 | 16 | 5 | 3 | 0 | 1 |
| White | 54 | 37 | 7 | 0 | 9 | 13 | 16 | 5 | 3 | 0 | 1 |
| Black 2 | 83 | 74 | 7 | 0 | 9 | 43 | 0 | 23 | 0 | 0 | 1 |
| White 2 | 83 | 74 | 7 | 0 | 9 | 43 | 0 | 23 | 0 | 0 | 1 |

Roaming rows are one per route the roamer can be met on (PKHeX: Crystal `RoamLocations`, Gen III
routes 16-49 / 101-125, Gen IV `PermitGrass*` ∪ `PermitWater*`, Gen V `IsRoamerMet`).

Native met location of a trade (PKHeX `Locations.LinkTrade*NPC`, pret for Crystal): Crystal
LANDMARK_GIFT 0x7E (caught level 0, time 0, as Crystal writes it), Gen III 254, Gen IV 2001,
Gen V 30002. Level ranges follow PKHeX's matching rules: Crystal / Gen III / Gen IV trades arrive at
the offered Pokémon's level ("Lv. 5+"); Gen V trades and every static / gift are a fixed level.

## How the editor uses it

One shared rule (`Integration/Encounter/MetLocationPolicy.h`), every generation:
`ExactGameMetLocations::templatesWithAncestry` (Crystal, Gen III) and `EncounterContext45::Catalogue`
(Gen IV / V) treat a gift / static / trade / fossil / prize / roaming / event / swarm row of the
species or a pre-evolution exactly like a wild row: VERIFIED when the Pokémon's level reaches it,
labelled with its kind ("Celadon City — Gift Lv. 25", "In-game trade — Trade Lv. 5+",
"Goldenrod City — Gift · Lv. 20 · via Eevee"); above the level it moves to the unverified list
("Lv. 25+; level too low"). These rows replace the older hand-carried Static / Gift / Event rows for
Crystal and Gen III. A marker (Crystal Gift / Event, Gen III 253-255, Gen IV 2000-2002, Gen V
30002 / 30003 / 60002 / 60003) WITHOUT a data row for the species stays an unverified marker. A row
with an unknown access prerequisite is listed but never verified (see "Three tiers").

## Three tiers (owner decision, Nov 2)

"Do not label a Pokémon's complete acquisition history Verified simply because its encounter exists
in the game's scripts." Every row carries two more fields (generator, `.inc`, JSON counts):

1. **Script-backed encounter** — every row; its provenance is `confidence`:
   `PretChecked` (PKHeX + pret cross-checked: the 33 Crystal static / gift / prize / OT-gift / trade
   rows), `PretSpotChecked` (PKHeX + one of the Gen III pret spot checks: 17 rows across R/S/E/FR/LG),
   `SingleSource` (PKHeX alone: every Gen IV / V row, every other Gen III row, Crystal roamers /
   swarms / egg gifts). A single-source row is never called independently verified: a verified Gen III /
   IV / V data row's editor / Review note reads "Encounter data: single source (PKHeX)".
2. **Access prerequisite** — `prereq`: `TicketOrItem` (ticket / key item distribution), `EventPokemon`
   (an event-distributed Pokémon in the party), `OtherGame` (a Pokémon only another game provides),
   with the reason in `kPrerequisiteReasons`.
3. **Incomplete** — a prerequisite the save cannot show. Design: the row STAYS in the default list,
   because the encounter is in the game ("Navel Rock — Event Lv. 70; access unverified"), but it is
   never a verified encounter: it is not counted, `verifiedEncounter` / `verifiedTemplate` reject it,
   the Gen III row is not a "verified catch", and the editor / Review note reads "Incomplete: encounter
   in game script; ticket / distribution not verifiable from the save" (or "event Pokémon" / "Pokémon
   from another game"). Moving them to the unverified list would hide that the encounter exists;
   keeping them labelled in place never shows them as fully verified.

| Prerequisite | Rows |
|---|---|
| Crystal Celebi, Ilex Forest (GS Ball — Japanese Crystal / 3DS Virtual Console only) | TicketOrItem |
| Emerald Lugia / Ho-Oh, Navel Rock (MysticTicket); Deoxys, Birth Island (AuroraTicket); Mew, Faraway Island (Old Sea Map); Latias / Latios Lv 50, Southern Island (Eon Ticket) | TicketOrItem |
| Ruby Latias / Sapphire Latios, Southern Island (Eon Ticket) | TicketOrItem |
| FireRed / LeafGreen Lugia / Ho-Oh, Navel Rock (MysticTicket); Deoxys, Birth Island (AuroraTicket) | TicketOrItem |
| Platinum Shaymin (Oak's Letter), Darkrai (Member Card) | TicketOrItem |
| HeartGold Latios / SoulSilver Latias, Pewter City (Enigma Stone) | TicketOrItem |
| Black / White Victini (Liberty Pass) | TicketOrItem |
| Black / White Zorua (event Celebi), Zoroark (event shiny beasts) | EventPokemon |
| HeartGold / SoulSilver Dialga / Palkia / Giratina, Sinjoh Ruins (event Arceus); Spiky-eared Pichu (event Pichu) | EventPokemon |
| HeartGold / SoulSilver Rayquaza (Kyogre + Groudon); Black / White Landorus (Tornadus + Thundurus); Diamond / Pearl / Platinum Regigigas (the three Regis); Platinum Regirock / Regice / Registeel (Regigigas) | OtherGame |

Arceus (Spear Pillar / Hall of Origin, Azure Flute), the HG/SS GS Ball Celebi and other pure
distributions have no data rows (no in-game script row in PKHeX's tables), so they stay unverified.

## Gaps (explicit; nothing invented)

- **Event distributions** (Mystery Gift / Wonder Cards; Gen III 255 fateful marker, Gen IV 3000+,
  Gen V 40000+ locations): no data. They stay unverified in every generation.
- **Prerequisite-dependent encounters** (owner decision, Nov 2): the encounter row is script-backed
  and kept, but its access cannot be known from a save, so the acquisition evidence is Incomplete
  (see "Three tiers" below). Their fateful flag is recorded but the pickers do not check the record's
  fateful flag.
- **Prerequisites not judged**: Black 2 / White 2 Regi keys (Key System / Unova Link) and
  Regigigas, Heart Gold / Soul Silver Kyogre / Groudon orb conditions, and similar "post-game" rows
  were not flagged because the generator only flags prerequisites it can name with certainty; they
  read as ordinary verified rows and should be reviewed.
- **Egg gifts** (Odd Egg, Togepi, Wynaut, Riolu, Happiny, Mareep / Wooper / Slugma, Larvesta): data
  rows only. An egg is met where it hatched, so they are not Met Location rows (egg / breeding
  histories are the breeding lane).
- **Not extracted**: HG/SS Pokéwalker (PKHeX binary `encounter_walker4.pkl`, met location 233),
  D/P/Pt My Pokémon Ranch, Ruby / Sapphire Colosseum / XD bonus-disc gifts, Black / White Dream World,
  Black 2 / White 2 Dream Radar and Hidden Grotto items, Gold / Silver (no caught data to verify;
  Crystal only for Gen II).
- **Second source**: every Crystal row is pret-checked; Gen III has pret spot checks only (listed
  above); Gen IV is PKHeX only (the pret Gen IV script banks are not mapped to met locations in a
  form the generator can check); Gen V is PKHeX only (no pret decompilation exists).
- **Classification rule**: PKHeX's fixed Poké Ball marks a gift; fossil species given with a fixed
  ball are "fossil"; "Game Corner" in PKHeX's section / comment is "prize"; PKHeX's `IsRoaming`,
  `FatefulEncounter` / "Event" section and egg fields set those kinds; everything else is "static".
  Crystal kinds come from the pret script command instead.

## Crystal time (decision 8)

Crystal's caught data stores the time of day in bits 14-15 of the 16-bit caught word (pret
`SetBoxmonOrEggmonCaughtData`: `wTimeOfDay + 1` → 1 morning, 2 day, 3 night; PKHeX PK2). Rows that
differ only in time are still one row, but the row now always names its times ("morn/day",
"night", and "any time" for a slot met morning, day and night), and a place whose times differ in level keeps one row per level
(Rattata, Route 29: "Wild · Lv. 2 · morn/day" and "Wild · Lv. 2–3 · night"; Raticate: the same with
"via Rattata"). Choosing a row writes the pending Met Time when the row occurs at it, otherwise the
row's first time, and the status line states the time written and the row's times ("Met time:
Morning (morn/day); change it on the Time row"). The editor / Review "Met evidence" line now also
judges the stored time: a time the slot never occurs at reads "Not a listed catch at this time of
day; unverified". Gift / trade rows at LANDMARK_GIFT write level 0 and time 0 as Crystal does.

## Owner acceptance steps (Switch)

1. Crystal: Create Eevee Lv 20 → Met Location: "Goldenrod City — Gift · Lv. 20" is in the default
   list (no "unverified"). At Lv 19 it is only in "Other location (unverified)…" as "Lv. 20+; level
   too low". Espeon Lv 30 shows the same row "· via Eevee".
2. Crystal: Create Rattata Lv 5 → Route 29 shows two rows, "Lv. 2 · morn/day" and "Lv. 2–3 · night".
   Set Met Time to Night, choose the morn/day row: the status says "Met time: Morning (morn/day)…";
   the Time row reads Morning. Add, save, reload: the time is still Morning.
3. Crystal: Create Machop Lv 10 → "Gift — Trade · Lv. 5+" is verified; after choosing it the record
   shows met level / time not recorded, and Review has no "Met evidence" line.
4. FireRed / LeafGreen: Lapras Lv 25 → "Silph Co. — Gift Lv. 25"; Mr. Mime Lv 5 → the in-game
   trade row reads "Trade Lv. 5+" (verified, not "Trade marker; unverified").
5. Emerald: Beldum Lv 5 → "Mossdeep City — Gift Lv. 5"; Metang Lv 20 → "… via Beldum".
6. HeartGold / SoulSilver: Chikorita / Cyndaquil / Totodile Lv 5 → "New Bark Town — Gift Lv. 5".
7. Platinum: Rotom Lv 20 → "Old Chateau — Static Lv. 20"; Giratina Lv 47 → Turnback Cave and
   Distortion World "Static Lv. 47". Diamond: Abra Lv 1 → the in-game trade row "Trade Lv. 1+".
8. Black / White: Zorua Lv 10 → "Castelia City — Event Lv. 10; access unverified", and its Met evidence
   reads "Incomplete: … event Pokémon not verifiable from the save". Black 2 / White 2: Eevee Lv 10 →
   "Castelia City — Gift Lv. 10"; Zorua Lv 25 → "Driftveil City — Static Lv. 25" (N's Pokémon).
9. Emerald Lugia Lv 70 → "Navel Rock — Event Lv. 70; access unverified"; after choosing it the editor's
   Encounter origin and Review read "Incomplete: encounter in game script; ticket / distribution not
   verifiable from the save". Crystal Celebi Lv 30 → Ilex Forest row "; access unverified", Met evidence
   Incomplete. HeartGold Chikorita gift → Met evidence "Encounter data: single source (PKHeX)".
10. Any game: a species with no gift / trade data (e.g. FireRed Pikachu) still shows the trade /
   event markers labelled "marker; unverified".
