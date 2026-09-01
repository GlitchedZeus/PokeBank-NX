<p align="center">
  <img src="assets/branding/pokebank-nx-header.png" alt="PokeBank NX" width="100%">
</p>

# PokeBank NX

**PokeBank NX** is an offline Pokémon storage and save-management project for Nintendo Switch homebrew.

The project started from the PKSE codebase, but the goal is broader: one native Switch app for browsing Pokémon saves, keeping a permanent local collection, organizing Pokémon into banks, tracking where they came from, and eventually moving compatible Pokémon between games and generations without depending on an online service.

This repository is under active development. It is not ready to be treated as a production-safe save manager yet.

---

## Project status

The public `main` branch currently contains the full **PKSE 1.1.3** source and Git history as the upstream foundation. PokeBank NX-specific development is being recovered and rebuilt on top of that base.

The rule for this project is simple: a feature only gets marked complete here after the code is present in this repository and the relevant build/tests pass.

### What is already in the repository

The imported PKSE foundation gives PokeBank NX a substantial starting point:

- native Nintendo Switch `.nro` application and build system
- controller-driven UI
- save backup and restore infrastructure
- party and box browsing
- Pokémon editing
- trainer and item editing
- Pokémon creation
- legality-related checks
- shared cross-game bank infrastructure
- Switch save access
- generated Pokémon/game data tables
- dark and light UI foundations
- support in the upstream code for:
  - Pokémon FireRed / LeafGreen on Nintendo Switch
  - Pokémon: Let's Go, Pikachu! / Eevee!
  - Pokémon Sword / Shield
  - Pokémon Brilliant Diamond / Shining Pearl
  - Pokémon Legends: Arceus
  - Pokémon Scarlet / Violet
  - Pokémon Legends: Z-A

The PokeBank NX layer will keep the useful parts of that foundation while changing the product around it.

---

## What PokeBank NX is trying to become

The end goal is an offline Pokémon collection hub that lives on the Switch.

A typical workflow should eventually look like this:

```text
Game save
   ↓
Party / boxes
   ↓
Select Pokémon
   ↓
PokeBank NX
   ├── Master Vault
   ├── Named Banks
   ├── Living Dex
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

A few decisions are non-negotiable because they affect almost every part of the project.

### Original data stays original

Imported saves and original Pokémon records should be treated as source material, not disposable working copies.

PokeBank NX is being designed around:

- preserving original bytes where practical
- keeping backups before writes
- staging modifications before committing them
- validating data after conversion
- keeping rollback/recovery information
- never silently deleting or replacing the source Pokémon

Until the PokeBank NX write pipeline has been fully validated on hardware, development builds should be treated with the same caution as any experimental save editor.

### Origin and current location are different things

If a Charizard was originally caught in FireRed on GBA and later moved into another game, PokeBank NX should remember both facts.

Example:

```text
Original origin:
FireRed — Game Boy Advance

Current location:
Master Vault

Previous locations:
FireRed — Nintendo Switch
LeafGreen — Nintendo Switch
```

The app should not overwrite a Pokémon's history every time it moves.

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

a real cross-game transfer instead of treating both releases as the same save environment.

---

## Planned home screen

The home screen will be built around detected game sources.

The target list includes:

### Game Boy / Game Boy Color

- Pokémon Red
- Pokémon Blue
- Pokémon Yellow
- Pokémon Gold
- Pokémon Silver
- Pokémon Crystal

### Game Boy Advance

- Pokémon Ruby
- Pokémon Sapphire
- Pokémon Emerald
- Pokémon FireRed
- Pokémon LeafGreen

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

### Nintendo 3DS

- Pokémon X
- Pokémon Y
- Pokémon Omega Ruby
- Pokémon Alpha Sapphire
- Pokémon Sun
- Pokémon Moon
- Pokémon Ultra Sun
- Pokémon Ultra Moon

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

A detected title and a fully supported title are not the same thing. Support will be added and tested game-by-game.

---

## Pokémon view and action menu

Selecting a Pokémon should open a summary first, not immediately modify anything.

Target action menu:

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

The A button should never silently move, delete, clone, or rewrite a Pokémon.

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

A Vault record may track:

```text
original Pokémon bytes
current Pokémon bytes
record hash

original game
original platform
source save

current game / location
parent record
clone relationship

transfer history
transformation history
legality state
provenance
```

The goal is to keep enough history that a Pokémon can be traced even after it has been copied, edited, converted, or transferred.

---

## Banks

Alongside the Master Vault, PokeBank NX will support regular named banks for day-to-day organization.

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
- copy / move
- clone
- duplicate detection
- search
- filtering
- sorting
- favorites
- bulk organization

Copying should be the safe default. Moving should only happen when the user explicitly chooses it.

---

## Living Dex

The Vault will eventually drive collection tracking.

Planned views include:

- National Living Dex
- Shiny Living Dex
- regional forms
- permanent alternate forms
- meaningful gender differences
- owned / missing Pokémon
- generation completion
- completion percentages

The long-term goal is to make it obvious what is missing without maintaining a separate spreadsheet.

---

## Fill Master Bank — All + Shinies

A planned convenience tool will be able to populate a collection automatically.

The target is:

- every supported species
- one normal version
- one shiny version where legally possible
- regional forms
- meaningful permanent forms
- meaningful gender differences

The generator must not:

- create duplicate floods when run twice
- present shiny-locked Pokémon as legal shinies
- create impossible combinations and label them legal

Generation should be deterministic and legality-aware where the project has enough data to verify the result.

---

## Legality and provenance

PokeBank NX will separate two questions that are often mixed together:

1. **Where did this Pokémon come from?**
2. **Is this Pokémon valid for the game/encounter it claims to come from?**

The provenance view should show things such as:

- original game
- original platform
- source save identity
- current location
- parent/clone relationship
- edits and transformations
- transfer history
- legality result
- warnings
- checks that are unsupported or unknown

An unknown result should stay **unknown**. It should not be displayed as legal just because no error was found.

---

## Make Shiny

The planned **Make Shiny** action is not intended to be a simple shiny-bit toggle.

Where supported, it should account for:

- shiny locks
- event restrictions
- PID-related rules
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

A transfer should never be reported as complete if the destination save was not actually written and verified.

---

## Save safety

Any future live-write path should follow a process similar to:

```text
Read source save
      ↓
Create backup
      ↓
Create staged copy
      ↓
Apply change
      ↓
Repair checksums / structures
      ↓
Validate
      ↓
Write destination
      ↓
Verify result
      ↓
Keep recovery copy
```

Save safety matters more than convenience. A Pokémon collection can represent years of playtime.

---

## Startup and loading

The current visual direction for the app is the Route 1-style splash shown at the top of this README.

The planned startup sequence is:

```text
PokeBank NX splash
      ↓
Load settings
      ↓
Initialize services
      ↓
Load game data
      ↓
Open Vault / Banks
      ↓
Scan saves
      ↓
Home
```

Rather than a generic progress bar, the eventual loading animation may use the Pokémon shown along the bottom of the splash screen as progress milestones — each one lighting up as another initialization stage completes.

Startup progress should represent real work, not a fake timer.

---

## Roadmap

This roadmap tracks **PokeBank NX**, not the upstream PKSE project.

### 0. Repository and recovery

- [x] Create `GlitchedZeus/PokeBank-NX`
- [x] Preserve/import PKSE 1.1.3 source and history
- [x] Keep PKSE available as the upstream foundation
- [ ] Recover and merge the interrupted PokeBank NX development workspace
- [ ] Add `PROJECT_STATUS.md`
- [ ] Establish repeatable milestone commits and pushes
- [ ] Add build/version information to the app

### 1. PokeBank NX identity and startup

- [x] Finalize project name
- [x] Create project splash/header artwork
- [ ] Replace remaining PKSE branding in the app
- [ ] Add PokeBank NX icon / NRO metadata
- [ ] Add real startup/loading screen
- [ ] Add build version + abbreviated Git commit
- [ ] OLED Black theme
- [ ] Dark theme
- [ ] Light theme

### 2. Game identity and discovery

- [ ] Stable release/platform game IDs
- [ ] FireRed GBA
- [ ] LeafGreen GBA
- [ ] FireRed Switch
- [ ] LeafGreen Switch
- [ ] RetroArch save discovery
- [ ] Manual save import
- [ ] Clear platform labels on game cards

### 3. Game browser

- [ ] Home screen with detected games
- [ ] Game artwork/cards
- [ ] Party browser
- [ ] Box browser
- [ ] Box names / numbers
- [ ] Pokémon grid
- [ ] Controller shortcuts
- [ ] Handheld/docked layout pass

### 4. Master Vault and banks

- [ ] Immutable Master Vault
- [ ] Persistent Vault database
- [ ] Named banks
- [ ] Add to Master Vault
- [ ] Add to Bank
- [ ] Clone
- [ ] Duplicate handling
- [ ] Provenance chain
- [ ] Original/current-location tracking
- [ ] Backup/recovery tools

### 5. Retro save support

- [ ] Red / Blue / Yellow
- [ ] Gold / Silver / Crystal
- [ ] Ruby / Sapphire / Emerald
- [ ] FireRed / LeafGreen GBA
- [ ] Read-only import validation
- [ ] Regression tests for each parser

### 6. Pokémon tools

- [ ] Full summary screen
- [ ] Controller-friendly editor
- [ ] Legality viewer
- [ ] Provenance viewer
- [ ] Make Shiny
- [ ] Search
- [ ] Filter
- [ ] Sort
- [ ] Import / export Pokémon files

### 7. Nintendo Switch titles

- [ ] FireRed Switch adapter
- [ ] LeafGreen Switch adapter
- [ ] Let's Go Pikachu / Eevee integration
- [ ] Sword / Shield integration
- [ ] Brilliant Diamond / Shining Pearl integration
- [ ] Legends: Arceus integration
- [ ] Scarlet / Violet integration
- [ ] Legends: Z-A integration

The imported PKSE code already contains support for these Switch titles. This phase is about integrating that functionality into the PokeBank NX model, Vault, game identities, provenance, and safety rules rather than simply listing upstream support as finished PokeBank work.

### 8. DS / 3DS support

- [ ] Diamond / Pearl / Platinum
- [ ] HeartGold / SoulSilver
- [ ] Black / White
- [ ] Black 2 / White 2
- [ ] X / Y
- [ ] Omega Ruby / Alpha Sapphire
- [ ] Sun / Moon
- [ ] Ultra Sun / Ultra Moon

### 9. Living collection

- [ ] National Living Dex
- [ ] Shiny Living Dex
- [ ] form tracking
- [ ] missing Pokémon views
- [ ] completion statistics
- [ ] Fill Master Bank
- [ ] Fill Master Bank — All + Shinies

### 10. Transfers

- [ ] Compatibility matrix
- [ ] Destination conversion
- [ ] Cross-generation conversion
- [ ] Provenance-preserving transfer history
- [ ] FireRed GBA → FireRed Switch
- [ ] LeafGreen GBA → LeafGreen Switch
- [ ] staged destination writes
- [ ] validation
- [ ] rollback/recovery

### 11. Events and advanced collection data

- [ ] Mystery Gift support
- [ ] Wonder Cards
- [ ] Event Vault
- [ ] event metadata
- [ ] event legality
- [ ] ribbons / marks
- [ ] event collection tracking

### 12. Release hardening

- [ ] crash recovery
- [ ] corruption detection
- [ ] low-space handling
- [ ] SD-card failure handling
- [ ] large-Vault stress testing
- [ ] performance pass
- [ ] complete regression suite
- [ ] physical Switch validation
- [ ] release packaging
- [ ] user documentation
- [ ] stable v1.0

---

## Build

PokeBank NX currently inherits the PKSE Switch build system.

### Requirements

- devkitPro
- devkitA64 / libnx
- Switch development packages
- Python 3 for asset/data generation tools
- Pillow if regenerating HD sprites

Set `DEVKITPRO` appropriately for your system.

### Fetch HD Pokémon sprites

The repository contains a helper script for generating the ROMFS sprite set:

```bash
python tools/gen_hdsprites.py
```

### Build the `.nro`

```bash
make clean && make all
```

If generated/downloaded assets are already present:

```bash
make clean && make
```

The resulting native Switch build should be treated separately from host tests:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

A successful `.nro` build is not the same thing as successful testing on physical hardware.

---

## Development workflow

GitHub is intended to be the permanent project state.

A normal development milestone should look like:

```text
implement
   ↓
test
   ↓
build .nro
   ↓
update PROJECT_STATUS.md
   ↓
commit
   ↓
push
```

Important work should not be left only inside a temporary coding-agent workspace.

The original PKSE repository should remain an **upstream source**, not the destination for PokeBank NX changes.

---

## Contributing

PokeBank NX is still changing quickly, so large pull requests should be discussed before replacing major architecture.

The priorities are:

1. do not lose user data
2. preserve provenance
3. keep the Switch UI simple
4. avoid regressions
5. test save-format changes
6. keep the build reproducible

When `PROJECT_STATUS.md` is added, contributors should read it before starting work.

---

## Upstream and credits

PokeBank NX is derived from **PKSE — Pokémon Save Editor** and intentionally keeps its upstream Git history.

The project also relies on work from the wider Pokémon and Switch homebrew communities, including:

- [PKSE](https://github.com/kiasta/PKSE)
- [PKHeX](https://github.com/kwsch/PKHeX)
- [PokeAPI sprites](https://github.com/PokeAPI/sprites)
- [devkitPro](https://devkitpro.org/)
- [libnx](https://github.com/switchbrew/libnx)

Please keep applicable upstream notices and licenses intact when redistributing modified builds or source.

---

## License

The codebase is licensed under the **GNU Affero General Public License v3.0** as inherited from PKSE.

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

The immediate goal is not to implement every generation at once.

The first usable PokeBank NX milestone is:

```text
Boot PokeBank NX
      ↓
Detect supported games
      ↓
Browse party / boxes
      ↓
Open Pokémon summary
      ↓
Add to Master Vault / Bank
      ↓
Persist safely
```

Once that works reliably on a physical Switch, the rest of the roadmap becomes expansion of a functioning app rather than construction of a prototype.ails.

