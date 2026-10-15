# Gen I–IV completion campaign (Thread C) — checkpoint

Branch: `claude/gen14-campaign-20261015`, created from `c952565c` (PR #307 head), forward-merged
with #307 `44a976e1`. Integrated into #307 by Thread 2R (merge only).

Every NRO built from this branch is **NOT DEVICE TESTED**.

## Gates (per push)

GCC and Clang `host-test` + `host-sanitize` (touched targets each unit, full suites per
milestone), every `tests/*.py` contract, `git diff --check`.

## Units

| # | Item | Status | Commit | Tests |
|---|------|--------|--------|-------|
| 1 | QA-F3b: Gen II export publishes with exclusive mkdir (fail-closed), not rename(2) | done | 0bac3cf1 | `test_gsc_export_transaction` (fails against the old rename publish) |
| 2 | PR #280 Gen III/IV empty-slot touch/A parity tests | done (behaviour already correct) | e2e9e0c7 | `test_gen3/gen4_hardware_surface_contract` |
| 4a | Phase 4 Gen I: verified export moved to `Integration/Gen1`; foreign/stale Bag splice refused; game id from parsed save; dead legacy writer delegates | done | e0ef916a | `test_gen1_export_transaction` (R/B/Y; Box, Bag, combined, faults, races, malformed) |
| 3a | Gen III PID-linked editing: no reroll on unchanged confirm, TID keeps shininess, species change drops unrepresentable requests, preview adoption, picker B nav reset | done | 8d7609f5 | `test_gen3_shared_pokemon_session` |
| 3b | Gen I/II: two-step Review discard; Gen I exit-dialog footer modal; Gen I held-stick handoff; Gen II Species tap opens picker; Gen II move-editor picker normalization | done | 34c084a1 | `test_gen12_review_parity` |
| 3c | Gen II Clone from Actions is swallowed by `ClassicReleaseActionFix.inc` (not owned) | reported to coordinator | | |
| 3d | Gen I/II "Y Random DVs" one-press rewrite | owner decision requested | | |
| 4b | Phase 4 Gen II/III/IV Party vs Box, malformed/stale inputs | pending | | |
| 5 | Remaining audit lows (Gen I post-Add focus, move picker drag geometry, Actions cursor after Edit, exit text when clean; Gen II shiny-toggle DV notice) | pending | | |

## Verification log

- Full GCC `host-test` + `host-sanitize`, Clang `host-test`: PASS at 0bac3cf1 tree (Clang sanitize running).
- Every unit: touched targets GCC+Clang, plain + ASan/UBSan; all `tests/*.py`; `git diff --check`.
- Native devkitA64 (pinned CI image `devkitpro/devkita64@sha256:1fc388c3…`) local build PASS through 34c084a1.
- Draft PR #316 (base #307) for CI evidence.

## Next

Phase 4 for Gen II/III/IV (Party vs Box separation, stale/missing/ambiguous source refusal), then the audit lows.
