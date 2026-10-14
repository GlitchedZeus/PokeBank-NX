# Main Development engineering threads: branch and file ownership

Approved by the owner on 2026-10-14. The coordinator session
(`session_01AnyHUH96AxxgM3qguezsET`, branch `claude/optimistic-euler-ph2z4f`)
assigns lanes, reads thread progress, and alone edits `docs/main-dev/**` and
`docs/handoffs/MAIN_DEV_CHECKPOINT.md`.

A thread writes **only** to its own branch and **only** to the files it owns.
If it needs a change in a file another thread owns, it asks the coordinator
(PR comment or its final report) instead of editing that file.

## Starting point

All new lanes branch from PR #307's head `a17c503ad272cc55e69b8b3cbd925cec999567cc`.
That head contains the Gen I–IV baseline from #306 (`4b9b9f65`, candidate
`9bf9d492`) plus #300 (with #302) and #304. Starting every lane there keeps Gen VI
and QA work cleanly mergeable into the Gen V application.

## Thread 1 — Gen I–IV hardware finalization (on call)

- **Branch:** `claude/gen14-final-20261014` (PR #306). Existing session
  `session_01Rrd1TZt7Q8wJTiwzwH3ZWX`, idle until the owner reports hardware
  results.
- **Owns:** `include|src/Integration/Gen1..Gen4/**`, Gen I–IV tests,
  `gen14-*` workflows, and the Gen I–IV docs under `docs/handoffs/GEN14_*`.
- **Preserve:** the exact `9bf9d492` candidate NRO and #306's history.
- **Codex coordination:** Codex owns the Gen I–III **RetroArch exit crash**. No
  Main Development thread edits RetroArch exit/return code until Codex publishes
  its branch or the owner releases the issue. Thread 1 may investigate the Gen IV
  DraStic Quit-to-Launcher return, which is a separate issue.
- Fixes flow forward: #306 → #307 by a forward merge that Thread 2 performs.

## Thread 2 — Gen V production completion (lead integrator)

- **Branch:** `claude/gen5-integration-20261014` (PR #307). Exclusive write access
  starts only after the coordinator confirms the handoff from the legality
  session.
- **Owns:** `include|src/Integration/Gen5/**`, `include|src/Integration/Common/**`,
  the shared editor and HOME UI shell (`include|src/UI/**`), `include/Source/**`,
  `Makefile`, `Makefile.host` and `Makefile.host.gen5|exports|source-instances|product-ui`,
  `native-pr-build.yml`, `gen5-*` workflows and the integrated candidate workflow,
  plus Gen V tests.
- Integrates other lanes forward when the owner approves.

## Thread 3 — Gen VI native save formats

- **Branch:** `claude/gen6-native-20261014`, new, from `a17c503a`.
- **Owns (new files only):** `include|src/Integration/Gen6/**`,
  `tests/test_gen6_*`, `tests/fixtures/gen6/**` (synthetic only),
  `Makefile.host.gen6`, `.github/workflows/gen6-focused-host.yml`,
  `docs/gen6/**`.
- **Allowed shared edit:** one `include Makefile.host.gen6` line in `Makefile.host`.
- **Must not edit:** shared UI, `Makefile`, `Integration/Common`, `Source/`, or
  Gen I–V files. The shared editor is reached by implementing the existing
  `ExactFormatEditorProvider` contract in `Integration/Gen6`. UI and provider
  registration hooks are requested from Thread 2.
- **Reads only:** PR #301 (legality lane) PK5/PK6 transport and PKSM-Core
  (`vendor/PKSM-Core`). Code taken from #301 comes in only as a reviewed,
  cited cherry-pick after owner approval.
- Later Gen VII work (SM/USUM, plus LGPE as its own family) continues on new
  `claude/gen7-*` lanes under the same rules.

## Thread 4 — Application QA, fuzzing and native CI

- **Branch:** `audit/app-qa-20261014`, new, from `a17c503a`.
- **Owns (new files only):** `tests/fuzz_*`, `tests/test_qa_*`,
  `tests/fixtures/qa/**` (synthetic only), `tools/qa/**`, `Makefile.host.qa`,
  new `.github/workflows/app-qa-*.yml`, `docs/qa/**`.
- **Allowed shared edit:** one `include Makefile.host.qa` line in `Makefile.host`.
- **Must not edit** production `src/` or `include/`. Each defect it finds goes to
  the owning thread with a failing regression test that can be ported there.
- Deleting or rewriting the stale `one-time-*` workflows needs owner approval
  first; until then Thread 4 only proposes it.

## Legality Engine boundary

Main Development never pushes to `feature/legality-*`, `integration/legality-*`,
`audit/legality-*`, `claude/pensive-cerf-pueceq`, or PRs #103, #144, #252,
#292–#299, #301, #303. Legality sessions do not push to Main Development
branches.
