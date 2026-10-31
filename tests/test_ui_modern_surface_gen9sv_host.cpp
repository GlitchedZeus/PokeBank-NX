// The ONE shared Gen VI-IX surface (src/UI/ModernSharedSurface.inc) for Scarlet, through
// the REAL TrainerViewScreen::update / draw (tests/host-ui harness), over the Gen9 SV adapter and a
// synthetic SAV9SV main (tests/fixtures/gen9sv), reusing the Sword/Shield scenario unchanged. Same screens and
// controls as Gen VI/VII: Party -> Edit -> Keep, Boxes Clone / Move / Release, battle-team members
// protected in the Actions sheet, Items honestly unavailable with the registry's reason, Trainer,
// Review, a verified Export Copy that strictly reopens, and Discard All. Not a hardware claim.
#include "HostUi.h"

#include "Enums/GameVersion.h"
#include "Integration/Gen9/Gen9SvNativeSession.h"
#include "Integration/Modern/NativeSession.h"
#include "Legacy/ModernReadOnlyTrainer.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SharedPokemonEditorWorkspace.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"
#include "fixtures/gen9sv/Gen9SvSyntheticSave.h"

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

namespace G8 = PokeVault::Integration::Gen9Sv;
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
    char pattern[] = "/tmp/modern-ui-gen9sv-XXXXXX";
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
// Edit opens focused on the first editable DETAILS row (Nickname) and Down skips rows the Gen VIII
// provider marks read-only (Level is ReadOnly here, unlike Gen VI), so Friendship is one press
// closer than on Gen VI: OT, Trainer ID, SID, Held Item, Friendship.
constexpr int kDownsToFriendship = 5;

void enterHubTile(Driver& d, UI::TrainerViewScreen& screen, int tile) {
    screen.homeMenuIndex = tile;
    d.press(HidNpadButton_A);
}

} // namespace

int main() {
    const std::string dir = makeTempDir();
    const std::string savePath = dir + "/main";
    const auto original = Gen9SvFixture::makeSave();
    {
        std::ofstream out(savePath, std::ios::binary);
        out.write(reinterpret_cast<const char*>(original.data()), static_cast<std::streamsize>(original.size()));
    }
    auto opened = G8::Gen9SvNativeSession::open(slurp(savePath), "scarlet_switch");
    check(opened.ready(), "synthetic Scarlet/Violet save opens through the Gen9 SV adapter: " + opened.diagnostic);
    if (!opened.ready()) return EXIT_FAILURE;
    std::string error;
    auto trainer = PokeVault::Legacy::ModernReadOnlyTrainer::create(std::move(opened.session), "scarlet_switch",
                                                                    Enums::GameVersion::SV, error);
    check(trainer != nullptr, "the shared Modern bridge wraps the Gen9 SV session: " + error);
    if (!trainer) return EXIT_FAILURE;
    auto& session = trainer->session();
    UI::TrainerViewScreen screen(*trainer, "Pokemon Scarlet", savePath, 0, AccountUid{},
                                 PokeVault::Safety::SourceKind::ExternalLegacy, "scarlet_switch", "Manual");
    Driver d{screen, {}, {}};
    d.frame();
    check(!screen.detailViewActive, "opens on the loaded-game hub");
    check(trainer->getBoxCount() == 32 && trainer->getSlotsPerBox() == 30 && trainer->getPartySize() == 2,
          "32 x 30 native boxes and the declared party");

    // A Scarlet/Violet screen that is NOT an assigned external file never reaches the shared surface's
    // static_cast to the Modern bridge (installed titles use the legacy reader).
    {
        UI::TrainerViewScreen installed(*trainer, "Pokemon Scarlet", savePath, 0, AccountUid{},
                                        PokeVault::Safety::SourceKind::InstalledGame, "scarlet_switch", "Manual");
        Driver di{installed, {}, {}};
        di.frame();
        enterHubTile(di, installed, 1);
        installed.selectedPartyIndex = 0;
        di.press(HidNpadButton_A);
        check(!drawnContaining("Nintendo Switch SOURCE / READ ONLY") && !session.draftActive(),
              "an InstalledGame-kind screen is not driven by the shared Modern surface");
    }

    // ---- Party -> Actions (no storage actions for Scarlet/Violet party) -> Edit -> Friendship -> Keep ----
    enterHubTile(d, screen, 1);
    check(screen.selectedMode == UI::TrainerViewScreen::ViewMode::Party && d.target(kPartyTouchBase + 1) != nullptr &&
              d.target(kPartyTouchBase + 2) == nullptr,
          "Party panel with one touch target per declared party slot");
    screen.selectedPartyIndex = 0;
    d.press(HidNpadButton_A);
    check(drawn("View") && drawn("Edit") && drawn("Origin & Legality") && drawn("Cancel"), "Party Actions");
    check(!drawn("Clone") && !drawn("Move") && !drawn("Release"),
          "Scarlet/Violet party records offer no Clone/Move/Release (deposit/compaction rules not verified)");
    check(drawnContaining("Nintendo Switch SOURCE / READ ONLY"), "the sheet names the Switch source as read-only");
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
    d.press(HidNpadButton_B);
    check(!screen.detailViewActive, "back on the hub");

    // ---- Boxes: a battle-team member offers Clone but never Move / Release ----
    enterHubTile(d, screen, 0);
    check(screen.selectedMode == UI::TrainerViewScreen::ViewMode::Boxes, "the Pokemon tile enters Boxes");
    screen.selectedBoxIndex = 0;
    screen.selectedItemIndex = 1;   // Box 1 slot 2: species 396, referenced by battle team 1
    d.press(HidNpadButton_A);
    check(drawn("Clone") && !drawn("Move") && !drawn("Release"),
          "a battle-team member can be cloned but is never offered Move/Release");
    d.press(HidNpadButton_B);

    // ---- Clone / Move / Release on a team-free record ----
    screen.selectedItemIndex = 0;   // Box 1 slot 1: species 921
    d.press(HidNpadButton_A);
    check(drawn("View") && drawn("Edit") && drawn("Clone") && drawn("Move") && drawn("Release"),
          "an unmodified team-free box record lists View / Edit / Clone / Move / Release");
    HostUi::numbers.push_back({true, 3});
    HostUi::numbers.push_back({true, 4});
    check(d.tap(kActionTouchBase + 2), "Clone row tap");
    check(session.view(SlotRef::boxed(2, 3))->species == 921 && session.view(SlotRef::boxed(0, 0))->species == 921,
          "Clone staged a copy in Box 3 slot 4 and kept the original (" + screen.storageStatus + ")");
    check(trainer->boxes[2][3] && trainer->boxes[2][3]->speciesID() == 921, "the Boxes presentation shows the clone");

    screen.selectedBoxIndex = 31;
    screen.selectedItemIndex = 29;  // Box 32 slot 30: species 1007
    d.press(HidNpadButton_A);
    HostUi::numbers.push_back({true, 2});
    HostUi::numbers.push_back({true, 1});
    check(d.tap(kActionTouchBase + 3), "Move row tap");
    check(session.view(SlotRef::boxed(1, 0))->species == 1007 && session.view(SlotRef::boxed(31, 29))->empty,
          "Move staged species 1007 into Box 2 slot 1 and emptied the source (" + screen.storageStatus + ")");

    screen.selectedBoxIndex = 0;
    screen.selectedItemIndex = 0;   // Box 1 slot 1 again: the clone lives in Box 3, the source is still unmodified
    d.press(HidNpadButton_A);
    check(d.tap(kActionTouchBase + 4), "Release row tap");
    check(drawn("Release this Pokemon?"), "Release asks for confirmation");
    d.press(HidNpadButton_Y);
    check(session.view(SlotRef::boxed(0, 0))->empty && !trainer->boxes[0][0], "Y released Box 1 slot 1 in the staged copy");
    d.press(HidNpadButton_B);

    // ---- Review lists the changes; X Export; Y confirms; a verified copy appears ----
    const std::string exportRoot = dir + "/exports";
    UI::ModernSharedSurface::exportRootOverrideForHostTests = exportRoot;
    d.press(HidNpadButton_Plus);
    check(drawn("Review Pending Changes"), "+ on the hub opens Review");
    check(drawnContaining("Clone of Box 1 / Slot 1"), "Review lists the clone");
    check(drawnContaining("Removed"), "Review lists the removals");
    check(session.reviewRows().size() == 5, "five review rows: clone, move (clone + removal), release, friendship");
    // Box rows sort before Party rows; the friendship row is below the first page.
    for (int i = 0; i < 6 && !drawnContaining("friendship 70 -> 222"); ++i) d.press(HidNpadButton_Down);
    check(drawnContaining("friendship 70 -> 222"), "scrolling Review reaches the friendship edit");
    check(drawnContaining("Generation IX"), "Review names the generation");
    d.press(HidNpadButton_X);
    check(drawn("Export Verified Generation IX Working Copy?"), "X asks to export");
    d.press(HidNpadButton_Y);
    check(screen.storageStatus.find("Verified app-owned Generation IX copy: ") == 0, "export status: " + screen.storageStatus);
    const auto exported = entries(exportRoot);
    check(exported.size() == 1, "one export directory under the override root");
    if (exported.size() == 1) {
        const std::string out = exportRoot + "/" + exported.front();
        check(exported.front().rfind("scarlet_switch_", 0) == 0, "directory stem names the exact game: " + exported.front());
        check(exists(out + "/original_backup.bin") && exists(out + "/edited.bin") && exists(out + "/EDIT_MANIFEST.txt"),
              "backup + edited + manifest written");
        check(slurp(out + "/original_backup.bin") == original, "the backup is the pristine source");
        const auto edited = slurp(out + "/edited.bin");
        check(edited.size() == original.size() && edited != original, "edited.bin differs from the source");
        auto reopened = G8::Gen9SvNativeSession::open(edited, "scarlet_switch");
        check(reopened.ready(), "edited.bin re-parses strictly as Scarlet: " + reopened.diagnostic);
        if (reopened.ready()) {
            auto& r = *reopened.session;
            check(r.view(SlotRef::party(0))->friendship == 222 && r.view(SlotRef::boxed(2, 3))->species == 921 &&
                      r.view(SlotRef::boxed(1, 0))->species == 1007 && r.view(SlotRef::boxed(31, 29))->empty &&
                      r.view(SlotRef::boxed(0, 0))->empty && r.view(SlotRef::boxed(0, 1))->species == 194,
                  "the exported copy carries exactly the reviewed changes");
        }
    }
    check(slurp(savePath) == original, "the fixture file bytes are unchanged");
    d.press(HidNpadButton_B);

    // ---- Items: unavailable, with the registry's named prerequisite ----
    enterHubTile(d, screen, 3);
    check(screen.selectedMode == UI::TrainerViewScreen::ViewMode::Items && drawn("Bag unavailable"),
          "Items shows Bag unavailable for Scarlet/Violet");
    check(drawnContaining("MyItem9") && !drawnContaining("failed validation"),
          "the reason is the missing verified decoder, not a false validation failure");
    d.press(HidNpadButton_B);

    // ---- Trainer page ----
    enterHubTile(d, screen, 4);
    check(screen.selectedMode == UI::TrainerViewScreen::ViewMode::Trainer && drawnContaining("Juliana"), "Trainer page shows the trainer");
    d.press(HidNpadButton_B);

    // ---- Discard All restores everything; source untouched ----
    d.press(HidNpadButton_Plus);
    d.press(HidNpadButton_Y);
    d.press(HidNpadButton_Y);
    check(!session.hasChanges() && trainer->party[0]->otFriendship() == 70 && session.view(SlotRef::boxed(0, 0))->species == 921 &&
              session.view(SlotRef::boxed(2, 3))->empty && session.view(SlotRef::boxed(31, 29))->species == 1007,
          "Discard All restores the source view");
    check(slurp(savePath) == original, "the fixture file is byte-identical");

    std::error_code ec;
    (void)ec;
    std::cout << "test_ui_modern_surface_gen9sv_host: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
