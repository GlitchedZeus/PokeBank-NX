<p align="center">
  <img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

<h1 align="center">PokeBank NX</h1>

<p align="center">
  Native Pokémon save browsing, staged editing, storage and preservation for CFW Nintendo Switch.
</p>

<p align="center">
  <img alt="Status: Alpha" src="https://img.shields.io/badge/status-alpha-orange" />
  <img alt="Platform: Nintendo Switch" src="https://img.shields.io/badge/platform-Nintendo%20Switch-E60012" />
  <img alt="CFW: Atmosphère" src="https://img.shields.io/badge/CFW-Atmosph%C3%A8re-blue" />
  <img alt="License: AGPL-3.0" src="https://img.shields.io/badge/license-AGPL--3.0-lightgrey" />
</p>

PokeBank NX is a native Nintendo Switch homebrew application for working with Pokémon saves without needing a PC or cloud service for normal use.

The project combines a controller-first Pokémon editor, multi-provider save discovery, staged save editing, storage/recovery foundations and generation-aware data handling behind one consistent interface.

> **Alpha software:** keep independent backups of irreplaceable saves. Ordinary editing currently happens in PokeBank-owned staged/working data and does not directly overwrite emulator or installed-game source saves.

---

## What works today

### Generation I

- Pokémon Red
- Pokémon Blue
- Pokémon Yellow
- Trainer, Party and Boxes
- staged Pokémon Create / View / Edit
- classic Inventory editing
- exact Gen I DV / Stat Exp behavior

### Generation II

- Pokémon Gold
- Pokémon Silver
- Pokémon Crystal
- Trainer, Party and Boxes
- staged Pokémon Create / View / Edit
- Held Items, Friendship and Pokérus
- generation-correct DV / Stat Exp behavior

### Generation III

- Pokémon Ruby
- Pokémon Sapphire
- Pokémon Emerald
- Pokémon FireRed
- Pokémon LeafGreen
- Trainer, Party and Boxes
- staged Pokémon Create / View / Edit
- IVs / EVs, Nature, Ability and PID-linked mechanics
- staged classic Inventory editing

### Generation IV

- Pokémon Diamond
- Pokémon Pearl
- Pokémon Platinum
- Pokémon HeartGold
- Pokémon SoulSilver
- strict Trainer / Party / Box reading
- staged Party and Box Pokémon View / Edit
- native PK4 checksum/encryption handling
- exact DP / Platinum / HGSS save-block validation

Generation IV **Create and the remaining full editor field set are actively being completed in G4-04 / PR #92**.

---

## One shared Pokémon editor

PokeBank NX uses one editor design across generations instead of building a separate UI for every game.

~~~text
shared Pokémon editor
        ↓
game / generation capabilities
        ↓
native staged adapter
        ↓
serialize + validate + reparse
~~~

That means the interaction stays familiar while the backend still respects each generation's actual rules.

The shared experience includes:

- controller and Left Stick navigation;
- held-stick scrolling;
- common Actions / View / Edit / Create flow;
- shared pickers and numeric input;
- contextual move editing;
- consistent themes and modal styling;
- dirty-edit protection so backing out cannot silently lose staged work.

---

## Save Instances

PokeBank NX separates **game identity** from the physical save file that provides it.

A single game can expose multiple validated Save Instances from supported providers without silently swapping one source for another.

Current provider support includes:

**Gen I–III**
- RetroArch
- configured mGBA save directories
- bounded Tico GB / GBC / GBA roots
- manual sources where supported

**Gen IV**
- RetroArch
- DraStic cartridge backups
- melonDS
- manual / remembered sources

DraStic **.dsv** cartridge backups are supported. **.dss** savestates are intentionally not treated as cartridge saves.

---

## Save safety

PokeBank NX is deliberately conservative with save data.

Current rules:

- normal editor actions work on app-owned staged/working data;
- malformed, ambiguous or wrong-game saves fail closed;
- remembered sources are revalidated before use;
- a different physical file is never silently substituted as the same source;
- installed-game and emulator-source injection remain disabled in current builds;
- cross-game True Move remains locked until route-specific safety is proven.

Future save writeback is being designed around:

~~~text
current source
    ↓
automatic immutable backup
    ↓
working copy
    ↓
edit + validate
    ↓
explicit Inject Save
~~~

That work is tracked separately in Issue #89.

---

## Current development

### Gen IV full editor — G4-04

The active main-development tranche is completing Generation IV editor parity with the mature Gen I–III experience.

Current work includes:

- Create Pokémon into empty Gen IV Box slots;
- Species editing;
- Held Item, Language, Ball and Pokérus controls;
- real DP / Platinum / HGSS Met Location data;
- native Gen IV move selection;
- fuller navigation across visible read-only information;
- exact Form support where Gen IV actually persists it;
- strict validation after every staged mutation.

Trainer identity and wider encounter-history fields remain read-only until their side effects are explicitly proven.

### Product UI and storage

The longer-term product roadmap includes:

- Master Vault and named Banks;
- search, filters, sorting and favorites;
- Living Dex / shiny collection views;
- richer provenance and legality presentation;
- backup management;
- broader SaveSource support;
- DS / 3DS expansion;
- modern Nintendo Switch game-save validation.

Modern Switch title support is tracked as **read-only validation first** before any staged mutation or writeback is authorized.

---

## Themes and controls

PokeBank NX is designed for both handheld and docked use.

Current UI work includes:

- Poké Classic, Dark, OLED and Light themes;
- readable shared control legends;
- D-pad and Left Stick parity;
- consistent A / B / X / Y glyphs;
- controller-first dialogs and pickers;
- graceful missing-art fallbacks;
- build/version diagnostics in Settings.

---

## Validation

The project uses permanent regression gates instead of relying only on manual testing.

Current validation includes:

- host regression suites;
- ASan / UBSan;
- native devkitA64 compile/link checks;
- exact-format save fixtures;
- malformed-input rejection;
- byte-preservation tests;
- staged-mutation rollback;
- source-immutability checks;
- exact Actions-built NRO hashes for hardware checkpoints.

Physical Switch testing is still required before a new milestone is called device accepted.

---

## Roadmap

~~~text
Gen I–III shared staged editor            DEVICE ACCEPTED
        ↓
Save Instances + source architecture       DEVICE ACCEPTED
        ↓
Gen IV Party + Box View/Edit               DEVICE ACCEPTED
        ↓
Gen IV full Create/Edit parity             ACTIVE
        ↓
automatic backup + explicit Inject Save
        ↓
Master Vault + named Banks
        ↓
broader DS / 3DS / modern Switch support
        ↓
conversion / legality / transfer expansion
        ↓
v1.0 hardening
~~~

The roadmap is intentionally capability-gated: discovering or parsing a save does not automatically authorize writing to it.

---

## Building

PokeBank NX is a native Switch **.nro** built with devkitPro / devkitA64 and libnx.

Generated source tables used by normal builds are committed to the repository so a standard build does not silently depend on live network data.

For exact engineering state, CI checkpoints and audit evidence, see:

- [Current Status](CURRENT_STATUS.md)
- [Project Status](PROJECT_STATUS.md)
- [v1 Roadmap](docs/V1_ROADMAP.md)
- [Game Support Matrix](docs/GAME_SUPPORT_MATRIX.md)
- [Reference Index](docs/REFERENCE_INDEX.md)

---

## Credits

PokeBank NX builds on years of Pokémon save research and homebrew work.

PKSE, PKHeX, PKSM / PKSM-Core, pkmn-chest, pkHouse and other community projects are used as references, correctness oracles or selective upstream data sources where appropriate.

PokeBank NX maintains its own product architecture for source discovery, staging, storage, recovery, provenance and write policy.

---

## License

PokeBank NX is licensed under **AGPL-3.0**. See [LICENSE](LICENSE).

---

## Disclaimer

PokeBank NX is an unofficial fan-made homebrew project. It is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK or Creatures Inc.

Pokémon and related trademarks, names and game assets are property of their respective owners.
