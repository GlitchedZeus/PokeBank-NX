# PokeBank NX — Research intake 2026-09-09

> **Research/navigation only.** This file does not expand the active milestone. `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md` remain authoritative.

The September 9 research sweep grew large enough that the detailed findings are now split into focused subsystem notes so Codex/Astra does not have to reread one giant intake file.

## Start here

```text
docs/RESEARCH_CURRENT_INDEX.md
```

That router tells the AI which single research note to open for the active subsystem.

## Focused notes

```text
docs/SWITCH_FILESYSTEM_SAFE_REPLACE_RESEARCH_2026-09-09.md
    immediate LeafGreen/libnx/fsdev safe-replace research

docs/research/2026-09-09/SAVE_FORMATS_PROFILES_CONTAINERS.md
    structural revisions, profiles/save identity, RTC, wrappers, emulator discovery, localization

docs/research/2026-09-09/TRANSFER_FIXTURES_PROVENANCE.md
    generation boundaries, Poke_Transporter_GB/PCCS, HOME provenance, real golden fixtures

docs/research/2026-09-09/VAULT_DATA_EVENTS_PERFORMANCE.md
    immutable Vault, SQLite/VFS, binary data packs, legality, events, fuzzing, benchmark requirements

docs/research/2026-09-09/LEGACY_RECOVERY_GAMECUBE_STADIUM.md
    Gen III/IV recovery, Pokémon Box R&S, Stadium, Colosseum/XD, PkmGCTools

docs/research/2026-09-09/SOURCE_MANIFEST.md
    source-paste fingerprints and unique-finding coverage map
```

## Existing master/reference docs still apply

```text
docs/RESEARCH_REFERENCE_MATRIX.md
    external-project classification / licensing posture

docs/GAMECUBE_REFERENCE_AUDIT_2026-09-09.md
    dedicated Colosseum/XD audit

docs/PKHEX_ORACLE.md
    host correctness/oracle policy

docs/PKSM_CORE_INTEGRATION.md
    host-oracle vs native exception-free integration boundary
```

## High-value references preserved from the original intake

```text
switchbrew/libnx
DNS-MITM_Manager
badpiggies_nx
JKSV
current upstream kiasta/PKSE
pkHouse / pkBakery
PKSM-Core
DrasticDS_nx
Azahar NX
PKHeX
Poke_Transporter_GB
Pokemon-Community-Conversion-Standard
PoGoEncTool
PokeFinder
EventsGallery
PKSM Mystery Gift packs
wcparse
uNSS transaction model
PokéBridge
PkmGCTools / LibPkmGC
Dolphin
Project Pokémon fixture/research corpuses
RoCs-PC / NX_Saves-style corpuses
PokéAPI / Veekun generic metadata sources
```

## Permanent conclusions from the sweep

```text
GameVersion != StructuralRevision
unknown structural revision => not writable
filesystem save identity != human profile display identity
container normalization happens before the Pokémon parser
preserve raw/opaque/text bytes on no-edit roundtrip
recovery framework generic, selection rules game-specific
conversion is sequential generation-boundary logic
never fabricate HOME Tracker / official server provenance
ordinary SD transactions != mounted Horizon savedata transactions
Vault authoritative objects/manifests should survive index/database loss
compact host-generated data packs are preferred for large runtime datasets
fixture provenance and redistribution rights are tracked explicitly
```

## Still unresolved

```text
exact Switch power-loss durability boundaries
real 10k-250k Vault performance on hardware
blanket redistribution rights for large event corpuses
perfect official before/after fixtures for every transfer boundary/language
final file-level license review before any external code/data reuse
```

Do not read the old monolithic version of this intake from Git history during normal development. Use the focused notes above. Git history retains the earlier form for archaeology if ever needed.
