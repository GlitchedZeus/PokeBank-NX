# Full Repository Forensic Audit

Status: IN PROGRESS

## Exact state

- Repository: GlitchedZeus/PokeBank-NX
- Audit branch: `audit/full-repository-line-by-line-20260928`
- Primary MAIN tree audited: PR #92 head `acfca273eff0fb145f2e18a8f6d07817e1475572`
- PR #92 branch: `feature/gen4-full-editor-20260928`
- Sibling UI overlay: PR #97 head `c63ce48ad6952128cabe94aaeaba627a460b04bd` (delta audited separately)
- Integration parent: PR #90 head `8b3bcc16c804247bfe8d1314b686974ce73051d8`
- Hardening parent: PR #79 head `00ee7a6ed7ac1b5a93c43246d70c252e135acec0`
- Default branch main: `aca2bf41c83d81084886a46d53195f6cead81ccc`

## Inventory

- Git tree truncated: false
- Tracked non-directory paths: 725
- Text/unknown candidates: 692
- Binary candidates: 32
- Symlinks: 0
- Submodule entries: 1
- Audit ledger: `docs/FULL_REPOSITORY_AUDIT_LEDGER.tsv`

## Coverage

- Audited tracked paths: 232 / 725
- Fully read text files: 198 / 692 (text/unknown classification remains provisional until content inspection completes)
- Binary/non-text inspected: 34 / 34 currently identified by exact extension/manifest scan

## Current checkpoint — live MAIN catch-up

- PR #92 advanced two commits beyond the prior frozen audit snapshot `dc8a64158f2ebaf22a7b456e52eb8ee320e9c1f4`.
- The live catch-up delta is bounded to five files: `src/Names/LocationNames.cpp`, `src/UI/Gen4SharedPokemonSurface.inc`, `src/UI/Panels/ItemsPanel.cpp`, `tests/test_gen4_hardware_surface_contract.cpp`, and `tools/gen_locations.py`.
- Fully read in this checkpoint: `include/Utils/DurableFile.h`, `src/Utils/DurableFile.cpp`, `src/Integration/Gen4/Gen4StagedPokemonEditor.cpp`, `src/Pokemon/Pokemon4Mutable.cpp`, `tests/test_durable_file.cpp`, and `tools/gen_locations.py`.
- The Gen IV staged editor currently preserves immutable source bytes, mutates a separate staged image, refreshes the affected save CRC, reparses the staged save, exact-verifies the serialized PK4, and rolls back the staged buffer on verification failure.
- The Gen IV UI bounds Current PP to the move-specific maximum and clamps PP when PP Ups decrease. The lower-level `Pokemon4Mutable::setPP` API itself does not enforce that semantic bound; this remains a follow-up/API-hardening question until whole-repository callers are traced, not a classified finding yet.
- `DurableFile` deliberately documents Switch SD directory-entry durability as a separate hardware gate; absence of a directory fsync in this primitive is therefore not being misreported as a newly discovered hidden guarantee violation.

## Sibling PR #97 overlay coverage

- Delta from PR #90: 6 commits / 18 changed paths.
- Fully read so far: 9 / 18 changed paths, including the complete 582-line `src/UI/AppShellScreen.cpp`, new app-shell/organization model headers, native UI workflow/build fragment, and their focused tests.
- This overlay count is intentionally separate from the 725-path PR #92 MAIN ledger.


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

## Findings

No findings are recorded here until supported by direct evidence from the frozen tree or active-overlay delta.


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
