#ifndef POKEBANK_GEN5_SHARED_SCREEN_STATE_H
#define POKEBANK_GEN5_SHARED_SCREEN_STATE_H

#include "Integration/Gen5/Gen5SharedEditorBridge.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace PokeVault::Integration::Gen5 {

// Session-local control model for the ONE shared Trainer/Party/Boxes screen.
// It owns only app-memory PK5 drafts. No SAV5 writer, Bank, Create, Inject,
// physical filesystem mutation, or unknown Gen V legality interpretation.
class Gen5SharedScreenState {
public:
    using Workspace=Gen5StagedPokemonWorkspace;
    using Slot=Workspace::Slot;
    using Action=Shared::Action;
    using Field=Shared::FieldIdentity;
    enum class Surface : uint8_t {
        Browse, Actions, View, Edit, ConfirmDraft, Review,
        ConfirmDiscardAll, ConfirmExit
    };
    struct EditableRow { Field field; size_t stat; const char* label; uint32_t maximum; };
    static constexpr std::array<EditableRow,14> kEditableRows{{
        {Field::Nature,0,"Nature",24}, {Field::Friendship,0,"Friendship",255},
        {Field::IV,0,"IV HP",31}, {Field::IV,1,"IV Atk",31},
        {Field::IV,2,"IV Def",31}, {Field::IV,3,"IV Spe",31},
        {Field::IV,4,"IV SpA",31}, {Field::IV,5,"IV SpD",31},
        {Field::EV,0,"EV HP",255}, {Field::EV,1,"EV Atk",255},
        {Field::EV,2,"EV Def",255}, {Field::EV,3,"EV Spe",255},
        {Field::EV,4,"EV SpA",255}, {Field::EV,5,"EV SpD",255}
    }};

    [[nodiscard]] Surface surface() const noexcept { return surface_; }
    [[nodiscard]] const Slot& target() const noexcept { return target_; }
    [[nodiscard]] int actionRow() const noexcept { return actionRow_; }
    [[nodiscard]] int fieldRow() const noexcept { return fieldRow_; }
    [[nodiscard]] size_t reviewPage() const noexcept { return reviewPage_; }
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
        if(action==Action::View || action==Action::Edit) {
            if(!openSharedDraft(draft_,workspace,selectedSlot(workspace,target_),action,error))
                return false;
            surface_=action==Action::View?Surface::View:Surface::Edit;
            fieldRow_=0;
            return true;
        }
        error="Generation V has no Create, Clone, Remove or automatic fix";
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
    [[nodiscard]] static uint32_t fieldValue(const Pokemon5ReadOnly& pk,
                                              const EditableRow& row) noexcept {
        switch(row.field) {
            case Field::Nature:return pk.nature();
            case Field::Friendship:return pk.friendship();
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
        const int64_t value=static_cast<int64_t>(fieldValue(*current,row))+delta;
        // No modulo wrapping; failed EV totals and range errors preserve
        // the full draft instead of introducing invalid intermediate states.
        if(value<0 || value>row.maximum) {
            error="Generation V value is outside the native field range";
            return false;
        }
        return stageSharedField(draft_,row.field,row.stat,
                                static_cast<uint32_t>(value),error);
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
        if(surface_==Surface::View || surface_==Surface::Edit) {
            if(draft_.back()) {surface_=Surface::Browse;return true;}
            surface_=Surface::ConfirmDraft;
            return false;
        }
        if(surface_==Surface::Actions || surface_==Surface::Review)
            surface_=Surface::Browse;
        else if(surface_==Surface::ConfirmDraft)continueEditing();
        else if(surface_==Surface::ConfirmDiscardAll)surface_=Surface::Review;
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
    Surface surface_=Surface::Browse;
    Slot target_{};
    int actionRow_=0;
    int fieldRow_=0;
    size_t reviewPage_=0;
    Gen5SharedPokemonSession draft_;
};

} // namespace PokeVault::Integration::Gen5
#endif
