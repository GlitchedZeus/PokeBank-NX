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
require('{"Games", "Banks", "Backups", "Search", "More"}' in source,
        "persistent dock must reserve Settings for the header and More for future features")
require("hubDockFocused" in source and "hubFeatureIndex" in source and "activateHubDock" in source,
        "approved home destinations must be controller-focusable, not decorative")
require("HidNpadButton_L" in source and "HidNpadButton_R" in source,
        "L/R must switch the selected game")
require('"Pokédex"' in source and '"Progress tracking  •  Coming Soon"' in source,
        "selected-game card must use the compact honest Pokédex Coming Soon presentation")
require('"Trainer"' in source,
        "selected-game card must expose trainer information")
require("PROFILE_AVATAR" in source and "SystemIcons::userIcon" in source,
        "product header must expose the current profile identity")

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
require("Colors::FocusBorder, 3" in source,
        "selected game OPEN action must carry the approved focus treatment")

# Pokémon identity and future navigation.
require('"UI/SpriteManager.h"' in source and "SpriteManager::getIconSprite" in source,
        "Product Home party must use the existing Pokémon sprite pipeline")
require("containSprite" in source,
        "party sprites must preserve aspect ratio")
require("MainMenuDestination::More" in source and "Dest::More" in ui_manager,
        "More must be a real routed product destination")
require('"Games   Open the selected game\'s PKSE-style workspace"' in source,
        "Help must explain that Games enters the existing game workspace")
require("helpReturnOverlay = Overlay::GameWorkspace;" in source and
        '"Game Workspace Controls"' in source,
        "Minus from Game Workspace must open contextual Help and return there cleanly")
require('"B   Back to Product Home"' in source and
        '"ZR   Launch the selected game"' in source,
        "workspace Help must explain its contextual Back and Launch controls")
require("Overlay::GameWorkspace" in source and "gameWorkspaceIndex" in header,
        "Games dock must open a real selected-game workspace instead of duplicating OPEN")
require('static constexpr const char* labels[8]' in source,
        "game workspace must expose the approved practical destination grid")
for label in ("Overview", "Party", "Boxes", "Pokédex", "Trainer",
              "Editor / Create", "Backups", "Source / Game File"):
    require(f'"{label}"' in source,
            f"game workspace is missing {label}")
activate_start = source.index("void SaveSelectScreen::activateHubDock()")
activate_end = source.index("void SaveSelectScreen::activateGameWorkspace()", activate_start)
dock_activation = source[activate_start:activate_end]
require("overlay = Overlay::GameWorkspace;" in dock_activation,
        "Games dock must enter Game Workspace")
require("selectCurrentTitle();" not in dock_activation.split("if (hubDockIndex == 0)", 1)[1].split("else if (hubDockIndex == 1)", 1)[0],
        "Games dock must not duplicate the fast OPEN route")
require("case 1: // Party" in source and "case 2: // Boxes" in source and
        "case 5: // Editor / Create" in source,
        "workspace implemented destinations must route through the validated existing game flow")
require('"-: Help"' in source and '"+: Settings' in source,
        "Product Home footer must expose Help and Settings shortcuts")
require("headerSettingsFocused" in source,
        "top-right Settings gear must participate in controller focus")
require('"Mystery Gifts"' in shell_source and '"Clone Lineage"' in shell_source,
        "More screen must reserve truthful future-feature modules")
