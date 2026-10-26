// Round 6 7D through the REAL TrainerViewScreen (tests/host-ui harness): in Gen I (Red) the Species
// picker and in Gen II (Silver) the Move picker close on a completed tap outside them, like B, and
// the tap reaches nothing underneath (no prompt opens, the editor stays open). Routes are real
// input from the Boxes page: A on an empty slot, A to Create, then the editor's own keys.
// Fixtures: the Red and Silver saves tests/test_ui_layout_host.cpp uses.
#include "HostUi.h"
#include "fuzz_synthetic_saves.h"
#include "fixtures/gsc_pokemon_fixture.h"

#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
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
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

std::vector<std::shared_ptr<void>> keepAlive;
template <class T>
T* keep(std::unique_ptr<T> p) {
    T* raw = p.get();
    keepAlive.push_back(std::shared_ptr<void>(std::move(p)));
    return raw;
}
UI::TrainerViewScreen* open(Trainer::Trainer& trainer, const char* title, const char* id) {
    return keep(std::make_unique<UI::TrainerViewScreen>(trainer, title, std::string("/host/") + id + ".sav", 0,
        AccountUid{}, PokeVault::Safety::SourceKind::RetroArchLegacy, id, "mGBA"));
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
    void press(u64 button) {
        frame(button);
        frame();
    }
    void tapAt(int x, int y) {
        HostUi::touch = {true, static_cast<u32>(x), static_cast<u32>(y)};
        frame();
        HostUi::touch = {true, static_cast<u32>(x + 2), static_cast<u32>(y + 1)};
        frame();
        HostUi::touch = {false, static_cast<u32>(x + 2), static_cast<u32>(y + 1)};
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
    // Boxes page, empty slot 1: A opens the empty-slot menu, A starts the Create draft.
    void openCreate() {
        frame();
        screen.detailViewActive = true;
        screen.selectedMode = UI::TrainerViewScreen::ViewMode::Boxes;
        screen.selectedBoxIndex = 0;
        screen.selectedItemIndex = 1;
        frame();
        press(HidNpadButton_A);
        press(HidNpadButton_A);
        dump("Create");
    }
};

// A tap outside the picker: left of every picker frame, above the footer band.
constexpr int kOutsideX = 30, kOutsideY = 330;

void redSpeciesPicker() {
    auto bytes = std::make_shared<std::vector<uint8_t>>(qasynth::gen1::save());
    keepAlive.push_back(bytes);
    qasynth::gen1::forge(*bytes);
    auto parsed = PokeVault::Integration::Gen1::parse(*bytes, PokeVault::Integration::Gen1::SourceGame::Red);
    check(static_cast<bool>(parsed), "Gen I fixture parses");
    if (!parsed) return;
    std::string error;
    auto* trainer = keep(PokeVault::Legacy::RBYReadOnlyTrainer::create(*parsed.save, error));
    check(trainer != nullptr, "Gen I trainer: " + error);
    if (!trainer) return;
    Driver d{*open(*trainer, "Pokemon Red", "red_gb"), {}, {}};
    d.openCreate();
    d.press(HidNpadButton_A);   // the Create editor opens on Species: A opens its picker
    d.dump("Red after A");
    check(d.drawn("Choose Species"), "Red: A on Species opens the Species picker");
    const int prompts = HostUi::promptCount;
    d.tapAt(kOutsideX, kOutsideY);
    d.dump("Red after outside tap");
    check(!d.drawn("Choose Species"), "Red: a tap outside closes the Species picker");
    check(HostUi::promptCount == prompts, "Red: the outside tap opens nothing underneath");
    d.frame();
    check(!d.drawn("Choose Species"), "Red: no later frame reopens the picker");
    // A tap inside the picker frame (its title bar) leaves it open.
    d.press(HidNpadButton_A);
    check(d.drawn("Choose Species"), "Red: A reopens the Species picker");
    d.tapAt(640, 130);
    check(d.drawn("Choose Species"), "Red: a tap inside the picker does not close it");
}

void silverMovePicker() {
    auto bytes = std::make_shared<std::vector<uint8_t>>(fixture(GS, true));
    keepAlive.push_back(bytes);
    auto parsed = parse(*bytes, SourceGame::Silver);
    check(static_cast<bool>(parsed), "Gen II fixture parses");
    if (!parsed) return;
    std::string error;
    auto* trainer = keep(PokeVault::Legacy::GSCReadOnlyTrainer::create(*parsed.save, error));
    check(trainer != nullptr, "Gen II trainer: " + error);
    if (!trainer) return;
    Driver d{*open(*trainer, "Pokemon Silver", "silver_gbc"), {}, {}};
    d.openCreate();
    // R reaches the Moves panel; A on a move opens its move editor, A on its Move row the picker.
    bool opened = false;
    for (int panel = 0; panel < 3 && !opened; ++panel) {
        d.press(HidNpadButton_R);
        d.press(HidNpadButton_A);
        d.dump("Silver panel " + std::to_string(panel) + " after A");
        opened = d.drawn("Move \xE2\x80\x94 Generation II");
        if (!opened) {
            d.press(HidNpadButton_A);
            d.dump("Silver panel " + std::to_string(panel) + " after A A");
            opened = d.drawn("Move \xE2\x80\x94 Generation II");
        }
        if (!opened) { d.press(HidNpadButton_B); d.press(HidNpadButton_B); }
    }
    d.dump("Silver move picker");
    check(opened, "Silver: A on a move row opens the Move picker");
    if (!opened) return;
    const int prompts = HostUi::promptCount;
    d.tapAt(kOutsideX, kOutsideY);
    d.dump("Silver after outside tap");
    check(!d.drawn("Move \xE2\x80\x94 Generation II"), "Silver: a tap outside closes the Move picker");
    check(HostUi::promptCount == prompts, "Silver: the outside tap opens nothing underneath");
    d.frame();
    check(!d.drawn("Move \xE2\x80\x94 Generation II"), "Silver: no later frame reopens the picker");
}

} // namespace

int main() {
    redSpeciesPicker();
    silverMovePicker();
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "7D Gen I Species / Gen II Move pickers close on an outside tap through the real "
                 "TrainerViewScreen (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
