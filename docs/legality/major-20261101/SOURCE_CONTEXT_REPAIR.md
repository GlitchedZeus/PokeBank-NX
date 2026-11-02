# Preserve known context and reject clean reports for unknown titles

The full Host CI at44016106, job124751719986, stopped in the unchanged
`test_gen3_legality_context.cpp:342`: a synthetic Ruby record is Invalid for its
hatched met level, but Ruby remains its known host title. The new coverage merge
correctly retained incomplete areas; the native producer incorrectly initialized
source-game coverage Partial for every uncertain record and thereby erased the
independent known context. This failure and its full log are retained separately
from the focused suites that had passed. No test expectation is weakened.

Gen III now marks its exact GBA source-game fact independently of encounter,
checksum and history results. Gen IV likewise retains a recognized host title
when other areas are Partial. All unresolved history fields continue to downgrade
through the same per-area minimum; the repair does not skip incomplete coverage.

The Gen IV audit also found the opposite existing bug: an unknown host title used
a default HGSS personal table, and a zero-finding result marked every coverage
area Complete. A literal native API probe, using the same public control bytes
and frozen c36a2e7c Gen4 source, returns clean for the empty label, unknown_nds,
ruby_gba and diamond_nds_extra. The repaired code returns Incomplete with Partial
source-game coverage and zero findings for all four. The actual diamond_nds
control remains clean. These are targeted API-context controls, separate from the
PKHeX synthetic accuracy denominator; no reference result is invented for them.

Permanent coverage tests exercise those contexts through the actual native
analyzers, require known Ruby/Ranch source coverage to stay Complete while their
unresolved internal/RNG areas remain Partial, and remove warnings to verify the
missing-coverage verdict survives. The exact previously failed Host test is added
to local GCC/Clang normal and ASan/UBSan/LeakSanitizer validation. The existing
Host CI continues to run that original test unchanged.

All six full reference streams retain identical input bytes. Every known-title
benchmark verdict remains unchanged from the c36a2e7c repair, including the
preserved raw false Invalid, false clean, Incomplete, Fishy and reference-error
counts. The source-area metadata becomes more precise. Known title identity is
not historical authentication or proof of an encounter. General finding/history
coverage, regional evidence, real-acquisition custody, production report rendering
and device acceptance remain release blockers.

Local GCC/Clang normal and ASan/UBSan/LeakSanitizer validation passes all ten
applicable suites (40 executions), now including the exact previously failed
Host test. The independently rebuilt twenty-title matrix passes183/183 both
normally and with sanitizers. SOURCE_CONTEXT_VALIDATION.json retains the exact
sample sizes, profile results and literal before/after native API output. The
broader normal Host suite is also running independently; full remote exact-head
CI remains a separate acceptance gate and is not inferred from these local checks.
