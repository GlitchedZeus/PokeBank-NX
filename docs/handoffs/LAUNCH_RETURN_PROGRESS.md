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

## Opt-in for the device test

The default remains the hardware-accepted direct handoff. To test the return host, create an empty file:

```
sdmc:/switch/PokeBank-NX/config/experimental_return_host.enable
```

Delete it to go back to the direct path. No other behaviour depends on it.

## Owner test procedure (exact NRO from this branch)

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

## Proven vs. unproven

| Claim | Status |
|---|---|
| Argv and paths are bounded and quoted exactly as libnx parses them | Host-tested (`libnxSplitArgv` port) |
| The default launch is byte-identical to the accepted direct handoff | Host-tested (plan argv equals the old `quoted` output for paths without `"` or `\`) |
| The host's decisions (quit, restart, DraStic launcher intercept) | Host-tested (pure policy) |
| Return focus record: round trip, staleness, consume-once | Host-tested, including the production file path |
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
