# Architecture overview

A short map of the code as it is today. For the long-term design (Master Vault service, transfer planner,
source write-back service and so on, most of which does not exist yet) see the
[target architecture](development/TARGET_ARCHITECTURE.md).

PokeBank NX is a single native C++20 Switch application (`PokeBankNX.nro`) built with devkitA64 and libnx.
It renders with NanoVG on OpenGL through SDL2 and packages its artwork, fonts and data in RomFS. The same
sources also compile on a desktop host for the test suite (`Makefile.host*`).

The code started as a fork of [PKSE](https://github.com/kiasta/PKSE) 1.1.3 (see the
[upstream reuse audit](UPSTREAM_AUDIT.md)), which is why the C++ namespace is `PokeVault::` and several UI types
keep `PKSE` in their names.

## Layers

```text
 UI (src/UI, include/UI)
   UIManager: top-level flow between screens
   SaveSelectScreen: Product Home and game/save selection
   AppShellScreen: app shell pages (Settings, More)
   BackupSelectionScreen: choosing a backup or copy
   TrainerViewScreen: Party, Boxes, Bag, Trainer, Review, Export Copy
   Panels / Dialogs / Modals, shared Pokémon editor surfaces per generation
        |
 Generation integrations (src/Integration/Gen1 ... Gen5, include/Integration/...)
   read-only save models, staged editors, export transactions, source discovery
        |
 Shared services
   Integration/Common/VerifiedStagedExport   verified, non-overwriting export
   Safety/SourceMutationPolicy, WritePolicy  what a source may do (default: view only)
   Legacy/*SourceBrowser, *Discovery         RetroArch and other provider save discovery
   Names, Pokemon, Inventory                 generated species/move/item/location tables
   Utils                                     paths, SHA-256, durable files, logging
        |
 Platform: libnx, SDL2, NanoVG, RomFS
```

## Save engines per generation

Each generation has its own native model. Nothing is converted to a modern record to be edited.

| Generation | Read model | Staged editing | Export |
|---|---|---|---|
| I (R/B/Y) | `Gen1ReadOnlySave`, `Gen1ReadOnlyInventory` | `Gen1StagedPokemonEditor`, `Gen1StagedInventoryEditor`, `Gen1StagedTrainerEditor` | `Gen1ExportTransaction` |
| II (G/S/C) | `Gen2ReadOnlySave`, `Gen2ReadOnlyInventory` | `Gen2StagedEditor`, `Gen2UniversalInventory` | `Gen2ExportTransaction` |
| III (R/S/E/FR/LG) | `FRLGReadModel`, `PKSMGen3NativeAdapter` behind `PKSMGen3Adapter` | `Gen3StagedPokemonEditor`, `Gen3StagedInventoryEditor` | `Gen3ExportTransaction` |
| IV (D/P/Pt/HG/SS) | `Gen4ReadOnlySave`, `Gen4SourceDiscovery`, `Gen4AssignedSource` | `Gen4StagedPokemonEditor`, `Gen4StagedTrainerEditor` | `Gen4ExportTransaction` |
| V (B/W/B2/W2) | `Gen5ReadOnlySave`, `Gen5SourceDiscovery`, `Gen5AssignedSource` | `Gen5StagedPokemonWorkspace`, `Gen5SharedEditorBridge` | `Gen5WorkingCopyExport` |

Gen I, III and IV exports go through `Integration/Common/VerifiedStagedExport`; Gen II and Gen V have their own
export transactions held to the same contracts. Single-Pokémon files (`.pk1`–`.pk5`, `.ek3`–`.ek5`) are handled in `src/PokemonFile/`.
Japanese Gen I/II saves are read-only behind `include/Integration/Common/JapaneseEditingGate.h`.

`src/Encryption`, `src/Trainer` and parts of `src/Pokemon` still contain the inherited Gen VI–IX code from PKSE.
On `main` those titles are not part of the product; Gen VI–IX work is paused on `development/gen6-plus`.

## Staging and export pipeline

```text
external save (read-only)
  -> strict parse, exact game identity, source fingerprint
  -> app-owned working copy (all edits staged here)
  -> Review lists the staged changes
  -> Export Copy: write original backup + edited save + manifest into a temporary location
       -> fsync, read back, hash-check every file
       -> strict reparse of the edited bytes as the same game (checksums included)
       -> publish under sdmc:/switch/PokeBank-NX/exports/<gen>/ without replacing anything
  -> any failure removes the partial output
```

The source save is never opened for writing. `include/Safety/WritePolicy.h` sets
`LIVE_SAVE_WRITES_ENABLED = false` as a compile-time constant, and `SourceMutationPolicy::canPerform` allows
mutation only for staged copies and app-owned storage. The contracts (source bytes unchanged, only intended bytes
change, no overwrite, no partial output, Discard All restores the original) are tested for all 18 export paths;
see [qa/SAVE_SAFETY_CHAIN.md](qa/SAVE_SAFETY_CHAIN.md).

App-owned files live under `sdmc:/switch/PokeBank-NX/` (`include/Utils/PokeBankPaths.h`): `config/`, `logs/`,
`backups/`, `exports/`, `transactions/` and `banks/legacy-pkse/` (the inherited PKSE bank format, kept as
legacy storage; it is not the planned Master Vault). See [STANDALONE_RUNTIME.md](STANDALONE_RUNTIME.md).

## UI

The UI is immediate-mode drawing on `PKSEFramebuffer` (NanoVG). `UIManager` (`include/UI/UI.h`) runs the flow
from Product Home to a save and back.
Most save interaction happens in `TrainerViewScreen` (with the large `TrainerViewScreenBase.inc`) plus one shared
Pokémon editor surface per generation (`Gen2…`/`Gen3…`/`Gen4SharedPokemonSurface.inc`,
`Gen5SharedReadOnlySurface.inc`, `SharedPokemonEditorSurface.inc`). Presentation rules that need tests live in
small header-only models under `include/UI/` (for example `ClassicCarryGrid.h`, `ActionSheetModel.h`), which the
host tests exercise without a Switch. Touch input is in `TouchInput.cpp`; launching games is in `GameLauncher.cpp`.

## Vendor code and generated data

- `vendor/PKSM-Core` (git submodule, GPLv3, FlagBrew) is a **host-side reference oracle** for Gen III tests. The
  NRO does not compile it; the app uses PokeBank-owned code behind `PKSMGen3Adapter`. See
  [PKSM_CORE_INTEGRATION.md](PKSM_CORE_INTEGRATION.md).
- `nanovg/` is vendored NanoVG.
- Species, move, item, learnset and encounter tables are generated by scripts in `tools/` and checked in; see
  [development/DATA_REGENERATION.md](development/DATA_REGENERATION.md).
- `recovery/assets_snapshot/` is a split archive of the complete RomFS used by CI to build an NRO with all artwork.

## Tests

- `tests/test_*.cpp`: host C++ tests, wired into the `Makefile.host.*` fragments that `Makefile.host` includes.
- `tests/test_*.py`: repository, workflow and documentation contracts.
- `tests/fuzz_*` and `tests/test_qa_*`: parser fuzzing and export-safety contracts (`make -f Makefile.host qa`).
