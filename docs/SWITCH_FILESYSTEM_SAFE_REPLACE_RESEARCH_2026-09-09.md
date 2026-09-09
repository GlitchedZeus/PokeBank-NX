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

## 3. Separate the two write abstractions

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

## 4. pkHouse / pkBakery lesson for mounted Switch saves

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

## 5. Recommended PokeBank SD replacement transaction

The exact implementation must still be verified on the Switch, but the desired safety contract is:

```text
1. recover any stale .tmp/.bak state first
2. serialize complete new contents in memory when practical
3. write target.tmp
4. flush/close target.tmp
5. validate target.tmp (size + parse/hash as appropriate)
6. move existing target -> target.bak
7. move target.tmp -> target
8. reopen/read/validate target
9. only after validation, delete target.bak
```

If step 7 or 8 fails, the previous valid file must remain recoverable from `.bak`.

Never intentionally enter a state where the only known-good copy is deleted before a replacement is validated.

## 6. Startup/recovery expectations

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

## 7. Suggested regression matrix

Host-side tests should simulate a failure after each state-changing step:

```text
write tmp
flush/close tmp
validate tmp
rename old -> bak
rename tmp -> target
validate target
delete bak
```

After every injected failure, reopening the storage abstraction must recover a valid previous/new database and never silently lose the already-working FireRed binding.

Physical Switch acceptance should then exercise at least:

```text
first assignment
second assignment
restart/reload
same-profile FR+LG
cross-profile isolation
power/app interruption where practical
FAT32/exFAT behavior if both environments become available
```

## 8. Current evidence boundary

Strongly supported:

- physical failure occurs after in-memory assignment and during persistence;
- libnx fsdev `rename()` delegates to `fsFsRenameFile()` directly;
- a mature Switch homebrew project uses `old -> .bak`, `.tmp -> old`, then removes `.bak`;
- mounted Horizon save-data has different commit/journal concerns from ordinary SD files.

Still to prove in the actual PokeBank patch/device test:

- the exact result code returned by the failed LeafGreen rename;
- whether destination-exists is the sole cause on the user's filesystem;
- the final replacement/recovery sequence that behaves correctly on the user's SD card.

Treat rename-over-existing as the leading hypothesis until the patched code logs/confirms the actual failure path.