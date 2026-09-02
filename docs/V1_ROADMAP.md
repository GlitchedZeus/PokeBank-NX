# PokeBank NX — v1.0 Roadmap

Last updated: 2026-09-02

This is the canonical end-to-end roadmap from the current alpha to a serious PokeBank NX v1.0 release.

Master tracking issue: **#29 — Track PokeBank NX v1.0 roadmap and release gates**.

`PROJECT_STATUS.md` remains authoritative for what is actually implemented/tested today. Planned items in this file are not implementation claims.

## Status rules

```text
[x] COMPLETE      = milestone evidence recorded
[ ] IN PROGRESS   = active work exists but acceptance gate incomplete
[ ] PLANNED       = not complete
[ ] STRETCH       = useful, but allowed to slip past v1 if cost is disproportionate
```

Physical evidence remains stricter:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED — PASS / PARTIAL PASS / FAIL
```

---

# v1.0 product target

A serious v1.0 should be a stable controller-first Switch application that can:

- safely discover/import advertised Pokémon save sources;
- keep installed-game sources read-only unless that exact adapter passes the live-write gate;
- support the core handheld/main-series lineage from GB/GBC through DS/3DS/Switch;
- support Pokémon Colosseum/XD GameCube saves as useful Gen III side-game sources;
- support Stadium 1/2 N64 storage as a legacy/stretch archival source if implementation cost stays reasonable;
- import Pokémon into a durable Master Vault with immutable raw data, hashes, origin and provenance;
- organize Vault entities into named Banks without accidental raw duplication;
- distinguish `COPY`, true `MOVE`, and deliberate `CLONE`;
- convert compatible Pokémon between supported generations with explicit provenance and validation;
- provide professional Summary, search, Pokédex/Living Dex and collection tools;
- support legality-aware editing/generation and event workflows where rules/data are understood;
- recover safely from malformed saves and interrupted Vault transactions;
- preserve exact release artifacts and verification metadata.

## Target catalog

Current source registry:

```text
23 host-tested release/platform identities
```

Planned expansion:

```text
+ 9 Nintendo DS
+ 8 Nintendo 3DS
+ 2 Nintendo GameCube (Colosseum/XD)
+ 2 Nintendo 64 Stadium
= 44 total release/source targets
```

The 21 planned additions are not `IDENTITY TESTED` until source IDs and tests exist.

Stadium is a **stretch** target and must not block core v1.

Detailed matrix: `docs/GAME_SUPPORT_MATRIX.md`.

---

# Phase checklist

## Phase 0 — Repository recovery / continuity — COMPLETE

- [x] recover usable history after interrupted work;
- [x] `origin = GlitchedZeus/PokeBank-NX` writable;
- [x] PKSE remains upstream-only;
- [x] distinguish application-source commits from later docs commits;
- [x] repeatable runbook/status/build discipline.

## Phase 1 — Save-safety hard lock / current identity foundation — COMPLETE FOUNDATION

- [x] low-level live installed-save write/restore hard lock;
- [x] legacy `injectToGame=1` disabled/ignored;
- [x] 23 stable current source identities;
- [x] GBA FireRed/LeafGreen distinct from Switch FireRed/LeafGreen;
- [ ] IN PROGRESS — inherited mutation UI audit above the lower-level lock (#23).

## Phase 2 — Controller-first Action Sheet — COMPLETE

- [x] A opens deliberate Pokémon actions rather than immediate mutation;
- [x] shared Party/Boxes/Storage Action Sheet;
- [x] B/Cancel zero-mutation behavior;
- [x] host/sanitizer/native build validation;
- [x] behavior physically exercised on the first combined device build.

Issue: #2.

## Phase 3 — HOME-style controls/themes — COMPLETE FOUNDATION / DEVICE PARTIAL

- [x] D-pad navigation;
- [x] contextual `+` / read-only `-` foundation;
- [x] OLED Black / Dark / Light;
- [x] theme persistence;
- [x] reusable cards/panels/modals/focus;
- [ ] IN PROGRESS — final controller normalization (#26).

## Phase 4 — First exact physical Switch milestone — COMPLETE

- [x] exact `.nro` physically launched;
- [x] source SHA / filename / size / SHA-256 recorded;
- [x] short and extended hardware reports recorded;
- [x] failures turned into tracked work.

Exact device-tested source:

```text
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
```

## Phase 5 — Visible PokeBank NX shell + real analog input — IN PROGRESS

- [x] visible PokeBank NX shell/chrome source checkpoint;
- [x] real libnx Left Stick position handling source checkpoint;
- [x] deadzone/hysteresis + host repeat tests reported;
- [x] NRO/window identity source work;
- [ ] physical visual acceptance (#13/#16);
- [ ] physical Left Stick acceptance (#19).

Useful source checkpoint:

```text
361c6f551496470db305948d702944c6ed9889c1
```

## Phase 6 — Safety/crash finish + second exact device build — CURRENT BLOCKER

- [ ] #23 classify Release/Create/Move/Edit/save/Storage persistence paths;
- [ ] block unsafe installed-source mutation UI;
- [ ] preserve low-level write lock;
- [ ] #24 harden old/malformed/unsupported PLA path;
- [ ] preserve Session 2.5 visible-shell + analog work;
- [ ] host tests PASS;
- [ ] ASan/UBSan PASS;
- [ ] `git diff --check` PASS;
- [ ] native `.nro` builds;
- [ ] new exact app-source checkpoint if source changes;
- [ ] replacement artifact filename/size/SHA-256 preserved;
- [ ] second physical Switch test.

Prompt: `docs/PROMPT_SESSION2_6_SAFETY_CRASH_FINISH.md`.

## Phase 7 — Expanded source identity catalog — PLANNED

Issue #30 plus side-game issues.

- [ ] 9 DS stable IDs;
- [ ] 8 3DS stable IDs;
- [ ] `colosseum_gc`;
- [ ] `xd_gale_of_darkness_gc`;
- [ ] `stadium_n64`;
- [ ] `stadium2_n64`;
- [ ] host uniqueness/stability tests;
- [ ] existing 23 IDs remain unchanged.

Issues: #30, #33, #34.

## Phase 8 — PKSM-Core Gen III spike — PLANNED NEXT MAX AFTER DEVICE GATE

- [ ] PK3 / Sav3 build/dependency audit;
- [ ] FireRed/LeafGreen GBA read-only parse;
- [ ] Party/Box extraction;
- [ ] active save slot/sectors;
- [ ] encryption/decryption/checksum behavior;
- [ ] compare PKSM-Core + full PKSM + Pokémon Chest + PKHeX;
- [ ] choose DIRECT REUSE / ADAPTER-WRAPPER / SELECTIVE PORT / REFERENCE ONLY.

Issue: #4.

## Phase 9 — Gen III main-series read adapters — PLANNED

- [ ] FireRed / LeafGreen GBA;
- [ ] Ruby / Sapphire / Emerald;
- [ ] malformed/truncated/checksum rejection;
- [ ] common read-only source interface;
- [ ] golden fixtures;
- [ ] physical Switch file-import/browse validation.

Issues: #4, #6, #17.

## Phase 10 — Master Vault v1 — PLANNED

- [ ] immutable raw entity storage;
- [ ] stable Vault IDs;
- [ ] SHA-256 integrity;
- [ ] source/origin provenance;
- [ ] active/current location separate from origin/history;
- [ ] parent/derived lineage;
- [ ] transaction journal;
- [ ] interrupted-transaction recovery;
- [ ] persistent reopen after restart;
- [ ] rebuildable metadata/search indexes;
- [ ] corruption/migration tests informed by PKSM BankFile.

Issue: #3.

## Phase 11 — Named Banks + legacy Storage migration — PLANNED

- [ ] named Banks reference Vault IDs;
- [ ] multiple views do not duplicate raw entity payloads;
- [ ] Bank deletion cannot silently destroy Vault entity;
- [ ] classify current inherited Storage (#27);
- [ ] preserve/migrate legitimate Storage Pokémon where feasible;
- [ ] optional PKSMBANK importer after Vault v1.

Issues: #3, #27.

## Phase 12 — GameCube Colosseum/XD read-only support — PLANNED

Issue #33.

- [ ] add/host-test GameCube identities;
- [ ] `.gci` import;
- [ ] `.raw` / `.bin` GameCube memory-card parsing where practical;
- [ ] Dolphin-compatible source handling where practical;
- [ ] Colosseum Party/storage extraction;
- [ ] XD Party/storage extraction;
- [ ] Shadow/Purification provenance where technically available;
- [ ] malformed/container/checksum rejection;
- [ ] PKHeX comparison (`SAV3Colosseum`, `SAV3XD`, `SAV3GCMemoryCard`);
- [ ] physical Switch read/import test.

Why before many later generations: these are Gen III sources and can reuse the Gen III entity/Vault foundation.

## Phase 13 — Gen I / II + RetroArch legacy reads — PLANNED

- [ ] Red / Blue / Yellow;
- [ ] Gold / Silver / Crystal;
- [ ] RetroArch source discovery;
- [ ] manual import fallback;
- [ ] language/region handling;
- [ ] compare PHBankGBC only as secondary reference;
- [ ] validate against PKSM-Core/PKHeX/fixtures.

Issue: #6.

## Phase 14 — Stadium 1 / Stadium 2 read-only source adapters — STRETCH

Issue #34.

- [ ] Stadium 1 save/storage recognition;
- [ ] Stadium 2 save/storage recognition;
- [ ] actual box/storage extraction;
- [ ] registered-team extraction where useful;
- [ ] checksum/backup handling;
- [ ] region/version rejection or explicit support;
- [ ] emulator/manual save import;
- [ ] PKHeX `SAV1Stadium` / `SAV2Stadium` comparison;
- [ ] physical Switch read/import test.

Important historical rule:

```text
Stadium/Gen I-II entity → later generation
```

may be converted by PokeBank NX later, but there was no normal official Gen II → Gen III direct transfer path. Provenance must preserve that historical break and never imply official HOME directly imported a Stadium save.

If Stadium container/region complexity becomes expensive, this phase may move post-v1 without blocking the core release.

## Phase 15 — Nintendo DS Gen IV/V read adapters — PLANNED

Issue #31.

Generation IV:

- [ ] Diamond / Pearl / Platinum;
- [ ] HeartGold / SoulSilver.

Generation V:

- [ ] Black / White;
- [ ] Black 2 / White 2.

Source paths:

- [ ] melonDS-compatible saves;
- [ ] RetroArch DS paths where applicable;
- [ ] validated conventional `.sav` imports;
- [ ] manual fallback.

Validation:

- [ ] PK4/PK5 + Sav4/Sav5 audit;
- [ ] PKSM-Core / Pokémon Chest / PKHeX comparison;
- [ ] malformed/version rejection;
- [ ] fixtures + physical Switch browse/import test.

## Phase 16 — Nintendo 3DS Gen VI/VII read adapters — PLANNED

Issue #32.

Generation VI:

- [ ] X / Y;
- [ ] Omega Ruby / Alpha Sapphire.

Generation VII:

- [ ] Sun / Moon;
- [ ] Ultra Sun / Ultra Moon.

Source paths:

- [ ] decrypted Checkpoint/JKSM-style exports;
- [ ] Citra/Lime3DS-compatible files/directories where practical;
- [ ] manual `main`/save import.

3DS VC R/B/Y/G/S/C should initially use original GB/GBC identities with 3DS-VC source provenance unless technical evidence requires new IDs.

## Phase 17 — Professional Summary + provenance — PLANNED

- [ ] generation-aware Summary fields;
- [ ] origin vs active/current location;
- [ ] transfer/provenance history;
- [ ] `UNKNOWN != LEGAL`;
- [ ] Pokémon sprite/artwork/render (#25);
- [ ] intentional missing-art fallback;
- [ ] optional Right Stick 3D rotation only if practical.

Issues: #9, #25.

## Phase 18 — Golden corpus + PKHeX Oracle — PLANNED

- [ ] versioned `.pk*` fixtures;
- [ ] save fixtures per supported family;
- [ ] malformed/corrupt variants;
- [ ] GameCube memory-card / `.gci` cases;
- [ ] Stadium storage cases if #34 pursued;
- [ ] DS/3DS family fixtures;
- [ ] SHA-256 manifest;
- [ ] host PKHeX `inspect` / `legality` / `convert`;
- [ ] machine-readable comparisons.

Issues: #5, #17.

## Phase 19 — Modern Switch read-adapter validation — PLANNED

Issue #11.

- [ ] Switch FireRed / LeafGreen;
- [ ] LGPE;
- [ ] Sword / Shield;
- [ ] BDSP;
- [ ] Legends: Arceus;
- [ ] Scarlet / Violet;
- [ ] Legends: Z-A;
- [ ] malformed/truncated/version mismatch rejection;
- [ ] title/container facts cross-checked;
- [ ] capability matrix updated per family.

## Phase 20 — Transfer Workspace / high-volume organization UX — PLANNED

- [ ] Game ↔ Vault/Bank workspace inspired by PHBank/Pokémon Chest;
- [ ] one-Pokémon selection;
- [ ] multi-select;
- [ ] whole-box operation UI where safe/supported;
- [ ] clear COPY / MOVE / CLONE semantics;
- [ ] preview before future destructive transactions.

## Phase 21 — Conversion + compatibility engine v1 — PLANNED

Issue #10.

- [ ] `SUPPORTED / SUPPORTED_WITH_CHANGES / UNSUPPORTED / UNKNOWN`;
- [ ] convert supported entity formats;
- [ ] preserve origin + parent/derived provenance;
- [ ] PKHeX Oracle comparisons;
- [ ] `.pk*` export;
- [ ] staged destination-save representation;
- [ ] transaction/journal state.

Special provenance cases:

- [ ] Colosseum/XD remain identified as GameCube Gen III origins;
- [ ] Stadium/Gen I-II forward conversions record the historical transfer discontinuity rather than pretending an official uninterrupted path existed.

## Phase 22 — Vault-driven Pokédex / Living Dex — PLANNED

Issue #7.

- [ ] National Dex;
- [ ] normal/shiny ownership from Vault;
- [ ] missing/owned filters/counts;
- [ ] regional Dex data;
- [ ] forms/gender distinctions;
- [ ] Alpha/Shiny Alpha where relevant;
- [ ] Living Dex / Shiny Living Dex views;
- [ ] rebuildable index.

## Phase 23 — Legality-aware editing / shiny operations — PLANNED

- [ ] edit app-owned/Vault entities safely;
- [ ] pre/post legality checks where available;
- [ ] `Make Shiny` respects shiny locks/rules;
- [ ] conservative UNKNOWN;
- [ ] derived provenance instead of silent original overwrite.

## Phase 24 — Encounter/legal generation — PLANNED

- [ ] encounter-driven generation;
- [ ] trainer/encounter consistency;
- [ ] form/shiny constraints;
- [ ] generation move/ability/item constraints;
- [ ] host-oracle comparisons.

No guaranteed online/ban-safety claims.

## Phase 25 — Bulk Vault / Living Dex helpers — PLANNED

- [ ] collection generation helpers;
- [ ] Living Dex assistance;
- [ ] shiny helpers where legal;
- [ ] dedupe/idempotency;
- [ ] explicit generated provenance.

## Phase 26 — Mystery Gifts / event archive — PLANNED

- [ ] audit PKSM Wondercard/event behavior;
- [ ] data licensing/provenance plan;
- [ ] historical distribution browsing where permitted;
- [ ] supported Wondercard import/export;
- [ ] event specimen provenance/legality.

## Phase 27 — Advanced transfer compatibility — PLANNED

- [ ] generation-hop rules;
- [ ] lost/changed moves, abilities, items, forms;
- [ ] trainer/origin rule handling;
- [ ] clear required-change preview;
- [ ] refuse unknown/unsupported destructive conversion.

## Phase 28 — Generic staged-save write architecture — PLANNED

- [ ] destination/source fingerprint;
- [ ] verified backup;
- [ ] isolated staged copy;
- [ ] intended mutation only;
- [ ] checksum/container repair;
- [ ] reparse/validate staged output;
- [ ] intended-vs-unintended diff;
- [ ] durable transaction state;
- [ ] rollback/recovery UI/state.

No generic global live-write switch.

## Phase 29 — First approved live-write adapter — PLANNED

- [ ] choose one narrow, well-understood adapter;
- [ ] fixture/round-trip validation;
- [ ] staged mutation validation;
- [ ] verified backup;
- [ ] write;
- [ ] readback;
- [ ] exact intended-change verification;
- [ ] rollback;
- [ ] physical Switch test;
- [ ] capability recorded only for that adapter.

## Phase 30 — Per-game live-write expansion — PLANNED

- [ ] approve additional adapters individually;
- [ ] track version/container constraints;
- [ ] no inherited/global authorization;
- [ ] unsupported versions remain read-only.

Legacy file sources such as Stadium/GameCube do not automatically need write support just because they can be read/imported.

## Phase 31 — True Move semantics — PLANNED LATE

Issue #20.

- [ ] Game → Vault/Bank destination durable before source removal;
- [ ] Vault/Bank → Game destination verified before active Vault placement retired;
- [ ] hidden archival history does not appear as another active playable copy;
- [ ] Copy remains explicit;
- [ ] Clone remains explicit;
- [ ] multi/whole-box Move uses one coherent transaction model.

## Phase 32 — Official HOME bridge workflow — PLANNED

Use supported modern game saves as the bridge instead of impersonating private HOME/Nintendo protocols.

- [ ] PokeBank NX → supported game → official HOME workflow where applicable;
- [ ] genuine HOME tracker/history preserved;
- [ ] no credential extraction/private protocol impersonation;
- [ ] no tracker forging;
- [ ] no guaranteed-ban-safety claims.

PokeBank NX may read offline source formats HOME itself cannot directly import. That does **not** mean HOME directly supports Stadium, GameCube memory-card, DS emulator, or other legacy save files.

## Phase 33 — Native `.nro` reliability / diagnostics / accessibility — PLANNED THROUGHOUT

Issue #21.

- [ ] Diagnostics + privacy-safe export;
- [ ] Applet/constrained-memory warning;
- [ ] privacy-safe crash/error logs;
- [ ] READ ONLY / VAULT / STAGED / active-location badges;
- [ ] search/filter;
- [ ] Quick Jump;
- [ ] recent/continue/favorites;
- [ ] Vault verify/index rebuild/recovery;
- [ ] virtualized large grids;
- [ ] bounded artwork caches;
- [ ] lazy entity loading;
- [ ] text sizing;
- [ ] Reduced Motion;
- [ ] non-color-only focus;
- [ ] optional original sounds/rumble.

## Phase 34 — Final branding/startup/icon/metadata — PLANNED

Issue #16.

- [ ] final PokeBank NX splash/startup;
- [ ] real loading stages tied to actual work;
- [ ] final title/icon/NACP;
- [ ] visible version + source SHA;
- [ ] graceful optional-resource failure;
- [ ] no normal-path PKSE identity.

## Phase 35 — Performance / scale / release-candidate torture — PLANNED

- [ ] huge synthetic Vault soak;
- [ ] bounded memory/cache behavior;
- [ ] SD low-space/error handling;
- [ ] handheld + docked pass;
- [ ] sleep/resume/controller reconnect;
- [ ] malformed save corpus;
- [ ] transaction interruption/recovery testing;
- [ ] release-candidate physical torture test.

## Phase 36 — v1.0 release — PLANNED

Core v1 gates:

- [ ] advertised core read adapters verified;
- [ ] all advertised live-write adapters individually safety-approved;
- [ ] no unsupported parser path crashes the app;
- [ ] Vault transaction recovery tested;
- [ ] large collection performance acceptable;
- [ ] handheld/docked UI accepted;
- [ ] exact source SHA frozen;
- [ ] host tests + sanitizers + native build pass;
- [ ] exact release `.nro` filename/size/SHA-256 preserved;
- [ ] README/support matrix/release notes match reality;
- [ ] v1.0 tag/release.

Stadium #34 is allowed to remain a post-v1 legacy enhancement if it is the only remaining disproportionate side-source blocker. Do not hold the core app hostage for it.

---

# Current critical path

As of 2026-09-02:

```text
#23 mutation safety audit
#24 PLA crash hardening
+ preserve #19 analog fix
+ preserve #13/#16 UI
        ↓
replacement exact .nro
        ↓
DEVICE TEST #2
        ↓
#4 PKSM-Core PK3/Sav3
        ↓
Gen III production reads
        ↓
Master Vault + Banks
        ↓
Colosseum/XD + Gen I/II + DS + 3DS legacy reads
        ↓
Summary / Oracle / modern Switch reads
        ↓
conversion / Dex / legality / events
        ↓
staged writes
        ↓
per-game approved live writes
        ↓
true Move
        ↓
RC hardening
        ↓
v1.0
```

Stadium can slot after Gen I/II without delaying the critical path if necessary.

---

# Supporting references

Core project docs:

- `PROJECT_STATUS.md`
- `README.md`
- `docs/GAME_SUPPORT_MATRIX.md`
- `docs/PROJECT_MAP.md`
- `docs/NEXT_SESSION_PLAN.md`
- `docs/ARCHITECTURE.md`
- `docs/SAVE_SAFETY.md`
- `docs/MASTER_VAULT_SPEC.md`
- `docs/TRANSFER_MODEL.md`
- `docs/NRO_QUALITY_ROADMAP.md`
- `docs/UPSTREAM_AUDIT.md`
- `docs/BANK_PROJECT_REFERENCE_AUDIT_2026-09-02.md`

Primary roadmap issue: **#29**.

Catalog/legacy issues:

```text
#30 DS/3DS identities
#31 Nintendo DS Gen IV/V reads
#32 Nintendo 3DS Gen VI/VII reads
#33 Colosseum/XD GameCube reads
#34 Stadium 1/2 N64 reads
```