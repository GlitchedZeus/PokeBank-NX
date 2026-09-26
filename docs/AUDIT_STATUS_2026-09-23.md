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

## A08 hardening tranche — profile/account + exact-game backup namespaces

The pre-A08 installed-save backup layout was:

```text
backups/<sanitized-display-title>/<workspace>
```

That directory had neither Switch-account ownership nor stable release/platform identity, so two
accounts using the same title could target the same mutable `Working` directory.

The writable layout is now:

```text
backups/account-<AccountUid-hex>/<exact-game-id>/<workspace>
```

Profile path encoding is deterministic lower-case hexadecimal:

```text
account-<uid.uid[0] as 16 hex digits><uid.uid[1] as 16 hex digits>
```

An all-zero/invalid `AccountUid` has no writable namespace. Display name is never part of the
authoritative profile identity.

Exact game identity is resolved from the selected Switch title ID through
`PokeVault::Games::findSwitchGame(titleId)`. There is no writable fallback to title text or a broad
generation name. This preserves release/platform distinctions such as:

- `sword_switch` vs `shield_switch`
- `letsgo_pikachu_switch` vs `letsgo_eevee_switch`
- `firered_switch` vs `leafgreen_switch`
- `firered_switch` vs `firered_gba`

Current path ownership:

- installed-save snapshot creation -> profile + exact-game root;
- reusable `Working` -> profile + exact-game root;
- timestamped backups -> profile + exact-game root;
- existing backup selection/deletion -> profile + exact-game root;
- trainer-screen `New backup...` -> derives the same profile/exact-game root and refuses a session
  whose current workspace is outside it;
- restore/injection staging consumes the selected workspace path and the separate live-write policy
  remains hard-disabled;
- installed-save discovery was already `AccountUid` aware and exact title-ID aware;
- RetroArch/legacy source assignment remains its separate explicit profile-binding/read-only flow.

No additional source-instance key is required for the current installed-save path: save discovery
deduplicates the selected account + application title into one game tile. If the platform later
supports multiple simultaneously editable source instances for one account/title, that must be
designed explicitly rather than silently sharing this namespace.

### Legacy unscoped policy

Pre-A08 title-only folders contain no reliable persisted `AccountUid` ownership metadata. The audit
therefore does **not** auto-migrate, auto-move, delete, or assign them to whichever account is
currently selected.

They remain in place and are surfaced separately as:

```text
LEGACY UNSCOPED / OWNERSHIP UNKNOWN
```

They are read-only/import-required from the normal backup picker:

- they cannot be selected as an editable workspace;
- they cannot be deleted from the normal profile-scoped picker;
- old `Working` folders are included so they do not disappear;
- discovered custom folder names are preserved verbatim, including spaces;
- future assignment/import must explicitly establish profile + exact-game ownership before creating
  a writable scoped copy.

### A08 regression coverage

`tests/test_backup_namespace_contract.cpp` locks:

- deterministic same-profile/same-game paths;
- different profiles -> different paths;
- different exact releases -> different paths;
- FireRed Switch != FireRed GBA;
- invalid/zero profile identity fails closed;
- traversal/absolute unsafe workspace components fail closed;
- writable path construction derives exact game identity from title ID rather than display title;
- legacy ownership-unknown folders remain quarantined;
- old/new custom folder names remain discoverable;
- trainer-screen named backups cannot escape/recreate the old title-only namespace.

### N06 — P2 — OPEN — backup-directory seeding is not a durable directory transaction

`Utils::copyDirectory` still seeds installed-save snapshots and named backup folders file-by-file.
If that directory copy fails partway through, the caller returns failure and does not open/commit the
operation, and the immutable source remains untouched, but a partial PokeBank-owned destination
directory can remain on disk. Timestamped/named partial folders could later be rediscovered.

This is not authorization for live writes and is not an A08 ownership collision. It is a remaining
directory-generation/recovery problem: future hardening should stage the complete directory
generation, verify it, then promote/recover it as one logical snapshot.

## A03 hardening tranche — immutable custody + prepared destination candidates

The pre-A03 storage placement path adapted the authoritative carried object in place before final
slot placement. Cross-game placement could replace `moveMon[idx]` with the converted representation,
while same-game placement could run destination-only AffixedRibbon/checksum repair directly against
the carried object. A refused/cancelled/blocked placement therefore risked returning a representation
different from the one originally picked up.

The storage path now separates **authoritative custody** from a disposable **PreparedPlacement**:

```text
authoritative carried original
-> prepare destination candidate from const source
-> validate/accept candidate
-> commit destination ownership
-> only then retire/replace original custody
```

Implementation pieces:

- `TrainerViewScreen::prepareForPane(const Pokemon::Pokemon&, int)` reads the carried original
  without replacing it;
- Bank destination sets `useOriginal` and performs no conversion, normalization or checksum rewrite;
- same-game save placement clones to a candidate, then runs destination-only
  `normalizeAffixedRibbon` + `refreshChecksum` on that candidate;
- cross-game save placement calls `Conversion::convert()` with the const original and keeps the
  returned destination-format object separate;
- `PokeBank::UIModel::commitNativeSwap()` moves the exact native object for Bank placement;
- `PokeBank::UIModel::commitPreparedCandidate()` assigns destination ownership first, then replaces
  carried custody with the displaced destination occupant (or clears custody when destination was empty).

Occupied-slot swaps therefore preserve the displaced Pokémon as native held custody. A blocked cell
never executes either commit primitive, so its original remains in `moveMon` unchanged. Multi-cell
placement retains the existing partial-success UX: successful cells may commit while blocked cells
remain original and in hand.

Warnings for LGPE/Gen III downgrade are still resolved before `putDownBlock()` performs preparation,
so cancelling the warning cannot leave a hidden converted candidate in custody. A02 rollback remains
in force: return-to-origin restores the still-authoritative carried representation.

`hasUnsavedChanges` is set by successful placement, not merely by candidate preparation.

### A03 regression coverage

`tests/test_storage_conversion_custody_contract.cpp` now locks:

- the old mutating `convertForPane(unique_ptr&, ...)` API is gone;
- preparation accepts a const original;
- Bank placement uses the original object with no candidate conversion;
- same-game normalization occurs only on a cloned candidate;
- cross-game conversion produces a separate candidate;
- locked destinations are rejected before preparation;
- failed preparation continues without any commit;
- warning/cancel flow occurs before candidate preparation;
- A02 return-to-origin custody protection remains present;
- executable occupied/empty destination commit-order tests prove the candidate owns the destination
  before the original is retired;
- executable Bank swap tests prove the exact native object changes location and the displaced object
  becomes custody.

A03 implementation checkpoint:

```text
application SHA:
cfb0d6901962c5cd8286dba3f60bab9144c3f60a

PokeBank NX Host Tests:
35889300643 / #1098 / SUCCESS

Audit Hardening Native Validation:
35889293109 / #15 / SUCCESS
```

This is CI verification only. It does not transfer physical-device acceptance from the accepted Gen
I–III checkpoint.

A03 does **not** solve A04. Slot ownership is now ordered safely in memory, but Bank persistence,
destination-save persistence, source retirement and crash recovery are not yet one durable
cross-store transaction.

## A04a hardening tranche — durable Move transaction journal/recovery core

A03 established correct **in-memory** custody ordering, but the exact-current persistence paths still
saved Bank and mutable workspace state independently. Individually durable files are not enough for a
true Move: a crash between separate persistence decisions can leave the app unable to prove which
store is authoritative.

A04a adds the persistence/recovery core without wiring final Storage UI Move semantics.

### Canonical journal ownership

Production transaction metadata is PokeBank-owned:

```text
sdmc:/switch/PokeBank-NX/transactions/
    records/
        tx-<generation>.pbtx
        tx-<generation>.destination.bin
        tx-<generation>.source-retired.bin
```

`PokeBank::Paths::transactionsRoot()` is the canonical production root. Host tests inject an
isolated temporary root; A04b production integration must use the canonical PokeBank-owned root.

The `.destination.bin` and `.source-retired.bin` files are immutable **recovery evidence/post-state
images**. They are not active Pokémon, Bank entries, Copies, Clones, or withdrawable entities.

### Journal schema and identity

Current schema: **binary v1**.

The journal uses:

- fixed magic/version;
- explicit transaction state;
- stable transaction ID;
- typed source/destination store descriptors;
- whole-store expected SHA-256 fingerprints;
- one or more Move records with source/destination slot diagnostics plus payload SHA-256 evidence;
- bounded string/move counts;
- SHA-256 integrity over the serialized journal body.

The SHA-256 implementation is locked against the standard `abc` test vector.

Current transaction IDs are allocated as retained monotonically-scanned IDs such as
`tx-0000000000000001`. IDs are never reused while a current journal or recovery-evidence sidecar
exists. The current Switch app is single-process; A04b must serialize transaction creation rather
than introducing concurrent ID allocation.

### Typed stores

A04a deliberately does not accept arbitrary source paths as transaction identity.

Current descriptor types are:

```text
BANK
MUTABLE_WORKSPACE_SINGLE_FILE
MUTABLE_WORKSPACE_FILE_SET   (reserved for future file-set support)
```

A mutable workspace descriptor carries filesystem-safe semantic identity:

```text
account/profile component
exact game identity
workspace identity
file identity
```

Bank uses the stable Bank descriptor rather than a caller-supplied arbitrary path.

A04b still needs the production resolver that maps these descriptors through reviewed PokeBank path
APIs to the exact authoritative Bank/workspace files. Installed-game, RetroArch, and other emulator
sources are not representable as writable A04 stores.

### State machine

The engine permits only sequential transitions:

```text
PREPARED
  -> DESTINATION_WRITTEN
  -> DESTINATION_VERIFIED
  -> SOURCE_RETIRE_PENDING
  -> SOURCE_RETIRED
  -> COMMITTED
```

Only the transaction `Engine` can persist state/evidence; external callers can inspect/load/scan
journals but cannot directly skip states.

Critical ordering:

```text
durable PREPARED
-> destination durable replace
-> destination reread / SHA-256 / store validation
-> durable DESTINATION_VERIFIED
-> durable SOURCE_RETIRE_PENDING
-> source durable retirement
-> source reread / SHA-256 / store validation
-> durable SOURCE_RETIRED
-> final destination re-verification
-> COMMITTED
```

A source retirement is never attempted before the durable
`SOURCE_RETIRE_PENDING` authorization state, which itself can only be reached after destination
verification.

A MOVE whose expected before/after images do not change both stores is rejected before PREPARED
becomes authoritative. This avoids ambiguous before/after fingerprint states.

### Recovery reconciliation

Recovery does not trust the journal state alone. It reconciles:

```text
journal state
+
actual current source SHA-256
+
actual current destination SHA-256
+
expected source-before/source-retired SHA-256
+
expected destination-before/destination-after SHA-256
+
retained post-state evidence
```

Examples:

- destination bytes landed but journal still says PREPARED -> recognize the exact destination
  post-hash, do not write it twice, then continue;
- destination is verified while source is still present -> treat that duplicate as a temporary
  transaction state, not Clone semantics, and safely retire source;
- source retirement landed but journal still says SOURCE_RETIRE_PENDING -> recognize the exact
  retired post-hash and never retire another Pokémon;
- COMMITTED -> repeated recovery performs no Move mutation.

Recovery is intentionally idempotent.

If source/destination matches neither the recorded before nor post fingerprint, recovery returns
**CONFLICT** and performs no destructive mutation.

Corrupt journal/evidence returns **CORRUPT** and performs no store mutation. A newer/unsupported
schema returns **UNSUPPORTED VERSION** and is not rewritten. Evidence remains retained for manual
recovery/audit.

`Journal::scan()` enumerates current `.pbtx` records deterministically by transaction ID and
classifies neighboring records independently, so one corrupt journal cannot erase or execute
another.

### Durable journal transitions

Journal files and post-state evidence use the existing reviewed
`PokeBank::Storage::DurableFile::replace` primitive. A failed journal transition blocks the next
destructive step. The transaction system does not introduce a weaker raw `fopen("wb")` persistence
path.

Completed journals/evidence are retained. A future bounded retention/cleanup policy may archive old
completed evidence, but cleanup is not allowed to make a committed transaction executable again.

### Fault-injection coverage

The executable A04a regression covers recovery after every declared boundary:

```text
AFTER_PREPARED_JOURNAL
AFTER_DESTINATION_WRITE
AFTER_DESTINATION_VERIFY
AFTER_DESTINATION_VERIFIED_JOURNAL
BEFORE_SOURCE_RETIRE
AFTER_SOURCE_WRITE
AFTER_SOURCE_VERIFY
AFTER_SOURCE_RETIRED_JOURNAL
BEFORE_COMMITTED
```

Each injected interruption must resume and converge to COMMITTED without duplicate destination
writes or double source retirement.

Additional tests cover:

- destination write failure;
- destination validation failure;
- source-retirement validation failure;
- source precondition conflict;
- destination precondition conflict;
- corrupt journal;
- unsupported newer schema;
- repeated recovery after COMMITTED;
- journal persistence failure before source retirement;
- multiple pending journals with one corrupt neighbor;
- profile/exact-game descriptor mismatch;
- path traversal/unsafe descriptor rejection;
- installed/RetroArch mutation policy remaining hard-disabled;
- multi-Pokémon Move-record serialization;
- retained completed journal/evidence;
- non-colliding retained transaction IDs.

### Bidirectional-session conclusion

A04b must **not** persist an arbitrary mixed-direction session by simply writing Bank first or
workspace first.

For example, a session containing both:

```text
Bank -> Game
Game -> Bank
```

has opposite destination-first requirements.

Current A04b prerequisite:

- cross-store logical Moves must be converted into individually ordered durable transactions from
  fresh authoritative store snapshots;
- a multi-Pokémon block moving in one direction may be represented by multiple Move records inside
  one transaction;
- opposite-direction Moves must be serialized as separate transactions;
- a second cross-store durable Move must not race an unresolved transaction;
- if the product cannot safely serialize that sequence, mixed-direction durable commit must be
  blocked rather than guessed.

### BDSP disposition

BDSP remains excluded from true-Move integration.

`SaveData.bin` + `Backup.bin` still require their own recoverable file-set generation primitive.
The A04a descriptor model reserves a future file-set store type but does not authorize sequential
two-file BDSP writes.

### A04b prerequisites

Before A04 can be called fixed, A04b must add and prove:

1. production descriptor -> canonical-path resolution;
2. a Bank store adapter using Bank serialization/round-trip validation/DurableFile;
3. single-file mutable-workspace adapters using the existing A09 validators/DurableFile;
4. exact A03 PreparedPlacement -> durable post-image integration;
5. cross-store UI flow that creates the durable transaction before source retirement;
6. startup recovery that resolves typed stores and presents conflict/corrupt results without guessing;
7. serialized handling of bidirectional session moves;
8. user-visible transaction failure/recovery status;
9. physical Switch FAT32/exFAT power-loss/recovery testing;
10. continued hard exclusion of BDSP until its file-set journal exists.

A04a itself does **not** auto-save Storage moves, change exit prompts, write installed-game saves,
write RetroArch saves, or enable other emulator writes.

### A04a exact code checkpoint

```text
application SHA:
bf799d5843bf9863233488664484187d14114659

PokeBank NX Host Tests:
35954237696 / #1103 / SUCCESS

Audit Hardening Native Validation:
35954234400 / #19 / SUCCESS
```

Host #1103 passed the permanent suite, focused RSE regression, and the sanitizer build/run. The
managed runner cannot perform LeakSanitizer's `/proc` inspection, but AddressSanitizer and
UndefinedBehaviorSanitizer ran, including the A04a transaction test.

Native #19 passed clean devkitA64 compile/link, AArch64 linkage verification, and NRO production.

This is CI verification only. It is not physical device acceptance.

## A04b hardening tranche — production true-Move integration

A04b now wires the A04a transaction engine into the real PokeBank-owned stores used by Storage:

```text
PokeBank bank.dat
<->
profile/exact-game PokeBank mutable workspace
```

This still does **not** write the installed-game filesystem, RetroArch saves, or other emulator source
files. The transactional workspace is the PokeBank-owned backup/staging copy only.

### Production store resolution

`MoveTransactionProduction` resolves typed transaction descriptors rather than accepting arbitrary
paths.

Supported writable descriptors are:

- canonical Bank `bank.dat`;
- `MUTABLE_WORKSPACE_SINGLE_FILE` beneath the A08 profile + exact-game + workspace namespace.

The exact-game allowlist currently enables:

- Let's Go Pikachu/Eevee -> `savedata.bin`;
- Sword/Shield -> `main`;
- Legends: Arceus -> `main`;
- Scarlet/Violet -> `main`;
- Legends: Z-A -> `main`;
- FireRed/LeafGreen Switch -> the one validated authoritative 128 KiB workspace file.

BDSP remains excluded because `SaveData.bin` + `Backup.bin` are still a coupled multi-file
generation. Legacy-unscoped folders, arbitrary safe-looking file names, traversal paths, live title
paths, RetroArch paths and other emulator paths are not valid production transaction stores.

The Bank adapter reuses Bank image serialization/round-trip validation and DurableFile. A missing
first-run `bank.dat` is represented by a semantic missing-Bank sentinel so recovery can distinguish
legitimate initial absence from unexpected disappearance of an established Bank.

Workspace adapters reuse the A09 exact-game validators and DurableFile. The transaction layer never
calls `restoreBackupToTitle` or `injectToTitle`.

### Serialization before persistence

Supported workspace save logic is now separable into:

```text
buildWorkspaceImage
validateWorkspaceImage
persistWorkspaceImage
```

and Bank exposes the narrow verified-image operations required by the transaction coordinator.

This lets A04b build the destination and retired-source whole-store images before any transaction
persistence begins.

### Clean-baseline rule

Cross-store true Move starts only from a clean transactional baseline.

Before pickup the UI records:

- workspace dirty state;
- Bank dirty state;
- editor/modal dirty state;
- authoritative Bank/workspace SHA-256 baselines.

Unrelated pre-existing staged changes block true Move with a Save/Discard-first message. Pickup itself
is treated as transaction staging rather than unrelated dirty state. Returning the held Pokémon to
origin restores the pre-pickup workspace dirty flag rather than inventing a new dirty state.

### Cross-store restrictions

For this first safe production integration:

- destination slots must be empty;
- destination slots must be unlocked;
- a block is prepared completely before transaction mutation;
- a same-direction multi-Pokémon block uses one transaction with multiple Move records;
- occupied-slot cross-store swaps are refused;
- Copy does not use source-retirement semantics;
- cross-game true Move is fail-closed until the F05-F13 conversion preservation audit proves those
  routes.

Game -> Bank deposits preserve the native Pokémon payload. Bank -> workspace true Move is enabled only
for same-game-group/native-compatible placement through the A03 prepared-candidate path.

### Production coordinator

Normal cross-store placement no longer falls through to the historical independently-saved Bank/save
staging path.

The coordinator performs:

```text
capture clean authoritative baseline
-> verify no unresolved transaction
-> verify empty/unlocked destinations
-> prepare every A03 destination candidate
-> re-read and compare authoritative pre-state fingerprints
-> stage destination candidates in memory
-> build verified Bank and workspace post-images
-> allocate transaction
-> durable PREPARED
-> A04a destination-first recovery/commit engine
-> COMMITTED
-> reload/rebaseline authoritative state
```

If the transaction becomes unresolved, normal cross-store mutation is locked and the user receives a
recovery-required status instead of the UI pretending that transient RAM is authoritative.

A successful transaction clears pending carry state, reloads the Bank from its committed disk image,
and makes the committed workspace state the new clean baseline. This prevents later legacy
Save/Discard behavior from resurrecting the retired Bank source or discarding only the destination
side of a committed Move.

### Startup recovery

Before a mutable workspace is parsed, UI startup now scans the canonical transaction journal and
attempts deterministic A04a recovery using the production resolver.

- recoverable active transaction -> reconcile and finish;
- committed -> do not execute again;
- corrupt / unsupported / conflicting / unresolvable -> no store mutation and Storage Move remains
  locked.

Visible notices distinguish recovered transactions from recovery-required cases. Account identifiers
are not required in the user-facing message.

### Bidirectional serialization

Opposite-direction Moves are serialized as separate transactions.

```text
Bank -> workspace
COMMITTED
rebaseline

workspace -> Bank
COMMITTED
```

The second transaction reads the first transaction's post-state fingerprints as its authoritative
baseline. An unresolved transaction blocks another cross-store Move.

### A04b tests

`tests/test_move_transaction_production.cpp` plus the permanent A04a suite lock:

- exact-game/file allowlist;
- first-run Bank sentinel behavior;
- dirty-state eligibility;
- occupied-destination refusal;
- cross-game true-Move refusal;
- coordinator `prepare` + `recover` wiring;
- committed Bank rebaseline before the success status;
- pickup/return dirty-state restoration;
- startup recovery before workspace parse;
- no live-title injection calls in the production transaction adapter;
- workspace image build/persist separation;
- BDSP true-Move refusal;
- sequential Bank -> workspace then workspace -> Bank transactions using the prior committed
  fingerprints as the next baseline.

### Remaining A04 gate

A04b is implemented in software for the supported single-file PokeBank-owned workspace routes, but
physical power-loss behavior is not device accepted yet.

Required physical matrix remains:

- Bank -> workspace Move;
- workspace -> Bank Move;
- power loss after destination write;
- after destination verification;
- before source retirement;
- after source retirement;
- restart/recovery;
- FAT32;
- exFAT where a supported test environment exists.

Cross-game routes remain intentionally disabled until the conversion audit proves preservation.
BDSP remains excluded until its multi-file generation transaction exists.

A04b exact code checkpoint:

```text
application SHA:
ca270828be90cc181cce67cf1cc6c5461a4b98db

PokeBank NX Host Tests:
35959492840 / #1123 / SUCCESS

Audit Hardening Native Validation:
35959488284 / #32 / SUCCESS
```

This is CI verification only. It is not physical device acceptance.

## Current A01–A09 status

| ID | Current status | Remaining gate |
|---|---|---|
| A01 | IMPLEMENTED | Durable single-file Bank replacement exists; physical Switch SD/FAT32/exFAT recovery behavior still needs hardware testing |
| A02 | FIXED | Failed rollback retains held Pokémon custody |
| A03 | IMPLEMENTED | Authoritative carried Pokémon remains unchanged until destination candidate/native placement commit; A04 durable cross-store transaction still required |
| A04 | IMPLEMENTED / PHYSICAL RECOVERY GATE REMAINS | A04a journal/recovery + A04b real Bank <-> supported single-file mutable-workspace true Move/startup recovery are wired; cross-game routes remain gated and physical Switch power-loss recovery is not yet accepted |
| A05 | FIXED | Unreadable Bank casualties use unique preserved generations |
| A06 | FIXED | BDSP minimum-layout/truncation refusal is guarded before fixed-offset parsing |
| A07 | FIXED | Newer/larger Bank layouts become migration-required/write-blocked; invalid counts fail closed |
| A08 | IMPLEMENTED | Profile + exact-game mutable workspace namespace is enforced; ownership-unknown legacy data is quarantined; physical multi-profile UX verification remains |
| A09 | PARTIAL / FAIL-CLOSED | Single-file mutable backup writers use DurableFile; BDSP remains blocked pending a recoverable multi-file journal |

CI validation on the exact audit head does not imply physical device acceptance.

## A09 hardening tranche — durable mutable backup workspaces

Current disposition after this tranche:

| Family | Disposition | Candidate validation |
|---|---|---|
| Let's Go Pikachu/Eevee | MIGRATED TO DURABLE REPLACEMENT | exact supported size + complete known block set + checksum round-trip probe |
| Sword/Shield | MIGRATED TO DURABLE REPLACEMENT | SC container integrity hash + complete block parse through `tryDecrypt` |
| Legends: Arceus | MIGRATED TO DURABLE REPLACEMENT | SC container integrity + complete block parse + PLA layout validation |
| Scarlet/Violet | MIGRATED TO DURABLE REPLACEMENT | SC container integrity hash + complete block parse through `tryDecrypt` |
| Legends: Z-A | MIGRATED TO DURABLE REPLACEMENT | SC container integrity hash + complete block parse through `tryDecrypt` |
| FireRed/LeafGreen | MIGRATED TO DURABLE REPLACEMENT | exact 128 KiB size + active sector table + active-sector checksum no-op probe |
| Brilliant Diamond/Shining Pearl | MULTI-FILE JOURNAL REQUIRED / FAIL CLOSED | `SaveData.bin` + `Backup.bin` are a coupled generation; saving is blocked until a recoverable file-set transaction exists |

The migrated single-file writers now use the same reviewed `PokeBank::Storage::DurableFile::replace` primitive as Bank persistence rather than direct `fopen(..., "wb") / fwrite / fclose` replacement.

BDSP is deliberately not treated as fixed by two sequential single-file writes. A crash between two promotions could leave a mixed generation, so the mutable BDSP save path fails closed until a recoverable multi-file journal/state machine exists.

This changes only PokeBank-owned mutable backup workspaces. Original installed saves and emulator-source files remain protected by the existing hard live-write locks.

### N05 — P1/P2 — FIXED IN A09 — FRLG recovery generations could shadow the authoritative save

The first DurableFile migration preserves same-directory `.previous.N`, `.failed.N` and temporary sibling files. FRLG's legacy save discovery scans for any 128 KiB file, so without an exclusion a recovery generation could be selected as the editable save purely because its size also matches.

The FRLG scanner now explicitly ignores DurableFile `.tmp.`, `.previous.` and `.failed.` siblings. Regression coverage locks this rule. Recovery evidence remains preserved but is never treated as the active save.


# Original A01–A09 findings at audit start — historical evidence

The detailed sections below preserve what was confirmed at the start of this audit. They are not the
current disposition; use the current-status table above for the hardened branch state.

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


---

# F05-F13 conversion fidelity / generated golden fixture tranche — 2026-09-24

This tranche begins from the CI-green F07 checkpoint `a999ed708a546d88717642bdf58c65c014314feb` and keeps cross-game true Move
globally disabled. It does not enable live source writes, Master Vault, another generation, BDSP
multi-file transactions, or N06.

## Permanent fixture contract

Generated fixtures are deterministic, redistributable byte/semantic vectors. Every policy fixture
calculates a SHA-256 before conversion-policy evaluation and asserts the source bytes remain unchanged.
Production conversion continues to receive `const Pokemon::Pokemon&` and returns a separate candidate.

The new `Conversion::Report` separates:

- **declared losses** — semantics/data that cannot exist unchanged in the target;
- **adaptations** — deterministic representation changes needed to preserve meaning.

Future cross-game true Move must treat an unacknowledged declared loss as non-retirable source state.

## F05-F13 disposition

| ID | Exact-current disposition | Fixture / policy |
|---|---|---|
| F05 shiny preservation | FIXED / GENERATED GOLDEN FIXTURE | Gen III XOR<8 vs modern XOR<16 boundary is tested; XOR 8..15 gets the transfer-style top-PID-bit adjustment so a Gen III non-shiny does not become shiny merely by entering a modern representation |
| F06 Unown form | FIXED / GENERATED GOLDEN FIXTURE | multiple PID-derived forms are stamped/verified; modern->Gen III PID search now constrains the requested Unown letter |
| F07 PID search exhaustion | ALREADY FIXED / PROVEN BY REGRESSION | bounded search returns no candidate and `TraitPreservationFailed`; no original-PID fallback |
| F08 ability slot/number | FIXED / GENERATED GOLDEN FIXTURE | slot 1, slot 2, duplicate ability and hidden-ability cases covered; hidden or changed/unrepresentable ability fails instead of silently becoming a normal ability |
| F09 S/V <-> Z-A divergent data | FIXED + DECLARED LOSS POLICY | S/V->Z-A declares Tera loss; Z-A->S/V synthesizes target-default Tera and declares Alpha loss when present; divergent record/level data is not preserved merely because offsets overlap |
| F10 Gen III EV policy | POLICY CHOSEN / GENERATED GOLDEN FIXTURE | 252 remains exact; Gen III 253/254/255 -> modern clamps to 252 and records `Gen3EVClamped`; modern->Gen III keeps representable raw byte values |
| F11 nickname/language/text | FIXED / FAIL CLOSED WHERE UNREPRESENTABLE | representable custom nickname survives modern->Gen III; default name canonicalizes to Gen III species spelling; overlength/unmappable text and unsupported Gen III language tables fail explicitly |
| F13 profile/provenance | PARTIAL / EXPLICIT BOUNDARY | source origin and destination entity origin are reported separately; modern->Gen III origin restamp is a declared provenance loss. Profile/account/current-location ownership remains outside PKM conversion in the A04b StoreDescriptor and must be retained by future Vault provenance |

## Declared loss bits added

- `Gen3EVClamped`
- `TeraDataDropped`
- `ZAAlphaDropped`
- `DivergentGameDataDropped`
- `OriginGameRestamped`
- `MoveDropped`
- `RelearnMoveDropped`
- `HeldItemDropped`
- `StatTrainingReset`
- `PLAExclusiveDataDropped`
- `RibbonDataDropped`
- `AbilitySlotNormalized` (duplicate Gen III ability with a set native selector bit becomes modern slot 1; ability identity is preserved but the redundant native slot bit cannot round-trip)

## Declared adaptations added

- Gen III -> modern shiny-threshold PID adjustment;
- target-default Tera synthesis where the destination is S/V;
- destination PP clamp;
- Gen III transfer-date synthesis;
- default Gen III nickname canonicalization.

## Generated fixture list

- `f05-gen3-modern-shiny-xor-0`
- `f05-gen3-modern-shiny-xor-8-boundary`
- `f05-gen3-modern-shiny-xor-16-nonshiny`
- `f06-unown-forms-0-1-13-27`
- `f07-impossible-gender-pid-search`
- `f08-ability-slot1-slot2-duplicate-hidden`
- `f09-sv-za-tera-alpha-policy`
- `f10-gen3-ev-252-253-254-255`
- `f11-gen3-international-text-and-language`
- `f13-origin-restamp-provenance`

## Route-level true-Move disposition

No cross-game route is broadly enabled by this tranche.

The generated fixtures prove the listed fidelity rules, but full route eligibility still requires
real serialized/reparsed entity golden vectors for every target-native field (including ribbons,
marks, HOME tracker behavior and game-specific side data) plus user-visible loss acknowledgement.
Therefore every cross-game A04b route remains **ROUTE MUST REMAIN DISABLED** for source retirement.

Specific candidate generation can continue for staged/copy-like workflows because source custody is
immutable and failures return no destination candidate.

## Provenance rule

A target format field is not allowed to overwrite historical truth in PokeBank provenance. In
particular, a modern Pokemon converted into PK3 may need a Gen III origin field value to serialize,
but that value is a compatibility representation and is recorded as `OriginGameRestamped`; it must
not replace the original modern origin in future provenance.

CI evidence for the exact implementation head is recorded in issue #69 / PR #79 after the permanent
Host + ASan/UBSan + Audit Hardening Native Validation gates complete.


### Follow-up from Host #1127

The first generated F06 test exposed a non-idempotent Unown form-stamping helper. The helper selected
the form pattern from PID bits that it then overwrote, so restamping could alter unrelated PID-derived
constraints. The forward fix selects the pattern only from bits outside the Unown form mask and asserts
idempotence.

The same follow-up hardens F11 default-name detection to ignore non-semantic Gen III bytes after the
0xFF string terminator, and records duplicate-ability selector normalization explicitly rather than
calling that round-trip byte drift lossless.


## Production entity golden layer

In addition to the generated policy vectors, `test_conversion_entity_golden.cpp` exercises the real
`Conversion::convert()` path on deterministic native entities:

- FireRed/LeafGreen PK3 -> Sword PK8 -> PK3 at the shiny-threshold boundary;
- Brilliant Diamond/Shining Pearl PK8 Unown form 13 -> PK3 -> PK8;
- Sword hidden-ability Pikachu -> PK3 explicit failure;
- Scarlet PK9 -> Z-A PA9 with Tera loss declaration;
- Z-A PA9 Alpha/divergent state -> Scarlet PK9 with declared loss + destination Tera synthesis;
- PK3 EV 252/253/254/255 -> Sword with declared 252 clamp;
- Sword custom nickname -> PK3 -> Sword with origin-restamp provenance declaration;
- unsupported Gen III text/language failure cases.

For each successful fixture the test serializes the destination back to its encrypted native entity
format, reparses it with the destination class, validates the checksum, and compares the reparsed
semantic fields. Every fixture hashes the exact encrypted source bytes and proves both the bytes and
SHA-256 are unchanged after conversion. Failed conversions prove the same source immutability.

These fixtures are intentionally **route slices**, not blanket proof for every species/form/field
combination. They do not unlock A04b cross-game source retirement. Route enablement remains a
separate future decision after broader corpus coverage (ribbons/marks/HOME side data, forms, events,
language tables and game-specific edge cases).


## F05-F13 final fidelity checkpoint — 2026-09-24

This checkpoint closes the requested **conversion-fidelity / golden-fixture audit tranche** only.
It does **not** enable any cross-game A04b source retirement, live source write, Master Vault,
another generation, BDSP file-set work, or N06.

### Exact implementation checkpoint before this documentation capture

```text
application SHA:
ff42ae8bc127942e19613d27e710aef89c25d31d

tree:
7de87c152ab395c9fc1ca0b90ed246efa35379cf

PokeBank NX Host Tests:
35970439207 / #1132 / SUCCESS

Host normal suite:
SUCCESS

Focused RSE regression:
SUCCESS

ASan + UBSan:
SUCCESS
```

The earlier native run `35970151738 / #39 / SUCCESS` belongs to
`c0d4ec96da875a7c2d5e4e4c7ef3ea20a6ddf835`, immediately before the final F09
loss-report fix, so it is retained as historical evidence only and is **not** transferred to
`ff42ae8...`. This documentation update is on the native workflow trigger list; the exact-head
native result for the final documented checkpoint is recorded in issue #69 / PR #79 after it completes.

### Final F05-F13 disposition

| ID | Classification | Exact tested conclusion |
|---|---|---|
| F05 shiny preservation | **PROVEN CORRECT BY FIXTURE** for tested PK3 <-> PK8 slice | Real PK3 -> Sword -> PK3 fixtures cover XOR 0 shiny, XOR 8 threshold boundary, and XOR 16 ordinary non-shiny. The Gen III XOR<8 vs modern XOR<16 difference uses the transfer-style top-PID-bit adaptation only when required; source bytes/hash remain exact and destination serialize/reparse/checksum passes. |
| F06 PID-derived Unown form | **PROVEN CORRECT BY FIXTURE** for tested BDSP <-> PK3 slice | Real entity round trips cover forms 0, 1, 13 and 27. The Gen III PID search constrains the requested Unown form and the form-stamping helper is idempotent. |
| F07 PID search exhaustion | **ALREADY FIXED** | Deterministic impossible-trait exhaustion returns no candidate and `TraitPreservationFailed`; the original incompatible PID is never silently reused. |
| F08 ability slot / number | **PROVEN CORRECT BY FIXTURE** for tested semantics; impossible hidden ability is **UNSUPPORTED / FAIL CLOSED** | Real Ralts slot-1 and slot-2 conversions round-trip through PK3. Pikachu's duplicate Gen III ability selector is explicitly normalized/loss-declared on return to modern. Hidden ability -> PK3 returns `AbilityNotRepresentable`. |
| F09 S/V <-> Z-A divergent fields | **CONFIRMED DEFECT -> FIXED / PROVEN BY FIXTURE** for tested slices | S/V -> Z-A declares Tera loss. Z-A -> S/V declares Alpha and divergent-field loss and synthesizes target-native Tera. Audit found that nonzero Z-A `0x94-0x9F` Plus-side data was being zeroed without contributing to `DivergentGameDataDropped`; `ff42ae8...` fixes both direct sibling and PK8-hub paths and adds real Z-A -> S/V and Z-A -> Sword fixtures. |
| F10 Gen III EV 252/253/254/255 | **NEEDS POLICY -> POLICY CHOSEN / PROVEN BY FIXTURE** | Gen III -> modern preserves 252 and clamps 253/254/255 to 252 with `Gen3EVClamped`. Reverse modern raw-byte fixtures prove 252/253/254/255 are retained entering PK3 when each destination total remains within the Gen III gameplay limit. This distinguishes later 252 legality policy from Gen III's byte representation. |
| F11 nickname / language / text | **PROVEN CORRECT BY FIXTURE** where representable; otherwise **UNSUPPORTED / FAIL CLOSED** | Representable custom nickname/OT/language survives Sword -> PK3 -> Sword. Default names canonicalize explicitly. Unmappable text and unsupported Gen III language tables return `TextNotRepresentable` / `LanguageNotRepresentable` instead of truncating or silently replacing. |
| F13 profile / provenance implications | **NEEDS MORE FIXTURES / EXPLICIT BOUNDARY** | PKM conversion reports source origin separately from destination entity origin. Modern -> PK3 origin restamping is a declared `OriginGameRestamped` loss and must not overwrite historical provenance. Profile/account/current-location ownership is not a Pokémon payload field; it remains in the A08/A04b StoreDescriptor/session layer and must be carried into future Vault provenance rather than fabricated by conversion. |

### Production entity fixtures now exercised

```text
F05
- f05-pk3-swsh-xor-0-shiny
- f05-pk3-swsh-xor-8-boundary
- f05-pk3-swsh-xor-16-nonshiny

F06
- f06-bdsp-unown0-pk3
- f06-bdsp-unown1-pk3
- f06-bdsp-unown13-pk3
- f06-bdsp-unown27-pk3

F08
- f08-ralts-slot1-pk3
- f08-ralts-slot2-pk3
- f08-pikachu-duplicate-slot2-pk3
- f08-swsh-hidden-pk3-fail

F09
- f09-sv-za-tera
- f09-za-sv-alpha-plus
- f09-za-swsh-plus-hub

F10
- f10-pk3-ev-252-255
- f10-swsh-raw-ev-252-pk3
- f10-swsh-raw-ev-253-pk3
- f10-swsh-raw-ev-254-pk3
- f10-swsh-raw-ev-255-pk3

F11 / F13
- f11-f13-swsh-pk3-sparky
- unsupported nickname/text -> PK3 explicit failure
- unsupported language -> PK3 explicit failure
```

The generated policy-golden layer remains in addition to these production entity vectors.

### Permanent golden-fixture assertions

For each production fixture where conversion succeeds:

1. serialize and capture exact native source bytes;
2. SHA-256 the source;
3. call `Conversion::convert(const Pokemon::Pokemon&, ...)`;
4. prove source native bytes and SHA-256 are unchanged;
5. serialize the destination candidate to its native encrypted entity form;
6. reparse through the destination Pokémon class;
7. require destination checksum validity;
8. compare target-native semantic fields exercised by that route.

Round trips are used where the formats can meaningfully support them. Byte differences caused by
fields that do not exist in the opposite format are not hidden: they must be a declared `Loss`,
a declared `Adaptation`, or an explicit conversion failure.

### Declared conversion losses at tranche close

```text
Gen3EVClamped
TeraDataDropped
ZAAlphaDropped
DivergentGameDataDropped
OriginGameRestamped
MoveDropped
RelearnMoveDropped
HeldItemDropped
StatTrainingReset
PLAExclusiveDataDropped
RibbonDataDropped
AbilitySlotNormalized
```

Declared deterministic adaptations:

```text
PidAdjustedForShinyThreshold
TargetDefaultTeraSynthesized
MovePPClamped
Gen3TransferDateSynthesized
DefaultNicknameCanonicalized
```

### Route-level true-Move gate

**ROUTE ELIGIBLE FOR FUTURE TRUE-MOVE ENABLEMENT: none yet.**

Every cross-game A04b route remains:

```text
ROUTE MUST REMAIN DISABLED
```

The fixture tranche proves specific semantic rules and route slices; it does not yet prove every
species/form/event/ribbon/mark/HOME-tracker/game-specific side field for an entire game-pair corpus.
It also does not yet provide the user-visible acknowledgement required before retiring a source
behind a candidate with declared loss.

Species or forms absent from the destination remain hard fail-closed through the destination dex gate.
A Pokémon with target-unrepresentable semantics such as a hidden ability entering Gen III likewise
fails with no candidate. Representable-as-loss cases such as Z-A Alpha/Tera/divergent data may be
built as staged candidates with declared loss, but **cannot retire the source through cross-game
true Move** in the current product.

### Remaining conversion risks / enablement blockers

- broader real fixture corpus for forms, events, ribbons, marks and HOME tracker behavior;
- LGPE and PLA route-specific production goldens beyond the policy-level loss declarations;
- full language tables, especially Japanese Gen III, if those routes are to become supported rather
  than fail-closed;
- user-visible loss acknowledgement and persisted provenance before any route with declared loss can
  be source-retiring;
- `canConvert()` is a coarse dex/route preflight; final candidate creation remains authoritative for
  ability/text/language fidelity failures. This is safe today because conversion failure retains
  custody, but UI preflight should eventually expose the more specific reason before route enablement;
- profile/account/current-location provenance remains a store/session responsibility rather than a
  PKM field and needs end-to-end fixtures before future Vault authority.

### Tranche stop

F05-F13 conversion-fidelity work stops here. Do not automatically begin BDSP multi-file
transactions, N06 directory promotion, Master Vault, Gen IV/DS/3DS, or live source writes.


---

# Route-level conversion golden corpus expansion — 2026-09-24

This tranche audits forward from the F05–F13 closeout and keeps every cross-game A04b true-Move route disabled.

## Exact code checkpoint before this documentation capture

```text
application SHA:
0e67c7be80a395db730f290d79844e3e0b22dafa

tree:
f292fc3be568b8c86cbad0e80f4adc3bcc2cd84d

PokeBank NX Host Tests:
36029879834 / #1142 / SUCCESS

normal host suite:
SUCCESS

focused RSE regression:
SUCCESS

ASan + UBSan:
SUCCESS

Audit Hardening Native Validation:
36029871975 / #46 / SUCCESS
```

This is CI verification only. It is not physical-device acceptance.

## Commits in the route-corpus tranche

```text
15966d976600a229b2077f1b62684989c36eb645  test(conversion): expand route-level production goldens
05752187fca86260834ac50a2c7d285615ffec92  fix(conversion): declare tracker ribbon and mark loss
ee03c7841163cf8c44cee36c9592dd69f3779130  feat(conversion): add route preflight and provenance evidence contract
146f9956dcce83420819aa8267dbd4887df3d21e  ci: validate conversion fidelity changes natively
d2ff53857b5acb249c77cda8fbc1660c247e5c31  fix(conversion): declare LGPE held-item loss before remap
8efd8d122c2705aee4fbd24cd5383d884e655453  test(conversion): complete tracker and preflight route matrix
5419710cb337a5b9e63cdf860a6f6db2f100eb6d  test(conversion): use route-legal event move fixture
809e1f568ea9b1c457b127e498814cc0d1b1502f  test(conversion): derive route move fixtures from learnsets
0e67c7be80a395db730f290d79844e3e0b22dafa  test(conversion): scope SWSH-SV learnset fixtures correctly
```

## New production golden fixtures / route slices

The expanded production-entity corpus now exercises:

- modern tracker-bearing Pokémon -> PB7/LGPE and PK3/Gen III trackerless targets, with explicit `HomeTrackerDropped`;
- Gen III ribbon payload -> Sword, with explicit `RibbonDataDropped`;
- LGPE Alolan Vulpix -> Sword -> LGPE, including form custody, fateful state and AV/EV reset policy;
- Sword -> LGPE, including held-item loss, stat-training reset and modern-mark loss;
- LGPE Melmetal -> Sword, covering mythical/genderless/shiny semantics;
- PLA Alpha/event Pokémon -> Sword, covering tracker, ribbons/marks, Alpha/PLA-exclusive loss and destination reparse;
- Sword -> PLA, covering tracker/ribbon/mark survival and held-item loss;
- Hisuian Zorua -> Sword fail-closed when the destination form is unavailable;
- Sword -> Scarlet/Violet -> Sword event-style fixture with HOME tracker, ribbon/mark, fateful state, one route-legal carried move and one route-illegal move that is explicitly dropped;
- Sword <-> Z-A tracker preservation in the tested no-Alpha/no-divergent-payload slice;
- BDSP -> Sword tracker preservation;
- BDSP <-> Scarlet/Violet tracker preservation plus target-default Tera synthesis / Tera-loss reporting;
- supported international Gen III language IDs through Sword -> PK3;
- Japanese/Korean/Chinese-style unsupported Gen III language IDs remaining explicit `LanguageNotRepresentable`;
- HOME-tracker matrix across PA8/PK9/PA9 -> PB7/PK3 and trackerless PB7/PK3 -> PK8 without tracker invention;
- production `preflightConvert()` using the real converter to expose held-item/stat losses and hidden-ability/language failures without mutating the source;
- F13 `RouteEvidence` data contract separating historical origin, native source/destination formats, store ownership, profile/account identity, source/destination payload digests and explicit loss acknowledgement.

Earlier F05–F13 fixtures remain part of the corpus, including direct S/V <-> Z-A divergent/Tera tests and Gen III shiny/PID/ability/text/EV coverage.

## New exact-current defects found and fixed

### P2 — HOME tracker / mark / ribbon loss could be silent on trackerless down-conversion

PB7 and PK3 have no HOME tracker field. Modern marks are also not representable there, and current Gen III ribbon remaps do not preserve every ribbon bit.

The route corpus reproduced silent semantic disappearance. Production conversion now declares:

```text
HomeTrackerDropped
MarkDataDropped
RibbonDataDropped
```

before a future source-retiring policy could ever accept such a candidate.

### P2 — LGPE held-item loss was cleared before generic loss reporting

LGPE has no held-item mechanic. The PB7 remap cleared the field before the later destination sanitizer could observe it, so `HeldItemDropped` could be omitted from the fidelity report.

The conversion now reports `HeldItemDropped` while the source held item is still observable, before PB7 remapping.

### P2 — route event-move fixture was not initially route-valid / correctly scoped

The expanded host corpus exposed that hard-coded move assumptions were not reliable across the current learnset tables. The fixture now derives a move legal in both Sword and S/V plus a Sword-only move from the production learnset table, assigns them in the SWSH->SV fixture itself, proves the shared move survives, and proves the Sword-only move is dropped with `MoveDropped`.

This was a test-fixture/build defect, not a newly proven production conversion defect.

No new P0 finding was produced by this tranche.

## HOME tracker disposition

- PK8/PA8/PK9/PA9 routes with tracker fields: preservation is fixture-proven only for the tested route slices.
- PB7 and PK3: no tracker field; nonzero incoming tracker is an explicit `HomeTrackerDropped` loss.
- PB7/PK3 -> tracker-capable modern formats: conversion does not invent a tracker.
- A tracker-loss candidate is not eligible for unacknowledged source-retiring true Move.

## Ribbon / mark disposition

- Common modern ribbon/mark bytes survive in the tested PLA/SWSH and SWSH/SV slices where both representations carry them.
- Gen III ribbon payload that the current modern remap cannot preserve is explicitly `RibbonDataDropped`.
- Modern marks entering PB7/PK3 are explicitly `MarkDataDropped`.
- No route is considered fully ribbon/mark proven across all species/events/formats.

## LGPE disposition

**ROUTE MUST REMAIN DISABLED** for source-retiring cross-game true Move.

Proven slices now include:

- ordinary modern <-> LGPE conversion infrastructure;
- Alolan Vulpix form custody;
- Melmetal mythical/genderless/shiny case;
- held-item loss;
- EV/AV stat-training reset;
- modern mark loss entering PB7;
- source immutability + destination serialize/reparse/checksum.

Remaining blockers include broader Kanto/Alolan form corpus, LGPE-specific balls/met semantics, event corpus, language breadth, move/relearn coverage, ribbon edge cases and acknowledgement/provenance for declared losses.

## PLA disposition

**ROUTE MUST REMAIN DISABLED** for source-retiring cross-game true Move.

Proven slices now include:

- ordinary SWSH <-> PLA entity conversion;
- Alpha/PLA-exclusive loss declaration on PLA -> SWSH;
- tracker/ribbon/mark survival in tested slices;
- held-item loss on SWSH -> PLA;
- Hisuian Zorua destination absence failing closed instead of flattening.

Remaining blockers include broader Hisuian/permanent-form coverage, PLA balls, mastery/GV semantics, event/special cases, language breadth and explicit acknowledgement/provenance for loss-bearing candidates.

## Event / special-Pokémon disposition

The corpus now includes event-style/fateful metadata, tracker, ribbon/mark and route-legal/route-illegal move behavior in representative modern fixtures.

This is **NEEDS MORE FIXTURES**, not route-wide proof. Fixed-PID/EC distributions, broader Cherish/event-ball semantics, special ribbons/marks, species-specific event moves and historical distribution quirks remain incomplete.

## F13 provenance / loss-acknowledgement result

A data-only `Conversion::RouteEvidence` contract now exists for future source-retiring policy. It keeps these concepts separate:

- historical origin;
- source native format;
- destination native format;
- source store;
- destination store;
- profile/account exact-game ownership;
- source payload digest;
- destination payload digest;
- fidelity losses/adaptations;
- explicit loss acknowledgement.

`preflightConvert()` reuses the production converter rather than creating a second rules engine. It can expose semantic conversion failures and loss reports without mutating the source.

This contract **does not enable Move** and is not yet persisted end-to-end through the A04b journal/UI/provenance flow.

## Route-level future true-Move gate

```text
ROUTE ELIGIBLE FOR FUTURE TRUE-MOVE ENABLEMENT:
NONE
```

Every cross-game route remains:

```text
ROUTE MUST REMAIN DISABLED
```

Current route slices are materially stronger, but no entire pair has a sufficiently broad species/form/event/ribbon/mark/language/game-specific corpus plus persisted loss acknowledgement/provenance to authorize source retirement.

In particular:

- LGPE routes remain blocked by incomplete PB7-specific corpus and unavoidable declared-loss cases;
- PLA routes remain blocked by incomplete Hisuian/Alpha/ball/mastery/GV/event coverage;
- SWSH <-> SV remains blocked despite event/tracker/move/Tera fixtures because full forms/events/ribbons/marks and loss acknowledgement are incomplete;
- SWSH <-> Z-A remains blocked because Alpha/divergent/form/event coverage is not route-complete;
- BDSP <-> SWSH and BDSP <-> SV remain blocked because tested tracker/Tera slices do not constitute route-wide proof;
- Gen III <-> modern remains blocked by language gaps, ribbon/history loss, PID-correlated semantics and incomplete event/form corpus;
- direct S/V <-> Z-A remains blocked by broader forms/events/marks/ribbons and loss acknowledgement/provenance.

## Remaining conversion risks

- broader species/form corpus per exact game pair;
- event distributions and special ribbons/marks/balls;
- HOME-tracker semantics across more real migrated entities;
- LGPE ball/met/AV and PLA ball/mastery/GV edge cases;
- Japanese Gen III and other unsupported text tables;
- persisted/user-visible declared-loss acknowledgement;
- end-to-end F13 provenance carried through A04b journal/recovery and future Vault history;
- route-specific preflight UX using the shared production result without weakening final candidate validation.

## Tranche stop

The route-level golden-corpus expansion stops here. Do not automatically enable cross-game true Move or begin BDSP multi-file transactions, N06, Master Vault, Gen IV/DS/3DS, or live source writes.


---

# Declared-loss acknowledgement + F13 persisted provenance — 2026-09-25

This tranche audits forward from the route-level conversion corpus checkpoint and builds the durable
evidence/policy layer required before any future loss-bearing cross-game true Move could retire its
source. **No cross-game route is enabled by this work.**

## Exact starting checkpoint

```text
starting audit head:
65bf91b2790d1a0a12b9d889b4bd60730933f289

starting tree:
a823e92c6f9bb0273845bc73daf57f692cffd79b
```

GitHub had already advanced with the core implementation when this audit resumed, so the work was
audited forward rather than reset/rebased backward.

## Exact code checkpoint before this documentation capture

```text
application SHA:
5a1cd9685dfe3dd37a3002ac1193795dcb676e0d

tree:
3bf1f2882d4c1c44c1b76be22e5b7b7a99a3a27b

PokeBank NX Host Tests:
36095559792 / #1162 / SUCCESS

normal host suite:
SUCCESS

focused RSE regression:
SUCCESS

ASan + UBSan:
SUCCESS

Audit Hardening Native Validation:
36095556266 / #64 / SUCCESS
```

This is CI verification only, not physical-device acceptance.

## Durable conversion-evidence format

A separate durable conversion-evidence record is stored beside the transaction journal under the
same transaction ID:

```text
transactions/records/<tx-id>.conversion-evidence.pbce
```

Current conversion-evidence schema:

```text
PBCE v1
kRouteEvidenceVersion = 1
magic = PBCEV01
maximum record size = 64 KiB
SHA-256 integrity trailer
persistence = DurableFile::replace() + parse/round-trip validator
```

The record is fail-closed on truncation, malformed payloads, bad integrity hash, unknown fidelity
bits, invalid store/game identity, invalid acknowledgement state, and unsupported/newer schema.

Stored evidence includes:

- transaction ID;
- exact source game identity;
- exact destination game identity;
- source native format;
- destination native format;
- historical origin version;
- source StoreDescriptor;
- destination StoreDescriptor;
- source payload SHA-256;
- destination candidate payload SHA-256;
- diagnostic source species/form/PID/EC;
- diagnostic destination species/form/PID/EC;
- conversion result;
- candidate-available flag;
- fidelity Loss bitset;
- fidelity Adaptation bitset;
- source-origin / destination-entity-origin fidelity metadata;
- whether losses were shown to the user;
- whether losses were explicitly acknowledged;
- creation timestamp;
- acknowledgement timestamp;
- acknowledgement binding SHA-256;
- provenance relation.

Profile/account identity remains in StoreDescriptor/provenance metadata. It is not written into PKM
payload fields.

## Fidelity presentation contract

Every current Loss has a stable user-facing mapping:

```text
Gen3EVClamped
TeraDataDropped
ZAAlphaDropped
DivergentGameDataDropped
OriginGameRestamped
MoveDropped
RelearnMoveDropped
HeldItemDropped
StatTrainingReset
PLAExclusiveDataDropped
RibbonDataDropped
AbilitySlotNormalized
HomeTrackerDropped
MarkDataDropped
```

Every current Adaptation also has a stable separate mapping:

```text
PidAdjustedForShinyThreshold
TargetDefaultTeraSynthesized
MovePPClamped
Gen3TransferDateSynthesized
DefaultNicknameCanonicalized
```

The mapping-completeness regression checks the Fidelity enum declarations against the presentation
catalog. Unknown/unrecognized Loss or Adaptation bits are not treated as safe and cannot authorize
retirement.

Losses and adaptations remain separate concepts in the presentation model.

## Acknowledgement policy

A successful candidate with no Loss entries does not require a loss acknowledgement. That does
**not** imply route enablement.

A candidate with any declared Loss requires:

1. the exact loss summary to have been shown;
2. explicit user acknowledgement;
3. a nonzero acknowledgement timestamp;
4. an acknowledgement SHA-256 binding matching the exact evidence semantics.

The acknowledgement binding covers the transaction ID, exact source/destination game identities,
native formats, historical origin, relation, both store descriptors, source/destination payload
hashes, diagnostic entity identities, conversion result/candidate state, Loss bitset, Adaptation
bitset, and source/destination origin metadata.

If any bound field changes, the acknowledgement becomes stale and retirement fails closed.

### P2 found during forward audit — shown-vs-acknowledged conflation

At the live forward head, `markLossesAcknowledged()` was setting `lossesShownToUser = true`
itself. That allowed an acknowledgement call to manufacture the evidence that the loss-summary
presentation step had happened.

Fixed in:

```text
36e87c578e23a56d704c418b47d9e602efbbcb17
fix(conversion): require loss summary before acknowledgement

9f64df839d6ef0f3e593ddfae80faacca252f110
test(conversion): separate shown-loss proof from acknowledgement

5a1cd9685dfe3dd37a3002ac1193795dcb676e0d
test(conversion): mark loss summary shown before acknowledgement
```

The helper now refuses a loss-bearing acknowledgement until the caller has independently recorded
that the deterministic loss summary was shown.

## A04a integration / recovery boundary

The Move journal is versioned through schema v2 for the cross-game conversion flag while retaining
support for the older v1 journal. Unknown newer journal versions remain fail-closed.

Conversion evidence is a separate durable file keyed by transaction ID. The transaction engine uses
a retirement gate for cross-game transactions.

Evidence is checked before:

```text
DESTINATION_VERIFIED -> SOURCE_RETIRE_PENDING
```

It is checked again after restart while in:

```text
SOURCE_RETIRE_PENDING
```

before touching the source, and checked again while in:

```text
SOURCE_RETIRED
```

before allowing the journal to reach:

```text
COMMITTED
```

Therefore missing/corrupt/unsupported/stale evidence cannot silently authorize source retirement
after a restart. If evidence disappears after SOURCE_RETIRED, COMMITTED is withheld for manual
reconciliation rather than finalizing a transaction without provenance.

Transaction-ID allocation also treats an existing conversion-evidence record as reserving that ID,
so stale/orphan evidence is not silently reused by a new transaction.

## Corruption / mismatch regressions

The evidence suite covers fail-closed behavior for:

- truncated evidence;
- corrupted bytes / integrity mismatch;
- unsupported newer evidence schema;
- missing evidence at retirement;
- stale Loss bitset after acknowledgement;
- stale Adaptation bitset after acknowledgement;
- source payload hash mismatch;
- destination payload hash mismatch;
- store/profile mismatch;
- route/exact-game mismatch;
- transaction-ID mismatch;
- ambiguous duplicate move match inside a transaction;
- unknown fidelity bits;
- failed/unavailable conversion candidate;
- evidence removal/corruption after SOURCE_RETIRE_PENDING;
- evidence removal after SOURCE_RETIRED before COMMITTED;
- restart/recovery at every A04a interruption boundary.

Negative retirement tests prove the source bytes remain exact, source SHA-256 remains exact, and the
source Store remains authoritative until an authorized retirement actually occurs.

## Route-policy separation

The retirement policy keeps these concepts distinct:

```text
candidate valid
loss policy satisfied
evidence matches transaction/candidate
route enabled
transaction state safe
```

Product policy remains:

```text
routeEnabledForTrueMove(...) = false
```

for every cross-game route.

A regression proves that even a valid, zero-loss conversion with valid persisted evidence cannot
retire the source while the route gate is disabled.

## F13 provenance result

The persisted record and `ProvenanceNode` keep historical origin separate from current transaction
location/ownership.

The provenance contract can represent:

```text
historical origin
source exact game / source store
destination exact game / destination store
profile/account ownership
transaction ID / sequence
source payload hash
destination payload hash
Losses
Adaptations
relationship
```

Historical origin is not rewritten merely because the current destination changes.

Relationship names remain explicitly separated for future history:

```text
CONVERSION
MOVE
COPY
EXACT_CLONE_OF
DERIVED_FROM
ARCHIVE_RECOVERY
```

This does not begin Master Vault or clone expansion.

## Route gate after this tranche

```text
ROUTE ELIGIBLE FOR FUTURE TRUE-MOVE ENABLEMENT:
NONE
```

Every cross-game route remains:

```text
ROUTE MUST REMAIN DISABLED
```

This tranche removes a major shared infrastructure blocker but does not make incomplete route
coverage disappear. Exact-pair species/form/event/ribbon/mark/language/game-specific evidence and
the physical recovery gate remain separate requirements.

No route is promoted to enabled status by this tranche.

## Findings classification

- **P0:** none newly confirmed.
- **P1:** none newly confirmed by this tranche.
- **P2:** acknowledgement helper conflated "loss summary shown" with "user acknowledged"; fixed and
  regression-covered.
- Existing physical FAT32/exFAT recovery acceptance, BDSP multi-file durability and N06 remain
  outside this tranche and unchanged.

## Tranche stop

Declared-loss acknowledgement + persisted F13 provenance infrastructure is implemented and tested
at the code checkpoint above.

Do not automatically begin route enablement, Master Vault, BDSP multi-file transactions, N06,
Gen IV/DS/3DS, live writes, or clone expansion.


---

# SWSH <-> S/V exact-pair route completion audit — 2026-09-25

This tranche audits Sword / Shield <-> Scarlet / Violet as eight exact title directions. It does
**not** enable any cross-game true-Move route.

## Starting checkpoint

```text
starting head:
62b638cd614bbb322581b032afb845559a0d572c

starting tree:
1d0ab45b705e409cff8a73dfaeeffa1028db3177
```

GitHub advanced before this audit resumed. The branch was audited forward without reset/rebase.

## Exact code checkpoint before this documentation capture

```text
application SHA:
8979478043d305378cb3e10340be6209d2382991

PokeBank NX Host Tests:
36101608492 / #1182 / SUCCESS

Audit Hardening Native Validation:
36101605089 / #83 / SUCCESS
```

This is CI verification only, not device acceptance.

## Forward commits included in this tranche

```text
190a6c34f57e08dc1c0b692eb524ed5799eaaae2  test(conversion): expose SWSH-SV divergent field loss
1e4f64ab1bec906e05b2bfa9022b9606c6e1dcfc  fix(conversion): preserve status and declare SWSH-SV divergent loss
55ac1e30e21c15253edfe1eb4f877e3a5da672c8  test(conversion): require explicit SWSH-SV synthesized-field adaptations
b4c2aec9e2540533d70b1d83373205933d7eedd0  fix(conversion): declare SWSH-SV synthesized field adaptations
53475e6579181ab0772161a9d13123d8898f6ac5  fix(conversion): declare SWSH-SV synthesized field adaptations
1f36ba1117592883998ac2a638191ab187ceb3f3  fix(conversion): declare SWSH-SV synthesized field adaptations
01104beeae7171ce9907654b273dba631ef5b4d2  fix(conversion): declare SWSH-SV synthesized field adaptations
94af61125a816039e16c73cd8ba314c7bfc29b04  test(conversion): expand SWSH-SV exact-pair production corpus
a8bcedb3df36934170aaae3bc5400b0bdc03736c  test(conversion): use native IV EV accessors in exact-pair corpus
4a65467b4ab971af330079c691598f32de96c648  test(conversion): bind all SWSH-SV exact title routes
2ca694d3bd6f6f77dd8aa619e61d63e97da34034  test(conversion): fail closed on SWSH-SV ability-table divergence
9040a34c043a3be4dec3f66abdf51ea538d8080d  fix(conversion): fail closed on SWSH-SV ability changes
d8f122298431dc4fc4c52b7bff4f853b387a7b5d  test(conversion): expose SWSH-SV base-stat tail drift
e1ad0a3ab8a7ded90fdbbde56a9d5f17dd1473a7  fix(pokemon): add Sword Shield base-stat override accessor
98e6ced199c9b85f6c77643d7aeca25194d98d95  fix(pokemon): model SWSH generation-specific base stats
5011dfd03459b6333c38789abfdad5f7e22b274d  fix(pokemon): use SWSH-native base stats for PK8
4d604fcb0f06fd31d9d99c62925939cd0e18de4e  fix(conversion): recalculate SWSH-SV destination battle stats
49a0807576947a6476640fe9217f032ced15f86a  test(conversion): fix exact-pair corpus compile warnings
f4d926eb81f6792377529dc4871526b29831067d  test(conversion): close SWSH-SV PP text and immutability gaps
c94acf9e9769dc6cd092fcfde4aab36fa911797c  test(conversion): pin friendship and historical origin across SWSH-SV
8979478043d305378cb3e10340be6209d2382991  test(conversion): expose SWSH-SV fidelity in presentation model
```

## Files changed in the forward exact-pair implementation

```text
include/Conversion/Fidelity.h
include/Pokemon/BaseStatsGen89.h
src/Conversion/Convert.cpp
src/Conversion/RouteEvidence.cpp
src/Pokemon/BaseStatsGen89.cpp
src/Pokemon/Pokemon8SWSH.cpp
tests/test_conversion_entity_golden.cpp
tests/test_conversion_evidence.cpp
```

## Exact-pair species / form matrix

The production personal table now pins the current SWSH/SV intersection rather than inferring
compatibility from one species:

```text
base species:
shared       420
SWSH-only    244
SV-only      313
neither       48

alternate forms:
shared        71
SWSH-only     50
SV-only      175
neither      169
```

Production fixtures prove representative base-species absence and regional-form absence return
`NotInDex`, produce no candidate and leave the source bytes/hash exact.

Representative shared forms currently covered include:

```text
Alolan Vulpix
Galarian Meowth
Rotom alternate form
```

Unavailable Galarian Ponyta and Paldean Tauros form examples fail closed instead of flattening.

Classification: **PROVEN BY SHARED IMPLEMENTATION + FIXTURE** for the personal-table gate itself;
**NEEDS MORE EVIDENCE** for route-wide species-specific form semantics and special/battle-only forms.

## Exact title identities

Production fixtures bind and exercise all eight exact title identities:

```text
Sword  -> Scarlet
Sword  -> Violet
Shield -> Scarlet
Shield -> Violet
Scarlet -> Sword
Scarlet -> Shield
Violet  -> Sword
Violet  -> Shield
```

Sword/Shield share the PK8 transform and Scarlet/Violet share the PK9 transform, but exact source
and destination versions remain separately represented in the conversion/provenance evidence.

## Coverage matrix

| Semantic category | Exact-pair result |
|---|---|
| species availability | **PROVEN BY SHARED IMPLEMENTATION + FIXTURE** — generated personal-table presence matrix + representative fail-closed source-immutability fixtures |
| forms | **NEEDS MORE EVIDENCE** — representative regional/permanent form preservation and fail-closed absence proven; not every special/form-specific rule class is closed |
| shiny / PID / EC | **PROVEN BY PRODUCTION FIXTURE** — modern XOR shiny classes plus non-shiny; PID/EC preserved |
| abilities | **PROVEN BY PRODUCTION FIXTURE / UNSUPPORTED FAIL CLOSED** — slots 1/2/hidden/duplicate covered; Shiftry/Gallade generation-specific slot-2 changes fail `AbilityNotRepresentable` |
| moves | **PROVEN BY PRODUCTION FIXTURE** — production learnsets dynamically derive shared/source-only classes; source-only moves are `MoveDropped` |
| relearn moves | **PROVEN BY PRODUCTION FIXTURE** — shared survive; destination-impossible relearns are `RelearnMoveDropped` |
| PP / PP Ups | **PROVEN BY PRODUCTION FIXTURE** — carried with moves and destination PP drift clamps with `MovePPClamped` |
| held items | **PROVEN BY SHARED IMPLEMENTATION + FIXTURE** — generated item-presence tables cover shared/source-only classes; unsupported item reports `HeldItemDropped` |
| ribbons | **NEEDS MORE EVIDENCE** — representative shared ribbon bytes preserve; broad event/special/distribution ribbon semantics are not route-complete |
| marks | **NEEDS MORE EVIDENCE** — representative mark-byte regions preserve; full semantic mark catalog/reserved-bit classification is incomplete |
| HOME tracker | **PROVEN BY PRODUCTION FIXTURE** — zero and multiple nonzero u64 values survive both directions and serialize/reparse |
| event / fateful | **NEEDS MORE EVIDENCE** — fateful, Cherish Ball, tracker, representative ribbon/mark and special-move style state covered; historical distribution-specific PID/EC/met/ribbon constraints remain incomplete |
| balls | **PROVEN BY REPRESENTATIVE PRODUCTION FIXTURE / NEEDS MORE EVIDENCE** — ordinary + special representative IDs serialize/reparse; complete generation-specific invalid-ball policy is not yet proven |
| language / text | **PROVEN BY REPRESENTATIVE PRODUCTION FIXTURE / NEEDS MORE EVIDENCE** — language IDs 1,2,3,4,5,7,8,9,10 and Japanese/Korean/Chinese UTF-16 examples preserve; full boundary/invalid UTF-16 policy remains incomplete |
| origin / met / history | **PROVEN BY PRODUCTION FIXTURE** for current transform — entity historical origin remains source history, including older BDSP origin living in Sword; exact store provenance remains F13 metadata |
| Tera | **LOSS / ADAPTATION EXPLICIT** — SWSH->SV synthesizes destination-default Tera with `TargetDefaultTeraSynthesized`; SV->SWSH reports `TeraDataDropped` |
| IV / EV / nature / mint nature | **PROVEN BY PRODUCTION FIXTURE** for representative shared values |
| friendship | **PROVEN BY PRODUCTION FIXTURE** |
| Hyper Training flags | **PROVEN BY PRODUCTION FIXTURE** for representative shared flag bytes |
| Pokerus | **PROVEN BY REPRESENTATIVE FIXTURE** for carried native state |
| battle-stat tail | **CONFIRMED DEFECT -> FIXED / PROVEN BY FIXTURE** — destination stats now recalculate against generation-specific base stats |
| PK8-only semantics | **LOSS DECLARED** — G-Max flag, Sociability, Dynamax level, Palma, PokeJob, Fullness/Enjoyment, TR records contribute to `DivergentGameDataDropped`; status is relocated, not lost |
| PK9-only semantics | **LOSS / ADAPTATION EXPLICIT** — Tera loss, Scale/Obedience divergence and TM-record regions feed explicit fidelity behavior |
| Scale | **ADAPTATION / LOSS POLICY EXPLICIT** — SWSH->SV synthesizes Scale from Height via `TargetScaleSynthesized`; SV->SWSH reports divergent loss when Scale differs from Height |
| ObedienceLevel | **ADAPTATION / LOSS POLICY EXPLICIT** — SWSH->SV synthesizes from MetLevel via `TargetObedienceLevelSynthesized`; SV->SWSH reports divergent loss when distinct |
| unknown / reserved bytes | **BLOCKER** — no proof yet that every remaining unmodeled PK8/PK9 byte is harmless padding; route-complete source retirement cannot be claimed |
| round trips | **PROVEN BY REPRESENTATIVE PRODUCTION FIXTURE** — Sword->Scarlet->Sword, Shield->Violet->Shield, Scarlet->Sword->Scarlet, Violet->Shield->Violet |
| preflight | **PROVEN BY PRODUCTION FIXTURE** — preflight and conversion Loss/Adaptation bitsets match on tested exact-pair cases |
| acknowledgement / F13 | **PROVEN BY EVIDENCE FIXTURE** — all eight exact game-store identities bind into acknowledgement; stale hashes/route identity fail closed |
| route-disabled override | **PROVEN** — product route gate remains false |

## New fidelity semantics

The SWSH/SV audit adds deterministic adaptations for destination-only fields:

```text
TargetScaleSynthesized
TargetObedienceLevelSynthesized
```

They are separate from destructive Loss values and are included in the presentation-model
completeness contract.

SWSH-specific source semantics that cannot exist in PK9 contribute to:

```text
DivergentGameDataDropped
```

SV -> SWSH continues to report:

```text
TeraDataDropped
```

and reports divergent loss when Scale, ObedienceLevel or move-record data cannot survive exactly.

## Confirmed defects found and fixed during the exact-pair audit

### P1 — SWSH/SV source-only fields could disappear without complete fidelity accounting

The focused corpus exposed known generation-specific regions that were being cleared/relocated
without all semantic changes being represented. The converter now preserves status condition and
declares source-only divergence instead of silently zeroing meaningful state.

### P1 — destination-only Scale / Obedience synthesis was implicit

PK8 has no independent PK9 Scale or ObedienceLevel fields. SWSH -> S/V now reports deterministic:

```text
TargetScaleSynthesized
TargetObedienceLevelSynthesized
```

rather than silently creating those fields.

### P1 — two shared species have incompatible normal ability slot 2 across generations

Current personal data identifies:

```text
Shiftry
Gallade
```

with slot-2 ability changes between SWSH and S/V.

The route now fails closed with `AbilityNotRepresentable` instead of carrying a destination-invalid
ability ID or silently substituting another effect.

### P1 — copied battle-stat tail could use the wrong generation's base stats

Cresselia and other shared species have base-stat changes between SWSH and S/V. Copying cached party
stats across generations could therefore create a destination entity whose stored battle stats did
not match destination-native base stats.

The converter now models SWSH-specific base-stat overrides for PK8 and recalculates the destination
battle-stat tail. Cresselia provides the focused two-way regression.

No new P0 was found by this tranche.

## Event / ribbon / mark disposition

The exact-pair corpus now proves representative event-like state:

- fateful encounter;
- Cherish Ball;
- HOME tracker;
- custom OT/nickname;
- representative ribbon and mark bytes;
- route-legal moves and route-illegal move loss;
- exact PID/EC preservation for modern representation.

This is still **NEEDS MORE EVIDENCE**, not historical event-distribution completeness. The converter
does not yet have a route-wide proof for every special/distribution ribbon, mark, event met rule,
fixed PID/EC distribution rule or event-specific move exception.

## Language / text disposition

Modern PK8 and PK9 use compatible UTF-16-style name storage in the current project model. Fixtures
cover the project language IDs:

```text
1  Japanese
2  English
3  French
4  Italian
5  German
7  Spanish
8  Korean
9  Simplified Chinese
10 Traditional Chinese
```

Non-Latin Japanese/Korean/Chinese nickname + OT examples preserve across both directions.

Remaining text blocker for a route-complete claim: explicit boundary corpus for maximum lengths,
terminators/trash handling and malformed/unpaired UTF-16 code units is not yet complete.

## HOME tracker result

Tracker values tested include zero and multiple nonzero 64-bit values. PK8 offset `0x135` and PK9
offset `0x127` relocate exactly in both directions, survive destination serialization/reparse and
round trips, and are not invented or randomized.

## Origin / F13 result

Historical origin remains an entity-history field, not the current store. A BDSP-origin entity
currently living in Sword and converted to Scarlet remains BDSP-origin.

All eight exact title/store identities are included in the persisted acknowledgement binding.
Changing Sword<->Shield, Scarlet<->Violet, either StoreDescriptor or either payload digest invalidates
the evidence.

This does not authorize retirement.

## Source immutability

Exact-pair success and failure fixtures capture encrypted native source bytes and SHA-256 before
conversion/preflight. They require the exact source bytes and SHA-256 to remain unchanged afterward.

`NotInDex` and `AbilityNotRepresentable` failure fixtures also prove no candidate is created and
the source remains byte-identical.

No route test retires a source slot.

## Unknown / reserved-data blocker

The known PK8/PK9 divergent regions now have materially better coverage, but the audit cannot prove
that every remaining unmodeled/reserved byte is semantically inert across all legitimate entities.

Per the fail-closed route-completion policy, that uncertainty is a **BLOCKER** to declaring an exact
direction route-complete for source-retiring true Move.

## Direction-by-direction decision

All directions remain disabled:

```text
Sword  -> Scarlet   ROUTE MUST REMAIN DISABLED
Sword  -> Violet    ROUTE MUST REMAIN DISABLED
Shield -> Scarlet   ROUTE MUST REMAIN DISABLED
Shield -> Violet    ROUTE MUST REMAIN DISABLED
Scarlet -> Sword    ROUTE MUST REMAIN DISABLED
Scarlet -> Shield   ROUTE MUST REMAIN DISABLED
Violet -> Sword     ROUTE MUST REMAIN DISABLED
Violet -> Shield    ROUTE MUST REMAIN DISABLED
```

None is promoted to `POTENTIAL ROUTE CANDIDATE FOR FUTURE ENABLEMENT` yet.

Shared blockers:

- unknown/reserved PK8/PK9 semantic classification is incomplete;
- route-wide special/event distribution semantics are incomplete;
- ribbon/mark semantic catalog coverage is incomplete;
- special/form-specific edge corpus remains incomplete;
- complete invalid/special-ball policy is not fully fixture-proven;
- full text-boundary/malformed-text corpus is incomplete;
- physical Switch FAT32/exFAT power-loss acceptance remains separate and open.

## True-Move gate

```text
ROUTE ELIGIBLE FOR FUTURE TRUE-MOVE ENABLEMENT:
NONE
```

Product `routeEnabledForTrueMove()` remains false.

No cross-game source retirement is enabled by this tranche.

## Recommended next tranche

Do not activate SWSH/SV yet.

The highest-value next work is a **PK8/PK9 unknown-reserved-byte + event/ribbon/mark closure audit**,
focused only on the exact fields still preventing SWSH/SV from being classified as a potential route
candidate. Keep route enablement off until that closure audit is complete.

## Tranche stop

Stop here. Do not begin route activation, BDSP multi-file transactions, N06, Master Vault,
Gen IV/DS/3DS, live source writes or clone expansion.

# PK8/PK9 semantic closure audit — 2026-09-25

## Identity, scope and verdict

Live starting PR #79 SHA: `48ca1386571262e133ab9975d35caf59fe549970`.
This is newer than the supplied `be39c6f1eb422a3b5410ddaa9004294e1e709650` checkpoint.
All intervening work was retained. Main was fetched at
`9ec20730e23681c6f7451f3da4944a156303f494`; its public status pages were read.
PR #77 and its device-accepted `996e6aa4` application were not changed.

**All eight directions remain ROUTE MUST REMAIN DISABLED.** No route is a potential
candidate yet. This audit closes specific representation and reporting cases; it does
not claim every unmodeled bit, official trade rule or distribution is understood.
`routeEnabledForTrueMove()` remains false. Original saves remain immutable and all
installed-game/emulator write locks remain hard off.

The final exact SHA, tree and workflow run IDs are recorded in PR #79 / issue #69
metadata after both workflows finish. They are intentionally not baked into an extra
self-invalidating documentation-only commit. A workflow success is CI VERIFIED, never
DEVICE ACCEPTED.

## Reference basis

Use the production data-generator pin, PKHeX
`6501f0ab46e8f8ca048539dbaf8cae8cb104e722` (`tools/pkhex_source.py`), rather than
silently mixing current upstream with the older design-oracle pin in `PKHEX_ORACLE.md`.
Reference models were read at that exact revision:

- [G8PKM](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/PKM/Shared/G8PKM.cs), [PK8](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/PKM/PK8.cs), [PK9](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/PKM/PK9.cs): offsets, widths, unused comments and named properties.
- [RibbonIndex](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Ribbons/RibbonIndex.cs): semantic names and MAX_G8 / MAX_G9 boundaries.
- [Ball](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Game/Enums/Ball.cs): complete named numeric domain.
- [LocationsHOME](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Game/Locations/LocationsHOME.cs), [PKH](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/PKM/HOME/PKH.cs): target representation versus historical origin.
- [FormInfo](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Tables/FormInfo.cs), [TradeRestrictions](https://github.com/kwsch/PKHeX/blob/6501f0ab46e8f8ca048539dbaf8cae8cb104e722/PKHeX.Core/Legality/Tables/TradeRestrictions.cs): transient/fused transfer restrictions.

Project counterparts are `Pokemon8SWSH.h`, `Pokemon9SV.h`, the encryption size
constants, `Convert.cpp`, and generated personal/move/item tables. Synthetic fixtures
exercise representation rules; no additional real event-save corpus was available.
A model's `ExtraBytes` list alone is not padding proof: PK8 PokéJob is on that list but
has named, meaningful flag accessors and is explicitly loss-reported.

## Exact semantic field comparison

Offsets are inclusive hexadecimal decrypted offsets; widths are bytes unless bits are
specified. P = preserved, R = relocated, A = deterministic adaptation, L = explicit loss,
F = fail closed, U = UNKNOWN SEMANTIC REGION, X = model-declared unused/alignment.
The tables below plus the full-address partition account for all 328 stored bytes and
16 party bytes. P means representational preservation, not historical-event legality.

| PK8 | PK9 | Width | Meaning and disposition |
|---|---|---:|---|
| 000–003 | same | 4 | EC P; encryption/shuffling regenerated |
| 004–005 | same | 2 | Sanity header copied; legal-domain validation not newly claimed |
| 006–007 | same | 2 | Checksum recomputed over stored 008–147 |
| 008–009 | same | 2 | Species national↔Gen9 internal index A; target absence F |
| 00A–00B | same | 2 | Held item P if target supports it, else HeldItemDropped |
| 00C–00F | same | 4 | TID/SID P |
| 010–013 | same | 4 | EXP P; target party stats recalculated |
| 014–015 | same | 2 | Ability P or AbilityNotRepresentable F |
| 016 bits0–2 | same | 3 bits | Ability slot P / existing checked ability policy |
| 016 bit3 | same | 1 bit | Favorite legacy property P; oracle labels unused |
| 016 bit4 | — | 1 bit | CanGigantamax L: DivergentGameDataDropped |
| 016 bits5–7 | 016 bits4–7 | 3/4 bits | U; not ordinary ability data |
| 018–019 low12 | same | 12 bits | Six two-bit markings P; high4 U |
| 01C–01F | same | 4 | PID P, same modern shiny threshold |
| 020 / 021 | same | 1 each | Nature / mint stat nature P |
| 022 bit0 | same | 1 bit | Fateful encounter P |
| 022 bit1 | — | 1 bit | Flag2, named but meaning uncertain; L if set; semantic U remains |
| 022 bits2–3 | 022 bits1–2 | 2 bits | Gender R |
| 022 bits4–7 | 022 bits3–7 | 4/5 bits | U; reconstruction clears these bits |
| 024 | same | 1 | Form P if present and transferable; otherwise NotInDex / FormNotTransferable |
| 025 | same | 1 | U for nonzero values: oracle form getter byte, setter clears upper ushort byte |
| 026–02B | same | 6 | Individual EVs P |
| 02C–031 | same | 6 | Contest values and sheen P as carried history |
| 032 | same | 1 | Pokérus strain/days P as stored history |
| 034–03B | same | 8 | Ribbon/mark indexes0–63; semantic catalog below |
| 03C / 03D | same | 1 each | Contest/battle memory ribbon counts P |
| 040–047 | same | 8 | Ribbon/mark indexes64–127; semantic catalog below |
| 048–04B | — | 4 | Sociability L: DivergentGameDataDropped |
| 050 / 051 | 048 / 049 | 1 each | Height / weight R |
| — | 04A | 1 | Scale A from height entering SV; L on exit if different from height |
| — | 04B–057 | 13 | DLC TM flags L if nonzero; target synthesized zero |
| 058–071 | same | 26 | Nickname UTF-16 P including terminator/trash; malformed F |
| 072–079 | same | 8 | Four moves P or MoveDropped |
| 07A–07D | same | 4 | PP P or MovePPClamped A for destination maximum |
| 07E–081 | same | 4 | PP Ups P |
| 082–089 | same | 8 | Relearn moves P or RelearnMoveDropped |
| 08A–08B | same | 2 | Current HP, destination stats reconciliation; zero HP can refill under existing policy |
| 08C–08F | same | 4 | Six IVs, egg/nicknamed flags P |
| 090 | — | 1 | Dynamax Level L if nonzero |
| 094–097 | 090–093 | 4 | Status condition R |
| 098–09B | — | 4 | Palma L if nonzero; domain meaning remains incompletely evidenced |
| — | 094 / 095 | 1 each | Original/override Tera A from target type entering SV; TeraDataDropped on exit |
| 0A8–0C1 | same | 26 | HT name UTF-16 P, malformed F |
| 0C2 / 0C3 / 0C4 | same | 1 each | HT gender / HT language / current handler P |
| 0C6–0C7 | same | 2 | HT ID copied P; reference says unused? so semantic U |
| 0C8–0CB | same | 4 | HT friendship/intensity/memory/feeling P |
| 0CC–0CD | same | 2 | HT memory variable P |
| 0CE–0DB | — | 14 | PokéJob flags L if any nonzero |
| 0DC / 0DD | — | 1 each | Fullness/enjoyment L if nonzero |
| 0DE | 0CE | 1 | Historical-origin representation R / HOME-like remap A on return; F13 keeps original |
| 0DF | 0CF | 1 | BattleVersion R entering SV; any nonzero SV marker cleared with BattleVersionDropped |
| 0E0–0E1 | — | 2 | Retired region/console-region comments: U, not proven harmless historical bytes |
| 0E2 | 0D5 | 1 | Language R |
| 0E4–0E7 | 0D0–0D3 | 4 | Entire FormArgument R, not just low byte |
| 0E8 | 0D4 | 1 | Affixed title R if retained/owned; otherwise AffixedTitleDropped, none=FF |
| 0F8–111 | same | 26 | OT name UTF-16 P, malformed F |
| 112–114 | same | 3 | OT friendship / memory intensity / memory P |
| 116–117 / 118 | same | 2 / 1 | OT memory variable / feeling P |
| 119–11B | same | 3 | Egg date P |
| 11C–11E | same | 3 | Met date P |
| — | 11F | 1 | Obedience A from met level entering SV; L on exit if different |
| 120–121 / 122–123 | same | 2 each | Egg/met locations P or HOME-like markers + LocationDetailDropped |
| 124 | same | 1 | Ball P for1–26; every other byte F |
| 125 low7 / high1 | same | 7 / 1 bits | Met level / OT gender P |
| 126 low6 | same | 6 bits | Hyper Training flags P; upper2 U |
| 127–134 | — | 14 | TR records L if nonzero |
| 135–13C | 127–12E | 8 | HOME tracker R |
| — | 12F–147 | 25 | Base TM records L if nonzero; target synthesized zero |
| 148 | same | 1 | Party level deterministic destination calculation |
| 14A–155 | same | 12 | Party stat cache recalculated with destination personal data |
| 156–157 | — | 2 | DynamaxType L if nonzero; target cleared |
| — | 156–157 | 2 | PK9 unmodeled party tail U |

All PK8-only losses in the table use `DivergentGameDataDropped` unless a specific name
is shown. SV scale/obedience equal to their deterministic reconstruction do not add that
loss; independent non-default values do. This is an explicit project reconstruction
policy, not a claim of identical official HOME behavior.

## Unused-region evidence and unknown-bit ledger

The following are X on the basis of explicit `unused`, `alignment` or `padding` comments
and absent semantic accessors in the pinned models, NOT because synthetic samples are zero:

- Shared: 017, 01A–01B, 023, 033, 03E–03F, 0C5, 115, 149.
- PK8: 04C–04F, 052–057, 091–093, 09C–0A7, 0E3, 0E9–0F7,
  11F, 13D–147. PK8 ExtraBytes corroborates the stored ranges.
- PK9: 096–0A7; 0D6–0F7 is described as “remainder unused” in the model.
  Conservatively its absence of semantic accessors is recorded, not proof from real saves.

Checksum inclusion does not make a region meaningful; these stored bytes are inside the
checksum domain. Party bytes are outside that checksum but include meaningful stats.
No inference of padding was made from checksum exclusion alone.

**Remaining UNKNOWN SEMANTIC REGION inventory:** PK8 016/E0 mask, 019/F0,
022/F0, 025/FF, 0E0–0E1, 126/C0; PK9 016/F0, 019/F0, 022/F8,
025/FF, 126/C0, 156–157. Shared HT ID 0C6–0C7 has uncertain use;
PK8 Flag2 and Palma are named but not fully explained. PK9 0D6–0F7 is retained
as a conservative evidence gap despite the model's unused comment. Whole-byte accounting
labels it UNKNOWN, so it cannot be quietly promoted by a zero-filled fixture.

Unknown values can currently be copied or cleared by the experimental converter; that
is not authorization to retire a source. No new general-purpose refusal code was invented
for semantics we cannot name. The product route gate is the fail-closed boundary for
these unresolved cases. Before readiness review: obtain legitimate nonzero fixtures or
an authoritative constraint for each range, then preserve/report/refuse explicitly.

## Ribbon and mark catalog

Index0–63 lives at `034 + index/8`; index64–127 at `040 + (index-64)/8`.
Bit is `index % 8`. The two count bytes 03C/03D are not ribbon bits. The complete
named-index table follows. Shared indexes are tested individually in both directions,
including the selected-title reference. Multiple simultaneous ribbons and memory counts
also have a separate fixture. Naming comes from RibbonIndex, not guessed raw bytes.

| Index | Semantic | PK8→PK9 | PK9→PK8 |
|---:|---|---|---|
| 0 | ChampionKalos | P | P |
| 1 | ChampionG3 | P | P |
| 2 | ChampionSinnoh | P | P |
| 3 | BestFriends | P | P |
| 4 | Training | P | P |
| 5 | BattlerSkillful | P | P |
| 6 | BattlerExpert | P | P |
| 7 | Effort | P | P |
| 8 | Alert | P | P |
| 9 | Shock | P | P |
| 10 | Downcast | P | P |
| 11 | Careless | P | P |
| 12 | Relax | P | P |
| 13 | Snooze | P | P |
| 14 | Smile | P | P |
| 15 | Gorgeous | P | P |
| 16 | Royal | P | P |
| 17 | GorgeousRoyal | P | P |
| 18 | Artist | P | P |
| 19 | Footprint | P | P |
| 20 | Record | P | P |
| 21 | Legend | P | P |
| 22 | Country | P | P |
| 23 | National | P | P |
| 24 | Earth | P | P |
| 25 | World | P | P |
| 26 | Classic | P | P |
| 27 | Premier | P | P |
| 28 | Event | P | P |
| 29 | Birthday | P | P |
| 30 | Special | P | P |
| 31 | Souvenir | P | P |
| 32 | Wishing | P | P |
| 33 | ChampionBattle | P | P |
| 34 | ChampionRegional | P | P |
| 35 | ChampionNational | P | P |
| 36 | ChampionWorld | P | P |
| 37 | CountMemoryContest | P | P |
| 38 | CountMemoryBattle | P | P |
| 39 | ChampionG6Hoenn | P | P |
| 40 | ContestStar | P | P |
| 41 | MasterCoolness | P | P |
| 42 | MasterBeauty | P | P |
| 43 | MasterCuteness | P | P |
| 44 | MasterCleverness | P | P |
| 45 | MasterToughness | P | P |
| 46 | ChampionAlola | P | P |
| 47 | BattleRoyale | P | P |
| 48 | BattleTreeGreat | P | P |
| 49 | BattleTreeMaster | P | P |
| 50 | ChampionGalar | P | P |
| 51 | TowerMaster | P | P |
| 52 | MasterRank | P | P |
| 53 | MarkLunchtime | P | P |
| 54 | MarkSleepyTime | P | P |
| 55 | MarkDusk | P | P |
| 56 | MarkDawn | P | P |
| 57 | MarkCloudy | P | P |
| 58 | MarkRainy | P | P |
| 59 | MarkStormy | P | P |
| 60 | MarkSnowy | P | P |
| 61 | MarkBlizzard | P | P |
| 62 | MarkDry | P | P |
| 63 | MarkSandstorm | P | P |
| 64 | MarkMisty | P | P |
| 65 | MarkDestiny | P | P |
| 66 | MarkFishing | P | P |
| 67 | MarkCurry | P | P |
| 68 | MarkUncommon | P | P |
| 69 | MarkRare | P | P |
| 70 | MarkRowdy | P | P |
| 71 | MarkAbsentMinded | P | P |
| 72 | MarkJittery | P | P |
| 73 | MarkExcited | P | P |
| 74 | MarkCharismatic | P | P |
| 75 | MarkCalmness | P | P |
| 76 | MarkIntense | P | P |
| 77 | MarkZonedOut | P | P |
| 78 | MarkJoyful | P | P |
| 79 | MarkAngry | P | P |
| 80 | MarkSmiley | P | P |
| 81 | MarkTeary | P | P |
| 82 | MarkUpbeat | P | P |
| 83 | MarkPeeved | P | P |
| 84 | MarkIntellectual | P | P |
| 85 | MarkFerocious | P | P |
| 86 | MarkCrafty | P | P |
| 87 | MarkScowling | P | P |
| 88 | MarkKindly | P | P |
| 89 | MarkFlustered | P | P |
| 90 | MarkPumpedUp | P | P |
| 91 | MarkZeroEnergy | P | P |
| 92 | MarkPrideful | P | P |
| 93 | MarkUnsure | P | P |
| 94 | MarkHumble | P | P |
| 95 | MarkThorny | P | P |
| 96 | MarkVigor | P | P |
| 97 | MarkSlump | P | P |
| 98 | Hisui | F if present in PK8 source | L: RibbonDataDropped |
| 99 | TwinklingStar | F if present in PK8 source | L: RibbonDataDropped |
| 100 | ChampionPaldea | F if present in PK8 source | L: RibbonDataDropped |
| 101 | MarkJumbo | F if present in PK8 source | L: MarkDataDropped |
| 102 | MarkMini | F if present in PK8 source | L: MarkDataDropped |
| 103 | MarkItemfinder | F if present in PK8 source | L: MarkDataDropped |
| 104 | MarkPartner | F if present in PK8 source | L: MarkDataDropped |
| 105 | MarkGourmand | F if present in PK8 source | L: MarkDataDropped |
| 106 | OnceInALifetime | F if present in PK8 source | L: RibbonDataDropped |
| 107 | MarkAlpha | F if present in PK8 source | L: MarkDataDropped |
| 108 | MarkMightiest | F if present in PK8 source | L: MarkDataDropped |
| 109 | MarkTitan | F if present in PK8 source | L: MarkDataDropped |
| 110 | Partner | F if present in PK8 source | L: RibbonDataDropped |
| 111–127 | Unnamed/reserved | RibbonMarkNotRepresentable F | RibbonMarkNotRepresentable F |

Indexes0–52 cover champion, effort/friendship, contest, memory, event and battle ribbons.
53–97 cover time/weather, destiny/rare and personality marks. 98–110 cover later Hisui,
Sinnoh remake and Paldea ribbon/mark semantics. This project intentionally refuses
post-MAX_G8 source bits even though the shared G8PKM model exposes future index accessors.
A named getter in a common model is not proof that Sword/Shield can display that title.

Affixed-title behavior: shared owned indexes survive; unowned/out-of-domain selection
becomes FF. Clearing it now adds `AffixedTitleDropped = 1u << 16`, whether the lost
selection was dangling or its owned ribbon/mark was explicitly removed. Valid carried
ribbons and marks do not produce this loss. The production change is scoped to SWSH↔SV.

## Ball domain

| ID | Name | Audit disposition |
|---:|---|---|
| 0 | None | F: no-ball invalid entity input |
| 1 | Master | P both directions |
| 2 | Ultra | P both directions |
| 3 | Great | P both directions |
| 4 | Poke | P both directions |
| 5 | Safari | P both directions |
| 6 | Net | P both directions |
| 7 | Dive | P both directions |
| 8 | Nest | P both directions |
| 9 | Repeat | P both directions |
| 10 | Timer | P both directions |
| 11 | Luxury | P both directions |
| 12 | Premier | P both directions |
| 13 | Dusk | P both directions |
| 14 | Heal | P both directions |
| 15 | Quick | P both directions |
| 16 | Cherish | P both directions; event-only acquisition |
| 17 | Fast | P both directions |
| 18 | Level | P both directions |
| 19 | Lure | P both directions |
| 20 | Heavy | P both directions |
| 21 | Love | P both directions |
| 22 | Friend | P both directions |
| 23 | Moon | P both directions |
| 24 | Sport | P both directions |
| 25 | Dream | P both directions |
| 26 | Beast | P both directions |
| 27 | Strange | F: later/PLA representation outside conservative shared domain |
| 28 | LAPoke | F: later/PLA representation outside conservative shared domain |
| 29 | LAGreat | F: later/PLA representation outside conservative shared domain |
| 30 | LAUltra | F: later/PLA representation outside conservative shared domain |
| 31 | LAFeather | F: later/PLA representation outside conservative shared domain |
| 32 | LAWing | F: later/PLA representation outside conservative shared domain |
| 33 | LAJet | F: later/PLA representation outside conservative shared domain |
| 34 | LAHeavy | F: later/PLA representation outside conservative shared domain |
| 35 | LALeaden | F: later/PLA representation outside conservative shared domain |
| 36 | LAGigaton | F: later/PLA representation outside conservative shared domain |
| 37 | LAOrigin | F: later/PLA representation outside conservative shared domain |
| 38–255 | Unnamed/reserved at pin | F: BallNotRepresentable |

No SWSH-only accepted ball exists in this common-domain policy. Strange/Hisui balls
are known IDs, not arbitrarily called corrupt bytes; the converter conservatively
refuses them instead of pretending they share SWSH meaning or coercing them to Poké Ball.
The new exhaustive fixture tests all256 values in both directions (512 cases).

## Event, forms, text, history and round-trip results

**Event rule classes:** the existing eight exact-title fixtures combine fateful encounter,
Cherish Ball, event/distribution ribbon bits, shared marks, HOME tracker, custom OT and
nickname, language, moves/relearns, locations, level, fixed PID/EC, dates and origin.
The single-field/ribbon/text suites separate those concerns so unrelated loss flags cannot
hide disappearance. Both shiny and non-shiny modern states use the same threshold; no
new PID/EC is invented. Shiny-lock authenticity and a real event's exact allowed OT,
move set, gender, ribbon combination and PID-generation restrictions belong to future
legality tooling. Synthetic combinations are not asserted to be legal distributed events.

**Forms:** target personal-table availability remains fail closed. Existing representative
corpus covers regional/permanent/gender variants, Rotom, Alcremie, authenticity forms,
Urshifu, item-associated forms and target-absent forms. Transient/fused states are explicitly
refused by `FormNotTransferable` before output creation. This includes fused Kyurem,
Necrozma/Calyrex, Crowned Zacian/Zamazenta, battle/weather forms, and other enumerated
transients in `swshSvFormTransferable`. Held-item/form consistency beyond that list remains
a legality/readiness concern; stored source form is never intentionally flattened to0.
Alcremie decoration3 tests a real FormArgument use in both directions and all four round trips.
Full32-bit relocation is implemented; semantic validity of every species-specific argument
value is not claimed by this representative fixture.

**Text:** exact12-code-unit ASCII and Japanese payloads, a surrogate pair ending at the
boundary, raw13-unit unterminated input, lone high/low surrogates, high-surrogate followed
by non-low, embedded NUL with trailing data, and control/zero-width characters are tested
for nickname/HT/OT in both directions. All26 raw bytes survive accepted conversion,
including data after the first NUL. Malformed pre-terminator UTF-16 / missing terminator
fails preflight and conversion. Control and non-display code points are preserved, not
claimed game-font-valid. A separate display-character policy remains unresolved.

One-past-maximum **model setter** attempts have a distinct boundary: existing void
`setNickname`, `setOTName`, `setHTName` wrappers call `Utils::setString(...,12)`, which
truncates before conversion can see the requested thirteenth unit. The converter cannot
recover or acknowledge discarded API input. This is an OPEN input-contract limitation,
not evidence of safe normalization. A checked caller-visible setter contract and tests
are required before any route promotion; this tranche does not rewrite shared text APIs
used by unrelated formats. Raw13-unit source entities do fail closed now.

**Met/date/version:** dates, OT gender and level preserve. Origin/history is not the
current store or destination title. Entering SV preserves historical origin representation.
Returning SL/VL/PLA/BD/SP-origin payloads to PK8 uses the pinned HOME-like origin/met
mapping; nonzero/non-FFFF egg location becomes65534. Lost location detail is reported.
F13 retains historical origin separately; a subsequent standalone PK8→PK9 conversion
cannot reconstruct discarded location detail without provenance. No false round-trip
identity claim is made.

**Handler:** HT name/gender/language/current-handler, friendship and memories preserve
at their own offsets. No invented handler or ownership switch is performed. Official
trade-handler update logic is deliberately not emulated; a future True-Move readiness
review must decide and validate that policy. HT ID's uncertain use remains in the ledger.

**BattleVersion:** zero stays zero; SW/SH marker entering SV relocates; any nonzero
marker returning from SV is cleared with `BattleVersionDropped`, including later markers
on older-origin Pokémon. That conservative policy does not claim an exact HOME battle-
eligibility simulation. Existing SH/VL fixtures and round-trip SW/SH/SL/VL fixtures cover
both directions and distinguish the marker from historical origin.

**Round trips:** Sword→Scarlet→Sword, Shield→Violet→Shield,
Scarlet→Sword→Scarlet and Violet→Shield→Violet preserve shared event/ribbon/mark/title,
handler, Alcremie decoration, Cherish Ball, maximum ASCII/Japanese text, PID/EC and tracker.
BattleVersion clearing, Tera disappearance/synthesis and HOME-like historical representation
are explicitly explained. Source and intermediate-source encrypted bytes+SHA remain unchanged.
Cached destination stats are recalculated. The inherited current-HP zero→full policy is
not proof of official fainted-state transfer behavior and remains a readiness policy question.

## Findings and fixes at the live starting head

| ID | Severity | Evidence / disposition |
|---|---|---|
| C01 | P2 fixed | Exact48ca Host run36213950230 failed a stale handler/history assertion demanding SL in PK8 after the deliberate remap. Locally reproduced, test now expects SW representation and preserved Report source origin. Native run36213948166 was green but did not prove the host contract. |
| C02 | P1 fixed | Dangling-affix fixture reproduced clearing a title without any title-loss bit. Narrow production fix adds AffixedTitleDropped whenever normalization clears a SWSH↔SV title. Valid-title, removed-title, every-index, preflight and PBCE tests cover it. |
| C03 | P2 fixed | Existing whole-byte labels could imply complete semantics. Explicit unknown region/partial-bit ledger and size-bound full-address partition now separate address coverage from meaning. |
| C05 | P2 fixed | Starting Host checkout log proves it tested synthetic merge836b095, not application48ca138. PR79 checkout now selects the exact PR head and asserts/logs application SHA + tree before running tests; other PR behavior unchanged. |
| C04 | P1 open boundary | Void modern text setters truncate overlength requested input before conversion. Existing-native-source conversion is protected by raw UTF-16 checks; checked input API/display policy remains prerequisite, outside this converter-only production patch. |

No P0 was discovered. Unresolved semantic/policy questions are blockers, not passed
assertions. Existing GCC16 diagnosed a self-vector `push_back(front())` fixture expression;
the test now copies the identical move from the original transaction, avoiding the diagnostic
without weakening the ambiguous-transaction rejection. No product transaction code changed.

## Presentation, PBCE/F13 and custody evidence

`AffixedTitleDropped` has a stable bit, human-readable presentation, catalog/mask completeness,
preflight visibility, persisted PBCE v1 round-trip and acknowledgement binding tests.
All eight exact-route evidence fixtures require explicit acknowledgement and reject a changed
loss mask; the product-disabled gate still prevents retirement even with valid evidence.
Existing LocationDetailDropped/BattleVersionDropped and history adaptation mappings remain.
Unknown enum/mask values fail closed. F13 keeps original origin, exact current source/destination
identity, source payload hash and candidate hash separate.

Every new conversion success/refusal captures native encrypted bytes and SHA-256 before
conversion, compares both afterward, and where applicable repeats the proof after preflight.
The four round trips also protect their intermediate entity. These pure converter tests do
not write stores. Persisted evidence recovery tests prove authoritative source bytes remain
and replacement count is zero when route permission/acknowledgement is absent. A04a/A04b
provide transaction-level no-retirement/no-custody-disappearance coverage.

## Fixture inventory and validation plan

Permanent tests: `test_conversion_entity_golden`, `test_conversion_evidence`,
`test_move_transaction_journal` (A04a), `test_move_transaction_production` (A04b).
The closure fixture groups in the golden executable are:

- `FLAG2`, `DMAXTYPE`, `BALL27`; `AFFIXOK`, `AFFIXBAD`, `AFFIXPK9`, `AFFIXG9`.
- Transient-form refusal; source-only field differential matrix; history remap/egg sentinel cases.
- `exact-pair-{sword,shield}-{scarlet,violet}` and the four reverse labels: eight event routes.
- Whole PK8/PK9 address partition plus partial-byte known/unknown masks.
- `RIBBONBITS` (multiple/counts), `G9RIBMARK`, `RIBRESERVE`.
- `BALLDOMAIN`, `PK9BALL`, `NOBALL`, `BADPK8BALL`; new `closure-ball-domain` all256×2.
- Handler/Alcremie history and BattleVersion; PP-difference, language and tracker families.
- Maximum text, unterminated and lone-surrogate fixtures; new `closure-text-fields` nine cases×three fields×two directions.
- New `closure-ribbon-domain` all128 indexes×two directions, including affix and preflight.
- Four requested round-trip cases extended with event/handler/title/form-argument/text state.

Focused goldens/evidence/A04a/A04b pass locally before candidate publication. Final verification
requires exact-head Host Tests (full host, focused RSE plus sanitizer, ASan and UBSan), and
Audit Hardening Native Validation (full devkitA64 compile/link, AArch64 inspection, recovered
RomFS asset preflight). GitHub run IDs and actual conclusions go in PR/issue metadata only
after completion; no old-SHA success is substituted. No downloadable application acceptance
or device acceptance is claimed by this tranche.

## Exact route decisions and remaining gates

| Direction | Decision | Exact outstanding blocker set |
|---|---|---|
| Sword→Scarlet | ROUTE MUST REMAIN DISABLED | PK8 unknown bits/retired regions; input/display text contract; handler/HP/form/event policy |
| Sword→Violet | ROUTE MUST REMAIN DISABLED | Same PK8 ledger, independently checked exact-title evidence |
| Shield→Scarlet | ROUTE MUST REMAIN DISABLED | Same PK8 ledger, independently checked exact-title evidence |
| Shield→Violet | ROUTE MUST REMAIN DISABLED | Same PK8 ledger, independently checked exact-title evidence |
| Scarlet→Sword | ROUTE MUST REMAIN DISABLED | PK9 unknown bits/tail; input/display text contract; handler/HP/form/event policy |
| Scarlet→Shield | ROUTE MUST REMAIN DISABLED | Same PK9 ledger, independently checked exact-title evidence |
| Violet→Sword | ROUTE MUST REMAIN DISABLED | Same PK9 ledger, independently checked exact-title evidence |
| Violet→Shield | ROUTE MUST REMAIN DISABLED | Same PK9 ledger, independently checked exact-title evidence |

**Potential route candidates: none.** Non-conversion gates remain exact title/account/store
qualification, staged workspace adapters, explicit loss acknowledgement and provenance recovery,
physical Switch FAT32/exFAT interruption/power-loss testing, and explicit route-enablement review.
A01–A09 are preserved infrastructure, not proof of those physical gates.

Recommended next tranche: resolve the enumerated PK8/PK9 unknown-value and checked-text-input
contracts with authoritative/legitimate fixtures, plus decide handler/HP and per-form legality
policy. Do not start another broad audit or another generation pair. Only once those blockers
are closed should a separate SWSH/SV True-Move enablement readiness review be proposed.
No merge, route activation or live-source write belongs to this tranche.

## Full-address partition (regression data)

Each range is counted exactly once; sizes are bound to production encryption constants.

### PK8

| Range | Bytes | Classification |
|---|---:|---|
| 000–016 | 23 | header/identity/ability |
| 017–017 | 1 | alignment |
| 018–019 | 2 | markings |
| 01A–01B | 2 | alignment |
| 01C–022 | 7 | PID/nature/fateful/Flag2/gender |
| 023–023 | 1 | alignment |
| 024–032 | 15 | form/EV/contest/Pokerus |
| 033–033 | 1 | padding |
| 034–03D | 10 | ribbons/memory counts |
| 03E–03F | 2 | padding |
| 040–047 | 8 | ribbons/marks |
| 048–04B | 4 | Sociability |
| 04C–04F | 4 | alignment |
| 050–051 | 2 | height/weight |
| 052–057 | 6 | alignment |
| 058–08F | 56 | nickname/moves/current HP/IV |
| 090–090 | 1 | DynamaxLevel |
| 091–093 | 3 | alignment |
| 094–09B | 8 | status/Palma |
| 09C–0A7 | 12 | alignment |
| 0A8–0C4 | 29 | HT name/gender/language/current handler |
| 0C5–0C5 | 1 | alignment |
| 0C6–0DD | 24 | HT id/memory/PokeJob/fullness/enjoyment |
| 0DE–0DF | 2 | origin/BattleVersion |
| 0E0–0E1 | 2 | UNKNOWN: retired region history |
| 0E2–0E2 | 1 | language |
| 0E3–0E3 | 1 | alignment |
| 0E4–0E8 | 5 | FormArgument/AffixedRibbon |
| 0E9–0F7 | 15 | padding |
| 0F8–114 | 29 | OT name/friendship/memory |
| 115–115 | 1 | alignment |
| 116–11E | 9 | OT memory/dates |
| 11F–11F | 1 | alignment |
| 120–126 | 7 | locations/ball/met/HyperTraining |
| 127–134 | 14 | TR records |
| 135–13C | 8 | HOME tracker |
| 13D–147 | 11 | alignment |
| 148–148 | 1 | party level |
| 149–149 | 1 | alignment |
| 14A–155 | 12 | party battle stats |
| 156–157 | 2 | DynamaxType |

### PK9

| Range | Bytes | Classification |
|---|---:|---|
| 000–016 | 23 | header/identity/ability |
| 017–017 | 1 | alignment |
| 018–019 | 2 | markings |
| 01A–01B | 2 | alignment |
| 01C–022 | 7 | PID/nature/fateful/gender |
| 023–023 | 1 | alignment |
| 024–032 | 15 | form/EV/contest/Pokerus |
| 033–033 | 1 | padding |
| 034–03D | 10 | ribbons/memory counts |
| 03E–03F | 2 | padding |
| 040–047 | 8 | ribbons/marks |
| 048–04A | 3 | height/weight/Scale |
| 04B–057 | 13 | DLC TM records |
| 058–08F | 56 | nickname/moves/current HP/IV |
| 090–095 | 6 | status/Tera |
| 096–0A7 | 18 | padding |
| 0A8–0C4 | 29 | HT name/gender/language/current handler |
| 0C5–0C5 | 1 | alignment |
| 0C6–0D5 | 16 | HT id/memory/origin/BattleVersion/FormArgument/AffixedRibbon/language |
| 0D6–0F7 | 34 | UNKNOWN: PK9 remainder unused; real-value evidence absent |
| 0F8–114 | 29 | OT name/friendship/memory |
| 115–115 | 1 | alignment |
| 116–11E | 9 | OT memory/dates |
| 11F–11F | 1 | ObedienceLevel |
| 120–126 | 7 | locations/ball/met/HyperTraining |
| 127–12E | 8 | HOME tracker |
| 12F–147 | 25 | TM records |
| 148–148 | 1 | party level |
| 149–149 | 1 | alignment |
| 14A–155 | 12 | party battle stats |
| 156–157 | 2 | UNKNOWN: PK9 unmodeled party tail |


# PK8/PK9 checked-input / handler / HP policy closure — 2026-09-26

## Audit-forward identity

This tranche audited forward from application SHA `1912cf01f211c809b17a98326652c778bd389dd4`,
tree `34a411b7a9ecc38c423779228435c6e6acc5c9fb`. The preceding exact-head Host run
`36216422831` / #1230 completed SUCCESS and Native Validation `36216420184` / #126
remained the prior native checkpoint. PR #79 stayed OPEN / DRAFT / NOT MERGED and PR #77
was not modified. Main remained `9ec20730e23681c6f7451f3da4944a156303f494`.

## Unknown-region disposition

No new authoritative legitimate nonzero fixture was found in-repository that proves the
remaining partial-byte/retired-tail semantics. Therefore this tranche does **not** promote
an `unused` comment, checksum participation, or synthetic zero to semantic proof.

The following remain **STILL UNKNOWN** and continue to block route promotion:

- PK8: 0x016 mask 0xE0; 0x019 mask 0xF0; 0x022 mask 0xF0; 0x025 mask 0xFF;
  0x0E0-0x0E1; 0x126 mask 0xC0.
- PK9: 0x016 mask 0xF0; 0x019 mask 0xF0; 0x022 mask 0xF8; 0x025 mask 0xFF;
  0x126 mask 0xC0; party tail 0x156-0x157.
- Shared HT ID 0x0C6-0x0C7 remains STILL UNKNOWN semantically, even though it is copied.
- PK8 Flag2 and Palma remain named-but-incompletely-explained semantics. Their nonzero
  SWSH->SV disappearance remains declared through existing `DivergentGameDataDropped`.
- PK9 0x0D6-0x0F7 remains STILL UNKNOWN for route-readiness purposes. The pinned model's
  "remainder unused" description is useful evidence, but this audit deliberately requires
  stronger proof before treating arbitrary nonzero bytes as canonical zero.

No unknown region is silently normalized as a readiness claim. All eight routes remain disabled.

## Checked modern text contract

PK8 and PK9 Nickname, Handling Trainer name and Original Trainer name now expose checked
setters returning exactly:

- `Accepted`
- `Overlength`
- `MalformedUtf16`

The storage contract is **12 UTF-16 code units plus one U+0000 terminator** in each 26-byte
field. This is a code-unit limit, not a Unicode-code-point or display-character limit.

The compatibility void setters now call the checked path. They no longer silently truncate:
invalid input leaves the entity unchanged. A caller that needs the reason must use the checked
setter.

The boundary corpus covers empty, 1 unit, 11 units, 12 units, one-past-maximum, Japanese at
12 units, a surrogate pair ending exactly at unit 12, a pair crossing the boundary, lone high
and low surrogates, and embedded NUL. Embedded NUL inside a requested logical string is rejected
as non-canonical storage text because accepting it would silently discard the suffix after the
field terminator.

Accepted cases are set, checksummed, serialized/encrypted, reparsed and compared exactly.
Rejected cases prove native bytes and SHA-256 are unchanged with no partial field write.

## Handler / ownership policy

The audited contract is:

`conversion = representation conversion only`

Destination owner / handler updates are a separate destination-placement policy. Conversion
continues to preserve the stored CurrentHandler, HT name/gender/language/friendship/memory
fields and the shared 0x0C6-0x0C7 bytes without inventing a new handler. F13 profile/account
ownership remains provenance/store metadata and is not written into PKM trainer fields.

This separation is intentional. A future destination-write readiness review may implement an
explicit owner/handler update, but it must not be hidden inside representation conversion.

## Current HP / status / party-state policy

For PK8/PK9 cross-generation conversion:

- Max HP, ATK, DEF, SPE, SpA, SpD and party Level are destination-derived cached party stats
  and are recalculated from destination personal data.
- Status condition is persistent representational state for this converter and is relocated
  between the PK8 and PK9 offsets rather than dropped.
- PK8 DynamaxType is source-generation party-only state with no PK9 counterpart; nonzero loss
  remains reported through `DivergentGameDataDropped`.
- PK9 0x156-0x157 remains STILL UNKNOWN and therefore blocks route-readiness promotion.
- Current HP at 0x08A is not part of the recalculated party-stat tail. Existing production
  behavior keeps a nonzero carried value, but refills a zero value to destination max HP after
  a fresh remap. That zero->full rule is retained as an **unresolved readiness policy** rather
  than being claimed as official HOME behavior.

Because fainted/0-HP semantics are not authoritatively closed, HP policy is not considered
fully route-ready in this tranche.

## Shared-form policy result

The converter continues to fail closed for the explicit fused/transient set already encoded
in `swshSvFormTransferable()`, including Kyurem/Necrozma/Calyrex fusions and the enumerated
battle-only states (Zen, Ash-Greninja, Complete Zygarde, Minior shields, Busted Mimikyu,
weather/battle forms, Crowned forms, Eternamax, Hero Palafin, Ogerpon/Terapagos battle states).

Target personal-table absence still returns `NotInDex`; unsupported forms are never flattened
to form 0 and are never silently unfused.

However, the repository does not yet contain a generated/exhaustive authoritative table proving
**every** SWSH/SV shared species/form pair. The hand-written deny-list is therefore still a
readiness blocker. This tranche records that limitation instead of relabeling partial evidence
as a complete shared-form policy.

## Event-policy boundary

Representation conversion preserves or reports representable stored event metadata. The existing
fixtures prove representation behavior for fateful encounter, Cherish Ball, PID/EC, HOME tracker,
event ribbons/marks, moves/relearn moves, OT/name/language, dates and met/history fields.

Those fixtures do **not** certify that an arbitrary synthetic combination corresponds to a real
historical distribution. Distribution-authenticity belongs to future legality tooling unless a
target-format structural requirement is needed for the entity itself to be valid. No full legality
checker was added here.

## Fidelity / provenance / acknowledgement impact

No new `Loss`, `Adaptation`, or conversion `Result` enum was introduced by the checked-text
fix, so the existing presentation catalog, known masks, PBCE mapping and acknowledgement schema
remain unchanged. Existing stale-evidence invalidation remains bound to source payload,
destination candidate, Loss/Adaptation masks, route, game identity, store descriptor and profile.

F13's historical-origin separation is unchanged, including the S/V historical origin represented
inside PK8 through the HOME-like Sword/Shield representation remap.

## Exact route decisions

All eight exact directions remain **ROUTE MUST REMAIN DISABLED**:

- Sword -> Scarlet
- Sword -> Violet
- Shield -> Scarlet
- Shield -> Violet
- Scarlet -> Sword
- Scarlet -> Shield
- Violet -> Sword
- Violet -> Shield

No direction qualifies as a POTENTIAL ROUTE CANDIDATE because unknown semantic regions, the
zero-current-HP policy, and exhaustive shared-form proof remain open converter/readiness blockers.
`routeEnabledForTrueMove()` remains false and source retirement remains gate-protected.

Final exact-head Host and Native run IDs/conclusions are recorded in PR #79 / issue #69 metadata
after the final code/document head finishes CI; they are intentionally not baked into another
self-invalidating docs-only commit.
