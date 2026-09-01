# PokeBank NX — Alpha Release / Artifact Checklist

Last updated: 2026-09-01

This checklist exists because a successful `.nro` that only lives in a temporary coding workspace is not a durable release artifact.

Use it for every build intended for physical testing or public prerelease distribution.

---

## Release naming

During pre-alpha/alpha development, use explicit prerelease versions such as:

```text
v0.1.0-alpha.1
v0.1.0-alpha.2
```

Do not call a build stable merely because it launches.

---

## Source gate

Before building:

- [ ] working tree is understood/clean or all intended changes are committed
- [ ] exact branch recorded
- [ ] exact full commit SHA recorded
- [ ] `PROJECT_STATUS.md` reflects the milestone
- [ ] no unresolved recovery/local-only source exists
- [ ] live-write policy for this build is explicitly known

---

## Automated verification gate

Run:

```bash
make -f Makefile.host host-clean
make -f Makefile.host host-test
make -f Makefile.host host-sanitize
git diff --check
```

Require:

- [ ] host tests PASS
- [ ] ASan/UBSan PASS
- [ ] GitHub host-test CI PASS for the release source commit or equivalent newer source-identical commit

---

## Native build gate

Build the Switch `.nro` using the documented devkitPro environment.

Require:

- [ ] native build PASS
- [ ] exact artifact filename recorded
- [ ] file size recorded
- [ ] SHA-256 recorded
- [ ] build environment/toolchain recorded sufficiently to reproduce

Example:

```bash
sha256sum PokeBank-NX.nro
```

---

## Artifact contents

A test/prerelease package should contain at minimum:

```text
PokeBank-NX.nro
README or release notes
SHA256SUMS.txt
```

If extra runtime assets are required, package them in the exact SD-card layout expected by the application.

Do not package:

- commercial game ROMs
- user save files
- console credentials/keys
- unrelated copyrighted game data that cannot be redistributed

---

## Release notes template

```markdown
# PokeBank NX v0.1.0-alpha.X

Source commit: `<full SHA>`
Artifact SHA-256: `<hash>`

## Verification
- Host tests: PASS
- ASan/UBSan: PASS
- Native `.nro`: BUILDS
- Physical Switch: NOT DEVICE TESTED / DEVICE TESTED

## Current safety posture
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

Never publish `DEVICE TESTED` unless that exact source/artifact was physically tested.

---

## First recovery-era test release target

The first new artifact should be a deliberately limited read-only alpha.

Suggested acceptance target:

```text
boots through hbmenu
controller navigation works
native game cards appear
platform labels are correct
party/boxes can be browsed for at least one supported title
A-button action sheet works if included in the milestone
live writes remain inaccessible
```

There is no requirement to finish Master Vault or cross-generation transfers before producing this hardware-test build.

In fact, testing the native foundation early is preferred.

---

## Installation note for test builds

Expected layout should eventually be standardized as:

```text
/switch/PokeBank-NX/PokeBank-NX.nro
```

If the inherited target still produces a temporary name, release notes must state the actual filename and destination rather than pretending branding cleanup is finished.

---

## Physical test

Use:

```text
docs/DEVICE_TEST_CHECKLIST.md
```

Record:

```text
Switch firmware
Atmosphère/CFW version
handheld/docked
exact source SHA
exact .nro SHA-256
installed games tested
results
crash codes/screenshots
```

---

## After device test

If PASS:

- [ ] update `PROJECT_STATUS.md`
- [ ] mark only the exact tested areas `DEVICE TESTED`
- [ ] attach/report test notes to GitHub issue #8
- [ ] preserve artifact/release so the exact tested binary remains available

If FAIL:

- [ ] keep the failed artifact/hash for reproducibility if useful
- [ ] open/update issue with reproduction steps
- [ ] do not silently replace the binary under the same version/tag
- [ ] fix on a new commit
- [ ] build a new artifact with a new hash/version

---

## Artifact immutability rule

Once an `.nro` has been published and identified by a version/hash, do not replace its bytes while keeping the same version/hash documentation.

New bytes require a new artifact/version or clearly distinct build identifier.

This makes user bug reports reproducible.

---

## Stable v1.0 is much later

A stable `v1.0` should require substantially more than the alpha test gate, including:

- broad supported-game device testing
- Vault durability/recovery testing
- corruption/low-space handling
- large-Vault stress testing
- complete regression/golden corpus for supported formats
- clear import/export/backup documentation
- any enabled live-write adapters individually satisfying `SAVE_SAFETY.md`
- release packaging and notices/licenses reviewed

Until then, use alpha/beta/pre-release labels accurately.
