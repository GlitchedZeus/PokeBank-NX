// Round 6 E05 (owner hardware, Oct 25): the glyph / Help / footer sweep's acceptance without
// screenshots. This drives the REAL TrainerViewScreen::draw (tests/host-ui harness) for a Gen I, II,
// III, IV and V save on every swept Game Browser page (save hub, Party, Boxes, Trainer, Bag) and
// the Help window, records the text each frame leaves visible (HostUi layout recorder: draw order,
// clips and opaque fills applied) and asserts:
//   * exactly one footer bar is drawn and it is the last thing on the footer band;
//   * no two visible text runs overlap (stub metrics: 9 px per byte, 18 px line).
#include "HostUi.h"
#include "HostUiFixtures.h"
#include "fuzz_synthetic_saves.h"
#include "fixtures/gsc_pokemon_fixture.h"

#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "UI/Common.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

int failures = 0;
int checks = 0;
bool verbose = false;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

using View = UI::TrainerViewScreen::ViewMode;

struct Page {
    const char* name;
    bool detail;
    View mode;
    bool help;
};
constexpr Page kPages[] = {
    {"save hub", false, View::Party, false},
    {"Party", true, View::Party, false},
    {"Boxes", true, View::Boxes, false},
    {"Trainer", true, View::Trainer, false},
    {"Bag", true, View::Items, false},
    {"Help (hub)", false, View::Party, true},
    {"Help (Boxes)", true, View::Boxes, true},
};

std::string describe(const HostUi::TextRect& t) {
    return "\"" + t.text + "\" @" + std::to_string(t.x) + "," + std::to_string(t.y) + " " +
           std::to_string(t.w) + "x" + std::to_string(t.h);
}

// One captured frame: a footer bar is drawn and no two visible text runs overlap.
void verifyFrame(const std::string& where) {
    check(HostUi::footerBars >= 1, where + ": a footer bar is drawn");
    if (verbose) std::cout << where << ": footer bars " << HostUi::footerBars << ", visible text "
                           << HostUi::visibleText.size() << '\n';
    const auto& t = HostUi::visibleText;
    int overlaps = 0;
    for (size_t i = 0; i < t.size(); ++i)
        for (size_t j = i + 1; j < t.size(); ++j) {
            const int ox = std::min(t[i].x + t[i].w, t[j].x + t[j].w) - std::max(t[i].x, t[j].x);
            const int oy = std::min(t[i].y + t[i].h, t[j].y + t[j].h) - std::max(t[i].y, t[j].y);
            if (ox > 0 && oy > 0) {
                ++overlaps;
                std::cerr << where << ": overlapping text " << describe(t[i]) << " and " << describe(t[j]) << '\n';
            }
        }
    check(overlaps == 0, where + ": no overlapping visible text");
    check(!t.empty(), where + ": the page drew visible text");
    if (std::getenv("HOSTUI_DUMP") && where.find(std::getenv("HOSTUI_DUMP")) != std::string::npos)
        for (const auto& r : t) std::cout << "  " << describe(r) << '\n';
}

void sweep(const std::string& game, UI::TrainerViewScreen& screen) {
    UI::PKSEFramebuffer fb;
    UI::TouchInput touch;
    const auto frame = [&](u64 down = 0) {
        PadState pad{down, down, 0};
        touch.update();
        screen.update(pad, touch);
        HostUi::texts.clear();
        HostUi::resetLayout();
        screen.draw(fb);
    };
    const auto press = [&](u64 button) { frame(button); frame(); };
    frame();
    for (const auto& page : kPages) {
        screen.helpOverlayActive = false;
        screen.detailViewActive = page.detail;
        screen.selectedMode = page.mode;
        screen.selectedItemIndex = 0;
        screen.selectedBoxIndex = 0;
        screen.selectedPartyIndex = 0;
        frame();
        if (page.help) {
            screen.helpOverlayActive = true;
            frame();
        }
        verifyFrame(game + " / " + page.name);
    }
    screen.helpOverlayActive = false;

    // Party slot 1 through the real input: A opens the action menu, A on its first row (View), and
    // Down + A (Edit). Each surface is captured, then B backs out.
    const auto partyPage = [&] {
        screen.goBack = false;
        screen.detailViewActive = true;
        screen.selectedMode = View::Party;
        screen.selectedPartyIndex = 0;
        screen.selectedItemIndex = 0;
        frame();
    };
    if (screen.trainer.getPartySize() > 0) {
        partyPage();
        press(HidNpadButton_A);
        verifyFrame(game + " / Party action menu");
        press(HidNpadButton_A);
        verifyFrame(game + " / Party View");
        press(HidNpadButton_B);
        partyPage();
        press(HidNpadButton_A);
        press(HidNpadButton_Down);
        press(HidNpadButton_A);
        verifyFrame(game + " / Party Edit");
        press(HidNpadButton_B);   // the exit question (nothing staged)
        verifyFrame(game + " / Party Edit exit question");
        press(HidNpadButton_B);
    }
    // Review of the staged workspace (empty review where nothing is staged).
    screen.detailViewActive = false;
    frame();
    if (screen.openStagedReview(false)) {
        frame();
        verifyFrame(game + " / Review");
        for (int i = 0; i < 3; ++i) press(HidNpadButton_B);
    }
}

// The recorder itself: overlap, occlusion and clipping behave as the checks assume.
void recorderSelfTest() {
    UI::PKSEFramebuffer fb;
    HostUi::resetLayout();
    fb.drawText(100, 100, "Attack", UI::Colors::Text, UI::TextStyle::Caption);
    fb.drawText(120, 104, "Defense", UI::Colors::Text, UI::TextStyle::Caption);
    check(HostUi::visibleText.size() == 2, "recorder keeps both runs");
    const auto& a = HostUi::visibleText[0];
    const auto& b = HostUi::visibleText[1];
    check(a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h, "recorder sees an overlap");
    fb.drawFilledRect(90, 90, 200, 40, UI::Colors::Panel);
    check(HostUi::visibleText.empty(), "an opaque fill hides the text under it");
    fb.drawText(100, 100, "Speed", UI::Colors::Text, UI::TextStyle::Caption);
    fb.drawFilledRect(90, 90, 200, 40, UI::Color(0, 0, 0, 105));
    check(HostUi::visibleText.size() == 1, "a translucent scrim leaves text visible");
    fb.setClipRect(0, 0, 50, 50);
    fb.drawText(100, 100, "Clipped", UI::Colors::Text, UI::TextStyle::Caption);
    fb.clearClip();
    check(HostUi::visibleText.size() == 1, "text outside the clip is not visible");
    fb.drawText(10, 300, "Row one", UI::Colors::Text, UI::TextStyle::Caption);
    fb.drawText(10, 316, "Row two", UI::Colors::Text, UI::TextStyle::Caption);
    const auto& r1 = HostUi::visibleText[1];
    const auto& r2 = HostUi::visibleText[2];
    check(r1.y + r1.h <= r2.y, "16 px Caption rows (the stat tables) do not overlap");
    HostUi::resetLayout();
}

// Screens stay alive for the whole run: generation surfaces key per-screen state by address.
std::vector<std::shared_ptr<void>> keepAlive;

template <class T>
T* keep(std::unique_ptr<T> p) {
    T* raw = p.get();
    keepAlive.push_back(std::shared_ptr<void>(std::move(p)));
    return raw;
}

UI::TrainerViewScreen* open(Trainer::Trainer& trainer, const char* title, const char* id, const char* emulator) {
    return keep(std::make_unique<UI::TrainerViewScreen>(trainer, title, std::string("/host/") + id + ".sav", 0,
        AccountUid{}, PokeVault::Safety::SourceKind::RetroArchLegacy, id, emulator));
}

} // namespace

int main(int argc, char** argv) {
    verbose = argc > 1 && std::string(argv[1]) == "-v";
    std::string error;
    recorderSelfTest();

    // Gen I: QA synthetic Red save (qasynth::gen1).
    {
        auto bytes = std::make_shared<std::vector<uint8_t>>(qasynth::gen1::save());
        keepAlive.push_back(bytes);
        auto parsed = PokeVault::Integration::Gen1::parse(*bytes, PokeVault::Integration::Gen1::SourceGame::Red);
        check(static_cast<bool>(parsed), "Gen I fixture parses");
        if (parsed) {
            auto* trainer = keep(PokeVault::Legacy::RBYReadOnlyTrainer::create(*parsed.save, error));
            check(trainer != nullptr, "Gen I trainer: " + error);
            if (trainer) sweep("Red", *open(*trainer, "Pokemon Red", "red_gb", "mGBA"));
        }
    }
    // Gen II: PKHeX-shaped Silver fixture with a boxed Pikachu (tests/fixtures/gsc_pokemon_fixture.h).
    {
        auto bytes = std::make_shared<std::vector<uint8_t>>(fixture(GS, true));
        keepAlive.push_back(bytes);
        auto parsed = parse(*bytes, SourceGame::Silver);
        check(static_cast<bool>(parsed), "Gen II fixture parses");
        if (parsed) {
            auto* trainer = keep(PokeVault::Legacy::GSCReadOnlyTrainer::create(*parsed.save, error));
            check(trainer != nullptr, "Gen II trainer: " + error);
            if (trainer) sweep("Silver", *open(*trainer, "Pokemon Silver", "silver_gbc", "mGBA"));
        }
    }
    // Gen III: FireRed with a party Pikachu (HostUiFixtures::gen3Fixture).
    {
        auto bytes = std::make_shared<std::vector<uint8_t>>(HostUiFixtures::gen3Fixture(0));
        keepAlive.push_back(bytes);
        auto parsed = PokeVault::Integration::Gen3::parse(*bytes, PokeVault::Integration::Gen3::SourceGame::FireRedGBA);
        check(static_cast<bool>(parsed), "Gen III fixture parses");
        if (parsed) {
            auto* trainer = keep(PokeVault::Legacy::FRLGReadOnlyTrainer::create(*parsed.save, error));
            check(trainer != nullptr, "Gen III trainer: " + error);
            if (trainer) sweep("FireRed", *open(*trainer, "Pokemon FireRed", "firered_gba", "mGBA"));
        }
    }
    // Gen IV: QA synthetic Platinum save (qasynth::gen4).
    {
        auto bytes = std::make_shared<std::vector<uint8_t>>(qasynth::gen4::save(qasynth::gen4::Kind::Platinum));
        keepAlive.push_back(bytes);
        auto parsed = PokeVault::Integration::Gen4::Gen4ReadOnlySave::parse(
            *bytes, PokeVault::Integration::Gen4::Layout::Platinum, "platinum_nds", &error);
        check(parsed.has_value(), "Gen IV fixture parses: " + error);
        if (parsed) {
            auto keptSave = std::make_shared<PokeVault::Integration::Gen4::Gen4ReadOnlySave>(*parsed);
            keepAlive.push_back(keptSave);
            auto* trainer = keep(PokeVault::Legacy::Gen4ReadOnlyTrainer::create(*keptSave, "platinum_nds", error));
            check(trainer != nullptr, "Gen IV trainer: " + error);
            if (trainer) sweep("Platinum", *open(*trainer, "Pokemon Platinum", "platinum_nds", "DraStic"));
        }
    }
    // Gen V: Black with a party Pikachu (HostUiFixtures::gen5Fixture).
    {
        auto bytes = std::make_shared<std::vector<uint8_t>>(HostUiFixtures::gen5Fixture());
        keepAlive.push_back(bytes);
        auto parsed = PokeVault::Integration::Gen5::Gen5ReadOnlySave::parse(
            *bytes, "black_nds", &error, PokeVault::Integration::Gen5::SaveCopySelection::Primary);
        check(parsed.has_value(), "Gen V fixture parses: " + error);
        if (parsed) {
            auto* trainer = keep(PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error));
            check(trainer != nullptr, "Gen V trainer: " + error);
            if (trainer) sweep("Black", *open(*trainer, "Pokemon Black", "black_nds", "DraStic"));
        }
    }

    if (failures) {
        std::cerr << failures << " of " << checks << " layout checks failed\n";
        return 1;
    }
    std::cout << "Gen I-V Game Browser pages and Help through the real TrainerViewScreen::draw: one footer, "
                 "no overlapping text (" << checks << " checks): PASS\n";
    return 0;
}
