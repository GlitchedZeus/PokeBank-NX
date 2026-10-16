# L6 Stage 1 Integration Baseline

Baseline integration commit: `2e9dbc0136ebdd8e4a00e335b171c48da5fdfdfc`.
Audit checkout: `2523969e21ea6745672ee9f865219c19486f01c5` before this stage.

PR #298's historical workflow `38296998403` is now fully complete: host, L1,
L3, L4, and L5 are green; only L2 is red. The audit-owned baseline runner
reproduces the L2 failure at
`tests/test_legality_l2_dp_alternative_egg_histories.cpp:130`:

```text
A::evaluate(dup).status == MissingOrDuplicateCandidateIdentity
```

The assertion contradicts the immediately following scenario: both construct
the same duplicate route identity with the first row unattested and the second
row independently attested, but the following scenario expects the valid later
row to survive. The native evaluator returns the latter result, as required by
the non-authenticating conditional-evidence rule.

The currently fetched L2 head `7e3c0a2a558d489b429795fc22a35cd2dd95fafb`
contains the lane-owned correction and broader 17-suite matrix. Coordinator
action: integrate that forward L2 work (including its corrected duplicate-route
expectations) into the coordinator branch; do not copy or alter L2-owned tests
from this Codex branch.

`tools/audit_l6_stage1_baseline.py` executes the unchanged lane L2 runner and
records the result as a classified red baseline, then runs an independent
production-header duplicate-route contract with GCC and Clang, normally and
under ASan/UBSan. A baseline report is passing only when the known red state is
actually reproduced and every executed native contract passes; it never turns
the unexecuted remainder of L2 into a green result.
