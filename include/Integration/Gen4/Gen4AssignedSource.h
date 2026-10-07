#ifndef POKEBANK_GEN4_ASSIGNED_SOURCE_H
#define POKEBANK_GEN4_ASSIGNED_SOURCE_H

#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Legacy/LegacySourceBindings.h"

namespace PokeVault::Integration::Gen4 {
// Backend only: uses the existing profile/game assignment database, no discovery UI.
enum class OpenStatus { Unassigned, Missing, Ambiguous, Unreadable, InvalidSave, AssignmentMismatch, Ready };
struct AssignedSourceReadOnly {
    OpenStatus status = OpenStatus::Unassigned;
    Legacy::AssignedFile source;
    std::optional<Gen4ReadOnlySave> save;
    std::string diagnostic;
};
[[nodiscard]] AssignedSourceReadOnly openAssignedSource(
    const Legacy::LegacySourceBindings& bindings, std::string_view profile, std::string_view game);
}
#endif
