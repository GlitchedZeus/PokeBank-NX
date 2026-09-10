# Switch save durability / power-loss research plan — 2026-09-09

> Research and future test planning only. This does **not** enable live save writing and does not change the current FRLG milestone.

This note preserves the September 9 deep research on the remaining physical-durability gap for PokeBank NX.

## What is now understood

Horizon title savedata is not equivalent to a normal `sdmc:` directory. Public Switch save-image documentation describes a transaction-oriented container with:

- dual main headers;
- Hierarchical Duplex Storage;
- journal data + journal block map;
- IVFC integrity trees;
- duplex master bitmaps / active selector;
- save-data commit metadata / Commit ID.

The intended logical model is that inactive data/bitmap state is updated and a commit publishes the new active state.

Relevant references:

- Switchbrew `Savegames`: https://switchbrew.org/wiki/Savegames
- libnx fsdev docs/source: https://switchbrew.github.io/libnx/fs__dev_8h.html
- libnx FS API: https://switchbrew.github.io/libnx/fs_8h.html
- hactool: https://github.com/SciresM/hactool
- LibHac: https://gitlab.com/ryujinx-classic/LibHac

libnx documents `fsdevCommitDevice()` / `fsFsCommit()` as required for mounted savedata writes and says unmounting is not a substitute for commit.

Permanent architecture rule:

```text
SdFileTransaction
    ordinary sdmc:/ files
    config
    Vault/index files
    RetroArch / emulator saves

!=

HorizonSaveTransaction
    mounted Nintendo title savedata
    explicit fsFsCommit / fsdevCommitDevice
    Horizon savedata journal/duplex semantics
```

## What remains genuinely unknown

Public documentation does not establish the exact electrical durability boundary after every CPU instruction. Even if `fsFsCommit()` returns success, final persistence still depends on Horizon FS, block-device behavior, controller caches, flash translation layers and physical power-loss timing.

Therefore exact power-loss guarantees must be measured on disposable hardware test data before PokeBank promises live-write safety.

## `powercut.nro` test concept

Never perform this test against a real Pokémon save.

### Horizon savedata checkpoints

```text
H0  old committed state verified
H1  open target
H2  write first half of generation N
H3  write full generation N
H4  fsFileFlush()
H5  close target
H6  immediately before fsFsCommit()
H7  commit in progress
H8  fsFsCommit() returned success
H9  reopen
H10 verify bytes + checksum + semantic parse
H11 close verified save
```

Test interruption classes separately:

```text
process termination
system reset/reboot
true electrical interruption where practical
```

After restart classify observed state as:

```text
OLD_VALID
NEW_VALID
MIXED_BYTES
FILE_MISSING
SAVE_MOUNT_FAILURE
INTEGRITY_FAILURE
OTHER
```

Record at minimum:

```text
checkpoint
native Horizon Result / errno where available
SHA-256
generation counter
payload CRC / native checksum
parser result
save Commit ID if available
mount result
repair/recovery behavior
```

### Ordinary `sdmc:` transaction checkpoints

A separate matrix exercises PokeBank's own temp/backup transaction:

```text
S0  TARGET old valid
S1  TMP created
S2  TMP partially written
S3  TMP fully written
S4  TMP flushed
S5  TMP closed
S6  TMP reopened + validated
S7  TARGET -> BAK complete
S8  TMP -> TARGET complete
S9  TARGET reopened + validated
S10 backup retention/cleanup
```

For every interruption point, startup recovery must still find at least one known-good generation.

## Filesystem/media test matrix

Eventually measure both FAT32 and exFAT if PokeBank intends to run on both. Include:

- multiple SD cards/controllers;
- nearly empty vs nearly full;
- new/healthy vs well-used media;
- common Switch FAT32 cluster sizes;
- relevant Switch models / HOS / Atmosphere / libnx revisions.

This does **not** imply PokeBank should recommend exFAT; the point is to test the environments users may actually have.

## Host-side forensic inspection

For disposable raw savedata captures, hactool/LibHac can provide deeper evidence than only checking whether the game reopens:

```text
Switch test
 -> interrupt at checkpoint
 -> dump disposable save image
 -> hactool / LibHac integrity inspection
 -> compare header / duplex / journal / IVFC / commit state
```

## Promotion gate

Live writing remains disabled until the corresponding adapter has passed the project's existing backup/staging/validation/readback gates plus deliberate real-device durability testing.
