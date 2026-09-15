# PokeBank NX — Project Status

Updated: 2026-09-15

## Accepted milestones

### Generation I shared editor
Device accepted and protected by permanent regression gates.

### Generation II shared editor
Device accepted at exact application `cfb703575144de28521e00b3e8798ab1fa7547bd` / tree `11c6d899f0d842ac97a3813a48c8d33beba675bb`.

PR #68 merged into `feature/pokebank-playable` as merge commit `48753c2b0093d30213aa32f016100df0906c3cfb`.

Accepted NRO SHA256: `fa74911ef4af8a911a01b8cfe931735e517ac48c0ab4650215306c1195acabea`.

## Current engineering milestone

Issue #71: **Pre-Gen-III shared editor / reuse audit**.

Branch: `audit/pre-gen3-shared-editor-71`.

The audit freezes what must be reused, what may be generalized above generation-native adapters, and what must remain generation-specific. It does not authorize Gen III editing.

Key architectural conclusion:

```text
PokeBank NX shared UI / lifecycle / capabilities
        ↓
PokeBank-owned exact-format provider descriptors
        ↓
generation-native read/staged adapters
        ↓
PKSE / PKSM-Core / PKHeX / references as licensed backends/oracles
```

Do not merge generation-native PK1/PK2/PK3 mutation structs into one fake universal serializer model.

## Existing Gen III foundation — current scope only

The repository already contains read-only FRLG/RSE adapter work and staged inventory infrastructure. That work remains read-only/product-unchanged during #71. No Gen III Pokémon Create/Edit route is authorized yet.

## Safety

All original sources remain immutable. Live RetroArch, installed-game and other-emulator writes remain HARD DISABLED. Future write capability is adapter-specific and requires its own backup/stage/validate/readback/device gates.

## Next gate

Complete/review #71 audit artifacts, then execute the P0 descriptor/provider foundation on the audit branch or a follow-on branch as explicitly instructed. Re-run accepted Gen I/II permanent host/sanitizer/native regressions. Only after that may a separately authorized Gen III implementation milestone begin.
