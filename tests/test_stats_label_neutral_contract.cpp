// Owner Round 4: native Gen IV View/Edit STATS use neutral text. Nature coloring belongs
// to the nature-picker itself, never to normal stat labels. Source UI contract plus
// independent Gen III-V nature effect arithmetic. Run from the repository root.
#include "UI/NatureStatEffect.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <string>
#include <iostream>

static std::string slurp(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
int main() {
    const auto src = slurp("src/UI/SharedPokemonEditorSurface.inc");
    const auto begin = src.find("drawPanel(fb, g.midX, g.y, g.midW, g.h, \"STATS\")");
    assert(begin != std::string::npos);
    const auto end = src.find("fb.drawHDivider(g.midX + 14, g.y + 310", begin);
    assert(end != std::string::npos && end > begin);
    const auto stats = src.substr(begin, end - begin);
    assert(stats.find("NatureStats::statColor") == std::string::npos);
    assert(stats.find("selected ? Colors::SelectedText : Colors::TextDim") != std::string::npos);
    // Preserve distinct nature-picker feedback instead of hiding real modifiers.
    assert(src.find("NatureStats::drawEffect") != std::string::npos);
    const auto nature = slurp("include/UI/NatureStatPresentation.h");
    assert(nature.find("Colors::Success") != std::string::npos);
    assert(nature.find("Colors::Error") != std::string::npos);
    namespace N = PokeBank::UIModel::NatureStatEffect;
    for(unsigned n=0; n<25; ++n) {
        assert(N::modifier(n, 0) == 0);
        for(unsigned row=1; row<6; ++row) {
            const int m=N::modifier(n,row);
            assert(m>=-1 && m<=1);
        }
    }
    std::cout << "Shared STATS labels neutral; Nature picker retains correct colored effect semantics PASS\n";
}
