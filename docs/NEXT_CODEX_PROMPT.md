# Current stop state — native link blocker

Read CURRENT_STATUS.md first. Recovery and the complete private GitHub RomFS snapshot are DONE at a2adac94f15504b90a83a295e77ad54154da4206. A fresh runner successfully restored it. Do not repeat external sprite restoration.

The exact 92bde34d application clean build failed at final linking with undefined reference to fsdevGetLastResult() from LegacySourceBindings.cpp:112. Translation-unit compilation had passed. See run 34404103716 and the current status for preserved logs/toolchain identity.

Do not claim or package a 92bde34d NRO. A future source-fix session should check the libnx C/C++ declaration/linkage and publish any necessary narrow correction as a NEW application source SHA, then update the pinned source/tree/embedded-SHA build gates. No persistence redesign, RSE or roadmap work. This recovery/build-only session stopped without changing runtime source.

DEVICE TESTED FOR NEW ARTIFACT: NO
DEVICE ACCEPTED: NO

---

## Prior recovery/build instructions (recovery completed; link failed)

# PokeBank NX — Authoritative Next Codex Prompt

Use HIGH reasoning.

## Current source checkpoint

The LeafGreen binding persistence source fix is already implemented, tested and pushed:

```text
92bde34d1586990aaa82adc4f60d42d7bc6b5bdf
legacy: safely replace profile binding database on Switch
```

Do **not** redo the binding investigation or implementation unless a new physical test proves a new defect.

Focused binding tests, ASan/UBSan, write-policy/source-mutation checks and whitespace checks passed. The changed binding translation unit also compiles with devkitA64 `-fno-exceptions/-fno-rtti` and the project's POSIX flags.

## Single mission for the next session

**Recover the complete generated build context, save a complete verified RomFS snapshot into the private GitHub repository, build/package the exact new LeafGreen retest NRO, then STOP for physical Switch testing.**

Do not start RSE or another roadmap feature.

## RECOVERY is now one command

Read:

```text
docs/RECOVERY_CONTRACT.md
recovery/RECOVERY_STATE.json
```

Then run:

```bash
python3 tools/recover_workspace.py
```

Normal expected behavior:

```text
try complete committed GitHub RomFS snapshot
        ↓
if not populated yet, restore missing pinned HD renders/types/fonts
        ↓
copy tracked game-card art
apply tracked recovery overrides
run device asset preflight
        ↓
RECOVERY COMPLETE
```

Do **not** begin with reflog/worktree/stash archaeology. Use forensic recovery only if this deterministic GitHub path actually fails or the user explicitly asks to rescue unsaved local-only work.

A previous workspace had only 1,200/3,260 generated HD renders after maintenance. That is no longer a reason to stop by itself. Reconstruct the full tree first.

Historical full visual baseline for comparison:

```text
PokeBank-NX-FRLG-Corrected-Retest-ea0b806b.nro
source: ea0b806bac4acdb5619f22f9841d616ea8a237ff
size: 156,592,377 bytes
SHA-256: 396f8ff9f4da53b5449aeb46b8d1237ca9358a0ac94998680017575916b6b1ee
HD renders: 3,260/3,260
base species: 1,025/1,025
type icons: 18/18
fonts: 3/3
embedded RomFS: 3,283/3,283
```

The user still has this historical NRO on the Switch, so it is an independent fallback/reference. Normal recovery must not require the user to re-upload it.

## One-time permanent asset snapshot — REQUIRED this session

After `tools/recover_workspace.py` reports a full passing tree, run:

```bash
python3 tools/pack_recovery_snapshot.py
```

It will create GitHub-safe 80 MiB chunks under:

```text
recovery/assets_snapshot/
```

Then:

```bash
git add -f recovery/assets_snapshot/
git commit -m "recovery: snapshot complete verified RomFS"
git push origin feature/pokebank-playable
```

Remote-verify that the manifest and **every listed part** exist before continuing.

This is the permanent fix for the disappearing Codex asset cache. After this one-time snapshot is pushed, future routine recovery should restore the complete RomFS directly from the private GitHub repo; network regeneration is only fallback.

Do not call the session safely saved if the full generated tree was rebuilt but the snapshot parts were left only in the workspace.

## Build/package after recovery snapshot is safely on GitHub

1. prepare/build the exact application source containing `92bde34d...` without changing application code;
2. run the normal native devkitA64 `-fno-exceptions` build;
3. rerun the device asset preflight if the build workflow changes/generated files;
4. verify the final embedded RomFS against the recovered source asset tree;
5. package the new retest `.nro` and `.zip`;
6. record exact filename, size and SHA-256;
7. update `CURRENT_STATUS.md` and GitHub issue #6;
8. push/remote-verify all project-authored source/docs/recovery changes;
9. STOP for the user's physical Switch retest.

Do not rerun the full PKSM-Core suite solely for this binding-config change. Reuse already valid unchanged-source evidence where appropriate.

## Required physical retest

The new NRO is not accepted until the user physically verifies:

1. FireRed current source still opens correctly.
2. LeafGreen can be assigned successfully.
3. Restart PokeBank NX and confirm LeafGreen assignment persists.
4. Test profile isolation.
5. Switch back and confirm the original profile assignment remains.
6. Recheck Trainer, Items, Party, Boxes 1-14, Pokémon View, Refresh and read-only locks.

Preserve the user's existing LeafGreen regression fixture:

```text
Pokemon - Leaf Green Version.srm
Trainer: Will
Party: 1
Fingerprint prefix: d76e3c7e25a4
```

Do not recreate, edit or replace that save.

## Parked RSE

Keep `b5ef83b` (and `1a921515` if available) parked. Do not merge/resume/reimplement/push RSE until FRLG physical acceptance.

## Strictly out of scope

Do not begin:

- RSE;
- Gen I/II;
- DS/3DS;
- modern Switch expansion;
- Vault/Banks;
- Create Pokémon/editor;
- legality/conversion;
- events/Gifts/Trade;
- Android/social;
- broad UI redesign;
- arbitrary RetroArch `.state` parsing;
- any live save writing.

## STOP condition

Stop only after either:

### A. New retest artifact produced

```text
RECOVERY COMPLETE
complete RomFS recovery snapshot pushed + remote-verified on GitHub
source fix remains pushed
new exact NRO packaged + hashed
embedded RomFS verified
DEVICE TESTED FOR NEW ARTIFACT: NO
DEVICE ACCEPTED: NO
```

or, only if recovery/build truly fails:

### B. Exact blocker reported

```text
RECOVERY BLOCKED or BUILD BLOCKED
exact failed stage
exact reason
no vague "build context is gone" wording
DEVICE ACCEPTED: NO
```

Do not roll directly into RSE.
