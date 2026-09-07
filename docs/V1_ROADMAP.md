# PokeBank NX — v1.0 Roadmap

Last updated: 2026-09-07

This is the canonical high-level path from the current alpha to a serious v1.0 release. `CURRENT_STATUS.md` is authoritative for what is actually implemented today; issue #29 is the master release tracker.

## Product target

A serious v1.0 should be a stable controller-first Switch homebrew app that can:

- discover and read advertised Pokémon save sources safely;
- keep installed-game and emulator source files read-only unless a specific adapter later passes its write gate;
- support the core handheld/main-series lineage from GB/GBC through DS/3DS/Switch;
- support Colosseum/XD as Gen III side-game sources;
- optionally support Stadium 1/2 as stretch archival sources;
- import Pokémon into a durable Master Vault with immutable raw data, hashes and provenance;
- organize Vault entities into named Banks;
- distinguish `COPY`, true `MOVE` and intentional `CLONE`;
- provide Summary, search, Pokédex/Living Dex, legality/provenance and historical transfer tools;
- recover safely from malformed sources and interrupted transactions;
- preserve exact release artifacts and verification metadata.

## Target catalog

```text
23 current host-tested identities
+ 9 Nintendo DS
+ 8 Nintendo 3DS
+ 2 GameCube
+ 2 N64 Stadium stretch
= 44 total target identities
```

See `docs/GAME_SUPPORT_MATRIX.md`.

---

## Phase 0 — repository/safety foundation — COMPLETE

- [x] repository recovery + repeatable Git discipline;
- [x] `origin = GlitchedZeus/PokeBank-NX` writable;
- [x] PKSE upstream-only;
- [x] hard live installed-save write lock;
- [x] 23 stable release/platform IDs;
- [x] GBA FireRed/LeafGreen distinct from Switch FireRed/LeafGreen.

## Phase 1 — controller/UI hardware foundation — COMPLETE FOR DEVELOPMENT

- [x] controller-first Pokémon Action Sheet (#2);
- [x] first exact physical Switch artifact (#8);
- [x] visible PokeBank NX shell accepted (#13);
- [x] physical Left Stick navigation accepted (#19);
- [x] installed-source mutation UI locked (#23);
- [x] old/malformed PLA failure path hardened (#24);
- [x] HD Pokémon artwork packaged and visible;
- [x] artificial sprite breathing/bobbing removed;
- [x] final current red-UI artifact physically accepted.

Accepted source/artifact:

```text
af2acf043a15dbf48b8195880a80cc5de562fced
PokeBank-NX-Red-UI-af2acf04.nro
SHA-256 898df286cf34b895f1f71f4abc35f0818e4afa66725b67c2d020fc20c01bfac4
```

Broad UI work is frozen until near release. Final startup/icon/NACP polish remains under #16.

## Phase 2 — Gen III engine foundation — COMPLETE

### Host oracle / adapter

```text
936e75d98daa7e61fcf8ea199bcda958b1b78d7a
gen3: add PKSM-Core read-only FRLG adapter
```

Pinned PKSM-Core:

```text
aa22d7a4f87c0351baf7da5962ba5acd01039a7c
```

Decision: **ADAPTER-WRAPPER**.

- [x] strict FRLG save-slot/sector validation;
- [x] rotating-slot + wrap-aware selection;
- [x] Party + all 14 Boxes;
- [x] PK3 crossing a PC-sector boundary;
- [x] malformed/truncated rejection;
- [x] source immutability;
- [x] canonical PK3 fields;
- [x] inherited PKSE independent cross-check;
- [x] byte-identical untouched boxed/party PK3 round trips;
- [x] deterministic generated fixture with stable SHA-256 (#17).

Fixture:

```text
131072 bytes
SHA-256 b416aa985e459cb939caf1e1c70ce8359edf0c99a536e24d3b2a2a32b0541120
```

### Switch-native backend

```text
43f3a9f90a3314725979d59afdd68f19ee159009
gen3: build exception-free native core slice
```

- [x] same public PokeBank adapter API;
- [x] native exception-free Gen III backend;
- [x] no global C++ exceptions;
- [x] no unrelated later-generation PKSM-Core dependency graph in the Switch app;
- [x] GitHub Actions run #158 PASS.

Issue #4 is complete.

## Phase 3 — RetroArch FRLG production source — CURRENT

Issue #6.

- [ ] recover interrupted local RetroArch FRLG catalog work;
- [ ] bounded `savefile_directory` discovery;
- [ ] `.sav` / `.srm` only;
- [ ] maximum directory depth 2;
- [ ] maximum 256 candidates by default;
- [ ] structural FRLG validation before path/name hints;
- [ ] ambiguous valid FRLG remains unclassified;
- [ ] `firered_gba` / `leafgreen_gba` remain distinct from Switch IDs;
- [ ] real runtime source-discovery lifecycle invokes/owns provider;
- [ ] Party / Boxes through native Gen III backend;
- [ ] source immutability tests;
- [ ] host/sanitizer/native build green;
- [ ] physical Switch read-only test when the path is user-visible enough to warrant an artifact.

## Phase 4 — broader Gen III production reads

- [ ] Ruby;
- [ ] Sapphire;
- [ ] Emerald;
- [ ] production FireRed/LeafGreen source hardening;
- [ ] real redistributable/local-required fixtures where appropriate;
- [ ] PKHeX Oracle comparison plan/results (#5/#17).

## Phase 5 — Master Vault + Banks foundation

Issue #3 / #27.

- [ ] immutable Pokémon entity records;
- [ ] content hash + stable Vault ID;
- [ ] original source bytes retained;
- [ ] provenance/source-game/source-save metadata;
- [ ] append-only journal/recovery strategy;
- [ ] named Banks as logical references/views;
- [ ] import from read-only game/emulator sources;
- [ ] migration/clear separation from inherited writable `PKSEBANK` Storage.

## Phase 6 — Gen III side games + legacy Gen I/II

- [ ] Colosseum (#33);
- [ ] XD: Gale of Darkness (#33);
- [ ] Red/Blue/Yellow read-only adapter + RetroArch (#6);
- [ ] Gold/Silver/Crystal read-only adapter + RetroArch (#6);
- [ ] Stadium 1/2 stretch after Gen I/II core (#34).

## Phase 7 — DS / 3DS source expansion

- [ ] add 9 DS stable IDs (#30);
- [ ] Gen IV/V read-only adapters (#31);
- [ ] add 8 3DS stable IDs (#30);
- [ ] Gen VI/VII read-only adapters (#32).

## Phase 8 — modern Switch validation + professional collection UX

- [ ] modern Switch read-only adapter validation (#11);
- [ ] professional Summary + provenance (#9/#25);
- [ ] host-side PKHeX Oracle (#5);
- [ ] search/filter/favorites/recent/Quick Jump (#21);
- [ ] Vault-driven Pokédex / Living Dex / shiny views (#7);
- [ ] optional Pokémon cries later (#35).

## Phase 9 — conversion / legality / events

- [ ] compatibility + conversion engine without live writes (#10);
- [ ] historical transfer provenance;
- [ ] staged/exported destination representations;
- [ ] legality/provenance comparison against PKHeX;
- [ ] legality-aware editing / Make Shiny safeguards;
- [ ] event / Mystery Gift workflows;
- [ ] Transfer Workspace for one/multi/whole-box operations.

## Phase 10 — staged writes / approved live writes / true Move

- [ ] generic staged-save transaction framework;
- [ ] backup + fingerprint + mutate-stage + validate + write + readback + rollback;
- [ ] first individually approved live-write adapter;
- [ ] additional write adapters one-by-one;
- [ ] true `MOVE` semantics (#20);
- [ ] `COPY`, `MOVE`, `CLONE` remain distinct.

There must never be a global unsafe live-write switch.

## Phase 11 — release hardening

- [ ] diagnostics/privacy-safe export;
- [ ] applet/constrained-memory handling;
- [ ] bounded artwork caches / virtualized large grids;
- [ ] storage-health + Vault recovery UI;
- [ ] accessibility / Reduced Motion / non-color-only focus;
- [ ] final startup stages + title/icon/NACP (#16);
- [ ] handheld + docked pass;
- [ ] release-candidate hardware torture pass;
- [ ] exact release `.nro` + source SHA + size + SHA-256 preserved;
- [ ] README/support matrix/release notes match actual capability;
- [ ] v1.0 tag/release.

---

## Current critical path

```text
recover + finish RetroArch FRLG runtime read path
        ↓
broader Gen III production reads
        ↓
Master Vault + Banks
        ↓
Colosseum/XD + Gen I/II
        ↓
DS + 3DS
        ↓
modern Switch validation + Oracle + Summary
        ↓
conversion / Dex / legality / events
        ↓
staged writes → per-adapter live writes → true Move
        ↓
release hardening / RC / v1.0
```

For the next coding session, use `docs/NEXT_CODEX_PROMPT.md` rather than pasting a giant prompt.
