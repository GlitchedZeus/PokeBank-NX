// Gen I list editors (owner campaign phases 1 and 5): only whole rows that end above the status
// line are drawn, so the focused row is never hidden under it, and no "Row x/y" counter remains in
// the Gen I / Gen II editors (the scroll position follows focus).
#include "UI/Gen1ListFit.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
namespace Fit = PokeBank::UIModel::Gen1ListFit;
std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

int main() {
    static_assert(Fit::wholeRows(0, 100, 48, 13) == 2);
    static_assert(Fit::wholeRows(0, 10, 48, 13) == 1);   // never zero rows
    static_assert(Fit::wholeRows(0, 2000, 48, 13) == 13); // capped at the row count
    // Both live lists across every panel height they can get: the last drawn row ends above the
    // status line (UX 30-field list: rows of 41 from y+82 above y+h-70; UX2 13-field list: rows of
    // 48 from y+92 above the divider at y+h-76).
    for (int h = 360; h <= 720; ++h) {
        const int uxRows = Fit::wholeRows(82, h - 70 - 8, 41, 12);
        assert(82 + uxRows * 41 <= h - 70 - 8);
        const int ux2Rows = Fit::wholeRows(92, h - 76 - 4, 48, 13);
        assert(92 + ux2Rows * 48 <= h - 76 - 4 || ux2Rows == 1);
    }
    const auto ux = read("src/UI/Gen1PokemonEditorOverlayUX.inc");
    const auto ux2 = read("src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc");
    assert(ux.find("Gen1ListFit::wholeRows(listY, y + h - 70 - 8, 41, 12)") != std::string::npos);
    assert(ux2.find("Gen1ListFit::wholeRows(listY, y + h - 76 - 4, rowH, 13)") != std::string::npos);
    for (const char* path : {"src/UI/Gen1PokemonEditorOverlayUX.inc", "src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc",
                             "src/UI/Gen1PokemonEditorOverlayFoundation.inc",
                             "src/UI/Gen1PokemonEditorFoundationHardwareFix.inc",
                             "src/UI/Gen2UnifiedPokemonWorkspace.inc", "src/UI/Gen2HardwareFinalFix.inc",
                             "src/UI/TrainerViewScreenGSCOverlay.inc"}) {
        const auto text = read(path);
        assert(text.find("\"Row \" + std::to_string(") == std::string::npos);
        assert(text.find("\"Rows \" + std::to_string(") == std::string::npos);
    }
    std::cout << "Gen I list editors: whole rows above the status line, no row counters PASS\n";
    return 0;
}
