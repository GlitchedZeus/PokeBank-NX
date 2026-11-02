// Round 14 (owner Round 13 R08, verbatim: "gen 4 footer says edit quanitty not edit amout or what gen 1-3
// have for the A button, gen 5 has almost none of the proper footers and im guessing controls"): through the
// REAL TrainerViewScreen update / draw (tests/host-ui harness), the footer the player sees on each screen
// kind -- Bag list, Add Item picker, Party, Boxes, Trainer, Review and the staged-exit question -- is the
// SAME list of (button, label) pairs in Gen I, II, III, IV and V, worded as Gen I-III word it, and every
// button a footer advertises does something when pressed (no advertised action that does nothing).
// The footer is read back from what the frame publishes: each footer badge's touch target
// (ScreenChrome g_navHits / g_navGestureHits) and the visible label drawn inside it.
// Run with -v to print every footer (the audit table in the Round 14 report).
// Synthetic saves only (HostUiFixtures / fuzz_synthetic_saves.h / fixtures/gsc_pokemon_fixture.h).
#include "HostUi.h"
#include "HostUiFixtures.h"
#include "fuzz_synthetic_saves.h"
#include "fixtures/gsc_pokemon_fixture.h"

#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Integration/Gen4/Gen4ReadOnlyInventory.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen5/Gen5Bag.h"
#include "Integration/Gen5/Gen5SharedScreenState.h"
#include "Legacy/BackupReadOnlyTrainer.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Legacy/Gen5ReadOnlyTrainer.h"
#include "Source/ImmutableSourceSnapshot.h"
#include "UI/ExportMessages.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/ScreenChrome.h"
#include "UI/SharedMenuGeometry.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

namespace MG = PokeBank::UIModel::SharedMenuGeometry;
using View = UI::TrainerViewScreen::ViewMode;

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

using Hint = std::pair<std::string, std::string>;   // (button, label)
using Footer = std::vector<Hint>;

std::string str(const Footer& footer) {
    std::string out;
    for (const auto& [glyph, label] : footer) {
        if (!out.empty()) out += "  |  ";
        out += glyph.empty() ? label : glyph + ": " + label;
    }
    return out.empty() ? "(none)" : out;
}

std::string buttonName(uint64_t button) {
    switch (button) {
        case HidNpadButton_A: return "A";
        case HidNpadButton_B: return "B";
        case HidNpadButton_X: return "X";
        case HidNpadButton_Y: return "Y";
        case HidNpadButton_Plus: return "+";
        case HidNpadButton_Minus: return "-";
        case HidNpadButton_L: return "L";
        case HidNpadButton_R: return "R";
        case HidNpadButton_ZL: return "ZL";
        case HidNpadButton_ZR: return "ZR";
        default: return "?";
    }
}
std::string gestureName(UI::NavGestureKind kind) {
    switch (kind) {
        case UI::NavGestureKind::UpDown: return "Up/Down";
        case UI::NavGestureKind::LeftRight: return "Left/Right";
        case UI::NavGestureKind::DPad: return "D-pad";
        case UI::NavGestureKind::LR: return "L/R";
        case UI::NavGestureKind::ZLZR: return "ZL/ZR";
        default: return "?";
    }
}

// The footer of the last frame, left to right: every badge's touch target with the label drawn inside it.
Footer footer() {
    struct Seg { int x, w; std::string glyph; uint64_t button; };
    std::vector<Seg> segs;
    for (const auto& h : UI::g_navHits) segs.push_back({h.x, h.w, buttonName(h.button), h.button});
    for (const auto& h : UI::g_navGestureHits) segs.push_back({h.x, h.w, gestureName(h.kind), 0});
    std::sort(segs.begin(), segs.end(), [](const Seg& a, const Seg& b) { return a.x < b.x; });
    Footer out;
    for (const auto& s : segs) {
        const HostUi::TextRect* label = nullptr;
        for (const auto& t : HostUi::visibleText)
            if (t.y >= MG::kScreenH - MG::kFooterH && t.x >= s.x && t.x < s.x + s.w && (!label || t.x > label->x))
                label = &t;
        out.push_back({s.glyph, label ? label->text : std::string("(no label)")});
    }
    return out;
}

struct Driver {
    explicit Driver(UI::TrainerViewScreen& s) : screen(s) {}
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
    bool drawn(const std::string& needle) const {
        return std::any_of(HostUi::texts.begin(), HostUi::texts.end(),
                           [&](const std::string& t) { return t.find(needle) != std::string::npos; });
    }
};

// One loaded game: makes a fresh TrainerViewScreen on the same staged trainer whenever asked, so
// pressing one advertised button never leaves state behind for the next.
struct Game {
    int generation = 0;
    std::string name;
    std::function<UI::TrainerViewScreen*()> open;
    int itemCategory = 0;    // the Bag category holding Potion (Gen V: R presses from the first pocket)
    int emptyCategory = 0;   // a Bag category with nothing in it (Gen V: R presses)
    bool partyOccupied = false;   // Party slot 1 holds a Pokemon
    bool boxOccupied = false;     // Box 1 slot 1 holds a Pokemon
};

std::vector<std::shared_ptr<void>> keepAlive;
template <class T>
T* keep(std::unique_ptr<T> p) {
    T* raw = p.get();
    keepAlive.push_back(std::shared_ptr<void>(std::move(p)));
    return raw;
}

// ---- screens ------------------------------------------------------------------------------
enum class Kind { Hub, BagItem, BagEmpty, AddPicker, RemoveConfirm, Party, Boxes, Trainer, Editor, Review, Exit };
const char* kindName(Kind k) {
    switch (k) {
        case Kind::Hub: return "Save hub";
        case Kind::Editor: return "Pokemon editor (Party slot 1, Edit)";
        case Kind::BagItem: return "Bag (item selected)";
        case Kind::BagEmpty: return "Bag (empty pocket)";
        case Kind::AddPicker: return "Add Item picker";
        case Kind::RemoveConfirm: return "Bag Remove question";
        case Kind::Party: return "Party";
        case Kind::Boxes: return "Boxes (occupied slot)";
        case Kind::Trainer: return "Trainer (Money row)";
        case Kind::Review: return "Review (one staged Bag change)";
        case Kind::Exit: return "Unsaved-changes question (one staged Bag change)";
    }
    return "?";
}

void enterBag(Game& g, Driver& d, bool item) {
    d.frame();
    d.screen.detailViewActive = true;
    d.screen.selectedMode = View::Items;
    d.screen.selectedItemIndex = 0;
    d.screen.currentPage = 0;
    if (g.generation == 5) {
        d.frame();
        // Gen V keeps its pocket cursor in Gen5BagScreen; an empty pocket is the one before Medicine.
        for (int i = 0; i < (item ? g.itemCategory : g.emptyCategory); ++i) d.press(HidNpadButton_R);
    } else {
        d.screen.selectedCategory = item ? g.itemCategory : g.emptyCategory;
    }
    d.frame();
    d.frame();
}

// Put the screen in `kind`. Returns false when that screen does not exist for this game.
bool enter(Game& g, Driver& d, Kind kind) {
    switch (kind) {
        case Kind::Hub:
            d.frame();
            d.screen.detailViewActive = false;
            d.frame();
            d.frame();
            return true;
        case Kind::Editor:
            // Party slot 1: A opens its actions, the second row is Edit (as test_ui_review_entry_host).
            if (!g.partyOccupied) return false;
            enter(g, d, Kind::Party);
            d.press(HidNpadButton_A);
            d.press(HidNpadButton_Down);
            d.press(HidNpadButton_A);
            return true;
        case Kind::BagItem: enterBag(g, d, true); return true;
        case Kind::BagEmpty: enterBag(g, d, false); return true;
        case Kind::AddPicker: enterBag(g, d, true); d.press(HidNpadButton_X); return true;
        case Kind::RemoveConfirm: enterBag(g, d, true); d.press(HidNpadButton_Y); return true;
        case Kind::Party:
            d.frame();
            d.screen.detailViewActive = true;
            d.screen.selectedMode = View::Party;
            d.screen.selectedItemIndex = 0;
            d.screen.selectedPartyIndex = 0;
            d.frame();
            d.frame();
            return true;
        case Kind::Boxes:
            d.frame();
            d.screen.detailViewActive = true;
            d.screen.selectedMode = View::Boxes;
            d.screen.selectedBoxIndex = 0;
            d.screen.selectedItemIndex = 0;
            d.frame();
            d.frame();
            return true;
        case Kind::Trainer:
            d.frame();
            d.screen.detailViewActive = true;
            d.screen.selectedMode = View::Trainer;
            d.screen.trainerSelectedRow = 1;
            if (g.generation == 5) {
                auto& t = static_cast<PokeVault::Legacy::Gen5ReadOnlyTrainer&>(d.screen.trainer);
                std::string e;
                check(t.trainerScreen().openEditor(t.stagedPokemon(), e), g.name + ": Gen V Trainer editor opens: " + e);
                d.frame();
                d.press(HidNpadButton_Down);   // Name -> Money
            }
            d.frame();
            d.frame();
            return true;
        case Kind::Review:
        case Kind::Exit: {
            // Stage one Bag change through the shared quantity numpad (Potion -> 7), then + opens
            // Review from the Bag, or B / B leaves the Bag and asks the unsaved-changes question.
            enterBag(g, d, true);
            HostUi::numbers.push_back({true, 7});
            d.press(HidNpadButton_A);
            HostUi::numbers.clear();
            if (kind == Kind::Review) {
                d.press(HidNpadButton_Plus);
            } else {
                d.press(HidNpadButton_B);
                d.press(HidNpadButton_B);
            }
            return true;
        }
    }
    return false;
}

struct Snapshot {
    std::vector<std::string> texts;
    std::vector<HostUi::Outline> outlines;   // a focus moved between panels redraws its outline
    std::string status;
    bool goBack, detail, help;
    View mode;
    int item, category, row;
};
Snapshot snap(const Driver& d) {
    return {HostUi::texts, HostUi::outlines, d.screen.storageStatus, d.screen.goBack, d.screen.detailViewActive,
            d.screen.helpOverlayActive, d.screen.selectedMode, d.screen.selectedItemIndex,
            d.screen.selectedCategory, d.screen.trainerSelectedRow};
}
bool same(const Snapshot& a, const Snapshot& b) {
    return a.texts == b.texts && a.outlines == b.outlines && a.status == b.status && a.goBack == b.goBack && a.detail == b.detail &&
           a.help == b.help && a.mode == b.mode && a.item == b.item && a.category == b.category && a.row == b.row;
}
uint64_t buttonFor(const std::string& glyph) {
    if (glyph == "L/R") return HidNpadButton_R;
    if (glyph == "ZL/ZR") return HidNpadButton_ZR;
    return UI::navButtonFor(glyph);
}

std::map<Kind, std::map<int, Footer>> table;

void audit(Game& g, Kind kind) {
    auto* screen = g.open();
    Driver d(*screen);
    if (!enter(g, d, kind)) return;
    const Footer seen = footer();
    table[kind][g.generation] = seen;
    // Every advertised button acts on a fresh copy of the same screen (the Gen I-IV Help and the
    // Add picker's Sort are included). The quantity numpad answers Cancel, so nothing is staged.
    for (const auto& [glyph, label] : seen) {
        const uint64_t button = buttonFor(glyph);
        if (!button) continue;   // D-pad / Up/Down navigation
        auto* fresh = g.open();
        Driver f(*fresh);
        enter(g, f, kind);
        const auto before = snap(f);
        const int prompts = HostUi::promptCount;
        f.press(button);
        const bool acted = !same(before, snap(f)) || HostUi::promptCount != prompts;
        // The keys every screen shares do exactly what their label says.
        if (glyph == "-" && label == "Help")
            check(f.screen.helpOverlayActive || f.drawn("Inventory Controls"), "Gen " + std::to_string(g.generation) + " " + kindName(kind) +
                                                  ": - opens Help");
        if (glyph == "X" && label == "Sort")
            check(f.drawn("Sort: Name A-Z"), "Gen " + std::to_string(g.generation) + " " + kindName(kind) +
                                                 ": X sorts the picker by name");
        check(acted, "Gen " + std::to_string(g.generation) + " " + kindName(kind) + ": footer \"" + glyph + ": " +
                         label + "\" does something when pressed");
    }
}

// ---- fixtures -----------------------------------------------------------------------------
UI::TrainerViewScreen* screenFor(Trainer::Trainer& trainer, const std::string& title, const std::string& id,
                                 const std::string& emulator, PokeVault::Safety::SourceKind kind) {
    return keep(std::make_unique<UI::TrainerViewScreen>(trainer, title, "/host/" + id + ".sav", 0, AccountUid{},
                                                        kind, id, emulator));
}

std::unique_ptr<Trainer::Trainer> openBackup(const qasynth::Bytes& bytes, const std::string& game) {
    namespace S = PokeVault::Source::ImmutableSnapshot;
    char pattern[] = "/tmp/pokebank-footer-parity-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    if (!dir) return nullptr;
    const std::string path = std::string(dir) + "/" + game + ".sav";
    {
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
    std::string error;
    S::File file;
    std::unique_ptr<Trainer::Trainer> trainer;
    if (S::read(path, dir, {}, file, error)) trainer = PokeVault::Legacy::openBackupTrainer(file, game, error);
    check(trainer != nullptr, game + ": strict parse + trainer: " + error);
    std::filesystem::remove_all(dir);
    return trainer;
}

using SK = PokeVault::Safety::SourceKind;

// Gen I: Red with Potion x3 in the Bag (Medicines); Party and Box 1 empty.
UI::TrainerViewScreen* openRed() {
    auto bytes = qasynth::gen1::save();
    bytes[0x25C9] = 1; bytes[0x25CA] = 0x14; bytes[0x25CB] = 3; bytes[0x25CC] = 0xFF;   // pret wNumBagItems
    bytes[0x27E6] = 0; bytes[0x27E7] = 0xFF;                                              // empty PC item list
    qasynth::gen1::forge(bytes);
    auto t = openBackup(bytes, "red_gb");
    if (!t) return nullptr;
    auto* trainer = keep(std::move(t));
    return screenFor(*trainer, "Pokemon Red", "red_gb", "RetroArch", SK::RetroArchLegacy);
}
// Gen II: Gold with Potion x2 (Items), Poke Ball x5 and PIKA in Box 1 slot 1.
UI::TrainerViewScreen* openGold() {
    auto t = openBackup(fixture(GS), "gold_gbc");
    if (!t) return nullptr;
    auto* trainer = keep(std::move(t));
    return screenFor(*trainer, "Pokemon Gold", "gold_gbc", "RetroArch", SK::RetroArchLegacy);
}
// Gen III: FireRed with Potion x3 in the Items pouch.
UI::TrainerViewScreen* openFireRed() {
    namespace G3 = PokeVault::Integration::Gen3;
    namespace D3 = PokeVault::Integration::Gen3::Detail;
    auto bytes = std::make_shared<std::vector<uint8_t>>(HostUiFixtures::gen3Fixture(0));
    auto& save = *bytes;
    const auto active = D3::validateSlot(save, 1);
    const auto sector = active.logicalSectorOffsets[1];
    const uint16_t key16 = 0xC3D4;   // low half of the fixture's security key
    HostUiFixtures::put16(save, sector + 0x310, 13);
    HostUiFixtures::put16(save, sector + 0x312, static_cast<uint16_t>(3 ^ key16));
    HostUiFixtures::put16(save, sector + 0xFF6,
        D3::sectorChecksum(std::span<const uint8_t>(save).subspan(sector, D3::kChunkLengths[1])));
    keepAlive.push_back(bytes);
    auto parsed = G3::parse(save, G3::SourceGame::FireRedGBA);
    if (!parsed) return nullptr;
    std::string error;
    auto* trainer = keep(PokeVault::Legacy::FRLGReadOnlyTrainer::create(*parsed.save, error));
    if (!trainer) return nullptr;
    return screenFor(*trainer, "Pokemon FireRed", "firered_gba", "mGBA", SK::RetroArchLegacy);
}
// Gen IV: Platinum with Potion x3 in Medicine.
UI::TrainerViewScreen* openPlatinum() {
    namespace G4 = PokeVault::Integration::Gen4;
    auto bytes = std::make_shared<std::vector<uint8_t>>(qasynth::gen4::save(qasynth::gen4::Kind::Platinum));
    auto& raw = *bytes;
    const auto layout = G4::bagLayout(G4::Layout::Platinum);
    for (std::size_t base : {std::size_t{0}, qasynth::gen4::kPartition}) {
        qasynth::w16(raw, base + layout[4].offset, 17);
        qasynth::w16(raw, base + layout[4].offset + 2, 3);
    }
    qasynth::gen4::forge(raw);
    keepAlive.push_back(bytes);
    std::string error;
    auto parsed = G4::Gen4ReadOnlySave::parse(raw, G4::Layout::Platinum, "platinum_nds", &error);
    if (!parsed) return nullptr;
    auto kept = std::make_shared<G4::Gen4ReadOnlySave>(*parsed);
    keepAlive.push_back(kept);
    auto* trainer = keep(PokeVault::Legacy::Gen4ReadOnlyTrainer::create(*kept, "platinum_nds", error));
    if (!trainer) return nullptr;
    return screenFor(*trainer, "Pokemon Platinum", "platinum_nds", "DraStic", SK::ExternalLegacy);
}
// Gen V: Black with Potion x3 in Medicine and Pikachu in Party slot 1. Gen V keeps its Bag, Trainer
// and shared editor state in the trainer, so every open() parses a fresh one.
UI::TrainerViewScreen* openBlack() {
    namespace G5 = PokeVault::Integration::Gen5;
    auto bytes = std::make_shared<std::vector<uint8_t>>(HostUiFixtures::gen5Fixture());
    auto& sav = *bytes;
    const std::size_t at = G5::itemsBlockOffset(G5::SaveFamily::BlackWhite) +
                           G5::bagPocketLayout5(G5::SaveFamily::BlackWhite, 3).offset;
    qasynth::w16(sav, at, 17);
    qasynth::w16(sav, at + 2, 3);
    qasynth::gen5::stamp(std::span<uint8_t>(sav).subspan(0, qasynth::gen5::copySize(G5::SaveFamily::BlackWhite)),
                         qasynth::gen5::blocks(G5::SaveFamily::BlackWhite));
    keepAlive.push_back(bytes);
    std::string error;
    auto parsed = G5::Gen5ReadOnlySave::parse(sav, "black_nds", &error, G5::SaveCopySelection::Primary);
    if (!parsed) return nullptr;
    auto* trainer = keep(PokeVault::Legacy::Gen5ReadOnlyTrainer::create(*parsed, "black_nds", error));
    if (!trainer) return nullptr;
    return screenFor(*trainer, "Pokemon Black", "black_nds", "DraStic", SK::ExternalLegacy);
}

std::vector<Game> games() {
    std::vector<Game> out;
    //               gen name        open          item empty party  box
    out.push_back({1, "Red",      openRed,      1,   0,    false, false});
    out.push_back({2, "Gold",     openGold,     0,   2,    true,  true});
    out.push_back({3, "FireRed",  openFireRed,  0,   1,    true,  false});
    out.push_back({4, "Platinum", openPlatinum, 4,   0,    true,  true});
    out.push_back({5, "Black",    openBlack,    3,   2,    true,  false});
    for (auto& g : out) check(g.open() != nullptr, g.name + " opens");
    return out;
}

bool has(const Footer& f, const std::string& glyph, const std::string& label) {
    return std::find(f.begin(), f.end(), Hint{glyph, label}) != f.end();
}

} // namespace

int main(int argc, char** argv) {
    verbose = argc > 1 && std::string(argv[1]) == "-v";
    auto list = games();
    check(list.size() == 5, "all five generations open");
    for (auto& g : list)
        for (Kind k : {Kind::Hub, Kind::BagItem, Kind::BagEmpty, Kind::AddPicker, Kind::RemoveConfirm, Kind::Party, Kind::Boxes,
                       Kind::Trainer, Kind::Editor, Kind::Review, Kind::Exit})
            audit(g, k);

    // ONE wording per screen kind: every generation in the same state (a Party / Box slot with or
    // without a Pokemon) shows exactly the same footer.
    const auto occupied = [&](Kind kind, int gen) {
        for (const auto& g : list)
            if (g.generation == gen) return kind == Kind::Party ? g.partyOccupied : kind == Kind::Boxes && g.boxOccupied;
        return false;
    };
    for (const auto& [kind, byGen] : table) {
        if (verbose) {
            std::cout << "== " << kindName(kind) << '\n';
            for (const auto& [gen, f] : byGen)
                std::cout << "  Gen " << gen << (occupied(kind, gen) ? " (occupied)" : "") << ": " << str(f) << '\n';
        }
        // The one per-generation data term the editor footer keeps: Gen I / II re-roll DVs, Gen III-V
        // IVs / EVs (different stored values, different action).
        const auto canonical = [&](Footer f) {
            if (kind == Kind::Editor)
                for (auto& h : f)
                    if (h.first == "Y" && h.second == "Random DVs") h.second = "Reroll IVs/EVs";
            return f;
        };
        for (const auto& [gen, raw] : byGen) {
            const auto f = canonical(raw);
            const auto reference = std::find_if(byGen.begin(), byGen.end(), [&](const auto& entry) {
                return occupied(kind, entry.first) == occupied(kind, gen);
            });
            check(f == canonical(reference->second), std::string(kindName(kind)) + ": Gen " + std::to_string(gen) + " footer \"" +
                                              str(f) + "\" == Gen " + std::to_string(reference->first) + " \"" +
                                              str(reference->second) + "\"");
        }
    }
    // An empty Party slot drops only A (nothing to open); every other hint stays.
    if (table.count(Kind::Party) && table[Kind::Party].count(1) && table[Kind::Party].count(2)) {
        auto withoutA = table[Kind::Party][2];
        std::erase_if(withoutA, [](const Hint& h) { return h.first == "A"; });
        check(table[Kind::Party][1] == withoutA, "empty Party slot: the occupied footer without A");
    }
    // The canonical Gen I-III Bag wording (owner R08: A is "Edit Amount", never "Edit Quantity" / "Quantity").
    for (const auto& [gen, f] : table[Kind::BagItem]) {
        const std::string g = "Gen " + std::to_string(gen) + " Bag: ";
        check(has(f, "A", "Edit Amount"), g + "A: Edit Amount");
        check(has(f, "X", "Add Item") && has(f, "Y", "Remove Item"), g + "X: Add Item, Y: Remove Item");
        check(has(f, "-", "Help") && has(f, "B", "Back") && has(f, "+", "Review"), g + "+ Review, - Help, B Back");
    }
    // Every hub's + opens the shared Review and B returns to the hub (Gen I-IV ClassicBrowseReview,
    // Gen V routeHub5), with one staged change.
    for (auto& g : list) {
        auto* screen = g.open();
        Driver d(*screen);
        enter(g, d, Kind::Exit);   // stages one Bag change, then B / B: the unsaved-changes question
        d.press(HidNpadButton_B);  // Continue Editing: back on the hub
        const std::string what = "Gen " + std::to_string(g.generation) + " hub";
        check(!d.screen.detailViewActive && !d.drawn("Review Pending Changes"), what + ": on the hub, no Review");
        check(has(footer(), "+", "Review"), what + ": the footer names + Review");
        d.press(HidNpadButton_Plus);
        check(d.drawn("Review Pending Changes"), what + ": + opens the shared Review");
        d.press(HidNpadButton_B);
        check(!d.drawn("Review Pending Changes") && !d.screen.detailViewActive && !d.screen.goBack,
              what + ": B returns to the hub");
    }

    // One shared export-result wording (UI/ExportMessages.h) for every generation.

    // Every hub's + opens the shared Review and B returns to the hub (Gen I-IV ClassicBrowseReview,
    // Gen V routeHub5), with one staged change.
    for (auto& g : list) {
        auto* screen = g.open();
        Driver d(*screen);
        enter(g, d, Kind::Exit);   // stages one Bag change, then B / B: the unsaved-changes question
        d.press(HidNpadButton_B);  // Continue Editing: back on the hub
        const std::string what = "Gen " + std::to_string(g.generation) + " hub";
        check(!d.screen.detailViewActive && !d.drawn("Review Pending Changes"), what + ": on the hub, no Review");
        check(has(footer(), "+", "Review"), what + ": the footer names + Review");
        d.press(HidNpadButton_Plus);
        check(d.drawn("Review Pending Changes"), what + ": + opens the shared Review");
        d.press(HidNpadButton_B);
        check(!d.drawn("Review Pending Changes") && !d.screen.detailViewActive && !d.screen.goBack,
              what + ": B returns to the hub");
    }

    // One shared export-result wording (UI/ExportMessages.h) for every generation.
    namespace Msg = PokeBank::UIModel::ExportMessages;
    check(Msg::exported("sdmc:/x") == "Verified copy exported: sdmc:/x (original save unchanged)", "export wording");
    check(Msg::failed("source changed") == "Export failed: source changed (original save unchanged)" &&
              Msg::failed("") == "Export failed (original save unchanged)",
          "export failure wording");

    if (failures) {
        std::cerr << failures << " of " << checks << " footer parity host checks failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "R08 footer parity through the real Gen I-V screens (Bag, Add picker, Party, Boxes, Trainer; "
                 "same wording, every advertised button acts) ("
              << checks << " checks): PASS\n";
    return EXIT_SUCCESS;
}
