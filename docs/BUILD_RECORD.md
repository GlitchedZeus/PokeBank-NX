# PokeBank NX — Build / Artifact Record

Last updated: 2026-09-02

This file distinguishes application source checkpoints, documentation commits, temporary runtime build records, and physically tested binaries.

A binary is not reproducibly identified by filename alone.

---

## Recovery-era safety build

The completed recovery session reported a native safety `.nro` with:

```text
size: 9,695,669 bytes
SHA-256: 0cf50b659ed5c648009e10d51a75a398bc2a3e69e4cbeb99cc0b74b9643ece07
```

The session recorded the local exact build checkpoint as short SHA:

```text
65aa52c
```

That SHA is **not available as a remote GitHub commit**.

Verified published remote safety milestone:

```text
c618bd5e44381635f92c17fc7b36c594b64aaa40
safety: hard-lock live game save writes
```

The original recovery `.nro` was not committed/published as a durable GitHub release artifact.

Do not chase `65aa52c` as required remote history. Rebuild from verified GitHub source when needed.

---

## Action Sheet device-test build

This is the first post-recovery PokeBank NX feature build with an exact remote application source and recorded binary hash.

```text
Version/tag: 0.1.0-alpha
Branch: feature/pokebank-playable
Application source SHA: 82a0779a5143cca0690d0c7068946d84ebe9f107
Commit message: ui: add controller Pokemon action sheet
Build date: 2026-09-01 UTC
Compiler: aarch64-none-elf-g++ (devkitA64) 15.2.0
Host test result: PASS
Sanitizer result: PASS (ASan/UBSan; leak detection disabled in managed sandbox)
git diff --check: PASS
Native build result: BUILDS
Artifact filename: PokeBank-NX-ActionSheet-82a0779.nro
Artifact size: 9,695,669 bytes
Artifact SHA-256: 6ff0f71c2e8f6d7fcf948a4bbc0037ba799e22bbaac433263be7cd0afac3b72b
Device tested: NO
Device-test notes/report: intentionally deferred while issue #13 UI/control build is completed
```

The later Session 1 status/documentation commit was:

```text
467f4b896280498bd8c36abd01eb4a0b39995863
docs: record action sheet device build
```

Do not confuse that later documentation SHA with the actual `.nro` source SHA.

Issue #2 is completed/closed. Physical testing remains tracked by issue #8.

---

## Session 2 HOME controls and three-theme device-test build

```text
Version/tag: 0.1.0-alpha
Branch: feature/pokebank-playable
Application source SHA: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Application source commit message: fix: restore complete controller UI source
Build date: 2026-09-02 UTC
Compiler: aarch64-none-elf-g++ (devkitA64) 15.2.0
Host test result: PASS (6 suites, including Action Sheet and write policy)
Sanitizer result: PASS (ASan/UBSan; leak detection disabled in managed sandbox)
git diff --check: PASS
Native build result: BUILDS
Embedded abbreviated commit: 3be4de6b
Artifact filename: PokeBank-NX-UI-Theme-3be4de6.nro
Artifact size: 9,707,957 bytes
Artifact SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
Persistent artifact location: ChatGPT Library `/PokeVault NX/PokeBank-NX-UI-Theme-3be4de6.nro`
Device tested: NO
Device-test notes/report: READY FOR DEVICE TEST using docs/DEVICE_TEST_CHECKLIST.md
```

This binary combines the completed issue #2 Action Sheet with issue #13's semantic themes,
controller repeat, contextual `+`/`-`, reusable surfaces and context-aware hint foundation.

---

## Branch-history reconciliation checkpoints

On 2026-09-01 GitHub `main` and `feature/pokebank-playable` briefly diverged because the Action Sheet application source was merged to `main` while the feature branch contained the later Action Sheet build/status record.

They were reconciled without force-push/reset at:

```text
9a2151ee70c73bab4451f35a1216c495d60b57ba
merge: reconcile action sheet build record with main
```

Subsequent documentation commits may advance the feature branch before `main` is fast-forwarded again. Application `.nro` identity must still point to the application source commit used for that binary, not the latest docs-only head.

---

## Artifact preservation issue

GitHub issue #15 tracks a recurring infrastructure improvement:

```text
Automate preservation of device-test .nro artifacts
```

Preferred future flow:

```text
application source commit
        ↓
host tests + sanitizers
        ↓
native .nro build
        ↓
metadata manifest / hashes
        ↓
GitHub Actions artifact or prerelease
        ↓
physical device test
```

The artifact system must never automatically mark a build `DEVICE TESTED`.

---

## Build record template

For every device-test/prerelease build append:

```text
Version/tag:
Branch:
Application source full SHA:
Application source commit message:
Build date/time:
Compiler/devkitPro environment:
Host test result:
Sanitizer result:
git diff --check result:
Native build result:
Artifact filename:
Artifact size:
Artifact SHA-256:
Persistent artifact/release location:
Device tested: yes/no
Exact device-test report/reference:
```

If a later documentation/status commit is created after the application source, record it separately.

---

## Artifact policy

Normal source commits should not accumulate `.nro` binaries unless the repository deliberately changes policy.

For meaningful device-test builds prefer a durable GitHub Actions artifact or prerelease containing:

```text
version
application source SHA
.nro
SHA-256
known supported/read-only status
known limitations
```

Until issue #15 is implemented, every coding session that produces a device build must provide/preserve the `.nro` before the temporary runtime disappears.

---

## Verification rule

These are different claims:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

A remembered filename, local-only session report, or successful host test is never enough to promote a binary to `DEVICE TESTED`.
