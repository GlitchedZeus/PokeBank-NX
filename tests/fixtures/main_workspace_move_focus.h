#pragma once
#include "UI/SharedPokemonEditorContract.h"
#include <cassert>

// Exercise the same outer navigation and render normalization on real session records,
// including populated slots (the hardware regression), Empty, and stale PP/Ups focus.
template<class Record>
void checkMainMoveFocus(PokeBank::UIModel::SharedPokemonEditor::Generation generation,
                        const Record& record, bool view, bool crystal = false) {
    namespace S = PokeBank::UIModel::SharedPokemonEditor;
    bool populated = false, empty = false;
    for (unsigned slot = 0; slot < 4; ++slot) {
        populated |= record.moves[slot] != 0;
        empty |= record.moves[slot] == 0;
        for (uint8_t stale = 0; stale < 3; ++stale) {
            S::Focus focus{S::Panel::Moves, static_cast<uint8_t>(slot), stale};
            auto visible = S::normalizeMoveRowFocus(generation, focus, crystal);
            assert(visible.column == 0 && visible.row == slot);
            const auto cell = S::cellFocusFor(generation, visible);
            assert(cell.x == 14 && cell.width == 170); // name, never PP or Ups
            for (int repeat = 0; repeat < 5; ++repeat) {
                focus = view ? S::passiveViewMoveColumn(generation, focus, 1, crystal)
                             : S::moveRowColumn(generation, focus, 1, crystal);
                assert(focus == visible);
            }
            auto left = view ? S::passiveViewMoveColumn(generation, focus, -1, crystal)
                             : S::moveRowColumn(generation, focus, -1, crystal);
            assert(left.panel == S::Panel::Values);
            auto returned = S::normalizeMoveRowFocus(generation,
                S::switchPanel(generation, left, 1, crystal), crystal);
            assert(returned.panel == S::Panel::Moves && returned.column == 0);
            for (int direction : {-1, 1}) {
                auto next = view ? S::passiveViewMoveVertical(generation, focus, direction, crystal)
                                 : S::normalizeMoveRowFocus(generation,
                                       S::moveVertical(generation, focus, direction, crystal), crystal);
                assert(next.panel == S::Panel::Moves && next.column == 0);
                assert(next.row == (int(slot) + direction + 4) % 4);
            }
        }
    }
    assert(populated && empty);
}
