#pragma once
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Legacy/FRLGReadOnlyTrainer.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Trainer/Trainer.h"
#include "UI/ClassicTrainerSession.h"

#include <string_view>

namespace PokeBank::UIModel::ClassicTrainerSession {

// Whether the Trainer page's Name / Money can be edited. Gen I: the staged Trainer editor exists
// and exports with the staged Pokemon workspace. Gen II: the staged editor exists. Gen III / IV:
// the staged Trainer editors (R2-BUG-002). Every other generation: the workspace itself is
// mutable. `sourceGameId` identifies an emulator Gen III source: two Trainer classes report the
// FRLG group, and only the emulator Gen III bridge (FRLGReadOnlyTrainer) is opened for the
// exact *_gba ids, the same guard ClassicInventoryOverlay / Gen3SharedPokemonSurface use.
inline bool isEmulatorGen3SourceId(std::string_view id) noexcept {
    return id == "ruby_gba" || id == "sapphire_gba" || id == "emerald_gba" || id == "firered_gba" ||
           id == "leafgreen_gba";
}

inline bool trainerFieldsEditable(const Trainer::Trainer& trainer, bool mutableWorkspace,
                                  std::string_view sourceGameId = {}) {
    // The NRO builds without RTTI: the game group identifies the class (only RBYReadOnlyTrainer
    // reports RBY and only GSCReadOnlyTrainer reports GSC), the same guard the GSC overlay uses.
    switch (trainer.getGameGroup()) {
        case Enums::GameVersion::RBY:
            return static_cast<const PokeVault::Legacy::RBYReadOnlyTrainer&>(trainer).stagedTrainerEditable();
        case Enums::GameVersion::GSC:
            return static_cast<const PokeVault::Legacy::GSCReadOnlyTrainer&>(trainer).stagedEditor() != nullptr;
        // Every Gen III game (Ruby / Sapphire / Emerald / FireRed / LeafGreen) reports FRLG.
        case Enums::GameVersion::FRLG:
            if (!isEmulatorGen3SourceId(sourceGameId)) return mutableWorkspace;
            return static_cast<const PokeVault::Legacy::FRLGReadOnlyTrainer&>(trainer).stagedTrainerAvailable();
        // Only Gen4ReadOnlyTrainer reports a Gen IV group (its parsed raw family).
        case Enums::GameVersion::DP:
        case Enums::GameVersion::PT:
        case Enums::GameVersion::HGSS:
            return static_cast<const PokeVault::Legacy::Gen4ReadOnlyTrainer&>(trainer).stagedPokemonAvailable();
        default:
            return mutableWorkspace;
    }
}

} // namespace PokeBank::UIModel::ClassicTrainerSession
