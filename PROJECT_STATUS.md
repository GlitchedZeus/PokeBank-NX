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
- Recovered baseline/status commit: `fabff21`
- Latest verified remote **source safety milestone**: `c618bd5`
- Recovery-era exact `.nro` build source was recorded locally as short SHA `65aa52c`; that SHA is not present as a remote GitHub commit. See `docs/BUILD_RECORD.md`.
- Local recovery commit: `afeb5da`
- Recovery branch: `recovery/interrupted-2026-09-01`

The commits after `c618bd5` currently consist of project documentation/research planning and GitHub host-test CI configuration; they do not represent a newer device-tested application milestone.

## Verification states

| Area | State | Evidence |
|---|---|---|
| Stable game identity registry | HOST TESTED | 23 unique IDs; exact Switch title-ID lookup; GBA/Switch FireRed and LeafGreen separation |
| Platform-aware native game cards | NRO BUILDS | Native Switch build links with platform labels and embedded short commit |
| Read-only live-save policy | HOST TESTED / NRO BUILDS | Backup-only destinations, generic save API has no injection parameter, filesystem restore rejects live writes |
| Host CI | HOST TESTED | `.github/workflows/host-tests.yml` passed on commit `49adc417` on `feature/pokebank-playable`; host tests + ASan/UBSan + whitespace check |
| Native `.nro` | NRO BUILDS | Recovery runtime produced a 9,695,669-byte safety build, SHA-256 `0cf50b659ed5c648009e10d51a75a398bc2a3e69e4cbeb99cc0b74b9643ece07`; rebuild a fresh artifact from verified GitHub source before hardware testing |
| Physical Switch execution | NOT DEVICE TESTED | No PokeBank NX build from this recovery has been run on hardware |
| Master Vault / immutable objects | NOT RECOVERED | Earlier implementation was not present in refs, reflogs, stashes, unreachable commits, workspace archives, or GitHub; v1 is now formally specified in `docs/MASTER_VAULT_SPEC.md` |
| RetroArch discovery / Gen 1-3 adapters | NOT RECOVERED | Earlier implementation was not present in recoverable project data; PKSM-Core integration/rebuild plan is now documented |
| PKSM-Core integration | PLANNED / AUDITED | Pinned GPLv3 revision and concrete PK3/Sav3 spike documented in `docs/PKSM_CORE_INTEGRATION.md` |
| Modern Switch reference audit | PLANNED / AUDITED | pkHouse pinned as reference-only with concrete FRLG/ZA/SV/SwSh/PLA/BDSP/LGPE notes in `docs/PKHOUSE_REFERENCE.md` |
| PKHeX Oracle | SPECIFIED | Host-side correctness/golden-vector tool specified in `docs/PKHEX_ORACLE.md` |
| Vault-driven Pokédex | SPECIFIED | Collection model specified in `docs/POKEDEX_SPEC.md`; Master Vault will be authoritative for ownership |

Do not promote an item from `IMPLEMENTED` to `HOST TESTED`, `NRO BUILDS`, or `DEVICE TESTED` without performing that level of verification.

## Baseline test status

Recovery session, 2026-09-01:

- `make -f Makefile.host host-test`: PASS (game identity and write-policy suites)
- `make -f Makefile.host host-sanitize`: PASS (AddressSanitizer and UndefinedBehaviorSanitizer; leak detection disabled in the managed recovery sandbox because it could not inspect `/proc`)
- native `make -j1`: PASS
- `git diff --check`: PASS at the recovered milestone

GitHub CI added later on 2026-09-01:

- workflow: `.github/workflows/host-tests.yml`
- tested commit: `49adc41713725aae5e2a6ab786bec2d5cf9a65f0`
- branch run: `feature/pokebank-playable`
- result: **SUCCESS**
- checks: whitespace, host tests, ASan/UBSan

The GitHub workflow intentionally does not claim to replace a native devkitPro `.nro` build or physical Switch test.

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
- GitHub Actions now automatically runs the portable host safety/identity tests on pushes to `main`, `feature/pokebank-playable`, and pull requests.

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
- Master Vault, named multi-bank storage, clone lineage and durable provenance must be reconstructed using the new written v1 contract.
- Living Dex, Shiny Living Dex, collection generator, themes and startup polish remain planned.
- Switch FireRed/LeafGreen identities and upstream handlers exist; their outer save-container behavior still requires safe, source-specific validation.
- Modern Switch save-family behavior needs comparison against PKHeX and the pkHouse reference before PokeBank-specific staged-write support is attempted.

## Safety posture

Direct game-save writing is hard-locked in this alpha. It has no visible destination or mutable setting, the generic save API always writes backup data only, and the low-level title restore path rejects calls before mounting save data.

This is `HOST TESTED / NRO BUILDS`, not `DEVICE TESTED`. Continue using copied/non-valuable saves for the first hardware tests. Live writes must not be enabled until the per-adapter backup, staging, validation, readback verification, rollback, and physical-device gates in `docs/SAVE_SAFETY.md` are met.

## Supported identities and save status

| Platform | Identities | Current PokeBank status |
|---|---|---|
| Game Boy | Red, Blue, Yellow | Identity only; parser not recovered; audit PKSM-Core before rebuilding |
| Game Boy Color | Gold, Silver, Crystal | Identity only; parser not recovered; audit PKSM-Core before rebuilding |
| Game Boy Advance | Ruby, Sapphire, Emerald, FireRed, LeafGreen | Identity only; parser not recovered; PK3/Sav3 integration spike specified |
| Nintendo Switch | FireRed, LeafGreen, Let's Go Pikachu/Eevee, Sword/Shield, Brilliant Diamond/Shining Pearl, Legends Arceus, Scarlet/Violet, Legends Z-A | Identity mapping builds; existing PKSE handlers remain present; modern source-specific validation is planned |

## Current architecture

- `include/Games/GameIdentity.h` / `src/Games/GameIdentity.cpp`: canonical game catalog and exact platform-aware lookup.
- `SaveSelectScreen`: enumerates supported installed titles, assigns canonical IDs, and renders title/platform separately.
- Existing PKSE `Save`, `Trainer`, `Pokemon`, `Conversion`, `Legality`, and UI layers remain intact.
- `Makefile.host`: isolated portable tests for PokeBank-specific pure C++ components.
- `.github/workflows/host-tests.yml`: automatic portable host test/sanitizer gate.
- Native devkitPro build remains the integration/link check.

## Project documentation map

Start with `docs/PROJECT_MAP.md`.

Key specifications/research files:

- `docs/SESSION_RUNBOOK.md` — recovery-complete coding-agent workflow
- `docs/UPSTREAM_AUDIT.md` — pinned external projects, licenses, files, reuse classifications
- `docs/PKSM_CORE_INTEGRATION.md` — concrete native C++ PK3/Sav3 integration spike
- `docs/PKHOUSE_REFERENCE.md` — concrete modern Switch behavior reference notes
- `docs/MASTER_VAULT_SPEC.md` — immutable Vault/provenance/bank/transaction contract
- `docs/SAVE_SAFETY.md` — future live-write gate
- `docs/TRANSFER_MODEL.md` — copy/move/clone/conversion/HOME Bridge semantics
- `docs/POKEDEX_SPEC.md` — Vault-driven collection model
- `docs/PKHEX_ORACLE.md` — host-side correctness tool design
- `docs/DEVICE_TEST_CHECKLIST.md` — exact physical Switch test plan
- `docs/BUILD_RECORD.md` — binary/source SHA bookkeeping and artifact policy

## Upstream / Reference Projects

These projects are tracked so future development sessions can reuse mature Pokémon research, avoid unnecessary reimplementation, and keep licensing/reuse decisions explicit. Detailed pinned revisions and file-level notes are in `docs/UPSTREAM_AUDIT.md`.

### PKSE

- Repository: https://github.com/kiasta/PKSE
- Role: original PokeBank NX foundation.
- Policy: **UPSTREAM ONLY.** Never push PokeBank NX changes to PKSE.

### PKSM-Core

- Repository: https://github.com/FlagBrew/PKSM-Core
- Role: native C++ Pokémon/save engine and highest-priority historical format integration candidate.
- Policy: audit/integrate deliberately; start with PK3 + Sav3 before hand-writing Gen III infrastructure.

### PKHeX

- Repository: https://github.com/kwsch/PKHeX
- Role: primary technical correctness/reference implementation and generated-data source.
- Planned use: host-side **PKHeX Oracle** for comparison tests and data generation.

### Auto Legality Mod / PKHeX-Plugins

- Repository: https://github.com/santacrab2/PKHeX-Plugins
- Role: encounter-driven generation/legalization reference for the host-side Oracle and future generated collections.

### pkHouse

- Repository: https://github.com/Insektaure/pkHouse
- Role: modern Nintendo Switch save/bank reference.
- Author guidance: Insektaure encouraged reference/reimplementation and offered to answer focused technical questions.
- Policy: **REFERENCE ONLY.** Do not copy source verbatim into PokeBank NX.

### pkDex

- Repository: https://github.com/Insektaure/pkDex
- Role: Pokédex UX/data organization reference.
- Policy: **REFERENCE ONLY.** PokeBank NX builds its own Vault-driven Pokédex.

### PKForge

- Repository: https://github.com/sofianeelhor/PKForge
- Role: architecture/reference for immutable Pokémon records, provenance, logical banks, backups, and staged/atomic save design.

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
- Recovery artifacts created in the recovery runtime:
  - `/mnt/data/PokeBank-NX-RECOVERY.bundle`
  - `/mnt/data/PokeBank-NX-WORKTREE-RECOVERY.tar.gz`

## Current task

**GitHub issue #2:** build the controller-first Pokémon action sheet so pressing A on a focused Pokémon opens a deliberate menu and never performs an immediate mutation.

## Next priorities

1. Complete issue #2 — playable A-button Pokémon action sheet without one-press mutations.
2. Complete issue #4 — PKSM-Core Gen III (`PK3` + `Sav3`) integration spike before rebuilding historical infrastructure.
3. Produce a fresh `.nro`, preserve the binary, and perform the first physical Switch read-only test using issue #8 / `docs/DEVICE_TEST_CHECKLIST.md` as soon as the next coherent playable build is ready.
4. Complete issue #3 — immutable Master Vault v1 and named Bank storage.
5. Complete issue #9 — professional Pokémon summary and provenance view.
6. Rebuild read-only legacy discovery/adapters (issue #6) and audit modern Switch adapters (issue #11).
7. Build the PKHeX Oracle (issue #5) and golden comparison corpus.
8. Build the Vault-driven Pokédex/Living Dex v1 (issue #7).
9. Build conversion/transfer without live writes (issue #10).
10. Keep live installed-game writes disabled until `docs/SAVE_SAFETY.md` gates are met for an explicitly named adapter.

## Known blockers / cautions

- The recovery coding runtime did not have shell Git authentication; the authenticated GitHub connector worked. Re-check credentials in each new coding environment rather than assuming either state persists.
- No physical Switch test has been performed yet.
- The previous custom Vault/RetroArch source was not recoverable; the replacement architecture is now specified but still must be implemented.
- The recovery `.nro` binary itself was not committed/published; create a fresh artifact before device testing.
- pkHouse and pkDex are GPLv2 reference projects; PokeBank NX policy is independent reimplementation rather than verbatim source copying.

## Exact build commands

Host tests:

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
```

Native Switch build in the recovery workspace used:

```bash
export DEVKITPRO="$PWD/build-deps/devkitpro-root/opt/devkitpro"
export DEVKITA64="$DEVKITPRO/devkitA64"
export PORTLIBS="$DEVKITPRO/portlibs/switch"
export PATH="$PWD/build-deps/pkgconf-install/bin:$DEVKITPRO/devkitA64/bin:$DEVKITPRO/tools/bin:$DEVKITPRO/portlibs/switch/bin:$PATH"
export LD_LIBRARY_PATH="$PWD/build-deps/pkgconf-install/lib"
make -j1
```

Build dependencies under `build-deps/` were local/ignored and are not committed.
