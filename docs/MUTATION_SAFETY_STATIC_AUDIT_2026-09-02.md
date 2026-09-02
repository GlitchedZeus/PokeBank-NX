# PokeBank NX — Initial Mutation Safety Static Audit — 2026-09-02

Tracker: **#23 — Audit and lock inherited live-save mutation UI before second device build**.

This document records source-level findings gathered after the extended physical test of the first exact `.nro`.

It is an **initial static audit**, not the completion record for #23. The next coding session must still systematically inspect all reachable call sites, implement the required UI blocks, add regression tests, rebuild, and physically retest the exact replacement artifact.

---

# Evidence boundary

Physical test proved:

```text
Release UI reachable
Create Pokémon UI reachable
Move/Multi UI reachable
writable Edit UI reachable
legacy Storage persistence observed
```

Physical test did **not** externally reopen the original game afterward, so it did not prove an installed live-title save write.

Current source inspection now substantially narrows the persistence model:

```text
installed live-title restore path      HARD DISABLED by policy
normal game Save Changes path          BACKUP/STAGED SAVE WRITE
Release/Create/Move/Edit in save pane  IN-MEMORY mutation, then backup-persistable
legacy Bank/Storage                     APP-OWNED STORAGE WRITE
```

Do not rewrite the historical physical report as though these source findings had been physically proven on the old artifact.

---

# 1. Global live-write policy

File:

```text
include/Safety/WritePolicy.h
```

Current policy:

```cpp
LIVE_SAVE_WRITES_ENABLED = false
```

Visible save destinations are only:

```text
WorkingBackup
NewBackup
```

`LiveGame` exists as an enum value but is excluded from the visible destination array.

`canWriteTo(LiveGame)` is false while the current constant remains false.

`allowLiveInjectionRequest(requested)` also resolves false while the current constant remains false.

Classification:

```text
LIVE INSTALLED SAVE WRITE — DISABLED BY POLICY
```

This is intentionally a source-code invariant, not a settings toggle.

---

# 2. Save dialog / normal game-save persistence

Files:

```text
include/UI/TrainerViewScreen.h
src/UI/TrainerViewScreen.cpp
src/Save/GetSaveFileContents.cpp
```

The TrainerView save picker exposes only the policy's backup-only destination list.

Before saving, the selected destination is passed through:

```cpp
PokeVault::Safety::canWriteTo(chosenDest)
```

The normal UI save operation calls:

```cpp
Save::saveTrainerInfo(trainer, destDir.c_str(), titleId, userUid)
```

and logs the destination as a backup.

The generic `Save::saveTrainerInfo(...)` dispatches to the supported concrete save writers with the injection argument explicitly set to `false`.

The concrete save writers therefore serialize edited data into files under the chosen backup directory. They contain older/low-level `injectToTitle` branches, but the normal generic UI flow does not request them.

Classification for normal `Save Changes` from the game-session UI:

```text
BACKUP/STAGED SAVE WRITE
```

This is why an IV/edit can survive reopening the edited backup/session without proving that the installed title was modified.

---

# 3. Lowest-level installed-title restore

File:

```text
src/Utils/FileUtilities.cpp
```

`restoreBackupToTitle(...)` begins by checking:

```cpp
PokeVault::Safety::canWriteTo(SaveDestination::LiveGame)
```

When policy rejects it, the function returns `false` **before** calling `fsdevMountSaveData("save", ...)`.

Only after that policy gate would the function:

```text
mount save:/
copy the named save files
fsdevCommitDevice("save")
unmount
```

Classification under current source policy:

```text
LIVE INSTALLED SAVE WRITE — DISABLED / UNREACHABLE THROUGH THIS LOW-LEVEL RESTORE
```

This low-level hard lock must remain intact even after the inherited mutation UI is hidden/blocked. UI restrictions are defense-in-depth, not a replacement for this gate.

---

# 4. Release from installed-save pane

File:

```text
src/UI/TrainerViewScreen.cpp
```

Current UI still advertises and exposes Release for save-box Pokémon.

On confirmation the target slot is reset and:

```cpp
hasUnsavedChanges = true;
```

That modifies the currently loaded Trainer/save representation in memory. The normal Save Changes path can then serialize that state into a backup.

Current classification:

```text
IMMEDIATE ACTION: IN-MEMORY ONLY
PERSISTENCE IF USER SAVES: BACKUP/STAGED SAVE WRITE
LIVE INSTALLED SAVE WRITE: BLOCKED BY LOWER LAYERS
```

Required Session 2.6 behavior:

```text
BLOCK Release for installed-game source panes while live writes are disabled.
```

The user should not be offered an operation that looks like a destructive live game action when the product contract says installed sources are read-only.

---

# 5. Create Pokémon in installed-save pane

File:

```text
src/UI/TrainerViewScreen.cpp
```

The current empty-slot creator constructs a new Pokémon and assigns it directly into:

```cpp
storageSlot(creator.pane, creator.box, creator.slot)
```

then sets `hasUnsavedChanges = true` and opens the writable editor.

When `creator.pane == save pane`, this is mutation of the backup-loaded game representation.

Classification for save pane:

```text
IMMEDIATE ACTION: IN-MEMORY ONLY
PERSISTENCE IF USER SAVES: BACKUP/STAGED SAVE WRITE
LIVE INSTALLED SAVE WRITE: BLOCKED BY LOWER LAYERS
```

Classification for legacy Bank pane:

```text
APP-OWNED STORAGE MUTATION
PERSISTENCE IF BANK SAVED: APP-OWNED STORAGE WRITE
```

Required Session 2.6 behavior:

```text
BLOCK Create Pokémon on installed-game source pane.
```

Creating directly in app-owned Storage may remain technically separate, but it must be clearly distinguished from future Master Vault behavior and should not expand Session 2.6 scope.

---

# 6. Move / Multi / swap in installed-save pane

File:

```text
src/UI/TrainerViewScreen.cpp
```

The current UI supports grab/move/multi-selection and direct save-box swapping. Source operations remove/move `unique_ptr<Pokemon>` objects or call `trainer.swapBoxSlots(...)`, then mark `hasUnsavedChanges = true`.

Classification for save-pane-only movement:

```text
IMMEDIATE ACTION: IN-MEMORY ONLY
PERSISTENCE IF USER SAVES: BACKUP/STAGED SAVE WRITE
LIVE INSTALLED SAVE WRITE: BLOCKED BY LOWER LAYERS
```

Classification for bank-pane movement:

```text
APP-OWNED STORAGE MUTATION
PERSISTENCE IF BANK SAVED: APP-OWNED STORAGE WRITE
```

Cross-pane operations are especially important to classify carefully because current legacy Storage semantics can make a source removal look like a true Move even though installed live writing is disabled.

Required Session 2.6 behavior:

```text
BLOCK unsafe source-removing Move/Multi operations on installed-game source panes.
Do not let the UI imply true Move semantics.
```

A future safe Copy-to-Vault/Bank or true Move workflow belongs to #3/#20, not this session.

---

# 7. Action Sheet Edit / writable details editor

Files:

```text
include/UI/ActionSheetModel.h
src/UI/TrainerViewScreen.cpp
```

The shared Action Sheet currently classifies `Edit` as implemented and dispatches it as:

```text
ActionResult::OpenEditor
```

TrainerView responds by opening the target details page with writable mode (`readOnly=false`).

The details/editor code performs real in-memory mutations including, among others:

```text
IV / EV / AV
shiny state
nature
nickname
EXP
met/egg data
Pokérus
other editable entity fields
```

and sets `hasUnsavedChanges = true`.

This source behavior matches the physical observation that an IV could be changed, applied/reopened, and then restored.

Classification for Party/save-box targets:

```text
IMMEDIATE ACTION: IN-MEMORY ONLY
PERSISTENCE IF USER SAVES: BACKUP/STAGED SAVE WRITE
LIVE INSTALLED SAVE WRITE: BLOCKED BY LOWER LAYERS
```

Classification for Bank target:

```text
APP-OWNED STORAGE MUTATION
PERSISTENCE IF BANK SAVED: APP-OWNED STORAGE WRITE
```

Required Session 2.6 behavior:

```text
For installed-game source targets, Edit must not open a writable source editor.
```

Until a safe copy-edit flow exists, preferred alpha behavior is a clear read-only / `Not yet supported` result for installed sources.

`View Pokémon` remains read-only.

---

# 8. Legacy Storage / Bank persistence

File:

```text
src/Trainer/Bank.cpp
```

Current Storage is a real app-owned persistent bank, not merely temporary UI state.

Current path:

```text
BASE_SAVE_DIRECTORY/bank/bank.dat
```

Header magic:

```text
PKSEBANK
```

The bank serializes its own fixed records and writes them to `bank.dat` using ordinary filesystem I/O. Storage exit can call `bank->save()`, while discard reloads from `bank.dat` using `bank->load()`.

This explains why the physically moved Arbok could remain visible in Storage later without establishing a live installed-game save write.

Classification:

```text
APP-OWNED STORAGE WRITE
```

This is **legacy PKSE bank storage**, not the future PokeBank NX Master Vault.

Issue #27 remains responsible for eventual classification/migration/UX cleanup.

Additional later-hardening note: current legacy `Bank::save()` writes `bank.dat` directly; Master Vault v1 must use its own stronger transaction/atomicity model rather than inheriting this format as the authoritative Vault database.

---

# Current classification matrix

| User path | Immediate mutation target | Persistent target if committed | Current classification | Session 2.6 action |
| --- | --- | --- | --- | --- |
| Browse / View | none | none | IN-MEMORY READ ONLY | preserve |
| Release — save pane | loaded Trainer/save model | chosen backup | IN-MEMORY → BACKUP/STAGED | block on installed source |
| Create — save pane | loaded Trainer/save model | chosen backup | IN-MEMORY → BACKUP/STAGED | block on installed source |
| Move/Multi — save pane | loaded Trainer/save model | chosen backup | IN-MEMORY → BACKUP/STAGED | block unsafe source mutation |
| Edit — Party/save box | loaded Pokémon/Trainer model | chosen backup | IN-MEMORY → BACKUP/STAGED | writable source editor unavailable |
| Save Changes | current Trainer model | Working/New backup | BACKUP/STAGED SAVE WRITE | keep backup-only |
| Release/Create/Move/Edit — Bank pane | legacy Bank model | `bank/bank.dat` | APP-OWNED STORAGE WRITE | preserve only where clearly app-owned/safe |
| `bank->save()` | legacy Bank serialization | `bank/bank.dat` | APP-OWNED STORAGE WRITE | classify/label; #27 later |
| `restoreBackupToTitle(...)` | none while policy off | installed `save:/` | DISABLED / UNREACHABLE | preserve hard lock |

---

# Still required before closing #23

This initial audit does **not** close #23.

Session 2.6 must still:

1. systematically search all callers of concrete save functions and all `injectToTitle` / restore / commit paths;
2. search for any other direct `fsdevMountSaveData` path capable of writing rather than backing up;
3. block Release/Create/unsafe Move/Multi on installed-game source panes;
4. block writable Edit on installed-game Party/save-box sources, unless explicitly converted into a safe app-owned copy workflow;
5. keep View read-only;
6. preserve backup-only Save Changes behavior;
7. preserve the low-level policy hard lock;
8. add host regression tests for installed-source UI capabilities/action dispatch;
9. run host tests + ASan/UBSan + `git diff --check` + native build;
10. produce a new exact application-source commit and exact replacement `.nro`;
11. physically retest that exact artifact.

Until those items are complete, issue #23 remains a second-device-build blocker.

---

# Current source-grounded conclusion

The source inspected so far supports this statement:

> The inherited mutation UI is real and changes the backup-loaded in-memory save representation, and those changes can be written into backup files. The normal UI does not currently expose a live-game destination, generic saves request no injection, and the lowest-level installed-title restore is blocked by policy before the game save is mounted. Legacy Storage is a separate persistent app-owned `PKSEBANK` file.

That is much stronger than the earlier `UNKNOWN`, but it is deliberately **not** a claim that every possible internal call path has already been exhausted or that the second-device safety work is complete.
