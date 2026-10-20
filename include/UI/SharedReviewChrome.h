#pragma once

// SharedReviewChrome.h - AUD-01 (Round 3 hardware): the chrome every Gen III-V "Review Pending
// Changes" screen shares, so Gen V matches Gen IV: the same title, the same source-protection line,
// the same footer words (the owner's "Export Copy" / "Discard All Staged") and the same Discard All
// confirmation. Each generation keeps its own change rows (Gen V's verified per-record rows were
// owner-accepted in S04). Input is unchanged: X exports, Y asks, B backs out; in the confirm only Y
// discards.

#include "UI/ScreenChrome.h"

#include <string>
#include <vector>

namespace UI::SharedReviewChrome {

inline constexpr const char* kTitle = "Review Pending Changes";
inline constexpr const char* kExportLabel = "Export Copy";
inline constexpr const char* kDiscardLabel = "Discard All Staged";

inline std::string sourceLine(const std::string& provider) {
    return "Original " + provider + " source: HARD READ ONLY";
}

// The Review footer. `paged`: the generation pages its rows with L/R.
inline std::vector<ControllerHint> footer(bool paged) {
    std::vector<ControllerHint> hints;
    if (paged) hints.push_back({"L/R", "Review Pages"});
    hints.push_back({"X", kExportLabel});
    hints.push_back({"Y", kDiscardLabel});
    hints.push_back({"B", "Back"});
    return hints;
}

// The Discard All confirmation every Review shows (each surface draws it in its own file, with its
// own touch layer and footer): same title, wording and B Keep Changes / Y Discard All choices.
inline constexpr const char* kDiscardTitle = "Discard all staged changes?";
inline constexpr const char* kDiscardKeepNote = "Export Copy first to keep them. The original save is never written.";
inline std::string discardBody(const char* generation) {
    return std::string("Every pending ") + generation + " edit in this workspace is lost.";
}

} // namespace UI::SharedReviewChrome
