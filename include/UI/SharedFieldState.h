#pragma once

// SharedFieldState.h - the ONE styling contract for an editor field, for every Gen I-V editor.
//
//   Editable   normal text (A opens its control).
//   ReadOnly   value is shown but cannot be changed here (stored-but-locked or derived from other
//              data, e.g. PID-linked traits, Gen V language): the established gold token
//              (Colors::ShinyStar). Gold means "locked", so gold never implies editability.
//   Disabled   the format/game has no such value or it is unsupported here: muted label and value,
//              so it reads differently from both of the above.
//
// The focus outline (the accepted 2 px FocusBorder rounded rect) is ALWAYS drawn on the focused
// field, whatever its state, so the cursor is never lost on a locked or unsupported row. Only an
// Editable field switches to SelectedText when focused; a ReadOnly / Disabled field keeps its own
// colour inside the outline, so focusing a gold value never makes it look like an editable one.
// A on a non-editable field never edits; the caller posts reason(state) instead.
// Note: Colors::TextDim is an alias of Colors::TextMuted, so Disabled is told apart from Editable
// by its VALUE colour (muted instead of Text) and from ReadOnly by not being gold.
//
// Pure model (no framebuffer): colours are tokens; UI::SharedEditorChrome maps them to Colors::*
// and draws them (fieldColor / drawFieldFocus). Lane C applies the same contract to Gen I/II.

#include "UI/SharedPokemonEditorContract.h"

#include <cstdint>

namespace PokeBank::UIModel::SharedFieldState {

enum class State : uint8_t { Editable, ReadOnly, Disabled };

enum class Token : uint8_t {
    Text,          // Colors::Text
    TextDim,       // Colors::TextDim (labels)
    TextMuted,     // Colors::TextMuted (disabled)
    Gold,          // Colors::ShinyStar (read-only / derived values)
    Selected,      // Colors::SelectedText (a focused Editable field)
    FocusBorder,   // Colors::FocusBorder (focus outline, every state)
};

struct Style {
    Token label = Token::TextDim;
    Token value = Token::Text;
    bool focusOutline = false;  // outline around the focused field
    Token outline = Token::FocusBorder;
    bool activates = false;     // A opens an editor control
};

constexpr Style style(State state, bool focused) noexcept {
    Style s;
    s.focusOutline = focused;
    switch (state) {
        case State::Editable:
            s.label = focused ? Token::Selected : Token::TextDim;
            s.value = focused ? Token::Selected : Token::Text;
            s.activates = true;
            break;
        case State::ReadOnly:
            s.label = Token::TextDim;
            s.value = Token::Gold;
            break;
        case State::Disabled:
            s.label = Token::TextMuted;
            s.value = Token::TextMuted;
            break;
    }
    return s;
}

// The shared editor contract's per-field access, as a field state. Hidden fields are not drawn.
constexpr State fromAccess(SharedPokemonEditor::FieldAccess access) noexcept {
    using A = SharedPokemonEditor::FieldAccess;
    switch (access) {
        case A::Editable: return State::Editable;
        case A::Derived:
        case A::ReadOnly: return State::ReadOnly;
        case A::Hidden: return State::Disabled;
    }
    return State::Disabled;
}

// Status line for A on a field that does not edit.
constexpr const char* reason(State state) noexcept {
    switch (state) {
        case State::ReadOnly: return "Read-only here: this value is locked or derived";
        case State::Disabled: return "Not available in this game";
        case State::Editable: return "";
    }
    return "";
}

// Contract checks: gold only ever marks a non-editable field; every focused field is outlined;
// a focused locked value keeps its own colour; the three states are visually distinct.
namespace detail {
constexpr bool contractHolds() noexcept {
    for (State st : {State::Editable, State::ReadOnly, State::Disabled})
        for (bool focused : {false, true}) {
            const Style s = style(st, focused);
            if ((s.value == Token::Gold || s.label == Token::Gold) && s.activates) return false;
            if (focused && !s.focusOutline) return false;
            if (focused && st != State::Editable && s.value == Token::Selected) return false;
            if (s.activates != (st == State::Editable)) return false;
        }
    const Style e = style(State::Editable, false), r = style(State::ReadOnly, false),
                d = style(State::Disabled, false);
    return e.value != r.value && r.value != d.value && e.value != d.value;
}
} // namespace detail
static_assert(detail::contractHolds(), "field-state styling contract");

} // namespace PokeBank::UIModel::SharedFieldState
