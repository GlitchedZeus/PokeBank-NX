# L5 — validation freeze and coordinator integration handoff (12 October 2026)

**PR:** [#294](https://github.com/GlitchedZeus/PokeBank-NX/pull/294), draft branch `audit/legality-l5-research-qa-20261012`. **Validated original PR head:** `ccd27dd7845e13872174fd609d9a3cdc91d7f393`. **Accepted legality engine head observed:** `d29eada5c087e201e00c749703f1a2de3ddfadeb`. Ahead 54 / behind 2 before this documentation-only freeze commit. No rebases, resets, force-pushes or self-merge.

## Executed tests — exact Git blob provenance

Every row below was retrieved from GitHub, locally reconstructed byte-for-byte, and checked using `git hash-object` against its exact **committed** Git blob SHA **before compiling**. Executed in an isolated Linux container; **not full-repo nor GitHub exact-head Actions**.

| Standalone suite | Git blob | g++ -O2 | g++ ASan+UBSan | clang++ -O2 |
|---|---|---|---|---|
| Q12 EV rule differential, 131,072 comparisons / 3,054 bounded hard-cap differences | `50487f7b` | PASS | PASS | PASS |
| Q04/Q05/Q07 source-condition triad (14 static assertions) | `33dc0efe` | PASS | PASS | PASS |
| Q16 witness-bound synthetic fuzz (24 static checks +65,536 trials) | `d8b0c6a3` | PASS | PASS | PASS |

GCC flags `-std=c++20 -Wall -Wextra -Wpedantic -Werror -O2`; sanitizers `-O1 -fsanitize=address,undefined -fno-omit-frame-pointer`, run with `ASAN_OPTIONS=detect_leaks=0`. Both compilers passed all three suites. The Q16 deterministic output is `24576/28672/12288 compatible/contradicted-one/unknown`.

Exact-source `tools/audit_legality_l5_ci_gate.py`, Git blob `97032366`, **26 embedded policy self-tests PASS**. Replaying the live queued/two-behind evidence returned **BLOCKED** with exit code **1**, as designed. Source-evidence JSON: [Q15_EXACT_SOURCE_LOCAL_VALIDATION_20261012.json](Q15_EXACT_SOURCE_LOCAL_VALIDATION_20261012.json).

Earlier tranche-6 independent JS byte decoder also executed **336 valid synthetic ciphertext records +12 deliberately corrupt +251,904 single-bit data tamper checks** with zero unexpected checksum passes. That is **not** compiled PokeBank native C++ or a full Pokémon parser.

## Not run — explicit blockers (must NOT mark green)

| New target | Reason |
|---|---|
| `tests/fixtures/legality_l5_native_encounters.cpp` | Needs full accepted generated encounter data/header includes |
| `tools/audit_legality_l5_native_codec.py --source-pin` (and `--sanitize`) | Needs full checkout/complete exact fixture corpus and original compiled production crypto; not executed |
| `make -f Makefile.host -f tools/audit_legality_l5_ev_probe.mk l5-ev-baseline` and sanitizer | Needs full accepted production `Legality::analyze` host link graph; not executed |
| `... l5-ev-fixed` and sanitizer | Requires coordinator-approved F1/F2 production fixes and full native PK3+encrypted PK4 analyzer run |
| `tools/audit_legality_l5_inventory.py`, `major2.py`, `major3.py`, `release_gate.py`, `ev_differential.py`, `native_ev_vectors.py` | Full data/source-checkout/pinned reference dependencies not mounted locally; no new exact-head execution claim |
| GitHub exact-HEAD clean host/focused RSE/full ASan+UBSan | Queue persists. Current shared SHA-identity step skipped on `audit/legality-l5-` prefix. |

**The 348 encrypted PK3/PK4 examples are synthetic format specimens only**; no original distribution archive or paired authenticated retail Pokémon is certified.

## Coordinator-owned integration checklist — files L5 MUST NOT modify

1. **Exact-head SHA check:** `.github/workflows/host-tests.yml` **lines 26 and 29** must include `audit/legality-l5-` alongside existing legality branch families, both for `actions/checkout` `ref` and for the `Verify exact pull-request application identity` `if` condition. Merely adding the conditional step but checking out the synthetic merge ref is insufficient.
2. **Permanent test registration:** existing `Makefile.host` includes independent fragments, but none of the L5 `tests/fixtures/legality_l5_*.cpp` are in `HOST_TESTS` / `HOST_SANITIZE_TESTS`. Coordinator can register a reviewed shared host fragment and call the L5 Python runners from Actions; L5 changes **no** shared Makefiles/workflows.
3. **Required native tests:** opt-in `tools/audit_legality_l5_ev_probe.mk` uses `GEN34_BALL_ANALYZER_SOURCES` from `Makefile.host.legality-ball-domain` and adds 36 real `Legality::analyze` synthetic PK3/PK4 EV/egg controls, baseline then fixed. `tools/audit_legality_l5_native_codec.py --source-pin` tests 348 true production crypto records and 251,904 data-bit flips in both compilation modes. Run the standalone encounter test and the 3 source-contract C++ suites as well.
4. **Production fix F1:** accepted `src/Legality/Legality.cpp:1629` unconditionally hard-rejects native Gen III/IV EV >252; pinned PKHeX Gen III/IV allows up to **255 per stat, total <=510**. Coordinator alone must patch by native format and test 252/253/254/255 and total 510/511 without altering original byte saves.
5. **Production fix F2:** pinned PKHeX invalidates **nonzero EVs on unhatched eggs**, while NX inspected EV/egg direct path lacks equivalent. Coordinator alone to implement exact native unhatched check; test egg EV 0 vs 1/100 across Gen III and IV, with unknown original breeding parents staying Incomplete.
6. **Forward-only ancestry:** accepted is 2 commits newer due Platinum egg work. Coordinator reconcile without discarding later commits, merging unrelated production lanes, force-pushing or rebasing backwards. Compare exact tree/changed paths after reconciliation; all current L5 files are in `docs/legality/research/l5/**`, `tests/fixtures/legality_l5_*`, or `tools/audit_legality_l5_*`.
7. **Do not promote unfinished legality:** Q06 exact retail encounter condition multisets; Q04 executable original DP/Pt binary/parent/MT histories; Q05 true move-method chronology; Q07 all 247 original event archive item SHA-256s, release/region/raw-text/ribbon proof and reuse rights; Q09/Q10 consented original before/after native saves; Q17 336 method×game actual behavioral differentials remain incomplete. `Unknown` remains `Incomplete`, not global `Invalid`.

## Merge disposition and freeze

**L5 validation package is coherent and frozen for coordinator review, not merge-green.** No more L5 feature/backlog expansion until the coordinator integrates this batch. The coordinator should perform the exact-head build and native probe checks, resolve F1/F2, sign off source/import rights, and then determine whether the research-only PR is acceptable. L5 itself will not self-merge or change MAIN/#103.
