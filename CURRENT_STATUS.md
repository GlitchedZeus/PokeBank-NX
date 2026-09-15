# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-14

## Current repository state

```text
Repository: GlitchedZeus/PokeBank-NX
Production branch: feature/pokebank-playable
Production checkpoint: a9fc4521087cdc80078c7db620f1be0107adce58
Focused branch: feature/gen2-shared-pokemon-editor-20260914
Draft PR: #68 — OPEN / DRAFT / NOT MERGED
Audit tracker: #69 — OPEN
Latest pre-documentation implementation checkpoint: a3917f20e0b09b31473f073398a9b1f1a6adfb51
Writable remote: origin
Upstream/reference: kiasta/PKSE
```

Do not reset the focused branch backward if newer commits exist. The final candidate identity is the exact PR head after this documentation refresh and is authoritative only when the exact-SHA `Gen II Audit Candidate Gate` passes. PR #68 must remain draft/unmerged until the owner physically accepts that exact artifact.

## Status vocabulary

```text
IMPLEMENTED          code exists on the focused branch
CI VERIFIED          exact source SHA passed the stated automated gate
DEVICE TEST PENDING  exact artifact is ready for owner hardware testing
DEVICE ACCEPTED      owner physically tested and accepted that exact artifact/hash
```

Never promote a milestone from IMPLEMENTED/CI VERIFIED to DEVICE ACCEPTED without the owner hardware result.

## Physically accepted milestones — preserve historical evidence

### Legacy read-only

```text
Red GB        DEVICE ACCEPTED
Blue GB       DEVICE ACCEPTED
Yellow GB     DEVICE ACCEPTED
Gold GBC      DEVICE ACCEPTED
Silver GBC    DEVICE ACCEPTED
Crystal GBC   DEVICE ACCEPTED
FireRed GBA   DEVICE ACCEPTED
LeafGreen GBA DEVICE ACCEPTED
Ruby GBA      DEVICE ACCEPTED
Sapphire GBA  DEVICE ACCEPTED
Emerald GBA   DEVICE ACCEPTED
```

Gold/Silver Trainer gender is fixed Male. Crystal Trainer gender is save-derived. Gen II SID does not exist.

### Classic staged Inventory

DEVICE ACCEPTED. Exact-game/exact-pocket catalogs, empty valid categories, machine move labels, Key Item warnings, staged-only mutation, and source immutability remain the contract.

### Generation I boxed staged Pokémon editor

Issue #62 is CLOSED / DEVICE ACCEPTED.

```text
Accepted source: 69668bc81629228ef25c1bdada7c7ce1aed9b666
Accepted NRO: PokeBank-NX-Gen1-UX4-Retest-69668bc8.nro
Accepted NRO SHA-256: 3ab11f7ba6938bbab5f7cbbf192d819532ce94f09bc7788a3bb0d8f6217f3763
```

PR #67 was subsequently hardware accepted and merged to unify Party/Storage/action-sheet View Pokémon into one passive no-cursor read-only presenter.

## Active milestone — Generation II shared Pokémon editor / PR #68

### IMPLEMENTED

The focused branch now contains:

- shared Gen II passive `DETAILS | VALUES | MOVES` View;
- occupied-slot action-sheet View using the passive read-only presenter;
- exact Gold/Silver vs Crystal move compatibility from generated offline tables pinned to PKHeX `77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5`;
- live View compatibility (`OK` / `Unusual preserved`) while encounter legality remains explicitly `Not checked`;
- local staged Create/Edit drafts rather than immediate source mutation;
- canonical Level/EXP transactions: Level and EXP never submit as conflicting canonical fields, Create serializes edited EXP, and EXP immediately refreshes Level/stats/radar;
- authentic 0–15 Attack/Defense/Speed/Special DVs plus derived HP DV;
- one stored Special DV/Stat Exp feeding split SpA/SpD presentation;
- six displayed battle stats plus six-axis radar in `HP / Atk / Def / SpA / SpD / Spe` order;
- exact-domain Gen II Held Item picker with unusual pre-existing raw values preservable;
- Friendship editing;
- named Species picker;
- named exact-game Move picker;
- user-facing Pokérus State/Strain/Days editing with byte round-trip behavior;
- PP / PP Ups immediate validation/clamping and correct reset/empty behavior;
- Crystal caught/met decode into native time/level/location/OT-gender presentation;
- Gold/Silver continue to hide Crystal-only caught/met data;
- Party-only Current HP / Max HP / Status presentation without fabricating Box values;
- species + Attack-DV-derived gender semantics;
- DV-derived shiny semantics;
- transactional existing-Pokémon Edit: unchanged Back exits; changed Back offers Keep / Discard this Edit session / Continue; Discard restores the exact staged baseline that existed when Edit opened;
- preservation of pre-existing unusual move/PP/PP-Up bytes during unrelated edits;
- contained passive Gen I/II touch Back using the same passive Back action as controller B;
- exact game/save/revision capability model including LGPE IV+AV and PLA effort-level distinctions for future editors;
- one authoritative verified Gen II staged-export path.

### Verified staged export architecture

The live Review -> Export action delegates to `publishVerifiedStagedEditorExport()`; the GSC overlay contains no second `fopen`/`fwrite` save-export implementation. The transaction performs:

```text
staged finalize
strict pre-write parse
source SHA-256 + edited SHA-256
unique temporary directory
original_backup.srm
edited.srm
synced/closed writes where supported
disk read-back
exact byte/hash verification
strict parse of the disk-read edited bytes
EDIT_MANIFEST.txt with source/game/application provenance
same-filesystem directory publish/rename
temporary cleanup on failure
```

Only a fully verified/published transaction is reported as success. Failure reports the specific transaction error and that the source remains unchanged. Live source writeback is not enabled.

### Audit issue #69 scope

Implemented P2 remediation in PR #68 includes Crystal caught/met decode, Party HP/status, named Species/Move/Pokérus controls, contained passive touch Back, and verified staged-export transactionality.

Global follow-ups intentionally remain separate:

```text
Full touch-only v1: DEFERRED -> #55
Global box/controller normalization: DEFERRED -> #26
Legacy Storage -> Master Vault migration / true Move architecture: DEFERRED -> #27
```

Issue #55, #26, and #27 remain open. Legacy `PKSEBANK` Storage is not the future Master Vault and is not product-level destructive Move semantics.

## Validation boundary

`a3917f20...` is the latest pre-documentation implementation/workflow checkpoint. It is not itself declared DEVICE ACCEPTED. The exact final candidate is the later PR head containing this documentation refresh, and all final evidence must name that same SHA.

The dedicated `Gen II Audit Candidate Gate` is the authoritative final automation. On the exact candidate SHA it verifies:

```text
git diff --check
pinned compatibility-data regeneration
Gen II read-only parser/discovery/bridge regressions
Gen II staged editor + session/Create/Edit/Level/EXP behavior
passive/action-sheet View contracts
Crystal caught/met + Party native presentation
Held Item / Species / Move / Pokérus / PP / PP Ups behavior
gender / shiny / six battle stats / six-axis radar
verified export UI + transaction / corruption rejection / temp cleanup
source mutation/write policy
Gen I regressions
Gen III regressions
Classic Inventory regression
full permanent host suite
ASan
UBSan
device asset preflight
complete RomFS recovery
clean devkitA64 build + final NRO link
AArch64 native linkage
embedded application SHA
embedded RomFS
source-addressed Gen2-Audit package + hashes
```

If any exact-head gate fails, fix only the demonstrated regression and restart validation from the new SHA. Do not reuse an older green run.

## Safety invariants

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
PARTY MUTATION: DEFERRED WHERE NOT SEPARATELY PROVEN
STAGED/VERIFIED EXPORT WORKFLOW: REQUIRED
```

## Stop condition

Once one exact candidate SHA is fully green and its CI-built `PokeBank-NX-Gen2-Audit-<shortsha>.nro` is independently verified/materialized, stop coding and hand that exact NRO to the owner for physical Gold/Silver/Crystal testing. Keep PR #68 OPEN / DRAFT / NOT MERGED and do not declare Gen II device accepted until the owner reports the physical result.
