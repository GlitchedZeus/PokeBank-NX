# PokeBank NX — v1.0 Roadmap

Last updated: **2026-10-05**

`CURRENT_STATUS.md` is authoritative for exact live state. This roadmap describes product direction, not branch ownership.

## v1 target

PokeBank NX v1 should be a stable native Switch application that can:

- discover supported Pokémon save sources safely;
- browse Trainer / Party / Boxes / inventory;
- stage generation-aware Pokémon edits;
- present one coherent Product Home and classic game workflow;
- preserve Pokémon in a PokeBank-owned Master Vault;
- organize named Banks and collection views;
- provide provenance, Pokédex, search and validation tooling;
- launch linked games/emulators where the target can be proven;
- use touch and controller input across the full application;
- only perform source writeback through individually approved transaction adapters.

## Phase 0 — repository / safety / native foundation — COMPLETE

- [x] native `.nro` build
- [x] controller-first runtime
- [x] hard source-write locks
- [x] stable game/platform identities
- [x] source immutability discipline
- [x] host regression / sanitizer / native CI gates
- [x] full forensic audit completed
- [x] audit remediation integrated into the active Gen I–IV application lane

## Phase 1 — Gen I–III read + inventory foundation — COMPLETE / DEVICE ACCEPTED

- [x] Red / Blue / Yellow
- [x] Gold / Silver / Crystal
- [x] Ruby / Sapphire / Emerald / FireRed / LeafGreen
- [x] Trainer / Party / Boxes
- [x] generation-appropriate details
- [x] inventory
- [x] bounded legacy discovery
- [x] physical Switch acceptance

## Phase 2 — Gen I–III shared Pokémon editor — COMPLETE / DEVICE ACCEPTED

- [x] shared generation-aware View/Create/Edit
- [x] staged transactional mutation
- [x] exact-game move behavior
- [x] generation-native values only
- [x] controller navigation and editor parity
- [x] physical hardware acceptance across the current Gen I–III foundation

## Phase 3 — Generation IV shared editor — ACTIVE / HARDWARE PENDING

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
- [x] strict reparse / checksum refresh / rollback
- [x] full G4-04 implementation integrated with Product UI
- [x] current exact-head automated application gates green
- [ ] physical acceptance of the current full G4-04 + Product UI candidate

## Phase 4 — product UI / source / launch integration — ACTIVE / FINAL HARDWARE POLISH

- [x] modern Product Home
- [x] selected-game Open / Launch flow
- [x] profile/game/source presentation
- [x] Party sprite presentation
- [x] Master Vault / Pokédex visual identities
- [x] real Gen I–IV per-save Pokédex progress
- [x] Gen IV trainer-name propagation
- [x] Classic Game Sources alternate workflow
- [x] cursor-memory foundation
- [x] Backpack / Items quick-open intent foundation
- [x] compact Items / Settings quick actions
- [x] two-pane Settings organization + cursor memory
- [x] grounded trainer portraits for Gen I–IV game/gender identities
- [x] region-aware Product Home scenery and readability treatment
- [x] full Games browser used as the game/save/profile assignment surface
- [x] X = Save / Source assignment flow for validated sources
- [x] game sorting / Release Date ordering foundation
- [x] Favorites foundation
- [x] installed HOME-forwarder preference for matching GBA/DS direct launch
- [x] exact-release launch matching to avoid near-name collisions
- [x] current exact-head Host / native / Product UI / Gen IV / focused regression gates green
- [ ] physically verify current GBA/DS direct-launch and fallback behavior
- [ ] physically verify current Quick Games / Items / Search / Favorites / sorting fixes
- [ ] integrated UI hardware acceptance

## Phase 5 — full touch controls

This starts **after** the integrated UI candidate is physically accepted.

- [ ] Product Home
- [ ] Classic Games
- [ ] Settings
- [ ] Trainer / Inventory / Party / Boxes
- [ ] Pokémon View/Create/Edit
- [ ] species/move/item pickers
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
- [ ] named Banks / logical collection views

## Phase 7 — universal provider expansion

- [x] RetroArch foundation
- [x] DraStic Gen IV source foundation
- [x] melonDS Gen IV source foundation
- [ ] Tico generalized adapter
- [ ] mGBA generalized adapter
- [ ] broader manual/custom folders
- [ ] Azahar
- [ ] other supported 3DS provider wrappers
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
- [x] per-game Favorites/sorting foundation in the current Games UI
- [ ] global search/filter/favorites/recent collection tooling
- [ ] origin/current-location/provenance separation
- [ ] product-facing legality/validation state
- [ ] cries where appropriate

## Phase 11 — conversion / legality / transfer workspace

- [x] evidence-aware read-only Gen I–IV legality engine foundation
- [x] coverage-aware legality verdict model: Invalid / No problems found / Incomplete
- [x] substantial generated Gen I–IV encounter/event/RNG evidence corpus
- [ ] complete host oracle/golden fixture corpus
- [ ] generation-aware conversion
- [ ] product-integrated legality/provenance validation
- [ ] controlled creation safeguards
- [ ] staged destination representations
- [ ] COPY / MOVE / CLONE remain distinct
- [ ] source retirement only after durable validated destination

Legality development remains **analysis only** until its evidence and product integration are complete. It does not grant write permission or silently convert missing evidence into a legal verdict.

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

- [ ] diagnostics/privacy-safe export
- [ ] constrained-memory handling
- [ ] bounded caches / large-grid virtualization
- [ ] recovery UX
- [ ] accessibility / Reduced Motion
- [ ] final title/icon/NACP/startup polish
- [ ] handheld + docked pass
- [x] README / support matrix / roadmap kept synchronized with active development
- [ ] release-candidate hardware torture pass
- [ ] v1.0 tag/release

## Current critical path

```text
Gen I–III shared editor            DEVICE ACCEPTED
Gen IV full shared editor          AUTOMATED GREEN / HARDWARE PENDING
Product UI / source integration    FINAL HARDWARE POLISH
Audit remediation                  INTEGRATED INTO ACTIVE APP LANE
Legality engine                    ACTIVE IN PARALLEL / READ-ONLY
        ↓
one exact integrated hardware candidate
        ↓
full touch controls
        ↓
Master Vault + Banks
        ↓
provider / later-generation expansion
        ↓
collection / provenance / search
        ↓
conversion / legality / transfers
        ↓
approved write transactions
        ↓
release hardening
        ↓
v1.0
```
