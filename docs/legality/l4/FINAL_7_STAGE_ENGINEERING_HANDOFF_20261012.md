# PokeBank NX — L4 FINAL 7-STAGE ENGINEERING HANDOFF

**Date:** October 12, 2026
**Repository:** `GlitchedZeus/PokeBank-NX`
**Lane/branch:** `feature/legality-l4-events-meta-20261012`
**PR:** #292 — OPEN / DRAFT / NOT MERGED
**Base:** `feature/legality-engine-gen1-4-20260929`
**Pre-documentation verified engineering head:** `6fd3bd779ebef74e8967ce2af8857b0ba6369a76`
**Do not merge to coordinator PR #298, accepted legality or MAIN without
independent coordinator review and the complete-checkout acceptance gate.**

## Summary of the seven major implementation stages

1. **Stage 1 — Source validation/hardening.** Validated original-archive
   attestation boundaries, converted originals, Gen IV Ranger active storage
   Ball identity, original-recipient/PID ambiguity and malformed historical
   generation/action tags. Unknown source/history never silently becomes a
   positive candidate. Stage 1 complete-checkout acceptance is STILL BLOCKED.
2. **Stage 2 — Gen I/II historical distributions.** Original Stadium/PCNY
   source identity and exact archive record digest gates, NPC offered trade
   level, game-font original OT/nickname bytes, Red/Blue/Yellow and
   Gold/Silver/Crystal independently witnessed version changes, Time Capsule
   crossing snapshots and candidate-only alternatives.
3. **Stage 3 — Gen III event distributions.** Source-attested release,
   original trainer/OT-native text and converted language history, move
   history, independent GameCube-to-GBA source and receipt, and distinct
   Colosseum/XD, Colosseum Bonus Disc and Pokémon Channel media.
4. **Stage 4 — Gen IV PCD/PGT/Ranger.** Original PCD card identity
   independent of a matching 260-byte PGT body, exact card ID/compatibility
   source, independently verified recipient save and individual PK4 receipt,
   DS-clock history and separate native Ranger recipient/PID/IV proof.
5. **Stage 5 — Original-to-current metadata.** Per-hop source/record
   digest continuity, before/after snapshots, documented evolution, trade,
   Time Capsule and Pal Park, 16-hop bound, immutable original metadata,
   original game-font and generation-specific field availability.
6. **Stage 6 — Original source and legal rights.** Exact L4↔L5 Q07/Q08
   catalog identity and blocked redistribution/imports, frozen indexed
   247-card records and 14-source rights catalog, Python malicious-manifest
   hardening, exhaustive read-only C++ rights boundary.
7. **Stage 7 — Final audited provenance gate.** New read-only
   `Gen14FinalProvenanceGateEvidence.h` integrates Stage 5 per-individual
   PK1–PK4 native record histories with Stage 6 researched source and
   redistribution/receipt safeguards. Separate source archive SHA-256,
   original Pokémon record digest and final Pokémon record digest are
   independently bound; neither can stand in for another. Multi-candidate
   results preserve ambiguous Unknowns; even every supplied candidate
   excluded NEVER globally invalidates a Pokémon.

L4 provides **candidate-local forensic evidence**, not an accepted
globally Legal/Invalid PKHeX-equivalent verdict. No final-stage code
changes installed/emulator saves or redistributes retail source payloads.

## Stage 7 code, exact-source tests and QA matrix

Production header:
`include/Legality/Gen14FinalProvenanceGateEvidence.h`
Git blob `3701100d9a9e0fbe10ab864730bc20978fe39955`

New exhaustive regression:
`tests/fixtures/legality_l4_stage7_final_provenance.cpp`
Git blob `61de28f8a20561999e48853f0c9db9ccd1469bf6`

Actual locally used previously committed dependency blobs:
- `Gen14HistoricalEventPathEvidence.h`:
  `3c9f399282d67b5ba872298b288bcf77635f455c`
- `Gen14OriginalCurrentChainEvidence.h`:
  `0c9bb43d63e3f5a31c11c364d4c2bbd737a3f09f`
- `Gen14OriginalSourceRightsEvidence.h`:
  `d8f83812c68d4d4080aad39bf5902a33e9e3bf38`

**12 actual exact-blob local C++ runs PASS, with no sanitizer findings:**
GCC 14.2 and Clang 17, normal `-O2` and
`-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer`,
each executing these three source-verified fixtures:

- `legality_l4_stage5_original_current_chain.cpp`: 32,768 field
  masks, 256 source-identity masks, 256 documented/unknown action tags,
  transfer/encounter history and original/recipient record digest drift.
- `legality_l4_stage6_rights_boundary.cpp`: 65,536 rights and
  receipt permutations, 256 source masks, 256 family labels, 256 frozen
  rights-catalog count labels.
- `legality_l4_stage7_final_provenance.cpp`: 32,768 field mismatch
  masks, 256 source/record proof masks, 256 source family labels,
  original-versus-Pokémon record digest substitutions, source ambiguity,
  competing verified/Unknown/excluded candidates and 65-claim overflow.

Actual command forms, repeated for all three files and both compilers:

```sh
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -O2 \
 tests/fixtures/legality_l4_stage7_final_provenance.cpp -o /tmp/l4-stage7
/tmp/l4-stage7
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -O1 -g \
 -fsanitize=address,undefined -fno-omit-frame-pointer \
 tests/fixtures/legality_l4_stage7_final_provenance.cpp -o /tmp/l4-stage7-asan
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 /tmp/l4-stage7-asan
```

**Actual GitHub tree audit at this stage:** 28 L4 registered C++
source programs, 28 unique declared runner entries, no duplicates or
missing registration, 7 additional L4 C++ fixtures. Both Stage 7 paths
present; new fixture runs in the PR-only L4 runner; exact-head workflow
tracks header/fixture changes and specifies GCC/Clang normal/sanitized.

## Independently verified L5 Q07 / Q08 evidence

L5 PR #294 at independently fetched
`b807b89b7522bfcbe5b5df1763130190bb63e2ee`:
Q07 blob `8261720c96cede640bff89aa091ea0ac5f1e4029`;
Q08 blob `7cd3d0a227dc9b008f7c304581a2d7e8d85f06e9`.

A live GitHub-fetched JS audit compared all **247/247** structural
projections, exact original template source Git blob
`6dd1fea4a9ba3669f957fbd4e70d076cb0f85529`,
70 distinct card IDs, 219 reused-ID rows and 42 numeric ID collisions.
Q07 reports **0** independently authenticated original card receipts,
**0** original archive SHA-256s and **0** permitted retail payload copies.
Q08 checks **14 distinct sources**, three import/redistribution blocks
per source and three global blocked usage categories. Catalog
consistency never authenticates an individual event or licenses original
payloads. If L5 HEAD advances, re-fetch and re-run the entire join.

## Acceptance remains BLOCKED — exact outstanding gates

**Full checkout unavailable:** `git ls-remote
https://github.com/GlitchedZeus/PokeBank-NX.git HEAD` failed with
`Could not resolve host: github.com`. The separately verified
header-only focused 12-run matrix is not equivalent to the 28 native
C++ test programs plus Python audits. GitHub Actions exact-head
four-configuration L4 matrix and standard host CI remain queued.
Do not infer passed gates from queued Actions or synthetic witness data.

**Not executed locally at the exact final branch head:**
- All 28 registered L4 C++ source-compiled programs including
  native encrypted PK3, PK4 and Ranger and both compilers/sanitizers.
- Four-configuration L4 Python frozen provenance/Q07/Q08 regression and
  actual Q07/Q08 CLI joins as run from a complete checkout.
- Full host CI, coordinator collision/conflict resolution, production
  legality-engine integration, hardware verification, original archive
  authenticity, licensing review, or original recipient receipts.

## Coordinator-only next actions / acceptance contract

1. Re-fetch PR #292, L5 PR #294, coordinator PR #298 and accepted/main
   heads. Preserve all new commits. Never force-push, reset or rebase back.
2. Run from the **exact** L4 PR head in a real complete checkout:
   `L4_EXPECTED_HEAD=$(git rev-parse HEAD) CXX=g++ bash
   tests/test_legality_l4_runner.sh`;
   repeat `L4_SANITIZE=1`, then both normal and sanitized with
   `CXX=clang++`. Check every program and every Python audit actually
   runs; inspect raw Actions per-job logs and failed assertions.
3. Supply *independently fetched, revision-pinned* L5 Q07 and Q08 JSON
   files to runner environment `L4_L5_Q07_JSON` and
   `L4_L5_Q08_JSON`. Verify original source blobs, all 247 composite
   indices, full Q08 source rows and every blocking permission.
4. Fix test, schema and actual C++ linkage failures **on L4 PR #292**
   before accepting it; never mark a failing sanitizer or unfinished
   Python audit as passed.
5. Audit source/rights ambiguity and L1–L3/L5 lane interfaces;
   reconcile into coordinator only after all acceptance gates pass.
   Global legality Verdicts must remain conservative where original
   event/transfer history is unavailable. Preserve immutable originals.
6. The coordinator alone may advance the accepted legality branch with
   reviewed code; L4 itself is NOT MERGED and not authorized to merge.
   Review separately for a Nintendo Switch NRO/hardware release.

**Final seven-stage L4 engineering program: delivered as a draft PR.
Acceptance and integration: BLOCKED.**
