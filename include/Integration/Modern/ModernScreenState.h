#ifndef POKEBANK_MODERN_SCREEN_STATE_H
#define POKEBANK_MODERN_SCREEN_STATE_H

// Session-local control model of the ONE shared Trainer/Party/Boxes/Items/Trainer screen for every
// Gen VI-IX title served by a Modern::NativeSession adapter (src/UI/ModernSharedSurface.inc).
//
// Modelled on Integration/Gen5/Gen5SharedScreenState.h, but generic over the adapter contract:
// every slot operation, draft bracket, bag and export call goes through NativeSession, and every
// available action is derived from the adapter's own support answers (cloneSupport / moveSupport /
// removeSupport / createSupport / exportSupport) AND the capability registry row the adapter
// carries. Nothing here owns bytes, writes a save or knows a title family; it is host-testable with
// no switch.h dependency (tests/test_modern_screen_state.cpp).
//
// Draft semantics (the Gen I-V "Keep / Discard draft" rules): View never begins a draft; Edit
// begins the adapter's draft bracket; B on a dirty draft asks Keep / Discard / Continue; Keep is
// refused (and the draft kept) when the Trainer presentation rejects the staged view; Discard
// restores the pre-draft snapshot through the adapter.

#include "Integration/Common/GameCapabilityRegistry.h"
#include "Integration/Modern/NativeSession.h"
#include "UI/SharedPokemonEditorContract.h"
#include "UI/SharedReviewPresentation.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace PokeVault::Integration::Modern {

class ModernScreenState {
public:
    using Action = PokeBank::UIModel::SharedPokemonEditor::Action;
    using ActionSet = PokeBank::UIModel::SharedPokemonEditor::ActionSet;
    using Workflow = Capability::Workflow;

    enum class Surface : uint8_t {
        Browse, Actions, View, Edit, ConfirmDraft, CloneDestination, ConfirmRemoveBox, Review,
        ConfirmDiscardAll, ConfirmExport, ConfirmExit,
        // Neutral Origin & Legality panel: stored facts only, never a verdict.
        Legality
    };
    static constexpr size_t kReviewPageSize = PokeBank::UIModel::SharedReview::kPageSize;

    [[nodiscard]] Surface surface() const noexcept { return surface_; }
    [[nodiscard]] const SlotRef& target() const noexcept { return target_; }
    [[nodiscard]] int actionRow() const noexcept { return actionRow_; }
    [[nodiscard]] size_t reviewPage() const noexcept { return reviewPage_; }
    [[nodiscard]] Surface reviewReturn() const noexcept { return reviewReturn_; }
    // The destination picker is shared by Clone and Move.
    [[nodiscard]] const std::optional<SlotRef>& cloneDestination() const noexcept { return cloneDestination_; }
    [[nodiscard]] bool moveDestination() const noexcept { return moveDestination_; }
    [[nodiscard]] bool isEditing() const noexcept { return surface_ == Surface::Edit; }
    [[nodiscard]] bool workspaceOpen() const noexcept {
        return surface_ == Surface::View || surface_ == Surface::Edit || surface_ == Surface::ConfirmDraft;
    }
    // The record exactly as it was when the current Edit draft began (inline toggles such as
    // Pokerus cycle back to this byte); empty outside a draft.
    [[nodiscard]] const std::optional<RecordView>& draftBaseline() const noexcept { return draftBaseline_; }

    // ---- slot facts (staged view) ----
    [[nodiscard]] static bool canonicalSlot(const NativeSession& s, SlotRef slot) {
        return s.view(slot).has_value();
    }
    [[nodiscard]] static bool occupied(const NativeSession& s, SlotRef slot) {
        const auto v = s.view(slot);
        return v && v->valid && !v->empty;
    }
    [[nodiscard]] static bool verifiedEmpty(const NativeSession& s, SlotRef slot) {
        const auto v = s.view(slot);
        return v && v->valid && v->empty;
    }

    // ---- available actions: adapter support answers AND the registry row ----
    [[nodiscard]] static ActionSet actionsFor(const NativeSession& s, SlotRef slot) {
        ActionSet result{};
        const auto append = [&result](Action action) {
            if (result.count < result.values.size()) result.values[result.count++] = action;
        };
        const auto& caps = s.capabilities();
        if (occupied(s, slot)) {
            append(Action::View);
            if (caps.visible(Workflow::PokemonEditor)) append(Action::Edit);
            if (caps.supports(Workflow::Clone) && s.cloneSupport(slot).available) append(Action::Clone);
            if (caps.supports(Workflow::Move) && s.moveSupport(slot).available) append(Action::Move);
            if (caps.supports(Workflow::Remove) && s.removeSupport(slot).available) append(Action::Remove);
            append(Action::LegalityProvenance);
        } else if (verifiedEmpty(s, slot) && slot.region == Region::Box &&
                   caps.supports(Workflow::Create) && s.createSupport(slot).available) {
            append(Action::Add);
        }
        if (caps.visible(Workflow::Review) && s.hasChanges()) append(Action::Review);
        append(Action::Close);
        return result;
    }
    // Why an empty slot offers no Add (the registry/adapter prerequisite), empty when Add is listed
    // or the slot is occupied. The Actions sheet footer shows this instead of a dead row.
    [[nodiscard]] static std::string createRefusal(const NativeSession& s, SlotRef slot) {
        if (occupied(s, slot) || !canonicalSlot(s, slot)) return {};
        const auto& status = s.capabilities().status(Workflow::Create);
        if (status.support != Capability::Support::Supported) return std::string(status.reason);
        if (const auto support = s.createSupport(slot); !support.available) return std::string(support.reason);
        return {};
    }
    // Why Export Copy is unavailable right now, empty when it is offered.
    [[nodiscard]] static std::string exportRefusal(const NativeSession& s) {
        const auto& status = s.capabilities().status(Workflow::Export);
        if (status.support != Capability::Support::Supported) return std::string(status.reason);
        if (const auto support = s.exportSupport(); !support.available) return std::string(support.reason);
        return {};
    }
    [[nodiscard]] static bool exportAvailable(const NativeSession& s) { return exportRefusal(s).empty(); }

    [[nodiscard]] bool openActions(const NativeSession& s, SlotRef slot, std::string& error) {
        if (surface_ != Surface::Browse || !canonicalSlot(s, slot)) {
            error = std::string(s.familyLabel()) + " action selection requires an idle, canonical slot";
            return false;
        }
        target_ = slot;
        actionRow_ = 0;
        surface_ = Surface::Actions;
        error.clear();
        return true;
    }
    [[nodiscard]] ActionSet availableActions(const NativeSession& s) const { return actionsFor(s, target_); }
    void moveAction(int delta, const NativeSession& s) {
        if (surface_ != Surface::Actions) return;
        const auto available = availableActions(s);
        if (available.count == 0) return;
        const int count = static_cast<int>(available.count);
        actionRow_ = (actionRow_ + delta % count + count) % count;
    }
    [[nodiscard]] bool activate(NativeSession& s, Action action, std::string& error) {
        if (surface_ != Surface::Actions) {
            error = "Pokemon Actions are not currently open";
            return false;
        }
        const auto allowed = availableActions(s);
        bool listed = false;
        for (size_t i = 0; i < allowed.count; ++i)
            if (allowed[i] == action) listed = true;
        if (!listed) {
            error = "Unsupported " + std::string(s.familyLabel()) + " action for this slot";
            return false;
        }
        error.clear();
        switch (action) {
            case Action::Close:
                surface_ = Surface::Browse;
                return true;
            case Action::Review:
                reviewPage_ = 0;
                reviewReturn_ = Surface::Browse;
                surface_ = Surface::Review;
                return true;
            case Action::Clone:
            case Action::Move: {
                const auto support = action == Action::Move ? s.moveSupport(target_) : s.cloneSupport(target_);
                if (!support.available) {
                    error = std::string(support.reason);
                    return false;
                }
                const auto first = s.firstEmptyBoxSlot();
                if (!first) {
                    error = std::string(s.familyLabel()) + " has no verified empty box destination";
                    return false;
                }
                moveDestination_ = action == Action::Move;
                cloneDestination_ = *first;
                surface_ = Surface::CloneDestination;
                return true;
            }
            case Action::Remove: {
                const auto support = s.removeSupport(target_);
                if (!support.available) {
                    error = std::string(support.reason);
                    return false;
                }
                surface_ = Surface::ConfirmRemoveBox;
                return true;
            }
            case Action::LegalityProvenance:
                surface_ = Surface::Legality;
                return true;
            case Action::View:
                // View never begins a draft: the staged view is shown read-only.
                if (!occupied(s, target_)) {
                    error = std::string(s.familyLabel()) + " View requires an occupied record";
                    return false;
                }
                draftBaseline_.reset();
                surface_ = Surface::View;
                return true;
            case Action::Edit:
                if (!s.beginDraft(target_, error)) return false;
                draftBaseline_ = s.view(target_);
                surface_ = Surface::Edit;
                return true;
            case Action::Add: {
                const auto support = s.createSupport(target_);
                error = support.available ? std::string(s.familyLabel()) + " Create has no shared draft builder in this build"
                                          : std::string(support.reason);
                return false;
            }
            default:
                break;
        }
        error = "Unsupported shared operation; the original source remains unchanged";
        return false;
    }
    [[nodiscard]] bool activateSelected(NativeSession& s, std::string& error) {
        const auto allowed = availableActions(s);
        if (surface_ != Surface::Actions || actionRow_ < 0 || static_cast<size_t>(actionRow_) >= allowed.count) {
            error = "Shared action selection is unavailable";
            return false;
        }
        return activate(s, allowed[static_cast<size_t>(actionRow_)], error);
    }
    [[nodiscard]] Action selectedAction(const NativeSession& s) const {
        const auto allowed = availableActions(s);
        if (surface_ != Surface::Actions || actionRow_ < 0 || static_cast<size_t>(actionRow_) >= allowed.count)
            return Action::None;
        return allowed[static_cast<size_t>(actionRow_)];
    }

    // ---- Clone / Move destination (the Gen IV/V box + slot keypads choose it) ----
    [[nodiscard]] bool setCloneDestination(const NativeSession& s, SlotRef destination, std::string& error) {
        if (surface_ != Surface::CloneDestination) {
            error = "No Clone or Move destination is being chosen";
            return false;
        }
        if (destination.region != Region::Box || !canonicalSlot(s, destination)) {
            error = "Destination must be a box slot of this save";
            return false;
        }
        cloneDestination_ = destination;
        error.clear();
        return true;
    }
    [[nodiscard]] bool cloneDestinationAllowed(const NativeSession& s) const {
        return cloneDestination_ && s.emptyDestination(*cloneDestination_);
    }
    template <class Refresh>
    [[nodiscard]] bool confirmClone(NativeSession& s, Refresh&& refresh, std::string& error) {
        if (surface_ != Surface::CloneDestination || !cloneDestination_) {
            error = "Clone or Move needs a chosen destination";
            return false;
        }
        const SlotRef destination = *cloneDestination_;
        if (!s.emptyDestination(destination)) {
            error = slotLabel(destination) + " is not an empty verified " + std::string(s.familyLabel()) + " box slot";
            return false;
        }
        const bool staged = moveDestination_ ? s.move(target_, destination, error) : s.clone(target_, destination, error);
        if (!staged) return false;
        if (!refresh(error)) {
            // Atomic: the adapter's per-slot undo restores both ends of the operation.
            (void)s.undo(destination);
            if (moveDestination_) (void)s.undo(target_);
            std::string restored;
            (void)refresh(restored);
            if (error.empty()) error = std::string(s.familyLabel()) + " presentation rejected the staged slot; stage rolled back";
            return false;
        }
        surface_ = Surface::Browse;
        error.clear();
        return true;
    }
    template <class Refresh>
    [[nodiscard]] bool confirmRemoveBox(NativeSession& s, Refresh&& refresh, std::string& error) {
        if (surface_ != Surface::ConfirmRemoveBox) {
            error = "Release requires explicit Y confirmation";
            return false;
        }
        if (!s.remove(target_, error)) return false;
        if (!refresh(error)) {
            (void)s.undo(target_);
            std::string restored;
            (void)refresh(restored);
            if (error.empty()) error = std::string(s.familyLabel()) + " presentation rejected the release; stage rolled back";
            return false;
        }
        surface_ = Surface::Browse;
        error.clear();
        return true;
    }
    void cancelBoxAction() noexcept {
        if (surface_ == Surface::CloneDestination || surface_ == Surface::ConfirmRemoveBox) {
            cloneDestination_.reset();
            surface_ = Surface::Actions;
        }
    }

    // ---- Review ----
    void moveReviewPage(int delta, size_t rows) noexcept {
        if (surface_ != Surface::Review) return;
        reviewPage_ = PokeBank::UIModel::SharedReview::move(reviewPage_, delta, rows);
    }
    // Opened from Party / Boxes / Bag / Trainer / the hub: B returns to Browse.
    [[nodiscard]] bool openReviewFromBrowse(const NativeSession& s) noexcept {
        if (surface_ != Surface::Browse || !s.hasChanges()) return false;
        reviewPage_ = 0;
        reviewReturn_ = Surface::Browse;
        surface_ = Surface::Review;
        return true;
    }
    // + in the Edit workspace opens the same Review; B returns to the workspace with its draft.
    [[nodiscard]] bool openReviewFrom(const NativeSession& s) noexcept {
        if (surface_ == Surface::Browse) return openReviewFromBrowse(s);
        if (surface_ != Surface::Edit || !s.hasChanges()) return false;
        reviewPage_ = 0;
        reviewReturn_ = Surface::Edit;
        surface_ = Surface::Review;
        return true;
    }

    // ---- Keep / Discard / Back ----
    // Keep is complete only once the Trainer presentation accepts the staged view; a rejected
    // presentation keeps the draft (and the workspace) exactly as it was.
    template <class Refresh>
    [[nodiscard]] bool keep(NativeSession& s, Refresh&& refresh, std::string& error) {
        if ((surface_ != Surface::Edit && surface_ != Surface::ConfirmDraft) || !s.draftActive()) {
            error = std::string(s.familyLabel()) + " Keep requires an active Edit draft";
            return false;
        }
        if (!refresh(error)) {
            if (error.empty()) error = std::string(s.familyLabel()) + " staged presentation rejected; draft preserved";
            return false;
        }
        s.keepDraft();
        draftBaseline_.reset();
        surface_ = Surface::Browse;
        error.clear();
        return true;
    }
    // B never loses a dirty draft: Edit asks first. Returns true when the screen is back in Browse.
    [[nodiscard]] bool back(NativeSession& s) {
        switch (surface_) {
            case Surface::CloneDestination:
            case Surface::ConfirmRemoveBox:
                cancelBoxAction();
                return false;
            case Surface::View:
                surface_ = Surface::Browse;
                return true;
            case Surface::Edit:
                if (s.draftActive() && s.draftDirty()) {
                    surface_ = Surface::ConfirmDraft;
                    return false;
                }
                s.discardDraft();   // a clean bracket: nothing to ask, nothing to lose
                draftBaseline_.reset();
                surface_ = Surface::Browse;
                return true;
            case Surface::Review:
                surface_ = reviewReturn_;
                reviewReturn_ = Surface::Browse;
                return false;
            case Surface::Actions:
                surface_ = Surface::Browse;
                return false;
            case Surface::Legality:
                surface_ = Surface::Actions;
                return false;
            case Surface::ConfirmDraft:
                continueEditing();
                return false;
            case Surface::ConfirmDiscardAll:
            case Surface::ConfirmExport:
                surface_ = Surface::Review;
                return false;
            case Surface::ConfirmExit:
                surface_ = Surface::Browse;
                return false;
            case Surface::Browse:
                break;
        }
        return false;
    }
    void continueEditing() noexcept {
        if (surface_ == Surface::ConfirmDraft) surface_ = Surface::Edit;
    }
    template <class Refresh>
    [[nodiscard]] bool discardDraft(NativeSession& s, Refresh&& refresh, std::string& error) {
        if (surface_ != Surface::Edit && surface_ != Surface::ConfirmDraft && surface_ != Surface::View) {
            error = "No draft is open";
            return false;
        }
        s.discardDraft();
        draftBaseline_.reset();
        surface_ = Surface::Browse;
        if (!refresh(error)) {
            if (error.empty()) error = std::string(s.familyLabel()) + " presentation could not be refreshed after Discard";
            return false;
        }
        error.clear();
        return true;
    }

    // ---- exit / export / discard all ----
    [[nodiscard]] bool requestExit(const NativeSession& s) noexcept {
        if (surface_ != Surface::Browse) return false;
        if (s.hasChanges()) {
            surface_ = Surface::ConfirmExit;
            return false;
        }
        return true;
    }
    // Export is a separate confirmation over the Review; A cannot publish and a failed export keeps
    // the confirmation open until the user cancels or retries with Y.
    [[nodiscard]] bool requestExport(const NativeSession& s) noexcept {
        if (surface_ != Surface::Review || !s.hasChanges() || !exportAvailable(s)) return false;
        surface_ = Surface::ConfirmExport;
        return true;
    }
    void finishExport() noexcept {
        if (surface_ == Surface::ConfirmExport) surface_ = Surface::Review;
    }
    void requestDiscardAll() noexcept {
        if (surface_ == Surface::Review) surface_ = Surface::ConfirmDiscardAll;
    }
    template <class Refresh>
    [[nodiscard]] bool confirmDiscardAll(NativeSession& s, Refresh&& refresh, std::string& error) {
        if (surface_ != Surface::ConfirmDiscardAll) {
            error = "Discard All requires an explicit confirmation";
            return false;
        }
        // Discard All removes what a workspace Review was editing too: never return into it.
        if (s.draftActive()) s.discardDraft();
        draftBaseline_.reset();
        s.discardAll();
        reviewReturn_ = Surface::Browse;
        surface_ = Surface::Browse;
        if (!refresh(error)) {
            if (error.empty()) error = std::string(s.familyLabel()) + " presentation could not be refreshed after Discard All";
            return false;
        }
        error.clear();
        return true;
    }
    [[nodiscard]] bool confirmDiscardAndExit(NativeSession& s) noexcept {
        if (surface_ != Surface::ConfirmExit) return false;
        if (s.draftActive()) s.discardDraft();
        draftBaseline_.reset();
        s.discardAll();
        surface_ = Surface::Browse;
        return true;
    }
    void reviewFromExit(const NativeSession& s) noexcept {
        if (surface_ == Surface::ConfirmExit && s.hasChanges()) {
            reviewPage_ = 0;
            reviewReturn_ = Surface::Browse;
            surface_ = Surface::Review;
        }
    }

    [[nodiscard]] static std::string slotLabel(SlotRef slot) {
        if (slot.region == Region::Party) return "Party / Slot " + std::to_string(slot.slot + 1);
        return "Box " + std::to_string(slot.box + 1) + " / Slot " + std::to_string(slot.slot + 1);
    }

private:
    Surface surface_ = Surface::Browse;
    SlotRef target_{};
    int actionRow_ = 0;
    size_t reviewPage_ = 0;
    Surface reviewReturn_ = Surface::Browse;
    std::optional<SlotRef> cloneDestination_;
    bool moveDestination_ = false;
    std::optional<RecordView> draftBaseline_;
};

} // namespace PokeVault::Integration::Modern

#endif
