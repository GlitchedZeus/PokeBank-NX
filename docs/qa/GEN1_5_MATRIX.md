# Gen I–V acceptance matrix (Thread 4)

Source of truth: `make -f Makefile.host qa-matrix` (part of `make qa` and the **App QA Contracts**
workflow, GCC and Clang, plain and ASan/UBSan). The runner fails on any FAIL, MISSING or DUPLICATE
cell. The tables below are its generated output on `audit/app-qa-20261014` after merging #307
`181423a` (GCC and Clang identical). CI writes the same tables to each run's step summary.

- **Rows:** all 20 Gen I–V versions. **Columns:** 21 acceptance cells (definitions in
  `tests/test_qa_matrix.h`).
- **How cells run:** through the real production entry points (RetroArch/DraStic discovery,
  `LegacySourceBindings`, read-only parsers, staged editors and workspaces, the export
  transactions, `HomeSnapshotCache` / `homeCardSnapshot`, `SharedPokemonWorkspace::navigate`), on
  synthetic saves only. Layout offsets come from existing in-repo builders, cited in each
  `tests/test_qa_matrix_gen*.cpp` header.
- **Status:** PASS; FAIL; KNOWN (open QA-F id, strict repro in `qa-known-failures`); HW
  (hardware-only); N/A (the format has no such data, with the reason cited).

**Result:** 415 PASS, 5 N/A (no player gender in the RBY and Gold/Silver formats), 0 FAIL, 0 KNOWN.

## What a PASS does not cover (hardware-only residue)

No whole cell had to be marked hardware-only: each has a host-testable production model. The
parts below run only on the Switch and are NOT covered by any PASS here. Every NRO stays
**NOT DEVICE TESTED** until the owner tests that exact file.

| Cell | Hardware-only part | Why it cannot run headlessly |
| --- | --- | --- |
| discovery / relaunch | Real SD-card roots (`sdmc:/retroarch/...`, DraStic/melonDS folders), real emulator config files, app relaunch from HOME | Needs the Switch filesystem and applet lifecycle; host runs use temp roots and fresh process state |
| navigation | Gen I/II/III row arithmetic inside `src/UI/*.inc` (Gen I UX3 overlay, `Gen2UnifiedPokemonWorkspace.inc`, `Gen3SharedPokemonSurface.inc` call the shared `Shared::` focus functions inline), D-pad/stick repeat, touch hit-boxes, controller/touch parity | UI translation units link libnx/framebuffer code and are not host-buildable; host cells exercise the shared focus model they call |
| actions | The rendered Actions sheet, touch release-confirm, keypad prompts (`Utils::promptNumber`) | Same: UI code and HID input |
| creation / fields / pickers | On-screen keyboard and number-pad entry, picker scrolling and rendering | swkbd / HID |
| home-refresh (Gen I–III) | The `SaveSelectScreen` size/mtime watch that triggers legacy re-discovery, and the party-strip drawing | `SaveSelectScreen.cpp` is UI-only; host cells drive the discovery and card functions it calls |
| home-refresh (Gen IV/V) | Sprite selection and drawing of the snapshot | Rendering only; the snapshot data path is fully host-tested |
| export | Writing to the real SD card (FAT32 rename/mkdir semantics) and the on-device Review / Export dialogs | Host runs on ext4/tmpfs; device filesystem semantics are not reproducible here |
| immutability | Emulator running concurrently against the same save | Needs a live emulator process; the app never writes the source (checked on host for bytes, size and nanosecond mtime) |

## Cross-generation comparison (classification)

The runner compares normalised facts across versions. Every difference it flags is classified here.

| Fact | Difference | Classification | Owner |
| --- | --- | --- | --- |
| `identity.source` | filename hint (RBY except Crystal, GS, all Gen III); assignment (Diamond/Pearl); save-internal (Crystal, Pt, HG/SS, Gen V) | Inherent to the formats: R/B, G/S, R/S and FR/LG saves are byte-identical families and DP lacks a version marker. **But** Gen III takes the hint from a raw substring of the whole path while Gen I/II accept only isolated tokens: filed as **QA-F6** | L (QA-F6) |
| `trainer.gender` | not stored in RBY and Gold/Silver | Inherent to the format (N/A cells) | — |
| `home.refresh` | legacy re-discovery (Gen I–III) vs `HomeSnapshotCache` (Gen IV/V) | Two mechanisms for one Home card; Gen I–III refresh only for a single-instance title (`SaveSelectScreen.cpp`). Consistency item | 2R (Home) |
| `home.party.form` | dropped for Gen II/III (Unown letter), kept for Gen IV/V | Cross-gen preview consistency, assigned to 2R by the coordinator (QA-F5 fixed the Gen V case) | 2R |
| `home.party.level` | dropped for Gen III (`addParty(i, species, 0, 0, false)`) | Same preview-consistency item | 2R |
| `home.party.shiny` | shown only for Gen V; Gen IV hidden by its own comment; Gen I–III pass `false` | Same preview-consistency item (a product decision is needed: show for all or none) | 2R |
| `create.default.species` | #1 Bulbasaur (Gen I, Gen IV) vs #25 Pikachu (Gen II, III, V) | Create defaults differ by generation in the one shared editor. Defaults are not format rules; consistency item | C (Gen I–IV) / 2R (Gen V) |

Consistent everywhere: `bag.read=yes`, `create.default.level=5`, `export.output=new-file-only`,
`export.reparse=strict`, `nav.mutates=no`.

## Observations from the matrix work

- Gen IV: Unown's letter cannot be edited. Filed as **QA-F7** (C), verified against PKSM-Core
  `PersonalDPPtHGSS::formCount`.
- Gen IV: a net no-op clone/move/release still reviews as "(empty) (edited)". Filed as **QA-F8** (C).
  The actions cell accepts both today's line and the fixed empty review.
- Gen III: unassigned RetroArch saves are visible to every profile (accepted behaviour,
  `test_legacy_frlg_browse.cpp`). The comment in `SaveSelectScreen::loadLegacySources` says a source
  "appears only after an explicit persistent assignment"; the comment is wrong (C).

## Generated matrix

| Version | discovery | linking | provider | identity | trainer-name | trainer-gender | party | boxes | pokedex | bag | navigation | creation | fields | pickers | actions | working-copy | review | export | immutability | home-refresh | relaunch |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| red | PASS | PASS | PASS | PASS | PASS | N/A | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| blue | PASS | PASS | PASS | PASS | PASS | N/A | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| yellow | PASS | PASS | PASS | PASS | PASS | N/A | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| gold | PASS | PASS | PASS | PASS | PASS | N/A | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| silver | PASS | PASS | PASS | PASS | PASS | N/A | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| crystal | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| ruby | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| sapphire | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| emerald | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| firered | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| leafgreen | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| diamond | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| pearl | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| platinum | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| heartgold | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| soulsilver | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| black | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| white | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| black2 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |
| white2 | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS | PASS |

Totals: PASS=415 KNOWN=0 HW=0 N/A=5 FAIL=0

### Cell notes

| Version | Cell | Status | Note |
| --- | --- | --- | --- |
| red | discovery | PASS | discoverConfiguredLegacySaves (RetroArch configured root) -> Ready red_gb card, platform Game Boy |
| red | linking | PASS | claimInstanceAndSave -> visible only to its profile, resolves to the same file, persists |
| red | provider | PASS | discovery-recorded provider 'RetroArch' -> display 'RetroArch' |
| red | identity | PASS | same bytes as 'Pokemon Blue.srm' -> blue_gb; neutral name -> AmbiguousIdentity (R/B not stored in the save) |
| red | trainer-name | PASS | WILL / TID 4660 |
| red | trainer-gender | N/A | Gen I save format stores no player gender (Gen1::TrainerRecord has no gender field; SaveSelectScreen.cpp loadLegacySources fixes gender 0: 'Red is fixed by the game') |
| red | party | PASS | 2 members (#25 L15 SPARKY, #112 L20 RHYDON); read-only by contract |
| red | boxes | PASS | 12 boxes; box 0 #4 CHAR; current box 2 #1 |
| red | pokedex | PASS | seen 2 / caught 1 / total 151 |
| red | bag | PASS | Bag item 20 x3 decoded; staged x25 round-trips through strict reparse |
| red | navigation | PASS | UX3 14-row workspace + addDraftDecision: only StageAdd commits; browsing every slot leaves the staged copy byte-identical (row arithmetic itself is UI code in Gen1PokemonEditorOverlayUXCleanup3.inc) |
| red | creation | PASS | BoxPokemonCreate{} -> #1 L5 at the first empty slot; species 0, level 0 and a gap rejected |
| red | fields | PASS | species, nickname, OT, level, EXP, TID, moves+PP+PP Ups, DVs, Stat Exp stage, read back and survive finalize + strict reparse (no held item/nature/ability in Gen I) |
| red | pickers | PASS | 151 named species; Pikachu move list (27 moves) includes Thundershock, all within 1..165 |
| red | actions | PASS | View/Edit/Clone/Remove/Legality/Cancel (+Review when pending); Clone and Remove execute and reparse |
| red | working-copy | PASS | original bytes untouched; staged copy carries the edit; source file unchanged |
| red | review | PASS | pendingChanges lists 'CHAR' -> 'QA'; revert clears it |
| red | export | PASS | Gen1::publishVerifiedGen1Export (Pokemon + Bag): backup == source, edited.srm strictly reparses with both edits, only box 0/bank checksum/Bag/main checksum bytes differ, second export -> new directory |
| red | immutability | PASS | source bytes, size and nanosecond mtime unchanged after discovery, linking, every edit and export |
| red | home-refresh | PASS | rewritten file -> legacy re-discovery shows new fingerprint/mtime and party L42; the stat()-triggered re-discovery call itself is UI code in SaveSelectScreen.cpp |
| red | relaunch | PASS | new LegacySourceBindings::load + new discovery -> same profile assignment, current file fingerprint |
| blue | discovery | PASS | discoverConfiguredLegacySaves (RetroArch configured root) -> Ready blue_gb card, platform Game Boy |
| blue | linking | PASS | claimInstanceAndSave -> visible only to its profile, resolves to the same file, persists |
| blue | provider | PASS | discovery-recorded provider 'RetroArch' -> display 'RetroArch' |
| blue | identity | PASS | same bytes as 'Pokemon Red.srm' -> red_gb; neutral name -> AmbiguousIdentity (R/B not stored in the save) |
| blue | trainer-name | PASS | WILL / TID 4660 |
| blue | trainer-gender | N/A | Gen I save format stores no player gender (Gen1::TrainerRecord has no gender field; SaveSelectScreen.cpp loadLegacySources fixes gender 0: 'Red is fixed by the game') |
| blue | party | PASS | 2 members (#25 L15 SPARKY, #112 L20 RHYDON); read-only by contract |
| blue | boxes | PASS | 12 boxes; box 0 #4 CHAR; current box 2 #1 |
| blue | pokedex | PASS | seen 2 / caught 1 / total 151 |
| blue | bag | PASS | Bag item 20 x3 decoded; staged x25 round-trips through strict reparse |
| blue | navigation | PASS | UX3 14-row workspace + addDraftDecision: only StageAdd commits; browsing every slot leaves the staged copy byte-identical (row arithmetic itself is UI code in Gen1PokemonEditorOverlayUXCleanup3.inc) |
| blue | creation | PASS | BoxPokemonCreate{} -> #1 L5 at the first empty slot; species 0, level 0 and a gap rejected |
| blue | fields | PASS | species, nickname, OT, level, EXP, TID, moves+PP+PP Ups, DVs, Stat Exp stage, read back and survive finalize + strict reparse (no held item/nature/ability in Gen I) |
| blue | pickers | PASS | 151 named species; Pikachu move list (27 moves) includes Thundershock, all within 1..165 |
| blue | actions | PASS | View/Edit/Clone/Remove/Legality/Cancel (+Review when pending); Clone and Remove execute and reparse |
| blue | working-copy | PASS | original bytes untouched; staged copy carries the edit; source file unchanged |
| blue | review | PASS | pendingChanges lists 'CHAR' -> 'QA'; revert clears it |
| blue | export | PASS | Gen1::publishVerifiedGen1Export (Pokemon + Bag): backup == source, edited.srm strictly reparses with both edits, only box 0/bank checksum/Bag/main checksum bytes differ, second export -> new directory |
| blue | immutability | PASS | source bytes, size and nanosecond mtime unchanged after discovery, linking, every edit and export |
| blue | home-refresh | PASS | rewritten file -> legacy re-discovery shows new fingerprint/mtime and party L42; the stat()-triggered re-discovery call itself is UI code in SaveSelectScreen.cpp |
| blue | relaunch | PASS | new LegacySourceBindings::load + new discovery -> same profile assignment, current file fingerprint |
| yellow | discovery | PASS | discoverConfiguredLegacySaves (RetroArch configured root) -> Ready yellow_gb card, platform Game Boy |
| yellow | linking | PASS | claimInstanceAndSave -> visible only to its profile, resolves to the same file, persists |
| yellow | provider | PASS | discovery-recorded provider 'RetroArch' -> display 'RetroArch' |
| yellow | identity | PASS | contradicting hint 'Pokemon Red.srm' rejected; neutral name -> AmbiguousIdentity |
| yellow | trainer-name | PASS | WILL / TID 4660 |
| yellow | trainer-gender | N/A | Gen I save format stores no player gender (Gen1::TrainerRecord has no gender field; SaveSelectScreen.cpp loadLegacySources fixes gender 0: 'Red is fixed by the game') |
| yellow | party | PASS | 2 members (#25 L15 SPARKY, #112 L20 RHYDON); read-only by contract |
| yellow | boxes | PASS | 12 boxes; box 0 #4 CHAR; current box 2 #1 |
| yellow | pokedex | PASS | seen 2 / caught 1 / total 151 |
| yellow | bag | PASS | Bag item 20 x3 decoded; staged x25 round-trips through strict reparse |
| yellow | navigation | PASS | UX3 14-row workspace + addDraftDecision: only StageAdd commits; browsing every slot leaves the staged copy byte-identical (row arithmetic itself is UI code in Gen1PokemonEditorOverlayUXCleanup3.inc) |
| yellow | creation | PASS | BoxPokemonCreate{} -> #1 L5 at the first empty slot; species 0, level 0 and a gap rejected |
| yellow | fields | PASS | species, nickname, OT, level, EXP, TID, moves+PP+PP Ups, DVs, Stat Exp stage, read back and survive finalize + strict reparse (no held item/nature/ability in Gen I) |
| yellow | pickers | PASS | 151 named species; Pikachu move list (30 moves) includes Thundershock, all within 1..165 |
| yellow | actions | PASS | View/Edit/Clone/Remove/Legality/Cancel (+Review when pending); Clone and Remove execute and reparse |
| yellow | working-copy | PASS | original bytes untouched; staged copy carries the edit; source file unchanged |
| yellow | review | PASS | pendingChanges lists 'CHAR' -> 'QA'; revert clears it |
| yellow | export | PASS | Gen1::publishVerifiedGen1Export (Pokemon + Bag): backup == source, edited.srm strictly reparses with both edits, only box 0/bank checksum/Bag/main checksum bytes differ, second export -> new directory |
| yellow | immutability | PASS | source bytes, size and nanosecond mtime unchanged after discovery, linking, every edit and export |
| yellow | home-refresh | PASS | rewritten file -> legacy re-discovery shows new fingerprint/mtime and party L42; the stat()-triggered re-discovery call itself is UI code in SaveSelectScreen.cpp |
| yellow | relaunch | PASS | new LegacySourceBindings::load + new discovery -> same profile assignment, current file fingerprint |
| gold | discovery | PASS | discoverConfiguredLegacySaves (RetroArch configured root) -> Ready gold_gbc card, platform GBC |
| gold | linking | PASS | claimInstanceAndSave -> visible only to its profile, resolves to the same file, persists |
| gold | provider | PASS | discovery-recorded provider 'RetroArch' -> display 'RetroArch' |
| gold | identity | PASS | contradicting hint 'Pokemon Crystal.srm' rejected; neutral name -> AmbiguousIdentity |
| gold | trainer-name | PASS | ASH / TID 4660 |
| gold | trainer-gender | N/A | Gold/Silver saves store no player gender (Gen2::TrainerRecord::gender is empty; SaveSelectScreen.cpp shows the fixed male player character) |
| gold | party | PASS | 1 member (#25 L15 PARTY); read-only by contract |
| gold | boxes | PASS | 14 boxes; boxes 0/1 hold #25 PIKA / PIKA2 |
| gold | pokedex | PASS | seen 2 / caught 1 / total 251 |
| gold | bag | PASS | Potion x2 / Poke Ball x5 read; staged Potion x7 round-trips through strict reparse |
| gold | navigation | PASS | 400 shared-contract focus moves stay in range (G/S 9 DETAILS rows); browsing leaves the staged copy byte-identical |
| gold | creation | PASS | Gen2PokemonSession Create (UI default record #25 L5) -> stageAddBoxPokemon -> strict reparse; species 0, level 0/101 and move 252 rejected |
| gold | fields | PASS | species, nickname, level, EXP, held item, moves+PP+PP Ups, DVs, Stat Exp, OT, TID, friendship, Pokerus, shiny, trainer name, money stage, read back and survive finalize + strict reparse |
| gold | pickers | PASS | 251 species with personal data; Pikachu move list (57 moves) includes Thundershock, all within 1..251 |
| gold | actions | PASS | View/Edit/Clone/Release/Legality/Cancel (+Review when pending); Clone and Release execute and reparse |
| gold | working-copy | PASS | original bytes untouched; staged copy carries the edit; source file unchanged |
| gold | review | PASS | pendingChanges lists 'PIKA2' -> 'QA'; discard clears it |
| gold | export | PASS | publishVerifiedStagedEditorExport: backup == source, edited.srm strictly reparses with the edit, only box 1 bytes differ, second export -> new directory |
| gold | immutability | PASS | source bytes, size and nanosecond mtime unchanged after discovery, linking, every edit and export |
| gold | home-refresh | PASS | rewritten file -> legacy re-discovery shows new fingerprint/mtime and party L42; the stat()-triggered re-discovery call itself is UI code in SaveSelectScreen.cpp |
| gold | relaunch | PASS | new LegacySourceBindings::load + new discovery -> same profile assignment, current file fingerprint |
| silver | discovery | PASS | discoverConfiguredLegacySaves (RetroArch configured root) -> Ready silver_gbc card, platform GBC |
| silver | linking | PASS | claimInstanceAndSave -> visible only to its profile, resolves to the same file, persists |
| silver | provider | PASS | discovery-recorded provider 'RetroArch' -> display 'RetroArch' |
| silver | identity | PASS | contradicting hint 'Pokemon Crystal.srm' rejected; neutral name -> AmbiguousIdentity |
| silver | trainer-name | PASS | ASH / TID 4660 |
| silver | trainer-gender | N/A | Gold/Silver saves store no player gender (Gen2::TrainerRecord::gender is empty; SaveSelectScreen.cpp shows the fixed male player character) |
| silver | party | PASS | 1 member (#25 L15 PARTY); read-only by contract |
| silver | boxes | PASS | 14 boxes; boxes 0/1 hold #25 PIKA / PIKA2 |
| silver | pokedex | PASS | seen 2 / caught 1 / total 251 |
| silver | bag | PASS | Potion x2 / Poke Ball x5 read; staged Potion x7 round-trips through strict reparse |
| silver | navigation | PASS | 400 shared-contract focus moves stay in range (G/S 9 DETAILS rows); browsing leaves the staged copy byte-identical |
| silver | creation | PASS | Gen2PokemonSession Create (UI default record #25 L5) -> stageAddBoxPokemon -> strict reparse; species 0, level 0/101 and move 252 rejected |
| silver | fields | PASS | species, nickname, level, EXP, held item, moves+PP+PP Ups, DVs, Stat Exp, OT, TID, friendship, Pokerus, shiny, trainer name, money stage, read back and survive finalize + strict reparse |
| silver | pickers | PASS | 251 species with personal data; Pikachu move list (57 moves) includes Thundershock, all within 1..251 |
| silver | actions | PASS | View/Edit/Clone/Release/Legality/Cancel (+Review when pending); Clone and Release execute and reparse |
| silver | working-copy | PASS | original bytes untouched; staged copy carries the edit; source file unchanged |
| silver | review | PASS | pendingChanges lists 'PIKA2' -> 'QA'; discard clears it |
| silver | export | PASS | publishVerifiedStagedEditorExport: backup == source, edited.srm strictly reparses with the edit, only box 1 bytes differ, second export -> new directory |
| silver | immutability | PASS | source bytes, size and nanosecond mtime unchanged after discovery, linking, every edit and export |
| silver | home-refresh | PASS | rewritten file -> legacy re-discovery shows new fingerprint/mtime and party L42; the stat()-triggered re-discovery call itself is UI code in SaveSelectScreen.cpp |
| silver | relaunch | PASS | new LegacySourceBindings::load + new discovery -> same profile assignment, current file fingerprint |
| crystal | discovery | PASS | discoverConfiguredLegacySaves (RetroArch configured root) -> Ready crystal_gbc card, platform GBC |
| crystal | linking | PASS | claimInstanceAndSave -> visible only to its profile, resolves to the same file, persists |
| crystal | provider | PASS | discovery-recorded provider 'RetroArch' -> display 'RetroArch' |
| crystal | identity | PASS | neutral file name -> crystal_gbc from save structure |
| crystal | trainer-name | PASS | ASH / TID 4660 |
| crystal | trainer-gender | PASS | Crystal stores the player gender (0x3E3D) -> female |
| crystal | party | PASS | 1 member (#25 L15 PARTY); read-only by contract |
| crystal | boxes | PASS | 14 boxes; boxes 0/1 hold #25 PIKA / PIKA2 |
| crystal | pokedex | PASS | seen 2 / caught 1 / total 251 |
| crystal | bag | PASS | Potion x2 / Poke Ball x5 read; staged Potion x7 round-trips through strict reparse |
| crystal | navigation | PASS | 400 shared-contract focus moves stay in range (Crystal 13 DETAILS rows); browsing leaves the staged copy byte-identical |
| crystal | creation | PASS | Gen2PokemonSession Create (UI default record #25 L5) -> stageAddBoxPokemon -> strict reparse; species 0, level 0/101 and move 252 rejected |
| crystal | fields | PASS | species, nickname, level, EXP, held item, moves+PP+PP Ups, DVs, Stat Exp, OT, TID, friendship, Pokerus, caught data, shiny, trainer name, money stage, read back and survive finalize + strict reparse |
| crystal | pickers | PASS | 251 species with personal data; Pikachu move list (57 moves) includes Thundershock, all within 1..251 |
| crystal | actions | PASS | View/Edit/Clone/Release/Legality/Cancel (+Review when pending); Clone and Release execute and reparse |
| crystal | working-copy | PASS | original bytes untouched; staged copy carries the edit; source file unchanged |
| crystal | review | PASS | pendingChanges lists 'PIKA2' -> 'QA'; discard clears it |
| crystal | export | PASS | publishVerifiedStagedEditorExport: backup == source, edited.srm strictly reparses with the edit, only box 1 bytes differ, second export -> new directory |
| crystal | immutability | PASS | source bytes, size and nanosecond mtime unchanged after discovery, linking, every edit and export |
| crystal | home-refresh | PASS | rewritten file -> legacy re-discovery shows new fingerprint/mtime and party L42; the stat()-triggered re-discovery call itself is UI code in SaveSelectScreen.cpp |
| crystal | relaunch | PASS | new LegacySourceBindings::load + new discovery -> same profile assignment, current file fingerprint |
| ruby | discovery | PASS | discoverConfiguredLegacySaves (conventional RetroArch root) -> Ready Gen III source |
| ruby | linking | PASS | unassigned: visible to all profiles; claimInstanceAndSave -> reload from disk -> owner-only card -> resolveFRLGSaveInstance = same file |
| ruby | provider | PASS | RetroArch (SaveProviderIdentity), platform Game Boy Advance |
| ruby | identity | PASS | exact id from the RetroArch filename hint, family verified in the save; no hint -> AmbiguousIdentity, wrong family -> InvalidSave (Ruby and Sapphire bytes are identical) |
| ruby | trainer-name | PASS | WILL (strict parse, Save Instance, trainer bridge) |
| ruby | trainer-gender | PASS | stored (block 0 +0x08), female round-trips |
| ruby | party | PASS | 1 member (Bulbasaur), checksum-valid, byte-identical round trip |
| ruby | boxes | PASS | 14x30; Box 1/1 and sector-split Box 2/20 decoded |
| ruby | pokedex | PASS | seen 1 / caught 1 / total 386 |
| ruby | bag | PASS | 6 pouches read; Potion staged to 7 in the Bag workspace |
| ruby | navigation | PASS | SharedPokemonWorkspace::navigate(Gen3) in View/Edit; draft and workspace unchanged (the Gen III surface calls the same Shared:: primitives inline) |
| ruby | creation | PASS | defaults Pikachu Lv5, OT/TID from trainer, origin game 2; occupied slot / long OT / out-of-range values refused |
| ruby | fields | PASS | nickname, level, EXP, item, moves/PP/PP Ups, IVs, EVs, OT, TID, friendship, Pokerus, ball, met level/location, nature, gender, shiny, ability, species: Keep -> finalize -> strict reparse -> reopen |
| ruby | pickers | PASS | species 1-386; exact-game move pools (Gen3LearnsetTable) via the exact-format descriptor; native ability slots |
| ruby | actions | PASS | View/Edit/Clone/Remove/Legality/Review/Cancel (occupied), Add/Review/Cancel (empty); Clone and Remove staged sparse |
| ruby | working-copy | PASS | source untouched; Pokemon and Bag staged copies carry the edits |
| ruby | review | PASS | 7 Pokemon change lines (edit, create, clone, remove) + 1 Bag line |
| ruby | export | PASS | new app-owned directory; edited.sav strict reparse; only active-slot payload/checksums changed; Pokemon+Bag merged |
| ruby | immutability | PASS | bytes, size and ns mtime unchanged after discovery, linking, staging, review and export |
| ruby | home-refresh | PASS | size/mtime change -> discoverConfiguredLegacySaves -> profile card shows the new snapshot (legacy re-discovery; Home party strip passes level 0, form 0, shiny false: SaveSelectScreen.cpp RetroArchFRLG preview) |
| ruby | relaunch | PASS | fresh bindings load + fresh discovery resolve the current file; clean workspace |
| sapphire | discovery | PASS | discoverConfiguredLegacySaves (conventional RetroArch root) -> Ready Gen III source |
| sapphire | linking | PASS | unassigned: visible to all profiles; claimInstanceAndSave -> reload from disk -> owner-only card -> resolveFRLGSaveInstance = same file |
| sapphire | provider | PASS | RetroArch (SaveProviderIdentity), platform Game Boy Advance |
| sapphire | identity | PASS | exact id from the RetroArch filename hint, family verified in the save; no hint -> AmbiguousIdentity, wrong family -> InvalidSave (Ruby and Sapphire bytes are identical) |
| sapphire | trainer-name | PASS | WILL (strict parse, Save Instance, trainer bridge) |
| sapphire | trainer-gender | PASS | stored (block 0 +0x08), female round-trips |
| sapphire | party | PASS | 1 member (Bulbasaur), checksum-valid, byte-identical round trip |
| sapphire | boxes | PASS | 14x30; Box 1/1 and sector-split Box 2/20 decoded |
| sapphire | pokedex | PASS | seen 1 / caught 1 / total 386 |
| sapphire | bag | PASS | 6 pouches read; Potion staged to 7 in the Bag workspace |
| sapphire | navigation | PASS | SharedPokemonWorkspace::navigate(Gen3) in View/Edit; draft and workspace unchanged (the Gen III surface calls the same Shared:: primitives inline) |
| sapphire | creation | PASS | defaults Pikachu Lv5, OT/TID from trainer, origin game 1; occupied slot / long OT / out-of-range values refused |
| sapphire | fields | PASS | nickname, level, EXP, item, moves/PP/PP Ups, IVs, EVs, OT, TID, friendship, Pokerus, ball, met level/location, nature, gender, shiny, ability, species: Keep -> finalize -> strict reparse -> reopen |
| sapphire | pickers | PASS | species 1-386; exact-game move pools (Gen3LearnsetTable) via the exact-format descriptor; native ability slots |
| sapphire | actions | PASS | View/Edit/Clone/Remove/Legality/Review/Cancel (occupied), Add/Review/Cancel (empty); Clone and Remove staged sparse |
| sapphire | working-copy | PASS | source untouched; Pokemon and Bag staged copies carry the edits |
| sapphire | review | PASS | 7 Pokemon change lines (edit, create, clone, remove) + 1 Bag line |
| sapphire | export | PASS | new app-owned directory; edited.sav strict reparse; only active-slot payload/checksums changed; Pokemon+Bag merged |
| sapphire | immutability | PASS | bytes, size and ns mtime unchanged after discovery, linking, staging, review and export |
| sapphire | home-refresh | PASS | size/mtime change -> discoverConfiguredLegacySaves -> profile card shows the new snapshot (legacy re-discovery; Home party strip passes level 0, form 0, shiny false: SaveSelectScreen.cpp RetroArchFRLG preview) |
| sapphire | relaunch | PASS | fresh bindings load + fresh discovery resolve the current file; clean workspace |
| emerald | discovery | PASS | discoverConfiguredLegacySaves (conventional RetroArch root) -> Ready Gen III source |
| emerald | linking | PASS | unassigned: visible to all profiles; claimInstanceAndSave -> reload from disk -> owner-only card -> resolveFRLGSaveInstance = same file |
| emerald | provider | PASS | RetroArch (SaveProviderIdentity), platform Game Boy Advance |
| emerald | identity | PASS | exact id from the RetroArch filename hint, family verified in the save; no hint -> AmbiguousIdentity, wrong family -> InvalidSave (Emerald family is save-internal but the exact id still needs the hint) |
| emerald | trainer-name | PASS | WILL (strict parse, Save Instance, trainer bridge) |
| emerald | trainer-gender | PASS | stored (block 0 +0x08), female round-trips |
| emerald | party | PASS | 1 member (Bulbasaur), checksum-valid, byte-identical round trip |
| emerald | boxes | PASS | 14x30; Box 1/1 and sector-split Box 2/20 decoded |
| emerald | pokedex | PASS | seen 1 / caught 1 / total 386 |
| emerald | bag | PASS | 6 pouches read; Potion staged to 7 in the Bag workspace |
| emerald | navigation | PASS | SharedPokemonWorkspace::navigate(Gen3) in View/Edit; draft and workspace unchanged (the Gen III surface calls the same Shared:: primitives inline) |
| emerald | creation | PASS | defaults Pikachu Lv5, OT/TID from trainer, origin game 3; occupied slot / long OT / out-of-range values refused |
| emerald | fields | PASS | nickname, level, EXP, item, moves/PP/PP Ups, IVs, EVs, OT, TID, friendship, Pokerus, ball, met level/location, nature, gender, shiny, ability, species: Keep -> finalize -> strict reparse -> reopen |
| emerald | pickers | PASS | species 1-386; exact-game move pools (Gen3LearnsetTable) via the exact-format descriptor; native ability slots |
| emerald | actions | PASS | View/Edit/Clone/Remove/Legality/Review/Cancel (occupied), Add/Review/Cancel (empty); Clone and Remove staged sparse |
| emerald | working-copy | PASS | source untouched; Pokemon and Bag staged copies carry the edits |
| emerald | review | PASS | 7 Pokemon change lines (edit, create, clone, remove) + 1 Bag line |
| emerald | export | PASS | new app-owned directory; edited.sav strict reparse; only active-slot payload/checksums changed; Pokemon+Bag merged |
| emerald | immutability | PASS | bytes, size and ns mtime unchanged after discovery, linking, staging, review and export |
| emerald | home-refresh | PASS | size/mtime change -> discoverConfiguredLegacySaves -> profile card shows the new snapshot (legacy re-discovery; Home party strip passes level 0, form 0, shiny false: SaveSelectScreen.cpp RetroArchFRLG preview) |
| emerald | relaunch | PASS | fresh bindings load + fresh discovery resolve the current file; clean workspace |
| firered | discovery | PASS | discoverConfiguredLegacySaves (conventional RetroArch root) -> Ready Gen III source |
| firered | linking | PASS | unassigned: visible to all profiles; claimInstanceAndSave -> reload from disk -> owner-only card -> resolveFRLGSaveInstance = same file |
| firered | provider | PASS | RetroArch (SaveProviderIdentity), platform Game Boy Advance |
| firered | identity | PASS | exact id from the RetroArch filename hint, family verified in the save; no hint -> AmbiguousIdentity, wrong family -> InvalidSave (FR and LG share one family marker) |
| firered | trainer-name | PASS | WILL (strict parse, Save Instance, trainer bridge) |
| firered | trainer-gender | PASS | stored (block 0 +0x08), female round-trips |
| firered | party | PASS | 1 member (Bulbasaur), checksum-valid, byte-identical round trip |
| firered | boxes | PASS | 14x30; Box 1/1 and sector-split Box 2/20 decoded |
| firered | pokedex | PASS | seen 1 / caught 1 / total 386 |
| firered | bag | PASS | 6 pouches read; Potion staged to 7 in the Bag workspace |
| firered | navigation | PASS | SharedPokemonWorkspace::navigate(Gen3) in View/Edit; draft and workspace unchanged (the Gen III surface calls the same Shared:: primitives inline) |
| firered | creation | PASS | defaults Pikachu Lv5, OT/TID from trainer, origin game 4; occupied slot / long OT / out-of-range values refused |
| firered | fields | PASS | nickname, level, EXP, item, moves/PP/PP Ups, IVs, EVs, OT, TID, friendship, Pokerus, ball, met level/location, nature, gender, shiny, ability, species: Keep -> finalize -> strict reparse -> reopen |
| firered | pickers | PASS | species 1-386; exact-game move pools (Gen3LearnsetTable) via the exact-format descriptor; native ability slots |
| firered | actions | PASS | View/Edit/Clone/Remove/Legality/Review/Cancel (occupied), Add/Review/Cancel (empty); Clone and Remove staged sparse |
| firered | working-copy | PASS | source untouched; Pokemon and Bag staged copies carry the edits |
| firered | review | PASS | 7 Pokemon change lines (edit, create, clone, remove) + 1 Bag line |
| firered | export | PASS | new app-owned directory; edited.sav strict reparse; only active-slot payload/checksums changed; Pokemon+Bag merged |
| firered | immutability | PASS | bytes, size and ns mtime unchanged after discovery, linking, staging, review and export |
| firered | home-refresh | PASS | size/mtime change -> discoverConfiguredLegacySaves -> profile card shows the new snapshot (legacy re-discovery; Home party strip passes level 0, form 0, shiny false: SaveSelectScreen.cpp RetroArchFRLG preview) |
| firered | relaunch | PASS | fresh bindings load + fresh discovery resolve the current file; clean workspace |
| leafgreen | discovery | PASS | discoverConfiguredLegacySaves (conventional RetroArch root) -> Ready Gen III source |
| leafgreen | linking | PASS | unassigned: visible to all profiles; claimInstanceAndSave -> reload from disk -> owner-only card -> resolveFRLGSaveInstance = same file |
| leafgreen | provider | PASS | RetroArch (SaveProviderIdentity), platform Game Boy Advance |
| leafgreen | identity | PASS | exact id from the RetroArch filename hint, family verified in the save; no hint -> AmbiguousIdentity, wrong family -> InvalidSave (FR and LG share one family marker) |
| leafgreen | trainer-name | PASS | WILL (strict parse, Save Instance, trainer bridge) |
| leafgreen | trainer-gender | PASS | stored (block 0 +0x08), female round-trips |
| leafgreen | party | PASS | 1 member (Bulbasaur), checksum-valid, byte-identical round trip |
| leafgreen | boxes | PASS | 14x30; Box 1/1 and sector-split Box 2/20 decoded |
| leafgreen | pokedex | PASS | seen 1 / caught 1 / total 386 |
| leafgreen | bag | PASS | 6 pouches read; Potion staged to 7 in the Bag workspace |
| leafgreen | navigation | PASS | SharedPokemonWorkspace::navigate(Gen3) in View/Edit; draft and workspace unchanged (the Gen III surface calls the same Shared:: primitives inline) |
| leafgreen | creation | PASS | defaults Pikachu Lv5, OT/TID from trainer, origin game 5; occupied slot / long OT / out-of-range values refused |
| leafgreen | fields | PASS | nickname, level, EXP, item, moves/PP/PP Ups, IVs, EVs, OT, TID, friendship, Pokerus, ball, met level/location, nature, gender, shiny, ability, species: Keep -> finalize -> strict reparse -> reopen |
| leafgreen | pickers | PASS | species 1-386; exact-game move pools (Gen3LearnsetTable) via the exact-format descriptor; native ability slots |
| leafgreen | actions | PASS | View/Edit/Clone/Remove/Legality/Review/Cancel (occupied), Add/Review/Cancel (empty); Clone and Remove staged sparse |
| leafgreen | working-copy | PASS | source untouched; Pokemon and Bag staged copies carry the edits |
| leafgreen | review | PASS | 7 Pokemon change lines (edit, create, clone, remove) + 1 Bag line |
| leafgreen | export | PASS | new app-owned directory; edited.sav strict reparse; only active-slot payload/checksums changed; Pokemon+Bag merged |
| leafgreen | immutability | PASS | bytes, size and ns mtime unchanged after discovery, linking, staging, review and export |
| leafgreen | home-refresh | PASS | size/mtime change -> discoverConfiguredLegacySaves -> profile card shows the new snapshot (legacy re-discovery; Home party strip passes level 0, form 0, shiny false: SaveSelectScreen.cpp RetroArchFRLG preview) |
| leafgreen | relaunch | PASS | fresh bindings load + fresh discovery resolve the current file; clean workspace |
| diamond | discovery | PASS | discoverSources(DraStic root) -> 1 Ready DP candidate + decoy Invalid; toSaveInstance Ready gen 4 |
| diamond | linking | PASS | assignFileAndSave -> reload in a new LegacySourceBindings -> resolveFileForGame Ready (same path/identity) -> openAssignedSource Ready |
| diamond | provider | PASS | binding sourceType DraStic -> "Nintendo DS • DraStic"; Save Instance row "DraStic / ..." |
| diamond | identity | PASS | raw DP save cannot prove D vs P (exactGameFromSave Invalid); exact game = assignment: same bytes Match as diamond_nds and pearl_nds, Create origin follows the assignment; platinum_nds assignment -> AssignmentMismatch |
| diamond | trainer-name | PASS | "Kylie" via Gen4ReadOnlyTrainer, TID/SID 12150/22558 |
| diamond | trainer-gender | PASS | stored (trainer +0x18 bit 0): female decoded |
| diamond | party | PASS | 2 members: Pikachu Lv20, Burmy Sandy Cloak (form 1) Lv9 |
| diamond | boxes | PASS | 18x30 native boxes; box 1 slot 1 Pikachu, rest empty, no invalid records |
| diamond | pokedex | PASS | seen 2 / caught 1 / total 493 |
| diamond | bag | PASS | 8 native pockets decoded; Medicine = Potion x5 (decodeReadOnlyBag + trainer view) |
| diamond | navigation | PASS | SharedPokemonWorkspace::navigate (Gen4, View + Edit) and Gen4BoxMoveNavigation move focus only; Edit draft and staged save byte-identical |
| diamond | creation | PASS | createBoxDraft(species 1 as Gen4SharedPokemonSurface.inc beginCreate) -> Lv5, Poke Ball, trainer OT/TID; Session Create -> keepCreate stages; occupied/out-of-range/invalid refused |
| diamond | fields | PASS | box: species/nickname/level/OT/TID/held item/friendship/language/met level/met location/ball/Pokerus/IV/EV/move/PP/PP Ups/nature/gender/shiny/ability; party: level/shiny/form (Burmy); bag quantity: all stage and round-trip through strict reparse |
| diamond | pickers | PASS | species names 1..493; Gen4MoveCompatibility::selectableMoves (the move picker source) per exact game; held-item catalog per family; descriptor move evaluator |
| diamond | actions | PASS | Gen4BoxTouchActions select/open + staged Clone/Move/Release primitives; descriptor grants BoxPokemon/Editing/Creation, never a source write (menu rows live in the non-host Gen4SharedPokemonSurface.inc); a net no-op clone/move/release may still review as "Box 2 slot 1: (empty) (edited)" (QA-F8) |
| diamond | working-copy | PASS | originalBytes == source file; staged/finalized bytes carry every edit; source file byte/mtime identical |
| diamond | review | PASS | describeStagedSlotChanges lists party slots 1-2 (edited) and Box 1 slot 1 Pikachu -> Eevee, Box 1 slot 4 (empty) -> Bulbasaur; bag edits are not listed (slot-only review) |
| diamond | export | PASS | publishVerifiedGen4Export: new directory with original_backup.sav (== source) + edited.sav; strict reparse Match; changes only inside the selected General + Storage blocks; unchanged editor refused |
| diamond | immutability | PASS | source bytes, size and ns mtime identical after discovery, linking, open, staging, actions, review and export |
| diamond | home-refresh | PASS | HomeSnapshotCache: Parsed then Reused x30 (1 parse); emulator rewrite (new mtime) -> Parsed with new party; invalid rewrite -> Stale, never Current |
| diamond | relaunch | PASS | fresh LegacySourceBindings load + fresh HomeSnapshotCache + re-inspect see the current file (Lv33 lead, new fingerprint, same source identity) |
| pearl | discovery | PASS | discoverSources(DraStic root) -> 1 Ready DP candidate + decoy Invalid; toSaveInstance Ready gen 4 |
| pearl | linking | PASS | assignFileAndSave -> reload in a new LegacySourceBindings -> resolveFileForGame Ready (same path/identity) -> openAssignedSource Ready |
| pearl | provider | PASS | binding sourceType DraStic -> "Nintendo DS • DraStic"; Save Instance row "DraStic / ..." |
| pearl | identity | PASS | raw DP save cannot prove D vs P (exactGameFromSave Invalid); exact game = assignment: same bytes Match as pearl_nds and diamond_nds, Create origin follows the assignment; heartgold_nds assignment -> AssignmentMismatch |
| pearl | trainer-name | PASS | "Kylie" via Gen4ReadOnlyTrainer, TID/SID 12150/22558 |
| pearl | trainer-gender | PASS | stored (trainer +0x18 bit 0): female decoded |
| pearl | party | PASS | 2 members: Pikachu Lv20, Burmy Sandy Cloak (form 1) Lv9 |
| pearl | boxes | PASS | 18x30 native boxes; box 1 slot 1 Pikachu, rest empty, no invalid records |
| pearl | pokedex | PASS | seen 2 / caught 1 / total 493 |
| pearl | bag | PASS | 8 native pockets decoded; Medicine = Potion x5 (decodeReadOnlyBag + trainer view) |
| pearl | navigation | PASS | SharedPokemonWorkspace::navigate (Gen4, View + Edit) and Gen4BoxMoveNavigation move focus only; Edit draft and staged save byte-identical |
| pearl | creation | PASS | createBoxDraft(species 1 as Gen4SharedPokemonSurface.inc beginCreate) -> Lv5, Poke Ball, trainer OT/TID; Session Create -> keepCreate stages; occupied/out-of-range/invalid refused |
| pearl | fields | PASS | box: species/nickname/level/OT/TID/held item/friendship/language/met level/met location/ball/Pokerus/IV/EV/move/PP/PP Ups/nature/gender/shiny/ability; party: level/shiny/form (Burmy); bag quantity: all stage and round-trip through strict reparse |
| pearl | pickers | PASS | species names 1..493; Gen4MoveCompatibility::selectableMoves (the move picker source) per exact game; held-item catalog per family; descriptor move evaluator |
| pearl | actions | PASS | Gen4BoxTouchActions select/open + staged Clone/Move/Release primitives; descriptor grants BoxPokemon/Editing/Creation, never a source write (menu rows live in the non-host Gen4SharedPokemonSurface.inc); a net no-op clone/move/release may still review as "Box 2 slot 1: (empty) (edited)" (QA-F8) |
| pearl | working-copy | PASS | originalBytes == source file; staged/finalized bytes carry every edit; source file byte/mtime identical |
| pearl | review | PASS | describeStagedSlotChanges lists party slots 1-2 (edited) and Box 1 slot 1 Pikachu -> Eevee, Box 1 slot 4 (empty) -> Bulbasaur; bag edits are not listed (slot-only review) |
| pearl | export | PASS | publishVerifiedGen4Export: new directory with original_backup.sav (== source) + edited.sav; strict reparse Match; changes only inside the selected General + Storage blocks; unchanged editor refused |
| pearl | immutability | PASS | source bytes, size and ns mtime identical after discovery, linking, open, staging, actions, review and export |
| pearl | home-refresh | PASS | HomeSnapshotCache: Parsed then Reused x30 (1 parse); emulator rewrite (new mtime) -> Parsed with new party; invalid rewrite -> Stale, never Current |
| pearl | relaunch | PASS | fresh LegacySourceBindings load + fresh HomeSnapshotCache + re-inspect see the current file (Lv33 lead, new fingerprint, same source identity) |
| platinum | discovery | PASS | discoverSources(DraStic root) -> 1 Ready PT candidate + decoy Invalid; toSaveInstance Ready gen 4 |
| platinum | linking | PASS | assignFileAndSave -> reload in a new LegacySourceBindings -> resolveFileForGame Ready (same path/identity) -> openAssignedSource Ready |
| platinum | provider | PASS | binding sourceType DraStic -> "Nintendo DS • DraStic"; Save Instance row "DraStic / ..." |
| platinum | identity | PASS | exact game from the save (Platinum layout); assignment diamond_nds -> AssignmentMismatch on inspect/open/SaveInstance |
| platinum | trainer-name | PASS | "Kylie" via Gen4ReadOnlyTrainer, TID/SID 12150/22558 |
| platinum | trainer-gender | PASS | stored (trainer +0x18 bit 0): female decoded |
| platinum | party | PASS | 2 members: Pikachu Lv20, Burmy Sandy Cloak (form 1) Lv9 |
| platinum | boxes | PASS | 18x30 native boxes; box 1 slot 1 Pikachu, rest empty, no invalid records |
| platinum | pokedex | PASS | seen 2 / caught 1 / total 493 |
| platinum | bag | PASS | 8 native pockets decoded; Medicine = Potion x5 (decodeReadOnlyBag + trainer view) |
| platinum | navigation | PASS | SharedPokemonWorkspace::navigate (Gen4, View + Edit) and Gen4BoxMoveNavigation move focus only; Edit draft and staged save byte-identical |
| platinum | creation | PASS | createBoxDraft(species 1 as Gen4SharedPokemonSurface.inc beginCreate) -> Lv5, Poke Ball, trainer OT/TID; Session Create -> keepCreate stages; occupied/out-of-range/invalid refused |
| platinum | fields | PASS | box: species/nickname/level/OT/TID/held item/friendship/language/met level/met location/ball/Pokerus/IV/EV/move/PP/PP Ups/nature/gender/shiny/ability; party: level/shiny/form (Burmy); bag quantity: all stage and round-trip through strict reparse |
| platinum | pickers | PASS | species names 1..493; Gen4MoveCompatibility::selectableMoves (the move picker source) per exact game; held-item catalog per family; descriptor move evaluator |
| platinum | actions | PASS | Gen4BoxTouchActions select/open + staged Clone/Move/Release primitives; descriptor grants BoxPokemon/Editing/Creation, never a source write (menu rows live in the non-host Gen4SharedPokemonSurface.inc); a net no-op clone/move/release may still review as "Box 2 slot 1: (empty) (edited)" (QA-F8) |
| platinum | working-copy | PASS | originalBytes == source file; staged/finalized bytes carry every edit; source file byte/mtime identical |
| platinum | review | PASS | describeStagedSlotChanges lists party slots 1-2 (edited) and Box 1 slot 1 Pikachu -> Eevee, Box 1 slot 4 (empty) -> Bulbasaur; bag edits are not listed (slot-only review) |
| platinum | export | PASS | publishVerifiedGen4Export: new directory with original_backup.sav (== source) + edited.sav; strict reparse Match; changes only inside the selected General + Storage blocks; unchanged editor refused |
| platinum | immutability | PASS | source bytes, size and ns mtime identical after discovery, linking, open, staging, actions, review and export |
| platinum | home-refresh | PASS | HomeSnapshotCache: Parsed then Reused x30 (1 parse); emulator rewrite (new mtime) -> Parsed with new party; invalid rewrite -> Stale, never Current |
| platinum | relaunch | PASS | fresh LegacySourceBindings load + fresh HomeSnapshotCache + re-inspect see the current file (Lv33 lead, new fingerprint, same source identity) |
| heartgold | discovery | PASS | discoverSources(DraStic root) -> 1 Ready HGSS candidate + decoy Invalid; toSaveInstance Ready gen 4 |
| heartgold | linking | PASS | assignFileAndSave -> reload in a new LegacySourceBindings -> resolveFileForGame Ready (same path/identity) -> openAssignedSource Ready |
| heartgold | provider | PASS | binding sourceType DraStic -> "Nintendo DS • DraStic"; Save Instance row "DraStic / ..." |
| heartgold | identity | PASS | exact game from the save (HGSS trainer ROMCode 0x1C); assignment soulsilver_nds -> AssignmentMismatch on inspect/open/SaveInstance |
| heartgold | trainer-name | PASS | "Kylie" via Gen4ReadOnlyTrainer, TID/SID 12150/22558 |
| heartgold | trainer-gender | PASS | stored (trainer +0x18 bit 0): female decoded |
| heartgold | party | PASS | 2 members: Pikachu Lv20, Burmy Sandy Cloak (form 1) Lv9 |
| heartgold | boxes | PASS | 18x30 native boxes; box 1 slot 1 Pikachu, rest empty, no invalid records |
| heartgold | pokedex | PASS | seen 2 / caught 1 / total 493 |
| heartgold | bag | PASS | 8 native pockets decoded; Medicine = Potion x5 (decodeReadOnlyBag + trainer view) |
| heartgold | navigation | PASS | SharedPokemonWorkspace::navigate (Gen4, View + Edit) and Gen4BoxMoveNavigation move focus only; Edit draft and staged save byte-identical |
| heartgold | creation | PASS | createBoxDraft(species 1 as Gen4SharedPokemonSurface.inc beginCreate) -> Lv5, Poke Ball, trainer OT/TID; Session Create -> keepCreate stages; occupied/out-of-range/invalid refused |
| heartgold | fields | PASS | box: species/nickname/level/OT/TID/held item/friendship/language/met level/met location/ball/Pokerus/IV/EV/move/PP/PP Ups/nature/gender/shiny/ability; party: level/shiny/form (Burmy); bag quantity: all stage and round-trip through strict reparse |
| heartgold | pickers | PASS | species names 1..493; Gen4MoveCompatibility::selectableMoves (the move picker source) per exact game; held-item catalog per family; descriptor move evaluator |
| heartgold | actions | PASS | Gen4BoxTouchActions select/open + staged Clone/Move/Release primitives; descriptor grants BoxPokemon/Editing/Creation, never a source write (menu rows live in the non-host Gen4SharedPokemonSurface.inc); a net no-op clone/move/release may still review as "Box 2 slot 1: (empty) (edited)" (QA-F8) |
| heartgold | working-copy | PASS | originalBytes == source file; staged/finalized bytes carry every edit; source file byte/mtime identical |
| heartgold | review | PASS | describeStagedSlotChanges lists party slots 1-2 (edited) and Box 1 slot 1 Pikachu -> Eevee, Box 1 slot 4 (empty) -> Bulbasaur; bag edits are not listed (slot-only review) |
| heartgold | export | PASS | publishVerifiedGen4Export: new directory with original_backup.sav (== source) + edited.sav; strict reparse Match; changes only inside the selected General + Storage blocks; unchanged editor refused |
| heartgold | immutability | PASS | source bytes, size and ns mtime identical after discovery, linking, open, staging, actions, review and export |
| heartgold | home-refresh | PASS | HomeSnapshotCache: Parsed then Reused x30 (1 parse); emulator rewrite (new mtime) -> Parsed with new party; invalid rewrite -> Stale, never Current |
| heartgold | relaunch | PASS | fresh LegacySourceBindings load + fresh HomeSnapshotCache + re-inspect see the current file (Lv33 lead, new fingerprint, same source identity) |
| soulsilver | discovery | PASS | discoverSources(DraStic root) -> 1 Ready HGSS candidate + decoy Invalid; toSaveInstance Ready gen 4 |
| soulsilver | linking | PASS | assignFileAndSave -> reload in a new LegacySourceBindings -> resolveFileForGame Ready (same path/identity) -> openAssignedSource Ready |
| soulsilver | provider | PASS | binding sourceType DraStic -> "Nintendo DS • DraStic"; Save Instance row "DraStic / ..." |
| soulsilver | identity | PASS | exact game from the save (HGSS trainer ROMCode 0x1C); assignment heartgold_nds -> AssignmentMismatch on inspect/open/SaveInstance |
| soulsilver | trainer-name | PASS | "Kylie" via Gen4ReadOnlyTrainer, TID/SID 12150/22558 |
| soulsilver | trainer-gender | PASS | stored (trainer +0x18 bit 0): female decoded |
| soulsilver | party | PASS | 2 members: Pikachu Lv20, Burmy Sandy Cloak (form 1) Lv9 |
| soulsilver | boxes | PASS | 18x30 native boxes; box 1 slot 1 Pikachu, rest empty, no invalid records |
| soulsilver | pokedex | PASS | seen 2 / caught 1 / total 493 |
| soulsilver | bag | PASS | 8 native pockets decoded; Medicine = Potion x5 (decodeReadOnlyBag + trainer view) |
| soulsilver | navigation | PASS | SharedPokemonWorkspace::navigate (Gen4, View + Edit) and Gen4BoxMoveNavigation move focus only; Edit draft and staged save byte-identical |
| soulsilver | creation | PASS | createBoxDraft(species 1 as Gen4SharedPokemonSurface.inc beginCreate) -> Lv5, Poke Ball, trainer OT/TID; Session Create -> keepCreate stages; occupied/out-of-range/invalid refused |
| soulsilver | fields | PASS | box: species/nickname/level/OT/TID/held item/friendship/language/met level/met location/ball/Pokerus/IV/EV/move/PP/PP Ups/nature/gender/shiny/ability; party: level/shiny/form (Burmy); bag quantity: all stage and round-trip through strict reparse |
| soulsilver | pickers | PASS | species names 1..493; Gen4MoveCompatibility::selectableMoves (the move picker source) per exact game; held-item catalog per family; descriptor move evaluator |
| soulsilver | actions | PASS | Gen4BoxTouchActions select/open + staged Clone/Move/Release primitives; descriptor grants BoxPokemon/Editing/Creation, never a source write (menu rows live in the non-host Gen4SharedPokemonSurface.inc); a net no-op clone/move/release may still review as "Box 2 slot 1: (empty) (edited)" (QA-F8) |
| soulsilver | working-copy | PASS | originalBytes == source file; staged/finalized bytes carry every edit; source file byte/mtime identical |
| soulsilver | review | PASS | describeStagedSlotChanges lists party slots 1-2 (edited) and Box 1 slot 1 Pikachu -> Eevee, Box 1 slot 4 (empty) -> Bulbasaur; bag edits are not listed (slot-only review) |
| soulsilver | export | PASS | publishVerifiedGen4Export: new directory with original_backup.sav (== source) + edited.sav; strict reparse Match; changes only inside the selected General + Storage blocks; unchanged editor refused |
| soulsilver | immutability | PASS | source bytes, size and ns mtime identical after discovery, linking, open, staging, actions, review and export |
| soulsilver | home-refresh | PASS | HomeSnapshotCache: Parsed then Reused x30 (1 parse); emulator rewrite (new mtime) -> Parsed with new party; invalid rewrite -> Stale, never Current |
| soulsilver | relaunch | PASS | fresh LegacySourceBindings load + fresh HomeSnapshotCache + re-inspect see the current file (Lv33 lead, new fingerprint, same source identity) |
| black | discovery | PASS | BW layout; discoverSources + inspectSourceFile ready; two different copies -> 2 labelled rows, Automatic refused |
| black | linking | PASS | assignValidatedSource -> reload -> resolveFileForGame Ready (same path, copy 0) -> openAssignedSource ready; other profile Unassigned, sibling title refused |
| black | provider | PASS | 'DraStic' -> 'DraStic' on the Save Instance row and the Home card (from the persisted binding) |
| black | identity | PASS | PlayerData5 version byte 21 -> black_nds although the file is named 'POKEMON W2.sav'; sibling white_nds refused (AssignmentMismatch) |
| black | trainer-name | PASS | QATEST via decodeTrainer5, Save Instance row and Gen5ReadOnlyTrainer; TID/SID/money decoded |
| black | trainer-gender | PASS | PlayerData5 0x21 = female through decodeTrainer5, Trainer bridge and card preview |
| black | party | PASS | 2 members (Pikachu L10, Gen IV origin Eevee) via parse and the Trainer bridge |
| black | boxes | PASS | 24x30; 2 occupied + retail encrypted blanks, 0 quarantined; Trainer bridge agrees |
| black | pokedex | PASS | seen 2 / caught 1 / total 649 (Zukan5 block 55 @0x21600) |
| black | bag | PASS | 5 pockets decoded (family-specific key item list) and shown by the Trainer bridge |
| black | navigation | PASS | SharedPokemonWorkspace::navigate over Gen V Edit and View drafts (18 detail rows) and Actions cursor: no byte changed; B on Edit asks Keep/Discard, Discard stages nothing |
| black | creation | PASS | defaults Pikachu L5 with the save's OT/TID/SID/gender/language and exact game; species 650, level 0, party format and sibling title refused; Add -> Create draft -> Keep -> verified |
| black | fields | PASS | 25 fields (every Editable FieldIdentity + Shiny row + Form) via stageSharedField/Name -> Keep -> verifyStagedReview -> candidate strict reparse |
| black | pickers | PASS | species 1-649 (650 refused), 559 moves + none, items <=638, 25 balls, 276 met locations, Deerling 4 forms, ability slots; Bag add lists (no Key Items/TMs), family key-item list |
| black | actions | PASS | party [View,Edit,Clone,Cancel], box [View,Edit,Move,Clone,Release,Cancel], empty [Add,Cancel]; wrap, Release+Undo, Clone, Move, Review after staging |
| black | working-copy | PASS | PK5 + Bag + Trainer staged in app memory; source image/file unchanged; candidate carries all three, backup copy byte-identical |
| black | review | PASS | 3 rows (Friendship 70->222 independently replayed, Bag 12->42, Trainer name); export confirmation; Discard All restores |
| black | export | PASS | new export dir (backup == source, edited.sav strict reparse, manifest); 159 changed bytes all whitelisted; stale observation refused, no files |
| black | immutability | PASS | bytes, size and ns mtime of the source (and its directory) unchanged after discovery, assignment, opens, all staging, candidates and export |
| black | home-refresh | PASS | HomeSnapshotCache + homeCardSnapshot: Parsed once, Reused while unchanged, re-validated Current after the rewrite (species 550, level 33, shiny), form 1 kept (QA-F5 fix holds) |
| black | relaunch | PASS | reloaded bindings + new cache + new Trainer session see the rewritten file; the old fingerprint is refused |
| white | discovery | PASS | BW layout; discoverSources + inspectSourceFile ready; two different copies -> 2 labelled rows, Automatic refused |
| white | linking | PASS | assignValidatedSource -> reload -> resolveFileForGame Ready (same path, copy 0) -> openAssignedSource ready; other profile Unassigned, sibling title refused |
| white | provider | PASS | 'melonDS' -> 'melonDS' on the Save Instance row and the Home card (from the persisted binding) |
| white | identity | PASS | PlayerData5 version byte 20 -> white_nds although the file is named 'POKEMON B2.sav'; sibling black_nds refused (AssignmentMismatch) |
| white | trainer-name | PASS | QATEST via decodeTrainer5, Save Instance row and Gen5ReadOnlyTrainer; TID/SID/money decoded |
| white | trainer-gender | PASS | PlayerData5 0x21 = female through decodeTrainer5, Trainer bridge and card preview |
| white | party | PASS | 2 members (Pikachu L10, Gen IV origin Eevee) via parse and the Trainer bridge |
| white | boxes | PASS | 24x30; 2 occupied + retail encrypted blanks, 0 quarantined; Trainer bridge agrees |
| white | pokedex | PASS | seen 2 / caught 1 / total 649 (Zukan5 block 55 @0x21600) |
| white | bag | PASS | 5 pockets decoded (family-specific key item list) and shown by the Trainer bridge |
| white | navigation | PASS | SharedPokemonWorkspace::navigate over Gen V Edit and View drafts (18 detail rows) and Actions cursor: no byte changed; B on Edit asks Keep/Discard, Discard stages nothing |
| white | creation | PASS | defaults Pikachu L5 with the save's OT/TID/SID/gender/language and exact game; species 650, level 0, party format and sibling title refused; Add -> Create draft -> Keep -> verified |
| white | fields | PASS | 25 fields (every Editable FieldIdentity + Shiny row + Form) via stageSharedField/Name -> Keep -> verifyStagedReview -> candidate strict reparse |
| white | pickers | PASS | species 1-649 (650 refused), 559 moves + none, items <=638, 25 balls, 276 met locations, Deerling 4 forms, ability slots; Bag add lists (no Key Items/TMs), family key-item list |
| white | actions | PASS | party [View,Edit,Clone,Cancel], box [View,Edit,Move,Clone,Release,Cancel], empty [Add,Cancel]; wrap, Release+Undo, Clone, Move, Review after staging |
| white | working-copy | PASS | PK5 + Bag + Trainer staged in app memory; source image/file unchanged; candidate carries all three, backup copy byte-identical |
| white | review | PASS | 3 rows (Friendship 70->222 independently replayed, Bag 12->42, Trainer name); export confirmation; Discard All restores |
| white | export | PASS | new export dir (backup == source, edited.sav strict reparse, manifest); 159 changed bytes all whitelisted; stale observation refused, no files |
| white | immutability | PASS | bytes, size and ns mtime of the source (and its directory) unchanged after discovery, assignment, opens, all staging, candidates and export |
| white | home-refresh | PASS | HomeSnapshotCache + homeCardSnapshot: Parsed once, Reused while unchanged, re-validated Current after the rewrite (species 550, level 33, shiny), form 1 kept (QA-F5 fix holds) |
| white | relaunch | PASS | reloaded bindings + new cache + new Trainer session see the rewritten file; the old fingerprint is refused |
| black2 | discovery | PASS | B2W2 layout; discoverSources + inspectSourceFile ready; two different copies -> 2 labelled rows, Automatic refused |
| black2 | linking | PASS | assignValidatedSource -> reload -> resolveFileForGame Ready (same path, copy 0) -> openAssignedSource ready; other profile Unassigned, sibling title refused |
| black2 | provider | PASS | 'RetroArch' -> 'RetroArch' on the Save Instance row and the Home card (from the persisted binding) |
| black2 | identity | PASS | PlayerData5 version byte 23 -> black2_nds although the file is named 'POKEMON W.sav'; sibling white2_nds refused (AssignmentMismatch) |
| black2 | trainer-name | PASS | QATEST via decodeTrainer5, Save Instance row and Gen5ReadOnlyTrainer; TID/SID/money decoded |
| black2 | trainer-gender | PASS | PlayerData5 0x21 = female through decodeTrainer5, Trainer bridge and card preview |
| black2 | party | PASS | 2 members (Pikachu L10, Gen IV origin Eevee) via parse and the Trainer bridge |
| black2 | boxes | PASS | 24x30; 2 occupied + retail encrypted blanks, 0 quarantined; Trainer bridge agrees |
| black2 | pokedex | PASS | seen 2 / caught 1 / total 649 (Zukan5 block 54 @0x21400) |
| black2 | bag | PASS | 5 pockets decoded (family-specific key item list) and shown by the Trainer bridge |
| black2 | navigation | PASS | SharedPokemonWorkspace::navigate over Gen V Edit and View drafts (18 detail rows) and Actions cursor: no byte changed; B on Edit asks Keep/Discard, Discard stages nothing |
| black2 | creation | PASS | defaults Pikachu L5 with the save's OT/TID/SID/gender/language and exact game; species 650, level 0, party format and sibling title refused; Add -> Create draft -> Keep -> verified |
| black2 | fields | PASS | 25 fields (every Editable FieldIdentity + Shiny row + Form) via stageSharedField/Name -> Keep -> verifyStagedReview -> candidate strict reparse |
| black2 | pickers | PASS | species 1-649 (650 refused), 559 moves + none, items <=638, 25 balls, 276 met locations, Deerling 4 forms, ability slots; Bag add lists (no Key Items/TMs), family key-item list |
| black2 | actions | PASS | party [View,Edit,Clone,Cancel], box [View,Edit,Move,Clone,Release,Cancel], empty [Add,Cancel]; wrap, Release+Undo, Clone, Move, Review after staging |
| black2 | working-copy | PASS | PK5 + Bag + Trainer staged in app memory; source image/file unchanged; candidate carries all three, backup copy byte-identical |
| black2 | review | PASS | 3 rows (Friendship 70->222 independently replayed, Bag 12->42, Trainer name); export confirmation; Discard All restores |
| black2 | export | PASS | new export dir (backup == source, edited.sav strict reparse, manifest); 159 changed bytes all whitelisted; stale observation refused, no files |
| black2 | immutability | PASS | bytes, size and ns mtime of the source (and its directory) unchanged after discovery, assignment, opens, all staging, candidates and export |
| black2 | home-refresh | PASS | HomeSnapshotCache + homeCardSnapshot: Parsed once, Reused while unchanged, re-validated Current after the rewrite (species 550, level 33, shiny), form 1 kept (QA-F5 fix holds) |
| black2 | relaunch | PASS | reloaded bindings + new cache + new Trainer session see the rewritten file; the old fingerprint is refused |
| white2 | discovery | PASS | B2W2 layout; discoverSources + inspectSourceFile ready; two different copies -> 2 labelled rows, Automatic refused |
| white2 | linking | PASS | assignValidatedSource -> reload -> resolveFileForGame Ready (same path, copy 0) -> openAssignedSource ready; other profile Unassigned, sibling title refused |
| white2 | provider | PASS | 'Manual' -> 'Manual file' on the Save Instance row and the Home card (from the persisted binding) |
| white2 | identity | PASS | PlayerData5 version byte 22 -> white2_nds although the file is named 'POKEMON B.sav'; sibling black2_nds refused (AssignmentMismatch) |
| white2 | trainer-name | PASS | QATEST via decodeTrainer5, Save Instance row and Gen5ReadOnlyTrainer; TID/SID/money decoded |
| white2 | trainer-gender | PASS | PlayerData5 0x21 = female through decodeTrainer5, Trainer bridge and card preview |
| white2 | party | PASS | 2 members (Pikachu L10, Gen IV origin Eevee) via parse and the Trainer bridge |
| white2 | boxes | PASS | 24x30; 2 occupied + retail encrypted blanks, 0 quarantined; Trainer bridge agrees |
| white2 | pokedex | PASS | seen 2 / caught 1 / total 649 (Zukan5 block 54 @0x21400) |
| white2 | bag | PASS | 5 pockets decoded (family-specific key item list) and shown by the Trainer bridge |
| white2 | navigation | PASS | SharedPokemonWorkspace::navigate over Gen V Edit and View drafts (18 detail rows) and Actions cursor: no byte changed; B on Edit asks Keep/Discard, Discard stages nothing |
| white2 | creation | PASS | defaults Pikachu L5 with the save's OT/TID/SID/gender/language and exact game; species 650, level 0, party format and sibling title refused; Add -> Create draft -> Keep -> verified |
| white2 | fields | PASS | 25 fields (every Editable FieldIdentity + Shiny row + Form) via stageSharedField/Name -> Keep -> verifyStagedReview -> candidate strict reparse |
| white2 | pickers | PASS | species 1-649 (650 refused), 559 moves + none, items <=638, 25 balls, 276 met locations, Deerling 4 forms, ability slots; Bag add lists (no Key Items/TMs), family key-item list |
| white2 | actions | PASS | party [View,Edit,Clone,Cancel], box [View,Edit,Move,Clone,Release,Cancel], empty [Add,Cancel]; wrap, Release+Undo, Clone, Move, Review after staging |
| white2 | working-copy | PASS | PK5 + Bag + Trainer staged in app memory; source image/file unchanged; candidate carries all three, backup copy byte-identical |
| white2 | review | PASS | 3 rows (Friendship 70->222 independently replayed, Bag 12->42, Trainer name); export confirmation; Discard All restores |
| white2 | export | PASS | new export dir (backup == source, edited.sav strict reparse, manifest); 159 changed bytes all whitelisted; stale observation refused, no files |
| white2 | immutability | PASS | bytes, size and ns mtime of the source (and its directory) unchanged after discovery, assignment, opens, all staging, candidates and export |
| white2 | home-refresh | PASS | HomeSnapshotCache + homeCardSnapshot: Parsed once, Reused while unchanged, re-validated Current after the rewrite (species 550, level 33, shiny), form 1 kept (QA-F5 fix holds) |
| white2 | relaunch | PASS | reloaded bindings + new cache + new Trainer session see the rewritten file; the old fingerprint is refused |

### Cross-generation comparison

| Fact | Values (versions) | Consistent |
| --- | --- | --- |
| `bag.read` | `yes` (red, blue, yellow, gold, silver, crystal, ruby, sapphire, emerald, firered, leafgreen, diamond, pearl, platinum, heartgold, soulsilver, black, white, black2, white2) | yes |
| `create.default.level` | `5` (red, blue, yellow, gold, silver, crystal, ruby, sapphire, emerald, firered, leafgreen, diamond, pearl, platinum, heartgold, soulsilver, black, white, black2, white2) | yes |
| `create.default.species` | `1` (red, blue, yellow, diamond, pearl, platinum, heartgold, soulsilver); `25` (gold, silver, crystal, ruby, sapphire, emerald, firered, leafgreen, black, white, black2, white2) | **NO** |
| `export.output` | `new-file-only` (red, blue, yellow, gold, silver, crystal, ruby, sapphire, emerald, firered, leafgreen, diamond, pearl, platinum, heartgold, soulsilver, black, white, black2, white2) | yes |
| `export.reparse` | `strict` (red, blue, yellow, gold, silver, crystal, ruby, sapphire, emerald, firered, leafgreen, diamond, pearl, platinum, heartgold, soulsilver, black, white, black2, white2) | yes |
| `home.party.form` | `dropped` (gold, silver, crystal, ruby, sapphire, emerald, firered, leafgreen); `kept` (diamond, pearl, platinum, heartgold, soulsilver, black, white, black2, white2); `n/a` (red, blue, yellow) | **NO** |
| `home.party.level` | `dropped` (ruby, sapphire, emerald, firered, leafgreen); `kept` (red, blue, yellow, gold, silver, crystal, diamond, pearl, platinum, heartgold, soulsilver, black, white, black2, white2) | **NO** |
| `home.party.shiny` | `hidden` (red, blue, yellow, gold, silver, crystal, ruby, sapphire, emerald, firered, leafgreen); `shown` (diamond, pearl, platinum, heartgold, soulsilver, black, white, black2, white2) | **NO** |
| `home.refresh` | `legacy-rediscovery` (red, blue, yellow, gold, silver, crystal, ruby, sapphire, emerald, firered, leafgreen); `snapshot-cache` (diamond, pearl, platinum, heartgold, soulsilver, black, white, black2, white2) | **NO** |
| `identity.source` | `assignment` (diamond, pearl); `filename` (red, blue, yellow, gold, silver, ruby, sapphire, emerald, firered, leafgreen); `save-internal` (crystal, platinum, heartgold, soulsilver, black, white, black2, white2) | **NO** |
| `nav.mutates` | `no` (red, blue, yellow, gold, silver, crystal, ruby, sapphire, emerald, firered, leafgreen, diamond, pearl, platinum, heartgold, soulsilver, black, white, black2, white2) | yes |
| `trainer.gender` | `not-stored` (red, blue, yellow, gold, silver); `stored` (crystal, ruby, sapphire, emerald, firered, leafgreen, diamond, pearl, platinum, heartgold, soulsilver, black, white, black2, white2) | **NO** |
