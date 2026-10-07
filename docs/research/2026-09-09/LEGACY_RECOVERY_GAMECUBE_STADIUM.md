# Legacy recovery, Pokémon Box, Stadium and GameCube research — 2026-09-09

> Research only. This file does not activate implementation scope. Verify source revisions/licenses before reuse.

This note consolidates the September 9 research on damaged-save analysis, redundant save structures, Pokémon Box: Ruby & Sapphire, Stadium/Stadium 2 and GameCube Colosseum/XD.

---

# 1. Recovery framework should be generic; selection rules must stay game-specific

A dangerous simplification would be:

```text
if slot A counter > slot B counter:
    use slot A
```

Different Pokémon games use redundancy differently, so recovery needs a common reporting framework but title-specific candidate construction/selection.

Preferred generic result model:

```text
struct RecoveryCandidate {
    RecoveryScope scope;
    uint64_t epoch;
    ChecksumState checksum;
    SignatureState signature;
    LinkageState linkage;
    StructuralState structure;
    double confidence;
};
```

or equivalent candidate/report objects that preserve exactly which source regions were used.

Permanent rule:

```text
never silently repair/write over the damaged original
```

Recovery output should be separate:

```text
recovered.sav
recovery.json
```

where the report can state things like:

```text
General block <- primary copy
Storage block <- backup copy
Sector 3      <- primary generation
Sector 6      <- backup generation
Box 5         <- backup region
```

---

# 2. Gen III recovery — sectorized redundant saves

pret Gen III research/code documents the rotating redundant save scheme.

Important metadata includes:

```text
sector ID
signature
checksum
save counter
```

A power interruption can leave a mixed state:

```text
some sectors from newest generation
some sectors from prior generation
some invalid sectors
```

because the save is not one atomic monolithic write.

Desired analysis output can look like:

```text
Slot/generation A
    counter: 4172
    13/14 sectors valid
    newest
    damaged sector: #7

Slot/generation B
    counter: 4171
    14/14 sectors valid
    older
    completely valid

Recommended normal open:
    B

Optional forensic reconstruction:
    newest valid regions + explicitly borrowed older region
```

Do not present a forensic reconstruction as if it were the untouched original.

Primary references:

```text
pret/pokeemerald
pret/pokefirered / pokeruby as relevant
PKHeX / PKSM-Core for independent validation
```

---

# 3. Gen IV recovery — general/storage structures are independently redundant

Diamond/Pearl research is an especially important warning.

A current general block can legitimately pair with a storage block from the opposite physical half because PC storage may not be rewritten every time the general save advances.

So recovery should do something like:

```text
validate general A
validate general B
choose newest checksum-valid general

validate storage A
validate storage B
compare:
    storage save count
    linked general count
    checksum/signature
choose coherent storage candidate
```

not simply "pick physical half A or B."

This is why PokeBank recovery must expose game-specific analyzers behind a shared reporting API.

References:

```text
pret/pokeplatinum
PKSM-Core Sav4
PKHeX Gen IV save handling
```

---

# 4. Other redundancy/recovery targets

The same framework will eventually need different title-specific rules for:

```text
Crystal main/backup + RTC-related state
Stadium primary/backup structures
Pokémon Box R&S block copies
GameCube memory-card filesystem metadata redundancy
modern Switch block/checksum/revision validation
```

Do not assume one universal "backup slot" algorithm.

---

# 5. Pokémon Box: Ruby & Sapphire is no longer an unknown format

The research found detailed completed Project Pokémon save-structure work plus a maintained PKHeX implementation.

Important mapped properties include:

```text
GCI/save length
rough raw save size around 0x76000 (verify exact wrapper/case)
two save copies/slots
23 blocks
block IDs
block sizes/data boundaries
save counters
checksums
stored Pokémon records
additional depositor trainer ID/SID metadata
```

Recommended implementation layers:

```text
container normalizer
    raw Box save / GCI / memory-card embedded file
        -> Box R&S save parser
        -> slot/block validator
        -> newest coherent valid generation selection
        -> trainer/box metadata
        -> Gen III Pokémon extraction
```

Initial support should be read-only until the normal PokeBank write/staging gates exist.

Potential stress fixtures include large public Pokémon Box saves, but fixture redistribution rights must be tracked before committing them.

Primary references:

```text
Project Pokémon — Pokémon Box R&S Save File Structure Research (completed thread)
PKHeX SAV3RSBox
Dolphin GCI/memory-card code
```

Roadmap interpretation: this is now a known adapter task, not a format-research mystery.

---

# 6. Stadium / Stadium 2 is also substantially more feasible

Project Pokémon research and current PKHeX implementations provide detailed layout/checksum guidance.

Research findings include:

```text
registered-team offsets
party structures
per-Pokémon record lengths
box structures and offsets
footer/checksum information
Japanese vs international differences
primary/backup behavior
```

Important warning:

```text
Japanese Stadium layouts != international Stadium layouts
```

Do not treat region as a cosmetic string difference.

## Emulator/container endian handling

Existing Stadium tooling such as SaveStadium deals with emulator byte-order/container differences, for example Project64 `.fla` versus big-endian `.sav` representations.

Architecture:

```text
N64 save/container normalizer
    -> canonical byte order/layout
    -> Stadium parser
```

Keep emulator endian quirks out of the actual Stadium data model.

Suggested fixture matrix:

```text
Stadium 1 JP
Stadium 1 US
Stadium 1 EU
Stadium 1 byte-swapped
Stadium 1 damaged primary / valid backup

Stadium 2 JP
Stadium 2 US
Stadium 2 EU
Stadium 2 Project64 endian/container
Stadium 2 damaged checksum
```

Roadmap interpretation: research now supports "known-but-needs-integration" rather than "we do not know whether this is feasible." It may still remain a later/stretch product scope decision.

---

# 7. Colosseum / XD reference stack

Dedicated existing audit remains:

```text
docs/GAMECUBE_REFERENCE_AUDIT_2026-09-09.md
```

Best triangulation stack found so far:

```text
PKHeX
logdog2325/PokéBridge
TuxSH/PkmGCTools / LibPkmGC
Dolphin memory-card implementation
```

## PokéBridge

Modern GameCube C/homebrew reference useful for:

```text
Colosseum/XD parsing
crypto/checksums
GBA interaction
physical memory-card behavior
reparse-before-write safety
```

## PkmGCTools / LibPkmGC

Repository:

```text
TuxSH/PkmGCTools
```

Useful as an independent historical implementation for:

```text
Colosseum/XD saves
GBA Pokémon
GC <-> GBA conversion
trainer/party/PC/daycare data
```

License split noted in prior audit:

```text
LibPkmGC backend: LGPLv3
editor: GPLv3
```

Because it is old, Boost-dependent and exception-using, default role is host/reference oracle rather than direct `-fno-exceptions` Switch runtime integration.

## Dolphin

Use for:

```text
GCI wrapper behavior
memory-card directory/filesystem structures
checksums/redundant metadata
container extraction/insertion semantics
```

---

# 8. Container normalization remains a cross-console foundation

Legacy-console support strengthens the case for one explicit normalizer layer:

```text
raw cartridge SRAM
RetroArch SRM
DeSmuME DSV
No$GBA compressed
GCI
GameCube memory-card image
N64 endian variants
RTC trailer/sidecar
    -> canonical game-save bytes + preserved wrapper metadata
```

The parser should not need to know which emulator filename/container produced the normalized bytes.

No-edit export should preserve enough original wrapper/opaque data to reconstruct the source exactly where practical.

---

# 9. Recovery test levels

For each supported format, build fixtures for:

```text
valid primary / valid backup
newer damaged / older valid
older damaged / newer valid
mixed redundant regions
bad checksum
bad signature
missing sector/block
truncated file
duplicate sector/block
counter wrap/edge cases
unknown wrapper bytes
wrong endian/container
```

Then require:

```text
analysis is deterministic
original input bytes never mutate
recommended normal candidate is explainable
forensic reconstruction lists every borrowed region
unsupported/corrupt input never becomes writable automatically
```

This is the foundation for a future preservation-grade Save Rescue mode rather than a black-box "Fix Save" button.
