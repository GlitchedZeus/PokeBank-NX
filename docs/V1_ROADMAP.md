# PokeBank NX — v1.0 Roadmap

Last updated: **2026-09-23**

`CURRENT_STATUS.md` is authoritative for exact current state. Issue #29 is the master release tracker.

## Phase 0 — repository / safety / native foundation — COMPLETE

- [x] origin/upstream separation
- [x] native `.nro` build path
- [x] controller-first shell
- [x] hard live-source write locks
- [x] stable game/platform identities
- [x] source immutability discipline
- [x] host regressions + ASan/UBSan + native gates

## Phase 1 — legacy read foundation — DEVICE ACCEPTED

- [x] Gen I Red/Blue/Yellow
- [x] Gen II Gold/Silver/Crystal
- [x] Gen III FireRed/LeafGreen/Ruby/Sapphire/Emerald
- [x] Trainer / Party / Boxes / generation-appropriate details
- [x] legacy inventory browsing
- [x] bounded RetroArch discovery
- [x] malformed/unsupported safe failure
- [x] physical Switch acceptance

## Phase 2 — classic staged Inventory — DEVICE ACCEPTED

- [x] staged-only mutation
- [x] exact-game/exact-pocket catalogs
- [x] `A Edit / X Add / Y Remove`
- [x] empty valid pockets
- [x] machine move labels
- [x] source immutability
- [x] physical acceptance

Universal Inventory architecture remains open for future game families under issue #59.

## Phase 3 — shared Pokémon editor foundation — DEVICE ACCEPTED FOR GEN I–III

### Generation I
- [x] shared `DETAILS | VALUES | MOVES`
- [x] staged Create/Edit
- [x] authentic DVs/Stat Exp
- [x] exact-game move compatibility
- [x] passive View
- [x] device acceptance

### Generation II
- [x] shared View/Create/Edit
- [x] authentic DV/Stat Exp model
- [x] Held Item / Friendship / Pokérus
- [x] Crystal-native caught/met behavior
- [x] six-stat battle presentation
- [x] exact-game move compatibility
- [x] packed movement / multi-select
- [x] device acceptance

### Generation III
- [x] shared View/Create/Edit for RSE/FRLG
- [x] exact-game Gen III capability provider
- [x] staged PK3 editor
- [x] IV/EV/Nature/Ability/held-item/origin presentation where native
- [x] shiny species-preview interaction
- [x] editable EXP
- [x] exact-game clean move picker
- [x] D-pad/Left Stick parity
- [x] readable PID/read-only presentation
- [x] device acceptance at `996e6aa40c96e4408282f3d55476dae8e64968b2`

## Phase 4 — FULL AUDIT / ORGANIZATION / DURABILITY HARDENING — CURRENT

Primary tracker: issue #69.

- [ ] turn A01–A09 into regression/reproduction tests
- [ ] atomic/durable Bank replacement
- [ ] custody-safe held Pokémon rollback
- [ ] destination conversion candidate separate from original custody
- [ ] cross-store transaction journal
- [ ] preserve multiple recovery/corruption generations
- [ ] malformed/truncated parser hardening
- [ ] block writes that would truncate unsupported newer Bank formats
- [ ] profile/account namespace mutable backup workspaces
- [ ] durable replacement for mutable backup saves
- [ ] re-run conversion findings against exact-current source
- [ ] golden conversion matrix/fixtures
- [ ] repository/reference/license/documentation organization
- [ ] physical Switch crash/recovery test matrix

This phase intentionally blocks trusting Master Vault with unique Pokémon.

## Phase 5 — Master Vault + named Banks

- [ ] immutable Pokémon entity records
- [ ] SHA-256 + stable Vault IDs
- [ ] provenance + origin/current-location/history separation
- [ ] parent/clone/derived relationships
- [ ] durable journal/recovery using Phase 4 primitives
- [ ] profile-aware ownership
- [ ] named Banks as logical references/views
- [ ] legacy Storage migration/import

## Phase 6 — universal SaveSource adapters

- [ ] generalized RetroArch adapter
- [ ] mGBA
- [ ] melonDS
- [ ] DraStic
- [ ] Azahar
- [ ] custom folders
- [ ] validated unknown-but-valid sources
- [ ] bounded/cancellable scanning
- [ ] provenance + deduplication
- [ ] source-change detection

## Phase 7 — Nintendo DS / 3DS

### DS
- [ ] Diamond / Pearl / Platinum
- [ ] HeartGold / SoulSilver
- [ ] Black / White / Black 2 / White 2

### 3DS
- [ ] X / Y
- [ ] Omega Ruby / Alpha Sapphire
- [ ] Sun / Moon / Ultra Sun / Ultra Moon

## Phase 8 — modern Switch validation

- [ ] Let's Go Pikachu/Eevee
- [ ] Sword/Shield
- [ ] Brilliant Diamond/Shining Pearl
- [ ] Legends: Arceus
- [ ] Scarlet/Violet
- [ ] Legends: Z-A
- [ ] FireRed Switch / LeafGreen Switch
- [ ] explicit save revision/update/DLC handling
- [ ] strict unsupported-version behavior
- [ ] per-game capability matrix

## Phase 9 — collection / Summary / provenance

- [ ] professional generation-aware Summary
- [ ] origin/current-location/provenance separation
- [ ] search/filter/favorites/recent
- [ ] National/game Pokédex
- [ ] Living Dex / Shiny Dex views
- [ ] legality state

## Phase 10 — conversion / legality / transfer workspace

- [ ] PKHeX host oracle
- [ ] golden fixture corpus
- [ ] generation-aware conversion/compatibility
- [ ] legality/provenance validation
- [ ] legality-aware editing
- [ ] staged destination representations
- [ ] explicit COPY / MOVE / CLONE semantics

## Phase 11 — staged transactions / individually approved writes

- [ ] source fingerprint
- [ ] backup
- [ ] staged mutation
- [ ] checksum/container repair
- [ ] strict reparse/validate
- [ ] write/readback/rollback
- [ ] approve write adapters individually
- [ ] true Move only after durable verified destination

There is no global unsafe write switch.

## Phase 12 — full touch-only handheld operation

- [ ] Home/source selection
- [ ] Trainer/Inventory/Party/Boxes
- [ ] Pokémon View/Create/Edit
- [ ] Vault/Banks
- [ ] keyboard/numeric/dialog flows
- [ ] scrolling/back/cancel
- [ ] physical touch-only acceptance

## Phase 13 — release hardening

- [ ] diagnostics/privacy-safe export
- [ ] constrained-memory handling
- [ ] bounded caches / virtualized grids
- [ ] storage health + Vault recovery UI
- [ ] accessibility / Reduced Motion
- [ ] final metadata/branding
- [ ] handheld + docked pass
- [ ] exact release artifact preservation
- [ ] documentation matches release capability
- [ ] RC hardware torture pass
- [ ] v1.0 tag/release

## Critical path

```text
Gen I / II / III reads + editors       DEVICE ACCEPTED
        ↓
full audit / organization / durability CURRENT
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
approved writes
        ↓
touch-only completion
        ↓
release hardening
        ↓
v1.0
```
