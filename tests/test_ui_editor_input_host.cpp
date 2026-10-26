// Round 6 L06 (owner hardware, Oct 25): in Gen III and Gen V, Pokérus did not respond; Gen II / IV
// worked. This test drives the REAL TrainerViewScreen::update / draw (tests/host-ui harness): A on a
// Party slot, the Actions menu to Edit, Down to the Pokérus row, then A, and separately a real
// finger down / up on that row's published touch target. Both must toggle the same staged value.
#include "HostUi.h"

#include "Encryption/Encryption3FRLG.h"
#include "Integration/Gen3/Gen3HeldItems.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Enums/Ball.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "UI/SharedPokemonEditorContract.h"

#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "fuzz_synthetic_saves.h"
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
#include <new>
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
        frame();   // a finger lands at least one frame after what it aims at is drawn
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
std::vector<uint8_t> gen5Fixture(uint8_t pokerus = 0, uint16_t move1 = 0) {
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
    party[0x82] = pokerus;   // native Pokérus byte: strain << 4 | days
    C5::write16(party, 0x28, move1);   // first move slot
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

    // CG74 (owner Oct 25 P3): Review of the created Pokemon lists its values, not just the species.
    d.press(HidNpadButton_X);   // Keep
    check(state.openReviewFromBrowse(trainer->stagedPokemon()), "Gen V Review opens after Create");
    std::vector<std::string> drawn;
    for (int page = 0; page < 6; ++page) {
        d.frame();
        drawn.insert(drawn.end(), HostUi::texts.begin(), HostUi::texts.end());
        d.press(HidNpadButton_R);
    }
    const auto shown = [&](const std::string& part) {
        for (const auto& t : drawn)
            if (t.find(part) != std::string::npos) return true;
        return false;
    };
    // The shared Create Review shape (Gen I-V): "<loc> / Field" labels with the value alone.
    const auto labelled = [&](const std::string& field) {
        const std::string suffix = " / " + field;
        for (const auto& t : drawn)
            if (t.size() >= suffix.size() && t.compare(t.size() - suffix.size(), suffix.size(), suffix) == 0)
                return true;
        return false;
    };
    const auto exact = [&](const std::string& value) {
        return std::find(drawn.begin(), drawn.end(), value) != drawn.end();
    };
    for (const char* field : {"Add Pok\xC3\xA9mon", "OT / TID / SID", "Ball", "Met Location", "Nature", "Gender",
                              "Ability", "Language", "Pokerus"})
        check(labelled(field), std::string("Gen V Create Review has a \"<loc> / ") + field + "\" row");
    check(exact("Empty -> Pikachu Lv 5") || exact("Empty -> PIKACHU Lv 5"), "Gen V Create Review header value");
    check(exact("Cured"), "Gen V Create Review shows the created Pokérus value");
    check(!labelled("Held Item") && !labelled("IVs") && !labelled("EVs") && !labelled("Trainer ID"),
          "Gen V Create Review adds no default noise and one OT / TID / SID row");
    check(!shown("Create in Box: ") && !shown(" -> Cured"),
          "Gen V Create Review shows created values, not edits from the template");
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
bool openGen3PartyAction(Driver& d, PokeBank::UIModel::SharedPokemonEditor::Action action);
bool openGen3PartyEdit(Driver& d) {
    return openGen3PartyAction(d, PokeBank::UIModel::SharedPokemonEditor::Action::Edit);
}
bool openGen3PartyAction(Driver& d, PokeBank::UIModel::SharedPokemonEditor::Action action) {
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
        if (list[i] == action) edit = static_cast<int>(i);
    check(edit >= 0 && d.target(89000 + edit) != nullptr,
          std::string("Gen III Party A opens the Party Actions popup with ") + Shared::actionLabel(action));
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
// CG69: the owner's Gen III Edit symptom. Pokérus (row 14 of 16, 8 visible) sits partly visible
// at the bottom of DETAILS; the FIRST tap on it must act like A (it used to only scroll and ask
// for "press A again"). No Down press ever brings row 14 into full view here.
void gen3PokerusFirstTapOnPartialRow() {
    auto& g3 = openGen3(13);
    if (!g3.screen) return;
    Driver d{*g3.screen, {}, {}};
    if (!openGen3PartyEdit(d)) return;
    constexpr int kBase = 31000, kPokerus = 14;
    for (int i = 0; i < 20 && !d.target(kBase + 13); ++i) d.press(HidNpadButton_Down);
    check(d.target(kBase + 13) != nullptr && d.target(kBase + kPokerus) == nullptr,
          "row 13 is the last fully visible DETAILS row; Pokérus is just below");
    // A short finger drag (past the tap slop) leaves Pokérus partly visible.
    const auto* r = d.target(kBase + 10);
    const u32 x = static_cast<u32>(r->x + r->w / 2), y = static_cast<u32>(r->y + r->h / 2);
    HostUi::touch = {true, x, y};
    d.frame();
    for (int k = 1; k <= 4; ++k) { HostUi::touch = {true, x, y - static_cast<u32>(k * 7)}; d.frame(); }
    const auto* partial = d.target(kBase + kPokerus);
    HostUi::touch = {false, x, y - 28};
    d.frame();
    partial = d.target(kBase + kPokerus);
    check(partial && partial->h < 30, "a finger drag leaves the Pokérus row partly visible (h=" + std::to_string(partial ? partial->h : -1) + ")");
    check(d.tap(kBase + kPokerus), "the partly visible Pokérus row is tappable");
    check(drawnValue("Pokerus") == "Infected", "the FIRST tap on the partly visible Pokérus row toggles it (got " +
                                                   drawnValue("Pokerus") + ")");
    check(d.screen.storageStatus.find("press A again") == std::string::npos, "no second press is needed");
}

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
    if (open()) {
        for (int i = 0; i < 20; ++i) d.press(HidNpadButton_Up);   // up to the first row of the grid
        check(d.target(kCell) != nullptr && d.tap(kCell + 3), "Gen III Held Item: the first page publishes its cells");
        chose("tap at the top");
    }
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

// Round 6 state leak: the generation surfaces keep per-screen state in statics. Save B's screen
// is constructed at EXACTLY save A's address (the device builds the screen on the stack for every
// opened game); nothing from A -- open editor, dirty draft, open picker -- may carry over to B.
void gen3NoStateAcrossScreens() {
    alignas(UI::TrainerViewScreen) static unsigned char storage[sizeof(UI::TrainerViewScreen)];
    auto bytesA = gen3Fixture(13), bytesB = gen3Fixture(13);
    auto parsedA = G3::parse(bytesA, G3::SourceGame::FireRedGBA);
    auto parsedB = G3::parse(bytesB, G3::SourceGame::FireRedGBA);
    std::string error;
    auto trainerA = PokeVault::Legacy::FRLGReadOnlyTrainer::create(*parsedA.save, error);
    auto trainerB = PokeVault::Legacy::FRLGReadOnlyTrainer::create(*parsedB.save, error);
    if (!trainerA || !trainerB) { check(false, "state-leak fixtures open"); return; }
    auto* a = new (storage) UI::TrainerViewScreen(*trainerA, "Pokemon FireRed", "/host/a.sav", 0, AccountUid{},
        PokeVault::Safety::SourceKind::RetroArchLegacy, "firered_gba", "mGBA");
    {
        Driver d{*a, {}, {}};
        if (!openGen3PartyEdit(d)) return;
        for (int i = 0; i < 20 && !d.target(31014); ++i) d.press(HidNpadButton_Down);
        d.tap(31014);                       // dirty draft: Pokérus None -> Infected
        check(drawnValue("Pokerus") == "Infected", "save A: the draft is dirty");
        d.tap(31007);                       // and the Held Item picker is open
        check(!cells(d, 4000).empty(), "save A: the Held Item picker is open");
    }
    a->~TrainerViewScreen();
    auto* b = new (storage) UI::TrainerViewScreen(*trainerB, "Pokemon FireRed", "/host/b.sav", 0, AccountUid{},
        PokeVault::Safety::SourceKind::RetroArchLegacy, "firered_gba", "mGBA");
    check(static_cast<void*>(a) == static_cast<void*>(b), "save B's screen reuses save A's address");
    {
        Driver d{*b, {}, {}};
        d.frame();
        d.screen.detailViewActive = true;
        d.screen.selectedMode = UI::TrainerViewScreen::ViewMode::Boxes;
        d.screen.selectedBoxIndex = 0;
        d.screen.selectedItemIndex = 0;
        d.frame();
        check(cells(d, 4000).empty(), "save B: no picker carried over");
        check(drawnValue("Pokerus").front() == '<', "save B: no editor carried over");
        d.press(HidNpadButton_X);           // X here must be B's own Add, never A's stale Keep
        check(trainerB->stagedPokemon()->pendingChanges().empty(), "save B: nothing from save A is staged");
        check(drawnValue("Species").find("#") != std::string::npos && trainerB->stagedPokemon()->pendingChanges().empty(),
              "save B: X opens a fresh Create draft (species " + drawnValue("Species") + ")");
    }
    b->~TrainerViewScreen();
}

void gen5NoStateAcrossScreens() {
    alignas(UI::TrainerViewScreen) static unsigned char storage[sizeof(UI::TrainerViewScreen)];
    const auto input = gen5Fixture();
    std::string error;
    const auto parsed = G5::Gen5ReadOnlySave::parse(input, "black_nds", &error, G5::SaveCopySelection::Primary);
    auto trainerA = PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error);
    auto trainerB = PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error);
    if (!trainerA || !trainerB) { check(false, "Gen V state-leak fixtures open"); return; }
    auto* a = new (storage) UI::TrainerViewScreen(*trainerA, "Pokemon Black", "/host/a.sav", 0, AccountUid{},
        PokeVault::Safety::SourceKind::ExternalLegacy, "black_nds", "DraStic");
    {
        Driver d{*a, {}, {}};
        d.frame();
        a->detailViewActive = true;
        a->selectedMode = UI::TrainerViewScreen::ViewMode::Party;
        d.frame();
        d.press(HidNpadButton_A);
        auto& state = trainerA->sharedScreen();
        const auto actions = state.availableActions(trainerA->stagedPokemon());
        for (size_t i = 0; i < actions.count && actions[i] != PokeBank::UIModel::SharedPokemonEditor::Action::Edit; ++i)
            d.press(HidNpadButton_Down);
        d.press(HidNpadButton_A);
        d.tap(Workspace::kDetailTouchBase + 7);   // the Held Item picker is open in A's editor workspace
        check(!cells(d, 6000).empty(), "Gen V save A: the Held Item picker is open");
    }
    a->~TrainerViewScreen();
    auto* b = new (storage) UI::TrainerViewScreen(*trainerB, "Pokemon Black", "/host/b.sav", 0, AccountUid{},
        PokeVault::Safety::SourceKind::ExternalLegacy, "black_nds", "DraStic");
    {
        Driver d{*b, {}, {}};
        d.frame();
        b->detailViewActive = true;
        b->selectedMode = UI::TrainerViewScreen::ViewMode::Party;
        d.frame();
        d.press(HidNpadButton_A);
        auto& state = trainerB->sharedScreen();
        const auto actions = state.availableActions(trainerB->stagedPokemon());
        for (size_t i = 0; i < actions.count && actions[i] != PokeBank::UIModel::SharedPokemonEditor::Action::Edit; ++i)
            d.press(HidNpadButton_Down);
        d.press(HidNpadButton_A);
        check(state.surface() == G5::Gen5SharedScreenState::Surface::Edit, "Gen V save B opens its own editor");
        check(cells(d, 6000).empty(), "Gen V save B: A's open picker did not carry over");
    }
    b->~TrainerViewScreen();
}

// QA's Gen V lead: Keep replays the 0 / 1 / 2 Pokérus mode against each record's own baseline
// byte. A record that already stores strain 2 / 1 day (Infected, 0x21) is cycled, kept and
// reviewed: the visible change must survive Keep and appear in Review, and cycling back to the
// stored mode restores the exact stored byte.
void gen5PokerusKeepWithStoredStrain() {
    const auto input = gen5Fixture(0x21);
    std::string error;
    const auto parsed = G5::Gen5ReadOnlySave::parse(input, "black_nds", &error, G5::SaveCopySelection::Primary);
    auto trainer = parsed ? PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error) : nullptr;
    if (!trainer) { check(false, "Gen V stored-strain fixture opens: " + error); return; }
    UI::TrainerViewScreen screen(*trainer, "Pokemon Black", "/host/black.sav", 0, AccountUid{},
                                 PokeVault::Safety::SourceKind::ExternalLegacy, "black_nds", "DraStic");
    Driver d{screen, {}, {}};
    const auto openEdit = [&] {
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
        for (int i = 0; i < 20 && !d.target(Workspace::kDetailTouchBase + 14); ++i) d.press(HidNpadButton_Down);
        return state.surface() == G5::Gen5SharedScreenState::Surface::Edit;
    };
    auto& state = trainer->sharedScreen();
    const auto staged = [&] {
        const auto p = trainer->stagedPokemon().viewParty(0);
        return p ? static_cast<int>(p->pokerusState()) : -1;
    };
    check(openEdit() && state.draft().current()->pokerusMode() == 2, "Gen V stored strain opens as Infected");
    d.tap(Workspace::kDetailTouchBase + 14);                      // Infected -> Cured
    check(state.draft().current()->pokerusMode() == 1, "Gen V tap: Infected -> Cured");
    d.press(HidNpadButton_X);                                      // Keep
    check(staged() == 0x20, "Gen V Keep stages Cured, keeping the stored strain (got " + std::to_string(staged()) + ")");
    check(state.openReviewFromBrowse(trainer->stagedPokemon()), "Gen V Review opens after Keep");
    d.frame();
    bool listed = false;
    for (const auto& text : HostUi::texts)
        if (text.find("Cured") != std::string::npos) listed = true;
    check(listed, "Gen V Review names the kept Pokérus change (Infected -> Cured)");
    d.press(HidNpadButton_B);
    // Back to Infected in a new draft: the exact stored byte (strain 2, 1 day) comes back.
    check(openEdit(), "Gen V editor reopens");
    d.tap(Workspace::kDetailTouchBase + 14);                      // Cured -> None
    d.tap(Workspace::kDetailTouchBase + 14);                      // None -> Infected (the stored mode)
    d.press(HidNpadButton_X);
    check(staged() == 0x21, "Gen V cycling back restores the exact stored byte (got " + std::to_string(staged()) + ")");
}

// CG71 (owner Oct 25 Gen III held-item decision). Caught Ball, held item and Bag are separate
// rules: Safari Ball (5) is refused as a NEW held item (PKHeX ItemStorage3RS.Unreleased) yet stays a
// valid caught Ball (1-12); a record that already holds it keeps it, labelled honestly.
void gen3SafariBallRoles() {
    namespace H = PokeVault::Integration::Gen3;
    check(H::nativeHeldItemSupported(5) && !H::releasedHeldItem(5) && H::storedUnreleasedHeldItem(5),
          "Safari Ball is a native held-item value but not a released held item");
    check(!H::releasedHeldItem(44) && H::storedUnreleasedHeldItem(44), "Berry Juice: same rule");
    check(H::releasedHeldItem(13) && !H::storedUnreleasedHeldItem(13), "Potion stays a released held item");
    const auto balls = Enums::getBallList(Enums::GameVersion::FRLG);
    check(balls.size() == 12 && balls[4] == 5, "Gen III caught Balls are 1-12, Safari Ball (5) included");

    auto& g3 = openGen3(5);   // a stored record that already holds a Safari Ball
    if (!g3.screen) return;
    Driver d{*g3.screen, {}, {}};
    if (!openGen3PartyEdit(d)) return;
    check(drawnValue("Held Item").rfind("Safari Ball (", 0) == 0,
          "a stored Safari Ball held item is labelled honestly (got " + drawnValue("Held Item") + ")");
    // The Held Item picker never offers it, explains why, and the stored item is not silently deleted.
    d.tap(31007);
    check(d.screen.storageStatus.find("unreleased as a held item") != std::string::npos,
          "opening the picker explains the unreleased held item (status: " + d.screen.storageStatus + ")");
    bool offered = false;
    bool keptLabelled = false;
    const auto scan = [&] {
        for (const auto& t : HostUi::texts) {
            if (t == "Safari Ball") offered = true;                       // a plain, choosable cell
            if (t == "Safari Ball (unreleased)") keptLabelled = true;      // the stored value, kept
        }
    };
    scan();
    d.press(HidNpadButton_L);   // the first page, where an id-5 item would sit by number
    scan();
    for (int i = 0; i < 30; ++i) d.press(HidNpadButton_R);   // the last page, where the kept value sits
    scan();
    check(!offered, "the Held Item picker never offers Safari Ball as a plain choice");
    check(keptLabelled, "the stored Safari Ball is listed only as the labelled, kept value");
    d.press(HidNpadButton_B);
    check(drawnValue("Held Item").rfind("Safari Ball (", 0) == 0, "closing the picker keeps the stored item");
    // As a caught Ball, Safari is a normal choice: Ball picker -> Safari Ball -> Keep.
    for (int i = 0; i < 20 && !d.target(31011); ++i) d.press(HidNpadButton_Down);
    check(d.tap(31011) && d.target(4000 + 4) != nullptr, "the Ball picker lists Safari Ball (5th)");
    // The picker's FIRST draw (its viewport still unset) publishes the same cells as the next one:
    // it used to start at row -1 (reading pickerValues[-1], caught by ASan) and shift every hitbox.
    const int firstFrameY = d.target(4000 + 4) ? d.target(4000 + 4)->y : -1;
    d.frame();
    check(d.target(4000 + 4) && d.target(4000 + 4)->y == firstFrameY, "a just-opened picker's hitboxes do not shift");
    d.tap(4000 + 4);
    check(drawnValue("Ball") == "Safari Ball", "Safari Ball is chosen as the caught Ball (got " + drawnValue("Ball") + ")");
    d.press(HidNpadButton_X);
    std::string error;
    const auto staged = g3.trainer->stagedPokemon()->partyPokemon(0, error);
    check(staged && staged->ball == 5, "Keep stages Safari Ball as the caught Ball");
    check(staged && staged->heldItem == 5, "Keep leaves the stored Safari Ball held item in place");
    bool heldRow = false;
    for (const auto& c : g3.trainer->stagedPokemon()->pendingChanges())
        if (c.label.find("Held Item") != std::string::npos) heldRow = true;
    check(!heldRow, "Review lists no Held Item change (nothing was rewritten)");
    // A NEW held item can never be Safari Ball / Berry Juice: the backend refuses it.
    auto copy = PokeVault::Integration::Gen3::StagedPokemonEditor::create(g3.bytes, G3::SourceGame::FireRedGBA, error);
    const auto heldEdit = [](uint16_t item) {
        PokeVault::Integration::Gen3::BoxPokemonEdit e;
        e.heldItem = item;
        return e;
    };
    const auto ballEdit = [](uint8_t ball) {
        PokeVault::Integration::Gen3::BoxPokemonEdit e;
        e.ball = ball;
        return e;
    };
    check(copy && !copy->stagePartyPokemonEdit(0, heldEdit(44), error),
          "the backend refuses Berry Juice as a new held item");
    check(copy && !copy->stagePartyPokemonEdit(0, heldEdit(5), error) &&
              copy->stagePartyPokemonEdit(0, ballEdit(5), error),
          "the backend refuses Safari Ball as a held item but accepts it as the caught Ball");
}

} // namespace

// Owner Oct 25 7C: Gen V Language cycles inline on A and on a tap; the changed value is the
// feedback, so neither posts a "Language: ..." status toast.
void gen5LanguageNoToast() {
    const auto input = gen5Fixture(0, 84);   // Pikachu knows Thunder Shock (a level-up move)
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
    d.press(HidNpadButton_A);
    auto& state = trainer->sharedScreen();
    const auto actions = state.availableActions(trainer->stagedPokemon());
    size_t editRow = actions.count;
    for (size_t i = 0; i < actions.count; ++i)
        if (actions[i] == PokeBank::UIModel::SharedPokemonEditor::Action::Edit) editRow = i;
    for (size_t i = 0; i < editRow; ++i) d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    check(state.surface() == G5::Gen5SharedScreenState::Surface::Edit, "Gen V Language: the editor opens");
    if (state.surface() != G5::Gen5SharedScreenState::Surface::Edit) return;

    // Owner Oct 25 7B: a directly learnable Gen V move reads "OK", as in Gen I-IV.
    d.frame();
    bool ok = false, learnable = false;
    for (const auto& t : HostUi::texts) {
        ok = ok || t == "OK";
        learnable = learnable || t.find("Learnable") != std::string::npos;
    }
    check(ok && !learnable, "Gen V Moves panel marks Thunder Shock OK, not Learnable");

    const auto language = [&] {
        const auto p = state.draft().current();
        return p ? static_cast<int>(p->language()) : -1;
    };
    const auto toast = [&] { return screen.storageStatus.find("Language") != std::string::npos; };
    screen.postStatus("", 0);
    const int before = language();
    constexpr int kLanguageRow = 9;   // rows 1..9 precede the first locked row (Origin)
    for (int i = 0; i < kLanguageRow; ++i) d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    const int afterA = language();
    check(afterA != before, "Gen V A on Language cycles the language");
    check(!toast(), "Gen V Language A posts no toast (got \"" + screen.storageStatus + "\")");
    d.press(HidNpadButton_Up);
    check(d.tap(Workspace::kDetailTouchBase + kLanguageRow), "the Language row publishes a touch target");
    check(language() != afterA, "Gen V tap on Language cycles the language");
    check(!toast(), "Gen V Language tap posts no toast (got \"" + screen.storageStatus + "\")");
    check(HostUi::promptCount == 0, "Language never opens a prompt");
}

// Owner Oct 25 L06 acceptance ("no dead taps or invisible restrictions"): in View, A or a tap on a
// field says why nothing changes, and a row locked in every mode never points at Edit.
void gen3ViewOnly() {
    namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
    auto& g3 = openGen3(13);
    if (!g3.screen) return;
    Driver d{*g3.screen, {}, {}};
    if (!openGen3PartyAction(d, Shared::Action::View)) return;
    auto& screen = *g3.screen;
    const auto status = [&] { return screen.storageStatus; };
    screen.postStatus("", 0);
    d.press(HidNpadButton_A);
    check(status() == "View only: choose Edit to change Species",
          "Gen III View A on Species explains View (got \"" + status() + "\")");
    const auto tapSays = [&](int id, const std::string& expected, const std::string& what) {
        for (int i = 0; i < 16 && !d.target(id); ++i) d.press(HidNpadButton_Down);
        screen.postStatus("", 0);
        const bool tapped = d.tap(id);
        check(tapped && status() == expected, "Gen III View tap on " + what + " (got \"" + status() + "\")");
    };
    tapSays(31014, "View only: choose Edit to change Pokerus", "Pokérus explains View");
    tapSays(31010, "Origin is read-only for this Pokemon", "Origin says read-only, not Edit");
    tapSays(31300, "View only: choose Edit to change moves", "a move explains View");
    tapSays(31206, "View only: choose Edit to change Nature", "Nature explains View");
    tapSays(31210, "PID is read-only for this Pokemon", "PID says read-only");
    check(HostUi::promptCount == 0, "Gen III View never opens a prompt");
    const auto* editor = g3.trainer->stagedPokemon();
    check(!editor || editor->pendingChanges().empty(), "Gen III View stages nothing");
}

void gen5ViewOnly() {
    const auto input = gen5Fixture(0, 84);
    std::string error;
    const auto parsed = G5::Gen5ReadOnlySave::parse(input, "black_nds", &error, G5::SaveCopySelection::Primary);
    auto trainer = parsed ? PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error) : nullptr;
    check(trainer != nullptr, "Gen V View fixture opens");
    if (!trainer) return;
    UI::TrainerViewScreen screen(*trainer, "Pokemon Black", "/host/black.sav", 0, AccountUid{},
                                 PokeVault::Safety::SourceKind::ExternalLegacy, "black_nds", "DraStic");
    Driver d{screen, {}, {}};
    d.frame();
    screen.detailViewActive = true;
    screen.selectedMode = UI::TrainerViewScreen::ViewMode::Party;
    screen.selectedPartyIndex = 0;
    d.frame();
    d.press(HidNpadButton_A);
    auto& state = trainer->sharedScreen();
    const auto actions = state.availableActions(trainer->stagedPokemon());
    size_t viewRow = actions.count;
    for (size_t i = 0; i < actions.count; ++i)
        if (actions[i] == PokeBank::UIModel::SharedPokemonEditor::Action::View) viewRow = i;
    for (size_t i = 0; i < viewRow; ++i) d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    check(state.surface() == G5::Gen5SharedScreenState::Surface::View, "Gen V Actions A on View opens View");
    if (state.surface() != G5::Gen5SharedScreenState::Surface::View) return;
    const auto status = [&] { return screen.storageStatus; };
    const auto tapSays = [&](int id, const std::string& expected, const std::string& what) {
        for (int i = 0; i < 18 && !d.target(id); ++i) d.press(HidNpadButton_Down);
        screen.postStatus("", 0);
        const bool tapped = d.tap(id);
        check(tapped && status() == expected, "Gen V View tap on " + what + " (got \"" + status() + "\")");
    };
    const int base = Workspace::kDetailTouchBase;
    tapSays(base + 1, "View only: choose Edit to change Nickname", "Nickname explains View");
    screen.postStatus("", 0);
    d.press(HidNpadButton_A);
    check(status() == "View only: choose Edit to change Nickname", "Gen V View A explains View (got \"" + status() + "\")");
    tapSays(base + 14, "View only: choose Edit to change Pokerus", "Pokérus explains View");
    tapSays(base + 10, "Origin is read-only for this Pokemon", "Origin says read-only");
    // The fixture's origin is not a Gen V game, so its Ball stays read-only in Edit too, as does
    // the PID itself: View must not promise otherwise. Its Nature is editable in Edit.
    tapSays(base + 11, "Ball is read-only for this Pokemon", "a transferred Ball says read-only");
    tapSays(41206, "View only: choose Edit to change Nature", "Nature explains View");
    tapSays(41210, "PID is read-only for this Pokemon", "PID says read-only");
    tapSays(41300, "View only: choose Edit to change moves", "a move explains View");
    check(HostUi::promptCount == 0, "Gen V View never opens a prompt");
    check(!trainer->stagedPokemon().hasChanges(), "Gen V View stages nothing");
}

// Gen IV View (Round 6 follow-up): the Gen IV adapter's own lock rules (pidRowLocked, detailLocked)
// through the real screen. QA's synthetic Platinum party Pikachu has a non-Gen IV origin byte, so its
// Ball stays read-only in Edit too; Pikachu has one ability in Gen IV; its gender ratio varies.
void gen4ViewOnly() {
    const auto bytes = qasynth::gen4::save(qasynth::gen4::Kind::Platinum);
    std::string error;
    auto parsed = PokeVault::Integration::Gen4::Gen4ReadOnlySave::parse(
        bytes, PokeVault::Integration::Gen4::Layout::Platinum, "platinum_nds", &error);
    check(parsed.has_value(), "Gen IV fixture parses: " + error);
    if (!parsed) return;
    auto trainer = PokeVault::Legacy::Gen4ReadOnlyTrainer::create(*parsed, "platinum_nds", error);
    check(trainer != nullptr, "Gen IV trainer opens: " + error);
    if (!trainer) return;
    UI::TrainerViewScreen screen(*trainer, "Pokemon Platinum", "/host/platinum.sav", 0, AccountUid{},
                                 PokeVault::Safety::SourceKind::ExternalLegacy, "platinum_nds", "DraStic");
    Driver d{screen, {}, {}};
    d.frame();
    screen.detailViewActive = true;
    screen.selectedMode = UI::TrainerViewScreen::ViewMode::Party;
    screen.selectedPartyIndex = 0;
    d.frame();
    d.press(HidNpadButton_A);   // the Party Actions menu
    d.press(HidNpadButton_A);   // its first row: View
    const int base = Workspace::kDetailTouchBase;
    check(d.target(base + 1) != nullptr, "Gen IV Party A, A opens View");
    if (!d.target(base + 1)) return;
    const auto status = [&] { return screen.storageStatus; };
    const auto tapSays = [&](int id, const std::string& expected, const std::string& what) {
        for (int i = 0; i < 18 && !d.target(id); ++i) d.press(HidNpadButton_Down);
        screen.postStatus("", 0);
        const bool tapped = d.tap(id);
        check(tapped && status() == expected, "Gen IV View tap on " + what + " (got \"" + status() + "\")");
    };
    tapSays(base + 1, "View only: choose Edit to change Nickname", "Nickname explains View");
    screen.postStatus("", 0);
    d.press(HidNpadButton_A);
    check(status() == "View only: choose Edit to change Nickname", "Gen IV View A explains View (got \"" + status() + "\")");
    tapSays(base + 6, "SID is read-only for this Pokemon", "SID says read-only");
    tapSays(base + 10, "Origin is read-only for this Pokemon", "Origin says read-only");
    tapSays(base + 11, "Ball is read-only for this Pokemon", "a transferred Ball says read-only");
    tapSays(41206, "View only: choose Edit to change Nature", "Nature explains View");
    tapSays(41207, "View only: choose Edit to change Gender", "a variable Gender explains View");
    tapSays(41209, "Ability is read-only for this Pokemon", "a single Ability says read-only");
    tapSays(41210, "PID is read-only for this Pokemon", "PID says read-only");
    tapSays(41300, "View only: choose Edit to change moves", "a move explains View");
    check(HostUi::promptCount == 0, "Gen IV View never opens a prompt");
}

int main() {
    gen5Pokerus();
    gen5CreatePokerus();
    gen3Pokerus();
    gen3CreatePokerus();
    gen3PokerusFirstTapOnPartialRow();
    gen3HeldItem();
    gen5HeldItemPartialRow();
    gen3NoStateAcrossScreens();
    gen5NoStateAcrossScreens();
    gen5PokerusKeepWithStoredStrain();
    gen3SafariBallRoles();
    gen5LanguageNoToast();
    gen3ViewOnly();
    gen5ViewOnly();
    gen4ViewOnly();
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "L06 editor input through the real TrainerViewScreen (" << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
