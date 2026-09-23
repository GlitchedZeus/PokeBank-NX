# PokeBank NX — Current Full-Project Audit Status

Date: 2026-09-23

This is the first exact-current deliverable for issue #69. It audits forward from the physically accepted Gen I–III editor checkpoint and does not transfer device acceptance to this branch.

## Exact repository map

```text
Default/docs branch:
main @ 6fa94ba360d36c6cc704f072fbf34a5bb43f5b16

Production branch:
feature/pokebank-playable @ 3aeecb8061a3cf8cf19670c042c5ce4cd06330fb

Device-accepted Gen I–III branch / PR:
feature/gen3-shared-pokemon-editor-20260919
PR #77 — OPEN / DRAFT / NOT MERGED
accepted application SHA:
996e6aa40c96e4408282f3d55476dae8e64968b2
accepted tree:
8826147ff5dc1b498b4b8505c9212243ed2f9498

Audit branch:
audit/full-project-hardening-20260923

Audit PR:
#79 — OPEN / DRAFT / NOT FOR MERGE YET
base:
feature/gen3-shared-pokemon-editor-20260919
```

The accepted Gen III NRO remains:

```text
PokeBank-NX-Gen3-SharedEditor-996e6aa4.nro
SHA-256:
ac3f6bd03d2a6733aee509729b81b6636cabe836095715c7b575b5dc84c8076c
```

No live source write lock has been changed.

## Open PR classification

### PR #77 — ACTIVE / DEVICE-ACCEPTED HISTORY / KEEP OPEN

Generation III accepted implementation line. Do not merge until explicitly instructed.

### PR #79 — ACTIVE AUDIT / DRAFT

Current safety-hardening line. Starts exactly from the accepted #77 SHA.

### PR #53 — SUPERSEDED / SAFE TO CLOSE WITHOUT MERGE

`WIP: Generation II GSC strict read-only support` predates the accepted Gen II/III work. Its branch history should be preserved. Close the PR only as repository housekeeping; do not delete the branch/history without owner approval.

## Branch organization

The repository has roughly five dozen retained branches. They currently fall into these categories:

- **ACTIVE:** `main`, `feature/pokebank-playable`, `feature/gen3-shared-pokemon-editor-20260919`, `audit/full-project-hardening-20260923`.
- **DEVICE-ACCEPTED / ARCHITECTURE HISTORY — KEEP:** accepted Gen I/II editor lines, packed-move lines, issue #71 architecture-audit/freeze lines, recovery/checkpoint lines.
- **SUPERSEDED CI/DEV HISTORY — KEEP FOR NOW:** the many 2026-09-12/13 `ci/*`, `dev/*`, older Gen II, standalone, inventory, and hardware-polish branches.
- **POSSIBLE FUTURE DELETE — OWNER APPROVAL REQUIRED:** obvious temporary lines such as `tmp-do-not-use`, `tmp-rby-adapter`, copy-probe branches, after their unique commits are proven reachable/preserved elsewhere.
- **RECOVERY ASSETS/BRANCHES:** preserve. Do not delete during this audit.

No destructive branch cleanup is authorized.

# A01–A09 exact-current disposition

## A01 — P1 — CONFIRMED — Bank persistence is not atomic/durable

Files:
- `src/Trainer/Bank.cpp`
- `include/Trainer/Bank.h`

`Bank::save()` still:

```text
serialize
verifyImage (failure does not block)
fopen(authoritative bank.dat, "wb")
fwrite
fclose (result ignored)
check only byte count
```

There is no temp generation, checked flush/close, reopen verification, prior-generation preservation, promotion, or deterministic recovery.

## A02 — P1 — CONFIRMED — failed return-to-origin can destroy held Pokémon

File:
- `src/UI/TrainerViewScreenBase.inc`

Accepted implementation attempted placements and then unconditionally called `moveMon.clear()`. A non-null unplaced `unique_ptr` was therefore destroyed.

### Test-first hardening status

Regression added first on PR #79.

Fix now changes rollback to:

```text
plan every return destination
reserve unique empty/unlocked slots
if any carried Pokémon cannot be planned:
    move nothing
    retain full custody
else:
    commit every planned return
    clear carry state
```

This is the first audit fix and still requires exact-head CI completion.

## A03 — P1 — CONFIRMED — conversion replaces custody too early

File:
- `src/UI/TrainerViewScreenBase.inc`

`convertForPane(std::unique_ptr<...>& pk,...)` explicitly converts in place and on success does:

```cpp
pk = std::move(converted);
```

That replaces the authoritative carried representation before final destination placement/durable commit.

Required redesign: immutable custody payload + separate destination candidate until commit.

## A04 — P1 — CONFIRMED — Bank and save are not one transaction

Files:
- `src/UI/TrainerViewScreenBase.inc`
- `src/Trainer/Bank.cpp`
- `src/Save/GetSaveFileContents.cpp`

Game-backup save and Bank save are explicitly separate user decisions and persistence paths. There is no shared journal/state machine.

The current UI even documents a bank-only discard case where a withdrawn copy can remain in the save workspace. That is acceptable only as current legacy staged/copy-like behavior; it is NOT approved future true-Move semantics.

True Move remains blocked until destination-first durable transaction recovery exists.

## A05 — P1 — CONFIRMED — unreadable Bank casualty can overwrite prior evidence

File:
- `src/Trainer/Bank.cpp`

Current load recovery uses fixed `bank.dat.unreadable` and removes the previous casualty before renaming the next one.

Required: unique immutable casualty generations; never remove the only recovery evidence.

## A06 — P1/P2 — CONFIRMED — truncated BDSP can reach unchecked party-count offset

Files:
- `src/Trainer/Trainer8BDSP.cpp`
- `include/Trainer/Trainer8BDSP.h`

Constructor calls parsers sequentially. `parseMyStatus()` can return on short input, then `parseParty()` immediately indexes `saveData[BDSP_PARTY_COUNT]` without its own whole-layout guard.

Required: constructor/layout validity gate before any fixed-offset parser plus exact-boundary/one-byte-short ASan fixtures.

## A07 — P1/P2 — CONFIRMED — larger Bank can be partially loaded then truncated

Files:
- `src/Trainer/Bank.cpp`
- `include/Trainer/Bank.h`

If `fileBoxes > BANK_BOX_COUNT`, code logs that extra boxes will be lost if saved, but leaves the Bank writable. No read-only/migration-required state exists.

Required: preserve the full file and block save until safe migration.

## A08 — P1/P2 — CONFIRMED — mutable backups are not account namespaced

Files:
- `src/Utils/FileUtilities.cpp`
- `include/Utils/PokeBankPaths.h`
- `src/UI/BackupSelectionScreen.cpp`
- `src/UI/SaveSelectScreen.cpp`

Installed save/source discovery is UID aware, but backup paths are currently:

```text
backups/<sanitized title>/<timestamp-or-Working>
```

even though `backupSaveData()` receives `AccountUid`.

Required destination:

```text
backups/<profile-id>/<exact-game-id>/<workspace>
```

Migration must preserve all old unscoped workspaces.

## A09 — P1/P2 — CONFIRMED — mutable backup save files are overwritten in place

File:
- `src/Save/GetSaveFileContents.cpp`

Direct `fopen(...,"wb") / fwrite / fclose` remains in LGPE, SwSh, BDSP, PLA, S/V, Z-A and FRLG backup writers.

BDSP writes `SaveData.bin` and `Backup.bin` sequentially, so the pair can advance only halfway.

Required: common durable replacement primitive; multi-file saves need a generation journal.

# Additional current findings

## N01 / F07 — P1/P2 — CONFIRMED — Gen III downgrade PID search silently falls back

File:
- `src/Conversion/Convert.cpp`

Modern/PK8-layout -> PK3 attempts up to 1,000,000 candidate PIDs to preserve Gen III-derived nature, gender, shiny status and ability slot.

The code explicitly leaves `outPid = original pid` if no candidate is found and still completes conversion.

That violates the audit contract:

> If required semantics cannot be preserved, fail explicitly.

Required: return a conversion failure instead of silently producing changed correlated traits.

## N02 — P2 — CONFIRMED — absurd Bank box count is coerced instead of failed closed

File:
- `src/Trainer/Bank.cpp`

A `fileBoxes` value of 0 or >4096 is replaced with the current build's `BANK_BOX_COUNT`. A damaged/unknown header should instead be quarantined/fail closed, not reinterpreted as the current layout.

## N03 / F10 — P2 — NEEDS FIXTURE/POLICY — Gen III EV 253–255 into modern

File:
- `src/Conversion/Convert.cpp`

Gen III EV bytes are copied directly into the modern hub layout. Gen III can store values above the later 252 per-stat legality cap. Build fixtures at 252/253/255 and decide whether normal conversion clamps/refuses while explicit illegal-edit mode preserves.

## N04 / F11 — P2 — NEEDS UI-LOSS AUDIT — custom nickname loss on modern -> Gen III

File:
- `src/Conversion/Convert.cpp`

The PK8 -> PK3 remap intentionally writes the uppercase species name instead of carrying a custom nickname down.

If this loss is intentional, it must be declared before commit and recorded in provenance. It must not be a silent transfer loss.

# Conversion audit status

Initial exact-current review:

- F05 shiny preservation: code contains explicit boolean shiny-preservation logic across the Gen III threshold difference; needs golden fixtures.
- F06 Unown form: current code explicitly derives/preserves PID-based Unown form; needs round-trip fixture.
- F07 PID search exhaustion: **confirmed defect** — silent fallback.
- F08 ability slot/number: explicit mappings exist; hidden-ability/down-convert policy needs fixtures.
- F09 S/V ↔ Z-A divergent/Tera fields: normalization exists; golden fixtures still required.
- F10 Gen III EV policy: needs fixture/policy; raw copy confirmed.
- F11 nickname semantics: down-convert loss exists; warning/provenance behavior needs audit.
- F13 profile/account namespacing: **confirmed by A08**.

No conversion pair should be called proven until source bytes, destination reparse, identity traits, checksum and declared losses are fixture-verified.

# Parser audit status

Initial fixed-offset scan:

- BDSP A06 is confirmed.
- S/V and Z-A current-box scalar access is guarded against empty block data.
- FRLG uses sector-table validation/bounded logical sector reads; no new P1 was proven in the first scan.
- SwSh/PLA direct scalar writes reviewed so far use block-size guards or block abstractions.
- Full malformed-count/size-multiplication/unknown-revision audit remains incomplete.

No P0 was found in this first pass.

# Move / Copy / Clone architecture assessment

Permanent target contract:

```text
MOVE
same Pokémon identity
one active location
destination first
source retired only after durable verified destination

COPY
explicit duplicate to another active location
source remains active

EXACT CLONE
native Pokémon payload byte-identical where format permits
new PokeBank/Vault entity ID
EXACT_CLONE_OF parent relationship

DERIVED / LEGAL CLONE
new PokeBank/Vault entity ID
DERIVED_FROM relationship
generation-aware regenerated identity-linked values
never claim historical encounter provenance that did not occur

ARCHIVE
hash/raw evidence/provenance only
not active
not withdrawable
not counted as another active Pokémon
```

Current legacy Gen II staged `Clone` is a source-unchanged box clone, but it has no future Vault entity/provenance model yet. Do not label it a final Vault Exact Clone until byte identity and separate Vault-ID lineage are proven.

Current Bank/Storage movement is not yet safe true-Move custody. A03/A04 block that claim.

# Master Vault readiness

**NOT READY to become authoritative storage for unique Pokémon.**

Blockers:

- A01/A05/A07 durable Bank persistence/recovery
- A02/A03 custody correctness
- A04 cross-store transaction journal
- A08 profile ownership/namespacing
- A09 durable mutable-save generations
- conversion golden fixtures
- corruption quarantine and schema migration rules
- active-location vs archival-history enforcement

Vault implementation should consume the durable primitive built in this audit, not create another persistence mechanism.

# Test / CI organization

Current accepted tree contains:

```text
13 GitHub workflow files
11 Makefile / Makefile.host fragments
72 C++/header test files
```

There is substantial historical duplication across Gen I cleanup candidates, Gen II audit candidates, packed-move focused workflows, inventory hardware retests and the Gen III candidate gate.

Disposition: **P3 maintenance debt, not deletion work yet.**

First preserve safety coverage, then consolidate common build/test primitives. Do not delete a candidate gate until its unique assertion/artifact responsibility is mapped to a permanent replacement.

# Recommended repair order

1. **A02 custody rollback** — Small/Medium — test-first fix in progress.
2. **A06 BDSP whole-layout guard** — Small — executable ASan regression.
3. **A07 + N02 Bank fail-closed/read-only state** — Medium.
4. **A05 recovery generation preservation** — Medium.
5. **A01 shared durable single-file replacement primitive** — Large.
6. **A09 migrate mutable backup writers to the durable primitive** — Large; BDSP multi-file journal is separate.
7. **A08 profile/exact-game backup namespace + non-destructive migration** — Medium/Large.
8. **A03 immutable custody + separate destination candidate** — Large.
9. **A04 cross-store destination-first transaction journal/recovery** — Very Large.
10. **F05–F13 golden conversion matrix, beginning with F07** — Very Large.
11. **whole-parser malformed-input hardening** — Large.
12. **physical Switch FAT32/exFAT/recovery test matrix** — Large.
13. **Master Vault persistence/migration** — Very Large and blocked until the above storage guarantees exist.

# Main files/functions expected to change

```text
src/UI/TrainerViewScreenBase.inc
include/UI/TrainerViewScreenBase.h

src/Trainer/Bank.cpp
include/Trainer/Bank.h

src/Save/GetSaveFileContents.cpp
include/Save/GetSaveFileContents.h

src/Utils/FileUtilities.cpp
include/Utils/FileUtilities.h
include/Utils/PokeBankPaths.h

src/UI/BackupSelectionScreen.cpp
include/UI/BackupSelectionScreen.h
src/UI/SaveSelectScreen.cpp

src/Trainer/Trainer8BDSP.cpp
include/Trainer/Trainer8BDSP.h

src/Conversion/Convert.cpp
include/Conversion/Convert.h

tests/*
Makefile.host*
.github/workflows/*
```

Likely new shared components later:

```text
DurableFile / DurableGeneration replacement service
transaction journal/state machine
profile-aware workspace path helper
conversion golden fixture helpers
```

Do not implement the full Vault until these primitives are reviewed and physically recovery-tested.
