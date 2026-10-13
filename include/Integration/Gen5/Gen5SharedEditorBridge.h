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
    if(!Gen5StagedPokemonWorkspace::canonicalSlot(location))
        return {location,false,false};
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
    // The existing shared Add action is available only for a physically
    // zero-filled, checksum-validated EMPTY box cell. Party append, invalid
    // slots and already-staged replacements cannot fabricate Add authority.
    Shared::ActionSet result{};
    const auto append=[&](Shared::Action action) {
        result.values[result.count++]=action;
    };
    // Re-read the live workspace; cached picker flags never grant edit access.
    const auto live=selectedSlot(workspace,slot.location);
    if(live.validated && !live.occupied &&
       slot.location.region==Gen5StagedPokemonWorkspace::Region::Box &&
       workspace.emptyCloneDestination(slot.location.box,slot.location.slot))
        append(Shared::Action::Add);
    if(live.validated && live.occupied) {
        append(Shared::Action::View);
        // Native Create/Clone results use the SAME shared Edit surface.
        // Only app-owned PK5 fields can be staged, never source-save writes.
        append(Shared::Action::Edit);
        if(workspace.originalOccupied(slot.location) &&
           workspace.firstEmptyCloneDestination()<720)
            append(Shared::Action::Clone);
        if(slot.location.region==Gen5StagedPokemonWorkspace::Region::Box &&
           workspace.originalOccupied(slot.location))
            append(Shared::Action::Remove);
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
// accepts only these verified native field transactions. Do not forward unsupported
// UI fields to a generic Pokemon mutator or alter the source save.
[[nodiscard]] inline bool stageSharedField(
    Gen5SharedPokemonSession& session,Shared::FieldIdentity field,
    size_t stat,uint32_t value,std::string& error) {
    using NativeField=StagedPokemon5Record::Field;
    switch(field) {
        case Shared::FieldIdentity::Species:
            return session.stage(NativeField::Species,stat,value,error);
        case Shared::FieldIdentity::Nature:
            return session.stage(NativeField::Nature,stat,value,error);
        case Shared::FieldIdentity::Level:
            return session.stage(NativeField::Level,stat,value,error);
        case Shared::FieldIdentity::Experience:
            return session.stage(NativeField::Experience,stat,value,error);
        case Shared::FieldIdentity::Friendship:
            return session.stage(NativeField::Friendship,stat,value,error);
        case Shared::FieldIdentity::IV:
            return session.stage(NativeField::IV,stat,value,error);
        case Shared::FieldIdentity::EV:
            return session.stage(NativeField::EV,stat,value,error);
        case Shared::FieldIdentity::HeldItem:
            return session.stage(NativeField::HeldItem,stat,value,error);
        case Shared::FieldIdentity::Ball:
            return session.stage(NativeField::Ball,stat,value,error);
        case Shared::FieldIdentity::MetLevel:
            return session.stage(NativeField::MetLevel,stat,value,error);
        case Shared::FieldIdentity::Move:
            return session.stage(NativeField::Move,stat,value,error);
        case Shared::FieldIdentity::MovePP:
            return session.stage(NativeField::MovePP,stat,value,error);
        case Shared::FieldIdentity::MovePPUps:
            return session.stage(NativeField::MovePPUps,stat,value,error);
        case Shared::FieldIdentity::TrainerId:
            return session.stage(NativeField::TrainerId,stat,value,error);
        case Shared::FieldIdentity::SecretId:
            return session.stage(NativeField::SecretId,stat,value,error);
        case Shared::FieldIdentity::OriginalTrainerGender:
            return session.stage(NativeField::OriginalTrainerGender,stat,value,error);
        case Shared::FieldIdentity::Pokerus:
            return session.stage(NativeField::Pokerus,stat,value,error);
        case Shared::FieldIdentity::MetLocation:
            return session.stage(NativeField::MetLocation,stat,value,error);
        case Shared::FieldIdentity::FatefulEncounter:
            return session.stage(NativeField::FatefulEncounter,stat,value,error);
        case Shared::FieldIdentity::Form:
            return session.stage(NativeField::Form,stat,value,error);
        case Shared::FieldIdentity::Ability:
            return session.stage(NativeField::AbilitySlot,stat,value,error);
        case Shared::FieldIdentity::Gender:
            return session.stage(NativeField::Gender,stat,value,error);
        default:
            error="Gen V shared editor field is not verified for staged editing";
            return false;
    }
}
[[nodiscard]] inline bool stageSharedName(
    Gen5SharedPokemonSession& session,Shared::FieldIdentity field,
    std::string_view name,std::string& error) {
    if(field==Shared::FieldIdentity::Nickname)
        return session.stageName(StagedPokemon5Record::Field::NicknameText,
                                 name,error);
    if(field==Shared::FieldIdentity::OriginalTrainer)
        return session.stageName(StagedPokemon5Record::Field::OriginalTrainerText,
                                 name,error);
    error="Gen V text field is unsupported";
    return false;
}
[[nodiscard]] inline std::optional<
    PokeBank::UIModel::ExactFormatEditor::ExactFormatEditorDescriptor>
exactDescriptor(std::string_view gameId,bool hasValidatedWorkspace) {
    return Gen5EditorProvider::descriptorForSource(gameId,hasValidatedWorkspace);
}
} // namespace PokeVault::Integration::Gen5
#endif
