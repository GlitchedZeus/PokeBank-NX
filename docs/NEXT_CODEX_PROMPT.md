# PokeBank NX — Authoritative Next Codex Prompt

Use HIGH reasoning.

## Single mission

Fix the physically confirmed **LeafGreen / second legacy-source binding persistence failure on Switch**, publish the source fix safely, then stop for a new physical-test build/retest.

Do not restart FRLG parsing/discovery work and do not begin RSE or another roadmap feature.

## Preserve interrupted work FIRST

The previous Codex session ran out immediately after reporting:

```text
Applying a code patch
```

Before syncing, resetting, cleaning, restoring, switching refs, rebasing, deleting generated state or changing worktrees:

1. inspect every relevant worktree;
2. run `git status`;
3. inspect unstaged and staged diffs;
4. inspect untracked files;
5. preserve any partial/uncommitted LeafGreen binding-persistence patch on a recovery ref or patch file.

Do **not** discard local changes just because remote HEAD is older.

Preserve parked RSE refs/worktrees including `b5ef83b`, `1a921515`, or verified equivalents. Do not merge/resume/reimplement/push RSE.

## Tested baseline

Physically tested application source:

```text
ea0b806bac4acdb5619f22f9841d616ea8a237ff
legacy: bind FRLG sources to profiles and expose diagnostics
```

Canonical tree:

```text
ed5912093886384894c44538d569fe4955fd2e47
```

Exact tested NRO:

```text
PokeBank-NX-FRLG-Corrected-Retest-ea0b806b.nro
156,592,377 bytes
SHA-256 396f8ff9f4da53b5449aeb46b8d1237ca9358a0ac94998680017575916b6b1ee
```

## Physical facts — do not re-investigate these

### FireRed

- normal RetroArch battery/in-game `.srm` is the correct source;
- current FireRed 2-Pokémon save opens correctly after a normal in-game save;
- trainer information parses correctly;
- items/inventory parse correctly;
- do **not** add `.state` savestate support as a workaround.

### LeafGreen

```text
File: Pokemon - Leaf Green Version.srm
Game: LeafGreen
Trainer: Will
Party: 1
Fingerprint prefix: d76e3c7e25a4
```

Discovery and parsing succeed.

Assignment to either available profile fails with:

```text
Assignment could not be saved; source remains unassigned.
```

## Known failure path

Code inspection already established:

```text
LegacySourceBindings::assign(...)
        -> succeeds in memory
LegacySourceBindings::save()
        -> returns false
```

Binding database:

```text
sdmc:/PKSE/legacy_source_bindings.cfg
```

Current writer behavior:

```text
write legacy_source_bindings.cfg.tmp
flush / close
rename(tmp, existing destination)
```

Leading hypotheses — NOT yet proven:

1. the first assignment succeeds because no destination exists, while a later assignment fails because Switch/libnx fsdev rename-over-existing behavior differs from ordinary POSIX host behavior;
2. Horizon may also reject rename/delete while a relevant file handle is still open.

The interrupted session had started checking libnx/fsdev semantics and had begun applying a patch.

## Research already found for this exact bug

Before doing new broad filesystem research, read:

```text
docs/SWITCH_FILESYSTEM_SAFE_REPLACE_RESEARCH_2026-09-09.md
```

That note records verified implementation references relevant to this failure:

- current libnx `fsdev_rename()` delegates a file rename directly to `fsFsRenameFile()`; it does not implement its own replace-existing fallback;
- `znxDomain/DNS-MITM_Manager` has a real Switch SD-card update pattern that writes a temp file, renames the old target to `.bak`, renames `.tmp` into the target path, then removes the backup;
- `ChanseyIsTheBest/badpiggies_nx` independently documents that Horizon does not provide normal POSIX replace-existing rename semantics and may reject rename/delete while the relevant path still has an open handle;
- `fsFileFlush()` exists for explicit file flushes;
- `fsdevCommitDevice()` / `fsFsCommit()` is specifically a mounted **savedata** write concern and must not be confused with ordinary `sdmc:` file replacement;
- mounted retail-title saves and ordinary SD-card files should remain separate abstractions.

Use those references to avoid rediscovering the platform behavior, but **do not copy their failure handling verbatim**. PokeBank must retain/recover a known-good binding database across every failure point.

The target design direction is a narrow `SafeSdFileReplace`-style transaction for this binding DB, while future retail Switch save writing remains a separate `SwitchSaveTransaction` concern.

## Required implementation work

1. Recover the interrupted patch if it survived.
2. Confirm or reject the actual fsdev/rename-over-existing and open-handle hypotheses using the relevant libnx/devoptab behavior and, where possible, capture/log the actual failure result/errno.
3. Implement a crash-conscious Switch-safe persistence transaction.
4. Explicitly ensure all file handles involving target/tmp/bak are closed before promotion.
5. Preserve an existing valid bindings database; do not simply delete it first and hope the next write succeeds.
6. Ensure a failed persistence attempt rolls back/reverts the in-memory assignment so UI state never claims an unsaved binding.
7. Preserve all existing FireRed behavior and all read-only locks.

## Required focused regression coverage

Add/verify tests for:

- first assignment saves;
- second independent assignment saves;
- reload preserves both;
- FireRed + LeafGreen can both belong to one profile;
- FireRed can belong to Profile A while LeafGreen belongs to Profile B;
- profile isolation survives reload;
- aliases do not duplicate/collapse real sources;
- failed persistence leaves the previous on-disk database valid;
- failed persistence rolls back the attempted in-memory assignment;
- failure injection around temp-write / validation / old-to-backup / temp-to-target / final-validation never leaves neither old nor new valid database recoverable;
- diagnostics distinguish destination-exists/open-handle/promotion failures where the platform exposes enough information.

Use the smallest reliable host/filesystem abstraction needed to reproduce replacement semantics. Do not redesign the whole storage system.

## Verification / checkpoint priority

The **source fix on GitHub is the first priority**.

Run the focused relevant host tests and cheap checks first. Because application source will change, run the verification appropriate to the changed code before a device artifact is called ready. Native build must remain `-fno-exceptions`.

As soon as the source fix and focused regression are coherent:

```text
commit
push -> origin/feature/pokebank-playable
update CURRENT_STATUS.md
update issue #6
```

Do this **before** expensive sprite/artifact work.

If the complete 3,260-render asset/build cache survives and enough session budget remains, produce a new retest NRO with fresh size/SHA-256 and preserve it immediately. If rebuilding assets would consume substantial session budget, stop after the source fix is safely pushed and defer packaging.

## Strictly out of scope

Do NOT begin or expand:

- RSE;
- Gen I/II;
- DS/3DS;
- modern Switch support;
- Vault/Banks;
- Create Pokémon/editor;
- legality/conversion;
- events/Gifts/Trade;
- Android/social;
- broad UI redesign;
- arbitrary RetroArch `.state` parsing;
- any live save writing.

## STOP condition

Stop after either:

### A. Source-fix checkpoint only

```text
LeafGreen binding persistence source fix committed + pushed
focused verification recorded
artifact build deferred
DEVICE TESTED: YES (ea0b806b historical physical test)
DEVICE ACCEPTED: NO
```

or, if practical:

### B. New retest artifact

```text
source fix committed + pushed
new exact NRO packaged and hashed
artifact preserved
DEVICE TESTED FOR NEW ARTIFACT: NO
DEVICE ACCEPTED: NO
```

Do not roll directly into RSE.

## End report

Report:

```text
recovered interrupted patch/ref (if any)
starting remote SHA
new application source SHA
confirmed root cause or rejected hypotheses
binding persistence strategy
actual Switch/libnx failure code/errno if captured
open-handle evidence if captured
regression tests added
host/focused verification
native -fno-exceptions build status
CI status
source fix pushed? yes/no
new NRO filename/size/SHA-256 if produced
asset reuse/restoration if any
RSE refs preserved
DEVICE ACCEPTED: NO
```
