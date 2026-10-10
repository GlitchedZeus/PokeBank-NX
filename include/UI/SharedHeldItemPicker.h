#pragma once
#include <algorithm>
#include <vector>

namespace PokeBank::UIModel::SharedHeldItemPicker {

// One dense Held Item picker contract for every generation that supports held items.
// Gen II, Gen III and all future generation adapters must use this same geometry.
inline constexpr int columns = 4;
inline constexpr int rows = 10;
inline constexpr int pageSize = columns * rows;

// Exact accepted Generation III Held Item modal geometry (1280x720).
// Gen II/III/IV share this grid; each generation supplies native item labels.
inline constexpr int modalWidth = 1040;
inline constexpr int modalHeight = 520;
inline constexpr int gridInsetX = 22;
inline constexpr int gridInsetY = 90;
inline constexpr int rowPitch = 40;
inline constexpr int cellWidth = (modalWidth - 2 * gridInsetX) / columns;
static_assert(pageSize == 40 && cellWidth == 249);
static_assert(gridInsetY + (rows - 1) * rowPitch + (rowPitch - 4) <= modalHeight);

template <typename T>
inline int initialIndex(const std::vector<T>& items, T current) {
    const auto found = std::find(items.begin(), items.end(), current);
    return found == items.end() ? 0 : static_cast<int>(found - items.begin());
}

constexpr int move(int index, int count, int dx, int dy, int pages = 0) noexcept {
    if (count <= 0) return 0;
    index = std::clamp(index, 0, count - 1);
    if (pages) {
        const int page = std::clamp(index / pageSize + pages, 0, (count - 1) / pageSize);
        int next = page * pageSize + index % pageSize;
        while (next >= count && next >= page * pageSize + columns) next -= columns;
        return next < count ? next : page * pageSize;
    }
    if (dx && (index % columns + dx < 0 || index % columns + dx >= columns)) return index;
    const int next = index + dx + columns * dy;
    return next >= 0 && next < count ? next : index;
}

constexpr bool futureGenerationsUseSharedGrid() noexcept { return true; }

} // namespace PokeBank::UIModel::SharedHeldItemPicker
