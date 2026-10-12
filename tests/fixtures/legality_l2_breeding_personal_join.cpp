#include "Legality/Gen34BreedingPairEligibility.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace G=Legality::Gen34BreedingPairEligibility;
G::Input original(const char* game) {
    G::Input in{};
    in.game=game;
    in.depositOrderIndependentlyVerified=true;
    in.slot0={101,25,5,5,G::Gender::Female,7,true,true,true};
    in.slot1={202,132,13,13,G::Gender::Genderless,11,true,true,true};
    return in;
}
int main() {
    unsigned checks=0;
    for(const char* game:{"emerald_gba","platinum_nds","heartgold_nds","soulsilver_nds"})
    for(unsigned seed=0;seed<1024;++seed) {
        auto in=original(game);
        in.slot0.fullOTId=seed;
        in.slot1.fullOTId=seed+1;
        if(seed&1u) {const auto old=in.slot0;in.slot0=in.slot1;in.slot1=old;}
        const auto good=G::evaluate(in);
        assert(good.status==G::Status::Compatible && good.scoreKnown);
        assert(good.compatibilityScore==50);
        assert(good.donorRolesKnown && good.fatherId==202 && good.motherId==101);
        assert(G::proofFor(good,202,101)==G::Move::PairProof::IndependentlyCompatible);
        ++checks;
        auto falseDitto=in;
        auto& ditto=(falseDitto.slot0.species==132)?falseDitto.slot0:falseDitto.slot1;
        ditto.eggGroup2=0;
        assert(G::evaluate(falseDitto).status==G::Status::UnknownPersonalData);
        ++checks;
        falseDitto=in;
        auto& normal=(falseDitto.slot0.species==25)?falseDitto.slot0:falseDitto.slot1;
        normal.eggGroup2=13;
        assert(G::evaluate(falseDitto).status==G::Status::UnknownPersonalData);
        ++checks;
        falseDitto=in;
        ((falseDitto.slot0.species==132)?falseDitto.slot0:falseDitto.slot1).gender=G::Gender::Male;
        assert(G::evaluate(falseDitto).status==G::Status::UnknownGender);
        ++checks;
        falseDitto=in;
        ((falseDitto.slot0.species==132)?falseDitto.slot0:falseDitto.slot1).gender=
            static_cast<G::Gender>(255);
        assert(G::evaluate(falseDitto).status==G::Status::UnknownGender);
        ++checks;
        falseDitto=in;
        ((falseDitto.slot0.species==25)?falseDitto.slot0:falseDitto.slot1).gender=
            static_cast<G::Gender>(255);
        assert(G::evaluate(falseDitto).status==G::Status::UnknownGender);
        ++checks;
        falseDitto=in;
        ((falseDitto.slot0.species==132)?falseDitto.slot0:falseDitto.slot1).personalDataIndependentlyVerified=false;
        assert(G::evaluate(falseDitto).status==G::Status::UnknownPersonalData);
        ++checks;
        falseDitto=in;
        ((falseDitto.slot0.species==25)?falseDitto.slot0:falseDitto.slot1).genderIndependentlyVerified=false;
        assert(G::evaluate(falseDitto).status==G::Status::Compatible);
        assert(!G::evaluate(falseDitto).donorRolesKnown);
        ++checks;
    }
    assert(checks==32768);
    std::cout<<"L2 original Gen III-IV personal-to-pair joins: "<<checks<<" checks PASS\n";
}
