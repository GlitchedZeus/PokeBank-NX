// The ONE shared Gen VI-IX surface (src/UI/ModernSharedSurface.inc) for Let's Go Pikachu, through the
// REAL TrainerViewScreen::update / draw (tests/host-ui harness), over the Gen7b adapter and a
// synthetic SAV7b `main` (tests/fixtures/gen7b). Same screens and controls as every other Gen VI+
// title: Party -> Edit -> Keep, Boxes Clone (appended at the next free slot) and Release (compaction),
// the partner and party members protected, no Move (gapless list), Items unavailable with the
// registry's reason, Review, a verified Export Copy that strictly reopens, and Discard All.
// Not a hardware claim.
#include "HostUi.h"

#include "Enums/GameVersion.h"
#include "Integration/Gen7b/Gen7bNativeSession.h"
#include "Integration/Modern/NativeSession.h"
#include "Legacy/ModernReadOnlyTrainer.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SharedPokemonEditorWorkspace.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"
#include "fixtures/gen7b/Gen7bSyntheticSave.h"

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

namespace G7b = PokeVault::Integration::Gen7b;
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
    char pattern[] = "/tmp/modern-ui-gen7b-XXXXXX";
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

uint32_t ecAt(const PokeVault::Integration::Modern::NativeSession& s, SlotRef at) {
    const auto v = s.view(at);
    return v ? v->encryptionConstant : 0;
}

int main() {
    const std::string dir = makeTempDir();
    const std::string savePath = dir + "/main";
    const auto original = Gen7bFixture::makeSave();
    {
        std::ofstream out(savePath, std::ios::binary);
        out.write(reinterpret_cast<const char*>(original.data()), static_cast<std::streamsize>(original.size()));
    }
    auto opened = G7b::Gen7bNativeSession::open(slurp(savePath), "letsgo_pikachu_switch");
    check(opened.ready(), "synthetic Let's Go main opens through the Gen7b adapter: " + opened.diagnostic);
    if (!opened.ready()) return EXIT_FAILURE;
    std::string error;
    auto trainer = PokeVault::Legacy::ModernReadOnlyTrainer::create(std::move(opened.session), "letsgo_pikachu_switch",
                                                                    Enums::GameVersion::GG, error);
    check(trainer != nullptr, "the shared Modern bridge wraps the Gen7b session: " + error);
    if (!trainer) return EXIT_FAILURE;
    auto& session = trainer->session();
    UI::TrainerViewScreen screen(*trainer, "Pokemon Let's Go Pikachu", savePath, 0, AccountUid{},
                                 PokeVault::Safety::SourceKind::ExternalLegacy, "letsgo_pikachu_switch", "Manual");
    Driver d{screen, {}, {}};
    d.frame();
    check(!screen.detailViewActive && trainer->getBoxCount() == 40 && trainer->getSlotsPerBox() == 25 && trainer->getPartySize() == 2,
          "hub over 40 x 25 storage with the two-member party");

    // ---- Party: the partner offers View / Edit / Clone but never Release / Move ----
    enterHubTile(d, screen, 1);
    check(screen.selectedMode == UI::TrainerViewScreen::ViewMode::Party && d.target(kPartyTouchBase + 1) != nullptr &&
              d.target(kPartyTouchBase + 2) == nullptr,
          "Party panel with one touch target per party pointer");
    screen.selectedPartyIndex = 0;
    d.press(HidNpadButton_A);
    check(drawn("View") && drawn("Edit") && drawn("Clone") && !drawn("Move") && !drawn("Release"),
          "partner in the party: View / Edit / Clone, no Move / Release");
    check(drawnContaining("Nintendo Switch SOURCE / READ ONLY"), "the sheet names the Switch source as read-only");
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    check(session.draftActive() && drawnContaining("STAGED"), "Edit opens the shared workspace on a draft");
    // Walk the editable rows until a keypad answer lands in TID16 (OT name is a text row, skipped by
    // giving it no answer; the first numeric editable row after Nickname/OT is Trainer ID).
    bool tidStaged = false;
    for (int i = 0; i < 6 && !tidStaged; ++i) {
        d.press(HidNpadButton_Down);
        HostUi::numbers.push_back({true, 4242});
        d.press(HidNpadButton_A);
        tidStaged = session.view(SlotRef::party(0))->tid == 4242;
        HostUi::numbers.clear();
    }
    check(tidStaged && session.draftDirty(), "the keypad answer is staged on Trainer ID in the draft");
    d.press(HidNpadButton_X);
    check(!session.draftActive() && session.stagedAt(SlotRef::boxed(0, 0)), "Keep stages the partner's storage record");
    d.press(HidNpadButton_B);
    check(!screen.detailViewActive, "back on the hub");

    // ---- Boxes: a box record offers Clone and Release, never Move ----
    enterHubTile(d, screen, 0);
    screen.selectedBoxIndex = 0;
    screen.selectedItemIndex = 3;
    d.press(HidNpadButton_A);
    check(drawn("View") && drawn("Edit") && drawn("Clone") && drawn("Release") && !drawn("Move"),
          "a box record lists View / Edit / Clone / Release (no Move: gapless storage)");
    HostUi::numbers.push_back({true, 1});
    HostUi::numbers.push_back({true, 7});
    check(d.tap(kActionTouchBase + 2), "Clone row tap");
    check(ecAt(session, SlotRef::boxed(0, 6)) == ecAt(session, SlotRef::boxed(0, 3)) && ecAt(session, SlotRef::boxed(0, 6)) != 0,
          "Clone appended at Box 1 / Slot 7 (" + screen.storageStatus + ")");

    const uint32_t movedUp = ecAt(session, SlotRef::boxed(0, 3));
    screen.selectedItemIndex = 2;
    d.press(HidNpadButton_A);
    check(d.tap(kActionTouchBase + 3), "Release row tap");
    check(drawn("Release this Pokemon?"), "Release asks for confirmation");
    d.press(HidNpadButton_Y);
    check(ecAt(session, SlotRef::boxed(0, 2)) == movedUp && session.view(SlotRef::boxed(0, 6))->empty,
          "Y released Box 1 / Slot 3 and later Pokemon moved up one slot");
    check(trainer->boxes[0][2] && !trainer->boxes[0][6], "the Boxes presentation shows the compaction");
    d.press(HidNpadButton_B);

    // ---- Review lists the changes; X Export; Y confirms; a verified copy appears ----
    const std::string exportRoot = dir + "/exports";
    UI::ModernSharedSurface::exportRootOverrideForHostTests = exportRoot;
    d.press(HidNpadButton_Plus);
    check(drawn("Review Pending Changes") && drawnContaining("clone of Box 1 / Slot 4") && drawnContaining("released"),
          "Review lists the clone and the release");
    check(session.reviewRows().size() == 3, "three review rows: TID edit, clone, release");
    d.press(HidNpadButton_X);
    check(drawn("Export Verified Generation VII Working Copy?"), "X asks to export");
    d.press(HidNpadButton_Y);
    check(screen.storageStatus.find("Verified app-owned Generation VII copy: ") == 0, "export status: " + screen.storageStatus);
    const auto exported = entries(exportRoot);
    check(exported.size() == 1, "one export directory under the override root");
    if (exported.size() == 1) {
        const std::string out = exportRoot + "/" + exported.front();
        check(exported.front().rfind("letsgo_pikachu_switch_", 0) == 0, "directory stem names the exact game");
        check(exists(out + "/original_backup.bin") && exists(out + "/edited.bin") && exists(out + "/EDIT_MANIFEST.txt"),
              "backup + edited + manifest written");
        check(slurp(out + "/original_backup.bin") == original, "the backup is the pristine source");
        const auto edited = slurp(out + "/edited.bin");
        auto reopened = G7b::Gen7bNativeSession::open(edited, "letsgo_pikachu_switch");
        check(reopened.ready(), "edited.bin re-parses strictly: " + reopened.diagnostic);
        if (reopened.ready())
            check(reopened.session->view(SlotRef::party(0))->tid == 4242 && reopened.session->partyCount() == 2 &&
                      ecAt(*reopened.session, SlotRef::boxed(0, 2)) == movedUp,
                  "the exported copy carries the edit and the compaction with the party intact");
    }
    check(slurp(savePath) == original, "the fixture file bytes are unchanged");
    d.press(HidNpadButton_B);

    // ---- Items: unavailable with the registry's named prerequisite ----
    enterHubTile(d, screen, 3);
    check(drawn("Bag unavailable") && drawnContaining("MyItem") && !drawnContaining("failed validation"),
          "Items names the missing verified decoder");
    d.press(HidNpadButton_B);

    // ---- Discard All restores everything ----
    d.press(HidNpadButton_Plus);
    d.press(HidNpadButton_Y);
    d.press(HidNpadButton_Y);
    check(!session.hasChanges() && session.view(SlotRef::party(0))->tid != 4242 && session.view(SlotRef::boxed(0, 6))->empty &&
              ecAt(session, SlotRef::boxed(0, 2)) != movedUp,
          "Discard All restores the source view");
    check(slurp(savePath) == original, "the fixture file is byte-identical");

    std::cout << "test_ui_modern_surface_gen7b_host: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
