// Round 6 L06 (owner hardware, Oct 25): in Gen III and Gen V, Pokérus did not respond; Gen II / IV
// worked. This test drives the REAL TrainerViewScreen::update / draw (tests/host-ui harness): A on a
// Party slot, the Actions menu to Edit, Down to the Pokérus row, then A, and separately a real
// finger down / up on that row's published touch target. Both must toggle the same staged value.
#include "HostUi.h"

#include "Encryption/Encryption3FRLG.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "UI/SharedPokemonEditorContract.h"

#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Names/ItemNames.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SharedPokemonEditorWorkspace.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/CRC16.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace G5 = PokeVault::Integration::Gen5;
namespace C5 = PokeVault::Integration::Gen5::Crypto;
namespace L5 = PokeVault::Integration::Gen5::LayoutInfo;
namespace Workspace = PokeBank::UIModel::SharedPokemonWorkspace;
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

// One device frame: input -> update -> draw (the draw publishes the next frame's touch targets).
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
    // A clean tap: finger down on the target's centre, then up at the same point.
    bool tap(int id) {
        const auto* b = target(id);
        if (!b) return false;
        const auto x = static_cast<u32>(b->x + b->w / 2), y = static_cast<u32>(b->y + b->h / 2);
        HostUi::touch = {true, x, y};
        frame();
        HostUi::touch = {false, x, y};
        frame();
        return true;
    }
};

// A Black save with one party Pikachu (the Gen V Bag-screen fixture shape).
std::vector<uint8_t> gen5Fixture() {
    std::vector<uint8_t> sav(L5::FullSaveSize, 0);
    // An established trainer "NX" (TID 12345 / SID 54321, 12:34:56), as test_gen5_trainer_footer.
    const size_t player = 0x19400;
    C5::write16(sav, player + 0x04, 'N');
    C5::write16(sav, player + 0x06, 'X');
    C5::write16(sav, player + 0x08, 0xFFFF);
    C5::write16(sav, player + 0x14, 12345);
    C5::write16(sav, player + 0x16, 54321);
    C5::write16(sav, player + 0x24, 12);
    sav[player + 0x26] = 34;
    sav[player + 0x27] = 56;
    sav[player + 0x1E] = 2;
    sav[player + 0x1F] = 21;
    sav[L5::PartyOffset + 4] = 1;
    std::vector<uint8_t> party(C5::PartySize, 0);
    party[0] = 0x23; party[1] = 0x01;
    C5::write16(party, 8, 25);
    party[0x8C] = 30;
    const auto encrypted = C5::encryptCandidate(party);
    std::copy(encrypted.begin(), encrypted.end(), sav.begin() + static_cast<std::ptrdiff_t>(L5::PartyOffset + 8));
    auto copy = std::span<uint8_t>(sav).subspan(0, L5::BlackWhiteCopySize);
    for (const auto& b : L5::BlackWhite) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C5::write16(copy, b.checksumOffset, crc);
        C5::write16(copy, b.mirrorOffset, crc);
    }
    return sav;
}

void gen5Pokerus() {
    const auto input = gen5Fixture();
    std::string error;
    const auto parsed = G5::Gen5ReadOnlySave::parse(input, "black_nds", &error, G5::SaveCopySelection::Primary);
    check(parsed.has_value(), "Gen V fixture parses: " + error);
    if (!parsed) return;
    auto trainer = PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error);
    check(trainer != nullptr, "Gen V trainer opens: " + error);
    if (!trainer) return;
    UI::TrainerViewScreen screen(*trainer, "Pokemon Black", "/host/black.sav", 0, AccountUid{},
                                 PokeVault::Safety::SourceKind::ExternalLegacy, "black_nds", "DraStic");
    Driver d{screen, {}, {}};
    d.frame();
    screen.detailViewActive = true;
    screen.selectedMode = UI::TrainerViewScreen::ViewMode::Party;
    screen.selectedPartyIndex = 0;
    d.frame();

    // A on the Party slot opens the Actions menu; walk it to Edit with Down, then A.
    d.press(HidNpadButton_A);
    auto& state = trainer->sharedScreen();
    check(state.surface() == G5::Gen5SharedScreenState::Surface::Actions, "Gen V Party A opens Actions");
    const auto actions = state.availableActions(trainer->stagedPokemon());
    size_t editRow = actions.count;
    for (size_t i = 0; i < actions.count; ++i)
        if (actions[i] == PokeBank::UIModel::SharedPokemonEditor::Action::Edit) editRow = i;
    check(editRow < actions.count, "Gen V Party Actions list Edit");
    for (size_t i = 0; i < editRow; ++i) d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    check(state.surface() == G5::Gen5SharedScreenState::Surface::Edit, "Gen V Actions A on Edit opens the editor");
    if (state.surface() != G5::Gen5SharedScreenState::Surface::Edit) return;

    const auto mode = [&] {
        const auto p = state.draft().current();
        return p ? static_cast<int>(p->pokerusMode()) : -1;
    };
    check(mode() == 0, "fixture starts with no Pokérus");
    // Down to the Pokérus row (Details row 14), then A: None -> Infected. Down skips locked rows:
    // Origin (10) always; Ball (11) / Met Location (13) because this record's origin is not a
    // Gen V game. So rows 1..9, 12, 14: eleven presses.
    constexpr int kPokerusRow = 14;
    for (int i = 0; i < 11; ++i) d.press(HidNpadButton_Down);
    check(d.target(Workspace::kDetailTouchBase + kPokerusRow) != nullptr, "Down scrolled the Pokérus row into view");
    d.press(HidNpadButton_A);
    check(mode() == 2, "Gen V A on Pokérus toggles None -> Infected (got " + std::to_string(mode()) + ")");

    // A real finger down / up on the Pokérus row's published target: Infected -> Cured.
    d.press(HidNpadButton_Up);   // focus elsewhere first, so the tap itself must select the row
    const bool tapped = d.tap(Workspace::kDetailTouchBase + kPokerusRow);
    check(tapped, "the Pokérus row publishes a touch target");
    check(mode() == 1, "Gen V tap on Pokérus toggles Infected -> Cured (got " + std::to_string(mode()) + ")");
    check(HostUi::promptCount == 0, "Pokérus never opens a prompt");
}

// Gen V Create: an empty box slot -> Add -> A on "Create A Pokemon" -> the shared editor in Create.
// Pokérus must toggle by tap and by A there too (the Gen IV Create rule).
void gen5CreatePokerus() {
    const auto input = gen5Fixture();
    std::string error;
    const auto parsed = G5::Gen5ReadOnlySave::parse(input, "black_nds", &error, G5::SaveCopySelection::Primary);
    auto trainer = parsed ? PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error) : nullptr;
    check(trainer != nullptr, "Gen V Create fixture opens");
    if (!trainer) return;
    UI::TrainerViewScreen screen(*trainer, "Pokemon Black", "/host/black.sav", 0, AccountUid{},
                                 PokeVault::Safety::SourceKind::ExternalLegacy, "black_nds", "DraStic");
    Driver d{screen, {}, {}};
    d.frame();
    screen.detailViewActive = true;
    screen.selectedMode = UI::TrainerViewScreen::ViewMode::Boxes;
    d.frame();
    auto& state = trainer->sharedScreen();
    const G5::Gen5StagedPokemonWorkspace::Slot empty{G5::Gen5StagedPokemonWorkspace::Region::Box, 0, 0};
    check(state.openActions(trainer->stagedPokemon(), empty, error) &&
              state.activate(trainer->stagedPokemon(), PokeBank::UIModel::SharedPokemonEditor::Action::Add, error),
          "Gen V empty box slot offers Add: " + error);
    d.frame();
    d.press(HidNpadButton_A);
    check(state.surface() == G5::Gen5SharedScreenState::Surface::Edit && state.createPending(),
          "Gen V A on Create opens the shared editor in Create (status: " + screen.storageStatus + ")");
    if (state.surface() != G5::Gen5SharedScreenState::Surface::Edit) return;
    const auto mode = [&] {
        const auto p = state.draft().current();
        return p ? static_cast<int>(p->pokerusMode()) : -1;
    };
    constexpr int kPokerusRow = 14;
    for (int i = 0; i < 20 && !d.target(Workspace::kDetailTouchBase + kPokerusRow); ++i) d.press(HidNpadButton_Down);
    check(d.tap(Workspace::kDetailTouchBase + kPokerusRow), "Gen V Create: Pokérus row publishes a touch target");
    check(mode() == 2, "Gen V Create tap on Pokérus toggles None -> Infected (got " + std::to_string(mode()) + ")");
    d.press(HidNpadButton_A);
    check(mode() == 1, "Gen V Create A on Pokérus toggles Infected -> Cured (got " + std::to_string(mode()) + ")");
}

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

void gen3Pokerus() {
    auto& g3 = openGen3(13);
    if (!g3.screen) return;
    Driver d{*g3.screen, {}, {}};
    if (!openGen3PartyEdit(d)) return;
    constexpr int kGen3DetailTouchBase = 31000, kPokerusRow = 14;
    for (int i = 0; i < 20 && !d.target(kGen3DetailTouchBase + kPokerusRow); ++i) d.press(HidNpadButton_Down);
    check(d.target(kGen3DetailTouchBase + kPokerusRow) != nullptr, "Gen III Pokérus row scrolled into view");
    // A real tap on the row focuses it and acts like A: None -> Infected.
    check(d.tap(kGen3DetailTouchBase + kPokerusRow), "Gen III Pokérus row publishes a touch target");
    check(drawnValue("Pokerus") == "Infected", "Gen III tap on Pokérus toggles None -> Infected (got " + drawnValue("Pokerus") + ")");
    // A on the now-focused row: Infected -> Cured.
    d.press(HidNpadButton_A);
    check(drawnValue("Pokerus") == "Cured", "Gen III A on Pokérus toggles Infected -> Cured (got " + drawnValue("Pokerus") + ")");
}

// Gen III Create: Party has room, so an empty box slot's Add opens the editor in Create mode.
void gen3CreatePokerus() {
    auto& g3 = openGen3(13);
    if (!g3.screen) return;
    Driver d{*g3.screen, {}, {}};
    d.frame();
    d.screen.detailViewActive = true;
    d.screen.selectedMode = UI::TrainerViewScreen::ViewMode::Boxes;
    d.screen.selectedBoxIndex = 0;
    d.screen.selectedItemIndex = 0;   // the fixture's boxes are empty
    d.frame();
    d.press(HidNpadButton_X);         // X on an empty box slot: Add (staged Create)
    check(drawnValue("Species").find("Pikachu") != std::string::npos || drawnValue("Species").find("#") != std::string::npos,
          "Gen III X on an empty box slot opens Create (status: " + d.screen.storageStatus + ", species " + drawnValue("Species") + ")");
    constexpr int kGen3DetailTouchBase = 31000, kPokerusRow = 14;
    for (int i = 0; i < 20 && !d.target(kGen3DetailTouchBase + kPokerusRow); ++i) d.press(HidNpadButton_Down);
    check(d.tap(kGen3DetailTouchBase + kPokerusRow), "Gen III Create: Pokérus row publishes a touch target");
    check(drawnValue("Pokerus") == "Infected", "Gen III Create tap on Pokérus toggles None -> Infected (got " + drawnValue("Pokerus") + ")");
    d.press(HidNpadButton_A);
    check(drawnValue("Pokerus") == "Cured", "Gen III Create A on Pokérus toggles Infected -> Cured (got " + drawnValue("Pokerus") + ")");
}

// Ids of the picker cells the last draw published (the visible, possibly clipped, grid).
std::vector<int> cells(const Driver& d, int base) {
    std::vector<int> ids;
    for (const auto& b : d.screen.touchButtons)
        if (b.id >= base && b.id < base + 1000) ids.push_back(b.id);
    return ids;
}
// A finger drag on the grid (past the 22 px tap slop), released and settled: the grid scrolls by
// pixels and leaves a partly visible row at its bottom edge. Returns that bottom-row cell.
int dragAndPickBottomCell(Driver& d, int base, int pixels) {
    const auto ids = cells(d, base);
    if (ids.size() < 12) return -1;
    const auto* a = d.target(ids[ids.size() / 2]);
    const u32 x = static_cast<u32>(a->x + a->w / 2), y = static_cast<u32>(a->y + a->h / 2);
    HostUi::touch = {true, x, y};
    d.frame();
    for (int k = 1; k <= 4; ++k) {
        HostUi::touch = {true, x, y - static_cast<u32>(k * pixels / 4)};
        d.frame();
    }
    HostUi::touch = {false, x, y - static_cast<u32>(pixels)};
    d.frame();
    for (int k = 0; k < 3; ++k) d.frame();
    int bottom = -1, bottomY = -1;
    for (const auto& b : d.screen.touchButtons)
        if (b.id >= base && b.id < base + 1000 && b.y > bottomY) { bottom = b.id; bottomY = b.y; }
    return bottom;
}

// L06: the Gen III Held Item picker selects with A and with a tap at the top, middle and bottom of
// the list, after a search, and on the partly visible row a finger scroll leaves (it used to only
// say "press A again"). The chosen item is what Keep stages.
void gen3HeldItem() {
    auto& g3 = openGen3(13);
    if (!g3.screen) return;
    Driver d{*g3.screen, {}, {}};
    if (!openGen3PartyEdit(d)) return;
    constexpr int kHeldItemRow = 31007, kCell = 4000;
    std::string held = drawnValue("Held Item");
    check(held == "Potion", "Gen III editor shows the stored Potion (got " + held + ")");
    const auto chose = [&](const std::string& how) {
        const std::string now = drawnValue("Held Item");
        check(now != held && !now.empty() && now[0] != '<', "Gen III Held Item: " + how + " selects (" + held + " -> " + now + ")");
        check(cells(d, kCell).empty(), "Gen III Held Item: " + how + " closes the picker");
        check(d.screen.storageStatus.find("press A again") == std::string::npos, "Gen III Held Item: " + how + " needs no second press");
        held = now;
        d.screen.storageStatus.clear();
    };
    const auto open = [&] {
        const bool ok = d.tap(kHeldItemRow) && !cells(d, kCell).empty();
        check(ok, "Gen III Held Item row opens the picker by tap");
        return ok;
    };
    // Search first, while the current item (Potion) does not match "A": the results then focus a
    // different item, which A chooses (the search grid owns the D-pad while it is open).
    if (open()) {
        d.press(HidNpadButton_Minus);   // open search
        d.press(HidNpadButton_A);       // type the focused key
        d.press(HidNpadButton_B);       // back to the results
        d.press(HidNpadButton_A);
        chose("A after a search");
    }
    if (open()) {
        d.press(HidNpadButton_Minus);
        d.press(HidNpadButton_A);
        d.press(HidNpadButton_B);
        const auto ids = cells(d, kCell);
        check(ids.size() > 2 && d.tap(ids[ids.size() / 2]), "Gen III Held Item: search results publish cells");
        chose("tap after a search");
    }
    if (open()) {
        const int partial = dragAndPickBottomCell(d, kCell, 30);
        const auto* b = d.target(partial);
        check(b && b->h < 36, "a 30 px finger scroll leaves a partly visible bottom row");
        d.tap(partial);
        chose("tap on the partly visible row after a finger scroll");
    }
    if (open()) { d.tap(kCell + 3); chose("tap at the top"); }
    if (open()) { d.press(HidNpadButton_Right); d.press(HidNpadButton_Down); d.press(HidNpadButton_Down); d.press(HidNpadButton_A); chose("A in the middle"); }
    if (open()) { for (int i = 0; i < 30; ++i) d.press(HidNpadButton_R); d.press(HidNpadButton_A); chose("A at the bottom"); }
    if (open()) {
        for (int i = 0; i < 30; ++i) d.press(HidNpadButton_R);
        const auto ids = cells(d, kCell);
        check(!ids.empty() && d.tap(ids.back()), "Gen III Held Item: last page publishes cells");
        chose("tap at the bottom");
    }
    // Keep stages exactly the item shown.
    d.press(HidNpadButton_X);
    std::string error;
    const auto staged = g3.trainer->stagedPokemon()->partyPokemon(0, error);
    check(staged && Names::getItemNameG3(staged->heldItem) == held,
          "Gen III Keep stages the chosen Held Item (" + held + ")");
}

// The shared Gen IV / V picker has the same partly visible row; Gen V's Held Item must take a tap there.
void gen5HeldItemPartialRow() {
    const auto input = gen5Fixture();
    std::string error;
    const auto parsed = G5::Gen5ReadOnlySave::parse(input, "black_nds", &error, G5::SaveCopySelection::Primary);
    auto trainer = parsed ? PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error) : nullptr;
    if (!trainer) { check(false, "Gen V held-item fixture opens"); return; }
    UI::TrainerViewScreen screen(*trainer, "Pokemon Black", "/host/black.sav", 0, AccountUid{},
                                 PokeVault::Safety::SourceKind::ExternalLegacy, "black_nds", "DraStic");
    Driver d{screen, {}, {}};
    d.frame();
    screen.detailViewActive = true;
    screen.selectedMode = UI::TrainerViewScreen::ViewMode::Party;
    d.frame();
    d.press(HidNpadButton_A);
    auto& state = trainer->sharedScreen();
    const auto actions = state.availableActions(trainer->stagedPokemon());
    for (size_t i = 0; i < actions.count && actions[i] != PokeBank::UIModel::SharedPokemonEditor::Action::Edit; ++i)
        d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    if (state.surface() != G5::Gen5SharedScreenState::Surface::Edit) { check(false, "Gen V Edit opens for Held Item"); return; }
    const auto held = [&] {
        const auto p = state.draft().current();
        return p ? static_cast<int>(p->heldItem()) : -1;
    };
    const int before = held();
    constexpr int kHeldItemRow = 7;
    check(d.tap(Workspace::kDetailTouchBase + kHeldItemRow) && !cells(d, 6000).empty(), "Gen V Held Item row opens the picker by tap");
    const int partial = dragAndPickBottomCell(d, 6000, 30);
    const auto* b = d.target(partial);
    check(b && b->h < 36, "Gen V: a 30 px finger scroll leaves a partly visible bottom row");
    d.tap(partial);
    check(held() != before && cells(d, 6000).empty(),
          "Gen V tap on the partly visible Held Item row selects it (" + std::to_string(before) + " -> " + std::to_string(held()) + ")");
    check(screen.storageStatus.find("press A again") == std::string::npos, "Gen V partly visible row needs no second press");
}

} // namespace

int main() {
    gen5Pokerus();
    gen5CreatePokerus();
    gen3Pokerus();
    gen3CreatePokerus();
    gen3HeldItem();
    gen5HeldItemPartialRow();
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "L06 editor input through the real TrainerViewScreen (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
