#include "Legality/Gen4OriginEvidence.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Legality::Gen4Origin;

    assert(kind(10) == Kind::Gen4Retail);
    assert(kind(11) == Kind::Gen4Retail);
    assert(kind(12) == Kind::Gen4Retail);
    assert(kind(7) == Kind::Gen4Retail);
    assert(kind(8) == Kind::Gen4Retail);
    assert(kind(16) == Kind::Gen4BattleRevolution);

    assert(exactRetailGameId(10) == "diamond_nds");
    assert(exactRetailGameId(11) == "pearl_nds");
    assert(exactRetailGameId(12) == "platinum_nds");
    assert(exactRetailGameId(7) == "heartgold_nds");
    assert(exactRetailGameId(8) == "soulsilver_nds");
    assert(exactRetailGameId(16).empty());

    assert(kind(1) == Kind::Gen3Handheld);
    assert(kind(2) == Kind::Gen3Handheld);
    assert(kind(3) == Kind::Gen3Handheld);
    assert(kind(4) == Kind::Gen3Handheld);
    assert(kind(5) == Kind::Gen3Handheld);
    assert(kind(15) == Kind::Gen3GameCube);

    assert(exactGen3GameId(1) == "sapphire_gba");
    assert(exactGen3GameId(2) == "ruby_gba");
    assert(exactGen3GameId(3) == "emerald_gba");
    assert(exactGen3GameId(4) == "firered_gba");
    assert(exactGen3GameId(5) == "leafgreen_gba");
    assert(exactGen3GameId(15).empty());

    assert(isNativeRetailGen4(10));
    assert(!isNativeRetailGen4(2));
    assert(isPalParkOrigin(2));
    assert(isPalParkOrigin(15));
    assert(!isPalParkOrigin(12));
    assert(kind(0) == Kind::Unknown);

    std::cout << "Gen IV stored-origin identity evidence: PASS\n";
}
