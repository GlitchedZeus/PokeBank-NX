# PokeBank NX — Project Status

Updated: 2026-09-15

## Accepted milestones

### Generation I shared editor
Device accepted and protected by permanent regression gates.

### Generation II shared editor
Device accepted at exact application `cfb703575144de28521e00b3e8798ab1fa7547bd` / tree `11c6d899f0d842ac97a3813a48c8d33beba675bb`.

PR #68 merged into `feature/pokebank-playable` as merge commit `48753c2b0093d30213aa32f016100df0906c3cfb`.

Accepted NRO SHA256: `fa74911ef4af8a911a01b8cfe931735e517ac48c0ab4650215306c1195acabea`.

## Current engineering milestone — Issue #71 P0

Branch: `audit/pre-gen3-shared-editor-71`.

Draft PR #72 contains the P0 pre-Gen-III descriptor/provider foundation and remains intentionally unmerged.

Architecture:

```text
PokeBank NX shared UI / lifecycle
        ↓
PokeBank-owned exact-format provider descriptors
        ↓
generation-native read / staged adapters
        ↓
reviewed backend/oracle/reference implementations
```

P0 adds field-state truth, storage semantics, stat presentation descriptors, exact-game move compatibility adapters, trainer descriptors, a save/UI capability bridge, and future staged Move/Multi operation vocabulary. It does not genericize PK1/PK2 serializers and it does not add PK3 mutation.

The Gen I/II proof adapter layer delegates move compatibility to the existing exact-game Gen I/II implementations. Native serialization remains owned by the accepted generation-specific staged editors.

## Existing Gen III foundation — unchanged scope

The repository still contains the existing read-only FRLG/RSE adapter work and staged inventory infrastructure. No Gen III Pokémon Create/Edit route, PK3 mutation, Gen III trainer mutation or party mutation is authorized by P0.

## Safety

All original sources remain immutable. Live RetroArch, installed-game and other-emulator writes remain HARD DISABLED. Provider descriptors cannot grant original-source write permission. Future write capability remains adapter-specific and requires separate backup/stage/validate/readback/device gates.

## Next gate

Review exact-head CI for draft PR #72: focused Gen I/II regressions, full permanent host suite, ASan/UBSan, source safety and real devkitA64/native compile. If P0 is green and confirmed to be no-product-change, review/accept the foundation. Only a subsequent explicit authorization may begin Generation III Pokémon editing.
