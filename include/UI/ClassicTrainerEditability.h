#pragma once
#include "Legacy/GSCReadOnlyTrainer.h"
#include "Legacy/Gen4ReadOnlyTrainer.h"
#include "Legacy/RBYReadOnlyTrainer.h"
#include "Trainer/Trainer.h"
#include "UI/ClassicTrainerSession.h"

namespace PokeBank::UIModel::ClassicTrainerSession {

// Whether the Trainer page's Name / Money can be edited. Gen I: the staged Trainer editor exists
// and exports with the staged Pokemon workspace. Gen II: the staged editor exists. Gen IV: the
// staged Gen IV workspace stages Name / Money (R2-BUG-002). Every other generation: the
// workspace itself is mutable.
inline bool trainerFieldsEditable(const Trainer::Trainer& trainer, bool mutableWorkspace) {
    // The NRO builds without RTTI: the game group identifies the class (only RBYReadOnlyTrainer
    // reports RBY and only GSCReadOnlyTrainer reports GSC), the same guard the GSC overlay uses.
    switch (trainer.getGameGroup()) {
        case Enums::GameVersion::RBY:
            return static_cast<const PokeVault::Legacy::RBYReadOnlyTrainer&>(trainer).stagedTrainerEditable();
        case Enums::GameVersion::GSC:
            return static_cast<const PokeVault::Legacy::GSCReadOnlyTrainer&>(trainer).stagedEditor() != nullptr;
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
