# CI workflow and host-Makefile audit — November 2, 2026

Branch `chore/ci-cleanup-20261102`, cut from `origin/main` at `9ce7f737` (PR #330 consolidation). The owner authorized retiring obsolete workflows, but only after confirming two things for each one: that active CI does not reference it, and that any important coverage it had is still provided by a kept workflow.

**Result: 48 workflows → 24.** This branch retires 24 files:

- 21 one-time patch workflows;
- 3 workflows whose coverage is duplicated elsewhere;
- the 20 `tools/` helper files that only the one-time workflows used.

No Makefile changes (§4). The prune script for remote branches, and its recovery guide, are in [RECOVERY.md](RECOVERY.md) and §5.

Workflows on `main` only govern pushes to `main` and PRs into `main`. Every active branch (`fix/round11-gen15-repairs`, `fix/round12-gen15-repairs`, `development/gen6-plus`, the legality line) runs the copies in its own tree, so retiring a file here changes nothing for those branches until they merge `main`. Columns R11 / R12 / G6+ / LEG show whether each retired file still exists on `fix/round11-gen15-repairs`, `fix/round12-gen15-repairs`, `development/gen6-plus` and `claude/gen1-5-legality-campaign-w9mkom` (= `development/legality-engine`).

## 1. Method

- **Triggers and commands.** Parsed every `.github/workflows/*.yml` with PyYAML to list triggers, branch and path filters, and every `make` invocation.
- **Recent runs.** Read the last three runs per workflow from the public API (`/actions/workflows/<file>/runs?per_page=3`) on 2026-11-02.
- **References.** For each file name, ran `git grep` over the whole tree (tests, tools, docs, other workflows). Tests that read workflow files:
  - `tests/test_ci_repository_safety_contract.py`: native-pr-build, host-tests, product-ui-native, and the nine `HISTORICAL / MANUAL` files (AUDIT-005).
  - `tests/test_game_hub_contract.py`: product-ui-native, gen4-shared-editor-candidate.
  - `tests/test_gen5_readonly_ui_contract.py`: native-pr-build, gen5-focused-host, gen15-integrated-candidate.
  - `tests/test_qa_nro_audit_workflow.py` and `tests/test_qa_cross_gen_contract.py`: app-qa-nro-audit, gen15-integrated-candidate.
  - `tests/test_save_instance_architecture_contract.py`: audit-hardening-native.
  - `tests/test_gsc_editor_surface_contract.cpp`: touch-input-gating.
  - `tests/test_qa_nro_audit.py`: native-pr-build.
  - `tests/test_hw_round1_regression_gate.py`: host-tests, host-tests-clang, app-qa-contracts.
  - `tests/test_qa_regressions_assets.py`, through `tools/qa/check_workflows.py`: every file must parse.
  - `tools/qa/manual_workflow_staleness.py`: every dispatch-only file.

  A workflow that a test reads was kept. No test was changed.
- **Coverage.** For each REDUNDANT claim, compared the exact sources, flags and targets with the CORE replacement (`make -f Makefile.host -pq` database for the host graph).
- **Branch protection.** `main` is not protected and has no required status checks (`GET /branches/main` → `protected: false`), so no required check disappears.

Classifications: **CORE** (keep); **REDUNDANT** (coverage fully duplicated by a named CORE workflow); **ONE-TIME** (recovery, freeze, migration or campaign job); **OBSOLETE** (dead trigger or removed code); **UNCERTAIN** (kept and documented).

## 2. Workflow inventory (48 on `origin/main`)

### 2.1 Kept — CORE (12)

| Workflow | Triggers | Builds / tests | Last runs (2026-11-02) | Referenced by | Notes |
|---|---|---|---|---|---|
| `host-tests.yml` | push `main` (+2 old lanes), every PR | tool syntax checks, CI-safety / v1-polish / PNG / species / Fontstash / authority contracts, `make -f Makefile.host host-test`, focused RSE regression, `host-sanitize` with LeakSanitizer | 5,393 runs; success on R12 / legality PRs | tests (safety, hw-round1), README, docs | Main host + sanitizer gate |
| `host-tests-clang.yml` | every PR, dispatch | `CXX=clang++ host-test` + `host-sanitize` | 243; success R12 | tests, docs | Second compiler |
| `native-pr-build.yml` | every PR (unfiltered), push touch lane, dispatch | pinned devkitA64 `make -j1`, packaged RomFS verify, smoke NRO | 1,597; success R12 | tests (AUDIT-002/003, gen5, nro_audit) | Native Switch gate |
| `gen15-integrated-candidate.yml` | every PR, dispatch | Gen V contracts + 6 Gen V bins, devkitA64 `make -j2`, Gen I–V candidate NRO | 239; success R12 | app-qa-nro-audit, tests, docs | Builds the Gen I–V candidate NRO |
| `app-qa-nro-audit.yml` | every PR, dispatch | exact-NRO audit of the gen15 artifact | 153; success R12 | tests, docs | Preserved by owner order |
| `archive-candidate-nro.yml` | dispatch | archives a candidate NRO as a pre-release | 10; last 2026-10-31 | NRO_ARCHIVE.md, branch_audit.py | Preserved by owner order |
| `app-qa-contracts.yml` | PR (path filter: src/include/tests/Makefile.host*) | `qa-contracts`, `qa-matrix` (gcc+clang), `qa-tools-test` | 171; success R12 | docs, hw-round1 test | QA / legality contract gate |
| `app-qa-fuzz.yml` | PR (paths), nightly 03:17 UTC, dispatch | `qa-fuzz-build`, libFuzzer, seeds, smoke + long fuzz | 172; nightly success on `main` | docs | Fuzzing |
| `app-qa-workflow-hygiene.yml` | PR touching workflows, dispatch | `tools/qa/check_workflows.py` | 129; success | docs | Keeps this directory valid |
| `gen5-focused-host.yml` | PR (Gen V paths), push 2 Gen V lanes, dispatch | 15 Gen V host bins + sanitizers, Gen V UI contract | 393; success R12 | test_gen5_readonly_ui_contract.py | Test-pinned fast Gen V gate. The bins are also in host-test, but the test pins the exact lists |
| `touch-input-gating.yml` | PR (UI paths), push touch lane, dispatch | touch / item-sprite / picker Python contracts, three small g++ checks, `build-host/test_gsc_workspace_render` | 939; success R12 | test_gsc_editor_surface_contract.cpp | Several Python contracts run only here |
| `gen12-packed-multimove-focused.yml` | PR (packed-move paths), push 2 old lanes | `tests/test_gen12_packed_multimove.cpp` (O2 + ASan / UBSan) | 1,511; success R12 | docs only | **Only runner of that test**. Not in any Makefile target. See §4.3 |

### 2.2 Kept — UNCERTAIN, test-pinned (3)

| Workflow | Triggers | Evidence | Why kept |
|---|---|---|---|
| `audit-hardening-native.yml` | push `audit/full-project-hardening-20260923` only, dispatch | Last run 2026-09-28. Uses mutable `devkita64:latest`. Packages the physical-durability audit kit (`ENABLE_PHYSICAL_AUDIT_V1.txt`) | `tests/test_save_instance_architecture_contract.py` requires its `include/Source/**` path filter. The physical-audit kit packaging exists nowhere else, and the workflow can still be dispatched from `main`. Recommendation: retire together with a one-line change to that test (native-pr-build is unfiltered, so the test's intent holds) once the owner says the audit kit is no longer needed |
| `product-ui-native.yml` | PRs into 3 retired lanes; push touch / gen5 lanes; dispatch | Last run 2026-10-14 | Required by `test_ci_repository_safety_contract.py` (AUDIT-003 pinned digest) and read by `test_game_hub_contract.py`. Its `product-ui-test` + native build overlap host-tests + native-pr-build, but its trigger never fires for `main` |
| `gen4-shared-editor-candidate.yml` | PRs into 4 retired lanes, dispatch | Last run 2026-10-14 | Read by `test_game_hub_contract.py` |

### 2.3 Kept — ONE-TIME / HISTORICAL, dispatch-only, test-pinned (9)

`gen1-cleanup3-candidate.yml`, `gen1-cleanup3-focused.yml`, `gen1-editor-candidate.yml`, `gen2-audit-candidate.yml`, `gen2-compat-data.yml`, `gen3-shared-editor-candidate.yml`, `inventory-hardware-retest.yml`, `leafgreen-device-build.yml`, `recovery-snapshot.yml`.

All nine are named `HISTORICAL / MANUAL — …` and have only `workflow_dispatch`. Their last runs were in September 2026. `test_ci_repository_safety_contract.py` (AUDIT-005) reads each file by name, and `tools/qa/manual_workflow_staleness.py` checks their embedded source checks. They never run on their own, so they cost nothing. `recovery-snapshot.yml` is the owner's RomFS snapshot tool. `inventory-hardware-retest.yml` is the only caller of `Makefile.host.classic_inventory` (§4.2). Retiring them would need an AUDIT-005 test edit, and that is the owner's call. They are listed here as the next candidates.

### 2.4 Retired on this branch (24)

| Workflow | Class | Evidence | Replacement / retained coverage | R11 | R12 | G6+ | LEG |
|---|---|---|---|---|---|---|---|
| `gen14-hardware-candidate.yml` | REDUNDANT | Every PR, 259 runs. Step-for-step subset of gen15: same identity check, `test_verify_nro_romfs.py`, `RECOVERY_SHA` staging, pinned digest, `make clean; make -j2`, `verify_nro_romfs.py --list`, package + upload. gen15 adds the Gen V contracts. Same tree → duplicate NRO under a stale "Gen1-4" name | `gen15-integrated-candidate.yml` | Y | Y | Y | – |
| `gen12-packed-move-focused.yml` | REDUNDANT | Compiles `test_gen12_packed_move.cpp` with the same 14 sources and the same flags as `Makefile.host.gen14` (`build-host/test_gen12_packed_move{,_sanitize}` ∈ `host-test` / `host-sanitize`; the Makefile comment R2-CHECK-009 says so) | `host-tests.yml` (every PR, unfiltered) + `host-tests-clang.yml` | Y | Y* | Y | Y |
| `v1-polish-native.yml` | OBSOLETE trigger | Only PRs into `audit/full-project-hardening-20260923`; last run 2026-09-28; never ran for `main` | contract → host-tests.yml; `check_device_assets.py` → `tools/prepare_release_romfs.sh` in native-pr-build / gen15; devkitA64 build → native-pr-build; packed move → host-test; multi-move → gen12-packed-multimove-focused | Y | Y* | Y | Y |
| `one-time-audit-forward-integrate.yml` | ONE-TIME | push `feature/gen4-full-editor-20260928` + own-file path filter; last 2026-10-07 | none needed (no tests run) | Y | Y | Y | – |
| `one-time-audit-forward-merge-check.yml` | ONE-TIME | same; last 2026-10-07 failure | none | Y | Y | Y | – |
| `one-time-audit-merge-materialize.yml` | ONE-TIME | same; last 2026-10-07 | none | Y | Y | Y | – |
| `one-time-audit-tree-materialize.yml` | ONE-TIME | same; last 2026-10-07 | none | Y | Y | Y | – |
| `one-time-home-lr-freeze-fix.yml` | ONE-TIME | same; contents: write | none | Y | Y | Y | – |
| `one-time-host-forwarder-helper-scope.yml` | ONE-TIME | same; contents: write | none | Y | Y | Y | – |
| `one-time-product-direct-launch-fix.yml` | ONE-TIME | dispatch-only; **1,280 failed runs**, latest 2026-11-02 (push to `claude/optimistic-euler-ph2z4f`) | none | Y | Y | Y | – |
| `one-time-product-direct-launch-fix-v2.yml` | ONE-TIME | dispatch-only; **1,279 failed runs**, latest 2026-11-02 | none | Y | Y | Y | – |
| `one-time-product-direct-launch-fix-v3.yml` | ONE-TIME | push gen4 lane + own file | none | Y | Y | Y | – |
| `one-time-product-freeze-party-cleanup.yml` | ONE-TIME | "Disabled" stub, dispatch | none | Y | Y | Y | – |
| `one-time-product-hardware-fix-v4.yml` … `-v7.yml` (4 files) | ONE-TIME | push gen4 lane + own file; contents: write | none | Y | Y | Y | – |
| `one-time-product-hardware-recovery-20261005.yml` | ONE-TIME | push gen4 lane + `tools/*_trigger.txt` | none | Y | Y | Y | – |
| `one-time-product-home-lr-noio.yml` | ONE-TIME | push gen4 lane + own file | none | Y | Y | Y | – |
| `one-time-product-performance-no-behavior.yml` | ONE-TIME | same | none | Y | Y | Y | – |
| `one-time-product-performance-run.yml` | ONE-TIME | push gen4 lane + trigger txt | none | Y | Y | Y | – |
| `one-time-quick-games-freeze-fix-20261005.yml` | ONE-TIME | dispatch-only; **1,201 failed runs**, latest 2026-11-02 | none | Y | Y | Y | – |
| `one-time-retroarch-ambiguity-reconcile.yml` | ONE-TIME | push gen4 lane + trigger txt | none | Y | Y | Y | – |
| `one-time-retroarch-nds-ambiguity-fix.yml` | ONE-TIME | same; last run failure | none | Y | Y | Y | – |

`*`: `fix/round12-gen15-repairs` edits these two files (it adds `Gen2UniversalInventory.cpp`, `ClassicInventoryCatalog.cpp`, `ItemQuantityPolicy.cpp` and `ItemNames.cpp` to the source lists, the same four it adds to `GEN12_PACKED_MOVE_SOURCES`). When round 11/12 merges into `main`, resolve the modify/delete conflict by keeping the deletion. The Makefile copy carries the change.

Helper files retired with the one-time workflows. Nothing else references them; `git grep` finds no import and no read. The only mentions are in `docs/qa/WORKFLOW_CLEANUP_PROPOSAL.md`.

- the seven `tools/apply_*.py` scripts;
- `tools/one_time_hardware_hotpath_fix_20261005.py`, `tools/one_time_hotpath_contract_fix_20261005.py` and `tools/one_time_product_direct_launch_fix.py`;
- the five `tools/*_trigger.txt`, three `tools/*_applied.txt` and two `tools/*_checkpoint*.txt` files.

All 21 one-time workflows were already listed for deletion in [WORKFLOW_CLEANUP_PROPOSAL.md](../qa/WORKFLOW_CLEANUP_PROPOSAL.md) §2 (2026-10-14), pending owner approval. Owner approval is now given. That proposal and `docs/qa/FINDINGS.md` still describe the files historically; this branch leaves `docs/qa/` alone because another lane owns the docs.

Restore any retired file with `git show 9ce7f737:.github/workflows/<file>`.

## 3. Validation of this branch

| Check | Before (9ce7f737) | After |
|---|---|---|
| Workflow files | 48 | 24 |
| `python3 tools/qa/check_workflows.py .` | PASS (48) | PASS (24) |
| PyYAML parse of every remaining workflow | – | all parse |
| `python3 tests/test_ci_repository_safety_contract.py` | PASS | PASS |
| `tests/test_*.py` (all) | 38/39 PASS (only `test_qa_hw_round1_gate_mutations.py` failed: it hit the 600 s timeout of the run, under load) | 38/39 PASS, identical per-file results |
| `make -f Makefile.host -j2 host-test` | exit 0; 264 test binaries built and run, all passing (57 min at -j2) | exit 0; the same 264 binaries, 0 rebuilt, all passing. No build input changed: `git diff origin/main -- Makefile* src include tests` is empty |
| `git diff --check` | – | clean |

## 4. Host Makefiles

### 4.1 Inventory and callers

`Makefile` is the devkitA64 / libnx application build. CORE callers: native-pr-build, gen15-integrated-candidate, app-qa-nro-audit, archive-candidate-nro. Many tests read it.

`Makefile.host` is the single host entry point that CORE CI uses. It contains only `include` lines, and its header says the split exists "to keep the accepted pre-GSC test graph byte-identical". The CORE targets are:

- `host-test` and `host-sanitize`: host-tests, host-tests-clang;
- `qa-contracts`, `qa-matrix` and `qa-tools-test`: app-qa-contracts;
- `qa-fuzz-*`, `qa-libfuzzer*` and `qa-seeds`: app-qa-fuzz;
- `build-host/<bin>`: gen15, gen5-focused-host, touch-input-gating.

| File | Included by `Makefile.host` | Direct callers | Status |
|---|---|---|---|
| `Makefile.host.base` | yes | also `make -f Makefile.host.base` in inventory-hardware-retest (historical); read by test_ci_repository_safety_contract (AUDIT-006) | CORE |
| `.gsc` `.editor` `.cleanup3` `.standalone` `.shared-editor` `.hardware-fixes` `.gen4` `.gen5` `.source-instances` `.product-ui` `.exports` `.home-providers` `.gen14` `.qa` `.item-quantity-policy` `.pickers` `.reroll-mechanics` `.species-change-moves` `.round4-r` `.round4-codex` `.host-ui` `.review-entry` `.pkse-upstream` `.multilingual` `.pkse-october` `.pkse-october-pid` `.pkfile-gen12` `.pkfile-gen3` `.pkfile-gen4` `.pkfile-gen5` `.pokemon-file-ui` `.recovery-view` `.jp-gen12` `.jp-gen1` `.jp-gen2` `.bag-top-up` `.pokemon-file-menus` `.machine` | yes (38 fragments) | through `Makefile.host` only; app-qa-contracts path filter `Makefile.host.*` | CORE fragments. Each adds tests to `host-test` / `host-sanitize` |
| `Makefile.host.deps` | yes (must stay last) | `tests/test_qa_host_header_deps.py`, `tools/qa/cxx_with_deps.sh` | CORE (header-dependency tracking) |
| `Makefile.host.classic_inventory` | **no** | only `inventory-hardware-retest.yml` (historical, dispatch-only) | Unused by CORE. **Not redundant**, see §4.2 |
| `Makefile.host.gen5-create` | **no** | none in CI. `docs/GEN5_NATIVE_CREATE.md` and `GEN5_NATIVE_CREATE_VALIDATION.md` document it for local use; gen5-focused-host has it only in its path filter | Redundant for CI: `test_gen5_native_create` and `test_gen5_create_integration` (+ `_sanitize`) are in `host-test` / `host-sanitize`. Its `debug` variant (`-O0 -g3 -D_GLIBCXX_ASSERTIONS`) and `CPPFLAGS` / `CREATE_BUILD` overrides exist only here and are documented for owner use. Kept |

### 4.2 `Makefile.host.classic_inventory`: four tests that no CORE target runs

Its `host-classic-inventory` target builds seven tests into `/tmp`. Three are also in `host-test`: `test_classic_inventory_ui_model`, `test_inventory_ui_contract` and `test_item_quantity_policy`. Four are built **only** here, and CORE CI never runs them:

- `tests/test_classic_inventory_catalog.cpp`
- `tests/test_rby_staged_inventory.cpp`
- `tests/test_gsc_universal_inventory.cpp`
- `tests/test_gen3_staged_inventory.cpp`

It therefore cannot be retired without losing those tests. Recommendation, as a separate change for the owner: add the four to a host fragment with `_sanitize` twins so `host-test` runs them, then retire the file together with the historical workflow. Run here on 2026-11-02: `make -f Makefile.host.classic_inventory host-classic-inventory` exits 0 and all 7 suites pass, so the move is safe to make.

### 4.3 `test_gen12_packed_multimove`

Only `gen12-packed-multimove-focused.yml` builds this test, and only on PRs that touch its path filter. It is not in any Makefile target. That is why the workflow stays CORE. Recommendation: add it to `Makefile.host.gen14` next to `test_gen12_packed_move`, then retire the workflow. This was not done here: `fix/round12-gen15-repairs` edits the same region of `Makefile.host.gen14`, and the change would conflict.

### 4.4 Why the fragments were not merged

- Merging the 38 fragments into fewer files would be a pure refactor, with no coverage gain.
- `Makefile.host` states the split is deliberate.
- `fix/round12-gen15-repairs` edits 11 host Makefiles and `development/gen6-plus` edits 2, so a consolidation now would cause conflicts on every active branch.

Recommendation: consolidate only after round 11/12 and Gen VI+ have merged.

## 5. Branch prune script (owner-run)

`tools/maintenance/prune_branches.sh` carries out Phase 2 of the cleanup in [REPO_CLEANUP_AUDIT_2026-11-02.md](REPO_CLEANUP_AUDIT_2026-11-02.md), using [branch_audit_2026-11-02.json](branch_audit_2026-11-02.json). Its behavior and safety rules are in its header. Restore procedures are in [RECOVERY.md](RECOVERY.md). Its actions are logged in [BRANCH_DELETION_LOG.md](BRANCH_DELETION_LOG.md). Repository description and topics are in [REPO_METADATA.md](REPO_METADATA.md).

### Dry run on 2026-11-02 (this session cannot delete refs or push tags)

The full dry run (`--bundle-source` = a local clone) did the following:

1. Built and verified a 462-ref bundle (git bundle verify, a clone-back ref diff, and every live branch tip present).
2. Re-ran `branch_audit.py --compare` and **aborted correctly** on live drift. `wip/legality-evidence-20261025` (group paused-contained) moved from `eabe41f3` to `da9b6102` on 2026-11-02. Before `--execute`, the owner must regenerate the audit, or wait until that branch settles and re-approve.

The other drift that `--compare` reports is on KEEP branches only, and none of them is targeted:

- the closed legality PRs;
- `development/legality-engine`, which is new and has the same tip as `integration/legality-gen14-major-20261012`; #314 now targets it.

The listing below comes from a run with `--skip-drift-check`:

| Group | Planned (audit) | Would act | Skipped (reason) | Tags |
|---|---:|---:|---|---:|
| contained | 64 | 58 | 5 `claude/*` (protected by rule); `audit/app-qa-20261014` (head of #310, owner-review list) | 0 |
| paused-contained | 160 | 159 | `wip/legality-evidence-20261025` (tip moved) | 0 |
| equivalent-archive | 6 | 4 | 2 `claude/*` | 4 |
| unique-archive | 48 | 47 | 1 `claude/*` | 47 |
| consolidation (opt-in) | 1 | 1 | PR #330 merged; contained in main | 0 |
| legality-base (opt-in, `--allow-legality-base`) | 1 | 1 | no open PR uses it now; contained in `claude/gen1-5-legality-campaign-w9mkom` | 0 |

Against the original 70 DELETE-SAFE, `--group delete-safe` would act on 62 (58 + 4). The difference is the 8 branches the owner's rules exclude: 7 `claude/*` and the head of #310. A full run would create 51 `archive/branches/*` tags (54 planned, minus the 3 `claude/*` exclusions).

No-tag containers verified live:

- `main`;
- `audit/gen1-4-legality-research-20261011`;
- `audit/legality-integration-hardening-20261012`;
- `development/gen6-plus`;
- `fix/round11-gen15-repairs`.

All of them are retained, and the script never deletes them. `audit/legality-integration-hardening-20261012` is still KEEP-PAUSED in the approved plan. The live audit would now call it ARCHIVE because its PRs closed. Keep it, or tag it, before anyone deletes it: about 50 paused branches depend on it for reachability.

The other modes have been tested as far as this environment allows:

- `--tag-pr-heads` dry run: would tag `archive/pr-122` @ `36c0d1d6…`, `archive/pr-163` @ `79679950…` and `archive/pr-280` @ `dcd855a7…`. All three heads were resolved live and match.
- `--archive-nro-fallback` and `--with-metadata` need an authenticated `gh` and refuse without it. With a read-only `gh` test double, the NRO fallback passed the NRO_ARCHIVE.md cross-check, the absence checks and the run/artifact/commit binding, then stopped at the artifact download, which needs authentication. The MILESTONE_PLAN.md parser yields 45 issue/PR assignments.
