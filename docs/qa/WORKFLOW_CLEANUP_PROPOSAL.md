# Workflow cleanup proposal (CI hygiene)

**Status: PROPOSAL ONLY. Nothing has been deleted or edited.** Deleting or rewriting workflows needs
the owner's approval (docs/main-dev/THREADS.md, Thread 4). Thread 4 owns only `app-qa-*.yml`.

Inventory taken on 2026-10-14 against the PR #307 tree (`a17c503a`) and the repository's registered
workflow list from the GitHub API (181 registered workflows, most of them only on older branches).

## 1. Urgent: three invalid workflow files fail on every push to every branch

These files are not valid YAML. GitHub cannot read their `on:` filters, so it records a **failed
run for every push to every branch** that contains them, under the file path instead of a name
(QA-F2 in [FINDINGS.md](FINDINGS.md); reproducer: `python3 tests/test_qa_known_failures_assets.py`).

| File | YAML error at | Failed runs so far | Latest examples |
| --- | --- | --- | --- |
| `one-time-product-direct-launch-fix.yml` | line 70 | 1,128 | run 38644385291 (`claude/gen6-native-20261014`), 38643805374 (`claude/optimistic-euler-ph2z4f`) |
| `one-time-product-direct-launch-fix-v2.yml` | line 67 | ~1,123 | run 38643570357 (`audit/app-qa-20261014`) |
| `one-time-quick-games-freeze-fix-20261005.yml` | line 42 | 1,049 | run 38644386442 (`claude/gen6-native-20261014`) |

Each push to any lane gets three red entries that have nothing to do with the change, which hides real
failures and makes "CI green" unreadable. The intended one-time patches were applied long ago (see §2).

**Proposed action:** delete the three files on the integration line that every lane branches from
(and therefore on every new lane). Fixing their YAML instead would re-arm one-time patch jobs that
hold `contents: write`, which is worse.

## 2. Stale `one-time-*` patch workflows (21 files in this tree)

Every file below is a single-use "apply a patch and push it" job. All are bound to
`feature/gen4-full-editor-20260928` and to a path filter on their own file or a `tools/*_trigger.txt`
(except `one-time-product-freeze-party-cleanup.yml`, which is dispatch-only). 17 of the 21 hold
`permissions: contents: write` and run `git push`; the read-only four are
`one-time-audit-forward-merge-check.yml`, `one-time-audit-merge-materialize.yml`,
`one-time-audit-tree-materialize.yml` and `one-time-product-freeze-party-cleanup.yml`. They will not run again unless someone edits the
file or trigger on that branch, but they are latent write-capable automation with no remaining
purpose, and three are broken (§1).

| Workflow | Patch script it runs (if any) |
| --- | --- |
| `one-time-audit-forward-integrate.yml` | inline |
| `one-time-audit-forward-merge-check.yml` | inline |
| `one-time-audit-merge-materialize.yml` | inline |
| `one-time-audit-tree-materialize.yml` | inline |
| `one-time-home-lr-freeze-fix.yml` | `tools/apply_home_lr_freeze_fix.py` |
| `one-time-host-forwarder-helper-scope.yml` | inline |
| `one-time-product-direct-launch-fix.yml` | inline (**invalid YAML**) |
| `one-time-product-direct-launch-fix-v2.yml` | inline (**invalid YAML**) |
| `one-time-product-direct-launch-fix-v3.yml` | `tools/one_time_product_direct_launch_fix.py` |
| `one-time-product-freeze-party-cleanup.yml` | inline (workflow_dispatch, read-only) |
| `one-time-product-hardware-fix-v4.yml` | inline |
| `one-time-product-hardware-fix-v5.yml` | `tools/apply_product_hardware_fix_v8.py` |
| `one-time-product-hardware-fix-v6.yml` | `tools/one_time_hotpath_contract_fix_20261005.py` |
| `one-time-product-hardware-fix-v7.yml` | inline |
| `one-time-product-hardware-recovery-20261005.yml` | `tools/apply_focused_product_recovery_20261005.py` |
| `one-time-product-home-lr-noio.yml` | `tools/apply_home_lr_navigation_noio.py` |
| `one-time-product-performance-no-behavior.yml` | inline |
| `one-time-product-performance-run.yml` | `tools/apply_product_performance_no_behavior.py` |
| `one-time-quick-games-freeze-fix-20261005.yml` | inline (**invalid YAML**) |
| `one-time-retroarch-ambiguity-reconcile.yml` | `tools/apply_retroarch_ambiguity_reconcile.py` |
| `one-time-retroarch-nds-ambiguity-fix.yml` | `tools/apply_retroarch_nds_ambiguity_fix.py` (last run 2026-10-07: failure, run 37652258386) |

Matching one-shot helper files in `tools/` that exist only to drive these jobs:
`*_trigger.txt` (5), `*_applied.txt` (3), `*_checkpoint*.txt` (2), `one_time_*.py` (3) and
`apply_*.py` (7). They should go with their workflows, after confirming nothing else imports the
`apply_*.py` scripts.

**Proposed action:** delete all 21 workflows and their helper files in one reviewed commit on the
integration line, after the owner approves the list.

## 3. Active workflows filtered to branches that no longer move

Not harmful, but they never run on current lanes and look like coverage that does not exist. Proposed
action: either retarget their filters to the current integration line or delete them.

| Workflow | Trigger filters |
| --- | --- |
| `audit-hardening-native.yml` | push `audit/full-project-hardening-20260923` |
| `v1-polish-native.yml` | PR into `audit/full-project-hardening-20260923` |
| `gen4-shared-editor-candidate.yml` | PR into four September/October lanes only |
| `product-ui-native.yml` | PR into three older lanes only (push filters include `integration/gen5-ui-hardware-candidate-20261012`) |
| `gen12-packed-move-focused.yml`, `gen12-packed-multimove-focused.yml` | push on September lanes; PR path-filtered (still useful) |
| `gen1-cleanup3-*`, `gen1-editor-candidate.yml`, `gen2-audit-candidate.yml`, `gen3-shared-editor-candidate.yml`, `inventory-hardware-retest.yml`, `leafgreen-device-build.yml` | `workflow_dispatch` only: historical candidate builds |

## 4. Registered workflows that exist only on old branches

The GitHub registry lists 181 workflows. About 140 do not exist in the current tree (for example 60+
`one-time-*`/`one-shot-*` files, `gen2-semantic-diagnostic*`, `classic-inventory-*`, `standalone-*`,
`rby-itemsfix-*`). They only run when someone pushes to a branch that still carries them. No action
is needed in the tree; if the owner wants a clean Actions sidebar, they can be **disabled** in the
GitHub UI (reversible) rather than deleted.

## 5. Not proposed for change

`native-pr-build.yml`, `host-tests.yml`, `host-tests-clang.yml`, `gen14-hardware-candidate.yml`,
`gen5-focused-host.yml`, `touch-input-gating.yml` and `recovery-snapshot.yml` are live gates or
owner tools. Note that `host-tests.yml`, `host-tests-clang.yml`, `native-pr-build.yml` and
`gen14-hardware-candidate.yml` run on every pull request without path filters; that is intended.
