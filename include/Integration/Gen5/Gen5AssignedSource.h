#ifndef POKEBANK_GEN5_ASSIGNED_SOURCE_H
#define POKEBANK_GEN5_ASSIGNED_SOURCE_H

#include "Integration/Gen5/Gen5SourceDiscovery.h"
#include "Legacy/LegacySourceBindings.h"

#include <optional>
#include <string>
#include <string_view>

namespace PokeVault::Integration::Gen5 {

// Backend-only, opt-in bridge to the SAME persistent profile/game bindings
// used by Gen I-IV. The Games menu is not activated for Gen V by this module.
enum class AssignedOpenStatus : uint8_t {
    Unassigned, Missing, Ambiguous, Unreadable,
    InvalidSave, AssignmentMismatch, Ready
};
struct AssignedSourceReadOnly {
    AssignedOpenStatus status = AssignedOpenStatus::Unassigned;
    Legacy::AssignedFile source;
    Source::SaveInstance instance;
    std::optional<Gen5ReadOnlySave> save;
    std::string diagnostic;

    [[nodiscard]] bool ready() const noexcept {
        return status == AssignedOpenStatus::Ready &&
               save.has_value() && instance.ready() && instance.readOnly();
    }
};

// Explicit source paths only. There is no scanner fallback, source write,
// trainer/profile inference, or automatic ambiguous-copy selection.
[[nodiscard]] AssignedSourceReadOnly openAssignedSource(
    const Legacy::LegacySourceBindings& bindings,
    std::string_view profile, std::string_view exactGame,
    SaveCopySelection copy = SaveCopySelection::Automatic);

} // namespace PokeVault::Integration::Gen5
#endif
