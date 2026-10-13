# L5 Stage 1 — executable Invalid-verdict QA

Date: 2026-10-12. Existing PR #294 only. Production and shared CI remain coordinator-owned.

## Exact source and audit

Starting L5 HEAD was 125d1e153fb2ba13ec6db24d54cc5eaea63891df. Current integration PR #298 HEAD at inspection was 2e9dbc0136ebdd8e4a00e335b171c48da5fdfdfc, OPEN/DRAFT. Inspected production file src/Legality/Legality.cpp at Git blob 2ee1624de1520630c54d69599834c8e29d589ffb.

The independent source scan found **226/226 explicit severity add(r,...) calls**, **37 direct Invalid** sites, no dynamic-severity emissions, and both native-generation F3 and F4 capability guards present. This is a lexical inventory, NOT historical truth for the 37 sites.

## Stage 1 implementation

**Actual production analyzer fixture extended to 200 synthetic native contexts.**

The file tests/fixtures/legality_l5_pk12_sourcefree_native_probe.cpp retains the original 12 cases and adds 188 after-fix cases: 40 PK1 source-identity/level controls; 140 PK2 source-identity/DV/gender controls for Bulbasaur, Nidoran female and male, Magnemite, Chansey, Tauros, Unown; 6 deliberate out-of-native-generation move-ID negative Invalid controls; and 2 empty-species zero-issue controls. Exact same-generation source IDs, mismatched known-generation IDs, unknown source labels, and missing labels are distinct. Assert that the previously absent PK1/PK2 fields never generate the F3/F4 Invalid, but genuinely impossible native Gen I move 200 or Gen II move 252 still Invalid.

**Execution boundary:** This 200-context real Legality::analyze probe is committed but NOT EXECUTED here due lack of a full source checkout and DNS to GitHub. The opt-in targets in tools/audit_legality_l5_pk12_probe.mk retain the same names, and coordinator CI can re-run them after integration.

**Compiled and run: independent 18+240 state source-applicability model.**

The exact updated source file tests/fixtures/legality_l5_gen12_sourcefree_contract.cpp is Git blob 17e729ff4a9aaed27d82db3455b5c02e6bb4476d; local git hash-object matched. The expanded matrix is 4 native generations x 5 gender-ratio classes x 3 genders x 4 source states = 240 combinations, plus 18 legacy model states. All tests actually PASSED:

- GCC 14.2.0 C++20 O2, Wall/Wextra/Wpedantic/Werror
- GCC 14.2.0 C++20 O1 with ASan+UBSan
- Clang 17.0.0 C++20 O2, Wall/Wextra/Wpedantic/Werror

This **does not execute the production analyzer** and cannot certify original-retail PKHeX parity.

**New executable audit gate.**

tools/audit_legality_l5_invalid_site_gate.py strips comments/string literals, classifies explicit and dynamic add(r,...) emissions and checks the two native capability guards. The current coordinator source was independently scanned with the equivalent expressions through connected GitHub data: 226 explicit, 37 Invalid, zero unclassified, F3/F4 present. The exact committed Python CLI blob 1e0df5ad81460d82292c3041ac2ddbe3bc9f4590 was independently byte-verified using local git hash-object, and its --self-test plus Python bytecode compilation PASSED. Its --source scan on the full production checkout is PENDING; the connected-source equivalent scan above was executed.

Run it with:

    python3 tools/audit_legality_l5_invalid_site_gate.py --self-test
    python3 tools/audit_legality_l5_invalid_site_gate.py --source src/Legality/Legality.cpp --expected-invalid 37 --require-f3-f4

## F1–F4 findings and coordinator gates

- **F1:** native Gen III/IV EV values 253–255 are permitted individually; prior coordinator 52-case actual analyzer suite passed normal/sanitized on ancestor run 38287166355. Fix currently in coordinator source.
- **F2:** native unhatched PK3/PK4 with nonzero EV is Invalid; same prior suite passed at the ancestor exact SHA.
- **F3:** source-free PK1/PK2 absent ability previously generated false Invalid. Fixed capability guard present in current coordinator blob; real analyzer verification of new 200-context probe still required.
- **F4:** PK1 has no native gender, so source-free neutrality must not become genderless-species Invalid. Fixed guard present; full analyzer verification still required.

Latest coordinator Actions run 38296998403 at 2e9dbc0 was **IN PROGRESS** at review. It is not marked green here. The accepted legality branch and MAIN were not modified.

## Remaining Invalid-site risks

P0: execute 200-context native probe in normal and ASan/UBSan modes on exact coordinator head after forward-only integration, then verify native Gen III/IV ability negative controls remain.

P1: exact Gen IV form 987; Korean static and hatch event 1003/1252; hatch location 1260; Pal Park transfer 1107/1159/1170/1175. No new false-Invalid claim until independent retail evidence and exhaustive alternative original routes exist.

P1: Gen IV PID-selected ability site 1682 and native species/move name-resource sentinels 1551/1702. Native ID domain and original event/static exceptions require proof before hard global Invalid.

P2: original-retail paired source histories, lawful archival data and full 336-cell PKHeX game/family verification still absent; synthetic test counts cannot establish historical authenticity.

Coordinator replay on a full reconciled checkout:

    make -f Makefile.host -f tools/audit_legality_l5_pk12_probe.mk l5-pk12-fixed
    make -f Makefile.host -f tools/audit_legality_l5_pk12_probe.mk l5-pk12-fixed-sanitize

Do NOT use the pre-fix baseline mode on corrected production; that mode intentionally requires the original false-Invalid symptoms. No production Legality.cpp, shared workflow, original source save, PR #298, accepted legality, main or unrelated lane was written. No merges, rewinds, force push, unsupported assets or fabricated retail provenance.

Stage 1 checkpoint complete. Next requested "continue": Stage 2 native storage/source-identity adversarial QA.
