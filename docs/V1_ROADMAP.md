# PokeBank NX — v1.0 Roadmap

Last updated: **2026-09-24**

CURRENT_STATUS.md is authoritative for the exact current engineering checkpoint. Issue #29 is the master release tracker.

## Phase 0 — repository / safety / native foundation — COMPLETE

- [x] native Switch .nro build path
- [x] controller-first shell
- [x] stable game/platform identities
- [x] hard live-source write locks
- [x] source immutability discipline
- [x] host regressions + ASan/UBSan + native gates

## Phase 1 — legacy read foundation — DEVICE ACCEPTED

- [x] Gen I Red/Blue/Yellow
- [x] Gen II Gold/Silver/Crystal
- [x] Gen III FireRed/LeafGreen/Ruby/Sapphire/Emerald
- [x] Trainer / Party / Boxes / generation-appropriate details
- [x] bounded RetroArch discovery
- [x] physical Switch acceptance

## Phase 2 — classic staged Inventory — DEVICE ACCEPTED

- [x] staged-only mutation
- [x] exact-game/exact-pocket catalogs
- [x] A Edit / X Add / Y Remove
- [x] empty valid pockets
- [x] source immutability
- [x] physical acceptance

Universal Inventory architecture remains open for future game families under issue #59.

## Phase 3 — shared Pokémon editor — DEVICE ACCEPTED FOR GEN I–III

### Gen I
- [x] shared DETAILS / VALUES / MOVES
- [x] staged Create/Edit
- [x] authentic DVs/Stat Exp
- [x] exact-game move compatibility
- [x] passive View
- [x] physical acceptance

### Gen II
- [x] shared View/Create/Edit
- [x] authentic DV/Stat Exp model
- [x] Held Item / Friendship / Pokérus
- [x] Crystal caught/met behavior
- [x] exact-game move compatibility
- [x] packed movement / multi-select
- [x] physical acceptance

### Gen III
- [x] shared View/Create/Edit for RSE/FRLG
- [x] exact-game Gen III capability provider
- [x] staged PK3 editor
- [x] IV/EV/Nature/Ability/held-item/origin presentation where native
- [x] shiny species preview interaction
- [x] editable EXP
- [x] exact-game clean move picker
- [x] D-pad / Left Stick parity
- [x] readable PID/read-only presentation
- [x] device acceptance at 996e6aa40c96e4408286dba3f60bab9144c3f60a

## Phase 4 — audit / durability / transaction hardening — MOSTLY IMPLEMENTED

Primary tracker: issue #69.

- [x] A01 durable Bank replacement
- [x] A02 custody-safe held Pokémon rollback
- [x] A03 immutable source custody + separate destination candidate
- [x] A04a durable Move journal/recovery core
- [x] A04b production Bank ↔ PokeBank-workspace true-Move software integration
- [x] A05 preserve corrupt/unreadable recovery generations
- [x] A06 BDSP truncated-layout guard
- [x] A07 newer/larger Bank write block
- [x] A08 profile/account + exact-game workspace namespaces
- [x] A09 supported single-file mutable workspace durability
- [ ] A09-BDSP recoverable multi-file generation
- [ ] F05–F13 conversion golden fixture matrix
- [ ] remaining malformed/truncated parser hardening
- [ ] N06 durable directory-generation promotion/recovery
- [ ] physical Switch FAT32/exFAT transaction recovery matrix

A04 is implemented in software for supported single-file PokeBank-owned workspaces, but physical power-loss acceptance remains open.

Cross-game true Move remains disabled until the conversion audit proves each route.

## Phase 5 — conversion fidelity / compatibility gate — CURRENT NEXT

- [ ] F05 Gen III/modern shiny preservation fixtures
- [ ] F06 PID-derived Unown form preservation fixtures
- [ ] F07/N01 PID-search exhaustion must fail explicitly
- [ ] F08 ability slot/ability-number fixtures
- [ ] F09 S/V ↔ Z-A divergent/Tera normalization fixtures
- [ ] F10 Gen III EV 252/253/255 policy fixtures
- [ ] F11 nickname/language/loss declaration fixtures
- [ ] F13 profile/account conversion provenance verification
- [ ] source bytes remain unchanged for every conversion
- [ ] destination reparse/checksum verification
- [ ] declared-loss / refusal policy for unsupported preservation
- [ ] cross-game true-Move routes enabled only after route-specific proof

## Phase 6 — Master Vault + named Banks

- [ ] immutable Pokémon entity records
- [ ] SHA-256 + stable Vault IDs
- [ ] active-location vs archival-history separation
- [ ] origin/source/platform provenance
- [ ] parent / exact-clone / derived-clone relationships
- [ ] durable journal/recovery using Phase 4 primitives
- [ ] profile-aware ownership
- [ ] named Banks as logical views/references
- [ ] legacy Storage migration/import

## Phase 7 — universal SaveSource adapters

- [ ] generalized RetroArch adapter
- [ ] mGBA
- [ ] melonDS
- [ ] DraStic
- [ ] Azahar
- [ ] custom folders
- [ ] validated unknown-but-recognized sources
- [ ] bounded/cancellable scanning
- [ ] provenance + deduplication
- [ ] source-change detection

## Phase 8 — Nintendo DS / 3DS

### DS
- [ ] Diamond / Pearl / Platinum
- [ ] HeartGold / SoulSilver
- [ ] Black / White / Black 2 / White 2

### 3DS
- [ ] X / Y
- [ ] Omega Ruby / Alpha Sapphire
- [ ] Sun / Moon / Ultra Sun / Ultra Moon

## Phase 9 — broader modern Switch validation

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

## Phase 10 — collection / Summary / provenance

- [ ] professional generation-aware Summary
- [ ] origin/current-location/provenance separation
- [ ] search/filter/favorites/recent
- [ ] National/game Pokédex
- [ ] Living Dex / Shiny Dex views
- [ ] legality state

## Phase 11 — legality / transfer / clone tooling

- [ ] PKHeX host oracle
- [ ] legality/provenance validation
- [ ] legality-aware editing
- [ ] explicit Copy / Move / Exact Clone / Derived Clone UX
- [ ] stable Vault lineage IDs
- [ ] explicit loss/warning records
- [ ] staged destination representations

## Phase 12 — individually approved write adapters

- [ ] source fingerprint
- [ ] backup
- [ ] staged mutation
- [ ] checksum/container repair
- [ ] strict reparse/validate
- [ ] write/readback/rollback
- [ ] approve source adapters individually
- [ ] no global unsafe write switch

## Phase 13 — touch / release hardening

- [ ] full touch-only handheld operation
- [ ] diagnostics/privacy-safe export
- [ ] constrained-memory handling
- [ ] bounded caches / virtualized grids
- [ ] storage health + Vault recovery UI
- [ ] accessibility / Reduced Motion
- [ ] final metadata/branding
- [ ] handheld + docked pass
- [ ] exact release artifact preservation
- [ ] RC hardware torture pass
- [ ] v1.0 tag/release

## Critical path

~~~text
Gen I / II / III editors                 DEVICE ACCEPTED
        ↓
storage / custody / transaction safety   SOFTWARE FOUNDATION BUILT
        ↓
conversion fidelity fixtures             CURRENT NEXT
        ↓
parser + physical recovery gates
        ↓
Master Vault + Banks
        ↓
SaveSource / DS / 3DS / modern expansion
        ↓
legality / provenance / approved writes
        ↓
release hardening
        ↓
v1.0
~~~
