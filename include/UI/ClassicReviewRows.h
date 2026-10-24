#pragma once

#include "UI/SharedReviewPresentation.h"

#include <string>

// Round 5: Gen I / II / III Review use the ONE shared paged Review presenter that Gen IV and V
// use (SharedEditorChrome::drawReviewPanel, SharedReview paging and footer, the shared Discard All
// confirmation). This turns a staged editor's (label, before, after) change into the same row
// shape Gen IV's describeStagedReviewChanges produces: "<where> / <field>" over "<from> -> <to>".
// The values are the editors' own semantic strings (species, move and item names); nothing is
// re-derived here.
namespace PokeBank::UIModel::ClassicReview {

inline SharedReview::Row row(std::string label, const std::string& before, const std::string& after) {
    // Editors write "Box 1 Slot 2: Move 1" / "Party 1: Species"; the shared rows use " / ".
    for (std::size_t at = label.find(": "); at != std::string::npos; at = label.find(": ", at + 3))
        label.replace(at, 2, " / ");
    return {std::move(label), before + " -> " + after};
}

template <class Changes>
SharedReview::Rows rows(const Changes& changes) {
    SharedReview::Rows out;
    out.reserve(changes.size());
    for (const auto& change : changes) out.push_back(row(change.label, change.beforeValue, change.afterValue));
    return out;
}

// Review input shared by the classic surfaces, exactly Gen IV's: L/R (or the D-pad) pages,
// X exports, Y asks and a second Y discards, B leaves (B cancels the question first).
enum class Input { None, PagePrevious, PageNext, Export, AskDiscard, Discard, CancelDiscard, Back };

constexpr Input input(bool confirming, bool previous, bool next, bool x, bool y, bool b) noexcept {
    if (confirming) {
        if (b) return Input::CancelDiscard;
        return y ? Input::Discard : Input::None; // A / X / paging are inert while asking
    }
    if (b) return Input::Back;
    if (previous) return Input::PagePrevious;
    if (next) return Input::PageNext;
    if (x) return Input::Export;
    if (y) return Input::AskDiscard;
    return Input::None;
}

} // namespace PokeBank::UIModel::ClassicReview
