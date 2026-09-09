# PokeBank NX — Current Verified Engineering State

Last updated: 2026-09-09

## LeafGreen binding replacement fix — source checkpoint (2026-09-09)

The binding writer now uses verified temporary output, explicit flush/fsync/close, old-to-backup rotation, promotion to an absent target and exact readback. The previous valid database remains recoverable at the target or .bak throughout replacement. Startup prefers a valid primary, recovers a missing/invalid primary from a valid backup, and never promotes speculative .tmp data. Invalid primary data is preserved and blocks further writes rather than being silently overwritten.

assignAndSave() restores the entire prior in-memory assignment map on persistence failure. SaveSelectScreen logs the exact failure stage, errno and (on Switch) fsdevGetLastResult; the native last-result field may be stale for local validation errors. No source save write path changed.

Evidence: old ea0b806b writer reproduced first-save success / second-save failure using an EEXIST-on-existing rename shim. The replacement passes first/second assignments, same/split-profile reload isolation, repeated aliases, separate source identities, failure checkpoints, real open failure, corrupt temp/target readback, failed rollback, stale tmp and backup recovery. All persistence-owned file handles close before rename/delete. No device errno/native Result or external-open-handle evidence has yet been captured; actual Switch root cause/acceptance remains pending retest.

Verification: focused binding tests PASS; focused binding ASan/UBSan PASS with -fno-exceptions/-fno-rtti; existing write-policy/source-mutation tests PASS; git diff --check PASS. Full native build and device artifact remain pending the post-checkpoint build gate. Do not rerun the full PKSM-Core suite solely for this config-file change.

Workspace maintenance removed the prior partial patch staging tree. Its small change was recovered from the recorded patch and hardened; surviving FRLG local changes and b5ef83b RSE ref were preserved. No LeafGreen .srm bytes were available in this runtime: preserve the user's existing fixture (Will, party 1, fingerprint prefix d76e3c7e25a4); no personal save was recreated or changed. RSE remains parked.

DEVICE TESTED FOR NEW SOURCE: NO
DEVICE ACCEPTED: NO



This is the short authoritative engineering handoff for coding sessions. The root `README.md` is the human-facing project dashboard. Historical snapshots and older device/build/session records are preserved under `docs/` and `docs/history/`.

## Authority / repository

```text
Repository: GlitchedZeus/PokeBank-NX
Development branch: feature/pokebank-playable
Writable remote: origin
Upstream reference: kiasta/PKSE
```

Never push custom PokeBank NX code upstream.

Live installed-game and RetroArch save writing remains **HARD DISABLED**.

Before any sync/reset/clean/rebase/ref/worktree/generated-asset action, inspect and preserve useful local/uncommitted/recovery/build state first.

## Current physically tested application source

```text
ea0b806bac4acdb5619f22f9841d616ea8a237ff
legacy: bind FRLG sources to profiles and expose diagnostics
```

Canonical application tree:

```text
ed5912093886384894c44538d569fe4955fd2e47
```

Corrected physical-test artifact:

```text
PokeBank-NX-FRLG-Corrected-Retest-ea0b806b.nro
size: 156,592,377 bytes
SHA-256: 396f8ff9f4da53b5449aeb46b8d1237ca9358a0ac94998680017575916b6b1ee
embedded: 0.1.0-alpha / ea0b806b
```

Artifact/build verification already completed:

```text
HD renders: 3260 / 3260
Base species: 1025 / 1025
Type icons: 18 / 18
Fonts: 3 / 3
FRLG GBA cards: PASS
Asset preflight: PASS
Embedded RomFS: PASS — 3283 / 3283 files byte-identical
Native devkitA64 -fno-exceptions build: PASS
Host tests for ea0b806b: 13 suites PASS
ASan/UBSan for ea0b806b: PASS
git diff --check for ea0b806b: PASS
GitHub application CI run #245: PASS
```

Do not rerun expensive unchanged-source verification merely because a new session starts. If application source changes, run verification appropriate to the changed source.

## Physical Switch result — FireRed GBA

The earlier stale-save concern is now understood and the normal RetroArch in-game save path works.

Physically confirmed on the exact `ea0b806b` artifact:

- normal RetroArch battery/in-game `.srm` save is discovered;
- after saving normally inside FireRed, PokeBank rereads the current save;
- current FireRed save opens with **2 Pokémon**;
- trainer information opens correctly;
- item/inventory information opens correctly;
- truthful source diagnostics are present;
- `.state` savestate support is **not** required and must not be added as a workaround.

FireRed is not the current blocker.

## Physical Switch result — LeafGreen GBA

The physical LeafGreen source is successfully discovered and parsed:

```text
File: Pokemon - Leaf Green Version.srm
Game: LeafGreen
Trainer: Will
Party count: 1
Displayed fingerprint prefix: d76e3c7e25a4
```

Attempting assignment to either available Nintendo/PokeBank profile fails with:

```text
Assignment could not be saved; source remains unassigned.
```

The remaining FRLG blocker is therefore **binding persistence**, not LeafGreen discovery or parsing.

## Known failure path

Code inspection established that the exact UI error above is reached only after:

```text
LegacySourceBindings::assign(...)
        -> succeeds in memory
LegacySourceBindings::save()
        -> returns false
```

Actual binding database path:

```text
sdmc:/PKSE/legacy_source_bindings.cfg
```

Current persistence behavior:

```text
write sdmc:/PKSE/legacy_source_bindings.cfg.tmp
flush / close
rename(tmp, existing destination)
```

Leading hypothesis — **not yet proven**:

- first assignment succeeds because the destination file does not yet exist;
- a later assignment fails because Switch/libnx fsdev rename-over-existing semantics differ from ordinary POSIX host behavior.

This hypothesis matches the physical pattern and exact failure location, but the next coding session must confirm or reject it before claiming root cause.

## Interrupted LeafGreen fix session

The previous Codex session ran out of credits immediately after reporting:

```text
Applying a code patch
```

It had already narrowed the issue to `LegacySourceBindings::save()` and was checking libnx/fsdev behavior.

**Critical first step next session:** before syncing, resetting, cleaning, switching refs or changing worktrees, inspect every relevant worktree with `git status`, `git diff`, staged diff and untracked files. Preserve/recover any partially applied binding-persistence patch before doing anything destructive.

Remote branch documentation HEAD at the time of this handoff may be newer than the tested application source; keep documentation SHA, application-source SHA and artifact SHA distinct.

## Immediate next milestone

Do only the LeafGreen / second-binding persistence fix:

```text
recover interrupted patch if present
        ↓
confirm actual fsdev replacement behavior
        ↓
implement crash-conscious Switch-safe binding persistence
        ↓
preserve existing valid FireRed binding
        ↓
add focused regressions for first + second assignments, reload,
profile isolation and rollback on persistence failure
        ↓
run focused verification
        ↓
commit + push SOURCE FIX first
        ↓
build a new device-test NRO only after source is safe
        ↓
STOP for physical retest
```

Do not reopen FRLG discovery/parser work. Do not add arbitrary RetroArch `.state` parsing. Do not start RSE or other roadmap features in the same session.

## Required physical retest after the persistence fix

1. FireRed current 2-Pokémon source still opens correctly.
2. LeafGreen can be assigned successfully.
3. Restart PokeBank NX and confirm LeafGreen assignment persists.
4. Assign/test profile isolation so one profile's source does not appear in the other profile.
5. Switch back and confirm the original profile assignment remains.
6. Recheck Trainer, Items, Party, Boxes 1-14, Pokémon View, Refresh and read-only locks.

Only after this passes may FRLG be marked physically accepted.

```text
DEVICE TESTED: YES
DEVICE ACCEPTED: NO
```

## Parked RSE recovery

Preserve if present:

```text
b5ef83b
1a921515
```

or equivalent recovered refs.

`b5ef83b` is the verified parked RSE recovery checkpoint. Do not merge, resume, reimplement or push RSE until FRLG physical acceptance.

## Fast session launcher

```text
Continue PokeBank NX on feature/pokebank-playable. Use HIGH reasoning. Before touching refs or worktrees, preserve all local/uncommitted/recovery work and inspect for the interrupted LeafGreen binding patch. Read CURRENT_STATUS.md and docs/CODEX_SESSION.md, then execute docs/NEXT_CODEX_PROMPT.md exactly. Use docs/PROJECT_RESOURCE_INDEX.md only for the active subsystem. Push coherent checkpoints only to origin/feature/pokebank-playable; never push custom code upstream.
```
