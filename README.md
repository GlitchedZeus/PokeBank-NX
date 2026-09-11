<p align="center">
<img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is a controller-first, local-first Pokémon storage, collection, provenance, transfer, and save-management project for **CFW Nintendo Switch**.

The long-term goal is one Switch app that can safely work with legacy emulator saves and modern Switch saves, preserve Pokémon with provenance, organize them into personal Vaults and Banks, support collection tracking and generation-aware summaries, and eventually provide carefully approved transfer/edit/write workflows without treating live saves as disposable data.

> **Current alpha safety rule:** live installed-game and RetroArch save writing is **HARD DISABLED**. Current legacy milestones are read-only. A game may only gain live writing later after its own backup, staging, checksum/container repair, reparse validation, readback, rollback, corruption testing, host regression, sanitizer, native-build, and physical-device gates pass.

---

## Where the project is now

PokeBank NX has moved beyond the early UI-only prototype stage. The app now has a working native Switch `.nro`, a controller-first shell, reproducible device builds, deterministic asset verification, physically tested legacy save adapters, and a repeatable host/sanitizer/native/device validation workflow.

The current accepted legacy baseline is:

```text
Generation I
  Pokémon Red       GB   READ ONLY   DEVICE ACCEPTED
  Pokémon Blue      GB   READ ONLY   DEVICE ACCEPTED
  Pokémon Yellow    GB   READ ONLY   DEVICE ACCEPTED

Generation III
  Pokémon FireRed   GBA  READ ONLY   DEVICE ACCEPTED
  Pokémon LeafGreen GBA  READ ONLY   DEVICE ACCEPTED
  Pokémon Ruby      GBA  READ ONLY   DEVICE ACCEPTED
  Pokémon Sapphire  GBA  READ ONLY   DEVICE ACCEPTED
  Pokémon Emerald   GBA  READ ONLY   DEVICE ACCEPTED
```

The next active milestone is:

```text
Generation II
  Pokémon Gold
  Pokémon Silver
  Pokémon Crystal
  Game Boy Color / RetroArch
  STRICT READ ONLY
```

Development branch: `feature/pokebank-playable`

Main milestone merge currently includes the physically accepted Generation I RBY work through PR **#52**.

---

## Project status at a glance

| Area | Status |
|---|---|
| Native Switch `.nro` | ✅ Working on real hardware |
| Controller-first UI | ✅ Working |
| HD Pokémon artwork | ✅ 3,260/3,260 verified |
| Deterministic device asset preflight | ✅ Working |
| Host regression suite | ✅ Working |
| ASan / UBSan gates | ✅ Working |
| Exact `.nro` hashing / artifact manifests | ✅ Working |
| Red GB | ✅ **Read-only physically accepted** |
| Blue GB | ✅ **Read-only physically accepted** |
| Yellow GB | ✅ **Read-only physically accepted** |
| Gen I RBY RetroArch read-only milestone | ✅ **Physically accepted** |
| FireRed GBA | ✅ **Read-only physically accepted** |
| LeafGreen GBA | ✅ **Read-only physically accepted** |
| Ruby GBA | ✅ **Read-only physically accepted** |
| Sapphire GBA | ✅ **Read-only physically accepted** |
| Emerald GBA | ✅ **Read-only physically accepted** |
| Gen III RetroArch read-only milestone | ✅ **Physically accepted** |
| Gold / Silver / Crystal | ▶️ **Current next read-only milestone** |
| Modern Switch source discovery | 🟨 Present; production read validation still pending per game |
| DS Gen IV/V | ⬜ Planned |
| 3DS Gen VI/VII | ⬜ Planned |
| Profile-scoped Vaults | ⬜ Planned |
| Named Banks | ⬜ Planned |
| Professional Summary / provenance | ⬜ Planned |
| Pokédex / Living Dex / shiny collection views | ⬜ Planned |
| Conversion / compatibility engine | ⬜ Planned |
| Legality-aware editor / creation | ⬜ Planned |
| Mystery Gifts / events | ⬜ Planned |
| Staged-save transaction engine | ⬜ Planned |
| True Move semantics | ⬜ Planned |
| Live save writing | 🔒 **HARD DISABLED** |

---

# What has been completed

## Native Switch application foundation

The project has a real controller-driven Nintendo Switch application path rather than only host-side parser experiments.

Completed foundation work includes:

- native `.nro` builds with devkitA64;
- controller-first navigation and action-sheet behavior;
- left-stick navigation physically accepted on Switch;
- PokeBank NX red UI shell accepted for development;
- HD Pokémon artwork packaged into RomFS;
- reproducible device-build asset preflight;
- embedded application identity verification;
- embedded RomFS verification;
- exact NRO/ZIP/manifests with SHA-256 recording;
- GitHub Actions artifact retrieval and independent re-hashing;
- malformed/unsupported save paths hardened to fail safely;
- inherited mutation paths locked while live writing is disabled.

The project deliberately keeps source-save reading, Vault/storage work, and future writes separated so a browsing milestone cannot silently become a destructive save-editor milestone.

---

# Generation I — Red / Blue / Yellow

## Physically accepted on real Switch hardware

The read-only Game Boy RBY source path is physically accepted for normal RetroArch battery saves.

Accepted capability includes:

- bounded RetroArch `.sav` / `.srm` discovery;
- stable `red_gb`, `blue_gb`, `yellow_gb` identities;
- strict supported save recognition;
- international and Japanese layout handling;
- strict checksum/structure validation;
- Trainer view;
- Party;
- PC Boxes / storage;
- PK1 / Gen I Pokémon details;
- Bag;
- PC Items;
- dedicated Generation I item naming;
- TM/HM handling through the real Gen I inventory model;
- generation-appropriate Trainer fields;
- correct GB platform identity;
- Refresh / current-save behavior;
- malformed optional inventory isolated from otherwise-readable Trainer/Party/Boxes where safe;
- source-byte immutability.

Generation I does **not** use modern item pouch categories. The accepted UI exposes the two real inventory containers used by RBY:

```text
Bag
PC Items
```

Poké Balls, medicines, Key Items, TMs and HMs remain Gen I inventory entries rather than fake modern pouches. Generation I has no Berries.

The Trainer view intentionally avoids fabricated modern values. RBY currently exposes generation-appropriate identity such as:

```text
Name
Money
Trainer ID
```

No fake SID is shown.

## Accepted RBY application identity

```text
Application source:
50dac31f53907143f48884681056f8d582813b76

Application tree:
1cf73ea12833e8a94a06dfaf2b9036e9059344ec

Commit:
gen1: add read-only RBY inventory support
```

## Exact physically accepted RBY artifact

```text
PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro
159754197 bytes
SHA-256:
b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b
```

Physical result:

```text
Red: DEVICE TESTED YES / DEVICE ACCEPTED YES
Blue: DEVICE TESTED YES / DEVICE ACCEPTED YES
Yellow: DEVICE TESTED YES / DEVICE ACCEPTED YES

Trainer: PASS
Party: PASS
Boxes: PASS
Pokémon details: PASS
General RBY browsing: PASS
Items: PASS
Bag: PASS
PC Items: PASS
Yellow GB platform label: PASS
```

## RBY bug-fix history

The first RBY physical-test runtime was:

```text
d9077e2da3909b6fbe9d8db9ce7384a71b8b98e7
```

That build proved the core RBY read path on hardware but exposed two device-visible defects:

```text
Items -> "Invalid category"
Yellow -> incorrectly labeled as GBA
```

The ItemsFix runtime `50dac31f...` added the strict Gen I inventory model, proper Gen I item naming and correct GB platform identity. The corrected artifact then passed the physical retest and became the accepted Generation I baseline.

This acceptance is **read-only browsing acceptance only**. It does not imply live writing, editing, transfers, Vault support, legality support or conversion support for RBY.

---

# Generation III — FireRed / LeafGreen / Ruby / Sapphire / Emerald

## Physically accepted on real Switch hardware

All five mainline Generation III GBA releases are accepted for the current read-only RetroArch path:

```text
FireRed GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
LeafGreen GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Ruby GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Sapphire GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Emerald GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
```

The accepted Gen III work includes, as applicable:

- bounded normal battery-save discovery;
- stable per-release/platform source identities;
- strict Gen III rotating-sector validation;
- signatures, counters and checksums;
- newest-valid-slot selection and safe fallback;
- Trainer;
- Party;
- all PC Boxes;
- PK3 extraction and Pokémon browsing;
- RSE + FRLG inventory support;
- Emerald keyed bag quantities;
- Refresh after normal in-game save changes;
- profile-scoped legacy-source assignment/persistence in the accepted FRLG flow;
- source-byte immutability;
- game-card artwork for all accepted Gen III games;
- optional inventory-model failure isolated from critical save validation.

Accepted final RSE application source:

```text
a2df4c1acdb7a556808bd58a2bdbcd4fc0335954
```

Accepted RSE artifact:

```text
PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro
SHA-256:
34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e
```

FireRed/LeafGreen GBA and FireRed/LeafGreen Switch are deliberately separate identities. A release/platform identity is never collapsed merely because the game name is similar.

---

# Current milestone — Generation II GSC

The next implementation target is strict read-only support for:

```text
gold_gbc
silver_gbc
crystal_gbc
```

Target source:

```text
Game Boy Color
RetroArch
normal .sav / .srm battery saves
READ ONLY
```

The milestone is expected to cover:

- PKSM-Core `Sav2` / `PK2` audit first;
- independent PKHeX oracle checks;
- Gold/Silver vs Crystal structural differences;
- international/Japanese layouts where supported;
- exact save-size/checksum rules;
- stable source identities;
- bounded RetroArch discovery;
- Trainer;
- Party;
- PC Boxes;
- Generation II inventory semantics;
- truthful PK2 Pokémon details;
- Refresh;
- source immutability;
- malformed/truncated rejection;
- Gen I + Gen III regressions;
- host tests;
- ASan / UBSan;
- native final link;
- exact physical-test `.nro` before acceptance.

Gold/Silver/Crystal are **not yet physically accepted** and must not be described as complete until an exact GSC artifact passes a real Switch test.

---

# Modern Switch sources

PokeBank NX already has a modern Switch game/source identity foundation for supported titles, including separate identities for same-name releases on different platforms.

Current target identities include modern Switch paths such as:

```text
FireRed Switch
LeafGreen Switch
Let's Go Pikachu
Let's Go Eevee
Sword
Shield
Brilliant Diamond
Shining Pearl
Legends: Arceus
Scarlet
Violet
Legends: Z-A
```

Some source paths have already been exercised during development, but production read validation remains a later milestone per game. The README does not treat source discovery alone as parser/device acceptance.

---

# Safety architecture

PokeBank NX is intentionally conservative around save data.

Current product contract:

```text
installed Switch source        READ ONLY
RetroArch / legacy source      READ ONLY
backup/staged workspace        separate from live source
Master Vault                  NOT IMPLEMENTED YET
live installed save writing   HARD DISABLED
live RetroArch save writing   HARD DISABLED
true Move                     NOT IMPLEMENTED
```

Permanent design rules include:

- original/live saves remain untouched during current read-only milestones;
- malformed or unsupported sources fail safely rather than being silently repaired;
- unknown save revisions are never automatically writable;
- source-container identity is separate from profile display identity;
- original Pokémon bytes and provenance must remain preservable;
- Copy, Move and Clone are distinct operations;
- future writes must use backup → stage → validate → write → readback → rollback-capable transactions;
- one proven write adapter will never automatically authorize every game.

---

# Long-term product direction

The target is closer to an offline, user-owned combination of a Pokémon bank, collection manager and generation-aware save tool than a simple save browser.

Planned major systems include:

### Master Vault

- immutable Pokémon records;
- raw/original bytes;
- content hashes;
- stable Vault IDs;
- source game/save metadata;
- provenance and parent links;
- recovery/journal support;
- profile-scoped ownership.

### Named Banks

- multiple logical collections over Vault entities;
- living dex;
- shiny dex;
- event/gift sets;
- species-specific collections;
- trade/extras banks;
- unlimited user-named banks.

### Professional Pokémon Summary

Generation-aware fields where they actually exist, including:

- species / Dex number;
- nickname / level / gender / shiny / form;
- types;
- nature / ability when the generation supports them;
- held item;
- moves;
- IV / EV / stat information where meaningful;
- OT / TID / SID only when truly represented;
- original game/platform;
- current location;
- met data / Ball / language;
- ribbons / marks where supported;
- legality state;
- provenance/history.

Older games must not be padded with fake modern values.

### Collection / Dex tools

Planned:

- National / game Pokédex views;
- Living Dex tracking;
- shiny collection tracking;
- search;
- filters;
- favorites;
- recent Pokémon;
- quick jumps;
- large-grid virtualization and bounded artwork caches.

### Conversion / legality / creation

Planned:

- compatibility engine;
- generation-boundary conversion;
- provenance-aware transfer history;
- PKHeX comparison/oracle tooling;
- legality-aware editing;
- Make Shiny safeguards;
- guided creation / Quick Legal flows;
- event / Mystery Gift support;
- staged destination representations before any live write.

### True Move

Eventually:

```text
Game A
  ↓
Vault / Bank
  ↓
Game B
```

`COPY`, `MOVE` and `CLONE` will remain different operations. A true Move will only retire/remove a source after the destination has been durably created, validated and — for game writes — read back successfully.

---

# Profile architecture direction

PokeBank NX is designed around the Nintendo Switch profile selected at launch.

The intended long-term model is:

```text
Horizon profile selector
        ↓
selected AccountUid
        ↓
PokeBank profile context
        ↓
that profile's sources / Vault / Banks / history / settings
```

Normal browsing should be profile-scoped rather than exposing every local user's data globally.

A future explicit **Admin / Profiles & Sharing** mode is planned for deliberate family/local-profile management without weakening the save-safety rules.

This profile architecture is planned; it is not yet presented here as a completed feature.

---

# RetroArch source philosophy

For legacy emulator games, PokeBank NX treats a normal battery save as the canonical virtual cartridge.

General rules:

```text
canonical source: .sav / .srm battery save
savestates: not canonical sources
source identity: provider + physical save
read-only milestone: never modifies the source
```

Discovery remains bounded rather than performing an unrestricted SD-card crawl.

Per-Switch-profile RetroArch save isolation is a separate future milestone and is not silently assumed by the current legacy adapters.

---

# Verification philosophy

A feature is not called physically accepted merely because it compiles.

Typical milestone progression:

```text
oracle / research
→ strict parser
→ focused host tests
→ full regressions
→ source immutability / write-policy tests
→ ASan
→ UBSan
→ git diff --check
→ deterministic asset preflight
→ devkitA64 compile
→ native FINAL LINK
→ embedded app identity verification
→ embedded RomFS verification
→ package exact NRO
→ independent SHA-256 verification
→ real Nintendo Switch test
→ physical acceptance
```

Exact source SHA, tree SHA, artifact filename, size and SHA-256 are preserved for accepted device milestones.

---

# Roadmap

High-level order from here:

```text
Generation II Gold / Silver / Crystal read-only
        ↓
profile-scoped Master Vault + Banks
        ↓
DS Gen IV/V read-only
        ↓
3DS Gen VI/VII read-only
        ↓
modern Switch read validation
        ↓
professional Summary / provenance / search / Dex
        ↓
conversion / legality / editor / events
        ↓
staged writes
        ↓
individually approved live-write adapters
        ↓
true Move
        ↓
release hardening / v1.0
```

GameCube Colosseum/XD and Stadium archival work are valuable expansion targets but are not required to block the first serious v1.0.

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

PKSE remains an upstream/reference source only. Custom PokeBank NX work must never be pushed upstream.

Start a development/recovery session with:

```text
CURRENT_STATUS.md
↓
docs/CODEX_SESSION.md
↓
docs/NEXT_CODEX_PROMPT.md
```

The normal deterministic recovery source of truth is `origin/feature/pokebank-playable`.

---

# Important scope note

PokeBank NX is still an **alpha project**, not a finished release.

Physically accepted legacy read support does **not** mean the project already has:

```text
Master Vault
Banks
live save writing
true Move
full legality engine
full editor
all-generation conversion
DS/3DS adapters
fully validated modern Switch adapters
release-grade recovery/diagnostics
```

Those remain active roadmap work.

---

# Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks and game assets are property of their respective owners.
