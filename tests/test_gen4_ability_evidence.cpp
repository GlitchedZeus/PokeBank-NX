#include "Pokemon/AbilityInfo.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Pokemon;

    // Gengar changed from Levitate in Gen IV to Cursed Body in later generations.
    const auto gengarDP = getAbilitySlots(94, 0, Enums::GameVersion::DP);
    assert(gengarDP.count == 2);
    assert(gengarDP.slot[0] == 26 && gengarDP.slot[1] == 26);
    assert(gengarDP.slot[2] == 0);

    const auto gengarModern = getAbilitySlots(94, 0, Enums::GameVersion::SV);
    assert(gengarModern.slot[0] == 130);

    // Two-ability species preserve their native pair and PID parity selects slot 1/2.
    const auto pidgeyDP = getAbilitySlots(16, 0, Enums::GameVersion::DP);
    assert(pidgeyDP.count == 2);
    assert(pidgeyDP.slot[0] == 51);
    assert(pidgeyDP.slot[1] == 77);
    assert(getAbilityNumberForId(pidgeyDP, 51) == 1);
    assert(getAbilityNumberForId(pidgeyDP, 77) == 2);

    const auto pidgeyPt = getAbilitySlots(16, 0, Enums::GameVersion::PT);
    const auto pidgeyHgss = getAbilitySlots(16, 0, Enums::GameVersion::HGSS);
    assert(pidgeyPt.slot[0] == 51 && pidgeyPt.slot[1] == 77);
    assert(pidgeyHgss.slot[0] == 51 && pidgeyHgss.slot[1] == 77);

    std::cout << "Gen IV native ability evidence: PASS\n";
}
