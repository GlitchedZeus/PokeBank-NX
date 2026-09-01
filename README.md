<p align="center">
<img width="1672" height="941" alt="THE FINAL" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is an offline Pokémon storage, collection, and save-management project for Nintendo Switch homebrew.

The project started from the PKSE codebase, but the long-term goal is broader: one controller-first Switch app for browsing Pokémon saves, keeping a permanent local Master Vault, organizing Pokémon into named Banks, tracking provenance, building Living Dex collections, and safely moving compatible Pokémon between games and generations.

PokeBank NX is under active alpha development. **Do not treat development builds as production-safe save-writing tools. Direct live installed-game save writing is intentionally hard disabled.**

---

## Current status — September 1, 2026

Recovery is complete and the project has moved back into normal feature development.

Development branch:

```text
feature/pokebank-playable
```

PKSE remains:

```text
upstream only
```

PokeBank NX changes must never be pushed to the original PKSE repository.

### Latest verified application milestone

The controller-first Pokémon Action Sheet is implemented, host-tested, sanitizer-tested, and builds as a native Switch `.nro`.

Application source:

```text
82a0779a5143cca0690d0c7068946d84ebe9f107
ui: add controller Pokemon action sheet
```

The Action Sheet is shared across Party, Boxes, and Storage and contains:

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

Opening the menu, moving focus, pressing B, or selecting Cancel performs zero mutation. `View Pokémon` is explicitly read-only. Unfinished actions return a safe `Not yet supported` result rather than falling through into another action.

GitHub issue **#2 is completed and closed**.

### Current device-test build

```text
Filename:
PokeBank-NX-ActionSheet-82a0779.nro

Source:
82a0779a5143cca0690d0c7068946d84ebe9f107

Size:
9,695,669 bytes

SHA-256:
6ff0f71c2e8f6d7fcf948a4bbc0037ba799e22bbaac433263be7cd0afac3b72b

Status:
NRO BUILDS
NOT DEVICE TESTED
```

The physical Switch test is tracked in issue **#8**. The current plan is to finish the interrupted HOME-style control/theme milestone first and then test the newer combined build so one hardware pass covers both the Action Sheet and UI/control work.

### Interrupted Session 2 warning

Issue **#13 — HOME-style controls and OLED/Dark/Light UI shell** is still open.

A coding session reported substantial **local-only** progress before usage ended, including semantic themes, Select Game focus/card styling, held navigation repeat, contextual `+`, read-only `-` Help, persisted theme cycling, bottom button hints, and reusable modal/card helpers.

However, that session ended **before** the new code was fully regression-tested, native-built, committed, pushed, or turned into a new `.nro`.

Therefore:

```text
Session 2 UI/theme work = REPORTED LOCAL WORK ONLY
GitHub implementation status = NOT YET VERIFIED / NOT YET PUSHED
```

The next coding session must inspect the previous workspace/local branches/reflog **before resetting or cleaning anything** and recover that local Session 2 work if it still exists.

For the detailed source-of-truth status, see [`PROJECT_STATUS.md`](PROJECT_STATUS.md).

---

## Verification snapshot

| Area | State |
|---|---|
| Repository recovery | **COMPLETE** |
| 23 stable release/platform game identities | **HOST TESTED** |
| GBA/Switch FireRed + LeafGreen identity separation | **HOST TESTED** |
| Platform-aware native game cards | **NRO BUILDS** |
| Live installed-game save writes | **HARD DISABLED** |
| Pokémon Action Sheet | **HOST TESTED / NRO BUILDS** |
| Session 2 HOME-style controls/theme implementation | **INTERRUPTED LOCAL-ONLY WORK** |
| OLED Black / Dark / Light design contract | **SPECIFIED** |
| Physical Switch validation | **NOT DEVICE TESTED** |
| Master Vault v1 | **SPECIFIED / NOT IMPLEMENTED** |
| PKSM-Core Gen III integration | **AUDITED / NEXT DEEP ENGINEERING SPIKE** |
| RetroArch Gen I-III discovery/adapters | **REBUILD REQUIRED** |
| Modern Switch adapter validation | **PLANNED / AUDITED** |
| PKHeX Oracle | **SPECIFIED** |
| Vault-driven Pokédex | **SPECIFIED** |

Verification vocabulary is deliberately strict:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

A successful `.nro` build is not physical hardware validation.

---

## What already works

### PokeBank NX-specific work

- stable game identity based on **release + platform**, not display name;
- 23 target game identities;
- separate identities for:
  - `firered_gba`
  - `leafgreen_gba`
  - `firered_switch`
  - `leafgreen_switch`;
- platform-aware native Switch title cards;
- app version + abbreviated Git commit display;
- host regression test target independent of devkitPro;
- ASan / UBSan host target;
- GitHub Actions host-test/sanitizer workflow;
- read-only installed-game source handling;
- backup-only destinations during the current alpha;
- generic save API cannot request title injection;
- low-level filesystem guard rejects live-title restore attempts before mounting save data;
- legacy `injectToGame=1` is ignored/rewritten disabled;
- shared controller-first Pokémon Action Sheet across Party, Boxes, and Storage;
- safe read-only View action;
- explicit safe handling for unfinished Action Sheet actions.

### PKSE foundation still present

The imported PKSE 1.1.3 source/history provides a substantial native Switch foundation:

- Nintendo Switch `.nro` build system;
- controller-driven UI;
- save selection and backup infrastructure;
- party and box browsing;
- Pokémon editing/creation infrastructure;
- trainer/item editing infrastructure;
- legality-related UI/data;
- bank/conversion framework;
- Switch save access;
- generated Pokémon/game data tables;
- existing handlers for the Switch game families represented by PKSE 1.1.3.

Those inherited capabilities are not automatically considered finished PokeBank NX features. They must be integrated into the PokeBank NX identity, Vault, provenance, and safety model and verified independently.

---

## UI direction

PokeBank NX should feel like a polished Switch application rather than a desktop save editor squeezed onto a console.

The visual contract is documented in [`docs/UI_STYLE_GUIDE.md`](docs/UI_STYLE_GUIDE.md).

The supplied reference concepts establish these major patterns:

### Select Game

```text
large cover-art cards
strong focus border
game title
platform
source/status
```

### Pokémon browsing

```text
Party / Boxes
Pokémon grid
selected Pokémon preview
controller hint bar
A -> deliberate Action Sheet
```

### Pokémon Summary

```text
large selected Pokémon render
grouped information cards
Overview / Stats / Moves / Origin / Legality + Provenance
compatible-games view
```

### Pokédex

```text
National / Regional / Living / Shiny / Missing
sprite grid
large selected-species preview/details pane
Vault-driven ownership state
```

The references are inspiration only; PokeBank NX should remain visually distinct.

---

## Themes

Required themes:

```text
OLED Black
Dark
Light
```

Screens should use semantic theme roles rather than hard-coded dark/light branches, for example:

```text
background
surface
surfaceRaised
surfaceSelected
textPrimary
textSecondary
textMuted
accentPrimary
accentSecondary
focusBorder
divider
success
warning
error
info
```

The design requirement exists on GitHub today. The interrupted Session 2 reportedly implemented much of the first theme foundation locally, but those source changes are **not yet claimed as pushed/verified** until the continuation session recovers and finishes them.

---

## Controller contract

The controller model is intentionally similar to Pokémon HOME because that interaction pattern fits a controller-first Pokémon storage app well.

Full contract: [`docs/CONTROLS.md`](docs/CONTROLS.md).

Target conventions:

| Control | PokeBank NX behavior |
|---|---|
| D-pad | precise menu/grid navigation |
| Left Stick | navigation + held scrolling |
| A | Select / Open; on Pokémon opens Action Sheet |
| B | Back / Cancel |
| X | Filter / Search / contextual tool |
| Y | Sort / View / secondary contextual action |
| L / R | previous/next box, Pokémon, or nearby tab |
| ZL / ZR | larger jumps / major navigation |
| `+` | contextual More / Options |
| `-` | Help / Controls / Screen Info |
| Right Stick | optional fast-scroll / secondary-pane behavior |

On Pokémon Summary, `+` should prefer **Compatible Games / Transfer Compatibility** when that data is available.

The bottom controller bar should only show buttons that actually do something on the current screen.

No controller shortcut may bypass the live-write safety lock.

---

## Master Vault

The **Master Vault** is the central PokeBank NX storage model.

The earlier custom implementation was not recoverable, so v1 is being rebuilt from a written contract instead of guessed from fragments.

Specification: [`docs/MASTER_VAULT_SPEC.md`](docs/MASTER_VAULT_SPEC.md).

Core principles:

```text
immutable original raw Pokémon bytes
stable Vault ID
SHA-256 integrity
source game/platform/save/box/slot provenance
parent/derived relationships
clone/edit/convert history
atomic transactions
named Banks reference Vault entities
UNKNOWN legality remains UNKNOWN
```

A conversion, clone, edit, or generated Pokémon should create a derived record rather than silently destroying the original.

---

## Banks

Banks are organization layers on top of the Master Vault.

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

Removing an entry from a Bank should remove the Bank reference, not automatically delete the underlying Master Vault entity.

---

## Pokédex / Living Dex

The Master Vault is intended to be authoritative for collection ownership.

The Pokédex should derive owned/shiny/form/Alpha/event state from actual Vault entities rather than using an independent manual checkbox database.

Planned views:

- National Pokédex;
- Regional Pokédexes;
- Living Dex;
- Shiny Living Dex;
- permanent forms;
- meaningful gender differences;
- Alpha / Shiny Alpha where applicable;
- event ownership;
- missing filters;
- completion percentages.

Specification: [`docs/POKEDEX_SPEC.md`](docs/POKEDEX_SPEC.md).

---

## Target game coverage

Detection, parsing, conversion, staging, live writing, and physical-device verification are separate capabilities.

### Game Boy / Game Boy Color

- Pokémon Red
- Pokémon Blue
- Pokémon Yellow
- Pokémon Gold
- Pokémon Silver
- Pokémon Crystal

Current state: stable identities exist; parser/adapters need rebuilding. Audit PKSM-Core first.

### Game Boy Advance

- Pokémon Ruby
- Pokémon Sapphire
- Pokémon Emerald
- Pokémon FireRed
- Pokémon LeafGreen

Current state: stable identities exist. The first deep engine spike is PKSM-Core `PK3` + `Sav3` read-only integration.

### Nintendo DS / 3DS

Planned expansion includes the mainline DS/3DS generations after the core Vault and adapter architecture is proven.

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

Current state: platform-aware identities build and inherited PKSE handlers remain present. Each family still requires PokeBank NX-specific read-only/round-trip validation before later staged-write work.

See [`docs/GAME_SUPPORT_MATRIX.md`](docs/GAME_SUPPORT_MATRIX.md).

---

## Legality, provenance, and generated Pokémon

PokeBank NX separates:

```text
where a Pokémon actually came from
```

from:

```text
whether its data is valid for the encounter/history it claims
```

Legality states remain explicit:

```text
LEGAL
INVALID
UNKNOWN
```

`UNKNOWN` must never silently become `LEGAL`.

Generated Pokémon should remain labeled as generated in provenance.

The planned host-side **PKHeX Oracle** will use pinned PKHeX.Core / Auto Legality revisions to produce comparison results and golden vectors for the native C++ implementation.

Specification: [`docs/PKHEX_ORACLE.md`](docs/PKHEX_ORACLE.md).

---

## Transfers and HOME Bridge

Transfer semantics are defined in [`docs/TRANSFER_MODEL.md`](docs/TRANSFER_MODEL.md).

PokeBank NX distinguishes Copy, Move, Clone, Add to Vault, and Conversion rather than treating all transfers as the same operation.

The planned Pokémon HOME bridge does **not** impersonate HOME or directly connect to private Nintendo/Pokémon services.

Conceptually:

```text
PokeBank NX
    ↓
compatible official game save
    ↓
official Pokémon HOME
```

and in reverse:

```text
official Pokémon HOME
    ↓
compatible official game
    ↓
PokeBank NX
    ↓
Master Vault
```

PokeBank NX should never forge HOME trackers or advertise generated/edited Pokémon as guaranteed "ban safe."

---

## Save safety

Direct live installed-game save writing is currently **hard disabled**.

The safety contract is documented in [`docs/SAVE_SAFETY.md`](docs/SAVE_SAFETY.md).

Future live writes require an adapter-specific pipeline such as:

```text
read source
    ↓
backup + fingerprint
    ↓
stage clone
    ↓
apply intended mutation
    ↓
repair checksums/structures
    ↓
reparse + validate
    ↓
write destination
    ↓
read back
    ↓
verify exact intended change
    ↓
retain rollback/recovery copy
```

A UI toggle is not allowed to bypass these gates.

---

## Upstream / reference stack

PokeBank NX explicitly checks mature Pokémon research before rebuilding difficult infrastructure from scratch.

| Project | Role |
|---|---|
| [PKSE](https://github.com/kiasta/PKSE) | Original native Switch foundation; **upstream only** |
| [PKSM-Core](https://github.com/FlagBrew/PKSM-Core) | High-priority native C++ Pokémon/save engine candidate, especially Gen I-VIII |
| [PKHeX](https://github.com/kwsch/PKHeX) | Primary correctness/reference implementation |
| [Auto Legality Mod / PKHeX-Plugins](https://github.com/santacrab2/PKHeX-Plugins) | Encounter-driven legality/generation reference |
| [pkHouse](https://github.com/Insektaure/pkHouse) | Modern Switch save-behavior reference |
| [pkDex](https://github.com/Insektaure/pkDex) | Switch Pokédex UX/data-organization reference |
| [PKForge](https://github.com/sofianeelhor/PKForge) | Vault/provenance/transaction architecture reference |

Detailed pinned revisions, licenses, files, and reuse classification: [`docs/UPSTREAM_AUDIT.md`](docs/UPSTREAM_AUDIT.md).

### Reference/reuse policy

Before implementing major format, legality, conversion, save parsing, Pokédex, banking, or generated-Pokémon functionality:

1. check the tracked references;
2. identify the relevant file/commit where practical;
3. record language/license;
4. classify the approach as `DIRECT REUSE`, `PORT`, `ADAPTER`, or `REFERENCE ONLY`;
5. prefer mature tested behavior over unnecessary reinvention;
6. add regression/golden tests;
7. preserve required attribution/license notices.

`pkHouse` and `pkDex` are currently **REFERENCE ONLY** for PokeBank NX. Their author encouraged using pkHouse as a technical reference and recommended reimplementation instead of pure copy/paste.

---

## Roadmap

The detailed issue/dependency view lives in [`docs/PROJECT_MAP.md`](docs/PROJECT_MAP.md).

### Completed foundation

- [x] Repository recovery
- [x] Preserve PKSE history/foundation
- [x] Separate writable `origin` from upstream PKSE
- [x] 23 stable release/platform game identities
- [x] GBA/Switch FireRed + LeafGreen identity separation
- [x] Host regression/sanitizer infrastructure
- [x] GitHub host CI
- [x] Native Switch build baseline
- [x] Hard-lock live installed-save writes
- [x] Controller-first Pokémon Action Sheet — issue #2
- [x] Action Sheet device-test `.nro` produced and hashed

### Current milestone

- [ ] Recover interrupted local Session 2 work before resetting it
- [ ] Finish issue #13 — HOME-style controls and reusable UI/theme shell
- [ ] Host-test/sanitize/build the Session 2 implementation
- [ ] Produce and preserve a new combined device-test `.nro`
- [ ] Physically test the exact combined build — issue #8

### Next deep engineering milestone

Use **Max/deep reasoning** for issue #4:

```text
PKSM-Core PK3 + Sav3
FireRed / LeafGreen GBA
read-only parse
box + party extraction
round-trip strategy
integration/dependency decision
```

Do not rewrite Gen III from scratch before this spike.

### Core product phases after that

- [ ] Master Vault v1 + named Banks — issue #3
- [ ] Professional Summary + provenance — issue #9
- [ ] RetroArch discovery + Gen I-III adapters — issue #6
- [ ] Modern Switch adapter audit — issue #11
- [ ] PKHeX Oracle — issue #5
- [ ] Vault-driven Pokédex / Living Dex — issue #7
- [ ] Conversion/transfer engine without live writes — issue #10
- [ ] Generated collection tools / All + Shinies
- [ ] Events / Wonder Cards / Event Vault
- [ ] Live writes only after per-adapter safety gates and repeated device validation
- [ ] Stable v1.0

### Infrastructure

- [ ] Issue #15 — automate/persist device-test `.nro` artifacts via GitHub Actions or prereleases

---

## Build

### Host verification

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
```

### Native Switch build

The native integration build uses devkitPro/devkitA64 and:

```bash
make -j1
```

The exact recovery environment commands remain recorded in [`PROJECT_STATUS.md`](PROJECT_STATUS.md).

For every meaningful test build record:

```text
source commit
filename
file size
SHA-256
host tests
sanitizers
native build result
device-tested yes/no
```

See [`docs/BUILD_RECORD.md`](docs/BUILD_RECORD.md).

---

## Development workflow

GitHub is the permanent project state. Temporary coding-agent workspaces are not.

Normal milestone discipline:

```text
IMPLEMENT
    ↓
HOST TEST
    ↓
SANITIZERS
    ↓
BUILD .NRO
    ↓
UPDATE PROJECT_STATUS.md
    ↓
COMMIT
    ↓
PUSH
    ↓
VERIFY REMOTE SHA
    ↓
PRESERVE DEVICE ARTIFACT
```

If a coding session ends before commit/push, the next session must inspect local branches/worktrees/reflog before resetting anything.

---

## Documentation map

Start with [`docs/PROJECT_MAP.md`](docs/PROJECT_MAP.md).

Important files:

- [`PROJECT_STATUS.md`](PROJECT_STATUS.md) — authoritative current verified state
- [`docs/NEXT_SESSION_PLAN.md`](docs/NEXT_SESSION_PLAN.md) — exact next coding sequence
- [`docs/SESSION_RUNBOOK.md`](docs/SESSION_RUNBOOK.md) — coding-agent workflow
- [`docs/CONTROLS.md`](docs/CONTROLS.md) — controller contract
- [`docs/UI_FLOW.md`](docs/UI_FLOW.md) — safe controller-first flows
- [`docs/UI_STYLE_GUIDE.md`](docs/UI_STYLE_GUIDE.md) — visual/theme contract
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) — module boundaries
- [`docs/MASTER_VAULT_SPEC.md`](docs/MASTER_VAULT_SPEC.md) — Vault contract
- [`docs/SAVE_SAFETY.md`](docs/SAVE_SAFETY.md) — save-write safety gate
- [`docs/PKSM_CORE_INTEGRATION.md`](docs/PKSM_CORE_INTEGRATION.md) — Gen III integration spike
- [`docs/PKHOUSE_REFERENCE.md`](docs/PKHOUSE_REFERENCE.md) — modern Switch reference notes
- [`docs/UPSTREAM_AUDIT.md`](docs/UPSTREAM_AUDIT.md) — pinned research/reuse audit
- [`docs/BUILD_RECORD.md`](docs/BUILD_RECORD.md) — source/binary identity records
- [`docs/DEVICE_TEST_CHECKLIST.md`](docs/DEVICE_TEST_CHECKLIST.md) — physical Switch test plan

---

## Credits

PokeBank NX is derived from **PKSE — Pokémon Save Editor** and intentionally preserves its upstream history.

The project also studies or relies on work from the wider Pokémon and Switch homebrew communities, including PKSM-Core, PKHeX, Auto Legality Mod, pkHouse, pkDex, PKForge, PokeAPI sprites, devkitPro, and libnx.

Reference does not automatically mean source code is copied or included. Applicable attribution and license requirements must be preserved whenever code is directly reused.

---

## License

The PokeBank NX codebase is licensed under the **GNU Affero General Public License v3.0** as inherited from PKSE.

See [`LICENSE`](LICENSE).

---

## Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks are property of their respective owners.

This repository is intended for homebrew software and management of a user's own save/Pokémon data. It does not include commercial game ROMs or proprietary Nintendo/game files that users do not have permission to redistribute.

Always keep independent backups of important saves while using development builds.

---

## Immediate next path

```text
Action Sheet                    DONE
      ↓
Recover interrupted Session 2 local work
      ↓
Finish HOME-style controls + OLED/Dark/Light UI foundation
      ↓
Host test + sanitizer + native .nro
      ↓
Preserve/hash combined test artifact
      ↓
Physical Switch test
      ↓
Fix device-only regressions if any
      ↓
MAX: PKSM-Core PK3/Sav3 Gen III spike
      ↓
Master Vault v1 + Banks
      ↓
Summary / Provenance
      ↓
Retro + modern adapters
      ↓
PKHeX Oracle / Pokédex / Transfers
```

The project is no longer blocked on figuring out what it is supposed to become. The next work is implementation and hardware validation against the contracts already in the repository.