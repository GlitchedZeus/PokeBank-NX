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
