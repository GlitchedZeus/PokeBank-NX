// The ONE shared Gen VI-IX surface (src/UI/ModernSharedSurface.inc) for Legends: Arceus, through the
// REAL TrainerViewScreen::update / draw (tests/host-ui harness), over the Gen8a adapter and a
// synthetic SAV8LA `main` (tests/fixtures/gen8a): Party -> Edit friendship -> Keep, Boxes Clone /
// Move / Release, Alpha shown read-only, Items unavailable with the registry's reason, Review, a
// verified Export Copy that strictly reopens, and Discard All. Not a hardware claim.
#include "HostUi.h"

#include "Enums/GameVersion.h"
#include "Integration/Gen8a/Gen8aNativeSession.h"
#include "Integration/Modern/NativeSession.h"
#include "Legacy/ModernReadOnlyTrainer.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SharedPokemonEditorWorkspace.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"
#include "fixtures/gen8a/Gen8aSyntheticSave.h"

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

namespace G8a = PokeVault::Integration::Gen8a;
namespace Modern = PokeVault::Integration::Modern;
using Modern::SlotRef;

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
    char pattern[] = "/tmp/modern-ui-gen8a-XXXXXX";
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
    bool tap(int id) {
        frame();
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

void enterHubTile(Driver& d, UI::TrainerViewScreen& screen, int tile) {
    screen.homeMenuIndex = tile;
    d.press(HidNpadButton_A);
}

} // namespace

int main() {
    const std::string dir = makeTempDir();
    const std::string savePath = dir + "/main";
    const auto original = Gen8aFixture::makeSave();
    {
        std::ofstream out(savePath, std::ios::binary);
        out.write(reinterpret_cast<const char*>(original.data()), static_cast<std::streamsize>(original.size()));
    }
    auto opened = G8a::Gen8aNativeSession::open(slurp(savePath), "legends_arceus_switch");
    check(opened.ready(), "synthetic PLA main opens: " + opened.diagnostic);
    if (!opened.ready()) return EXIT_FAILURE;
    std::string error;
    auto trainer = PokeVault::Legacy::ModernReadOnlyTrainer::create(std::move(opened.session), "legends_arceus_switch",
                                                                    Enums::GameVersion::PLA, error);
    check(trainer != nullptr, "the shared bridge wraps the PLA session: " + error);
    if (!trainer) return EXIT_FAILURE;
    auto& session = trainer->session();
    UI::TrainerViewScreen screen(*trainer, "Pokemon Legends Arceus", savePath, 0, AccountUid{},
                                 PokeVault::Safety::SourceKind::ExternalLegacy, "legends_arceus_switch", "Manual");
    Driver d{screen, {}, {}};
    d.frame();
    check(!screen.detailViewActive && trainer->getBoxCount() == 32 && trainer->getPartySize() == 2, "hub over 32 x 30 boxes");

    enterHubTile(d, screen, 1);
    screen.selectedPartyIndex = 0;
    d.press(HidNpadButton_A);
    check(drawn("View") && drawn("Edit") && !drawn("Clone") && !drawn("Release"), "party Actions: no storage actions");
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    check(session.draftActive(), "Edit opens a draft");
    // Editable rows after Nickname: OT, Trainer ID, Secret ID, Friendship (Level and Held Item are
    // read-only in Legends: Arceus, so Down skips them): four presses.
    for (int i = 0; i < 4; ++i) d.press(HidNpadButton_Down);
    HostUi::numbers.push_back({true, 222});
    const int promptsBefore = HostUi::promptCount;
    d.press(HidNpadButton_A);
    const bool staged = HostUi::promptCount == promptsBefore + 1 && session.view(SlotRef::party(0))->friendship == 222;
    check(staged && session.draftDirty(), "friendship staged through the keypad");
    d.press(HidNpadButton_X);
    check(!session.draftActive() && session.stagedAt(SlotRef::party(0)), "Keep stages it");
    d.press(HidNpadButton_B);

    enterHubTile(d, screen, 0);
    screen.selectedBoxIndex = 0;
    screen.selectedItemIndex = 1;
    d.press(HidNpadButton_A);
    check(drawn("Clone") && drawn("Move") && drawn("Release"), "box record lists Clone / Move / Release");
    HostUi::numbers.push_back({true, 3});
    HostUi::numbers.push_back({true, 4});
    check(d.tap(kActionTouchBase + 2), "Clone tap");
    check(session.view(SlotRef::boxed(2, 3))->species == 403, "clone staged at Box 3 / Slot 4 (" + screen.storageStatus + ")");
    screen.selectedBoxIndex = 31;
    screen.selectedItemIndex = 29;
    d.press(HidNpadButton_A);
    HostUi::numbers.push_back({true, 2});
    HostUi::numbers.push_back({true, 1});
    check(d.tap(kActionTouchBase + 3), "Move tap");
    check(session.view(SlotRef::boxed(1, 0))->species == 493 && session.view(SlotRef::boxed(31, 29))->empty, "move staged");
    screen.selectedBoxIndex = 0;
    screen.selectedItemIndex = 0;
    d.press(HidNpadButton_A);
    check(d.tap(kActionTouchBase + 4), "Release tap");
    d.press(HidNpadButton_Y);
    check(session.view(SlotRef::boxed(0, 0))->empty, "Y released the Alpha in the staged copy");
    d.press(HidNpadButton_B);

    const std::string exportRoot = dir + "/exports";
    UI::ModernSharedSurface::exportRootOverrideForHostTests = exportRoot;
    d.press(HidNpadButton_Plus);
    check(drawn("Review Pending Changes") && session.reviewRows().size() == 5, "Review lists five changes");
    d.press(HidNpadButton_X);
    check(drawn("Export Verified Generation VIII Working Copy?"), "X asks to export");
    d.press(HidNpadButton_Y);
    const auto exported = entries(exportRoot);
    check(exported.size() == 1 && exported.front().rfind("legends_arceus_switch_", 0) == 0, "one export directory");
    if (exported.size() == 1) {
        const std::string out = exportRoot + "/" + exported.front();
        check(exists(out + "/EDIT_MANIFEST.txt") && slurp(out + "/original_backup.bin") == original, "backup + manifest");
        auto re = G8a::Gen8aNativeSession::open(slurp(out + "/edited.bin"), "legends_arceus_switch");
        check(re.ready() && re.session->view(SlotRef::party(0))->friendship == 222 && re.session->view(SlotRef::boxed(1, 0))->species == 493 &&
                  re.session->view(SlotRef::boxed(0, 0))->empty,
              "edited.bin strictly reopens with the reviewed changes");
    }
    check(slurp(savePath) == original, "fixture unchanged");
    d.press(HidNpadButton_B);

    enterHubTile(d, screen, 3);
    check(drawn("Bag unavailable") && drawnContaining("PlayerBag8a"), "Items names the missing verified editor");
    d.press(HidNpadButton_B);
    d.press(HidNpadButton_Plus);
    d.press(HidNpadButton_Y);
    d.press(HidNpadButton_Y);
    check(!session.hasChanges() && session.view(SlotRef::boxed(0, 0))->species == 399, "Discard All restores");

    std::cout << "test_ui_modern_surface_gen8a_host: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
