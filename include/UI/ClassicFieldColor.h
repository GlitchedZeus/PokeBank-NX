#pragma once
// Gen I / Gen II value colour on the ONE field-state contract: the drawn colour of a field value
// in the given state (UI/SharedFieldState.h through UI/SharedEditorChrome.h). Editor surfaces call
// this instead of drawing a raw token, so a value is gold only when it is locked (QA CG16).
#include "UI/ClassicStatFieldState.h"
#include "UI/SharedEditorChrome.h"

namespace UI::ClassicFieldColor {

inline Color valueColor(PokeBank::UIModel::SharedFieldState::State state, bool focused) {
    return SharedEditorChrome::fieldColor(SharedEditorChrome::FieldState::style(state, focused).value);
}

// Shiny Yes / No: an Editable inline toggle, so never gold (ClassicStatFields::kShinyState).
inline Color shinyValueColor(bool focused) {
    return valueColor(PokeBank::UIModel::ClassicStatFields::kShinyState, focused);
}

// Split Special Attack / Special Defense display: calculated, ReadOnly gold.
inline Color splitSpecialColor() {
    return valueColor(PokeBank::UIModel::ClassicStatFields::kSplitSpecialState, false);
}

} // namespace UI::ClassicFieldColor
