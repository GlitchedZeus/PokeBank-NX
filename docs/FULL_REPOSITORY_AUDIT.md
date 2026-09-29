# Full Repository Forensic Audit

Status: IN PROGRESS

## Exact state

### Live MAIN forward-delta reconciliation (2026-09-29)
- PR #92 advanced from the last reconciled MAIN checkpoint `491476b392ef78f7bc70b4dad7dd669d26f9361f` to live head `fb7b1d8cd3f3847aec8646104716e390060cfcc8`.
- The forward delta is 56 commits / 27 changed paths: 15 newly tracked text paths plus 12 modifications to paths that had already been audited.
- Recursive live-tree inventory at `fb7b1d8c…`: 746 tracked paths, 713 text/unknown paths, 32 binary/non-text paths, and 1 submodule. Binary/non-text count is unchanged.
- The 15 new paths were inserted into the ledger as PENDING. The 12 changed previously-AUDITED paths were demoted to PENDING until their current blobs are fully re-read. Historical findings/follow-up flags were preserved.
- The report therefore does **not** yet promote the Primary MAIN audited head to `fb7b1d8c…`; `491476b3…` remains the last fully reconciled MAIN checkpoint while this delta is inspected forward.


- Repository: GlitchedZeus/PokeBank-NX
- Audit branch: `audit/full-repository-line-by-line-20260928`
- Primary MAIN tree audited: PR #92 head `491476b392ef78f7bc70b4dad7dd669d26f9361f`
- PR #92 branch: `feature/gen4-full-editor-20260928`
- Sibling UI overlay coverage baseline remains `5f19fd14628c182db135038864036b6eb1b86c49`; live PR #97 is now `068fb5e101780037e79d0f17f457574cdcc800b7`, with its forward delta tracked separately from the MAIN denominator
- Integration parent: PR #90 head `8b3bcc16c804247bfe8d1314b686974ce73051d8`
- Hardening parent: PR #79 head `00ee7a6ed7ac1b5a93c43246d70c252e135acec0`
- Default branch main: `aca2bf41c83d81084886a46d53195f6cead81ccc`

## Inventory

- Git tree truncated: false
- Tracked non-directory paths: 731
- Text/unknown candidates: 698
- Binary candidates: 32
- Symlinks: 0
- Submodule entries: 1
- Audit ledger: `docs/FULL_REPOSITORY_AUDIT_LEDGER.tsv`

## Coverage

- Audited tracked paths: 653 / 746
- Fully read text files: 619 / 713
- Binary/non-text inspected: 34 / 34 currently identified by exact extension/manifest scan

## Current checkpoint — live MAIN catch-up

- PR #92 advanced from `85762adce4a1ce6f30763a0d76b3b11735da7d49` to `491476b392ef78f7bc70b4dad7dd669d26f9361f` (2 commits / 8 changed paths). Every changed path that previously carried AUDITED status was completely re-read at the new head: `include/UI/MovePickerPresentation.h`, `src/UI/Gen2HardwarePickerFix.inc`, and `src/UI/Gen4SharedPokemonSurface.inc`. Their ledger line/byte metadata was refreshed. The other five changed paths were already PENDING and remain catch-up work rather than being promoted without a full read.
- This delta centralizes the compact move-picker geometry and applies it to Gen II/IV while preserving exact-game move-stat lookup and compatible-move filtering. No new numbered defect was confirmed in the re-read audited files; staged/source immutability remains unchanged.
- `src/UI/Gen3SharedPokemonSurface.inc` was fully read at `491476b3…` (1,751 lines / 79,423 bytes) and promoted from PENDING to AUDITED. The Gen III shared surface preserves staged-only Create/Edit/Clone/Release, exact-game compatible move filtering, explicit exit safeguards, and encounter-constrained met-location/level editing. No new numbered defect was confirmed in this file.
- Latest shared-editor test tranche fully read at `491476b3…`: `tests/test_gen3_hardware_surface_contract.cpp`, `tests/test_gen4_hardware_surface_contract.cpp`, `tests/test_gsc_editor_surface_contract.cpp`, and `tests/test_shared_pokemon_editor_contract.cpp` were promoted from PENDING to AUDITED; `tests/test_move_picker_presentation.cpp` was re-read and its metadata refreshed. These contracts cover row-only move focus, exact-generation Acc/Pwr/PP presentation, staged immutable-source exit behavior, Gen II picker parity, and Gen III/IV shared-surface routing. No stale expectation or new numbered defect was confirmed.
- `src/UI/SaveSelectScreen.cpp` was fully read at PR #92 head `491476b3…` (1,278 lines / 64,968 bytes) and promoted to AUDITED. Its save-opening boundary revalidates displayed legacy snapshots before open and Gen IV candidate assignment re-inspects the selected file. One UI scroll-window mismatch was confirmed and recorded as AUDIT-038.
- `src/UI/TrainerViewScreenGSCOverlay.inc` was fully read at `491476b3…` (787 lines / 34,758 bytes) and promoted to AUDITED. Gen II export remains delegated to `Gen2ExportTransaction`, staged review/discard paths do not write source bytes, and the validated-GSC input shim contains no direct file-write path. No new numbered defect was confirmed.
- `src/UI/TrainerViewScreenBase.inc` was fully read at `491476b3…` (5,070 lines / 295,298 bytes) and promoted to AUDITED, completing the pending `src/UI` tranche. Its durable cross-store true-Move path still requires an app-owned mutable workspace, exact-game/profile scoping, pre-pickup store digests, empty/unlocked destinations, destination verification before source retirement, and recovery locking. One separate backup-save custody bug was confirmed and recorded as AUDIT-039.
- Remaining first-party core tranche fully read at `491476b3…`: `include/Globals.h`, `include/Inventory/ClassicInventoryCatalog.h`, `src/Inventory/ClassicInventoryCatalog.cpp`, `include/Legality/Legality.h`, `src/Legality/Legality.cpp`, and `src/main.cpp`. Startup keeps the physical durability harness exact-marker opt-in and separate from normal UI startup; the classic catalog keeps per-generation item/machine namespaces explicit; legality remains informational and preserves exact Gen III source identity where available. No new numbered defect was confirmed in this tranche.
- Compact host-test tranche fully read at `491476b3…`: action sheet, classic inventory catalog/UI, classic picker typing, clean-install runtime, controller model, encounter guardrails, game identity, Gen I battle stats, and Gen I move compatibility. All ten were promoted from PENDING to AUDITED. No stale expectation or new numbered defect was confirmed.
- Small safety/fixture test tranche fully read at `491476b3…`: canonical Gen IV blank fixtures + provenance, write-policy lock, sprite layout, runtime dependency path guard, Gen III party-record fidelity, PID-search fail-closed behavior, Gen I shiny DV behavior, and exact-source Gen III legality context. All ten were promoted from PENDING to AUDITED. No stale expectation or new numbered defect was confirmed.
- Classic navigation/path test tranche fully read at `491476b3…`: Gen II personal/gender oracle, Gen I radar rendering, shared move-row focus fixture, PokeBank path ownership, generation-aware machine display, RBY source/profile binding, held-repeat navigation, Gen I shared-editor UX, Gen II inventory decoding, and exact G/S/C move compatibility. All ten were promoted from PENDING to AUDITED. No stale expectation or new numbered defect was confirmed.
- Source-model safety test tranche fully read at `491476b3…`: pre-Gen III shared-editor reuse guard, Gen III shell rendering, RBY inventory, Gen II runtime catalog refresh, Gen I summary presentation, passive Gen I-III views, provider-neutral SaveInstance semantics, and Gen IV external-source read-only routing. All eight were promoted from PENDING to AUDITED. No stale expectation or new numbered defect was confirmed.
- Save-instance/GSC transaction tranche fully read at `491476b3…`: provider-neutral architecture gate, shared Gen II fixture builder, GSC source browser, GSC UI/capability rules, RBY staged inventory, and Gen II verified staged export transaction. All six were promoted from PENDING to AUDITED. No stale expectation or new numbered defect was confirmed.
- Medium parser/polish test tranche fully read at `491476b3…`: shared inventory UI, bounded GSC discovery, Gen II native presentation, Gen III exact-format editor provider, strict GSC parser adapter, universal staged GSC inventory, and V1 polish static contract. All seven were promoted from PENDING to AUDITED. No stale expectation or new numbered defect was confirmed.
- Staged/source-bridge test tranche fully read at `491476b3…`: RBY read-only bridge, RSE open-with-optional-inventory regression, shared editor foundation contract, and Gen I Cleanup #3 staged semantics. All four were promoted from PENDING to AUDITED. No stale expectation or new numbered defect was confirmed.
- Staged inventory/GSC bridge tranche fully read at `491476b3…`: Gen II workspace rendering, Gen II staged editor round-trip, Gen III exact-game staged inventory/sector preservation, and Gen II read-only bridge with staged presentation refresh. All four were promoted from PENDING to AUDITED. No stale expectation or new numbered defect was confirmed.
- RBY discovery / Gen II shared-editor parity tests fully read at `491476b3…`. Discovery keeps bounded traversal, physical alias dedupe, stable source identity and refresh fingerprints; the Gen II session contract preserves existing unusual moves while rejecting newly introduced incompatible moves and keeps Create/Edit/Trainer work staged-only. Both were promoted to AUDITED with no new numbered defect.
- Shared-editor presentation/session tests fully read at `491476b3…`: Gen I/II fullscreen heading/type/focus hardware contract and Gen III shared editor draft/session contract. Both were promoted to AUDITED; no stale expectation or new numbered defect was confirmed.

- PR #92 live-head catch-up is now reconciled through `85762adce4a1ce6f30763a0d76b3b11735da7d49` (4 commits / 16 changed paths beyond `a86c8d039209bc17ac613524c6631f03309037a0`). Every path in that delta that already carried AUDITED status was fully re-read at the new head: `include/UI/Gen1PokemonEditorUIContract.h`, `include/UI/SharedPokemonEditorContract.h`, `src/Names/MachineDisplay.cpp`, `src/Names/TMMoves.cpp`, `src/UI/Gen2HardwarePickerFix.inc`, `src/UI/Gen2SharedPokemonSurface.inc`, and `src/UI/Gen4SharedPokemonSurface.inc`. The remaining changed paths were already PENDING and stay PENDING. No new numbered defect was confirmed from this catch-up; staged/source immutability and row-level move focus semantics remain intact.
- UI tranche progress: `src/UI/Panels/BoxPokemonPanel.cpp` was fully read at `85762adc…` (381 lines / 22,188 bytes) and is now AUDITED. The file renders native box grids, carried-slot presentation, quick summary fields, and generation-aware stat radars. No new correctness or source-mutation defect was confirmed in this file.
- `src/UI/Modals/PokemonDetailsModal.cpp` was also fully read at `85762adc…` (571 lines / 34,505 bytes) and is now AUDITED. It is the generic modern summary/edit presentation path; Gen I is explicitly routed to its native modal first, while legality source identity is left empty for Bank targets rather than borrowing the currently-open save. No new confirmed defect was found in this file.

- PR #92 catch-up is reconciled through live head `a86c8d039209bc17ac613524c6631f03309037a0`. The four changed paths that already carried AUDITED status (`include/Integration/Gen4/Gen4StagedPokemonEditor.h`, `src/Integration/Gen4/Gen4StagedPokemonEditor.cpp`, `src/UI/Gen4SharedPokemonSurface.inc`, `tests/test_gen4_staged_pokemon_editor.cpp`) were fully re-read at that head; the other two changed paths were already PENDING and remain PENDING.
- Live tracked inventory is 731 non-directory paths / 698 text-or-unknown candidates; 653 paths are currently reconciled at the live-tree inventory level and 619 text files are fully read at their current audited blobs.
- The entire Names tranche is now closed: no `include/Names` or `src/Names` file remains PENDING. Generated species tables contain 1,026 entries in each of nine languages; the modern item-name table contains ids 0..2684; Gen III direct item names cover ids 0..376.
- MovePresence's unknown-group/id-0 behavior contradicts its comment, but all audited real game-group callers are routed through known groups; kept as a hardening follow-up, not a numbered defect.
- Recovery/package/source-pin tooling is now substantially audited. Supported CI invokes `verify_embedded_romfs.py` with normal `python3`; its assert-based checks are therefore live today, while replacing asserts with explicit failures remains a robustness follow-up.
- HD sprite recovery/preflight currently proves filenames/counts more strongly than file validity. A corrupt or zero-byte existing sprite can survive count-based recovery and presence-only preflight. Recorded as AUDIT-034.
- Exact-game Gen IV move-stat presentation defect remains AUDIT-033: Diamond/Pearl are currently shown the shared HGSS values for at least Hypnosis.


## Sibling PR #97 overlay coverage

- Last fully reconciled sibling checkpoint: `456a493771bc25ba49b7b9c27269477eec835a07`; newer live PR #97 head observed: `cf28531af5f9c23467d938083ad1a88e55bd042e`.
- Exact delta from PR #90 `8b3bcc16c804247bfe8d1314b686974ce73051d8`: 35 commits / 27 changed paths.
- The sibling overlay remains intentionally outside the 731-path PR #92 MAIN denominator. Full line-by-line coverage of all 27 sibling-overlay changed paths is still incomplete and is not claimed here.
- The forward delta from checkpoint `3831989f7b320797068e752321ae0c36c2216f49` to live head `456a493771bc25ba49b7b9c27269477eec835a07` is 14 commits / 9 changed paths. All nine current-head files in that forward delta were fully read: `include/UI/AppShellModel.h`, `include/UI/AppShellScreen.h`, `include/UI/SaveSelectScreen.h`, `include/UI/ScreenChrome.h`, `src/UI/AppShellScreen.cpp`, `src/UI/SaveSelectScreen.cpp`, `src/UI/UI.cpp`, `tests/test_app_shell_model.cpp`, and `tests/test_game_hub_contract.py`.
- Historical AUDIT-032 remains FIXED at the live sibling head: the removed `AppShellSection::Collections` member is no longer referenced by `AppShellScreen.cpp`.
- Current sibling Product Home is `SaveSelectScreen`; its inline nav hint truthfully says `ZL: Profile`, `L/R: Change Game`, and `B: Exit`, but the reachable Help overlay still advertises `L / R` as profile switching and `B` as returning to a Main Menu. This is recorded as AUDIT-036.
- The newer `456a4937… -> cf28531a…` sibling delta is 2 commits / 2 changed files (`src/UI/AppShellScreen.cpp`, `src/UI/SaveSelectScreen.cpp`) and is not yet claimed as fully line-by-line reconciled. AUDIT-036 was specifically revalidated against `cf28531a…` and still reproduces in the live Help copy.



### Shared UI header tranche

- Fully read 32 previously PENDING `include/UI` paths at live PR #92 head `a86c8d039209bc17ac613524c6631f03309037a0`, covering action-sheet models, backup-selection interfaces, classic inventory/picker helpers, dialog contracts, exact-save capability mapping, controller/analog repeat, shared exit policy, Gen I/II/III/IV editor session state machines, and the shared editor/foundation navigation contracts.
- No new confirmed defect was produced by this tranche. A suspected Gen III Create SID/origin loss was explicitly disproved: the backend reconstructs SID from the validated trainer, derives origin from the exact source game, and includes origin in its strict semantic reparse verification.
- `ClassicSpeciesLevelPolicy.h` remains a follow-up-only compatibility shim because it intentionally returns the fallback level and directs new code to `SpeciesChangeLevelPolicy`; no live production caller defect was established in this pass.
- A few older foundation booleans are stale-looking relative to newer product capabilities, but without a reachable consumer contradiction they remain caller-trace/hardening questions rather than findings.


### UI header tranche closure

- Closed the remaining 34 PENDING `include/UI` files at live PR #92 head `a86c8d039209bc17ac613524c6631f03309037a0`. The primary-tree `include/UI` tranche is now 0 PENDING.
- The remaining pass covered Gen I/II presentation/picker helpers, Gen II native caught-data/Pokérus state, Gen III encounter-picker helpers, generic modal/panel interfaces, sprite/cache contracts, touch/controller chrome, theme palettes, framebuffer interface, the shared Pokémon shell and stat-radar renderer, plus the MAIN SaveSelect/UI manager interfaces.
- `Color::toRGBA8()` in `include/UI/Common.h` performs left shifts after integral promotion to signed `int`; an alpha such as 255 shifted by 24 is a latent undefined-behavior API hazard. No live call site was found in the audited renderer path, so this is recorded as hardening/follow-up rather than a numbered production defect.
- MAIN's Gen II held-item helper still has the older outline/text presentation while sibling PR #97 carries the selected-surface/SelectedText readability cleanup. Without a proven current MAIN production caller contradiction, this is tracked as UI integration follow-up rather than a separate finding.
- No source-write or staged-save safety invariant was weakened by the audited header code.



### Core UI runtime tranche

- `src/UI/Gen1PokemonEditorOverlayUX.inc` was fully read (1,172 lines / 50,626 bytes). This first included UX layer keeps mutations staged/export-only; its older navigation semantics are macro-renamed and wrapped by later Cleanup/Foundation layers before live dispatch. No new defect was confirmed.
- `src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc` was fully read (1,192 lines / 56,299 bytes). Contextual Level/EXP, Move, DV and Stat Exp editors hold temporary state until explicit Apply; B exits those sub-editors without staging the temporary edits. No new transaction defect was confirmed.
- `src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc` was fully read (725 lines / 35,411 bytes). Exact-game move filtering, DV-backed shiny toggling with session restoration state, species-change level policy, and joystick repeat were traced into the live wrapper chain. Foundation remains the owner of current main Edit/Create exit confirmation. No new defect was confirmed.

- `src/UI/Panels/StoragePanel.cpp` was fully read at PR #92 head `85762adce4a1ce6f30763a0d76b3b11735da7d49` (628 lines / 35,794 bytes). It is presentation/chrome for save/bank storage, carried groups and confirmation dialogs; runtime mutation remains outside this panel and read-only source/action gating is preserved. No new defect was confirmed.
- `src/UI/Gen1PokemonEditorOverlayFoundation.inc` was fully read (636 lines / 31,073 bytes). Its duplicate legacy Values-panel Level presentation is unreachable because `PokemonEditorFoundationContract` exposes exactly six Values rows (five stat rows plus Shiny); no current-path defect was assigned.
- `src/UI/Gen1PokemonEditorOverlay.cpp` was fully read (1,095 lines / 48,396 bytes). It contains an older compiled Gen I staged-editor overlay, but the live `TrainerViewScreenCompositeOverlay.cpp` dispatches through the included UX/foundation chain and no UI translation-unit runtime call site invokes this file's `Gen1PokemonEditor::handleInput()`/`drawOverlay()` pair. Its old direct Edit→Actions B behavior is therefore not the current Gen I exit path; no false finding was created.

- `src/UI/Gen1PokemonEditorFoundationHardwareFix.inc` was fully read at PR #92 head `85762adce4a1ce6f30763a0d76b3b11735da7d49` (367 lines / 20,578 bytes). It is fullscreen Gen I presentation/focus/picker ownership code; native unified Special, derived HP DV, row-level move focus, exact-game context, and truthful unsupported-legality labeling remain intact. No new defect was confirmed.
- `src/UI/ClassicInventoryOverlay.cpp` was fully read at the same MAIN head (915 lines / 40,334 bytes). Gen I-III inventory edits remain staged; export writes only app-owned backup/edited-copy/manifest artifacts and explicitly keeps live emulator/source writes disabled. A presentation-refresh failure after a completed inventory mutation can leave the old UI snapshot visible, but the refresh uses build-then-swap semantics and the backend mutation is a completed staged edit rather than an open transaction; no new integrity finding was assigned.

- `src/UI/ClassicPackedMoveOverlay.inc` was fully read at PR #92 head `85762adce4a1ce6f30763a0d76b3b11735da7d49` (650 lines / 27,633 bytes). Its normal single-pickup rollback path is sound, but the multi-select Gen I/II pickup path does not cancel the backend packed-group transaction if the post-pickup presentation refresh fails. Backend tracing confirmed that the selected records have already been removed from staged bytes and `cancelPackedMove()` is the intended rollback. Recorded as AUDIT-037.

- Fully read 16 previously PENDING `src/UI` implementation paths at live PR #92 head `a86c8d039209bc17ac613524c6631f03309037a0`: classic inventory model logic; item/picker/save/stat dialogs; Gen I/II details renderers and dispatcher; loaded-workspace home rendering; NanoVG framebuffer; sprite and system-icon caches; touch polling; and the TrainerView composition/shim translation units.
- No new numbered finding was confirmed. The renderer does not call the latent `Color::toRGBA8()` helper; it converts colors directly through NanoVG's unsigned-channel API, which supports keeping the signed-shift issue as hardening rather than a production-path defect.
- Sprite ownership is coherent in this tranche: LRU eviction invalidates the renderer's texture before freeing a decoded buffer, the active-framebuffer callback is cleared during teardown, and system/profile/game-card icon caches free their session-owned decoded buffers at shutdown.
- Save/exit dialog rendering retains the installed-save write lock and does not introduce an Apply-to-live-title path.



### UI routing / panel tranche

- Fully read eight additional `src/UI` paths: the Gen I passive view; Gen II hardware move-picker/workspace and staged trainer adapters; Items and Party panels; the composite Gen I-IV TrainerView dispatcher; and MAIN `src/UI/UI.cpp`.
- MAIN source-opening orchestration still constructs external Gen I-IV emulator sources under read-only source kinds, routes installed-title editing through app-owned backup workspaces, and performs pending durable-Move recovery before parsing a mutable backup workspace.
- A suspected Items-panel pre-guard pouch-index risk was traced through `TrainerViewScreenBase.h`: `selectedCategory` is default-initialized to zero and every normal category transition wraps through the exact generation-specific pouch count. Several low-level `getPouchInfo*` helpers remain unbounded for arbitrary enum inputs, but no production UI-state route to an invalid value was established; retained as API hardening only.
- No new numbered finding was confirmed in this tranche.



### Gen II parity / staged-action tranche

- Fully read the remaining current Gen II picker/parity/shared-action and classic Release layers: `Gen2PokemonPickerOverlay.inc`, `Gen2HardwareFinalFix.inc`, `Gen2SharedSurfaceParity.inc`, `Gen2SharedPokemonSurface.inc`, and `ClassicReleaseActionFix.inc`.
- Species/Move/Crystal encounter pickers update only the in-memory working draft until explicit commit; the hardware Move layer narrows choices to Empty plus exact-game-compatible Gen II moves.
- Shared Box/Storage action input is consumed before the inherited source-write guards can see the same A/X press. Clone and Release call only the staged Gen II editor, and Release remains behind an explicit confirmation surface.
- A suspected Clone destination mismatch was disproved: Gen II boxes are compact lists, so the UI's first empty slot is the backend's current count/destination slot. The backend copies native body/OT/nickname bytes, synchronizes the current-box mirror, and verifies the staged result.
- The passive View footer in one final repaint still says `D-pad` rather than `D-pad/Stick` despite controller parity; this is UI-consistency follow-up only.
- No new numbered finding was confirmed.


### Save-safety checkpoint — write-path trace

- `include/Safety/WritePolicy.h` keeps `LIVE_SAVE_WRITES_ENABLED = false`; `restoreBackupToTitle()` checks the low-level policy before mounting a live title save.
- `include/Safety/SourceMutationPolicy.h` permits mutation only for `BackupOrStaged` and `AppOwnedStorage`; installed, RetroArch and external legacy sources are not mutation-capable through this policy.
- Production true-Move store resolution accepts only the app-owned Bank or validated PokeBank backup workspace paths. It does not resolve arbitrary emulator/source paths.
- Cross-game transactions require a retirement gate before source retirement; the ordinary production recovery engine has no such gate, so a cross-game journal cannot silently retire a source through the default path.
- These observations support the current immutable-source/live-write-lock invariants for the audited paths; emulator discovery/caller tracing remains pending and is not yet claimed complete.


### Emulator/source immutability checkpoint

- Gen I-III RetroArch discovery reads battery saves read-only and validates exact-size/native structures before surfacing them.
- mGBA discovery consumes only its configured `savegamePath`, rejects parent traversal and filesystem roots, and does not infer permission to crawl the SD card.
- Tico discovery is restricted to `tico/saves/gb`, `tico/saves/gbc`, and `tico/saves/gba` with depth 0.
- Gen IV discovery is bounded to configured RetroArch plus known DraStic/melonDS roots; `.dss` savestates are diagnostics-only and are not assignable.
- Every audited source-card bridge marks external/emulator instances `ReadOnly`; activation rechecks identity/path/size/mtime/content fingerprint and exact game support.
- The Gen I/III staged mutation cores reviewed here operate on copied in-memory bytes and do not reopen or write the physical source path. Gen IV follows the same source-bytes-to-staged-image pattern already audited.
- `SourceCapabilityBridge::canWriteOriginalSource()` is hard-coded false. This supports the immutable-source invariant in the audited source/editor boundary.

### Encryption checkpoint

- Fully read the complete tracked `include/Encryption/*` + `src/Encryption/*` tranche: generic SC save crypto plus Gen III, IV, LGPE, SWSH, BDSP, PLA, SV and Z-A Pokémon crypto.
- Gen III and Gen IV implementations explicitly handle their native record geometry; Gen IV rejects non-0x88/0xEC record sizes before crypt/shuffle.
- Later-generation helpers assume their callers supply a complete native record span. Current audited callers predominantly construct fixed-size records; malformed-span reachability remains a caller-trace follow-up rather than a confirmed corruption finding.
- No cryptographic round-trip defect was confirmed in this tranche.

### Conversion fidelity checkpoint

- Fully read the conversion API/fidelity/PID-search core, the complete 1,451-line production converter, the complete 761-line route-evidence implementation, and the storage conversion custody contract test.
- Production save placement passes `saveOriginVersion(trainer, titleId)` to the converter. That helper resolves the exact installed title first, preserving FireRed vs LeafGreen (and other paired-title identities) instead of relying only on a format group.
- Cross-game preparation operates on a const authoritative source and produces a separate candidate; commit ordering places the prepared candidate before retiring/replacing carried custody.
- Conversion sanitizes destination-illegal moves/relearn moves/items, clamps destination PP, refreshes checksum, and records declared fidelity losses/adaptations.
- Persisted cross-game evidence binds transaction/store identities, source/destination payload hashes, species/form identity, declared fidelity semantics and acknowledgement state. Product true-Move cross-game retirement remains disabled by policy.
- No new conversion-corruption defect was confirmed in this tranche. Remaining follow-up is broader caller/test coverage, not an identified failing conversion rule.

### Modern Trainer/save serializer checkpoint

- Fully read the shared Trainer base plus the BDSP, Sword/Shield, Legends: Arceus, Scarlet/Violet and Legends: Z-A Trainer headers and complete parser/serializer implementations at live PR #92 head `09c168ddfd4493ed5d06a33066c0ba56cdc9dff8`.
- Reconciled the #92 head drift against the ledger. The two changed paths that were already marked AUDITED — `src/Pokemon/Pokemon4Mutable.cpp` and `tests/test_shared_species_picker_surface_contract.cpp` — were re-read at the live head; the other changed paths were already PENDING, so no stale audited status remains.
- PLA remains the strongest SC-save open path in this group: it verifies container hash/parse completion, required block uniqueness/size, and every nonempty party/pasture Pokémon checksum before opening.
- BDSP fixed-offset parsing is now length-gated and its mutable save route is still deliberately blocked until a recoverable multi-file journal exists. Its pre-open gate still does not verify the stored whole-file MD5.
- Sword/Shield, Scarlet/Violet and Z-A still rely on the generic SC hash/container validator rather than a game-specific required-block layout validator. This is now recorded separately because their mutators can grow malformed short fixed-role blocks and emit a fresh valid SC hash.
- The Gen IV mutable core added exact native Create/OT/form behavior at the new #92 head and continues to reparse/verify transactional edits. The previously noted unrestricted low-level `setPP()` API remains a caller-hardening follow-up rather than a confirmed production corruption path.

### Central save orchestration checkpoint

- Fully read `include/Save/GetSaveFileContents.h` and all 977 lines of `src/Save/GetSaveFileContents.cpp` at live PR #92 head `09c168ddfd4493ed5d06a33066c0ba56cdc9dff8`.
- The generic save path builds a complete post-image first, validates it, then promotes it through `DurableFile::replace()`; BDSP remains excluded because its SaveData.bin + Backup.bin pair still lacks a recoverable file-set journal.
- PLA's pre-open and post-image path share the stronger semantic validator. SWSH/SV/Z-A still use the weaker generic SC structural/hash validator recorded in AUDIT-021.
- FRLG and LGPE post-image validators recompute/check native checksums, but the open path still reaches their parsers before the equivalent source-integrity proof, as recorded in AUDIT-017/AUDIT-018.
- Cross-checking the current PKHeX SaveUtil confirmed LGPE's physical save is `0x100000` bytes and its active Beluga region is the first `0xB8800` bytes. That exposed a current full-file validation mismatch now recorded as AUDIT-023.

### Inventory codec checkpoint

- Fully read every tracked generation-specific inventory model/header: shared item state, FRLG, LGPE, SWSH, BDSP, PLA, Scarlet/Violet and Legends: Z-A.
- FRLG/LGPE/SWSH record layouts initialize every common item field they later persist; BDSP/PLA headers are primarily geometry/catalog definitions and did not introduce a new codec defect in this tranche.
- Scarlet/Violet and Z-A share a distinct decoder bug: their derived item records decode raw flags but never initialize the inherited common `isNew` / `isFavorite` fields before the object is copied into the base inventory vector. The writer later branches on `isNew`, making the indeterminate state observable in save bytes. This is recorded as AUDIT-024.
- No dedicated Gen IX inventory round-trip tests were found in the current test tree; existing inventory tests cover classic-generation/UI contracts rather than this native record decoder.

### Pokémon ownership / Bank API checkpoint

- Fully read the 645-line abstract Pokémon ownership/edit interface plus the unified Bank API and its box-count compatibility policy.
- Unified Bank v1 records are reconstructed from fixed native record spans and checksum-rejected before entering live Bank storage. The already-recorded legacy migration checksum gap remains AUDIT-013; no separate unified-Bank format defect was added here.
- The base Pokémon class owns its decrypted buffer as a raw allocation but defaults move construction/assignment. Six modern concrete formats explicitly default their own move operations as “Allow moving for efficient transfers,” so the broken base ownership transfer is publicly exposed. Those large concrete headers remain PENDING until their complete line-by-line reads, but the exact move declarations were directly verified for LGPE/SWSH/BDSP/PLA/SV/Z-A.
- Current production containers predominantly move `unique_ptr<Pokemon>`, so this is recorded as a latent P3 memory-safety API defect rather than evidence of an already-triggered crash.

### Modern Pokémon entity checkpoint

- Fully read the LGPE, Sword/Shield, BDSP, Legends: Arceus, Scarlet/Violet and Legends: Z-A concrete Pokémon headers and implementation files at live PR #92 head `09c168ddfd4493ed5d06a33066c0ba56cdc9dff8`.
- Re-read the associated modern entity decryptors and the shared `cryptPokemon()` boundary. Their fixed block geometry is correct for valid native records, but no decoder enforces a minimum native entity length before reading the EC and fixed block region.
- SWSH/BDSP/SV/Z-A constructors explicitly document support for both 0x148 stored and 0x158 party entities yet retain the supplied size. Their `level()`/battle-stat APIs and `setLevel()`/`setExp()`/`recalculateStats()` assume the party tail exists. Current Trainer box models and unified Bank deliberately pass party-sized records, so no current production corruption path was proven; the public entity API itself remains unsafe for a documented stored-size input.
- PLA already follows the safer normalization pattern: a stored 0x168 PA8 is copied into a zero-padded 0x178 party-sized buffer before party-stat access. Current PKHeX PK9/PA9 likewise normalizes stored entities to party size, which supports applying the same pattern to the other modern classes.
- A suspected current-HP offset issue was explicitly disproved rather than logged: current PKHeX confirms `Stat_HPCurrent` at `0x8A` for PK8/PK9/PA9 and `0x92` for PA8, matching PokeBank NX.

### Legacy Pokémon entity checkpoint

- Fully read the Gen I and Gen II immutable presentation wrappers, the Gen III FRLG PK3 entity and stored/party helpers, and the strict Gen IV read-only model plus its shared-UI projection.
- Gen I/II consume typed records produced by their strict save readers and copy only bounded raw-body bytes. Their mutation APIs are inert by design.
- Gen III correctly distinguishes 80-byte stored PK3 from 100-byte party PK3: party-tail reads/writes are guarded by `dataSize`, and its crypto implementation returns a short copied buffer rather than overrunning if called with less than 0x50 bytes. However the resulting entity's ordinary fixed-offset accessors assume a native record, so constructing `Pokemon3FRLG` from an arbitrary short span remains unsafe at the entity boundary.
- Gen IV is the strongest legacy entity boundary: `Encryption4` rejects any size other than exact 0x88 stored or 0xEC party records before decrypting; the immutable parser additionally requires valid size, sanity and checksum before exposing semantic fields.
- The raw-buffer move-ownership defect from AUDIT-025 also reaches Gen I/II wrappers through implicitly generated moves and Gen III through explicitly defaulted moves.

### Conversion-critical Pokémon data checkpoint

- Fully audited the shared experience curves, Gen III/IX species-index converters, generated modern + Gen III personal metadata, LGPE/modern base-stat datasets, form-persistence rules, type helpers and ability-slot helpers.
- Programmatic structural validation covered every one of the 1,491 modern generated `PersonalInfo` rows: base rows 0..1025 are aligned, every alternate-form redirect remains within the table and points back to the expected species/form, presence masks fit the supported six-game domain, and type ids are valid or the explicit monotype sentinel. No structural generated-table defect was found.
- Gen III and Gen IX internal/National species mappings are bounded and carry compile-time boundary/field-report checks. The Gen IX divergent table covers exactly the National #917..#1025 range.
- The current PKHeX `BattleForms` / `BattleMegas` source was rechecked against `FormInfo.cpp`, including the newer Z-A Mega set through Glimmora; no stale Mega-membership defect was found.
- Two handwritten base-stat lookup layers failed independently: LGPE cannot reach the appended Meltan/Melmetal rows (AUDIT-027), while the modern form router has valid-form zero/wrong/OOB routes shared by SWSH/BDSP/PLA/SV/Z-A (AUDIT-028).

### Generated Pokémon table checkpoint

- Fully read the remaining generated Pokémon lookup/table files in this tranche: Gen IV DP/Pt/HGSS personal tables, PLA/SV/SWSH dex lookups, form-sprite mappings, the learnset API/placeholder source, and the Gen IV mutable header.
- The Gen IV personal accessors reject species 0/out-of-range, fall back safely for unsupported forms, guard zero form-index redirects, and bounds-check alternate-form rows.
- PLA's `MAX_FORM = 120` is an exclusive upper bound in the PKHeX oracle, so PokeBank's `form >= 120` rejection is correct; only the local comment describing it as the “highest form” is imprecise.
- S/V form redirects are bounds-checked before indexing; SWSH species lookup is bounded by the generated table maximum.
- `src/Pokemon/LearnsetTable.cpp` is currently an empty placeholder and no indexed repository reference to the declared lookup functions was found in this pass. This remains a completeness follow-up, not a confirmed production defect.

### Gen III/IV staged-save integration checkpoint

- Fully read the remaining Gen IV assigned-source/read-only parser interfaces and implementation plus the pending Gen III sector validator and staged Pokémon editor at live PR #92 head `084ab83547d8d2f49b9ad414e37d351d00fe069d`.
- Gen IV source opening is fail-closed: assignment resolution is read-only, actual DP/Pt/HGSS layout is detected from CRC-valid bytes rather than trusted assignment metadata, multiple valid layouts are rejected as ambiguous, and exact family/assignment mismatches require explicit reassignment.
- Gen IV raw parsing requires the exact 0x80000 save length, CRC-valid General and Storage candidates, bounded trainer/party/box/name fields, party count <= 6, current box < 18, and valid HG/SS ROM identity. No new correctness defect was confirmed in that boundary.
- The Gen III staged editor validates rotating sectors/checksums and exact save family before construction, mutates only the active logical slot, repairs touched sector checksums, proves the inactive slot byte-identical, and strictly reparses edits/Create/Clone/Release/Sparse Move operations before accepting them.
- Gen III Experience editing through the current UI is bounded to the exact level-100 maximum. Although the lower-level staged edit API would inherit `Pokemon3FRLG::setExp()` clamping for a direct oversized caller, no current UI path can supply that value, so this remains API hardening rather than a finding.

### Gen II save/edit/export checkpoint

- Fully read the strict Gen II read-only parser, staged editor, packed-move engine, inventory decoder/editor, and verified export transaction at live PR #92 head `084ab83547d8d2f49b9ad414e37d351d00fe069d`.
- The read path accepts only 32/64 KiB payloads plus recognized RTC footers, requires a unique checksum+structure-valid layout, enforces GS-vs-Crystal source-family agreement, validates party/box list markers and bodies, and rejects invalid retail trainer values.
- International staged editing is deliberately fail-closed. Pokémon edits are semantically reparsed, the current-box retail cache is synchronized, finalization repairs primary/secondary checksums and mirror regions, RTC footer bytes are required to remain identical, and the export publisher fsyncs/readbacks/hashes both original and edited images before publishing a temporary directory by rename.
- The read-only inventory decoder uses a local 26-slot Key Items bound while pret's retail constant is 25. Because the following Balls list also has to validate, a 26th key item cannot yield an available inventory, so this is recorded as cleanup/hardening rather than a standalone corruption finding.
- The packed relocation engine does expose a finalization gap: beginning a carry removes/compacts the source bytes immediately, while `finalizedBytes()` does not reject an active carry. That is recorded as AUDIT-029.

### Gen I save/edit checkpoint

- Fully read the Gen I strict read-only save parser, inventory decoder/editor, boxed Pokémon editor and packed relocation engine at live PR #92 head `084ab83547d8d2f49b9ad414e37d351d00fe069d`.
- The parser requires exact 0x8000 bytes, unique International/Japanese checksum+list validation, Red/Blue-vs-Yellow evidence agreement where available, packed-BCD money, party/list marker agreement, and box-bank/per-box checksums whenever stored banks are initialized.
- Inventory editing remains sidecar-only until finalization, validates exact classic catalog/range rules, round-trips the rewritten inventory list, repairs the main checksum and strictly reparses before returning a save image.
- Boxed Pokémon edits/Create/Clone/Remove repair and reparse after every commit and preserve untouched/reverted box bytes where possible.
- The packed relocation engine repeats the Gen II transaction-boundary problem: pickup immediately stages removals, while finalization has no active-carry guard. This is recorded as AUDIT-030.

### Core utility integrity checkpoint

- Fully read the core unaudited utility layer at live PR #92 head `6e45edd8b038d0be15272605846fa3fe2e4339d6`: owned-path construction, byte-order helpers, whole-file reads, MD5, SHA-256, UTF/string helpers, debug/event logging, CRC16 and the SC xor stream helper.
- Existing backup-copy durability behavior remains covered by AUDIT-015; `src/Utils/FileUtilities.cpp` itself was already audited, so this tranche did not duplicate that finding.
- MD5 and CRC16 implementations did not expose a new defect in this pass. Owned-path helpers reject traversal components and constrain generated backup/export components as intended in the reviewed paths.
- The SHA-256 message schedule contains signed left-shift undefined behavior on ordinary high-bit input bytes. Because that hash authenticates SC-container saves, this is recorded as AUDIT-031.

## Checkpoint — Names, legacy bridges, enums, and safety tests

- Rebased audit attention onto live PR #92 head `6e45edd8b038d0be15272605846fa3fe2e4339d6`. Its delta from the prior audited head is confined to move-picker/shared-editor UI and tests; the Names/Legacy/Enums files completed in this checkpoint are unchanged across that delta.
- Completed 20 Names logic/API files covering per-game move PP, TM/HM/TR machine mapping, held-item presence, language routing, ribbons/marks, ability/nature/type names, and Gen IV held-item structural presence. No new defect was confirmed in this logic tranche.
- Upstream PKHeX was checked through the GitHub connector for current Legends: Z-A ability bounds: `MaxAbilityID_9a` still resolves to `Ability.PoisonPuppeteer`, matching PokeBank NX's ability table through ID 310.
- Completed 13 Legacy source bridge/binding files. Source-facing Trainer mutation entry points remain no-op; staged editors own separate byte clones; exact Gen IV assignment is revalidated; binding persistence is bounded, validates temp/target images, and rolls back on promotion failures. No independent new Legacy finding was added. Gen I/II finalization follow-ups remain covered by the existing active-carry findings rather than duplicated.
- Completed all five pending Enums/SCType files. SC scalar sizes are fixed-width and current Gen IV language/version grouping behavior is explicit.
- Completed the durable Move fault matrix, journal, production integration, conversion-fidelity golden, BDSP layout-guard, PLA read-validation, legacy-binding recovery, and three major Gen IV safety suites. These tests support the existing classifications: PLA's semantic read validation is comparatively strong; BDSP still lacks an MD5-mismatch test; cross-game true Move is intentionally still locked; Gen IV staged writes prove a narrow mutation footprint plus CRC refresh and immutable source preservation.
- Ledger is authoritative at this checkpoint: 406 text paths AUDITED + 34 non-text paths INSPECTED_INDIRECTLY = 440 / 725 accounted, with 285 pending.
- PR #97 advanced to `5f19fd14628c182db135038864036b6eb1b86c49` during this pass. The one-commit delta from the prior live overlay head touches seven app-shell/game-hub files and remains explicitly pending rather than silently inherited as audited.

### PR #97 main-menu overlay catch-up

- Inspected the exact one-commit delta from `af4d2450983f837706be92a1d83c28fe308444e9` to live PR #97 head `5f19fd14628c182db135038864036b6eb1b86c49`: `AppShellModel.h`, `AppShellScreen.h/.cpp`, `SaveSelectScreen.cpp`, `UI.cpp`, and the two app-shell/game-hub contract tests.
- The delta replaces the prior developer-dashboard-style root with a dedicated Main Menu, makes Games a destination instead of the application root, routes B from Games back to Main Menu, and moves Diagnostics under Settings. Future Vault/Pokédex/Banks/Search surfaces remain explicitly non-backend previews.
- The changed navigation/touch/action flow was traced through the current source, not inferred from the model test alone.
- Historical AUDIT-032 was revalidated against live PR #97 head `3831989f7b320797068e752321ae0c36c2216f49`: the stale `AppShellSection::Collections` reference is gone, so the compile blocker is FIXED in newer sibling-lane commits. The finding remains in history because it was valid at the audited older head.

## Findings

Confirmed findings below are recorded only when supported by direct evidence from the audited tree or an explicitly identified active-overlay delta.


## Confirmed findings — repository metadata / CI

### AUDIT-001 — destructive mutable PKSE import workflow
- Severity: P1
- Confidence: CONFIRMED
- Area: CI / repository integrity
- File: `.github/workflows/import-pkse.yml`
- Evidence: `contents: write`; trigger on a push to `main` changing this workflow; fetches unpinned `pkse/master`; runs `git checkout "$upstream_sha" -- .`; commits and pushes to `main`. The workflow calls this “PKSE 1.1.3” without pinning a 1.1.3 tag/SHA.
- Why it matters: editing this workflow can replace the PokeBank NX default-branch tree with whatever PKSE master contains at run time.
- Recommended fix: retire it after the historical import, or make it manual/read-only and pin an immutable upstream revision. Never overlay upstream onto `.` and push directly to `main`.
- Owner: dedicated cleanup / MAIN.

### AUDIT-002 — no general unfiltered native PR compile gate
- Severity: P2
- Confidence: HIGH
- Area: CI / Switch-native coverage
- Files: `.github/workflows/**`
- Evidence: `host-tests.yml` is unfiltered across PRs but host-only. devkitA64 jobs are branch/path scoped or historical/manual; shared native-sensitive files outside those path filters can change without a Switch compile/link job.
- Why it matters: a host-green PR can still break libnx/device-only compilation or packaging.
- Recommended fix: add one broad PR native compile/link gate; keep specialized filtered jobs as supplemental checks.
- Owner: MAIN / CI.

### AUDIT-003 — mutable native toolchain image
- Severity: P3
- Confidence: CONFIRMED
- Area: CI reproducibility
- Files: multiple native workflows
- Evidence: builds use `devkitpro/devkita64:latest` rather than an immutable digest.
- Why it matters: identical source SHAs can build differently after the image advances.
- Recommended fix: pin a tested digest/version for candidate/release builds and update deliberately.
- Owner: MAIN / CI.

### AUDIT-004 — stale theme choices in physical bug template
- Severity: P4
- Confidence: CONFIRMED
- Area: docs/templates
- File: `.github/ISSUE_TEMPLATE/device_bug_report.md`
- Evidence: it only lists `OLED Black / Dark / Light` even though current builds support additional themes.
- Recommended fix: make the field free-form or list the current set.
- Owner: docs-only / UI-QoL.

### AUDIT-005 — historical branch-specific workflows remain tracked
- Severity: P4
- Confidence: CONFIRMED
- Area: CI maintainability
- Files: old Gen I/II/III candidate/retest/recovery workflows
- Evidence: several jobs are permanently tied to `feature/pokebank-playable` or frozen historical SHAs while live work now flows through PR #79 → #90 with #92/#97 overlays.
- Recommended fix: preserve historical evidence in docs/releases, then retire or clearly mark historical/manual workflows.
- Owner: dedicated cleanup.


### AUDIT-006 — LeakSanitizer is disabled even where comments say CI keeps it enabled
- Severity: P3
- Confidence: CONFIRMED
- Area: test/sanitizer coverage
- File: `Makefile.host.base`
- Exact symbol: `host-sanitize`
- Evidence: every sanitizer executable is launched with `ASAN_OPTIONS=detect_leaks=0`. The adjacent comment says leak checks “remain enabled in unrestricted CI builds,” but the reviewed CI workflows invoke this same target and therefore inherit `detect_leaks=0`.
- Why it matters: ASan/UBSan still provide useful coverage, but leak detection is absent and the comment overstates the gate.
- Recommended fix: either enable leak detection in Linux CI with an environment override, or correct the comment and add a separate leak-capable job if practical.
- Owner: MAIN / CI.

### AUDIT-007 — top-level README materially understates current Gen IV implementation
- Severity: P4
- Confidence: CONFIRMED
- Area: public documentation
- File: `README.md`
- Evidence: it is dated 2026-09-27 and still labels Gen IV as a read-only preview with editing/Create disabled and hardware smoke pending, while current authoritative status documents and PR #92 describe the later G4-03 accepted staged View/Edit milestone and active G4-04 Create/editor work.
- Why it matters: the public front page gives contributors/users the wrong current support boundary.
- Recommended fix: refresh only the human-facing current-status sections after the active Gen IV lane reaches the intended documentation checkpoint.
- Owner: docs-only / MAIN.

### AUDIT-008 — checked-in Visual Studio metadata is stale PKSE-era configuration
- Severity: P4
- Confidence: CONFIRMED
- Area: developer tooling / stale code metadata
- Files: `CppProperties.json`, `PKSE.sln`, `PKSE.vcxproj`, `PKSE.vcxproj.filters`
- Evidence: the project remains named `PKSE`, expects `PKSE.nro` while the Makefile target is `PokeBankNX`, carries `PKSE_VERSION="1.1.3"` in IntelliSense configuration, hard-codes a devkitA64 GCC 16.1.0 Windows include layout, and enumerates an old subset of sources that omits the newer Gen I/II/IV integration tree.
- Why it matters: the files are vestigial/IDE-facing rather than the authoritative build, but they can mislead contributors and produce incorrect IDE diagnostics/up-to-date expectations.
- Recommended fix: either regenerate/rename the VS metadata for PokeBank NX or remove it and document the supported editor setup.
- Owner: dedicated cleanup.

### AUDIT-009 — recovery metadata still describes the repository as private / old production branch
- Severity: P4
- Confidence: CONFIRMED
- Area: recovery documentation
- Files: `recovery/RECOVERY_STATE.json`, `recovery/assets_snapshot/README.md`
- Evidence: recovery state is dated 2026-09-10 and points at `feature/pokebank-playable`; the snapshot README states “The project repository is private” and instructs pushing recovery material to that historical branch. The repository is currently public and active development has moved to the #79/#90/#92/#97 hierarchy.
- Why it matters: recovery bytes themselves remain pinned and usable, but operational instructions are stale.
- Recommended fix: separate immutable historical snapshot identity from current recovery/publishing instructions.
- Owner: docs-only / dedicated cleanup.

## Completed build-system checks at this checkpoint
- The native Makefile directory wildcard covers all 148 tracked production `.cpp/.c/.s` files under `src/` and `nanovg/`.
- The 94 tracked C/C++ files outside those native source directories are tests; no production translation unit was found silently omitted from the Switch source list.
- All 32 tracked image assets were inspected by decoded binary signature and dimensions. Game-card PNG Git blob IDs match the manifest where listed; the icon is a valid 256x256 JPEG; all 14 screenshots are valid 1280x720 JPEGs; the banner is a valid 848x208 PNG.
- The two 80-MiB-class recovery tar parts are accounted for indirectly through the tracked manifest: their tree sizes match the manifest and the manifest records SHA-256 for each part and the reconstructed archive.


### AUDIT-010 — canonical engineering authority chain points to obsolete work
- Severity: P3
- Confidence: CONFIRMED
- Area: operational documentation / development safety
- Files: `docs/PROJECT_RESOURCE_INDEX.md`, `docs/RESEARCH_CURRENT_INDEX.md`, `docs/CODEX_SESSION.md`, `docs/NEXT_CODEX_PROMPT.md`, `docs/NEXT_SESSION_PLAN.md`, `docs/RECOVERY_CONTRACT.md`
- Evidence: the resource/index documents explicitly instruct normal coding sessions to treat `CURRENT_STATUS.md -> CODEX_SESSION.md -> NEXT_CODEX_PROMPT.md` as current engineering authority and still name `origin/feature/pokebank-playable` as the writable target. Yet `NEXT_CODEX_PROMPT.md` is the old Gen II PR #68 candidate prompt, `NEXT_SESSION_PLAN.md` is the older Gen III PR #77 handoff, and `RECOVERY_CONTRACT.md` still assumes the old branch/repository visibility state. Live GitHub on this audit is PR #79 -> #90 with #92 MAIN and #97 UI overlays.
- Why it matters: unlike dated reports explicitly kept as history, these files advertise themselves as the fast/current authority path. A fresh agent or contributor following repository instructions can resume an obsolete milestone or write to an obsolete branch.
- Recommended fix: keep one tiny live handoff/authority file that starts by re-fetching GitHub and records only the current integration hierarchy; move obsolete prompts to `docs/history/` or mark them historical at the top; make recovery branch-agnostic where possible.
- Owner: docs-only / MAIN.

### AUDIT-011 — Search preview vertical wrap changes columns and the test blesses it
- Severity: P3
- Confidence: CONFIRMED
- Area: UI/controller navigation / test correctness
- Files: `include/UI/OrganizationPreviewModel.h`, `tests/test_organization_preview_model.cpp` on sibling UI PR #97
- Exact symbols: `previewMoveSelection`; Search regression assertion `previewMoveSelection(OrganizationPreviewKind::Search, 6, 0, 1) == 1`
- Problem: Search renders seven filters in a two-column grid. Vertical movement adds/subtracts the column count and then wraps the flat index modulo seven. Because seven is not divisible by two, vertical wrapping changes columns: index 6 (bottom-left) + Down becomes index 1 (top-right); Up from index 0 similarly lands on index 5 (bottom-right).
- Why it matters: controller movement no longer matches the visible two-column geometry at the incomplete final row, contrary to the stated spatial-navigation contract. The existing test currently codifies the defect instead of detecting it.
- Evidence: the model computes `next = current + dy * cols` and repeatedly adds/subtracts `count`; the Search model has `count=7`, `cols=2`. The test explicitly expects the 6 -> 1 transition.
- Current tests: host test covers and accepts the incorrect transition.
- Missing tests: column-preserving wrap for incomplete rows, including Search 0 + Up and 6 + Down.
- Recommended fix: perform row/column navigation geometrically. On vertical wrap, preserve the current column and choose the nearest valid row entry; for the one-item final Search row, bottom-left should wrap to top-left. Update the regression expectations accordingly.
- Risk of fix: low; UI-model/controller behavior only.
- Owner: UI/QoL.

### AUDIT-012 — Settings persistence truncates in place and ignores write/close failure
- Severity: P3
- Confidence: CONFIRMED
- Area: app configuration persistence
- Files: `src/Utils/Settings.cpp`
- Exact lines/symbols: `saveSettings()`, especially the direct `fopen(settingsPath().c_str(), "w")`, unchecked `fprintf` calls, and unchecked `fclose`.
- Problem: the authoritative settings file is truncated before the replacement content is known to be complete, and write/close failures are not observed.
- Why it matters: interruption, ENOSPC, or a short/failed write can leave an empty/partial `settings.cfg` while the caller receives no failure signal. This does not corrupt Pokémon/save data, so it is not a P1/P2 issue.
- Evidence: unlike Bank/workspace persistence, `saveSettings()` does not use a temp/validate/promote transaction and returns `void`; only the initial `fopen` failure is logged.
- Current tests: no durable/partial-write settings persistence test found in the audited tranche.
- Missing tests: injected write failure, close failure, interrupted/truncated file recovery, and successful round-trip of all current keys.
- Recommended fix: serialize settings to memory and use the app-owned durable replacement primitive (or an equivalent narrow config transaction), validate/read back the promoted text, and return/report failure.
- Risk of fix: low.
- Owner: dedicated cleanup / MAIN storage utility owner.

### AUDIT-013 — Legacy Bank migration skips checksum validation used by normal Bank load
- Severity: P2
- Confidence: CONFIRMED
- Area: Bank migration / Pokémon data integrity
- Files: `src/Trainer/Bank.cpp`, `tests/test_bank_recovery_contract.cpp`, `tests/test_bank_format_policy.cpp`
- Exact lines/symbols: `Bank::migrateLegacyBanks()` versus the checksum gate in `Bank::load()`.
- Problem: normal unified Bank loading requires a non-empty Pokémon whose stored checksum equals `calculateChecksum()`. The legacy per-group migration path accepts any record for which `makePokemon(...)` succeeds and `speciesID() != 0`, then places it into the in-memory Bank without the checksum test.
- Why it matters: a damaged legacy Bank record that happens to decode to a nonzero species can be surfaced as a real migrated Pokémon. Later persistence is likely to fail closed when full Bank validation runs, but the corrupted record has already entered the active in-memory model and can affect browsing/export workflows.
- Evidence: the two load paths use different acceptance predicates in the same implementation; the normal path explicitly calls the checksum “the decisive test,” while migration omits it.
- Current tests: the reviewed Bank tests check unreadable-file preservation and box-count fail-closed behavior using source-string assertions; neither constructs a corrupted legacy record and exercises migration.
- Missing tests: corrupted-checksum legacy record is skipped, valid record migrates, mixed valid/corrupt legacy records preserve only valid entries, and migration never mutates the legacy source file.
- Recommended fix: apply the same nonempty + checksum validation predicate before `placeNext`; count/log rejected corrupt migration records separately from capacity overflow.
- Risk of fix: low to medium because it changes one-time migration acceptance; preserve original legacy files as recovery evidence.
- Owner: MAIN.

### AUDIT-014 — Session-wide source read-only gate disables app-owned Bank mutation
- Severity: P2
- Confidence: CONFIRMED
- Area: source/destination capability separation / Storage / Bank UI
- Files: `include/UI/TrainerViewScreenBase.h`, `src/UI/TrainerViewScreenBase.inc`, `include/UI/ExactFormatEditorProvider.h`, `tests/test_source_mutation_policy.cpp`
- Exact symbols: `sourceReadOnly()`, `requireMutableWorkspace()`, `renameBankBox()`, `openStorageEditor()`, `openActionSheetTargetDetails()`, `pickupSingle()`, `pickupMulti()`, `grabSelection()`, `putDownBlock()`, `sortStorageBox()`.
- Problem: mutation permission is decided from the session's source kind rather than from the actual mutation target. When a RetroArch/DraStic/melonDS/manual or installed source is read-only, the same gate also blocks operations whose target is PokeBank-owned `Bank` storage.
- Why it matters: the safety model explicitly distinguishes immutable source material from mutable `AppOwnedStorage`. Today a read-only source session incorrectly makes Bank Pokémon/Bank boxes read-only too, preventing Bank edit, rename, sort and Bank-only move operations even though those operations do not write the source.
- Evidence: `requireMutableWorkspace()` returns false solely from `sourceReadOnly()`. `renameBankBox()` calls that helper; `openStorageEditor()` calls it before checking `pane == 1`; `openActionSheetTargetDetails()` forces `readOnly = readOnly || sourceReadOnly()` even for a Bank target; Bank carry/sort paths use the same helper. `SourceMutationPolicy` itself correctly permits `AppOwnedStorage`, but the UI does not ask about the destination kind.
- Current tests: `test_source_mutation_policy.cpp` loops Party, SaveBox **and Bank** using one read-only action-sheet capability and asserts Edit is unavailable for all three, so the current test encodes the conflation rather than catching it.
- Missing tests: immutable source + mutable Bank target; Bank rename/edit/sort during RetroArch/ExternalLegacy browsing; source-box mutation remains blocked; cross-store source-retiring Move remains blocked; copy/import into Bank must not imply source retirement.
- Recommended fix: replace the session-wide mutation gate with a target-aware capability decision. Save/Party/SaveBox targets inherit the source/workspace capability; Bank targets use `SourceKind::AppOwnedStorage`. Keep cross-store True Move source retirement behind its existing transaction/evidence gates and do not turn this cleanup into emulator source writing.
- Risk of fix: medium because Storage input currently shares helpers across both panes; regression tests must prove Bank mutability without weakening source immutability.
- Owner: MAIN / storage UI architecture.

### AUDIT-015 — Failed backup creation can leave a partial folder surfaced as a backup
- Severity: P2
- Confidence: CONFIRMED
- Area: backup creation / filesystem durability / backup picker
- Files: `src/Utils/FileUtilities.cpp`, `src/UI/TrainerViewScreenBase.inc`, `src/UI/BackupSelectionScreen.cpp`, `tests/test_backup_namespace_contract.cpp`, `tests/test_backup_workspace_durability.cpp`
- Exact symbols: `copyDirectoryRecursive()`, `copyDirectory()`, `backupSaveData()`, `TrainerViewScreen::createNamedBackupDir()`, `listBackupDirectories()`, `BackupSelectionScreen::loadBackups()`.
- Problem: backup creation writes directly into the final destination directory. If any recursive file copy fails, the function returns failure but leaves the partially populated destination directory behind. User-named and timestamped folders are then enumerated as ordinary editable backups solely because they are directories under the scoped game root.
- Why it matters: low-space, read error, short write, or close failure can create a folder the UI later presents as a real backup even though the operation that created it explicitly failed. For most game families the open preflight is not a complete backup-set validator, so a partial workspace can progress farther than it should.
- Evidence: `copyDirectoryRecursive()` opens destination files directly with `"wb"`, records `overallSuccess=false` on failures, and never rolls back already-created files/directories; `copyDirectory()` creates the final directory before copying. Both `backupSaveData()` and `createNamedBackupDir()` return failure without quarantining/removing the failed destination. `listBackupDirectories()` intentionally treats every ordinary subdirectory (except hidden Working by default) as user-facing, and `BackupSelectionScreen::loadBackups()` does not require a completion marker or validate the backup before listing it.
- Current tests: namespace tests prove scoped paths and durability tests prove later workspace *writes* use `DurableFile`, but neither injects backup-copy failure or verifies that a failed seed is absent/quarantined from the picker.
- Missing tests: ENOSPC/short-write/read/close failure while copying; timestamped and named backup failure cleanup/quarantine; incomplete backup excluded from picker; Working-copy recovery semantics; multi-file backup completeness checks.
- Recommended fix: create backups under a unique temporary/incomplete directory, copy + close/readback/validate the required game file set there, then promote/rename to the final visible backup name only after success. On failure retain evidence under an explicitly non-browsable failed/incomplete name or remove it only when safe. The picker should ignore transaction temp/failed markers and/or require a completed manifest.
- Risk of fix: medium because installed-title backups may contain multiple files and directory promotion behavior must be verified on Switch SD storage.
- Owner: MAIN / backup-storage transaction layer.

### AUDIT-016 — RetroArch launch matching is basename-only and first-match wins
- Severity: P3
- Confidence: CONFIRMED
- Area: Game Hub / emulator launch routing
- Files: `src/UI/GameLauncher.cpp`, `include/UI/GameLaunchModel.h`, `tests/test_game_launch_model.cpp` on sibling UI PR #97
- Exact symbols: `normalizedLaunchStem()`, `resolveRetroArch()`.
- Problem: the resolver reduces the validated save path and each playlist content path to an alphanumeric basename stem, then returns the first playlist entry whose stem matches. Directory, extension, core, exact game identity and content hash are not part of the match key.
- Why it matters: two ROM/content files in different directories can legitimately share the same basename. In that case ZR Launch can start the wrong game/content even though the selected save itself was validated correctly.
- Evidence: `resolveRetroArch` compares only `normalizedLaunchStem(content) != wanted`; as soon as a regular file plus usable core is found it returns `Ready` and exits the playlist scan. No ambiguity detection exists.
- Current tests: normalization and provider classification are tested, but duplicate-stem/ambiguous-playlist behavior is not.
- Missing tests: two valid playlist entries with the same normalized stem but different paths; ambiguity must not silently select one.
- Recommended fix: gather all viable matches first. If exactly one remains, launch it. If multiple remain, require an explicit stored content binding or chooser keyed by exact source/game identity; never pick by directory iteration order.
- Risk of fix: low; launch resolution only, no save mutation.
- Owner: sibling UI/QoL lane.

### AUDIT-017 — FRLG mutable workspace selects rotating slot before checksum validation
- Severity: P2
- Confidence: CONFIRMED
- Area: FireRed/LeafGreen mutable backup workspace / save integrity
- Files: `src/Trainer/Trainer3FRLG.cpp`, `src/Save/GetSaveFileContents.cpp`; contrast with `tests/test_gen3_staged_pokemon_editor.cpp`
- Exact symbols: `Trainer3FRLG::selectActiveSlot()`, `validateTrainerSaveForOpen()`, `saveTrainerInfoFRLG()`, `Trainer3FRLG::finalizeChecksums()`.
- Problem: the legacy mutable FRLG trainer chooses the active rotating save slot from save counters and sector-id presence only. It does not verify the selected slot's 14 sector checksums before parsing trainer/party/boxes. The generic pre-open validation path special-cases BDSP and PLA but currently returns success for FRLG without running a checksum/slot-recovery gate.
- Why it matters: if the higher-counter slot is corrupt while the older slot is still valid, this path can parse and present the corrupt slot instead of recovering from the valid one. A later save calls `finalizeChecksums()` before durable workspace validation, which rewrites all active-slot checksums; corruption can therefore be normalized into a newly checksum-valid workspace generation rather than refused.
- Evidence: `selectActiveSlot()` compares counters, builds an id table, and sets `m_valid` from all ids being present; there is no checksum predicate. `validateFRLGWorkspace()` detects checksum disagreement only on the bytes it is given, but `saveTrainerInfoFRLG()` calls `finalizeChecksums()` before that validator sees the candidate. The newer staged Gen III implementation already tests the intended policy: one corrupt rotating slot recovers from the other, while two corrupt slots fail closed.
- Current tests: staged Gen III tests cover checksum-aware rotating-slot recovery, but the `Trainer3FRLG` mutable workspace path does not share that implementation and has no equivalent regression in the reviewed tests.
- Missing tests: corrupt newest/valid older slot must select the older valid slot; both invalid slots must refuse open; no mutation may occur merely to make an invalid selected slot checksum-valid.
- Recommended fix: reuse the checksum-aware Gen III slot validator/selection policy (or one shared lower-level implementation) before constructing mutable FRLG state. Make FRLG participate in `validateTrainerSaveForOpen()` and keep the original corrupted candidate only as recovery evidence, never auto-repair it on ordinary save.
- Risk of fix: medium because active-slot selection changes; validate against real FR/LG saves and RTC-footer variants.
- Owner: MAIN / save-integrity lane.

### AUDIT-018 — LGPE mutable workspace rewrites CRCs before validating pre-existing block integrity
- Severity: P2
- Confidence: CONFIRMED
- Area: Let's Go Pikachu/Eevee mutable backup workspace / save integrity
- Files: `src/Trainer/Trainer7LGPE.cpp`, `src/Save/GetSaveFileContents.cpp`
- Exact symbols: `createBlocksFromSaveData7LGPE()`, `writeBlocksToSaveData7LGPE()`, `validateTrainerSaveForOpen()`, `saveTrainerInfoLetsGo()`, `validateLGPEWorkspace()`.
- Problem: the LGPE read/open path extracts fixed-offset blocks from a correctly-sized file but does not verify the existing BEEF footer CRC for those blocks before constructing mutable trainer/storage state. The generic pre-open gate does not currently perform LGPE integrity validation.
- Why it matters: a workspace with a corrupted block can still be parsed and edited. During save, `writeBlocksToSaveData7LGPE()` patches the selected blocks and recomputes their CRCs before `persistWorkspaceFile()` invokes `validateLGPEWorkspace()`. The validator therefore sees a newly rechecksummed candidate, not proof that the source workspace was valid before mutation. Pre-existing corruption can be normalized into a checksum-valid new generation.
- Evidence: `createBlocksFromSaveData7LGPE()` gates only on total file size and fixed offset/length bounds. It does not compare block bytes against footer CRCs. `writeBlocksToSaveData7LGPE()` unconditionally computes fresh CRC-16/ARC values for the extracted blocks. `validateTrainerSaveForOpen()` special-cases BDSP and PLA but otherwise returns success, including LGPE.
- Current tests: no dedicated LGPE integrity/CRC-open regression was found by filename in the current test tree; existing save validation checks the post-serialization candidate rather than the pre-edit source state.
- Missing tests: corrupt one covered LGPE block while leaving the old footer CRC unchanged and require open to fail; verify an untouched valid fixture passes; prove save never repairs an invalid source merely as a side effect of ordinary editing.
- Recommended fix: add a read-only LGPE integrity validator that verifies every consumed Beluga block against its existing BEEF footer CRC before constructing `Trainer7LGPE`; invoke it from `validateTrainerSaveForOpen()` and from any direct LGPE load path. Keep rechecksum-on-write only after a valid source has been established.
- Risk of fix: low to medium; validate checksum geometry against real Pikachu/Eevee saves before gating hardware opens.
- Owner: MAIN / save-integrity lane.

### AUDIT-019 — modern encrypted blank slots are parsed as live species-0 objects
- Severity: P3
- Confidence: CONFIRMED
- Area: SWSH / PLA / SV / Z-A party and box model correctness
- Files: `src/Trainer/Trainer8SWSH.cpp`, `src/Trainer/Trainer8LA.cpp`, `src/Trainer/Trainer9SV.cpp`, `src/Trainer/Trainer9LZA.cpp`, corresponding Trainer headers, `src/UI/TrainerViewScreenBase.inc`
- Exact symbols: each game's `parsePartyBlock()` / `parseBoxBlock()`, derived `getPartySize()`, direct Boxes-mode Y/swap occupancy check.
- Problem: these parsers decide emptiness from whether the RAW encrypted slot bytes are all zero. Their own write paths document the opposite native fact: a legitimate empty slot is normally a non-zero encrypted blank which decrypts to species 0. The parser therefore constructs live Pokémon objects for native empty slots instead of representing them as empty/null.
- Why it matters: party vectors can contain species-0 ghost entries and `getPartySize()` returns the vector size, not the number of real Pokémon. Most modern UI paths defensively test `speciesID()!=0`, but not all do: direct Boxes-mode Y/swap currently computes `cursorOccupied` from pointer non-nullness alone, so a visually empty ghost slot can enter the grab/swap flow as though occupied.
- Evidence: SWSH and PLA writer comments explicitly state that native empty party slots are non-zero encrypted blanks and note that the old parser inflates `party.size()`; SV/Z-A use the same raw-all-zero parse test and encrypted-blank writer model. The shared UI already contains multiple comments/workarounds naming “species-0 ghost” slots.
- Current tests: no dedicated modern Trainer party/box blank-round-trip test was found in the current test tree. PLA read-validation tests validate structural Pokémon records but do not assert Trainer representation of encrypted empty slots.
- Missing tests: a native party with two real Pokémon plus four encrypted blanks must parse to two real party entries; native encrypted empty box slots must become null/empty model cells; Y on a visually empty box cell must remain a no-op; round-trip must preserve valid native blank bytes/count semantics.
- Recommended fix: decrypt/validate each slot before deciding occupancy and store only species-nonzero entities in the logical model (or explicitly preserve positional empties as null). Where the save has an authoritative party count, parse and validate it rather than deriving party size from raw slot nonzeroness. Keep species-aware UI occupancy checks as defense in depth.
- Risk of fix: medium because party index/count behavior is shared by several screens; validate on real saves for all four families.
- Owner: MAIN / modern save-model lane.

### AUDIT-020 — SV/Z-A MyStatus size guard permits an out-of-bounds gender read
- Severity: P2
- Confidence: CONFIRMED
- Area: malformed-save safety / Gen IX trainer parsing
- Files: `src/Trainer/Trainer9SV.cpp`, `src/Trainer/Trainer9LZA.cpp`
- Exact symbol: `parseMyStatusBlock()`.
- Problem: both functions reject only when `block.data.size() < 4`, read ID32, conditionally guard the OT-name field, and then unconditionally read `block.data[0x05]` for trainer gender. A 4- or 5-byte MyStatus object therefore passes the initial guard and indexes past the vector.
- Why it matters: SC container validity only proves the block stream/hash is self-consistent; Object block lengths are data. A hash-valid but malformed/unsupported save can reach this read and trigger undefined behavior instead of failing closed.
- Evidence: the generic SC parser accepts arbitrary object lengths that fit inside the authenticated container. SV/Z-A have no game-specific pre-open layout validator comparable to PLA's `validatePLAReadLayout()`.
- Missing tests: authenticated SC fixtures with MyStatus sizes 4 and 5 must be rejected cleanly without constructing a Trainer; size 6 must not read beyond bounds; full expected native MyStatus geometry should pass.
- Recommended fix: require the complete minimum region actually consumed by the parser before any field access (at minimum through gender byte, preferably the full supported MyStatus layout), and make that requirement part of a pre-open game-specific layout validator.
- Risk of fix: low.
- Owner: MAIN / save-integrity lane.

### AUDIT-021 — SWSH/SV/Z-A authenticate the SC container but do not validate required game layout
- Severity: P2
- Confidence: CONFIRMED
- Area: modern save structural validation / durable writeback
- Files: `src/Save/GetSaveFileContents.cpp`, `src/Save/Block.cpp`, `src/Trainer/Trainer8SWSH.cpp`, `src/Trainer/Trainer9SV.cpp`, `src/Trainer/Trainer9LZA.cpp`
- Exact symbols: `validateTrainerSaveForOpen()`, `validateSCWorkspace()`, `parseAllBlocks()`, the modern Trainer constructors and fixed-role block writers.
- Problem: unlike PLA, Sword/Shield, Scarlet/Violet and Z-A have no per-game semantic layout gate. Their open preflight returns success without validating the SC file, and their post-write validator verifies only the SC hash plus complete syntactic block parsing. It does not require unique keys or required block types/sizes. The generic parser also permits duplicate keys.
- Why it matters: an ordinary random byte corruption is caught by the SC hash, but a hash-valid malformed/unsupported image (for example from another tool, a bad migration, or future layout) can be interpreted ambiguously. Duplicate Party/MyStatus keys are parsed more than once while several writers update only the first matching key. Short Party/Box blocks in these classes can be automatically resized, populated and then re-encrypted with a fresh valid hash, converting an unsupported input geometry into a newly authenticated PokeBank output rather than refusing it.
- Evidence: `validateSCWorkspace()` returns true immediately after `Encryption::tryDecrypt()`; `parseAllBlocks()` has no duplicate-key set; PLA's validator explicitly rejects duplicate keys and enforces required sizes, demonstrating the intended stronger boundary. SWSH/SV/Z-A Party/Box writers call `resize()` when their fixed-role blocks are short.
- Missing tests: duplicate required keys; missing required blocks; short Party/Box/MyStatus blocks with a valid outer hash; unsupported block type for a required key; all must fail before Trainer construction and before any serializer mutation.
- Recommended fix: add game-specific SWSH/SV/Z-A read/layout validators modeled on `validatePLAReadLayout()`: unique keys, exact/minimum supported type and size for every consumed block, Pokémon checksum/basic-domain validation for occupied slots, and fail-closed open/write validation. Do not use resize as recovery for a malformed required native block.
- Risk of fix: medium; geometry must be pinned to real saves/revisions so valid DLC/version differences are not rejected.
- Owner: MAIN / save-integrity lane.

### AUDIT-022 — BDSP pre-open validation ignores its stored whole-file MD5
- Severity: P3
- Confidence: CONFIRMED
- Area: BDSP flat-save integrity / open gate
- Files: `include/Save/BDSPReadValidation.h`, `src/Save/GetSaveFileContents.cpp`, `src/Trainer/Trainer8BDSP.cpp`, `tests/test_bdsp_layout_guard.cpp`
- Exact symbols: `hasMinimumLayout()`, `validateTrainerSaveForOpen()`, `Trainer8BDSP::Trainer8BDSP()`, `Trainer8BDSP::recomputeHash()`.
- Problem: BDSP is documented and implemented as a flat blob guarded by a 16-byte whole-file MD5. The open gate and Trainer constructor verify only that the file extends through that hash field; neither compares the stored digest with a computed digest before parsing trainer/party/box data.
- Why it matters: a corrupt but correctly-sized `SaveData.bin` can be surfaced as editable in-memory state. This is currently contained because the normal BDSP save path is hard-blocked until a recoverable multi-file journal exists, so this is P3 rather than a current write-corruption P2. Once BDSP writeback is enabled, the missing read-integrity gate must be fixed first or `recomputeHash()` could certify already-corrupt input.
- Current tests: `test_bdsp_layout_guard.cpp` covers undersized/exact-minimum/oversized length behavior and verifies the pre-open size guard, but not MD5 mismatch.
- Missing tests: flip one covered byte in a valid BDSP fixture without updating its stored digest and require open refusal; confirm a correct digest passes; confirm the validator itself does not mutate the candidate while checking.
- Recommended fix: implement a read-only whole-file MD5 verifier using the same zero-the-hash-field convention as `recomputeHash()`, call it from the BDSP pre-open gate and Trainer validity boundary, and add fixture tests before enabling multi-file writeback.
- Risk of fix: low once the exact BDSP digest convention is fixture-verified.
- Owner: MAIN / BDSP integrity lane.

### AUDIT-023 — LGPE durable validator rejects the authentic 1 MiB save image
- Severity: P2
- Confidence: CONFIRMED
- Area: Let's Go backup workspace save / durable promotion
- Files: `include/Save/GetSaveFileContents.h`, `src/Save/GetSaveFileContents.cpp`, `include/Trainer/Trainer7LGPE.h`
- Exact symbols: `buildWorkspaceImage()`, `saveTrainerInfoLetsGo()`, `validateWorkspaceImage()`, `validateLGPEWorkspace()`, `readTrainerInfoLetsGo()`.
- Problem: the read/write code correctly treats LGPE `savedata.bin` as a full file containing an active `0xB8800` region plus trailing data, but the durable validator requires `bytes.size() == SAVE_SIZE7_LGPE`, where `SAVE_SIZE7_LGPE` is `0xB8800`. Both the generic workspace builder and the direct LGPE save function preserve the FULL input file and pass that full image to `validateLGPEWorkspace()`.
- Why it matters: a normal authentic 1 MiB LGPE save image fails validation before durable promotion, so edits can be prepared in memory but cannot be successfully saved through either current LGPE write route. This is fail-closed (no data loss), but it blocks a supported game's core save workflow.
- External oracle: current PKHeX `SaveUtil.SIZE_G7GG` is `0x100000`; `IsG7LGPE()` requires that full length and then slices the first `0xB8800` bytes for the active save/footer checks. This matches PokeBank NX's own `readTrainerInfoLetsGo()` comments and full-file-preservation write design.
- Evidence in PokeBank NX: `buildWorkspaceImage()` reads `savedata.bin` and assigns all `size` bytes into `out.bytes`; `saveTrainerInfoLetsGo()` likewise keeps all `fileSize` bytes; both eventually call `validateLGPEWorkspace()`, whose first condition rejects any size other than `0xB8800`.
- Missing tests: full `0x100000` LGPE fixture must validate after block patching while preserving bytes `0xB8800..0xFFFFF` exactly; active-region checksum corruption must fail; define explicitly whether cropped `0xB8800` images are supported or rejected rather than conflating active-region size with physical-file size.
- Recommended fix: introduce separate constants for physical LGPE file size (`0x100000`) and active Beluga region size (`0xB8800`). Validate the physical workspace shape, slice only the active region for block/CRC verification, and preserve the trailing region byte-for-byte during build/persist.
- Risk of fix: low to medium; hardware-test both Pikachu and Eevee backups and verify untouched trailing bytes remain identical.
- Owner: MAIN / LGPE save-integrity lane.

### AUDIT-024 — Gen IX inventory decoder leaves persisted flags indeterminate
- Severity: P2
- Confidence: CONFIRMED
- Area: Scarlet/Violet + Legends: Z-A inventory parsing/writeback
- Files: `include/Trainer/Inventory9SV.h`, `include/Trainer/Inventory9LZA.h`, `src/Trainer/Trainer9SV.cpp`, `src/Trainer/Trainer9LZA.cpp`, `include/Trainer/Trainer.h`
- Exact symbols: `InventoryItem9SV::fromBytes()`, `InventoryItem9LZA::fromBytes()`, both `parseItemBlock()` and `updateItemBlock()`.
- Problem: `InventoryItem9SV item;` / `InventoryItem9LZA item;` default-initialize an aggregate whose inherited `InventoryItem::isNew` and `isFavorite` booleans have no default member initializers. `fromBytes()` decodes the native `flags` word but never maps it into those inherited booleans. The derived object is then copied into `Trainer::items`, whose element type is the base `InventoryItem`, preserving the indeterminate boolean state while discarding the decoded `flags` member.
- Why it matters: both Gen IX writers later execute `if (item.isNew) ... |= 0x01` on that base item. Merely loading and saving an existing item can therefore branch on an indeterminate value and spuriously set the native NEW flag. This is undefined behavior feeding persistent save bytes, even when the user did not request a flag edit.
- Additional decoder hazard: the same functions assemble 32-bit pouch/count/flags fields with expressions such as `data[3] << 24`. Because each `uint8_t` is integer-promoted to signed `int` first, high-byte values that cannot be represented after the shift invoke undefined behavior. Use unsigned 32-bit widening before shifting or the project's little-endian readers instead.
- Evidence: `Trainer9SV::parseItemBlock()` and `Trainer9LZA::parseItemBlock()` push the derived decoder result directly into `std::vector<InventoryItem>`; their write paths later branch on `item.isNew`. Existing raw flags remain in the underlying block only by accident because the writer ORs rather than clears; the in-memory flag model itself is not initialized from those bytes.
- Current tests: no Gen IX inventory record/round-trip test was found among the inventory/item/bag/pouch test filenames; current inventory tests are classic-generation or shared UI-focused.
- Missing tests: native records with NEW clear/set and FAVORITE clear/set must decode deterministically; parse→write with no edits must be byte-identical; repeated runs under UBSan/ASan-compatible host builds must not depend on stack contents; high-bit 32-bit fields must decode using defined unsigned operations.
- Recommended fix: value-initialize the record (`InventoryItem9SV item{}` / `InventoryItem9LZA item{}`), decode flags into `isNew` and `isFavorite` explicitly, and replace manual signed-shift assembly with `readUInt32LittleEndian()` or `static_cast<uint32_t>(data[n]) << shift`. Add exact byte round-trip tests for both games.
- Risk of fix: low; the corrected decoder should make no-byte-change round trips deterministic.
- Owner: MAIN / Gen IX inventory lane.

### AUDIT-025 — defaulted Pokémon move operations duplicate raw-buffer ownership
- Severity: P3
- Confidence: CONFIRMED
- Area: Pokémon entity ownership / C++ memory safety
- Files: `include/Pokemon/Pokemon.h`; `Pokemon1ReadOnly.h`, `Pokemon2ReadOnly.h`, `Pokemon3FRLG.h`; directly verified modern declarations in `Pokemon7LGPE.h`, `Pokemon8SWSH.h`, `Pokemon8BDSP.h`, `Pokemon8LA.h`, `Pokemon9SV.h`, `Pokemon9LZA.h`
- Exact symbols: `Pokemon(Pokemon&&) noexcept = default`, `Pokemon::operator=(Pokemon&&) noexcept = default`, the explicit defaulted moves in Gen III and modern concrete formats, and the implicitly generated Gen I/II wrapper moves.
- Problem: `Pokemon` owns `buffer` as a raw `std::byte*` and its destructor executes `delete[] buffer`. A compiler-generated move of a raw pointer copies the pointer value; it does not null the source. The defaulted move also copies the `std::span`, so after a move both source and destination refer to the same allocation.
- Why it matters: moving any of these concrete Pokémon objects by value leaves the destination pointing at storage still owned by the moved-from object. Destruction of the source creates a dangling destination; destruction of both produces a double free. Move assignment additionally risks leaking/overwriting the destination's prior allocation before the later double-free condition.
- Reachability: current audited storage paths overwhelmingly traffic in `std::unique_ptr<Pokemon>`, so no present production crash path was confirmed in this tranche. However the move constructors/assignments are public and explicitly documented as supported, making this a real latent ownership defect rather than dead code.
- Evidence: the base class destructor owns/frees `buffer`; its move operations are defaulted. `Pokemon3FRLG` and LGPE/SWSH/BDSP/PLA/SV/Z-A explicitly default their derived moves. Gen I/II wrappers own the same base buffer and declare no copy/move/destructor that would supply safe ownership transfer, so their implicitly generated move path inherits the broken base semantics.
- Missing tests: move-construct and move-assign each concrete format under ASan; destroy the moved-from object before reading the moved-to object; verify data remains valid and exactly one owner frees the allocation.
- Recommended fix: implement custom base move construction/assignment that transfers `buffer`, rebuilds `data` to the transferred allocation, copies `dataSize`, and clears the source's pointer/span/size. Alternatively replace the raw allocation with `std::unique_ptr<std::byte[]>` and still ensure the span is rebound after moves.
- Risk of fix: low to medium; move assignment must safely release any existing destination allocation and rebind spans.
- Owner: MAIN / Pokémon core lane.

### AUDIT-026 — Gen III and modern entity constructors do not fully enforce/normalize native record length
- Severity: P3
- Confidence: CONFIRMED
- Area: Gen III + modern Pokémon entity parsing / memory safety
- Files: `include/Pokemon/Pokemon3FRLG.h`; `include/Pokemon/Pokemon7LGPE.h`, `include/Pokemon/Pokemon8SWSH.h`, `include/Pokemon/Pokemon8BDSP.h`, `include/Pokemon/Pokemon8LA.h`, `include/Pokemon/Pokemon9SV.h`, `include/Pokemon/Pokemon9LZA.h`; corresponding entity encryption implementations; shared `src/Encryption/Encryption.cpp`; stat implementations for SWSH/BDSP/SV/Z-A.
- Exact symbols: each concrete span constructor, each `decryptArray*` / `shuffleArray*`, `cryptPokemon()`, and SWSH/BDSP/SV/Z-A party-stat getters/recalculation methods.
- Problem: the modern entity constructors accept an arbitrary `std::span<const std::byte>` and their decryptors immediately read the first four bytes and then operate on a fixed header + four-block region without verifying that the supplied span contains it. `cryptPokemon()` likewise forms a fixed `subspan(8, blockSize * blockCount)`. Gen III's decryptor is safer and returns a copied short buffer when input is below 0x50, but `Pokemon3FRLG` still binds that short result as a live entity and its ordinary fixed-offset accessors then assume the native header/data region exists. Truncated entity input can therefore reach out-of-bounds access before checksum validation rejects it.
- Documented stored-size subcase: SWSH, BDSP, SV and Z-A explicitly document 0x148 stored records as valid constructor input and retain `dataSize = raw.size()`, but their `level()`, battle-stat accessors, `setLevel()`, `setExp()`, and `recalculateStats()` use offsets 0x148 through 0x154. A valid 0x148 stored record therefore creates an object whose advertised API reads/writes beyond its allocation.
- Current reachability: audited live Trainer box paths for SWSH/BDSP/SV/Z-A use party-sized entity payloads, and unified Bank `recordSizeFor()` also uses party size for every modern group. No current save/Bank path was found constructing those four classes from 0x148 stored entities. This limits present severity, but the constructors publicly claim that input is supported.
- Safer existing pattern: `Pokemon8LA` accepts a true stored-size 0x168 PA8 from PLA boxes, then normalizes it into a zero-initialized 0x178 party-sized owned buffer before exposing party-stat APIs. Current PKHeX PK9/PA9 performs the same stored→party padding in `DecryptParty()`.
- Missing tests: Gen III and every modern entity constructor should reject spans shorter than the native stored size without exposing a usable entity; exact stored-size and exact party-size records should construct deterministically; SWSH/BDSP/SV/Z-A stored-size objects should support level/stat getters and stat-affecting edits under ASan without an out-of-bounds access; unexpected intermediate/oversized lengths should have an explicit policy.
- Recommended fix: make entity length an enforced boundary before decryption. Accept only documented native stored/party sizes (or an explicitly justified superset), and normalize valid stored entities to an owned party-sized buffer before any party-stat API is exposed. Return failure/invalid state for malformed lengths instead of relying on callers to be perfect.
- Risk of fix: medium because clone/Bank/encryption code currently preserves `dataSize`; normalize carefully so box serialization still writes only the intended native prefix where the save format genuinely stores stored-size entities.
- Owner: MAIN / Pokémon core + encryption lane.

### AUDIT-027 — LGPE Meltan/Melmetal base-stat rows are unreachable and edits rewrite party stats from base 0
- Severity: P2
- Confidence: CONFIRMED
- Area: Let's Go Pokémon stats / entity editing
- Files: `include/Pokemon/BaseStatsGen7.h`, `src/Pokemon/BaseStatsGen7.cpp`, `src/Pokemon/Pokemon7LGPE.cpp`, `include/Pokemon/Pokemon7LGPE.h`
- Exact symbols: `BASE_STATS_TABLE_GEN7`, `BASE_STATS_COUNT_GEN7`, `getBaseStatsGen7()`, `Pokemon7LGPE::baseHP/baseATK/baseDEF/baseSPE/baseSPA/baseSPD()`, `computeStat()`, `recalculateStats()`.
- Problem: the LGPE base-stat table is mostly a dense 0..151 array, then appends Meltan (#808) and Melmetal (#809) as the final two records. The getter does not search by the record's `id`; after regional-form handling it rejects any `speciesId >= BASE_STATS_COUNT_GEN7` and otherwise indexes `BASE_STATS_TABLE_GEN7[speciesId]`. The array count is therefore only the number of rows, not the highest supported species id. Species 808/809 always return the all-zero fallback and the appended rows are unreachable.
- Why it matters: LGPE stat display and mutation share this lookup. `Pokemon7LGPE::computeStat()` derives all six battle stats from these base stats, and `recalculateStats()` writes Level/current HP/max HP/ATK/DEF/SPE/SPA/SPD plus Combat Power into the PB7 party tail that the game reads. Species, form, level, EXP, nature, IV and AV edits invoke this recalculation. Editing Meltan or Melmetal can therefore persist a structurally valid PB7 whose party stat tail was calculated from base stat 0.
- Encoding confirmation: current PKHeX `PB7.Species` reads/writes the u16 at `0x08` directly and its personal lookup indexes `PersonalTable.GG.GetFormEntry(Species, Form)`; there is no LGPE-local species numbering that maps Meltan/Melmetal to dense rows 152/153.
- Tests: no Meltan/Melmetal or LGPE base-stat regression test was found by filename in the current test tree.
- Missing tests: `getBaseStatsGen7(808,0)` and `(809,0)` must return the native Meltan/Melmetal rows; editing Level/IV/AV on fixture PB7s must produce the same party stats/CP as PKHeX and must not collapse them toward base-0 results.
- Recommended fix: stop treating this sparse table as a dense dex-indexed array. Either special-case/search sparse IDs, split the 0..151 dense table from Meltan/Melmetal, or generate a true id-indexed table large enough for 809. Add explicit compile-time/runtime coverage for 808/809.
- Risk of fix: low; Pikachu/Eevee/Alolan dense lookups can remain unchanged while sparse IDs are resolved explicitly.
- Owner: MAIN / LGPE Pokémon/stat-data lane.

### AUDIT-028 — Gen VIII/IX base-stat form routing returns zero/wrong rows and can index beyond valid arrays
- Severity: P2
- Confidence: CONFIRMED
- Area: modern Pokémon base stats / form handling / stat writeback
- Files: `include/Pokemon/BaseStatsGen89.h`, `src/Pokemon/BaseStatsGen89.cpp`, and consumers `src/Pokemon/Pokemon8SWSH.cpp`, `Pokemon8BDSP.cpp`, `Pokemon8LA.cpp`, `Pokemon9SV.cpp`, `Pokemon9LZA.cpp`
- Exact symbol: `getBaseStatsGen89()` (and `getBaseStatsSWSH()` through fallback).
- Problem: the modern base-stat table deliberately places all-zero placeholders in the dense species array for many form-driven species and relies on `getBaseStatsGen89()` to redirect them into dedicated form arrays. That handwritten routing is incomplete and contains wrong offsets/raw-form indexing.
- Confirmed valid-form examples:
  - Tauros has four supported forms. The dedicated array has only the three Paldean breeds, but the main switch indexes it with raw `form`. Kanto Tauros form 0 therefore receives Combat Breed stats instead of its dense base row, and Paldean form 3 indexes one past the three-row array.
  - Thundurus form 0 falls through to an intentional all-zero dense placeholder; form 1 is routed to index 1 of the combined Tornadus/Thundurus/Landorus array, which is **Tornadus-Therian**, not Thundurus-Therian.
  - Landorus form 0 likewise returns an all-zero placeholder; form 1 is routed to combined-array index 2, which is **Thundurus-Incarnate**, not Landorus-Therian.
  - Ursaluna's dedicated array contains only one row, Bloodmoon. The code indexes it with raw form, so base Ursaluna form 0 gets Bloodmoon stats and legitimate Bloodmoon form 1 indexes past the one-row array.
  - The dense table contains zero placeholders for Aegislash, Wishiwashi, Minior, Eiscue, Morpeko and Palafin, but the switch has no cases for those species at all. Valid base/out-of-battle forms therefore receive zero base stats.
  - Darmanitan has a complete four-form table declared but that table is never referenced; the dense row is a zero placeholder and the separate regional branch only handles one raw form. Several legitimate Darmanitan forms therefore resolve through zero/wrong data.
- Additional incomplete coverage: declared Mega, Primal, Ash-Greninja, Aegislash, Wishiwashi, Minior, Eiscue, Morpeko, Eternatus and Palafin form arrays are never referenced by `getBaseStatsGen89()`. Some of these are battle-only states, but the entity layer can still display/recalculate party records that contain them.
- Why it matters: all five modern mutable entity implementations obtain base stats through this helper (SWSH via `getBaseStatsSWSH()`) and their Level/EXP/IV/EV/species/form/nature edits call `recalculateStats()`. Wrong/zero/OOB lookup results therefore feed the party-stat tail written back into otherwise checksum-valid Pokémon. Valid Bloodmoon Ursaluna / Paldean Tauros cases can take the out-of-range path without malformed input.
- Source-data cross-check: the generated `PersonalInfo` table independently reports the expected form counts (for example Tauros 4 and Ursaluna 2), and a structural audit of all 1,491 generated rows found no form-redirection, bounds, presence or type-domain errors. The routing defect is in the handwritten base-stat helper, not in that generated form metadata.
- Tests: no focused base-stat/form tests for Tauros, Ursaluna, Thundurus, Landorus, Aegislash, Wishiwashi, Minior, Eiscue, Morpeko, Palafin or Darmanitan were found by filename.
- Missing tests: iterate every `PersonalInfo` species/form pair present in each supported game and require a nonzero, correct base-stat record with matching species id; compare representative alternate forms against the corresponding game personal table; run every form under bounds sanitizers; verify stat-affecting edit round-trips for the affected valid forms.
- Recommended fix: retire the handwritten form-routing table in favor of generated per-game personal/base-stat data using the same form-index redirection model already used by `PersonalInfo`. If a compatibility helper remains, every lookup must first validate `form < formCount` and every dedicated array must have explicit, tested mapping rather than raw-form arithmetic. Preserve SWSH's historical stat differences from later generations through per-game generated data, not ad-hoc post-fixes.
- Risk of fix: medium because this helper is shared across five entity formats; golden fixtures should prove both unchanged ordinary species and corrected alternate forms before merge.
- Owner: MAIN / Pokémon personal-data + stat-calculation lane.

### AUDIT-029 — Gen II finalization can serialize an in-progress packed move with carried Pokémon omitted
- Severity: P2
- Confidence: CONFIRMED
- Area: Generation II staged relocation / verified export safety
- Files: `include/Integration/Gen2/Gen2StagedEditor.h`, `src/Integration/Gen2/Gen2PackedMove.cpp`, `src/Integration/Gen2/Gen2StagedEditor.cpp`, `src/Integration/Gen2/Gen2ExportTransaction.cpp`
- Exact symbols: `beginPackedGroupMove()`, `placePackedGroupMove()`, `cancelPackedMove()`, `finalizedBytes()`, `publishVerifiedStagedEditorExport()`.
- Problem: beginning a packed move immediately removes the selected Pokémon from its source box and compacts the staged retail list, while keeping the removed records only in the in-memory `packedMove_.carried` state. `finalizedBytes()` does not test `packedMove_.active`; it repairs checksums/mirrors and strictly reparses the already-compacted bytes. The export transaction then accepts those bytes because the file is structurally valid.
- Why it matters: finalization during an in-progress carry can produce a valid edited `.srm` that simply omits the carried Pokémon. If there is an unrelated pending change, the normal export wrapper's `hasPendingChanges()` check is already satisfied even though the carry itself has not yet been placed. The verified-export hash/reparse checks cannot detect the semantic omission because the resulting save is internally consistent.
- Recovery context: the export bundle also includes the immutable original backup, so this does not destroy the authoritative source save. It can still generate an incorrect edited image and violates the staged move transaction contract.
- Comparison: the Gen III staged editor explicitly refuses `finalizedBytes()` while `carryingSparseMove()` is true.
- Missing tests: begin a packed move with another unrelated pending edit, call `finalizedBytes()` / `publishVerifiedStagedEditorExport()`, and require failure until place or cancel; verify cancel restores byte-identical pre-carry staged state and placement enables finalization.
- Recommended fix: make `finalizedBytes()` fail closed whenever `packedMove_.active` is true, with a clear “place or cancel carried Pokémon first” error. Also consider rejecting unrelated staged mutations while a carry is active, matching the Gen III transaction boundary.
- Risk of fix: low.
- Owner: MAIN / Gen II staged-edit lane.

### AUDIT-030 — Gen I finalization can serialize an in-progress packed move
- Severity: P2
- Confidence: CONFIRMED
- Area: Generation I staged relocation / export safety
- Files: `include/Integration/Gen1/Gen1StagedPokemonEditor.h`, `src/Integration/Gen1/Gen1PackedMove.cpp`, `src/Integration/Gen1/Gen1StagedPokemonEditor.cpp`
- Exact symbols: `beginPackedGroupMove()`, `placePackedGroupMove()`, `cancelPackedMove()`, `finalizedBytes()`.
- Problem: `beginPackedGroupMove()` saves the pre-carry bytes, then implements pickup by calling `stageRemove()` for each selected slot. Those removals are immediately committed into the staged view and rebuild the pending-change list. The carried Pokémon exist only in `packedMove_.beforeBytes` plus source-slot metadata until placement/cancel. `finalizedBytes()` never checks `packedMove_.active`.
- Why it matters: while a carry is active, `hasPendingChanges()` is already true because pickup itself staged removals. Finalization can therefore checksum and strictly reparse a structurally valid save in which the carried Pokémon are absent. A caller that exports those bytes gets an internally valid but semantically incomplete edited save.
- Comparison: Gen III explicitly blocks finalization while carrying a sparse move. Gen II has the same missing guard recorded in AUDIT-029.
- Missing tests: begin single/group carry and require `finalizedBytes()` failure before placement/cancel; ensure cancel restores byte-identical pre-carry bytes and pending changes; ensure successful placement allows finalization and preserves exact carried records.
- Recommended fix: fail closed at the top of `finalizedBytes()` when `packedMove_.active`, and consider blocking unrelated staged mutations while a carry is active. Apply the same transaction invariant to Gen I and II.
- Risk of fix: low.
- Owner: MAIN / Gen I staged-edit lane.

### AUDIT-031 — SC SHA-256 message decoding uses signed-shift undefined behavior
- Severity: P2
- Confidence: CONFIRMED
- Area: modern SC-container integrity primitive
- Files: `src/Utils/SHA256.cpp`, `include/Utils/SHA256.h`; consumed by the SC save encryption/decryption layer.
- Exact symbol: `SHA256::transform()`.
- Problem: the first sixteen message-schedule words are assembled with expressions such as `buffer[i * 4] << 24`. Each `buffer[]` element is a `uint8_t`, which undergoes integer promotion to signed `int` before the shift. For a first byte >= 0x80, shifting that promoted positive int by 24 produces a value outside the representable signed-int range, which is undefined behavior in C++.
- Why it matters: this SHA implementation is the integrity primitive used by the authenticated SC save containers. Real save/hash input naturally contains arbitrary high-bit bytes, so the undefined operation is not limited to malformed input. Current GCC/Clang targets will commonly emit the intended bit pattern, but the language contract does not guarantee it; optimizer/sanitizer/toolchain changes can make save hashes nondeterministic or wrong.
- Contrast: the MD5 and little-endian helper code in the same repository explicitly widens bytes to `uint32_t` before high shifts and does not have this problem.
- Current tests: no focused SHA-256 known-answer or high-bit input test was found in the current test tree.
- Missing tests: standard SHA-256 vectors (empty string, `abc`, multi-block input), a block containing bytes >= 0x80 in every word position, and a captured SC-container hash fixture under UBSan/host CI.
- Recommended fix: cast every byte to `uint32_t` before shifting, e.g. `(static_cast<uint32_t>(buffer[n]) << 24)`, or centralize through an unsigned big-endian 32-bit reader. Add known-answer tests and a real SC fixture.
- Risk of fix: low; intended output is unchanged, while behavior becomes defined.
- Owner: MAIN / encryption-integrity lane.

### AUDIT-032 — PR #97 removes AppShellSection::Collections but still references it
- Current disposition: FIXED at live PR #97 head `456a493771bc25ba49b7b9c27269477eec835a07`. `AppShellSection` now contains `MasterVault`, `Pokedex`, `Banks`, `Settings`, and `Games`, and `AppShellScreen::update()` maps the Pokédex preview without referencing the removed `Collections` member. Historical evidence below remains valid for head `5f19fd14628c182db135038864036b6eb1b86c49`.
- Severity: P1
- Confidence: CONFIRMED
- Area: sibling PR #97 / native Main Menu build
- Files: `include/UI/AppShellModel.h`, `src/UI/AppShellScreen.cpp`, `.github/workflows/product-ui-native.yml`
- Exact symbols: `enum class AppShellSection`, `AppShellScreen::update()`.
- Problem: PR #97 head `5f19fd14628c182db135038864036b6eb1b86c49` removes `Collections` from `AppShellSection` when replacing the old eight-area dashboard, but `AppShellScreen::update()` still contains `infoSection == PokeBank::UIModel::AppShellSection::Collections` while selecting an organization-preview kind.
- Why it matters: this is a compile-time reference to a nonexistent enum member. The new native Main Menu cannot be considered a buildable hardware candidate at this head until the stale branch is removed or updated.
- Reachability: `src/UI/AppShellScreen.cpp` is explicitly part of the product-UI native workflow path, and that workflow performs a clean devkitA64 `make -j1` followed by required `.elf` and `.nro` existence checks.
- Current tests: `tests/test_app_shell_model.cpp` validates the revised enum/navigation model but does not compile `AppShellScreen.cpp`, so it cannot catch this stale screen-level enum reference. `tests/test_game_hub_contract.py` is also text-contract based.
- Recommended fix: remove the obsolete `AppShellSection::Collections` branch from `OrganizationPreview` handling or replace it with the intended `Pokedex` behavior, then run the native product-UI workflow and host contract tests at the exact PR head.
- Risk of fix: low; the model now has only Pokédex/Banks/Search as previewable root sections.
- Owner: sibling UI/QoL lane (PR #97). Do not merge into MAIN until its normal lane validation passes.

### AUDIT-033 — Gen IV move-stat presentation uses HGSS values for Diamond/Pearl
- Severity: P3
- Confidence: CONFIRMED
- Area: Gen IV move picker / exact battle-stat display
- Files: `include/Names/MoveBattleStats.h`, `include/UI/MovePickerPresentation.h`, `tools/gen_move_battle_stats.py`, `src/UI/Gen4SharedPokemonSurface.inc`
- Exact symbols: generated `MoveBattleStatsData::Gen4`, `getMoveBattleStats()`, `MovePickerPresentation::statsLabel()/rowLabel()`.
- Problem: the generator sources one Generation IV move table exclusively from pinned `pret/pokeheartgold` and `getMoveBattleStats()` returns that same table for Diamond/Pearl, Platinum and HGSS. Generation IV is not fully uniform across those releases.
- Concrete verified example: Hypnosis (move 95) is generated as accuracy 60 and therefore displayed as 60% in Diamond/Pearl. Diamond/Pearl use 70% accuracy; Platinum changed it back to 60%, which HGSS retains.
- Why it matters: the user-facing picker explicitly claims “exact Gen IV Acc / Pwr / PP”. For Diamond/Pearl, that claim is false for at least Hypnosis, so move quality information can be wrong even though no Pokémon/save bytes are corrupted.
- Current test gap: `test_move_picker_presentation.cpp` checks cross-generation differences but treats all Gen IV games as one table and has no within-Gen-IV DP-vs-Pt regression.
- Recommended fix: generate distinct DP and Pt/HGSS move-stat tables (or patch the known intra-generation differences explicitly), dispatch by exact game/version rather than only `GameVersion::DP/PT/HGSS` to one common array, and add a Hypnosis regression asserting DP=70 and Pt/HGSS=60.
- Risk of fix: low; presentation-only data, but exact-game routing must remain deterministic.
- Owner: MAIN / move presentation data lane.

### AUDIT-034 — HD sprite recovery/preflight can accept corrupt existing PNGs
- Severity: P3
- Confidence: CONFIRMED
- Area: device-artifact/recovery visual integrity
- Files: `tools/gen_hdsprites.py`, `tools/check_device_assets.py`, `tools/recover_workspace.py`; final packaging consumes the preflight in `tools/package_device_build.py`.
- Exact symbols: `gen_hdsprites._one()`, `check_device_assets.main()`, `recover_workspace.png_count()/restore_hd_sprites()`.
- Problem: an existing HD sprite is considered reusable by `gen_hdsprites.py` solely because its path exists. Recovery decides the HD tree is complete primarily from the number of `.png` filenames. The device asset preflight likewise builds a filename set and checks required names; it reports directory size totals but does not require each PNG to be non-empty or decodable.
- Concrete failure mode: replace a required file such as `romfs/sprites/pokemon_hd/25.png` with a zero-byte or otherwise invalid PNG while keeping the filename. If the directory still contains the expected number of PNG filenames, normal recovery need not invoke regeneration; even if the generator runs without `--force`, it returns `have` for that existing file. The preflight's required-name and representative-file checks can still pass it.
- Why it matters: `package_device_build.py` treats the asset preflight as the visual-acceptance packaging gate. The final NRO can therefore be source-addressed and byte-verified against the intended RomFS tree while that intended tree itself contains an unreadable sprite. This can create missing/broken Pokémon art on hardware despite a nominal asset PASS.
- Scope: visual/artifact integrity only. No save-data corruption path is created by this defect.
- Missing tests: zero-byte required HD sprite; non-PNG bytes under a `.png` name; truncated PNG; recovery with correct filename count but one corrupt required file; preflight must fail all of them.
- Recommended fix: validate every required HD asset as a non-empty decodable PNG (at minimum PNG signature + dimensions; preferably Pillow verification where available), and have recovery regenerate any invalid file rather than using filename count as completeness. The final preflight should independently repeat the validity check before packaging.
- Risk of fix: low; runtime assets only, with increased preflight cost.
- Owner: MAIN / device artifact + recovery lane.

### AUDIT-035 — personal/learnset regeneration still searches the removed SPECIES_NAMES symbol
- Severity: P3
- Confidence: CONFIRMED
- Area: reproducible generated legality/personal data refresh
- Files: `tools/gen_personal.py`, `tools/gen_learnsets.py`, `tools/gen_speciesnames.py`, `src/Names/SpeciesNames.cpp`, `tools/regenerate.py`
- Exact symbols: both generators' `load_species_names()`; multilingual species-name emission in `gen_speciesnames.py`.
- Problem: `gen_personal.py` and `gen_learnsets.py` still parse the checked-in species source with the regex `SPECIES_NAMES\[\]\s*=...`. The multilingual species-name generator no longer emits that symbol: it emits `SPECIES_NAMES_EN[]` (plus the other eight language arrays) and `SPECIES_NAMES_BY_LANGUAGE[]`.
- Why it matters: both scripts call `load_species_names()` before emitting their tables and explicitly terminate when the old symbol is absent. The documented aggregate command `python tools/regenerate.py --tables` discovers and runs these scripts, so a current clean checkout cannot complete its advertised full generated-table refresh even though normal builds still compile the already-committed outputs.
- Scope: regeneration/reproducibility blocker, not an immediate runtime/save-data defect. Existing committed PersonalInfoTable/LearnsetTable artifacts remain usable until a deliberate refresh is required.
- Related drift: `check_device_assets.py::gen1_species_names()` also searches the old `SPECIES_NAMES[]` spelling, but it catches the lookup failure and falls back to generic “Species N” labels; that only weakens missing-asset diagnostics rather than blocking packaging.
- Missing tests: run every discovered table generator against the current tree in a pinned/offline-cache CI job; at minimum invoke both `gen_personal.py` and `gen_learnsets.py` after regenerating species names and require successful no-drift output.
- Recommended fix: make both generators consume the shared language helper/current English symbol explicitly (or better, consume PKHeX's source species resource directly rather than reparsing another generated C++ file). Update the asset preflight's parser at the same time. Add a regeneration smoke test so future symbol refactors cannot silently break the generator chain.
- Risk of fix: low; generator-only parsing change, but regenerated table diffs must still be reviewed before commit.
- Owner: MAIN / generated-data reproducibility lane.

### AUDIT-036 — Product Home Help overlay advertises stale controller actions
- Severity: P3
- Confidence: CONFIRMED
- Area: sibling PR #97 / Product Home control guidance
- File: `src/UI/SaveSelectScreen.cpp`
- Exact behavior: the live Product Home input handler uses `ZL` to switch profile when multiple users exist, `L/R` to change the selected game, and `B` to set both `appExitRequested` and `exitRequested`, exiting PokeBank NX. The normal bottom hint matches that behavior: `ZL: Profile | L/R: Change Game | ... | B: Exit`.
- Problem: the reachable Help overlay opened with Minus still says `L / R   Previous or next Switch user` and `B   Return to the PokeBank NX Main Menu`. Those instructions describe an older navigation model and contradict the live handler on the same screen.
- Why it matters: a user following Help can change the wrong thing when trying to switch profiles and can exit the application when expecting only to go back one screen.
- Test gap: `tests/test_game_hub_contract.py` proves that L/R exists for game switching but does not assert that Help copy matches the handler/nav hint, so this drift can remain test-green.
- Recommended fix: make Help use `ZL` for profile switching, `L/R` for game switching, and `B` for Exit; add a contract assertion tying the visible Help strings to those current actions.
- Risk of fix: low; user-facing copy/test only.
- Owner: sibling UI/QoL lane (PR #97).



### AUDIT-037 — Gen I/II group pickup can strand an active staged move after presentation-refresh failure
- Severity: P2
- Confidence: CONFIRMED
- Area: classic Gen I/II multi-select box move / staged transaction recovery
- Files: `src/UI/ClassicPackedMoveOverlay.inc`, `src/Integration/Gen1/Gen1PackedMove.cpp`, `src/Integration/Gen2/Gen2PackedMove.cpp`, `src/Legacy/RBYReadOnlyTrainer.cpp`, `src/Legacy/GSCReadOnlyTrainer.cpp`
- Exact path: `beginSelectedGroup()` calls `beginPackedGroupMove()`, then calls the generation-specific presentation refresh. If refresh returns false, the function only posts an error and calls `clearTransientSelection()`; unlike `beginGen1()` and `beginGen2()`, it does not call `cancelPackedMove()`.
- Backend evidence: both packed-group implementations make the carry transaction active and remove the selected Pokémon from staged box bytes before returning success. Their `cancelPackedMove()` implementations restore the saved pre-pickup staged state. The Gen I/II backend multi-move tests explicitly assert that cancel restores the pre-pickup bytes, confirming rollback is part of the transaction contract.
- Failure consequence: if the subsequent bridge/presentation refresh fails, the UI clears its multi-select/carry state while the backend still owns an active packed move and staged boxes remain in the post-removal state. A later pickup is rejected as “already being carried”; the normal B-cancel route is no longer reachable through this overlay. The original source remains immutable, but the app-owned working copy can be left missing the carried records until a broader discard/reopen path restores it.
- Export risk: both generations can construct finalized staged bytes from their current staged state; the incomplete carry is not itself a hard export prohibition. A refresh failure therefore must not be treated as a harmless display-only failure.
- Test gap: backend tests cover begin/place/cancel and failed destination placement, but there is no UI/bridge fault-injection test proving that a failed post-pickup refresh rolls the group transaction back.
- Recommended fix: mirror the single-Pokémon pickup path: on Gen I/II group refresh failure, call `cancelPackedMove()`, refresh presentation again from the restored staged state, preserve the first failure for diagnostics, and only clear UI state after rollback has been attempted. Add a fault-injection contract test for both generations.
- Risk of fix: low-to-medium; recovery-path-only change, but test both rollback success and rollback-refresh failure without weakening source immutability.
- Owner: MAIN / classic staged move transaction lane.


### AUDIT-038 — Legacy Save Instances scroll logic assumes one more visible row than the renderer draws
- Severity: P3
- Confidence: CONFIRMED
- Area: MAIN game/source picker / legacy Save Instances navigation
- File: `src/UI/SaveSelectScreen.cpp`
- Exact behavior: the Legacy Save Instances draw path declares `visibleRows = 5` and passes that value to `drawSaveInstanceRows()`, so only five save rows are rendered. The matching `Overlay::LegacyInstances` input path separately declares `visibleRows = 6` when deciding whether to advance `legacyInstanceScroll`.
- Failure consequence: when the selection moves to the sixth logical row below the current scroll origin, input considers it still visible and does not scroll, while draw renders only rows 1–5. The focused save can therefore move off-screen until the next navigation step changes the scroll origin.
- Safety impact: presentation/navigation only. The selected save is still snapshot-revalidated at the open boundary, so this does not weaken source immutability or stale-file protection.
- Test gap: no contract currently binds the Legacy Save Instances renderer's visible-row count to the input scroll-window count.
- Recommended fix: define the visible-row count once and share it between draw and input, or derive scrolling from the same layout constant used by the renderer. Add a navigation test that walks across the fifth/sixth-row boundary and asserts the focused row remains rendered.
- Risk of fix: low.
- Owner: MAIN / SaveSelect source-picker UI lane.


### AUDIT-039 — Backup save can serialize while held-Pokémon rollback failed
- Severity: P2
- Confidence: CONFIRMED
- Area: MAIN Storage / game-save custody handoff
- File: `src/UI/TrainerViewScreenBase.inc`
- Exact behavior: `returnHeldToOrigin()` deliberately plans the entire rollback before moving any carried Pokémon. If no complete destination plan exists, it posts `Could not return the held Pokemon safely - custody retained.` and returns with `moveMon` still owning the carried records. The regular game-save confirmation path calls `returnHeldToOrigin()` immediately before serialization, but it does not inspect a success result and does not check `carrying()` afterward; it can continue through the safety destination check and call `performSave(destDir)`.
- Failure consequence: if a Pokémon was lifted from the save pane and its origin/fallback space is no longer available, the in-memory trainer still lacks that Pokémon while custody survives only in `moveMon`. A successful backup serialization can therefore write an incomplete app-owned save image and then clear `hasUnsavedChanges`, even though the held Pokémon was never restored to the serialized trainer state.
- Safety scope: the original external/source save remains immutable and live writes remain locked. The risk is to the newly written app-owned backup/workspace, not the source file.
- Caller comparison: the Storage-view B path is safe because it calls `returnHeldToOrigin()` and returns immediately; if custody remains, the next B retries. The carried-block menu also only attempts the return and performs no serialization. The regular save-confirm path is the path that continues into a write after a failed return.
- Test gap: there is no regression test that exhausts/locks all return destinations, leaves custody active, then attempts a game backup save and proves serialization is refused.
- Recommended fix: make `returnHeldToOrigin()` report success/failure, or explicitly test `carrying()` after the call. If custody remains, abort the save, keep the dialog/session active, and surface the custody-retained error. Add tests for both failed return => no save and successful return => save proceeds.
- Risk of fix: low-to-medium; save-gating/recovery path only, but it must preserve the existing all-or-nothing custody rule.
- Owner: MAIN / Storage + backup-save orchestration lane.
