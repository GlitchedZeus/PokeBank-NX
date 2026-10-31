#pragma once

// Round 6 (B08 / P1-RETURN): the shared Review straight from Gen I-IV Party and Trainer (Y or +)
// and Boxes (+; Y stays Move / Multi), per ReviewEntry.h. The Trainer page's own staged session
// opens the same Review; this also covers a Trainer page whose rows cannot be edited. Only on the
// plain page: the caller passes `gridIdle` = no generation overlay, Party Actions sheet or
// carry/selection owns the frame; the
// base modals are checked here. Screen::openStagedReview marks the Review standalone
// (ClassicReviewOrigin), so its B (and a closing Discard All) returns to this screen with the
// cursor, box and page as they were: nothing in Review touches them.
// A template over the screen so the same code runs on the console and in host tests
// (tests/test_review_entry.cpp).

#include "UI/ReviewEntry.h"

#include <cstdint>

namespace PokeBank::UIModel::ClassicBrowseReview {

template <class Screen>
[[nodiscard]] bool baseModalOpen(const Screen& screen) {
    return screen.carrying() || screen.currentlySelecting || screen.helpOverlayActive || screen.details.active ||
           screen.actionSheet.isOpen() || screen.saveConfirmActive || screen.pickerActive ||
           screen.itemEditDialogActive || screen.releaseConfirmActive || screen.groupMenuActive ||
           screen.storageExitConfirmActive;
}

// True when it consumed the frame (Review opened, or "nothing to review" was shown).
template <class Screen>
bool open(Screen& screen, std::uint64_t down, int generation, bool gridIdle) {
    namespace Entry = PokeBank::UIModel::ReviewEntry;
    if (generation < 1 || generation > 4 || !screen.detailViewActive) return false;
    const bool party = screen.selectedMode == Screen::ViewMode::Party;
    const bool boxes = screen.selectedMode == Screen::ViewMode::Boxes;
    const bool trainer = screen.selectedMode == Screen::ViewMode::Trainer;
    if (!party && !boxes && !trainer) return false;
    const Entry::Screen where = party ? Entry::Screen::Party : boxes ? Entry::Screen::Boxes : Entry::Screen::Trainer;
    if (!Entry::opensReview(where, generation, down)) return false;
    if (!gridIdle || baseModalOpen(screen)) return false;
    if (!screen.openStagedReview(false)) screen.postStatus("No staged changes to review for this save", 220);
    return true;
}

} // namespace PokeBank::UIModel::ClassicBrowseReview
