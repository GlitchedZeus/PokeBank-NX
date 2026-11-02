// Round 7 (B08 (1), P0): Gen II Create -> Review through the REAL TrainerViewScreen::update / draw
// (tests/host-ui harness, Round 6 E05 layout recorder). Owner Round 6 (26186464): "While creating
// Unown in Gen II, opening Review displays what appears to be an invisible screen. The application
// appears frozen, but pressing B exits the invisible screen and returns it to normal."
#include "HostUi.h"
#include "fixtures/gsc_pokemon_fixture.h"

#include "Legacy/GSCReadOnlyTrainer.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
}
using View = UI::TrainerViewScreen::ViewMode;

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
};
void dump(const char* tag) {
    if (!std::getenv("HOSTUI_DUMP")) return;
    std::cout << "== " << tag << '\n';
    for (const auto& t : HostUi::visibleText) std::cout << "  [" << t.text << "] " << t.x << "," << t.y << '\n';
}
// Visible after the whole frame was painted (the recorder drops text a later opaque fill covers).
bool visible(const std::string& text) {
    for (const auto& t : HostUi::visibleText)
        if (t.text == text) return true;
    return false;
}
bool visibleEndsWith(const std::string& suffix) {
    for (const auto& t : HostUi::visibleText)
        if (t.text.size() >= suffix.size() && t.text.compare(t.text.size() - suffix.size(), suffix.size(), suffix) == 0)
            return true;
    return false;
}
// The Review header's "<n> change(s) / Page p of q" line, or "" when none is visible.
std::string countLine() {
    for (const auto& t : HostUi::visibleText)
        if (t.text.find(" change") != std::string::npos && t.text.find(" / Page ") != std::string::npos) return t.text;
    return {};
}

// Boxes slot `slot` of Box 1 -> A (Empty Slot sheet) -> A (Add Pokemon): the Create workspace.
void openCreate(Driver& d, int slot) {
    d.screen.goBack = false;
    d.screen.detailViewActive = true;
    d.screen.selectedMode = View::Boxes;
    d.screen.selectedBoxIndex = 0;
    d.screen.selectedItemIndex = slot;
    d.frame();
    d.press(HidNpadButton_A);
    d.press(HidNpadButton_A);
}
// The draft's species through the real picker: A on Species, Down from the current row, A.
void chooseSpecies(Driver& d, int downs) {
    d.press(HidNpadButton_A);
    for (int i = 0; i < downs; ++i) d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
}

// Round 11 (owner H28 / shared CreateRequiredFields): a Create draft starts with no moves and Keep
// needs at least one (PokeBank app policy), so the scenario picks Move 1 through the real UI:
// R to the Moves panel, A on Move 1 (its editor), A on its Move row (the picker), Down + A on the
// first compatible move, then Apply in the move dialog.
bool chooseFirstMove(Driver& d) {
    bool opened = false;
    for (int panel = 0; panel < 3 && !opened; ++panel) {
        d.press(HidNpadButton_R);
        d.press(HidNpadButton_A);
        opened = visible("Move \xE2\x80\x94 Generation II");
        if (!opened) {
            d.press(HidNpadButton_A);
            opened = visible("Move \xE2\x80\x94 Generation II");
        }
        if (!opened) { d.press(HidNpadButton_B); d.press(HidNpadButton_B); }
    }
    dump("move picker");
    if (!opened) return false;
    d.press(HidNpadButton_Down);   // past "Empty move slot" to the first compatible move
    d.press(HidNpadButton_A);
    dump("move chosen");
    // The move dialog: Move, PP, PP Ups, Apply ("Keep these move details"), Cancel.
    for (int i = 0; i < 6 && !visible("Keep these move details"); ++i) d.press(HidNpadButton_B);
    for (int i = 0; i < 3; ++i) d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);   // Apply
    dump("move applied");
    const bool applied = !visible("Keep these move details") && !visible("Empty move slot");
    for (int i = 0; i < 3; ++i) d.press(HidNpadButton_L);
    return applied && visible("DRAFT");
}

constexpr const char* kReviewTitle = "Review Pending Changes";

void scenario(const std::string& game, bool crystal, bool unown) {
    const std::string what = game + (unown ? " Unown" : " Pikachu") + " Create";
    std::vector<uint8_t> bytes = fixture(crystal ? C : GS, true);
    const std::vector<uint8_t> original = bytes;
    auto parsed = parse(bytes, crystal ? SourceGame::Crystal : SourceGame::Silver);
    check(static_cast<bool>(parsed), what + ": fixture parses");
    if (!parsed) return;
    std::string error;
    auto trainer = PokeVault::Legacy::GSCReadOnlyTrainer::create(*parsed.save, error);
    check(trainer != nullptr, what + ": trainer: " + error);
    if (!trainer) return;
    const std::string id = crystal ? "crystal_gbc" : "silver_gbc";
    UI::TrainerViewScreen screen(*trainer, "Pokemon " + game, "/host/" + id + ".sav", 0, AccountUid{},
                                 PokeVault::Safety::SourceKind::RetroArchLegacy, id, "mGBA");
    Driver d{screen, {}, {}};
    d.frame();
    // The default draft is Pikachu (#25); the picker lists by National Dex number, so Unown (#201)
    // is 176 rows further down.
    const int downs = unown ? 201 - 25 : 0;
    const std::string title = unown ? "UNOWN" : "PIKACHU";

    // 1. Create at slot 1 and stage it (B -> exit question -> A Keep / Add).
    openCreate(d, 1);
    if (unown) chooseSpecies(d, downs);
    check(chooseFirstMove(d), what + ": Move 1 chosen through the real Move picker");
    dump("create 1");
    check(visible(title) && visible("DRAFT"), what + ": the Create workspace shows the " + title + " draft");
    d.press(HidNpadButton_B);
    d.press(HidNpadButton_A);
    dump("after stage");
    check(!visible("DRAFT"), what + ": Keep / Add staged the draft and closed the workspace");

    // 2. A second Create at slot 2, then + Review from the workspace.
    openCreate(d, 2);
    if (unown) chooseSpecies(d, downs);
    check(visible(title) && visible("DRAFT"), what + ": second " + title + " draft open");
    d.press(HidNpadButton_Plus);
    dump("review");
    check(visible(kReviewTitle), what + ": + draws Review Pending Changes, visible (not painted over)");
    const std::string count = countLine();
    check(!count.empty() && count.rfind("0 changes", 0) != 0, what + ": a non-zero change count is visible (" + count + ")");
    check(visibleEndsWith("/ Add Pok\xC3\xA9mon"), what + ": the staged Create row is visible");
    check(!visible("DRAFT"), what + ": the Create workspace is not drawn over or under the Review");

    // 3. Y asks to Discard; only that confirmation's Y discards. B cancels, nothing discarded.
    d.press(HidNpadButton_Y);
    dump("discard ask");
    check(visible(kReviewTitle), what + ": the Discard question is drawn over a visible Review");
    check(visible("Discard all staged changes?"), what + ": the Discard question is visible");
    d.press(HidNpadButton_B);
    check(visible(kReviewTitle) && countLine() == count,
          what + ": B cancels the Discard question; the staged changes are kept");

    // 4. X Export Copy: whatever the host file system allows, the source bytes never change and the
    //    staged change stays.
    d.press(HidNpadButton_X);
    check(visible(kReviewTitle) && countLine() == count, what + ": X Export keeps Review and the staged changes");
    check(bytes == original, what + ": the source save bytes are unchanged");

    // 5. B closes Review back to the same Create draft, still open and unstaged.
    d.press(HidNpadButton_B);
    dump("back");
    check(!visible(kReviewTitle), what + ": B closes Review");
    check(visible(title) && visible("DRAFT"), what + ": B returns to the same " + title + " Create draft");
    // The draft still answers its own keys (no frozen screen): + opens Review again, B closes it.
    d.press(HidNpadButton_Plus);
    check(visible(kReviewTitle), what + ": + opens Review again from the draft");
    d.press(HidNpadButton_B);
    check(visible(title) && visible("DRAFT"), what + ": and B returns to the draft again");
    // Round 7 (one Review key per screen): Y is this workspace's Random DVs, never Review.
    d.press(HidNpadButton_Y);
    check(!visible(kReviewTitle) && visible(title) && visible("DRAFT"), what + ": Y does not open Review");
}
} // namespace

int main() {
    for (const bool unown : {true, false}) {
        scenario("Silver", false, unown);
        scenario("Crystal", true, unown);
    }
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Gen II Create -> Review visible and returns to the draft (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
