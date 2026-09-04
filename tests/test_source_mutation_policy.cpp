#include <cassert>
#include <iostream>
#include "Safety/SourceMutationPolicy.h"
#include "Safety/WritePolicy.h"
#include "UI/ActionSheetModel.h"

int main() {
    using namespace PokeVault::Safety;
    using namespace PokeVault::UIModel;
    for (const auto action : {SourceMutation::Release, SourceMutation::CreatePokemon,
             SourceMutation::DirectMove, SourceMutation::Edit, SourceMutation::Rename,
             SourceMutation::EditTrainer, SourceMutation::EditItems, SourceMutation::SaveChanges}) {
        assert(!canPerform(SourceKind::InstalledGame, action));
        assert(canPerform(SourceKind::BackupOrStaged, action));
        assert(canPerform(SourceKind::AppOwnedStorage, action));
    }
    assert(canPerform(SourceKind::InstalledGame, SourceMutation::View));
    for (auto location : {PokemonLocation::Party, PokemonLocation::SaveBox, PokemonLocation::Bank}) {
        PokemonActionSheet sheet;
        for (int repeat = 0; repeat < 100; ++repeat) {
            sheet.open({location, 0, 0});
            sheet.select(4); // Edit is unavailable in installed-source browsing.
            assert(sheet.activate(false) == ActionResult::NotYetSupported);
            assert(sheet.isOpen());
            sheet.select(0);
            assert(sheet.activate(false) == ActionResult::OpenView);
            sheet.open({location, 0, 0});
            assert(sheet.close() == ActionResult::Closed);
            sheet.open({location, 0, 0});
            sheet.select(8);
            assert(sheet.activate(false) == ActionResult::Closed);
        }
    }
    assert(dispatchAction(PokemonAction::Edit, true) == ActionResult::OpenEditor);
    std::cout << "Source mutation policy: PASS\n";
}
