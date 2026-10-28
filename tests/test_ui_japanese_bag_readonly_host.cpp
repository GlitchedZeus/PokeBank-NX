// Japanese Gen I Bag stays read-only through the REAL TrainerViewScreen (tests/host-ui harness).
// Owner rule: Japanese Gen I / II saves are read-only until their native writers are validated
// (Integration/Common/JapaneseEditingGate.h). On a Japanese Red / Yellow Bag, X (Add), A (Edit
// Amount), Y (Remove), + (Review, then the Bag's own Options) and Options -> Export Staged Save
// must stage and export nothing, the footer must be the read-only browse footer, and the source
// bytes stay as opened. An international Red Bag is the control: the same keys still stage.
// Fixtures: synthetic saves in the layout of tests/test_rby_staged_inventory.cpp.
#include "HostUi.h"

#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace {

using PokeVault::Integration::Gen1::RegionLayout;
using PokeVault::Integration::Gen1::SourceGame;

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

// pokered SAV layout (test_rby_staged_inventory.cpp): checksum, main length, bag, PC, party,
// current box, current box index, starter, Pikachu friendship.
struct Layout {
    std::size_t checksum, mainLength, bag, pc, party, currentBox, currentBoxIndex, starter, friendship;
};
constexpr Layout kIntl{0x3523, 0x0F8B, 0x25C9, 0x27E6, 0x2F2C, 0x30C0, 0x284C, 0x29C3, 0x271C};
constexpr Layout kJpn{0x3594, 0x0FFC, 0x25C4, 0x27DC, 0x2ED5, 0x302D, 0x2842, 0x29B9, 0x2712};

uint8_t diff8(std::span<const uint8_t> bytes) {
    uint8_t value = 0xFF;
    for (uint8_t byte : bytes) value = static_cast<uint8_t>(value - byte);
    return value;
}
// Bag: Escape Rope x3 and Repel x5 (Items pocket, quantity-editable); PC empty; no Pokemon.
std::vector<uint8_t> save(SourceGame game, const Layout& l) {
    std::vector<uint8_t> raw(0x8000, 0);
    raw[l.party] = 0; raw[l.party + 1] = 0xFF;
    raw[l.currentBox] = 0; raw[l.currentBox + 1] = 0xFF;
    raw[l.currentBoxIndex] = 2;
    raw[l.starter] = game == SourceGame::Yellow ? 0x54 : 0x99;
    raw[l.friendship] = game == SourceGame::Yellow ? 90 : 0;
    raw[l.bag] = 2;
    raw[l.bag + 1] = 29; raw[l.bag + 2] = 3;
    raw[l.bag + 3] = 30; raw[l.bag + 4] = 5;
    raw[l.bag + 5] = 0xFF;
    raw[l.pc] = 0; raw[l.pc + 1] = 0xFF;
    raw[l.checksum] = diff8(std::span<const uint8_t>(raw).subspan(0x2598, l.mainLength));
    return raw;
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
    void openBag() {
        frame();
        screen.detailViewActive = true;
        screen.selectedMode = UI::TrainerViewScreen::ViewMode::Items;
        screen.selectedCategory = 0;
        screen.selectedItemIndex = 0;
        frame();
        frame();
    }
};

struct Opened {
    std::shared_ptr<std::vector<uint8_t>> bytes;
    std::vector<uint8_t> original;
    PokeVault::Legacy::RBYReadOnlyTrainer* trainer = nullptr;
    UI::TrainerViewScreen* screen = nullptr;
};
Opened open(SourceGame game, const Layout& layout, const char* title, const char* id, RegionLayout region) {
    Opened o;
    o.bytes = std::make_shared<std::vector<uint8_t>>(save(game, layout));
    keepAlive.push_back(o.bytes);
    o.original = *o.bytes;
    auto parsed = PokeVault::Integration::Gen1::parse(*o.bytes, game);
    check(static_cast<bool>(parsed), std::string(title) + ": fixture parses");
    if (!parsed) return o;
    check(parsed.save->metadata().region == region, std::string(title) + ": fixture region");
    std::string error;
    o.trainer = keep(PokeVault::Legacy::RBYReadOnlyTrainer::create(*parsed.save, error));
    check(o.trainer != nullptr, std::string(title) + ": trainer: " + error);
    if (!o.trainer) return o;
    o.screen = keep(std::make_unique<UI::TrainerViewScreen>(*o.trainer, title, std::string("/host/") + id + ".sav", 0,
        AccountUid{}, PokeVault::Safety::SourceKind::RetroArchLegacy, id, "mGBA"));
    return o;
}

bool pending(const PokeVault::Legacy::RBYReadOnlyTrainer& trainer) {
    return trainer.stagedInventory() && trainer.stagedInventory()->hasPendingChanges();
}

void japaneseBag(SourceGame game, const char* title, const char* id) {
    const std::string who = title;
    auto o = open(game, kJpn, title, id, RegionLayout::Japanese);
    if (!o.screen) return;
    check(!o.trainer->stagedInventoryAvailable(), who + ": no staged Bag editor for a Japanese save");
    check(o.trainer->stagedInventoryUnavailableReason() == "Japanese Generation I saves are read-only",
          who + ": unavailable reason names the Japanese read-only rule (got \"" +
              o.trainer->stagedInventoryUnavailableReason() + "\")");
    check(!o.trainer->stagedPokemonAvailable(), who + ": no staged Pokemon editor either");

    Driver d{*o.screen, {}, {}};
    d.openBag();
    d.dump(who + " Bag");
    // The harness records each footer glyph and label as its own string.
    check(d.drawn("Escape Rope") && d.drawn("Repel"), who + ": the Bag still lists its items");
    check(d.drawn("Navigate") && d.drawn("Back"), who + ": the Bag shows the read-only browse footer");
    check(!d.drawn("Add Item") && !d.drawn("Remove Item") && !d.drawn("Edit Amount"),
          who + ": the Bag footer offers no Add Item / Edit Amount / Remove Item");

    // X: Add Item. A (and A again) would pick the first addable item in the picker.
    d.press(HidNpadButton_X);
    d.dump(who + " after X");
    check(!d.drawn("Add Item to"), who + ": X opens no Add Item picker");
    d.press(HidNpadButton_A);
    d.press(HidNpadButton_A);
    check(!pending(*o.trainer), who + ": X then A stages nothing");

    // A: Edit Amount, answered with 9 if a numpad opens.
    HostUi::numbers.push_back({true, 9});
    const int prompts = HostUi::promptCount;
    d.screen.selectedItemIndex = 0;
    d.press(HidNpadButton_A);
    d.dump(who + " after A");
    check(HostUi::promptCount == prompts, who + ": A opens no quantity prompt");
    check(!pending(*o.trainer), who + ": A stages nothing");
    HostUi::numbers.clear();

    // Y: Remove, confirmed with A if a confirmation opens.
    d.press(HidNpadButton_Y);
    d.dump(who + " after Y");
    check(!d.drawn("Remove "), who + ": Y opens no Remove confirmation");
    d.press(HidNpadButton_A);
    check(!pending(*o.trainer), who + ": Y then A stages nothing");

    // +: Review, else the Bag's own Options; Down + A would choose Export Staged Save.
    d.press(HidNpadButton_Plus);
    d.dump(who + " after +");
    check(!d.drawn("Export Staged Save"), who + ": + opens no Bag Options with Export Staged Save");
    check(d.drawn("Japanese Generation I saves are read-only"), who + ": + says why the Bag is read-only");
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    d.dump(who + " after + Down A");
    check(!d.drawn("Exported edited copy"), who + ": nothing is exported");
    check(!pending(*o.trainer), who + ": + / Options stage nothing");

    check(*o.bytes == o.original, who + ": the opened save bytes are unchanged");
}

void internationalBagStillEdits() {
    auto o = open(SourceGame::Red, kIntl, "Pokemon Red", "red_gb", RegionLayout::International);
    if (!o.screen) return;
    check(o.trainer->stagedInventoryAvailable(), "International Red: staged Bag editor available");
    Driver d{*o.screen, {}, {}};
    d.openBag();
    d.dump("International Red Bag");
    check(d.drawn("Add Item") && d.drawn("Edit Amount") && d.drawn("Remove Item"),
          "International Red: the Bag footer offers Add Item / Edit Amount / Remove Item");
    HostUi::numbers.push_back({true, 9});
    const int prompts = HostUi::promptCount;
    d.screen.selectedItemIndex = 0;
    d.press(HidNpadButton_A);
    d.dump("International Red after A");
    check(HostUi::promptCount == prompts + 1, "International Red: A opens the quantity prompt");
    check(pending(*o.trainer), "International Red: A stages the new amount");
    check(*o.bytes == o.original, "International Red: the opened save bytes are unchanged");
    HostUi::numbers.clear();
}

} // namespace

int main() {
    japaneseBag(SourceGame::Red, "Pokemon Red (Japanese)", "red_gb");
    japaneseBag(SourceGame::Yellow, "Pokemon Yellow (Japanese)", "yellow_gb");
    internationalBagStillEdits();
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Japanese Gen I Bag stays read-only through the real TrainerViewScreen; international "
                 "Red still stages (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
