# Application proposal — exact #307 head, separate from the engine push

App base: `9d4b8b71befea78c76802c15f1911a0ca24e775c`. `app-integration.json` binds each original and proposed file by SHA-256. `prepare_app_patch.py` refuses another HEAD and writes a narrow three-file patch without changing the app checkout. This patch is **not applied to the active app branch**. Recompute it when #307 advances; never force it through conflicts.

1. Selectively review/import the legality implementation and data prerequisites from #314. This includes the common Report/Context model, native generation analyzers, Gen5AppAdapter, Gen5WorkspaceAdapter, ReportPresentation and their dependency closure. The current app's old Legality.h is incompatible with the new engine. Do not replace the app's native Pokemon/crypto/workspace headers with older engine-lane versions. The bridge test uses only a legality-header overlay and the current app's native types.
2. The Gen V surface calls `Gen5App::analyzeWorkspace` on the actual current staged slot. It supplies exact source identity, party/box location and source trainer, not a guess from the Pokemon's stored origin. It preserves the stored-origin/source-mutation rows and modal ownership, adds controller/touch report navigation, and resets scrolling when opened. It performs no staging, save export or source write.
3. The existing PokemonDetailsModal summary, footer and scroll overlay read `Report::verdict()`. Incomplete always says Incomplete / Unknown, even with zero warnings; its coverage rows remain visible. The current touch clipping, scrollbar, drag handling and close control are preserved. Opening the report uses the same exact source id as rendering and does not borrow the open save's identity for Bank targets.
4. Gen III berry evidence is an optional production API, tested through `Legality::analyze(Pokemon3FRLG, Context)`. The app must explicitly create it from the actual validated source bytes and the save's known language, and attach it only to that source's Pokemon. This proposal does not yet add that provider plumbing; context-free Enigma results remain Unknown.

Executed: exact-head patch application check; real #307 native save/workspace -> production bridge for all four Gen V titles, staged key-item Invalid, source immutability and bounds, GCC normal and Clang ASan+UBSan with leak detection. The helper consumes decoded staged bytes, so a staged edit cannot accidentally be judged using the original slot.

The rendered-line model passed all 59,049 coverage/severity combinations with GCC and Clang normal and sanitizers. This is not a Switch screenshot or native renderer acceptance test. The native UI translation units/NRO and physical Switch display have not been built/tested here; the environment has no devkitA64/container runtime. Recheck the app's latest native CI and perform UI acceptance before applying the proposal. Independent reviewer approval and a conflict-reviewed dependency import remain outstanding. This handoff is not permission to merge the entire campaign or main.

Reproduce:

```sh
python3 tools/legality_completion/prepare_app_patch.py /path/to/exact-app-checkout docs/legality/completion-20261019/app-integration.patch
git -C /path/to/exact-app-checkout apply --check /absolute/path/to/app-integration.patch
bash tools/legality_completion/test_app_bridge.sh /path/to/exact-app-checkout
CXX=clang++ APP_BRIDGE_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer -O1' APP_BRIDGE_BUILD=/tmp/bridge-clang bash tools/legality_completion/test_app_bridge.sh /path/to/exact-app-checkout
```

Final refresh: #307 advanced from 678627f5 to 9d4b8b71 during the session. The three proposal files and the app headers consumed by the compiled bridge are byte-identical between those heads; the newer Gen I Box Move and QA contract changes are preserved. Patch/header dependency identity was rechecked and both already-built bridge binaries rerun. The final proposal/CI fixture now binds 9d4b8b71, rather than the initial snapshot.
