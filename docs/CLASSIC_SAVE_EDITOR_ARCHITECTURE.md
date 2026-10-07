# Classic Save Editor Architecture Inventory

Status: first staged classic-editor slice, 2026-09-12.

This document records what PokeBank NX already inherited/implemented before adding classic-save editing, so classic support extends the existing architecture instead of creating a second unrelated editor stack.

## Existing shared editor infrastructure

`Trainer::Trainer` is already the common mutable save model for supported native Switch formats. It provides common Trainer fields, inventory/pouch collections, party/box collections, per-game limits and virtual serialization hooks such as:

- `updateTrainerInfoBlock()`
- `updateItemBlock()`
- `updatePartyBlock()`
- `updateBoxBlock()`
- `getMaxMoney()`
- `getMaxTrainerNameLength()`
- item-pouch capacity / ID-indexed item helpers

`TrainerViewScreen` already provides controller-first editing surfaces for Trainer Name, Money and items, including the Switch software keyboard / number pad, item dialogs, validation/error status and save-confirmation flows.

The existing source mutation policy remains authoritative: only `BackupOrStaged` and `AppOwnedStorage` are mutable. `InstalledGame` and `RetroArchLegacy` remain blocked. Classic staging does not change that policy.

## Existing supported Switch editor inventory

| Game family | Trainer name | Money | Inventory serializer | Notes |
|---|---:|---:|---:|---|
| Let's Go Pikachu/Eevee | Yes | Yes | Yes | Existing LGPE trainer/item block update path and save integrity handling. |
| Sword/Shield | Yes | Yes | Yes | Existing SCBlock trainer/money/item update path. |
| Brilliant Diamond/Shining Pearl | Yes | Yes | **No** | Trainer update + whole-file MD5 support exist; `updateItemBlock()` is currently a deliberate no-op stub. |
| Legends: Arceus | Yes | Yes | Yes | Existing SCBlock trainer/money/item update path with pouch capacities. |
| Scarlet/Violet | Yes | Yes | Yes | Existing Gen 9 SCBlock trainer/money/item update path. |
| Legends: Z-A | Yes | Yes | Yes | Existing Gen 9 SCBlock trainer/money/item update path. |

These entries describe code capability, not authorization for live installed-game writes. Live installed-game writes remain hard disabled until source/game-specific write safety is separately approved.

The repository also contains a mutable Gen III `Trainer3FRLG` implementation with Trainer, Money, Inventory and 14-sector checksum finalization. Current GBA RetroArch browsing deliberately uses the accepted read-only bridge instead; the mutable FRLG implementation is a reuse target for a future staged Gen III adapter, not permission to mutate a RetroArch source.

## Common classic-save capability model

Classic editors expose explicit capabilities rather than making the UI guess that every generation supports the same fields:

- TrainerIdentity
- Money
- Inventory
- Balls
- Medicine
- BattleItems
- TMsHMs
- Berries
- KeyItems
- Badges
- PlayerPosition
- PartyPokemon
- BoxPokemon
- PCItems

Unsupported capabilities must remain absent from the UI. In particular:

- Generation I/II do not fabricate SID.
- Gold/Silver fixed male player gender is not an editable field.
- Key Items are not part of the first normal-consumable editor path.
- Player position/story fields are not part of the first staged editor slice.

## First classic staged implementation: Generation II

The first complete device target is an international Pokémon Gold battery save because the accepted Gold read path has already passed physical Switch testing.

The Gen II staged editor:

1. receives an already strictly validated `Gen2::ReadOnlySave`;
2. copies the complete original source bytes into an immutable original buffer and a separate staged buffer;
3. never changes the `RetroArchLegacy` mutation policy or the real source `.srm`;
4. validates Generation II trainer-name encoding/length and field ranges;
5. validates item IDs against the target Generation II pocket table;
6. stages Trainer Name, Money, Items and Poké Ball-pocket quantities;
7. records semantic pending changes for review;
8. serializes a separate edited copy;
9. synchronizes the retail Gen II mirror regions;
10. repairs both overall checksum copies;
11. strict-reloads the serialized bytes before export;
12. preserves file size and an appended RTC footer;
13. exports an app-owned `original_backup.srm`, `edited.srm` and provenance manifest.

The first UI intentionally exposes only a tightly controlled subset needed for device validation:

- Trainer Name
- Money
- Potion quantity
- Poké Ball quantity
- Master Ball quantity
- Pending Changes review
- Export Edited Save
- Discard Staged Changes

A general searchable item picker, Key Items, TMs/HMs, badges, position, party editing and box editing remain later slices.

## Region gate

Generation II read support remains unchanged. The first write/export slice only enables staging for validated **international** Gold/Silver/Crystal layouts. Japanese saves remain fully readable but staging is refused until Japanese character encoding and mirror/write round-trips have their own fixtures and tests.

## Next classic targets

1. Reuse the same staged transaction/capability model for RBY Trainer Name + Money, then Gen I Bag/PC Items.
2. Adapt the existing mutable Gen III format work behind the staged boundary for FRLG, then RSE, with sector/checksum/current-slot round-trip tests.
3. Replace the first fixed Gold item controls with the shared searchable inventory picker after the generic pocket model is proven across generations.
4. Add advanced Key Items only behind explicit progression warnings.
5. Research and add Player Position later, per game/generation, with map/coordinate validation.

No step above authorizes live RetroArch or live installed-game writes.


## Standalone export ownership

Generation II staged exports are PokeBank-owned and resolve under `sdmc:/switch/PokeBank-NX/exports/gen2/`. The serializer and source-read-only policy are unchanged by this path migration.
