# Pokémon Bank -> HOME preservation capture plan — 2026-09-09

> Time-sensitive research/preservation planning only. This does not change the current FRLG coding milestone.

## External deadline

The official Pokémon Bank page states that service ends **February 25, 2027 at 7:00 p.m. PST**. Pokémon Support expresses the same shutdown as **February 26, 2027 at 03:00 UTC**. After that, Bank -> HOME transfers will no longer be possible.

Official references:

- https://www.pokemon.com/us/pokemon-video-games/pokemon-bank/
- https://support.pokemon.com/

This makes controlled Bank -> HOME captures a 2026 preservation task rather than something to postpone until the Gen VII/VIII implementation milestone.

## What we are actually trying to capture

We are **not** trying to send PokeBank NX Pokémon into HOME or somehow transfer them back to the 3DS.

The practical goal is simply:

```text
known exact Pokémon before
        ↓
official Bank -> HOME path
        ↓
known exact Pokémon / HOME state after
```

The value is the before/after difference. Years later, PokeBank NX can compare the source bytes to the official-path result and learn what Nintendo's real service changed.

A small, carefully documented corpus is sufficient. This is preservation/test research, not a user collection migration.

## Practical workflow with another researcher

The easiest safe setup is to prepare a **throwaway Gen VII research save** or a numbered set of exact `.pk7` files, then let a trusted researcher with working Bank/HOME perform the official transfer.

Recommended source package:

```text
PBX-BH-001-Bulbasaur.pk7
PBX-BH-002-Pikachu.pk7
PBX-BH-003-Unown.pk7
...
PBX-BH-050-Example.pk7

source_save/
  main

manifest.json
sha256sums.txt
```

The researcher can either:

1. restore the clean research save to a compatible 3DS setup; or
2. inject the exact numbered `.pk7` files into their own clean compatible Gen VII test save.

For this research, option 2 is acceptable because the thing being measured is the official **Bank -> HOME transformation of known source bytes**, not proof that the source Pokémon was personally caught by the original researcher.

Then:

```text
source PK7 / test save
        ↓
Pokémon Bank
        ↓
official Bank -> HOME transfer
        ↓
HOME result capture
        ↓
optional official withdrawal into compatible Switch title
        ↓
resulting PB7 / PK8 / PB8 / PA8 / PK9 / PA9
```

The source and returned files should preserve the numeric fixture ID so before/after pairing is unambiguous.

## What the researcher should return

### Minimum useful result

For each specimen:

```text
before/PBX-BH-001-Example.pk7

after/PBX-BH-001-Example.pk8   # or other official destination format
```

plus:

```text
manifest.json
sha256sums.txt
```

A whole destination save may also be useful, but individual resulting Pokémon files are cleaner for differential tests and avoid asking the researcher to share unrelated personal save data.

### Best possible result

If the researcher has an online-safe hacked Switch they are comfortable using with HOME, also capture the HOME-native representation using the HOME Live Plugin **dumper**:

```text
before/PBX-BH-001-Example.pk7
home_after_import/PBX-BH-001-Example.ph?/eh?
after/PBX-BH-001-Example.pk8
home_after_destination/PBX-BH-001-Example.ph?/eh?
```

Do **not** require or pressure anyone to put their primary modded Switch/account at risk merely for this research. Existing community captures remain useful if a safe collaborator is unavailable.

## Why native HOME state matters

A simple:

```text
PK7 before
PK8 after
```

is useful, but is not a complete modern transfer fixture. HOME has server-created identity/state, including the HOME Tracker and game-specific representations/side data.

Permanent rule remains:

```text
Never fabricate a HOME Tracker or claim official HOME provenance that was not actually observed.
```

## HOME Live Plugin reference

Repository:

```text
Xieons-Gaming-Corner/HOME-Live-Plugin
```

The reviewed README states that the dumper creates a **1:1 dump of encrypted Pokémon HOME data** and supports HOME representations including:

```text
PKH / EKH
PH1 / EH1
PH2 / EH2
PH3 / EH3
PH4 / EH4
```

It also explicitly warns that the Viewer/game-format conversion is an unofficial simulation and may not reproduce an official transfer exactly.

Therefore:

```text
HOME Live Dumper
    = high-value capture/reference tool

Viewer-generated game PKM
    = NOT official-transfer ground truth
```

Reference:
https://github.com/Xieons-Gaming-Corner/HOME-Live-Plugin

## Recommended canonical capture stages

For deliberately selected specimens, preserve:

```text
A. source game save
B. source PK6 / PK7 entity
C. Bank/source evidence where available
D. HOME native PH/EH encrypted dump immediately after Bank -> HOME
E. destination game save after official withdrawal
F. destination PB7 / PK8 / PB8 / PA8 / PK9 / PA9 as applicable
G. HOME native dump again after revisiting HOME
```

Suggested fixture tree:

```text
fixture/
  metadata.json
  source/
  home_after_import/
  destination/
  home_after_destination/
  sha256sums.txt
```

## Metadata to record

```text
fixture_id
capture_date_utc
source_game
source_game_version
source_language
source_console_region
Bank version
HOME version
Switch firmware / environment
destination_game
route
notes
SHA-256 for every captured artifact
```

## Prefer diversity over raw count

A carefully selected **20–50 excellent specimens** may already be more valuable than hundreds of poorly documented ordinary Pokémon. The broader target can remain 50–200 if enough safe collaborators/time exist.

Cover where practical:

```text
ENG / JPN / KOR / FRE / GER / ITA / SPA
CHS / CHT where the route supports them
male / female / genderless
shiny / non-shiny
event / fateful encounter
older-origin Pokémon (Gen III/IV/V/VI/VII)
legacy moves
ribbons / memory ribbons
unusual balls
alternate / normalized forms
nickname / OT encoding edge cases
country/subregion metadata
PID / gender / shiny threshold edge cases
```

Do **not** use irreplaceable personal living-dex/event specimens for the experiment when generated/duplicated legal research specimens can exercise the same transformation. Bank -> HOME is one-way.

## Destination compatibility

Not every source Pokémon can be withdrawn into every Switch game. Do not assume the complete test set can all become `PK8` or all become `PK9`.

Two valid approaches:

```text
A. capture HOME-native state and stop there for incompatible species

B. deliberately split the test set across compatible destination games
   SWSH / BDSP / PLA / SV / ZA as appropriate
```

The research question is the official transformation path, not forcing every specimen into one destination title.

## Suggested collaborator request

A concise request can be framed as:

```text
I have a numbered research set of exact legal PK7 Pokémon and need them
passed through the real Pokémon Bank -> Pokémon HOME route before Bank
shuts down. I will provide the source PK7 files and/or a clean test save.
Please do not manually edit the Pokémon after receipt.

After transfer I need the resulting Pokémon data. If possible, HOME-native
PH/EH dumps are ideal; otherwise an official withdrawal into a compatible
Switch game and the resulting PK8/PK9/etc files is still useful.

I do not need your personal save or account information.
```

## Modern HOME route model

Treat HOME as a hub with observed per-game representations:

```text
HOME native/canonical observed state
    + SWSH view
    + BDSP view
    + PLA view
    + SV view
    + ZA view
```

Do not assume direct byte mutation from one game PKM format to another reproduces official HOME behavior.

## Fixture provenance vocabulary

Use **official-path captured fixture**, not “official fixture.”

These are community-captured before/after examples of an official transfer path, not Nintendo-published regression vectors.

Suggested fixture origin states:

```text
BUNDLED
DOWNLOADABLE
LOCAL_ONLY
SELF_CAPTURED
GENERATED
```

and transfer authority:

```text
OfficialObserved
OfficialModeled
CompatibilityConversion
Unknown
```

## Priority

This preservation capture does not need to interrupt the current LeafGreen fix, but it should remain visible as a deadline-bound research task during 2026 so the Bank shutdown does not erase the opportunity for new authoritative captures.
