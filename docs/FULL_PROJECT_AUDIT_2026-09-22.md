# CURRENT CHECKPOINT — 2026-09-23

The hardware gate that originally blocked this audit is complete.

```text
Gen I / II / III editor milestone:
DEVICE ACCEPTED

Accepted Gen III application SHA:
996e6aa40c96e4408282f3d55476dae8e64968b2

Tree:
8826147ff5dc1b498b4b8505c9212243ed2f9498

NRO SHA-256:
ac3f6bd03d2a6733aee509729b81b6636cabe836095715c7b575b5dc84c8076c

PR #77:
OPEN / DRAFT / NOT MERGED
```

**This audit is now the active engineering phase.** The previous wording below that says “wait for the current hardware test” is historical context from 2026-09-22; that gate has now passed.

Immediate order:

```text
repository/evidence organization
-> A01-A09 reproduction tests
-> durable Bank/backup replacement primitive
-> custody/transaction hardening
-> exact-current conversion re-audit
-> malformed-input parser hardening
-> physical Switch recovery testing
-> Master Vault persistence/recovery
-> broader feature expansion
```

Do not begin Gen IV/DS/3DS or authorize live source writes before this safety gate is complete.

---

# PokeBank NX — Full Project Audit / Recovery Map

Date: 2026-09-22

This document is the durable audit/recovery map for the project while the current PR #77 candidate is undergoing physical Switch testing.

It is intentionally **documentation-only**. It does not modify the candidate branch, source code, build inputs, or tested NRO.

## Exact candidate under test

```text
Repository:       GlitchedZeus/PokeBank-NX
PR:               #77
Branch:           feature/gen3-shared-pokemon-editor-20260919
Application SHA:  2e4780412377abab3ffbe4fc2e4757339214a90f
Tree SHA:         cdbf0b8faae901c8d765d06b95cac3140bd69e75
PR state:         OPEN / DRAFT / NOT MERGED

NRO:
PokeBank-NX-Gen1-UX4-Retest-2e478041.nro

NRO size:
161,984,241 bytes

NRO SHA-256:
32b08c1cf589252022e68bf50fe0847fea7cbf1b86835178e7d2a2c268e3b43c

Authoritative candidate workflow:
Gen I Cleanup3 Candidate Gate
Run: 35684844741

Artifact:
Gen1-UX4-Retest-Candidate
Artifact ID: 10677006389
Artifact size: 309,160,016 bytes
Artifact digest:
sha256:af3f8ff553463f9d84b1963f813b4ca19aecd518038facf6c897636cf807bb85

Status:
CI VERIFIED
DEVICE RETEST REQUIRED
NOT DEVICE ACCEPTED
```

All eight pull-request-triggered workflows returned success for this exact application SHA:

```text
Gen I/II Packed Move Focused          35684844770  SUCCESS
Gen I/II Packed Multi-Move Focused    35684844728  SUCCESS
Gen I Cleanup3 Focused Gates          35684844735  SUCCESS
PokeBank NX Host Tests                35684844757  SUCCESS
Gen II Shared Editor Candidate Gate   35684844766  SUCCESS
Gen III Shared Editor Candidate Gate  35684844731  SUCCESS
Gen I Editor Candidate Gate           35684844724  SUCCESS
Gen I Cleanup3 Candidate Gate         35684844741  SUCCESS
```

The current hardware test is specifically authoritative for the exact NRO/hash above. Do not transfer a PASS or FAIL to a later SHA automatically.

---

# Non-negotiable safety invariants

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
UNKNOWN SAVE VARIANTS: FAIL CLOSED
DEVICE ACCEPTED: exact physically accepted artifact only
```

The audit may identify defects in PokeBank-owned Bank/Vault/staging code without weakening those source-write locks.

---

# Current confirmed safety findings

These are source-level findings verified against the exact PR #77 candidate above. They are **not being fixed during the current physical test**, because any source change would create a new candidate SHA.

## A01 — P1 — Bank persistence is not crash-safe / atomic

File: `src/Trainer/Bank.cpp`, `Bank::save()`.

Observed behavior:

1. serialize the full bank;
2. run `verifyImage(buf)`;
3. verification failure is recorded but deliberately does **not** abort;
4. open the authoritative `bank.dat` directly with `fopen(..., "wb")`;
5. write with `fwrite`;
6. call `fclose` without checking its result;
7. no temporary generation + durable flush + verified rename transaction.

Risk:

- power loss, SD removal, filesystem failure, short write, or close/flush failure can destroy/truncate the only current bank image;
- a failed in-memory verification can still be written;
- the file is mutated before a replacement is known durable.

Required direction after current Gen I–III hardware acceptance:

```text
serialize
-> strict verification MUST pass
-> write unique temp generation
-> flush / close and check every result
-> re-open / verify written bytes
-> preserve previous known-good generation
-> atomic/replace-style promotion where filesystem semantics permit
-> deterministic crash recovery
```

Test the actual Switch SD-card filesystem behavior, not just host POSIX semantics.

## A02 — P1 — Held Pokémon can be destroyed on failed return-to-origin

File: `src/UI/TrainerViewScreenBase.inc`, `returnHeldToOrigin()`.

Observed behavior:

- each carried Pokémon first tries its original cell;
- if occupied, it scans for another empty cell in the origin pane;
- if no destination is available, the `unique_ptr` remains in `moveMon[i]`;
- after the loop, `moveMon.clear()` is unconditional.

Risk:

If the original pane is full and the original position was displaced/filled, an unplaced Pokémon can be destroyed when the held vector is cleared.

Required invariant:

> A Pokémon in custody may never be released from memory until a destination or exact rollback location is durably/atomically established.

Cancellation/rollback must fail closed while retaining custody if no safe location exists.

## A03 — P1 — Conversion occurs before placement is committed

File: `src/UI/TrainerViewScreenBase.inc`, `convertForPane()`.

Observed behavior:

- a foreign held Pokémon is converted for the destination save;
- on success the held pointer itself is replaced with the converted entity;
- the conversion therefore mutates the object in custody **before** final placement/durable transaction completion.

Risk:

A later rejected placement or cancellation can return a converted descendant to the origin instead of the exact original representation.

Required direction:

- retain immutable source/custody payload;
- derive a destination candidate separately;
- only promote destination state after validation and durable commit;
- cancellation restores the exact original payload/identity.

## A04 — P1 — Bank + destination save are not one durable transaction

Observed across `TrainerViewScreenBase.inc`, Bank persistence, and backup-save flow.

The Bank has its own `Bank::save()`; game backup saving is performed separately through `Save::saveTrainerInfo(...)`.

Risk:

A logical Move spanning Bank and save can persist only one side before a crash/failure, producing duplication, loss, or ambiguous custody.

Required direction:

Implement a transaction journal/state machine before true Move is considered trustworthy:

```text
PREPARED
DESTINATION_WRITTEN
DESTINATION_VERIFIED
SOURCE_RETIRE_PENDING
SOURCE_RETIRED
COMMITTED
```

Recovery must be idempotent from every state.

## A05 — P1 — Recovery can discard previous unreadable-bank evidence

File: `src/Trainer/Bank.cpp`, unreadable-file handling.

Observed behavior:

```cpp
std::remove(aside.c_str());
std::rename(path.c_str(), aside.c_str());
```

where `aside` is the fixed `bank.dat.unreadable` path.

Risk:

A second unreadable/corrupt bank can delete the first preserved casualty before the new one is successfully retained.

Required direction:

- never overwrite/delete the only recovery evidence;
- use unique generation/timestamp/hash-qualified casualty names;
- maintain an explicit recovery inventory;
- only prune after at least one independently verified good generation exists.

## A06 — P1/P2 — Short BDSP input can reach unchecked party-count read

Files:

- `include/Trainer/Trainer8BDSP.h`
- `src/Trainer/Trainer8BDSP.cpp`

Observed behavior:

- constructor calls `parseMyStatus(); parseParty(); ...`;
- `parseMyStatus()` returns early on a too-short buffer;
- constructor continues;
- `parseParty()` immediately reads `saveData[BDSP_PARTY_COUNT]`;
- `BDSP_PARTY_COUNT` is an absolute fixed offset.

Risk:

Malformed/truncated input can trigger an out-of-bounds read before the later per-slot checks.

Required direction:

- validate the minimum layout/revision once before any fixed-offset parser runs;
- fail construction/import cleanly;
- add exact-boundary and one-byte-short fixtures under ASan/UBSan.

## A07 — P1/P2 — Larger Bank format can be loaded partially and later truncated

File: `src/Trainer/Bank.cpp`.

Observed behavior:

When a bank file declares more boxes than the current build supports, load logs that extra boxes will be lost if saved, but the supported prefix remains usable.

Risk:

A normal later save can destroy data belonging to unsupported higher boxes.

Required direction:

- mark the bank session read-only / migration-required when the input has unsupported capacity;
- preserve the full foreign/newer file;
- never allow a save operation that knowingly truncates unseen records.

## A08 — P1/P2 — Backup workspaces are not namespaced by Switch account

Files:

- `include/Utils/PokeBankPaths.h`
- `src/UI/BackupSelectionScreen.cpp`
- `src/UI/SaveSelectScreen.cpp`

Observed behavior:

- installed-save discovery correctly enumerates `FsSaveDataType_Account` entries and filters them by the selected `AccountUid`;
- legacy filesystem saves use an explicit persistent profile binding;
- but the PokeBank-owned backup UI constructs the workspace directory as:

```text
sdmc:/switch/PokeBank-NX/backups/<sanitized title name>/
```

with no account UID in that path.

Risk:

Two Switch users with the same Pokémon title share one backup namespace. A user can therefore see/select/delete/edit backup workspaces created from another profile, and generic `Working`/timestamp naming can collide semantically even if source discovery itself was profile-correct.

Required direction:

```text
backups/<profile-id>/<exact-game-id>/<timestamp-or-working>/
```

Migration must preserve every existing unscoped backup and require explicit ownership assignment when it cannot be inferred safely.

Do not silently move/delete old backups.

## A09 — P1/P2 — Mutable backup save files are also overwritten in place

File: `src/Save/GetSaveFileContents.cpp`.

Observed across LGPE / SwSh / BDSP / PLA / SV / Z-A / FRLG backup writers:

```text
fopen(destination, "wb")
fwrite(...)
fclose(...)   // close result not checked
then only the fwrite byte count is checked
```

BDSP additionally writes `SaveData.bin` and `Backup.bin` sequentially, so a failure between the two can leave the pair inconsistent.

Risk:

Although these are PokeBank-owned mutable backup workspaces rather than original installed saves, a crash, SD removal, disk-full condition, or close/flush failure can corrupt the working copy. If the user has made significant staged edits since the last immutable copy, those edits can be lost. A partially updated multi-file save can also become internally inconsistent.

Required direction:

Use the same durable replacement primitive as Bank/Vault storage:

```text
write temp
-> flush/close/check
-> re-open/reparse/checksum validate
-> preserve prior generation
-> promote atomically/transactionally
```

For multi-file saves, journal the set so all required files advance as one logical generation.

## Existing internal safe-write pattern worth reusing

File: `src/Legacy/LegacySourceBindings.cpp`.

This subsystem already implements a much safer replacement sequence than `Bank::save()` and the backup save writers:

- write `.tmp`;
- check `fwrite`;
- `fflush`;
- `fsync(fileno(...))`;
- check `fclose`;
- re-open and validate the temporary bytes;
- preserve the prior primary as `.bak`;
- promote temp to primary;
- validate the promoted target;
- rollback on promotion/validation failure.

Do not duplicate three different persistence strategies. After the current hardware gate, extract/generalize the proven pieces into one PokeBank-owned durable replacement/transaction layer and then use it for Bank, profile bindings, editable backup workspaces, and later Vault metadata/payload indices.

Still audit actual Switch/FAT32/exFAT rename + directory-durability semantics before calling this fully power-loss safe.

---

# Prior audit findings requiring exact-current revalidation

An earlier independent static review of production snapshot `3aeecb8061a3cf8cf19670c042c5ce4cd06330fb` reported the following. Some conversion code has changed since then, so these are **not automatically carried forward as current defects**.

They remain mandatory audit questions:

```text
F05  Gen III -> modern shiny-threshold preservation
F06  modern -> Gen III Unown form preservation
F07  PID-search exhaustion / silent incompatible fallback
F08  ability selector/ability-number masking
F09  Z-A -> S/V Tera-field normalization
F10  Gen III stored EV max 255 vs modern 252 legality policy
F11  Gen III -> modern nickname flag semantics
F13  account/profile namespacing of mutable PokeBank-owned data
```

Current source already contains explicit work around several of these areas (for example Gen III form handling and PK8/PK9 field remaps), so each must be re-proven with fixtures rather than assumed broken from the old report.

---

# Correctness audit still required

## Gen I / II / III editors

For every visible editable field:

```text
UI value
-> staged model
-> native serializer
-> checksum/finalization
-> reparse
-> exact displayed value
```

No control may be “fake” (changes on screen but is omitted from serialization).

Specifically re-check:

- Create and Edit separately;
- per-field ranges;
- cancel/discard exact restoration;
- derived vs stored fields;
- exact-game move/item/location lists;
- PID-correlated fields;
- PP/PP Ups constraints;
- Gen I HP DV derivation;
- Gen II one-Special-DV/Stat-Exp semantics;
- Gen III IV/EV/met-level persistence;
- Pokérus state UI vs raw native byte preservation;
- exact source game origin/version.

## Conversion

Build golden conversion matrices for every implemented source/destination pair.

Required properties:

- source object remains byte-identical;
- species/form preserved or transfer explicitly refused;
- shiny preserved unless a documented intentional policy says otherwise;
- gender/ability/nature correlated fields remain internally consistent;
- nickname and language flags remain semantically correct;
- HOME tracker policy explicit;
- held item/move loss explicit;
- Unown and other PID-derived forms preserved;
- destination checksum valid;
- destination reparses to the expected values;
- no hidden “best effort” fallback silently produces a different Pokémon.

## Save parsers

Every fixed-offset parser needs:

- minimum length before first indexed read;
- exact revision/layout validation;
- integer overflow/size multiplication guards;
- malformed count bounds;
- truncated party/box/item fixtures;
- unknown-version fail-closed behavior;
- ASan/UBSan coverage.

## PokeBank-owned mutable storage

Before the Master Vault is trusted with unique Pokémon:

- atomic/durable generation replacement;
- crash recovery at every write phase;
- no fixed single `.bak`/casualty overwrite;
- content hashes;
- versioned schema;
- migration rollback;
- profile/account namespacing;
- corruption quarantine;
- journal replay;
- duplicate detection;
- immutable original payload preservation.

## File/path safety

Current named-backup flow already sanitizes user-entered backup folder names to a restricted safe character set and suffixes existing names rather than blindly overwriting them.

Continue auditing:

- all user-provided names/paths;
- symlink/path traversal assumptions where applicable;
- recursive copy failure behavior;
- partial-directory cleanup;
- SD removal mid-copy;
- disk-full behavior.

## Build / artifact identity

Permanent candidate requirements:

- one exact source SHA;
- all required workflows from that SHA only;
- sanitizers;
- source-write lock tests;
- staged export safety;
- native devkitA64 compile/link;
- AArch64/readelf identity;
- RomFS/assets;
- embedded application SHA;
- deterministic manifest/hash evidence;
- exact CI artifact;
- exact NRO SHA-256;
- physical Switch result bound to that exact NRO.

---

# Repository/documentation audit

## Active authoritative line

```text
Production: feature/pokebank-playable
Active PR:  #77
Active dev branch:
feature/gen3-shared-pokemon-editor-20260919
```

Do not restart Gen III on another branch.

## Known stale/superseded line

PR #72 predates the issue #71 universal-editor architecture freeze and is superseded by the active PR #77 line.

Closing a superseded PR is safe repository housekeeping because it does not erase its commits/history. Do not delete the branch as part of cleanup.

## Documentation drift

Several historical status documents and the production-branch README still describe the old Gen II PR #68 phase. Preserve them as history unless intentionally refreshed on their owning branch; do not rewrite a currently tested candidate just to make prose current.

The default-branch README should be the public status page. This audit and `docs/REFERENCE_INDEX.md` are the durable deeper maps.

## Large recovery assets

The repository contains intentionally committed recovery snapshot parts under `recovery/assets_snapshot/`.

Do **not** delete them during cleanup. Treat repository-size optimization as a separate, explicit preservation task: first prove an equivalent recovery copy exists elsewhere and preserve hashes/history.

---

# External-reference / license audit

Use `docs/REFERENCE_INDEX.md` for the consolidated map.

Important current policy examples:

- PKSE: inherited AGPL foundation.
- PKSM-Core: vendored/reference/adapter candidate under GPLv3.
- PKSM / pkmn-chest: valuable GPLv3 bank/integration references.
- OpenHomeNX: valuable reference; do not inherit live-write policy.
- PKSM-Scripts: save/event research; scripts are not legality proof.
- pret/pokediamond: native Gen IV reference.
- Project Pokémon Pokemon-Legality-Checker: **reference only; no license file found** — do not copy source without explicit permission.
- EventsGallery: data redistribution/permission must be resolved before bundling.

Every directly reused external subsystem/data set needs an exact source revision and license/provenance record.

---

# Recommended repair order after current hardware gate

Do not execute this sequence until the owner finishes the current exact-NRO test.

```text
1. Finish physical Gen I/II/III candidate testing and fix only demonstrated editor regressions.
2. Freeze an exact accepted Gen I–III milestone.
3. Turn A01–A07 into regression/reproduction tests.
4. Build durable Bank/transaction/recovery primitives.
5. Re-run the prior conversion findings F05–F13 against the accepted source.
6. Harden malformed-input/save parser boundaries.
7. Re-test storage/recovery on physical Switch + SD filesystem.
8. Implement/harden Master Vault immutable-original persistence.
9. Physically test Vault recovery and provenance.
10. Only then expand into EventDex/Mystery Gifts, Living Dex, DS/3DS, broader legality/generation features.
```

---

# No-delete cleanup rule for this audit cycle

During the current hardware test:

- do not delete source;
- do not delete branches;
- do not delete recovery assets;
- do not rewrite history;
- do not reset/rebase backward;
- do not merge PR #77;
- do not modify the exact candidate branch merely for cleanup;
- prefer additive documentation, issue updates, and closing only clearly superseded PRs while preserving their commits/history.

This keeps the tested candidate reproducible while still making the repository understandable when development resumes.


---

# Documentation-only maintenance performed during hardware test

The following maintenance was intentionally performed on `main` / issue/PR metadata only. It did **not** change PR #77's candidate source SHA.

```text
README public status refresh                     2dd22fe6f9d38fcc6ab4f0f73b632949b404e21a
Reference index                                 77f45d88371f59b712fe24f5f10c49e631b1e428
Initial full-project audit record               4e1b9c3c5450d04074390c181f1485e33f5b653d
Current status recovery header                  d82c2208f245f315512eb4cbefd3625bb7641b7e
Project status recovery header                  33df759bf55dd18b87b65d4a6223eddabe1481b7
Next-session recovery plan                      2d881ee97e2fc669c8f74b3ec784e70326282225
Upstream audit -> reference-index pointer       1bb886862bba34caf679850c763ef87325466c37
Project map current recovery header             97f385f32d720a1011f1ea969adc230740d0bd84
Project resource index current recovery header  9ed08b2e283378b3d037d52e0f33cdb72249b600
Research current-index navigation refresh       918c073e668ee4ede52c9c320f9126d9f86b3e58
Research reference-matrix navigation refresh    d632dce409e476ee938ea9021fb4268c3d082e2c
Archived old PR #68 Codex prompt                0840e5f8a9411dda374c408eeaf6f062df44a34b
Current PR #77 Codex prompt                     af8fe5c7728b8a1fd8d96269d13617f541898c23
Codex active-branch recovery discipline         b5891869338fd33bde36cc3d22d659b77314330c
Recovery contract active-branch correction      b9f64d60b3b3718110d8ed9ae3c1c441e6875d84
Audit A08/A09 additions                         72564522b77db087fd0ab3f35ddf059af54ab6a5
```

Issue metadata:

- issue #69 updated with the current exact candidate + confirmed audit backlog;
- issue #29 updated with the current v1 checkpoint/critical path.

Pull-request metadata:

- PR #72 closed **without merge** as superseded by completed issue #71 / merged PR #75; its branch/commits/history were intentionally left intact;
- PR #77 body updated with exact current candidate identity/status only;
- PR #77 was not merged and its head was not changed by this documentation pass.
