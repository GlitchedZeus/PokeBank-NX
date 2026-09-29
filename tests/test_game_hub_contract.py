from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "src/UI/SaveSelectScreen.cpp").read_text(encoding="utf-8")
header = (ROOT / "include/UI/SaveSelectScreen.h").read_text(encoding="utf-8")
ui_manager = (ROOT / "src/UI/UI.cpp").read_text(encoding="utf-8")

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
require('{"Games", "Banks", "Backups", "Search", "Settings"}' in source,
        "persistent dock must match the approved five destinations")
require("hubDockFocused" in source and "hubFeatureIndex" in source and "activateHubDock" in source,
        "approved home destinations must be controller-focusable, not decorative")
require("HidNpadButton_L" in source and "HidNpadButton_R" in source,
        "L/R must switch the selected game")
require('"Pokédex Progress"' in source,
        "selected-game card must reserve honest Pokédex progress presentation")
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
