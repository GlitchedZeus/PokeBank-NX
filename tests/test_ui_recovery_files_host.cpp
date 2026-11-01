// Owner item #10 (recovery management) through the REAL SaveSelectScreen update / draw on the host
// harness (tests/host-ui): Product Home -> + (Current Game) -> Backups -> X "Recovery files".
// The app's sdmc:/ root is a private mkdtemp working directory holding synthetic files only: two
// deliberate backups of the opening game's (Pokemon Diamond's) emulator save and two of another
// game's save (made by the real SourceBackups::createBackup), this profile's Switch-save Working
// folder with two DurableFile .previous generations, and a Bank file.
// Checked: the Backups footer offers X; the view lists rows grouped by save with Protected /
// Removable badges; nothing is selected by default and Y with nothing selected removes nothing; A on
// a protected row only explains; A selects a removable row (badge + footer count); Y opens a
// confirmation naming every file and its size; B there keeps everything; Y there removes exactly the
// selected copy; the newest copy, the source save, the .previous files and the Bank stay.
// Round 11 H12 (owner: "i cant see what is selected and nothing in the footer and nothing on screen
// changes"): checked through the DRAWN style (HostUi::fills), not text only. A picked row is
// drawn with the shared marked fill and Success bar and the others are not; focus and the mark
// are different rows' looks; headings are unboxed and a tap on one changes nothing; the footer
// reads Select / Deselect / Why kept, Y only with a selection, - Help, B Back; - opens Help and
// keeps the selection; focus starts on the opening game's first Removable copy although another
// game's group is listed first; A on a protected row explains in a toast, which a focus move
// clears; the confirmation footer names B; and select, Y, B, B (back to Backups) or Y+B in one
// frame leaves every file under sdmc:/ byte-identical.
#include "HostUi.h"

#include "Utils/NXTypes.h"
#include "Legacy/LegacySourceBindings.h"
#include "Legacy/RetroArchFRLGDiscovery.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SaveSelectScreen.h"
#include "UI/SharedBoxedRows.h"
#include "UI/SourceBackupClassification.h"
#include "UI/TouchInput.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
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

// Every regular file under `root`, path -> bytes (the byte audit for the safety regression).
std::map<std::string, std::string> snapshot(const fs::path& root) {
    std::map<std::string, std::string> files;
    for (const auto& entry : fs::recursive_directory_iterator(root))
        if (entry.is_regular_file()) files[entry.path().generic_string()] = slurp(entry.path());
    return files;
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
    void tapAt(int x, int y) {
        HostUi::touch = {true, static_cast<u32>(x), static_cast<u32>(y)};
        frame();
        HostUi::touch = {true, static_cast<u32>(x + 1), static_cast<u32>(y)};
        frame();
        HostUi::touch = {false, static_cast<u32>(x + 1), static_cast<u32>(y)};
        frame();
        frame();
    }
    bool exactly(const std::string& text) const {
        for (const auto& t : HostUi::texts)
            if (t == text) return true;
        return false;
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

// Drawn-style probes (Round 11 H12). A row box is a radius-10 rounded fill (SharedBoxedRows).
bool sameColor(uint32_t rgba, const UI::Color& c) { return rgba == c.toRGBA8(); }
std::vector<HostUi::Fill> boxes(const UI::Color& c) {
    std::vector<HostUi::Fill> out;
    for (const auto& f : HostUi::fills)
        if (f.radius == UI::SharedBoxedRows::kRadius && sameColor(f.rgba, c)) out.push_back(f);
    return out;
}
int markBars() {
    int n = 0;
    for (const auto& f : HostUi::fills)
        if (f.w == UI::SharedBoxedRows::kMarkBarWidth && sameColor(f.rgba, UI::Colors::Success)) ++n;
    return n;
}
const HostUi::TextRect* visible(const std::string& needle) {
    for (const auto& t : HostUi::visibleText)
        if (t.text.find(needle) != std::string::npos) return &t;
    return nullptr;
}
// The topmost row box (any colour, radius 10, row height) that the visible text containing
// `needle` sits in.
const HostUi::Fill* boxOf(const std::string& needle) {
    const HostUi::TextRect* t = visible(needle);
    if (!t) return nullptr;
    for (auto f = HostUi::fills.rbegin(); f != HostUi::fills.rend(); ++f)
        if (f->radius == UI::SharedBoxedRows::kRadius && f->h < 80 && t->x >= f->x && t->x < f->x + f->w &&
            t->y >= f->y && t->y < f->y + f->h)
            return &*f;
    return nullptr;
}
bool inMarkedBox(const std::string& needle) {
    const HostUi::Fill* f = boxOf(needle);
    return f && sameColor(f->rgba, UI::SharedBoxedRows::markedFill());
}
bool inPlainBox(const std::string& needle) {
    const HostUi::Fill* f = boxOf(needle);
    return f && sameColor(f->rgba, UI::Colors::Surface);
}

void recoveryFiles() {
    const std::string app = "sdmc:/switch/PokeBank-NX";
    const fs::path source = "sdmc:/roms/nds/Diamond.sav";   // the opening game (Pokemon Diamond)
    write(source, std::string(512, 'B'));
    const std::string manual = SB::emulatorBackupsRoot(app + "/backups");
    const auto older = SB::createBackup(manual, "diamond_nds", source.string(), "20261001_100000");
    const auto newer = SB::createBackup(manual, "diamond_nds", source.string(), "20261002_100000");
    check(older.ok && newer.ok, "two deliberate backups of the save");
    const fs::path working = fs::path(app) / "backups" / ("account-" + kProfile) / "0100ABF008968000" / "Working";
    write(working / "main", std::string(64, 'M'));
    write(working / "main.previous.0", "old");
    write(working / "main.previous.1", "new");
    fs::last_write_time(working / "main.previous.0", fs::file_time_type::clock::now() - std::chrono::seconds(600));
    write(fs::path(app) / "banks" / "legacy-pkse" / "bank.dat", "bank");
    // Another game's history, whose group sorts BEFORE Diamond's ("Black.sav" < "Diamond.sav"):
    // focus must still start in the opening game's (Diamond's) group.
    const fs::path otherSource = "sdmc:/roms/nds/Black.sav";
    write(otherSource, std::string(256, 'K'));
    const auto otherOld = SB::createBackup(manual, "black_nds", otherSource.string(), "20260901_100000");
    const auto otherNew = SB::createBackup(manual, "black_nds", otherSource.string(), "20260902_100000");
    check(otherOld.ok && otherNew.ok, "another game's two backups");
    const std::string olderLeaf = fs::path(older.recordDir).filename().string();
    const std::string newerLeaf = fs::path(newer.recordDir).filename().string();

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
    // The emulator Backups list keeps its look: plain Surface boxes, nothing marked.
    check(!boxes(UI::Colors::Surface).empty() && boxes(UI::SharedBoxedRows::markedFill()).empty() && markBars() == 0,
          "the Backups list draws plain boxes (no marked style)");
    const auto everything = snapshot("sdmc:");

    d.press(HidNpadButton_X);
    d.dump("recovery files");
    check(d.drawn("Older copies and leftovers"), "X opens Recovery files");
    check(d.drawn("Save: Diamond.sav"), "rows are grouped under the save they belong to");
    check(d.drawn("Save: Black.sav"), "the global list stays: another game's history is listed");
    check(d.drawn("Save: main"), "the Switch Working file's leftovers have their own group");
    check(d.drawn("Protected") && d.drawn("Removable"), "rows show Protected and Removable");
    // No card is assigned Diamond.sav on this harness, so its history is recovery only.
    check(d.drawn("(recovery only); its newest copy is always kept."), "the newest copy says why it stays");
    check(!d.drawn("Selected"), "nothing is selected by default");
    check(boxes(UI::SharedBoxedRows::markedFill()).empty() && markBars() == 0, "no row is drawn marked by default");
    check(!d.drawn("Remove selected"), "no remove action until something is selected");
    check(visible("Save: Diamond.sav") && !boxOf("Save: Diamond.sav"), "a save heading is drawn unboxed (a label)");
    check(inPlainBox(olderLeaf) && inPlainBox(newerLeaf), "copies are boxed rows");
    // Focus starts on Diamond's older (Removable) backup, not on Black's group listed first.
    check(d.exactly("Select") && d.drawn("Help") && d.drawn("Back") && !d.drawn("Why kept"),
          "footer: A Select on the opening game's first Removable copy, - Help, B Back");

    // A tap on a heading neither focuses it nor changes anything.
    for (const char* heading : {"Save: Diamond.sav", "Save: Black.sav"}) {
        const HostUi::TextRect* t = visible(heading);
        check(t != nullptr, std::string("heading visible: ") + heading);
        if (!t) continue;
        d.tapAt(t->x + 4, t->y + 2);
        check(d.exactly("Select") && !d.drawn("Selected") && boxes(UI::SharedBoxedRows::markedFill()).empty() &&
                  !d.drawn("Why kept"),
              std::string("a tap on a heading changes neither focus nor selection: ") + heading);
    }

    // Positive control: the same tap on a copy's row does reach it (focus + A toggles it).
    const std::string otherOldLeaf = fs::path(otherOld.recordDir).filename().string();
    if (const HostUi::TextRect* t = visible(otherOldLeaf)) {
        d.tapAt(t->x + 4, t->y + 2);
        check(inMarkedBox(otherOldLeaf) && d.drawn("Deselect"), "a tap on a copy focuses and selects it");
        d.tapAt(t->x + 4, t->y + 2);
        check(boxes(UI::SharedBoxedRows::markedFill()).empty() && !d.drawn("Selected"), "a second tap deselects it");
    } else {
        check(false, "another game's older copy is visible");
    }
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_Down);
    check(d.exactly("Select") && visible(olderLeaf), "focus back on the opening game's older copy");

    // Further down: the Switch Working leftovers and the read-only Bank (focus skips headings).
    for (int i = 0; i < 3; ++i) d.press(HidNpadButton_Down);
    check(d.drawn("Bank (read only)"), "the Bank is listed read-only");
    for (int i = 0; i < 3; ++i) d.press(HidNpadButton_Up);
    check(d.exactly("Select"), "back on the opening game's older copy");

    // Y with nothing selected: an explanation, no confirmation, nothing removed.
    d.press(HidNpadButton_Y);
    check(d.drawn("Select removable copies with A first"), "Y without a selection only explains");
    check(!d.drawn("These files will be removed"), "no confirmation without a selection");

    // A selects the focused row: it is drawn marked (fill + bar + Success badge); nothing else is.
    d.press(HidNpadButton_A);
    d.dump("selected");
    check(d.drawn("Selected"), "A selects the removable row");
    check(boxes(UI::SharedBoxedRows::markedFill()).size() == 1 && markBars() == 1, "exactly one row is drawn marked");
    check(inMarkedBox(olderLeaf) && inMarkedBox("Selected"), "the picked row (older Diamond backup) is the marked one");
    check(visible("Selected") && sameColor(visible("Selected")->rgba, UI::Colors::Success), "its badge is drawn in Success");
    check(inPlainBox(newerLeaf), "the newest copy stays plain");
    check(d.drawn("Remove selected (1)"), "the footer counts the selection");
    check(d.drawn("Deselect"), "the footer offers A Deselect on the picked row");
    check(!d.drawn("Select removable copies"), "a successful select clears the notice");

    // Up: focus moves to the newest (protected) copy; the mark stays on the picked row, so focus
    // and selection are two different looks on two different rows.
    d.press(HidNpadButton_Up);
    check(inMarkedBox(olderLeaf) && inPlainBox(newerLeaf), "focus and the mark are visibly different rows");
    check(d.drawn("Why kept") && !d.drawn("Deselect"), "the footer follows focus: A Why kept");
    d.press(HidNpadButton_A);
    check(d.drawn("Only Removable copies can be selected.") && d.drawn("its newest copy is always kept."),
          "A on a protected row says why in a toast");
    check(boxes(UI::SharedBoxedRows::markedFill()).size() == 1 && d.drawn("Remove selected (1)"),
          "A on a protected row changes no selection");
    d.press(HidNpadButton_Down);
    check(!d.drawn("Only Removable copies can be selected."), "moving focus clears the stale toast");
    check(d.drawn("Deselect"), "focus is back on the picked row");
    d.press(HidNpadButton_A);
    check(boxes(UI::SharedBoxedRows::markedFill()).empty() && markBars() == 0 && !d.drawn("Remove selected"),
          "A again deselects: no row marked, no Y");
    d.press(HidNpadButton_A);
    check(inMarkedBox(olderLeaf) && d.drawn("Remove selected (1)"), "A selects it again");

    // - opens Help; B closes it with the selection kept.
    d.press(HidNpadButton_Minus);
    d.dump("help");
    check(d.drawn("Recovery files Controls") && d.drawn("Close Help"), "- opens the Recovery files Help");
    check(d.drawn("a confirmation lists every file"), "Help explains that Y asks first");
    d.press(HidNpadButton_B);
    check(d.drawn("Older copies and leftovers") && inMarkedBox(olderLeaf) && d.drawn("Remove selected (1)"),
          "B closes Help; the selection is kept");

    // Safety regression (H12 steps): select, Y, B keeps every file byte for byte.
    d.press(HidNpadButton_Y);
    d.dump("confirm");
    check(d.drawn("Remove 1 copy (2 files"), "the confirmation summarises the plan");
    check(d.drawn(older.recordDir + "/save.sav  (512 B)"), "the backup file is named with its size");
    check(d.drawn(older.recordDir + "/BACKUP_MANIFEST.txt  ("), "the manifest is named with its size");
    check(d.drawn("These files will be removed. Press ") && d.drawn(" to remove; ") && d.drawn(" keeps them."),
          "the confirmation says which key removes");
    check(d.drawn("Keep") && d.drawn("Remove") && !d.drawn("Cancel"), "the confirmation footer names B Keep and Y Remove");
    d.press(HidNpadButton_B);
    check(d.drawn("Older copies and leftovers") && fs::exists(older.recordDir), "B keeps everything");
    check(snapshot("sdmc:") == everything, "select, Y, B: every file is byte-identical");
    check(inMarkedBox(olderLeaf), "B returns with the selection kept");
    // Y and B in the same frame on the confirmation: B wins, nothing is removed.
    d.press(HidNpadButton_Y);
    d.press(HidNpadButton_Y | HidNpadButton_B);
    check(d.drawn("Older copies and leftovers") && snapshot("sdmc:") == everything,
          "Y+B in one frame on the confirmation keeps every file");
    d.press(HidNpadButton_Y);
    d.press(HidNpadButton_A);
    check(d.drawn("Older copies and leftovers") && snapshot("sdmc:") == everything, "A on the confirmation keeps every file");
    // B back to Backups: still nothing touched.
    d.press(HidNpadButton_B);
    check(!d.drawn("Older copies and leftovers") && d.drawn("Backups"), "B returns to Backups");
    check(snapshot("sdmc:") == everything, "select, Y, B, B: every file is byte-identical");

    // Reopen: nothing is selected again; select, Y, Y removes exactly the selected copy.
    d.press(HidNpadButton_X);
    check(!d.drawn("Selected") && boxes(UI::SharedBoxedRows::markedFill()).empty(), "reopening starts with no selection");
    d.press(HidNpadButton_A);
    d.press(HidNpadButton_Y);
    d.press(HidNpadButton_Y);
    d.dump("after remove");
    check(d.drawn("Removed 1 of 1."), "the result is reported");
    check(!fs::exists(older.recordDir), "the selected older backup is removed");
    check(fs::exists(fs::path(newer.recordDir) / "save.sav"), "the newest backup stays");
    check(fs::exists(fs::path(otherOld.recordDir) / "save.sav") && fs::exists(fs::path(otherNew.recordDir) / "save.sav"),
          "the other game's backups stay");
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
