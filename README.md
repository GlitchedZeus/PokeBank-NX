<p align="center">
  <img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

<h1 align="center">PokeBank NX</h1>

<p align="center">
  Native, offline-first Pokémon storage, save browsing and staged editing for CFW Nintendo Switch.
</p>

<p align="center">
  <img alt="Status: Alpha" src="https://img.shields.io/badge/status-alpha-orange" />
  <img alt="Platform: Nintendo Switch" src="https://img.shields.io/badge/platform-Nintendo%20Switch-E60012" />
  <img alt="CFW: Atmosphère" src="https://img.shields.io/badge/CFW-Atmosph%C3%A8re-blue" />
  <img alt="License: AGPL-3.0" src="https://img.shields.io/badge/license-AGPL--3.0-lightgrey" />
</p>

PokeBank NX is a native Nintendo Switch homebrew application for browsing, preserving and editing Pokémon without requiring a PC or cloud service for normal use.

The project is intentionally conservative with save data. External game and emulator saves are treated as **immutable sources**; editing happens in PokeBank-owned staged workspaces. Unsupported or ambiguous data fails closed instead of being guessed into a new format.

**Last updated:** September 28, 2026

> **Alpha software:** keep independent backups of irreplaceable saves. PokeBank NX deliberately disables live writeback to installed games and emulator source files.

---

## Project status

PokeBank NX currently has a hardware-tested Gen I-IV source-browser foundation and a device-accepted shared editor for Generations I-III.

### Hardware-accepted milestones

| Area | Status |
|---|---|
| Red / Blue / Yellow staged Pokémon editor | ✅ Device accepted |
| Gold / Silver / Crystal staged Pokémon editor | ✅ Device accepted |
| Ruby / Sapphire / Emerald / FireRed / LeafGreen staged Pokémon editor | ✅ Device accepted |
| Classic staged inventory editing | ✅ Device accepted |
| D-pad / Left Stick navigation parity and held-repeat scrolling | ✅ Device accepted |
| Multi-provider **Save Instances** browser across Gen I-IV | ✅ Device accepted at exact tested checkpoint |
| Platinum / DraStic .dsv read-only loading | ✅ Hardware tested |
| FAT32 transaction interruption/recovery harness | ✅ 8 / 8 hardware tests passed |
| Cross-game True Move | 🔒 Disabled |
| Live external-source writes | 🔒 Disabled |
| Master Vault | 🗺️ Planned, not started |

The latest hardware-accepted Save Instances checkpoint is **d49efd0c16433aaa1aa171501671a6e811c9da64**.

The active development line is **PR #79** on **audit/full-project-hardening-20260923**. Its current provider-neutral Save Instance architecture is newer than the accepted artifact and therefore does **not** inherit device acceptance automatically.

---

## Supported games

### Generation I — staged editing

- Pokémon Red
- Pokémon Blue
- Pokémon Yellow

### Generation II — staged editing

- Pokémon Gold
- Pokémon Silver
- Pokémon Crystal

### Generation III — staged editing

- Pokémon Ruby
- Pokémon Sapphire
- Pokémon Emerald
- Pokémon FireRed
- Pokémon LeafGreen

### Generation IV — read only

- Pokémon Diamond
- Pokémon Pearl
- Pokémon Platinum
- Pokémon HeartGold
- Pokémon SoulSilver

Generation IV currently supports read-only Trainer, Party, Boxes and Pokémon detail browsing. Editing, Create/Delete, conversion and source writeback are intentionally not enabled for Gen IV.

---

## Save Instances

A game card represents the **game**, not one hard-coded emulator path.

Opening an external game source follows one model:

~~~text
GAME IDENTITY
    ↓
SAVE INSTANCES
    ↓
PROVIDER
    ↓
VALIDATED SAVE
    ↓
OPEN READ ONLY / authorized staged workspace
~~~

Example:

~~~text
Pokémon Crystal
  ├─ RetroArch — Pokemon Crystal.srm
  ├─ Tico      — Pokemon Crystal.sav
  ├─ mGBA      — Pokemon Crystal.sav
  └─ Manual    — MyOldCrystal.sav

Pokémon Platinum
  ├─ DraStic   — Platinum.dsv
  ├─ melonDS   — Platinum.sav
  ├─ RetroArch — Platinum.srm
  └─ Manual    — PlatinumBackup.sav
~~~

The browser preserves provider identity, deduplicates the same physical file, sorts trustworthy timestamps newest-first and keeps profile claims isolated.

### Current provider support

**Generation I-III**

- RetroArch
- configured mGBA battery-save directory
- Tico GB / GBC / GBA battery-save directories
- explicit/manual sources where supported

**Generation IV**

- RetroArch
- DraStic cartridge backups
- melonDS
- explicit/manual remembered files

Provider scans are deliberately bounded. PokeBank NX does not recursively crawl arbitrary ROM directories or the entire SD card. DraStic .dss files are savestates and remain unsupported as cartridge saves.

---

## Safety model

PokeBank NX separates **what can be viewed or edited** from **what may be written back**.

Permanent project rules:

- original external saves remain immutable;
- installed Switch saves are never modified by browsing;
- RetroArch, mGBA, Tico, DraStic, melonDS and other emulator sources remain read only;
- staged PokeBank-owned workspaces may be edited only where already supported;
- unknown save variants and exact-game mismatches fail closed;
- selecting or opening a save is non-destructive;
- a remembered source may never silently substitute a different file;
- recovery evidence is not treated as another active Pokémon;
- device acceptance belongs only to the exact NRO that was physically tested.

The project uses verified replacement, journaling, SHA-256 evidence and restart recovery for PokeBank-owned durable operations rather than assuming a filesystem write completed successfully.

---

## Shared editor architecture

The Gen I-III editor is one shared UI backed by generation-native adapters:

~~~text
shared Pokémon editor
        ↓
exact-game capability provider
        ↓
generation-native staged adapter
        ↓
strict serialization + validation
~~~

That keeps the interface consistent while each generation preserves its actual mechanics.

- Gen I: DVs, Stat Exp and generation-specific move/data rules
- Gen II: DVs, Stat Exp, Held Item, Friendship and Pokérus
- Gen III: IVs/EVs, Nature, Ability, PID-linked mechanics and richer origin data

The same principle now applies to source discovery: generation-specific parsers remain separate, while provider metadata, Save Instances presentation, deduplication, sorting and profile visibility converge on one shared model.

---

## Current engineering work

The current tranche is **Issue #85 — provider-neutral Save Instance architecture**.

The active PR #79 head at the latest project checkpoint is **00ee7a6ed7ac1b5a93c43246d70c252e135acec0**.

This work is consolidating duplicated source-browser plumbing used by Gen I-IV while preserving the already proven generation-specific parsers.

The shared source-instance layer carries common metadata such as exact game identity, provider identity, physical and normalized paths, stable source identity, file size/time, trainer and party summary, validation state, recovery diagnostics, provenance/fingerprint data, profile claims and an opaque generation-specific validation handle when needed.

The goal is a cleaner architecture, not a giant universal parser.

A fresh hardware test will be required for the newer runtime refactor before it can be called device accepted.

---

## Storage, Move, Copy and Clone

These operations are intentionally distinct.

**Move** — one logical Pokémon changes active location. Destination durability and verification must succeed before source retirement can ever be authorized.

**Copy** — an explicit duplicate while the source remains active.

**Exact Clone** — a deliberate native-payload duplicate as closely as the source format allows.

**Derived / Legit Clone** — a separate related Pokémon with its own future Vault identity and lineage relationship.

Cross-game product True Move remains locked. Converter research and transaction infrastructure do not automatically authorize source retirement.

---

## Validation and testing

PokeBank NX is developed with permanent regression gates rather than one-off manual checks.

Current engineering practices include:

- host C++ regression suites;
- ASan and UBSan;
- exact-head GitHub Actions checks;
- native devkitA64 compile/link validation;
- byte-exact save fixtures;
- source-immutability checks;
- strict malformed/unsupported-input rejection;
- conversion golden fixtures;
- transaction fault injection and restart recovery;
- exact NRO hashes for hardware checkpoints.

Generated source tables are committed to the repository. Normal builds do not regenerate data or require network access.

---

## Roadmap

PokeBank NX progresses by proven layers rather than adding generations as quickly as possible.

~~~text
Gen I-III shared staged editor              DEVICE ACCEPTED
        ↓
storage / recovery hardening                IMPLEMENTED + TESTED
        ↓
Gen IV strict read-only foundation          IMPLEMENTED
        ↓
multi-provider Save Instances               DEVICE ACCEPTED
        ↓
provider-neutral source architecture        CURRENT
        ↓
Gen IV read-only stabilization
        ↓
future generation work                      ONLY WHEN EXPLICITLY STARTED
        ↓
Master Vault / broader v1 hardening         PLANNED
~~~

Gen V and Master Vault are **not** current implementation tranches.

---

## Project documents

For exact engineering state and audit evidence, see:

- [Current Status](CURRENT_STATUS.md)
- [Project Status](PROJECT_STATUS.md)
- [v1 Roadmap](docs/V1_ROADMAP.md)
- [Game Support Matrix](docs/GAME_SUPPORT_MATRIX.md)
- [Issue #85 Save Instances audit](docs/audit/ISSUE85-SAVE-INSTANCES.md)
- [Full Project Audit](docs/FULL_PROJECT_AUDIT_2026-09-22.md)
- [Reference Index](docs/REFERENCE_INDEX.md)

The README is intentionally the public-facing overview. Exact CI run IDs, tree hashes and audit history belong in the engineering documents and GitHub issues.

---

## Building

PokeBank NX is a native Switch .nro built with devkitPro / devkitA64 and libnx.

The repository is intended to remain reproducible from committed source and generated data. Upstream data tables are refreshed only as deliberate maintenance work; a normal build does not silently pull new PKHeX or PokeAPI data.

---

## Credits and upstream references

PokeBank NX builds on years of Pokémon reverse engineering and homebrew work.

PKSE, PKHeX, PKSM / PKSM-Core, pkmn-chest and other community projects are used as format references, correctness oracles or selective upstream sources where appropriate.

PokeBank NX keeps its own architecture for source immutability, staging, provider discovery, durable storage, recovery, provenance and product write policy.

---

## License

PokeBank NX is licensed under **AGPL-3.0**. See [LICENSE](LICENSE).

---

## Disclaimer

PokeBank NX is an unofficial fan-made homebrew project. It is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK or Creatures Inc.

Pokémon and related trademarks, names and game assets are property of their respective owners.
