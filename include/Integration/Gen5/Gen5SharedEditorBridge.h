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
    // Re-read the live workspace; cached picker flags never grant edit access.
    const auto live=selectedSlot(workspace,slot.location);
    if(live.validated && live.occupied) {
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
    // Cached slot metadata is presentation only. Revalidate against the
    // current workspace before beginning any View/Edit transaction.
    const auto live=selectedSlot(workspace,slot.location);
    if(!live.validated || !live.occupied) {
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
// The existing editor speaks FieldIdentity; the PK5 draft intentionally
// accepts only these four verified transactions. Do not forward unsupported
// UI fields to a generic Pokemon mutator or alter the source save.
[[nodiscard]] inline bool stageSharedField(
    Gen5SharedPokemonSession& session,Shared::FieldIdentity field,
    size_t stat,uint32_t value,std::string& error) {
    using NativeField=StagedPokemon5Record::Field;
    switch(field) {
        case Shared::FieldIdentity::Nature:
            return session.stage(NativeField::Nature,stat,value,error);
        case Shared::FieldIdentity::Friendship:
            return session.stage(NativeField::Friendship,stat,value,error);
        case Shared::FieldIdentity::IV:
            return session.stage(NativeField::IV,stat,value,error);
        case Shared::FieldIdentity::EV:
            return session.stage(NativeField::EV,stat,value,error);
        default:
            error="Gen V shared editor field is not verified for staged editing";
            return false;
    }
}
[[nodiscard]] inline std::optional<
    PokeBank::UIModel::ExactFormatEditor::ExactFormatEditorDescriptor>
exactDescriptor(std::string_view gameId,bool hasValidatedWorkspace) {
    return Gen5EditorProvider::descriptorForSource(gameId,hasValidatedWorkspace);
}
} // namespace PokeVault::Integration::Gen5
#endif
