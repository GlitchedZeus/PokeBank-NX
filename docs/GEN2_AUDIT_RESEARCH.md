# Gen II audit: exact native domains and final remediation evidence

Last updated: 2026-09-14

This document records the Generation II audit research and the implementation boundaries used by PR #68. It is not hardware acceptance evidence.

## Held items

Primary reference: [PKHeX ItemStorage2 at 77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5](https://github.com/kwsch/PKHeX/blob/77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5/PKHeX.Core/Items/ItemStorage2.cs).
`GetAllHeld()` explicitly combines General, Balls, and the first 50 Machine entries (TMs).
The committed numeric domain in `Gen2HeldItems.h` contains exactly those 192 IDs plus a separate None choice.
This domain is common to G/S/C; Crystal-only key items remain excluded, as do HMs, key/story items, placeholders and unused IDs.
The project item-name table supplies labels only; a name does not grant selectability.
Backend Create and explicit held-item changes use the same domain as the picker.
An unrelated edit preserves an existing unusual byte; View identifies it as Raw item N.
This is a held-item selection domain, not proof of complete encounter legality.

## Exact capabilities

The exact resolver checks game ID, platform, generation and current record/save format together and retains region/family/revision. Unknown or mismatched identities return no capabilities. Generation defaults remain only for established callers; exact identities win in shared presentation. This model does not enable modern editors or writes.

- RBY uses single Special, DVs 0–15 and Stat Exp.
- GSC uses split battle Special with one stored Special DV/Stat Exp; only Crystal exposes caught/met fields.
- [PB7 at the same pin](https://github.com/kwsch/PKHeX/blob/77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5/PKHeX.Core/PKM/PB7.cs) stores six AV bytes and adds them in its stat calculation. LGPE is represented as IV + AV (0–200), not generic EV editing.
- [PA8 at the same pin](https://github.com/kwsch/PKHeX/blob/77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5/PKHeX.Core/PKM/PA8.cs) uses Ganbaru values and its own stat calculation. PLA is represented by an explicit effort-level model (0–10), not generic Gen VIII EV controls.
- SV has its distinct PK9 identity with modern IV/EV presentation. Future format/revision exceptions belong in the exact resolver, not broad-generation inference.

## Crystal caught/met packed data

The Crystal-only two-byte caught data is decoded according to the proven Generation II format instead of displayed as an opaque integer. Presentation exposes:

- time of day;
- caught/met level;
- original-trainer gender bit;
- caught/met location.

The raw 16-bit field remains preserved in the parsed/staged record. This remediation adds presentation/interpretation; it does not authorize source writing. Gold/Silver capability rules continue to hide the Crystal-only caught/met presentation.

## Party-only native data

Stored PK2 Party records contain native state not present in a boxed PK2 record. The passive View now conditionally shows:

- Current HP;
- Max HP;
- status condition;
- the six stored party battle stats.

Box View continues to use calculated read-only battle stats and does not fabricate Party Current HP/status.

## Species / Move / Pokérus controls

The shared Gen II editor no longer requires normal users to enter raw Species or Move IDs:

- Species is a named 1–251 picker;
- Move is a named 0–251 picker with Empty and exact-game compatibility/preservation state;
- Pokérus is edited as user-facing State / Strain / Days while retaining exact byte encode/decode semantics.

Behavioral tests prove picker browsing does not mutate the working transaction, selecting Species re-synchronizes growth/Level/EXP/derived gender state, selecting a new Move resets PP/PP Ups correctly, Empty zeroes PP/PP Ups, and re-selecting an unusual existing Move preserves its unusual stored PP.

## Passive View touch foundation

Gen I and Gen II action-sheet passive View now expose a rectangular touch Back target routed through the same `passiveBackTarget` action model used by controller B. This is a contained reusable touch foundation only.

Full touch-only handheld operation remains issue #55 and is not claimed complete here.

## Verified staged-export transaction

Generation II now has one authoritative app-owned staged export flow. The real GSC Review -> Export UI delegates to `publishVerifiedStagedEditorExport()`; it does not contain an independent direct-write sequence.

The transaction:

1. finalizes staged bytes through `StagedEditor`;
2. strictly parses the finalized in-memory edited bytes before touching disk;
3. hashes original and edited bytes with SHA-256;
4. creates a unique temporary directory under the app-owned Gen II export root;
5. writes `original_backup.srm` and `edited.srm`;
6. flushes and `fsync`s file descriptors where supported, then closes them;
7. reopens and reads both files back from disk;
8. compares exact bytes and SHA-256 values against the intended buffers;
9. strictly parses the actual edited bytes read from disk;
10. writes `EDIT_MANIFEST.txt` with game/source/save-format/source-size/source-hash/edited-size/edited-hash/timestamp/application-SHA provenance and explicit live-write-disabled declarations;
11. publishes the complete directory by same-filesystem rename only after every verification succeeds;
12. removes incomplete temporary files/directories on failure.

The live UI reports success only after the verified directory has been published. On failure it reports the specific transaction failure and states that the source remains unchanged.

### Export regression coverage

The permanent G/S/C test runs the same staged-editor-level abstraction used by the UI and verifies:

- valid staged export succeeds;
- `original_backup.srm` exists and equals original source bytes;
- `edited.srm` exists and equals expected finalized bytes;
- `EDIT_MANIFEST.txt` exists;
- game ID and source identity are recorded;
- source size and SHA-256 are recorded;
- edited size and SHA-256 are recorded;
- application SHA is recorded;
- both `LIVE_RETROARCH_WRITE=DISABLED` and `LIVE_INSTALLED_GAME_WRITE=DISABLED` are recorded;
- the disk edited save strictly reparses;
- injected partial-export failure cleans temporary output;
- a corruption at `0x2009`, inside the checksum-covered payload for both G/S and Crystal test fixtures, fails strict parse and is refused before export publication;
- original `ReadOnlySave` bytes and `StagedEditor::originalBytes()` remain unchanged.

A separate surface guard forbids `std::fopen`, `std::fwrite`, `writeBytes(` and `writeText(` from returning to the GSC overlay while requiring the verified staged-export delegation marker.

## Deferred global audit items

These are intentionally not absorbed into the final Gen II device gate:

```text
Full touch-only v1: DEFERRED -> issue #55
Global box/controller normalization: DEFERRED -> issue #26
Legacy Storage / Master Vault / true Move architecture: DEFERRED -> issue #27
```

Issue #26 requires a dedicated controller semantics pass and physical retest. Issue #27 explicitly classifies inherited `PKSEBANK` / `bank.dat` Storage as app-owned mutable legacy storage, not the future immutable/provenance Master Vault and not true destructive Move semantics.

## Final validation boundary

The exact PR head after the final documentation refresh must pass `.github/workflows/gen2-audit-candidate.yml`. That workflow is the source-addressed proof for focused Gen II behavior, export safety, Gen I/III/inventory regressions, source immutability, full host, ASan/UBSan, complete RomFS recovery, clean devkitA64 build/link, native AArch64 linkage, embedded application SHA, embedded RomFS and final `PokeBank-NX-Gen2-Audit-<shortsha>` packaging.

Passing automation means **CI VERIFIED / DEVICE TEST PENDING**, not DEVICE ACCEPTED. Physical Gold/Silver/Crystal testing by the owner remains mandatory before PR #68 can be considered for merge.
