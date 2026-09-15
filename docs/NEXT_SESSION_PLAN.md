# PokeBank NX — Next Session Plan

Updated: 2026-09-15

## Starting point

Production Gen I/II milestone is merged and device accepted.

Production merge commit:
`48753c2b0093d30213aa32f016100df0906c3cfb`

Issue #71 audit branch:
`audit/pre-gen3-shared-editor-71`

## Next task — P0 pre-Gen-III foundation only

Read first:
- `docs/GEN3_INPUT_DOCS_INDEX.md`
- `docs/GEN3_UI_REUSE_MATRIX.md`
- `docs/GEN3_REUSE_FREEZE.md`
- `docs/GEN3_ITEM_LANE_DECISION_TREE.md`
- `docs/GEN3_MINIMAL_SAFE_CHANGE_LIST.md`

Then implement only the P0 descriptor/provider foundation:

1. generation-neutral exact-format editor descriptor;
2. explicit native storage semantics descriptor;
3. stat-presentation schema without collapsing DV/IV/Stat Exp/EV semantics;
4. exact-game move-compatibility provider interface;
5. trainer field support/editability descriptor;
6. minimal shared capability-vocabulary extension where proven necessary;
7. explicit bridge between save-operation capabilities and UI-field capabilities;
8. Gen I + Gen II adapters/contracts proving no product behavior changes.

## Hard boundary

DO NOT implement Gen III Pokémon editing yet.

DO NOT add a Gen III Create/Edit route.

DO NOT write PK3 mutation/serialization code.

DO NOT enable trainer or party mutation.

DO NOT alter source-write policy.

DO NOT make Gen I/II sparse.

DO NOT create another generation-specific editor shell.

## Validation

P0 must pass existing accepted Gen I/II focused gates, full host suite, ASan/UBSan and real devkitA64/native compile. Any source SHA that changes accepted runtime behavior requires its own evidence and must not inherit hardware acceptance automatically.

Stop after P0 evidence/report; Gen III product implementation requires a separate explicit authorization.
