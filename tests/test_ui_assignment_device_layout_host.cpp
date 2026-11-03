// Round 16 P0 (owner hardware, NRO 11ea9790): Yellow/Blue listed as validated RetroArch saves, but
// picking one said "Couldn't remember that exact save choice. Nothing was opened."; choosing the
// Yellow .srm in sdmc:/retroarch/cores/savefiles/Gambatte with the SD browser said "The save
// assignment could not be stored safely."
//
// Production layout, not a pre-seeded test fixture: a private working directory stands in for the
// SD card; the bindings database lives at the production path (sdmc:/switch/PokeBank-NX/config/...)
// with the session recovery UIManager enables; saves are found by the REAL bounded discovery
// (discoverConfiguredLegacySaves) under the RetroArch Gambatte core folder with No-Intro names.
// Scenarios through the real SaveSelectScreen: fresh SD; restart; torn database + .bak; damaged
// database without backup; Save Instances and SD browser agree; profile B isolation; Forget;
// source bytes unchanged. Evidence class: SYNTHETIC host files (Gen I bytes built in-test).
#include "HostUi.h"

#include "Utils/NXTypes.h"

#include "Legacy/LegacySourceBindings.h"
#include "Legacy/RetroArchFRLGDiscovery.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SaveSelectScreen.h"
#include "UI/TouchInput.h"
#include "Utils/PokeBankPaths.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

namespace fsx = std::filesystem;
using Kind = UI::SaveSelectScreen::SelectedSourceKind;
using PokeVault::Legacy::BindingResult;
using PokeVault::Legacy::LegacySourceBindings;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << what << '\n'; }
}
constexpr u64 kOwner[2] = {0x0123456789ABCDEFULL, 0x0FEDCBA987654321ULL};
constexpr u64 kSecond[2] = {0x1111111111111111ULL, 0x2222222222222222ULL};
const std::string kProfileA = "0123456789abcdef0fedcba987654321";
const std::string kProfileB = "11111111111111112222222222222222";
const std::string kGambatte = "sdmc:/retroarch/cores/savefiles/Gambatte";
const std::string kYellowLeaf = "Pokemon - Yellow Version (USA, Europe) (GBC Edition).srm";
const std::string kBlueLeaf = "Pokemon - Blue Version (USA, Europe) (SGB Enhanced).srm";

std::string bytesOf(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
void put(const std::string& path, const std::string& bytes) {
    fsx::create_directories(fsx::path(path).parent_path());
    std::ofstream f(path, std::ios::binary);
    f << bytes;
}
// Gen I battery save (same construction as tests/test_ui_save_instances_gen15_host.cpp).
uint8_t gen1Sum(const std::string& v, size_t start, size_t n) {
    unsigned s = 0;
    for (size_t i = start; i < start + n; ++i) s += static_cast<uint8_t>(v[i]);
    return static_cast<uint8_t>((~s) & 255);
}
std::string gen1Save(uint8_t tidLow, int starter) {
    const auto bank = [](size_t b) { return (b < 6 ? 0x4000 : 0x6000) + (b % 6) * 0x462; };
    std::string v(0x8000, '\0');
    const auto set = [&](size_t o, int value) { v[o] = static_cast<char>(value); };
    set(0x2598, 0x91); set(0x2599, 0x84); set(0x259A, 0x83); set(0x259B, 0x50);
    set(0x2605, 0x12); set(0x2606, tidLow);
    set(0x25CA, 0xFF); set(0x27E7, 0xFF); set(0x2F2D, 0xFF);
    set(0x284C, 0x82);
    set(0x29C3, starter);   // Yellow: Pikachu (0x54); Red/Blue: another starter
    for (size_t b = 0; b < 12; ++b) set(bank(b) + 1, 0xFF);
    set(0x30C0 + 1, 0xFF);
    set(0x3523, gen1Sum(v, 0x2598, 0xF8B));
    for (size_t b : {size_t(0x4000), size_t(0x6000)}) {
        set(b + 0x1A4C, gen1Sum(v, b, 0x1A4C));
        for (size_t i = 0; i < 6; ++i) set(b + 0x1A4D + i, gen1Sum(v, b + i * 0x462, 0x462));
    }
    return v;
}

struct Driver {
    UI::SaveSelectScreen& screen;
    UI::PKSEFramebuffer fb;
    UI::TouchInput touch;
    void frame(u64 down = 0) {
        PadState pad{down, down, 0};
        touch.update();
        screen.update(pad, touch);
        HostUi::texts.clear();
        HostUi::resetLayout();
        if (!screen.hasSelectedTitle() && !screen.shouldExit()) screen.draw(fb);
    }
    void press(u64 button) { frame(button); frame(); }
    bool drawn(const std::string& text) const {
        for (const auto& t : HostUi::texts)
            if (t.find(text) != std::string::npos) return true;
        return false;
    }
    bool tapText(const std::string& text) {
        for (const auto& t : HostUi::visibleText) {
            if (t.text.find(text) == std::string::npos) continue;
            const auto x = static_cast<u32>(t.x + 6), y = static_cast<u32>(t.y + 6);
            HostUi::touch = {true, x, y}; frame();
            HostUi::touch = {true, x, y}; frame();
            HostUi::touch = {false, x, y}; frame();
            frame();
            return true;
        }
        return false;
    }
    void dump(const std::string& where) const {
        if (!std::getenv("HOSTUI_DUMP")) return;
        std::cout << "== " << where << '\n';
        for (const auto& t : HostUi::texts) std::cout << "  " << t << '\n';
    }
};

LegacySourceBindings productionBindings() {
    LegacySourceBindings b(PokeBank::Paths::legacySourceBindingsFile());
    b.enableSessionRecovery();   // exactly what UIManager does
    (void)b.load();
    return b;
}
UI::SaveSelectScreen::NavigationState focus(const std::string& profile, const std::string& game) {
    UI::SaveSelectScreen::NavigationState nav;
    nav.profileIdentity = profile;
    nav.gameId = game;
    nav.sourceKind = Kind::RetroArchFRLG;
    return nav;
}
std::string openedPath(const UI::SaveSelectScreen& s, const PokeVault::Legacy::FRLGDiscoveryResult& c) {
    const size_t i = s.getSelectedLegacySourceIndex();
    return s.hasSelectedTitle() && i < c.sources.size() ? c.sources[i].path : std::string();
}

// One app launch: real discovery + production bindings, focus the card, press A. If Save Instances
// opens, pick the entry whose label contains `pick` (A on the highlighted row when pick is empty).
struct Launch {
    PokeVault::Legacy::FRLGDiscoveryResult catalog;
    LegacySourceBindings bindings;
    Launch() : catalog(PokeVault::Legacy::discoverConfiguredLegacySaves()), bindings(productionBindings()) {}
};
std::string openWithA(const std::string& what, const std::string& profile, const std::string& game,
                      bool expectChooser, bool* sawChooser = nullptr, std::string* notice = nullptr) {
    Launch app;
    const auto nav = focus(profile, game);
    UI::SaveSelectScreen screen(app.catalog, app.bindings, &nav);
    Driver d{screen, {}, {}};
    d.frame();
    check(screen.navigationState().gameId == game, what + ": the " + game + " card is focused");
    d.press(HidNpadButton_A);
    d.dump(what + " after A");
    const bool chooser = !screen.hasSelectedTitle() && d.drawn("Save Instances");
    if (sawChooser) *sawChooser = chooser;
    if (expectChooser) {
        check(chooser, what + ": A shows Save Instances (not yet assigned)");
        d.press(HidNpadButton_A);
        d.dump(what + " after picking");
        if (notice) for (const auto& t : HostUi::texts) *notice += t + "\n";
    }
    return openedPath(screen, app.catalog);
}
}

int main() {
    char pattern[] = "/tmp/pokebank-r16-layout-XXXXXX";
    if (!::mkdtemp(pattern)) return EXIT_FAILURE;
    const std::string root = pattern;
    const auto home = fsx::current_path();
    fsx::current_path(root);   // the private "SD card": sdmc:/... resolves inside it on the host

    const std::string yellow = kGambatte + "/" + kYellowLeaf;
    const std::string blue = kGambatte + "/" + kBlueLeaf;
    put(yellow, gen1Save(0x34, 0x54));
    put(blue, gen1Save(0x56, 0x99));
    const std::string yellowBytes = bytesOf(yellow), blueBytes = bytesOf(blue);
    const std::string db = PokeBank::Paths::legacySourceBindingsFile();

    HostUi::consoleSaves = {};
    HostUi::consoleSaveCount = 0;
    HostUi::consoleSaves.active = true;
    HostUi::consoleSaves.user[0] = kOwner[0];
    HostUi::consoleSaves.user[1] = kOwner[1];
    HostUi::consoleSaves.user2[0] = kSecond[0];
    HostUi::consoleSaves.user2[1] = kSecond[1];
    HostUi::consoleSaves.userCount = 2;

    {
        const auto catalog = PokeVault::Legacy::discoverConfiguredLegacySaves();
        int yellowReady = 0, blueReady = 0;
        for (const auto& s : catalog.sources) {
            if (s.ready() && s.gameId == "yellow_gb") ++yellowReady;
            if (s.ready() && s.gameId == "blue_gb") ++blueReady;
        }
        check(yellowReady == 1 && blueReady == 1, "real discovery finds the Gambatte Yellow and Blue .srm as ready");
        if (yellowReady != 1 || blueReady != 1)
            for (const auto& s : catalog.sources)
                std::cerr << "  discovered " << s.path << " game=" << s.gameId << " status="
                          << static_cast<int>(s.status) << " detail=" << s.detail << '\n';
    }
    check(!fsx::exists("sdmc:/switch"), "fresh SD: no PokeBank NX folder at all");

    // 1. Fresh SD, Save Instances pick (Failure A).
    check(openWithA("fresh Yellow", kProfileA, "yellow_gb", true) == yellow,
          "fresh SD: picking the discovered Yellow save opens it");
    check(fsx::is_regular_file(db), "fresh SD: the assignment is stored at the production path");
    // 2. Restart: opens directly, all routes agree on the same file.
    check(openWithA("restart Yellow", kProfileA, "yellow_gb", false) == yellow, "restart: A opens Yellow directly");
    // 3. Blue too, then a torn primary with a valid .bak (crash / power loss on FAT).
    check(openWithA("fresh Blue", kProfileA, "blue_gb", true) == blue, "Blue assigned through Save Instances");
    {
        const std::string good = bytesOf(db);
        fsx::copy_file(db, db + ".bak", fsx::copy_options::overwrite_existing);
        put(db, good.substr(0, good.size() - 7));
        check(openWithA("torn Yellow", kProfileA, "yellow_gb", false) == yellow,
              "torn database: Yellow still opens from the trusted .bak generation");
        // Forget then reassign: a WRITE on a torn primary (Round 15 refused every write here).
        Launch app;
        check(app.bindings.forgetGameAndSave(kProfileA, "yellow_gb", {}) &&
              app.bindings.lastResult() == BindingResult::Succeeded,
              "torn database: Forget is stored (damaged primary quarantined)");
        check(bytesOf(db + ".corrupt-1") == good.substr(0, good.size() - 7),
              "torn database: damaged bytes preserved byte-for-byte");
        check(openWithA("reassign Yellow", kProfileA, "yellow_gb", true) == yellow, "Forget then reassign: Yellow opens");
        check(openWithA("blue after repair", kProfileA, "blue_gb", false) == blue, "Blue assignment survived the repair");
    }
    // 4. Profile B never gets Profile A's saves.
    {
        bool chooser = false;
        const std::string opened = openWithA("profile B Yellow", kProfileB, "yellow_gb", false, &chooser);
        check(opened.empty(), "Profile B: Profile A's Yellow never opens");
    }
    // 5. Damaged database with no backup: never reset, file untouched, game still opens this session.
    {
        const std::string damaged = "# PokeBank NX legacy source bindings v1\nnot-hex\n";
        put(db, damaged);
        fsx::remove(db + ".bak");
        std::string notice;
        check(openWithA("damaged Yellow", kProfileA, "yellow_gb", true, nullptr, &notice) == yellow,
              "damaged database: picking Yellow opens it for this session");
        check(bytesOf(db) == damaged, "damaged database: kept byte-for-byte, never reset to empty");
        // Home is rebuilt after the editor: the session-only state is shown with its reason.
        Launch again;
        again.bindings.enableSessionRecovery();
        auto& shared = again.bindings;
        const auto blueCard = focus(kProfileA, "blue_gb");
        {
            // Same store object as the running app: make one session-only choice, then rebuild Home.
            UI::SaveSelectScreen first(again.catalog, shared, &blueCard);
            Driver d1{first, {}, {}};
            d1.frame();
            d1.press(HidNpadButton_A);
            d1.press(HidNpadButton_A);
            check(first.hasSelectedTitle(), "damaged database: Blue opens for this session");
        }
        check(shared.hasUnsavedSessionChanges(), "damaged database: the store knows the choice is session-only");
        UI::SaveSelectScreen home(again.catalog, shared, &blueCard);
        Driver d2{home, {}, {}};
        d2.frame();
        d2.dump("home after session-only choice");
        check(d2.drawn("kept until PokeBank NX closes"), "Home tells the user the choice is session-only");
    }
    // 6. SD browser (Failure B) on the damaged database: the same .srm opens through the manual route.
    {
        PokeVault::Legacy::FRLGDiscoveryResult none;   // as if discovery had not listed it
        auto bindings = productionBindings();
        const auto nav = focus(kProfileA, "yellow_gb");
        UI::SaveSelectScreen screen(none, bindings, &nav);
        Driver d{screen, {}, {}};
        d.frame();
        d.press(HidNpadButton_A);
        d.dump("browser root");
        check(d.drawn("ASSIGN SAVE FILE"), "SD browser opens for Yellow");
        check(d.tapText("Gambatte"), "SD browser lists the Gambatte core folder");
        d.dump("browser Gambatte");
        check(d.tapText("Yellow Version"), "SD browser lists the Yellow .srm");
        d.dump("browser after pick");
        check(screen.hasSelectedTitle() && screen.getSelectedGameId() == "yellow_gb",
              "SD browser: the Yellow .srm opens (no 'could not be stored safely')");
    }
    fsx::rename(db, root + "/damaged-kept.cfg");
    // 7. SD browser on a fresh database stores the file assignment; a restart opens it directly.
    {
        PokeVault::Legacy::FRLGDiscoveryResult none;
        auto bindings = productionBindings();
        const auto nav = focus(kProfileA, "yellow_gb");
        UI::SaveSelectScreen screen(none, bindings, &nav);
        Driver d{screen, {}, {}};
        d.frame();
        d.press(HidNpadButton_A);
        check(d.tapText("Gambatte") && d.tapText("Yellow Version") && screen.hasSelectedTitle(),
              "SD browser (healthy database): Yellow opens");
        check(bindings.lastResult() == BindingResult::Succeeded && !bindings.hasUnsavedSessionChanges(),
              "SD browser (healthy database): stored, not session-only");
        check(openWithA("after manual restart", kProfileA, "yellow_gb", false) == yellow,
              "restart: the manually chosen .srm opens directly; discovered and manual agree");
    }
    check(bytesOf(yellow) == yellowBytes && bytesOf(blue) == blueBytes, "source .srm files byte-identical");

    HostUi::consoleSaves = {};
    fsx::current_path(home);
    std::error_code ignored;
    fsx::remove_all(root, ignored);
    if (failures) { std::cerr << failures << " of " << checks << " check(s) failed\n"; return EXIT_FAILURE; }
    std::cout << "Round 16 production-layout assignment (real RetroArch Gambatte discovery, production "
                 "config path, fresh SD / restart / torn / damaged DB, Save Instances + SD browser, "
                 "profiles, Forget) through the real SaveSelectScreen (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
