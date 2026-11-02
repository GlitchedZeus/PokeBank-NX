# Project status

**Last updated: 2026-11-02. Alpha. There is no stable release and no v1.0.**

## Summary

| Area | Status |
|---|---|
| **Gen I–V application** (Game Boy, Game Boy Color, GBA and DS games played through emulators on the Switch) | The product. Active alpha, repaired round by round against the owner's hardware tests. Not yet accepted across all 20 releases. |
| **Current candidate** | Round 13, `PokeBank-NX-Gen1-5-Candidate-9c301c48.nro`, SHA-256 `0ecf8655bcbef6490e73e2367801d972a8a9339396e73e16a4afe8f6ca76367d`, build run [41392463888](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/41392463888). **Owner hardware-tested: 15 PASS / 12 FAIL; not accepted.** Built from `fix/round12-gen15-repairs` (draft PR #333, stacked on #332); not on `main` yet. |
| **Round 13 owner result** | 15 PASS / 12 FAIL / 0 NOT TESTED across 27 checks. Not accepted. [Original owner JSON](hardware/round13/OWNER_RESULTS_9c301c48.json). |
| **Previous owner hardware result** | Round 12 `066e1db9`: 20 PASS / 4 FAIL on the owner's 24-item checklist (Gen V move/review controls, Clone/Move pop-ups and carry cursor, Gen II/III Met Location lists, Bag Top Up). Round 13 targets those findings. |
| **Gen VI–IX** | **Paused** on `development/gen6-plus`. Not feature complete. The experimental candidates `5895e6b9` and `43c296dc` **failed** owner hardware tests. |
| **Legality engine** | **Paused** at PR #314 (lanes #327 and #328). Read-only research engine, **not integrated into the app**. |
| **Master Vault and v1.0 storage features** | **Not implemented.** Required for v1.0. |

Every build and its result: [RELEASES.md](RELEASES.md). Round history: [CHANGELOG](../CHANGELOG.md).

## What works today (Gen I–V, alpha)

These paths exist in the current code, and most have owner hardware passes on recent candidates. None is accepted
across all 20 releases yet, and code that changed after a pass must be re-verified on the current candidate. The
per-item state is in the master hardware acceptance ledger linked below.

- Finding emulator saves (RetroArch for Game Boy, Game Boy Color and GBA; DraStic, melonDS and manual sources for
  DS), assigning a save to its game, and choosing between a primary and backup copy. FireRed and LeafGreen from
  Nintendo Switch Online are recognised by title ID; installed-game saves are read-only sources.
- Browsing Trainer, Party, Boxes and Bag for all 20 Gen I–V releases.
- One generation-aware Pokémon editor (View, Edit, Create, Clone, Move within the save, Release), with native
  limits per generation: DVs and Stat Exp in Gen I/II, natures/abilities/PIDs from Gen III on.
- Bag item add/remove/quantity editing, including Top Up / Add Missing.
- Import and export of single Pokémon files (`.pk1`–`.pk5`, `.ek3`–`.ek5`).
- Review of every staged change, then **Export Copy**: a new verified save written next to an exact backup of the
  original.
- Save Backups history and a Recovery files view.

Japanese Gen I/II saves open **read-only**. Their editors exist but are locked until the native writers are
proven.

## How original saves are protected

- The app never opens an emulator or installed-game save for writing. Live writes are disabled in code
  (`include/Safety/WritePolicy.h`), not by a setting.
- Edits are staged in an app-owned working copy. **Export Copy** writes a new folder under
  `sdmc:/switch/PokeBank-NX/exports/` containing an exact backup of the original, the edited save and a manifest.
  Each file is read back and hash-checked, and the edited save must strictly reparse as the same game with valid
  checksums. A failure at any step leaves no partial export, and an existing export is never overwritten.
- Using an exported save means copying it into place yourself; the app does not inject it.
- Moving a Pokémon between different games is locked. Unknown or ambiguous saves are refused rather than guessed.

Details: [save-safety contract](user/SAVE_SAFETY.md) and the automated [save-safety chain](qa/SAVE_SAFETY_CHAIN.md).

## What remains before v1.0

Required and **not implemented**:

- Master Vault: persistent cross-game storage owned by PokeBank NX.
- Move to Vault from the Pokémon Actions menu, and Vault deposit / withdraw.
- Vault search with highlighted matches.
- Cross-generation transfer (for example Gen I to Gen V) with explicit route rules.
- Renaming boxes inside a game save.

In progress: source-backed gift and static encounter data, so Met Location and origin choices only offer what the
games allow.

Then: one exact NRO must pass the full Gen I–V hardware gate across all 20 releases
([gate](handoffs/GEN15_HARDWARE_ACCEPTANCE_GATE_20261015.md), issue #313).

## Status documents

- Candidate archive with exact identities: [hardware/NRO_ARCHIVE.md](hardware/NRO_ARCHIVE.md).
- Round 13 checklist (on `fix/round12-gen15-repairs`): [RETEST_9c301c48.md](https://github.com/GlitchedZeus/PokeBank-NX/blob/fix/round12-gen15-repairs/docs/hardware/RETEST_9c301c48.md).
- Master hardware acceptance ledger, 276 tracked issues (on `fix/round12-gen15-repairs`): [MASTER_HARDWARE_ACCEPTANCE_LEDGER.md](https://github.com/GlitchedZeus/PokeBank-NX/blob/fix/round12-gen15-repairs/docs/qa/MASTER_HARDWARE_ACCEPTANCE_LEDGER.md).
- v1.0 feature gaps (on `fix/round12-gen15-repairs`): [V1_RELEASE_FEATURE_GAPS.md](https://github.com/GlitchedZeus/PokeBank-NX/blob/fix/round12-gen15-repairs/docs/qa/V1_RELEASE_FEATURE_GAPS.md).
- Repository cleanup audit: [maintenance/REPO_CLEANUP_AUDIT_2026-11-02.md](maintenance/REPO_CLEANUP_AUDIT_2026-11-02.md).

When those branch files merge to `main`, these links can point to `main`.
