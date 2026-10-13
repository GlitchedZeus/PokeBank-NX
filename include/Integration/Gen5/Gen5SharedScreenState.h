#ifndef POKEBANK_GEN5_SHARED_SCREEN_STATE_H
#define POKEBANK_GEN5_SHARED_SCREEN_STATE_H

#include "Integration/Gen5/Gen5SharedEditorBridge.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace PokeVault::Integration::Gen5 {

// Session-local control model for the ONE shared Trainer/Party/Boxes screen.
// It owns app-memory PK5 drafts and the existing shared Add action.
// No SAV5 writer, Bank/Inject or physical source-file mutation authority.
class Gen5SharedScreenState {
public:
    using Workspace=Gen5StagedPokemonWorkspace;
    using Slot=Workspace::Slot;
    using Action=Shared::Action;
    using Field=Shared::FieldIdentity;
    enum class Surface : uint8_t {
        Browse, Actions, View, Edit, MovePicker, LocationPicker, IdentityPicker, SpeciesPicker, NaturePicker, CloneDestination, ConfirmRemoveBox, ConfirmCreateBox, ConfirmDraft, Review,
        ConfirmDiscardAll, ConfirmExport, ConfirmExit
    };
    struct EditableRow { Field field; size_t stat; const char* label; uint32_t maximum; };
    static constexpr std::array<EditableRow,43> kEditableRows{{
        {Field::Nature,0,"Nature",24}, {Field::Friendship,0,"Friendship",255},
        {Field::HeldItem,0,"Held Item ID",638},
        {Field::Ball,0,"Ball ID",25},
        {Field::MetLevel,0,"Met Level",100},
        {Field::TrainerId,0,"Trainer ID",65535},
        {Field::SecretId,0,"Secret ID",65535},
        {Field::OriginalTrainerGender,0,"OT Gender",1},
        {Field::Pokerus,0,"Pokerus",2},
        {Field::MetLocation,0,"Met Location",65535},
        {Field::FatefulEncounter,0,"Fateful",1},
        {Field::Move,0,"Move 1",MaxNativeMove5},
        {Field::Move,1,"Move 2",MaxNativeMove5},
        {Field::Move,2,"Move 3",MaxNativeMove5},
        {Field::Move,3,"Move 4",MaxNativeMove5},
        {Field::MovePPUps,0,"PP Ups 1",3},
        {Field::MovePPUps,1,"PP Ups 2",3},
        {Field::MovePPUps,2,"PP Ups 3",3},
        {Field::MovePPUps,3,"PP Ups 4",3},
        {Field::MovePP,0,"PP 1",64},
        {Field::MovePP,1,"PP 2",64},
        {Field::MovePP,2,"PP 3",64},
        {Field::MovePP,3,"PP 4",64},
        {Field::IV,0,"IV HP",31}, {Field::IV,1,"IV Atk",31},
        {Field::IV,2,"IV Def",31}, {Field::IV,3,"IV Spe",31},
        {Field::IV,4,"IV SpA",31}, {Field::IV,5,"IV SpD",31},
        {Field::EV,0,"EV HP",255}, {Field::EV,1,"EV Atk",255},
        {Field::EV,2,"EV Def",255}, {Field::EV,3,"EV Spe",255},
        {Field::EV,4,"EV SpA",255}, {Field::EV,5,"EV SpD",255},
        {Field::Ability,0,"Ability Slot",2},
        {Field::Form,0,"Form",31},
        {Field::Gender,0,"Gender",2},
        {Field::Level,0,"Level",100},
        {Field::Experience,0,"Experience",1640000},
        {Field::Nickname,0,"Nickname",10},
        {Field::OriginalTrainer,0,"OT Name",7},
        {Field::Species,0,"Species Dex ID",649}
    }};

    [[nodiscard]] Surface surface() const noexcept { return surface_; }
    [[nodiscard]] const Slot& target() const noexcept { return target_; }
    [[nodiscard]] int actionRow() const noexcept { return actionRow_; }
    [[nodiscard]] int fieldRow() const noexcept { return fieldRow_; }
    [[nodiscard]] uint16_t movePickerSelection() const noexcept { return movePickerSelection_; }
    // PK5's 25 nature IDs follow the Gen III+ 5x5 battle-stat grid.
    static constexpr std::array<std::string_view,25> kNatureNames{{
        "Hardy","Lonely","Brave","Adamant","Naughty",
        "Bold","Docile","Relaxed","Impish","Lax",
        "Timid","Hasty","Serious","Jolly","Naive",
        "Modest","Mild","Quiet","Bashful","Rash",
        "Calm","Gentle","Sassy","Careful","Quirky"
    }};
    [[nodiscard]] uint8_t natureSelection() const noexcept { return natureSelection_; }
    [[nodiscard]] static constexpr std::string_view natureName(uint32_t id) noexcept {
        return id<kNatureNames.size()?kNatureNames[id]:std::string_view{};
    }
    [[nodiscard]] size_t locationPickerIndex() const noexcept { return locationPickerIndex_; }
    [[nodiscard]] uint32_t identitySelection() const noexcept { return identitySelection_; }
    [[nodiscard]] size_t speciesChoiceCount() const noexcept { return speciesCount_; }
    [[nodiscard]] size_t speciesChoiceIndex() const noexcept { return speciesIndex_; }
    [[nodiscard]] std::string_view speciesSearch() const noexcept { return speciesSearch_; }
    [[nodiscard]] uint16_t speciesChoiceId(size_t index) const noexcept {
        return index<speciesCount_?speciesResults_[index]:0;
    }
    [[nodiscard]] uint16_t chosenSpeciesId() const noexcept {
        return speciesChoiceId(speciesIndex_);
    }
    [[nodiscard]] bool speciesChoiceAllowed(uint32_t species) const noexcept {
        const auto current=draft_.current();
        if(!current || !current->valid() || current->empty() ||
           current->form()!=0 || species<1 || species>649)return false;
        if(species==current->species())return true;
        if(!draft_.originalBaseFormForSpecies())return false;
        // Do not carry an unnicknamed old species name into a new
        // identity: require a user-confirmed nickname edit first.
        if((Crypto::read32(current->decodedBytes(),0x38)&0x80000000u)==0)
            return false;
        const auto source=personal5(current->species(),0,black2White2_);
        const auto target=personal5(species,0,black2White2_);
        if(!source || !target || source.info->growthRate>5 ||
           source.info->growthRate!=target.info->growthRate ||
           !genderAllowed5(target.info->genderRatio,current->gender()))
            return false;
        const uint8_t slot=current->hiddenAbility()?2:
            static_cast<uint8_t>((current->pid()>>16)&1);
        return abilityForSlot5(*target.info,slot)!=0;
    }
    [[nodiscard]] bool black2White2() const noexcept { return black2White2_; }
    [[nodiscard]] size_t reviewPage() const noexcept { return reviewPage_; }
    [[nodiscard]] size_t cloneDestinationIndex() const noexcept { return cloneDestinationIndex_; }
    // The shared Add wizard retains accepted input only in this Trainer
    // session. Rejecting a native field or presentation need not make the
    // player enter all previous PK5 values again.
    [[nodiscard]] const std::optional<CreateRequest5>& pendingCreateInput()
        const noexcept { return pendingCreateInput_; }
    [[nodiscard]] bool rememberCreateInput(const Workspace& workspace,
                                            const CreateRequest5& request) {
        if(surface_!=Surface::ConfirmCreateBox ||
           target_.region!=Workspace::Region::Box ||
           !workspace.emptyCloneDestination(target_.box,target_.slot) ||
           static_cast<uint32_t>(request.game)!=workspace.sourceSave().trainer().gameVersion ||
           request.format!=CreateFormat5::Boxed)return false;
        pendingCreateInput_=request;
        return true;
    }
    [[nodiscard]] bool cloneDestinationAllowed(const Workspace& workspace) const {
        return cloneDestinationIndex_<720 &&
            workspace.emptyCloneDestination(cloneDestinationIndex_/30,
                                            cloneDestinationIndex_%30);
    }
    static constexpr size_t kReviewPageSize = 4;
    [[nodiscard]] const Gen5SharedPokemonSession& draft() const noexcept { return draft_; }
    [[nodiscard]] bool isEditing() const noexcept { return surface_==Surface::Edit; }

    [[nodiscard]] bool openActions(const Workspace& workspace,Slot slot,
                                    std::string& error) {
        if(surface_!=Surface::Browse || !Workspace::canonicalSlot(slot)) {
            error="Gen V action selection requires an idle, canonical PK5 slot";
            return false;
        }
        target_=slot;
        actionRow_=0;
        surface_=Surface::Actions;
        error.clear();
        return true;
    }
    [[nodiscard]] Shared::ActionSet availableActions(const Workspace& workspace) const {
        return actions(workspace,selectedSlot(workspace,target_));
    }
    void moveAction(int delta,const Workspace& workspace) noexcept {
        if(surface_!=Surface::Actions)return;
        const auto available=availableActions(workspace);
        if(available.count==0)return;
        const int count=static_cast<int>(available.count);
        actionRow_=(actionRow_+delta%count+count)%count;
    }
    [[nodiscard]] bool activate(const Workspace& workspace,Action action,
                                 std::string& error) {
        if(surface_!=Surface::Actions) {
            error="Gen V actions are not currently open";
            return false;
        }
        const auto allowed=availableActions(workspace);
        bool listed=false;
        for(size_t i=0;i<allowed.count;++i)if(allowed[i]==action)listed=true;
        if(!listed) {
            error="Unsupported Generation V action for this selected slot";
            return false;
        }
        if(action==Action::Close) {surface_=Surface::Browse;error.clear();return true;}
        if(action==Action::Review) {
            reviewPage_=0;
            surface_=Surface::Review;
            error.clear();
            return true;
        }
        if(action==Action::Clone) {
            cloneDestinationIndex_=workspace.firstEmptyCloneDestination();
            if(cloneDestinationIndex_>=720) {
                error="Gen V has no verified empty box destination";
                return false;
            }
            surface_=Surface::CloneDestination;
            error.clear();
            return true;
        }
        if(action==Action::Add) {
            if(target_.region!=Workspace::Region::Box ||
               !workspace.emptyCloneDestination(target_.box,target_.slot)) {
                error="Gen V Add requires an unchanged empty native box slot";
                return false;
            }
            pendingCreateInput_.reset();
            surface_=Surface::ConfirmCreateBox;
            error.clear();
            return true;
        }
        if(action==Action::Remove) {
            if(target_.region!=Workspace::Region::Box ||
               !workspace.originalOccupied(target_)) {
                error="Gen V removal requires an occupied original box slot";
                return false;
            }
            surface_=Surface::ConfirmRemoveBox;
            error.clear();
            return true;
        }
        if(action==Action::View || action==Action::Edit) {
            if(!openSharedDraft(draft_,workspace,selectedSlot(workspace,target_),action,error))
                return false;
            surface_=action==Action::View?Surface::View:Surface::Edit;
            fieldRow_=0;
            black2White2_=workspace.black2White2();
            return true;
        }
        error="Unsupported shared Gen V operation; original source remains unchanged";
        return false;
    }
    [[nodiscard]] bool activateSelected(const Workspace& workspace,std::string& error) {
        const auto allowed=availableActions(workspace);
        if(surface_!=Surface::Actions || actionRow_<0 ||
           static_cast<size_t>(actionRow_)>=allowed.count) {
            error="Generation V shared action selection is unavailable";
            return false;
        }
        return activate(workspace,allowed[static_cast<size_t>(actionRow_)],error);
    }

    void moveCloneCursor(int delta) noexcept {
        if(surface_!=Surface::CloneDestination)return;
        cloneDestinationIndex_=static_cast<size_t>(std::clamp(
            static_cast<int>(cloneDestinationIndex_)+delta,0,719));
    }
    template<class Refresh>
    [[nodiscard]] bool confirmClone(Workspace& workspace,Refresh&& refresh,
                                    std::string& error) {
        if(surface_!=Surface::CloneDestination ||
           !cloneDestinationAllowed(workspace)) {
            error="Gen V clone destination is not an empty verified box slot";
            return false;
        }
        Workspace snapshot=workspace;
        if(!workspace.cloneToEmptyBox(target_,cloneDestinationIndex_/30,
                                      cloneDestinationIndex_%30,&error))return false;
        if(!std::forward<Refresh>(refresh)(error)) {
            workspace=std::move(snapshot);
            if(error.empty())error="Gen V clone presentation rejected; stage rolled back";
            return false;
        }
        surface_=Surface::Browse;
        error.clear();
        return true;
    }
    template<class Refresh>
    [[nodiscard]] bool confirmRemoveBox(Workspace& workspace,Refresh&& refresh,
                                        std::string& error) {
        if(surface_!=Surface::ConfirmRemoveBox ||
           target_.region!=Workspace::Region::Box) {
            error="Gen V removal requires explicit Y confirmation";
            return false;
        }
        Workspace snapshot=workspace;
        if(!workspace.removeBox(target_.box,target_.slot,&error))return false;
        if(!std::forward<Refresh>(refresh)(error)) {
            workspace=std::move(snapshot);
            if(error.empty())error="Gen V remove presentation rejected; stage rolled back";
            return false;
        }
        surface_=Surface::Browse;
        error.clear();
        return true;
    }
    // Undo only the selected app-owned Clone / Remove / Create operation.
    // This uses the existing shared Actions screen (Y), not a new editor or
    // generic source mutation path. Failed presentation rolls back atomically.
    [[nodiscard]] bool selectedBoxUndoAvailable(const Workspace& ws) const noexcept {
        return surface_==Surface::Actions &&
            target_.region==Workspace::Region::Box &&
            Workspace::canonicalSlot(target_) &&
            ws.isBoxReplacement(target_.box,target_.slot);
    }
    template<class Refresh>
    [[nodiscard]] bool undoSelectedBoxReplacement(Workspace& ws,
                                                   Refresh&& refresh,
                                                   std::string& error) {
        if(!selectedBoxUndoAvailable(ws)) {
            error="Gen V Undo requires one selected staged box operation";
            return false;
        }
        Workspace snapshot=ws;
        if(!ws.undoBoxReplacement(target_.box,target_.slot)) {
            error="Gen V box operation was already cleared";
            return false;
        }
        if(!std::forward<Refresh>(refresh)(error)) {
            ws=std::move(snapshot);
            if(error.empty())error="Gen V Undo presentation failed; stage restored";
            return false;
        }
        surface_=Surface::Browse;
        error.clear();
        return true;
    }
    // No native game or encounter fields are guessed by the state machine;
    // the shared Add wizard passes a fully explicit request to this atomic
    // stage+presentation boundary. User may cancel without a source mutation.
    template<class Refresh>
    [[nodiscard]] bool confirmCreateBox(Workspace& ws,
                                         const CreateRequest5& request,
                                         Refresh&& refresh,
                                         std::string& error) {
        if(surface_!=Surface::ConfirmCreateBox ||
           target_.region!=Workspace::Region::Box ||
           !ws.emptyCloneDestination(target_.box,target_.slot)) {
            error="Gen V Add confirmation requires the original empty box slot";
            return false;
        }
        Workspace snapshot=ws;
        if(!ws.createInEmptyBox(request,target_.box,target_.slot,&error))
            return false;
        if(!std::forward<Refresh>(refresh)(error)) {
            ws=std::move(snapshot);
            if(error.empty())error="Gen V Add presentation failed; stage rolled back";
            return false;
        }
        pendingCreateInput_.reset();
        surface_=Surface::Browse;
        error.clear();
        return true;
    }
    void cancelBoxAction() noexcept {
        if(surface_==Surface::ConfirmCreateBox)pendingCreateInput_.reset();
        if(surface_==Surface::CloneDestination ||
           surface_==Surface::ConfirmRemoveBox ||
           surface_==Surface::ConfirmCreateBox)surface_=Surface::Actions;
    }
    void moveReviewPage(int delta,const Workspace& workspace) noexcept {
        if(surface_!=Surface::Review)return;
        const size_t records=workspace.changedRecordCount();
        const size_t pages=std::max(size_t{1},
            (records+kReviewPageSize-1)/kReviewPageSize);
        if(delta>0 && reviewPage_+1<pages)++reviewPage_;
        else if(delta<0 && reviewPage_>0)--reviewPage_;
    }

    void moveField(int delta) noexcept {
        if(surface_!=Surface::Edit)return;
        constexpr int count=static_cast<int>(kEditableRows.size());
        fieldRow_=(fieldRow_+delta%count+count)%count;
    }
    [[nodiscard]] bool openNaturePicker(std::string& error) {
        if(surface_!=Surface::Edit || !draft_.editable() ||
           kEditableRows[static_cast<size_t>(fieldRow_)].field!=Field::Nature) {
            error="Gen V nature picker requires an Edit draft's Nature field";
            return false;
        }
        const auto current=draft_.current();
        if(!current || !current->valid() || current->nature()>24) {
            error="Gen V nature picker requires a validated PK5 nature";
            return false;
        }
        natureSelection_=current->nature();
        surface_=Surface::NaturePicker;
        error.clear();
        return true;
    }
    void moveNatureCursor(int delta) noexcept {
        if(surface_!=Surface::NaturePicker)return;
        natureSelection_=static_cast<uint8_t>(std::clamp(
            static_cast<int>(natureSelection_)+delta,0,24));
    }
    [[nodiscard]] bool acceptNaturePicker(std::string& error) {
        if(surface_!=Surface::NaturePicker || !draft_.editable() ||
           !stageSharedField(draft_,Field::Nature,0,natureSelection_,error))
            return false;
        surface_=Surface::Edit;
        error.clear();
        return true;
    }
    void cancelNaturePicker() noexcept {
        if(surface_==Surface::NaturePicker)surface_=Surface::Edit;
    }
    // The Move Picker is local to the existing shared PK5 draft. It never
    // stages a selection until A confirms the target. B is lossless and
    // returns to the previously selected Move row.
    [[nodiscard]] bool openMovePicker(std::string& error) {
        if(surface_!=Surface::Edit || !draft_.editable() ||
           fieldRow_<0 || static_cast<size_t>(fieldRow_)>=kEditableRows.size() ||
           kEditableRows[static_cast<size_t>(fieldRow_)].field!=Field::Move) {
            error="Gen V move picker requires a selected Move row in Edit";
            return false;
        }
        const auto current=draft_.current();
        if(!current || !current->valid()) {
            error="Gen V move picker requires a valid PK5 draft";
            return false;
        }
        const auto id=current->moves()[kEditableRows[static_cast<size_t>(fieldRow_)].stat];
        movePickerSelection_=validNativeMove5(id)?id:0;
        surface_=Surface::MovePicker;
        error.clear();
        return true;
    }
    void movePickerCursor(int delta) noexcept {
        if(surface_!=Surface::MovePicker)return;
        const int next=std::clamp(static_cast<int>(movePickerSelection_)+delta,
                                  0,static_cast<int>(MaxNativeMove5));
        movePickerSelection_=static_cast<uint16_t>(next);
    }
    [[nodiscard]] bool acceptMovePicker(std::string& error) {
        if(surface_!=Surface::MovePicker || !draft_.editable()) {
            error="Gen V picker has no active Edit draft";
            return false;
        }
        const auto& row=kEditableRows[static_cast<size_t>(fieldRow_)];
        if(row.field!=Field::Move ||
           !stageSharedField(draft_,Field::Move,row.stat,movePickerSelection_,error))
            return false;
        surface_=Surface::Edit;
        error.clear();
        return true;
    }
    void cancelMovePicker() noexcept {
        if(surface_==Surface::MovePicker)surface_=Surface::Edit;
    }
    // Native source-backed encounter-location chooser. Its selection is
    // inert until explicit A confirmation; unknown original IDs remain
    // untouched on cancellation and in read-only views.
    [[nodiscard]] bool openLocationPicker(std::string& error) {
        if(surface_!=Surface::Edit || !draft_.editable() ||
           kEditableRows[static_cast<size_t>(fieldRow_)].field!=Field::MetLocation) {
            error="Gen V met-location picker requires its Edit field";
            return false;
        }
        const auto current=draft_.current();
        if(!current || !current->valid()) {
            error="Gen V location picker has no validated PK5 draft";
            return false;
        }
        const auto index=locationIndex5(current->metLocation());
        locationPickerIndex_=index==NativeMetLocations5.size()?0:index;
        surface_=Surface::LocationPicker;
        error.clear();
        return true;
    }
    void moveLocationCursor(int delta) noexcept {
        if(surface_!=Surface::LocationPicker)return;
        const int next=std::clamp(static_cast<int>(locationPickerIndex_)+delta,
                                  0,static_cast<int>(NativeMetLocations5.size())-1);
        locationPickerIndex_=static_cast<size_t>(next);
    }
    [[nodiscard]] bool acceptLocationPicker(std::string& error) {
        if(surface_!=Surface::LocationPicker || !draft_.editable()) {
            error="Gen V location picker has no active draft";
            return false;
        }
        if(!stageSharedField(draft_,Field::MetLocation,0,
                             NativeMetLocations5[locationPickerIndex_],error))
            return false;
        surface_=Surface::Edit;
        error.clear();
        return true;
    }
    void cancelLocationPicker() noexcept {
        if(surface_==Surface::LocationPicker)surface_=Surface::Edit;
    }
    // Species chooser is a navigation-only modal until explicit A. The
    // existing native same-growth/identity transaction is the authority.
    // Do not conflate a compatible byte-level edit with encounter legality.
    [[nodiscard]] bool openSpeciesPicker(std::string& error) {
        if(surface_!=Surface::Edit || !draft_.editable() ||
           kEditableRows[static_cast<size_t>(fieldRow_)].field!=Field::Species) {
            error="Gen V species picker requires the Species field in Edit";
            return false;
        }
        const auto current=draft_.current();
        if(!current || !current->valid() || current->empty()) {
            error="Gen V species picker requires a valid occupied PK5";
            return false;
        }
        speciesSearch_.clear();
        speciesCount_=649;
        for(size_t i=0;i<speciesCount_;++i)
            speciesResults_[i]=static_cast<uint16_t>(i+1);
        speciesIndex_=current->species()-1;
        surface_=Surface::SpeciesPicker;
        error.clear();
        return true;
    }
    // Search uses a caller-provided species name lookup, so the native
    // state model stays independent of UI string tables and linking.
    template<class NameLookup>
    void filterSpecies(std::string_view query,NameLookup&& lookup) {
        if(surface_!=Surface::SpeciesPicker)return;
        if(query.size()>32)return; // bounded search input
        const uint16_t previous=chosenSpeciesId();
        speciesSearch_=std::string(query);
        speciesCount_=0;
        speciesIndex_=0;
        for(uint32_t id=1;id<=649;++id) {
            const char* const name=lookup(static_cast<uint16_t>(id));
            if(!name || !containsAsciiCaseInsensitive(name,query))continue;
            speciesResults_[speciesCount_++]=static_cast<uint16_t>(id);
        }
        for(size_t i=0;i<speciesCount_;++i)
            if(speciesResults_[i]==previous) {
                speciesIndex_=i;
                break;
            }
    }
    void moveSpeciesCursor(int delta) noexcept {
        if(surface_!=Surface::SpeciesPicker || speciesCount_==0)return;
        speciesIndex_=static_cast<size_t>(std::clamp(
            static_cast<int>(speciesIndex_)+delta,0,
            static_cast<int>(speciesCount_-1)));
    }
    [[nodiscard]] bool acceptSpeciesPicker(std::string& error) {
        const uint16_t species=chosenSpeciesId();
        if(surface_!=Surface::SpeciesPicker || !draft_.editable() ||
           !speciesChoiceAllowed(species)) {
            error="Gen V species cannot preserve this PK5 identity and growth group";
            return false;
        }
        if(!stageSharedField(draft_,Field::Species,0,species,error))return false;
        speciesSearch_.clear();
        surface_=Surface::Edit;
        error.clear();
        return true;
    }
    void cancelSpeciesPicker() noexcept {
        if(surface_==Surface::SpeciesPicker) {
            speciesSearch_.clear();
            surface_=Surface::Edit;
        }
    }
    [[nodiscard]] bool openIdentityPicker(std::string& error) {
        if(surface_!=Surface::Edit || !draft_.editable() ||
           fieldRow_<0 || static_cast<size_t>(fieldRow_)>=kEditableRows.size()) {
            error="Gen V identity picker requires an Edit draft";
            return false;
        }
        const auto& row=kEditableRows[static_cast<size_t>(fieldRow_)];
        if(row.field!=Field::Form && row.field!=Field::Ability &&
           row.field!=Field::Gender) {
            error="Gen V picker requires Form, Ability or Gender";
            return false;
        }
        const auto current=draft_.current();
        if(!current || !current->valid() ||
           !personal5(current->species(),current->form(),black2White2_)) {
            error="Gen V has no verified native identity for this Pokemon";
            return false;
        }
        identitySelection_=fieldValue(*current,row,black2White2_);
        surface_=Surface::IdentityPicker;
        error.clear();
        return true;
    }
    [[nodiscard]] size_t identityChoiceCount() const noexcept {
        const auto current=draft_.current();
        if(!current || fieldRow_<0 || static_cast<size_t>(fieldRow_)>=kEditableRows.size())return 0;
        const auto info=personal5(current->species(),current->form(),black2White2_);
        if(!info)return 0;
        const auto field=kEditableRows[static_cast<size_t>(fieldRow_)].field;
        return field==Field::Form?info.forms:
               field==Field::Ability?3:field==Field::Gender?3:0;
    }
    [[nodiscard]] bool identityChoiceAllowed(uint32_t value) const noexcept {
        const auto current=draft_.current();
        if(!current)return false;
        const auto info=personal5(current->species(),current->form(),black2White2_);
        if(!info)return false;
        const auto field=kEditableRows[static_cast<size_t>(fieldRow_)].field;
        if(field==Field::Form)
            return value<info.forms &&
                   static_cast<bool>(personal5(current->species(),value,black2White2_));
        if(field==Field::Ability)
            return value<3 && abilityForSlot5(*info.info,value)!=0;
        if(field==Field::Gender)
            return genderAllowed5(info.info->genderRatio,value);
        return false;
    }
    void moveIdentityCursor(int delta) noexcept {
        if(surface_!=Surface::IdentityPicker)return;
        const auto count=identityChoiceCount();
        if(!count)return;
        const int value=std::clamp(static_cast<int>(identitySelection_)+delta,
                                   0,static_cast<int>(count)-1);
        identitySelection_=static_cast<uint32_t>(value);
    }
    [[nodiscard]] bool acceptIdentityPicker(std::string& error) {
        if(surface_!=Surface::IdentityPicker || !draft_.editable() ||
           !identityChoiceAllowed(identitySelection_)) {
            error="Gen V identity choice is unavailable for this species/form";
            return false;
        }
        const auto& row=kEditableRows[static_cast<size_t>(fieldRow_)];
        if(!stageSharedField(draft_,row.field,row.stat,identitySelection_,error))
            return false;
        surface_=Surface::Edit;
        error.clear();
        return true;
    }
    void cancelIdentityPicker() noexcept {
        if(surface_==Surface::IdentityPicker)surface_=Surface::Edit;
    }
    [[nodiscard]] bool stageNameValue(std::string_view name,std::string& error) {
        if(surface_!=Surface::Edit || !draft_.editable()) {
            error="Gen V has no editable native text draft";
            return false;
        }
        const auto& row=kEditableRows[static_cast<size_t>(fieldRow_)];
        return stageSharedName(draft_,row.field,name,error);
    }
    [[nodiscard]] bool stageFieldValue(uint32_t value,std::string& error) {
        if(surface_!=Surface::Edit || !draft_.editable()) {
            error="Gen V has no editable draft";
            return false;
        }
        const auto& row=kEditableRows[static_cast<size_t>(fieldRow_)];
        if(row.field==Field::Move || row.field==Field::MetLocation ||
           row.field==Field::Ability || row.field==Field::Form || row.field==Field::Gender ||
           row.field==Field::Nickname || row.field==Field::OriginalTrainer ||
           value>row.maximum) {
            error="Use the verified native picker or remain within this field's range";
            return false;
        }
        return stageSharedField(draft_,row.field,row.stat,value,error);
    }
    [[nodiscard]] static uint32_t fieldValue(const Pokemon5ReadOnly& pk,
                                              const EditableRow& row,
                                              bool black2White2=false) noexcept {
        switch(row.field) {
            case Field::Species:return pk.species();
            case Field::Level: {
                const auto info=personal5(pk.species(),pk.form(),black2White2);
                                return pk.partyRecord()?pk.partyLevel():
                       info?levelForExperience5(info.info->growthRate,
                                                pk.experience()):0;
            }
            case Field::Experience:return pk.experience();
            case Field::Nature:return pk.nature();
            case Field::Friendship:return pk.friendship();
            case Field::HeldItem:return pk.heldItem();
            case Field::Ball:return pk.ball();
            case Field::MetLevel:return pk.metLevel();
            case Field::TrainerId:return pk.tid();
            case Field::SecretId:return pk.sid();
            case Field::OriginalTrainerGender:return pk.originalTrainerGender();
            case Field::Pokerus:return pk.pokerusMode();
            case Field::MetLocation:return pk.metLocation();
            case Field::FatefulEncounter:return pk.fatefulEncounter()?1:0;
            case Field::Form:return pk.form();
            case Field::Gender:return pk.gender();
            case Field::Ability:return pk.hiddenAbility()?2:((pk.pid()>>16)&1);
            case Field::Move:return pk.moves()[row.stat];
            case Field::MovePPUps:return pk.ppUps()[row.stat];
            case Field::MovePP:return pk.pp()[row.stat];
            case Field::IV:return pk.ivs()[row.stat];
            case Field::EV:return pk.evs()[row.stat];
            default:return 0;
        }
    }
    [[nodiscard]] bool adjustField(int delta,std::string& error) {
        if(surface_!=Surface::Edit) {
            error="Generation V is not in a verified Edit draft";
            return false;
        }
        const auto current=draft_.current();
        if(!current || !current->valid()) {
            error="Generation V PK5 edit draft failed strict validation";
            return false;
        }
        const auto& row=kEditableRows[static_cast<size_t>(fieldRow_)];
        if(row.field==Field::Move || row.field==Field::MetLocation ||
           row.field==Field::Ability || row.field==Field::Form || row.field==Field::Gender ||
           row.field==Field::Nickname || row.field==Field::OriginalTrainer ||
           row.field==Field::Species) {
            error="Press A to choose a verified Generation V picker value";
            return false;
        }
        const int64_t value=static_cast<int64_t>(fieldValue(*current,row,black2White2_))+delta;
        // No modulo wrapping; failed EV totals and range errors preserve
        // the full draft instead of introducing invalid intermediate states.
        if(value<0 || value>row.maximum) {
            error="Generation V value is outside the native field range";
            return false;
        }
        return stageFieldValue(static_cast<uint32_t>(value),error);
    }
    [[nodiscard]] bool keep(Workspace& workspace,std::string& error) {
        // A on the dirty-draft decision is a real Keep, not a dead action.
        // Both states own the SAME active edit draft; View cannot Keep.
        if((surface_!=Surface::Edit && surface_!=Surface::ConfirmDraft) ||
           !draft_.editable()) {
            error="Generation V Keep requires an active Edit draft";
            return false;
        }
        if(!draft_.keep(workspace,error))return false;
        surface_=Surface::Browse;
        return true;
    }

    // An app-memory Keep is not complete until the existing Trainer renderer
    // has accepted the new strictly validated PK5 presentation. The ordinary
    // session Keep intentionally closes its draft, so snapshot BOTH the
    // workspace and draft first. Failed presentation cannot lose the draft,
    // alter a previous staged record, or leave a half-updated screen.
    template <typename Refresh>
    [[nodiscard]] bool keepWithPresentation(Workspace& workspace,
                                            Refresh&& refresh,std::string& error) {
        if((surface_!=Surface::Edit && surface_!=Surface::ConfirmDraft) ||
           !draft_.editable()) {
            error="Generation V presentation Keep requires an active Edit draft";
            return false;
        }
        Workspace baselineWorkspace=workspace;
        Gen5SharedPokemonSession baselineDraft=draft_;
        const Surface baselineSurface=surface_;
        if(!keep(workspace,error))return false;
        if(std::forward<Refresh>(refresh)(error)) {
            error.clear();
            return true;
        }
        workspace=std::move(baselineWorkspace);
        draft_=std::move(baselineDraft);
        surface_=baselineSurface;
        if(error.empty())error="Generation V staged presentation rejected; draft preserved";
        return false;
    }
    // B never loses a dirty local draft. User must explicitly Keep or Discard.
    [[nodiscard]] bool back() noexcept {
        if(surface_==Surface::MovePicker) {
            cancelMovePicker();
            return false;
        }
        if(surface_==Surface::LocationPicker) {
            cancelLocationPicker();
            return false;
        }
        if(surface_==Surface::IdentityPicker) {
            cancelIdentityPicker();
            return false;
        }
        if(surface_==Surface::SpeciesPicker) {
            cancelSpeciesPicker();
            return false;
        }
        if(surface_==Surface::NaturePicker) {
            cancelNaturePicker();
            return false;
        }
        if(surface_==Surface::CloneDestination ||
           surface_==Surface::ConfirmRemoveBox ||
           surface_==Surface::ConfirmCreateBox) {
            cancelBoxAction();
            return false;
        }
        if(surface_==Surface::View || surface_==Surface::Edit) {
            if(draft_.back()) {surface_=Surface::Browse;return true;}
            surface_=Surface::ConfirmDraft;
            return false;
        }
        if(surface_==Surface::Actions || surface_==Surface::Review)
            surface_=Surface::Browse;
        else if(surface_==Surface::ConfirmDraft)continueEditing();
        else if(surface_==Surface::ConfirmDiscardAll)surface_=Surface::Review;
        else if(surface_==Surface::ConfirmExport)surface_=Surface::Review;
        else if(surface_==Surface::ConfirmExit)surface_=Surface::Browse;
        return false;
    }
    void continueEditing() noexcept {
        if(surface_==Surface::ConfirmDraft) {
            draft_.continueEditing();
            surface_=Surface::Edit;
        }
    }
    void discardDraft() noexcept {
        if(surface_==Surface::Edit || surface_==Surface::ConfirmDraft ||
           surface_==Surface::View) {
            draft_.discardDraft();
            surface_=Surface::Browse;
        }
    }
    [[nodiscard]] bool requestExit(const Workspace& workspace) noexcept {
        if(surface_!=Surface::Browse)return false;
        if(workspace.hasChanges()) {surface_=Surface::ConfirmExit;return false;}
        return true;
    }
    // The export confirmation is separate from staging and leaves all
    // reviewed bytes intact. A cannot publish, and failed I/O retains
    // the confirmation until the user cancels or retries with Y.
    [[nodiscard]] bool requestExport(const Workspace& workspace) noexcept {
        if(surface_!=Surface::Review || !workspace.hasChanges())return false;
        surface_=Surface::ConfirmExport;
        return true;
    }
    void finishExport() noexcept {
        if(surface_==Surface::ConfirmExport)surface_=Surface::Review;
    }
    void requestDiscardAll() noexcept {
        if(surface_==Surface::Review)surface_=Surface::ConfirmDiscardAll;
    }
    [[nodiscard]] bool confirmDiscardAll(Workspace& workspace) noexcept {
        if(surface_!=Surface::ConfirmDiscardAll)return false;
        workspace.discardAll();
        surface_=Surface::Browse;
        return true;
    }

    // Discard All must refresh atomically too. If an adapter rejects the
    // restored source presentation, keep the prior staged bytes and the
    // confirmation state so the user can cancel without losing anything.
    template <typename Refresh>
    [[nodiscard]] bool discardAllWithPresentation(Workspace& workspace,
                                                  Refresh&& refresh,std::string& error) {
        if(surface_!=Surface::ConfirmDiscardAll) {
            error="Gen V Discard All requires an explicit confirmation";
            return false;
        }
        Workspace previous=workspace;
        if(!confirmDiscardAll(workspace))return false;
        if(std::forward<Refresh>(refresh)(error)) {
            error.clear();
            return true;
        }
        workspace=std::move(previous);
        surface_=Surface::ConfirmDiscardAll;
        if(error.empty())error="Gen V discard refresh rejected; staged changes preserved";
        return false;
    }
    [[nodiscard]] bool confirmDiscardAndExit(Workspace& workspace) noexcept {
        if(surface_!=Surface::ConfirmExit)return false;
        workspace.discardAll();
        surface_=Surface::Browse;
        return true;
    }
    void reviewFromExit(const Workspace& workspace) noexcept {
        if(surface_==Surface::ConfirmExit && workspace.hasChanges()) {
            reviewPage_=0;
            surface_=Surface::Review;
        }
    }

private:
    [[nodiscard]] static bool containsAsciiCaseInsensitive(
        std::string_view source,std::string_view query) noexcept {
        if(query.empty())return true;
        if(query.size()>source.size())return false;
        const auto lower=[](unsigned char c) {
            return c>='A' && c<='Z' ? static_cast<unsigned char>(c+32) : c;
        };
        for(size_t i=0;i+query.size()<=source.size();++i) {
            bool equal=true;
            for(size_t j=0;j<query.size();++j)
                if(lower(static_cast<unsigned char>(source[i+j]))!=
                   lower(static_cast<unsigned char>(query[j]))) {
                    equal=false;
                    break;
                }
            if(equal)return true;
        }
        return false;
    }
    Surface surface_=Surface::Browse;
    Slot target_{};
    int actionRow_=0;
    int fieldRow_=0;
    uint16_t movePickerSelection_=0;
    uint8_t natureSelection_=0;
    uint32_t identitySelection_=0;
    std::array<uint16_t,649> speciesResults_{};
    size_t speciesCount_=0;
    size_t speciesIndex_=0;
    std::string speciesSearch_;
    bool black2White2_=false;
    size_t locationPickerIndex_=0;
    size_t reviewPage_=0;
    size_t cloneDestinationIndex_=0;
    Gen5SharedPokemonSession draft_;
    std::optional<CreateRequest5> pendingCreateInput_;
};

} // namespace PokeVault::Integration::Gen5
#endif
