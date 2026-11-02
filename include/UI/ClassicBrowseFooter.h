#pragma once

#include <string>
#include <vector>

// Round 6 E05: the Gen I-IV Game Browser browse footer (save hub, Party, Boxes, Storage and the
// read-only pages, and the Gen I-III Boxes grid repaint in ClassicPackedMoveOverlay) in one fixed order, the same as Gen V (Gen5SharedReadOnlySurface: D-pad, A,
// L/R, ZL/ZR, Review, B): D-pad/Stick, A, L/R, ZL/ZR, [Review], B, -. Owner Round 5 (1ca2faec)
// found Party footers inconsistent: the old string put B before A and appended A afterwards.
//
// Each hint is shown only when its handler acts on this page (TrainerViewScreenBase browse
// input): A opens the focused Pokemon's actions or the hub panel, L/R and ZL/ZR change box,
// B goes back and - opens Help. The Review slot is OFF by default; the lane that wires a real
// Review key on these pages turns it on here (one place), naming the key its handler reads.
namespace PokeBank::UIModel::ClassicBrowseFooter {

struct Hint {
    std::string glyph;
    std::string label;
};

struct Spec {
    bool canOpen = false;        // A: the focused slot / hub panel opens
    std::string openLabel = "View / Actions";
    std::vector<Hint> actions;   // page actions after A (Gen I-IV Boxes: X Export / Import, Y Move / Hold Multi)
    bool boxes = false;          // L/R and ZL/ZR change box (Boxes, Storage)
    bool review = false;         // Review slot: off until a Review key is handled on this page
    std::string reviewGlyph = "Y";
};

inline std::vector<Hint> hints(const Spec& spec) {
    std::vector<Hint> out{{"D-pad/Stick", "Navigate"}};
    if (spec.canOpen) out.push_back({"A", spec.openLabel});
    for (const auto& action : spec.actions) out.push_back(action);
    if (spec.boxes) {
        out.push_back({"L/R", "Box"});
        out.push_back({"ZL/ZR", "Jump"});
    }
    if (spec.review && !spec.reviewGlyph.empty()) out.push_back({spec.reviewGlyph, "Review"});
    out.push_back({"B", "Back"});
    out.push_back({"-", "Help"});
    return out;
}

// The "K: label  |  K: label" form ScreenChrome::drawNavBar(const std::string&) draws as badges.
inline std::string serialize(const std::vector<Hint>& list) {
    std::string out;
    for (const auto& hint : list) {
        if (hint.glyph.empty() || hint.label.empty()) continue;
        if (!out.empty()) out += "  |  ";
        out += hint.glyph + ": " + hint.label;
    }
    return out;
}

inline std::string instructions(const Spec& spec) { return serialize(hints(spec)); }

} // namespace PokeBank::UIModel::ClassicBrowseFooter
