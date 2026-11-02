from pathlib import Path


def replace_once(path: str, old: str, new: str, label: str) -> None:
    p = Path(path)
    text = p.read_text(encoding="utf-8")
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected one match, found {count}")
    p.write_text(text.replace(old, new, 1), encoding="utf-8")


# Startup: remove the always-on Pikachu decode that existed only as a sprite-loader self-test.
replace_once(
    "src/main.cpp",
    '''    Utils::logInfoToFile("Testing sprite loading...");
    UI::Sprite* testSprite = UI::SpriteManager::getSprite(25, false); // Pikachu
    if (testSprite && testSprite->data) {
        logInfoToFile(("SUCCESS: Test sprite loaded! (" +
            std::to_string(testSprite->width) + "x" +
            std::to_string(testSprite->height) + ")").c_str());
    } else {
        Utils::logInfoToFile("WARNING: Test sprite failed to load - sprites may not be available");
    }

''',
    "",
    "startup sprite self-test",
)

# HOME forwarders: enumerate installed application metadata once per process, then match games
# against the in-memory cache. Exact-game matching and ambiguity handling remain unchanged.
old_lookup = '''#ifdef __SWITCH__
uint64_t installedGameForwarderTitle(std::string_view gameId) {
    static std::map<std::string, uint64_t> cache;
    const std::string key(gameId);
    if (const auto found = cache.find(key); found != cache.end()) return found->second;

    if (!PokeVault::Games::findGame(gameId)) { cache[key] = 0; return 0; }

    uint64_t unique = 0;
    int matches = 0;
    s32 offset = 0;
    while (offset < 2048) {
        NsApplicationRecord records[32]{};
        s32 count = 0;
        if (R_FAILED(nsListApplicationRecord(records, 32, offset, &count)) || count <= 0) break;
        for (s32 i = 0; i < count; ++i) {
            auto* control = static_cast<NsApplicationControlData*>(
                std::malloc(sizeof(NsApplicationControlData)));
            if (!control) continue;
            u64 outSize = 0;
            const Result rc = nsGetApplicationControlData(
                NsApplicationControlSource_Storage, records[i].application_id,
                control, sizeof(NsApplicationControlData), &outSize);
            if (R_SUCCEEDED(rc) && outSize != 0) {
                NacpLanguageEntry* language = nullptr;
                if (R_SUCCEEDED(nacpGetLanguageEntry(&control->nacp, &language)) && language) {
                    const std::string name = compactGameTitle(language->name);
                    if (installedForwarderNameMatches(gameId, name)) {
                        unique = records[i].application_id;
                        ++matches;
                    }
                }
            }
            std::free(control);
            if (matches > 1) break;
        }
        if (matches > 1 || count < 32) break;
        offset += count;
    }
    if (matches != 1) unique = 0;
    cache[key] = unique;
    return unique;
}
#endif
'''
new_lookup = '''#ifdef __SWITCH__
struct InstalledApplicationName {
    uint64_t titleId = 0;
    std::string normalizedName;
};

const std::vector<InstalledApplicationName>& installedApplicationNames() {
    // HOME application metadata is console-global for the lifetime of this process. Build it
    // once, then let every game identity match in memory instead of re-querying the same control
    // records for FireRed, Emerald, Platinum and every other launch candidate.
    static const std::vector<InstalledApplicationName> applications = [] {
        std::vector<InstalledApplicationName> found;
        s32 offset = 0;
        while (offset < 2048) {
            NsApplicationRecord records[32]{};
            s32 count = 0;
            if (R_FAILED(nsListApplicationRecord(records, 32, offset, &count)) || count <= 0)
                break;
            for (s32 i = 0; i < count; ++i) {
                auto* control = static_cast<NsApplicationControlData*>(
                    std::malloc(sizeof(NsApplicationControlData)));
                if (!control) continue;
                u64 outSize = 0;
                const Result rc = nsGetApplicationControlData(
                    NsApplicationControlSource_Storage, records[i].application_id,
                    control, sizeof(NsApplicationControlData), &outSize);
                if (R_SUCCEEDED(rc) && outSize != 0) {
                    NacpLanguageEntry* language = nullptr;
                    if (R_SUCCEEDED(nacpGetLanguageEntry(&control->nacp, &language)) && language) {
                        const std::string name = compactGameTitle(language->name);
                        if (!name.empty()) found.push_back({records[i].application_id, name});
                    }
                }
                std::free(control);
            }
            if (count < 32) break;
            offset += count;
        }
        return found;
    }();
    return applications;
}

uint64_t installedGameForwarderTitle(std::string_view gameId) {
    static std::map<std::string, uint64_t> cache;
    const std::string key(gameId);
    if (const auto found = cache.find(key); found != cache.end()) return found->second;

    if (!PokeVault::Games::findGame(gameId)) { cache[key] = 0; return 0; }

    uint64_t unique = 0;
    int matches = 0;
    for (const auto& application : installedApplicationNames()) {
        if (!installedForwarderNameMatches(gameId, application.normalizedName)) continue;
        unique = application.titleId;
        if (++matches > 1) break;
    }
    if (matches != 1) unique = 0;
    cache[key] = unique;
    return unique;
}
#endif
'''
replace_once("src/UI/GameLauncher.cpp", old_lookup, new_lookup, "HOME application cache")

# Quick Games X is Save/Source setup. Keep exactly the same destination, but do not mount/parse
# the selected save or resolve its launch target first.
replace_once(
    "src/UI/SaveSelectScreen.cpp",
    '''                    titleIndex = gamesDrawerIndex;
                    scrollSelectionIntoView();
                    refreshHubPreview();
                    openSaveSourceForCurrentTitle(true, false);
''',
    '''                    titleIndex = gamesDrawerIndex;
                    scrollSelectionIntoView();
                    refreshHubSelectionFromCache();
                    openSaveSourceForCurrentTitle(true, false);
''',
    "Quick Games X cached selection",
)

Path("tests/test_product_performance_contract.py").write_text('''from pathlib import Path

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
''', encoding="utf-8")

print("Applied Product Home performance patch without changing product behavior.")
