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

The project is intentionally conservative with save data. Ordinary editing happens only in **PokeBank-owned staged/working copies**; emulator source files are not modified while you edit. Unsupported or ambiguous data fails closed instead of being guessed into another format. A future explicit **Inject Save** workflow (Issue #89) is planned to write a validated edited copy back only after an automatic immutable backup has been verified.

**Last updated:** September 28, 2026

> **Alpha software:** keep independent backups of irreplaceable saves. Current builds still disable emulator/source injection; Issue #89 tracks automatic backups plus an explicit validated Inject Save transaction for a later milestone.

---

## Project status

PokeBank NX currently has a device-accepted Gen I-III shared staged editor, a device-accepted Gen I-IV Save Instances/source-browser foundation, and an active Gen IV **Party + Box View/Edit** hardware-retest milestone.

| Area | Status |
|---|---|
| Red / Blue / Yellow staged Pokémon editor | ✅ Device accepted |
| Gold / Silver / Crystal staged Pokémon editor | ✅ Device accepted |
| Ruby / Sapphire / Emerald / FireRed / LeafGreen staged Pokémon editor | ✅ Device accepted |
| Classic staged inventory editing | ✅ Device accepted |
| D-pad / Left Stick parity + held-repeat scrolling | ✅ Device accepted |
| Multi-provider Save Instances across Gen I-IV | ✅ Device accepted |
| Provider-neutral Save Instance backend | ✅ Device accepted |
| Platinum / DraStic .dsv read-only loading | ✅ Hardware tested |
| FAT32 interruption/recovery harness | ✅ 8 / 8 hardware tests passed |
| Gen IV Party + Box Pokémon View/Edit | 🧪 Automated gates pass / hardware retest pending |
| Cross-game True Move | 🔒 Disabled |
| Live external-source writes | 🔒 Disabled |
| Master Vault | 🗺️ Planned |

The latest device-accepted provider-neutral source-browser checkpoint is:

`00ee7a6ed7ac1b5a93c43246d70c252e135acec0`

The active feature line is **PR #87 — G4-03: Gen IV shared staged Pokémon editor**, stacked on the accepted audit/source architecture in PR #79.

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

### Generation IV — read foundation + staged editor in active development

- Pokémon Diamond
- Pokémon Pearl
- Pokémon Platinum
- Pokémon HeartGold
- Pokémon SoulSilver

Generation IV already supports strict Trainer, Party, Boxes and Pokémon-detail browsing.

The first G4-03 hardware candidate was **rejected on Switch** because a party-only Platinum save could not reach the editor and Party View fell through to an older generic read-only screen. The corrected work now routes **Party and Box Pokémon** into the shared View/Edit surface and stages both PK4 record types safely. The corrected exact Actions-built candidate has passed all required automated gates and is ready for physical Switch retesting.

Gen IV Create, Inventory editing, source injection and cross-game True Move remain disabled.

---

## One shared Pokémon editor

PokeBank NX does not build a new editor UI for every generation.

~~~text
shared Pokémon editor
        ↓
exact-game capability provider
        ↓
generation-native staged adapter
        ↓
strict serialization + validation
~~~

That keeps the interface familiar while each generation preserves its real mechanics.

Examples:

- Gen I: DVs, Stat Exp and generation-specific move/data rules
- Gen II: DVs, Stat Exp, Held Item, Friendship and Pokérus
- Gen III: IVs/EVs, Nature, Ability, PID-linked mechanics and richer origin data
- Gen IV: PK4 encryption/checksum rules, exact DP/Pt/HGSS save-block integrity and PID-linked Nature/Gender/Shiny/Ability behavior

The active Gen IV milestone intentionally reuses the existing editor layout, numpad/keyboard/picker patterns, move editor, joystick behavior, themes, discard confirmation and direct inline controls such as Gender.

---

## Save Instances

A game card represents the **game**, not one hard-coded emulator path.

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

A single game may expose independent validated saves from different providers without silently substituting one source for another.

Current provider support includes:

**Generation I-III**
- RetroArch
- configured mGBA battery-save directory
- Tico GB / GBC / GBA save roots
- explicit/manual sources where supported

**Generation IV**
- RetroArch
- DraStic cartridge backups
- melonDS
- explicit/manual remembered files

Provider scans are deliberately bounded. PokeBank NX does not recursively crawl arbitrary ROM directories or the entire SD card. DraStic `.dss` files are savestates and remain unsupported as cartridge saves.

---

## Safety model

Permanent project rules:

- ordinary editing never mutates an external emulator source;
- installed-game live writes remain disabled;
- emulator source injection remains disabled in current builds;
- PokeBank-owned staged workspaces may be edited only where explicitly supported;
- unknown save variants and exact-game mismatches fail closed;
- a remembered source may never silently substitute another physical file;
- selecting or opening a save is non-destructive;
- device acceptance belongs only to the exact NRO physically tested;
- cross-game True Move remains locked until route-specific safety is proven.

PokeBank-owned durable operations use validation, hashes, journaling and recovery rather than assuming a filesystem write completed successfully.

---

## Active engineering work

### G4-03 — Gen IV shared staged Pokémon editor

Tracking issue: **#86**  
Draft PR: **#87**

Current scope is deliberately narrow:

- Party + boxed PK4 View/Edit;
- shared Gen I-III editor UX reused;
- exact DP / Platinum / HGSS capability mapping;
- mutable PK4 serialization using the existing Gen IV crypto/checksum layer;
- staged save mutation only;
- strict reparse after commit;
- Storage CRC refresh for boxed edits and General-block CRC refresh for Party edits;
- unrelated-byte preservation;
- constrained PID-linked edits for Nature, Gender, Shiny and Ability;
- source bytes unchanged during ordinary editing;
- staged dirty-session exit protection so repeated B cannot silently lose work.

Not part of this milestone:

- Gen IV Create;
- Inventory editing;
- explicit source injection (tracked separately in Issue #89);
- cross-game True Move;
- Gen V;
- Master Vault.

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

PokeBank NX uses permanent regression gates rather than relying only on manual testing.

Current practices include:

- host C++ regression suites;
- ASan and UBSan;
- exact-head GitHub Actions;
- native devkitA64 compile/link validation;
- byte-exact fixtures;
- source-immutability checks;
- malformed/unsupported-input rejection;
- transaction fault injection and restart recovery;
- exact NRO hashes for physical checkpoints.

Generated source tables are committed to the repository. Normal builds do not regenerate data or require network access.

---

## Roadmap

~~~text
Gen I-III shared staged editor              DEVICE ACCEPTED
        ↓
storage / recovery hardening                IMPLEMENTED + TESTED
        ↓
Gen IV strict read-only foundation          IMPLEMENTED
        ↓
multi-provider Save Instances               DEVICE ACCEPTED
        ↓
provider-neutral source architecture        DEVICE ACCEPTED
        ↓
Gen IV Party + Box shared View/Edit         HARDWARE RETEST / PR #87
        ↓
Gen IV editor stabilization + hardware pass
        ↓
Gen IV Create / broader staged features     AFTER EDIT IS PROVEN
        ↓
broader v1 hardening
~~~

Gen V and Master Vault are not current implementation tranches. Automatic source backup + explicit Inject Save is tracked separately in Issue #89 and remains disabled in current builds.

---

## Project documents

For engineering state and audit evidence:

- [Current Status](CURRENT_STATUS.md)
- [Project Status](PROJECT_STATUS.md)
- [v1 Roadmap](docs/V1_ROADMAP.md)
- [Game Support Matrix](docs/GAME_SUPPORT_MATRIX.md)
- [Issue #85 Save Instances audit](docs/audit/ISSUE85-SAVE-INSTANCES.md)
- [Full Project Audit](docs/FULL_PROJECT_AUDIT_2026-09-22.md)
- [Reference Index](docs/REFERENCE_INDEX.md)

The README is intentionally the public-facing overview. Exact CI run IDs, tree hashes and audit breadcrumbs belong in engineering documents and GitHub issues/PRs.

---

## Building

PokeBank NX is a native Switch `.nro` built with devkitPro / devkitA64 and libnx.

The repository is intended to remain reproducible from committed source and generated data. Upstream tables are refreshed only as deliberate maintenance work; a normal build does not silently pull new PKHeX or PokeAPI data.

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
