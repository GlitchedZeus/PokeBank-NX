#include "Legality/Gen34OriginalPersonalSnapshotGuard.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace S=Legality::Gen34OriginalPersonalSnapshotGuard;
int main() {
    unsigned long long checks=0;
    for(const std::uint16_t species:{std::uint16_t{1},std::uint16_t{25},std::uint16_t{132},std::uint16_t{386},std::uint16_t{493},std::uint16_t{65535}})
    for(unsigned g1=0;g1<=16;++g1)
    for(unsigned g2=0;g2<=16;++g2) {
        const auto expected=species==132 ? (g1==13 && g2==13) : (g1!=13 && g2!=13);
        const auto got=S::coherentDittoGroups(species,static_cast<std::uint8_t>(g1),static_cast<std::uint8_t>(g2));
        assert(got==expected);
        ++checks;
        for(unsigned ratio:{0u,31u,127u,254u,255u}) {
            assert(S::coherentDittoGenderRatio(species,static_cast<std::uint8_t>(ratio))==(species!=132 || ratio==255));
            ++checks;
        }
    }
    for(unsigned value=0;value<256;++value) {
        assert(S::validKnownGender(static_cast<std::uint8_t>(value))==(value<=3));
        ++checks;
    }
    assert(checks==10660);
    std::cout << "L2 personal Ditto provenance matrix: " << checks << " checks PASS\n";
}
