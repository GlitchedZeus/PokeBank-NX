#pragma once

// The ONE mapping from UI/SharedFieldState.h tokens to the theme colours (Colors::*), shared by
// the DETAILS rows (SharedPokemonShell) and every other field drawn through SharedEditorChrome.

#include "UI/Common.h"
#include "UI/SharedFieldState.h"

namespace UI::SharedFieldStateColors {

inline Color color(PokeBank::UIModel::SharedFieldState::Token token) {
    using T = PokeBank::UIModel::SharedFieldState::Token;
    switch (token) {
        case T::Text: return Colors::Text;
        case T::TextDim: return Colors::TextDim;
        case T::TextMuted: return Colors::TextMuted;
        case T::Gold: return Colors::ShinyStar;
        case T::Selected: return Colors::SelectedText;
        case T::FocusBorder: return Colors::FocusBorder;
    }
    return Colors::Text;
}

} // namespace UI::SharedFieldStateColors
