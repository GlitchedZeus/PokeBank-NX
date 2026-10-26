# Thread L — Emulator launch and return: progress checkpoint

Branch `claude/launch-return-20261015`, created from `c952565cf878705726b2541546d085fa9724108d`
(PR #307 head). Thread 2R merges this branch forward into #307.

Every NRO built from this work is **NOT DEVICE TESTED**.

## Pinned references

| Reference | Revision | Used for |
|---|---|---|
| switchbrew/nx-hbloader | `82b95122c5ae8dc059bf23893ba7623c72c86773` | Loader loop. `DEFAULT_NRO "sdmc:/hbmenu.nro"` loads when a child leaves no next-load. Next-load buffers are 512 B (path) and 2048 B (argv). `main()` closes SM before loading NROs. |
| switchbrew/libnx | `20cbcfd90567f7bf84df7725f27e345e2164fa6b` | `env.c envSetNextLoad()` copies with `strcpy` and no bound. `argv.c argvSetup()` treats `"` only as a delimiter and has no escape character. `romfs_dev.c romfsMountSelf()` opens the NRO named by `argv[0]`. `switch_crt0.s` saves `__stack_top`. |
| libretro/RetroArch | `b2538f1249e81d687dbcaece491674713a38ae68` | `frontend/drivers/platform_switch.c`: a normal Quit leaves `FRONTEND_FORK_NONE`, so `frontend_switch_exitspawn()` schedules nothing. Restart and core loads use `envSetNextLoad`. |
| NaGaa95/DrasticDS_nx | `3698668ba5ff01ec7fd721e178e5c59bf57c020b` | `DrasticDS.nro` is an SDL launcher: `launcher/source/main.cpp` with `argv[1]` as the silent direct ROM. It writes `drastic.ini` with `Wrapper/LauncherPath` = its own `argv[0]` (`forwarder.cpp resolveLauncherPath`), then chains `.emu/DrasticDS_nx.nro`. The in-game "Quit to launcher" sets `exit_requested`, and `source/main.c` calls `envSetNextLoad(launcher_path, launcher_path)`. Its NSP forwarder (`launcher/fwd/main.c`) ends the process when the child schedules nothing. |

## Owner hardware evidence (PR comments, quoted)

- `97f540e4` (PR #92, 2026-10-06): "Gen I/II/III RetroArch launch: PASS". Also: "RetroArch quit lifecycle: FAIL — quitting RetroArch drops/crashes to Nintendo HOME instead of returning to PokeBank NX."
- Same comment: "DraStic return-to-PokeBank is no longer an acceptance requirement: DraStic exposes Quit to Launcher rather than a return-to-caller contract".
- `698ea4d1` (PR #92, 2026-10-07): **REJECTED ON DEVICE**. With the return host on the default path, Gen I–III RetroArch "crash on launch" for Red/Blue/Yellow, Gold/Silver/Crystal and Ruby…Emerald. `8803c9ea` then restored the direct handoff.
- `0760334f`: owner-accepted with the direct handoff. Its launch/exit chain (GameLauncher, return host, `main.cpp`) differs from `c952565c` only in the ROM-header identity check (`41a8cee`). No exit-path code changed in between.
- PR #307 (2026-10-15) still lists both "Gen I–III RetroArch exit crash" and "Gen IV/V Quit to Launcher returns to DraStic" as open.

## Diagnosis

### Bug 1 — Gen I–III RetroArch: quit "drops/crashes to HOME"

1. On the accepted path PokeBank calls `envSetNextLoad(core, "\"core\" \"rom\"")` and exits. The loader then runs the core.
2. RetroArch's normal Quit schedules nothing (`FRONTEND_FORK_NONE`).
3. The outer loader then picks the next NRO:
   - Stock hbloader (title override): loads `sdmc:/hbmenu.nro`.
   - An NSP forwarder: ends the process. On HOME this looks exactly like "drops/crashes to HOME".
   - If hbmenu is missing, hbloader aborts with result `2347-0003`.

So the behaviour comes from the loader once RetroArch has quit, not from a PokeBank code path. Nothing on the exit path changed between `0760334f` and today. Returning to PokeBank needs something to schedule PokeBank's NRO after RetroArch exits. Only a nested loader (the return host) can do that without modifying RetroArch or hbmenu.

Rejected alternative: passing PokeBank as RetroArch's `argv[0]`. RetroArch adopts `argv[0]` as `RARCH_PATH_CORE` (commit `4ab408e6`), and libnx mounts the RomFS named by `argv[0]`. The emulator would break.

### Bug 2 — Gen IV/V DraStic: "Quit to Launcher" returns to DraStic

PokeBank chains to `DrasticDS.nro` (the launcher) with the ROM. The launcher records its own `argv[0]` as `Wrapper/LauncherPath` and starts the emulator. "Quit to launcher" then reloads that path, by design.

PokeBank cannot change that stored path safely. The launcher takes it from `argv[0]`, and libnx mounts RomFS from `argv[0]`. Passing PokeBank's NRO there would make DraStic load PokeBank's RomFS and fail to extract its core and emulator. It would also aim DraStic's self-updater (`LauncherUpdate_*`) at PokeBank's NRO. **Rejected.**

What PokeBank *can* control is the loader DraStic runs under. In the return host's `return-on-launcher` mode, the host runs the launcher (hop 1) and the emulator (hop 2). When the emulator then requests the launcher's own path, the host reloads PokeBank instead.

### Why `698ea4d1` crashed on launch — still open

A line-by-line comparison of the host with nx-hbloader turned up no definitive fault:

- same mapping geometry
- same TLS save and restore
- same trampoline and `__stack_top` use (verified in libnx `switch_crt0.s`)
- same applet type, syscall hints and code-memory capability
- the inner heap (64 KiB) covers libnx `argvSetup`'s 0x9000-byte arena

Differences fixed here:

- The host kept SM open while children ran; nx-hbloader closes it.
- The host backslash-escaped argv, which libnx never unescapes.

Neither is proven to be the cause. That is why the host stays opt-in, and it now writes a stage log to tell the remaining possibilities apart on device.

### Bug 3 — general reliability findings

- **Fixed: unbounded loader writes.** `envSetNextLoad` copies into fixed 512/2048-byte loader buffers with no bound. The HomebrewNro path (DraStic, melonDS, mGBA, Tico) never checked argv length, so a long ROM path could overrun hbloader memory. The plan builder now refuses this before PokeBank exits.
- **Fixed: argv quoting.** Paths containing `"` are refused. A `\` is passed through literally, as libnx parses it.
- **Fixed: selection lost after return.** Product Home did not keep the selected game across the new process a return starts. Its hub state stores only sort mode and per-game records. `UI.cpp` now records the exact NavigationState (profile, game, source identity, kind, title, indices) when a launch hands off. The next start consumes it once (7-day staleness bound; corrupt or stale records are deleted).
- **Expected reordering:** with "Recently Played" or "Most Played" sorting, the launched game moves by design (`recordCurrentLaunch`). Focus follows it, because restoration matches identity, not index.

## What changed (commit list in the git log of this branch)

| File | Change |
|---|---|
| `runtime/return_host/source/return_host_policy.h` | New, pure C. Holds argv protocol parsing, libnx-exact quoting, loader bounds, SD-path equality and next-action decisions (`RETURN` / `CHAIN` / `RETURN_INTERCEPTED`). |
| `runtime/return_host/source/main.c` | Uses the policy. Adds `return-on-launcher` mode, closes SM before children, writes the stage log `sdmc:/switch/PokeBank-NX/runtime/return-host.log` (truncated per launch) and gives explicit return reasons. |
| `include/UI/GameLaunchModel.h` | Adds `buildGameLaunchChainPlan`, `GameLaunchQuitRoute` with `gameLaunchQuitRouteNotice`, and the return-selection serialize/parse. |
| `src/UI/GameLauncher.cpp`, `include/UI/GameLauncher.h` | Every homebrew handoff goes through the plan. The return host is used only when the opt-in marker exists. Adds `previewGameLaunchQuitRoute`, `gameLaunchHandoffRequested`, and save/consume of the return selection. |
| `src/UI/UI.cpp` (launch/return path only) | `handleSaveSelection()`: restores the recorded focus on the first Home of the process and records it on a launch exit. Helpers live in the file-local namespace. |
| Tests | `test_game_launch_model` (libnx argv tokenizer port, host policy, plans, return selection), `test_retroarch_launch_resolution` (production-path file record, quit-route preview), `test_return_host_packaging_contract.py`, and the launch block of `test_game_hub_contract.py`. |

## Round 3 (2026-10-19): RetroArch core evidence and an honest quit preview

Base: forward-merged #307 `0faf7e9c` (merge `2e32b775`). Fix commit `07478b48`. Both defects come from 2R's source audit of `0faf7e9c`.

### 1. A melonDS save launched in DeSmuME

`defaultRetroArchCore()` chose DeSmuME first from the game ID alone. Stored game-file links recomputed that default, and a playlist core overrode it only when present. The emulator therefore opened without the user's save.

The fix is `retroArchSaveCoreEvidence()` in `GameLaunchModel.h`. It proves the core from the discovered save itself:

| Evidence | Core | Pinned reference |
|---|---|---|
| Save's parent folder is a core `library_name` (RetroArch "sort saves by core" puts saves in `<dir>[/<content dir>]/<library_name>/`) | that core | RetroArch `b2538f12` `runloop.c` |
| `library_name` values | `DeSmuME`, `melonDS`, `Gambatte`, `mGBA` | desmume `95b4d798` `libretro.cpp:517`; melonDS `66b5d263` `libretro.cpp:114`; gambatte `d9d6cd06` `libretro.cpp:1655`; mgba `7a12d6d4` (`projectName` = CMake project `mGBA`) |
| `<game>.dsv` for a DS game | DeSmuME | desmume `mc.cpp:235` |
| `<game>.sav` for a DS game | melonDS | melonDS `libretro.cpp:895` |
| Folder `melonDS DS` | ambiguous (no Switch build, per its README) | — |

Rules, applied in every RetroArch path (playlist match, loose ROM roots, stored links):

- **Proven evidence wins** over the playlist core and the game-ID default.
- **Proven but not installed:** `LauncherMissing`, with a message naming the core. It never silently launches a different core.
- **Contradictory or impossible evidence** (folder versus file, or a core that cannot run the game family): the previous choice is kept and `descriptor.launchNotice` explains why. `gameLaunchDialogNotice()` puts it in front of the quit-route notice.
- **No evidence:** the game-ID default, unchanged.

### 2. The quit preview could promise a return that the launch then skipped

`previewGameLaunchQuitRoute()` promised a return whenever the opt-in marker existed. `prepareReturnHost()` could still fail at launch time and silently fall back to the direct chain.

The fix is a single `returnHostReadiness()` check: opted in, own NRO known, and the bundled host installed through a verified temporary copy. It is cached and re-validated on every query. Both the preview and `planGameLaunchHandoff()` (which `requestGameLaunch()` calls) build the plan from it. If a launch's route differs from the one last previewed for the same descriptor, for example because the host vanished and could not be reinstalled, the launch is refused once and the message names the new route. The next preview shows it. While opted in, previewing now installs the host on the SD card, the same as the launch already did.

### Owner device checks for round 3 (add to procedure A)

- **R3-1:** a DS game saved with RetroArch's melonDS core (`.sav`, or saves sorted into a `melonDS` folder). Launch it from PokeBank: melonDS must open **with that save**, even when DeSmuME is installed and listed in the playlist.
- **R3-2:** a DeSmuME `.dsv` save must still launch in DeSmuME.
- **R3-3:** uninstall the melonDS core and repeat R3-1. PokeBank must say the melonDS core is missing instead of opening DeSmuME.
- **R3-4:** with the return-host marker present, note what the launch dialog says Quit will do, then quit. The outcome must match the promise: "returns to PokeBank NX" means PokeBank reopens; any other text means Homebrew Menu/HOME (or DraStic's launcher).

Host tests cover all four decisions. Only the device can prove the emulator actually loads the save.

### Request for 2R (SaveSelectScreen, not this lane's file)

So the launch dialog shows the core notice, replace the two `gameLaunchQuitRouteNotice(previewGameLaunchQuitRoute(...))` calls (`openLaunchDecision` and `currentLaunchQuitNotice`) with `gameLaunchDialogNotice(...)`. Its quit-route text is identical when there is no notice.

## Round 4 (2026-10-20): Save / Backup browser (R2-EMUBACKUP, C07, S05)

**Base:** #307 `c2287f50`. Owner results are in `docs/hardware/ROUND3_RESULTS_62a2f8c3.md`.

**Owner finding (screenshot 1):** Pokémon Black's "+ → Backups" showed only "Original DraStic save — READ ONLY" and "Staged edits — IN EDITOR", with no usable action. The list was built only from export records, and export (B08) has not produced any yet.

**Repair (emulator saves: RetroArch, DraStic and melonDS):**

| Owner flow | Now |
|---|---|
| Pick a game | Home card → + → Backups. One section per exact assigned or validated save. |
| List saves and existing backups | The original save, then this save's backups (deliberate copies and export records), newest first, matched by exact `SOURCE_PATH`. |
| Browse the current save | A on the original save runs the normal read-only open (`selectCurrentTitle`). It never makes a backup. |
| Deliberately create a backup | A on "Create backup now" (`SourceBackups::createBackup`) copies the save byte for byte into `backups/emulator-saves/<game>/<timestamp>/save<ext>`. It reads the copy back and compares, then writes `BACKUP_MANIFEST.txt` last. This is the only place an emulator backup is made. |
| Delete a backup | Y opens the confirmation. Only an explicit Y deletes that one backup. It is refused for foreign files or a forged path, and the last remaining copy is kept when the emulator file is gone. |
| Never write the emulator's save | The source is only ever opened `"rb"`. Tests check that it stays byte- and mtime-identical. |

**Tests:**
- `test_emulator_backup_access`: create, list and delete for RetroArch `.srm`, DraStic `.dsv` and melonDS `.sav`; nothing is made on list or open; same-second backups get distinct records; other saves are excluded; foreign, forged and last-copy cases are refused; refusals create nothing.
- `test_emulator_backup_layout`: Create, Browse and Details actions; all copy fits the BackupMenu rows and toast.
- `test_game_hub_contract`: create-only-on-action and browse-is-read-only.

**Installed Switch titles (SwSh/BDSP/LA/SV): no automatic backup on open (coordinator-confirmed).**
- `handleDefaultQuickOpen`, `handleItemsQuickOpen` and the Backups "Browse current save" row now call `openWorkingCopy`. It reads the installed save into the single reusable "Working" copy through `WorkingCopyRefresh::refresh`.
- If an earlier Working copy is byte-identical to the fresh one, it is deleted and no backup appears. If it differs, it holds PKSE edits, so it is kept as a dated backup folder and never discarded. If the fresh copy fails, the earlier one is put back.
- A timestamped backup is made only by Save Backups → "Create backup now" (`backupSaveData(..., true)`, called exactly once in `UI.cpp`). Delete stays X, then Y confirmation.
- The Settings **Auto-Backup** toggle is retired, in both the Product Home settings and the in-editor settings panel, along with the `g_autoBackupEnabled` global. A toggle that did nothing would mislead. An old `autoBackup=` line in `settings.cfg` is read and ignored, and no longer written.
- Tests: `test_emulator_backup_access` `workingCopyRefresh` (fresh / unchanged dropped / edits preserved / failure restores); `test_game_hub_contract` pins the open paths, the single create call and the retired setting.

**Device checks for round 4 (add to the checklist):**
- **R4-1:** Black (DraStic): + → Backups shows "Create backup now", the original DraStic save and the staged-edits note.
- **R4-2:** A on "Create backup now": a "Backup created" notice; a new "Backup · <date>" row is focused; the `.dsv` on the SD card is unchanged (SHA-256 before and after).
- **R4-3:** Y on that backup, then Y: deleted. Y, then A or B: kept.
- **R4-4:** A on the original save opens the game read-only. Back in Backups, no new backup appeared.
- **R4-5:** Repeat with a RetroArch Gen I–III save and a melonDS save.
- **R4-6:** Switch title (e.g. SwSh): open, close, open again. No new dated backup appears in Save Backups. "Create backup now" adds exactly one.
- **R4-7:** Settings (Product Home → System, and the in-editor panel) shows no Auto-Backup row. The other toggles still work by controller and touch.

## Round 5 (#322): open a backup as a staged copy (verified against Codex 80377153 / 8d482845 / f3bbd43f)

Already correct in #307 (Codex):
- Emulator Save Backups: full-screen card (background cleared), rows with date, provider, path and type, newest first, the current save marked read-only, unassigned recovery history marked read-only.
- A on an emulator copy opens THAT file through the strict Gen I-V parsers from an immutable snapshot (`openBackupTrainer`). Export Copy only; no backup on open; Y delete keeps the record and last-copy guards.
- Native rows lead with the date and badge legacy folders "ownership unknown"; legacy is never deletable.

Fixed here:
- **Native Switch backups no longer open as a read-only preview.** A copies the selected backup (owned or legacy) into the isolated `Opened-Backup` workspace of the exact profile/game (`WorkingCopy::openBackupSession`) and opens that as a normal staged session. The backup's contents are validated for the exact title first, and its listing snapshot must still match after the copy. Edits saved in an earlier session are kept as a dated backup, never overwritten. The backup itself stays byte-identical.
- The editor's top bar names the copy: "Opened from backup - <date>" (legacy: "Opened from legacy backup (owner unknown) - <date>").
- Gen IV emulator copies get the Open snapshot, so Export Copy fails closed if the backup file changed after it was opened.
- Emulator copies show the card's real provider instead of "Unknown provider".

Device checks:
- **R5-1:** SwSh/BDSP: Save Backups, A on an owned backup opens it in the editor (title bar says "Opened from backup"). Edit, save, back out: the backup folder is unchanged and a "Opened-Backup" row never appears.
- **R5-2:** Same with a legacy PKSE folder: opens, never offers delete, the legacy folder is unchanged.
- **R5-3:** Open backup A, edit and save, then open backup B: the edits from A appear as a new dated backup.
- **R5-4:** DraStic Platinum: A on a backup opens it; Export Copy succeeds; the backup `.dsv` is unchanged.

## Round 6: Review entry and return in Gen I–V (B08, P1-RETURN, AUD-01, 4D-HELP follow-up)

The four-option staged exit is unchanged. One rule for the Review button (`include/UI/ReviewEntry.h`):

| Screen | Gen I–IV | Gen V | Why |
|---|---|---|---|
| Trainer Info | Y (and +) | Y (and +) | Y was already Review, as on Gen V's Trainer page. |
| Party | **Y** or + | **Y** or + | Y had no job on Party. |
| Boxes | **+** | **Y** or + | Gen I–IV Y is Move / hold for Multi; Gen V Browse Y was free. |
| Bag | **+** | **+** | Y is Remove Item. In the Gen I–III Bag, + used to open an Options menu whose Review was a row ("buried", B08). + now opens the shared Review directly; the menu's Export and Discard are X and Y inside Review (Bag changes included in Gen I, II and III). The menu remains only as the fallback when a source has no Review workspace. |
| Create / Edit | **+** | **+** | Y is re-roll, and Discard inside the exit confirmation. |

How Review returns to the screen it was opened from:
- **Gen I–IV Party / Boxes / Bag / Trainer:** `openStagedReview` marks the Review standalone (`ClassicReviewOrigin`). Its B, or a closing Discard All, returns to the screen underneath. Cursor, box and page are untouched while Review is open. Party and Boxes go through one router (`ClassicBrowseReview.h`) that only acts on the plain grid: no overlay, Actions sheet, carry or modal.
- **Gen I Create / Edit:** `UX2State::reviewReturn` restores the workspace mode. The draft and focus survive, because `foundationSyncMainMode` leaves them alone for the same mode. A Discard All closes the workspace instead, since the draft's baseline is gone.
- **Gen II Create / Edit:** Review is routed and drawn ahead of the still-open workspace. B closes it back to the workspace.
- **Gen III / IV Create / Edit:** Review is already handled and drawn before the workspace, so B returns to it.
- **Gen V:** `Gen5SharedScreenState::openReviewFrom` records the return surface (Browse or Edit). B returns there with the draft, and Create's staged record, intact. Discard All lands on Browse and clears the draft.
- **Footers** advertise the entry (`footerHint`), and the Help pages' Review lines now match the code.

Tests:
- `test_review_entry`: the button rule over 5 generations × 6 screens, and the real Gen I–IV router plus Review origin on a stub screen: open, close, same screen, cursor, box and page.
- `test_gen5_review_discard_export_trace` `workspaceReviewReturn`: the real Gen V reader, Trainer bridge and state machine (Browse / Edit / Create return; Discard All).
- `test_classic_help_accuracy` re-pinned to the new claims.
- `test_ui_review_entry_host` (Lane S's real-input harness, `tests/host-ui`): the REAL `TrainerViewScreen::update`/`draw`. Covers:
  - Gen I, II, III, IV and V Trainer, Party, Boxes and Bag: the Review key opens the shared Review, and B returns to the same screen and cursor.
  - Gen III and Gen V Edit and Create: `+` opens Review, and B returns to the draft.
  - Gen III Export Copy is refused once the source changed since Open.
  - Disabling the Gen I–IV router makes 24 checks fail.
- Merged into #307: Party / Boxes / Trainer use Lane R's `ClassicBrowseFooter` `spec.review` (glyph from `ReviewEntry::footerHint`). CG63 was retargeted, because `+` now opens Review on Gen I–IV editor pages, but never on the save hub. The Gen I Edit row "Review / Stage Add" now returns to Edit, not Actions.
- **7E G5 closed:** Gen I–III Open takes the exact file's snapshot (`ClassicSourceGuard::snapshotAtOpen`). The file must still be the bytes discovery parsed. Every Gen I–III Export Copy re-checks it and refuses a changed source (`unchangedSinceOpen`), as Gen IV / V already do.

Save safety (7E): `docs/audits/ROUND6_SAVE_SAFETY_PKSE.md` (audit only; no save-path change).

Device checks:
- **R6-1:** Each generation, Party: Y opens Review; B returns to Party on the same slot. + does the same.
- **R6-2:** Gen I–IV Boxes: + opens Review; B returns to the same box and slot. Y still moves and holds for Multi. Gen V Boxes: Y and + open Review.
- **R6-3:** Each generation, Bag: + opens the shared Review directly (no Options menu); B returns to the same pocket and row. Y still asks to remove.
- **R6-4:** Each generation, Edit a Pokémon, change a field, press +: Review opens. B returns to Edit with the change still in the draft. Same from Create.
- **R6-5:** Gen I and Gen V: in a Review opened from Create/Edit, choose Discard All: you land outside the workspace, never back in a stale draft.
- **R6-6:** Trainer Info: Y and + open Review; the four-option exit (B with staged changes) is unchanged.
- **R6-7:** Gen I–III: open a save, edit, then let the emulator save over it (or replace the file). Export Copy is refused with "the save file changed since it was opened"; nothing is written.

## Opt-in for the device test

The default remains the hardware-accepted direct handoff. To test the return host, create an empty file:

```
sdmc:/switch/PokeBank-NX/config/experimental_return_host.enable
```

Delete it to go back to the direct path. No other behaviour depends on it.

## Final Switch checklist — owner A/B procedure (exact integrated NRO)

Use the integrated #307 candidate NRO that contains this branch. Record the NRO SHA-256 with the results.

Record for each step: PASS/FAIL, any error code, and the contents of `sdmc:/switch/PokeBank-NX/runtime/return-host.log` after the step.

**A. Default path (no marker file)**

1. Launch a Gen I–III game through RetroArch. It must open the correct game.
2. Quit RetroArch. Expect the Homebrew Menu, or HOME when PokeBank was started from a forwarder. This is the unchanged accepted behaviour.
3. Reopen PokeBank NX. Product Home must have the game you launched selected, with the same source.
4. Launch a Gen IV/V game through DraStic. Pick "Quit to launcher" in DraStic. Expect DraStic's launcher (unchanged).
5. Reopen PokeBank NX. The launched DS game must be selected.

**B. Return host (marker file present)**

6. Launch Red in RetroArch.
   - If it crashes on launch, send the error code and the log. The log names the last stage reached: `start` → `load hop=1 rc=…`.
7. Quit RetroArch. PokeBank NX must reopen with Red selected. The log must end with `return reason=chain-ended`.
8. Repeat 6–7 for Crystal (gambatte) and Emerald (mGBA).
9. In RetroArch, use Close Content, then Quit. It must still return to PokeBank.
10. Launch Platinum in DraStic, then pick "Quit to launcher". PokeBank NX must reopen with Platinum selected, and the log must show `return reason=launcher-requested`.
11. Launch Black (Gen V) through DraStic and repeat step 10.
12. Delete the marker file and repeat step 1. The direct path must still work.

For every launch, confirm that the save file's SHA-256 is unchanged by PokeBank. PokeBank never writes emulator saves; only the emulator does.

### Reading `return-host.log` (B steps)

The log is rewritten for every hosted launch. Its **last line** shows where a failure happened:

| Last line | Meaning | Next engineering step |
|---|---|---|
| (file missing) | The host never ran, or could not mount the SD card. Check the PokeBank log for `Launch handoff via return host` / `Return host unavailable`. | Outer loader or host install |
| `refused …` | Protocol or loader environment rejected; the host exited cleanly. | Argv or loader mismatch |
| `start …` / `environment …` | Crashed while probing the environment, or before loading. | Compare with the env values logged |
| `load hop=N rc≠0 stage=X` | Host failed at step X (`open`, `read-image`, `map-code`, `perm-*`) and returned to PokeBank. | That syscall or mapping step |
| `enter hop=N …` | Host handed control to the child, and the child crashed or never returned. | Child environment (heap, applet type, exception chaining) |
| `child-exit … next=(none)` then `return reason=chain-ended` | Correct RetroArch quit path. | — |
| `return reason=launcher-requested` | Correct DraStic Quit-to-launcher interception. | — |

`698ea4d1`'s device crash had no log. One B6 run of this build splits it into "host stage X failed" versus "child faulted after `enter`".

## Proven vs. unproven

| Claim | Status |
|---|---|
| Argv and paths are bounded and quoted exactly as libnx parses them | Host-tested (`libnxSplitArgv` port) |
| The default launch is byte-identical to the accepted direct handoff | Host-tested (plan argv equals the old `quoted` output for paths without `"` or `\`) |
| The host's decisions (quit, restart, DraStic launcher intercept) | Host-tested (pure policy) |
| Return focus record: round trip, staleness, corrupt file, consume-once, game no longer in catalog (silent drop), bounded indices | Host-tested, including the production file path |
| Return host starts RetroArch / DraStic on device | **UNPROVEN.** `698ea4d1` crashed; root cause not identified. Opt-in only. |
| DraStic emulator runs nested (JIT + fastmem exceptions chained through the host) | **UNPROVEN on device** |
| Quit returns to PokeBank on device | **UNPROVEN on device** |

## Requests for other threads (via coordinator)

- **2R (UI copy):** to tell users where Quit goes before launching, show `gameLaunchQuitRouteNotice(previewGameLaunchQuitRoute(launchDescriptor))` in the launch confirmation or notice. This lives in SaveSelectScreen, which this thread does not edit.
- **2R:** `tests/test_game_hub_contract.py` is shared. Only its launch block was changed here, to the new plan-based strings, with the same intent: a direct core handoff by default and the host honouring child chaining.

## Gates (this checkpoint)

Application code SHA `86c3490f97b4d8dfdb65084eced46d05681f611c`. The later commit adds docs and one test assertion; that assertion was also run under GCC.

| Gate | Result |
|---|---|
| `make -f Makefile.host host-test` (GCC 13) | PASS |
| `make -f Makefile.host host-sanitize` (GCC 13, ASan/UBSan) | PASS |
| `make -f Makefile.host host-test CXX=clang++` (Clang 18) | PASS |
| `make -f Makefile.host host-sanitize CXX=clang++` | PASS (after installing the container's missing `libclang-rt-18-dev`; the first run failed to link every sanitize binary for that reason alone) |
| All 20 `tests/test_*.py` contracts | PASS |
| `git diff --check` | PASS |
| Native devkitA64 (pinned `sha256:1fc388c3…`) `make`, including `runtime/return_host` | PASS. `PokeBankNX.nro` 24,250,505 B, SHA-256 `4f4445215eab76921424046205604d2825479f94cabb5999fc7467b15e60e216`. Built without the recovered RomFS assets, so it is **not** a device candidate; CI builds the candidate. |

## Next

1. Thread 2R forward-merges this branch into #307. The candidate NRO comes from #307's integrated build.
2. Owner device round: procedure A, then B.
3. If B6 crashes, read the `return-host.log` stage, then narrow the host fault from it (`load` reached or not, `child-exit` reached or not).
