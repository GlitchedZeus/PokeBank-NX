#pragma once

namespace PokeBank::UIModel::ClassicStagedExit {
enum class Action { None, Review, ContinueEditing, DiscardExit, ExportCopy };

// B always cancels an exit, including mixed controller input. Touch uses exactly the four
// visible choices. Only the dedicated Y choice discards the app-owned staged workspace.
constexpr Action choose(bool a, bool b, bool x, bool y, int touch = -1) noexcept {
    if (b || touch == 0) return Action::ContinueEditing;
    if (y || touch == 1) return Action::DiscardExit;
    if (x || touch == 3) return Action::ExportCopy;
    if (a || touch == 2) return Action::Review;
    return Action::None;
}
} // namespace PokeBank::UIModel::ClassicStagedExit
