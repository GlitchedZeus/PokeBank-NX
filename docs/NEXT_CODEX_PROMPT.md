# NEXT CODEX PROMPT — GENERATION II GSC READ-ONLY MILESTONE

Continue PokeBank NX on `feature/pokebank-playable`.

Use MEDIUM reasoning unless a concrete parser/build defect requires deeper analysis.

## RECOVER FIRST

This is the next coding milestone after physically accepted Generation I RBY and Generation III legacy read-only support.

Before implementing anything:

1. follow `docs/RECOVERY_CONTRACT.md` and recover `origin/feature/pokebank-playable`;
2. read `CURRENT_STATUS.md` and `docs/CODEX_SESSION.md`;
3. verify the accepted Gen I and Gen III checkpoints below are present;
4. open only the GB/GBC game-family resource material and relevant research/reference notes;
5. keep all custom work on `origin/feature/pokebank-playable`, never PKSE upstream.

Live installed-game writing and live RetroArch writing remain **HARD DISABLED**.

## ACCEPTED BASELINE — DO NOT REOPEN WITHOUT DEVICE EVIDENCE

Generation I RBY:

```text
Red GB: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Blue GB: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Yellow GB: DEVICE TESTED = YES / DEVICE ACCEPTED = YES
Application source: 50dac31f53907143f48884681056f8d582813b76
Application tree: 1cf73ea12833e8a94a06dfaf2b9036e9059344ec
Accepted NRO: PokeBank-NX-RBY-ItemsFix-Retest-50dac31f.nro
bytes: 159754197
SHA-256: b2a8c68a80b27ca647777e7286da976b25d66ff7460555f9a404a1a783a1c16b
GEN I RBY LEGACY READ-ONLY: PHYSICALLY ACCEPTED
```

Historical RBY record to preserve: runtime `d9077e2d` was the first device test and exposed the Items/category + Yellow GB/GBA defects; `50dac31f...` fixed them and passed physical retest.

Generation III:

```text
FireRed GBA: DEVICE ACCEPTED YES
LeafGreen GBA: DEVICE ACCEPTED YES
Ruby GBA: DEVICE ACCEPTED YES
Sapphire GBA: DEVICE ACCEPTED YES
Emerald GBA: DEVICE ACCEPTED YES
GEN III LEGACY READ-ONLY: PHYSICALLY ACCEPTED
```

Do not change accepted Gen I/III behavior unless a new physical defect directly implicates it.

## ACTIVE MILESTONE — GENERATION II

Implement focused read-only support for:

```text
gold_gbc
silver_gbc
crystal_gbc
Game Boy Color / RetroArch
READ ONLY
```

This milestone is normal RetroArch battery saves only (`.sav` / `.srm`). Savestates are not canonical sources.

## RESEARCH / ORACLE ORDER

Before writing parser constants:

1. inspect the project's pinned PKSM-Core `Sav2` / `PK2` implementation;
2. independently check PKHeX for save-layout, checksum, language/region and Pokémon-structure behavior;
3. use existing project research/indexed references where relevant;
4. do not copy unverified historical constants from PHBankGBC or forums without an independent oracle/fixture.

## REQUIRED GSC SCOPE

Prove and implement, as applicable:

- supported international/Japanese save layouts;
- exact supported save sizes;
- strict checksums / structural validation;
- explicit Gold/Silver vs Crystal differences;
- stable `gold_gbc`, `silver_gbc`, `crystal_gbc` identities;
- bounded RetroArch discovery;
- Trainer;
- Party;
- PC Boxes/storage;
- Items/inventory with Generation II semantics;
- truthful PK2 / Generation II Pokémon detail mapping;
- Refresh/current-save behavior;
- source-byte immutability;
- malformed/truncated rejection;
- nonfatal handling for optional sections only when proven safe;
- Gen I and Gen III regression preservation.

Do not fabricate modern fields that do not exist in Gen II.

## VERIFICATION GATES

Before any physical-test artifact:

```text
focused GSC parser/oracle tests
GSC discovery tests
GSC source-browser/bridge tests
malformed/truncated rejection
source mutation/write-policy tests
full host suite
Gen I regression
FRLG regression
RSE regression
ASan
UBSan
git diff --check
native device asset preflight
devkitA64 compile
native FINAL LINK
embedded application identity
embedded RomFS verification
```

Device acceptance requires a new exact `.nro`, exact application-source SHA/tree, bytes, SHA-256 and a physical Switch test. Do not mark GSC `DEVICE TESTED` or `DEVICE ACCEPTED` before that happens.

## SAFETY

```text
Live installed-game writes: HARD DISABLED
Live RetroArch writes: HARD DISABLED
Read-only source bytes: MUST REMAIN IMMUTABLE
Unknown/malformed variants: FAIL SAFELY
```

No physical acceptance of prior read-only paths authorizes write support.

## DO NOT START IN THE GSC MILESTONE

Do not expand into DS, 3DS, Vault/Banks, RetroArch per-profile isolation, Admin Mode, RetroArch Profile Bridge, transfers, conversion, editor, legality expansion, events or live writing unless separately authorized after the focused GSC milestone.

## STOP CONDITION

Stop after a coherent GSC read-only implementation is host/sanitizer/native verified and an exact physical-test artifact is ready, unless the user explicitly changes scope earlier. Do not roll automatically into DS/3DS or product/Vault work.
