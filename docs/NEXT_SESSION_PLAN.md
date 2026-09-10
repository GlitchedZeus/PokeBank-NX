# PokeBank NX — Next Session Plan

Last updated: 2026-09-10
Status: **GEN III GBA READ-ONLY PHYSICALLY ACCEPTED / RBY NEXT**

## Accepted baseline

```text
FireRed: DEVICE ACCEPTED YES
LeafGreen: DEVICE ACCEPTED YES
Ruby: DEVICE ACCEPTED YES
Sapphire: DEVICE ACCEPTED YES
Emerald: DEVICE ACCEPTED YES

Final accepted RSE application source:
a2df4c1acdb7a556808bd58a2bdbcd4fc0335954

Accepted NRO:
PokeBank-NX-RSE-OpenFix-Retest-a2df4c1a.nro
SHA-256: 34fc0893ae0f0ee1a3e244c11469a5d44de181d68318040fce386470b2e0e80e

Recovery snapshot:
2321fa488668e32392de25afed84e38919fbd21f
3286 RomFS files
```

Live installed-game and RetroArch source writing remains **HARD DISABLED**.

## Next action

Implement strict read-only RetroArch support for:

```text
Pokémon Red
Pokémon Blue
Pokémon Yellow
```

Use normal battery saves, strict Gen I size/layout/checksum validation, truthful structural/language handling, Trainer, Party, PC storage, Pokémon view, Refresh and source-byte immutability. Reuse the existing provider/read-model architecture and pinned correctness references.

Run RBY focused tests plus full Gen III regressions, sanitizers and native devkitA64 final link. Push coherent checkpoints early. Produce one exact RBY physical-test NRO and STOP for the user's Switch test.

Do not begin Gold/Silver/Crystal in the same session unless RBY is complete and the user explicitly authorizes continuing.

Canonical detailed instructions: `docs/NEXT_CODEX_PROMPT.md`.
