# PokeBank NX — Exact Device Artifact Packaging

Purpose: make device-test `.nro` handoff reproducible and prevent application-source SHA / docs-HEAD / hash mixups.

Tool:

```text
tools/package_device_build.py
```

The packager is stdlib-only and should be run **while HEAD is the exact application-source commit used for the clean native build**, before later docs-only commits are added.

## Normal second-device flow

After source verification/tests and the clean native build:

```bash
python tools/package_device_build.py PokeBankNX.nro --label Second-Device --zip
```

By default it:

```text
refuses a dirty worktree
runs tools/check_device_assets.py
reads full + short git HEAD SHA
checks that the short SHA appears in the .nro bytes
computes artifact byte size
computes SHA-256
copies to a source-addressed filename
writes a plain-text manifest
optionally creates ZIP fallback
refuses to overwrite previous evidence
```

Expected naming pattern:

```text
PokeBank-NX-Second-Device-<shortsha>.nro
PokeBank-NX-Second-Device-<shortsha>.nro.manifest.txt
PokeBank-NX-Second-Device-<shortsha>.zip
```

Default output directory:

```text
dist-device/
```

## Evidence boundary

The generated manifest says:

```text
READY FOR DEVICE TEST
NOT DEVICE TESTED
```

Packaging, embedded-SHA verification and hashing do **not** promote an artifact to `DEVICE TESTED`.

Only a human physically running that exact filename/hash on Switch can do that.

## Overrides

The script has escape hatches such as:

```text
--allow-dirty
--skip-asset-preflight
--skip-embedded-sha-check
```

These are diagnostic/emergency overrides, not the normal device-test path. If any override is used for an artifact that is handed to a tester, document exactly why and do not present the corresponding check as PASS.

## Relationship to docs commits

Application source identity remains the commit used to compile the binary.

If README/status/build-record commits are added afterward, those later branch HEADs do not become the `.nro` source SHA.

The artifact manifest should be copied into the project/device-test record before those docs updates are committed.
