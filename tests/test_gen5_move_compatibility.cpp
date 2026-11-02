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
    size_t bwDirect=0,b2Direct=0;
    for(uint16_t move=1;move<=559;++move) {
        bwDirect+=C::direct("black_nds",25,0,move);
        b2Direct+=C::direct("black2_nds",25,0,move);
    }
    assert(bwDirect==55 && b2Direct==64);
    for(auto game:{"black_nds","white_nds","black2_nds","white2_nds"}) {
        // Wormadam cannot switch its evolved cloak. Never substitute Plant's
        // Grass learnset for Sandy/Trash; pre-evolution moves remain inherited.
        assert(C::direct(game,413,0,437)); // Leaf Storm, Plant.
        assert(!C::direct(game,413,1,437) && !C::transferable(413,1,437));
        assert(C::direct(game,413,1,89)); // Earthquake, Sandy.
        assert(!C::direct(game,413,0,89));
        assert(C::direct(game,413,2,442)); // Iron Head, Trash.
        assert(!C::direct(game,413,2,89));
        assert(C::direct(game,386,1,276)); // Attack Deoxys: Superpower.
        assert(!C::direct(game,386,0,276));
        assert(C::priorForm(game,386,276)); // changing back can retain it.
        for(uint8_t form=0;form<=5;++form)for(uint8_t other=1;other<=5;++other) {
            const auto move=PokeVault::Integration::Gen4MoveCompatibility::rotomApplianceMove(other);
            assert(C::direct(game,479,form,move)==(form==other));
            if(form!=other)assert(C::classify(game,479,form,move,false)==C::Availability::Unavailable);
        }
        assert(C::direct(game,235,0,459)); // Smeargle is the genuine Sketch exception.
        assert(!C::direct(game,235,0,165) && !C::direct(game,235,0,448));
        assert(!C::formAvailable(game,25,1));
        assert((C::selectableMoves(game,25,1,{459,0,0,0})==std::vector<uint16_t>{0,459}));
        assert(C::classify(game,25,1,459,true)==C::Availability::Preserved);
    }
    for(auto game:{"black_nds","white_nds"}) {
        assert(!C::formAvailable(game,646,1) && !C::formAvailable(game,641,1));
        assert(!C::direct(game,646,1,558));
        assert(!C::direct(game,571,0,555)); // Snarl TM95 never released in BW.
        assert(C::classify(game,571,0,555,false)==C::Availability::Transfer);
    }
    for(auto game:{"black2_nds","white2_nds"}) {
        assert(C::direct(game,646,1,558) && C::direct(game,646,1,554)); // White Kyurem.
        assert(C::direct(game,646,2,559) && C::direct(game,646,2,553)); // Black Kyurem.
        assert(!C::direct(game,646,1,559) && !C::transferable(646,1,559));
        assert(!C::direct(game,646,0,558) && !C::transferable(646,0,558));
        assert(C::formAvailable(game,641,1) && C::direct(game,641,1,542));
        assert(C::direct(game,571,0,555));
    }
    // All supported native form rows are nonempty; post-Gen-V and invalid forms
    // never offer new moves. Every offered move has an explicit classification.
    for(auto game:{"black_nds","black2_nds"})for(const auto& row:C::FormCounts) {
        for(uint8_t form=0;form<32;++form) {
            const auto moves=C::selectableMoves(game,row.species,form,{});
            assert((moves.size()>1)==C::formAvailable(game,row.species,form));
            for(auto move:moves)assert(C::classify(game,row.species,form,move,false)!=C::Availability::Unavailable);
        }
    }
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
    std::cout<<"Gen V exact BW/B2W2 species/forms, Rotom/Kyurem/Wormadam, prior forms, transfer, Sketch and preserved history; Pikachu 55/64 direct PASS\n";
}
