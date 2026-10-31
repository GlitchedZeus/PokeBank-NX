#ifndef POKEBANK_UI_MUTATION_TARGET_POLICY_H
#define POKEBANK_UI_MUTATION_TARGET_POLICY_H

#include "Safety/SourceMutationPolicy.h"
#include "UI/ActionSheetModel.h"

namespace PokeBank::UIModel {

constexpr PokeVault::Safety::SourceKind mutationSourceForTarget(
    PokeVault::Safety::SourceKind sessionSource,
    PokeVault::UIModel::PokemonLocation location) noexcept {
    return location == PokeVault::UIModel::PokemonLocation::Bank
        ? PokeVault::Safety::SourceKind::AppOwnedStorage
        : sessionSource;
}

constexpr bool canPerformOnTarget(
    PokeVault::Safety::SourceKind sessionSource,
    PokeVault::UIModel::PokemonLocation location,
    PokeVault::Safety::SourceMutation mutation) noexcept {
    return PokeVault::Safety::canPerform(
        mutationSourceForTarget(sessionSource, location), mutation);
}

constexpr PokeVault::Safety::SourceKind mutationSourceForStoragePane(
    PokeVault::Safety::SourceKind sessionSource, int pane) noexcept {
    return pane == 1
        ? PokeVault::Safety::SourceKind::AppOwnedStorage
        : sessionSource;
}

constexpr bool canPerformOnStoragePane(
    PokeVault::Safety::SourceKind sessionSource, int pane,
    PokeVault::Safety::SourceMutation mutation) noexcept {
    return PokeVault::Safety::canPerform(
        mutationSourceForStoragePane(sessionSource, pane), mutation);
}

} // namespace PokeBank::UIModel

#endif
