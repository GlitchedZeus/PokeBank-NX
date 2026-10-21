# L6 Stage 6: executable evidence qualification

Review target: PR #299, `audit/legality-integration-hardening-20261012`.
Stage 6 adds an audit-owned C++ qualification interface and two native
fixtures. Production legality, the parity ledger, and L1–L5 sources are
read-only inputs. The implementation and runner integration are separate
commits for coordinator review.

## Implemented boundaries

`tools/legality_l6_stage6_qualification.h` validates supplied receipt metadata:
reference revision, application tree, original game identity, native specimen
digest, matching specimen used by the reference runtime, completion/exit
status, comparison result, and policy-required parent/encounter/event/transfer
witnesses. A consistent receipt is an intermediate result, never authenticated
parity or permission to accept production code. Synthetic/source-model evidence
stays Unknown even if all other fields assert success.

Event qualification binds projection index plus species/TID/SID/card ID and
reference revision. Missing archive digests, original release receipts, or
review rights stay Unknown. Redistribution permission is an independent
restriction; this tool neither reads nor exports original archive binaries.

The native integration fixture includes the real production
`Gen4EventTemplate.h`, consumes all 336 Q17 game/family cell identities and all
247 Q07 event identities, and checks every event against the compiled
projection. Catalog booleans, identity coverage, revisions, and rights must
match the reviewed schema. New provenance claims require an independently
reviewed proof adapter, rather than silently promoting these existing inputs.
The production projection blob must match Q07's recorded Git blob.

## Execution and scope

`tools/audit_l6_stage6_evidence.py` builds both fixtures and runs a third catalog
integration invocation for each compiler/profile. It emits HEAD, tree,
compiler version, profile, duration, source SHA-256 values, and actual counts:

| Native execution | Checks per compiler/profile |
| --- | ---: |
| Six-lane release receipt state mutations | 67,974 |
| Parity/history/event receipt qualification mutations | 12,525 |
| Existing Q17/Q07 catalog joined to production projection | 495 |
| Total | 80,994 |

GCC and Clang normal and ASan/UBSan executions passed during development:
323,976 checks across four profiles, eight compiled binaries and twelve native
invocations. These are executable synthetic metadata assertions, not a count
of authenticated retail specimens. Final clean-commit reproduction:

```sh
python3 tools/audit_l6_stage6_evidence.py --compiler g++ \
  --expect-head "$(git rev-parse HEAD)" --expect-tree "$(git rev-parse 'HEAD^{tree}')"
python3 tools/audit_l6_stage6_evidence.py --compiler clang++ \
  --expect-head "$(git rev-parse HEAD)" --expect-tree "$(git rev-parse 'HEAD^{tree}')"
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest \
  tests/test_legality_l6_stage6_evidence.py \
  tests/test_legality_integration_audit_runner.py \
  tests/test_legality_l6_stage2_matrix.py \
  tests/test_legality_l6_stage4_audit_failures.py
```

Pinned execution refuses a dirty checkout, including untracked source files.
Unpinned development runs explicitly report cleanliness and hashes. Source
changes during a run, compilation errors, timeouts, and process failures fail
the gate and retain a machine-readable failure receipt.

## Confirmed audit infrastructure gaps corrected

- Stage 4 used its default GCC compiler and Stage 5 hardcoded GCC even when
  the master runner was invoked with `--compiler clang++`. Both now propagate
  the requested compiler; a regression checks Stage 4–6 commands.
- The Stage 2 matrix counted three audit fixtures after the previous merge,
  omitting six existing Stage 2/4/5 fixtures. It now matches the master manifest
  exactly: eleven audit fixtures including both Stage 6 programs, 114 total
  programs under the previously recorded lane receipt snapshot.
- The inherited Stage 6 release-evidence header had no compiled regression or
  execution gate. It is now exercised and both Stage 6 fixtures are registered.

## Remaining evidence and integration blockers

The inspected checkout has neither a `dotnet` runtime on PATH nor a PKHeX
verifier binary in the repository. Actual PKHeX runtime comparison was not
executed. The pinned reference is
`6501f0ab46e8f8ca048539dbaf8cae8cb104e722`, as recorded in the reference freeze;
it is not asserted to be current upstream.

Q17 still has zero authenticated original specimens/runtime comparisons;
all 336 cells remain Unknown. All 247 Q07 projections lack authenticated
archive/release receipts and remain Unknown. No third-party archive was
downloaded, imported, or redistributed.

The audit checkout's full lane matrix still lacks 49 programs relative to its
recorded L1–L4 receipt snapshot and has two L5 wrapper source-pin mismatches.
Stage 6 success does not clear that matrix or the historical analyzer blockers.
The next stage is Stage 7's rehearsal on a freshly reconciled integration tree.
