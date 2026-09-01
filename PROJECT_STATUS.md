# PokeBank NX Project Status

Last updated: 2026-09-01

## Project identity

- Product: PokeBank NX (the recovered build still uses the temporary `PokeVault NX` app/target name)
- Version: `0.1.0-alpha`
- Repository: `GlitchedZeus/PokeBank-NX`
- Writable remote: `origin`
- Upstream-only remote: `upstream` (`kiasta/PKSE`)
- Current development branch: `feature/pokebank-playable`
- Latest verified GitHub recovery commit: `3101fc0`
- Latest verified GitHub status commit: `fabff21`
- Latest exact-source safety build commit: `65aa52c`
- Local recovery commit: `afeb5da`
- Recovery branch: `recovery/interrupted-2026-09-01`

## Verification states

| Area | State | Evidence |
|---|---|---|
| Stable game identity registry | HOST TESTED | 23 unique IDs; exact Switch title-ID lookup; GBA/Switch FireRed and LeafGreen separation |
| Platform-aware native game cards | NRO BUILDS | Native Switch build links with platform labels and embedded short commit |
| Read-only live-save policy | HOST TESTED / NRO BUILDS | Backup-only destinations, generic save API has no injection parameter, filesystem restore rejects live writes |
| Native `.nro` | NRO BUILDS | Safety build from `65aa52c`: 9,695,669 bytes, SHA-256 `0cf50b659ed5c648009e10d51a75a398bc2a3e69e4cbeb99cc0b74b9643ece07` |
| Physical Switch execution | NOT DEVICE TESTED | No PokeBank NX build from this recovery has been run on hardware |
| Master Vault / immutable objects | NOT RECOVERED | Earlier implementation was not present in refs, reflogs, stashes, unreachable commits, workspace archives, or GitHub |
| RetroArch discovery / Gen 1-3 adapters | NOT RECOVERED | Earlier implementation was not present in recoverable project data |

Do not promote an item from `IMPLEMENTED` to `HOST TESTED`, `NRO BUILDS`, or `DEVICE TESTED` without performing that level of verification.

## Baseline test status

On 2026-09-01:

- `make -f Makefile.host host-test`: PASS (game identity and write-policy suites)
- `make -f Makefile.host host-sanitize`: PASS (both suites under AddressSanitizer and UndefinedBehaviorSanitizer; leak detection disabled only because the managed sandbox cannot inspect `/proc`)
- native `make -j1`: PASS
- `git diff --check`: PASS at the recovered milestone

## Working features

### Recovered PokeBank NX work

- Stable internal identity uses release + platform, not display name.
- Registry contains 23 target game identities.
- These four identities are distinct and regression-tested:
  - `firered_gba`
  - `leafgreen_gba`
  - `firered_switch`
  - `leafgreen_switch`
- Native Switch title cards carry a stable game ID and display `Nintendo Switch` as a separate platform line.
- App header exposes version and abbreviated Git commit.
- Host test and sanitizer targets exist independently of the devkitPro build.
- Installed game saves are exposed as read-only sources in the save UI.
- Save destinations are limited to the session's working backup and a new named backup.
- The generic save API cannot request title injection.
- The final filesystem restore entry point rejects live-write attempts before mounting save data.
- A legacy `injectToGame=1` setting is ignored and rewritten as zero.

### PKSE upstream foundation present in this tree

- Native Switch application framework and controller UI.
- Switch save selection, backup, restore, party/box browsing, Pokémon editing and creation.
- Existing cross-game bank and conversion framework.
- Existing legality-related UI/data.
- Existing save handlers for the seven Switch game families represented by PKSE 1.1.3.

These upstream capabilities are a foundation, not proof that the PokeBank NX safety contract has been met.

## Partially working or planned

- Professional Home screen: native installed-title cards work; legacy/RetroArch cards are not implemented in the recovered tree.
- Game to party/boxes: upstream flow exists for supported native titles; PokeBank platform/source integration is incomplete.
- Pokémon action sheet: required A-button behavior is not yet implemented.
- Professional Pokémon summary screen: upstream details UI exists, but the required provenance-focused summary is incomplete.
- Master Vault, named multi-bank storage, clone lineage and durable provenance must be reconstructed.
- Living Dex, Shiny Living Dex, collection generator, themes and startup polish remain planned.
- Switch FireRed/LeafGreen identities and upstream handlers exist; their outer save-container behavior still requires safe, source-specific validation.

## Safety posture

Direct game-save writing is hard-locked in this alpha. It has no visible destination or mutable setting, the generic save API always writes backup data only, and the low-level title restore path rejects calls before mounting save data.

This is `HOST TESTED / NRO BUILDS`, not `DEVICE TESTED`. Continue using copied/non-valuable saves for the first hardware tests. Live writes must not be enabled until the full backup, verification, rollback, and physical-device test pipeline is complete.

## Supported identities and save status

| Platform | Identities | Current PokeBank status |
|---|---|---|
| Game Boy | Red, Blue, Yellow | Identity only; parser not recovered |
| Game Boy Color | Gold, Silver, Crystal | Identity only; parser not recovered |
| Game Boy Advance | Ruby, Sapphire, Emerald, FireRed, LeafGreen | Identity only; parser not recovered |
| Nintendo Switch | FireRed, LeafGreen, Let's Go Pikachu/Eevee, Sword/Shield, Brilliant Diamond/Shining Pearl, Legends Arceus, Scarlet/Violet, Legends Z-A | Identity mapping builds; existing PKSE handlers remain present |

## Current architecture

- `include/Games/GameIdentity.h` / `src/Games/GameIdentity.cpp`: canonical game catalog and exact platform-aware lookup.
- `SaveSelectScreen`: enumerates supported installed titles, assigns canonical IDs, and renders title/platform separately.
- Existing PKSE `Save`, `Trainer`, `Pokemon`, `Conversion`, `Legality`, and UI layers remain intact.
- `Makefile.host`: isolated portable tests for PokeBank-specific pure C++ components.
- Native devkitPro build remains the integration/link check.

## Upstream / Reference Projects

These projects are tracked so future development sessions can reuse mature Pokémon research, avoid unnecessary reimplementation, and keep licensing/reuse decisions explicit.

### PKSE

- Repository: https://github.com/kiasta/PKSE
- Role: original PokeBank NX foundation; native Switch application, UI, save handling, Pokémon editing, conversion framework, and legality-related foundation.
- Policy: **UPSTREAM ONLY.** Never push PokeBank NX changes to PKSE.

### PKSM-Core

- Repository: https://github.com/FlagBrew/PKSM-Core
- Role: native C++ Pokémon/save-editing engine with Gen I-VIII support and substantial Pokémon-format logic translated from PKHeX research.
- Priority: **VERY HIGH**.
- Potential use: Pokémon structure parsing, retro save support, conversion helpers, generation-specific behavior, and avoiding unnecessary rewrites of mature C++ code.
- Action: audit for direct integration or adaptation before implementing Gen I-VIII infrastructure from scratch.

### PKHeX

- Repository: https://github.com/kwsch/PKHeX
- Role: primary technical correctness/reference implementation.
- Potential use: Pokémon formats, save structures, encounters, forms, moves, locations, legality, conversion, Mystery Gifts, Pokédex data, and golden test vectors.
- Planned use: host-side **PKHeX Oracle** developer tool for comparison testing and data generation. The Switch application remains native C++.

### Auto Legality Mod / PKHeX-Plugins

- Repository: https://github.com/santacrab2/PKHeX-Plugins
- Role: encounter-driven legal Pokémon generation/legalization reference.
- Potential use: encounter selection, shiny constraints, trainer/encounter consistency, generated Pokémon workflows, legality test vectors, and future Fill Master Vault features.
- Planned use: pair a pinned compatible AutoMod revision with the PKHeX Oracle.

### pkHouse

- Repository: https://github.com/Insektaure/pkHouse
- Role: modern Nintendo Switch Pokémon-bank/save implementation reference.
- High-value areas: LGPE, Sword/Shield, BDSP, Legends Arceus, Scarlet/Violet, Legends Z-A, Switch FireRed/LeafGreen, SCBlock handling, backup behavior, Pokédex registration, handling-trainer updates, Wondercards, and save verification.
- Author guidance: Insektaure explicitly encouraged PokeBank NX to use pkHouse as a reference and recommended reimplementing needed behavior instead of pure copy/paste. The author also offered to answer technical questions about how parts of pkHouse work.
- Policy: **REFERENCE ONLY.** Do not copy source verbatim. Reimplement needed behavior inside PokeBank NX architecture and cross-check important format facts against PKHeX, PKSM-Core, game data, and tests where practical.

### pkDex

- Repository: https://github.com/Insektaure/pkDex
- Role: Pokédex UX/data/reference for Nintendo Switch.
- Useful ideas: National/regional Dex organization, regular/shiny tracking, Alpha/Shiny Alpha tracking where applicable, evolution information, locations, version exclusives, shiny-lock display, region/DLC organization, controller navigation, bulk actions, and multi-select.
- Policy: **REFERENCE ONLY.** PokeBank NX builds its own Pokédex.
- Architecture rule: the **Master Vault is authoritative for Pokémon ownership**. Pokédex owned/shiny/form/event status should be derived automatically from Vault data rather than maintained as an independent manual tracker.

### PKForge

- Repository: https://github.com/sofianeelhor/PKForge
- Role: architecture/reference for safe Pokémon bank and save-management design.
- Useful ideas: immutable raw Pokémon records, stable IDs, SHA-256 hashes, provenance sidecars, versioned schemas, backups, atomic save writes, engine abstraction, and PKHeX + AutoMod integration.
- Policy: adapt useful architecture concepts to PokeBank NX's native Switch/C++ requirements rather than copying the Android application architecture wholesale.

### Reuse rule

Before implementing major Pokémon-format, legality, conversion, save-parsing, Pokédex, banking, or generated-Pokémon functionality:

1. Check the tracked references first.
2. Determine what PokeBank NX already implements.
3. Identify the relevant upstream/reference implementation and exact files/commit where practical.
4. Record language and license.
5. Classify the approach as `DIRECT REUSE`, `PORT`, or `REFERENCE ONLY`.
6. Prefer mature, tested behavior over unnecessary reinvention.
7. Add regression/golden tests before replacing or integrating existing behavior.
8. Preserve required attribution and license notices whenever code is directly reused.

## Recovery record

- Requested historical checkpoint `1932cf0` is absent from this clone and from every available reflog/ref/unreachable commit.
- No stash entries were present.
- The only unreachable objects were a discarded README merge-conflict tree/blob, not source history.
- The interrupted 23-game identity milestone was recovered from the worktree, committed locally as `afeb5da`, and published without overwriting newer README edits as GitHub commit `3101fc0`.
- Recovery artifacts:
  - `/mnt/data/PokeBank-NX-RECOVERY.bundle`
  - `/mnt/data/PokeBank-NX-WORKTREE-RECOVERY.tar.gz`

## Current task

Build the controller-first Pokémon action sheet so pressing A on a focused Pokémon opens a deliberate menu and never performs an immediate mutation.

## Next priorities

1. Rebuild the playable A-button Pokémon action sheet without one-press mutations.
2. Reconstruct immutable Master Vault insertion and named bank storage.
3. Reconstruct RetroArch discovery and read-only Gen 1-3 adapters.
4. Build the professional Pokémon summary and provenance views.
5. Expand read-only safety tests around staged-save integration.

## Known blockers

- Shell Git authentication is unavailable in this runtime. Authenticated GitHub API integration is working and was used to publish the recovery checkpoint.
- No physical Switch test has been performed.
- The previous custom vault/RetroArch source was not recoverable and must be rebuilt from specifications and tests.

## Exact build commands

Host tests:

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
```

Native Switch build in the current workspace:

```bash
export DEVKITPRO="$PWD/build-deps/devkitpro-root/opt/devkitpro"
export DEVKITA64="$DEVKITPRO/devkitA64"
export PORTLIBS="$DEVKITPRO/portlibs/switch"
export PATH="$PWD/build-deps/pkgconf-install/bin:$DEVKITPRO/devkitA64/bin:$DEVKITPRO/tools/bin:$DEVKITPRO/portlibs/switch/bin:$PATH"
export LD_LIBRARY_PATH="$PWD/build-deps/pkgconf-install/lib"
make -j1
```

Build dependencies under `build-deps/` are local/ignored and are not committed.
