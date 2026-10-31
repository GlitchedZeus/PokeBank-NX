# PokeBank NX — v1.0 Roadmap

> **ROADMAP, NOT BRANCH AUTHORITY:** This file describes current product direction as a dated snapshot. Re-fetch GitHub and use `docs/ENGINEERING_AUTHORITY.md` for active lane/task routing.

## OWNER-LOCKED RELEASE ORDER — OCTOBER 17, 2026

**This current release order supersedes older speculative phase labels below; it does not authorize unsafe merges or writes.**

1. **Gen I–V shared-UI repair round:** [app #307](https://github.com/GlitchedZeus/PokeBank-NX/pull/307) addresses the owner-reported Bag Add/quantity ordering, Gen V PP/PP Ups and left-panel clipping, plus compact shared pickers, live prefix species search, draggable scrollbars, Gen I Trainer/Money, Crystal Met Time and other shared polish. Preserve the **42 owner-confirmed passes** from [`7fea1797`](hardware/OWNER_RETEST_7fea1797_20261017.md).
2. **Hardware retest / targeted defect round:** one exact GitHub-built NRO, native/RomFS, GCC/Clang, sanitizers, QA cross-gen contracts, app-owned export/reparse, independent exact-binary audit and Switch verification. Distinguish six problem/investigation IDs from six other unverified checks in the 54-item retest. Test exported `edited.sav` with the corresponding emulator; original saves remain untouched. More than two passes are permitted if a safety blocker remains.
3. **Finish and review [Gen I–V legality #314](https://github.com/GlitchedZeus/PokeBank-NX/pull/314):** PK1–PK4 production analyzers and a PK5 engine path exist on the separate legality branch. Finish source-backed GameCube/Orre and historical provenance where possible, retain Unknown where necessary, rerun full PKHeX differentials and rule audits, and prepare a reviewed Gen V app adapter.
4. **Reviewed integration after app stabilization:** preserve both branch histories, resolve conflicts forward, test combined app+legality safely and publish one combined exact-NRO candidate, not a false declaration of global PKHeX completeness.
5. **Full device acceptance:** [all 20 titles and every reachable action](handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md) under [#313](https://github.com/GlitchedZeus/PokeBank-NX/issues/313), all required providers/profiles, control/touch, launch/return, staged Review/Discard/Export and source immutability. Only the owner can approve the exact NRO SHA-256.
6. **Post-acceptance:** independent final audit, README and project-state review, safe PR/branch consolidation preserving unique commits, then owner-authorized **Gen VI–IX application and new-emulator development**. Gen VI/VII/VIII drafts [#309](https://github.com/GlitchedZeus/PokeBank-NX/pull/309), [#311](https://github.com/GlitchedZeus/PokeBank-NX/pull/311), [#312](https://github.com/GlitchedZeus/PokeBank-NX/pull/312) are preserved research, not current release scope.

**Latest tested artifact** is [Gen I–V candidate `7fea1797`](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38946941097/artifacts/12066747065), exact NRO SHA-256 `217f24becaaa4eb940abaef01962c80a2fb5536bc6af77934add92d900bac1d5`. It passed the independent NRO audit and received **42/54 targeted owner-reported checks**, **not** full device acceptance. App #307 and legality #314 remain open draft workstreams.

The rest of this roadmap preserves historical development phases and longer-term capabilities; date-specific claims in older sections do not supersede this release order.

---

Last updated: **2026-10-14**

`CURRENT_STATUS.md` and the [October 14 development snapshot](ENGINEERING_SNAPSHOT_20261014.md) track exact live branch/CI evidence. This roadmap is the long-term product direction, not a claim of merged code or full PKHeX legality parity.

## v1 target

**Progressive release contract:** a tested safe editor plus whatever legality rules are demonstrably verified may ship before exhaustive PKHeX coverage. Expand reliable generation-native formats and the legality rule set in subsequent v1.x releases. Persistent Vault/Banks and live source writes must not be described as finished before their own acceptance gates.

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

## Phase 3 — Generation IV shared editor — FOUNDATION IMPLEMENTED / NEW CANDIDATE HARDWARE PENDING

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

## Phase 4 — product UI / source / launch integration — ACTIVE CLAUDE FINALIZATION #306

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
- [x] Black / White / Black 2 / White 2 — experimental shared staged PK5, 24 boxes, Create/Review/export and profile assignment implemented in [#300](https://github.com/GlitchedZeus/PokeBank-NX/pull/300)
- [ ] **physical Switch acceptance** of Gen V source discovery/assign/reopen/editor using real existing emulator saves
- [ ] reconcile separately tested [#304](https://github.com/GlitchedZeus/PokeBank-NX/pull/304) source-hardening changes only after review

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

## Current critical path — October 14, 2026

| Priority | Implemented / underway | Required next gate |
|---|---|---|
| Gen I–IV application | Owner-accepted foundation and Claude's [finalization #306](https://github.com/GlitchedZeus/PokeBank-NX/pull/306) | Complete the remaining full host check at `3ac0ae29`; packaged artwork, Gen IV, touch and native candidate checks passed; one owner-tested NRO still required |
| Gen V application | [#300](https://github.com/GlitchedZeus/PokeBank-NX/pull/300) staged PK5 editor / Create / profile assignment / app-owned export is **six-check green** | Owner tests the existing same-folder RetroArch/DraStic/melonDS saves, selected retail copy, persistent profile link, safe edits and exit; evaluate stacked #304 |
| Touch / Items / Product UI | HOME-like cards, Quick Games, shared native action paths | All-controller/handheld touch parity and stable modal navigation on real Switch |
| Read-only legality | Gen I–IV five-lane coordinator [#298](https://github.com/GlitchedZeus/PokeBank-NX/pull/298), Claude [#303](https://github.com/GlitchedZeus/PokeBank-NX/pull/303), later-format foundations [#301](https://github.com/GlitchedZeus/PokeBank-NX/pull/301) | Resolve exact combined host/PKHeX/source CI blockers, independently source-audit rule applicability, preserve Unknown/Incomplete |
| Future product layers | Vault/Banks, Dex/Search, transfer workflows, additional generations, per-source live transactions | Real tested backends and separate authorization; no premature release claim |

**Last owner-accepted integrated application:** `0760334f` (Oct 8). **Gen V six-check-green candidate:** `58bd564b` / [Actions artifact](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38523683607); **not hardware accepted**. An exact NRO hash and user device pass must precede each production milestone.

A green PR is not the same as a finished release. No fabricated completion percentages, silent source overwrite, cross-game True Move unlock or unsafe global Inject Save.
