# Generation I UX Cleanup #3 Model Checkpoint

This checkpoint adds only reusable model/test infrastructure. It does not move production, enable live writes, or claim hardware acceptance.

## Shiny model

Red / Blue / Yellow do not store a native shiny flag. The UI derives shiny state from the exact Generation II-compatible DV pattern:

- Defense = 10
- Speed = 10
- Special = 10
- Attack in 2, 3, 6, 7, 10, 11, 14, 15
- HP DV remains derived from the four stored DVs.

`Gen1Shiny.h` therefore contains only pure DV transformations/detection. It never adds a serialized shiny byte/bit/extension.

The UI will keep a transient pre-shiny DV snapshot so Normal -> Shiny -> Normal can restore the previous values where possible. `makeNormalFallback()` exists only for an already-shiny source opened without such a snapshot.

## Move compatibility model

See `GEN1_MOVE_COMPATIBILITY_REFERENCE.md` for the pinned PKHeX reference and the deliberately conservative boundary between normal-editor learnability and full encounter legality.

Existing weird/hacked source Pokemon are preservation cases; this model is for normal new/replacement move choices and must not silently normalize source history.
