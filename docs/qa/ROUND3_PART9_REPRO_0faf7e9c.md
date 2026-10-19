# Round 3, Part 9: source reproduction audit on 0faf7e9c

This is a read-only audit. `git diff 0faf7e9c -- src include` is empty, so the findings apply to 0faf7e9c. Every result comes from source traces and host tests. No device or emulator was used. Wherever a row says "device-only", the owner has to check that part.

| # | Report | Verdict | Owning lane |
|---|---|---|---|
| 1 | Gen V Create/Edit/Clone/Move/Release parity | **REPRODUCES (partial)** | S |
| 2 | Gen V species picker sprites, shiny preview, restrictions | Does not reproduce in source; sprites for 494-649 need a device check | — |
| 3 | Generic emulator labels | Does not reproduce | — |
| 4 | RetroArch/DraStic launch and exit | **REPRODUCES as designed**: return-to-app is opt-in; launch needs a device | 2R |
| 5 | Touch Quick View drag / modal isolation | Does not reproduce on host; drag feel needs a device | P (test gap) |
| 6 | TM/HM/Mail/Ball/item sprites | **REPRODUCES (Gen V only)** | I |
| 7 | Gen V save-instance profile assignment | Does not reproduce on host; profile UID needs a device | — |
| 8 | Gen IV multi-select Move / Y | **REPRODUCES in the Storage view** | S |
| 9 | Home / Profile navigation | Not a defect; one help-text nit | 2R (nit) |
| 10 | Region backgrounds / RomFS | Does not reproduce for Gen I-V; the packaged NRO needs CI | — |

## Details

1. **Gen V parity.** Clone, Move and Release are offered only when `workspace.originalOccupied(slot)` is true (`include/Integration/Gen5/Gen5SharedEditorBridge.h:59-68`). That check is false for a slot with a pending change (`Gen5StagedPokemonWorkspace.h:226-231`). A host probe showed:
   - an original slot gets `[View] [Edit] [Clone] [Move Pokemon] [Remove] [Cancel]`;
   - a cloned or created slot gets only `[View] [Edit] [Review Pending Changes] [Cancel]`.

   Gen IV works from the reparsed staged buffer (`Gen4StagedPokemonEditor.cpp:421-438, 551-560`). Gen V Move also uses Box/Slot number prompts (`Gen5SharedReadOnlySurface.inc:1216-1238`), where Gen IV uses the shared ClassicPackedMove carry. Gen V has no Origin & Legality row (Gen IV: `Gen4SharedPokemonSurface.inc:954`).
   **Fix:** base eligibility on the staged view, and route Move through ClassicPackedMove.
2. **Species picker.** It is the shared picker, species 1-649, with a Y toggle for Normal/Shiny (`SharedSpeciesPicker.h:389-430`). Ineligible picks are refused after the choice (`Gen5SharedScreenState.h:399-419`), the same as Gen III/IV. `tools/check_device_assets.py` checks shiny sprites only for Gen I (lines 178-179), so the Gen V shiny art is unchecked.
3. **Emulator labels.** Home cards show the provider name: DraStic, melonDS, RetroArch, or "RetroArch (DeSmuME)" (`SaveProviderIdentity.h:66-142`). `test_home_card_providers` passes. Minor: `legacyProviderLabel` shows the raw label "RetroArch/DeSmuME".
4. **Launch/exit.** The return host runs only when `<configRoot>/experimental_return_host.enable` exists (`GameLauncher.cpp:38-41, 1258-1268`). By default the Home confirm says that quitting the emulator returns to the Homebrew Menu, or to DraStic's own launcher. With the marker present, the plan returns to PokeBank NX. This is intended until the return host is validated on device.
5. **Touch.** No "Quick View" surface exists. View/Edit drag, panel swipe and modal layering are wired (`SharedPokemonEditorSurface.inc:533-617`; `ScreenChrome.h:306, 408-418`). Gen V Review draws without `beginModal` (`Gen5SharedReadOnlySurface.inc:1673-1676`), but it only accepts keys. **Gap:** `tests/test_touch_scroll_state.cpp` is in no `Makefile.host*`. It passes when built by hand.
6. **Item sprites.** `test_item_sprite_coverage.py` passes for Gen I-IV: 124/221/307/514 eligible, 0 missing. Gen V is not covered by that test. Findings for Gen V:
   - 7 feathers have no icon (ids 565-571). The pinned art exists as `*-wing`, so only the aliases are missing.
   - All 101 TMs/HMs have no icon. `MachineDisplay.cpp:32-112` has no BW/B2W2 case, and `ClassicMachineArtwork.h:109` returns type 0.
   - HM01-06 (ids 420-425) are labelled "TM93".."TM98", the BDSP names, so they read as duplicate TM93-98 (`ItemNames.cpp:429-434`). A host probe printed `game=BW id=420 label='TM93'`.

   **Fix:** add BW/B2W2 machine mapping (TM01-92 = 328-419, HM01-06 = 420-425, TM93-95 = 618-620) and the feather to wing aliases, and extend coverage to Gen V. Citation to verify against the pinned PKHeX item tables before landing.
7. **Profile assignment.** Assign, reopen, stage and export is complete (`SaveSelectScreen.cpp:2435-2686`). `test_gen5_source_e2e`, `test_gen5_readonly_save`, `test_save_instance_model` and the architecture contract all pass.
8. **Gen IV Storage view.** In the save pane:
   - Y is stripped as read-only (`TrainerViewScreenBase.inc:2369-2372`).
   - Actions → Move picks up from `stSaveBox/stSaveSlot` (`Gen4SharedPokemonSurface.inc:276`), but `placeGen4` drops at the Boxes-tab cursor (`ClassicPackedMoveOverlay.inc:604-605`).
   - No carry is drawn there (`:982`; `StoragePanel.cpp` never calls `hidesGen4CarriedSource`).

   The Boxes view works and is covered by `test_gen4_hardware_surface_contract`. **Fix:** use the Storage cursor for pickup, drop and the drawn carry, or send Move to Boxes, and keep Y when a staged Gen IV editor exists.
9. **Home/Profile.** In Home, L/R changes the game. In the Games browser, L/R changes the profile when there is more than one, and the footer says so. `test_game_hub_contract.py` passes. Nit: the Help row "L/R Switch profile" also shows with a single profile (`SaveSelectScreen.cpp:4508`).
10. **RomFS.** The referenced regions are covered by the NRO gate (1268 paths). hisui and paldea (Switch-only titles) are fetched at release. `test_qa_regressions_assets.py` (QA-F1), `test_verify_nro_romfs.py` and `test_device_asset_png_validation.py` pass.

## Owner device checks

- Gen V species picker: Victini-Genesect with the Y shiny toggle.
- Emulator quit: you return where the notice says.
- View Details: drag and panel swipe.
- A second Switch profile sees "belongs to another Switch profile".

All of these are NOT DEVICE TESTED.
