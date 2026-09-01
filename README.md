<p align="center">
<img width="1672" height="941" alt="THE FINAL" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is an offline Pokémon storage and save-management project for Nintendo Switch homebrew.

The project started from the PKSE codebase, but the goal is broader: one native Switch app for browsing Pokémon saves, keeping a permanent local collection, organizing Pokémon into banks, tracking where they came from, and eventually moving compatible Pokémon between games and generations without depending on an online service for the core collection.

This repository is under active development. It is **not** ready to be treated as a production-safe save manager yet.

---

## Project status

PokeBank NX has moved past the initial repository-recovery stage.

The interrupted PokeBank NX work that was still present was recovered, backed up, committed, and published to GitHub. The active development line is now:

```text
feature/pokebank-playable
```

The latest verified remote safety milestone from the recovery session is:

```text
c618bd5 — safety: hard-lock live game save writes
```

A native Nintendo Switch `.nro` builds successfully from the recovered development tree, and host tests plus ASan/UBSan pass. The current recovery build has **not yet been physically tested on a Switch**, so it is correctly classified as `NRO BUILDS`, not `DEVICE TESTED`.

The actual `.nro` binary from the recovery runtime is not currently committed or published as a GitHub release. It should be rebuilt from the verified source before the first hardware test.

For the detailed source-of-truth status, see [`PROJECT_STATUS.md`](PROJECT_STATUS.md).

### Current verification snapshot

| Area | Status |
|---|---|
| Repository recovery | **COMPLETE** |
| 23 stable release/platform game identities | **HOST TESTED** |
| GBA/Switch FireRed + LeafGreen identity separation | **HOST TESTED** |
| Platform-aware native game cards | **NRO BUILDS** |
| Host regression tests | **PASS** |
| ASan / UBSan | **PASS** |
| Native Switch `.nro` | **NRO BUILDS** |
| Direct live-game save writing | **HARD DISABLED** |
| Physical Switch execution | **NOT DEVICE TESTED** |
| Pokémon A-button action sheet | **NEXT ACTIVE FEATURE** |
| Master Vault / durable provenance | **REBUILD REQUIRED** |
| RetroArch discovery / Gen I-III adapters | **REBUILD REQUIRED** |

A feature is only promoted through these states when that level of verification has actually happened:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

A successful `.nro` build is not the same thing as successful testing on physical hardware.

---

## What is already in the repository

### Recovered PokeBank NX work

- stable internal game identity based on **release + platform**, not display name
- 23 target game identities
- separate identities for:
  - `firered_gba`
  - `leafgreen_gba`
  - `firered_switch`
  - `leafgreen_switch`
- platform-aware native Switch title cards
- app version + abbreviated Git commit display
- host-side test target independent of devkitPro
- AddressSanitizer / UndefinedBehaviorSanitizer test target
- read-only installed-game source handling
- backup-only save destinations during this alpha
- a generic save API that cannot request title injection
- a low-level filesystem guard that rejects live-title restore attempts before mounting save data
- legacy `injectToGame=1` settings ignored and rewritten to disabled

### PKSE upstream foundation

The imported PKSE 1.1.3 history/source still gives PokeBank NX a substantial native Switch foundation:

- Nintendo Switch `.nro` application and build system
- controller-driven UI
- save selection, backup and restore infrastructure
- party and box browsing
- Pokémon editing and creation
- trainer and item editing
- existing legality-related UI/data
- existing bank/conversion framework
- Switch save access
- generated Pokémon/game data tables
- save handlers for the Switch game families represented by PKSE 1.1.3

That upstream functionality is a foundation. It is **not** automatically considered finished PokeBank NX functionality until it has been integrated into the Vault, identity, provenance and safety model and verified accordingly.

---

## What PokeBank NX is trying to become

The end goal is an offline Pokémon collection hub that lives on the Switch.

A normal workflow should eventually look like:

```text
Game / save source
        ↓
Party / boxes
        ↓
Select Pokémon
        ↓
Action menu
        ↓
PokeBank NX
   ├── Master Vault
   ├── Named Banks
   ├── Pokédex / Living Dex
   ├── Shiny Living Dex
   └── Transfer / Export
```

The app should feel like a Switch application first, not a desktop editor squeezed onto a console.

That means:

- controller-first navigation
- fast box browsing
- clear game artwork and platform labels
- a proper Pokémon summary screen
- safe, deliberate actions
- useful search and organization
- simple backup/recovery
- no subscription or online account required for the core collection

---

## Design rules

### Original data stays original

Imported saves and original Pokémon records are source material, not disposable working copies.

PokeBank NX is being designed around:

- preserving original Pokémon bytes where practical
- keeping backups before writes
- staging modifications before committing them
- validating data after conversion
- keeping rollback/recovery information
- never silently deleting or replacing the source Pokémon

Direct live-game save writing is currently hard-disabled while the safe write pipeline is being built and tested.

### Origin and current location are different things

If a Charizard was originally caught in FireRed on GBA and later moved into another game, PokeBank NX should remember both facts.

Example:

```text
Original origin:
FireRed — Game Boy Advance

Current location:
Master Vault

Transfer history:
FireRed GBA → Vault → Sword
```

A Pokémon's history should not disappear every time it moves.

### Every release is its own game identity

PokeBank NX identifies a game by its release and platform, not just its title.

For example:

```text
firered_gba
leafgreen_gba

firered_switch
leafgreen_switch
```

Those are four different game identities.

That makes a future transfer such as:

```text
FireRed — GBA
      ↓
PokeBank NX
      ↓
FireRed — Nintendo Switch
```

a real cross-game transfer instead of pretending both releases are the same save environment.

---

## Target game coverage

Detection and full support are separate milestones. A title appearing in the catalog does not mean its parser/write path is complete.

### Game Boy / Game Boy Color

- Pokémon Red
- Pokémon Blue
- Pokémon Yellow
- Pokémon Gold
- Pokémon Silver
- Pokémon Crystal

Current state: stable identities exist; recovered parsers do not.

### Game Boy Advance

- Pokémon Ruby
- Pokémon Sapphire
- Pokémon Emerald
- Pokémon FireRed
- Pokémon LeafGreen

Current state: stable identities exist; recovered parsers do not.

### Nintendo DS

- Pokémon Diamond
- Pokémon Pearl
- Pokémon Platinum
- Pokémon HeartGold
- Pokémon SoulSilver
- Pokémon Black
- Pokémon White
- Pokémon Black 2
- Pokémon White 2

Current state: planned expansion.

### Nintendo 3DS

- Pokémon X
- Pokémon Y
- Pokémon Omega Ruby
- Pokémon Alpha Sapphire
- Pokémon Sun
- Pokémon Moon
- Pokémon Ultra Sun
- Pokémon Ultra Moon

Current state: planned expansion.

### Nintendo Switch

- Pokémon FireRed
- Pokémon LeafGreen
- Pokémon: Let's Go, Pikachu!
- Pokémon: Let's Go, Eevee!
- Pokémon Sword
- Pokémon Shield
- Pokémon Brilliant Diamond
- Pokémon Shining Pearl
- Pokémon Legends: Arceus
- Pokémon Scarlet
- Pokémon Violet
- Pokémon Legends: Z-A

Current state: platform-aware identities build; existing PKSE handlers remain present and still need full PokeBank NX integration and source-specific validation.

---

## Pokémon view and action menu

The current active development feature is the controller-first Pokémon action sheet.

Pressing **A** on a focused Pokémon must open a deliberate menu instead of performing an immediate mutation.

Target menu:

```text
View Pokémon
Add to Master Vault
Add to Bank…
Transfer to Game…
Edit
Clone
Make Shiny
Legality & Provenance
Cancel
```

The A button must never silently move, delete, clone, edit or rewrite a Pokémon.

### Summary screen

Where the source format supports it, the summary should include:

- species and National Dex number
- nickname
- level
- gender
- shiny status
- form
- type
- nature
- ability
- held item
- moves
- IVs / EVs
- stats
- Original Trainer
- Trainer ID / Secret ID
- origin game and platform
- current location
- met information
- Poké Ball
- language
- ribbons / marks
- legality state
- provenance

Older formats should only display information that actually exists in that generation.

---

## Master Vault

The **Master Vault** is the core PokeBank NX concept.

It is intended to be a permanent, game-independent archive of a collection rather than another temporary PC box.

The previous custom Vault implementation was not recoverable from the interrupted workspace, so it must be rebuilt from the project specification and tests.

A Vault record should eventually track information such as:

```text
stable Vault ID
exact original Pokémon bytes
format / generation
SHA-256

original game
original platform
source save
original box / slot

current location
parent / derived record
clone relationship

transfer history
transformation history
legality state
provenance
```

The original record should be immutable by default. A conversion, clone or transformation should create a derived record rather than destroy the source.

---

## Banks

Named banks sit on top of the Master Vault for day-to-day organization.

Examples:

```text
Living Dex
Shiny Living Dex
Events
Competitive
Favorites
Gen III
Sword & Shield
Scarlet & Violet
Legends: Z-A
```

Planned bank features:

- unlimited named banks
- multiple boxes per bank
- custom box names
- copy / explicit move
- clone
- duplicate detection
- search
- filtering
- sorting
- favorites / tags
- bulk organization
- import / export

Where possible, banks should reference Vault records instead of creating unnecessary duplicate Pokémon records.

---

## Pokédex and Living Dex

The Master Vault should be authoritative for **what Pokémon the user actually owns**.

The Pokédex should be a view/index over that collection rather than a second manual database that can drift out of sync.

Planned views include:

- National Pokédex
- regional Pokédexes
- Living Dex
- Shiny Living Dex
- regional forms
- permanent alternate forms
- meaningful gender differences
- Alpha / Shiny Alpha where applicable
- event ownership
- owned / missing Pokémon
- generation completion
- completion percentages

The `Insektaure/pkDex` project is being tracked as a Switch Pokédex UX/data reference, while PokeBank NX will implement its own Vault-driven Pokédex.

---

## Fill Master Vault — All + Shinies

A planned convenience tool will be able to populate a collection automatically.

The target is:

- every supported species
- one normal version
- one shiny version where legally possible
- regional forms
- meaningful permanent forms
- meaningful gender differences

Generation must be:

- deterministic
- duplicate-aware
- encounter-driven
- legality-aware
- safe to rerun

It must not create impossible shiny-locked Pokémon and label them legal.

---

## Legality and provenance

PokeBank NX separates two questions:

1. **Where did this Pokémon come from?**
2. **Is its data valid for the game/encounter it claims to come from?**

The provenance view should show information such as:

- original game
- original platform
- source save identity
- current location
- parent/clone relationship
- edits and transformations
- transfer history
- legality result
- warnings
- unsupported / unknown checks

Legality states should remain explicit:

```text
LEGAL
INVALID
UNKNOWN
```

`UNKNOWN` must never silently become `LEGAL`.

---

## Make Shiny

The planned **Make Shiny** action is not a simple shiny-bit toggle.

Where supported, it should account for:

- shiny locks
- event restrictions
- PID-related rules
- trainer ID relationships
- game restrictions
- form restrictions
- legality

The preferred workflow is to create a derived copy and preserve the source record.

---

## Transfers

Cross-game transfers are one of the larger goals of the project.

PokeBank NX should eventually distinguish between:

### Copy to Game

Create a destination-compatible copy while leaving the source intact.

### Move to Game

Write and verify the destination first, then complete the move safely.

### Trade

A future workflow involving two separate game/save identities.

### Add to Vault

Store the Pokémon independently from either game.

A transfer must never be reported as complete if the destination was not actually written and verified.

---

## Pokémon HOME bridge

PokeBank NX is **not** intended to impersonate the official Pokémon HOME client or directly connect to Nintendo/Pokémon servers.

The planned bridge is deliberately based on official game compatibility:

```text
PokeBank NX
     ↓
compatible Switch game
     ↓
official Pokémon HOME
```

Reverse direction:

```text
official Pokémon HOME
     ↓
compatible Switch game
     ↓
PokeBank NX
     ↓
Master Vault
```

Future HOME Bridge work may include:

- destination compatibility selection
- recommended bridge-game selection
- conservative preflight validation
- batch import/export preparation
- preservation of HOME-related identity/provenance when it already exists

PokeBank NX should not generate or forge HOME trackers, and it should not claim that generated or edited Pokémon are "ban safe" or guaranteed to pass future server checks.

---

## Save safety

Direct live-game save writing is currently **hard-disabled**.

Any future live-write path should follow a process similar to:

```text
Read source save
      ↓
Create backup
      ↓
Hash / record source
      ↓
Create staged copy
      ↓
Apply change
      ↓
Repair checksums / structures
      ↓
Reparse + validate
      ↓
Write destination
      ↓
Read back
      ↓
Verify intended mutation
      ↓
Keep recovery copy
```

Anything failing should leave the original recoverable.

Save safety matters more than convenience. A Pokémon collection can represent years of playtime.

---

## Startup and loading

The current visual direction for the app is the Route 1-style artwork shown at the top of this README.

The planned startup sequence is:

```text
PokeBank NX splash
      ↓
Load settings
      ↓
Initialize services
      ↓
Load game registry
      ↓
Load Pokémon data
      ↓
Initialize sprites
      ↓
Open Master Vault
      ↓
Load Banks
      ↓
Scan saves
      ↓
Home
```

Rather than a fake timer, startup progress should represent real initialization work.

One planned visual option is to use the Pokémon along the bottom of the splash as progress milestones, lighting them up as real startup stages complete.

---

## Upstream and reference stack

Before PokeBank NX reimplements major Pokémon-format, legality, conversion, save-parsing, Pokédex, banking or generation systems, the project now explicitly audits mature existing work first.

| Project | Role in PokeBank NX |
|---|---|
| [PKSE](https://github.com/kiasta/PKSE) | Original native Switch foundation. Upstream only. |
| [PKSM-Core](https://github.com/FlagBrew/PKSM-Core) | High-priority native C++ Pokémon/save engine candidate, especially Gen I-VIII. |
| [PKHeX](https://github.com/kwsch/PKHeX) | Primary correctness/reference implementation for formats, saves, legality, encounters and conversion. |
| [Auto Legality Mod / PKHeX-Plugins](https://github.com/santacrab2/PKHeX-Plugins) | Encounter-driven legality/generation reference and planned host-side test/generation support. |
| [pkHouse](https://github.com/Insektaure/pkHouse) | Modern Switch save/bank behavior reference: LGPE, SwSh, BDSP, PLA, SV, Z-A, Switch FRLG, SCBlocks, Pokédex/HT behavior and more. |
| [pkDex](https://github.com/Insektaure/pkDex) | Switch Pokédex UX/data reference. PokeBank NX builds its own Vault-driven Pokédex. |
| [PKForge](https://github.com/sofianeelhor/PKForge) | Vault/provenance/atomic-write architecture reference and PKHeX/AutoMod integration example. |

### pkHouse / pkDex policy

The pkHouse author, Insektaure, explicitly encouraged using pkHouse as a reference and recommended reimplementing needed behavior rather than pure copy/paste. The author also offered to answer technical questions about how parts of pkHouse work.

For PokeBank NX, pkHouse and pkDex are therefore treated as **REFERENCE ONLY** unless a future, explicit code-reuse decision is made with compatible licensing.

### PKHeX Oracle

A planned host-side developer tool will pin compatible revisions of PKHeX.Core and Auto Legality Mod and expose machine-readable legality, encounter, conversion and generation results.

Conceptually:

```text
PKHeX.Core + AutoMod
        ↓
PokeBank Oracle
        ↓
JSON / golden test vectors
        ↓
compare with native C++ PokeBank NX behavior
```

The Oracle runs on the development computer. The Switch application remains native C++.

---

## Roadmap

This roadmap tracks **PokeBank NX**, not the upstream PKSE project.

### 0. Repository recovery and safety baseline

- [x] Create `GlitchedZeus/PokeBank-NX`
- [x] Preserve/import PKSE 1.1.3 source and history
- [x] Keep PKSE as upstream-only
- [x] Recover the interrupted 23-game-identity milestone
- [x] Publish a verified recovery branch to GitHub
- [x] Add `PROJECT_STATUS.md`
- [x] Add host regression tests
- [x] Add ASan / UBSan test target
- [x] Verify native `.nro` build
- [x] Add app version + abbreviated Git commit display
- [x] Hard-disable live game-save writes during alpha development
- [ ] Run the recovered build on physical Switch hardware
- [ ] Publish a repeatable test `.nro` artifact/release

### 1. Upstream reuse audit

- [x] Track PKSE
- [x] Track PKSM-Core
- [x] Track PKHeX
- [x] Track Auto Legality Mod / PKHeX-Plugins
- [x] Track pkHouse
- [x] Track pkDex
- [x] Track PKForge
- [ ] Audit PKSM-Core for direct native C++ integration/adaptation
- [ ] Build the host-side PKHeX Oracle
- [ ] Add golden comparison tests against PKHeX
- [ ] Record exact upstream files/SHAs/licenses when functionality is adopted

### 2. PokeBank NX identity and startup

- [x] Finalize project name
- [x] Create project splash/header artwork
- [x] Add build version + abbreviated Git commit display
- [ ] Replace remaining PKSE/PokeVault branding in the app
- [ ] Add final PokeBank NX icon / NRO metadata
- [ ] Add real startup/loading screen
- [ ] OLED Black theme
- [ ] Dark theme
- [ ] Light theme

### 3. Game identity and discovery

- [x] Stable release/platform game IDs
- [x] FireRed GBA identity
- [x] LeafGreen GBA identity
- [x] FireRed Switch identity
- [x] LeafGreen Switch identity
- [x] Clear platform labels on native game cards
- [ ] RetroArch save discovery
- [ ] Manual save import
- [ ] Validate Switch FireRed/LeafGreen outer save containers

### 4. Controller-first playable workflow

- [x] Native installed-title cards
- [ ] Pokémon A-button action sheet
- [ ] Game → party flow under PokeBank identity/source model
- [ ] Game → boxes flow under PokeBank identity/source model
- [ ] Pokémon summary screen
- [ ] Controller shortcuts
- [ ] Handheld/docked layout pass

### 5. Master Vault and banks

- [ ] Immutable Master Vault
- [ ] Stable Vault IDs
- [ ] Exact raw entity preservation
- [ ] SHA-256 / integrity metadata
- [ ] Persistent Vault database/store
- [ ] Named banks
- [ ] Add to Master Vault
- [ ] Add to Bank
- [ ] Clone / derived-record lineage
- [ ] Duplicate handling
- [ ] Provenance chain
- [ ] Original/current-location tracking
- [ ] Backup/recovery tools

### 6. Retro save support

- [ ] Audit/adapt PKSM-Core before rewriting mature Gen I-VIII logic
- [ ] Red / Blue / Yellow
- [ ] Gold / Silver / Crystal
- [ ] Ruby / Sapphire / Emerald
- [ ] FireRed / LeafGreen GBA
- [ ] Read-only import validation
- [ ] Byte-identical no-edit round-trip tests where applicable
- [ ] Regression tests for each parser

### 7. Pokémon tools

- [ ] Full summary screen
- [ ] Controller-friendly editor
- [ ] Legality viewer
- [ ] Provenance viewer
- [ ] Make Shiny
- [ ] Search
- [ ] Filter
- [ ] Sort
- [ ] Import / export Pokémon files
- [ ] PKHeX Oracle comparison tests

### 8. Nintendo Switch title integration

- [ ] FireRed Switch adapter integration
- [ ] LeafGreen Switch adapter integration
- [ ] Let's Go Pikachu / Eevee integration
- [ ] Sword / Shield integration
- [ ] Brilliant Diamond / Shining Pearl integration
- [ ] Legends: Arceus integration
- [ ] Scarlet / Violet integration
- [ ] Legends: Z-A integration
- [ ] Handling Trainer behavior
- [ ] Pokédex registration behavior
- [ ] save checksum / round-trip validation

The imported PKSE code already contains handlers for these Switch families. This phase is about integrating that functionality into PokeBank NX's Vault, game identities, provenance and safety rules. pkHouse is tracked as a reference for modern Switch behavior, not as code to paste wholesale.

### 9. DS / 3DS support

- [ ] Diamond / Pearl / Platinum
- [ ] HeartGold / SoulSilver
- [ ] Black / White
- [ ] Black 2 / White 2
- [ ] X / Y
- [ ] Omega Ruby / Alpha Sapphire
- [ ] Sun / Moon
- [ ] Ultra Sun / Ultra Moon

### 10. Pokédex and living collection

- [ ] National Pokédex
- [ ] Regional Pokédexes
- [ ] Vault-driven ownership indexing
- [ ] Living Dex
- [ ] Shiny Living Dex
- [ ] form tracking
- [ ] meaningful gender differences
- [ ] Alpha / Shiny Alpha tracking where applicable
- [ ] event ownership
- [ ] missing Pokémon views
- [ ] completion statistics

### 11. Generated collection tools

- [ ] Encounter-driven generator
- [ ] Auto Legality / PKHeX reference integration in development tooling
- [ ] shiny-lock data
- [ ] generated-record provenance
- [ ] Fill Master Vault
- [ ] Fill Master Vault — All + Shinies
- [ ] deterministic / duplicate-aware reruns

### 12. Transfers

- [ ] Compatibility matrix
- [ ] Destination conversion
- [ ] Cross-generation conversion
- [ ] Provenance-preserving transfer history
- [ ] FireRed GBA → FireRed Switch
- [ ] LeafGreen GBA → LeafGreen Switch
- [ ] staged destination writes
- [ ] validation
- [ ] rollback/recovery

### 13. Pokémon HOME bridge

- [ ] HOME-compatible destination selector
- [ ] Conservative HOME preflight validation
- [ ] Vault → compatible Switch game → official HOME workflow
- [ ] Official HOME → compatible Switch game → Vault workflow
- [ ] Preserve existing HOME-related identity/provenance data
- [ ] Recommended bridge-game selection
- [ ] Batch HOME import/export preparation
- [ ] External-storage adapter architecture for future official services

PokeBank NX will not directly impersonate Pokémon HOME or forge HOME tracker/server history.

### 14. Events and advanced collection data

- [ ] Mystery Gift support
- [ ] Wonder Cards
- [ ] Event Vault
- [ ] event metadata
- [ ] event legality
- [ ] ribbons / marks
- [ ] event collection tracking

### 15. Safe live writing and release hardening

- [x] Hard-lock live game-save writing during early alpha
- [ ] backup-before-mutation pipeline
- [ ] staged save clone
- [ ] checksum/structure repair
- [ ] reparse + validate written save
- [ ] read-back verification
- [ ] rollback/recovery
- [ ] corruption detection
- [ ] low-space handling
- [ ] SD-card failure handling
- [ ] crash recovery
- [ ] large-Vault stress testing
- [ ] performance pass
- [ ] complete regression suite
- [ ] repeated physical Switch validation
- [ ] release packaging
- [ ] user documentation
- [ ] stable v1.0

---

## Build

PokeBank NX currently inherits the PKSE Switch build system.

### Host tests

The recovered development branch has dedicated host-side test targets:

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
```

### Native Nintendo Switch build

The recovery environment used devkitPro/devkitA64 and a native `make -j1` integration build.

The exact environment setup is tracked in [`PROJECT_STATUS.md`](PROJECT_STATUS.md).

The existing asset/data generator remains available, including:

```bash
python tools/gen_hdsprites.py
```

A native build should always be reported separately from host testing and physical hardware validation.

---

## Development workflow

GitHub is the permanent project state. Temporary coding-agent workspaces are not.

A normal meaningful milestone should follow:

```text
implement
   ↓
host tests
   ↓
sanitzer / regression checks
   ↓
build .nro
   ↓
update PROJECT_STATUS.md
   ↓
commit
   ↓
push to GlitchedZeus/PokeBank-NX
   ↓
verify remote SHA
   ↓
continue
```

The original PKSE repository remains **upstream only** and must never receive PokeBank NX-specific pushes.

Before implementing major Pokémon infrastructure, contributors should also check the tracked reference projects rather than rewriting mature functionality unnecessarily.

---

## Contributing

PokeBank NX is still changing quickly, so large pull requests should be discussed before replacing major architecture.

Priorities:

1. do not lose user data
2. preserve original Pokémon and provenance
3. keep live writes disabled until the safety pipeline is proven
4. keep the Switch UI simple and controller-first
5. avoid regressions
6. test save-format changes
7. prefer mature, verified research over unnecessary reinvention
8. keep the build reproducible
9. keep GitHub and `PROJECT_STATUS.md` current

Read [`PROJECT_STATUS.md`](PROJECT_STATUS.md) before starting development work.

---

## Upstream and credits

PokeBank NX is derived from **PKSE — Pokémon Save Editor** and intentionally keeps its upstream history.

The project also studies or relies on work from the wider Pokémon and Switch homebrew communities, including:

- [PKSE](https://github.com/kiasta/PKSE)
- [PKSM-Core](https://github.com/FlagBrew/PKSM-Core)
- [PKHeX](https://github.com/kwsch/PKHeX)
- [Auto Legality Mod / PKHeX-Plugins](https://github.com/santacrab2/PKHeX-Plugins)
- [pkHouse](https://github.com/Insektaure/pkHouse)
- [pkDex](https://github.com/Insektaure/pkDex)
- [PKForge](https://github.com/sofianeelhor/PKForge)
- [PokeAPI sprites](https://github.com/PokeAPI/sprites)
- [devkitPro](https://devkitpro.org/)
- [libnx](https://github.com/switchbrew/libnx)

Reference does not automatically mean source code is copied or included. Applicable upstream notices, attribution and license requirements must be preserved whenever code is directly reused.

---

## License

The PokeBank NX codebase is licensed under the **GNU Affero General Public License v3.0** as inherited from PKSE.

See [`LICENSE`](LICENSE) for the full license text.

---

## Disclaimer

PokeBank NX is an unofficial, fan-made homebrew project.

It is not affiliated with, sponsored by, or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc.

Pokémon and related trademarks are the property of their respective owners.

This repository is intended for homebrew software and management of a user's own save/Pokémon data. It does not include commercial game ROMs, Nintendo proprietary software, or other copyrighted game files that users do not have permission to redistribute.

Always keep independent backups of important saves while using development builds.

---

## Current focus

The immediate goal is **not** to implement every generation at once.

The current development path is:

```text
Verified GitHub baseline
      ↓
Upstream reuse audit
      ↓
A-button Pokémon action sheet
      ↓
Master Vault v1
      ↓
Named Banks
      ↓
Pokémon summary + provenance
      ↓
Retro read-only adapters
      ↓
Modern Switch integration
      ↓
Cross-generation conversion
```

The first genuinely useful PokeBank NX milestone remains:

```text
Boot PokeBank NX
      ↓
Detect supported games
      ↓
Browse party / boxes
      ↓
Open deliberate Pokémon action menu
      ↓
View Pokémon summary
      ↓
Add to Master Vault / Bank
      ↓
Persist safely
```

Once that works reliably on a physical Switch, the rest of the roadmap becomes expansion of a functioning app rather than construction of a prototype.
