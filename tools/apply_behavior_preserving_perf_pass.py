#!/usr/bin/env python3
from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    p = Path(path)
    text = p.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected exactly one match, found {count}")
    p.write_text(text.replace(old, new, 1))


# 1) Startup: remove the product-irrelevant Pikachu decode probe. Sprite loading is exercised by
# normal UI use and CI; decoding a PNG before the first frame only adds startup I/O/CPU work.
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
    '''    // Do not decode a sacrificial test sprite on every boot. Real screens load sprites through
    // the same cached SpriteManager path, and CI covers the asset/runtime contracts. Keeping startup
    // free of this probe preserves identical product behavior while removing needless pre-frame I/O.

''')

# 2) Installed HOME forwarders: enumerate application control data once per PokeBank session instead
# of re-reading the entire HOME application database for every distinct Pokemon game identity.
gl = Path("src/UI/GameLauncher.cpp")
text = gl.read_text()
start_marker = "#ifdef __SWITCH__\nuint64_t installedGameForwarderTitle(std::string_view gameId) {"
start = text.find(start_marker)
if start < 0:
    raise SystemExit("GameLauncher.cpp: installedGameForwarderTitle start not found")
end = text.find("#endif", start)
if end < 0:
    raise SystemExit("GameLauncher.cpp: installedGameForwarderTitle #endif not found")
end += len("#endif")
old = text[start:end]
new = r'''#ifdef __SWITCH__
struct InstalledHomeApplication {
    uint64_t titleId = 0;
    std::string normalizedName;
};

const std::vector<InstalledHomeApplication>& installedHomeApplications() {
    // nsGetApplicationControlData() is comparatively expensive. HOME's installed-application set
    // cannot change while PokeBank is the foreground application, so build one immutable snapshot
    // per process and reuse it for every exact-game forwarder lookup.
    static const std::vector<InstalledHomeApplication> applications = [] {
        std::vector<InstalledHomeApplication> result;
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
                        InstalledHomeApplication app;
                        app.titleId = records[i].application_id;
                        app.normalizedName = compactGameTitle(language->name);
                        result.push_back(std::move(app));
                    }
                }
                std::free(control);
            }
            if (count < 32) break;
            offset += count;
        }
        return result;
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
    for (const auto& app : installedHomeApplications()) {
        if (!installedForwarderNameMatches(gameId, app.normalizedName)) continue;
        unique = app.titleId;
        ++matches;
        if (matches > 1) break;
    }
    if (matches != 1) unique = 0;
    cache[key] = unique;
    return unique;
}
#endif'''
gl.write_text(text[:start] + new + text[end:])

# 3) Switch save enumeration: list the console's save-data index once, then distribute matching
# Pokemon saves to profiles. The old code reread the identical save index once per user.
hdr = Path("include/UI/SaveSelectScreen.h")
htext = hdr.read_text()
old_decl = '''        void loadTitlesForUser(UserEntry& user);
        static bool scanSaveSpace(UserEntry& user, int spaceId, int& scanned, int& forUser);
'''
new_decl = '''        void loadTitlesForUsers();
        static bool scanSaveSpaceForUsers(std::vector<UserEntry>& users, int spaceId,
                                          int& scanned, std::vector<int>& forUsers);
'''
if htext.count(old_decl) != 1:
    raise SystemExit("SaveSelectScreen.h: save scan declarations changed unexpectedly")
hdr.write_text(htext.replace(old_decl, new_decl, 1))

src = Path("src/UI/SaveSelectScreen.cpp")
stext = src.read_text()
block_start = stext.find("    void SaveSelectScreen::loadUsers() {")
block_end = stext.find("    const SaveSelectScreen::UserEntry* SaveSelectScreen::currentUser() const {", block_start)
if block_start < 0 or block_end < 0:
    raise SystemExit("SaveSelectScreen.cpp: loadUsers/save-scan block not found")
replacement = r'''    void SaveSelectScreen::loadUsers() {
        users.clear();

        AccountUid userIds[ACC_USER_LIST_SIZE];
        s32 userCount = 0;
        Result rc = accountListAllUsers(userIds, ACC_USER_LIST_SIZE, &userCount);
        if (R_FAILED(rc)) {
            logErrorToFile("SaveSelect: failed to list users");
            userCount = 0;
        }

        for (s32 i = 0; i < userCount; i++) {
            UserEntry entry;
            entry.uid = userIds[i];

            AccountProfile profile;
            AccountProfileBase base;
            if (R_SUCCEEDED(accountGetProfile(&profile, userIds[i]))) {
                if (R_SUCCEEDED(accountProfileGet(&profile, NULL, &base))) {
                    entry.name = std::string(base.nickname);
                } else {
                    entry.name = "Unknown User";
                }
                accountProfileClose(&profile);
            } else {
                entry.name = "Unknown User";
            }
            users.push_back(std::move(entry));
        }

        if (users.empty()) {
            UserEntry def;
            def.name = "No users found";
            memset(&def.uid, 0, sizeof(AccountUid));
            users.push_back(std::move(def));
            return;
        }

        // The system save-data index is global. Read it once, then distribute matching Pokemon
        // saves to their owning profiles instead of reopening/re-reading the same index per user.
        loadTitlesForUsers();
    }

    /**
     * List Pokemon saves for all loaded users by enumerating SAVE DATA once.
     *
     * Cartridge/archived saves remain visible because this still enumerates save data rather than
     * installed applications. This is behavior-identical to the previous per-user scans; it only
     * removes duplicate traversal of the same system index when more than one Switch profile exists.
     */
    bool SaveSelectScreen::scanSaveSpaceForUsers(std::vector<UserEntry>& users, int spaceId,
                                                  int& scanned, std::vector<int>& forUsers) {
        FsSaveDataInfoReader reader;
        Result rc = fsOpenSaveDataInfoReader(&reader, static_cast<FsSaveDataSpaceId>(spaceId));
        if (R_FAILED(rc)) {
            char m[112];
            snprintf(m, sizeof(m), "SaveSelect: cannot read save space %d (rc=0x%08X)",
                     spaceId, (unsigned)rc);
            logInfoToFile(m);
            return false;
        }

        FsSaveDataInfo info[24];
        s64 readCount = 0;
        while (R_SUCCEEDED(fsSaveDataInfoReaderRead(&reader, info, 24, &readCount)) && readCount > 0) {
            for (s64 i = 0; i < readCount; i++) {
                ++scanned;
                if (info[i].save_data_type != FsSaveDataType_Account) continue;

                size_t owner = users.size();
                for (size_t u = 0; u < users.size(); ++u) {
                    if (memcmp(&info[i].uid, &users[u].uid, sizeof(AccountUid)) == 0) {
                        owner = u;
                        break;
                    }
                }
                if (owner >= users.size()) continue;
                if (owner < forUsers.size()) ++forUsers[owner];

                const u64 titleId = info[i].application_id;
                GameVersion gv = getGameVersion(titleId);
                if (gv == GameVersion::Invalid) continue;
                const auto* identity = PokeVault::Games::findSwitchGame(titleId);
                if (!identity) continue;

                UserEntry& user = users[owner];
                bool dup = false;
                for (const auto& t : user.titles) {
                    if (t.titleId == titleId) { dup = true; break; }
                }
                if (dup) continue;

                char m[128];
                snprintf(m, sizeof(m), "SaveSelect: %s (%016llX) -> listed",
                         getGameVersionName(gv).c_str(), (unsigned long long)titleId);
                logInfoToFile(m);

                TitleEntry t;
                t.titleId = titleId;
                t.label = getGameVersionName(gv);
                t.name = "Pokemon " + t.label;
                t.gameId = std::string(identity->id);
                t.platformLabel = std::string(PokeVault::Games::platformName(identity->platform));
                user.titles.push_back(std::move(t));
            }
        }
        fsSaveDataInfoReaderClose(&reader);
        return true;
    }

    void SaveSelectScreen::loadTitlesForUsers() {
        int scanned = 0;
        std::vector<int> forUsers(users.size(), 0);

        if (!scanSaveSpaceForUsers(users, FsSaveDataSpaceId_All, scanned, forUsers)) {
            scanSaveSpaceForUsers(users, FsSaveDataSpaceId_User, scanned, forUsers);
            scanSaveSpaceForUsers(users, FsSaveDataSpaceId_SdUser, scanned, forUsers);
        }

        for (size_t i = 0; i < users.size(); ++i) {
            char summary[192];
            snprintf(summary, sizeof(summary),
                     "SaveSelect: %d save entries on console, %d for %s, %d Pokemon titles listed",
                     scanned, i < forUsers.size() ? forUsers[i] : 0,
                     users[i].name.c_str(), static_cast<int>(users[i].titles.size()));
            logInfoToFile(summary);
        }
    }

'''
src.write_text(stext[:block_start] + replacement + stext[block_end:])

# 4) Quick Games X is a Save/Source action. It does not need to mount/parse a save or enumerate HOME
# launch targets first. Keep selected-card metadata coherent using the no-I/O cache refresh.
text = src.read_text()
old_quick_x = '''                    titleIndex = gamesDrawerIndex;
                    scrollSelectionIntoView();
                    refreshHubPreview();
                    openSaveSourceForCurrentTitle(true, false);
'''
new_quick_x = '''                    titleIndex = gamesDrawerIndex;
                    scrollSelectionIntoView();
                    refreshHubSelectionFromCache();
                    openSaveSourceForCurrentTitle(true, false);
'''
if text.count(old_quick_x) != 1:
    raise SystemExit(f"SaveSelectScreen.cpp: expected one Quick Games X refresh block, got {text.count(old_quick_x)}")
src.write_text(text.replace(old_quick_x, new_quick_x, 1))

# 5) Lock the performance boundaries in the existing Product Home contract test.
test = Path("tests/test_game_hub_contract.py")
t = test.read_text()
append = r'''

# Behavior-preserving performance boundaries: navigation/assignment must not trigger avoidable I/O,
# and process-global system indexes should be enumerated once rather than once per game/profile.
main_source = (ROOT / "src/main.cpp").read_text()
launcher_source = (ROOT / "src/UI/GameLauncher.cpp").read_text()
header_source = (ROOT / "include/UI/SaveSelectScreen.h").read_text()
require('Testing sprite loading...' not in main_source,
        "startup must not decode a sacrificial Pikachu sprite before the first product frame")
require('installedHomeApplications()' in launcher_source and
        'for (const auto& app : installedHomeApplications())' in launcher_source,
        "installed HOME application control data must be snapshotted once per process")
require('loadTitlesForUsers();' in source and 'scanSaveSpaceForUsers' in source and
        'loadTitlesForUser(entry);' not in source,
        "Switch save-data enumeration must be shared across profiles instead of repeated per user")
quick_x_start = source.find('if (overlay == Overlay::GamesDrawer)')
quick_x_end = source.find('if (overlay == Overlay::ProfilePicker)', quick_x_start)
quick_x_block = source[quick_x_start:quick_x_end]
require('refreshHubSelectionFromCache();\n                    openSaveSourceForCurrentTitle(true, false);' in quick_x_block and
        'refreshHubPreview();\n                    openSaveSourceForCurrentTitle(true, false);' not in quick_x_block,
        "Quick Games X Save/Source must stay on the no-I/O selection path")
'''
if "Behavior-preserving performance boundaries" not in t:
    test.write_text(t + append)

print("Applied behavior-preserving Product Home/startup performance pass")
