#include "Pokemon/Gen3PartyRecord.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
int main() {
    std::array<uint8_t,Pokemon::GEN3_PARTY_RECORD_SIZE> empty{};
    Pokemon::makeEmptyGen3PartySlot(empty);
    for (std::size_t i=0;i<empty.size();++i)
        assert(empty[i] == (i==Pokemon::GEN3_PARTY_MAIL_ID_OFFSET ? 0xFF : 0));
    std::array<uint8_t,Pokemon::GEN3_PARTY_RECORD_SIZE> promoted{};
    const Pokemon::Gen3PartyDerivedStats stats{25,77,44,45,46,47,48};
    assert(!Pokemon::hasStoredGen3PartyTail(promoted,Pokemon::GEN3_STORED_RECORD_SIZE));
    Pokemon::initializeGen3PartyTail(promoted,stats);
    assert(promoted[0x50]==0 && promoted[0x54]==25 && promoted[0x55]==0xFF);
    assert(promoted[0x56]==77 && promoted[0x57]==0 && promoted[0x58]==77 && promoted[0x59]==0);
    assert(Pokemon::hasStoredGen3PartyTail(promoted,promoted.size()));
    std::array<uint8_t,Pokemon::GEN3_PARTY_RECORD_SIZE> stored{};
    stored[0x50]=0x08; stored[0x54]=92; stored[0x55]=0xFF; stored[0x56]=0x34; stored[0x57]=0x12;
    const auto before=stored;
    if (!Pokemon::hasStoredGen3PartyTail(stored,stored.size()))
        Pokemon::initializeGen3PartyTail(stored,stats);
    assert(stored==before);
    std::cout<<"Gen III party-record byte-fidelity tests: PASS\n";
    return 0;
}
