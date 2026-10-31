#ifndef POKEBANK_UI_NATURE_PICKER_LAYOUT_H
#define POKEBANK_UI_NATURE_PICKER_LAYOUT_H

namespace PokeBank::UIModel::NaturePickerLayout {

// Nature has short names and only 25 choices. A seven-row, narrow modal
// retains readable body-sized labels while avoiding the large generic picker.
// The same geometry applies to Gen III (RSE/FRLG) and Gen IV (DPPt/HGSS).
struct Metrics {
    int width;
    int height;
    int visibleRows;
    int rowStep;
    int highlightHeight;
};

[[nodiscard]] constexpr Metrics forChoices(int count) noexcept {
    const int rows=count<1?1:count>7?7:count;
    return {480,124+rows*38,rows,38,34};
}

} // namespace PokeBank::UIModel::NaturePickerLayout

#endif
