// Round 17 autonomous QA: regressions for the defects the Games-20 monkey
// (tests/test_ui_games20_monkey_host.cpp) found, each replayed deterministically through the REAL
// TrainerViewScreen for all 20 Gen I-V releases. Every case failed before its fix.
//
// R17-01 (Gen I-IV, found by the monkey in Gold seed 4001, Silver 5002, Pearl 13001):
//   Legacy Storage, cursor on the game-save pane, bank changed, B -> "Save bank changes?".
//   A (and a tap on Save & Exit / Discard changes) opened the generation's "Pokemon Actions" /
//   "Empty Slot" menu instead of answering, because each generation's Storage entry ignored the open
//   question. The question could never be answered from that pane: the player could not save the
//   bank or leave Legacy Storage without first finding that the cursor had to move to the bank pane.
// R17-02 (all): the Import-from-file browser with nothing listable (no SD root) had B = "Up Folder"
//   with no parent to go to, so B did nothing for ever (only - Cancel closed it).
// Evidence class: SYNTHETIC saves, host harness.
#include "HostUi.h"

#include "Utils/NXTypes.h"
#include "Games20.h"
#include "Legacy/RetroArchFRLGDiscovery.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/PokemonFileDialogModel.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>

#ifndef GAMES20_FIXTURE_DIR
#define GAMES20_FIXTURE_DIR "build-host/games20"
#endif

namespace fsx = std::filesystem;
using View = UI::TrainerViewScreen::ViewMode;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << what << '\n'; }
}

struct Driver {
    UI::TrainerViewScreen& screen;
    UI::PKSEFramebuffer fb;
    UI::TouchInput touch;
    void frame(u64 down = 0) {
        PadState pad{down, down, 0};
        touch.update();
        screen.update(pad, touch);
        HostUi::texts.clear();
        HostUi::resetLayout();
        screen.draw(fb);
    }
    void press(u64 button) { frame(button); frame(); }
    bool drawn(const std::string& needle) const {
        return std::any_of(HostUi::texts.begin(), HostUi::texts.end(),
                           [&](const std::string& t) { return t.find(needle) != std::string::npos; });
    }
    bool tapText(const std::string& text) {
        for (const auto& t : HostUi::visibleText) {
            if (t.text != text) continue;
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

bool generationMenuOpen(const Driver& d) {
    return d.drawn("Pokemon Actions") || d.drawn("Empty Slot") || d.drawn("Add Pokemon") || d.screen.actionSheet.isOpen();
}

// Legacy Storage on the game-save pane with a changed bank and the exit question open.
bool openBankQuestion(const Games20::GameSpec& game, Driver& d, const std::string& what) {
    d.frame();
    d.screen.detailViewActive = true;
    d.screen.selectedMode = View::Storage;
    d.screen.storageFocusPane = 0;
    d.screen.stSaveBox = 0;
    d.screen.stSaveSlot = 0;
    d.frame();
    d.frame();
    if (!d.screen.bank) {
        check(false, what + ": Legacy Storage has a bank on " + game.id);
        return false;
    }
    HostUi::textAnswers.clear();
    // The bank is app-wide (shared by every game), so each case renames to a new name.
    static int serial = 0;
    HostUi::textAnswers.push_back({true, "QA" + std::to_string(++serial)});
    d.screen.renameBankBox(d.screen.stBankBox);
    HostUi::textAnswers.clear();
    d.frame();
    check(d.screen.bank->hasChanged(), what + ": renaming a bank box marks the bank changed");
    d.press(HidNpadButton_B);
    d.dump(what + " question");
    const bool open = d.screen.storageExitConfirmActive && d.drawn("Save bank changes?");
    check(open, what + ": B asks \"Save bank changes?\"");
    return open;
}

void bankQuestion(const Games20::GameSpec& game, const fsx::path& source) {
    const std::string id = game.id;
    if (game.generation == 5) {
        // Gen V has no Legacy Storage view in this build: its hub refuses the tile with a reason
        // (Gen5SharedReadOnlySurface HubCommand5::StorageRefused), so no bank question can open.
        auto opened = Games20::open(game, source);
        if (!opened) { check(false, id + ": the save opens: " + opened.error); return; }
        Driver d{*opened.screen, {}, {}};
        d.frame();
        check(d.tapText("Legacy Storage"), id + ": the Legacy Storage tile is on the hub");
        d.dump(id + " Legacy Storage tile");
        check(d.drawn("does not accept Gen V"), id + ": Legacy Storage is refused with a reason, not opened");
        check(!d.screen.storageExitConfirmActive && !generationMenuOpen(d), id + ": no question and no menu opens");
        d.press(HidNpadButton_B);
        return;
    }
    // A on Cancel, A on Discard changes, a tap on Discard changes, A on Save & Exit.
    struct Answer { const char* name; int downs; bool tap; bool leaves; };
    const Answer answers[] = {{"A on Cancel", 2, false, false}, {"A on Discard changes", 1, false, true},
                              {"tap Discard changes", 0, true, true}, {"A on Save & Exit", 0, false, true}};
    for (const auto& answer : answers) {
        auto opened = Games20::open(game, source);
        if (!opened) { check(false, id + ": the save opens: " + opened.error); return; }
        Driver d{*opened.screen, {}, {}};
        const std::string what = id + " / bank question / " + answer.name;
        if (!openBankQuestion(game, d, what)) continue;
        if (answer.tap) {
            check(d.tapText("Discard changes"), what + ": the option is on screen to tap");
        } else {
            for (int i = 0; i < answer.downs; ++i) d.press(HidNpadButton_Down);
            check(!generationMenuOpen(d), what + ": moving the highlight opens no Pokemon menu");
            d.press(HidNpadButton_A);
        }
        d.dump(what + " answered");
        check(!generationMenuOpen(d), what + ": the answer never opens the Pokemon Actions / Empty Slot menu");
        check(!d.screen.storageExitConfirmActive, what + ": the question is answered (closed)");
        if (answer.leaves) check(!d.screen.detailViewActive || d.screen.selectedMode != View::Storage || d.screen.saveConfirmActive,
                                 what + ": the answer leaves Legacy Storage");
        else check(d.screen.detailViewActive && d.screen.selectedMode == View::Storage, what + ": Cancel stays in Legacy Storage");
    }
    // The other two base Storage questions own input the same way (never a Pokemon menu under them).
    for (const char* which : {"release", "group menu"}) {
        auto opened = Games20::open(game, source);
        if (!opened) return;
        Driver d{*opened.screen, {}, {}};
        d.frame();
        d.screen.detailViewActive = true;
        d.screen.selectedMode = View::Storage;
        d.screen.storageFocusPane = 0;
        d.screen.stSaveBox = 0;
        d.screen.stSaveSlot = 0;
        d.frame();
        if (std::string(which) == "release") d.screen.releaseConfirmActive = true;
        else d.screen.groupMenuActive = true;
        d.frame();
        d.press(HidNpadButton_A);
        check(!generationMenuOpen(d), id + " / Storage " + which + " question: A never opens a Pokemon menu under it");
    }
}

// R17-02: the model's B at a folder with no parent and no listing closes; the label agrees.
void browserWithoutRoot() {
    namespace Model = PokeBank::UIModel::PokemonFileDialog;
    Model::State s;
    s.startFolder = "sdmc:/";          // nothing could be listed: listing.folder stays empty
    check(Model::bCloses(s), "file browser with no listable folder: B closes (label \"Close\")");
    const auto out = Model::press(s, Model::Key::B, {}, 8);
    check(out.command == Model::Command::Close, "file browser with no listable folder: B's command is Close");
}

// R17-03 (found by the Product Home monkey, seed 7005: the Red card opened "Pokemon - Blue
// Version.srm"): the SD picker accepted a file explicitly named for the sibling release wherever the
// save bytes cannot tell the two apart (Red/Blue, Gold/Silver, Ruby/Sapphire, FireRed/LeafGreen),
// although discovery files that same name under the other card. Now refused with the reason; a
// neutral name and the card's own name stay assignable. Releases the bytes identify were already
// refused (pinned here too).
void siblingNamedFiles(const fsx::path& fixtures, const fsx::path& root) {
    struct Case { const char* card; const char* bytesOf; const char* name; bool accepted; const char* reason; };
    const Case cases[] = {
        {"red_gb", "blue_gb", "Pokemon - Blue Version (USA, Europe) (SGB Enhanced).srm", false, "named as a Pokemon Blue save"},
        {"blue_gb", "red_gb", "Pokemon - Red Version (USA, Europe) (SGB Enhanced).srm", false, "named as a Pokemon Red save"},
        {"red_gb", "yellow_gb", "Pokemon - Yellow Version (USA, Europe).srm", false, ""},
        {"gold_gbc", "silver_gbc", "Pokemon - Silver Version (USA, Europe).srm", false, "named as a Pokemon Silver save"},
        {"silver_gbc", "gold_gbc", "Pokemon - Gold Version (USA, Europe).srm", false, "named as a Pokemon Gold save"},
        {"gold_gbc", "crystal_gbc", "Pokemon - Crystal Version (USA, Europe).srm", false, ""},
        {"ruby_gba", "sapphire_gba", "Pokemon - Sapphire Version (USA, Europe).srm", false, "named as a Pokemon Sapphire save"},
        {"sapphire_gba", "ruby_gba", "Pokemon - Ruby Version (USA, Europe).srm", false, "named as a Pokemon Ruby save"},
        {"firered_gba", "leafgreen_gba", "Pokemon - LeafGreen Version (USA, Europe).srm", false, "named as a Pokemon LeafGreen save"},
        {"leafgreen_gba", "firered_gba", "Pokemon - FireRed Version (USA, Europe).srm", false, "named as a Pokemon FireRed save"},
        {"ruby_gba", "emerald_gba", "Pokemon - Emerald Version (USA, Europe).srm", false, ""},
        // The card's own name, and neutral names, are the user's explicit choice: accepted.
        {"red_gb", "red_gb", "Pokemon - Red Version (USA, Europe).srm", true, ""},
        {"blue_gb", "blue_gb", "Pokemon - Blue Version (USA, Europe).srm", true, ""},
        {"red_gb", "red_gb", "slot1.srm", true, ""},
        {"blue_gb", "red_gb", "my save.sav", true, ""},
        {"gold_gbc", "gold_gbc", "Pokemon - Gold Version (USA, Europe).srm", true, ""},
        {"silver_gbc", "gold_gbc", "battery.srm", true, ""},
        {"ruby_gba", "ruby_gba", "Pokemon - Ruby Version (USA, Europe).srm", true, ""},
        {"sapphire_gba", "ruby_gba", "save1.srm", true, ""},
        {"leafgreen_gba", "leafgreen_gba", "Pokemon - LeafGreen Version (USA, Europe).srm", true, ""},
        {"firered_gba", "leafgreen_gba", "gba.sav", true, ""},
    };
    int n = 0;
    for (const auto& c : cases) {
        const auto bytes = Games20::readAll(fixtures / (std::string(c.bytesOf) + ".sav"));
        const fsx::path dir = root / "sibling" / std::to_string(++n);
        fsx::create_directories(dir);
        const fsx::path file = dir / c.name;
        { std::ofstream out(file, std::ios::binary); out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())); }
        const auto source = PokeVault::Legacy::inspectLegacySaveFile(file.string(), c.card, "Manual");
        const std::string what = std::string(c.card) + " <- \"" + c.name + "\"";
        check(source.ready() == c.accepted, what + (c.accepted ? ": accepted" : ": refused") + " (" + source.detail + ")");
        if (*c.reason) check(source.detail.find(c.reason) != std::string::npos, what + ": the reason names the other release (" + source.detail + ")");
        check(Games20::readAll(file) == bytes, what + ": the file is byte-identical");
    }
}

} // namespace

int main(int argc, char** argv) {
    const fsx::path fixtures = fsx::absolute(argc > 1 ? argv[1] : GAMES20_FIXTURE_DIR);
    char pattern[] = "/tmp/pokebank-games20-regress-XXXXXX";
    const char* temp = ::mkdtemp(pattern);
    if (!temp) { std::cerr << "mkdtemp failed\n"; return EXIT_FAILURE; }
    const fsx::path root = temp, home = fsx::current_path();
    fsx::current_path(root);
    fsx::create_directories("sdmc:");   // the SD root always exists on the device
    const std::string only = std::getenv("GAMES20_ONLY") ? std::getenv("GAMES20_ONLY") : "";

    int games = 0;
    for (const auto& game : Games20::kGames) {
        if (!only.empty() && only != game.id) continue;
        std::vector<uint8_t> bytes;
        const auto source = Games20::install(game, fixtures, root, &bytes);
        if (source.empty()) { check(false, std::string(game.id) + ": fixture missing in " + fixtures.string()); continue; }
        ++games;
        bankQuestion(game, source);
        check(Games20::readAll(source) == bytes, std::string(game.id) + ": the source save is byte-identical");
    }
    if (only.empty()) check(games == 20, "all 20 Gen I-V releases ran");
    browserWithoutRoot();
    if (only.empty()) siblingNamedFiles(fixtures, root);

    fsx::current_path(home);
    std::error_code ignored;
    fsx::remove_all(root, ignored);
    if (failures) { std::cerr << failures << " of " << checks << " check(s) failed\n"; return EXIT_FAILURE; }
    std::cout << "Games-20 regressions (R17-01 bank question answerable on the save pane, R17-02 browser B, "
                 "R17-03 sibling-named files refused; "
              << games << " releases, " << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
