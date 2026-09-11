# PokeBank NX — Next Session Plan

Last updated: 2026-09-11
Status: **GEN I RBY + GEN III LEGACY READ-ONLY PHYSICALLY ACCEPTED / NEXT: GSC READ-ONLY**

## Preserved accepted baseline

```text
Red GB: DEVICE ACCEPTED YES
Blue GB: DEVICE ACCEPTED YES
Yellow GB: DEVICE ACCEPTED YES
FireRed GBA: DEVICE ACCEPTED YES
LeafGreen GBA: DEVICE ACCEPTED YES
Ruby GBA: DEVICE ACCEPTED YES
Sapphire GBA: DEVICE ACCEPTED YES
Emerald GBA: DEVICE ACCEPTED YES
Live installed-game writing: HARD DISABLED
Live RetroArch writing: HARD DISABLED
```

Accepted RBY runtime/artifact:

```text
Application source: 50dac31f53907143f48884681056f8d582813b76
Application tree: 1cf73ea12833e8a94a06dfaf2b9036e9059344ec
PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro
bytes: 159754197
SHA-256: b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b
RBY DEVICE TESTED: YES
RBY DEVICE ACCEPTED: YES
```

Preserve the first `d9077e2d` device-test record as historical evidence: it exposed the RBY Items/category defect and Yellow GB/GBA label defect before the accepted `50dac31f...` retest.

## Next milestone — Generation II Gold / Silver / Crystal

Target identities:

```text
gold_gbc
silver_gbc
crystal_gbc
```

Platform/source:

```text
Game Boy Color / RetroArch
normal battery saves only (.sav / .srm)
READ ONLY
```

## Start the next coding session with recovery

1. Recover the exact accepted branch state using the repository recovery contract.
2. Read `CURRENT_STATUS.md`, `docs/CODEX_SESSION.md`, `docs/NEXT_CODEX_PROMPT.md` and the GB/GBC resource chunk only.
3. Verify accepted Gen I and Gen III baselines remain present before touching GSC.
4. Keep live installed-game and RetroArch writes hard disabled.

## GSC audit / implementation scope for the next coding session

- inspect pinned PKSM-Core `Sav2` / `PK2` first;
- independently oracle relevant behavior with PKHeX;
- determine international/Japanese save layouts;
- prove supported save sizes/checksums;
- model Gold/Silver vs Crystal differences explicitly;
- Trainer;
- Party;
- PC Boxes;
- Items/inventory;
- truthful Generation II Pokémon summary fields;
- bounded RetroArch normal battery-save discovery;
- `.sav` / `.srm` only, not savestates;
- source immutability;
- Refresh/current-save behavior;
- malformed/truncated rejection;
- Gen I + Gen III regressions;
- focused host tests + full host suite;
- ASan/UBSan;
- native build;
- eventual exact physical-test NRO before device acceptance.

Do not guess offsets/layouts or weaken strict identity/checksum validation merely to accept a fixture.

## Out of scope until separately authorized

Do not start DS, 3DS, Vault/Banks, RetroArch per-profile isolation, Admin Mode, RetroArch Profile Bridge, transfers, conversion, editor, legality expansion, events or live writing merely because RBY is now accepted.

Canonical handoff: `docs/NEXT_CODEX_PROMPT.md`.
