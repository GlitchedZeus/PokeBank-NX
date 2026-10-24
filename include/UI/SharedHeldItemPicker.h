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

// Round 5 NEW-06: while the search keyboard is open (the Species chooser's layout) the results
// keep kSearchColumns columns on the left and the keyboard + focused artwork take the right side.
inline constexpr int kSearchColumns = 2;
constexpr int columnsFor(bool searching) noexcept { return searching ? kSearchColumns : columns; }
constexpr int gridWidth(int cols) noexcept { return cols * cellWidth; }
static_assert(gridWidth(columns) == modalWidth - 2 * gridInsetX);

// Grid navigation over `cols` columns (pages are `cols` x rows).
constexpr int move(int index, int count, int dx, int dy, int pages, int cols) noexcept {
    if (count <= 0) return 0;
    cols = std::max(1, cols);
    const int page = cols * rows;
    index = std::clamp(index, 0, count - 1);
    if (pages) {
        const int target = std::clamp(index / page + pages, 0, (count - 1) / page);
        int next = target * page + index % page;
        while (next >= count && next >= target * page + cols) next -= cols;
        return next < count ? next : target * page;
    }
    if (dx && (index % cols + dx < 0 || index % cols + dx >= cols)) return index;
    const int next = index + dx + cols * dy;
    return next >= 0 && next < count ? next : index;
}
constexpr int move(int index, int count, int dx, int dy, int pages = 0) noexcept {
    return move(index, count, dx, dy, pages, columns);
}

constexpr bool futureGenerationsUseSharedGrid() noexcept { return true; }

} // namespace PokeBank::UIModel::SharedHeldItemPicker
