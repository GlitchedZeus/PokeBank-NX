# L5 seven-stage program — Stage 1 independent parity/release-gate run

Date: 2026-10-12. Repository: GlitchedZeus/PokeBank-NX, L5 draft PR #294.
This is an executed **scope-limited QA stage**, not a release signature or original
retail PKHeX certificate. Changes stay within L5-owned `tools/` and `docs/`.

## Re-fetched branches and integration status

| Lane | Exact observed HEAD |
|---|---|
| L1 #293 | `075a597146ab2924fb86d3efaa0311eb065fba3c` |
| L2 #295 | `7e3c0a2a558d489b429795fc22a35cd2dd95fafb` |
| L3 #296 | `3a0dec90afc35eb8056d8cb55c17aa89e2ae02e0` |
| L4 #292 | `5f7e5b44bedc555c67171e400fc86b347fc6810a` |
| L5 #294 before this report | `d01e1de90f40e7ce57f984dd9a0b772b0738b2a1` |
| Coordinator #298 | `2e9dbc0136ebdd8e4a00e335b171c48da5fdfdfc` |
| Accepted legality | `d29eada5c087e201e00c749703f1a2de3ddfadeb` |

At L5 `d01e1de`, GitHub compare with accepted: 96 commits ahead,
2 behind, 83 changed paths and **zero outside L5 ownership**.
No coordinator/accepted/MMAIN edits, no rebase, reset, merge or force-push.

The latest **observed** L5 exact-head workflow
[#38307234720](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38307234720)
was **queued** at the time of inspection. A queued run is not PASS.
Coordinator [#38296998403](https://github.com/GlitchedZeus/PokeBank-NX/actions/runs/38296998403)
at exact `2e9dbc01` completed **failure**: host GCC + L1, L3, L4,
L5 jobs succeeded; the L2 normal/sanitizer job failed.
Do not call coordinator integration CI green.

## Independent compiled source tests — actually rerun

Fetched existing L5 fixtures at GitHub L5 HEAD and reconstructed them
byte-for-byte in an isolated Linux container. `git hash-object` verified
the exact checked-in Git blob before/after normalizing a terminal newline.
Each was compiled with `-std=c++20 -Wall -Wextra -Wpedantic -Werror`
using GCC 14.2 and Clang 17 in **both** `-O2` normal and `-O1
-fsanitize=address,undefined -fno-omit-frame-pointer` modes.
ASan leak detection disabled and UBSan halt-on-error enabled.

| Executed fixture | Git blob | GCC normal | Clang normal | GCC ASan/UBSan | Clang ASan/UBSan |
|---|---|---|---|---|---|
| `legality_l5_source_gate_triad.cpp` | `33dc0efe6fdd1dc5ee115c22761a4eb9a81a0db8` | PASS | PASS | PASS | PASS |
| `legality_l5_ev_cap_reference_diff.cpp` | `50487f7baa20fbdb368acb5df98cf9926877a728` | PASS | PASS | PASS | PASS |
| `legality_l5_witness_bound_fuzz.cpp` | `d8b0c6a352602102012cd1a611d97de38e1faf23` | PASS | PASS | PASS | PASS |

**12 successful native binaries.** Triad: 14 constant-evidence assertions.
EV reference differential: 131,072 two-EV synthetic comparisons and
3,054 former-source EV-cap mismatches (historical pre-F1 rule, **not** current
production `Legality.cpp`; the coordinator has already corrected F1/F2).
Witness fuzz: 24 scoped assertions plus 65,536 deterministic candidates,
observed 24,576 compatible / 28,672 contradicted-one /
12,288 Unknown.

These three suites intentionally **do not** compile the full production
analyzer or run the actual PKHeX runtime; do not upgrade 336-cell counts.

## Real GitHub evidence verification

An independent JavaScript integrity check in the connected GitHub session
loaded the **complete current** Q01 CSV and both Q17 JSONs, compared all
336 keyed rows and generation/family/applicability labels, recomputed counts,
checked source-SHA consistency and inspected the archived run identity.
Result: **0 integrity discrepancies**; 16 game groups, 21 families,
26 not-native cells, 310 potentially applicable cells, **0 fully
certified parity**, **0 paired original histories** and 6 recorded
source-free-defect cells. The Git blob of the original Q17 file is
`ba31eece267abc10c5d75774d2d8adf3a36503f1` and matches the
archived integration manifest pin.

**Important stale-head negative control:** archived Q17 integration evidence
is for `c91b53e84ef75669049d159616894ec68cd4b0dd`, **not** current
coordinator `2e9dbc0136ebdd8e4a00e335b171c48da5fdfdfc`.
It is historical source-scope evidence, never a current-head pass.

## New executable Stage 1 gate

`tools/audit_legality_l5_stage1_parity_gate.py` performs keyed 336-cell
reconciliation, prevents duplicate/extra/missing game/family cells, checks
generation and native applicability, verifies pinned Git blob identity,
rejects ungrounded full-parity/original claims, checks historical run
identity, and blocks stale coordinator evidence. The included self-test
defines 17 tampering mutations plus 3 live-head negative controls.

Run with a complete checkout:

```sh
python3 tools/audit_legality_l5_stage1_parity_gate.py --self-test
python3 tools/audit_legality_l5_stage1_parity_gate.py \
  --current-integration-head 2e9dbc0136ebdd8e4a00e335b171c48da5fdfdfc \
  --verify-manifests
# For release decision omit --verify-manifests: expected exit 1 (BLOCKED).
```

**Execution disclosure:** the newly committed Python program has **not**
yet been locally executed against a full source checkout; its entire manifest
predicate set was separately evaluated on the actual complete remote
data in JavaScript as described above. Treat Python self-test as
**PENDING EXACT RUN**, not claimed PASS, until independent execution.

## Required Stage 2 and coordinator work

1. Run the new Python self-test and gate on a full immutable SHA-pinned
   checkout. Run `legality_l5_reference_differential.cpp` and all
   production analyzer probes (GCC/Clang normal+sanitized); these were
   not rerun in this Stage 1 session.
2. Diagnose coordinator's failing L2 CI; no edits by L5 to that lane.
3. Stage 2: run the native `legality_l5_pk12_sourcefree_native_probe.cpp`
   and `legality_l5_pk2_egg_stat_exp_analyzer_probe.cpp` against actual
   `Legality::analyze` with field/source alternatives and format guard.
4. Never classify unavailable original specimen/PKHeX paired-method evidence
   as Invalid or a parity certificate. No original file mutations.
