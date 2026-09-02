# PokeBank NX — Build / Device / Release Checklist

Last updated: 2026-09-02

Use this for every build intended for physical testing, prerelease distribution, release-candidate validation, or v1.0.

Issue #15 tracks persistent artifact automation.  
`docs/V1_ROADMAP.md` / issue #29 track the full product roadmap.

The core rule this file protects:

> A successful `.nro` that only existed in a temporary coding runtime is not a durable/reproducible release artifact.

---

# Verification vocabulary

Keep these claims separate:

```text
IMPLEMENTED
HOST TESTED
NRO BUILDS
DEVICE TESTED
```

A prerelease upload or CI pass does **not** automatically make a build `DEVICE TESTED`.

Physical results must include:

```text
DEVICE TESTED — PASS
DEVICE TESTED — PARTIAL PASS
DEVICE TESTED — FAIL
```

and identify the exact artifact hash.

---

# Application source vs documentation branch HEAD

Normal workflow may be:

```text
application source commit
        ↓
build exact .nro
        ↓
record/hash artifact
        ↓
later docs/status/research commits
```

The `.nro` belongs to the **application source commit**, not automatically the later branch HEAD.

If source changes after a checkpoint, do not reuse the old source identity for new bytes.

---

# Pre-build source gate

Before a publishable/device-test build:

- [ ] repository/worktree is understood;
- [ ] exact development branch is recorded;
- [ ] exact application-source full SHA is recorded;
- [ ] intended application changes are committed;
- [ ] no unresolved merge/conflict state exists;
- [ ] live-write capability/policy is explicitly known;
- [ ] unsafe temporary debug/write flags are absent;
- [ ] current device blocker issues were checked;
- [ ] README/status docs are not being mistaken for application source.

For Session 2.6, read #23/#24/#19/#13/#16 before building.

---

# Host verification gate

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
- [ ] relevant GitHub host CI PASS when available for the exact source/source-identical state.

For parser/storage/write work, add targeted regression fixtures rather than relying only on generic suites.

---

# Native build gate

Build using the documented devkitPro/devkitA64 environment:

```bash
make clean
make -j1
```

Require:

- [ ] native build PASS;
- [ ] final `.nro` comes from the exact recorded application source;
- [ ] embedded/displayed source version/SHA is correct where exposed;
- [ ] exact artifact filename recorded;
- [ ] byte size recorded;
- [ ] SHA-256 recorded;
- [ ] build environment/toolchain recorded sufficiently for reproduction.

Example:

```bash
sha256sum PokeBankNX.nro
```

---

# Build record

Update:

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
Embedded short SHA/version:
Artifact filename:
Artifact size:
Artifact SHA-256:
Persistent artifact location:
Device tested: yes/no
Device result:
Permanent device report:
```

Never invent a filename/hash for a build that has not actually been produced.

---

# Artifact package

A device-test/prerelease package should contain at minimum:

```text
PokeBank NX .nro
SHA256SUMS.txt or equivalent recorded hash
release/test notes identifying exact source
```

If runtime assets become mandatory, package/document the exact SD-card layout.

Do not package:

- ROMs;
- user saves;
- console keys/credentials;
- unrelated proprietary game/system files;
- private diagnostic contents not needed for the test.

---

# Persistent artifact target

Preferred flow (#15):

```text
verified application-source commit
        ↓
host verification
        ↓
reproducible native build
        ↓
.nro + metadata + SHA256SUMS
        ↓
Actions artifact / GitHub prerelease
        ↓
physical device result attached later
```

Artifact automation must **never** automatically mark a build `DEVICE TESTED`.

Until automation exists, explicitly preserve/provide the `.nro` before the coding runtime disappears.

---

# Release naming

Use explicit prerelease identifiers or source-addressed device builds, for example:

```text
v0.1.0-alpha.1
v0.1.0-alpha.2
PokeBank-NX-SecondDevice-<short-source-sha>.nro
```

Do not silently replace bytes under the same artifact/version identifier.

---

# Release notes template

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
- Physical Switch: NOT DEVICE TESTED / DEVICE TESTED — ...

## Safety posture
Live installed-game save writing: DISABLED / or list the exact individually approved adapter(s).

## What works
- ...

## What is incomplete
- ...

## Device-test focus
- ...

## Known issues
- ...
```

---

# Current device-test target — replacement build #2

The first exact device-tested build was:

```text
Application source: 3be4de6b0b1ce00d5fe369cff9795c3fffbfa31a
Artifact: PokeBank-NX-UI-Theme-3be4de6.nro
SHA-256: df7199c528c11b8792cccb483e15d5b2fa742d4d895b8df78b12f329dc90694a
Result: DEVICE TESTED — PARTIAL PASS / KNOWN FAILURES
```

The useful later UI/analog application checkpoint is:

```text
361c6f551496470db305948d702944c6ed9889c1
ui: add visible PokeBank shell and physical stick input
```

That source is not yet the guaranteed final second-device artifact because the extended first-build test found:

```text
#23 inherited mutation-path safety blocker
#24 reproducible old PLA save crash
```

Session 2.6 must preserve the shell/analog work while fixing/auditing these blockers. If application source changes, create a **new source commit** and build from it.

Before second-device handoff require:

- [ ] #23 mutation paths classified;
- [ ] unsafe installed-source Release/Create/Move/Edit/save UI blocked;
- [ ] low-level live-write hard lock intact;
- [ ] #24 defensive PLA failure path hardened as far as available evidence permits;
- [ ] Left Stick source fix preserved;
- [ ] visible PokeBank NX shell preserved;
- [ ] host tests PASS;
- [ ] ASan/UBSan PASS;
- [ ] `git diff --check` PASS;
- [ ] native `.nro` BUILDS;
- [ ] exact app-source SHA pushed/verified;
- [ ] exact artifact filename/size/SHA-256 recorded;
- [ ] artifact preserved/provided;
- [ ] status says `READY FOR SECOND DEVICE TEST / NOT DEVICE TESTED`.

---

# Second physical Switch matrix

Test at minimum:

```text
BOOT
VISIBLY POKEBANK NX
OBVIOUS PKSE BRANDING REMOVED
D-PAD
LEFT STICK SINGLE TAP
LEFT STICK HELD REPEAT
LEFT STICK DIAGONAL
A ACTION SHEET
B/CANCEL
NO RELEASE ON INSTALLED SOURCE
NO CREATE ON INSTALLED SOURCE
NO UNSAFE MOVE/MULTI ON INSTALLED SOURCE
EDIT CANNOT WRITE INSTALLED SOURCE
OLD PLA SAVE OPENS OR FAILS GRACEFULLY / NO CRASH
OLED BLACK
DARK
LIGHT
THEME PERSISTENCE
PARTY
BOXES
STORAGE
NO NEW CRASHES
```

Only exercised behavior becomes device evidence.

Use `docs/DEVICE_TEST_CHECKLIST.md` plus an issue-specific checklist if generated.

---

# After physical test

If PASS/acceptable partial pass:

- [ ] update `PROJECT_STATUS.md`;
- [ ] update `README.md` roadmap checklist;
- [ ] update `docs/BUILD_RECORD.md`;
- [ ] update `docs/NEXT_SESSION_PLAN.md`;
- [ ] update `docs/PROJECT_MAP.md`;
- [ ] update `docs/V1_ROADMAP.md` completed/in-progress states;
- [ ] update/close only issues whose exact acceptance gates passed;
- [ ] mark only exercised capabilities `DEVICE TESTED`;
- [ ] retain exact tested artifact.

If FAIL:

- [ ] retain the failed exact source/hash evidence;
- [ ] update/open issue with exact reproduction;
- [ ] do not replace bytes under the same identifier;
- [ ] fix on a new application source;
- [ ] produce a new artifact/hash;
- [ ] retest.

---

# Installation note

Target standardized layout:

```text
/switch/PokeBank-NX/PokeBank-NX.nro
```

If the actual tested artifact/output uses a different filename, release/test notes must state the exact file tested.

---

# Artifact immutability

Once an artifact has a version/build name + SHA-256, do not silently replace its bytes.

New bytes require a new identifier/hash.

---

# Beta / release-candidate gates

Before calling a build beta or RC, require progressively stronger evidence:

- [ ] advertised read adapters have regression coverage;
- [ ] malformed/unsupported saves fail gracefully;
- [ ] Master Vault durability/recovery tests exist once Vault is implemented;
- [ ] large-Vault performance/memory tests exist;
- [ ] conversion/staging has golden comparisons;
- [ ] device-test artifacts are reproducibly preserved;
- [ ] README/support matrix matches actual capability;
- [ ] no generic/global live-write switch bypasses per-adapter safety.

---

# Stable v1.0 release gates

Detailed product sequencing: `docs/V1_ROADMAP.md` / issue #29.

Before a v1.0 tag/release:

- [ ] all advertised read adapters pass their defined host/device gates;
- [ ] every advertised live-write adapter independently passes `docs/SAVE_SAFETY.md`;
- [ ] no unsupported parser path crashes the application;
- [ ] Vault transaction interruption/recovery is tested;
- [ ] low-space/storage-health/index-rebuild behavior is tested;
- [ ] large synthetic Vault soak passes;
- [ ] handheld + docked UI passes;
- [ ] sleep/resume/controller reconnect passes;
- [ ] release-candidate hardware torture pass completed;
- [ ] packaging/notices/licenses reviewed;
- [ ] exact release source SHA frozen;
- [ ] host tests + sanitizers + native build pass;
- [ ] exact release `.nro` filename/size/SHA-256 preserved;
- [ ] README/support matrix/release notes updated from actual release state;
- [ ] v1.0 tag/release created.

Until these gates are met, use alpha/beta/RC terminology accurately.