# PokeBank NX — Current Status

Updated: 2026-09-15

## Production

Repository: `GlitchedZeus/PokeBank-NX`  
Production branch: `feature/pokebank-playable`  
Production head after Gen II merge: `48753c2b0093d30213aa32f016100df0906c3cfb`

## Gen I / Gen II accepted baseline

PR #68 is MERGED.

Physically accepted application:
`cfb703575144de28521e00b3e8798ab1fa7547bd`

Accepted tree:
`11c6d899f0d842ac97a3813a48c8d33beba675bb`

Accepted NRO:
`PokeBank-NX-Gen2-SharedEditor-cfb70357.nro`

NRO SHA256:
`fa74911ef4af8a911a01b8cfe931735e517ac48c0ab4650215306c1195acabea`

Physical result: CI VERIFIED / DEVICE RETESTED / DEVICE ACCEPTED / no hardware bugs found in final retest.

The production merge commit preserves the accepted SHA as a parent and uses the accepted tree.

## Active phase

Issue #71 — pre-Gen-III reuse/shared-editor audit.

Audit branch:
`audit/pre-gen3-shared-editor-71`

Scope: architecture/reuse/freeze only. **Gen III Pokémon editing implementation has NOT started.**

Required audit artifacts:
- `docs/GEN3_INPUT_DOCS_INDEX.md`
- `docs/GEN3_UI_REUSE_MATRIX.md`
- `docs/GEN3_REUSE_FREEZE.md`
- `docs/GEN3_ITEM_LANE_DECISION_TREE.md`
- `docs/GEN3_MINIMAL_SAFE_CHANGE_LIST.md`

Next implementation phase after audit approval: P0 descriptor/provider foundation only; still no Gen III product routing.

## Permanent safety contract

- original source saves immutable;
- live RetroArch writes HARD DISABLED;
- live installed-game writes HARD DISABLED;
- live other-emulator writes HARD DISABLED;
- staged PokeBank editing/export allowed;
- Gen I native boxes PACKED;
- Gen II native boxes PACKED;
- ONE shared Pokémon editor;
- exact-format capabilities; unsupported fields are hidden, not fabricated;
- legality `UNKNOWN` / `Not checked` remains truthful when no engine ran.
