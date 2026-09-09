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

**Recover the generated build context deterministically from GitHub, build/package the exact new LeafGreen retest NRO, then STOP for physical Switch testing.**

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
restore missing pinned HD renders
restore pinned type icons
restore fonts
copy tracked game-card art
apply tracked recovery overrides
run device asset preflight
        ↓
RECOVERY COMPLETE
```

Do **not** begin with reflog/worktree/stash archaeology. Use forensic recovery only if this deterministic GitHub path actually fails or the user explicitly asks to rescue unsaved local-only work.

A previous workspace had only 1,200/3,260 generated HD renders after maintenance. That is no longer a reason to stop by itself: `romfs/` is generated, and the recovery recipe is now tracked on GitHub. Reconstruct it first.

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

## Build/package after recovery

Once `tools/recover_workspace.py` prints `RECOVERY COMPLETE`:

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
