#ifndef POKEBANK_UI_GEN4_BOX_MOVE_MODE_H
#define POKEBANK_UI_GEN4_BOX_MOVE_MODE_H

#include <cstdint>

namespace PokeBank::UIModel::Gen4BoxMoveControl {

// Match the existing PKSE Storage cursor-mode cycle. Y chooses what the
// selection controller does; X takes the selected slot or anchors a group.
// The native PK4 transaction is kept separate from this input-only model.
enum class Mode : std::uint8_t { Menu, Move, Multi };

[[nodiscard]] constexpr Mode next(Mode mode) noexcept {
    switch (mode) {
        case Mode::Menu: return Mode::Move;
        case Mode::Move: return Mode::Multi;
        case Mode::Multi: return Mode::Menu;
    }
    return Mode::Menu;
}
[[nodiscard]] constexpr bool canPickUp(Mode mode, bool occupied) noexcept {
    return mode == Mode::Multi || (mode == Mode::Move && occupied);
}

} // namespace PokeBank::UIModel::Gen4BoxMoveControl
#endif
