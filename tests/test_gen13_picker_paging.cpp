// Divergence item 7: Gen I-III list pickers page exactly like the shared Gen IV/V editor's list
// pickers. Pure model + equivalence with the shared surface's own code + each live picker route.
#include "UI/Gen13PickerPaging.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace P = PokeBank::UIModel::SharedPickerPaging;

namespace {
std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void has(const std::string& t, const char* n) {
    if (t.find(n) == std::string::npos) std::cerr << "Missing: " << n << '\n';
    assert(t.find(n) != std::string::npos);
}
void lacks(const std::string& t, const char* n) {
    if (t.find(n) != std::string::npos) std::cerr << "Forbidden: " << n << '\n';
    assert(t.find(n) == std::string::npos);
}
P::Keys key(char k) {
    P::Keys keys;
    keys.up = k == 'U'; keys.down = k == 'D'; keys.left = k == '<'; keys.right = k == '>';
    keys.l = k == 'L'; keys.r = k == 'R'; keys.zl = k == 'l'; keys.zr = k == 'r';
    return keys;
}
// The shared surface's list-picker code (SharedPokemonEditorSurface.inc handleValuePicker),
// transcribed: the reference the Gen I-III pickers must match.
int sharedReference(int row, int count, const P::Keys& k) {
    if (k.up) row = (row - 1 + count) % count;
    if (k.down) row = (row + 1) % count;
    if (k.left || k.l) row = std::max(0, row - 10);
    if (k.right || k.r) row = std::min(count - 1, row + 10);
    if (k.zl) row = std::max(0, row - 50);
    if (k.zr) row = std::min(count - 1, row + 50);
    return row;
}
} // namespace

int main() {
    for (const int count : {1, 9, 151, 251, 252, 386}) {
        for (int row = 0; row < count; ++row)
            for (const char k : {'U', 'D', '<', '>', 'L', 'R', 'l', 'r'})
                assert(P::pagedRow(row, count, key(k)) == sharedReference(row, count, key(k)));
    }
    static_assert(P::pagedRow(0, 151, {true}) == 150);             // Up wraps
    static_assert(P::pagedRow(150, 151, {false, true}) == 0);      // Down wraps
    assert(P::pagedRow(5, 151, key('L')) == 0 && P::pagedRow(145, 151, key('R')) == 150);
    assert(P::pagedRow(100, 386, key('l')) == 50 && P::pagedRow(100, 386, key('r')) == 150);
    assert(P::pagedRow(7, 0, key('D')) == 0 && !P::Keys{}.any());

    // The shared surface still uses exactly this scheme (so the reference above is current).
    const auto shared = read("src/UI/SharedPokemonEditorSurface.inc");
    has(shared, "if (down & (HidNpadButton_Left | HidNpadButton_L))\n            state.pickerRow = std::max(0, state.pickerRow - 10);");
    has(shared, "if (down & HidNpadButton_ZL)\n            state.pickerRow = std::max(0, state.pickerRow - 50);");

    // Gen I species + move pickers.
    const auto gen1 = read("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc");
    has(gen1, "state.pickerValue = PokeBank::UIModel::SharedPickerPaging::pagedRow(state.pickerValue - 1, 151, keys) + 1;");
    has(gen1, "index = PokeBank::UIModel::SharedPickerPaging::pagedRow(index, static_cast<int>(choices.size()), keys);");
    has(gen1, "(down & HidNpadButton_ZL) != 0, (down & HidNpadButton_ZR) != 0};");
    lacks(gen1, "index = std::min(static_cast<int>(choices.size()) - 1, index + 10)");
    // Gen II species/move/location pickers (stick repeats via nav).
    const auto gen2 = read("src/UI/Gen2PokemonPickerOverlay.inc");
    has(gen2, "picker.model.index = PokeBank::UIModel::SharedPickerPaging::pagedRow(");
    lacks(gen2, "stepList(-10)");
    const auto gen2Move = read("src/UI/Gen2HardwarePickerFix.inc");
    has(gen2Move, "const int next = PokeBank::UIModel::SharedPickerPaging::pagedRow(selectedPos, count, pageKeys);");
    lacks(gen2Move, "stepHardwareMovePicker(");
    // Gen III value list + species pickers.
    const auto gen3 = read("src/UI/Gen3SharedPokemonSurface.inc");
    has(gen3, "state.pickerRow = PokeBank::UIModel::SharedPickerPaging::pagedRow(\n            state.pickerRow, count, gen3PickerKeys(down));");
    has(gen3, "static_cast<int>(state.speciesPreview) - 1, 386, gen3PickerKeys(down)) + 1);");
    has(gen3, "(down & HidNpadButton_ZL) != 0, (down & HidNpadButton_ZR) != 0};");

    std::cout << "Gen I-III picker paging matches the shared editor: PASS\n";
}
