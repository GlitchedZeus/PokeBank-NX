// Independent exhaustive original-source role matrix. This is structural
// preflight, not an attestation of original retail parents or a global
// legality verdict. Run alongside the 17 existing real L2 test programs.
#include "Legality/Gen34OriginalDaycareRoles.h"
#include <cassert>
#include <array>
#include <cstdint>
#include <iostream>
namespace R=Legality::Gen34OriginalDaycareRoles;
constexpr R::Status independent(R::Parent a,R::Parent b,uint16_t ceiling){
    if(!a.species||!b.species||a.species>ceiling||b.species>ceiling||a.gender>2||b.gender>2)
        return R::Status::UnknownOriginalMetadata;
    if(a.species==132&&b.species==132)return R::Status::IncompatibleOrdinaryPair;
    if((a.species==132&&a.gender!=2)||(b.species==132&&b.gender!=2))
        return R::Status::UnknownOriginalMetadata;
    if(a.species==132||b.species==132)return R::Status::StructurallyCompatible;
    return (a.gender==0&&b.gender==1)||(a.gender==1&&b.gender==0)?
        R::Status::StructurallyCompatible:R::Status::IncompatibleOrdinaryPair;
}
int main(){
    unsigned long long checks=0;
    for(auto max:{uint16_t{386},uint16_t{493}})
    for(uint16_t a:{uint16_t{0},uint16_t{25},uint16_t{81},uint16_t{132},uint16_t{386},uint16_t{387},uint16_t{493},uint16_t{494},uint16_t{65535}})
    for(uint16_t b:{uint16_t{0},uint16_t{25},uint16_t{81},uint16_t{132},uint16_t{386},uint16_t{387},uint16_t{493},uint16_t{494},uint16_t{65535}})
    for(unsigned g0=0;g0<5;++g0)
    for(unsigned g1=0;g1<5;++g1){
        R::Parent p{a,static_cast<uint8_t>(g0)},q{b,static_cast<uint8_t>(g1)};
        const auto actual=R::evaluate(p,q,max);
        assert(actual==independent(p,q,max));
        assert(actual==R::evaluate(q,p,max));
        if(actual==R::Status::StructurallyCompatible){
            assert(p.species&&q.species&&p.species<=max&&q.species<=max);
        }
        checks+=3;
    }
    assert(checks==12150);
    std::cout<<"L2 source parent role matrix: "<<checks<<" checks PASS\n";
}
