#ifndef POKEVAULT_SAFETY_SOURCE_MUTATION_POLICY_H
#define POKEVAULT_SAFETY_SOURCE_MUTATION_POLICY_H

#include <cstdint>

namespace PokeVault::Safety {
    enum class SourceKind : uint8_t {
        InstalledGame,
        BackupOrStaged,
        AppOwnedStorage,
    };

    enum class SourceMutation : uint8_t {
        View,
        Release,
        CreatePokemon,
        DirectMove,
        Edit,
        Rename,
        EditTrainer,
        EditItems,
        SaveChanges,
    };

    // Defense in depth for the current alpha. The low-level WritePolicy remains the final barrier;
    // this policy prevents installed-game browsing sessions from even entering inherited mutators.
    constexpr bool canPerform(SourceKind source, SourceMutation mutation) noexcept {
        if (mutation == SourceMutation::View) return true;
        return source == SourceKind::BackupOrStaged || source == SourceKind::AppOwnedStorage;
    }
}

#endif
