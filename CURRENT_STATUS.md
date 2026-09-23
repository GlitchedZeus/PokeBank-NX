# PokeBank NX — Current Verified Engineering State

Last updated: **2026-09-23**

## Authoritative checkpoint

```text
Repository: GlitchedZeus/PokeBank-NX
Default/docs branch: main
Production branch: feature/pokebank-playable

PR #77: OPEN / DRAFT / NOT MERGED
Branch: feature/gen3-shared-pokemon-editor-20260919

DEVICE-ACCEPTED Gen III application SHA:
996e6aa40c96e4408282f3d55476dae8e64968b2

Tree:
8826147ff5dc1b498b4b8505c9212243ed2f9498

NRO:
PokeBank-NX-Gen3-SharedEditor-996e6aa4.nro

NRO SHA-256:
ac3f6bd03d2a6733aee509729b81b6636cabe836095715c7b575b5dc84c8076c

Actions run:
35825830004

Artifact ID:
10735208869

Artifact digest:
sha256:9e052b90ab6d7ab27cfef55f4bb0016bf784174b35fa203bb9dfcef2627b47e6
```

Status:

```text
CI VERIFIED
DEVICE ACCEPTED
GENERATION III DONE
```

Acceptance belongs to the exact application SHA/NRO above. PR #77 remains unmerged until an explicit merge instruction.

## Device-accepted product baseline

- Gen I Red/Blue/Yellow read + boxed staged editor.
- Gen II Gold/Silver/Crystal read + shared staged editor.
- Gen III Ruby/Sapphire/Emerald/FireRed/LeafGreen read + shared staged editor.
- Classic staged Inventory.
- Shared View/Create/Edit product architecture for Gen I–III.
- D-pad/Left Stick parity and held-repeat navigation.
- Gen I/II packed movement + rectangular multi-select.
- Gen III shiny species preview/commit/cancel.
- Gen III editable EXP via numeric input.
- Gen III clean exact-current-game move picker.
- Gen III dark shared modal/picker styling.
- Gen III readable PID/read-only presentation.
- 20-theme readability system with Poke Classic preserved.

## Permanent safety invariants

```text
ORIGINAL SOURCE SAVE: IMMUTABLE
LIVE INSTALLED-GAME WRITE: HARD DISABLED
LIVE RETROARCH WRITE: HARD DISABLED
LIVE OTHER-EMULATOR WRITE: HARD DISABLED
POKEBANK STAGED EDITING: ALLOWED
UNKNOWN SAVE VARIANTS: FAIL CLOSED
```

No accepted editor milestone authorizes live source writes.

## Current engineering milestone — AUDIT / ORGANIZATION / DURABILITY

Issue **#69** is now the active engineering backlog.

Do not start Gen IV/DS/3DS or trust Master Vault with unique Pokémon until the current storage/transfer/conversion layer has been audited and hardened.

Confirmed priorities:

```text
A01 atomic/durable Bank persistence
A02 held-Pokémon custody on rollback failure
A03 conversion candidate must not replace original custody before commit
A04 cross-store transaction durability
A05 preserve all unreadable/corrupt recovery evidence
A06 malformed/truncated BDSP whole-layout boundary
A07 unsupported larger/newer Bank must become non-writable
A08 profile/account namespacing for mutable backup workspaces
A09 durable replacement for mutable backup save files
```

Then re-verify the older conversion findings against the exact accepted/current source instead of carrying them forward blindly.

## Required next order

```text
1. Freeze/document Gen I–III accepted evidence.
2. Repository/documentation/reference cleanup with no history destruction.
3. Turn A01–A09 into reproducible regression tests.
4. Extract one durable replacement/recovery primitive.
5. Apply it to Bank + mutable backup workspaces.
6. Re-audit conversion correctness with golden fixtures.
7. Harden malformed-input/parser boundaries.
8. Physically test storage/recovery failure cases on Switch SD.
9. Harden/implement Master Vault persistence and legacy migration.
10. Only then expand feature/game coverage.
```

## Recovery maps

- `docs/FULL_PROJECT_AUDIT_2026-09-22.md`
- `docs/REFERENCE_INDEX.md`
- `docs/NEXT_SESSION_PLAN.md`
- `docs/NEXT_CODEX_PROMPT.md`
- `docs/V1_ROADMAP.md`
- issue #69
- issue #29

GitHub is authoritative. Re-fetch active branches before every write and never reset/rebase backward to a recorded checkpoint.
