# PokeBank NX — v1.0 Roadmap

Last updated: 2026-09-14

`CURRENT_STATUS.md` is authoritative for exact current state. Issue #29 is the master release tracker.

## Product target

PokeBank NX v1 should be a stable native Switch application that can safely discover advertised Pokémon save sources, browse and stage edits, preserve Pokémon in a profile-aware Master Vault, organize named Banks, provide generation-aware Summary/provenance/search/Dex tooling, and only perform source writeback through individually approved adapters.

GameCube/Stadium remain valuable later expansion targets and do not block v1.

## Phase 0 — repository / safety / native foundation — COMPLETE

- [x] origin/upstream separation
- [x] native `.nro` build path
- [x] controller-first shell
- [x] hard live-source write locks
- [x] stable game/platform identities
- [x] source immutability discipline
- [x] host regressions + ASan/UBSan + native build gates

## Phase 1 — legacy read foundation — COMPLETE / DEVICE ACCEPTED

- [x] Gen I Red/Blue/Yellow read-only
- [x] Gen II Gold/Silver/Crystal read-only
- [x] Gen III FireRed/LeafGreen/Ruby/Sapphire/Emerald read-only
- [x] Trainer / Party / Boxes
- [x] generation-appropriate Pokémon details
- [x] legacy inventory browsing
- [x] bounded RetroArch battery-save discovery
- [x] malformed/unsupported safe failure
- [x] physical Switch acceptance for the tracked read milestone

## Phase 2 — shared Inventory editor — COMPLETE FOR CURRENT CLASSIC MILESTONE / DEVICE ACCEPTED

- [x] staged inventory mutation
- [x] `A Edit / X Add / Y Remove`
- [x] exact-game + exact-pocket catalogs
- [x] empty valid pocket handling
- [x] TM/HM/TR move-name display
- [x] Key Item warnings
- [x] semantic Pending Changes
- [x] source immutability
- [x] physical hardware acceptance

Issue #59 remains open because the universal inventory architecture must continue forward into future DS/3DS/modern adapters.

## Phase 3 — shared Pokémon editor foundation — IN PROGRESS

### Generation I — COMPLETE / DEVICE ACCEPTED

- [x] capability-driven `DETAILS | VALUES | MOVES`
- [x] staged Create/Edit
- [x] transactional existing-Pokémon Edit sessions
- [x] authentic DVs/Stat Exp
- [x] DV-derived shiny state
- [x] exact-game move compatibility
- [x] five-stat battle presentation/radar
- [x] passive no-cursor View presentation
- [x] physical Switch acceptance

### Generation II — CURRENT / PR #68 / HARDWARE PENDING

Current implementation includes:

- [x] passive shared View shell
- [x] Gold/Silver vs Crystal exact-game move compatibility data
- [x] passive `OK` / `Unusual preserved` compatibility status
- [x] shared Create/Edit surface
- [x] local Add/Edit drafts
- [x] transactional Keep/Discard/Continue Edit flow
- [x] authentic DVs + derived HP DV
- [x] one Special DV feeding split SpA/SpD battle display
- [x] six-stat / six-axis battle presentation
- [x] Held Item picker
- [x] Friendship
- [x] Pokérus
- [x] Crystal-specific caught/met support
- [x] DV-derived shiny
- [x] species/Attack-DV gender
- [x] preservation of unusual existing move bytes
- [x] permanent compatibility/passive-view/editor-surface tests
- [ ] freeze one exact final candidate SHA
- [ ] full latest-head host/regression pass
- [ ] ASan / UBSan latest-head pass
- [ ] devkitA64 + final NRO link latest-head pass
- [ ] complete RomFS / embedded source identity verification
- [ ] exact candidate artifact package
- [ ] physical Gold/Silver/Crystal acceptance

### Generation III — NEXT AFTER GEN II ACCEPTANCE

- [ ] bring RSE/FRLG boxed Pokémon editing onto the same shared capability-driven shell
- [ ] expose exact Gen III-native fields only
- [ ] retain accepted read foundation
- [ ] preserve source immutability
- [ ] exact hardware acceptance before moving on

## Phase 4 — Master Vault + named Banks

- [ ] immutable Pokémon entity records
- [ ] SHA-256 + stable Vault IDs
- [ ] source game/save/platform provenance
- [ ] parent/clone/derived relationships
- [ ] journal/recovery
- [ ] profile-aware ownership
- [ ] named Banks as logical references/views
- [ ] migration/import path from legacy storage where appropriate

## Phase 5 — universal SaveSource adapters

- [ ] RetroArch adapter generalized
- [ ] Tico
- [ ] mGBA standalone
- [ ] melonDS standalone/libretro
- [ ] DraStic
- [ ] Azahar
- [ ] custom folders
- [ ] unknown-but-valid validated saves
- [ ] bounded fast scan + cancellable deep scan
- [ ] source provenance + deduplication
- [ ] source-change detection before future writes

## Phase 6 — Nintendo DS / 3DS

### DS

- [ ] Diamond / Pearl / Platinum
- [ ] HeartGold / SoulSilver
- [ ] Black / White / Black 2 / White 2
- [ ] validated read adapters
- [ ] source-provider integrations

### 3DS

- [ ] X / Y
- [ ] Omega Ruby / Alpha Sapphire
- [ ] Sun / Moon / Ultra Sun / Ultra Moon
- [ ] validated read adapters
- [ ] Azahar/source-provider integrations

## Phase 7 — modern Switch validation

- [ ] Let's Go Pikachu/Eevee
- [ ] Sword/Shield
- [ ] Brilliant Diamond/Shining Pearl
- [ ] Legends: Arceus
- [ ] Scarlet/Violet
- [ ] Legends: Z-A
- [ ] FireRed Switch / LeafGreen Switch
- [ ] explicit save-revision/update/DLC handling
- [ ] strict unsupported-version behavior
- [ ] per-game read/edit/write capability matrix

## Phase 8 — collection / Summary / provenance

- [ ] professional generation-aware Summary
- [ ] origin/current-location/provenance separation
- [ ] search/filter/favorites/recent
- [ ] National/game Pokédex
- [ ] Living Dex / shiny collection views
- [ ] legality state
- [ ] optional cry playback where appropriate

## Phase 9 — conversion / legality / transfer workspace

- [ ] host PKHeX Oracle
- [ ] golden fixture corpus
- [ ] generation-aware compatibility/conversion
- [ ] legality/provenance validation
- [ ] legality-aware editing
- [ ] controlled creation / Make Shiny safeguards
- [ ] staged destination representations
- [ ] `COPY`, `MOVE`, `CLONE` remain distinct

## Phase 10 — staged transaction framework / approved writes

- [ ] fingerprint source
- [ ] backup
- [ ] stage mutation
- [ ] repair checksums/container
- [ ] strict reparse/validate
- [ ] write
- [ ] readback
- [ ] rollback/recovery
- [ ] approve live-write adapters individually
- [ ] true Move only after durable validated destination

No global unsafe write switch.

## Phase 11 — full touch-only coverage

- [ ] Home/source selection
- [ ] Trainer/Inventory/Party/Boxes
- [ ] Pokémon View/Create/Edit
- [ ] Vault/Banks
- [ ] keyboard/numeric/dialog flows
- [ ] scrolling/back/cancel
- [ ] controller + touch coexistence
- [ ] physical touch-only acceptance

## Phase 12 — release hardening

- [ ] diagnostics/privacy-safe export
- [ ] constrained-memory handling
- [ ] bounded caches / virtualized large grids
- [ ] storage health + Vault recovery UI
- [ ] accessibility / Reduced Motion
- [ ] final title/icon/NACP/startup polish
- [ ] handheld + docked pass
- [ ] exact release artifact preservation
- [ ] README/support matrix/release notes match reality
- [ ] release-candidate hardware torture pass
- [ ] v1.0 tag/release

## Critical path

```text
legacy Gen I/II/III reads            DEVICE ACCEPTED
classic Inventory                    DEVICE ACCEPTED
Gen I boxed Pokémon editor           DEVICE ACCEPTED
Gen II shared Pokémon editor         CURRENT
        ↓
Gen III shared Pokémon editor
        ↓
Master Vault + Banks
        ↓
universal SaveSource adapters
        ↓
DS / 3DS
        ↓
modern Switch validation
        ↓
Summary / Dex / provenance / search
        ↓
conversion / legality / transfers
        ↓
staged transaction framework
        ↓
approved writes / true Move
        ↓
full touch-only completion
        ↓
release hardening
        ↓
v1.0
```
