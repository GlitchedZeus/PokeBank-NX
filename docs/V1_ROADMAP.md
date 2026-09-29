# PokeBank NX — v1.0 Roadmap

Last updated: **2026-09-29**

`CURRENT_STATUS.md` is authoritative for exact live state. This roadmap describes product direction.

## v1 target

PokeBank NX v1 should be a stable native Switch application that can safely discover supported Pokémon saves, browse Trainer / Party / Boxes / inventory, stage generation-aware edits, provide a coherent Product Home and Classic Game Sources workflow, preserve Pokémon in a PokeBank-owned Master Vault, organize Banks/collections, launch proven game targets, and eventually write back only through individually approved source transactions.

## Phase 0 — repository / safety / native foundation — COMPLETE / REMEDIATION ACTIVE

- [x] native `.nro` build
- [x] controller-first runtime
- [x] hard source-write locks
- [x] stable game/platform identities
- [x] source immutability discipline
- [x] host regression / sanitizer / native CI foundation
- [x] full tracked-repository forensic audit
- [ ] complete audit remediation matrix

## Phase 1 — Gen I–III read + inventory — COMPLETE / DEVICE ACCEPTED

- [x] Red / Blue / Yellow
- [x] Gold / Silver / Crystal
- [x] Ruby / Sapphire / Emerald / FireRed / LeafGreen
- [x] Trainer / Party / Boxes
- [x] generation-native details
- [x] inventory
- [x] bounded legacy discovery
- [x] physical Switch acceptance

## Phase 2 — Gen I–III shared Pokémon editor — COMPLETE / DEVICE ACCEPTED

- [x] shared generation-aware View/Create/Edit
- [x] staged transactional mutation
- [x] exact-game move behavior
- [x] generation-native values only
- [x] controller navigation/editor parity
- [x] physical hardware acceptance

## Phase 3 — Generation IV shared editor — ACTIVE

- [x] strict DP/Pt/HGSS read backend
- [x] persistent provider/source assignment
- [x] Party / Box View/Edit
- [x] first safe milestone hardware accepted
- [x] empty Box Add/Create
- [x] Held Item / Language / Ball / Pokérus / Met Location
- [x] native move picker and PP handling
- [x] species-compatible move filtering
- [x] Species mutation and dependent-state reconciliation
- [x] Form editing with exact-game restrictions
- [x] trainer/origin inspection
- [x] Box/Party action parity
- [ ] one fully integrated G4-04 + Product UI green candidate
- [ ] physical acceptance of that exact candidate

## Phase 4 — Product UI / source / launch integration — ACTIVE

- [x] modern Product Home
- [x] selected-game Open / Launch flow
- [x] profile/game/source presentation
- [x] Party sprites
- [x] Master Vault / Pokédex visual identities
- [x] real Gen I–IV per-save Pokédex progress
- [x] Gen IV trainer-name propagation
- [x] game/gender-grounded trainer portraits
- [x] Classic Game Sources alternate workflow
- [x] cursor-memory foundation
- [x] Backpack / Items quick-open intent
- [x] compact Items / Settings quick actions
- [x] two-pane Settings organization + cursor memory
- [x] truthful current emulator-launch/control presentation
- [ ] exact-head CI green
- [ ] verify DraStic and melonDS direct-launch handoff on hardware
- [ ] integrated UI hardware acceptance

## Phase 5 — full touch controls

Starts **after** the integrated UI candidate is physically accepted.

- [ ] Product Home
- [ ] Classic Games
- [ ] Settings
- [ ] Trainer / Inventory / Party / Boxes
- [ ] Pokémon View/Create/Edit
- [ ] species / move / item pickers
- [ ] dialogs and numeric/keyboard flows
- [ ] scrolling / back / cancel
- [ ] controller + touch coexistence
- [ ] physical touch-only acceptance

## Phase 6 — Master Vault + named Banks

- [ ] immutable authoritative Pokémon records
- [ ] stable Vault IDs + hashes
- [ ] source provenance
- [ ] parent / clone / derived relationships
- [ ] byte-for-byte Clone
- [ ] Legit Clone lineage
- [ ] journal/recovery
- [ ] profile-aware ownership
- [ ] named Banks / logical collections

## Phase 7 — provider expansion

- [x] RetroArch foundation
- [x] DraStic Gen IV source foundation
- [x] melonDS Gen IV source foundation
- [ ] generalized Tico adapter
- [ ] generalized mGBA adapter
- [ ] broader manual/custom folders
- [ ] Azahar
- [ ] other supported 3DS wrappers
- [ ] source-change detection before any future approved write

## Phase 8 — later DS / 3DS

### DS
- [x] Gen IV foundation
- [ ] Black / White / Black 2 / White 2

### 3DS
- [ ] X / Y
- [ ] Omega Ruby / Alpha Sapphire
- [ ] Sun / Moon / Ultra Sun / Ultra Moon
- [ ] validated provider integrations

## Phase 9 — modern Switch validation

- [ ] Let's Go Pikachu/Eevee production validation
- [ ] Sword/Shield
- [ ] Brilliant Diamond/Shining Pearl
- [ ] Legends: Arceus
- [ ] Scarlet/Violet
- [ ] Legends: Z-A
- [ ] explicit revision/update/DLC handling
- [ ] per-game capability matrix

## Phase 10 — collection / Pokédex / provenance / search

- [ ] global National/game Pokédex
- [ ] Living Dex / Shiny Living Dex
- [ ] search/filter/favorites/recent
- [ ] origin/current-location/provenance separation
- [ ] legality/validation state
- [ ] cries where appropriate

## Phase 11 — conversion / legality / transfers

- [ ] golden fixture corpus
- [ ] generation-aware conversion
- [ ] legality/provenance validation
- [ ] controlled creation safeguards
- [ ] staged destination representations
- [ ] COPY / MOVE / CLONE remain distinct
- [ ] source retirement only after durable validated destination

## Phase 12 — approved write transactions

- [ ] fingerprint source
- [ ] backup
- [ ] stage mutation
- [ ] repair checksums/container
- [ ] strict reparse/validate
- [ ] explicit write
- [ ] readback
- [ ] rollback/recovery
- [ ] approve adapters individually

There is no global unsafe-write switch.

## Phase 13 — release hardening

- [ ] finish all audit remediation or explicitly defer with justification
- [ ] diagnostics/privacy-safe export
- [ ] constrained-memory handling
- [ ] bounded caches / large-grid virtualization
- [ ] recovery UX
- [ ] accessibility / Reduced Motion
- [ ] final title/icon/NACP/startup polish
- [ ] handheld + docked pass
- [ ] README/support matrix/release notes match reality
- [ ] release-candidate hardware torture pass
- [ ] v1.0 tag/release

## Current critical path

```text
Gen I–III shared editor            DEVICE ACCEPTED
Gen IV full shared editor          ACTIVE
Product UI/source integration      ACTIVE
Audit remediation                  ACTIVE IN PARALLEL
        ↓
one integrated hardware candidate
        ↓
full touch controls
        ↓
Master Vault + Banks
        ↓
provider / later-generation expansion
        ↓
collections / provenance / search
        ↓
conversion / legality / transfers
        ↓
approved write transactions
        ↓
release hardening
        ↓
v1.0
```
