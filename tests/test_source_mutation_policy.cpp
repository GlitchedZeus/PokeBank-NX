#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "Safety/SourceMutationPolicy.h"
#include "Safety/WritePolicy.h"
#include "UI/ActionSheetModel.h"
#include "UI/MutationTargetPolicy.h"

namespace {
std::string read(const char* path) {
    std::ifstream in(path);
    assert(in);
    return {std::istreambuf_iterator<char>(in), {}};
}
}

int main() {
    using namespace PokeVault::Safety;
    using namespace PokeVault::UIModel;

    for (const auto action : {SourceMutation::Release, SourceMutation::CreatePokemon,
             SourceMutation::DirectMove, SourceMutation::Edit, SourceMutation::Rename,
             SourceMutation::EditTrainer, SourceMutation::EditItems, SourceMutation::SaveChanges}) {
        assert(!canPerform(SourceKind::InstalledGame, action));
        assert(!canPerform(SourceKind::RetroArchLegacy, action));
        assert(!canPerform(SourceKind::ExternalLegacy, action));
        assert(canPerform(SourceKind::BackupOrStaged, action));
        assert(canPerform(SourceKind::AppOwnedStorage, action));
    }
    assert(canPerform(SourceKind::InstalledGame, SourceMutation::View));
    assert(canPerform(SourceKind::RetroArchLegacy, SourceMutation::View));

    // Party/SaveBox inherit the immutable session source. Bank is app-owned and
    // remains mutable even while browsing an immutable installed/emulator source.
    for (const auto source : {SourceKind::InstalledGame, SourceKind::RetroArchLegacy,
                              SourceKind::ExternalLegacy}) {
        assert(!PokeBank::UIModel::canPerformOnTarget(
            source, PokemonLocation::Party, SourceMutation::Edit));
        assert(!PokeBank::UIModel::canPerformOnTarget(
            source, PokemonLocation::SaveBox, SourceMutation::Edit));
        assert(PokeBank::UIModel::canPerformOnTarget(
            source, PokemonLocation::Bank, SourceMutation::Edit));

        assert(!PokeBank::UIModel::canPerformOnStoragePane(
            source, 0, SourceMutation::DirectMove));
        assert(PokeBank::UIModel::canPerformOnStoragePane(
            source, 1, SourceMutation::DirectMove));
        assert(PokeBank::UIModel::canPerformOnStoragePane(
            source, 1, SourceMutation::Rename));
    }

    // The action sheet consumes the target-aware capability, not one session-wide bool.
    for (auto location : {PokemonLocation::Party, PokemonLocation::SaveBox}) {
        PokemonActionSheet sheet;
        sheet.open({location, 0, 0});
        sheet.select(4);
        assert(sheet.activate(PokeBank::UIModel::canPerformOnTarget(
            SourceKind::RetroArchLegacy, location, SourceMutation::Edit))
            == ActionResult::NotYetSupported);
        assert(sheet.isOpen());
        sheet.select(0);
        assert(sheet.activate(false) == ActionResult::OpenView);
    }
    {
        PokemonActionSheet bankSheet;
        bankSheet.open({PokemonLocation::Bank, 0, 0});
        bankSheet.select(4);
        assert(bankSheet.activate(PokeBank::UIModel::canPerformOnTarget(
            SourceKind::RetroArchLegacy, PokemonLocation::Bank, SourceMutation::Edit))
            == ActionResult::OpenEditor);
    }

    // Low-level live writes remain locked.
    static_assert(!LIVE_SAVE_WRITES_ENABLED);
    assert(!canWriteTo(SaveDestination::LiveGame));

    // Cross-store source-retiring true Move remains explicitly unavailable when the
    // session source is read-only; target-aware Bank mutation does not weaken this gate.
    const std::string ui = read("src/UI/TrainerViewScreenBase.inc");
    const auto begin = ui.find("bool TrainerViewScreen::buildCrossStoreDescriptors(");
    const auto end = ui.find("bool TrainerViewScreen::captureCrossStoreMoveBaseline(", begin);
    assert(begin != std::string::npos && end != std::string::npos && end > begin);
    const std::string crossStore = ui.substr(begin, end - begin);
    assert(crossStore.find("if (sourceReadOnly())") != std::string::npos);
    assert(crossStore.find("true Move requires a PokeBank-owned mutable workspace") != std::string::npos);

    assert(dispatchAction(PokemonAction::Edit, true) == ActionResult::OpenEditor);
    std::cout << "Source mutation target policy: PASS\n";
}
