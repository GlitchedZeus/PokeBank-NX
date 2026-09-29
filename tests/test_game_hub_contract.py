from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "src/UI/SaveSelectScreen.cpp").read_text(encoding="utf-8")
header = (ROOT / "include/UI/SaveSelectScreen.h").read_text(encoding="utf-8")
ui_manager = (ROOT / "src/UI/UI.cpp").read_text(encoding="utf-8")
shell_source = (ROOT / "src/UI/AppShellScreen.cpp").read_text(encoding="utf-8")

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
require('{"Games", "Banks", "Backups", "Search", "More", "Items", "Settings"}' in source,
        "persistent dock must expose approved Games/Banks/Backups/Search/More plus Items and Settings quick controls")
require("hubDockFocused" in source and "hubFeatureIndex" in source and "activateHubDock" in source,
        "approved home destinations must be controller-focusable, not decorative")
require("HidNpadButton_L" in source and "HidNpadButton_R" in source,
        "L/R must switch the selected game")
require('"Pokédex Progress"' in source and
        "previewDexSeen" in source and "previewDexCaught" in source and
        '"   •   Owned "' in source,
        "selected-game card must show real parsed Pokédex Seen/Owned progress when supported")
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
        "dock destinations must route through the secondary shell")

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
require('"Games   Open the familiar Game Sources grid"' in source,
        "Product Home Help must explain that Games opens Classic Game Sources")
require("classicGamesActive" in source and "drawClassicGameSources" in source,
        "Games must retain the familiar Classic Game Sources interface")
require('"Game Sources Controls"' in source and
        '"B   Back to Product Home"' in source,
        "Classic Game Sources must expose contextual controls and return Home")
activate_start = source.index("void SaveSelectScreen::activateHubDock()")
activate_end = source.index("void SaveSelectScreen::activateGameWorkspace()", activate_start)
dock_activation = source[activate_start:activate_end]
require("classicGamesActive = true;" in dock_activation,
        "Games dock must enter Classic Game Sources")
require("selectCurrentTitle();" not in dock_activation.split("if (hubDockIndex == 0)", 1)[1].split("else if (hubDockIndex == 1)", 1)[0],
        "Games dock must not duplicate the fast OPEN route")
require('"D-pad/Stick","Choose Game"' in source and
        '"L/R","Switch User"' in source and
        '"ZR","Launch"' in source,
        "Classic Game Sources must retain controller navigation and launch controls")
require("title.trainerName.empty() ? title.sourceLabel : title.trainerName" in source and
        "title.dexCaught" in source and "title.dexTotal" in source,
        "Classic Game Sources cards must show real trainer/dex metadata when available")
require("-: Help" in source and "+: Settings" in source,
        "Product Home footer must expose Help and Settings shortcuts")
update_start = source.index("void SaveSelectScreen::update")
classic_input = source.index("if (classicGamesActive)", update_start)
root_exit = source.index("// Games is now the app root", update_start)
require(classic_input < root_exit,
        "Classic Games controls must run before Product Home B-exit handling")
require('{"-","Help"}' in source and '"-   Help / Controls"' in source,
        "Classic Games must visibly expose contextual Minus Help")
require("GameLaunchState::LauncherOnly" in launcher and
        "Direct selected-ROM handoff is not supported by this DraStic build." in launcher,
        "DraStic launch must be truthful launcher-only until its frontend supports ROM argv")
require("descriptor.state != GameLaunchState::LauncherOnly" in launcher,
        "launcher-only emulators must never receive a falsely linked ROM argument")
require("GameLaunchProviderKind::MelonDS" in launcher,
        "melonDS provider-aware direct content launch support must remain present")
require("OpenIntent::Items" in source and "hubDockIndex == 2" in source,
        "Backpack/Items must replace the old Backups root slot")
require("selectCurrentTitleForItems" in source and
        "sameValidatedSnapshot" in source and
        "openAssignedSource" in source,
        "Backpack must revalidate the already-selected source without opening a chooser")
require("const int gearCx = 1239" not in source and
        '{"Games", "Banks", "Items", "Search", "More", "Settings"}' in source,
        "Product Home must render exactly one Settings gear")
require('kSettingsCategories' in shell_source and
        '"User", "Look", "System", "Data", "Update", "Developer", "Info"' in shell_source,
        "Settings must use the approved two-pane category model")
require("settingsCategoryFocused" in shell_source and "settingsOptionCount" in shell_source,
        "Settings must keep independent category/option focus")
require('"Left/Right", "Pane"' in shell_source,
        "Settings footer must explain two-pane navigation")
require('"Source Save Protection", "LOCKED"' in shell_source and
        '"Update Support", "Coming Soon"' in shell_source,
        "Settings must expose truthful Data and Update states without fake backends")
require('"Mystery Gifts"' in shell_source and '"Clone Lineage"' in shell_source,
        "More screen must reserve truthful future-feature modules")

require("handleItemsQuickOpen" in ui_manager and "backupSaveData" in ui_manager,
        "Switch Backpack quick-open must create the normal protected backup before Items")
require("if (!shell.hasOverlay()) break;" in ui_manager,
        "closing a secondary shell must not draw the retired shell for one stale frame")
require("slotW - 12, 66" in source and "slotY + 96" in source,
        "Product Home party sprites and level text must use the enlarged readable layout")

require("SystemIcons::trainerPortrait" in source and "portrait.assetKey" in source,
        "trainer presentation must load optional real portrait artwork when packaged")
require('"red"' in source and '"dawn"' in source and '"lucas"' in source and
        '"ethan"' in source and '"lyra"' in source,
        "trainer portrait mapping must reserve canonical Gen I-IV asset keys")
require("productSourceLabel" in source and '"System save"' in source and '"Linked save"' in source,
        "Product Home must translate raw source-state diagnostics into consumer-facing labels")
require('"Pokémon storage, transfer & lineage"' in source and
        '"Research species, forms & collection"' in source,
        "Vault and Pokédex cards must carry distinct Pokémon-specific product identities")
