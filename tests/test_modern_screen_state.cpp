// The shared Gen VI-IX screen state machine (include/Integration/Modern/ModernScreenState.h) over a
// real adapter (Gen6NativeSession on the synthetic X save). Every transition the ONE shared surface
// relies on: Actions derived from adapter support + registry, View without a draft, Edit draft with
// Keep / Discard / Continue, Clone / Move destinations, Release, Review paging and return, Export and
// Discard All confirmations and the staged-exit decision. No switch.h, no UI, no save writes.
#include "Integration/Gen6/Gen6NativeSession.h"
#include "Integration/Modern/ModernScreenState.h"
#include "fixtures/gen6/Gen6SyntheticSave.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace G6 = PokeVault::Integration::Gen6;
namespace Modern = PokeVault::Integration::Modern;
using Modern::SlotRef;
using State = Modern::ModernScreenState;
using Surface = State::Surface;
using Action = State::Action;
using Field = Modern::Field;

namespace {
int failures = 0, checks = 0;
void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}
bool lists(const State::ActionSet& set, Action action) {
    for (size_t i = 0; i < set.count; ++i)
        if (set[i] == action) return true;
    return false;
}
std::unique_ptr<Modern::NativeSession> openFixture() {
    auto opened = G6::Gen6NativeSession::open(Gen6Fixture::makeSave({}), "x_3ds");
    if (!opened.ready()) {
        std::cerr << "fixture failed to open: " << opened.diagnostic << '\n';
        std::exit(EXIT_FAILURE);
    }
    return std::move(opened.session);
}
const auto accept = [](std::string&) { return true; };
const auto reject = [](std::string& why) { why = "presentation rejected (test)"; return false; };
} // namespace

int main() {
    auto session = openFixture();
    auto& s = *session;
    std::string error;

    // ---- available actions: adapter support answers AND the registry row ----
    {
        const auto party = State::actionsFor(s, SlotRef::party(0));
        check(lists(party, Action::View) && lists(party, Action::Edit) && lists(party, Action::LegalityProvenance) &&
                  lists(party, Action::Close), "party: View / Edit / Origin & Legality / Cancel");
        check(lists(party, Action::Clone), "party: Clone (adapter allows a party source)");
        check(!lists(party, Action::Move) && !lists(party, Action::Remove), "party: no Move / Release (adapter refuses party removal)");
        check(!lists(party, Action::Review), "no Review row without staged changes");
        const auto boxed = State::actionsFor(s, SlotRef::boxed(0, 0));
        check(lists(boxed, Action::Clone) && lists(boxed, Action::Move) && lists(boxed, Action::Remove), "box: Clone / Move / Release");
        const auto empty = State::actionsFor(s, SlotRef::boxed(5, 5));
        check(empty.count == 1 && empty[0] == Action::Close, "empty box slot: Cancel only, never Add (Create is Blocked)");
        check(!State::createRefusal(s, SlotRef::boxed(5, 5)).empty(), "empty slot names why Add is missing");
        check(State::createRefusal(s, SlotRef::boxed(0, 0)).empty(), "occupied slot has no Create refusal");
        check(!State::exportAvailable(s) && !State::exportRefusal(s).empty(), "export unavailable without changes, with reason");
    }

    // ---- Actions sheet ----
    State st;
    check(st.surface() == Surface::Browse, "starts in Browse");
    check(!st.openActions(s, SlotRef::boxed(40, 0), error) && !error.empty(), "non-canonical slot refused");
    check(st.openActions(s, SlotRef::boxed(5, 5), error) && st.surface() == Surface::Actions, "empty slot opens Actions");
    check(!st.activate(s, Action::Add, error) && !error.empty(), "Add is refused with a reason");
    check(!st.activate(s, Action::Edit, error), "Edit is not listed for an empty slot");
    st.moveAction(1, s);
    check(st.actionRow() == 0, "one row wraps onto itself");
    check(st.back(s) == false && st.surface() == Surface::Browse, "B closes Actions");

    // ---- View never begins a draft ----
    check(st.openActions(s, SlotRef::party(0), error), "party Actions");
    st.moveAction(3, s);
    st.moveAction(-3, s);
    check(st.selectedAction(s) == Action::View, "cursor wraps back to View");
    check(st.activateSelected(s, error) && st.surface() == Surface::View, "A on View opens View");
    check(!s.draftActive(), "View did not begin a draft");
    check(st.back(s) && st.surface() == Surface::Browse, "B leaves View");

    // ---- Legality panel ----
    check(st.openActions(s, SlotRef::boxed(0, 0), error) && st.activate(s, Action::LegalityProvenance, error) &&
              st.surface() == Surface::Legality, "Origin & Legality opens");
    check(!st.back(s) && st.surface() == Surface::Actions, "B from Legality returns to Actions");
    check(!st.back(s) && st.surface() == Surface::Browse, "B again closes Actions");

    // ---- Edit draft: Keep / Discard / Continue ----
    check(st.openActions(s, SlotRef::boxed(0, 0), error) && st.activate(s, Action::Edit, error), "Edit opens: " + error);
    check(st.surface() == Surface::Edit && s.draftActive() && s.draftSlot() == SlotRef::boxed(0, 0), "Edit began the adapter draft");
    check(st.draftBaseline() && st.draftBaseline()->friendship == 70, "draft baseline captured");
    check(st.back(s) && st.surface() == Surface::Browse && !s.draftActive(), "B on a clean draft closes it silently");
    check(st.openActions(s, SlotRef::boxed(0, 0), error) && st.activate(s, Action::Edit, error), "Edit again");
    check(s.stageValue(SlotRef::boxed(0, 0), Field::Friendship, 200, 0, error), "stage friendship: " + error);
    check(!st.back(s) && st.surface() == Surface::ConfirmDraft, "B on a dirty draft asks");
    st.continueEditing();
    check(st.surface() == Surface::Edit && s.draftDirty(), "Continue returns to Edit with the draft");
    check(!st.keep(s, reject, error) && st.surface() == Surface::Edit && s.draftActive(), "a rejected presentation keeps the draft open");
    check(!error.empty(), "rejected Keep names why");
    check(st.keep(s, accept, error) && st.surface() == Surface::Browse && !s.draftActive(), "Keep ends the bracket");
    check(s.hasChanges() && s.stagedAt(SlotRef::boxed(0, 0)) && s.view(SlotRef::boxed(0, 0))->friendship == 200, "kept edit is staged");
    check(st.openActions(s, SlotRef::boxed(0, 0), error) && st.activate(s, Action::Edit, error), "Edit over a kept edit");
    check(s.stageValue(SlotRef::boxed(0, 0), Field::Friendship, 150, 0, error) && !st.back(s), "dirty again");
    check(st.discardDraft(s, accept, error) && st.surface() == Surface::Browse, "Discard from the question");
    check(s.view(SlotRef::boxed(0, 0))->friendship == 200 && !s.draftActive(), "Discard restored the KEPT value");

    // ---- Review from Browse and from Edit ----
    check(st.openReviewFromBrowse(s) && st.surface() == Surface::Review && st.reviewPage() == 0, "Review opens from Browse");
    st.moveReviewPage(1, 9);
    check(st.reviewPage() == 1, "page forward");
    st.moveReviewPage(-1, 9);
    check(st.reviewPage() == 0, "page back");
    check(!st.back(s) && st.surface() == Surface::Browse, "B from Review returns to Browse");
    check(st.openActions(s, SlotRef::boxed(0, 0), error) && st.activate(s, Action::Edit, error), "Edit for a workspace Review");
    check(st.openReviewFrom(s) && st.reviewReturn() == Surface::Edit, "+ opens Review over the workspace");
    check(!st.back(s) && st.surface() == Surface::Edit && s.draftActive(), "B returns into the workspace with its draft");
    check(st.back(s) && st.surface() == Surface::Browse, "clean draft closes");

    // ---- Clone destination (keypad-chosen) ----
    check(st.openActions(s, SlotRef::boxed(0, 1), error) && st.activate(s, Action::Clone, error), "Clone: " + error);
    check(st.surface() == Surface::CloneDestination && !st.moveDestination(), "destination step for Clone");
    check(st.cloneDestination() == SlotRef::boxed(0, 3), "defaults to the first empty box slot");
    check(st.setCloneDestination(s, SlotRef::boxed(0, 0), error) && !st.cloneDestinationAllowed(s), "an occupied destination is not allowed");
    check(!st.confirmClone(s, accept, error) && !error.empty() && st.surface() == Surface::CloneDestination,
          "confirm refuses the occupied destination with a reason");
    check(!st.setCloneDestination(s, SlotRef::party(0), error), "party is never a destination");
    check(st.setCloneDestination(s, SlotRef::boxed(0, 3), error) && st.cloneDestinationAllowed(s), "empty destination accepted");
    check(!st.confirmClone(s, reject, error) && s.view(SlotRef::boxed(0, 3))->empty && !s.stagedAt(SlotRef::boxed(0, 3)),
          "a rejected presentation rolls the clone back");
    check(st.surface() == Surface::CloneDestination, "confirmation stays open after the rollback");
    check(st.confirmClone(s, accept, error) && st.surface() == Surface::Browse, "clone staged: " + error);
    check(s.view(SlotRef::boxed(0, 3))->species == 101, "clone landed in Box 1 / Slot 4");
    st.cancelBoxAction();
    check(st.surface() == Surface::Browse, "cancelBoxAction is inert outside the box steps");

    // ---- Move (box to box): the clone's source is still an unmodified original ----
    check(st.openActions(s, SlotRef::boxed(0, 1), error) && st.activate(s, Action::Move, error), "Move: " + error);
    check(st.moveDestination() && st.cloneDestination() == SlotRef::boxed(0, 4), "Move defaults to the next empty slot");
    check(st.setCloneDestination(s, SlotRef::boxed(1, 0), error) && st.confirmClone(s, accept, error), "move staged: " + error);
    check(s.view(SlotRef::boxed(1, 0))->species == 101 && s.view(SlotRef::boxed(0, 1))->empty, "move = clone + remove");
    check(st.openActions(s, SlotRef::party(1), error) && !st.activate(s, Action::Move, error) && !error.empty(),
          "party Move is not listed and is refused with a reason");
    check(st.surface() == Surface::Actions, "the refused action leaves Actions open");
    check(!st.back(s), "close Actions");

    // ---- Release (an unmodified original box record) ----
    check(st.openActions(s, SlotRef::boxed(0, 2), error) && st.activate(s, Action::Remove, error) &&
              st.surface() == Surface::ConfirmRemoveBox, "Release asks for Y");
    st.cancelBoxAction();
    check(st.surface() == Surface::Actions, "B cancels the Release question");
    check(st.activate(s, Action::Remove, error) && st.confirmRemoveBox(s, accept, error) && st.surface() == Surface::Browse, "Y releases");
    check(s.view(SlotRef::boxed(0, 2))->empty && s.stagedAt(SlotRef::boxed(0, 2)), "released slot reads empty in the staged copy");

    // ---- Export confirmation ----
    check(State::exportAvailable(s), "export available with staged changes");
    check(!st.requestExport(s), "Export needs the Review open");
    check(st.openReviewFromBrowse(s) && st.requestExport(s) && st.surface() == Surface::ConfirmExport, "X asks to export");
    check(!st.back(s) && st.surface() == Surface::Review, "B cancels the export question");
    check(st.requestExport(s), "ask again");
    st.finishExport();
    check(st.surface() == Surface::Review, "a finished export returns to Review");

    // ---- Discard All (from a workspace Review the draft goes too) ----
    check(!st.back(s) && st.openActions(s, SlotRef::boxed(0, 0), error) && st.activate(s, Action::Edit, error), "Edit");
    check(s.stageValue(SlotRef::boxed(0, 0), Field::Friendship, 10, 0, error) && st.openReviewFrom(s), "dirty draft + Review");
    st.requestDiscardAll();
    check(st.surface() == Surface::ConfirmDiscardAll, "Y asks to discard all");
    check(!st.back(s) && st.surface() == Surface::Review, "B keeps the changes");
    st.requestDiscardAll();
    check(st.confirmDiscardAll(s, accept, error) && st.surface() == Surface::Browse, "Discard All: " + error);
    check(!s.hasChanges() && !s.draftActive() && st.reviewReturn() == Surface::Browse, "everything discarded, no return into the workspace");
    check(s.view(SlotRef::boxed(0, 0))->friendship == 70 && !s.view(SlotRef::boxed(0, 2))->empty &&
              s.view(SlotRef::boxed(1, 0))->empty, "source view restored");

    // ---- staged exit ----
    check(st.requestExit(s), "no changes: exit at once");
    check(s.stageValue(SlotRef::boxed(0, 0), Field::HeldItem, 3, 0, error), "stage a change outside a draft");
    check(!st.requestExit(s) && st.surface() == Surface::ConfirmExit, "changes: exit asks");
    st.reviewFromExit(s);
    check(st.surface() == Surface::Review && st.reviewReturn() == Surface::Browse, "A reviews from the exit question");
    check(!st.back(s) && st.surface() == Surface::Browse, "B from that Review returns to Browse");
    check(!st.requestExit(s) && !st.back(s) && st.surface() == Surface::Browse, "B on the exit question continues editing");
    check(!st.requestExit(s) && st.confirmDiscardAndExit(s) && !s.hasChanges() && st.surface() == Surface::Browse,
          "Y discards and exits");
    check(!st.confirmDiscardAndExit(s), "confirmDiscardAndExit needs the question open");

    std::cout << "test_modern_screen_state: " << (failures ? "FAILED " : "all ") << checks << " checks"
              << (failures ? " (" + std::to_string(failures) + " failed)" : " passed") << '\n';
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
