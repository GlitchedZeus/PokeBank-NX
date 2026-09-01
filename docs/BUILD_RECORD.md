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
