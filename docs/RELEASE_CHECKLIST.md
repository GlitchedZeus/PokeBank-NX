# PokeBank NX — Alpha Release / Artifact Checklist

Last updated: 2026-09-01

Use this checklist for every build intended for physical testing or prerelease distribution.

The core problem this document prevents is simple: a successful `.nro` that only exists in a temporary coding runtime is not a durable/reproducible artifact.

Issue #15 tracks automation of this process.

---

## Verification vocabulary

Keep these claims separate:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

A release/prerelease upload does not automatically make a build `DEVICE TESTED`.

---

## Release naming

Use explicit prerelease versions or uniquely identified development artifacts, for example:

```text
v0.1.0-alpha.1
v0.1.0-alpha.2
PokeBank-NX-UI-<short-source-sha>.nro
```

Do not call a build stable merely because it launches.

---

## Application source vs later documentation

A common PokeBank NX workflow is:

```text
application source commit
    ↓
build .nro
    ↓
record artifact
    ↓
later PROJECT_STATUS / BUILD_RECORD docs commit
```

The `.nro` belongs to the **application source commit**, not the later documentation commit.

Record both when they differ.

Example from the Action Sheet milestone:

```text
Application source:
82a0779a5143cca0690d0c7068946d84ebe9f107

Later status record:
467f4b896280498bd8c36abd01eb4a0b39995863
```

---

## Pre-build source gate

Before building:

- [ ] local work is understood;
- [ ] interrupted-session branches/worktrees/reflog were checked before destructive cleanup;
- [ ] exact application source branch is recorded;
- [ ] exact application source full SHA is recorded;
- [ ] intended changes are committed or the build is clearly labeled temporary/non-publishable;
- [ ] live-write policy is explicitly known;
- [ ] no unresolved merge/conflict state exists.

For a publishable/testable milestone, prefer building from a committed application source SHA.

---

## Host verification gate

Run:

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
```

Require:

- [ ] host tests PASS;
- [ ] ASan/UBSan PASS;
- [ ] `git diff --check` PASS;
- [ ] relevant GitHub host CI PASS when available for the source commit/source-identical state.

---

## Native build gate

Build using the documented devkitPro/devkitA64 environment:

```bash
make -j1
```

Require:

- [ ] native build PASS;
- [ ] exact `.nro` filename recorded;
- [ ] file size recorded;
- [ ] SHA-256 recorded;
- [ ] build environment/toolchain recorded sufficiently for reproduction.

Example:

```bash
sha256sum PokeBank-NX.nro
```

---

## Build record

Append/update:

```text
docs/BUILD_RECORD.md
```

Record at minimum:

```text
Version/build label:
Application source branch:
Application source full SHA:
Application source commit message:
Later status/docs SHA if different:
Build date/time:
Compiler/devkitPro:
Host tests:
Sanitizers:
git diff --check:
Native build:
Artifact filename:
Artifact size:
Artifact SHA-256:
Persistent artifact location:
Device tested: yes/no
```

---

## Artifact package

A test/prerelease package should contain at minimum:

```text
PokeBank-NX.nro
release notes / README
SHA256SUMS.txt
```

If extra runtime assets are required, package the exact SD-card layout expected by the app.

Do not package:

- commercial ROMs;
- user saves;
- console keys/credentials;
- proprietary game files that cannot be redistributed.

---

## Persistent artifact target

Preferred future flow, tracked by issue #15:

```text
verified application source commit
        ↓
GitHub Actions / reproducible native build
        ↓
.nro + metadata + SHA256SUMS
        ↓
Actions artifact or GitHub prerelease
```

Until that is automated, the coding session must explicitly provide/preserve the `.nro` before the runtime disappears.

Normal source history should not accumulate `.nro` binaries unless project policy is deliberately changed.

---

## Release notes template

```markdown
# PokeBank NX v0.1.0-alpha.X

Application source: `<full SHA>`
Artifact: `<filename>`
Artifact size: `<bytes>`
Artifact SHA-256: `<hash>`

## Verification
- Host tests: PASS
- ASan/UBSan: PASS
- git diff --check: PASS
- Native `.nro`: BUILDS
- Physical Switch: NOT DEVICE TESTED / DEVICE TESTED

## Safety posture
Live installed-game save writing is HARD DISABLED.

## What works
- ...

## What is incomplete
- ...

## Device-test focus
- ...

## Known issues
- ...
```

Never publish `DEVICE TESTED` without a physical report for the exact artifact hash.

---

## Current first-device-test target

The Action Sheet build already exists:

```text
PokeBank-NX-ActionSheet-82a0779.nro
Source: 82a0779a5143cca0690d0c7068946d84ebe9f107
SHA-256: 6ff0f71c2e8f6d7fcf948a4bbc0037ba799e22bbaac433263be7cd0afac3b72b
```

It remains `NOT DEVICE TESTED`.

The preferred next hardware artifact is the combined issue #13 build after the interrupted local HOME-style controls/theme work is recovered, tested, built, committed, and pushed.

That combined first test should cover:

```text
launch
Select Game cards/focus
D-pad + Left Stick
held navigation repeat
A/B Action Sheet behavior
L/R and any ZL/ZR behavior
+ More/Options (not Exit)
- Help/Controls
bottom controller hint bar
OLED Black / Dark / Light
theme persistence
read-only Party/Boxes browsing
live-write hard lock
```

Use `docs/DEVICE_TEST_CHECKLIST.md`.

---

## Installation note

Target standardized layout:

```text
/switch/PokeBank-NX/PokeBank-NX.nro
```

If inherited target branding still produces another filename, release notes must state the actual file tested.

---

## After physical test

If PASS:

- [ ] update `PROJECT_STATUS.md`;
- [ ] update `docs/BUILD_RECORD.md`;
- [ ] update issue #8 with exact source SHA + artifact SHA-256 + result;
- [ ] mark only capabilities actually exercised as `DEVICE TESTED`;
- [ ] retain the exact tested artifact.

If FAIL:

- [ ] retain failed source/hash evidence if useful;
- [ ] update/open issue with exact reproduction steps;
- [ ] do not replace bytes under the same version/build identifier;
- [ ] fix on a new application source commit;
- [ ] produce a new artifact/hash;
- [ ] retest.

---

## Artifact immutability

Once an artifact has been identified by version/build name + SHA-256, do not silently replace its bytes.

New bytes require a new identifier/hash.

This keeps device reports reproducible.

---

## Stable v1.0 is much later

Stable v1.0 requires substantially more than a successful alpha launch, including:

- broad supported-game hardware validation;
- Master Vault durability/recovery testing;
- corruption/low-space/SD failure handling;
- large-Vault stress/performance testing;
- regression/golden corpus for supported formats;
- import/export/backup documentation;
- any enabled live-write adapter independently satisfying `docs/SAVE_SAFETY.md`;
- packaging/notices/licenses review;
- repeated physical testing.

Until then, label builds as alpha/beta/prerelease accurately.