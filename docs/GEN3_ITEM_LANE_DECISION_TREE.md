# Issue #71 — Gen III item-lane decision tree

Goal: prevent a second item catalog or a confused raw/normalized item path.

```text
Need to display/select an item for RSE/FRLG?
        |
        +-- Is this a Bag/PC inventory entry?
        |       |
        |       +-- catalog/pocket/name/addability/quantity rule
        |       |       -> ClassicInventoryCatalog (exact ClassicGame)
        |       |
        |       +-- staged quantity mutation
        |               -> existing Gen3StagedInventoryEditor
        |               -> staged copy only
        |
        +-- Is this a Pokémon Held Item?
                |
                +-- user-facing allowed-choice/name filtering
                |       -> ClassicInventoryCatalog / exact-game item provider
                |
                +-- raw PK3 field decode/encode
                        -> generation-native Pokémon adapter
                        -> preserve raw/native item identity mapping
                        -> DO NOT make inventory UI serialize Pokémon bytes
```

## Rules

1. `ClassicInventoryCatalog` is the shared exact-game behavior source for classic item/pocket UI. It already covers Ruby, Sapphire, Emerald, FireRed and LeafGreen.
2. `Gen3StagedInventoryEditor` remains the staged save-inventory mutation owner; it is not a Pokémon editor.
3. The future Gen III Pokémon adapter owns held-item raw serialization/validation. UI supplies a selected logical/native item through the adapter contract.
4. Story-sensitive key items, unsupported pockets and impossible exact-game items remain filtered/blocked according to the catalog and adapter.
5. Do not copy PKSM/PKHeX item databases into a second runtime catalog merely because the future Pokémon editor needs Held Item.
6. If a raw PK3 item identifier differs from the UI/catalog identity, preserve an explicit mapping in the Gen III adapter; do not hide the distinction in the shared UI.
7. Browsing a picker never mutates source or staged bytes. A committed choice mutates only the PokeBank-owned draft/staged record.

## Research/reuse decision

- **EXISTING PROJECT RESEARCH CHECKED:** yes
- **RELEVANT DOCS:** `CLASSIC_SAVE_EDITOR_ARCHITECTURE`, `PROJECT_RESOURCE_INDEX`, `UPSTREAM_AUDIT`, `PKSM_CORE_INTEGRATION`
- **EXTERNAL SOURCE/PROJECT:** inherited PKSE, PKSM-Core, PKHeX (reference/oracle)
- **CURRENT IN-TREE IMPLEMENTATION:** `ClassicInventoryCatalog`, `Gen3StagedInventoryEditor`, `PKSMGen3Adapter::PokemonRecord`
- **DECISION:** REUSE DIRECTLY / ADAPT
- **LICENSE:** retain PokeBank-owned catalog/runtime surface; external engines remain behind reviewed boundaries.
- **TEST/SAFETY:** exact-game catalog tests + staged inventory regression + source mutation policy.
