from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "src/UI/SaveSelectScreen.cpp").read_text(encoding="utf-8")
header = (ROOT / "include/UI/SaveSelectScreen.h").read_text(encoding="utf-8")
ui_manager = (ROOT / "src/UI/UI.cpp").read_text(encoding="utf-8")
shell_source = (ROOT / "src/UI/AppShellScreen.cpp").read_text(encoding="utf-8")
system_icons = (ROOT / "src/UI/SystemIcons.cpp").read_text(encoding="utf-8")
product_art_fetch = (ROOT / "tools/fetch_product_art.py").read_text(encoding="utf-8")
product_workflow = (ROOT / ".github/workflows/product-ui-native.yml").read_text(encoding="utf-8")
gen4_workflow = (ROOT / ".github/workflows/gen4-shared-editor-candidate.yml").read_text(encoding="utf-8")
framebuffer = (ROOT / "src/UI/PKSEFramebuffer.cpp").read_text(encoding="utf-8")
game_identity = (ROOT / "src/Games/GameIdentity.cpp").read_text(encoding="utf-8")

def require(cond: bool, message: str) -> None:
    if not cond:
        raise SystemExit(message)

# Approved product-home hierarchy.
require('"MASTER VAULT"' in source, "product home must expose Master Vault")
require('"POKÉDEX"' in source, "product home must expose Pokédex")
require('"PARTY"' in source, "product home must expose the real party strip")
require('"A", "OPEN"' in source, "selected-game card must expose the Open action")
require('"ZR", launchLabel' in source,
        "selected-game card must expose the dynamic Launch / Link Game File action")
require('{"Games", "Banks", "Items", "Search", "More"}' in source,
        "persistent dock must expose Games/Banks/Items/Search/More without a duplicate Settings slot")
require("hubDockFocused" in source and "hubFeatureIndex" in source and "activateHubDock" in source,
        "approved home destinations must be controller-focusable, not decorative")
require("HidNpadButton_L" in source and "HidNpadButton_R" in source,
        "L/R must switch the selected game")
require('"Pokédex Progress"' in source and
        "previewDexSeen" in source and "previewDexCaught" in source and
        '"   •   Owned "' in source,
        "selected-game card must show real parsed Pokédex Seen/Owned progress when supported")
require("parsed.pokedexProgress()" in source and
        "previewDexSeen = dex.seen;" in source and
        "previewDexCaught = dex.caught;" in source and
        "previewDexTotal = dex.total;" in source,
        "installed Switch titles must populate Product Home from the trainer's authoritative Pokédex reader")
for dex_reader in (
    "src/Trainer/Trainer7LGPE.cpp",
    "src/Trainer/Trainer8BDSP.cpp",
    "src/Trainer/Trainer8SWSH.cpp",
    "src/Trainer/Trainer8LA.cpp",
    "src/Trainer/Trainer9SV.cpp",
):
    require("pokedexProgress() const" in (ROOT / dex_reader).read_text(encoding="utf-8"),
            f"missing authoritative Pokédex progress reader: {dex_reader}")
require('"Trainer"' in source,
        "selected-game card must expose trainer information")
require("PROFILE_AVATAR" in source and "SystemIcons::userIcon" in source,
        "product header must expose the current profile identity")
require("trainerPortraitForGame" in source and "drawTrainerPortrait" in source,
        "Product Home must use the grounded trainer portrait model")
for trainer in ("Brendan", "May", "Red", "Leaf", "Lucas", "Dawn", "Ethan", "Lyra", "Kris"):
    require(f'"{trainer}"' in source, f"trainer portrait mapping is missing {trainer}")
require("trainerGenderKnown" in header and "previewTrainerGenderKnown" in header,
        "trainer portraits must distinguish proven gender from unknown appearance")
require("opened.save->trainer().gender" in source,
        "Gen IV portrait identity must use the parsed save gender")

# Safe preview and launch boundaries remain unchanged.
require("requestGameLaunch" in source, "launch shortcut must route through the shared launcher")
require('fsdevMountSaveData("pbpreview"' in source,
        "native party preview must use an explicit read-only preview mount boundary")
require('fsdevUnmountDevice("pbpreview")' in source,
        "native party preview must always leave the preview mount")
require("fsdevCommitDevice" not in source,
        "product-home preview must never commit a live source save")
require("restoreBackupToTitle" not in source,
        "product-home preview must never restore/inject a save")
require("PartyPreviewSlot" in header, "party preview model must remain explicit")

# Product Home, not the retired dashboard, is the app root.
require("const auto destination = handleSaveSelection();" in ui_manager,
        "approved Games/product-home screen must be the app root")
require("shell.hasOverlay()" in ui_manager,
        "secondary destinations must return directly to product home")
require("Dest::MasterVault" in ui_manager and "Dest::Pokedex" in ui_manager,
        "right-side feature cards must route through the secondary shell")
require("Dest::Banks" in ui_manager and "Dest::Search" in ui_manager and "Dest::Settings" in ui_manager,
        "secondary destinations and the Product Home Settings gear must route through the shell")

# In-app launch linking remains app-owned and path-safe.
require("GameFilePicker" in header and "Overlay::GameFilePicker" in source,
        "product home must retain the in-app game-file browser")
require("saveGameLaunchBinding" in source,
        "Link Game File must persist app-owned launch metadata")
require('"Y", "Up Folder"' in source,
        "game-file browser must support controller folder navigation")
require("promptText(" not in source[source.find("openGameFilePicker"):source.find("openGen4Setup")],
        "launch linking must not require typing a raw SD path")

launcher = (ROOT / "src/UI/GameLauncher.cpp").read_text(encoding="utf-8")
require("The fifth field must consume the rest of the row" in launcher,
        "launch binding parser must reject extra fields")
require("result.launcherPath = defaultLauncherPath" in launcher,
        "stored launch metadata must not choose an arbitrary launcher NRO")
require("result.corePath = defaultRetroArchCore" in launcher,
        "stored launch metadata must not choose an arbitrary RetroArch core")
require("familyPrefix" in launcher and "const StoredLaunchBinding* unique = nullptr;" in launcher and
        "if (unique) return false;" in launcher and "regularFile(stored.contentPath)" in launcher,
        "launch binding compatibility must recover exactly one valid old source identity and fail closed on ambiguity")

# Developer-dashboard language and the rejected Trade dock must not return to Product Home.
require('"QUICK ACCESS"' not in source,
        "approved Product Home must not restore the old Quick Access sub-dock")
require('"Trade is not implemented yet."' not in source,
        "approved dock must not restore the rejected Trade destination")

# Secondary destinations stay inside the approved product shell rather than developer-style modals.
require("overlay = Overlay::Settings;" in shell_source,
        "Diagnostics Back must return to Settings")
require("constexpr int x = 24, y = 78, w = 1232, h = 548;" in shell_source,
        "Banks/Search/Pokédex must use the wide product destination surface")
require('"Coming Soon — no save data was changed."' in shell_source,
        "future destinations must use product-facing unavailable copy")
require("if (gameFocused)" in source and "Colors::Info, 3" in source,
        "selected game OPEN action must carry the approved teal/cyan focus treatment")

# Pokémon identity and future navigation.
require('"UI/SpriteManager.h"' in source and "SpriteManager::getIconSprite" in source,
        "Product Home party must use the existing Pokémon sprite pipeline")
require("containSprite" in source,
        "party sprites must preserve aspect ratio")
require("MainMenuDestination::More" in source and "Dest::More" in ui_manager,
        "More must be a real routed product destination")
require("drawProductHelpOverlay" in source and
        '"Games opens the full artwork browser. Profile and Settings use the top-right controls."' in source and
        '"Open the quick right-side artwork browser"' in source,
        "Product Home Help must explain the full Games browser, Y quick drawer, Profile and Settings")
activate_start = source.index("void SaveSelectScreen::activateHubDock()")
activate_end = source.index("void SaveSelectScreen::activateGameWorkspace()", activate_start)
dock_activation = source[activate_start:activate_end]
require("classicGamesActive = true;" in dock_activation and
        "overlay = Overlay::GamesDrawer;" not in dock_activation and
        "overlay = Overlay::GameWorkspace;" not in dock_activation,
        "Games must open the restored full-screen artwork browser")
require('"Pokémon Games"' in source and "CLASSIC_ICON" in source and
        "SystemIcons::gameCardIcon" in source and
        "CLASSIC_MAX_COLS = 6" in source and
        "constexpr int avatarSize = 44;" in source,
        "the restored Games browser must use the PKSE-style six-column artwork grid with compact top-right profile control")
classic_draw = source[source.index("if (classicGamesActive) {", source.index("void SaveSelectScreen::draw(")):
                      source.index("drawAppBackdrop(fb);", source.index("void SaveSelectScreen::draw("))]
require("scrollClassicSelectionIntoView();" in classic_draw and "std::clamp(titleIndex" in classic_draw,
        "the full Games artwork browser must normalize selection/scroll immediately after save/source changes")
require('{"+", "Current Game"}' in source,
        "Plus must open Current Game tools")
require("+: Settings" not in source and '{"+" , "Settings"}' not in source and '{"+" , "Settings"}' not in shell_source,
        "Plus must never be a Settings shortcut")
require("HidNpadButton_ZL" not in source,
        "Product Home must not retain the old ZL profile shortcut")
require("kind == GameLaunchProviderKind::DraStic" in launcher and
        'findContentMatches({"sdmc:/switch/drastic/games"}' in launcher and
        "Direct selected-ROM handoff is not supported by this DraStic build." not in launcher,
        "current DraStic integration must resolve/link a real DS game path instead of launcher-only mode")
require("descriptor.state != GameLaunchState::LauncherOnly" in launcher,
        "launcher-only fallbacks must never receive a falsely linked ROM argument")
require("GameLaunchProviderKind::MelonDS" in launcher,
        "melonDS provider-aware direct content launch support must remain present")
require("OpenIntent::Items" in source and "hubDockIndex == 2" in source,
        "Backpack/Items must replace the old Backups root slot")
require("selectCurrentTitleForItems" in source and
        "sameValidatedSnapshot" in source and
        "openAssignedSource" in source,
        "Backpack must revalidate the already-selected source without opening a chooser")
require("headerRects.push_back" in source and
        "drawHubDockIcon(fb, 5, settingsX, settingsY, PROFILE_AVATAR, settingsFocused)" in source and
        '{"Games", "Banks", "Items", "Search", "More"}' in source,
        "Product Home must render one selectable header Settings gear and no dock Settings")
require("The right-side feature cards are stacked" in source and
        "The round-logo strip behaves like an ordinary horizontal control row." in source and
        "if (hubDockIndex > 0)" in source and "--hubDockIndex;" in source and
        "if (hubDockIndex < 4) ++hubDockIndex;" in source and
        "headerActionIndex = 0;" in source,
        "Product Home right-side focus must keep header reachability while dock Left/Right moves exactly one logo at a time")
require("constexpr int featureH = 132;" in source and
        "const int navY = dexY + featureH + 20;" in source and
        "constexpr int buttonD = 74;" in source and
        "fb.drawFilledCircle(cx, cy, buttonD / 2, Colors::SurfaceRaised);" in source,
        "Vault/Pokédex must use larger stacked cards while Games/Banks/Items/Search/More stay lower round logos")
require('drawNavBar(fb, {{"L/R", "Change Game"}, {"A", "Open"}, {"Y", "Quick Games"}' in source,
        "Product Home footer must expose Y Quick Games without repeating ZR Launch")
require("ProfilePicker" in header and "profilePickerIndex" in source and
        "setUser(profilePickerIndex)" in source and
        '"SWITCH PROFILE"' in source and '"Choose Profile"' in source and '"CURRENT"' in source,
        "the profile avatar must open the centered professional profile chooser")
require("GamesDrawer" in header and '"QUICK GAMES"' in source and
        "constexpr int cols = 3;" in source and "constexpr int visibleRows = 3;" in source and
        "constexpr int w = 520;" in source and
        "gamesDrawerIndex = std::clamp(" in source and
        "gamesDrawerScroll = std::clamp(" in source and
        "gameCardHasSpecificArtwork" in source and '"1 save"' in source and '" saves"' in source and
        '{"Y", "Close"}' not in source and '{"X", "Save / Source"}' in source,
        "Y Quick Games must be a three-row/three-column artwork-first browser with save counts and no Y-close hint")
drawer_update_start = source.index("if (overlay == Overlay::GamesDrawer)", source.index("void SaveSelectScreen::update"))
drawer_update_end = source.index("if (overlay == Overlay::ProfilePicker)", drawer_update_start)
drawer_update = source[drawer_update_start:drawer_update_end]
drawer_draw_start = source.index("if (overlay == Overlay::GamesDrawer)", source.index("void SaveSelectScreen::draw("))
drawer_draw_end = source.index("} else if (overlay == Overlay::ProfilePicker)", drawer_draw_start)
drawer_draw = source[drawer_draw_start:drawer_draw_end]
require("if (kDown & (HidNpadButton_B | HidNpadButton_Y))" not in drawer_update and
        "// B is the only close/back control" in drawer_update and
        "fb.drawFilledRect(x, 0, 2, h, Colors::FocusBorder);" not in drawer_draw,
        "Quick Games Y must be inert, B-only close, and the colored drawer edge stripe must stay removed")
require("openGen4Setup(game.gameId, \"Assign, repair, or change this game's save source.\", true)" in source and
        "Overlay::LegacyAssignment" in source,
        "Games save assignment must cover Gen IV linking and unassigned Gen I-III sources")
require("gen4SetupFromGamesDrawer ? Overlay::GamesDrawer : Overlay::None" in source and
        "gen4SetupFromGamesDrawer = false;" in source,
        "Gen IV source setup must return cleanly to Games when launched from the drawer")
require('kSettingsCategories' in shell_source and
        '"Look", "System", "Data", "Update", "Developer", "Info"' in shell_source and
        '"User", "Look"' not in shell_source and '"Profile", "Switch User"' not in shell_source,
        "Settings must omit the non-functional User category and keep the useful two-pane categories")
require("settingsCategoryFocused" in shell_source and "settingsOptionCount" in shell_source,
        "Settings must keep independent category/option focus")
require('"Left/Right", "Pane"' in shell_source,
        "Settings footer must explain two-pane navigation")
require('"Source Save Protection", "LOCKED"' in shell_source and
        '"Update Support", "Coming Soon"' in shell_source,
        "Settings must expose truthful Data and Update states without fake backends")
require('"Mystery Gifts"' in shell_source and '"Clone Lineage"' in shell_source,
        "More screen must reserve truthful future-feature modules")

require("handleDefaultQuickOpen" in ui_manager and "backupSaveData" in ui_manager and
        "SaveSelectScreen::OpenIntent::Backups" in ui_manager,
        "normal Switch Open must auto-create a protected backup/working copy while Backups stays explicit")
require("Backups" in header and source.count("openIntent = OpenIntent::Backups;") == 1,
        "Current Game -> Backups must be the only normal route into backup history")
require("handleItemsQuickOpen" in ui_manager and "backupSaveData" in ui_manager,
        "Switch Backpack quick-open must create the normal protected backup before Items")
require("if (kDown & HidNpadButton_Plus) {\n        overlay = Overlay::Settings;" not in shell_source,
        "secondary menus must not expose a Plus-to-Settings shortcut")
home_menu = (ROOT / "src/UI/Panels/HomeMenuPanel.cpp").read_text(encoding="utf-8")
trainer_base = (ROOT / "src/UI/TrainerViewScreenBase.inc").read_text(encoding="utf-8")
trainer_header = (ROOT / "include/UI/TrainerViewScreenBase.h").read_text(encoding="utf-8")
save_confirm = (ROOT / "src/UI/Dialogs/SaveConfirmDialog.cpp").read_text(encoding="utf-8")
require("itemsShortcutActive = true;" in trainer_header and
        "selectedMode == ViewMode::Items && itemsShortcutActive" in trainer_base and
        "Product Home -> Items has a dedicated escape-safe contract" in trainer_base and
        "A: Save Working Copy & Home" in trainer_base and
        "Y: Discard & Home" in trainer_base and
        "B: Keep Editing" in trainer_base and
        "if (!goBack) exitAfterSave = false;" in trainer_base and
        '"Save Copy & Home"' in save_confirm and
        '"Discard & Home"' in save_confirm and
        '"Keep Editing"' in save_confirm,
        "dirty Product Home Items must always offer save-working-copy, discard-to-home and keep-editing without trapping the user")
require('{ "Settings", "S", 5 }' not in home_menu and
        'Icon icons[2]' in home_menu,
        "loaded-game home must not expose a Settings icon")
plus_start = trainer_base.index("// Product Home owns Settings and the Current Game tools menu.")
plus_end = trainer_base.index("// Reusable value picker", plus_start)
require("selectedMode = ViewMode::Settings" not in trainer_base[plus_start:plus_end],
        "loaded-game Plus handling must not open Settings")
require("if (!shell.hasOverlay()) break;" in ui_manager,
        "closing a secondary shell must not draw the retired shell for one stale frame")
require("!selectScreen.hasSelectedTitle() && !selectScreen.shouldExit()" in ui_manager and
        "if (selectScreen.shouldExit()) break;" in ui_manager,
        "Product Home must stop drawing immediately after selection or exit")
require("!backupScreen.shouldExit() && !backupScreen.hasSelectedBackup()" in ui_manager and
        "if (backupScreen.shouldExit()) break;" in ui_manager,
        "backup chooser must stop drawing immediately after selection or exit")
require(ui_manager.count(
            "if (trainerScreen.shouldExit() || trainerScreen.hasRequestedExit()) break;") >= 3,
        "trainer/game loops must retire before draw on every supported source path")
require("partySpriteH = 82" in source and
        "partySpriteBottom = 80" in source and
        "rect.y = slotY + partySpriteBottom - rect.height" in source and
        "fb.measureText(name, nw, nh, TextStyle::Body);" in source and
        "name, Colors::TextPrimary, TextStyle::Body" in source and
        "slotY + 101" in source and
        "level, Colors::TextSecondary, TextStyle::Caption" in source and
        "drawShinyMark(sx + slotW - 19, slotY + 6, 13" in source,
        "Product Home party must use larger bottom-aligned sprites, 20px names, secondary levels and a reserved shiny corner")

require("gameRegionBackdropKey(title.gameId)" in source and
        "SystemIcons::regionBackdrop(regionKey)" in source and
        "regionH = 270" in source and
        "Color(5, 14, 30, 96)" in source,
        "selected-game Product Home card must support a visible region-scene backdrop with a readability scrim")
region_loader = system_icons[system_icons.index("const IconImage& SystemIcons::regionBackdrop"):
                             system_icons.index("const IconImage& SystemIcons::trainerPortrait")]
require('"brilliant_diamond_switch"' in game_identity and
        '"shining_pearl_switch"' in game_identity and
        'return "sinnoh";' in game_identity and
        '"romfs:/region_backdrops/" + key + ".png"' in region_loader and
        "makeSinnohBackdrop()" not in region_loader and
        "real region backdrop not packaged" in region_loader,
        "region heroes must use real packaged artwork only and never substitute generated Sinnoh scenery")

require("17.0f,  // Caption / secondary information" in framebuffer and
        "20.0f,  // Body / normal labels" in framebuffer and
        "28.0f,  // Heading" in framebuffer and
        "32.0f,  // Title" in framebuffer and
        "style == TextStyle::Body" in framebuffer and "x + 0.30f" in framebuffer,
        "handheld typography must use the readable 17/20/28/32 Nunito scale with a medium body-weight pass")

require("SystemIcons::trainerPortrait" in source and "portrait.assetKey" in source,
        "trainer presentation must load optional real portrait artwork when packaged")
require("branch-romfs-overrides" in product_workflow and
        'test -s "romfs/trainer_portraits/$f.png"' in product_workflow and
        "'romfs/trainer_portraits/**'" in product_workflow and
        "'romfs/region_backdrops/**'" in product_workflow and
        "exact PNG bytes are missing from final NRO RomFS" in product_workflow and
        "region backdrop runtime path is missing from final NRO" in product_workflow,
        "Product UI native packaging must preserve and verify trainer/region presentation payload in the final NRO")
require("branch-romfs-overrides" in gen4_workflow and
        'test -s "application/romfs/trainer_portraits/$f.png"' in gen4_workflow and
        "'romfs/trainer_portraits/**'" in gen4_workflow and
        "'romfs/region_backdrops/**'" in gen4_workflow and
        "exact PNG bytes are missing from final NRO RomFS" in gen4_workflow and
        "region backdrop runtime path is missing from final NRO" in gen4_workflow,
        "Gen IV candidate packaging must preserve and verify trainer/region presentation payload in the final NRO")
require('"romfs:/trainer_portraits/" + key + ".png"' in system_icons and
        "trainerPortraitFromAtlas" not in system_icons and
        "real trainer portrait missing or invalid" in system_icons,
        "trainer portraits must load individual real PNG assets and never depend on the corrupted atlas")
for portrait_key in ("red", "gold", "kris", "brendan", "may",
                     "lucas", "dawn", "ethan", "lyra"):
    require(f'"{portrait_key}.png"' in product_art_fetch,
            f"hardware product-art fetch must include trainer asset: {portrait_key}")
require('"leaf.png"' not in product_art_fetch,
        "do not package or pretend a Leaf portrait until an actual Leaf asset is supplied")
for region_key in ("hoenn.png", "sinnoh.png", "kalos.png"):
    require(f'"{region_key}"' in product_art_fetch,
            f"hardware product-art fetch must include real region artwork: {region_key}")
require("truncated PNG chunk" in product_art_fetch and "PNG has no complete IEND" in product_art_fetch,
        "product-art preflight must reject structurally truncated PNGs before packaging")
require('"red"' in source and '"dawn"' in source and '"lucas"' in source and
        '"ethan"' in source and '"lyra"' in source,
        "trainer portrait mapping must reserve canonical Gen I-IV asset keys")
require("productSourceLabel" in source and '"System save"' in source and '"Linked save"' in source and
        '"Choose save"' in source and '"Needs attention"' in source,
        "Product Home must translate raw source-state diagnostics into consumer-facing labels")
require('Truthful fallback: a Poké Ball identity badge' in source and
        'fake "character portrait"' in source,
        "missing trainer art must fall back to a truthful Poké Ball identity, never fake human art")
require('"Pokémon storage"' in source and '"Transfer & lineage"' in source and
        '"Species & forms"' in source and '"Research & collection"' in source,
        "compact Vault and Pokédex cards must retain distinct Pokémon-specific identities")
require("padGetButtonsDown(&pad)" in source and
        "padGetButtons(&pad) & HidNpadButton_A" not in source,
        "action buttons must remain edge-triggered while held input is reserved for navigation repeat")
require('"Multiple saves exist. Open Source / Game File once to choose the exact save."' in source and
        '"That save changed while opening. Nothing was opened."' in source,
        "ambiguous or changed legacy sources must fail closed instead of guessing")
require("preferGameSourceAndSave(" in source and "preferredLegacySourceIndex(" in source,
        "choosing one of multiple classic saves must persist and reuse the exact source identity")
require('users.front().name = "Game Sources";' not in source and
        '"Re-link it from Game Sources."' not in source and
        'Re-link it from Source / Game File.' in source and
        '"Game Sources  /  v"' not in source and
        '"Manage Game Sources"' not in source and '"Pokémon Games"' in source,
        "reachable Games copy must use the consumer-facing Pokémon Games presentation")

select_start = source.index("void SaveSelectScreen::selectCurrentTitle()")
select_end = source.index("void SaveSelectScreen::selectCurrentTitleForItems()", select_start)
stable_open = source[select_start:select_end]
require("const TitleEntry selected =" in stable_open and
        "const std::string selectedGameId = selected.gameId;" in stable_open,
        "game open must snapshot stable game identity before refreshing source discovery")
require("candidate.gameId == selectedGameId" in stable_open and
        "candidate.sourceIdentity == shown.sourceIdentity" in stable_open,
        "legacy open must re-map by gameId + source identity after refresh, never stale list index")
require("profileIdentity" in header and "sourceIdentity" in header and "sourceKind" in header and
        "titleId" in header and "currentSourceIdentity()" in header,
        "Product Home navigation state must retain stable profile/game/source identity rather than numeric indices alone")
constructor_start = source.index("SaveSelectScreen::SaveSelectScreen(")
constructor_end = source.index("void SaveSelectScreen::loadLegacySources", constructor_start)
constructor_block = source[constructor_start:constructor_end]
require("resumeState->profileIdentity" in constructor_block and
        "title.gameId != resumeState->gameId" in constructor_block and
        "assigned.sourceIdentity == resumeState->sourceIdentity" in constructor_block and
        "if (!restoredTitle)" in constructor_block,
        "Product Home resume must restore exact stable identity first and fall back to an index only when it disappeared")
require("openAssignedSource" in stable_open and "discoverGen4Candidates();" not in stable_open,
        "remembered Gen IV saves must open the exact assigned game directly instead of re-entering the candidate grid")

# The old grid used to call refreshHubPreview() unconditionally every frame, which could mount/read
# Switch saves or reopen Gen IV files dozens of times per second and made A-open look hung.
update_start = source.index("void SaveSelectScreen::update")
classic_runtime = source.index("if (classicGamesActive)", update_start)
root_runtime = source.index("// Games is now the app root", classic_runtime)
classic_runtime_block = source[classic_runtime:root_runtime]
require("if (userIndex == beforeUser && titleIndex != beforeTitle)" in classic_runtime_block,
        "Classic grid preview work must run only when the selected game actually changes")
require("selectCurrentTitle();\n                    // Do not mount/reparse" in classic_runtime_block and
        "return;" in classic_runtime_block,
        "A-open must hand off immediately instead of doing another heavy preview refresh")
require("scrollClassicSelectionIntoView();\n                    refreshHubPreview();" in classic_runtime_block,
        "Classic grid may refresh preview only inside the explicit selection-change guard")

# Hardware-regression contracts added after the 941ac9d7 failure report.
draw_start = source.index("void SaveSelectScreen::draw(PKSEFramebuffer& fb)")
draw_block = source[draw_start:]
require("Blocking overlays are shared by Product Home and the classic artwork browser." in draw_block and
        "if (overlay == Overlay::GameFilePicker)" in draw_block,
        "classic Games launch/source overlays must render above the classic surface instead of becoming invisible input blockers")
require("stbi_failure_reason()" in system_icons and
        "real trainer portrait missing or invalid" in system_icons,
        "trainer portrait runtime decode failures must leave hardware-useful diagnostics")
require("gameLaunchBindingFamilyPrefix" in (ROOT / "include/UI/GameLaunchModel.h").read_text(encoding="utf-8"),
        "launch model must expose the stable profile+game binding family used for compatibility lookup")
