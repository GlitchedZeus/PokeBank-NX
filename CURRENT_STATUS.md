# PokeBank NX — Current Verified Engineering State

Last updated: **2026-09-28**

GitHub is authoritative. Re-fetch live heads before new work and preserve any newer commits.

## Active development line

PR #79 — **OPEN / DRAFT / NOT MERGED**

Branch:
**audit/full-project-hardening-20260923**

Current exact head:
**00ee7a6ed7ac1b5a93c43246d70c252e135acec0**

Current tranche:
**Issue #85 — provider-neutral Save Instance architecture**

PR #77 remains untouched at **996e6aa40c96e4408282f3d55476dae8e64968b2** and remains OPEN / DRAFT / NOT MERGED.

## Hardware-accepted checkpoints

### Gen I-III shared editor

Application SHA:
**996e6aa40c96e4408282f3d55476dae8e64968b2**

Status:
**DEVICE ACCEPTED**

### Multi-provider Save Instances

Application SHA:
**d49efd0c16433aaa1aa171501671a6e811c9da64**

NRO SHA-256:
**5b724f2de948bc666957699ecd76319ae54deaa5d85e5c9d9b62c92675ef8d18**

Status:
**DEVICE ACCEPTED**

The current 00ee7a6e runtime refactor is newer than this hardware checkpoint and is **not device accepted yet**.

## Supported game state

| Area | Current state |
|---|---|
| Gen I Red / Blue / Yellow | Read + staged Pokémon editing |
| Gen II Gold / Silver / Crystal | Read + staged Pokémon editing |
| Gen III R/S/E/FR/LG | Read + staged Pokémon editing |
| Gen IV D/P/Pt/HG/SS | Strict read-only Trainer / Party / Boxes / Pokémon details |
| Classic Inventory | Staged editing where already supported |
| Multi-provider Save Instances | Implemented for Gen I-IV |
| Cross-game True Move | Disabled |
| Gen IV editing / Create / Delete / conversion | Disabled |
| Live external-source writes | Disabled |
| Master Vault | Not started |

## Provider-neutral Save Instance architecture

The current refactor unifies duplicated source-browser metadata and presentation without replacing generation-specific parsers.

Shared responsibilities now include:

- provider identity;
- source path and normalized path;
- physical/stable source identity;
- file size and modified time;
- trainer / party summary where available;
- validation and access state;
- recovered-copy diagnostics;
- source fingerprints;
- remembered-source state;
- profile claims;
- Save Instances sorting and deduplication;
- shared row rendering;
- changed/missing-source revalidation before open.

Generation-specific parser/validation handles remain generation-specific.

Important safety fixes at 00ee7a6e include preserving remembered/alias metadata through dedupe, transactional alias claims, cross-profile and cross-game conflict refusal, and refusing a source that changed after its row was displayed.

## Provider support

### Gen I-III

- RetroArch
- configured mGBA battery-save directory only
- Tico GB / GBC / GBA verified save roots only
- Manual where supported

Verified Tico roots remain:

- sdmc:/tico/saves/gb
- sdmc:/tico/saves/gbc
- sdmc:/tico/saves/gba

No Tico DS root is authorized.

### Gen IV

- RetroArch
- DraStic cartridge backups
- melonDS
- Manual / remembered sources

DraStic .dsv cartridge backups are supported read only. DraStic .dss savestates remain unsupported.

## Current CI state for 00ee7a6e

- Focused Packed Move: **PASS**
- Focused Packed Multi-Move: **PASS**
- Full local host suite: **PASS**
- Full local ASan / UBSan: **PASS**
- Exact-head native devkitA64 compile/link: reported **PASS** during the tranche
- GitHub Actions Host Tests #1385: **still in progress at latest re-fetch**

Do not freeze or label a new hardware candidate all-green until the exact-head Actions host workflow completes successfully.

## Permanent safety invariants

- Original external saves are immutable.
- Installed-game live writes are disabled.
- RetroArch live writes are disabled.
- mGBA live writes are disabled.
- Tico live writes are disabled.
- DraStic live writes are disabled.
- melonDS live writes are disabled.
- Unknown / ambiguous saves fail closed.
- Remembered sources may not silently substitute a different physical file.
- A-button is never destructive by itself.
- Cross-game True Move remains locked.

## Next gate

Finish Issue #85 exact-head CI, freeze the resulting SHA, produce an Actions-built NRO, verify its SHA-256 and artifact identity, and hand it to the owner for physical Switch testing.

Until that test passes, the new provider-neutral runtime candidate remains **CI candidate / hardware pending**, not DEVICE ACCEPTED.

## Canonical project documents

- README.md — public project front page
- CURRENT_STATUS.md — exact engineering checkpoint
- PROJECT_STATUS.md — high-level project state
- docs/V1_ROADMAP.md — release roadmap
- docs/GAME_SUPPORT_MATRIX.md — game support matrix
- docs/audit/ISSUE85-SAVE-INSTANCES.md — provider-neutral source architecture audit
- docs/REFERENCE_INDEX.md — upstream/reference index
