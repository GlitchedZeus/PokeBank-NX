from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "src/UI/SaveSelectScreen.cpp").read_text(encoding="utf-8")
header = (ROOT / "include/UI/SaveSelectScreen.h").read_text(encoding="utf-8")
ui_manager = (ROOT / "src/UI/UI.cpp").read_text(encoding="utf-8")

def require(cond: bool, message: str) -> None:
    if not cond:
        raise SystemExit(message)

require('"ACTIVE PARTY"' in source, "game hub must expose the active party strip")
require('"A", "Open / Edit"' in source, "game hub must expose an Open/Edit primary action")
require('{"ZR", gameLaunchActionLabel(launchDescriptor.state)}' in source,
        "game hub must expose the dynamic Launch / Link Game File shortcut")
require("requestGameLaunch" in source, "launch shortcut must route through the shared launcher")
require('fsdevMountSaveData("pbpreview"' in source,
        "native party preview must use an explicit read-only preview mount boundary")
require('fsdevUnmountDevice("pbpreview")' in source,
        "native party preview must always leave the preview mount")
require("fsdevCommitDevice" not in source,
        "game hub preview must never commit a live source save")
require("restoreBackupToTitle" not in source,
        "game hub preview must never restore/inject a save")
require("PartyPreviewSlot" in header, "game hub party preview model must be explicit")

require("AppShellScreen shell" in ui_manager,
        "the professional main menu must be the app root")
require("handleSaveSelection()" in ui_manager,
        "Games destination must open the existing profile/game hub")
require('"Games  /  v"' in source,
        "the profile/game hub must present itself as the Games destination")
require('{"B", "Main Menu"}' in source,
        "B from Games must return to the professional main menu")
require('"Exit PokeBank NX"' in source,
        "the game hub must retain an explicit full-app exit action")

require("GameFilePicker" in header and "Overlay::GameFilePicker" in source,
        "game hub must provide an in-app game-file browser")
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

require('"QUICK ACCESS"' in source,
        "game hub must expose the compact quick-access dock")
require('{"Storage", "Banks", "Backups", "Trade", "Settings"}' in source,
        "game hub dock must present Storage, Banks, Backups, future Trade and Settings")
require("hubDockFocused" in source and "activateHubDock" in source,
        "game hub quick-access dock must be controller-focusable, not decorative")
require("MainMenuDestination::Banks" in source and "MainMenuDestination::Settings" in source,
        "Banks and Settings dock actions must route back through the root product shell")
require('"Trade is not implemented yet."' in source,
        "future Trade must remain explicitly unavailable")
require("shell.openSection(PokeBank::UIModel::AppShellSection::Banks)" in ui_manager,
        "Banks quick access must land on the root Banks destination")
require("shell.openSection(PokeBank::UIModel::AppShellSection::Settings)" in ui_manager,
        "Settings quick access must land on the real root Settings destination")
