// Round 17 autonomous QA: Product Home (the REAL SaveSelectScreen) for all 20 Gen I-V releases on a
// production-layout SD card, with two console profiles. This is the area of the Round 15 / 16
// hardware failures (save discovery, assignment, profile isolation), driven two ways:
//
//  A. Directed pass, per release: the card is found by the real bounded discovery
//     (discoverConfiguredLegacySaves for RetroArch Gen I-III; the DraStic backup folder for Gen IV /
//     V), A opens the chooser, the save is picked, the game opens; after a "restart" (new store and
//     screen objects from disk) A opens it directly; Profile B never gets it.
//  B. Monkey: seeded random controller / touch / prompt input (profile switches included). After
//     EVERY action:
//       H1  no crash / sanitizer report;
//       H2  every emulator save on the card is byte-identical (Home never writes a source);
//       H3  whatever opened is an initialized save of exactly the selected game, assigned to the
//           CURRENT profile (never a sibling game, never another profile's save, never unassigned);
//       H4  ownership only ever changes for the profile that is current (no assignment appears for,
//           or disappears from, the other profile);
//       H5  the assignment database on disk always loads strictly (never torn, never reset).
// The store is the production path with the session recovery UIManager enables.
// Evidence class: SYNTHETIC saves (the QA matrix's), host harness. Not Switch hardware.
#include "HostUi.h"

#include "Utils/NXTypes.h"
#include "Games20.h"
#include "Legacy/LegacySourceBindings.h"
#include "Legacy/RetroArchFRLGDiscovery.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SaveSelectScreen.h"
#include "UI/TouchInput.h"
#include "Utils/PokeBankPaths.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <unistd.h>
#include <vector>

#ifndef GAMES20_FIXTURE_DIR
#define GAMES20_FIXTURE_DIR "build-host/games20"
#endif

namespace fsx = std::filesystem;
using Kind = UI::SaveSelectScreen::SelectedSourceKind;
using PokeVault::Legacy::LegacySourceBindings;

namespace {
int failures = 0;
long checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << what << '\n'; }
}

constexpr u64 kUserA[2] = {0x0123456789ABCDEFULL, 0x0FEDCBA987654321ULL};
constexpr u64 kUserB[2] = {0x1111222233334444ULL, 0x5555666677778888ULL};
const std::string kProfileA = "0123456789abcdef0fedcba987654321";
const std::string kProfileB = "11112222333344445555666677778888";
std::string profileOf(AccountUid uid) {
    char buffer[33];
    std::snprintf(buffer, sizeof(buffer), "%016llx%016llx", static_cast<unsigned long long>(uid.uid[0]),
                  static_cast<unsigned long long>(uid.uid[1]));
    return buffer;
}

// Where each release's save sits on the card, named as the emulators name them.
struct Placement { const char* id; const char* path; };
const Placement kPlacements[20] = {
    {"red_gb", "sdmc:/retroarch/cores/savefiles/Gambatte/Pokemon - Red Version (USA, Europe) (SGB Enhanced).srm"},
    {"blue_gb", "sdmc:/retroarch/cores/savefiles/Gambatte/Pokemon - Blue Version (USA, Europe) (SGB Enhanced).srm"},
    {"yellow_gb", "sdmc:/retroarch/cores/savefiles/Gambatte/Pokemon - Yellow Version (USA, Europe) (GBC Edition).srm"},
    {"gold_gbc", "sdmc:/retroarch/cores/savefiles/Gambatte/Pokemon - Gold Version (USA, Europe) (SGB Enhanced) (GB Compatible).srm"},
    {"silver_gbc", "sdmc:/retroarch/cores/savefiles/Gambatte/Pokemon - Silver Version (USA, Europe) (SGB Enhanced) (GB Compatible).srm"},
    {"crystal_gbc", "sdmc:/retroarch/cores/savefiles/Gambatte/Pokemon - Crystal Version (USA, Europe) (Rev 1).srm"},
    {"ruby_gba", "sdmc:/retroarch/cores/savefiles/mGBA/Pokemon - Ruby Version (USA, Europe) (Rev 2).srm"},
    {"sapphire_gba", "sdmc:/retroarch/cores/savefiles/mGBA/Pokemon - Sapphire Version (USA, Europe) (Rev 2).srm"},
    {"emerald_gba", "sdmc:/retroarch/cores/savefiles/mGBA/Pokemon - Emerald Version (USA, Europe).srm"},
    {"firered_gba", "sdmc:/retroarch/cores/savefiles/mGBA/Pokemon - FireRed Version (USA, Europe) (Rev 1).srm"},
    {"leafgreen_gba", "sdmc:/retroarch/cores/savefiles/mGBA/Pokemon - LeafGreen Version (USA, Europe) (Rev 1).srm"},
    {"diamond_nds", "sdmc:/switch/drastic/backup/Pokemon Diamond Version.sav"},
    {"pearl_nds", "sdmc:/switch/drastic/backup/Pokemon Pearl Version.sav"},
    {"platinum_nds", "sdmc:/switch/drastic/backup/Pokemon Platinum Version.sav"},
    {"heartgold_nds", "sdmc:/switch/drastic/backup/Pokemon HeartGold Version.sav"},
    {"soulsilver_nds", "sdmc:/switch/drastic/backup/Pokemon SoulSilver Version.sav"},
    {"black_nds", "sdmc:/switch/drastic/backup/Pokemon Black Version.sav"},
    {"white_nds", "sdmc:/switch/drastic/backup/Pokemon White Version.sav"},
    {"black2_nds", "sdmc:/switch/drastic/backup/Pokemon Black Version 2.sav"},
    {"white2_nds", "sdmc:/switch/drastic/backup/Pokemon White Version 2.sav"},
};
const Games20::GameSpec* specOf(const std::string& id) {
    for (const auto& g : Games20::kGames) if (id == g.id) return &g;
    return nullptr;
}
const Placement* placementOf(const std::string& id) {
    for (const auto& p : kPlacements) if (id == p.id) return &p;
    return nullptr;
}
Kind kindOf(const Games20::GameSpec& g) {
    return g.generation <= 3 ? Kind::RetroArchFRLG : g.generation == 4 ? Kind::Gen4AssignedFile : Kind::Gen5AssignedFile;
}

std::map<std::string, std::vector<uint8_t>> gOriginal;   // every emulator save on the card
bool sourcesIntact(const std::string& when) {
    for (const auto& [path, bytes] : gOriginal)
        if (Games20::readAll(path) != bytes) {
            check(false, "an emulator save changed on disk (" + when + "): " + path);
            return false;
        }
    ++checks;
    return true;
}

// The committed assignment rows, read straight from disk: source identity -> profile.
int hexValue(char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1; }
std::string unhex(const std::string& s) {
    std::string out;
    for (size_t i = 0; i + 1 < s.size(); i += 2) out.push_back(static_cast<char>(hexValue(s[i]) * 16 + hexValue(s[i + 1])));
    return out;
}
using Owners = std::map<std::string, std::string>;
Owners ownersOnDisk() {
    Owners out;
    std::ifstream in(PokeBank::Paths::legacySourceBindingsFile());
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto tab = line.find('\t');
        if (tab == std::string::npos) continue;
        const auto next = line.find('\t', tab + 1);
        out[unhex(line.substr(0, tab))] = unhex(line.substr(tab + 1, next == std::string::npos ? std::string::npos : next - tab - 1));
    }
    return out;
}

struct App {
    PokeVault::Legacy::FRLGDiscoveryResult catalog;
    LegacySourceBindings bindings;
    std::unique_ptr<UI::SaveSelectScreen> screen;
    UI::PKSEFramebuffer fb;
    UI::TouchInput touch;
    App() : catalog(PokeVault::Legacy::discoverConfiguredLegacySaves()), bindings(PokeBank::Paths::legacySourceBindingsFile()) {
        bindings.enableSessionRecovery();   // exactly what UIManager does
        (void)bindings.load();
    }
    void open(const UI::SaveSelectScreen::NavigationState* nav) {
        screen = std::make_unique<UI::SaveSelectScreen>(catalog, bindings, nav);
    }
    void frame(u64 down = 0, u64 held = 0) {
        // As UIManager::handleSaveSelection: once a title is selected or Home asked to leave, this
        // screen gets no further update until the caller has handled it (resumeAfterEditor / rebuild).
        if (screen->hasSelectedTitle() || screen->shouldExit()) return;
        PadState pad{down, held | down, 0};
        touch.update();
        screen->update(pad, touch);
        HostUi::texts.clear();
        HostUi::resetLayout();
        if (!screen->hasSelectedTitle() && !screen->shouldExit()) screen->draw(fb);
    }
    void press(u64 b) { frame(b); frame(); }
    void tap(u32 x, u32 y) {
        HostUi::touch = {true, x, y}; frame();
        HostUi::touch = {true, x, y}; frame();
        HostUi::touch = {false, x, y}; frame();
        frame();
    }
    bool drawn(const std::string& needle) const {
        return std::any_of(HostUi::texts.begin(), HostUi::texts.end(),
                           [&](const std::string& t) { return t.find(needle) != std::string::npos; });
    }
    std::string profile() const { return screen->navigationState().profileIdentity; }
};

std::map<std::string, long> gOpens;   // per release: opens that passed H3

// H3: what just opened is the selected game's own initialized save, assigned to this profile.
bool openIsLegitimate(App& app, const std::string& context) {
    auto& s = *app.screen;
    const std::string game = s.getSelectedGameId();
    const std::string profile = profileOf(s.getSelectedUser());
    const Kind kind = s.getSelectedSourceKind();
    const auto* spec = specOf(game);
    const std::string what = context + ": opened " + game + " for profile " + profile.substr(0, 8);
    if (!spec) { check(false, what + ": not one of the 20 Gen I-V releases on this card"); return false; }
    if (profile != kProfileA && profile != kProfileB) { check(false, what + ": no scripted profile"); return false; }
    if (std::getenv("HOME_MONKEY_TRACE"))
        std::cerr << "    open: game=" << game << " kind=" << static_cast<int>(kind) << " intent=" << static_cast<int>(s.getOpenIntent())
                  << " backup=\"" << s.getSelectedBackupSnapshot().path << "\" label=\"" << s.getSelectedBackupLabel()
                  << "\" provider=\"" << s.getSelectedBackupProvider() << "\"\n";
    if (kind != kindOf(*spec)) { check(false, what + ": wrong source kind " + std::to_string(static_cast<int>(kind))); return false; }
    if (const std::string backup = s.getSelectedBackupSnapshot().path; !backup.empty()) {
        // The Backups route: an app-owned copy (never a source save), viewed read-only. It must be
        // a copy of this exact game, and of a save that no OTHER profile owns (R17-04).
        check(backup.find("/switch/PokeBank-NX/") != std::string::npos && !gOriginal.count(backup),
              what + ": the backup is an app-owned copy, not an emulator save (" + backup + ")");
        std::string viewError;
        const auto viewed = PokeVault::Legacy::openBackupTrainer(s.getSelectedBackupSnapshot(), game, viewError);
        check(viewed != nullptr, what + ": the backup strictly parses as " + game + ": " + viewError);
        std::string sourceOfBackup;
        for (const char* manifest : {"BACKUP_MANIFEST.txt", "EDIT_MANIFEST.txt", "EXPORT_MANIFEST.txt"}) {
            std::ifstream in(fsx::path(backup).parent_path() / manifest);
            std::string line;
            while (std::getline(in, line))
                if (line.rfind("SOURCE_PATH=", 0) == 0) sourceOfBackup = line.substr(12);
        }
        check(!sourceOfBackup.empty(), what + ": the backup record names its source save");
        const std::string other = profile == kProfileA ? kProfileB : kProfileA;
        bool othersSave = app.bindings.resolveFileForGame(other, game).binding.sourcePath == sourceOfBackup;
        for (const auto& source : app.catalog.sources)
            if (source.path == sourceOfBackup) othersSave |= app.bindings.assignedProfile(source.sourceIdentity) == other;
        check(!othersSave, what + ": the backup is not a copy of the other profile's save (" + sourceOfBackup + ")");
        ++gOpens[game];
        return true;
    }
    std::string path;
    if (kind == Kind::RetroArchFRLG) {
        const size_t index = s.getSelectedLegacySourceIndex();
        if (index >= app.catalog.sources.size()) { check(false, what + ": source index out of range"); return false; }
        const auto& source = app.catalog.sources[index];
        path = source.path;
        check(source.ready() && source.gameId == game, what + ": the catalog entry is a ready " + game + " save (" + source.gameId + ")");
        PokeVault::Source::SaveInstance instance;
        instance.sourceIdentity = source.sourceIdentity;
        instance.sourceAliases = source.sourceAliases;
        instance.gameId = game;
        const auto file = app.bindings.resolveFileForGame(profile, game);
        check(app.bindings.isPreferredGameSource(instance, profile, game) || file.binding.sourcePath == path,
              what + ": this profile assigned exactly this save (" + path + ")");
        const std::string owner = app.bindings.assignedProfile(source.sourceIdentity);
        check(owner.empty() || owner == profile, what + ": the save is not another profile's (owner " + owner.substr(0, 8) + ")");
    } else {
        const auto file = app.bindings.resolveFileForGame(profile, game);
        check(file.status == PokeVault::Legacy::AssignedFileStatus::Ready, what + ": this profile has a ready assigned file");
        path = file.binding.sourcePath;
    }
    if (path.empty()) return false;
    const auto opened = Games20::open(*spec, path);
    check(static_cast<bool>(opened), what + ": the file strictly parses as an initialized " + game + " save (" + path + "): " + opened.error);
    const auto* placement = placementOf(game);
    check(placement && fsx::path(path).filename() == fsx::path(placement->path).filename(),
          what + ": it is this release's own file, not a sibling's (" + fsx::path(path).filename().string() + ")");
    ++gOpens[game];
    return true;
}

// H4: an ownership row may appear, disappear or change only for the profile that is current.
void ownershipStep(const Owners& before, const Owners& after, const std::string& profileBefore,
                   const std::string& profileAfter, const std::string& context) {
    const auto current = [&](const std::string& p) { return p == profileBefore || p == profileAfter; };
    for (const auto& [identity, owner] : after) {
        const auto old = before.find(identity);
        if (old == before.end()) check(current(owner), context + ": an assignment appeared for a profile that is not current (" + owner.substr(0, 8) + ")");
        else if (old->second != owner)
            check(false, context + ": a save changed owner " + old->second.substr(0, 8) + " -> " + owner.substr(0, 8) + " without a Forget");
    }
    for (const auto& [identity, owner] : before)
        if (!after.count(identity)) check(current(owner), context + ": another profile's assignment disappeared (" + owner.substr(0, 8) + ")");
}

void databaseLoads(const std::string& context) {
    if (!fsx::exists(PokeBank::Paths::legacySourceBindingsFile())) { ++checks; return; }
    LegacySourceBindings strict(PokeBank::Paths::legacySourceBindingsFile());
    check(strict.load(), context + ": the assignment database loads strictly: " + strict.lastError());
}

UI::SaveSelectScreen::NavigationState focus(const std::string& profile, const Games20::GameSpec& g) {
    UI::SaveSelectScreen::NavigationState nav;
    nav.profileIdentity = profile;
    nav.gameId = g.id;
    nav.sourceKind = kindOf(g);
    return nav;
}

// A. Directed: assign through the chooser, restart, reopen, other profile.
void directed() {
    for (const auto& g : Games20::kGames) {
        const std::string id = g.id;
        {
            App app;
            const auto nav = focus(kProfileA, g);
            app.open(&nav);
            app.frame();
            const bool focused = app.screen->navigationState().gameId == id && app.profile() == kProfileA;
            check(focused, id + ": the card exists for Profile A (real discovery found its save)");
            if (!focused) continue;
            app.press(HidNpadButton_A);
            if (!app.screen->hasSelectedTitle()) {
                // Not yet assigned: the chooser (Save Instances / source setup / candidates) is open.
                for (int i = 0; i < 6 && !app.screen->hasSelectedTitle(); ++i) app.press(HidNpadButton_A);
            }
            if (std::getenv("HOSTUI_DUMP")) { std::cout << "== " << id << " after assign\n"; for (const auto& t : HostUi::texts) std::cout << "  " << t << '\n'; }
            check(app.screen->hasSelectedTitle(), id + ": A then picking the listed save opens the game");
            if (app.screen->hasSelectedTitle()) openIsLegitimate(app, id + " first open");
        }
        {
            App app;   // "restart": new store and screen, state only from disk
            const auto nav = focus(kProfileA, g);
            app.open(&nav);
            app.frame();
            app.press(HidNpadButton_A);
            check(app.screen->hasSelectedTitle(), id + ": after a restart A opens the assigned save directly");
            if (app.screen->hasSelectedTitle()) openIsLegitimate(app, id + " restart");
        }
        {
            App app;
            const auto nav = focus(kProfileB, g);
            app.open(&nav);
            app.frame();
            if (app.profile() == kProfileB) {
                app.press(HidNpadButton_A);
                check(!app.screen->hasSelectedTitle(), id + ": Profile B's A never opens Profile A's save");
            } else {
                check(false, id + ": Profile B is selectable");
            }
        }
        sourcesIntact(id + " directed");
        databaseLoads(id + " directed");
    }
}

// R17-04 (found by the monkey, seeds 7000..7005: Profile B opened Profile A's Black 2 backup):
// a deliberate emulator-save backup (and a Gen I-IV export record) is stored per game, and the
// Backups list shows every history record of the game. From the save chooser (+ Backups), which
// needs no assignment, another profile could list and VIEW a backup of a save that is assigned to
// Profile A. Now the history of a save another profile owns is not listed for this profile; the
// owner still sees it, and nothing is deleted or moved.
void dump(const App& app, const std::string& where) {
    if (!std::getenv("HOSTUI_DUMP")) return;
    std::cout << "== " << where << '\n';
    for (const auto& t : HostUi::texts) std::cout << "  " << t << '\n';
}
bool backupRowDrawn() {   // a history row names its app-owned copy
    return std::any_of(HostUi::texts.begin(), HostUi::texts.end(),
                       [](const std::string& t) { return t.find("/backups/emulator-saves/") != std::string::npos; });
}
void backupsStayWithTheirProfile() {
    for (const char* id : {"red_gb", "platinum_nds", "black2_nds"}) {
        const auto& g = *specOf(id);
        const std::string game = id;
        const fsx::path records = fsx::path(PokeBank::Paths::backupsRoot()) / "emulator-saves" / game;
        {   // Profile A: assigned (directed pass). Game Workspace -> Backups -> Create backup now.
            App app;
            const auto nav = focus(kProfileA, g);
            app.open(&nav);
            app.frame();
            app.press(HidNpadButton_Plus);
            check(app.drawn("GAME WORKSPACE"), game + ": Profile A's + opens the Game Workspace (assigned save)");
            bool tapped = false;
            for (const auto& t : HostUi::visibleText)
                if (t.text == "Backups") { app.tap(static_cast<u32>(t.x + 6), static_cast<u32>(t.y + 6)); tapped = true; break; }
            check(tapped, game + ": the Backups row is in the Game Workspace");
            dump(app, game + " A backups");
            app.press(HidNpadButton_A);   // the leading "Create backup now" row
            dump(app, game + " A after create");
            std::error_code ec;
            const bool created = fsx::exists(records, ec) && !fsx::is_empty(records, ec);
            check(created, game + ": Profile A created a backup record under " + records.string());
            check(backupRowDrawn(), game + ": Profile A sees its backup in the list");
        }
        {   // Profile B: no assignment. A -> chooser -> + Backups.
            App app;
            const auto nav = focus(kProfileB, g);
            app.open(&nav);
            app.frame();
            check(app.profile() == kProfileB, game + ": Profile B is current");
            app.press(HidNpadButton_A);
            check(!app.screen->hasSelectedTitle(), game + ": Profile B's A opens the chooser, not a save");
            app.press(HidNpadButton_Plus);
            dump(app, game + " B backups");
            check(!backupRowDrawn(), game + ": Profile B's Backups list does not show Profile A's backup");
            // X: Recovery files (older copies; A marks, Y removes). Another profile's copies of a
            // save it owns are not B's to list or remove either.
            app.press(HidNpadButton_X);
            dump(app, game + " B recovery files");
            check(app.drawn("Recovery files"), game + ": X opens Recovery files");
            check(!backupRowDrawn() && !app.drawn("Backup - ") && !app.drawn(fsx::path(placementOf(game)->path).filename().string().substr(0, 20)),
                  game + ": Profile B's Recovery files do not list Profile A's backup copies");
            app.press(HidNpadButton_B);
            for (int i = 0; i < 3 && !app.screen->hasSelectedTitle(); ++i) app.press(HidNpadButton_A);
            check(!app.screen->hasSelectedTitle() || app.screen->getSelectedBackupSnapshot().path.empty(),
                  game + ": Profile B can never open Profile A's backup");
        }
        {   // Profile A again (restart): the record is still there and still listed.
            App app;
            const auto nav = focus(kProfileA, g);
            app.open(&nav);
            app.frame();
            app.press(HidNpadButton_Plus);
            for (const auto& t : HostUi::visibleText)
                if (t.text == "Backups") { app.tap(static_cast<u32>(t.x + 6), static_cast<u32>(t.y + 6)); break; }
            check(backupRowDrawn(), game + ": Profile A still sees its backup after Profile B looked");
        }
        sourcesIntact(game + " backups");
    }
}

const u64 kButtons[] = {HidNpadButton_A, HidNpadButton_B, HidNpadButton_X, HidNpadButton_Y, HidNpadButton_L,
                        HidNpadButton_R, HidNpadButton_ZL, HidNpadButton_ZR, HidNpadButton_Plus, HidNpadButton_Minus,
                        HidNpadButton_Up, HidNpadButton_Down, HidNpadButton_Left, HidNpadButton_Right};
const char* kButtonNames[] = {"A", "B", "X", "Y", "L", "R", "ZL", "ZR", "+", "-", "Up", "Down", "Left", "Right"};
const int kWeights[] = {24, 10, 5, 8, 3, 3, 2, 2, 6, 6, 8, 10, 8, 10};
const char* kNames[] = {"", "A", "QA", "PIKACHU", "\xE3\x83\x94\xE3\x82\xAB", "a b", "0"};

void monkey(uint32_t seed, int steps, bool trace) {
    std::mt19937 rng(seed);
    std::vector<std::string> log;
    App app;
    app.open(nullptr);
    app.frame();
    const std::string where = "home monkey seed " + std::to_string(seed);
    const int failuresAtStart = failures;
    const auto tail = [&] {
        std::cerr << "  replay: HOME_MONKEY_SEED=" << seed << " HOME_MONKEY_STEPS=" << steps << " HOME_MONKEY_TRACE=1\n  last actions:";
        for (size_t i = log.size() > 30 ? log.size() - 30 : 0; i < log.size(); ++i) std::cerr << ' ' << log[i] << ';';
        std::cerr << '\n';
    };
    for (int i = 0; i < steps && failures == failuresAtStart; ++i) {
        const Owners before = ownersOnDisk();
        const std::string profileBefore = app.profile();
        HostUi::numbers.clear();
        HostUi::textAnswers.clear();
        for (int k = 0; k < 2; ++k) {
            HostUi::numbers.push_back({rng() % 3 != 0, static_cast<int>(rng() % 1000)});
            HostUi::textAnswers.push_back({rng() % 3 != 0, kNames[rng() % std::size(kNames)]});
        }
        const unsigned roll = rng() % 100;
        std::string action;
        if (roll < 62) {
            int total = 0;
            for (int w : kWeights) total += w;
            int pick = static_cast<int>(rng() % static_cast<unsigned>(total));
            size_t b = 0;
            for (; b < std::size(kButtons); ++b) { pick -= kWeights[b]; if (pick < 0) break; }
            b = std::min(b, std::size(kButtons) - 1);
            action = kButtonNames[b];
            if (rng() % 8 == 0) {
                const int n = 2 + static_cast<int>(rng() % 5);
                action += "x" + std::to_string(n);
                for (int k = 0; k < n; ++k) app.frame(kButtons[b]);
                app.frame();
            } else {
                app.press(kButtons[b]);
            }
        } else if (roll < 92 && !HostUi::visibleText.empty()) {
            const auto t = HostUi::visibleText[rng() % HostUi::visibleText.size()];
            const u32 x = static_cast<u32>(std::clamp(t.x + 4, 0, 1279)), y = static_cast<u32>(std::clamp(t.y + 4, 0, 719));
            action = "tap \"" + t.text.substr(0, 18) + "\"";
            app.tap(x, y);
        } else {
            const u32 x = rng() % 1280, y = rng() % 720;
            action = "tap@" + std::to_string(x) + "," + std::to_string(y);
            app.tap(x, y);
        }
        log.push_back(action);
        const std::string context = where + " action #" + std::to_string(log.size()) + " (" + action + ")";
        if (trace) std::cerr << "  [" << seed << " #" << log.size() << "] " << action << "  profile=" << app.profile().substr(0, 4)
                             << " game=" << app.screen->navigationState().gameId << " opened=" << app.screen->hasSelectedTitle() << '\n';
        if (app.screen->hasSelectedTitle()) {
            openIsLegitimate(app, context);
            app.screen->resumeAfterEditor();
            app.frame();
        } else if (app.screen->shouldExit()) {
            // A destination (Vault, Settings, ...) or an app exit: Home comes back on the same state.
            const auto nav = app.screen->navigationState();
            app.open(&nav);
            app.frame();
        }
        ownershipStep(before, ownersOnDisk(), profileBefore, app.profile(), context);
        if (i % 25 == 24) { sourcesIntact(context); databaseLoads(context); }
    }
    if (failures != failuresAtStart) tail();
    sourcesIntact(where + " end");
    databaseLoads(where + " end");
}

} // namespace

int main(int argc, char** argv) {
    const fsx::path fixtures = fsx::absolute(argc > 1 ? argv[1] : GAMES20_FIXTURE_DIR);
    const auto env = [](const char* name, long fallback) {
        const char* v = std::getenv(name);
        return v && *v ? std::atol(v) : fallback;
    };
    const long seeds = env("HOME_MONKEY_SEEDS", 4), steps = env("HOME_MONKEY_STEPS", 500), oneSeed = env("HOME_MONKEY_SEED", -1);
    const bool trace = std::getenv("HOME_MONKEY_TRACE") != nullptr;

    char pattern[] = "/tmp/pokebank-games20-home-XXXXXX";
    const char* temp = ::mkdtemp(pattern);
    if (!temp) { std::cerr << "mkdtemp failed\n"; return EXIT_FAILURE; }
    const fsx::path root = temp, home = fsx::current_path();
    fsx::current_path(root);   // the private SD card
    for (const auto& p : kPlacements) {
        const auto bytes = Games20::readAll(fixtures / (std::string(p.id) + ".sav"));
        if (bytes.empty()) { check(false, std::string(p.id) + ": fixture missing in " + fixtures.string()); continue; }
        fsx::create_directories(fsx::path(p.path).parent_path());
        std::ofstream out(p.path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        out.close();
        gOriginal[p.path] = bytes;
    }
    check(gOriginal.size() == 20, "all 20 releases' saves are on the card");
    HostUi::clampNumbersLikeDevice = true;
    HostUi::consoleSaves = {};
    HostUi::consoleSaveCount = 0;
    HostUi::consoleSaves.active = true;
    HostUi::consoleSaves.user[0] = kUserA[0];
    HostUi::consoleSaves.user[1] = kUserA[1];
    HostUi::consoleSaves.user2[0] = kUserB[0];
    HostUi::consoleSaves.user2[1] = kUserB[1];
    HostUi::consoleSaves.userCount = 2;

    if (oneSeed < 0 && !std::getenv("HOME_MONKEY_ONLY")) {
        directed();
        backupsStayWithTheirProfile();
    }
    const int directedFailures = failures;
    long walks = 0;
    for (long s = 0; s < (oneSeed >= 0 ? 1 : seeds) && failures == directedFailures; ++s, ++walks)
        monkey(oneSeed >= 0 ? static_cast<uint32_t>(oneSeed) : static_cast<uint32_t>(7000 + s), static_cast<int>(steps), trace);

    HostUi::consoleSaves = {};
    fsx::current_path(home);
    std::error_code ignored;
    if (!failures || !std::getenv("MONKEY_KEEP")) fsx::remove_all(root, ignored);
    int opened = 0;
    for (const auto& g : Games20::kGames) opened += gOpens[g.id] > 0;
    if (failures) {
        std::cerr << failures << " failure(s) in " << checks << " checks; releases opened: " << opened << "/20\n";
        return EXIT_FAILURE;
    }
    std::cout << "Games-20 Product Home (real discovery, production store, two profiles; directed assign / restart / "
                 "other-profile for 20 releases, then " << walks << " monkey walks x " << steps << " actions; every open is the "
                 "current profile's own exact-game save, sources byte-identical, database always loads; releases opened "
              << opened << "/20; " << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
