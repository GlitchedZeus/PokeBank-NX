// Owner item #10 (recovery management) through the REAL SaveSelectScreen update / draw on the host
// harness (tests/host-ui): Product Home -> + (Current Game) -> Backups -> X "Recovery files".
// The app's sdmc:/ root is a private mkdtemp working directory holding synthetic files only: two
// deliberate backups of one emulator save (made by the real SourceBackups::createBackup), this
// profile's Switch-save Working folder with two DurableFile .previous generations, and a Bank file.
// Checked: the Backups footer offers X; the view lists rows grouped by save with Protected /
// Removable badges; nothing is selected by default and Y with nothing selected removes nothing; A on
// a protected row only explains; A selects a removable row (badge + footer count); Y opens a
// confirmation naming every file and its size; B there keeps everything; Y there removes exactly the
// selected copy; the newest copy, the source save, the .previous files and the Bank stay.
#include "HostUi.h"

#include "Utils/NXTypes.h"
#include "Legacy/LegacySourceBindings.h"
#include "Legacy/RetroArchFRLGDiscovery.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SaveSelectScreen.h"
#include "UI/SourceBackupClassification.h"
#include "UI/TouchInput.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
namespace SB = PokeBank::UIModel::SourceBackups;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}
std::string slurp(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void write(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream(path, std::ios::binary) << text;
}

constexpr u64 kOwner[2] = {0x0123456789ABCDEFULL, 0x0FEDCBA987654321ULL};
const std::string kProfile = "0123456789abcdef0fedcba987654321";

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
        screen.draw(fb);
    }
    void frames(int n) { for (int i = 0; i < n; ++i) frame(); }
    void press(u64 button) {
        frame(button);
        frame();
    }
    bool drawn(const std::string& text) const {
        for (const auto& t : HostUi::texts)
            if (t.find(text) != std::string::npos) return true;
        return false;
    }
    void dump(const std::string& where) const {
        if (!std::getenv("HOSTUI_DUMP")) return;
        std::cout << "== " << where << '\n';
        for (const auto& t : HostUi::texts) std::cout << "  " << t << '\n';
    }
};

void recoveryFiles() {
    const std::string app = "sdmc:/switch/PokeBank-NX";
    const fs::path source = "sdmc:/roms/nds/Black.sav";
    write(source, std::string(512, 'B'));
    const std::string manual = SB::emulatorBackupsRoot(app + "/backups");
    const auto older = SB::createBackup(manual, "black_nds", source.string(), "20261001_100000");
    const auto newer = SB::createBackup(manual, "black_nds", source.string(), "20261002_100000");
    check(older.ok && newer.ok, "two deliberate backups of the save");
    const fs::path working = fs::path(app) / "backups" / ("account-" + kProfile) / "0100ABF008968000" / "Working";
    write(working / "main", std::string(64, 'M'));
    write(working / "main.previous.0", "old");
    write(working / "main.previous.1", "new");
    fs::last_write_time(working / "main.previous.0", fs::file_time_type::clock::now() - std::chrono::seconds(600));
    write(fs::path(app) / "banks" / "legacy-pkse" / "bank.dat", "bank");

    HostUi::consoleSaves = {};
    HostUi::consoleSaveCount = 0;
    HostUi::consoleSaves.active = true;
    HostUi::consoleSaves.user[0] = kOwner[0];
    HostUi::consoleSaves.user[1] = kOwner[1];

    PokeVault::Legacy::FRLGDiscoveryResult legacy;
    PokeVault::Legacy::LegacySourceBindings bindings;
    UI::SaveSelectScreen screen(legacy, bindings);
    Driver d{screen, {}, {}};
    d.frames(3);
    d.dump("home");

    // + opens the Current Game menu; Backups is its seventh tile (row 4, column 1: Up from Overview).
    d.press(HidNpadButton_Plus);
    d.press(HidNpadButton_Up);
    d.press(HidNpadButton_A);
    d.dump("backups");
    check(d.drawn("Backups"), "the game's Backups screen is open");
    check(d.drawn("Recovery files"), "the Backups footer offers X: Recovery files");

    d.press(HidNpadButton_X);
    d.dump("recovery files");
    check(d.drawn("Older copies and leftovers"), "X opens Recovery files");
    check(d.drawn("Save: Black.sav"), "rows are grouped under the save they belong to");
    check(d.drawn("Save: main"), "the Switch Working file's leftovers have their own group");
    check(d.drawn("Bank (read only)"), "the Bank is listed read-only");
    check(d.drawn("Protected") && d.drawn("Removable"), "rows show Protected and Removable");
    // No card is assigned Black.sav on this harness, so its history is recovery only.
    check(d.drawn("(recovery only); its newest copy is always kept."), "the newest copy says why it stays");
    check(!d.drawn("Selected"), "nothing is selected by default");
    check(!d.drawn("Remove selected"), "no remove action until something is selected");

    // Y with nothing selected: an explanation, no confirmation, nothing removed.
    d.press(HidNpadButton_Y);
    check(d.drawn("Select removable copies with A first"), "Y without a selection only explains");
    check(!d.drawn("These files will be removed"), "no confirmation without a selection");

    // Focus starts on the newest (protected) backup: A only explains.
    d.press(HidNpadButton_A);
    check(d.drawn("its newest copy is always kept.") && !d.drawn("Selected"),
          "A on a protected row selects nothing");

    // Down to the older backup; A selects it.
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    d.dump("selected");
    check(d.drawn("Selected"), "A selects the removable row");
    check(d.drawn("Remove selected (1)"), "the footer counts the selection");

    // Y opens the confirmation listing every file with its size; B keeps everything.
    d.press(HidNpadButton_Y);
    d.dump("confirm");
    check(d.drawn("Remove 1 copy (2 files"), "the confirmation summarises the plan");
    check(d.drawn(older.recordDir + "/save.sav  (512 B)"), "the backup file is named with its size");
    check(d.drawn(older.recordDir + "/BACKUP_MANIFEST.txt  ("), "the manifest is named with its size");
    check(d.drawn("These files will be removed. Press ") && d.drawn(" to remove; ") && d.drawn(" keeps them."),
          "the confirmation says which key removes");
    d.press(HidNpadButton_B);
    check(d.drawn("Older copies and leftovers") && fs::exists(older.recordDir), "B keeps everything");

    // Y, then Y: exactly the selected copy goes.
    d.press(HidNpadButton_Y);
    d.press(HidNpadButton_Y);
    d.dump("after remove");
    check(d.drawn("Removed 1 of 1."), "the result is reported");
    check(!fs::exists(older.recordDir), "the selected older backup is removed");
    check(fs::exists(fs::path(newer.recordDir) / "save.sav"), "the newest backup stays");
    check(slurp(source) == std::string(512, 'B'), "the original save is untouched");
    check(slurp(working / "main.previous.0") == "old" && slurp(working / "main.previous.1") == "new",
          "unselected leftovers stay");
    check(slurp(fs::path(app) / "banks" / "legacy-pkse" / "bank.dat") == "bank", "the Bank is untouched");
    check(!d.drawn("Selected"), "the selection is cleared after removal");

    // B returns to Backups.
    d.press(HidNpadButton_B);
    check(!d.drawn("Older copies and leftovers") && d.drawn("Backups"), "B returns to Backups");
}

} // namespace

int main() {
    char templ[] = "/tmp/pokebank-recovery-ui-XXXXXX";
    const char* made = ::mkdtemp(templ);
    if (!made) return 1;
    const fs::path previous = fs::current_path();
    fs::current_path(made);
    fs::create_directory("sdmc:");
    recoveryFiles();
    fs::current_path(previous);
    std::error_code ec;
    fs::remove_all(made, ec);
    check(!fs::exists(made), "temporary directory cleaned up");
    std::cout << "Recovery files (real SaveSelectScreen, host harness): " << checks << " checks, " << failures
              << " failure(s). SYNTHETIC files only.\n";
    return failures == 0 ? 0 : 1;
}
