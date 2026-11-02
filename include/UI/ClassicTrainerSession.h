#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

// The Trainer page (HOME Trainer card) for every generation: rows 0 Name and 1 Money.
//  - Footer: "A: Edit" is offered only when those fields really can be edited in this session
//    (a mutable workspace, or a classic staged Trainer editor); a read-only Trainer page never
//    shows an edit affordance (R2-TRAINER).
//  - Gen I and Gen II stage Name / Money through their staged editors (never the source save);
//    both use the same session flow below (UI::ClassicTrainerSession::handle).
namespace PokeBank::UIModel::ClassicTrainerSession {

inline constexpr int kRows = 2; // 0 Name, 1 Money
// P3 (Round 5): rows a classic staged Trainer session adds where the native writer exists:
// 2 Play Time, then Gender (row 3 with Play Time, else row 2). Gender is never added without a
// native gender byte (Gen I, Gold / Silver).
inline constexpr int kPlayTimeRow = 2;
constexpr int genderRow(bool playTime) noexcept { return playTime ? 3 : 2; }
constexpr int rowCount(bool playTime, bool gender) noexcept {
    return kRows + (playTime ? 1 : 0) + (gender ? 1 : 0);
}
inline constexpr int kReviewTouchId = 61240;

// Round 6 (B08): Y Review whether or not the focused row is editable; on a page without the
// staged Trainer session the Gen I-IV grid router (ClassicBrowseReview.h) handles the key.
// Round 14 R08: this is the drawn footer, exactly (the old "+: Exit App" tail was always erased by the
// HOME footer pass and "-: Help" appended); trainerFooterHints below is the same list as entries.
constexpr const char* trainerPageInstructions(bool editable) noexcept {
    return editable ? "Up/Down: Select  |  A: Edit  |  Y: Review  |  B: Back  |  -: Help"
                    : "Up/Down: Select  |  Y: Review  |  B: Back  |  -: Help";
}

// Round 14 R08 (owner: "gen 5 has almost none of the proper footers and im guessing controls"): the
// ONE Trainer page footer model every generation draws (Gen I-IV through trainerPageInstructions, Gen V
// through Gen5TrainerScreen::footer). `aLabel` names what A does on the focused row ("Edit"; Gen V's
// Pokedex row says "Open Pokedex"); nullptr when the focused row cannot act on A.
struct TrainerFooterHint {
    const char* glyph;
    const char* label;
};
struct TrainerFooter {
    TrainerFooterHint hints[5]{};
    std::size_t count = 0;
    constexpr void push(const char* glyph, const char* label) noexcept { hints[count++] = {glyph, label}; }
    constexpr const TrainerFooterHint* begin() const noexcept { return hints; }
    constexpr const TrainerFooterHint* end() const noexcept { return hints + count; }
};
constexpr TrainerFooter trainerFooterHints(const char* aLabel) noexcept {
    TrainerFooter f;
    f.push("Up/Down", "Select");
    if (aLabel) f.push("A", aLabel);
    f.push("Y", "Review");
    f.push("B", "Back");
    f.push("-", "Help");
    return f;
}

// R2-BUG-001: on an editable Trainer page "A: Edit" is offered for the FOCUSED row only: Money
// always, Name only when this save's name can be staged (a Japanese Gen I / III / IV name stays
// read-only while Money still stages).
constexpr bool focusedRowEditable(bool nameEditable, int focusedRow) noexcept {
    return focusedRow != 0 || nameEditable;
}

// R2-BUG-001: an emulator source is read-only, yet its Trainer page is editable through a staged
// Trainer editor. The generic read-only footer override ("Arrows: Navigate | B: Back") ran after
// the Trainer footer and replaced it on the same frame (hardware: no "A: Edit" on Gen I). The
// override treats the source as read-only for the footer EXCEPT on an editable Trainer page.
constexpr bool readOnlyForFooterOverride(bool sourceReadOnly, bool trainerPage,
                                         bool trainerFieldsEditable) noexcept {
    return sourceReadOnly && !(trainerPage && trainerFieldsEditable);
}

} // namespace PokeBank::UIModel::ClassicTrainerSession
