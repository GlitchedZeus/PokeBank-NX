# Pokémon Bank -> HOME preservation capture plan — 2026-09-09

> Time-sensitive research/preservation planning only. This does not change the current FRLG coding milestone.

## External deadline

The official Pokémon Bank page states that service ends **February 25, 2027 at 7:00 p.m. PST**. Pokémon Support expresses the same shutdown as **February 26, 2027 at 03:00 UTC**. After that, Bank -> HOME transfers will no longer be possible.

Official references:

- https://www.pokemon.com/us/pokemon-video-games/pokemon-bank/
- https://support.pokemon.com/

This makes controlled Bank -> HOME captures a 2026 preservation task rather than something to postpone until the Gen VII/VIII implementation milestone.

## Why native HOME state matters

A simple:

```text
PK7 before
PK8 after
```

is not a complete modern transfer fixture. HOME has server-created identity/state, including the HOME Tracker and game-specific representations/side data.

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

A carefully selected 50–200-specimen corpus is likely more useful than thousands of ordinary random Pokémon.

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
