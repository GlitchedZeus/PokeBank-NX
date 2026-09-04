# Session 2.6 safety implementation — 2026-09-04

Scope: #23 source-state UI safety, #24 defensive PLA opening, preserved #19 analog and
#13/#16 shell, and #37 existing RomFS packaging. No live-write enablement or new engines.

## Source state and persistence

The initial installed-title browse still reads an automatic backup, but `loadedFromCart`
now selects an enforced **read-only session**. Opening an existing backup row deliberately
selects a **mutable backup workspace**. The title bar identifies that state. `LIVE LOCKED`
describes the installed-save policy, not whether a backup or legacy bank is editable.

| Reachable operation | Final classification / guard |
| --- | --- |
| Action Sheet View, B, Cancel, navigation | IN-MEMORY ONLY; no Pokémon mutation |
| Installed-source Release/Create/Move/Multi/Edit/rename/items/trainer/Save | DISABLED / UNREACHABLE via source policy and entry/deferred-modal guards |
| Explicit backup Release/Create/Move/Edit/apply | IN-MEMORY ONLY until an explicit backup save |
| Backup Save / unsaved-changes confirmation | BACKUP/STAGED SAVE WRITE: `performSave` → `saveTrainerInfo` → game-specific writer, always `injectToTitle=false` |
| Details Apply / keep | IN-MEMORY ONLY; updates edit snapshot, not an installed title |
| Legacy Storage placement | IN-MEMORY ONLY until the legacy bank confirmation |
| Legacy Storage Save | APP-OWNED STORAGE WRITE: `Bank::save` → `sdmc:/PKSE/bank/bank.dat` (`PKSEBANK`) |
| Restore/inject into installed title | DISABLED / UNREACHABLE: `restoreBackupToTitle` checks the unchanged hard lock before `fsdevMountSaveData` / `fsdevCommitDevice` |

The whole initial read-only session includes legacy Storage: use an explicit backup workspace
for inherited bank editing. This avoids cross-pane mutation loopholes. Legacy Storage is **not**
Master Vault, immutable storage, guaranteed atomic storage, or product-level true Move. Its existing
format/path is unchanged so prior user data remains accessible. Backup edits never imply removal
from an installed title. The September 3 hardware report remains historical evidence, not a test
of this replacement.

Source guards also stop background compaction/current-box persistence during read-only browsing.
Action-sheet Edit is disabled by the shared dispatch model, including Party, Boxes and Bank targets.
Touch and deferred dialogs share the guarded mutation helpers. Backup workspace editor behavior is
preserved; unsupported action-sheet entries remain unsupported.

## PLA and shared-reader hardening

- Reject missing/non-regular, truncated and unexpectedly large PLA `main` before opening.
- Verify salted container hash; require complete SCBlock consumption, not a partial parse.
- Bound object/array decoding, reject unknown/zero-width array elements, avoid arithmetic overflow,
  and handle null block input. No malformed-input assertion abort.
- Require the trainer, party and pasture blocks consumed by the existing PLA parser, with the
  expected minimum layout sizes/types; reject duplicate keys.
- Validate nonblank PA8 slot checksums and supported species/nature ranges before constructing
  Pokémon objects. This is structural validation, **not legality** or a full revision whitelist.
- Bound UTF-16 string reads by complete code units and output capacity.
- Remove unaligned `uint16_t*` cipher access.
- Fix a concrete allocation mismatch: `readAllBytes` formerly returned `malloc` memory while all
  save readers and Bank used `delete[]`. Its extracted, host-testable implementation now uses
  `new[]` consistently; file-copy callers were updated too. Failed reads clear the reported size.

An invalid source returns an explanatory message to the backup selector. It is never repaired or
written. Container hash/complete-consumption checking also protects existing shared SC readers.
The exact failing older PLA save is unavailable here. **#24 stays open; DEVICE FIXED is not claimed.**
Synthetic acceptance tests are not evidence that every real historical PLA revision opens.

## Verification

Eight host suites include policy/Action Sheet regressions and synthetic PLA/container/file-reader
tests; the test loops now fail on the first failed binary instead of masking an earlier failure.
Existing game-identity, live-write-policy, theme, controller and analog-repeat suites are retained.
ASan/UBSan includes file allocation/deallocation and odd-length string cases.

Final source SHA, clean native build, sprite preflight, binary hash and hardware checklist belong
in `BUILD_RECORD.md` and the second-device checklist after packaging. No physical result is implied.
