// Round 6 L06 / 7D through the REAL TrainerViewScreen (tests/host-ui harness, Lane S): the Gen III
// Held Item picker commits the item drawn in the cell a finger taps (top, middle, bottom rows of the
// grid) and the item A commits after D-pad navigation; a completed tap outside the picker, the Party
// Actions popup or Help closes it like B and activates nothing underneath. The Gen III fixture and
// the Party -> Edit route are the ones tests/test_ui_editor_input_host.cpp uses.
#include "HostUi.h"

#include "Encryption/Encryption3FRLG.h"
#include "Integration/Gen3/Gen3HeldItems.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Names/ItemNames.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace G3 = PokeVault::Integration::Gen3;
namespace D3 = PokeVault::Integration::Gen3::Detail;

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
    void frame(u64 down = 0, u64 held = 0) {
        PadState pad{down, held | down, 0};
        touch.update();
        screen.update(pad, touch);
        HostUi::texts.clear();
        screen.draw(fb);
    }
    void press(u64 button) {
        frame(button);
        frame();
    }
    const UI::TrainerViewScreen::TouchButton* target(int id) const {
        for (const auto& b : screen.touchButtons)
            if (b.id == id) return &b;
        return nullptr;
    }
    std::vector<int> targets(int first, int end) const {
        std::vector<int> ids;
        for (const auto& b : screen.touchButtons)
            if (b.id >= first && b.id < end) ids.push_back(b.id);
        std::sort(ids.begin(), ids.end());
        return ids;
    }
    // A finger down at (x, y), a few pixels of wobble, then up.
    void tapAt(int x, int y, int dy = 0) {
        HostUi::touch = {true, static_cast<u32>(x), static_cast<u32>(y)};
        frame();
        HostUi::touch = {true, static_cast<u32>(x), static_cast<u32>(y + dy)};
        frame();
        HostUi::touch = {false, static_cast<u32>(x), static_cast<u32>(y + dy)};
        frame();
    }
    bool tap(int id, int dy = 0) {
        const auto* b = target(id);
        if (!b) return false;
        tapAt(b->x + b->w / 2, b->y + b->h / 2, dy);
        return true;
    }
};

// ---- Gen III (FireRed): one 100-byte party Pikachu in the active rotated slot, the record shape
// test_gen3_staged_pokemon_editor builds (Pokemon3FRLG + Encryption3FRLG, sector checksums). ----
void put16(std::span<uint8_t> bytes, std::size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}
void put32(std::span<uint8_t> bytes, std::size_t offset, uint32_t value) {
    put16(bytes, offset, static_cast<uint16_t>(value));
    put16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
}
std::vector<uint8_t> gen3Fixture(uint16_t heldItem) {
    using Sectors = std::array<std::array<uint8_t, D3::kSectorSize>, D3::kSectorCount>;
    Sectors logical{};
    constexpr uint32_t key = 0xA1B2C3D4;
    put32(logical[0], 0xAC, 1);
    put32(logical[0], 0xF20, key);
    put32(logical[1], 0x0290, key);
    put16(logical[0], 0x0A, 12345);
    put16(logical[0], 0x0C, 54321);
    std::vector<uint8_t> save(D3::kSaveSize, 0);
    for (uint8_t slot = 0; slot < 2; ++slot) {
        const uint8_t rotation = slot == 0 ? 0 : 5;
        const uint32_t counter = slot == 0 ? 7 : 9;
        for (std::size_t physical = 0; physical < D3::kSectorCount; ++physical) {
            const uint16_t id = static_cast<uint16_t>((physical + rotation) % D3::kSectorCount);
            const std::size_t offset = D3::kSlotBases[slot] + physical * D3::kSectorSize;
            std::copy(logical[id].begin(), logical[id].end(), save.begin() + static_cast<std::ptrdiff_t>(offset));
            put16(save, offset + 0xFF4, id);
            put16(save, offset + 0xFF6, D3::sectorChecksum(std::span<const uint8_t>(save).subspan(offset, D3::kChunkLengths[id])));
            put32(save, offset + 0xFF8, D3::kSectorSignature);
            put32(save, offset + 0xFFC, counter);
        }
    }
    const auto game = G3::SourceGame::FireRedGBA;
    const auto active = D3::validateSlot(save, 1);
    const auto sector = active.logicalSectorOffsets[1];
    const auto body = sector + D3::partyDataOffset(game);
    save[sector + D3::partyCountOffset(game)] = 1;
    std::array<uint8_t, 100> blank{};
    Pokemon::Pokemon3FRLG p(std::span<const std::byte>(reinterpret_cast<const std::byte*>(blank.data()), blank.size()));
    p.setPID(0x12345678u);
    p.setTID16(12345);
    p.setSID16(54321);
    p.setSpecies(25);
    p.setLanguage(2);
    p.setOTName(u"RED");
    p.setNickname(u"PIKACHU");
    p.setHeldItem(heldItem);
    p.setFriendship(70);
    p.setOriginGame(4);
    p.setBall(4);
    p.setMetLevel(20);
    p.setMetLocation(1);
    p.setLevel(20);
    p.setMove(0, 33);
    p.setMovePP(0, 35);
    p.recalculateStats();
    p.setStatHPCurrent(p.statHPMax());
    auto* encrypted = Encryption::encryptArray3FRLG(p.getData());
    std::memcpy(save.data() + body, encrypted, 100);
    delete[] encrypted;
    put16(save, sector + 0xFF6, D3::sectorChecksum(std::span<const uint8_t>(save).subspan(sector, D3::kChunkLengths[1])));
    return save;
}

struct Gen3Screen {
    std::vector<uint8_t> bytes;
    std::unique_ptr<PokeVault::Legacy::FRLGReadOnlyTrainer> trainer;
    std::unique_ptr<UI::TrainerViewScreen> screen;
};
// Screens stay alive for the whole run: the generation surfaces key their per-screen state by the
// screen's address, so a later screen must never reuse an earlier one's.
std::vector<Gen3Screen> gen3Screens;
Gen3Screen& openGen3(uint16_t heldItem) {
    auto& out = gen3Screens.emplace_back();
    out.bytes = gen3Fixture(heldItem);
    auto parsed = G3::parse(out.bytes, G3::SourceGame::FireRedGBA);
    check(static_cast<bool>(parsed), "Gen III fixture parses: " + parsed.detail);
    if (!parsed) return out;
    std::string error;
    out.trainer = PokeVault::Legacy::FRLGReadOnlyTrainer::create(*parsed.save, error);
    check(out.trainer != nullptr, "Gen III trainer opens: " + error);
    if (!out.trainer) return out;
    out.screen = std::make_unique<UI::TrainerViewScreen>(*out.trainer, "Pokemon FireRed", "/host/firered.sav", 0,
        AccountUid{}, PokeVault::Safety::SourceKind::RetroArchLegacy, "firered_gba", "mGBA");
    return out;
}

// The value the editor draws for a Details label (label then value, in draw order).
std::string drawnValue(const std::string& label) {
    for (size_t i = 0; i + 1 < HostUi::texts.size(); ++i)
        if (HostUi::texts[i] == label) return HostUi::texts[i + 1];
    return "<" + label + " not drawn>";
}

// Party A -> the classic Party Actions popup -> a real tap on its Edit row -> the Gen III editor.
bool openGen3PartyEdit(Driver& d) {
    d.frame();
    d.screen.detailViewActive = true;
    d.screen.selectedMode = UI::TrainerViewScreen::ViewMode::Party;
    d.screen.selectedPartyIndex = 0;
    d.frame();
    d.press(HidNpadButton_A);
    namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
    const auto list = Shared::actionsForSlot(true, {false, false, true}, false);
    int edit = -1;
    for (size_t i = 0; i < list.count; ++i)
        if (list[i] == Shared::Action::Edit) edit = static_cast<int>(i);
    check(edit >= 0 && d.target(89000 + edit) != nullptr, "Gen III Party A opens the Party Actions popup with Edit");
    if (edit < 0 || !d.tap(89000 + edit)) return false;
    return true;
}

// The native Gen III held-item list the picker offers, in its default (native) order.
std::vector<uint16_t> gen3HeldItems() {
    std::vector<uint16_t> ids;
    for (std::size_t id = 0; id < Names::getItemCountG3(); ++id) {
        const char* name = Names::getItemNameG3(static_cast<uint16_t>(id));
        if (G3::releasedHeldItem(static_cast<uint16_t>(id)) && (id == 0 || (name && *name && std::string(name) != "???")))
            ids.push_back(static_cast<uint16_t>(id));
    }
    return ids;
}

constexpr int kGen3DetailTouchBase = 31000, kHeldItemRow = 7, kCellBase = 4000;

bool openHeldItemPicker(Driver& d) {
    for (int i = 0; i < 20 && !d.target(kGen3DetailTouchBase + kHeldItemRow); ++i) d.press(HidNpadButton_Down);
    if (!d.tap(kGen3DetailTouchBase + kHeldItemRow)) return false;
    return !d.targets(kCellBase, kCellBase + 1000).empty();
}

void gen3HeldItemTapAndA() {
    const auto items = gen3HeldItems();
    // Tap: the top, a middle and the bottom published cell, each with a few pixels of wobble.
    for (const int which : {0, 1, 2}) {
        auto& g3 = openGen3(13);
        if (!g3.screen) return;
        Driver d{*g3.screen, {}, {}};
        if (!openGen3PartyEdit(d)) return;
        check(openHeldItemPicker(d), "Gen III: a tap on Held Item opens the grid with published cells");
        // Scroll the grid down a few pages first, so the rows are not the list's first ones.
        for (int i = 0; i < 2; ++i) d.press(HidNpadButton_R);
        const auto cells = d.targets(kCellBase, kCellBase + 1000);
        check(!cells.empty(), "Gen III Held Item grid publishes cells");
        if (cells.empty()) return;
        const int id = which == 0 ? cells.front() : which == 1 ? cells[cells.size() / 2] : cells.back();
        const auto item = items[static_cast<std::size_t>(id - kCellBase)];
        check(d.tap(id, which == 1 ? 9 : -6), "Gen III Held Item cell tapped");
        check(d.targets(kCellBase, kCellBase + 1000).empty(), "Gen III Held Item tap closes the picker");
        check(drawnValue("Held Item") == Names::getItemNameG3(item),
              "Gen III Held Item tap commits the tapped cell's item " + std::string(Names::getItemNameG3(item)) +
              " (got " + drawnValue("Held Item") + ")");
    }
    // A: from Potion (13), three rows down and one right, then A.
    auto& g3 = openGen3(13);
    if (!g3.screen) return;
    Driver d{*g3.screen, {}, {}};
    if (!openGen3PartyEdit(d)) return;
    check(openHeldItemPicker(d), "Gen III Held Item picker reopens");
    const auto start = static_cast<int>(std::find(items.begin(), items.end(), uint16_t{13}) - items.begin());
    for (int i = 0; i < 3; ++i) d.press(HidNpadButton_Down);
    d.press(HidNpadButton_Right);
    d.press(HidNpadButton_A);
    const auto item = items[static_cast<std::size_t>(start + 3 * 4 + 1)];
    check(drawnValue("Held Item") == Names::getItemNameG3(item),
          "Gen III Held Item A commits the focused item " + std::string(Names::getItemNameG3(item)) +
          " (got " + drawnValue("Held Item") + ")");
}

void gen3PickerTapOutside() {
    auto& g3 = openGen3(13);
    if (!g3.screen) return;
    Driver d{*g3.screen, {}, {}};
    if (!openGen3PartyEdit(d)) return;
    check(openHeldItemPicker(d), "Gen III Held Item picker opens");
    const int prompts = HostUi::promptCount;
    // Left of the 1040 px grid modal (x 120..1160), over the editor's Details column.
    d.tapAt(40, 300);
    check(d.targets(kCellBase, kCellBase + 1000).empty(), "Gen III: a tap outside closes the Held Item picker");
    check(drawnValue("Held Item") == Names::getItemNameG3(13), "Gen III: the outside tap changes no held item");
    check(d.target(kGen3DetailTouchBase + kHeldItemRow) != nullptr, "Gen III: the editor stays open underneath");
    check(HostUi::promptCount == prompts, "Gen III: the outside tap opens nothing underneath");
    d.frame();
    d.frame();
    check(d.targets(kCellBase, kCellBase + 1000).empty(), "Gen III: no later frame reacts to the dismissing tap");
}

void partyActionsAndHelpTapOutside() {
    auto& g3 = openGen3(13);
    if (!g3.screen) return;
    Driver d{*g3.screen, {}, {}};
    d.frame();
    d.screen.detailViewActive = true;
    d.screen.selectedMode = UI::TrainerViewScreen::ViewMode::Party;
    d.screen.selectedPartyIndex = 0;
    d.frame();
    d.press(HidNpadButton_A);
    check(!d.targets(89000, 89100).empty(), "Party A opens the Party Actions popup");
    d.tapAt(30, 120);
    check(d.targets(89000, 89100).empty(), "a tap outside closes the Party Actions popup");
    check(d.target(kGen3DetailTouchBase + kHeldItemRow) == nullptr, "the outside tap opens no editor");
    d.press(HidNpadButton_Minus);
    check(d.screen.helpOverlayActive, "Minus opens Help");
    d.tapAt(20, 40);
    check(!d.screen.helpOverlayActive, "a tap outside Help closes it");
    check(d.targets(89000, 89100).empty(), "the Help-dismissing tap opens nothing underneath");
}

} // namespace

int main() {
    gen3HeldItemTapAndA();
    gen3PickerTapOutside();
    partyActionsAndHelpTapOutside();
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "L06 Held Item / 7D outside taps through the real TrainerViewScreen (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
