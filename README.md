<p align="center">
<img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is a local-first Pokémon bank, save manager, collection manager and preservation project for **CFW Nintendo Switch**.

The goal is one native Switch application that can safely discover Pokémon saves, browse and edit them through generation-aware tools, preserve Pokémon in a provenance-aware Master Vault, organize them into named Banks, and eventually support carefully approved transfer/write workflows without treating live saves as disposable data.

PokeBank NX is being built around two equal input paths:

- full controller operation;
- full touch-only handheld operation for users with dead, missing or disconnected Joy-Cons/controllers.

> **Current alpha safety rule:** live installed-game and emulator-source writes remain **HARD DISABLED** unless a specific source/game adapter passes its own staged backup, validation, checksum/container repair, write, readback and rollback safety gate. Current accepted GB/GBC/GBA milestones are read-only.

---

# Where the project is now

As of **2026-09-12**, the read-only legacy foundation for **Generation I, Generation II and Generation III** has been physically tested on a real Nintendo Switch.

Current development branch:

```text
feature/pokebank-playable
```

Current accepted GSC code checkpoint:

```text
50e32d4456f84ea8ac9680c820526770c2b2cf3e
gen2: show fixed male trainer gender in gold and silver
```

Current device-accepted legacy status:

```text
Generation I
  Pokémon Red       GB   READ ONLY   DEVICE ACCEPTED
  Pokémon Blue      GB   READ ONLY   DEVICE ACCEPTED
  Pokémon Yellow    GB   READ ONLY   DEVICE ACCEPTED

Generation II
  Pokémon Gold      GBC  READ ONLY   DEVICE ACCEPTED
  Pokémon Silver    GBC  READ ONLY   DEVICE ACCEPTED
  Pokémon Crystal   GBC  READ ONLY   DEVICE ACCEPTED

Generation III
  Pokémon FireRed   GBA  READ ONLY   DEVICE ACCEPTED
  Pokémon LeafGreen GBA  READ ONLY   DEVICE ACCEPTED
  Pokémon Ruby      GBA  READ ONLY   DEVICE ACCEPTED
  Pokémon Sapphire  GBA  READ ONLY   DEVICE ACCEPTED
  Pokémon Emerald   GBA  READ ONLY   DEVICE ACCEPTED
```

The latest physical GSC retest confirmed Gold, Silver and Crystal working as intended. Gold/Silver now explicitly show the fixed trainer gender as `Male`; Crystal keeps save-derived gender behavior; Gen II does not fabricate a modern SID.

The project is now moving from **legacy read-only foundation work** into the larger v1.0 product phase:

```text
safe save editor
Master Vault + named Banks
universal emulator/save discovery
full touch-only UI
DS + 3DS support
modern Switch validation
Summary / Dex / provenance / legality
staged writes / individually approved live writes
release hardening
```

---

# Project status at a glance

| Area | Status |
|---|---|
| Native Switch `.nro` | ✅ Working on real hardware |
| Controller-first UI | ✅ Working |
| Gen I Red/Blue/Yellow | ✅ **Read-only device accepted** |
| Gen II Gold/Silver/Crystal | ✅ **Read-only device accepted** |
| Gen III FRLG/RSE | ✅ **Read-only device accepted** |
| Trainer / Party / Boxes on accepted legacy games | ✅ Working |
| Legacy inventory browsing | ✅ Working |
| Generation-aware Trainer fields | ✅ Working |
| HD game/Pokémon resource packaging | ✅ Working / gated |
| Host regression suite | ✅ Working |
| ASan / UBSan gates | ✅ Working |
| Deterministic native build verification | ✅ Working |
| Universal save editor | 🚧 **Next active phase** |
| Master Vault | ⬜ Planned for v1 |
| Named Banks | ⬜ Planned for v1 |
| Universal emulator discovery | ⬜ **v1 requirement — #56** |
| Full touch-only handheld UI | ⬜ **v1 requirement — #55** |
| Nintendo DS Gen IV/V | ⬜ Planned for v1 |
| Nintendo 3DS Gen VI/VII | ⬜ Planned for v1 |
| Modern Switch per-game validation | 🟨 Foundation exists; production validation pending |
| Professional Summary / provenance | ⬜ Planned |
| Pokédex / Living Dex / shiny views | ⬜ Planned |
| Conversion / legality engine | ⬜ Planned |
| Safe staged transaction engine | ⬜ Planned |
| True Move | ⬜ Planned after approved write adapters |
| Live save writing | 🔒 **HARD DISABLED by default** |

---

# Accepted legacy read-only foundation

## Generation I — Red / Blue / Yellow

Physically accepted capabilities include:

- bounded RetroArch battery-save discovery;
- stable `red_gb`, `blue_gb`, `yellow_gb` identities;
- strict save recognition/validation;
- international/Japanese layout handling where supported;
- Trainer;
- Party;
- PC Boxes;
- PK1 details;
- Bag;
- PC Items;
- generation-appropriate item naming;
- generation-appropriate Trainer fields;
- correct GB platform identity;
- source immutability;
- safe malformed-input rejection.

Generation I inventory is represented according to the actual games rather than fake modern pouch categories:

```text
Bag
PC Items
```

Accepted RBY source checkpoint:

```text
50dac31f53907143f48884681056f8d582813b76
```

Accepted RBY artifact:

```text
PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro
159754197 bytes
SHA-256:
b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b
```

---

## Generation II — Gold / Silver / Crystal

Physically accepted capabilities include:

- Gold / Silver / Crystal source identity;
- strict GSC read-only parsing;
- Trainer;
- Party;
- PC Boxes;
- Gen II items/inventory;
- PK2 details;
- permanent GSC card artwork/assets;
- source immutability;
- Gen I + Gen III regression preservation;
- physical Switch acceptance.

Trainer identity behavior is generation-accurate:

```text
Gold    Gender: Male (fixed by game)
Silver  Gender: Male (fixed by game)
Crystal Gender: save-derived Male/Female
SID     N/A — Generation II has no modern SID field
```

Current accepted GSC code checkpoint:

```text
50e32d4456f84ea8ac9680c820526770c2b2cf3e
```

---

## Generation III — FireRed / LeafGreen / Ruby / Sapphire / Emerald

Physically accepted capabilities include:

- strict rotating-sector save parsing;
- signatures, counters and checksums;
- newest-valid-slot selection / safe fallback;
- Trainer;
- Party;
- all PC Boxes;
- PK3 extraction/details;
- FRLG/RSE inventory;
- Emerald keyed quantity handling;
- Refresh/current-save behavior;
- source immutability;
- bounded RetroArch battery-save discovery.

Accepted RSE source checkpoint:

```text
a2df4c1acdb7a556808bd58a2bdbcd4fc0335954
```

Accepted RSE artifact SHA-256:

```text
34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e
```

FireRed/LeafGreen GBA and FireRed/LeafGreen Switch remain separate release/platform identities.

---

# NEXT — universal save editor

The next major phase is to make PokeBank NX a safe, generation-aware save editor rather than only a browser.

Initial classic targets:

```text
Red / Blue / Yellow
Gold / Silver / Crystal
Ruby / Sapphire / Emerald
FireRed GBA / LeafGreen GBA
```

The editor should reuse proven PKSE infrastructure where appropriate instead of building an unrelated second editor system.

First editor slice:

- Trainer name;
- money;
- inventory quantities;
- Poké Balls / Master Balls;
- Potions / Super Potions / Hyper Potions / Max Potions where present;
- TMs/HMs where appropriate;
- Berries where appropriate;
- advanced Key Items with warnings;
- later player/map position;
- later full Pokémon editing for party/boxes.

Editing must be staged:

```text
source save
   ↓ read only
immutable original bytes
   ↓
in-memory staged changes
   ↓
Review Changes
   ↓
validate / repair required checksums
   ↓
export/test copy
   ↓
future individually approved write adapter
```

An editor existing in the UI does **not** automatically authorize writing back to the source save.

---

# Master Vault and named Banks

The long-term product is centered around a real PokeBank-owned storage layer rather than inherited writable PKSE Storage.

Master Vault v1 is planned to provide:

- immutable original Pokémon payloads;
- SHA-256 hashes;
- stable Vault IDs;
- source game/platform/save provenance;
- parent/clone/derived relationships;
- active-location vs archival-history separation;
- journal/recovery behavior;
- profile-aware ownership.

Named Banks will be logical organization over Vault IDs, allowing collections such as:

```text
Living Dex
Shiny Dex
Events
Trade / Extras
Shiny Eevee
personal named banks
```

One Vault Pokémon may appear in multiple organizational Banks without duplicating the authoritative raw payload.

Tracked by issue **#3**.

---

# Universal emulator save discovery — v1.0 requirement

RetroArch-only discovery is not enough for the final app.

PokeBank NX v1 is planned to use an extensible **Save Source Adapter** architecture for reasonable GB/GBC/GBA/DS/3DS emulator setups.

Initial target sources include:

```text
RetroArch
Tico
mGBA standalone
melonDS standalone
melonDS / libretro
DraStic
Azahar
Custom folders
Unknown-but-valid Pokémon save files
```

The intended pipeline is:

```text
SD card / emulator
        ↓
SaveSourceAdapter
        ↓
source format / wrapper normalization
        ↓
Pokémon save parser
        ↓
PokeBank NX
```

Key rules:

- read the emulator's actual configuration where possible;
- do not rely only on hardcoded folders;
- keep emulator/source provenance;
- allow multiple saves for the same Pokémon game;
- deduplicate the same physical file discovered by multiple routes;
- support a fast targeted scan for normal startup;
- support an optional cancellable deep SD scan;
- allow `Settings → Save Sources → Add Folder` for custom/future emulators;
- validate candidates by actual save structure, not filename alone;
- normalize wrappers such as `.sav`, `.srm`, `.dsv` and 3DS title-save layouts at the source layer;
- do not enable write-back merely because a save was discovered.

Tracked by issue **#56**.

---

# Full touch-only handheld support — v1.0 requirement

PokeBank NX must be usable in handheld mode when Joy-Cons/controllers are dead, missing or disconnected.

The final target is true dual-input operation:

```text
controller only ✅
touchscreen only ✅
mix controller + touch ✅
```

Touch-only operation must cover:

- Home / game/source selection;
- Trainer;
- Inventory;
- Party;
- Boxes;
- Master Vault / Banks;
- Pokémon details/action menus;
- save editor;
- software keyboard;
- numeric input;
- dialogs/warnings;
- scrolling;
- Back/Cancel navigation.

No essential handheld operation should require A/B/X/Y/L/R/ZL/ZR/D-pad/sticks/+/- when using the touchscreen.

Controller and docked behavior must not regress.

Tracked by issue **#55**.

---

# Nintendo DS and Nintendo 3DS

PokeBank NX v1 plans to add file/emulator-source support for the main-series DS and 3DS games.

## Nintendo DS

```text
Diamond
Pearl
Platinum
HeartGold
SoulSilver
Black
White
Black 2
White 2
```

Planned source integrations include melonDS, DraStic, libretro sources and custom/imported files through the common Save Source architecture.

Tracked by issues **#30**, **#31**, **#56**.

## Nintendo 3DS

```text
X
Y
Omega Ruby
Alpha Sapphire
Sun
Moon
Ultra Sun
Ultra Moon
```

Azahar support should understand its configurable virtual SD/NAND/title-save storage rather than searching only for flat `.sav` files.

Tracked by issues **#30**, **#32**, **#56**.

---

# Modern Switch game direction

The source/identity foundation includes current Switch families such as:

```text
Let's Go Pikachu / Eevee
Sword / Shield
Brilliant Diamond / Shining Pearl
Legends: Arceus
Scarlet / Violet
Legends: Z-A
FireRed Switch / LeafGreen Switch
```

Existing PKSE behavior is useful, but v1 still needs explicit production validation of supported save revisions, block/container behavior and read/write capability per game family.

One game being writable must never globally authorize every game.

Tracked by issue **#11**.

---

# Summary, collection and provenance

Planned v1 product work includes:

- professional generation-aware Pokémon Summary;
- origin/current-location/provenance separation;
- Pokémon artwork/render support;
- search/filter/favorites/recent items;
- National and game Pokédex views;
- Living Dex tracking;
- shiny collection tracking;
- legality state;
- optional Pokémon cry playback where safe/appropriate.

Older generations must not be padded with fake modern fields.

Tracked primarily by **#7, #9, #21, #25, #35**.

---

# Conversion, legality and transfers

Planned systems include:

- host-side PKHeX Oracle;
- reproducible golden save/Pokémon fixture corpus;
- compatibility engine;
- generation-aware conversion;
- legality/provenance validation;
- legality-aware editing;
- Make Shiny safeguards;
- staged destination representations;
- future true Move.

`COPY`, `MOVE` and `CLONE` remain distinct operations.

A true Move can only remove/retire a source after a destination has been durably created and validated.

Historical Gen II → III conversion must be recorded honestly as an unofficial preservation/conversion bridge rather than pretending an official uninterrupted transfer path existed.

Tracked by **#5, #10, #17, #20**.

---

# Save safety architecture

PokeBank NX treats save safety as a product requirement, not an implementation detail.

Current contract:

```text
installed Switch source          READ ONLY unless explicitly approved later
legacy/emulator source           READ ONLY unless explicitly approved later
original source bytes            preserved
staged workspace                 separate
live installed writing           HARD DISABLED by default
live emulator-source writing     HARD DISABLED by default
```

A future write adapter must use a pipeline equivalent to:

```text
fingerprint source
→ backup
→ stage mutation
→ repair format/checksum/container
→ reparse and validate
→ write
→ read back
→ validate exact result
→ rollback/recovery if necessary
```

If a source changed since it was loaded, PokeBank NX should refuse to overwrite newer progress and require reload/reconciliation.

---

# v1.0 roadmap

Current high-level order:

```text
Gen I / II / III legacy read-only
        COMPLETE / DEVICE ACCEPTED
        ↓
universal save-editor foundation
        ↓
Master Vault + named Banks
        ↓
universal emulator SaveSource adapters
        ↓
DS + 3DS identities/read adapters
        ↓
modern Switch source validation
        ↓
Summary / provenance / Dex / search
        ↓
conversion / legality / staged destination editing
        ↓
staged transaction framework
        ↓
individually approved writes / true Move
        ↓
complete touch-only coverage
        ↓
release hardening / documentation / RC hardware pass
        ↓
v1.0
```

Touch support should be added while new editor/Vault screens are built rather than retrofitted after every screen is finished.

Universal save-source discovery should likewise be established before DS/3DS support fragments into one-off emulator path hacks.

The canonical GitHub roadmap is issue **#29**.

---

# v1.0 release gates

Before v1.0:

- all advertised read adapters must have regression coverage;
- all advertised write adapters must pass their own safety/device gate;
- unsupported/malformed saves must fail safely;
- Vault interruption/recovery must be tested;
- large collection performance must be tested;
- controller operation must pass;
- touch-only handheld operation must pass;
- supported emulator/source discovery must pass on physical SD-card layouts;
- handheld and docked operation must pass;
- a release-candidate hardware torture pass must succeed;
- exact release source SHA, `.nro` byte size and SHA-256 must be preserved;
- README/support matrix/release notes must match real capability;
- then tag/release v1.0.

---

# Additional / post-v1 work

Important future work already tracked separately includes:

- Android companion + USB-C PokeBank NX Link (**#39**);
- historical Prepare for HOME workflow (**#40**);
- Companion Mode / maps / guides / emulator telemetry (**#41**);
- full Gift/Event/Mystery Gift Library + EventDex (**#46**);
- real GB/GBC/GBA hardware link bridge (**#47**).

These are intentionally outside the core v1 critical path unless explicitly promoted later.

---

# Repository / developer handoff

Repository:

```text
GlitchedZeus/PokeBank-NX
```

Development branch:

```text
feature/pokebank-playable
```

Writable remote:

```text
origin
```

PKSE remains an upstream/reference source. Custom PokeBank NX work must never be pushed to the PKSE upstream repository.

Useful project state files:

```text
README.md
CURRENT_STATUS.md
docs/V1_ROADMAP.md
docs/GAME_SUPPORT_MATRIX.md
docs/NEXT_SESSION_PLAN.md
docs/NEXT_CODEX_PROMPT.md
```

---

# Current project note

PokeBank NX is still an **alpha project**, not a public v1.0 release.

The important difference now is that the foundational GB/GBC/GBA read-only path is no longer theoretical: **Red, Blue, Yellow, Gold, Silver, Crystal, FireRed, LeafGreen, Ruby, Sapphire and Emerald have all passed the current physical read-only device milestone.**

The remaining work is the larger product layer: editing, Vault/Banks, broader save discovery, DS/3DS, modern Switch validation, transfers/legality, full touch support and release hardening.

---

# Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks and game assets are property of their respective owners.


## Standalone runtime ownership

PokeBank NX is a standalone all-in-one Nintendo Switch Pokémon management application. External Pokémon homebrew and PC tools may be development references or optional migration tools, but are not prerequisites for normal advertised operation.

Canonical runtime root: `sdmc:/switch/PokeBank-NX/`. See `docs/STANDALONE_RUNTIME.md` and `docs/UI_OWNERSHIP_STATUS.md`. Issue #58 remains open for the full visible v1 UI conversion.
