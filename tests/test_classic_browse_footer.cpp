// Round 6 E05: the Gen I-IV Game Browser browse footer has one fixed order, the same as Gen V:
// D-pad/Stick, A, L/R, ZL/ZR, [Review], B, -. Owner Round 5 (1ca2faec): Party footers were
// inconsistent ("Arrows: Navigate | B: Back | A: View / Actions"). The Review slot is off by
// default; the lane that handles a Review key on these pages turns it on in this one model.
#include "UI/ClassicBrowseFooter.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace F = PokeBank::UIModel::ClassicBrowseFooter;

namespace {
std::string glyphs(const F::Spec& spec) {
    std::string out;
    for (const auto& h : F::hints(spec)) out += (out.empty() ? "" : ",") + h.glyph;
    return out;
}
std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

int main() {
    F::Spec party;
    party.canOpen = true;
    assert(glyphs(party) == "D-pad/Stick,A,B,-");                     // Party / hub with a Pokemon
    assert(F::instructions(party) == "D-pad/Stick: Navigate  |  A: View / Actions  |  B: Back  |  -: Help");
    F::Spec emptySlot;
    assert(glyphs(emptySlot) == "D-pad/Stick,B,-");                   // no A where nothing opens
    F::Spec boxes;
    boxes.canOpen = true;
    boxes.boxes = true;
    assert(glyphs(boxes) == "D-pad/Stick,A,L/R,ZL/ZR,B,-");           // same order as Gen V Boxes
    // Review slot: off by default, and when turned on it sits before B, as Gen V's "+ Review".
    assert(!F::Spec{}.review);
    boxes.review = true;
    assert(glyphs(boxes) == "D-pad/Stick,A,L/R,ZL/ZR,Y,B,-");
    boxes.reviewGlyph = "";
    assert(glyphs(boxes) == "D-pad/Stick,A,L/R,ZL/ZR,B,-");           // no key, no Review badge
    for (const auto& h : F::hints(boxes)) assert(!h.glyph.empty() && !h.label.empty());

    // The production browse block uses the model (the old B-before-A string is gone).
    const auto base = read("src/UI/TrainerViewScreenBase.inc");
    assert(base.find("instructions = BrowseFooter::instructions(spec);") != std::string::npos);
    assert(base.find("instructions = \"Arrows: Navigate  |  B: Back\";") == std::string::npos);
    assert(base.find("instructions += \"  |  A: View / Actions\";") == std::string::npos);
    std::cout << "Gen I-IV browse footer: D-pad, A, L/R, ZL/ZR, [Review], B, - (Gen V order) PASS\n";
    return 0;
}
