# Gen I original TM/HM legality versus existing editor parity

Source-matched 151×55 original Red/Blue/Yellow machine allowances against
the existing editor's 152×3 move-union table with 24 Yellow overrides.
**All 9,117 source-allowed machine moves appear in the existing editor**
(2×3,037 Red/Blue + 3,043 Yellow). The editor separately lists eight
species/move entries without original machine compatibility that can be
available through other methods; they must not be mistaken for a TM receipt.

`tests/test_legality_gen1_editor_original_machines_subset.cpp` exercises
24,915 original indexed species/game/machine pairs and checks the **one-way
subset** contract. Registered in normal and ASan/UBSan-focused Gen I smoke,
and full L3 runner. No product picker changed or original save written.

**Not original PKHeX parity:** historical training, special/event transfer,
per-individual TM use and alternative routes remain Unknown where unauthenticated.
Exact-head CI remains a gate, not an assumed pass.
