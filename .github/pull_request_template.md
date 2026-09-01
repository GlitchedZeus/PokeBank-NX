## PokeBank NX change summary

Describe the smallest coherent milestone implemented by this PR.

## Scope

- [ ] This PR targets PokeBank NX, not PKSE upstream.
- [ ] Unrelated refactors/features were kept out where practical.
- [ ] Any new external code/reference use is documented with project/revision/license/reuse classification.

## Safety

- [ ] Live installed-game save writing remains hard disabled, OR this PR explicitly names the adapter/safety gate authorizing a deliberate change.
- [ ] Controller/navigation changes do not create accidental mutation shortcuts.
- [ ] Read-only sources remain read-only unless an explicitly reviewed staged-write milestone says otherwise.
- [ ] No personal saves, ROMs, console keys/credentials, or inappropriate proprietary assets were committed.

## Verification

- [ ] `make -f Makefile.host host-test` PASS
- [ ] `make -f Makefile.host host-sanitize` PASS
- [ ] `git diff --check` PASS
- [ ] native `make -j1` PASS when the Switch build environment is available
- [ ] relevant regression tests were added/updated

## Artifact identity

If this PR produces a device-test `.nro`:

```text
Application source SHA:
Artifact filename:
Artifact size:
SHA-256:
Host tests:
Sanitizers:
Native build:
Physical Switch status: NOT DEVICE TESTED / DEVICE TESTED
```

- [ ] The `.nro` source SHA is the actual application-source commit, not a later docs-only commit.
- [ ] The binary was preserved/provided outside a temporary coding runtime.
- [ ] `DEVICE TESTED` is used only if a human physically ran that exact binary/hash.

## Documentation / tracking

- [ ] `PROJECT_STATUS.md` updated if verified project state changed.
- [ ] `docs/BUILD_RECORD.md` updated for a meaningful device-test artifact.
- [ ] Relevant GitHub issue(s) linked/updated.
- [ ] `docs/NEXT_SESSION_PLAN.md` / `docs/PROJECT_MAP.md` updated if priority/order changed.

## Known limitations / next step

State what remains deliberately unfinished and what the next coherent milestone should be.