// Research-only defect reproducer, not a production patch.
// PKHeX 6501f0ab46e8f8ca048539dbaf8cae8cb104e722 AND
// 641c26ba100225f3605e5a44b61b3b16e15a3774:
// Items/Bags/PlayerBag7SM.cs and PlayerBag7USUM.cs GetMaxCount:
// KeyItems, item797 (Z-Ring) may have count2. NX #311 ignores this exception.
#include "Integration/Gen7/Gen7Bag.h"
#include <cassert>
#include <iostream>
#include <vector>
namespace G = PokeVault::Integration::Gen7;
int main() {
    for (auto family : {G::Layout::Family::SunMoon, G::Layout::Family::UltraSunUltraMoon}) {
        std::vector<uint8_t> block(G::itemsBlockLength7(family), 0);
        const auto pocket=static_cast<size_t>(G::BagPocket7::KeyItems);
        const auto offset=G::bagPocketLayout7(family,pocket).offset;
        assert(G::gen7BagItemInPocket(family,pocket,797));
        G::PK7::write32(block,offset,797u|(1u<<10));
        assert(G::decodeBag7(family,block));
        G::PK7::write32(block,offset,797u|(2u<<10));
        assert(!G::decodeBag7(family,block));
    }
    std::cout << "REPRODUCED UNFIXED: both families reject PKHeX-supported Z-Ring x2; ordinary x1 accepted\n";
}
