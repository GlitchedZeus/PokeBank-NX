## Recovery complete; exact-source native link blocked (2026-09-09)

Application source remains **92bde34d1586990aaa82adc4f60d42d7bc6b5bdf**, tree **08215dcdae0959685eae3796a63d67e76c2b49da**. No application source was modified in this recovery/build session.

**RECOVERY COMPLETE.** GitHub snapshot commit: **a2adac94f15504b90a83a295e77ad54154da4206**. The prescribed tools/recover_workspace.py and tools/pack_recovery_snapshot.py ran successfully in GitHub Actions after local HTTPS clone authentication was unavailable and workspace pruning interrupted the local recovery. All 3,260 HD renders, 1,025 base species, 18 type icons, 3 fonts, both FRLG cards and all 3,283 files passed. Every PNG decoded successfully. Both snapshot parts and the manifest were pushed and remote-verified. A fresh build runner then restored the committed snapshot successfully without external sprite regeneration.

Snapshot archive: 150,712,320 bytes; SHA-256 **0c9b85da78fdb7ee06187d8be787426a198803e9fb5e866672a13d27ee249f2a**. See recovery/assets_snapshot/manifest.json for each part's size/hash. The complete snapshot is in normal private Git history, as requested; no NRO/ZIP was committed there.

**BUILD BLOCKED — final native link**, not asset recovery. Clean build of the exact application commit under devkitA64 GCC 15.2.0 and normal -fno-exceptions flags compiled the application translation units, then failed with:

    LegacySourceBindings.o: in function PokeVault::Legacy::LegacySourceBindings::fail(char const*) const:
    src/Legacy/LegacySourceBindings.cpp:112: undefined reference to fsdevGetLastResult()
    collect2: error: ld returned 1 exit status

Build run: https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/34404103716
Recovery/snapshot run (PASS): https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/34403768782
Build diagnostics (native log, toolchain image digest and package versions): https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/34404103716/artifacts/10124695368

The previously recorded focused host/sanitizer and compile-only evidence remains historical; it did not prove a full native link. Do not call this source NRO BUILDS. No replacement NRO/ZIP/manifest was packaged or released. Embedded-final-RomFS comparison was not run because linking failed. Do not relabel the historical ea0b806b artifact.

Next authorized coding task, in a source-fix session, is the narrow native symbol/linkage issue for fsdevGetLastResult; verify C/C++ linkage against libnx before choosing the fix. This is not another binding persistence investigation. If runtime source changes, publish a new application SHA and adjust the exact-source build gates instead of labelling it 92bde34d. Do not rerun asset downloads; the full snapshot now exists. Do not resume RSE. Parked local work/refs were not altered.

DEVICE TESTED FOR NEW ARTIFACT: NO
DEVICE ACCEPTED: NO

