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

## Active phase — Issue #71 P0 foundation

Issue #71 remains OPEN.

Branch:
`audit/pre-gen3-shared-editor-71`

Draft validation PR:
`#72 — P0: add pre-Gen-III exact-format editor provider foundation`

P0 now provides generation-neutral, PokeBank-owned descriptor/provider vocabulary above the generation-native adapters:

- field state: Hidden / Derived / ReadOnly / Editable;
- exact-format editor descriptor;
- PackedNative / SparseOwned / AdapterDefined storage semantics;
- generation-truthful stat presentation schema;
- exact-game move compatibility provider interface plus Gen I/II proof adapters;
- trainer field descriptor;
- source/save-operation to UI-field bridge without source-write permission;
- future Move/Multi storage-operation vocabulary only, disabled in P0;
- permanent host architecture guard preventing a parallel Gen3PokemonEditor shell.

Gen I and Gen II serializers, runtime editor routing, visual layout, navigation and packed-box behavior are not rewritten by P0.

**Generation III Pokémon editing implementation has NOT started. Generation III product routing remains unchanged/read-only.**

PR #72 is intentionally DRAFT / NOT MERGED. Exact-head host, sanitizer and native validation must be reviewed before P0 is accepted and before any later Gen III implementation is authorized.

## Permanent safety contract

- original source saves immutable;
- live RetroArch writes HARD DISABLED;
- live installed-game writes HARD DISABLED;
- live other-emulator writes HARD DISABLED;
- staged PokeBank editing/export allowed only through approved generation-native adapters;
- party mutation separately gated;
- Gen I native boxes PACKED;
- Gen II native boxes PACKED;
- ONE shared Pokémon editor;
- exact-format capabilities; unsupported fields are hidden, not fabricated;
- legality `UNKNOWN` / `Not checked` remains truthful when no engine ran.
