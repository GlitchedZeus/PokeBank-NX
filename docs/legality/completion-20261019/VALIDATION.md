# Executed validation and practical limits

Implementation commit: `d2253f8a36314216010c873039314a0dbcc92bfb`, a forward child of #314 `b174f06a577b16a45d340741f39fc6b64a8e89bd`. No app/main merge. Source saves never edited.

## Focused host evidence

GCC 16.2.1 and Clang 23.1.1, C++20, -Wall -Wextra -Wpedantic -Werror. Each of these six executables passed normal O2 and ASan+UBSan O1 with leak detection under both compilers: **24/24 executions**.

| Executable | Evidence per execution |
|---|---|
| test_gen2_korean_save | 2,816 two-byte charset pairs, Gold/Silver synthetic layout/pockets/raw names, malformed fields/checksum, input immutability, Korean edit refusal |
| test_gen3_berry_context | Real production item verdict with and without validated source evidence; invalid/no-data handling; RS/E synthetic sources |
| test_gen3_berry_source | 70 title/sector-rotation combinations, all five titles, Japanese/international/unknown charsets, native berry checksum, newer backup selection, source immutability |
| test_legality_report_presentation | 59,049 coverage/severity combinations; all three visible verdict states; warning-free Incomplete is never clean |
| test_legality_gen5_adapter | 5,069 existing and strengthened cases, encrypted/decrypted box/party, malformed/fuzz and rendered-line invariants |
| test_gb_legality_names | 172 checks, including preserved malformed sixth Korean character assertion and new page-index-0x50 case |

Additional: existing strict test_gsc_gen2_adapter passed; actual #307 workspace bridge passed GCC normal and Clang ASan+UBSan with leak detection (four titles each); pinned Korean table regenerates byte-identically; Python syntax, shell syntax, git diff whitespace and app exact-head patch checks pass. Compiler-only fixes exclude Clang from GCC-only diagnostic pragmas and mark an existing helper maybe_unused; no warning flags or assertions were relaxed.

The initial berry production fixture passed decoded bytes to an encrypted-record constructor; it was corrected to encrypt the fixture, preserving its clean-verdict assertion. The name counter initially let a dangling sixth lead evade the old bound; the implementation was fixed and the old assertion preserved. A Japanese-charset test initially expected an ASCII USA name to decode unchanged: source tracing showed full-width Latin; the fixture now tests the actual NoMatch and a real Japanese encoded name. The tracing sandbox prevents LSan; unchanged completed sanitizer binaries passed outside it with detect_leaks=1. No sanitizer finding was hidden.

## Differential evidence

See OBSERVED_METRICS.json, OBSERVED_METRICS.mismatches.csv, DATASETS.json and README.md. Every record remains in the raw denominators, including Unknown/Fishy, Korean, events and GameCube-origin transfers. The Gen I/II existing predicate-aware gate is distinct from raw PKHeX agreement; passing it does not mean 100% agreement. The retained streams contain the exact record IDs/bytes, both verdicts and issue text. Gen V replay preserved all 135,193 record IDs/bytes/metadata while exposing Fishy flags. Three-state agreement is separately reported; Fishy was not silently collapsed into clean.

Current upstream probe **FAILED** with 12 unallowlisted EggMoveCombination cases, retained byte-for-byte in UPSTREAM_BREEDING_CASES.json. Baseline Gen I/II raw differences and Gen IV's 45 previously documented stricter met-location cases remain in the mismatch index and percentages. Gen III five and Gen V 307 PKHeX-invalid baseline records remain Incomplete. The extra Gen V simultaneous-mutation set leaves 178 PKHeX-invalid cases Incomplete. These are unresolved cases, not completed certifications.

The old generators bound some transfer template sampling and can fail conversions. The new Gen V oracle logs caught generation exceptions; those failures do not become passing verdicts. The original holdout generation predates that logging, so its omitted-attempt count is unmeasured. Replay/adversarial sets operate on the retained baseline; they do not retroactively make that generator exhaustive. PKHeX conversions use some unseeded randomness: retain and hash actual corpus files, not only random seeds.

## Review/release decision

Self-review plus adversarial tests completed. **Independent human/agent QA has not occurred**; neither the author-role wording nor passing tests substitutes for it. No native NRO or physical Switch acceptance test here. Full repository host tests and all L1–L5 lane gates were not locally rerun; existing GitHub workflows remain enabled, and the new focused workflow adds evidence rather than replacing them. Workflow results must be checked on the final exact head.

**Not release-ready as a 100% complete Gen I–V legality engine.** Semantic applicability, primary historical provenance, current-upstream father chains, source-context/provider reachability, event/transfer uncertainties, independent disagreement review and native UI acceptance remain required.

## CI source-binding follow-up

The implementation commit's Gen I original-oracle workflow stopped at its immutable source preflight: Legality.cpp changed from blob dac0fe31a7247cb4dd81162d896d27240ce8e84d to ba2d7d2be048d24acd4e196e0446a65feed3ea55. The delta supplies Korean PK2 encoding and optional Gen III berry context; it does not change PK1 encounter/data logic. The follow-up updates only that local coordinator blob pin. External PKHeX/accepted-engine pins, fixture hashes, assertions and minimum source counts are unchanged. The preflight and its normal/duplicate/stale/missing/truncated self-tests pass with 12 local and 3 external pins. Full runtime CI must rerun on the follow-up head; the earlier run remains recorded as failed, not passed.

The affected Gen I coordinator control executable was rebuilt against the final production sources: 72/72 source-context vectors passed GCC normal and ASan+UBSan with detect_leaks=1, and both JSONL streams were byte-identical. No expected vector changed. These focused reruns do not substitute for completion of all final-head CI jobs.

Concurrent coordinator maintenance was preserved: d3727653d3f7d361896a6e749af245415981ddb0 advanced #314 after the implementation push and refreshed the same Gen I pin plus four native-codec audit bindings. The evidence follow-up is a forward child of that commit; none of its runner/fixture fixes was overwritten. These are source-binding updates, not independently adjudicated legality disagreements.
