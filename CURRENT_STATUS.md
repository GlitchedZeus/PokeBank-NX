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
| Gen III R/S/E/FRLG | Read + staged Pokémon editing |
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

## Frozen hardware-test candidate

Application SHA:
**00ee7a6ed7ac1b5a93c43246d70c252e135acec0**

Tree SHA:
**b0832910df44898114a413191ebad3228bded8af**

Exact Actions NRO:
**PokeBank-NX-PhysicalAudit-00ee7a6e.nro**

NRO SHA-256:
**5fad07002c5074ffb3d1d2bdd91275ef29fbdf199f7db263f39c5a3a9f86ca25**

Artifact:
**PokeBank-NX-PhysicalAudit-00ee7a6ed7ac1b5a93c43246d70c252e135acec0**

Artifact ID:
**10956604914**

## Exact-head automated verification

- GitHub Actions Host Tests #1385 / run ID 36392644853: **PASS**
  - exact application identity: PASS
  - clean host build: PASS
  - full host tests: PASS
  - focused RSE save-open bridge regression: PASS
  - ASan / UBSan: PASS
- Audit Hardening Native Validation #226 / run ID 36392639705: **PASS**
- Focused Packed Multi-Move #201 / run ID 36392644998: **PASS**
- Focused Packed Move #202 / run ID 36392645087: **PASS**

**AUTOMATED GATES: PASS**

**DEVICE ACCEPTANCE: PENDING OWNER HARDWARE TEST**

The artifact's build identity and SHA256 manifest were independently checked after download and match the frozen application/tree/NRO identity above.

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

Owner physical Switch testing of the exact frozen Actions-built NRO.

Until that hardware test passes, the provider-neutral runtime candidate remains **automated PASS / hardware pending**, not DEVICE ACCEPTED.

## Canonical project documents

- README.md — public project front page
- CURRENT_STATUS.md — exact engineering checkpoint
- PROJECT_STATUS.md — high-level project state
- docs/V1_ROADMAP.md — release roadmap
- docs/GAME_SUPPORT_MATRIX.md — game support matrix
- docs/audit/ISSUE85-SAVE-INSTANCES.md — provider-neutral source architecture audit
- docs/REFERENCE_INDEX.md — upstream/reference index
