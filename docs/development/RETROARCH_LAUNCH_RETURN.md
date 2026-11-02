# RetroArch launch and return (Gen I–III): investigation, Round 15 lane `retroarch`

Status: **Open: Needs Hardware Evidence.** This lane does not fix the crash on quit. The code change
only adds diagnostics, plus one honest pre-launch warning. Nothing here is "Fixed in Code" for R04.

Base `fix/round14-gen15-repairs` (`07fa5014`). Branch `r15/retroarch`.

## The owner's report (Round 13 R04, verbatim)

> "retroacrch games so gens 1-3 crash on quit in retroarch, would be nice if we could have the
> return feature that we had but im not 100 hopeful, drastic wil always to to the launcher menu for
> drastic so no return to pokebank so not mad, dekopon we aint there yet and native games i dont care"

Earlier reports of the same item:
- PR #92 (2026-10-06, `97f540e4`): "quitting RetroArch drops/crashes to Nintendo HOME instead of
  returning to PokeBank NX".
- Round 3 AUD-04: NOT TESTED.
- Ledger `PBX-LQT-004`: Needs Investigation.

Scope: DraStic (Gen IV/V) Quit to launcher is out of scope by the owner's decision (`PBX-LQT-005`).
So are Dekopon and native Switch titles.

## 1. How PokeBank launches a Gen I–III RetroArch game today

| Step | Where | What happens |
|---|---|---|
| PokeBank records its own NRO path | `src/main.cpp:37` | `UI::setGameLaunchReturnPath(argv[0])`. A path starting with `/` is normalised to `sdmc:/…` (`src/UI/GameLauncher.cpp`, `setGameLaunchReturnPath`). |
| Core chosen | `src/UI/GameLauncher.cpp:618-625`, `:653` | `sdmc:/retroarch/cores/gambatte_libretro_libnx.nro` (GB/GBC) or `mgba_libretro_libnx.nro` (GBA, or GB fallback). Save evidence (per-core save folder, `.srm` location) can choose another core (`retroArchSaveCoreEvidence`). |
| Plan built | `include/UI/GameLaunchModel.h:680-743` `buildGameLaunchChainPlan` | Default: `nro = <core>` and `argv = "<core>" "<rom>"`. Paths are bounded to hbloader's 512-byte path and 2048-byte argv buffers. Values containing `"` are refused, because libnx `argvSetup` has no escape character. Spaces and UTF-8 such as `Pokémon` pass through verbatim (host-tested with the libnx tokenizer port in `tests/test_game_launch_model.cpp`). |
| Handoff | `src/UI/GameLauncher.cpp:1421` | `envSetNextLoad(plan.nroPath, plan.argv)`, then PokeBank exits normally. UI, SDL, RomFS, account and ns are torn down in `src/main.cpp`. The loader that started PokeBank then maps the core NRO. |
| Quit route shown before launch | `GameLaunchModel.h` `gameLaunchQuitRouteNotice` | "Quitting returns to the Homebrew Menu (or HOME via a forwarder)." |
| Return host (opt-in only) | `src/UI/GameLauncher.cpp:41`, `runtime/return_host/` | Used only when `sdmc:/switch/PokeBank-NX/config/experimental_return_host.enable` exists. Then the loader runs `PokeBankReturnHost.nro`, which runs the core nested and reloads PokeBank when the core exits with nothing scheduled. |

Installed Switch titles use `appletRequestLaunchApplication` (`src/UI/GameLauncher.cpp:1382`). That
path is unrelated to R04.

## 2. History of the "return feature that we had" (from `git log -S`)

| Commit | Date | What |
|---|---|---|
| `3a9cd893` | 2026-10-05 | First direct emulator launch: `envSetNextLoad(core, argv)`. No return. |
| `98727a5b` | 2026-10-06 | **Return host added and made the default:** "fix(launch): return from RetroArch to PokeBank". |
| `698ea4d1` (candidate) | 2026-10-07 | Owner: Gen I–III RetroArch **crash on launch** with the return host on the default path (`PBX-LQT-006`). |
| `8803c9ea` | 2026-10-07 | "restore hardware-safe RetroArch startup": back to the direct handoff. The return host stays bundled but unused. |
| `86c3490f`, `95255c97` | 2026-10-15 | Return host rebuilt as an **opt-in** behind the marker file. Changes: a pure, host-tested policy (`return_host_policy.h`); SM closed before children run, as nx-hbloader does; no backslash escaping; a stage log at `sdmc:/switch/PokeBank-NX/runtime/return-host.log`. Its own commit message says "NOT DEVICE TESTED". |
| since then | — | No owner run with the marker has been recorded (AUD-04 NOT TESTED in Round 3; not on any later checklist). |

So the return feature reached the owner exactly once (`698ea4d1`), and it crashed at launch. It has
never returned to PokeBank on hardware. The opt-in rebuild has never been tried on the device.

## 3. What happens on Quit: upstream source evidence

Pinned revisions. All were fetched from raw.githubusercontent.com in this lane, and each SHA was
checked to resolve.
- libretro/RetroArch `b2538f1249e81d687dbcaece491674713a38ae68`. Current master is
  `54d27c523ad13b31f2d5692a08335c150c9a09ac`; `frontend_switch_exitspawn` and
  `frontend_switch_deinit` are identical in both.
- switchbrew/nx-hbloader `82b95122c5ae8dc059bf23893ba7623c72c86773`. This is still `HEAD`.
- switchbrew/libnx `20cbcfd90567f7bf84df7725f27e345e2164fa6b` (`env.c`, `argv.c`). `HEAD` is now
  `b156b4774fb2df7b93402169509266da7547fc4c`; `envSetNextLoad` is unchanged in substance.

1. **RetroArch's Quit schedules nothing.** In
   [`frontend/drivers/platform_switch.c`](https://github.com/libretro/RetroArch/blob/b2538f1249e81d687dbcaece491674713a38ae68/frontend/drivers/platform_switch.c),
   `frontend_switch_exitspawn()` returns at once when `switch_fork_mode == FRONTEND_FORK_NONE` (lines
   413-430). Only core switches and Restart set a fork mode and call `envSetNextLoad` (`frontend_switch_exec`,
   lines 340-386). `frontend_switch_deinit()` (lines 310-337) only restores the CPU clock and exits
   nifm/psm/clkrst. It never touches the next-load buffers.
2. **The next step belongs to the outer loader.**
   [`nx-hbloader source/main.c`](https://github.com/switchbrew/nx-hbloader/blob/82b95122c5ae8dc059bf23893ba7623c72c86773/source/main.c),
   `loadNro()` lines 354-377: with an empty next-load path it loads `DEFAULT_NRO "sdmc:/hbmenu.nro"`.
   If that file cannot be opened, it calls `diagAbortWithResult(MAKERESULT(Module_HomebrewLoader, 3))`.
   That is **error 2347-0003**, a real crash screen. NSP forwarders ship their own loader builds,
   and those usually end the process, which drops to HOME. Both outcomes are outside PokeBank's
   control once RetroArch has quit.
3. **PokeBank's next-load is consumed, not stale.** hbloader clears `g_nextNroPath` as soon as it
   opens the core (line 379). Nothing PokeBank set survives into RetroArch's quit.
4. **RetroArch uses argv[0] as the core identity.** `frontend_switch_process_args()` (lines 697-711)
   adopts an `.nro` argv[0] as `RARCH_PATH_CORE`. PokeBank passes the core path itself there.
   Passing PokeBank's NRO as argv[0], to "trick" a return, would break RetroArch's core identity and
   libnx's RomFS mount. **Rejected.**

## 4. Root-cause candidates for "crash on quit", ranked

PokeBank's own code is not on the quit path: by the time RetroArch quits, PokeBank's image has been
unmapped and its next-load request consumed. I checked PokeBank's exit for state that could outlive
it:
- no threads (`threadCreate`, `std::thread` and `SDL_CreateThread` are not used);
- `plExit` after the NanoVG teardown;
- SDL, RomFS, account and ns exited in `src/main.cpp`;
- the save mounts (`fsdevMountSaveData` "save" and "pbpreview") are released by libnx's
  `fsdevUnmountAll` in `__libnx_exit`.

None of this survives into RetroArch. The candidates are therefore about the environment:

| # | Candidate | What the owner would see | Confidence | How to tell |
|---|---|---|---|---|
| A | PokeBank started from a **HOME forwarder (NSP)** whose loader ends the process when the chain ends | Drops straight to HOME, as if it crashed | High if PokeBank is on HOME | `last-launch.txt` `applet=application` with `loader=` not `nx-hbloader …`; no error code |
| B | Stock hbloader but **`sdmc:/hbmenu.nro` missing**, for example when another menu is used without the `hbmenu.nro` name | Error **2347-0003** after Quit | Medium | `last-launch.txt` `hbmenu=missing`; the launch dialog now warns before launch |
| C | **RetroArch itself faults during exit** (version, video/audio driver, shader, or applet mode with limited memory) | An Atmosphère error screen whose code is not 2347-xxxx; a crash report in `sdmc:/atmosphere/crash_reports/` | Medium-low | Launch the same core and ROM from hbmenu/RetroArch directly, without PokeBank, and quit. If it also crashes, it is RetroArch, not PokeBank |
| D | Applet (Album) mode memory limits | RetroArch warns at start; quitting may fail | Low | `last-launch.txt` `applet=applet` |

A and B are loader behaviour. C and D happen inside RetroArch. None of them is a defect in how
PokeBank builds the path or argv. The ROM path quoting is exact, and a stale next-load is impossible
(point 3 above).

## 5. What changed in code in this lane (diagnostics only; not a fix)

- `include/UI/GameLaunchModel.h` adds pure, host-tested helpers:
  - `GameLaunchLoaderFacts`, `gameLaunchLoaderInfoLine()`
  - `gameLaunchQuitExpectation()`, `gameLaunchQuitNotice()`
  - `formatGameLaunchHandoffRecord()`
- `src/UI/GameLauncher.cpp`:
  - `currentLoaderFacts()` (Switch only) reads `envGetLoaderInfo()`, `appletGetAppletType()`, whether
    `sdmc:/hbmenu.nro` exists, and PokeBank's own argv[0]. Every read is read-only.
  - After a successful `envSetNextLoad`, PokeBank writes **`sdmc:/switch/PokeBank-NX/runtime/last-launch.txt`**
    (rewritten for each launch, best effort, never blocks the launch). It holds the route, NRO,
    argv, ROM path, loader, applet type, hbmenu presence and the expected quit outcome.
  - The launch dialog's quit sentence is unchanged, except when the loader is stock nx-hbloader and
    `sdmc:/hbmenu.nro` is missing. In that case it reads "sdmc:/hbmenu.nro is missing: quitting may
    show error 2347-0003." The text fits the dialog alongside the longest core notice (host-tested).
- The launch itself (`nro`, `argv`) is byte-identical to before. The return host stays opt-in and
  unchanged. No save file is read or written by any of this.

Tests: `tests/test_game_launch_model.cpp` `testQuitExpectationAndHandoffRecord`. It fails to compile
on `07fa5014` because the helpers do not exist there, and passes after the change. Existing
`test_retroarch_launch_resolution` and `test_leave_decision_layout` (plain and sanitize) still pass.

## 6. Engineering requirement for a real return to PokeBank

The only mechanism that can return to PokeBank after a plain RetroArch Quit, without modifying
RetroArch or the owner's loader, is a nested loader: the existing return host. Before it can become
the default:
1. **R-RH-1 (device):** with the marker present, Red, Crystal and Emerald each launch, and Quit
   returns to PokeBank. `return-host.log` must end with `return reason=chain-ended`.
2. **R-RH-2 (device):** if launch crashes as `698ea4d1` did, the last line of `return-host.log`
   names the failing stage (see `docs/handoffs/LAUNCH_RETURN_PROGRESS.md`, "Reading return-host.log").
   Engineering fixes that stage. Until then the default stays direct.
3. **R-RH-3:** only after R-RH-1 passes on the owner's console may the default flip. The direct
   path must remain the fallback, and the launch dialog must keep promising only the route actually
   taken (`planGameLaunchHandoff`).
4. **Out of scope:** DraStic Quit to launcher (owner decision), patching RetroArch, or replacing
   the owner's hbmenu or forwarder.

## 7. Owner hardware steps (simple)

Before you start, write down:
- your **RetroArch version** (RetroArch main menu → Information → System Information, or the
  version at the top of the menu);
- **how you start PokeBank NX**: Homebrew Menu (hold R on a game), the Album, or a HOME-screen
  forwarder icon.

**Test 1: the normal path (no changes on the SD card)**
1. Open PokeBank NX, pick **Pokémon Red** (or any Gen I–III game), and launch it.
2. Before confirming, read the line in the launch box about where quitting goes. Write it down.
3. In RetroArch, open the menu and choose **Quit RetroArch**.
4. Report what you see: Homebrew Menu / HOME screen / an error screen. If there is an error
   screen, **write down the error code** (for example 2347-0003, or 2168-0002) and take a photo.
5. Send these files from the SD card:
   - `sdmc:/switch/PokeBank-NX/runtime/last-launch.txt`
   - the newest file in `sdmc:/atmosphere/crash_reports/`, if one was created at that time.

**Test 2: is it RetroArch or PokeBank?**
6. Start RetroArch on its own (not from PokeBank), load the same game, then Quit RetroArch.
7. Report whether it crashes the same way. If it does, the crash is RetroArch's, not PokeBank's.

**Test 3: the return feature (optional, experimental)**
8. Create an empty file named `experimental_return_host.enable` in `sdmc:/switch/PokeBank-NX/config/`.
9. Launch Red from PokeBank. The launch box should now say quitting returns to PokeBank NX.
10. Quit RetroArch. Report whether PokeBank NX reopens, crashes at launch, or crashes at quit.
11. Send `sdmc:/switch/PokeBank-NX/runtime/return-host.log` and `last-launch.txt`.
12. Delete the `.enable` file afterwards to go back to the normal path.

PokeBank never writes emulator saves in any of these steps. RetroArch itself writes its `.srm` when
you quit, as it always does.
