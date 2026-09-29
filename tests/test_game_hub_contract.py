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

require("AppShellScreen shell" not in ui_manager,
        "the old 8-card developer dashboard must never sit in front of the game hub")
require("handleSaveSelection();" in ui_manager,
        "PokeBank NX must boot directly into the profile/game hub")
require('"PokeBank NX  /  v"' in source,
        "the root hub must present itself as PokeBank NX, not a Games sub-screen")
require('{"B", "Home"}' not in source and '{"B", "Exit"}' in source,
        "root hub must not navigate back to the removed dashboard")

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
