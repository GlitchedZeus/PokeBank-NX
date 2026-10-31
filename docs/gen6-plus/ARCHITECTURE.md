# Gen VI-IX shared application layer: architecture

Branch `development/gen6-plus`. Audience: a maintainer adding or changing a title family. Every
statement here is about code on this branch, cited by path; the per-title status table is
`docs/gen6-plus/CAPABILITY_MATRIX.md`, generated from the registry (see "Generated matrix").
Nothing in this document is a hardware claim: every Gen VI-IX title is NOT DEVICE TESTED.

Rules that apply before anything else: `docs/ENGINEERING_PROTOCOL.md`.

## 1. The cumulative-inheritance rule

A shared application workflow that any earlier generation makes visible (Supported or ReadOnly)
must exist, in some honest form, for every later title:

- `Supported`: implemented through the shared workflow and covered by host tests;
- `ReadOnly`: visible through the shared workflow, editing not verified;
- `Blocked` with a named prerequisite (`reason` must be non-empty);
- `NotApplicable` with the native reason the game lacks the data (`reason` non-empty).

`NotImplemented` is never acceptable for an inherited workflow. A later generation may therefore
never silently drop Party, Boxes, the editor, Bag, Review, Export or any other workflow Gen I-V
already has; it must either serve it through the shared code or say why it cannot yet.

### How it is enforced

1. Compile time. `include/Integration/Common/GameCapabilityRegistry.h` defines
   `inheritedWorkflow(Workflow, generation)` (does any earlier-generation row make the workflow
   visible?) and `firstInheritanceViolation()`, which walks all 38 rows and returns the first row
   whose inherited workflow is `NotImplemented` or is `Blocked`/`NotApplicable` without a reason.
   The header ends with
   `static_assert(!firstInheritanceViolation().has_value(), ...)`, so any translation unit that
   includes the registry refuses to compile when a row breaks the rule. The registry is
   `constexpr`, so the check costs nothing at run time.
2. Host test. `tests/test_game_capability_registry.cpp` (built by `Makefile.host.gen6-plus`,
   in `HOST_TESTS` and `HOST_SANITIZE_TESTS`) additionally checks: exactly 38 rows; unique ids;
   every id is a `Games::GameDescriptor` whose `dataGeneration` and `title` agree
   (`src/Games/GameIdentity.cpp`); every `Blocked`/`NotApplicable` status names a reason; every
   Gen I-V row has every workflow visible; no row anywhere is `NotImplemented`; the Gen VI rows are
   `modern-native` with Export to `exports/gen6` and Create blocked; the SM/USUM rows have Export
   blocked by the SAV7 reason; mechanics belong only to the families that have them (Z-Moves to
   SM/USUM, Dynamax to SWSH, Tera to SV, Awakening Values to LGPE, Effort Levels to PLA, Plus flags
   to Z-A, Fairy from Gen VI); Z-A is `PA9`, not an SV alias; the Switch-reader titles have the
   editor and Export blocked with a prerequisite and no export root.
3. Generated document. `tests/test_capability_matrix_doc.py` fails `host-test` when
   `docs/gen6-plus/CAPABILITY_MATRIX.md` is not byte-identical to what
   `tools/capability_matrix.cpp` prints from the same registry.

## 2. Layers

```
GameCapabilityRegistry.h          constexpr truth: 38 rows x 16 workflows, mechanics, geometry
        |
NativeSession adapters            one per family: Gen6NativeSession, Gen7NativeSession, ...
        |
ModernReadOnlyTrainer             the Trainer bridge the shared screens consume
        |
ModernSharedSurface.inc           the ONE Trainer/Party/Boxes/Items/Trainer screen (+ ModernScreenState)
        |
ModernSources                     Save Instances: discovery, validation, assignment, open, Home card
        |
VerifiedStagedExport              separate verified Export Copy; never the source
```

### 2.1 `include/Integration/Common/GameCapabilityRegistry.h`

- `Registry` is a `constexpr std::array<GameCapabilities, 38>`: Gen I-V baseline (20 titles,
  provider `classic`, `HardwareState::OwnerTestedEarlierCandidate`, `UiIntegration::ClassicSharedUi`),
  Gen VI and SM/USUM (provider `modern-native`, `UiIntegration::ModernSurfaceHostTested`), LGPE /
  SWSH / BDSP / PLA / SV / Z-A (provider `legacy-modern-reader`, `UiIntegration::LegacyReaderOnly`).
- Each row carries `id` (must equal the `GameIdentity` id), `generation`, `platform`, `family`,
  `recordFormat`, `statModel`, `storage` geometry, `NativeMechanics` flags, 16 `WorkflowStatus`
  entries, `hardware`, `exportRoot` (empty when export is blocked), `provider`, `ui` (how far the
  application path is wired), `uiEvidence` (the real-screen host test that reaches it) and
  `hardwareEvidence` (empty unless the owner accepted an exact NRO).
- Lookups: `all()`, `find(id)`, `isModernNativeId(id)`, `row.status(w)`, `row.supports(w)`,
  `row.visible(w)`. Display names: `workflowName(w)`, `supportName(s)`.
- Only this table may advertise a capability. The shared surface, Save Instances and the
  cross-generation matrix read it; adapters repeat its reasons rather than inventing their own.

### 2.2 NativeSession adapters (`include/Integration/Modern/NativeSession.h`)

`Modern::NativeSession` is the one backend interface for Gen VI-IX. Everything above it is written
once against this contract; a family contributes an adapter and a registry row, nothing else.

Contract groups (read the header for the exact signatures):

- identity: `exactGameId()`, `capabilities()` (its registry row), `familyLabel()`, box/party
  geometry, `boxName()`, `sourceBytes()` (immutable parsed copy);
- records: `view(SlotRef)` returns a generation-neutral `RecordView` (species, form, PID/EC,
  IV/EV, moves, met data, ... plus `nativeFacts` label/value pairs for title-specific mechanics);
  `originalOccupied`, `emptyDestination`, `stagedAt`, `firstEmptyBoxSlot`;
- shared editor field model: `fieldState(Field, Region)` (Hidden / Derived / ReadOnly / Editable
  from `include/UI/ExactFormatEditorProvider.h`), `stageValue`, `stageText`, `fieldMaximum`,
  `heldItemChoices`, `ballChoices`, `maxPP`;
- draft bracket for the shared View/Edit workspace: `beginDraft`, `draftActive`, `draftSlot`,
  `draftDirty`, `keepDraft`, `discardDraft` (the Gen I-V "Keep / Discard draft" semantics);
- slot operations: `cloneSupport` / `moveSupport` / `removeSupport` / `createSupport` answer with
  an `OperationSupport{available, reason}`; `clone`, `move`, `remove`, `undo` act in app memory;
- bag: `bagAvailable`, `bag()` pockets with `editable` and `reason`, `bagAdd`, `bagSetQuantity`,
  `bagRemove`, `discardBag`;
- trainer / pokedex facts (read-only): `trainerFacts`, `trainerName`, `trainerGender`,
  `trainerId`, `secretId`, `pokedex()`;
- staging lifecycle: `hasChanges`, `changeCount`, `reviewRows`, `discardAll`;
- export: `exportSupport()` and `exportWorkingCopy(ExportRequest)` returning
  `Common::VerifiedExportResult`.

Adapters on this branch:

| Family | Adapter | Over | Test |
| --- | --- | --- | --- |
| X / Y / Omega Ruby / Alpha Sapphire | `include/Integration/Gen6/Gen6NativeSession.h`, `src/Integration/Gen6/Gen6NativeSession.cpp` | `Gen6StagedSession` (strict SAV6 parse, app-memory staging, byte-whitelist verifier) | `tests/test_gen6_native_session.cpp` |
| Sun / Moon / Ultra Sun / Ultra Moon | `include/Integration/Gen7/Gen7NativeSession.h`, `src/Integration/Gen7/Gen7NativeSession.cpp` | `Gen7StagedSession` (signature-verified SAV7 parse, unsigned candidate) | `tests/test_gen7_native_session.cpp` |

Both expose a static `open(bytes, assignedExactGame)` returning `OpenedSession{session,
diagnostic}`. The assigned exact title is authoritative: a MyStatus version byte that disagrees is
refused with a diagnostic, never silently retargeted. `createSupport()` repeats
`Gen6StagedSession::createCapability().reason` (Gen VI) and `exportSupport()` on Gen VII repeats
the registry's Export reason; neither adapter invents a reason of its own.

Note: the comment at the end of `NativeSession.h` mentions an `Integration/Modern/ModernFamilies.h`
registration header. That file does not exist on this branch; family dispatch lives in
`ModernSources::openSession` (section 2.5).

### 2.3 `include/Legacy/ModernReadOnlyTrainer.h`, `src/Legacy/ModernReadOnlyTrainer.cpp`

The one `Trainer::Trainer` bridge for every Gen VI-IX title. `create(session, exactGameId, group,
error)` wraps a `NativeSession`; `session()` exposes it to the surface; `refreshPresentation(error)`
rebuilds party, boxes, items and trainer facts from the adapter's staged view and refuses (keeping
the previous presentation) when any declared party slot cannot be shown as a verified record.
Every inherited `update*Block()` is a no-op, `createBlankPokemon()` returns `nullptr`,
`supportsBoxNames()` is false. There is no code path from this class to a source writer.
Test: `tests/test_modern_trainer_bridge.cpp`.

### 2.4 The one shared surface (contract, not internals)

`src/UI/ModernSharedSurface.inc` and `include/Integration/Modern/ModernScreenState.h` are being
written by another worker in parallel; this section describes what they must hold to, not their
internals. The reference it mirrors is the Gen V adapter of the shared Gen I-V surface:
`src/UI/Gen5SharedReadOnlySurface.inc`, `src/UI/SharedPokemonEditorSurface.inc`,
`include/Integration/Gen5/Gen5SharedScreenState.h`, hooked in through
`src/UI/TrainerViewScreenCompositeOverlay.cpp` and `include/UI/TrainerViewScreenBase.h`.

Contract:

- It consumes `ModernReadOnlyTrainer::session()` and nothing family-specific. The same compiled
  surface must run Gen VI and Gen VII (and every later adapter) with no `if (generation == ...)`.
- Every available action is derived from two sources only: the adapter's own support answers
  (`cloneSupport`, `moveSupport`, `removeSupport`, `createSupport`, `exportSupport`,
  `BagPocketView::editable`) and the registry row the adapter carries (`capabilities()`). A
  workflow the registry marks `Blocked` is shown with its reason, never offered.
- The Pokemon editor is the shared `SharedPokemonEditorSurface.inc` template parameterised by an
  Editor policy; field visibility and editability come from `fieldState()`, mechanics rows from
  `RecordView::nativeFacts` and the row's `NativeMechanics`.
- Draft semantics are the Gen I-V ones: View never begins a draft; Edit begins the adapter's
  draft bracket; B on a dirty draft asks Keep / Discard / Continue; Keep is refused (draft kept)
  when `refreshPresentation` rejects the staged view; Discard restores the snapshot through
  `discardDraft()`.
- Destructive actions are confirmed as the protocol says (Y-confirmation; Bag Remove is the
  owner-directed exception).
- Review shows `reviewRows()`; Export is offered only when `exportSupport().available` and runs
  `exportWorkingCopy` into the app-owned root from `ModernSources::exportsRootFor`.
- It owns no bytes and never calls anything that could write a source save.
- The state machine is host-testable without `switch.h` (`tests/test_modern_screen_state.cpp`);
  the drawn surface is driven by the host-UI harness (`tests/test_ui_modern_surface_host.cpp`,
  GCC only, like `Makefile.host.host-ui`). Build fragment: `Makefile.host.modern-ui`.

### 2.5 Save Instances: `include/Integration/Modern/ModernSources.h`, `src/Integration/Modern/ModernSources.cpp`

Mirrors the accepted Gen IV/V model (`Gen5GameSourceCatalog` / `Gen5AssignedSource`):

- family dispatch from the registry: `isModernAssignedId`, `expectedFamily`, `gameGroupFor`,
  `platformLabelFor`, `generationLabelFor`, `exportsRootFor` (`Paths::root()/<exportRoot>`, empty
  when the registry blocks export) and `openSession(gameId, bytes)`, which routes generation 6 to
  `Gen6NativeSession::open`, generation 7 on Nintendo 3DS to `Gen7NativeSession::open`, and
  answers every other registered title with "no verified staged adapter yet" plus the registry's
  editor reason;
- strict inspection: `inspectSourceFile` / `inspectSourceChoices` read a regular file read-only,
  probe it with the family's `Sources::probeSave` and always return a `Source::SaveInstance` row
  (a failed validation is a row with a diagnostic);
- discovery over verified provider layouts: `discoverKnownSources` with `DiscoveryRoots`
  (Checkpoint `sdmc:/3ds/Checkpoint/saves`, Dekopon pointer file / default user dir); provider
  labels `kProviderDekopon`, `kProviderCheckpoint`, `kProviderManual`, variant marker
  `kSourceVariant3ds` ("3ds-single-copy"); labels must be known to
  `include/Source/SaveProviderIdentity.h` (`identifyProvider`);
- catalog / assignment / open: `forGameAndProfile`, `assignValidatedSource` (explicit per-profile
  binding in `Legacy::LegacySourceBindings`), `openAssignedSource` (content-fingerprint
  revalidation, `AssignedOpenStatus`), and `homeCardSnapshot` (read-only Home card from the same
  strict session, no partial party).

No function in this layer can write a source save. Test: `tests/test_modern_sources.cpp`.

### 2.6 Export: `include/Integration/Common/VerifiedStagedExport.h`, `src/Integration/Common/VerifiedStagedExport.cpp`

Generation-neutral. The transaction writes only inside the app-owned export root: a temporary
directory receives the exact original backup, the edited save and a provenance manifest; every
file is fsync'ed, read back and hash-checked; the edited bytes are strictly re-parsed by the
caller's `StrictSaveValidator`; then the directory is atomically renamed into place. Any failure
removes the temporary directory. The adapter's `exportWorkingCopy` builds the request from its
staged session (Gen VI: `Gen6StagedSession::exportWorkingCopy`); on Gen VII it returns the
registry's Export reason and writes nothing. The original save path is evidence in the manifest
only; it is never opened for writing.

## 3. Checklist: what a NEW generation (or family) must implement

Nothing else is needed; in particular no new screen, editor or menu.

1. Adapter. `include/Integration/GenN/GenNNativeSession.h` + `src/Integration/GenN/...cpp`,
   `final : Modern::NativeSession`, over a strict staged session that holds an immutable parsed
   copy plus app-memory changes. `open(bytes, assignedExactGame)` refuses a title mismatch. Every
   support answer (`cloneSupport`, ..., `exportSupport`) and every `fieldState` must be backed by
   a verified writer or answer ReadOnly / unavailable with a reason. Reasons are the registry's.
2. Registry row. A `Detail::genNRow(...)`-style builder in `GameCapabilityRegistry.h` with the
   16 `WorkflowStatus` entries, `NativeMechanics` flags true only for mechanics that title has,
   `StorageGeometry`, `recordFormat`, `statModel`, `exportRoot` (empty while Export is blocked),
   `provider` (`modern-native` once the adapter serves it), `HardwareState::NotDeviceTested`.
   Any workflow an earlier generation makes visible must be `ok()`, `ro(reason)`,
   `blocked(reason)` or `na(reason)`; the header's `static_assert` rejects anything else. If the
   title count changes, update the array size and the row count in
   `tests/test_game_capability_registry.cpp` and `tests/test_capability_matrix_doc.py`.
3. GameIdentity row. `src/Games/GameIdentity.cpp` must have a descriptor with the same id, title
   and `dataGeneration` (all 38 current ids already do); `test_game_capability_registry` and
   `tools/capability_matrix.cpp` both refuse a registry id without one.
4. ExactSaveCapabilities row. `include/UI/ExactSaveCapabilities.h`: `SaveFormat`, `SaveFamily`,
   the `exactSaveCapabilities(identity)` branch and the id lookup, so the shared editor contract
   knows the format (PK6/PK7 are the pattern to copy). This describes the audited UI model only
   and enables no writer.
5. Fixtures and test. `tests/fixtures/genN/GenNSyntheticSave.h` (synthetic, re-signed save
   built from source-backed layout constants; never a real save) and
   `tests/test_genN_native_session.cpp` mirroring the sections of
   `tests/test_gen6_native_session.cpp`: open (exact-title assignment authoritative), record
   views, field model, staging, clone/move/remove/create, bag, trainer/dex, export (separate
   verified copy, source bytes untouched, or refused with the registry reason), draft bracket.
   Add the family to `tests/test_modern_sources.cpp` (discovery, validation, open, Home card).
6. Makefile fragment. `Makefile.host.genN-plus` modelled on `Makefile.host.gen7-plus`: GCC and
   Clang, plain and `_sanitize` binaries, added to `HOST_TESTS` / `HOST_SANITIZE_TESTS` and to
   `host-test` / `host-sanitize`; one `include` line in `Makefile.host`.
7. Dispatch and labels. `src/Integration/Modern/ModernSources.cpp`: `openSession`,
   `gameGroupFor` (the `Enums::GameVersion` group), `generationLabelFor`, the `inspectSourceFile`
   validation branch, and any new provider label (which `Source::identifyProvider` must know).
8. Regenerate the matrix: `make -f Makefile.host capability-matrix-doc`, commit
   `docs/gen6-plus/CAPABILITY_MATRIX.md` with the registry change; `host-test` fails otherwise.
9. Record the honest state in the handoff (`docs/handoffs/MAIN_ACTIVE_HANDOFF.md`) with the
   status words of section 5. The row stays `NotDeviceTested` until the owner records a result.

## 4. What must NEVER be done

- No per-generation editor, screen or menu. The surface, editor template, Review, Discard and
  Export flow are shared; a family contributes an adapter and a registry row only. A family that
  needs a UI special case has a missing field in `RecordView`, `fieldState`, `nativeFacts` or
  `NativeMechanics`, and that is where the change goes.
- No source writes. Original saves (emulator, Checkpoint/Dekopon exports, installed titles) are
  immutable. `NativeSession` has no write path to the source; `ModernReadOnlyTrainer`'s
  `update*Block()` are no-ops; Export produces a separate verified copy under the app-owned root.
  Installed-game injection on Switch titles is hard-disabled (registry reason).
- No invented offsets, limits or mechanics. Every layout constant, checksum, signature scheme and
  mechanic rule must cite PKHeX (pinned `6501f0ab`), PKSM-Core (pinned `aa22d7a4`) or pret. When
  the two references disagree (the SAV7 signature payload tail, `docs/gen7/README.md`), the
  workflow stays Blocked until owner evidence settles it; guessing is forbidden.
- No automatic legality edits, and missing legality evidence is never a verdict (Unknown /
  Incomplete, not Invalid).
- No capability advertised anywhere but the registry; no `Supported` without host tests.
- No weakened, skipped or deleted assertion to get a build green; no backward reset or
  force-push.
- No hardware claim by anyone but the owner. No row is `OwnerAcceptedExactBuild` (a
  `static_assert` requires `hardwareEvidence` naming the exact NRO for that state). Gen I-V rows are
  `OwnerTestedEarlierCandidate`: owner rounds ran on earlier candidates, issue #313 (all-release
  acceptance) is open. Every Gen VI-IX row is `NotDeviceTested`; the generated matrix check refuses
  anything else.

## 5. Status vocabulary

Registry `Support` and the generated matrix:

| Registry | Matrix cell | Means |
| --- | --- | --- |
| `Supported` | `PASS` | reachable through the shared application workflow and host-tested; never hardware acceptance |
| `ReadOnly` | `READ-ONLY` | visible through the shared workflow; editing not verified |
| `Blocked` | `BLOCKED - <reason>` | deliberately unavailable; the reason names the prerequisite |
| `NotApplicable` | `NOT APPLICABLE - <reason>` | the native game has no such data |
| `NotImplemented` | `NOT IMPLEMENTED` | nothing exists; only legal where no earlier generation supports it (currently unused) |

`HardwareState`: `NotDeviceTested` ("NOT TESTED"), `OwnerTestedEarlierCandidate` ("NOT ACCEPTED
(owner rounds on earlier candidates; issue #313 open)") or `OwnerAcceptedExactBuild` ("OWNER ACCEPTED
on <exact NRO>", requires `hardwareEvidence`; currently unused). Only the owner's recorded result
on the exact NRO may set the last one.

`UiIntegration` separates backend support from a working application path: `ModernSurfaceHostTested`
means the title runs through `src/UI/ModernSharedSurface.inc` in the real `TrainerViewScreen`
driven by a host-UI harness test (`uiEvidence`); it is still not a hardware result.

Engineering status words (`docs/ENGINEERING_PROTOCOL.md`): Implemented (code written), Tested
(host tests run and passed; name the command and result), Pushed (SHA verified by
`git ls-remote`), Integrated (on #307 with CI green), Hardware verified (owner-recorded PASS on a
Switch). Never "verified on hardware" for anything the owner has not recorded.

## 6. Current honest state per family (reasons copied from the registry)

Workflows not listed are `Supported` for the `modern-native` families and `ReadOnly` or `Blocked`
as stated for the Switch titles; the exact per-title cells are in
`docs/gen6-plus/CAPABILITY_MATRIX.md`.

### Gen VI: X, Y, Omega Ruby, Alpha Sapphire (`modern-native`, PK6, 31 x 30 SparseNative, export root `exports/gen6`)

- Create: BLOCKED - "Create needs verifiable EC/PID, personal, met, handler, memory and
  geolocation defaults (Gen6StagedSession::createCapability)".
- Native mechanics: READ-ONLY - "Mega/form, Super Training and Pokemon-Amie bytes are shown
  read-only; no verified writer".
- Trainer: READ-ONLY - "MyStatus trainer fields are read-only until a byte-mask verifier exists
  for block 17".
- Pokedex: READ-ONLY - "Zukan seen/caught counts are read-only".
- Dashboard, Save Instances, Party, Boxes, editor, Bag, Clone, Move, Remove, Backup, Review,
  Export: Supported (host-tested; `tests/test_gen6_native_session.cpp`,
  `tests/test_modern_sources.cpp`, `tests/test_modern_trainer_bridge.cpp`).

### Gen VII: Sun, Moon, Ultra Sun, Ultra Moon (`modern-native`, PK7, 32 x 30 SparseNative, no export root)

- Export: BLOCKED - "SAV7 MemeCrypto re-signing has no retail acceptance evidence
  (docs/gen7/README.md); staged changes stay in app memory". Background: PKHeX `6501f0ab`
  `MemeCrypto.SignInPlace` and PKSM-Core `aa22d7a4` `resign` build the signed payload tail
  differently (zero vs non-zero); both verify, and the project rule forbids guessing which the
  game expects. The owner check that settles it is written in `docs/gen7/README.md`.
- Create: BLOCKED - "Create is unsupported for the same reasons as Gen VI, plus SAV7 export is
  blocked".
- Native mechanics: READ-ONLY - "Z-Crystal, Alolan form and Hyper Training bytes are shown
  read-only; no verified writer". Trainer: READ-ONLY - "MyStatus trainer fields are read-only".
  Pokedex: READ-ONLY - "Zukan seen/caught counts are read-only".
- Dashboard, Save Instances, Party, Boxes, editor, Bag, Clone, Move, Remove, Backup, Review:
  Supported in app memory (`tests/test_gen7_native_session.cpp`).

### Switch titles (`legacy-modern-reader`): installed-title readers exist (`Trainer7LGPE`, `Trainer8SWSH`, ...) but present through the pre-shared generic screens; there is no verified staged session or export verifier yet

Common to all of them: Dashboard Supported (installed-title card via the account save mount);
Save Instances READ-ONLY ("Installed save is console-managed; manual/Checkpoint copies need the
strict parser of the adapter"); Party and Boxes READ-ONLY ("Shown by the legacy modern reader;
staged editing needs the shared Modern adapter"); Trainer, Bag, Pokedex READ-ONLY; Backup
READ-ONLY ("Immutable Switch historical backups are read-only previews"); Export BLOCKED -
"Installed-game injection is hard-disabled; a verified Export Copy needs the adapter's strict
re-parse and byte-diff verifier". Pokemon editor, Native mechanics, Create, Clone, Move, Remove and
Review are BLOCKED with the family's adapter prerequisite:

- Let's Go Pikachu / Eevee (PB7, 40 x 25 PointerBacked, IV/Awakening): "LGPE needs a PB7 staged
  session over the pointer-backed 1000-slot storage with starter/party pointer repair".
- Sword / Shield (PK8, 32 x 30 SCBlock): "SWSH needs Gen8StagedSession over the strict SCBlock
  parser (Integration/Gen8) with a per-field byte mask, battle-team lock refusal and
  companion-file carry-through".
- Brilliant Diamond / Shining Pearl (PB8, 40 x 30 FlatPairedFile): "BDSP needs the paired-file
  authority protocol (which of the two save files is current) before any staged session".
- Legends Arceus (PA8, 32 x 30 Pastures, IV/EffortLevel): "PLA needs a PA8 staged session over
  the SCBlock container with Effort Level, mastery and pasture geometry verification".
- Scarlet / Violet (PK9, 32 x 30 SCBlock): "SV needs a PK9 staged session over the SCBlock
  container with revision/DLC block verification and Tera/Minted-Nature masks".
- Legends Z-A (PA9, 32 x 30 SCBlock): "Z-A needs a separate PA9/SAV9ZA staged session (0x198 box
  stride, Plus flags, footer/presence updates); it is not SV".

## 7. Generated matrix

- Generator: `tools/capability_matrix.cpp` (links `GameCapabilityRegistry.h` and
  `src/Games/GameIdentity.cpp`; refuses to print when a registry id has no agreeing descriptor).
- Output: `docs/gen6-plus/CAPABILITY_MATRIX.md`, one row per title grouped by generation, 12
  capability columns, a "Native mechanics per title" table and a deduplicated "Blockers" list.
- Fragment: `Makefile.host.capability-matrix` (`$(HOST_BUILD)/capability_matrix`,
  `capability-matrix-doc`, `capability-matrix-doc-check`; the check is a prerequisite of
  `host-test` and `host-sanitize`, like `gen4-safety-contract`).
- Check: `python3 tests/test_capability_matrix_doc.py --binary <HOST_BUILD>/capability_matrix`
  asserts byte identity, 38 rows, the 12 columns in order, no empty cell, and NOT TESTED in every
  Gen VI-IX hardware cell.
