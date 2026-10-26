// Round 6 7D: a tap outside a non-destructive editor picker closes it like B, the dismissing touch
// never activates the screen underneath (no tap-through), and confirmations (Discard All, Delete
// Backup, Export, Keep / Discard) are never dismissed by a stray tap. UI/SharedPopupDismiss.h is
// driven with real touch sequences; the Gen II / III / IV-V wiring is pinned in source.
#include "UI/SharedPopupDismiss.h"
#include "UI/TouchInput.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
namespace D = UI::SharedPopupDismiss;

std::string read(const std::string& path) {
    std::ifstream in(path);
    assert(in && "source readable");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void has(const std::string& text, const std::string& needle) {
    if (text.find(needle) == std::string::npos) std::cerr << "Missing: " << needle << '\n';
    assert(text.find(needle) != std::string::npos);
}
int occurrences(const std::string& text, const std::string& needle) {
    int n = 0;
    for (auto at = text.find(needle); at != std::string::npos; at = text.find(needle, at + 1)) ++n;
    return n;
}

// The screen as the editors run it: the open picker's handler goes first and consumes every frame;
// otherwise the workspace's release-confirmed button underneath may activate
// (TrainerViewScreen::touchedButtonId: a clean tap that starts and ends on the button).
struct Screen {
    bool pickerOpen = true;
    int underneathActivations = 0;
    int dismissals = 0;
    const D::Frame button{100, 100, 300, 80, true};   // a workspace row under / beside the picker

    void frame(const UI::TouchInput& touch) {
        if (pickerOpen) {
            if (D::tappedOutside(touch)) { pickerOpen = false; ++dismissals; }
            return;   // the picker handler returns true: nothing reaches the workspace
        }
        if (touch.justTapped() && button.contains(touch.startX(), touch.startY()) &&
            button.contains(touch.x(), touch.y()))
            ++underneathActivations;
    }
};

void run(Screen& s, UI::TouchInput& touch, int frames = 1) {
    for (int i = 0; i < frames; ++i) { s.frame(touch); touch.nextFrame(); }
}

void testTapOutsideDismissesWithoutTapThrough() {
    D::publish(300, 150, 620, 300);                 // a compact picker over the workspace
    Screen s;
    UI::TouchInput touch;
    touch.contact(150, 130); run(s, touch);         // touch-down outside, on the button beneath
    assert(s.pickerOpen && s.dismissals == 0);      // contact alone never closes it
    touch.contact(153, 128); run(s, touch, 3);      // held, small wobble
    assert(s.pickerOpen);
    touch.release(); run(s, touch);                 // release: dismissed on this frame
    assert(!s.pickerOpen && s.dismissals == 1);
    run(s, touch, 5);                               // later frames: no stale tap edge
    assert(s.underneathActivations == 0);
    // The workspace is live again: a NEW tap on that button works normally.
    touch.contact(150, 130); run(s, touch);
    touch.release(); run(s, touch);
    assert(s.underneathActivations == 1);
    std::cout << "tap outside dismisses on release, nothing underneath activates ok\n";
}

void testNotADismissal() {
    D::publish(300, 150, 620, 300);
    const auto tapAt = [](int x0, int y0, int x1, int y1) {
        Screen s;
        UI::TouchInput touch;
        touch.contact(x0, y0); run(s, touch);
        touch.contact(x1, y1); run(s, touch);
        touch.release(); run(s, touch, 3);
        assert(s.underneathActivations == 0);
        return !s.pickerOpen;
    };
    assert(tapAt(150, 130, 152, 131));              // outside, above the footer: dismiss
    assert(tapAt(1200, 600, 1195, 610));            // outside, right of the frame
    assert(!tapAt(400, 200, 402, 203));             // inside the picker
    assert(!tapAt(299, 200, 310, 200));             // starts outside, released inside
    assert(!tapAt(310, 200, 299, 200));             // starts inside, released outside
    assert(!tapAt(150, 130, 150, 300));             // a drag outside, not a tap
    assert(!tapAt(600, D::kFooterTop + 10, 600, D::kFooterTop + 12));   // the footer band
    assert(!tapAt(150, D::kFooterTop - 5, 150, D::kFooterTop + 5));
    std::cout << "inside, edge-crossing, drag and footer touches never dismiss ok\n";
}

void testConfirmationsAreNeverDismissed() {
    D::publish(300, 150, 620, 300);
    D::clear();                                     // what SharedEditorChrome::beginModal does
    Screen s;
    UI::TouchInput touch;
    touch.contact(150, 130); run(s, touch);
    touch.release(); run(s, touch, 3);
    assert(s.pickerOpen && s.dismissals == 0 && s.underneathActivations == 0);
    std::cout << "a modal without a published picker frame is never dismissed ok\n";
}

void testWiring() {
    const std::string chrome = read("include/UI/SharedEditorChrome.h");
    has(chrome, "    beginTopTouchLayer();\n    SharedPopupDismiss::clear();");
    const std::string gen2 = read("src/UI/Gen2PokemonEditorFoundation.inc");
    const std::string gen3 = read("src/UI/Gen3SharedPokemonSurface.inc");
    const std::string gen45 = read("src/UI/SharedPokemonEditorSurface.inc");
    // Each picker handler checks first and returns, like B.
    has(gen2, "    if (SharedPopupDismiss::tappedOutside(touch)) {\n"
              "        // 7D: a release-confirmed tap outside the picker cancels it like B; this frame's\n"
              "        // return keeps the gesture from reaching the workspace underneath.\n"
              "        state.itemScroll.stop();\n        state.itemPicker = false;");
    has(gen3, "    if (SharedPopupDismiss::tappedOutside(touch)) {\n        state.pickerScroll.stop();\n        screen.controllerNavigation.reset();\n        state.valuePicker = false;");
    has(gen3, "    if (!state.speciesPicker) return false;\n    if (SharedPopupDismiss::tappedOutside(touch)) {");
    has(gen45, "    if (SharedPopupDismiss::tappedOutside(touch)) {\n        state.pickerScroll.stop();\n        state.valuePicker = false;");
    // Only picker draws publish a frame, each right after its beginModal / frame geometry.
    has(gen2, "    UI::SharedEditorChrome::beginModal(screen.touchButtons);\n    SharedPopupDismiss::publish(x, y, w, h);");
    has(gen3, "    const int y = (fb.getHeight() - kNavBarH - h) / 2;\n    SharedPopupDismiss::publish(x, y, w, h);");
    has(gen3, "SharedPopupDismiss::publish(modal.x, modal.y, modal.width, modal.height);");
    has(gen45, "    const int y = (fb.getHeight() - kNavBarH - h) / 2;\n    SharedPopupDismiss::publish(x, y, w, h);");
    has(gen45, "            SharedPopupDismiss::publish(left, top, std::max(x + w, modal.x + modal.width) - left,");
    int publishers = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator("src")) {
        if (!entry.is_regular_file()) continue;
        const std::string text = read(entry.path().string());
        const int n = occurrences(text, "SharedPopupDismiss::publish(");
        const std::string name = entry.path().filename().string();
        if (n) assert(name == "Gen2PokemonEditorFoundation.inc" || name == "Gen3SharedPokemonSurface.inc" ||
                      name == "SharedPokemonEditorSurface.inc" || name == "SaveSelectScreen.cpp" ||
                      name == "ClassicPartyActions.inc" || name == "Gen1PokemonEditorOverlayUXCleanup3.inc" ||
                      name == "Gen2PokemonPickerOverlay.inc" || name == "Gen2HardwarePickerFix.inc" ||
                      name == "StoragePanel.cpp");
        publishers += n;
    }
    // Gen II Held Item; Gen III value + species; Gen IV/V value + species; Game Browser Help;
    // Party Actions; Gen I species + move; Gen II species, Crystal Met Location, move list and the
    // hardware move picker. Game / Trainer Help publishes from ScreenChrome drawInfoOverlay.
    // Round 7 C04: Product Home Quick Games, profile / game-file pickers, Legacy details, Quick
    // Options (SaveSelectScreen.cpp, now 6), and the Pokémon actions / carried-block menus
    // (StoragePanel.cpp drawPopupMenu, only when `dismissible`).
    assert(publishers == 19);
    const std::string home = read("src/UI/SaveSelectScreen.cpp");
    has(home, "        if (SharedPopupDismiss::tappedOutside(touch)) {\n            switch (overlay) {\n"
              "                case Overlay::GamesDrawer:\n                    if (!quickDrawerMotion.interactive()) break;\n"
              "                    quickDrawerMotion.closeAnimated();");
    for (const char* never : {"case Overlay::QuitConfirm:\n                    overlay", "case Overlay::EmulatorBackupDeleteConfirm:\n                    overlay",
                              "case Overlay::Gen5UnassignConfirm:\n                    overlay"})
        assert(home.find(never) == std::string::npos);
    const std::string storage = read("src/UI/Panels/StoragePanel.cpp");
    has(storage, "                if (dismissible) SharedPopupDismiss::publish(x, y, w, h);\n"
                 "                else SharedPopupDismiss::clear();");
    // The storage-exit "Save bank changes?" confirmation never passes `dismissible`.
    has(storage, "drawPopupMenu(screen, fb, \"Save bank changes?\", items, 3, screen.storageExitConfirmIndex);");
    const std::string browser = read("src/UI/TrainerViewScreenBase.inc");
    has(browser, "            if (SharedPopupDismiss::tappedOutside(touch)) {\n                actionSheet.close();\n                return;");
    has(browser, "            if (SharedPopupDismiss::tappedOutside(touch)) {\n                groupMenuActive = false;\n                return;");
    // Gen I / II species and move pickers: the one input entry closes the open picker like B.
    has(read("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc"),
        "    if (isGen1SourceUX(screen) && ux2StateFor(screen).picker != UX2Picker::None &&\n"
        "        SharedPopupDismiss::tappedOutside(touch)) {");
    has(read("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc"), "        state.picker = UX2Picker::None;\n        return true;");
    has(read("src/UI/Gen2HardwarePickerFix.inc"),
        "    if (picker.model.active() && state.mode != Mode::View && SharedPopupDismiss::tappedOutside(touch)) {\n"
        "        closePicker(screen, picker);\n        return true;");
    has(read("include/UI/ScreenChrome.h"), "        drawModalSurface(fb, x, y, w, h);\n"
        "        SharedPopupDismiss::publish(x, y, w, h);   // 7D: Help closes on a tap outside it");
    has(read("src/UI/SaveSelectScreen.cpp"), "            if ((kDown & (HidNpadButton_B | HidNpadButton_Minus)) || "
        "SharedPopupDismiss::tappedOutside(touch)) {\n                overlay = helpReturnOverlay;");
    has(read("src/UI/TrainerViewScreenBase.inc"), "            else if (SharedPopupDismiss::tappedOutside(touch)) "
        "helpOverlayActive = false;\n            return;");
    has(read("src/UI/ClassicPartyActions.inc"), "if(SharedPopupDismiss::tappedOutside(touch)){close(screen);return true;}");
    // Destructive confirmations never consult it.
    for (const char* rel : {"src/UI/BackupSelectionScreen.cpp", "src/UI/ClassicReleaseActionFix.inc",
                            "include/UI/Gen12ReviewDiscardConfirm.h"})
        assert(read(rel).find("SharedPopupDismiss") == std::string::npos);
    std::cout << "Gen II / III / IV-V picker wiring, confirmations never publish ok\n";
}
} // namespace

int main() {
    testTapOutsideDismissesWithoutTapThrough();
    testNotADismissal();
    testConfirmationsAreNeverDismissed();
    testWiring();
    std::cout << "test_popup_dismiss: all passed\n";
    return 0;
}
