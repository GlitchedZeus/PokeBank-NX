# Legality Integration Validation Checklist

This checkout-owned gate is the coordinator's reproducible acceptance record
for Gen I-IV legality integration. It deliberately does not edit shared build
files, CI workflows, legality implementation code, or saved test data.

## Preconditions

- Start from the exact candidate commit in a tracked-clean checkout.
- Record both its commit SHA and tree SHA; a matching branch name is not
  enough evidence.
- Keep the existing L2 contradiction visible: an outer pass must not hide a
  failing lane.

## Full gate

Run the following from the repository root, replacing the two substitutions
with the candidate's immutable identities:

```sh
python3 tools/run_legality_integration_audit.py \
  --expect-head "$(git rev-parse HEAD)" \
  --expect-tree "$(git rev-parse 'HEAD^{tree}')" \
  --timeout 900 --keep-going \
  --output /tmp/pokebank-legality-integration.json
```

The report records the declared L1-L5 and audit-owned program manifests,
compiler/Python identity, per-command outcome, timeout status, and the exact
checkout identity. `--dry-run` validates identity and manifest registration
only; it is not a substitute for executing normal and sanitizer commands.

## Decision rule

Merge only when the JSON report says `passed: true`, the coordinator has
reviewed every listed command, and the result is attached to the same commit
and tree proposed for integration. If any lane fails, preserve its first
failure output, reproduce the individual lane with the recorded command, and
return that defect to its owning lane rather than weakening the outer gate.

The gate exports `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`; sanitizer failures are
therefore integration blockers, not optional diagnostics.
