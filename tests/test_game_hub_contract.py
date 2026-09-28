from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "src/UI/SaveSelectScreen.cpp").read_text(encoding="utf-8")
header = (ROOT / "include/UI/SaveSelectScreen.h").read_text(encoding="utf-8")

def require(cond: bool, message: str) -> None:
    if not cond:
        raise SystemExit(message)

require('"ACTIVE PARTY"' in source, "game hub must expose the active party strip")
require('"A", "Open / Edit"' in source, "game hub must expose an Open/Edit primary action")
require('"ZR", "Launch"' in source, "game hub must expose the launch shortcut")
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
