from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT / "src/main.cpp").read_text(encoding="utf-8")
launcher = (ROOT / "src/UI/GameLauncher.cpp").read_text(encoding="utf-8")
home = (ROOT / "src/UI/SaveSelectScreen.cpp").read_text(encoding="utf-8")


def require(cond: bool, message: str) -> None:
    if not cond:
        raise SystemExit(message)


require("Testing sprite loading..." not in main and "getSprite(25, false)" not in main,
        "normal startup must not decode Pikachu only as a sprite-loader self-test")
require("installedApplicationNames()" in launcher and
        "static const std::vector<InstalledApplicationName> applications" in launcher,
        "installed HOME application metadata must be cached once per process")
lookup_start = launcher.index("uint64_t installedGameForwarderTitle")
lookup_end = launcher.index("#endif", lookup_start)
lookup = launcher[lookup_start:lookup_end]
require("nsListApplicationRecord" not in lookup and "nsGetApplicationControlData" not in lookup,
        "per-game forwarder lookup must not re-enumerate HOME applications")
drawer_start = home.index("if (overlay == Overlay::GamesDrawer)")
drawer_end = home.index("if (overlay == Overlay::ProfilePicker)", drawer_start)
drawer = home[drawer_start:drawer_end]
x_start = drawer.index("if (kDown & HidNpadButton_X)")
x_end = drawer.index("if (count > 0)", x_start)
x_block = drawer[x_start:x_end]
require("refreshHubSelectionFromCache();" in x_block and "refreshHubPreview();" not in x_block,
        "Quick Games X / Save-Source must stay off full preview I/O")
require("openSaveSourceForCurrentTitle(true, false);" in x_block,
        "Quick Games X must keep the same Save/Source action")
print("product performance source contract passed")
