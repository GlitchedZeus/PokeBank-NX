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
- Local recovery commit: `afeb5da`
- Recovery branch: `recovery/interrupted-2026-09-01`

## Verification states

| Area | State | Evidence |
|---|---|---|
| Stable game identity registry | HOST TESTED | 23 unique IDs; exact Switch title-ID lookup; GBA/Switch FireRed and LeafGreen separation |
| Platform-aware native game cards | NRO BUILDS | Native Switch build links with platform labels and embedded short commit |
| Native `.nro` | NRO BUILDS | `PokeVaultNX.nro`, 9,703,861 bytes, SHA-256 `a255717659d666a1b60e74c5eaf36d20dfb6a28ed595a5677eae32b8aa624ca8` |
| Physical Switch execution | NOT DEVICE TESTED | No PokeBank NX build from this recovery has been run on hardware |
| Master Vault / immutable objects | NOT RECOVERED | Earlier implementation was not present in refs, reflogs, stashes, unreachable commits, workspace archives, or GitHub |
| RetroArch discovery / Gen 1-3 adapters | NOT RECOVERED | Earlier implementation was not present in recoverable project data |

Do not promote an item from `IMPLEMENTED` to `HOST TESTED`, `NRO BUILDS`, or `DEVICE TESTED` without performing that level of verification.

## Baseline test status

On 2026-09-01:

- `make -f Makefile.host host-test`: PASS
- `make -f Makefile.host host-sanitize`: PASS (AddressSanitizer and UndefinedBehaviorSanitizer; leak detection disabled only because the managed sandbox cannot inspect `/proc`)
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

## Critical safety blocker

The recovered PKSE foundation can still offer a live game-save destination. That violates the current PokeBank NX phase rule.

**Do not install this recovery build for use against valuable saves.** The next implementation milestone must hard-disable all live save writes at the policy and UI layers while retaining backup/staged-copy output. Direct game-save writing remains prohibited until the full backup, verification, rollback, and physical-device test pipeline is complete.

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

## Recovery record

- Requested historical checkpoint `1932cf0` is absent from this clone and from every available reflog/ref/unreachable commit.
- No stash entries were present.
- The only unreachable objects were a discarded README merge-conflict tree/blob, not source history.
- The interrupted 23-game identity milestone was recovered from the worktree, committed locally as `afeb5da`, and published without overwriting newer README edits as GitHub commit `3101fc0`.
- Recovery artifacts:
  - `/mnt/data/PokeBank-NX-RECOVERY.bundle`
  - `/mnt/data/PokeBank-NX-WORKTREE-RECOVERY.tar.gz`

## Current task

Implement and test a centralized read-only write policy that removes every live game-save destination from the UI and rejects any attempted live injection defensively.

## Next priorities

1. Hard-lock live game-save writes and add host regression tests.
2. Rebuild the playable A-button Pokémon action sheet without one-press mutations.
3. Reconstruct immutable Master Vault insertion and named bank storage.
4. Reconstruct RetroArch discovery and read-only Gen 1-3 adapters.
5. Build the professional Pokémon summary and provenance views.

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
