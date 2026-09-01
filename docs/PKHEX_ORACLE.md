# PokeBank NX — PKHeX Oracle Specification

Status: DESIGN SPECIFICATION  
Last updated: 2026-09-01

The PKHeX Oracle is a **host-side developer/test utility**, not part of the Nintendo Switch runtime. Its purpose is to give PokeBank NX a repeatable external correctness reference for Pokémon parsing, legality, encounter selection, conversion, and generated golden test vectors.

The Switch application remains native C++.

---

## Goals

The Oracle should let tests ask a pinned PKHeX.Core/AutoMod environment questions in a machine-readable way.

Primary goals:

- compare PokeBank NX parsing against PKHeX
- compare conversions against PKHeX
- generate legality reports for regression samples
- retrieve encounter candidates for controlled generation
- produce deterministic golden vectors where possible
- expose enough engine/version metadata that results can be reproduced later

---

## Pinned research revisions at design time

PKHeX:

```text
kwsch/PKHeX
e15d2467b32da7bc26ce7cc8e5c4ede32740e20a
GPLv3
```

Auto Legality Mod / active plugin fork:

```text
santacrab2/PKHeX-Plugins
90410f2681a0a72680d12280a1e0f14715e67dff
MIT
```

The implementation may need a different paired revision if AutoMod compatibility requires it. If so, pin the exact pair in source and documentation rather than floating on `main`.

---

## Proposed location

```text
tools/pkhex-oracle/
```

Suggested contents:

```text
README.md
PokeBank.Oracle.csproj
Program.cs
Commands/
Schemas/
PinnedVersions.md
```

Vendoring strategy should be decided during implementation. Git submodules, source fetch scripts, or pinned source copies are all possible, but builds must remain reproducible and license notices must be preserved.

---

## Output contract

Normal output should be JSON to stdout.

Diagnostics go to stderr.

Successful process exit:

```text
0
```

Invalid input/unsupported operation/internal failure should use non-zero exit codes and structured error JSON when possible.

Every successful result should contain version identity:

```json
{
  "oracle_schema": 1,
  "pkhex_commit": "...",
  "automod_commit": "...",
  "command": "inspect",
  "result": {}
}
```

---

## Command: inspect

Example:

```bash
pokebank-oracle inspect samples/charmander.pk3
```

Return normalized fields such as:

```json
{
  "format": "PK3",
  "species": 4,
  "form": 0,
  "pid": 123456789,
  "encryption_constant": null,
  "tid": 12345,
  "sid": 54321,
  "nature": 10,
  "ability": 0,
  "gender": "male",
  "shiny": false,
  "ivs": [31,31,31,31,31,31],
  "evs": [0,0,0,0,0,0],
  "moves": [10,45,0,0],
  "ball": 4,
  "language": 2,
  "checksum_valid": true
}
```

Fields that do not exist in a format should be `null` or omitted according to the schema; they must not be synthesized as if native to that generation.

---

## Command: legality

Example:

```bash
pokebank-oracle legality samples/charmander.pk3
```

Return:

```json
{
  "state": "LEGAL",
  "summary": "...",
  "checks": [],
  "encounter": {},
  "warnings": []
}
```

PokeBank NX should map results into its own `LEGAL / INVALID / UNKNOWN` presentation rather than expose arbitrary engine text as the permanent internal model.

The Oracle result is a reference/check, not proof of real-world legitimacy.

---

## Command: convert

Example:

```bash
pokebank-oracle convert samples/charmander.pk3 --target PK8 --output /tmp/charmander.pk8
```

JSON should describe:

```text
input format/hash
output format/hash
conversion success/failure
important normalized fields before/after
warnings/losses
```

The output file becomes a golden reference artifact for native comparison tests.

---

## Command: encounters

Example:

```bash
pokebank-oracle encounters --species 4 --game firered_gba
```

Return normalized encounter candidates sufficient for development/testing:

```text
encounter/game/version
location
level/range
method/type
form
shiny restrictions
ball restrictions where applicable
special/event flags
```

Do not tie the PokeBank NX native runtime directly to the Oracle's internal class names.

---

## Command: generate

Example:

```bash
pokebank-oracle generate --species 4 --game firered_gba --shiny false --output /tmp/charmander.pk3
```

This command is for development vectors and future Fill Master Vault support.

It should record:

```text
requested constraints
selected encounter
trainer context used
result format/hash
legality result
engine versions
```

Generated Pokémon must be identified as generated in PokeBank NX provenance. Do not use the Oracle to fabricate an "officially obtained" history.

---

## Command: save-inspect

Later addition:

```bash
pokebank-oracle save-inspect test-save.sav
```

Useful normalized output:

```text
detected game/version
save validity
trainer basics
party count
box count
box slot summaries
save/block/checksum warnings
```

This provides another comparison layer for PokeBank native/PKSM-Core save adapters.

---

## Golden test corpus

Create a repository test corpus whose provenance/license allows redistribution, or generate synthetic fixtures in tests.

Coverage should eventually include:

```text
PK1
PK2
PK3
PK4
PK5
PK6
PK7
PB7
PK8
PB8
PA8
PK9
PA9
```

For each format, include samples exercising:

- normal species
- shiny
- gender edge cases
- forms
- held items where supported
- move data
- IV/EV extremes
- language/OT strings
- event/special cases where redistributable
- invalid checksum/sample for negative tests

---

## Comparison schema

PokeBank NX test tooling should compare normalized objects rather than textual UI output.

Minimum comparison fields:

```text
species
form
PID
EC where applicable
TID/SID
IVs
EVs
nature
ability
gender
shiny
moves
ball
OT
language
met/origin fields
checksum validity
serialized hash/size when exact output should match
```

A mismatch must be classified:

```text
BUG IN POKEBANK
EXPECTED IMPLEMENTATION DIFFERENCE
ORACLE VERSION CHANGE
UNSUPPORTED / UNKNOWN
```

Do not update golden files automatically just because a test fails.

---

## PKSM-Core triangulation

For historical formats, aim for three-way comparison:

```text
sample
  ├── PKHeX Oracle
  ├── PKSM-Core adapter
  └── PokeBank NX
```

Agreement among independent implementations significantly reduces the risk of silently encoding a parser mistake.

For modern Switch formats, add pkHouse behavior as a reference where useful, while independently implementing PokeBank NX code.

---

## Reproducibility

The Oracle build must record:

```text
.NET SDK version
PKHeX commit
AutoMod commit
Oracle source commit
schema version
```

CI/test logs should print these values.

No test should silently fetch a new upstream `main` and change expected behavior.

---

## Security/scope boundaries

The Oracle is for local Pokémon/save correctness research and testing.

It must not be designed to:

- extract Nintendo/Pokémon account credentials
- impersonate Pokémon HOME services
- forge HOME tracker/history data
- bypass bans/restrictions
- promise online acceptance

HOME-related workflows remain official-game mediated.

---

## Minimum Oracle v1 milestone

- [ ] pinned PKHeX.Core builds
- [ ] `inspect`
- [ ] `legality`
- [ ] `convert` for at least one historical → later format path
- [ ] stable JSON schema
- [ ] engine version metadata in output
- [ ] PK3 golden vectors
- [ ] one modern-format golden vector
- [ ] automated comparison runner usable by PokeBank NX host tests
- [ ] README with exact reproduction commands

AutoMod-backed `encounters`/`generate` can follow once the pinned PKHeX + AutoMod pair is stable.
