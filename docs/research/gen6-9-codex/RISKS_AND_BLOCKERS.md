# Risks, unsupported cases and evidence gates

Research snapshot: 2026-10-18. Main base `bd6528149c41a2cf444e0a62afcaf5fc1bad2cad`.

Evidence vocabulary: **ORIGINAL** = inspected original project's source; **INDEPENDENT** = separate implementation or experiment (not game acceptance); **PKHEX** = inspected PKHeX source; **NX-TEST** = executed PokeBank test; **INFERRED** = proposed design/unverified consequence; **UNKNOWN** = evidence missing; **UNSUPPORTED** = deliberately disabled. These are distinct, not a single confidence score. Every production/hardware acceptance claim in this package is UNKNOWN unless explicitly attributed to historical evidence.

## Release and ownership blockers

- Gen I–V acceptance is not established by this session. #307 NRO audit fails candidate identity; #314 Gen V differential fails. Refresh exact heads before any October 20 transition.
- #309 mergeability is false against its current base. The #311/#312 stack contains older shared app changes. Carry reviewed native files forward into the then-current app lane; do not reset or wholesale replace shared UI code.
- Gen VI–IX code remains outside production integration on this branch. No #307/#314/main changes, merge or hardware claim.

## Critical format/export blockers

1. **SAV7:** reverseCrypt uninitialized-read/no-output defect reproduced. Initialized payload signing verifies but game acceptance unknown. Existing vendor shift UB/leak exemptions stay narrow and unchanged. Export remains disabled.
2. **BDSP:** two-file authority/fallback protocol unknown; transactional output alone does not prove game consistency. No single-file in-place rewrite.
3. **ZA:** PA9, not PK9. Box/party metadata gaps and revision-specific presence/footer changes need dedicated whitelist tests. Unknown DLC/revision/title IDs stay unsupported, including unverified Switch 2 equivalence.
4. **LGPE:** PB7 storage/pointer compaction/partner constraints and native AV/CP derivation; do not treat as SAV7 or a normal 30-slot box.
5. **PLA:** PA8, effort levels/mastery/alpha/noble/research state; do not reuse SWSH field semantics or 0x158 box stride.
6. Full title/revision species/forms/moves/items/events/transfer membership tables are not yet delivered as certified production tables. Numeric ceiling checks are insufficient. Unknown history remains incomplete.

## Engineering risks

Source parser success and reversible stage success do not establish safe export. Common legacy readers expose mutable buffers and legacy writers replace workspace files. Use new immutable source snapshot plus independently verified export bundle. Preserve quarantined records and unknown blocks. Party removal, current party stats/HP policy, battle team locks, fused references, daycare pointers, partner/starter restrictions and destination emptiness require native invariants.

Raw SC payload block wrapper types and scalar types differ. Current legacy source has fixes for wrongly requiring CurrentBox UInt32 when actual consumption is a byte; trust consumed geometry and blank-block data, not stale comments. Do not relax true scalar validation. Cross-family same-size entities need trusted native context.

A hash/checksum-valid malicious save is still possible: all checksums here are reproducible. Bound block counts, lengths, allocations, metadata arithmetic, source scan depth, string decoding and UI lists. Catch allocation failure at adapter boundary. Existing Gen8 encoder assumes internally valid blocks; constrain experimental caller inputs before exposing it to malformed data.

Licensing review must precede moving GPL adaptations into other components. No vendor patches or copying of ROM/event/game assets is included. Current source snapshots prove implementations, not retail acceptance. Historical decomp projects cannot verify Gen VI–IX game logic.

## Explicit unknown/unsupported register

UNKNOWN: retail save acceptance of every new export; exact BDSP multi-file winner; robust initialized SAV7 payload acceptance across both families/patches; comprehensive later patch/save sizes; Switch 2 parity; full original-game code evidence; complete modern encounter/event chronology; precise on-device memory/time; current Bank service availability; per-game Dekopon performance; independent PLA/SV/ZA parser oracle; full move/form/item extraction and actual edited-save load tests.

UNSUPPORTED: ORAS demo in native lane; SAV7 production export; ZIP/unknown archive backups; Azahar auto-discovery on Switch; arbitrary invented emulator roots; arbitrary unknown SC revisions; live installed-game/source writes; cross-game True Move; unreviewed Create/Trainer/Party operations; claiming legality from checksum alone.

## What evidence is actually needed from a device

No broad file upload is required to continue source engineering. Keep personal saves local. Supply family/game patch, provider/version, file names/lengths, CRC/hash/signature diagnostic categories, pre/post source hashes and test-copy game load/save/reopen result when the corresponding stage is ready. BDSP additionally needs paired-save behavior; SAV7 needs isolated signing construction tests. Original personal data and raw Trainer/OT strings should not enter this repository.

Reproduced additional blocker for complete Gen VII Bag support: Z-Ring item797 count2 is incorrectly rejected. Repair read/stage/verifier quantity policy together; add a regression that tests the exception while continuing to prohibit story-item Add.
