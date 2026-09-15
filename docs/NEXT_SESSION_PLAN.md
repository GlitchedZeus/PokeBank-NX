# PokeBank NX — Next Session Plan

Updated: 2026-09-15

## Starting point

Production Gen I/II milestone is merged and device accepted.

Production merge commit:
`48753c2b0093d30213aa32f016100df0906c3cfb`

Issue #71 / P0 branch:
`audit/pre-gen3-shared-editor-71`

Draft PR:
`#72 — P0: add pre-Gen-III exact-format editor provider foundation`

P0 implementation is present on the audit branch and is intentionally not merged. Exact-head CI validation is the acceptance gate.

## Next task — review P0 and decide whether Gen III may be authorized

Review:

- exact final branch SHA/tree;
- `include/UI/ExactFormatEditorProvider.h`;
- `include/Integration/Classic/ClassicExactFormatEditorProviders.h`;
- `tests/test_exact_format_editor_provider.cpp`;
- `tests/test_pre_gen3_reuse_guard.cpp`;
- draft PR #72 exact-head workflow results;
- Issue #71 final P0 evidence comment.

Confirm the P0 invariants:

1. field state is Hidden / Derived / ReadOnly / Editable;
2. Gen I/II storage remains PackedNative;
3. Gen I five-stat / one-Special semantics remain unchanged;
4. Gen II DV/Stat Exp / derived HP DV / split battle presentation remains unchanged;
5. exact-game move compatibility delegates to existing Gen I/II implementations;
6. trainer descriptors are truthful without adding Gen III trainer mutation;
7. SaveEdit capabilities remain separate from UI-field state;
8. provider descriptors cannot grant original-source writes;
9. Move/Multi is vocabulary only and remains disabled in P0;
10. no Gen3PokemonEditor shell or Gen III Create/Edit product route exists.

## Hard boundary

DO NOT implement Gen III Pokémon editing merely because P0 code exists.

DO NOT merge draft PR #72 until the P0 review explicitly authorizes the merge.

DO NOT add a Gen III Create/Edit route, PK3 mutation/serialization, Gen III trainer mutation, party mutation or source writes during the review.

If exact-head CI is fully green and the review confirms no product behavior change, P0 may be accepted without a new hardware retest. A later, separately explicit prompt must authorize the first Generation III Pokémon-editing milestone.
