#pragma once

// One presentation contract for the Gen I-V Pokemon editor. Everything here is
// framebuffer-free and constexpr where practical, so the generation adapters (and
// host tests) share the SAME strings, footers, toggle rules and layout bounds.
//
//   * Origin & Legality heading + honest legality status labels
//   * Picker footer: browse/page/jump hints, optional X/Y, always A Select / B Cancel
//   * Modal footer: a modal names only the buttons it handles
//   * Inline toggles: Shiny Yes/No, Pokerus None/Infected/Cured, Gender, Language
//   * Left DETAILS viewport bounds (no "Rows N-M/T" counter; rows never under the footer)
//
// The drawing side lives in UI/SharedEditorChrome.h (footers, heading) and
// UI/SharedMovesPanel.h (moves panel and compact move chooser).

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace PokeBank::UIModel::SharedEditorPresentation {

// ---------------------------------------------------------------------------------------------
// Origin & Legality
// ---------------------------------------------------------------------------------------------

// The single heading every generation uses for the provenance block and its action/modal.
inline constexpr const char* kOriginLegalityHeading = "Origin & Legality";
// Action-sheet row that opens the same block.
inline constexpr const char* kOriginLegalityAction = "Origin & Legality";

// What the app actually knows. Only Verified may ever read as a positive result, and
// nothing in Gen III-V currently produces Verified: no full legality checker exists.
enum class LegalityState : uint8_t {
    Unknown,       // nothing was evaluated
    Incomplete,    // some guardrails ran (structure, moves, encounter table) but not a full check
    NotChecked,    // a specific check exists but was not run for this record
    Verified,      // a full checker ran and found nothing (reserved; never fabricated)
    ProblemsFound, // a check ran and found an issue
    Invalid,       // proven invalid native data; never inferred from missing coverage
};

constexpr const char* legalityLabel(LegalityState state) noexcept {
    switch (state) {
        case LegalityState::Unknown: return "Unknown";
        case LegalityState::Incomplete: return "Incomplete";
        case LegalityState::NotChecked: return "Not checked";
        case LegalityState::Verified: return "No problems found";
        case LegalityState::ProblemsFound: return "Problems found";
        case LegalityState::Invalid: return "Invalid";
    }
    return "Unknown";
}

// Copy that claims a positive verdict. Tests scan the Gen III-V surfaces for these so an
// unverified record can never be called legal.
inline constexpr std::array<std::string_view, 4> kUnverifiedForbiddenClaims{{
    "No problems found", "Legal", "Legality: OK", "Legality: PASS",
}};

// ---------------------------------------------------------------------------------------------
// Footers
// ---------------------------------------------------------------------------------------------

// Glyph tokens are the ones ScreenChrome::buttonGlyph() already badges: "A" is the green
// face button and "B" the red one, so every footer below inherits the app's existing styling.
struct Hint {
    const char* glyph = "";
    const char* label = "";
};

struct Footer {
    std::array<Hint, 8> hints{};
    std::size_t count = 0;

    constexpr void push(const char* glyph, const char* label) noexcept {
        if (count < hints.size()) hints[count++] = {glyph, label};
    }
    constexpr const Hint* begin() const noexcept { return hints.data(); }
    constexpr const Hint* end() const noexcept { return hints.data() + count; }
    constexpr bool has(std::string_view glyph, std::string_view label) const noexcept {
        for (std::size_t i = 0; i < count; ++i)
            if (glyph == hints[i].glyph && label == hints[i].label) return true;
        return false;
    }
};

inline constexpr const char* kSelectLabel = "Select";
inline constexpr const char* kCancelLabel = "Cancel";

struct PickerFooterSpec {
    bool browse = true;          // D-pad/Stick: Browse
    bool page = true;            // Left/Right (or L/R on a grid): Page
    bool gridPaging = false;     // grid pickers page with L/R instead of Left/Right
    bool jump = false;           // ZL/ZR: Jump
    const char* xLabel = nullptr; // e.g. "Search" or "Sort"
    const char* yLabel = nullptr; // e.g. "Normal/Shiny"
    const char* minusLabel = nullptr; // NEW-06: e.g. "Search" (Held Item pickers, where X is Sort)
};

// Every picker (species, move, held item, location, ability, ball, nature, form, language)
// ends with exactly A Select then B Cancel. No developer copy ("commits to draft") appears.
constexpr Footer pickerFooter(const PickerFooterSpec& spec = {}) noexcept {
    Footer f;
    if (spec.browse) f.push("D-pad/Stick", "Browse");
    if (spec.page) f.push(spec.gridPaging ? "L/R" : "Left/Right", "Page");
    if (spec.jump) f.push("ZL/ZR", "Jump");
    if (spec.xLabel) f.push("X", spec.xLabel);
    if (spec.yLabel) f.push("Y", spec.yLabel);
    if (spec.minusLabel) f.push("-", spec.minusLabel);
    f.push("A", kSelectLabel);
    f.push("B", kCancelLabel);
    return f;
}

// A confirmation modal: B backs out first, the destructive/committing action last.
constexpr Footer confirmFooter(const char* confirmGlyph, const char* confirmLabel,
                               const char* extraGlyph = nullptr,
                               const char* extraLabel = nullptr) noexcept {
    Footer f;
    f.push("B", "Back");
    if (extraGlyph && extraLabel) f.push(extraGlyph, extraLabel);
    f.push(confirmGlyph, confirmLabel);
    return f;
}

// A destructive confirm (Release, Remove, Discard): only Y commits. A and B both back out, and
// a frame with B held alongside Y cancels, so a stray press can never destroy anything.
enum class DestructiveConfirm : uint8_t { None, Commit, Cancel };
constexpr DestructiveConfirm destructiveConfirmStep(bool y, bool a, bool b) noexcept {
    if (a || b) return DestructiveConfirm::Cancel;
    return y ? DestructiveConfirm::Commit : DestructiveConfirm::None;
}

// ---------------------------------------------------------------------------------------------
// Inline toggles: A cycles a small value set in place, no popup.
// ---------------------------------------------------------------------------------------------

// Next value after `current` in `values`, wrapping. A current value outside the set (a
// preserved/unusual native value) advances to the first supported value.
template <std::size_t N>
constexpr uint8_t cycleNext(const std::array<uint8_t, N>& values, std::size_t count,
                            uint8_t current) noexcept {
    if (count == 0) return current;
    if (count > N) count = N;
    for (std::size_t i = 0; i < count; ++i)
        if (values[i] == current) return values[(i + 1) % count];
    return values[0];
}

constexpr const char* yesNo(bool value) noexcept { return value ? "Yes" : "No"; }

// Shiny is a row of its own in every generation: "Shiny: Yes/No". A flips it; the native
// adapter applies its PID rule (Gen III-V PID/TID/SID xor < 8) or reports why it cannot.
constexpr bool nextShiny(bool current) noexcept { return !current; }

// Pokerus byte shared by Gen III (PK3 0x44), Gen IV (PK4 0x82) and Gen V (PK5 0x82):
// high nibble = strain, low nibble = days remaining; strain 0 with days 0 = never infected,
// strain != 0 with days 0 = cured. pokeemerald src/pokemon.c (MON_DATA_POKERUS,
// UpdatePartyPokerusTime) and PKHeX PKX Pokerus (days max = (strain & 3) + 1).
enum class PokerusState : uint8_t { None, Infected, Cured };

constexpr uint8_t pokerusStrain(uint8_t raw) noexcept { return static_cast<uint8_t>(raw >> 4); }
constexpr uint8_t pokerusDays(uint8_t raw) noexcept { return static_cast<uint8_t>(raw & 0x0F); }
constexpr uint8_t pokerusMaxDays(uint8_t strain) noexcept {
    return static_cast<uint8_t>((strain & 3) + 1);
}

constexpr PokerusState pokerusState(uint8_t raw) noexcept {
    if (pokerusDays(raw) != 0) return PokerusState::Infected;
    if (pokerusStrain(raw) != 0) return PokerusState::Cured;
    return PokerusState::None;
}

constexpr const char* pokerusLabel(PokerusState state) noexcept {
    switch (state) {
        case PokerusState::None: return "None";
        case PokerusState::Infected: return "Infected";
        case PokerusState::Cured: return "Cured";
    }
    return "None";
}

// A valid byte has a non-zero strain whenever days are set and days within the strain's range.
constexpr bool pokerusByteValid(uint8_t raw) noexcept {
    if (raw == 0) return true;
    const uint8_t strain = pokerusStrain(raw), days = pokerusDays(raw);
    return strain != 0 && days <= pokerusMaxDays(strain);
}

// The next state's byte. Cycling back to the untouched state restores `baseline` exactly, so
// a user who did not mean to change Pokerus leaves the native byte alone. New infections keep
// the baseline strain when it has one, otherwise strain 1; days start at 1 (always in range).
constexpr uint8_t nextPokerusByte(uint8_t current, uint8_t baseline) noexcept {
    const PokerusState next = pokerusState(current) == PokerusState::None ? PokerusState::Infected
        : pokerusState(current) == PokerusState::Infected ? PokerusState::Cured
        : PokerusState::None;
    if (pokerusState(baseline) == next && pokerusByteValid(baseline)) return baseline;
    uint8_t strain = pokerusStrain(current) ? pokerusStrain(current) : pokerusStrain(baseline);
    if (strain == 0) strain = 1;
    switch (next) {
        case PokerusState::None: return 0;
        case PokerusState::Infected: return static_cast<uint8_t>((strain << 4) | 1);
        case PokerusState::Cured: return static_cast<uint8_t>(strain << 4);
    }
    return current;
}

// Gender: 0 Male, 1 Female, 2 Genderless. Personal-table ratio encoding is identical in
// Gen III-V: 0 male-only, 254 female-only, 255 genderless, otherwise variable.
struct GenderChoices {
    std::array<uint8_t, 2> values{};
    std::size_t count = 0;
};

constexpr GenderChoices genderChoices(uint8_t ratio) noexcept {
    if (ratio == 255) return {{2, 2}, 1};
    if (ratio == 254) return {{1, 1}, 1};
    if (ratio == 0) return {{0, 0}, 1};
    return {{0, 1}, 2};
}

constexpr bool genderSelectable(uint8_t ratio) noexcept { return genderChoices(ratio).count > 1; }

constexpr uint8_t nextGender(uint8_t ratio, uint8_t current) noexcept {
    const auto choices = genderChoices(ratio);
    return cycleNext(choices.values, choices.count, current);
}

constexpr const char* genderLabel(uint8_t gender) noexcept {
    return gender == 0 ? "Male" : gender == 1 ? "Female" : "Genderless";
}

// Gen III language ids (PKHeX LanguageID / pokeemerald LANGUAGE_*): 1 Japanese, 2 English,
// 3 French, 4 Italian, 5 German, 7 Spanish. 6 is unused and never offered.
inline constexpr std::array<uint8_t, 6> kGen3Languages{{1, 2, 3, 4, 5, 7}};

constexpr uint8_t nextGen3Language(uint8_t current) noexcept {
    return cycleNext(kGen3Languages, kGen3Languages.size(), current);
}

// ---------------------------------------------------------------------------------------------
// Left DETAILS viewport (shared shell geometry: panel at y=96, 1280x720, 56px footer).
// ---------------------------------------------------------------------------------------------

struct DetailsViewport {
    int top = 0;        // clip top
    int height = 0;     // clip height
    int firstRowY = 0;  // y of the first visible row
    int rowStep = 38;
    int rowHeight = 30; // focus outline height
    int visibleRows = 0;
    constexpr int bottom() const noexcept { return top + height; }
};

// S03: ONE label/value column for every generation's DETAILS rows (300 px left panel).
inline constexpr int kDetailLabelX = 16;
inline constexpr int kDetailValueInset = 124;
inline constexpr int kDetailLabelWidth = kDetailValueInset - kDetailLabelX - 12;   // 96 px
inline constexpr int kDetailValueRightPad = 28;
inline constexpr int kDetailsPanelWidth = 300;
constexpr int detailValueWidth(int panelW) noexcept {
    return panelW - kDetailValueInset - kDetailValueRightPad;
}
static_assert(detailValueWidth(kDetailsPanelWidth) == 148, "DETAILS values have 148 px");

inline constexpr int kDetailsRowStep = 38;
inline constexpr int kDetailsHeaderHeight = 216; // portrait + type badges above the rows
inline constexpr int kDetailsRowInset = 8;

// Visible rows fill the panel below the portrait, stopping 8px above the panel's bottom edge
// (which itself sits above the footer). No row counter line is reserved any more.
constexpr DetailsViewport detailsViewport(int panelY, int panelH) noexcept {
    DetailsViewport v;
    v.top = panelY + kDetailsHeaderHeight;
    const int available = panelY + panelH - 8 - v.top;
    v.visibleRows = available > kDetailsRowInset
        ? (available - kDetailsRowInset - (kDetailsRowStep - 32)) / kDetailsRowStep : 0;
    if (v.visibleRows < 1) v.visibleRows = 1;
    v.height = kDetailsRowInset + v.visibleRows * kDetailsRowStep;
    v.firstRowY = v.top + kDetailsRowInset;
    return v;
}

} // namespace PokeBank::UIModel::SharedEditorPresentation
