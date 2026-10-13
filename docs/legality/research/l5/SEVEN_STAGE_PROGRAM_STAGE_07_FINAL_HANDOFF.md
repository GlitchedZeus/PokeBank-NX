# PokeBank NX — L5 Seven-Stage Program — Final Stage 7 Integration and Release-Gate Handoff

**Date:** 2026-10-12  
**Lane:** independent L5 only, draft PR #294  
**Coordinator:** draft PR #298  
**Program:** all seven major stages delivered as research/QA engineering.
**Product release decision:** **BLOCKED — no game-family full PKHeX parity certified**.

This handoff ends the requested seven-stage L5 program. It does not
authorize merges, accepted-branch changes, native-source writes,
third-party archive import, or a release-ready claim.

## Final source refs actually re-fetched before new work

| Branch / lane | Inspected 40-character SHA |
|---|---|
| L1 encounters, draft #293 | `5a00a6d69da2f9a6dd1ab27d133a354c86607307` |
| L2 breeding, draft #295 | `4b4539798e4ee3efade6ebf00c6c656414e88ee4` |
| L3 moves, draft #296 | `e63ff70a9ddb8b5aa7c7fb06a08cb63870da58c1` |
| L4 events, draft #292 | `3a8392243c559600fddc7b741b480dfc54895163` |
| L5 before Stage 7, draft #294 | `93b5a60d91807e90825488437b6ec159fbac9976` |
| Coordinator, draft #298 | `ad9c1c859f94c0321a5d17489c72b0e7dbb9f440` |
| Accepted original legality | `d29eada5c087e201e00c749703f1a2de3ddfadeb` |

The coordinator's inspected `src/Legality/Legality.cpp` Git blob is
`4ac8a0dc0badb8211eb105811eb3fa9f928e36e4`.
Do not transplant historic results to a newer branch without re-fetching,
re-pinning and executing the exact newer source.

## Stage 7: substantial independent code and regression delivered

- `tools/audit_legality_l5_stage7_verdict_model.h` — independent
  336-cell original-history / PKHeX executable-proof lattice and 38-site
  candidate-scope model. A local candidate contradiction, stored-field
  assertion, or model-only result cannot certify whole-Pokémon parity.
  Exact **tested and committed** Git blob:
  `477548009e0e67e6784a6d5d16a113d26ebc9341`.
- `tests/fixtures/legality_l5_stage7_verdict_fuzz.cpp` — 180,000
  deterministic candidate/release-grade mutation iterations, 336
  individual evidence-position mutations, alternative-acquisition
  non-promotion, full-identity and original-proof negative controls.
  Exact tested/committed blob:
  `c2c8ac0c0053e7c106ad9e014cbcf3df515ab0d0`.
- `tools/audit_legality_l5_stage7_final.py` — exact-blob-pinned Q01/
  Q17/Q18 manifest reconciliation, keyed 336-cell output, current
  coordinator source-blob check, direct Invalid-site lexical scan,
  anti-fabrication and no-silent-parity safeguards. A separate
  GitHub-observed Q21 snapshot must agree **by game group and family
  ID**, not by incidental row order.
  Exact final local/committed Git blob is pinned in the independent
  Stage 7 workflow.
- `docs/legality/research/l5/Q21_STAGE7_FROZEN_DASHBOARD_336_38.json`
  — source-observed **336** game/family entries plus exact **38**
  current coordinator Invalid-site line numbers; provenance explicitly
  labels this a source/manifest inspection, **not** executable parity.
  Frozen Git blob: `15f4ba61ca069d87f6cd7a7c65e9da3fa044d9fb`.
- `docs/legality/research/l5/Q22_STAGE7_COORDINATOR_ACCEPTANCE_LEDGER.json`
  — machine-readable 11-gate final coordinator requirements; never
  treats audit-method success as a full release signal.
- `.github/workflows/l5-independent-final-stage7.yml` — independent
  exact-head L5 Actions job, checks out the pinned coordinator source
  read-only, validates the original Q01/Q17/Q18 Git blobs, verifies Q21
  cross-check and 38 current direct sites, emits 336-cell CSV + JSON,
  runs GCC/Clang optimized + ASan/UBSan, requires NDEBUG fail-closed,
  and checks no tracked L5/coordinator source changed.

No `src/Legality/Legality.cpp`, source save, coordinator branch,
accepted branch, MAIN or sibling L1–L4 file was modified.

## Tests actually executed in this Stage 7 session

Local environment: GCC **14.2.0**, Clang **17.0.0**, Python **3.13.5**.

| Actually executed test | Result | Assertions |
|---|---|---:|
| GCC C++20 `-O2 -Wall -Wextra -Wpedantic -Werror` | PASS | 1,980,354 |
| GCC C++20 `-O1 -g -fsanitize=address,undefined` | PASS | 1,980,354 |
| Clang C++20 optimized `-O2`, strict warnings | PASS | 1,980,354 |
| Clang C++20 `-O1 -g -fsanitize=address,undefined` | PASS | 1,980,354 |
| Python `-m py_compile` | PASS | syntax |
| Python `--self-test` | PASS | 14 forged promotions rejected; parser controls |
| C++ `-DNDEBUG` negative build | expected compilation failure | assertions cannot be disabled |
| Python full scan without coordinator checkout | expected blocked failure | never claims successful execution |

**7,921,416 actually executed C++ assertions**, zero sanitizer
diagnostics observed in those completed local runs. Sanitized commands
used `ASAN_OPTIONS=detect_leaks=0:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. All four runs
executed L5's **independent evidence model**, not the linked native
production analyzer or executable PKHeX.

Reproduce under a local checkout:

```bash
python3 -m py_compile tools/audit_legality_l5_stage7_final.py
python3 tools/audit_legality_l5_stage7_final.py --self-test

g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -I. \
  tests/fixtures/legality_l5_stage7_verdict_fuzz.cpp -o /tmp/l5-stage7-normal
/tmp/l5-stage7-normal

g++ -std=c++20 -O1 -g -Wall -Wextra -Wpedantic -Werror \
  -fsanitize=address,undefined -fno-omit-frame-pointer -I. \
  tests/fixtures/legality_l5_stage7_verdict_fuzz.cpp -o /tmp/l5-stage7-sanitized
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 /tmp/l5-stage7-sanitized

# On an actual checkout of the exact coordinator head, after pin verification:
python3 tools/audit_legality_l5_stage7_final.py \
  --coordinator-root /path/to/pinned/coordinator-checkout \
  --current-coordinator-head ad9c1c859f94c0321a5d17489c72b0e7dbb9f440 \
  --verify-manifests --csv-out /tmp/l5-336.csv --json-out /tmp/l5-final.json
```

**Important:** The final Python command has **not** executed locally
against the full repository: GitHub clone failed name resolution. It
has been placed in GitHub Actions with its exact original source hashes.
`--verify-manifests` is an **integrity-only** exit-0 gate even if the
reported release status remains BLOCKED. Without that flag, the tool
intentionally exits nonzero when full release acceptance is blocked.
Neither mode invents authenticated original histories.

## Grounded dashboard / Invalid source findings

The frozen Q17 source observed via live GitHub has:

| Historical source-only state | Cells |
|---|---:|
| Research only or not tested | 318 |
| Source-semantic defect annotations (not six current proven full cases) | 6 |
| Partial native format execution | 12 |
| **Total** | **336** |
| Potentially applicable to native format | 310 |
| Not-native-format applicability | 26 |
| **Full original-retail PKHeX parity certified** | **0** |
| **Authenticated original history** | **0** |

The source-observed Q21 checkpoint was independently checked against
all 336 Q17 IDs and the exact coordinator C++ source with **zero
source-observation mismatches** at inspection time. This only verifies
internal manifest/source consistency.

**Critical current-source mismatch:** Archived
`Q18_INTEGRATED_INVALID_37_SITE_AUDIT.json` analyzes **37** direct
`Severity::Invalid` emissions on integration head
`c91b53e84ef75669049d159616894ec68cd4b0dd`, analyzer blob
`a81cf6b878da50cc85837edf2c8c37ea11357d8a`.
Current coordinator at the inspected head contains **38 direct
`add(r,Severity::Invalid,...)` lexical emission sites**. This
means the old 37-site inventory is STALE and cannot certify the
current 38 source sites. A complete new source-by-source and
native-executable review is required. The additional lexical count
is **not**, by itself, proof of a new false-Invalid runtime defect.

Events/provenance remain blocked: Q07's 247 converted WC4 templates
have only 70 distinct card IDs, 219 records with repeated IDs, **zero
authenticated original Wonder Card receipts**, and **zero independent
original event SHA256 payload attestations**. Q10's 22 paired
original-history requests are unfulfilled. Third-party redistribution
is not automatically authorized by a source-code hash.

## All seven stage results — audit scope, not release certification

1. **Stage 1 — parity foundation**: 336-cell fail-closed manifest
   and separation of synthetic/native/original evidence.
2. **Stage 2 — native Gen I/II hardening**: executable format and
   source-free Invalid investigations, with sanitizer gates.
3. **Stage 3 — PK3/PK4 native encryption/format fuzz**: real original
   codec helper testing across deterministic encrypted specimens.
4. **Stage 4 — source-pinned PKHeX differential**: independently
   modeled source-method behavior vs original NX helper; explicitly
   no live PKHeX or full engine execution.
5. **Stage 5 — cross-lane alternative original histories**: L5-owned
   witness consistency lattice plus new pinned L1–L4 bridge tests.
   The four-original-lane bridge must be run successfully in CI.
6. **Stage 6 — chain of custody**: Q07/Q08/Q09/Q10 provenance and
   permission constraints, synthetic cryptographic chain tamper tests.
7. **Stage 7 — this final state**: source-observed 336/38 snapshot,
   executable evidence promotion fuzz, independent CI audit and
   final machine-readable 11-gate coordinator action ledger.

Earlier stages' numeric logs and reports remain in their separate
commits/Stage documents. Do not add counts from distinct/synthetic
test scopes or describe a prior passing local helper run as a verified
newer coordinator full-analyzer run.

## Final coordinator requirements and stop instruction

The coordinator must re-fetch latest branch heads, run all original
native-source and sanitizer workflows on **one immutable integration
head**, and audit the complete **current 38-source Invalid inventory**.
Then, separately obtain independently authenticated original game
histories, closed alternative acquisition routes, actual executable
game-specific PKHeX oracle comparisons, original event distribution
receipts and rights review. A single candidate rejection, source
lookup, controlled source model or synthetic fixture cannot substitute.

**No L5 auto-merge, no release claim, no hidden acceptance waiver.**
Leave research PR #294 draft/unmerged unless the coordinator explicitly
evaluates and approves its scope. The L5 seven-stage program stops here;
further product engineering belongs to the coordinator/owner's next
separately assigned program.
