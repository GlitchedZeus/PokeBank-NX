<p align="center">
<img width="1672" height="941" alt="PokeBank NX Route 1 Adventure" src="https://github.com/user-attachments/assets/73d50980-7930-43f9-8b5f-3ae59d86bd58" />
</p>

# PokeBank NX

**PokeBank NX** is an offline Pokémon storage, collection, and save-management project for Nintendo Switch homebrew.

The project started from the PKSE codebase, but the long-term goal is broader: one controller-first Switch app for browsing Pokémon saves, keeping a permanent local Master Vault, organizing Pokémon into named Banks, tracking provenance, building Living Dex collections, and safely moving compatible Pokémon between games and generations.

> **Alpha warning:** direct live installed-game save writing is intentionally hard disabled. Development builds are not production save-writing tools.

---

## Current status — September 2026

Development branch:

```text
feature/pokebank-playable
```

PKSE remains upstream only. PokeBank NX changes must never be pushed to the original PKSE repository.

### First physical Switch milestone complete

The first exact recorded PokeBank NX `.nro` has now been physically run on Nintendo Switch hardware.

```text
Application source:
3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a

Artifact:
PokeBank-NX-UI-Theme-3be4de6.nro

Size:
9,707,957 bytes

SHA-256:
df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a

Result:
DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

Physical results:

| Area | Result |
|---|---|
| Boot | **PASS** |
| D-pad navigation | **PASS** |
| Left Stick + held repeat | **FAIL** |
| A → Pokémon Action Sheet | **PASS** |
| B / Cancel | **PASS** |
| L / R | **PASS** |
| ZL / ZR | **PASS** |
| `+` Options / More | **PASS** |
| `-` Help / Controls | **PASS** |
| OLED Black | **PASS** |
| Dark | **PASS** |
| Light | **PASS** |
| Theme persistence | **PASS** |
| Party | **PASS** |
| Boxes | **PASS** |
| Storage | **PASS** |
| Crashes | **NONE OBSERVED** |

Permanent test report: [`docs/DEVICE_TEST_REPORT_2026-09-01.md`](docs/DEVICE_TEST_REPORT_2026-09-01.md).

### Important hardware finding

The control/theme foundation works, but the visible application still looks substantially like inherited PKSE. Obvious PKSE branding/logo and inherited screen identity remain visible.

That means the project has passed an important **functional hardware milestone**, but the visible PokeBank NX product shell is not finished.

Current follow-up work:

```text
#13  REOPENED — apply visible PokeBank NX shell
#19  OPEN — fix held Left Stick navigation repeat
#16  OPEN / elevated — branding/startup/NRO metadata
```

The next HIGH coding session is permanently stored at:

[`docs/PROMPT_SESSION2_5_VISUAL_SHELL.md`](docs/PROMPT_SESSION2_5_VISUAL_SHELL.md)

After the replacement UI/input build passes a second hardware test, the next MAX/deep-engineering target is the PKSM-Core `PK3` / `Sav3` Gen III spike.

---

## Verification vocabulary

PokeBank NX deliberately separates these claims:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

For physical builds, the result is also recorded:

```text
DEVICE TESTED — PASS
DEVICE TESTED — PARTIAL PASS
DEVICE TESTED — FAIL
```

A build being physically tested does not mean every tested feature passed.

---

## Current PokeBank NX-specific work

### Controller-first Pokémon Action Sheet

Issue #2 is completed.

Pressing **A** on an occupied Pokémon opens one shared deliberate Action Sheet across Party, Boxes, and Storage:

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

Opening the Action Sheet, moving focus, pressing B, or selecting Cancel performs zero mutation. `View Pokémon` is read-only. Unfinished actions remain safely unsupported rather than falling through into another action.

This behavior has now been exercised successfully on physical Switch hardware through the combined Session 2 build.

### HOME-style controller foundation

Current controller contract:

| Control | Behavior |
|---|---|
| D-pad | precise navigation |
| Left Stick | navigation + held repeat target |
| A | Select / Open; Pokémon → Action Sheet |
| B | Back / Cancel |
| X | Filter / Search / context |
| Y | Sort / View / secondary context |
| L / R | previous/next box, Pokémon, or nearby tab |
| ZL / ZR | larger jumps / major navigation |
| `+` | contextual More / Options |
| `-` | Help / Controls / Screen Info |
| Right Stick | optional fast-scroll / secondary pane |

D-pad, A/B, L/R, ZL/ZR, `+`, and `-` have physical-device evidence on the first tested build. Held Left Stick repeat currently has a real hardware failure tracked by issue #19.

Full controller contract: [`docs/CONTROLS.md`](docs/CONTROLS.md).

### Themes

The current semantic theme system supports:

```text
OLED Black
Dark
Light
```

All three themes and theme persistence passed the first physical Switch test.

The next UI milestone is to apply these primitives broadly enough that the application is visibly PokeBank NX instead of mostly inherited PKSE.

Visual contract: [`docs/UI_STYLE_GUIDE.md`](docs/UI_STYLE_GUIDE.md).

---

## Safety

**LIVE INSTALLED-GAME SAVE WRITING IS HARD DISABLED.**

Current protections include:

- safe/backup destination posture;
- generic save API cannot request title injection;
- low-level restore rejects live-title writes before mounting save data;
- legacy `injectToGame=1` is disabled/rewritten;
- Action Sheet open/navigation/B/Cancel remain non-mutating.

The first physical hardware pass did not report a visible live-write safety regression.

Future write support must pass the explicit adapter-specific gates in [`docs/SAVE_SAFETY.md`](docs/SAVE_SAFETY.md).

---

## Master Vault vision

The **Master Vault** is the planned central PokeBank NX storage model.

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

Specification: [`docs/MASTER_VAULT_SPEC.md`](docs/MASTER_VAULT_SPEC.md).

Banks are organization layers over Vault entities rather than destructive copies of storage state.

---

## Pokédex / Living Dex vision

The Master Vault is intended to be authoritative for collection ownership.

Planned views include:

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

## Current game identities

PokeBank NX currently tracks 23 stable release/platform identities.

### Game Boy / Game Boy Color

- Red
- Blue
- Yellow
- Gold
- Silver
- Crystal

### Game Boy Advance

- Ruby
- Sapphire
- Emerald
- FireRed
- LeafGreen

### Nintendo Switch

- FireRed
- LeafGreen
- Let's Go, Pikachu!
- Let's Go, Eevee!
- Sword
- Shield
- Brilliant Diamond
- Shining Pearl
- Legends: Arceus
- Scarlet
- Violet
- Legends: Z-A

GBA FireRed/LeafGreen and Switch FireRed/LeafGreen are separate stable identities.

Detection, parsing, conversion, staging, live writing, and physical-device verification are tracked as separate capabilities.

See [`docs/GAME_SUPPORT_MATRIX.md`](docs/GAME_SUPPORT_MATRIX.md).

---

## Next deep engineering target

After the current visible-shell/input replacement build is physically stable enough, issue #4 begins the first PKSM-Core integration spike.

Narrow target:

```text
PK3
Sav3
FireRed / LeafGreen GBA
read-only parsing
box / party extraction
active save-slot / sector behavior
PK3 checksum / encryption behavior
untouched round-trip strategy
adapter / dependency decision
```

Plan: [`docs/PKSM_CORE_INTEGRATION.md`](docs/PKSM_CORE_INTEGRATION.md).

Ready-to-run MAX prompt: [`docs/PROMPT_SESSION3_PKSM_CORE.md`](docs/PROMPT_SESSION3_PKSM_CORE.md).

---

## Upstream / reference stack

PokeBank NX checks mature Pokémon research before rebuilding difficult infrastructure from scratch.

| Project | Role |
|---|---|
| [PKSE](https://github.com/kiasta/PKSE) | Original native Switch foundation; **upstream only** |
| [PKSM-Core](https://github.com/FlagBrew/PKSM-Core) | Native C++ historical Pokémon/save engine candidate |
| [PKHeX](https://github.com/kwsch/PKHeX) | Primary correctness/reference implementation |
| [PKHeX-Plugins / Auto Legality](https://github.com/santacrab2/PKHeX-Plugins) | Encounter-driven legality/generation reference |
| [pkHouse](https://github.com/Insektaure/pkHouse) | Modern Switch save-behavior reference |
| [pkDex](https://github.com/Insektaure/pkDex) | Pokédex UX/data-organization reference |
| [PKForge](https://github.com/sofianeelhor/PKForge) | Vault/provenance/transaction architecture reference |

Pinned revisions, licenses, and reuse classifications are documented in [`docs/UPSTREAM_AUDIT.md`](docs/UPSTREAM_AUDIT.md).

`pkHouse` and `pkDex` are currently treated as **REFERENCE ONLY** for implementation behavior; PokeBank NX reimplements/cross-checks rather than blindly copy/pasting code.

---

## Roadmap

Current near-term order:

```text
HIGH: #13 + #19 + visible portion of #16
        ↓
second exact .nro
        ↓
physical Switch retest
        ↓
MAX: #4 PKSM-Core PK3/Sav3
        ↓
#3 Master Vault v1 + Banks
        ↓
#9 Professional Summary + provenance
        ↓
#6 RetroArch / read-only Gen I-III adapters
        ↓
#11 modern Switch adapter validation
        ↓
#5 PKHeX Oracle
        ↓
#7 Vault-driven Pokédex / Living Dex
        ↓
#10 conversion / transfer without live writes
```

Detailed issue/dependency map: [`docs/PROJECT_MAP.md`](docs/PROJECT_MAP.md).

---

## Start here for development

Read in this order:

1. [`PROJECT_STATUS.md`](PROJECT_STATUS.md)
2. [`docs/DEVICE_TEST_REPORT_2026-09-01.md`](docs/DEVICE_TEST_REPORT_2026-09-01.md)
3. [`docs/NEXT_SESSION_PLAN.md`](docs/NEXT_SESSION_PLAN.md)
4. [`docs/SESSION_RUNBOOK.md`](docs/SESSION_RUNBOOK.md)
5. [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
6. [`docs/SAVE_SAFETY.md`](docs/SAVE_SAFETY.md)
7. the relevant implementation prompt/spec for the current issue

Current HIGH prompt:

```text
docs/PROMPT_SESSION2_5_VISUAL_SHELL.md
```

Current later MAX prompt:

```text
docs/PROMPT_SESSION3_PKSM_CORE.md
```

---

## License / attribution

PokeBank NX is distributed under the repository's **GNU Affero General Public License v3**. See [`LICENSE`](LICENSE).

The project began from PKSE and preserves required upstream/license attribution. External projects referenced during development retain their own licenses and notices; reuse classifications are tracked in `docs/UPSTREAM_AUDIT.md`.

---

## Disclaimer

PokeBank NX is an unofficial fan-made homebrew project and is not affiliated with or endorsed by Nintendo, The Pokémon Company, GAME FREAK, or Creatures Inc. Pokémon and related trademarks are property of their respective owners.

Unofficial fan-made homebrew project • © 2026 PokeBank NX Contributors
