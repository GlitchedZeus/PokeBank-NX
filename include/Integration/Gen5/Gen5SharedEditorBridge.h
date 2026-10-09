#ifndef POKEBANK_GEN5_SHARED_EDITOR_BRIDGE_H
#define POKEBANK_GEN5_SHARED_EDITOR_BRIDGE_H

#include "Integration/Gen5/Gen5ExactFormatEditorProvider.h"
#include "Integration/Gen5/Gen5SharedPokemonSession.h"
#include "UI/SharedPokemonEditorContract.h"

#include <optional>
#include <string_view>

namespace PokeVault::Integration::Gen5 {

// Adapter for the ONE shared editor's action/field model. This is an
// app-owned PK5 draft contract, not a new editor renderer or SAV writer.
namespace Shared = PokeBank::UIModel::SharedPokemonEditor;
struct SharedEditorSlot {
    Gen5StagedPokemonWorkspace::Slot location;
    bool occupied = false;
    bool validated = false;
};
[[nodiscard]] inline SharedEditorSlot selectedSlot(
    const Gen5StagedPokemonWorkspace& workspace,
    Gen5StagedPokemonWorkspace::Slot location) {
    std::optional<Pokemon5ReadOnly> pk;
    if(location.region==Gen5StagedPokemonWorkspace::Region::Party && location.box==0)
        pk=workspace.viewParty(location.slot);
    else if(location.region==Gen5StagedPokemonWorkspace::Region::Box)
        pk=workspace.viewBox(location.box,location.slot);
    const bool valid=pk && pk->valid();
    return {location,valid && !pk->empty(),valid};
}

[[nodiscard]] inline Shared::ActionSet actions(
    const Gen5StagedPokemonWorkspace& workspace,
    const SharedEditorSlot& slot) {
    // Unknown/empty slots do not expose Add, Clone or Remove. The only
    // actionable PK5 editing path is an occupied, strictly validated slot.
    Shared::ActionSet result{};
    const auto append=[&](Shared::Action action) {
        result.values[result.count++]=action;
    };
    if(slot.validated && slot.occupied) {
        append(Shared::Action::View);
        append(Shared::Action::Edit);
    }
    if(workspace.hasChanges())append(Shared::Action::Review);
    append(Shared::Action::Close);
    return result;
}

[[nodiscard]] constexpr Shared::FieldAccess fieldAccess(
    Shared::FieldIdentity field) noexcept {
    return Shared::fieldAccessForGeneration(Shared::Generation::Gen5,field);
}
[[nodiscard]] inline bool openSharedDraft(
    Gen5SharedPokemonSession& session,
    const Gen5StagedPokemonWorkspace& workspace,
    const SharedEditorSlot& slot,
    Shared::Action action,std::string& error) {
    if(!slot.validated || !slot.occupied) {
        error="Gen V editor requires a valid, occupied native PK5 slot";
        return false;
    }
    if(action!=Shared::Action::View && action!=Shared::Action::Edit) {
        error="Gen V action is unsupported in the staged shared editor";
        return false;
    }
    const auto mode=action==Shared::Action::View?
        Gen5SharedPokemonSession::Mode::View:
        Gen5SharedPokemonSession::Mode::Edit;
    return session.begin(workspace,slot.location,mode,error);
}
[[nodiscard]] inline std::optional<
    PokeBank::UIModel::ExactFormatEditor::ExactFormatEditorDescriptor>
exactDescriptor(std::string_view gameId,bool hasValidatedWorkspace) {
    return Gen5EditorProvider::descriptorForSource(gameId,hasValidatedWorkspace);
}
} // namespace PokeVault::Integration::Gen5
#endif
