// The ONE shared Gen VI-IX application surface (src/UI/ModernSharedSurface.inc) through the REAL
// TrainerViewScreen::update / draw with scripted pad and touch input (tests/host-ui harness), over
// the Gen VI adapter and the synthetic X save: hub -> Party -> Actions (View / Edit / Clone, never Add
// for an empty box slot: the registry reason is shown instead) -> Edit -> Friendship keypad -> Keep ->
// Review lists the change -> X Export -> Y -> a verified separate copy appears under the override root
// while the fixture file is byte-identical; release-confirmed taps on Party and Actions targets; Items
// and Trainer pages; Discard All; the staged-exit confirmation on B. Nothing here is a hardware claim.
#include "HostUi.h"

#include "Enums/GameVersion.h"
#include "Integration/Gen6/Gen6Bag.h"
#include "Integration/Gen6/Gen6NativeSession.h"
#include "Integration/Modern/NativeSession.h"
#include "Legacy/ModernReadOnlyTrainer.h"
#include "UI/PKSEFramebuffer.h"
#include "UI/SharedPokemonEditorWorkspace.h"
#include "UI/TouchInput.h"
#include "UI/TrainerViewScreen.h"
#include "fixtures/gen6/Gen6SyntheticSave.h"

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

namespace G6 = PokeVault::Integration::Gen6;
namespace Modern = PokeVault::Integration::Modern;
namespace Workspace = PokeBank::UIModel::SharedPokemonWorkspace;
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
// Edit opens focused on the first editable DETAILS row (Nickname, row 1) and Down skips the rows
// the Gen VI provider marks read-only: Level 2, OT 4, Trainer ID 5, SID 6, Held Item 7,
// Friendship 8, so Friendship is six presses away.
constexpr int kDownsToFriendship = 6;

struct Fixture {
    std::string dir;
    std::string savePath;
    std::vector<uint8_t> original;
    std::unique_ptr<PokeVault::Legacy::ModernReadOnlyTrainer> trainer;
};

Fixture openFixture() {
    Fixture f;
    f.dir = makeTempDir();
    f.savePath = f.dir + "/main";
    f.original = Gen6Fixture::makeSave({});
    // A native MyItem block: two Items stacks and one Medicine stack (XY pocket offsets).
    const auto medicine = G6::bagPocketLayout6(Gen6Fixture::Layout::Family::XY,
                                               static_cast<size_t>(G6::BagPocket6::Medicine)).offset;
    Gen6Fixture::writeBag(f.original, Gen6Fixture::Layout::Family::XY, {{0, 1, 3}, {0, 4, 10}, {medicine, 17, 2}});
    Gen6Fixture::resignAll(f.original, Gen6Fixture::Layout::Family::XY);
    {
        std::ofstream out(f.savePath, std::ios::binary);
        out.write(reinterpret_cast<const char*>(f.original.data()), static_cast<std::streamsize>(f.original.size()));
    }
    auto opened = G6::Gen6NativeSession::open(slurp(f.savePath), "x_3ds");
    check(opened.ready(), "synthetic X save opens through the Gen VI adapter: " + opened.diagnostic);
    if (!opened.ready()) return f;
    std::string error;
    f.trainer = PokeVault::Legacy::ModernReadOnlyTrainer::create(std::move(opened.session), "x_3ds", Enums::GameVersion::XY, error);
    check(f.trainer != nullptr, "the shared Modern bridge wraps the session: " + error);
    return f;
}

void enterHubTile(Driver& d, UI::TrainerViewScreen& screen, int tile) {
    screen.homeMenuIndex = tile;   // the base hub owns cursor movement; the surface owns activation
    d.press(HidNpadButton_A);
}

} // namespace

int main() {
    Fixture f = openFixture();
    if (!f.trainer) return EXIT_FAILURE;
    auto& session = f.trainer->session();
    UI::TrainerViewScreen screen(*f.trainer, "Pokemon X", f.savePath, 0, AccountUid{},
                                 PokeVault::Safety::SourceKind::ExternalLegacy, "x_3ds", "Checkpoint");
    Driver d{screen, {}, {}};
    d.frame();
    check(!screen.detailViewActive, "opens on the loaded-game hub");

    // ---- hub -> Party -> Actions: View / Edit / Clone, Origin & Legality, Cancel ----
    enterHubTile(d, screen, 1);
    check(screen.detailViewActive && screen.selectedMode == UI::TrainerViewScreen::ViewMode::Party, "the Party tile enters the Party panel");
    check(d.target(kPartyTouchBase) != nullptr && d.target(kPartyTouchBase + 1) != nullptr, "Party publishes a touch target per party slot");
    check(d.target(kPartyTouchBase + 2) == nullptr, "no target for a party slot the save does not have");
    screen.selectedPartyIndex = 0;
    d.press(HidNpadButton_A);
    check(drawn("View") && drawn("Edit") && drawn("Clone") && drawn("Origin & Legality") && drawn("Cancel"),
          "Party Actions list View / Edit / Clone / Origin & Legality / Cancel");
    check(!drawn("Move") && !drawn("Release"), "party records offer no Move / Release (adapter refuses party removal)");
    check(!drawn("Add Pokemon"), "an occupied slot never lists Add");
    check(d.targetsFrom(kActionTouchBase, 10) == 5, "five release-confirmed Actions touch targets");
    check(drawnContaining("Nintendo 3DS SOURCE / READ ONLY"), "the sheet names the 3DS source as read-only");
    d.press(HidNpadButton_B);
    check(!drawn("Cancel"), "B closes the Actions sheet");

    // ---- an empty box slot: Cancel only, with the registry's Create reason in the footer ----
    d.press(HidNpadButton_B);   // back to the hub
    check(!screen.detailViewActive, "B returns to the hub");
    enterHubTile(d, screen, 0);
    check(screen.selectedMode == UI::TrainerViewScreen::ViewMode::Boxes, "the Pokemon tile enters Boxes");
    screen.selectedBoxIndex = 5;
    screen.selectedItemIndex = 5;
    d.press(HidNpadButton_A);
    check(drawn("Cancel") && !drawn("Add Pokemon") && !drawn("Edit"), "empty box slot: Cancel only, never Add");
    check(drawnContaining("Add Pokemon unavailable"), "the sheet footer says why Add is missing");
    check(screen.storageStatus.find("Gen6StagedSession::createCapability") != std::string::npos ||
              screen.storageStatus.find("Create") != std::string::npos,
          "the status line carries the registry's Create prerequisite (" + screen.storageStatus + ")");
    d.press(HidNpadButton_B);
    d.press(HidNpadButton_B);
    check(!screen.detailViewActive, "back on the hub");

    // ---- Storage is refused with the format named ----
    screen.homeMenuIndex = 2;
    d.press(HidNpadButton_A);
    check(!screen.detailViewActive && screen.storageStatus.find("Legacy Storage (Bank) has no PK6 conversion") != std::string::npos,
          "Storage refused: " + screen.storageStatus);

    // ---- Edit: Friendship keypad -> Keep -> staged ----
    enterHubTile(d, screen, 1);
    screen.selectedPartyIndex = 0;
    d.press(HidNpadButton_A);
    d.press(HidNpadButton_Down);   // View -> Edit
    d.press(HidNpadButton_A);
    check(session.draftActive() && session.draftSlot() == SlotRef::party(0), "Actions A on Edit begins the adapter draft on Party 1");
    check(drawnContaining("STAGED"), "the workspace header reads STAGED");
    // The DETAILS panel scrolls: the first rows are visible, the Gen V-shaped tail (OT Gender,
    // Fateful) scrolls in later.
    check(drawn("Species") && drawn("Nickname") && drawn("Held Item") && drawn("Friendship"), "the DETAILS rows open at the top of the Gen V set");
    for (int i = 0; i < kDownsToFriendship; ++i) d.press(HidNpadButton_Down);
    check(d.target(Workspace::kDetailTouchBase + kFriendshipRow) != nullptr, "Down reached the Friendship row (its touch target is visible)");
    HostUi::numbers.push_back({true, 200});
    d.press(HidNpadButton_A);
    check(HostUi::promptCount == 1 && HostUi::numbers.empty(), "A on Friendship opened exactly one keypad");
    check(session.view(SlotRef::party(0))->friendship == 200 && session.draftDirty(), "the keypad answer is staged in the draft");
    check(f.trainer->party.size() == 2 && f.trainer->party[0]->otFriendship() == 70, "Keep has not happened yet: the presentation is unchanged");
    d.press(HidNpadButton_X);   // Keep
    check(!session.draftActive() && session.hasChanges() && session.stagedAt(SlotRef::party(0)), "X keeps the draft as a staged change");
    check(f.trainer->party[0]->otFriendship() == 200, "the Trainer presentation was refreshed from the staged view");
    check(screen.storageStatus.find("PK6 draft kept in app memory only") != std::string::npos, "Keep status: " + screen.storageStatus);
    check(screen.detailViewActive && screen.selectedMode == UI::TrainerViewScreen::ViewMode::Party, "Keep returns to the Party panel");

    // ---- read-only moves explain instead of opening the move editor ----
    d.press(HidNpadButton_A);
    d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);   // Edit again
    check(session.draftActive(), "Edit reopened");
    d.press(HidNpadButton_R);   // DETAILS -> STATS
    d.press(HidNpadButton_R);   // STATS -> MOVES
    d.press(HidNpadButton_A);
    check(screen.storageStatus.find("Moves are read-only for Generation VI") != std::string::npos, "A on a move row explains (" + screen.storageStatus + ")");
    check(!drawn("Move 1"), "no move editor opened");
    d.press(HidNpadButton_B);   // a clean draft closes silently
    check(!session.draftActive() && screen.selectedMode == UI::TrainerViewScreen::ViewMode::Party, "B on a clean draft returns to Party");

    // ---- Review lists the change; X Export; Y confirms; a verified copy appears ----
    const std::string exportRoot = f.dir + "/exports";
    UI::ModernSharedSurface::exportRootOverrideForHostTests = exportRoot;
    d.press(HidNpadButton_Y);   // Party: Y is the one Review key
    check(drawn("Review Pending Changes"), "Y on Party opens Review");
    check(drawn("Party 1") && drawnContaining("friendship 70 -> 200"), "Review names the staged friendship change at its slot");
    check(drawnContaining("Generation VI / original source remains unchanged"), "Review names the generation");
    d.press(HidNpadButton_X);
    check(drawn("Export Verified Generation VI Working Copy?"), "X asks to export");
    check(drawnContaining("never modified"), "the export question states the source stays untouched");
    d.press(HidNpadButton_Y);
    check(screen.storageStatus.find("Verified app-owned Generation VI copy: ") == 0, "export status: " + screen.storageStatus);
    const auto exported = entries(exportRoot);
    check(exported.size() == 1, "one export directory under the override root");
    if (exported.size() == 1) {
        const std::string dir = exportRoot + "/" + exported.front();
        check(exported.front().rfind("x_3ds_", 0) == 0, "directory stem names the exact game: " + exported.front());
        check(exists(dir + "/original_backup.sav") && exists(dir + "/edited.sav") && exists(dir + "/EDIT_MANIFEST.txt"),
              "backup + edited + manifest written");
        check(slurp(dir + "/original_backup.sav") == f.original, "the backup is the pristine source");
        const auto edited = slurp(dir + "/edited.sav");
        check(edited.size() == f.original.size() && edited != f.original, "edited.sav differs from the source");
        auto reopened = G6::Gen6NativeSession::open(edited, "x_3ds");
        check(reopened.ready(), "edited.sav re-parses as x_3ds: " + reopened.diagnostic);
        if (reopened.ready())
            check(reopened.session->view(SlotRef::party(0))->friendship == 200, "the exported copy carries the new friendship");
    }
    check(slurp(f.savePath) == f.original, "the fixture file bytes are unchanged");
    check(drawn("Review Pending Changes"), "export returns to Review");
    d.press(HidNpadButton_B);
    check(screen.selectedMode == UI::TrainerViewScreen::ViewMode::Party && !drawn("Review Pending Changes"), "B leaves Review");

    // ---- touch: a tap on Party slot 2, then a tap on the View row ----
    check(d.tap(kPartyTouchBase + 1), "Party slot 2 publishes a touch target");
    check(screen.selectedPartyIndex == 1 && drawn("Ivysaur"), "the tap opens Actions for the tapped slot");
    check(d.tap(kActionTouchBase + 0), "the View row publishes a touch target");
    check(drawnContaining("READ ONLY") && !session.draftActive(), "tapping View opens the read-only workspace without a draft");
    d.press(HidNpadButton_A);
    check(!screen.storageStatus.empty(), "A in View explains why nothing changes (" + screen.storageStatus + ")");
    d.press(HidNpadButton_B);
    check(screen.selectedMode == UI::TrainerViewScreen::ViewMode::Party && !drawnContaining("READ ONLY"), "B leaves View");

    // ---- Items: the adapter's pockets, read-only pockets refuse ----
    d.press(HidNpadButton_B);
    enterHubTile(d, screen, 3);
    check(screen.selectedMode == UI::TrainerViewScreen::ViewMode::Items && drawn("Items - Items"), "the Items tile shows the first native pocket");
    d.press(HidNpadButton_R);
    check(drawn("Items - Key Items"), "R moves to the next pocket");
    d.press(HidNpadButton_X);
    check(screen.storageStatus.find("story pockets") != std::string::npos, "Add on a read-only pocket is refused with its reason (" + screen.storageStatus + ")");
    d.press(HidNpadButton_L);
    d.press(HidNpadButton_X);
    check(drawn("Add Item"), "X on an editable pocket opens the shared Add picker");
    const size_t changesBeforeAdd = session.changeCount();
    d.press(HidNpadButton_A);   // the first choice (Master Ball) is already stacked: focus it, stage nothing
    check(session.bag()[0].items.size() == 2 && session.changeCount() == changesBeforeAdd &&
              screen.storageStatus.find("Already in this pocket") != std::string::npos,
          "Add of an item already in the pocket focuses its stack (status: " + screen.storageStatus + ")");
    d.press(HidNpadButton_X);
    d.press(HidNpadButton_Right);   // Ultra Ball (2): not in the pocket
    d.press(HidNpadButton_A);
    check(session.bag()[0].items.size() == 3 && session.changeCount() == changesBeforeAdd + 1, "A stages a new item x1 with no keypad (status: " + screen.storageStatus + ")");
    check(HostUi::promptCount == 1, "no keypad for Add");
    d.press(HidNpadButton_B);
    check(!screen.detailViewActive, "B returns to the hub from Items");

    // ---- Trainer page: facts read-only, Pokedex summary opens ----
    enterHubTile(d, screen, 4);
    check(screen.selectedMode == UI::TrainerViewScreen::ViewMode::Trainer && drawn("Trainer") && drawn("Pokédex"), "the Trainer tile shows the facts and the Pokedex row");
    check(drawn("TESTER") && drawn("12345"), "trainer facts come from the adapter");
    d.press(HidNpadButton_A);
    check(screen.storageStatus.find("read-only") != std::string::npos, "A on a fact says it is read-only (" + screen.storageStatus + ")");
    const int rows = d.targetsFrom(kTrainerRowTouchBase, 32);
    check(rows >= 2, "trainer rows publish touch targets");
    for (int i = 0; i < rows - 1; ++i) d.press(HidNpadButton_Down);
    d.press(HidNpadButton_A);
    check(drawn("Pokedex - Generation VI (read-only)") && drawnContaining("of 721"), "the Pokedex row opens the counts-only summary");
    d.press(HidNpadButton_B);
    d.press(HidNpadButton_Y);
    check(drawn("Review Pending Changes"), "Y on Trainer opens Review");
    d.press(HidNpadButton_B);
    d.press(HidNpadButton_B);
    check(!screen.detailViewActive, "back on the hub");

    // ---- Boxes: Clone, Move and Remove through the real Actions sheet ----
    enterHubTile(d, screen, 0);
    check(screen.selectedMode == UI::TrainerViewScreen::ViewMode::Boxes, "the Pokemon tile enters Boxes");
    screen.selectedBoxIndex = 0;
    screen.selectedItemIndex = 1;               // Box 1 slot 2: species 101
    d.press(HidNpadButton_A);
    check(drawn("View") && drawn("Edit") && drawn("Clone") && drawn("Move") && drawn("Release"),
          "an unmodified box record lists View / Edit / Clone / Move / Release");
    const int promptsBeforeClone = HostUi::promptCount;
    HostUi::numbers.push_back({true, 2});      // Box 2
    HostUi::numbers.push_back({true, 1});      // Slot 1
    check(d.tap(kActionTouchBase + 2), "the Clone row publishes a touch target");
    check(HostUi::promptCount == promptsBeforeClone + 2, "Clone asks for the destination box and slot");
    check(session.view(SlotRef::boxed(1, 0))->species == 101 && session.view(SlotRef::boxed(0, 1))->species == 101,
          "Clone staged a copy in Box 2 slot 1 and kept the original (" + screen.storageStatus + ")");
    check(f.trainer->boxes[1][0] && f.trainer->boxes[1][0]->speciesID() == 101, "the Boxes presentation shows the clone");

    screen.selectedItemIndex = 2;               // Box 1 slot 3: Furfrou in a trim form
    d.press(HidNpadButton_A);
    check(!drawn("Clone") && !drawn("Move") && drawn("Release"),
          "a trimmed Furfrou is never offered Clone/Move (the lane refuses boxing a non-base form)");
    d.press(HidNpadButton_B);

    screen.selectedItemIndex = 0;               // Box 1 slot 1: species 100
    d.press(HidNpadButton_A);
    HostUi::numbers.push_back({true, 2});
    HostUi::numbers.push_back({true, 2});
    check(d.tap(kActionTouchBase + 3), "the Move row publishes a touch target");
    check(session.view(SlotRef::boxed(1, 1))->species == 100 && session.view(SlotRef::boxed(0, 0))->empty,
          "Move staged species 100 into Box 2 slot 2 and emptied the source (" + screen.storageStatus + ")");
    check(f.trainer->boxes[1][1] && !f.trainer->boxes[0][0], "the Boxes presentation shows the move");

    screen.selectedItemIndex = 1;               // Box 1 slot 2 again (still unmodified: the clone lives in Box 2)
    d.press(HidNpadButton_A);
    check(d.tap(kActionTouchBase + 4), "the Release row publishes a touch target");
    check(drawn("Release this Pokemon?"), "Release asks for confirmation");
    d.press(HidNpadButton_A);
    check(!session.view(SlotRef::boxed(0, 1))->empty, "A does not release; only Y confirms");
    d.press(HidNpadButton_Y);
    check(session.view(SlotRef::boxed(0, 1))->empty && !f.trainer->boxes[0][1], "Y released Box 1 slot 2 in the staged copy");

    screen.selectedItemIndex = 5;               // an empty slot: Create is not offered for Gen VI
    d.press(HidNpadButton_A);
    check(!drawn("Add Pokemon") && drawn("Review Pending Changes"), "an empty slot offers Review, never Add, once changes exist");
    d.press(HidNpadButton_B);
    d.press(HidNpadButton_Y);
    check(drawnContaining("Clone of Box 1 / Slot 2") && drawnContaining("Removed"), "Review lists the clone and the removals");
    d.press(HidNpadButton_B);
    d.press(HidNpadButton_B);
    check(!screen.detailViewActive, "back on the hub");

    // ---- Discard All from Review ----
    d.press(HidNpadButton_Plus);   // the hub's Review key
    check(drawn("Review Pending Changes"), "+ on the hub opens Review");
    d.press(HidNpadButton_Y);
    check(drawn("Discard all staged changes?"), "Y asks to discard all");
    d.press(HidNpadButton_Y);
    check(!session.hasChanges() && session.view(SlotRef::party(0))->friendship == 70 && f.trainer->party[0]->otFriendship() == 70 &&
              session.view(SlotRef::boxed(0, 0))->species == 100 && session.view(SlotRef::boxed(0, 1))->species == 101 &&
              session.view(SlotRef::boxed(1, 0))->empty && session.view(SlotRef::boxed(1, 1))->empty &&
              f.trainer->boxes[0][0] && f.trainer->boxes[0][1] && !f.trainer->boxes[1][0],
          "Discard All restores the source view and presentation");
    check(!screen.detailViewActive && !drawn("Review Pending Changes"), "Discard All returns to the hub");

    // ---- B-exit with staged changes: Continue, then Discard & Exit ----
    std::string error;
    check(session.stageValue(SlotRef::boxed(0, 0), Modern::Field::HeldItem, 5, 0, error), "stage a change: " + error);
    d.press(HidNpadButton_B);
    check(!screen.goBack && drawn("Unsaved Changes"), "B on the hub with staged changes asks first");
    d.press(HidNpadButton_B);
    check(!screen.goBack && !drawn("Unsaved Changes"), "B continues editing");
    d.press(HidNpadButton_B);
    d.press(HidNpadButton_A);
    check(drawn("Review Pending Changes"), "A reviews from the exit question");
    d.press(HidNpadButton_B);
    d.press(HidNpadButton_B);
    d.press(HidNpadButton_Y);
    check(screen.goBack && !session.hasChanges(), "Y discards and exits");
    check(slurp(f.savePath) == f.original, "the fixture file is still byte-identical");

    std::cout << "test_ui_modern_surface_host: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
