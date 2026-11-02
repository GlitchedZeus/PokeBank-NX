// Owner item 9 (round 6 feedback; Oct 28 list): while a search key grid is open, L is Backspace and
// R is consumed (matching ignores spaces, so a space could not change results); L/R never page the
// list behind the grid. With the grid closed, L/R are not taken by search.
#include "UI/SharedSearchInput.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace S = UI::SharedSearchInput;

int main() {
    S::State s;
    S::Keys l; l.l = true;
    S::Keys r; r.r = true;
    assert(S::handleKeys(s, l) == S::Event::None);             // grid closed: L belongs to the owner
    s.openGrid();
    assert(S::apply(s, S::keyAt(0, 0)) == S::Event::QueryChanged && s.query.size() == 1);
    assert(S::apply(s, S::keyAt(0, 1)) == S::Event::QueryChanged && s.query.size() == 2);
    const auto rev = s.revision;
    assert(S::handleKeys(s, l) == S::Event::QueryChanged && s.query.size() == 1 && s.revision == rev + 1);
    assert(S::handleKeys(s, r) == S::Event::Consumed && s.query.size() == 1);   // consumed, no paging
    assert(S::handleKeys(s, l) == S::Event::QueryChanged && s.query.empty());
    assert(S::handleKeys(s, l) == S::Event::Consumed);           // nothing left to delete; still consumed
    assert(s.open);
    // Every search owner passes L and R into the grid.
    for (const char* path : {"src/UI/Gen2PokemonEditorFoundation.inc", "src/UI/SharedPokemonEditorSurface.inc",
                             "src/UI/Gen3SharedPokemonSurface.inc"}) {
        std::ifstream in(path);
        const std::string src{std::istreambuf_iterator<char>(in), {}};
        assert(src.find("bool(down&HidNpadButton_L),bool(down&HidNpadButton_R)};") != std::string::npos);
    }
    std::ifstream in("include/UI/SharedSpeciesPicker.h");
    const std::string picker{std::istreambuf_iterator<char>(in), {}};
    assert(picker.find("nav.left, nav.right, a, b, nav.l, nav.r}") != std::string::npos);
    std::cout << "Search grid shoulder keys (L = Backspace, no paging while open): PASS\n";
    return 0;
}
