# Durable generation ordering — independent QA patch archive

**UNMERGED QA EVIDENCE ONLY — not release-approved.**

The independent QA agent found a recovery-ordering bug in `src/Utils/DurableFile.cpp`: `uniqueSibling()` chooses the lowest available `.previous.N`, while `recoverMissingTarget()` selects the highest N as newest. Deleting lower-numbered backups can cause recovery to choose an older copy.

- `fix-durable-generation-order.patch` — standalone local fix, originally reported as commit `b531e3d1ed1649944f64d7504ad55ad948b91f32` on QA branch `qa2/work`.
- `regression-test-durable-generation-order.patch` — adds `tests/test_durable_generation_order.cpp` and wires host/sanitizer targets.

User-provided original file SHA-256:
- fix: `20aab4c76039bddc83d605e71876bf5e9b7332849a44b43c2eb8b8b76d683917`
- test: `4a3b05c4697550185f7ea583bbc05d1689a67d65c7f7cad6379a0ec5f7239d38`

The QA agent reported a failing-before/passing-after reproduction, host-test and ASan/UBSan passes, but no hardware or Recovery UI test. Review the actual patch and rerun all gates before integrating.

**Review flags:** the provided implementation falls back to the lowest-free generation after exhausting `0..999999`, potentially reintroducing the ordering problem. Also review handling of `opendir` failures and existing malformed/ambiguous `.previous.N` names. A conservative safe failure is preferable to silent stale recovery.

This directory is on a dedicated archive branch and is **not** part of application builds or `main`.
