# PokeBank NX — Authoritative Next Codex Prompt

Use MEDIUM reasoning. If RSE save-format/sector work becomes ambiguous or difficult, switch to HIGH reasoning.

## Current accepted milestone

FRLG read-only browsing/source assignment is physically accepted on real Switch hardware using application source:

```text
d78b76503f02ae26309855970fc5ce0b35c12bcb
```

FireRed GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES.
LeafGreen GBA: DEVICE TESTED = YES / DEVICE ACCEPTED = YES.

Do not reopen FRLG persistence or the `fsdevGetLastResult()` work without new device evidence of a new defect. Live save writing remains HARD DISABLED and was not part of the acceptance.

Recovery is already complete. The verified private GitHub RomFS snapshot remains:

```text
a2adac94f15504b90a83a295e77ad54154da4206
HD renders 3260/3260
base species 1025/1025
types 18/18
fonts 3/3
total RomFS 3283/3283
```

Do not regenerate sprites or redo recovery.

## Single active mission — RSE read-only

Implement/verify Pokémon Ruby, Pokémon Sapphire and Pokémon Emerald GBA / RetroArch normal battery saves using the existing Gen III architecture.

Before coding, perform one narrow check for parked RSE checkpoint `b5ef83b` and `1a921515` if available or a verified equivalent. Inspect only the RSE-specific diff and reuse useful pieces without resetting the current branch or losing newer FRLG/recovery/UI/asset/safety work. If the parked checkpoint is genuinely unavailable, continue from the current architecture; do not begin broad reflog/worktree archaeology.

Architecture:

```text
raw GBA battery save
        ↓
strict PokeBank validation
        ↓
valid rotating save-slot selection
        ↓
Gen III adapter / PKSM-Core where appropriate
        ↓
PokeBank-owned read model
        ↓
UI
```

Validate 128 KiB raw battery saves, both rotating Gen III save slots, expected sector IDs, sector signatures, section checksums, save index/counter coherence, malformed/truncated input and newest-valid-slot selection. A newer corrupted slot must never beat an older valid coherent slot.

Implement/verify for Ruby, Sapphire and Emerald:

- RetroArch normal in-game battery save discovery (`.srm` and already-supported equivalent raw normalization); no arbitrary `.state` parsing;
- truthful Ruby/Sapphire/Emerald identification with no false FRLG identification;
- trainer name, gender, TID, SID where available, money, badges and other existing read-model fields where supported;
- party count and all party Pokémon fields supported by the current summary model;
- all PC boxes in correct order, names where available, empty-slot behavior, and Pokémon reconstruction across Gen III PC sector boundaries;
- existing Pokémon View/artwork path without crashes for legal normal records;
- Refresh rereads a newly saved RetroArch battery save;
- no source mutation and no live-write path.

Focused fixtures/tests should cover where practical:

```text
valid slot A
valid slot B newer
newer corrupted slot + older valid slot
bad sector checksum
bad sector signature
missing sector
truncated save
party Pokémon
boxed Pokémon
Pokémon crossing PC sector boundary
trainer data
Ruby identification
Sapphire identification
Emerald identification
no source mutation
```

Do not replace the cross-sector-boundary fixture with an aligned-only fixture.

Run focused RSE/Gen III tests, relevant FRLG regressions, ASan/UBSan, source-mutation/write-policy checks, `git diff --check`, and a full native devkitA64 build/link.

As soon as a coherent RSE source milestone passes host verification, commit/push `origin/feature/pokebank-playable` and remote-verify it. Do not leave substantial work only in a temporary workspace.

Then restore the committed RomFS snapshot using existing tooling, verify 3260/3260 HD, 1025/1025 base, 18/18 types, 3/3 fonts and device asset preflight PASS. Build/package the exact new RSE application source as `PokeBank-NX-RSE-Retest-<shortsha>.nro/.zip`, record application SHA and documentation SHA separately, verify the embedded 3283-file RomFS, hash the artifact, and STOP for physical Ruby/Sapphire/Emerald testing.

## Out of scope

Do not start Gen I, Gen II, DS, 3DS, modern Switch expansion, Vault, editor/Create Pokémon, transfers, events/gifts, RetroArch per-Switch-user changes, or live save writing.

## STOP condition

Stop after the RSE physical-test artifact exists and has been verified/packaged. Report Ruby/Sapphire/Emerald host status, focused tests, ASan/UBSan, native full-link status, exact application source SHA, push/remote verification, unchanged recovery snapshot counts, exact NRO filename/size/SHA-256, and:

```text
DEVICE TESTED FOR RSE: NO
DEVICE ACCEPTED FOR RSE: NO
```
