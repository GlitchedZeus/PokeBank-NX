<p align="center">
<img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is an offline Pokémon storage, collection, provenance, transfer, and save-management project for **CFW Nintendo Switch**.

The long-term goal is a controller-first, offline Pokémon HOME-style application that can safely browse supported game saves, collect Pokémon into a permanent local Vault, organize them into Banks, preserve origin/provenance, and eventually move compatible Pokémon between supported games without relying on Nintendo's private online services.

> **Alpha / development warning:** live installed-game save writing is **not** an approved current feature. Installed-game sources remain read-only until each individual write adapter passes explicit backup, staging, validation, readback, rollback, and physical-device safety gates.

---

## Current development status

Active development branch:

```text
feature/pokebank-playable
```

Current engineering focus:

```text
RetroArch FireRed / LeafGreen GBA read-only runtime integration
```

The UI has been accepted as **good enough for development** and is intentionally frozen until the app is much closer to completion. Current work is focused on Pokémon/save functionality rather than more visual redesign.

For the exact engineering state, see:

- [`CURRENT_STATUS.md`](CURRENT_STATUS.md)
- [`docs/NEXT_SESSION_PLAN.md`](docs/NEXT_SESSION_PLAN.md)
- [`docs/GAME_SUPPORT_MATRIX.md`](docs/GAME_SUPPORT_MATRIX.md)
- [`docs/V1_ROADMAP.md`](docs/V1_ROADMAP.md)

---

# ✅ What already works

These are the parts a normal homebrew user would actually notice or care about today.

### App / controls

- ✅ Native Nintendo Switch `.nro` boots and runs on real hardware.
- ✅ Controller-first navigation.
- ✅ D-pad navigation.
- ✅ Left Stick navigation, including the physical-input fix.
- ✅ Pokémon Action Sheet for deliberate actions instead of accidental instant mutation.
- ✅ Handheld testing completed on current development builds.
- ✅ HOME / sleep / wake / controller reconnect behavior has been exercised successfully.

### UI / presentation

- ✅ PokeBank NX branding and shell are visible in-app.
- ✅ Red PokeBank NX identity accepted for development.
- ✅ OLED Black, Dark, and Light themes.
- ✅ Theme persistence.
- ✅ HD Pokémon artwork packaged in the application.
- ✅ Pokémon artwork renders on real Switch hardware.
- ✅ Artificial inherited sprite "breathing" / bobbing effect removed.
- ✅ Permanent inherited left-side accent bar removed.
- ✅ Current UI is frozen so engineering effort can go into the actual Pokémon features.

### Save safety

- ✅ Installed Switch game sources are treated as read-only.
- ✅ Low-level live installed-save write path is hard-disabled.
- ✅ Inherited Release/Create/Move/Edit paths are blocked from directly mutating installed sources.
- ✅ Backup/staged representations remain separated from installed-source browsing.
- ✅ App-owned legacy Storage is identified separately from the future Master Vault.
- ✅ Malformed / old Legends: Arceus input is handled with a graceful error instead of crashing the app.
- ✅ Source immutability is covered by regression tests for the new Gen III read path.

### Pokémon / save engine

- ✅ Stable current registry for 23 release/platform identities.
- ✅ FireRed GBA and LeafGreen GBA remain distinct from the separate Switch FireRed/LeafGreen identities.
- ✅ PKSM-Core pinned and integrated as a host-side Gen III correctness oracle.
- ✅ Strict FireRed/LeafGreen GBA Gen III save validation.
- ✅ Two rotating Gen III save slots handled.
- ✅ All 14 Gen III sectors validated and reassembled.
- ✅ Sector IDs, signatures, counters, checksums, and counter wraparound handled.
- ✅ Safe fallback to an older valid slot when a newer slot is corrupt.
- ✅ Party Pokémon enumeration.
- ✅ All 14 PC Boxes enumerated.
- ✅ PK3 records that cross a PC-sector boundary are reconstructed correctly.
- ✅ Species, PID, TID, SID, EXP, held item, moves, PP, IVs, EVs, nickname, and OT are extracted.
- ✅ Malformed / truncated / invalid Gen III input is rejected safely.
- ✅ Untouched boxed and party PK3 round trips are byte-identical in tests.
- ✅ Existing PKSE Gen III crypto independently agrees with the new adapter test data.
- ✅ Exception-free Switch-native Gen III backend builds under the normal `-fno-exceptions` toolchain.
- ✅ Host regression tests and ASan/UBSan coverage are in place.
- ✅ GitHub CI runs the host suite with recursive pinned PKSM-Core submodules.

---

# 🚧 In development right now

### RetroArch FireRed / LeafGreen GBA source support

The next user-facing milestone is to let PokeBank NX find and open real FireRed/LeafGreen saves from RetroArch on the Switch.

Target flow:

```text
RetroArch savefile_directory
        ↓
bounded read-only .sav / .srm discovery
        ↓
full FRLG structural validation
        ↓
FireRed GBA / LeafGreen GBA identity when reliable
        ↓
exception-free native Gen III backend
        ↓
Party / Boxes
        ↓
existing PokeBank browsing flow
```

Current design rules:

- read-only;
- no broad uncontrolled SD-card crawl;
- only known RetroArch save roots / configured save directory;
- bounded depth and candidate count;
- never trust filename alone;
- ambiguous FRLG sources remain unclassified rather than guessed;
- no save repair or writeback.

Tracked by issue **#6**.

---

# 🗺️ Planned / to be added

These are planned features, not claims that they already work.

### Core storage product

- ⬜ **Master Vault** — permanent game-independent Pokémon storage.
- ⬜ Immutable raw Pokémon payloads with SHA-256 integrity.
- ⬜ Origin / source / provenance history.
- ⬜ Stable Vault IDs and parent/derived lineage.
- ⬜ Named **Banks** built over Vault entries.
- ⬜ Living Dex / Shiny Living Dex / Favorites / Events / Competitive collections.
- ⬜ Search, filters, favorites, recent items, and Quick Jump.
- ⬜ Crash-safe Vault journal, recovery, and index rebuild tools.

### More game support

- ⬜ Ruby / Sapphire / Emerald GBA.
- ⬜ Red / Blue / Yellow GB.
- ⬜ Gold / Silver / Crystal GBC.
- ⬜ Nintendo DS Gen IV / V games.
- ⬜ Nintendo 3DS Gen VI / VII games.
- ⬜ Pokémon Colosseum.
- ⬜ Pokémon XD: Gale of Darkness.
- ⬜ Pokémon Stadium / Stadium 2 as stretch archival targets.
- ⬜ Validation and hardening of modern supported Switch save adapters.

Planned source catalog target:

```text
23 current host-tested identities
+ 9 Nintendo DS
+ 8 Nintendo 3DS
+ 2 Nintendo GameCube
+ 2 Nintendo 64 Stadium stretch
= 44 total target identities
```

### Pokémon tools

- ⬜ Professional Pokémon Summary / provenance view.
- ⬜ Vault-driven Pokédex.
- ⬜ Living Dex and shiny collection views.
- ⬜ PKHeX host-side correctness / legality oracle.
- ⬜ Legality-aware editing.
- ⬜ Clone / Make Shiny safeguards with provenance.
- ⬜ Historical Mystery Gift / event workflows.
- ⬜ Pokémon cry playback in Summary.

### Transfer system

- ⬜ Explicit **COPY**, **MOVE**, and **CLONE** semantics.
- ⬜ Cross-generation conversion / compatibility engine.
- ⬜ Safe staged destination representations.
- ⬜ Backup → mutate stage → validate → write → readback → rollback transaction pipeline.
- ⬜ Individually approved game write adapters.
- ⬜ True Move only after destination success is verified.
- ⬜ Supported-game bridge workflow compatible with official Pokémon HOME without impersonating private Nintendo/HOME protocols or forging tracker/history data.

### Final polish / release quality

- ⬜ Final title/icon/NACP/startup polish.
- ⬜ Final controller semantics and hints.
- ⬜ Docked-mode polish and release-candidate hardware testing.
- ⬜ Accessibility options such as text sizing and Reduced Motion.
- ⬜ Privacy-safe diagnostics / crash reports.
- ⬜ Large-Vault performance / cache optimization.
- ⬜ Persistent reproducible device-test artifact automation.
- ⬜ Release candidate, exact binary/hash preservation, and v1.0 release.

---

# Current accepted physical UI milestone

Accepted application source:

```text
af2acf043a15dbf48b8195880a80cc5de562fced
ui: adopt red PokeBank identity accents
```

Accepted artifact:

```text
PokeBank-NX-Red-UI-af2acf04.nro
SHA-256: 898df286cf34b895f1f71f4abc35f0818e4afa66725b67c2d020fc20c01bfac4
```

Physical result for continued development:

| Area | Result |
|---|---|
| D-pad | ✅ PASS |
| Left Stick | ✅ PASS |
| HD Pokémon artwork | ✅ PASS |
| Red PokeBank NX identity | ✅ ACCEPTED FOR NOW |
| Left accent bar | ✅ REMOVED |
| Fake sprite breathing/bobbing | ✅ REMOVED |
| Old/problem PLA source | ✅ GRACEFUL ERROR / NO CRASH |
| Installed-source live write | 🔒 HARD DISABLED |

---

# Gen III engineering checkpoints

Host PKSM-Core adapter:

```text
936e75d98daa7e61fcf8ea199bcda958b1b78d7a
gen3: add PKSM-Core read-only FRLG adapter
```

Pinned Core:

```text
FlagBrew/PKSM-Core
aa22d7a4f87c0351baf7da5962ba5acd01039a7c
```

Native exception-free backend:

```text
43f3a9f90a3314725979d59afdd68f19ee159009
gen3: build exception-free native core slice
```

Deterministic FRLG regression fixture:

```text
Size:    131072 bytes
SHA-256: b416aa985e459cb939caf1e1c70ce8359edf0c99a536e24d3b2a2a32b0541120
```

More technical detail lives in [`CURRENT_STATUS.md`](CURRENT_STATUS.md) and [`docs/PKSM_CORE_INTEGRATION.md`](docs/PKSM_CORE_INTEGRATION.md).

---

# Safety principles

- Installed Switch save sources are read-only today.
- RetroArch / legacy save sources are read-only during parser/import milestones.
- No parser should auto-repair or overwrite malformed source data.
- Master Vault will be app-owned storage, separate from original game saves.
- Live writes will be approved **one game adapter at a time**, never by one global unsafe switch.
- `COPY`, `MOVE`, and `CLONE` remain intentionally different operations.
- PokeBank NX does not impersonate private Nintendo/Pokémon HOME protocols or forge HOME tracker/history data.

---

# Development / Codex continuation

PokeBank NX custom work belongs only in:

```text
GlitchedZeus/PokeBank-NX
```

`kiasta/PKSE` remains **upstream only**. Never push custom PokeBank NX code upstream.

Future coding sessions can use the short launcher:

```text
Continue PokeBank NX on feature/pokebank-playable. Read CURRENT_STATUS.md and execute docs/NEXT_CODEX_PROMPT.md. Use HIGH reasoning. Preserve local work, push coherent checkpoints early, and never push custom code upstream.
```

---

# Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks are property of their respective owners.
