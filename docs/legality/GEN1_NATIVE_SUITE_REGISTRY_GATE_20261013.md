# Gen I exact-head suite registry acceptance

The coordinator has 22 focused Gen I source-specific native C++20 programs
and the independent full L3 native runner has 29 programs, comprising 45
distinct source files. The new independent preflight checks that the runner
lists, compulsory source-provenance classes and native+ASan/UBSan compilation
flags cannot silently regress or skip source files.

The workflow now triggers when the existing Gen I editor move compatibility
data or its direct legality parity test changes. Previously those paths
could change without launching this Gen I focused native test suite.

Local standalone Python self-tests executed using genuine Python/git tooling
and passed for a valid registry, duplicate registrations, missing source,
removed suite, sanitizer removal, removed fail-fast, computed shell command,
and removal of a mandatory original RBY evidence case. The exact current
GitHub repository tree was independently inspected: 22 focused + 29 L3
entries, 45 unique source files, zero missing or duplicate registrations.

This source-list preflight does not replace actual normal/ASan+UBSan C++
execution, original historical event authentication, or original PKHeX
differential parity. Those gates remain blocked until the exact new HEAD
has executed successfully. No original saves or accepted/main code changed.
