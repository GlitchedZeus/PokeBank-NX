# L4 Stage 6 — Authenticated original-source, Q07/Q08 and rights gates

Date: 2026-10-12. Repository `GlitchedZeus/PokeBank-NX`,
L4 branch `feature/legality-l4-events-meta-20261012` and PR #292.
Stage 6 engineering delivered, **integration/acceptance BLOCKED** until
exact-head full native checkout and coordinator authorization. No other lane
or accepted branch was modified.

## Authoritative source pins and live cross-lane audit

Independently fetched GitHub BEFORE modifications:

- L4 old head: `2765f6e6cf47669054f7627be4c838d65e3a6c38`.
- L5 PR #294 independent head:
  `b807b89b7522bfcbe5b5df1763130190bb63e2ee`.
- L4 frozen original projection C++ blob:
  `6dd1fea4a9ba3669f957fbd4e70d076cb0f85529`.
- L5 Q07 original research artifact:
  `docs/legality/research/l5/Q07_WC4_EVENT_PROVENANCE_JOIN.json`,
  Git blob `8261720c96cede640bff89aa091ea0ac5f1e4029`.
- L5 Q08 source/rights artifact:
  `docs/legality/research/l5/Q08_REDISTRIBUTION_GATE.json`,
  Git blob `7cd3d0a227dc9b008f7c304581a2d7e8d85f06e9`.

A **live GitHub-fetched JS parity audit was actually executed**:
247/247 same indexed/species/TID/SID/card/version/language identities;
247 exact composite keys; same original L4 C++ source Git blob;
70 distinct card IDs; 219 projections sharing card IDs; 42 duplicated
numeric card IDs; zero independently verified original-card receipts and
zero independently verified original-card SHA-256 records. All archived
original payload, release receipt, raw trainer, move/ribbon and license
claims remain Unknown/unverified. Independent Q08 review checked all
14 distinct researched source rows and all three per-source permission
fields against their blocked values; all three global bulk import or
binary redistribution dispositions remain BLOCKED. No drift was found
in this explicitly executed comparison. This is **research catalog parity
only**, not original event authenticity or permission to embed archives.

## Implementation, source and tests

New production L4 C++ evidence boundary:
`include/Legality/Gen14OriginalSourceRightsEvidence.h`
Git blob `d8f83812c68d4d4080aad39bf5902a33e9e3bf38`.

The gate separately pins L4/Q07/Q08 source blob identities, frozen
247 projection count, Q08's 14 fully reviewed source-rows and their
currently blocked permissions, original archive source digest, original
release and specific original recipient/transfer chain. Structural research
can be matched as research-only. Unattested original archives, missing
receipts and uncertain historical transfers remain Unknown. The
default and every purported future grant flag **always blocks** retail
payload embedding, third-party code imports and bulk source-table imports.
Own-save consent is not a copyright grant. The evaluator never
authenticates a gift or declares a Pokémon globally Valid/Invalid.

New exhaustive C++ regression:
`tests/fixtures/legality_l4_stage6_rights_boundary.cpp`
Git blob `c0646f0b62d8800b5febdded785f02fab4131fdc`.

**Actually executed and passed 4/4:**
GCC 14.2 normal, GCC 14.2 ASan+UBSan, Clang 17 normal, Clang 17
ASan+UBSan, with 256 missing independent-source-flag combinations,
65,536 rights/receipt/research-mask configurations, 256 original-family
labels, 256 Q08 source-count values and 3 independent source-digest
drift cases. Actual commands:

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -O2 \
    tests/fixtures/legality_l4_stage6_rights_boundary.cpp -o /tmp/l4stage6
/tmp/l4stage6
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Iinclude -O1 -g \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    tests/fixtures/legality_l4_stage6_rights_boundary.cpp -o /tmp/l4stage6asan
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1 /tmp/l4stage6asan
```

The same normal and sanitized runs were executed using `clang++`,
and the exact locally compiled header and fixture `git hash-object`
match the committed blobs above. No sanitizer diagnostics occurred.
The test is included in the L4-owned exact-head runner in addition to
the 28 registered C++ suite programs; L4 workflow paths match it.

**Python policy hardening committed but NOT locally executed:**
- `tools/legality_l4_l5_q07_join.py` now checks frozen Q07 schema,
  catalog identity, the no-binary-copy/unknown-default grade policy,
  exact original row-key schema, and numeric source metadata types.
  No inserted binary card payload field can silently become a research
  original.
- `tools/legality_l4_stage5_source_rights.py` now requires all 14
  *named* sources exactly, prohibits unlisted payload fields/row schema
  changes, and fails when the legal basis or claimed usage changes.
  Previously dropping one source out of fourteen could pass.
- `tests/test_legality_l4_l5_q07_join.py` and
  `tests/test_legality_l4_stage5_source_rights.py` now contain negative
  schema, missing-source, malformed-typed-identifier, binary field
  and rights-policy mutation cases. These tests MUST run against the
  complete checkout before acceptance. Do not claim them passed.

## Outstanding acceptance blockers and Stage 7 handoff

A complete source checkout is not available here:
`git ls-remote https://github.com/GlitchedZeus/PokeBank-NX.git HEAD`
failed `Could not resolve host: github.com`. GitHub Actions jobs
for the four compiler/sanitizer configurations and ordinary host CI
have remained queued at observed prior heads. Re-fetch exact final
HEAD and inspect job conclusions before integration; **no full 28-suite
L4 C++ nor full Python + native encrypted PK3/PK4 regression gate is
verified at this final head**.

Individual original retail PCD/PGT source bytes/digests/recipient
histories and binary import licenses remain independently Unknown.
One source-specific conditional candidate is not a PKHeX legality
verdict or authentic historical receipt. Never use this Stage 6 audit
to redistribute copyrighted material.

Stage 7: complete auditable PK1–PK4 source/rights/provenance gate
integration within the L4 draft branch only, preserve all newer
commits, deliver final coordinator handoff, and leave acceptance
BLOCKED until actual complete-checkout CI and independent review.
Do not merge into coordinator PR #298, accepted legality or MAIN.
