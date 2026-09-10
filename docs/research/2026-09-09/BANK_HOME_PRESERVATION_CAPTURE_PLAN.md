# Pokémon Bank -> HOME preservation capture plan — 2026-09-09

> Time-sensitive research/preservation planning only. This does not change the current FRLG coding milestone.

## External deadline

The official Pokémon Bank page states that service ends **February 25, 2027 at 7:00 p.m. PST**. Pokémon Support expresses the same shutdown as **February 26, 2027 at 03:00 UTC**. After that, Bank -> HOME transfers will no longer be possible.

Official references:

- https://www.pokemon.com/us/pokemon-video-games/pokemon-bank/
- https://support.pokemon.com/

The deadline remains real, but the public-corpus research significantly reduces how much custom transfer work should be manufactured from scratch.

---

# Revised strategy after public-corpus discovery

Do **not** start by building a new 20–50 Pokémon custom transfer set.

First open:

```text
docs/research/2026-09-09/BANK_HOME_PUBLIC_CORPUS_AUDIT.md
```

The public audit now contains:

```text
10 explicit PK7 -> real Bank/HOME -> PK9/PA9 pairs from one Project Pokémon thread
1 explicit shiny Jirachi PK7 -> PK9 pair
2 Gen VII -> tracked Gen IX Treecko/Torchic pairs
1 Gen IV -> transferred Gen VIII Shedinja pair
48-Pokémon challengePK7.bin source corpus
an event-heavy source campaign lead
PH1/PH2/PH3/PH4 + game-format HOME-native corpus
```

Therefore the preservation pipeline is now:

```text
PUBLIC CORPUS
    ↓
download + hash
    ↓
pair validator
    ↓
field-level diff
    ↓
coverage matrix
    ↓
identify actual holes
    ↓
ONLY THEN custom Bank -> HOME captures
```

Expected custom work may shrink to roughly **5–15 deliberately chosen missing cases**, potentially fewer.

Do not count a visible forum attachment as a verified fixture until the actual binary has been downloaded, hashed and parsed.

---

# Public pair validation requirement

Before promoting any public pair into the golden fixture suite:

```text
parse source
parse destination
validate surviving identity/invariant fields
record every delta
classify explained vs unexplained changes
record source/target SHA-256
record provenance and redistribution status
```

At minimum compare:

```text
species
PID
EC
TID/SID
OT
origin game
language
IVs
ribbons
met data
nickname
gender
form
shiny state
```

Known side edits must be preserved explicitly rather than silently discarded. For example, the ten-pair Nobomoth thread documents contest-stat changes to Metagross, Tyranitar and Blissey. Those fixtures remain useful, but contaminated fields must not be used to infer Bank/HOME behavior.

Suggested evidence labels:

```text
A = same thread contains before + requested real transfer + returned after
B = one archive explicitly documents transferred before/after forms
C = source-only or descendant-only lead
```

Additional flags:

```text
KNOWN_POST_OR_SIDE_EDIT
HOME_TRACKER_PRESENT
HOME_NATIVE_PRESENT
REDISTRIBUTION_UNKNOWN
```

---

## What we are actually trying to preserve

We are **not** trying to send PokeBank NX Pokémon into HOME or somehow move them back to 3DS.

The research goal is:

```text
known exact Pokémon before
        ↓
official Bank -> HOME path
        ↓
known exact Pokémon / HOME state after
```

The public internet now gives us a useful set of those before/after examples already.

---

## Why native HOME state still matters

A simple:

```text
PK7 before
PK8/PK9 after
```

is useful, but not a complete modern transfer fixture. HOME has server-created identity/state, including the HOME Tracker and game-specific representations/side data.

Permanent rule remains:

```text
Never fabricate a HOME Tracker or claim official HOME provenance that was not actually observed.
```

The biggest still-missing ideal specimen is a same-Pokémon three-stage capture:

```text
original.pk7
    ↓ real Bank -> HOME
same Pokémon native PH/EH dump
    ↓ official withdrawal
same Pokémon pk8/pb8/pa8/pk9/pa9
```

Source -> destination pairs and separate HOME-native corpuses already exist, but the research has not yet established a public perfect three-stage Bank specimen.

---

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

Do not risk the user's primary modded Switch or primary Nintendo account merely to improve a research corpus. Use a trusted/disposable/appropriate setup only if custom capture is still needed after public-corpus coverage is mapped.

---

## Recommended canonical capture stages for remaining custom holes

For deliberately selected specimens that remain genuinely missing after corpus analysis, preserve:

```text
A. source game save
B. source PK6 / PK7 entity
C. Bank/source evidence where available
D. HOME native PH/EH encrypted dump immediately after Bank -> HOME, if safely practical
E. destination game save after official withdrawal
F. destination PB7 / PK8 / PB8 / PA8 / PK9 / PA9 as applicable
G. HOME native dump again after revisiting HOME, if safely practical
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
manual_edits_disclosed
source evidence URL
redistribution status
SHA-256 for every captured artifact
```

---

## Prefer diversity over raw count

Only fill coverage gaps after public-corpus analysis.

Potential holes to inspect include:

```text
ENG / JPN / KOR / CHS / CHT
male / female / genderless
shiny / non-shiny
egg / hatched
event / fateful encounter
Gen III / IV / V / VI / VII origins
legacy moves
old ribbons / memory ribbons
unusual balls
alternate / normalized forms
nickname / OT encoding edge cases
country/subregion metadata
PID / gender / shiny threshold edge cases
SWSH destination
BDSP destination
PLA destination
SV destination
ZA destination
```

A handful of deliberately selected holes is more useful than repeating hundreds of ordinary cases already represented publicly.

---

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

---

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

---

## Priority

This preservation work does not interrupt the current LeafGreen fix.

Before February 2027, the actual order should be:

```text
1. acquire the identified public binaries in a normal/local environment
2. hash and build the manifest
3. run pair/invariant validation
4. run field-level diffs
5. map Bank/HOME coverage holes
6. search for missing descendants/intermediates in existing public archives
7. only then schedule a small custom capture campaign if holes remain
```

The deadline remains important, but the task has changed from **"manufacture a research corpus"** to **"preserve, validate and complete an already-emerging public corpus."**
