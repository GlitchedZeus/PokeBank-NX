// Gen V Pokémon Language: pressing A cycles the native values INLINE;
// no modal, no fake ID 6, no writes until the usual staged Keep/Export.
#include "UI/Gen5LanguageCycle.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>

using PokeBank::UIModel::Gen5LanguageCycle::next;
using PokeBank::UIModel::Gen5LanguageCycle::supported;
static_assert(next(1)==2 && next(2)==3 && next(3)==4 && next(4)==5);
static_assert(next(5)==7 && next(7)==8 && next(8)==1);
static_assert(next(0)==1 && next(6)==1 && next(255)==1);
static_assert(!supported(0) && !supported(6) && !supported(255));
static_assert(supported(1) && supported(2) && supported(3) &&
              supported(4) && supported(5) && supported(7) && supported(8));

int main() {
    for (const uint8_t original : PokeBank::UIModel::Gen5LanguageCycle::kSupported) {
        uint8_t value=original;
        for (int press=0; press<7; ++press) {
            value=next(value);
            assert(supported(value));
        }
        assert(value==original); // full cycle returns to original native ID
    }
    std::ifstream f("src/UI/Gen5SharedReadOnlySurface.inc");
    assert(f && "run from repository root");
    const std::string src{std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
    const auto edit=src.find("static bool editDetail(");
    assert(edit!=std::string::npos);
    const auto lang=src.find("case Language: {",edit);
    const auto end=src.find("case Friendship:",lang);
    assert(lang!=std::string::npos && end!=std::string::npos && end>lang);
    const auto branch=src.substr(lang,end-lang);
    assert(branch.find("Gen5LanguageCycle::next(p.language())")!=std::string::npos);
    // Round 6 7C: the staged next language is passed straight to stage() and no toast follows.
    assert(branch.find("stage(screen, Field::Language, 0, PokeBank::UIModel::Gen5LanguageCycle::next(p.language()))")!=std::string::npos);
    assert(branch.find("postStatus(")==std::string::npos);
    assert(branch.find("openPicker(")==std::string::npos);
    // Both A and touch share the same adapter detail activation path.
    assert(src.find("static bool editDetail(")!=std::string::npos);
    std::cout<<"Gen V language A/touch native inline cycle, no modal, seven valid IDs PASS\n";
}
