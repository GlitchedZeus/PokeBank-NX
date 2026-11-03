# PokeBank NX — Project rules

Owner decisions reaffirmed 2026-11-02. These rules apply to Claude, Codex, ChatGPT and all contributors. This file records project policy; actual GitHub branch protection and CI settings must be configured separately.

## 1. Never discard unfinished work

- Do not delete a branch because it is old, inactive, named `wip`, looks duplicated, or has an archive tag.
- A branch may be deleted only after its **complete commit ancestry** is verified live as contained in a surviving branch, its live SHA matches the reviewed SHA, and it is not the head or base of an open PR. Patch-equivalent changes or a recovery tag alone are **not** proof of integration.
- Preserve unmerged Gen VI–IX code, Vault work, emulator integrations, legality research and algorithms, test fixtures, owner hardware evidence, and unique history.
- Keep a verified off-repository Git bundle and SHA/ref manifest before deletion batches. Document deleted branch name, original tip, containing ref, and recovery path. Treat a failed or partial deletion run as an incident to audit and restore before continuing.
- Long-term topology, *after* evidence-backed integration: `main`, one app development branch, and one legality development branch. Do not bulk merge to achieve this count.

## 2. One clear owner per change

- Check existing open PRs and branches before starting new work. Extend the correct PR rather than creating a competing implementation.
- Close a duplicate PR only when its alternative is identified and preserved; do not delete unique branch history as part of PR cleanup.
- Keep Gen I–V release repairs, Gen VI–IX application, legality, infrastructure, and research in clearly identified lanes. Do not silently mix them.
- Each change report must name its PR, full or abbreviated commit SHA, exact base branch, remaining blockers, and what has **not** been verified.

## 3. Verification is not hardware acceptance

- Test the exact branch head, not a parent or an older NRO. Record successes **and failures** for GCC, Clang, ASan/UBSan, regression/contract tests, native devkitA64, and artifact checks where applicable.
- Never disable, weaken, or delete tests to make a failing PR green. Investigate whether an NRO identity audit rejects an outdated binary versus an actual regression; document the distinction without calling it a pass.
- A generated NRO must identify its source commit, build, SHA-256, package contents, and `NOT DEVICE TESTED` status until the owner tests it.
- Only owner sign-off of the exact NRO and every applicable action across the 20 Gen I–V game releases can close hardware acceptance: [issue #313](https://github.com/GlitchedZeus/PokeBank-NX/issues/313).
- A bug that failed on Switch remains open until a matching, explicitly recorded Switch retest passes. Repeated bug regressions require permanent regression tests and examination of shared causes.

## 4. Save data and Pokémon correctness come first

- Original emulator and console saves are immutable through browsing/editing. Stage changes; require explicit export/injection with confirmation. Never silently overwrite original saves.
- No unsupported conversion or transfer is allowed; notably, no assumed SWSH↔SV path. Preserve exact bytes where unchanged and reject unsafe or unrepresentable data.
- No Pokémon is called legal or `No problems found` unless the dedicated source-backed legality engine establishes complete applicable coverage. Unknown or unsupported evidence means **Incomplete**, not a false clean.
- The independent legality engine remains outside the release NRO until the owner accepts its accuracy and integration.

## 5. Build and repository hygiene

- The repository root contains only `Makefile` for the Switch build and `Makefile.host` for host tests. Host recipes belong in the ordered `build-system/host/*.mk` modules; keep both standalone suites in `build-system/host/standalone/`.
- Do not introduce a new root `Makefile.host.*`. New test rules must join an existing module or receive a reviewed module change, preserve test reachability and ordering, and update CI references.
- Consolidate rather than accumulate duplicate docs, workflows, issues, PRs and implementation shells. Preserve historical test/QA evidence and working build paths.
- Keep the README badges/layout and its claims consistent with real merged code and device acceptance. Do not advertise draft Gen VI–IX features as production-ready.

## 6. Release order and handoffs

- Prioritize Gen I–V save discovery, profile isolation, editor access, crash and Create/Import blockers before declaring the classic app accepted.
- Preserve and continue preparing Gen VI–IX in a separate lane, but do not assert all-generation parity or merge unfinished features into the accepted classic release.
- Record a concise handoff in GitHub: what changed, full SHA, passing/failing checks, unmerged branches, remaining blockers and exact next step. A chat timing out is not evidence that work was completed.
- Do not promise future asynchronous delivery. Only report commits, workflows, artifacts and actions that actually exist.

## 7. Upstream-first engineering and reuse decisions

- **Before implementing or replacing a substantial subsystem** (save readers/writers, emulator discovery, Gen I–IX record codecs, encounter/location data, legality rules, Pokémon conversion, Vault/Bank storage, events, editing workflows, or test-fixture infrastructure), inspect the relevant existing PokeBank NX code **and** the matching references in `docs/REFERENCE_INDEX.md`, `docs/RESEARCH_REFERENCE_MATRIX.md`, `docs/UPSTREAM_AUDIT.md`, and any focused research audit. These are research indexes, **not** current branch/status authority; verify relevant facts live.
- Compare concrete upstream files/functions and **pinned commit IDs**, not just README claims. Start with the relevant proven implementations: PKSE, the vendored and current upstream PKSM-Core, PKHeX, PKSM, OpenHomeNX/OpenHome, PKForge, pkHouse and Pokémon Chest; consult others where the feature warrants it. Check our existing implementation and test coverage before saying a feature is missing. An upstream feature is **not** proof that it is correct or native-Switch-compatible.
- For each substantial task record a short **reuse decision** alongside its implementation PR or engineering issue: (1) existing local implementation, (2) relevant upstream repo / exact commit / files, (3) `already implemented`, `direct reuse`, `selective adapter/port`, `reference/test oracle`, or `independent implementation required`, (4) license, attribution and redistribution constraints, (5) chosen integration boundary and reason, and (6) regression or golden tests proving behavior and byte fidelity. When independent implementation is necessary, explain why using the upstream code would be unsuitable.
- **Prefer a small, tested native adapter or generated data pipeline** over a second bespoke implementation. Reuse PokeBank NX's existing interfaces and preserve functioning features. Do not wholesale import a desktop framework, an upstream bank schema, direct-write paths, unrelated UI, event payloads, or GPL-incompatible code. Reference-only projects cannot be copied without appropriate rights.
- When a source or generator changes, pin its version, preserve reproducible inputs/hashes where practical, compare against an independent oracle or game evidence, run the affected host/native and staged round-trip tests, and update the relevant research index to `integrated`, `pending`, or `rejected` with evidence. Research findings are **not** marked integrated until working code and tests exist on the recorded branch.
- **Scope and delivery protection:** this research gate applies to meaningful new/rewritten subsystems, not trivial fixes or urgent regressions. Do not postpone a validated Gen I–V hardware candidate, rework active lanes, or create duplicate PRs merely to satisfy a research exercise. An unverified upstream update is not grounds to replace proven code.

Related state: [branch cleanup record](docs/maintenance/BRANCH_CLEANUP_STATE_2026-11-02.md), [Gen I–V acceptance issue](https://github.com/GlitchedZeus/PokeBank-NX/issues/313), and [open PRs](https://github.com/GlitchedZeus/PokeBank-NX/pulls).
