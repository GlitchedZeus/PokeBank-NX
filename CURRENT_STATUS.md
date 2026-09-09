# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-09

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


## Current application source

Published and remote-verified application source:

```text
92bde34d1586990aaa82adc4f60d42d7bc6b5bdf
legacy: safely replace profile binding database on Switch
```

Application tree at that source checkpoint:

```text
08215dcdae0959685eae3796a63d67e76c2b49da
```

The LeafGreen binding persistence **source fix is complete**. Do not redo the implementation unless new device evidence proves a new defect.

The binding writer now:

- writes and validates temporary output;
- flushes/fsyncs/closes before rename/delete;
- rotates the old valid database to `.bak`;
- promotes the new file only into an absent target;
- verifies exact readback;
- recovers from a valid backup;
- does not promote speculative `.tmp` data;
- preserves invalid primary data instead of silently overwriting it;
- restores the entire prior in-memory binding map if persistence fails;
- logs exact failure stage, errno and Switch fsdev last Result where available.

Focused evidence for `92bde34d...`:

```text
first assignment save: PASS
second assignment under EEXIST-style rename restriction: PASS
reload / same-profile / split-profile isolation: PASS
failure-injection / rollback / backup recovery: PASS
focused binding ASan/UBSan: PASS
write-policy/source-mutation checks: PASS
git diff --check: PASS
changed binding translation unit devkitA64 -fno-exceptions/-fno-rtti: PASS
```

No source-save write path changed. Live installed-game and RetroArch writing remains **HARD DISABLED**.

## RECOVERY policy is now deterministic

Routine recovery no longer means manually inspecting every worktree/reflog/history first.

Authoritative recovery files:

```text
docs/RECOVERY_CONTRACT.md
recovery/RECOVERY_STATE.json
tools/recover_workspace.py
```

Normal recovery command:

```bash
python3 tools/recover_workspace.py
```

Expected result:

```text
RECOVERY COMPLETE
Generated asset preflight: PASS
Active task can continue: YES
```

The script reconstructs missing generated RomFS inputs from GitHub-tracked source pins/scripts, restores tracked game-card art, applies tracked recovery overrides and runs the offline asset preflight.

Forensic status/reflog/worktree archaeology is now an exception path only if this deterministic GitHub recovery path fails or the user explicitly asks to rescue unsaved local-only work.

Permanent save rule: no project-authored fix may exist only in `romfs/`, `build/`, `/mnt/data/`, a temporary Codex worktree or an unpushed commit. A manual generated-asset correction must be promoted into tracked generator/mapping/transform logic or a legally-safe tracked recovery override before a session is called saved.

## Historical full visual baseline

Physically tested application source:

```text
ea0b806bac4acdb5619f22f9841d616ea8a237ff
legacy: bind FRLG sources to profiles and expose diagnostics
```

Canonical tree:

```text
ed5912093886384894c44538d569fe4955fd2e47
```

Exact physically tested artifact:

```text
PokeBank-NX-FRLG-Corrected-Retest-ea0b806b.nro
size: 156,592,377 bytes
SHA-256: 396f8ff9f4da53b5449aeb46b8d1237ca9358a0ac94998680017575916b6b1ee
embedded: 0.1.0-alpha / ea0b806b
```

Verified build context for that artifact:

```text
HD renders: 3,260 / 3,260
Base species: 1,025 / 1,025
Type icons: 18 / 18
Fonts: 3 / 3
FRLG GBA cards: PASS
Asset preflight: PASS
Embedded RomFS: 3,283 / 3,283 files byte-identical
Native devkitA64 -fno-exceptions build: PASS
Host tests: 13 suites PASS
ASan/UBSan: PASS
git diff --check: PASS
GitHub application CI run #245: PASS
```

The user still has this exact historical NRO on the physical Switch. It is an independent fallback/reference copy, but normal recovery must not depend on re-uploading it.

The historical NRO does **not** contain the new `92bde34d...` persistence fix.

## Physical Switch result — FireRed

Physically confirmed on the exact `ea0b806b` artifact:

- normal RetroArch battery/in-game `.srm` is the correct source;
- current FireRed save rereads correctly after a normal in-game save;
- current save opens with 2 Pokémon;
- trainer information opens correctly;
- items/inventory open correctly;
- truthful source diagnostics are present;
- `.state` savestate support is not required and must not be added as a workaround.

FireRed is not the blocker.

## Physical Switch result — LeafGreen before the new fix

Regression fixture to preserve unchanged:

```text
File: Pokemon - Leaf Green Version.srm
Game: LeafGreen
Trainer: Will
Party count: 1
Displayed fingerprint prefix: d76e3c7e25a4
```

On `ea0b806b`, discovery/parsing succeeded but assignment failed with:

```text
Assignment could not be saved; source remains unassigned.
```

That failure was narrowed to binding persistence, reproduced under rename-over-existing restrictions, and addressed by `92bde34d...`.

The **new source has not yet been physically tested**.

## Immediate next milestone

Do only:

```text
run deterministic recovery
        ↓
full 3,260-render / 18-type / 3-font preflight PASS
        ↓
build exact 92bde34d application source
        ↓
verify embedded RomFS
        ↓
package/hash new retest NRO + ZIP
        ↓
STOP for physical Switch retest
```

Do not restart parser/discovery work. Do not start RSE or another roadmap feature.

## Required physical retest

1. FireRed current 2-Pokémon source still opens correctly.
2. LeafGreen can be assigned successfully.
3. Restart PokeBank NX and confirm LeafGreen assignment persists.
4. Test profile isolation.
5. Switch back and confirm the original profile assignment remains.
6. Recheck Trainer, Items, Party, Boxes 1-14, Pokémon View, Refresh and read-only locks.

Only after this passes may FRLG be marked physically accepted.

```text
DEVICE TESTED FOR 92bde34d SOURCE: NO
DEVICE ACCEPTED: NO
```

## Parked RSE recovery

Preserve:

```text
b5ef83b
```

and `1a921515` if available or its verified equivalent.

Do not merge, resume, reimplement or push RSE until FRLG physical acceptance.

## Repository authority

```text
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream reference: kiasta/PKSE
```

Never push custom PokeBank NX code upstream.

## Fast launcher

Normal coding:

```text
Continue PokeBank NX on feature/pokebank-playable. Use HIGH reasoning. Read CURRENT_STATUS.md and docs/CODEX_SESSION.md, then execute docs/NEXT_CODEX_PROMPT.md exactly.
```

Recovery:

```text
RECOVERY. Use origin/feature/pokebank-playable as the normal source of truth. Read docs/RECOVERY_CONTRACT.md, CURRENT_STATUS.md and docs/NEXT_CODEX_PROMPT.md, then run `python3 tools/recover_workspace.py`. If it prints RECOVERY COMPLETE, continue immediately. Do not inspect reflogs/worktrees/history unless that deterministic path fails or I explicitly ask to rescue unsaved local-only work.
```
