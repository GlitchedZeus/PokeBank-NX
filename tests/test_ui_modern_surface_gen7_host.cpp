// The ONE shared Gen VI-IX surface (src/UI/ModernSharedSurface.inc) for a Gen VII title, through the
// REAL TrainerViewScreen::update / draw (tests/host-ui harness), over the Gen VII adapter and a
// synthetic, MemeCrypto-signed Sun save. Same screens and controls as Gen VI: Party, Edit -> Keep,
// Boxes Clone / Move / Release, Items with the seven native pockets (Z-Crystals read-only), Trainer,
// Review and Discard All. Export Copy is the one difference: SAV7 re-signing has no retail
// acceptance evidence, so X in Review explains that and NOTHING is written. Not a hardware claim.
#include "HostUi.h"

#include "Enums/GameVersion.h"
#include "Integration/Gen7/Gen7Bag.h"
#include "Integration/Gen7/Gen7NativeSession.h"
#include "Integration/Modern/NativeSession.h"
#include "Legacy/ModernReadOnlyTrainer.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SharedPokemonEditorWorkspace.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"
#include "fixtures/gen6/Gen6SyntheticSave.h"
#include "fixtures/gen7/Gen7SyntheticSave.h"

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace UI::ModernSharedSurface {
extern std::string exportRootOverrideForHostTests;
}

namespace G7 = PokeVault::Integration::Gen7;
namespace Modern = PokeVault::Integration::Modern;
namespace Workspace = PokeBank::UIModel::SharedPokemonWorkspace;
namespace Layout = PokeVault::Integration::Gen7::Layout;
namespace PK7 = PokeVault::Integration::Gen7::PK7;
using Modern::SlotRef;
using G7::BagPocket7;

namespace {

int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}
bool drawn(const std::string& text) {
    return std::any_of(HostUi::texts.begin(), HostUi::texts.end(), [&](const std::string& t) { return t == text; });
}
bool drawnContaining(const std::string& part) {
    return std::any_of(HostUi::texts.begin(), HostUi::texts.end(),
                       [&](const std::string& t) { return t.find(part) != std::string::npos; });
}
std::string makeTempDir() {
    char pattern[] = "/tmp/modern-ui-XXXXXX";
    const char* dir = ::mkdtemp(pattern);
    return dir ? dir : "";
}
std::vector<uint8_t> slurp(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
bool exists(const std::string& path) { return std::ifstream(path, std::ios::binary).good(); }
std::vector<std::string> entries(const std::string& dir) {
    std::vector<std::string> out;
    if (DIR* d = ::opendir(dir.c_str())) {
        while (const dirent* e = ::readdir(d)) {
            const std::string name = e->d_name;
            if (name != "." && name != "..") out.push_back(name);
        }
        ::closedir(d);
    }
    std::sort(out.begin(), out.end());
    return out;
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
    int targetsFrom(int base, int span) const {
        int n = 0;
        for (const auto& b : screen.touchButtons)
            if (b.id >= base && b.id < base + span) ++n;
        return n;
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

constexpr int kActionTouchBase = 79000;
constexpr int kPartyTouchBase = 79400;
constexpr int kTrainerRowTouchBase = 82000;
constexpr int kFriendshipRow = 8;   // ModernSharedSurface DetailRow::Friendship
// Edit opens focused on the first editable DETAILS row (Nickname, row 1); Down visits every row the
// Gen VII provider marks editable: Level 2, Hyper Training 3, OT 4, Trainer ID 5, SID 6, Held Item 7,
// Friendship 8, so Friendship is seven presses away.
constexpr int kDownsToFriendship = 7;


constexpr size_t P(BagPocket7 p) { return static_cast<size_t>(p); }
std::vector<uint8_t> saveWithBag(Layout::Family f, uint8_t version) {
    auto save = Gen7Fixture::makeSave({f, version, 2, false, false});
    for (size_t s = 0; s < 3; ++s) {
        Gen6Fixture::MonSpec m;
        m.species = s == 2 ? 103 : static_cast<uint16_t>(100 + s);
        m.form = s == 2 ? 1 : 0;   // Alolan Exeggutor
        m.ec = 0xA0000000u + static_cast<uint32_t>(s) * 0x01010101u;
        m.pid = 0x0B000000u + static_cast<uint32_t>(s) * 0x00FF00FFu;
        m.version = version;
        Gen6Fixture::put(save, G7::Gen7Save::boxRecordOffset(f, 0, s), Gen6Fixture::encryptedMon(m, false));
    }
    const auto block = Layout::blocks(f)[G7::ItemsBlock7];
    std::fill_n(save.begin() + block.offset, block.length, uint8_t{0});
    const auto put = [&](size_t pocket, size_t slot, uint32_t value) {
        PK7::write32(save, block.offset + G7::bagPocketLayout7(f, pocket).offset + slot * 4, value);
    };
    put(P(BagPocket7::Items), 0, 1u | (3u << 10));
    put(P(BagPocket7::Items), 1, 4u | (10u << 10));
    put(P(BagPocket7::Medicine), 0, 17u | (2u << 10));
    put(P(BagPocket7::KeyItems), 0, 465u | (1u << 10));
    put(P(BagPocket7::ZCrystals), 0, 807u | (1u << 10));
    if (f == Layout::Family::UltraSunUltraMoon) put(P(BagPocket7::BattleItems), 0, 949u | (1u << 10));
    Gen7Fixture::resignBlocks(save, f);
    Gen7Fixture::signMeme(save, f);
    return save;
}

void enterHubTile(Driver& d, UI::TrainerViewScreen& screen, int tile) {
    screen.homeMenuIndex = tile;
    d.press(HidNpadButton_A);
}

} // namespace

int main() {
    const std::string dir = makeTempDir();
    const std::string savePath = dir + "/main";
    const auto original = saveWithBag(Layout::Family::SunMoon, Layout::VersionSun);
    {
        std::ofstream out(savePath, std::ios::binary);
        out.write(reinterpret_cast<const char*>(original.data()), static_cast<std::streamsize>(original.size()));
    }
    auto opened = G7::Gen7NativeSession::open(slurp(savePath), "sun_3ds");
    check(opened.ready(), "signed synthetic Sun save opens through the Gen VII adapter: " + opened.diagnostic);
    if (!opened.ready()) return EXIT_FAILURE;
    std::string error;
    auto trainer = PokeVault::Legacy::ModernReadOnlyTrainer::create(std::move(opened.session), "sun_3ds", Enums::GameVersion::SM, error);
    check(trainer != nullptr, "the shared Modern bridge wraps the Gen VII session: " + error);
    if (!trainer) return EXIT_FAILURE;
    auto& session = trainer->session();
    UI::TrainerViewScreen screen(*trainer, "Pokemon Sun", savePath, 0, AccountUid{},
                                 PokeVault::Safety::SourceKind::ExternalLegacy, "sun_3ds", "Checkpoint");
    Driver d{screen, {}, {}};
    d.frame();
    check(!screen.detailViewActive, "opens on the loaded-game hub");
    check(trainer->getBoxCount() == 32 && trainer->getSlotsPerBox() == 30, "32 x 30 native boxes");

    // ---- Party -> Edit -> Friendship -> Keep ----
    enterHubTile(d, screen, 1);
    check(screen.selectedMode == UI::TrainerViewScreen::ViewMode::Party && d.target(kPartyTouchBase + 1) != nullptr,
          "Party panel with a touch target per party slot");
    screen.selectedPartyIndex = 0;
    d.press(HidNpadButton_A);
    check(drawn("View") && drawn("Edit") && drawn("Clone") && drawn("Origin & Legality"), "Party Actions");
    check(drawnContaining("Nintendo 3DS SOURCE / READ ONLY"), "the sheet names the 3DS source as read-only");
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    check(session.draftActive() && drawnContaining("STAGED"), "Edit opens the shared workspace on a draft");
    for (int i = 0; i < kDownsToFriendship; ++i) d.press(HidNpadButton_Down);
    HostUi::numbers.push_back({true, 222});
    const int promptsBefore = HostUi::promptCount;
    d.press(HidNpadButton_A);
    check(HostUi::promptCount == promptsBefore + 1, "A on Friendship opened the keypad");
    check(session.view(SlotRef::party(0))->friendship == 222 && session.draftDirty(), "the keypad answer is staged in the draft");
    d.press(HidNpadButton_X);
    check(!session.draftActive() && session.stagedAt(SlotRef::party(0)) && trainer->party[0]->otFriendship() == 222,
          "Keep stages the change and refreshes the presentation");
    // ---- Hyper Training through the same workspace: Level 100 (keypad), then the Hyper Training row ----
    d.press(HidNpadButton_A);                       // Actions sheet again (View / Edit / ...)
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    check(session.draftActive(), "Edit reopens a draft for Hyper Training");
    d.press(HidNpadButton_Down);                    // Nickname -> Level (editable in SM/USUM)
    HostUi::numbers.push_back({true, 100});
    d.press(HidNpadButton_A);
    check(session.view(SlotRef::party(0))->level == 100, "the Level keypad staged level 100");
    d.press(HidNpadButton_Down);                    // Level -> Hyper Training (row 3, editable in SM/USUM)
    check(drawn("HyperTrain") && drawn("None"), "the HyperTrain row is drawn in full and shows None");
    check(drawnContaining("EXP"), "the Level row shows the stored EXP");
    d.press(HidNpadButton_A);
    check(drawnContaining("Hyper Training: toggle a stat") && drawnContaining("Atk: not trained"),
          "A opens the stat picker");
    const auto atkBefore = session.view(SlotRef::party(0))->stats[1];
    d.press(HidNpadButton_Down);                    // HP -> Atk
    d.press(HidNpadButton_A);
    check(session.view(SlotRef::party(0))->hyperTrainFlags == 0x02, "choosing Atk trains Atk (bit 1)");
    check(session.view(SlotRef::party(0))->stats[1] > atkBefore, "the party Atk stat is recomputed with IV 31");
    d.frame();
    check(drawnContaining("Atk"), "the row shows the trained stat");
    d.press(HidNpadButton_X);
    check(!session.draftActive() && session.stagedAt(SlotRef::party(0)), "Keep stages Hyper Training");
    bool reviewRow = false;
    for (const auto& r : session.reviewRows())
        reviewRow |= (r.label + r.value).find("Hyper Training -> Atk") != std::string::npos;
    check(reviewRow, "Review lists the Hyper Training change");

    d.press(HidNpadButton_B);   // Party -> hub
    check(!screen.detailViewActive, "back on the hub");

    // ---- Boxes: Clone, then Release ----
    enterHubTile(d, screen, 0);
    screen.selectedBoxIndex = 0;
    screen.selectedItemIndex = 2;   // Alolan Exeggutor
    d.press(HidNpadButton_A);
    check(drawn("Clone") && drawn("Move") && drawn("Release"), "box record lists Clone / Move / Release");
    HostUi::numbers.push_back({true, 3});
    HostUi::numbers.push_back({true, 4});
    check(d.tap(kActionTouchBase + 2), "Clone row tap");
    check(session.view(SlotRef::boxed(2, 3))->species == 103 && session.view(SlotRef::boxed(2, 3))->form == 1,
          "Clone staged the Alolan form into Box 3 slot 4 (" + screen.storageStatus + ")");
    screen.selectedItemIndex = 1;
    d.press(HidNpadButton_A);
    check(d.tap(kActionTouchBase + 4), "Release row tap");
    d.press(HidNpadButton_Y);
    check(session.view(SlotRef::boxed(0, 1))->empty && !trainer->boxes[0][1], "Y released Box 1 slot 2");

    // ---- Review: changes listed; X explains the SAV7 gate and writes nothing ----
    const std::string exportRoot = dir + "/exports";
    UI::ModernSharedSurface::exportRootOverrideForHostTests = exportRoot;
    d.press(HidNpadButton_B);
    d.press(HidNpadButton_Plus);
    check(drawn("Review Pending Changes"), "+ on the hub opens Review");
    check(drawnContaining("friendship") && drawnContaining("Clone of Box 1 / Slot 3"), "Review lists the Gen VII changes");
    d.press(HidNpadButton_X);
    check(!drawnContaining("Export Verified") && screen.storageStatus.find("SAV7") != std::string::npos,
          "X explains that SAV7 export is blocked (" + screen.storageStatus + ")");
    check(entries(exportRoot).empty() && !exists(exportRoot), "no export directory or file was created");
    d.press(HidNpadButton_B);

    // ---- Items: seven native pockets on Sun/Moon minus the Ultra-only one; Z-Crystals read-only ----
    enterHubTile(d, screen, 3);
    check(screen.selectedMode == UI::TrainerViewScreen::ViewMode::Items && drawn("Items - Items"), "Items opens on the first pocket");
    const auto pockets = session.bag();
    bool zReadOnly = false, hasZ = false;
    for (const auto& p : pockets) if (p.name.find("Z-Crystal") != std::string::npos) { hasZ = true; zReadOnly = !p.editable && !p.reason.empty(); }
    check(hasZ && zReadOnly, "the Z-Crystals pocket is present and read-only with a reason");
    d.press(HidNpadButton_B);

    // ---- Trainer page ----
    enterHubTile(d, screen, 4);
    check(screen.selectedMode == UI::TrainerViewScreen::ViewMode::Trainer && drawn("Pokédex"), "Trainer page");
    d.press(HidNpadButton_B);

    // ---- Discard All restores everything; source untouched ----
    d.press(HidNpadButton_Plus);
    d.press(HidNpadButton_Y);
    d.press(HidNpadButton_Y);
    check(!session.hasChanges() && trainer->party[0]->otFriendship() == 70 && session.view(SlotRef::boxed(0, 1))->species == 101 &&
              session.view(SlotRef::boxed(2, 3))->empty,
          "Discard All restores the source view");
    check(slurp(savePath) == original, "the fixture file is byte-identical");

    // ---- Ultra Sun: the same surface, the Ultra-only pocket appears ----
    {
        const auto usum = saveWithBag(Layout::Family::UltraSunUltraMoon, Layout::VersionUltraSun);
        auto u = G7::Gen7NativeSession::open(usum, "ultrasun_3ds");
        check(u.ready() && u.session->bag().size() == session.bag().size() + 1, "Ultra Sun adds the Ultra-only pocket");
    }

    std::cout << "test_ui_modern_surface_gen7_host: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
