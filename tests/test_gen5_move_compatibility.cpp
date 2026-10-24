// Gen V moves must be species-specific, not the whole 559 native move catalog.
// Source-pinned standard BW/B2W2 learnsets are not event provenance certificates.
#include "Integration/Gen5/Gen5MoveCompatibility.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
namespace C=PokeVault::Integration::Gen5MoveCompatibility;
static std::string source(const char* path) {
    std::ifstream f(path); assert(f);
    return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
int main() {
    static_assert(C::MaxSpecies==649&&C::MaxMove==559);
    assert(C::tableFor("black_nds")&&C::tableFor("white_nds")&&
           C::tableFor("black2_nds")&&C::tableFor("white2_nds"));
    assert(C::tableFor("platinum_nds")==nullptr);
    for(auto game:{"black_nds","white_nds","black2_nds","white2_nds"}) {
        assert(C::direct(game,25,0,85)); // Pikachu: Thunderbolt
        assert(!C::direct(game,25,0,459)); // no ordinary Roar of Time Pikachu
        const auto list=C::selectableMoves(game,25,0,{});
        assert(list.front()==0&&list.size()>40&&list.size()<180);
        assert(std::is_sorted(list.begin(),list.end()));
        assert(std::find(list.begin(),list.end(),85)!=list.end());
        assert(std::find(list.begin(),list.end(),459)==list.end());
        assert(std::find(list.begin(),list.end(),559)==list.end());
        assert(C::classify(game,25,0,459,false)==C::Availability::Unavailable);
        const auto preserved=C::selectableMoves(game,25,0,{459,0,0,0});
        assert(std::find(preserved.begin(),preserved.end(),459)!=preserved.end());
        assert(C::classify(game,25,0,459,true)==C::Availability::Preserved);
    }
    assert(C::contains(C::BlackWhite,25,85)&&C::contains(C::Black2White2,25,85));
    const auto bw=C::selectableMoves("black_nds",25,0,{});
    const auto b2=C::selectableMoves("black2_nds",25,0,{});
    assert(b2.size()>=bw.size());
    assert(C::selectableMoves("platinum_nds",25,0,{}).size()==1);
    for(auto game:{"black_nds","black2_nds"})
        for(uint16_t species:{uint16_t{1},uint16_t{25},uint16_t{150},uint16_t{479},uint16_t{646},uint16_t{649}})
            for(auto id:C::selectableMoves(game,species,0,{}))
                assert(id==0 || C::classify(game,species,0,id,false)!=C::Availability::Unavailable);
    // UI uses the currently edited species, game, form and source moves.
    const auto ui=source("src/UI/Gen5SharedReadOnlySurface.inc");
    assert(ui.find("Fields::moveChoices(screen.sourceGameId,p->species(),p->form(),p->moves())")!=std::string::npos);
    assert(ui.find("status = \"Transfer\";")!=std::string::npos);
    const auto fields=source("include/Integration/Gen5/Gen5EditorFields.h");
    assert(fields.find("for (uint16_t move = 0; move <= MaxNativeMove5; ++move)") == std::string::npos);
    std::cout<<"Gen V BW/B2W2 species moves, transfer history, current unknown preserved; Pikachu not 559 PASS\n";
}
