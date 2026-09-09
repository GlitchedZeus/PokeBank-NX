# September 9, 2026 research source manifest

This manifest records the large research pastes supplied during the September 9 PokeBank NX research session and where their unique findings were consolidated.

The long raw pastes intentionally are **not** normal Codex reading material. Their useful findings are distilled into the dedicated notes linked below so future sessions do not spend context rereading repeated/overlapping research prose.

## Source paste fingerprints

The conversation attachments available during consolidation had these local SHA-256 fingerprints:

```text
Pasted markdown.md
2078e2ba9083187c0c6e0e2f31c679198052d68c07bb8a87dedf0e273c55fd3d

Pasted markdown(1).md
4c6268dc5a0d05da750a5f3b16b9ff23d0f75037e950212e004756dcd47f620b

Pasted markdown (2).md
6e73ad0c4b4e5bdfbe9a067a84ac6c00d139f2f81e53dbb51cb5ab20e2404693

Pasted markdown(2).md
c3cbdd67c8cc6c5513c3cd4523330b132ecc0185b9583082a9e83dacca28408e

Pasted markdown (2)(1).md
5cfa3a0329a6051d719b3162bf4eaa46e7cf0211e0484ad5c9b5d49aefc1e31f

Pasted markdown (3).md
9c4ad0b632074db00fcebe5c268371b6ccc913f204cbce3887dc81583d3780cb

Pasted markdown(3).md
3a3974f99d22f0cd730959e4c79f2884fd51e165107821999631fe467ca8a15e
```

Several are revised/overlapping passes over the same research buckets. Consolidation preserves the unique findings rather than intentionally duplicating near-identical prose throughout the engineering docs.

## Consolidation targets

```text
docs/RESEARCH_CURRENT_INDEX.md
    short AI router for the whole sweep

docs/SWITCH_FILESYSTEM_SAFE_REPLACE_RESEARCH_2026-09-09.md
    libnx/fsdev, DNS-MITM_Manager, badpiggies_nx, open-handle/destination-exists failure modes

docs/research/2026-09-09/SAVE_FORMATS_PROFILES_CONTAINERS.md
    save revision fingerprints, Switch profile/container identity, RTC, wrappers, emulator discovery, localization

docs/research/2026-09-09/TRANSFER_FIXTURES_PROVENANCE.md
    generation boundaries, PKSM/PKHeX oracles, Poke_Transporter_GB/PCCS, HOME provenance, golden fixture corpuses

docs/research/2026-09-09/VAULT_DATA_EVENTS_PERFORMANCE.md
    immutable Vault, SQLite/VFS/WAL research, compact data packs, events/licensing, fuzzing, benchmark design

docs/research/2026-09-09/LEGACY_RECOVERY_GAMECUBE_STADIUM.md
    Gen III/IV recovery, Pokémon Box R&S, Stadium, Colosseum/XD, PkmGCTools, container normalization
```

## Unique findings accounted for

The consolidation includes the research sweep's material on:

```text
Switch save revision/update/DLC migration paths
SwSh/SV/BDSP/PLA/Z-A structural save detection
Switch user/profile/save-container identity
orphan/deleted profile handling
save container data/journal size metadata
Gen I/II RTC sidecars/trailers
Japanese/Korean/Western string encodings and trash bytes
Gen III and Gen IV corruption/recovery behavior
sequential generation-boundary conversions
Pal Park / Poké Transfer / Bank / HOME fixtures
HOME Tracker and Handling Trainer provenance
Nintendo savedata commit semantics
parser fuzzing
immutable/content-addressed Vault architecture
Vault schema migration
large-Vault performance benchmark requirements
Pokémon Box R&S format research
Stadium/Stadium 2 region/endian/recovery research
GameCube Colosseum/XD references
PkmGCTools / LibPkmGC
Poke_Transporter_GB and PCCS conversion modes
DeSmuME DSV normalization
No$GBA compressed-save normalization
DraStic Switch save discovery
Azahar NX 3DS save discovery
multilingual NPC gift/trade PKM corpus
Project Pokémon public save corpuses
PKMDS independent fixtures
PKSM compact Mystery Gift packs
PKHeX BinLinker / legality binary architecture
PoGoEncTool generated legality data
nxmp custom SQLite VFS findings
Switch WAL caveats
PKVault staged-session/backup concept
wcparse Wonder Card oracle
PokeTools-style event metadata verification reference
PokéAPI / Veekun generic metadata generation
EventsGallery redistribution uncertainty
fixture manifest / hashes-only policy
```

If a future session finds a missing unique result from the September 9 source pastes, add it to the appropriate dedicated note and this manifest rather than making every Codex session reread all source material.
