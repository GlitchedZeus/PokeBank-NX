# PokeBank NX Project Status

Last updated: 2026-09-14

For the shortest recovery handoff, read `CURRENT_STATUS.md` and `docs/NEXT_SESSION_PLAN.md`.

## Project identity

```text
Product: PokeBank NX
Version: 0.1.0-alpha
Repository: GlitchedZeus/PokeBank-NX
Production branch: feature/pokebank-playable
Production checkpoint: a9fc4521087cdc80078c7db620f1be0107adce58
Focused branch: feature/gen2-shared-pokemon-editor-20260914
Active PR: #68 — OPEN / DRAFT / NOT MERGED
Audit tracker: #69 — OPEN
Latest pre-documentation implementation checkpoint: a3917f20e0b09b31473f073398a9b1f1a6adfb51
Writable remote: origin
Upstream/reference: kiasta/PKSE
```

`DEVICE ACCEPTED` is reserved for an exact artifact physically tested by the owner. The final Gen II candidate SHA is the exact PR head after the audit/status documentation refresh and must pass the dedicated exact-SHA candidate workflow before handoff.

## Headline

- Generation I, II, and III legacy read-only paths are physically accepted.
- Classic staged Inventory is hardware accepted.
- The Generation I boxed staged Pokémon editor and passive View are hardware accepted.
- Generation II shared View/Create/Edit and final audit remediation are IMPLEMENTED on PR #68.
- PR #68 now includes Crystal-native caught/met decoding, Party-only HP/status, named Species/Move/Pokérus controls, and a verified transactional staged-export path.
- Full v1 touch remains #55; global controller normalization remains #26; Legacy Storage/Master Vault separation remains #27.
- Live installed-game and emulator-source writes remain HARD DISABLED.
- Gen II DEVICE TEST is still pending an exact green candidate artifact and owner physical Gold/Silver/Crystal test.

## Accepted legacy/read/editor baselines

| Milestone | Status |
|---|---|
| Red / Blue / Yellow read-only | DEVICE ACCEPTED |
| Gold / Silver / Crystal read-only | DEVICE ACCEPTED |
| FireRed / LeafGreen / Ruby / Sapphire / Emerald read-only | DEVICE ACCEPTED |
| Classic staged Inventory | DEVICE ACCEPTED |
| Gen I boxed staged Pokémon editor | DEVICE ACCEPTED |
| Gen I passive View unification | DEVICE ACCEPTED |

Generation II read semantics remain:

```text
Gold Trainer gender: Male
Silver Trainer gender: Male
Crystal Trainer gender: save-derived
Gen II SID: N/A — does not exist
```

Accepted Gen I editor artifact remains:

```text
Source: 69668bc81629228ef25c1bdada7c7ce1aed9b666
NRO: PokeBank-NX-Gen1-UX4-Retest-69668bc8.nro
SHA-256: 3ab11f7ba6938bbab5f7cbbf192d819532ce94f09bc7788a3bb0d8f6217f3763
```

Do not rewrite or replace this historical acceptance evidence while validating Gen II.

## Active Generation II shared editor milestone — PR #68

### Editor and presentation

Implemented:

- capability-driven `DETAILS | VALUES | MOVES` shell;
- passive Party/Box/action-sheet View with no edit cursor;
- exact Gold/Silver vs Crystal move compatibility generated from pinned PKHeX data;
- canonical Level/EXP behavior for Edit and Create with immediate stats/radar synchronization;
- local/staged Create/Edit only;
- transactional Keep / Discard current Edit session / Continue behavior;
- authentic Gen II DVs/Stat Exp, derived HP DV, split SpA/SpD display from the one stored Special value;
- six battle stats and six-axis radar;
- exact Held Item selection domain with preservation of unusual existing bytes;
- Friendship;
- named Species picker;
- named exact-game Move picker;
- user-facing Pokérus State/Strain/Days controls;
- immediate PP/PP Ups validation/clamping;
- gender derived from species + Attack DV;
- shiny derived from authentic DV patterns;
- Crystal caught/met time, level, location and original-trainer gender presentation;
- Gold/Silver hiding of Crystal-only caught/met fields;
- Party-only Current HP / Max HP / Status presentation;
- contained touch Back for passive Gen I/II View routed through the same Back action as controller B;
- exact save/game/revision capability model for future per-format editors.

### Verified staged export

There is one authoritative Generation II save-export implementation:

```text
UI Review -> Export
  -> publishVerifiedStagedEditorExport()
  -> finalized staged bytes
  -> strict pre-write parse
  -> source + edited SHA-256
  -> temporary app-owned directory
  -> original_backup.srm + edited.srm
  -> flush/fsync/close where supported
  -> disk read-back and byte/hash verification
  -> strict reparse of the disk bytes
  -> provenance EDIT_MANIFEST.txt
  -> same-filesystem publish/rename
  -> incomplete temp cleanup on failure
```

The GSC UI overlay no longer performs an independent `fopen/fwrite` export sequence. UI success is posted only after the transaction has verified and published. Failure states that the source remains unchanged.

### Permanent automated coverage

The Gen II audit candidate gate includes focused behavior/regression coverage for:

- parser/discovery/source identity;
- Level/EXP/Create/transaction semantics;
- action-sheet and passive View;
- Crystal caught/met and Party-native presentation;
- Held Item, Species, Move, Pokérus, PP/PP Ups;
- gender, shiny, six-stat calculation, radar;
- verified staged export including disk equality, strict reparse, corruption rejection, cleanup, source immutability and provenance manifest;
- Gen I, Gen III and Classic Inventory regressions;
- full host suite and ASan/UBSan;
- clean devkitA64 build, RomFS, embedded source identity and native-link proof;
- exact source-addressed `Gen2-Audit` packaging.

## Audit tracker #69

The following PR #68 remediation is implemented and is eligible to be marked complete once the final exact candidate SHA is fully green:

- Crystal caught/met meaningful decode;
- Party-only HP/status View;
- named/user-facing Species, Move and Pokérus controls;
- staged export transaction hardening.

Contained passive View touch Back is implemented, but the full touch-only v1 requirement remains **DEFERRED -> #55**.

Global box/controller normalization remains **DEFERRED -> #26** and requires its own dedicated/device-tested pass.

Legacy `PKSEBANK` Storage migration / Master Vault / true Move architecture remains **DEFERRED -> #27**. Legacy Storage is not Master Vault and backup-side operations are not product-level destructive Move semantics.

## Permanent editor architecture

```text
exact game/save/revision capabilities
        ↓
shared View/Create/Edit shell
        ↓
generation-specific adapters and user-facing pickers
        ↓
local/staged mutations only
        ↓
strict serialize/finalize/reparse validation
        ↓
verified exported edited copy + original backup + provenance
        ↓
future separately approved source-specific write adapter
```

## Standalone runtime contract

```text
Runtime root: sdmc:/switch/PokeBank-NX/
Required /PKSE/: NO
Required PKSE.nro: NO
Required PKSM: NO
Required pkDex: NO
Required pkHouse: NO
Required JKSV: NO
Required Checkpoint: NO
Required PC PKHeX: NO
```

External tools may be development references/oracles or optional migration helpers; they are not prerequisites for normal advertised runtime operation.

## Safety

- Original source bytes are immutable.
- Live installed-game writes are HARD DISABLED.
- Live RetroArch writes are HARD DISABLED.
- Live other-emulator writes are HARD DISABLED.
- Party mutation remains deferred where safety proof is incomplete.
- Unknown save variants fail closed.
- Read acceptance does not authorize writes.
- A staged export is not successful until read-back/hash/strict-reparse/publish completes.

## Final Gen II gate / stop condition

Do not begin another major feature milestone. The exact PR head containing this status refresh is to be treated as the sole candidate source if and only if all exact-head workflows are green. Retrieve the CI-built `PokeBank-NX-Gen2-Audit-<shortsha>.nro`, independently verify hashes/package identity, deliver that exact binary, and STOP coding for owner Gold/Silver/Crystal hardware testing.

PR #68 remains OPEN / DRAFT / NOT MERGED. Gen II remains DEVICE TEST PENDING until the owner accepts the exact candidate artifact.
