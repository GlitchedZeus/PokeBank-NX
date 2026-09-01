# PokeBank NX — Build / Artifact Record

Last updated: 2026-09-01

This file distinguishes source checkpoints from temporary build artifacts so a future session does not mistake a remembered local SHA for a remotely available GitHub commit.

---

## Recovery-era safety build

The completed recovery session reported a native safety `.nro` with:

```text
size: 9,695,669 bytes
SHA-256: 0cf50b659ed5c648009e10d51a75a398bc2a3e69e4cbeb99cc0b74b9643ece07
```

The session status recorded the exact local source/build checkpoint as:

```text
65aa52c
```

That short SHA is **not currently available as a GitHub commit in the remote repository**.

The verified published remote safety milestone is:

```text
c618bd5e44381635f92c17fc7b36c594b64aaa40
safety: hard-lock live game save writes
```

The original `.nro` binary from the temporary recovery runtime is not stored in the Git repository and is not currently published as a GitHub release artifact.

Therefore the correct procedure before physical testing is:

```text
fetch current GitHub source
      ↓
verify expected source milestone/history
      ↓
run host tests
      ↓
run sanitizers
      ↓
build fresh .nro
      ↓
record exact source commit + size + SHA-256
      ↓
preserve/provide the binary
      ↓
physical Switch test
```

Do not attempt to recover `65aa52c` as if it were required remote history. Treat `c618bd5` as the verified remote safety baseline and produce a fresh build from the current verified source tree.

---

## Current documentation head

On 2026-09-01, `main` was a clean fast-forward descendant of `feature/pokebank-playable`, with the merge base at `c618bd5` and no source divergence. The development branch was fast-forwarded to include the documentation/specification commits.

After future code changes, use `PROJECT_STATUS.md` and Git history as the source of truth rather than this historical note.

---

## Action Sheet device-test build

```text
Version/tag: 0.1.0-alpha
Branch: feature/pokebank-playable
Full commit SHA: 82a0779a5143cca0690d0c7068946d84ebe9f107
Commit message: ui: add controller Pokemon action sheet
Build date: 2026-09-01 UTC
Compiler: aarch64-none-elf-g++ (devkitA64) 15.2.0
Host test result: PASS
Sanitizer result: PASS (ASan/UBSan; leak detection disabled in managed sandbox)
Native build result: BUILDS
Artifact filename: PokeBank-NX-ActionSheet-82a0779.nro
Artifact size: 9,695,669 bytes
Artifact SHA-256: 6ff0f71c2e8f6d7fcf948a4bbc0037ba799e22bbaac433263be7cd0afac3b72b
Source location: GlitchedZeus/PokeBank-NX, feature/pokebank-playable
Device tested: no
Device-test notes/report: awaiting user test of this exact binary
```

The binary is preserved outside normal source history for direct user download. Do not mark it
`DEVICE TESTED` until this exact SHA-256 is run on physical Switch hardware.

---

## Build record template

For every device-test or prerelease build, append/record:

```text
Version/tag:
Branch:
Full commit SHA:
Commit message:
Build date/time:
Compiler/devkitPro environment:
Host test result:
Sanitizer result:
Native build result:
Artifact filename:
Artifact size:
Artifact SHA-256:
GitHub release/artifact location:
Device tested: yes/no
Device-test notes/report:
```

A binary is not reproducibly identified by filename alone.

---

## Artifact policy

Normal source commits should not accumulate `.nro` binaries unless the repository deliberately changes policy.

For meaningful hardware-test builds, prefer a GitHub prerelease/release artifact with:

```text
version
source commit
.nro
SHA-256
known supported/read-only status
known limitations
```

This avoids losing binaries when a temporary coding workspace disappears.
