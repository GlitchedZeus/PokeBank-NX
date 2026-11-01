#ifndef POKEBANK_GEN5_SHARED_SCREEN_STATE_H
#define POKEBANK_GEN5_SHARED_SCREEN_STATE_H

#include "Integration/Gen5/Gen5SharedEditorBridge.h"
#include "Integration/Gen5/Gen5CreateValidation.h"
#include "Integration/Gen5/Gen5CreateInputRules.h"
#include "Integration/Gen5/Gen5TrainerName.h"
#include "Integration/Gen5/Gen5ReviewRows.h"
#include "Integration/Gen5/Gen5MoveCompatibility.h"
#include "Integration/Encounter/EncounterContext45.h"
#include "UI/CreateRequiredFields.h"
#include "UI/SharedReviewPresentation.h"
#include "UI/SpeciesChangeHistoryPolicy.h"
#include "UI/SpeciesChangeMoves.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace PokeVault::Integration::Gen5 {

// Round 11 (owner H28, UI/SpeciesChangeMoves.h): the moves a Gen V Pokemon keeps when it becomes
// `species` -- each current move the new species can use in this exact game (Gen5MoveCompatibility:
// native, Transfer or Form history) with its own PP and PP Ups; the rest clear; nothing is added.
// Edit (applySpecies) and the Create rebuild (recreateCreateSpecies) both use it.
[[nodiscard]] inline PokeBank::UIModel::SpeciesMoveDefaults::Slots speciesChangeMoves5(
    std::string_view game, uint32_t species, uint8_t form, const Pokemon5ReadOnly& p) {
    return PokeBank::UIModel::SpeciesMoveDefaults::keepLearnable(p.moves(), p.pp(), p.ppUps(), [&](uint16_t move) {
        return Gen5MoveCompatibility::classify(game, static_cast<uint16_t>(species), form, move, false) !=
               Gen5MoveCompatibility::Availability::Unavailable;
    });
}

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
        Browse, Actions, View, Edit, CloneDestination, ConfirmRemoveBox, ConfirmCreateBox, ConfirmDraft, Review,
        ConfirmDiscardAll, ConfirmExport, ConfirmExit,
        // Neutral Origin & Legality panel (Round 3 Part 9): read-only, no verdict.
        Legality
    };
    [[nodiscard]] Surface surface() const noexcept { return surface_; }
    [[nodiscard]] const Slot& target() const noexcept { return target_; }
    [[nodiscard]] int actionRow() const noexcept { return actionRow_; }
    [[nodiscard]] bool black2White2() const noexcept { return black2White2_; }
    [[nodiscard]] size_t reviewPage() const noexcept { return reviewPage_; }
    [[nodiscard]] size_t cloneDestinationIndex() const noexcept { return cloneDestinationIndex_; }
    // The destination picker is shared by Clone and Move Pokemon.
    [[nodiscard]] bool moveDestination() const noexcept { return moveDestination_; }
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
    static constexpr size_t kReviewPageSize = PokeBank::UIModel::SharedReview::kPageSize;
    [[nodiscard]] const Gen5SharedPokemonSession& draft() const noexcept { return draft_; }
    [[nodiscard]] bool isEditing() const noexcept { return surface_==Surface::Edit; }

    [[nodiscard]] bool openActions(const Workspace& /*workspace*/,Slot slot,
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
            reviewReturn_=Surface::Browse;
            surface_=Surface::Review;
            error.clear();
            return true;
        }
        if(action==Action::Clone || action==Action::Move) {
            if(action==Action::Move && target_.region!=Workspace::Region::Box) {
                error="Gen V Move is available for box slots only";
                return false;
            }
            moveDestination_=action==Action::Move;
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
        if(action==Action::LegalityProvenance) {
            surface_=Surface::Legality;
            error.clear();
            return true;
        }
        if(action==Action::Remove) {
            if(target_.region!=Workspace::Region::Box ||
               !(workspace.originalOccupied(target_) || workspace.stagedOperand(target_))) {
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
        // Untouched originals use the strict primitives; a Pokemon staged in
        // this session uses the certificate-preserving companions.
        const bool original=workspace.originalOccupied(target_);
        const size_t box=cloneDestinationIndex_/30,slot=cloneDestinationIndex_%30;
        const bool staged=moveDestination_
            ? (original ? workspace.moveToEmptyBox(target_,box,slot,&error)
                        : workspace.moveStagedToEmptyBox(target_,box,slot,&error))
            : (original ? workspace.cloneToEmptyBox(target_,box,slot,&error)
                        : workspace.cloneStagedToEmptyBox(target_,box,slot,&error));
        if(!staged)return false;
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
        const bool released=workspace.originalOccupied(target_)
            ? workspace.removeBox(target_.box,target_.slot,&error)
            : workspace.releaseStagedBox(target_.box,target_.slot,&error);
        if(!released)return false;
        if(!std::forward<Refresh>(refresh)(error)) {
            workspace=std::move(snapshot);
            if(error.empty())error="Gen V remove presentation rejected; stage rolled back";
            return false;
        }
        surface_=Surface::Browse;
        error.clear();
        return true;
    }
    // Move Pokemon uses the SHARED Gen I-IV carry (ClassicPackedMoveOverlay):
    // Actions -> Move closes the menu and the overlay picks the Pokemon up;
    // nothing is staged until A drops it on a verified empty box slot. B
    // cancels with nothing to restore. The carry remembers the exact staged
    // record it picked up and refuses a drop if that record has changed.
    [[nodiscard]] bool beginMoveCarry(const Workspace& workspace,std::string& error) {
        const auto allowed=availableActions(workspace);
        bool listed=false;
        for(size_t i=0;i<allowed.count;++i)if(allowed[i]==Action::Move)listed=true;
        if(surface_!=Surface::Actions || !listed ||
           target_.region!=Workspace::Region::Box) {
            error="Gen V Move is available for occupied box slots only";
            return false;
        }
        surface_=Surface::Browse;
        error.clear();
        return true;
    }
    template<class Refresh>
    [[nodiscard]] bool dropCarriedPokemon(Workspace& workspace,const Slot& from,
                                          std::span<const uint8_t> pickedUp,
                                          size_t box,size_t slot,
                                          Refresh&& refresh,std::string& error) {
        const auto current=from.region==Workspace::Region::Box
            ? workspace.viewBox(from.box,from.slot) : std::nullopt;
        if(surface_!=Surface::Browse || !current || !current->valid() || current->empty() ||
           !std::equal(current->originalEncryptedBytes().begin(),
                       current->originalEncryptedBytes().end(),
                       pickedUp.begin(),pickedUp.end())) {
            error="Gen V carried Pokemon changed; no move staged";
            return false;
        }
        if(!workspace.emptyCloneDestination(box,slot)) {
            error="Box "+std::to_string(box+1)+" / Slot "+std::to_string(slot+1)+
                  " is not an empty verified Gen V box slot";
            return false;
        }
        Workspace snapshot=workspace;
        const bool staged=workspace.originalOccupied(from)
            ? workspace.moveToEmptyBox(from,box,slot,&error)
            : workspace.moveStagedToEmptyBox(from,box,slot,&error);
        if(!staged)return false;
        if(!std::forward<Refresh>(refresh)(error)) {
            workspace=std::move(snapshot);
            if(error.empty())error="Gen V move presentation rejected; stage rolled back";
            return false;
        }
        error.clear();
        return true;
    }
    // Round 11 H31 (owner): Clone uses the same shared carry as Move. Actions -> Clone closes
    // the menu and the overlay carries a ghost copy; nothing is staged until A drops it on a
    // verified empty box slot, B cancels with nothing to restore. Unlike Move, a Party source is
    // allowed: it lands as the native boxed form (cloneToEmptyBox / cloneStagedToEmptyBox).
    [[nodiscard]] bool beginCloneCarry(const Workspace& workspace,std::string& error) {
        const auto allowed=availableActions(workspace);
        bool listed=false;
        for(size_t i=0;i<allowed.count;++i)if(allowed[i]==Action::Clone)listed=true;
        if(surface_!=Surface::Actions || !listed) {
            error="Gen V Clone is not available for this slot";
            return false;
        }
        if(workspace.firstEmptyCloneDestination()>=720) {
            error="Gen V has no verified empty box destination";
            return false;
        }
        surface_=Surface::Browse;
        error.clear();
        return true;
    }
    // The Clone drop: the exact staged PK5 picked up (box or party) must be unchanged, and the
    // destination must be a verified empty box slot (never an occupied, staged or non-native slot).
    // One snapshot transaction: a refused presentation rolls the clone back.
    template<class Refresh>
    [[nodiscard]] bool dropCarriedClone(Workspace& workspace,const Slot& from,
                                        std::span<const uint8_t> pickedUp,
                                        size_t box,size_t slot,
                                        Refresh&& refresh,std::string& error) {
        const auto current=from.region==Workspace::Region::Box
            ? workspace.viewBox(from.box,from.slot) : workspace.viewParty(from.slot);
        if(surface_!=Surface::Browse || !current || !current->valid() || current->empty() ||
           !std::equal(current->originalEncryptedBytes().begin(),
                       current->originalEncryptedBytes().end(),
                       pickedUp.begin(),pickedUp.end())) {
            error="Gen V carried Pokemon changed; no clone staged";
            return false;
        }
        if(!workspace.emptyCloneDestination(box,slot)) {
            error="Box "+std::to_string(box+1)+" / Slot "+std::to_string(slot+1)+
                  " is not an empty verified Gen V box slot";
            return false;
        }
        Workspace snapshot=workspace;
        const bool staged=workspace.originalOccupied(from)
            ? workspace.cloneToEmptyBox(from,box,slot,&error)
            : workspace.cloneStagedToEmptyBox(from,box,slot,&error);
        if(!staged) {
            workspace=std::move(snapshot);
            return false;
        }
        if(!std::forward<Refresh>(refresh)(error)) {
            workspace=std::move(snapshot);
            if(error.empty())error="Gen V clone presentation rejected; stage rolled back";
            return false;
        }
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
    // ---- Shared Create (Gen I-IV parity) --------------------------------
    // Create opens the SAME Edit draft used by Edit, not a prompt wizard. A
    // native PK5 is built from the save's own trainer (OT name, TID, SID,
    // gender, language, exact game) with the caller's PID and date, placed in
    // the empty slot, and opened for editing on Species. Nothing persists
    // unless Keep is chosen: B asks Keep/Discard, and Discard undoes the
    // staged creation so the slot is the original native empty slot again.
    [[nodiscard]] bool createPending() const noexcept { return createPending_; }
    // Owner item 4: a new Pokemon's Met Location is the user's choice. The native record needs a
    // valid location to exist at all, so Create stages a placeholder (kSharedCreateMetLocation),
    // shows the row as not chosen, and Keep refuses until a location is picked (picking the
    // placeholder's own value explicitly counts as a choice).
    [[nodiscard]] bool createNeedsMetLocation() const noexcept { return createPending_ && !createMetLocationChosen_; }
    void noteCreateMetLocationChosen() noexcept { if(createPending_) createMetLocationChosen_=true; }
    // Round 11: the shared Gen I-V Keep rule (UI/CreateRequiredFields.h) for the open draft. A
    // pending Create needs a chosen Met Location and at least one move (it starts with none); an
    // Edit is refused only when this session took the record's last move away. Empty: Keep may run.
    [[nodiscard]] std::string missingRequiredFields() const {
        namespace Required = PokeBank::UIModel::CreateRequiredFields;
        const auto current=draft_.current();
        if(!current || !current->valid() || current->empty())return {};
        if(createPending_)
            return Required::message(Required::forCreate(5,!createNeedsMetLocation(),current->moves()));
        const auto original=draft_.originalRecord();
        if(!original)return {};
        return Required::message(Required::forEdit(original->moves(),current->moves()));
    }
    struct CreateDate5 { uint32_t year=0, month=0, day=0; };
    [[nodiscard]] static std::optional<CreateRequest5> sharedCreateDefaults(
        const Workspace& ws, uint32_t pid, std::string_view nickname,
        CreateDate5 date, std::string& error) {
        const auto& trainer=ws.sourceSave().trainer();
        const uint32_t version=trainer.gameVersion;
        if(version<20 || version>23) {
            error="Gen V Create needs the validated exact game version";
            return std::nullopt;
        }
        if(newGameNoTrainer5(trainer.rawName.size(),trainer.tid,trainer.sid,trainer.playedHours,
                             trainer.playedMinutes,trainer.playedSeconds)) {
            error="New-game save: no trainer yet, so there is no Original Trainer to copy. "
                  "Play until the game saves, then Create";
            return std::nullopt;
        }
        CreateTrainer5 ot;
        for(const char16_t ch:trainer.rawName) {
            if(ch<0x20 || ch>0x7e) {
                error="Save trainer name is not plain ASCII; Create cannot copy it as OT";
                return std::nullopt;
            }
            ot.name.push_back(static_cast<char>(ch));
        }
        if(ot.name.empty() || trainer.gender>1 || !createLanguageAllowed5(trainer.language)) {
            error="Save trainer identity is incomplete; Create cannot copy it as OT";
            return std::nullopt;
        }
        ot.tid=trainer.tid;
        ot.sid=trainer.sid;
        ot.gender=trainer.gender;
        ot.language=trainer.language;
        CreateRequest5 r;
        r.game=static_cast<CreateGame5>(version);
        r.format=CreateFormat5::Boxed;
        r.species=kSharedCreateSpecies; r.form=0; r.level=5;
        r.pid=pid;
        r.nature=pid%25;
        const auto personal=personal5(r.species,0,version>=22);
        if(!personal) {
            error="Gen V personal data unavailable for the default species";
            return std::nullopt;
        }
        r.gender=createGenderForPID5(personal.info->genderRatio,pid);
        r.abilitySlot=(pid>>16)&1u;
        r.friendship=70;
        r.heldItem=0;
        r.trainer=ot;
        r.nickname=std::string(nickname);
        r.encounter={CreateHistory5::CallerSuppliedNative,kSharedCreateMetLocation,r.level,
                     kSharedCreateBall,date.year,date.month,date.day};
        r.ivs={};
        r.evs={};
        // Round 11 (owner H28 / Extra 5 / Extra 6): a new Pokemon starts with no moves; the user
        // picks them and Keep refuses until one is chosen (missingRequiredFields).
        r.moves={};
        r.draftWithoutMoves=true;
        if(const auto checked=validateCreate5(r);!checked) {
            error=std::string(checked.message);
            return std::nullopt;
        }
        error.clear();
        return r;
    }
    template<class Refresh>
    [[nodiscard]] bool beginCreateDraft(Workspace& ws,const CreateRequest5& request,
                                        Refresh&& refresh,std::string& error) {
        if(surface_!=Surface::ConfirmCreateBox ||
           target_.region!=Workspace::Region::Box ||
           !ws.emptyCloneDestination(target_.box,target_.slot)) {
            error="Gen V Create requires the original empty native box slot";
            return false;
        }
        Workspace snapshot=ws;
        if(!ws.createInEmptyBox(request,target_.box,target_.slot,&error))return false;
        if(!refresh(error)) {
            ws=std::move(snapshot);
            std::string restored;
            (void)refresh(restored);
            if(error.empty())error="Gen V Create presentation failed; stage rolled back";
            return false;
        }
        if(!openSharedDraft(draft_,ws,selectedSlot(ws,target_),Action::Edit,error)) {
            ws=std::move(snapshot);
            std::string restored;
            (void)refresh(restored);
            return false;
        }
        pendingCreateInput_.reset();
        black2White2_=ws.black2White2();
        surface_=Surface::Edit;
        createPending_=true;
        createMetLocationChosen_=false;
        error.clear();
        return true;
    }
    template<class Refresh>
    [[nodiscard]] bool discardCreateDraft(Workspace& ws,Refresh&& refresh,std::string& error) {
        if(!createPending_ || (surface_!=Surface::Edit && surface_!=Surface::ConfirmDraft) ||
           target_.region!=Workspace::Region::Box) {
            error="No pending Gen V Create to discard";
            return false;
        }
        Workspace snapshot=ws;
        if(!ws.undoBoxReplacement(target_.box,target_.slot)) {
            error="Gen V Create was already cleared";
            return false;
        }
        if(!refresh(error)) {
            ws=std::move(snapshot);
            if(error.empty())error="Gen V Create discard presentation failed; draft kept";
            return false;
        }
        draft_.discardDraft();
        createPending_=false;
        surface_=Surface::Browse;
        error.clear();
        return true;
    }
    // The shared editor workspace (SharedPokemonEditorSurface.inc) stages
    // every field through this guarded native draft while View/Edit is open.
    [[nodiscard]] Gen5SharedPokemonSession& editDraft() noexcept { return draft_; }
    // Why an occupied Pokemon cannot become `species` without inventing data;
    // empty when the native identity transaction can preserve it.
    [[nodiscard]] std::string speciesRefusal(uint32_t species) const {
        const auto current=draft_.current();
        if(!current || !current->valid() || current->empty())
            return "No Generation V Pokemon draft is open";
        if(species<1 || species>649)
            return "Black/White and Black 2/White 2 have National Dex species 1-649 only";
        if(species==current->species())return {};
        if(current->form()!=0 || !draft_.originalBaseFormForSpecies())
            return "Species can change only on a base-form Pokemon; set Form to its default first";
        if((Crypto::read32(current->decodedBytes(),0x38)&0x80000000u)==0)
            return "Nickname this Pokemon first: an unnicknamed Pokemon's name follows its species";
        const auto source=personal5(current->species(),0,black2White2_);
        const auto target=personal5(species,0,black2White2_);
        if(!source || !target || source.info->growthRate>5 ||
           source.info->growthRate!=target.info->growthRate)
            return "That species uses a different EXP growth group; its level would change";
        if(!genderAllowed5(target.info->genderRatio,current->gender()))
            return "That species cannot have this Pokemon's gender";
        if(genderFromPid5(current->pid(),source.info->genderRatio)==current->gender() &&
           genderFromPid5(current->pid(),target.info->genderRatio)!=current->gender())
            return "That species would read this Pokemon's PID as the other gender";
        const uint8_t slot=current->hiddenAbility()?2:
            static_cast<uint8_t>((current->pid()>>16)&1);
        if(abilityForSlot5(*target.info,slot)==0)
            return "That species has no ability in this Pokemon's ability slot";
        return {};
    }
    // A pending Create has no history to preserve, so a species choice rebuilds
    // the created PK5 natively (validateCreate5) with the draft's own values:
    // PID (shiny state as requested), nature, level, IVs, EVs, item, met data
    // and nickname (a default nickname follows the new species). Round 11:
    // moves -- a real species change keeps only the draft's moves the new
    // species can use (speciesChangeMoves5) and never adds one, so a blank draft
    // stays blank; the same species (a shiny-only rebuild) keeps the draft's own
    // moves. History (SpeciesChangeHistoryPolicy, owner H25) -- Pokerus is never
    // carried (the rebuild has none), and a chosen Met Location that is neither
    // a wild encounter of the new species or its ancestry nor a noted special
    // location goes back to the placeholder and "Choose a location".
    // `encounterReset` reports that last case to the caller's status line.
    template<class Refresh>
    [[nodiscard]] bool recreateCreateSpecies(Workspace& ws,uint32_t species,bool shiny,
                                             std::string_view newDefaultName,
                                             std::string_view oldDefaultName,
                                             Refresh&& refresh,std::string& error) {
        bool encounterReset=false;
        return recreateCreateSpecies(ws,species,shiny,newDefaultName,oldDefaultName,
                                     std::forward<Refresh>(refresh),error,encounterReset);
    }
    template<class Refresh>
    [[nodiscard]] bool recreateCreateSpecies(Workspace& ws,uint32_t species,bool shiny,
                                             std::string_view newDefaultName,
                                             std::string_view oldDefaultName,
                                             Refresh&& refresh,std::string& error,
                                             bool& encounterReset) {
        encounterReset=false;
        const auto p=draft_.current();
        if(!createPending_ || surface_!=Surface::Edit || !p ||
           target_.region!=Workspace::Region::Box) {
            error="Gen V species rebuild requires a pending Create draft";
            return false;
        }
        const auto& trainer=ws.sourceSave().trainer();
        const auto target=personal5(species,0,trainer.gameVersion>=22);
        if(species<1 || species>649 || !target) {
            error="Black/White and Black 2/White 2 have National Dex species 1-649 only";
            return false;
        }
        CreateRequest5 r;
        r.game=static_cast<CreateGame5>(trainer.gameVersion);
        r.format=CreateFormat5::Boxed;
        r.species=species; r.form=0;
        const auto oldPersonal=personal5(p->species(),p->form(),black2White2_);
        r.level=p->partyRecord()?p->partyLevel():
            oldPersonal?levelForExperience5(oldPersonal.info->growthRate,p->experience()):5;
        if(r.level<1)r.level=1;
        uint32_t pid=withShiny5(p->pid(),p->tid(),p->sid(),shiny);
        r.pid=pid;
        r.nature=p->nature();
        r.gender=createGenderForPID5(target.info->genderRatio,pid);
        r.abilitySlot=p->hiddenAbility() && target.info->abilityHidden?2u:((pid>>16)&1u);
        if(abilityForSlot5(*target.info,r.abilitySlot)==0)r.abilitySlot=(pid>>16)&1u;
        r.friendship=p->friendship();
        r.heldItem=p->heldItem();
        CreateTrainer5 ot;
        const auto otName=asciiName5(p->decodedBytes(),false);
        ot.name=otName?*otName:std::string{};
        ot.tid=p->tid(); ot.sid=p->sid(); ot.gender=p->originalTrainerGender();
        ot.language=p->language();
        r.trainer=ot;
        const auto nickname=asciiName5(p->decodedBytes(),true);
        r.nickname=nickname && *nickname!=oldDefaultName?*nickname:std::string(newDefaultName);
        const auto met=p->metDate();
        namespace History=PokeBank::UIModel::SpeciesChangeHistoryPolicy;
        const uint8_t origin=static_cast<uint8_t>(trainer.gameVersion);
        const auto history=species==p->species() ? History::Decision{} : History::reconcile(
            History::Mode::Create,createMetLocationChosen_,[&] {
                const uint16_t location=static_cast<uint16_t>(p->metLocation());
                const EncounterContext45::Catalogue context(origin,static_cast<uint16_t>(species),location,{},{});
                return context.wildRow(location)!=nullptr ||
                       !::Names::specialMetLocation(origin,location).empty();
            });
        const uint32_t metLocation=history.resetMetLocation?kSharedCreateMetLocation:p->metLocation();
        r.encounter={CreateHistory5::CallerSuppliedNative,metLocation,
                     std::min<uint32_t>(std::max<uint32_t>(p->metLevel(),1),r.level),
                     p->ball(),met.year,met.month,met.day};
        const auto ivs=p->ivs(); const auto evs=p->evs();
        for(size_t i=0;i<6;++i){r.ivs[i]=ivs[i];r.evs[i]=evs[i];}
        if(species!=p->species()) {
            const auto kept=speciesChangeMoves5(EncounterContext45::gameId(origin),species,0,*p);
            for(size_t i=0;i<4;++i)r.moves[i]={kept.moves[i],kept.moves[i]?kept.ppUps[i]:0u,
                                               kept.moves[i]?std::optional<uint32_t>(kept.pp[i]):std::nullopt};
        } else {
            const auto moves=p->moves(); const auto ups=p->ppUps(); const auto pp=p->pp();
            for(size_t i=0;i<4;++i)r.moves[i]={moves[i],moves[i]?ups[i]:0u,
                                               moves[i]?std::optional<uint32_t>(pp[i]):std::nullopt};
        }
        r.draftWithoutMoves=r.moves[0].id==0;
        if(const auto checked=validateCreate5(r);!checked) {
            error=std::string(checked.message);
            return false;
        }
        Workspace snapshot=ws;
        Gen5SharedPokemonSession draftSnapshot=draft_;
        if(!ws.undoBoxReplacement(target_.box,target_.slot) ||
           !ws.createInEmptyBox(r,target_.box,target_.slot,&error)) {
            ws=std::move(snapshot);
            if(error.empty())error="Gen V Create could not be rebuilt for that species";
            return false;
        }
        draft_.discardDraft();
        if(!refresh(error) ||
           !openSharedDraft(draft_,ws,selectedSlot(ws,target_),Action::Edit,error)) {
            ws=std::move(snapshot);
            draft_=std::move(draftSnapshot);
            std::string restored;
            (void)refresh(restored);
            if(error.empty())error="Gen V Create presentation failed; previous draft kept";
            return false;
        }
        if(history.resetMetLocation)createMetLocationChosen_=false;
        encounterReset=history.resetMetLocation;
        error.clear();
        return true;
    }
    void cancelBoxAction() noexcept {
        if(surface_==Surface::ConfirmCreateBox)pendingCreateInput_.reset();
        if(surface_==Surface::CloneDestination ||
           surface_==Surface::ConfirmRemoveBox ||
           surface_==Surface::ConfirmCreateBox)surface_=Surface::Actions;
    }
    void moveReviewPage(int delta,const Workspace& workspace,size_t presentationRows=0) {
        if(surface_!=Surface::Review)return;
        reviewPage_=PokeBank::UIModel::SharedReview::move(reviewPage_,delta,
            presentationRows?presentationRows:reviewRowCount(workspace));
    }

    // Review lists staged PK5 records (a staged Move as ONE row, Gen5ReviewRows.h), then
    // each verified Bag change, then each verified Trainer field change.
    [[nodiscard]] static size_t reviewRowCount(const Workspace& workspace) {
        return reviewRecordRows5(workspace)+workspace.bagReviewChanges().size()+
               workspace.trainerReviewChanges().size();
    }
    // The Bag has no Pokemon action menu, so the shared Items panel opens the
    // same Review surface directly (+). Nothing is staged or exported here.
    [[nodiscard]] bool openReviewFromBrowse(const Workspace& workspace) noexcept {
        if(surface_!=Surface::Browse || !workspace.hasChanges())return false;
        reviewPage_=0;
        reviewReturn_=Surface::Browse;
        surface_=Surface::Review;
        return true;
    }
    // Round 6 (P1-RETURN): + in the Create / Edit workspace opens the same Review, and B there
    // returns to that workspace with its draft (and Create's staged record) intact. Only Browse
    // and Edit may open it; every other surface is a modal step that must finish first.
    [[nodiscard]] bool openReviewFrom(const Workspace& workspace) noexcept {
        if(surface_==Surface::Browse)return openReviewFromBrowse(workspace);
        if(surface_!=Surface::Edit || !workspace.hasChanges())return false;
        reviewPage_=0;
        reviewReturn_=Surface::Edit;
        surface_=Surface::Review;
        return true;
    }
    // Where B from Review goes: the workspace it was opened from, otherwise Browse.
    [[nodiscard]] Surface reviewReturn() const noexcept { return reviewReturn_; }

    [[nodiscard]] bool keep(Workspace& workspace,std::string& error) {
        // A on the dirty-draft decision is a real Keep, not a dead action.
        // Both states own the SAME active edit draft; View cannot Keep.
        if((surface_!=Surface::Edit && surface_!=Surface::ConfirmDraft) ||
           !draft_.editable()) {
            error="Generation V Keep requires an active Edit draft";
            return false;
        }
        if(auto missing=missingRequiredFields();!missing.empty()) {
            error=std::move(missing);
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
        // H07 (owner PASS, now for every generation): one status naming every missing field.
        if(auto missing=missingRequiredFields();!missing.empty()) {
            error=std::move(missing);
            return false;
        }
        if(createPending_ && !draft_.dirty()) {
            // The created record is already staged and presented; Keep with
            // no further edits simply finalizes it.
            draft_.discardDraft();
            createPending_=false;
            surface_=Surface::Browse;
            error.clear();
            return true;
        }
        Workspace baselineWorkspace=workspace;
        Gen5SharedPokemonSession baselineDraft=draft_;
        const Surface baselineSurface=surface_;
        if(!keep(workspace,error))return false;
        if(std::forward<Refresh>(refresh)(error)) {
            createPending_=false;
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
        if(surface_==Surface::CloneDestination ||
           surface_==Surface::ConfirmRemoveBox ||
           surface_==Surface::ConfirmCreateBox) {
            cancelBoxAction();
            return false;
        }
        if(surface_==Surface::Edit && createPending_) {
            surface_=Surface::ConfirmDraft;
            return false;
        }
        if(surface_==Surface::View || surface_==Surface::Edit) {
            if(draft_.back()) {surface_=Surface::Browse;return true;}
            surface_=Surface::ConfirmDraft;
            return false;
        }
        if(surface_==Surface::Review) {
            surface_=reviewReturn_;
            reviewReturn_=Surface::Browse;
        } else if(surface_==Surface::Actions)
            surface_=Surface::Browse;
        else if(surface_==Surface::Legality)surface_=Surface::Actions;
        else if(surface_==Surface::ConfirmDraft)continueEditing();
        else if(surface_==Surface::ConfirmDiscardAll)surface_=Surface::Review;
        else if(surface_==Surface::ConfirmExport)surface_=Surface::Review;
        else if(surface_==Surface::ConfirmExit)surface_=Surface::Browse;
        return false;
    }
    void continueEditing() noexcept {
        if(surface_==Surface::ConfirmDraft) {
            if(!createPending_)draft_.continueEditing();
            surface_=Surface::Edit;
        }
    }
    void discardDraft() noexcept {
        if(createPending_)return; // use discardCreateDraft: it undoes the staged record
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
        if(reviewReturn_!=Surface::Browse) {
            // Discard All removed what the workspace was editing (a Create's staged record
            // included): never return into it; the draft goes with the discarded changes.
            reviewReturn_=Surface::Browse;
            draft_.discardDraft();
            createPending_=false;
            pendingCreateInput_.reset();
        }
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
        // A rejected refresh restores everything, including a workspace Review's draft.
        Gen5SharedPokemonSession previousDraft=draft_;
        const Surface previousReturn=reviewReturn_;
        const bool previousCreatePending=createPending_;
        const auto previousCreateInput=pendingCreateInput_;
        if(!confirmDiscardAll(workspace))return false;
        if(std::forward<Refresh>(refresh)(error)) {
            error.clear();
            return true;
        }
        workspace=std::move(previous);
        draft_=std::move(previousDraft);
        reviewReturn_=previousReturn;
        createPending_=previousCreatePending;
        pendingCreateInput_=previousCreateInput;
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
            reviewReturn_=Surface::Browse;
            surface_=Surface::Review;
        }
    }

private:
    Surface surface_=Surface::Browse;
    Slot target_{};
    int actionRow_=0;
    bool black2White2_=false;
    size_t reviewPage_=0;
    size_t cloneDestinationIndex_=0;
    bool moveDestination_=false;
    Gen5SharedPokemonSession draft_;
    std::optional<CreateRequest5> pendingCreateInput_;
    bool createPending_=false;
    bool createMetLocationChosen_=false;
    Surface reviewReturn_=Surface::Browse;
public:
    // Shared Create defaults (not format rules): a native species, move and
    // met data the Gen V validators accept, all editable before Keep.
    static constexpr uint32_t kSharedCreateSpecies=25;    // Pikachu (no moves: Round 11)
    static constexpr uint32_t kSharedCreateMetLocation=1; // native location list entry
    static constexpr uint32_t kSharedCreateBall=4;        // Poke Ball
};

} // namespace PokeVault::Integration::Gen5
#endif
