# PokeBank NX — HOME Bridge & Historical Transfer Research

Date: 2026-09-03
Status: POST-v1 / v1.1 RESEARCH — DO NOT IMPLEMENT DURING SESSION 2.6

## Purpose

Record useful research for preserving genuinely old Pokémon, converting them across the historical Generation II → III break, and preparing compatible modern-game representations for the user's own official Pokémon HOME workflow without impersonating HOME or fabricating server-side tracker/history state.

## Important terminology

PokeBank NX must distinguish:

```text
AUTHENTIC SOURCE PROVENANCE
    the original save/Pokémon really came from the stated old game

LOCAL DATA LEGALITY
    a derived representation satisfies known game/format legality rules

OFFICIAL CONTINUOUS TRANSFER HISTORY
    the Pokémon traversed only officially supported transfer mechanisms

HOME SERVER HISTORY
    server-side tracker/history known only to Pokémon HOME
```

These are not interchangeable.

## Video / real-world concept reference

### PaPaSea video — YouTube ID `1mUHI9yhbtQ`

External references identify this video as a demonstration of taking an old Pokémon Yellow Blastoise forward to Scarlet/Violet using a fan-made Generation I/II → III bridge before continuing through later-generation transfer mechanisms.

This is conceptually important for PokeBank NX because it demonstrates the preservation goal:

```text
real old Game Boy specimen
        ↓
community conversion across the historical break
        ↓
later-generation representation
        ↓
modern game / HOME workflow
```

The Generation I/II → III step is not an official Game Freak transfer path and must be recorded as an explicit community conversion.

### YouTube ID `-8HAP8Gp53w`

Current community transfer guides embed this video as an overview/reference for moving Pokémon from older generations toward modern Switch/HOME games. Treat it as a user-facing transfer-chain reference, not as a technical source of truth. Exact generation rules must come from format implementations, official documentation where available, and verified test vectors.

## Tyranitar example

Tyranitar did not exist in Red/Blue/Yellow. It was introduced in Generation II.

A historically authentic example is therefore:

```text
Pokémon Gold / Silver / Crystal
Larvitar / Pupitar / Tyranitar
        ↓
UNOFFICIAL Gen II → III community conversion
        ↓
Ruby / Sapphire / Emerald / FireRed / LeafGreen representation
        ↓
(optional) trade to/from Colosseum or XD
        ↓
Gen IV Pal Park
        ↓
Gen V Poké Transfer
        ↓
Bank / HOME route where still available
        ↓
modern Switch title
```

Colosseum/XD is an optional Generation III side-game stop, not a mechanism that solves the Generation II → III break by itself.

## Poké Transporter GB

Repository:

```text
Striaton-Lab-Team/Poke_Transporter_GB
```

High-value finding.

The project is specifically designed to transfer Pokémon from:

```text
Red / Blue / Yellow
Gold / Silver / Crystal
```

into:

```text
Ruby / Sapphire / Emerald
FireRed / LeafGreen
```

using real Game Boy / GBA hardware and a community conversion design intended to feel similar to later official transfer facilities.

It explicitly modifies both source and destination saves and recommends backups.

It currently uses the Pokémon Community Conversion Standard (PCCS).

Classification:

```text
GEN I/II → III CONVERSION REFERENCE — VERY HIGH VALUE
HISTORICAL-BREAK UX REFERENCE — VERY HIGH VALUE
TEST/ORACLE CANDIDATE — HIGH VALUE
DIRECT DESTRUCTIVE MOVE MODEL — DO NOT COPY AS POKEBANK NX SAFETY POLICY
```

## Pokémon Community Conversion Standard (PCCS)

Repository:

```text
Striaton-Lab-Team/Pokemon-Community-Conversion-Standard
```

PCCS is a C++ library defining community conversion policies for Generation I/II → III.

Documented policies include:

```text
FAITHFUL
LEGAL
VIRTUAL
ORIGINAL
```

The distinction is extremely relevant to PokeBank NX.

PokeBank NX should not silently choose between preserving original information and producing a destination-legal representation. A future conversion workspace should show the trade-off explicitly.

Potential product concepts:

```text
PRESERVE / FAITHFUL
    retain as much original information as technically representable

COMPATIBILITY
    derive a destination-compatible representation while preserving the untouched original in Vault history

VC-LIKE
    emulate documented Virtual Console-era transformation behavior where appropriate
```

Do not label any community Gen I/II → III method as an official historical transfer.

The authoritative PokeBank NX Vault entity should retain the original raw Gen I/II payload and provenance even when a Gen III+ derived representation is created.

## HOME-Live-Plugin

Repository:

```text
Manu098vm/HOME-Live-Plugin
```

Very useful read-only HOME-format reference.

The project explicitly states that it is for dumping/viewing Pokémon already deposited in HOME and that it will not support editing or injection into HOME.

It supports HOME-side formats such as:

```text
PKH / EKH
PH1/EH1
PH2/EH2
PH3/EH3
PH4/EH4
```

and can simulate conversion from HOME data into game-specific formats through PKHeX.Core.

Important warning from the project itself: those simulated conversions are unofficial and cannot be assumed to exactly reproduce an official HOME transfer.

Classification:

```text
HOME FORMAT / PRESERVATION REFERENCE — VERY HIGH VALUE
READ-ONLY RESEARCH REFERENCE — VERY HIGH VALUE
HOME → GAME CONVERSION CROSS-CHECK — HIGH VALUE
HOME INJECTION / TRACKER FORGERY — OUT OF SCOPE
```

Potential PokeBank NX use after v1:

- understand and preserve genuine HOME-origin metadata when a user supplies a legitimately dumped HOME representation;
- compare HOME/game conversion behavior with PKHeX;
- improve Summary/provenance fields;
- preserve genuine tracker values without claiming they are server-valid;
- never create or inject fabricated HOME server history.

## `awesome-pokemon-home`

Repository:

```text
alexrodba/awesome-pokemon-home
```

Useful current user-facing compatibility reference.

Its transfer documentation tracks one-way restrictions and title-specific HOME compatibility traps. This is useful inspiration for a future `Prepare for HOME` preflight UI.

Classification:

```text
HOME COMPATIBILITY / UX REFERENCE — HIGH VALUE
TECHNICAL FORMAT ENGINE — NO
```

Future PokeBank NX could expose warnings such as:

```text
SUPPORTED DESTINATION
ONE-WAY TRANSFER
CANNOT RETURN TO SOURCE GAME
SPECIES/FORM NOT SUPPORTED
HOME TRACKER REQUIRED / UNKNOWN
SERVER ACCEPTANCE CANNOT BE GUARANTEED
```

Exact rules must still be independently verified before becoming product logic.

## `pokemonchaintrades`

Repository:

```text
knutkirkhorn/pokemonchaintrades
```

The project describes itself as a planner for chains of Pokémon trades.

Potential value is conceptual/UI rather than save parsing:

```text
source
  ↓
possible intermediate games
  ↓
destination
```

A later PokeBank NX route planner could similarly answer:

```text
What is the best preservation/compatibility route for this Pokémon?
```

while accounting for generation breaks, supported species/forms, conversion steps, provenance loss, and HOME requirements.

## HOME tracker/server boundary

PokeBank NX must preserve genuine tracker values where present but must not fabricate, randomize, clone, or claim to validate HOME tracker values.

Current PKHeX research establishes an important boundary: local software can determine when a tracker is expected, but actual tracker validity is server-side.

Therefore future UI terminology should be:

```text
TRACKER NOT EXPECTED
TRACKER REQUIRED
TRACKER PRESENT — SERVER VALIDITY UNKNOWN
TRACKER MISSING — HOME REJECTION POSSIBLE/EXPECTED
```

Never:

```text
BAN SAFE
VALID HOME TRACKER
MAKE HOME LEGIT
SPOOF TRACKER
```

unless Pokémon HOME itself has authoritatively produced/validated that server state.

## Proposed v1.1 feature: Prepare for HOME

PokeBank NX should not directly impersonate or talk to private HOME APIs.

Preferred workflow:

```text
Vault Pokémon
    ↓
select HOME-compatible destination title
    ↓
compatibility check
    ↓
select conversion policy if a historical break exists
    ↓
create derived destination representation
    ↓
PKHeX/native legality + provenance checks
    ↓
staged/verified save write through approved adapter
    ↓
user launches official game
    ↓
user launches official Pokémon HOME
    ↓
HOME server decides acceptance / assigns or preserves tracker state
```

If HOME rejects the Pokémon, PokeBank NX retains the original Vault entity and destination backup and records only a user-reported outcome if desired.

## Historical provenance example

```text
TYRANITAR

Original source:
Pokémon Crystal — Game Boy Color
Original raw entity preserved: YES
Original save hash: ...

History:
2001  Crystal specimen
2026  Imported into PokeBank NX
2026  Community Gen II → III conversion (PCCS-compatible policy)
2026  Derived Gen III representation
2026  Optional XD/Colosseum visit
2026  Derived later-generation representation
2026  Prepared for modern HOME-compatible title

Official continuous transfer chain:
NO — Generation II → III used an unofficial preservation bridge

HOME tracker:
<none/present>
Server validity:
UNKNOWN unless HOME itself accepted it
```

This is honest provenance while still preserving the emotional/history value of an actual decades-old Pokémon.

## Relationship to roadmap

This research belongs after the Switch v1.0 foundation and does not change the current critical path.

Likely relationship:

```text
v1.0
stable offline Switch Vault / adapters / conversion / safe writes

v1.1+
Historical Transfer Workspace
Prepare for HOME
HOME compatibility route planner
PCCS-style Gen I/II → III policy support
read-only HOME-format/provenance research

v2.x
Android / PokeBank NX Link
```

## Safety / product rules

- never rewrite archival history to pretend an unofficial conversion was official;
- never delete the untouched original Vault payload when deriving later formats;
- never promise Nintendo/HOME acceptance;
- never promise ban safety;
- never forge HOME tracker/history state;
- never depend on private Nintendo/Pokémon HOME service impersonation;
- use official game + official HOME as the server-side bridge when the user chooses to attempt HOME import.
