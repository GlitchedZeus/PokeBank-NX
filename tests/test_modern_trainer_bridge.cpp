// The ONE Trainer bridge for Modern-adapter titles: the shared Trainer/Party/Boxes/Items panels
// read party/boxes/items/trainer facts through ModernReadOnlyTrainer, built from the adapter's
// staged view, and every inherited writer is a no-op. Exercised here over the Gen VI adapter.
#include "Legacy/ModernReadOnlyTrainer.h"

#include "Integration/Gen6/Gen6NativeSession.h"
#include "Integration/Gen6/Gen6Bag.h"
#include "Pokemon/ModernRecordView.h"
#include "fixtures/gen6/Gen6SyntheticSave.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace G6 = PokeVault::Integration::Gen6;
namespace Modern = PokeVault::Integration::Modern;
namespace Layout = PokeVault::Integration::Gen6::Layout;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}
} // namespace

int main() {
    auto save = Gen6Fixture::makeSave({});
    const auto medicine = G6::bagPocketLayout6(Layout::Family::XY, static_cast<size_t>(G6::BagPocket6::Medicine)).offset;
    Gen6Fixture::writeBag(save, Layout::Family::XY, {{0, 1, 3}, {medicine, 17, 2}});
    Gen6Fixture::resignAll(save, Layout::Family::XY);
    const auto pristine = save;

    std::string error;
    auto opened = G6::Gen6NativeSession::open(save, "x_3ds");
    check(opened.ready(), "fixture opens: " + opened.diagnostic);
    if (!opened.ready()) return EXIT_FAILURE;

    // Mismatched exact title is refused by the bridge, not papered over.
    {
        auto other = G6::Gen6NativeSession::open(save, "x_3ds");
        check(!PokeVault::Legacy::ModernReadOnlyTrainer::create(std::move(other.session), "y_3ds", Enums::GameVersion::XY, error) &&
                  !error.empty(),
              "bridge refuses a title mismatch");
        check(!PokeVault::Legacy::ModernReadOnlyTrainer::create(nullptr, "x_3ds", Enums::GameVersion::XY, error), "bridge refuses a null session");
    }

    auto trainer = PokeVault::Legacy::ModernReadOnlyTrainer::create(std::move(opened.session), "x_3ds", Enums::GameVersion::XY, error);
    check(trainer != nullptr, "bridge created: " + error);
    if (!trainer) return EXIT_FAILURE;
    auto& t = *trainer;
    check(t.getGameGroup() == Enums::GameVersion::XY && t.sourceGameId() == "x_3ds", "group and exact id");
    check(t.getBoxCount() == 31 && t.getSlotsPerBox() == 30 && t.getPartySize() == 2, "native geometry through the Trainer interface");
    check(t.party.size() == 2 && t.boxes.size() == 31, "presentation vectors sized natively");
    check(t.party[0] && t.party[0]->speciesID() == 1 && t.party[0]->level() == 12, "party slot 0 presented");
    check(t.party[1] && t.party[1]->speciesID() == 2, "party slot 1 presented");
    check(t.boxes[0][0] && t.boxes[0][0]->speciesID() == 100 && t.boxes[0][2] && t.boxes[0][2]->speciesID() == 676 &&
              t.boxes[0][2]->form() == 2 && !t.boxes[0][3] && !t.boxes[30][29],
          "box presentation mirrors the native records; blanks stay empty");
    check(t.trainerName == "TESTER" && t.TID16 == 12345 && t.SID16 == 54321 && t.ID32 == (54321u << 16 | 12345u) && t.trainerGender == 1,
          "trainer identity from the session, not parsed from strings");
    check(t.boxNames.size() == 31 && !t.boxNames[0].empty(), "31 box names");
    check(t.items.size() == 5 && t.items[0].size() == 1 && t.items[0][0].itemId == 1 && t.items[0][0].count == 3 &&
              t.items[3].size() == 1 && t.items[3][0].itemId == 17,
          "five pockets presented in the shared Items panel order");
    check(t.pokedexProgress().available() && t.pokedexProgress().total == 721, "dex progress available");
    check(!t.hasStagedChanges() && !t.supportsBoxNames() && t.createBlankPokemon() == nullptr, "no staged changes, no writers");
    check(t.saveRevisionString.rfind("Generation VI / READ ONLY", 0) == 0, "revision string names the family and read-only state");

    // Staging through the session is reflected only after an explicit presentation refresh.
    auto& s = t.session();
    check(s.stageValue(Modern::SlotRef::boxed(0, 0), Modern::Field::HeldItem, 5, 0, error), "stage held item: " + error);
    check(t.hasStagedChanges(), "trainer reports staged changes");
    check(t.boxes[0][0]->heldItem() == 1, "presentation is a snapshot until refreshed");
    check(t.refreshPresentation(error) && t.boxes[0][0]->heldItem() == 5, "refresh shows the staged value");
    check(s.clone(Modern::SlotRef::party(0), Modern::SlotRef::boxed(1, 0), error) && t.refreshPresentation(error) &&
              t.boxes[1][0] && t.boxes[1][0]->speciesID() == 1,
          "clone appears after refresh");
    check(s.remove(Modern::SlotRef::boxed(0, 1), error) && t.refreshPresentation(error) && !t.boxes[0][1], "remove clears the slot after refresh");
    s.discardAll();
    check(t.refreshPresentation(error) && t.boxes[0][0]->heldItem() == 1 && t.boxes[0][1] && !t.boxes[1][0], "discard all restores the presentation");

    // Inherited writers are inert: calling them changes nothing anywhere.
    t.updatePartyBlock();
    t.updateBoxBlock();
    t.updateItemBlock();
    t.updateTrainerInfoBlock();
    t.updateBoxNameBlock();
    t.updateCurrentBoxBlock();
    check(save == pristine, "the caller's bytes never changed");
    check(s.sourceBytes().size() == pristine.size() && std::equal(s.sourceBytes().begin(), s.sourceBytes().end(), pristine.begin()),
          "the session's immutable source never changed");

    std::cout << "test_modern_trainer_bridge: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
