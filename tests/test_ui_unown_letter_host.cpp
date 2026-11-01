// Round 7 (owner B07 R2-REROLL / C09 UNOWN-LETTER, 26186464): the Gen II Unown letter, Shiny and
// Reroll through the REAL TrainerViewScreen input and draw (tests/host-ui harness). A Silver save
// holds an Unown A (DVs 0/0/0/0) in Box 2. The test opens it in Edit, then:
//   * A on the Values Letter row opens the grid, which states the current letter and shiny state;
//   * picking K applies DVs for K (not shiny);
//   * Shiny on K asks "Shiny Unown can only be I and V. Change letter to ...?"; B cancels with
//     nothing changed, A applies a shiny I / V;
//   * picking K while shiny asks "Unown K can't be shiny. Turn Shiny off and use K?"; A applies;
//   * Y (Reroll) in both states changes the DVs and keeps letter and shiny;
//   * Round 11 H22 (UI/EditFeedback.h): an applied letter, a confirmed Shiny and a successful
//     Reroll post no bottom-centre toast (the changed letter / DVs / sprite are the feedback);
//     the cancelled question still says "Unown unchanged";
//   * X keeps the draft: the stored record's form (the Box sprite key) and Review name the letter
//     the DVs give.
#include "HostUi.h"
#include "fixtures/gsc_pokemon_fixture.h"

#include "Legacy/GSCReadOnlyTrainer.h"
#include "Pokemon/Pokemon2ReadOnly.h"
#include "UI/Gen12DVReroll.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

int failures = 0;
int checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
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
};

bool drew(const std::string& needle) {
    for (const auto& t : HostUi::texts)
        if (t.find(needle) != std::string::npos) return true;
    return false;
}
// The frame draws a footer bar and no two visible text runs overlap (HostUi layout recorder).
void layoutOk(const std::string& where) {
    check(HostUi::footerBars >= 1, where + ": a footer bar is drawn");
    const auto& t = HostUi::visibleText;
    for (size_t i = 0; i < t.size(); ++i)
        for (size_t j = i + 1; j < t.size(); ++j) {
            const bool overlap = t[i].x < t[j].x + t[j].w && t[j].x < t[i].x + t[i].w &&
                                 t[i].y < t[j].y + t[j].h && t[j].y < t[i].y + t[i].h;
            if (overlap) check(false, where + ": \"" + t[i].text + "\" overlaps \"" + t[j].text + "\"");
        }
}
// The value drawn right after the LAST occurrence of a label (the final hardware layer paints last).
std::string valueAfter(const std::string& label) {
    std::string value = "<" + label + " not drawn>";
    for (size_t i = 0; i + 1 < HostUi::texts.size(); ++i)
        if (HostUi::texts[i] == label) value = HostUi::texts[i + 1];
    return value;
}

} // namespace

int main() {
    using namespace PokeVault::Integration::Gen2;
    auto bytes = fixture(GS, true);
    setBoxCount(bytes, 1, 1, 201);
    writeStoredPokemon(bytes, 1, 0, 201, "OT", "UNOWN", 0, 0, 0, 0);   // Unown A, not shiny
    checksum(bytes, GS);
    auto parsed = parse(bytes, SourceGame::Silver);
    check(static_cast<bool>(parsed), "Silver fixture with an Unown parses");
    if (!parsed) return 1;
    std::string error;
    const auto quiet = [](UI::TrainerViewScreen& s) { s.storageStatus.clear(); s.storageStatusFrames = 0; };
    // The four editable DVs as drawn in the STATS grid (the value after each row label).
    const auto dvsShown = [] {
        return valueAfter("Attack") + "/" + valueAfter("Defense") + "/" + valueAfter("Speed") + "/" +
               valueAfter("Special");
    };
    auto trainer = PokeVault::Legacy::GSCReadOnlyTrainer::create(*parsed.save, error);
    check(trainer != nullptr, "Silver trainer: " + error);
    if (!trainer) return 1;
    UI::TrainerViewScreen screen(*trainer, "Pokemon Silver", "/host/silver_gbc.sav", 0, AccountUid{},
                                 PokeVault::Safety::SourceKind::RetroArchLegacy, "silver_gbc", "mGBA");
    Driver d{screen, {}, {}};
    d.frame();
    screen.detailViewActive = true;
    screen.selectedMode = UI::TrainerViewScreen::ViewMode::Boxes;
    screen.selectedBoxIndex = 1;
    screen.selectedItemIndex = 0;
    d.frame();
    // Round 11 H22: the Special row's Calculated cell shows Sp. Atk then Sp. Def ("<a>  <b>"), the
    // same two numbers the labelled Special block below the grid shows, in View and in Edit.
    const auto specialCellOk = [](const std::string& where) {
        const std::string spa = valueAfter("Sp. Atk"), spd = valueAfter("Sp. Def");
        check(!spa.empty() && spa[0] != '<' && !spd.empty() && spd[0] != '<', where + ": the Special block is drawn");
        check(drew(spa + "  " + spd), where + ": the Special Calculated cell shows \"" + spa + "  " + spd + "\"");
    };
    {   // View on a screen of its own, so its passive focus does not carry into the Edit below.
        UI::TrainerViewScreen viewScreen(*trainer, "Pokemon Silver", "/host/silver_gbc.sav", 0, AccountUid{},
                                         PokeVault::Safety::SourceKind::RetroArchLegacy, "silver_gbc", "mGBA");
        Driver v{viewScreen, {}, {}};
        v.frame();
        viewScreen.detailViewActive = true;
        viewScreen.selectedMode = UI::TrainerViewScreen::ViewMode::Boxes;
        viewScreen.selectedBoxIndex = 1;
        viewScreen.selectedItemIndex = 0;
        v.frame();
        v.press(HidNpadButton_A);  // actions
        v.press(HidNpadButton_A);  // View
        specialCellOk("Gen II View");
    }
    d.press(HidNpadButton_A);      // actions
    d.press(HidNpadButton_Down);   // Edit
    d.press(HidNpadButton_A);
    check(drew("Letter"), "Unown Edit shows the Letter row");
    specialCellOk("Gen II Edit");
    check(valueAfter("Letter") == "A", "starts as Unown A, got " + valueAfter("Letter"));
    check(drew("Letter follows DVs; only I and V can be shiny"), "the Values note is visible");

    // Values panel, row 6 (Letter).
    d.press(HidNpadButton_R);
    for (int i = 0; i < 6; ++i) d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    check(drew("Unown letter: now A, not shiny"), "the grid states the current letter and shiny state");
    check(drew("Letter follows DVs; only I and V can be shiny (marked)"), "the grid marks the shiny letters");
    layoutOk("Letter grid");
    // A..G on row 1, H..N on row 2: Down + Right x3 = K.
    d.press(HidNpadButton_Down);
    for (int i = 0; i < 3; ++i) d.press(HidNpadButton_Right);
    quiet(screen);
    d.press(HidNpadButton_A);
    check(!drew("Unown letter: now"), "the grid closes after a pick");
    check(screen.storageStatus.empty(), "H22: picking a letter posts no toast (got \"" + screen.storageStatus + "\")");
    check(valueAfter("Letter") == "K", "picked K, got " + valueAfter("Letter"));
    check(valueAfter("Shiny") == "No", "K stays not shiny");

    // Shiny on K: asked first; B cancels with nothing changed.
    d.press(HidNpadButton_Up);     // row 5, Shiny
    d.press(HidNpadButton_A);
    check(drew("Shiny Unown can only be I and V. Change letter to"), "Shiny on K asks before changing the letter");
    layoutOk("Shiny question");
    d.press(HidNpadButton_B);
    check(valueAfter("Letter") == "K" && valueAfter("Shiny") == "No", "B cancels: still K, not shiny");
    check(screen.storageStatus == "Unown unchanged", "B on the question still says nothing changed");
    d.press(HidNpadButton_A);
    quiet(screen);
    d.press(HidNpadButton_A);      // confirm
    check(screen.storageStatus.empty(), "H22: a confirmed Shiny letter change posts no toast (got \"" +
                                            screen.storageStatus + "\")");
    const std::string shinyLetter = valueAfter("Letter");
    check(valueAfter("Shiny") == "Yes" && (shinyLetter == "I" || shinyLetter == "V"),
          "A confirms: shiny I or V, got " + shinyLetter);

    // Reroll while shiny: different DVs, same letter and shiny, and no toast (H22).
    const std::string shinyDvs = dvsShown();
    quiet(screen);
    d.press(HidNpadButton_Y);
    check(screen.storageStatus.empty() && !drew("DVs re-rolled"),
          "H22: a shiny Reroll posts no toast (got \"" + screen.storageStatus + "\")");
    check(dvsShown() != shinyDvs, "shiny Reroll changes the DVs shown (" + shinyDvs + " -> " + dvsShown() + ")");
    check(valueAfter("Letter") == shinyLetter && valueAfter("Shiny") == "Yes", "shiny Reroll keeps letter and shiny");

    // Picking K while shiny asks to turn Shiny off; A applies K, not shiny.
    d.press(HidNpadButton_Down);   // row 6, Letter
    d.press(HidNpadButton_A);
    check(drew("Unown letter: now " + shinyLetter + ", shiny"), "the grid shows the shiny state");
    // From I (8: row 2 col 2) or V (21: row 4 col 1) to K (10: row 2 col 4).
    if (shinyLetter == "I") { d.press(HidNpadButton_Right); d.press(HidNpadButton_Right); }
    else { d.press(HidNpadButton_Up); for (int i = 0; i < 3; ++i) d.press(HidNpadButton_Right); }
    d.press(HidNpadButton_A);
    check(drew("Unown K can't be shiny. Turn Shiny off and use K?"), "a non-shiny letter while shiny is asked");
    layoutOk("Letter question");
    check(valueAfter("Shiny") == "Yes", "nothing changes before the answer");
    quiet(screen);
    d.press(HidNpadButton_A);
    check(valueAfter("Letter") == "K" && valueAfter("Shiny") == "No", "A: Shiny off, K");
    check(screen.storageStatus.empty(), "H22: an applied letter posts no toast (got \"" + screen.storageStatus + "\")");

    // Reroll while not shiny.
    const std::string plainDvs = dvsShown();
    quiet(screen);
    d.press(HidNpadButton_Y);
    check(screen.storageStatus.empty() && !drew("DVs re-rolled"),
          "H22: Reroll posts no toast (got \"" + screen.storageStatus + "\")");
    check(dvsShown() != plainDvs, "Reroll changes the DVs shown (" + plainDvs + " -> " + dvsShown() + ")");
    check(valueAfter("Letter") == "K", "Reroll keeps K");

    // Keep: the stored record (Box sprite key pokemon->form()) and Review agree with the DVs.
    d.press(HidNpadButton_X);
    const auto* stored = screen.trainer.boxes[1][0].get();
    check(stored && stored->speciesID() == 201 && stored->form() == 10, "stored Unown form (Box sprite key) is K");
    if (stored) {
        const auto* gen2 = static_cast<const ::Pokemon::Pokemon2ReadOnly*>(stored);
        const auto& r = gen2->strictRecord();
        check(PokeBank::UIModel::Gen12DVReroll::unownLetter(PokeBank::UIModel::Gen12DVReroll::DVs{r.dvs[1], r.dvs[2], r.dvs[3], r.dvs[4]}) == stored->form(),
              "the stored form equals the letter its DVs give");
    }
    screen.detailViewActive = false;
    d.frame();
    check(screen.openStagedReview(false), "Review opens");
    d.frame();
    check(drew("Unown K"), "Review names Unown K");

    if (failures) {
        std::cerr << failures << " of " << checks << " checks failed\n";
        return 1;
    }
    std::cout << "Gen II Unown Letter grid, Shiny questions and silent Reroll through the real input (" << checks
              << " checks): PASS\n";
    return 0;
}
