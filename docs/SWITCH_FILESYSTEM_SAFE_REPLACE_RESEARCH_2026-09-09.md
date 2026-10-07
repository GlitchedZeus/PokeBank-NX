# Switch filesystem safe-replace research — 2026-09-09

> Supporting engineering research. This document does **not** change active scope. `CURRENT_STATUS.md` and `docs/NEXT_CODEX_PROMPT.md` remain authoritative.

This note exists because the physical LeafGreen test isolated a real Switch-only persistence failure in `LegacySourceBindings::save()` while writing:

```text
sdmc:/PKSE/legacy_source_bindings.cfg
```

The current implementation writes a temporary file and then tries to rename it directly over the existing destination. The first assignment can succeed while a later assignment fails.

## 1. libnx / fsdev behavior

Current libnx `rename()` support in `nx/source/runtime/devices/fs_dev.c` resolves the old/new paths and, for files, directly calls:

```text
fsFsRenameFile(&device->fs, old_path, new_path)
```

There is no POSIX-style fallback in `fsdev_rename()` that first removes/replaces an already-existing destination.

Primary references:

```text
switchbrew/libnx
nx/source/runtime/devices/fs_dev.c
nx/include/switch/services/fs.h
nx/source/services/fs.c
```

Also relevant:

- `fsFileFlush(FsFile*)` exists for explicit file flushes.
- `fsdev` calls `fsFileFlush()` for files opened with `O_SYNC`.
- `fsdevCommitDevice()` / `fsFsCommit()` is specifically documented as required after **savedata** writes. That is a separate concern from ordinary files on `sdmc:`.

This distinction matters: a mounted Horizon title save filesystem and a normal SD-card file should not share one blindly generic write strategy.

## 2. Mature Switch pattern: DNS-MITM_Manager

`znxDomain/DNS-MITM_Manager` contains a real libnx SD-card replacement sequence in `source/GlobalObjects.cpp`.

Observed pattern:

```text
write <target>.tmp
close temporary file
rename target -> target.bak
rename target.tmp -> target
delete target.bak
```

It uses direct `FsFileSystem` / `fsFsRenameFile()` calls instead of relying on rename-over-existing behavior.

This is valuable evidence for the LeafGreen hypothesis, but **do not copy its failure behavior verbatim**. The reference calls `fatalThrow()` on several failures and does not provide the full recovery state machine PokeBank needs.

Reference:

```text
https://github.com/znxDomain/DNS-MITM_Manager
source/GlobalObjects.cpp
```

## 3. Independent Switch confirmation: badpiggies_nx

`ChanseyIsTheBest/badpiggies_nx` contains a compatibility shim for software that expects desktop/POSIX rename semantics. Its comments explicitly document two Horizon behaviors that matter to the current PokeBank failure:

```text
POSIX rename:
    replaces an existing destination

Horizon fsFsRenameFile:
    does not replace an existing destination

Horizon filesystem:
    may also reject rename/delete while a relevant file handle remains open
```

This gives the current LeafGreen persistence bug two concrete suspects:

```text
A. legacy_source_bindings.cfg already exists
B. target/source/tmp/bak still has an open handle at promotion time
```

Reference:

```text
https://github.com/ChanseyIsTheBest/badpiggies_nx
source/libc_shim.c
source/libc_shim.h
```

The project is MIT-licensed at the reviewed revision, but PokeBank should still implement its own narrow transaction abstraction rather than import the Android compatibility shim wholesale.

### Diagnostic requirement for the next physical retest

Do not reduce another hardware failure to only:

```text
save failed
```

Capture enough stage information to distinguish the failure:

```text
SAFE_REPLACE BEGIN
 target/tmp/bak existence
 write/flush/close result
 rename(target,bak): errno + fsdevGetLastResult where available
 rename(tmp,target): errno + fsdevGetLastResult where available
 reopen target
 parse/size/hash validation
 SAFE_REPLACE END
```

All readers/writers involving the target transaction paths should be closed before old-to-backup or temp-to-target promotion.

## 4. Separate the two write abstractions

PokeBank should keep these concepts distinct:

```text
SafeSdFileReplace
    -> legacy-source binding/config files
    -> RetroArch battery saves when a write adapter is eventually approved
    -> Vault manifests/indexes
    -> Banks
    -> app configuration

SwitchSaveTransaction
    -> mounted retail Switch-title saves
    -> proper Horizon save-filesystem commit semantics
    -> destination-specific serialization/checksums/encryption
    -> backup before mutation
    -> readback verification
```

Do not collapse both into one generic `writeFile()` helper.

## 5. pkHouse / pkBakery lesson for mounted Switch saves

Current native Switch projects from `Insektaure` are useful references for the *other* side of this boundary.

`pkHouse` supports LGPE, SwSh, BDSP, PLA, SV, Z-A and Switch FRLG save families and documents automatic backups. Its save layer explicitly writes mounted title-save files **in place** rather than truncating/replacing them, because the author found truncation/rewrite could interfere with the Switch save filesystem journal.

`pkBakery` documents the same principle for Legends: Z-A SCBlock saves and adds:

- account/profile selection;
- automatic backup before modification;
- in-place `r+b` writing;
- round-trip verification (`encrypt(decrypt(file)) == file`).

These are **references**, not a reason to use in-place mutation for ordinary `sdmc:` config/Vault/emulator files. The failure models are different.

References:

```text
https://github.com/Insektaure/pkHouse
https://github.com/Insektaure/pkBakery
```

Both repositories are GPL-2.0 at the reviewed revision; default to study/reference unless project licensing decisions explicitly allow direct reuse.

## 6. Recommended PokeBank SD replacement transaction

The exact implementation must still be verified on the Switch, but the desired safety contract is:

```text
1. recover any stale .tmp/.bak state first
2. close every reader/writer involving target/tmp/bak
3. serialize complete new contents in memory when practical
4. write target.tmp
5. flush/close target.tmp
6. reopen + validate target.tmp (size + parse/hash as appropriate)
7. close target.tmp again
8. move existing target -> target.bak
9. move target.tmp -> target
10. reopen/read/validate target
11. retain target.bak until a later known-good startup/retention point
```

If step 9 or 10 fails, the previous valid file must remain recoverable from `.bak`.

Never intentionally enter a state where the only known-good copy is deleted before a replacement is validated.

## 7. Startup/recovery expectations

`SafeSdFileReplace` should eventually have deterministic recovery rules for combinations of:

```text
target
target.tmp
target.bak
```

At startup/reopen, validate candidates before choosing one. The core invariant should be:

```text
a crash at any individual transaction step leaves either
THE PREVIOUS VALID FILE
or
THE NEW VALID FILE
recoverable — never neither.
```

For the current LeafGreen bug, keep the patch narrow: solve binding persistence safely, add regression/failure-injection coverage, and do not turn this session into the entire future Vault transaction implementation.

## 8. Suggested regression matrix

Host-side tests should simulate a failure after each state-changing step:

```text
write tmp
flush/close tmp
validate tmp
rename old -> bak
rename tmp -> target
validate target
backup retention/cleanup
```

After every injected failure, reopening the storage abstraction must recover a valid previous/new database and never silently lose the already-working FireRed binding.

Physical Switch acceptance should then exercise at least:

```text
first assignment
second assignment
restart/reload
same-profile FR+LG
cross-profile isolation
open-handle diagnostics
power/app interruption where practical
FAT32/exFAT behavior if both environments become available
```

## 9. Current evidence boundary

Strongly supported:

- physical failure occurs after in-memory assignment and during persistence;
- libnx fsdev `rename()` delegates to `fsFsRenameFile()` directly;
- mature Switch homebrew uses `old -> .bak`, `.tmp -> old` rather than rename-over-existing;
- an independent Switch project explicitly documents destination-exists and open-handle differences from desktop POSIX behavior;
- mounted Horizon save-data has different commit/journal concerns from ordinary SD files.

Still to prove in the actual PokeBank patch/device test:

- the exact result code returned by the failed LeafGreen transaction;
- whether destination-exists, an open handle, or both cause the user's failure;
- the final replacement/recovery sequence that behaves correctly on the user's SD card.

Treat rename-over-existing as the leading hypothesis, with open-handle rejection as a second concrete suspect, until the patched code logs/confirms the actual failure path.
