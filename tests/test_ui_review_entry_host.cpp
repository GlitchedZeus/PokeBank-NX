// Round 6 (B08, P1-RETURN): through the REAL TrainerViewScreen::update / draw (tests/host-ui
// harness), every generation's Trainer, Party, Boxes and Bag screen opens the shared Review with its
// Review key, and B returns to the same screen with the same cursor. Gen III and Gen V Create / Edit
// open it with + and B returns to the draft. Synthetic saves only: Gen I / II / IV from the QA
// builders (tests/fuzz_synthetic_saves.h), Gen III / V from the L06 harness fixtures' shapes.
#include "HostUi.h"
#include "fuzz_synthetic_saves.h"
#include "fixtures/gsc_pokemon_fixture.h"

#include "Encryption/Encryption3FRLG.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5SharedScreenState.h"
#include "Legacy/BackupReadOnlyTrainer.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Source/ImmutableSourceSnapshot.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ReviewEntry.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"
#include "Utils/CRC16.h"

#include <array>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <unistd.h>
#include <vector>

namespace G5 = PokeVault::Integration::Gen5;
namespace C5 = PokeVault::Integration::Gen5::Crypto;
namespace L5 = PokeVault::Integration::Gen5::LayoutInfo;
namespace G3 = PokeVault::Integration::Gen3;
namespace D3 = PokeVault::Integration::Gen3::Detail;
namespace Entry = PokeBank::UIModel::ReviewEntry;
using View = UI::TrainerViewScreen::ViewMode;

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
};

// The shared Review panel's heading (SharedEditorChrome::drawReviewPanel) is on screen.
bool reviewShown() {
    for (const auto& text : HostUi::texts)
        if (text == "Review Pending Changes") return true;
    return false;
}

struct Cursor {
    View mode;
    bool detail;
    int item, box, party, category, page, trainerRow;
};
Cursor cursor(const UI::TrainerViewScreen& s) {
    return {s.selectedMode, s.detailViewActive, s.selectedItemIndex, s.selectedBoxIndex, s.selectedPartyIndex,
            s.selectedCategory, s.currentPage, s.trainerSelectedRow};
}
bool same(const Cursor& a, const Cursor& b) {
    return a.mode == b.mode && a.detail == b.detail && a.item == b.item && a.box == b.box && a.party == b.party &&
           a.category == b.category && a.page == b.page && a.trainerRow == b.trainerRow;
}

const char* modeName(View mode) {
    switch (mode) {
        case View::Trainer: return "Trainer";
        case View::Party:   return "Party";
        case View::Boxes:   return "Boxes";
        case View::Items:   return "Bag";
        default:            return "other";
    }
}
Entry::Screen entryScreen(View mode) {
    switch (mode) {
        case View::Trainer: return Entry::Screen::Trainer;
        case View::Party:   return Entry::Screen::Party;
        case View::Boxes:   return Entry::Screen::Boxes;
        default:            return Entry::Screen::Bag;
    }
}

// Enter `mode`, place a cursor, press the screen's Review key: the shared Review is drawn. B returns
// to the same screen with the same cursor and no Review.
template <class Enter = void (*)()>
void reviewRoundTrip(Driver& d, int generation, const std::string& game, View mode, Enter&& enter = [] {}) {
    const std::string what = "Gen " + std::to_string(generation) + " " + game + " " + modeName(mode);
    d.screen.detailViewActive = true;
    d.screen.selectedMode = mode;
    d.screen.selectedItemIndex = mode == View::Boxes ? 7 : 0;
    d.screen.selectedBoxIndex = mode == View::Boxes ? 1 : 0;
    d.screen.selectedPartyIndex = 0;
    d.screen.selectedCategory = mode == View::Items ? 1 : 0;
    d.screen.currentPage = 0;
    enter();   // a page that opens through its own panel (Gen V Trainer editor)
    d.frame();
    d.frame();
    check(!reviewShown(), what + ": no Review before the key");
    const Cursor before = cursor(d.screen);
    const auto hint = Entry::footerHint(entryScreen(mode), generation);
    const u64 key = hint.glyph[0] == 'Y' ? HidNpadButton_Y : HidNpadButton_Plus;
    d.press(key);
    check(reviewShown(), what + ": " + hint.glyph + " opens the shared Review (status: " + d.screen.storageStatus + ")");
    d.press(HidNpadButton_B);
    check(!reviewShown(), what + ": B closes Review");
    check(same(cursor(d.screen), before), what + ": B returns to the same screen and cursor");
    if (key == HidNpadButton_Y && mode != View::Trainer) {
        // + is the same entry on every screen.
        d.press(HidNpadButton_Plus);
        check(reviewShown(), what + ": + opens Review too");
        d.press(HidNpadButton_B);
        check(!reviewShown() && same(cursor(d.screen), before), what + ": + Review returns the same way");
    }
}

// ---- Gen I / II / IV: synthetic saves through the real strict parsers (openBackupTrainer). ----
struct Opened {
    std::unique_ptr<Trainer::Trainer> trainer;
    std::unique_ptr<UI::TrainerViewScreen> screen;
};
std::vector<Opened> keepAlive;   // surfaces key per-screen state by address: never reuse one

Opened* openClassic(const qasynth::Bytes& bytes, const std::string& game, PokeVault::Safety::SourceKind kind,
                    const std::string& provider) {
    namespace S = PokeVault::Source::ImmutableSnapshot;
    char pattern[] = "/tmp/pokebank-review-entry-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    check(dir != nullptr, game + ": temp dir");
    if (!dir) return nullptr;
    const std::string path = std::string(dir) + "/" + game + ".sav";
    {
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    std::string error;
    S::File file;
    check(S::read(path, dir, {}, file, error), game + ": read synthetic save: " + error);
    auto trainer = PokeVault::Legacy::openBackupTrainer(file, game, error);
    check(trainer != nullptr, game + ": strict parse + trainer: " + error);
    std::filesystem::remove_all(dir);
    if (!trainer) return nullptr;
    auto& opened = keepAlive.emplace_back();
    opened.trainer = std::move(trainer);
    opened.screen = std::make_unique<UI::TrainerViewScreen>(*opened.trainer, "Pokemon " + game, path, 0,
                                                           AccountUid{}, kind, game, provider);
    return &opened;
}

void classicGeneration(int generation, const qasynth::Bytes& bytes, const std::string& game,
                       PokeVault::Safety::SourceKind kind, const std::string& provider) {
    auto* opened = openClassic(bytes, game, kind, provider);
    if (!opened) return;
    Driver d{*opened->screen, {}, {}};
    d.frame();
    for (const View mode : {View::Trainer, View::Party, View::Boxes, View::Items})
        reviewRoundTrip(d, generation, game, mode);
}

// ---- Gen V: a Black save with one party Pikachu (the L06 harness fixture shape). ----
std::vector<uint8_t> gen5Fixture() {
    std::vector<uint8_t> sav(L5::FullSaveSize, 0);
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

struct Gen5Opened {
    std::vector<uint8_t> bytes;
    std::unique_ptr<PokeVault::Legacy::Gen5ReadOnlyTrainer> trainer;
    std::unique_ptr<UI::TrainerViewScreen> screen;
};
std::vector<Gen5Opened> gen5Alive;
Gen5Opened* openGen5() {
    auto& out = gen5Alive.emplace_back();
    out.bytes = gen5Fixture();
    std::string error;
    const auto parsed = G5::Gen5ReadOnlySave::parse(out.bytes, "black_nds", &error, G5::SaveCopySelection::Primary);
    check(parsed.has_value(), "Gen V fixture parses: " + error);
    if (!parsed) return nullptr;
    out.trainer = PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error);
    check(out.trainer != nullptr, "Gen V trainer: " + error);
    if (!out.trainer) return nullptr;
    out.screen = std::make_unique<UI::TrainerViewScreen>(*out.trainer, "Pokemon Black", "/host/black.sav", 0,
        AccountUid{}, PokeVault::Safety::SourceKind::ExternalLegacy, "black_nds", "DraStic");
    return &out;
}

// Gen V Review needs a staged change: stage one Level edit through the shared state machine.
bool stageGen5Change(Gen5Opened& g5) {
    auto& ws = g5.trainer->stagedPokemon();
    auto& ui = g5.trainer->sharedScreen();
    const auto refresh = [&](std::string& why) { return g5.trainer->refreshStagedPokemonPresentation(why); };
    std::string e;
    const bool ok = ui.openActions(ws, {G5::Gen5StagedPokemonWorkspace::Region::Party, 0, 0}, e) &&
                    ui.activate(ws, PokeBank::UIModel::SharedPokemonEditor::Action::Edit, e) &&
                    G5::stageSharedField(ui.editDraft(), G5::Gen5SharedScreenState::Field::Level, 0, 31, e) &&
                    ui.keepWithPresentation(ws, refresh, e);
    check(ok && ws.hasChanges(), "Gen V staged one change: " + e);
    return ok;
}

void gen5Screens() {
    auto* g5 = openGen5();
    if (!g5 || !stageGen5Change(*g5)) return;
    Driver d{*g5->screen, {}, {}};
    d.frame();
    // The Gen V Trainer page is its editor, opened from the hub tile as on the console.
    reviewRoundTrip(d, 5, "black_nds", View::Trainer, [&] {
        std::string e;
        check(g5->trainer->trainerScreen().openEditor(g5->trainer->stagedPokemon(), e), "Gen V Trainer editor opens: " + e);
    });
    for (const View mode : {View::Party, View::Boxes, View::Items})
        reviewRoundTrip(d, 5, "black_nds", mode);
}

// Gen V Edit and Create: + opens Review; B returns to the same workspace with the draft intact.
void gen5Workspaces() {
    auto* g5 = openGen5();
    if (!g5 || !stageGen5Change(*g5)) return;
    auto& ui = g5->trainer->sharedScreen();
    auto& ws = g5->trainer->stagedPokemon();
    Driver d{*g5->screen, {}, {}};
    d.frame();
    d.screen.detailViewActive = true;
    d.screen.selectedMode = View::Party;
    d.frame();
    std::string e;
    check(ui.openActions(ws, {G5::Gen5StagedPokemonWorkspace::Region::Party, 0, 0}, e) &&
              ui.activate(ws, PokeBank::UIModel::SharedPokemonEditor::Action::Edit, e),
          "Gen V Edit opens: " + e);
    check(G5::stageSharedField(ui.editDraft(), G5::Gen5SharedScreenState::Field::Level, 0, 44, e),
          "Gen V draft change: " + e);
    d.frame();
    // Level is stored as experience: the draft's experience identifies the unsaved change.
    const auto exp = [&] { const auto p = ui.draft().current(); return p ? static_cast<long>(p->experience()) : -1L; };
    const long staged = exp();
    check(staged > 0 && ui.editDraft().dirty(), "Gen V Edit draft holds the change");
    d.press(HidNpadButton_Plus);
    check(ui.surface() == G5::Gen5SharedScreenState::Surface::Review && reviewShown(), "Gen V Edit: + opens Review");
    d.press(HidNpadButton_B);
    check(ui.surface() == G5::Gen5SharedScreenState::Surface::Edit && !reviewShown() && exp() == staged &&
              ui.editDraft().dirty(),
          "Gen V Edit: B returns to the Edit draft unchanged");

    // Create: an empty box slot's Add -> A on Create -> the shared editor in Create; + Review; B
    // back in Create with its staged record.
    const auto refresh = [&](std::string& why) { return g5->trainer->refreshStagedPokemonPresentation(why); };
    check(ui.keepWithPresentation(ws, refresh, e), "Gen V Edit kept: " + e);
    d.screen.selectedMode = View::Boxes;
    d.frame();
    const G5::Gen5StagedPokemonWorkspace::Slot empty{G5::Gen5StagedPokemonWorkspace::Region::Box, 0, 0};
    check(ui.openActions(ws, empty, e) && ui.activate(ws, PokeBank::UIModel::SharedPokemonEditor::Action::Add, e),
          "Gen V empty slot offers Add: " + e);
    d.frame();
    d.press(HidNpadButton_A);
    check(ui.surface() == G5::Gen5SharedScreenState::Surface::Edit && ui.createPending(),
          "Gen V A on Create opens the editor in Create (status: " + d.screen.storageStatus + ")");
    d.press(HidNpadButton_Plus);
    check(ui.surface() == G5::Gen5SharedScreenState::Surface::Review && reviewShown(), "Gen V Create: + opens Review");
    d.press(HidNpadButton_B);
    check(ui.surface() == G5::Gen5SharedScreenState::Surface::Edit && ui.createPending() && !reviewShown(),
          "Gen V Create: B returns to Create with its staged record");
}

// ---- Gen III (FireRed): one party Pikachu, the L06 harness fixture shape. ----
void put16(std::span<uint8_t> bytes, std::size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}
void put32(std::span<uint8_t> bytes, std::size_t offset, uint32_t value) {
    put16(bytes, offset, static_cast<uint16_t>(value));
    put16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
}
std::vector<uint8_t> gen3Fixture() {
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

struct Gen3Opened {
    std::vector<uint8_t> bytes;
    std::unique_ptr<PokeVault::Legacy::FRLGReadOnlyTrainer> trainer;
    std::unique_ptr<UI::TrainerViewScreen> screen;
};
std::vector<Gen3Opened> gen3Alive;
Gen3Opened* openGen3() {
    auto& out = gen3Alive.emplace_back();
    out.bytes = gen3Fixture();
    auto parsed = G3::parse(out.bytes, G3::SourceGame::FireRedGBA);
    check(static_cast<bool>(parsed), "Gen III fixture parses: " + parsed.detail);
    if (!parsed) return nullptr;
    std::string error;
    out.trainer = PokeVault::Legacy::FRLGReadOnlyTrainer::create(*parsed.save, error);
    check(out.trainer != nullptr, "Gen III trainer: " + error);
    if (!out.trainer) return nullptr;
    out.screen = std::make_unique<UI::TrainerViewScreen>(*out.trainer, "Pokemon FireRed", "/host/firered.sav", 0,
        AccountUid{}, PokeVault::Safety::SourceKind::RetroArchLegacy, "firered_gba", "mGBA");
    return &out;
}

std::string drawnValue(const std::string& label) {
    for (size_t i = 0; i + 1 < HostUi::texts.size(); ++i)
        if (HostUi::texts[i] == label) return HostUi::texts[i + 1];
    return "<" + label + " not drawn>";
}

void gen3Screens() {
    auto* g3 = openGen3();
    if (!g3) return;
    Driver d{*g3->screen, {}, {}};
    d.frame();
    for (const View mode : {View::Trainer, View::Party, View::Boxes, View::Items})
        reviewRoundTrip(d, 3, "firered_gba", mode);
}

void gen3Create(Driver& d);
// Gen III Edit (Party A -> Party Actions -> Edit) and Create (X on an empty box slot): + opens
// Review, B returns to the same workspace with what it showed.
void gen3Workspaces() {
    auto* g3 = openGen3();
    if (!g3) return;
    Driver d{*g3->screen, {}, {}};
    d.frame();
    d.screen.detailViewActive = true;
    d.screen.selectedMode = View::Party;
    d.screen.selectedPartyIndex = 0;
    d.frame();
    d.press(HidNpadButton_A);
    namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
    const auto list = Shared::actionsForSlot(true, {false, false, true}, false);
    int edit = -1;
    for (size_t i = 0; i < list.count; ++i)
        if (list[i] == Shared::Action::Edit) edit = static_cast<int>(i);
    for (int i = 0; i < edit; ++i) d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    const std::string species = drawnValue("Species");
    check(species.find("Pikachu") != std::string::npos || species.find("PIKACHU") != std::string::npos ||
              species.find("#") != std::string::npos,
          "Gen III Party Edit opens the editor (species " + species + ")");
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_Down);
    d.frame();
    const auto texts = HostUi::texts;
    d.press(HidNpadButton_Plus);
    check(reviewShown(), "Gen III Edit: + opens Review");
    d.press(HidNpadButton_B);
    check(!reviewShown() && HostUi::texts == texts, "Gen III Edit: B returns to the same Edit draw");

    // Create from an empty box slot, on a fresh screen (as the L06 Create case).
    auto* created3 = openGen3();
    if (!created3) return;
    Driver c{*created3->screen, {}, {}};
    c.frame();
    gen3Create(c);
}

void gen3Create(Driver& d) {
    d.screen.detailViewActive = true;
    d.screen.selectedMode = View::Boxes;
    d.screen.selectedBoxIndex = 0;
    d.screen.selectedItemIndex = 0;
    d.frame();
    d.press(HidNpadButton_X);
    const std::string created = drawnValue("Species");
    check(created.rfind("<", 0) != 0, "Gen III X on an empty box slot opens Create (" + created + ")");
    d.frame();
    const auto createTexts = HostUi::texts;
    d.press(HidNpadButton_Plus);
    check(reviewShown(), "Gen III Create: + opens Review");
    d.press(HidNpadButton_B);
    check(!reviewShown() && HostUi::texts == createTexts, "Gen III Create: B returns to the same Create draw");
}


// Round 6 (7E gap G5) through the real screen: a Gen III session opened from a file on disk keeps
// its Open snapshot; once the emulator rewrites that file, Review's X Export Copy is refused and
// nothing is exported. The source is only read.
void gen3ExportRefusesChangedSource() {
    namespace Snap = PokeVault::Source::ImmutableSnapshot;
    auto* g3 = openGen3();
    if (!g3) return;
    char pattern[] = "/tmp/pokebank-g5-guard-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    check(dir != nullptr, "G5 temp dir");
    if (!dir) return;
    const std::string path = std::string(dir) + "/firered.sav";
    const auto writeFile = [&](const std::vector<uint8_t>& bytes) {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    };
    writeFile(g3->bytes);
    std::string error;
    Snap::File file;
    check(Snap::read(path, dir, {}, file, error) && file.sha256 == Snap::fingerprint(g3->bytes), "G5 snapshot: " + error);
    g3->screen->classicSourceSnapshot = file;   // as UIManager::handleLegacyFRLGView does at Open
    check(g3->trainer->stagedTrainer() && g3->trainer->stagedTrainer()->stageMoney(4321, error),
          "G5 one staged Trainer change: " + error);

    Driver d{*g3->screen, {}, {}};
    d.frame();
    d.screen.detailViewActive = true;
    d.screen.selectedMode = View::Party;
    d.frame();
    d.press(HidNpadButton_Plus);
    check(reviewShown(), "G5: Review open");
    auto changed = g3->bytes;
    changed[0x10] ^= 0x5A;   // the emulator saved meanwhile
    ::usleep(20000);
    writeFile(changed);
    const auto exportsBefore = std::filesystem::exists("sdmc:");
    d.press(HidNpadButton_X);   // Review X: Export Copy
    check(d.screen.storageStatus.find("changed since it was opened") != std::string::npos,
          "G5: Export Copy refused after the source changed (status: " + d.screen.storageStatus + ")");
    check(std::filesystem::exists("sdmc:") == exportsBefore, "G5: nothing was exported");
    std::ifstream in(path, std::ios::binary);
    const std::vector<uint8_t> after{std::istreambuf_iterator<char>(in), {}};
    check(after == changed, "G5: the source file was only read");
    std::filesystem::remove_all(dir);
}
} // namespace

int main() {
    using Kind = PokeVault::Safety::SourceKind;
    classicGeneration(1, qasynth::gen1::save(), "red_gb", Kind::RetroArchLegacy, "RetroArch");
    classicGeneration(2, fixture(GS), "gold_gbc", Kind::RetroArchLegacy, "RetroArch");
    gen3Screens();
    gen3Workspaces();
    gen3ExportRefusesChangedSource();
    classicGeneration(4, qasynth::gen4::save(qasynth::gen4::Kind::Platinum), "platinum_nds", Kind::ExternalLegacy,
                      "DraStic");
    gen5Screens();
    gen5Workspaces();
    if (failures) {
        std::cerr << failures << " of " << checks << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "Round 6 Review entry / return through the real TrainerViewScreen (" << checks
              << " checks): PASS\n";
    return EXIT_SUCCESS;
}
