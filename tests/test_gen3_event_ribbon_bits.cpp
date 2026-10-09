#include "Pokemon/Pokemon3FRLG.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>

namespace {
Pokemon::Pokemon3FRLG blankPk3() {
    std::array<uint8_t, 80> raw{};
    return Pokemon::Pokemon3FRLG(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(raw.data()), raw.size()));
}

void setRibbonWord(Pokemon::Pokemon3FRLG& p, uint32_t value) {
    p.wr32(0x4C, value);
    p.refreshChecksum();
    assert(p.rd32(0x4C) == value);
}
}

int main() {
    auto p = blankPk3();

    // PK3 event-ribbon bits are adjacent but semantically distinct. Lock the
    // reader masks independently from the WC3 matcher so a future bit-shift
    // regression cannot silently change event-template evidence.
    setRibbonWord(p, 0x00F00000u); // bits 20-23
    assert(p.ribbonChampionBattle());
    assert(p.ribbonChampionRegional());
    assert(p.ribbonChampionNational());
    assert(p.ribbonCountry());
    assert(!p.ribbonNational());
    assert(p.ribbonEarth() == (p.rd32(0x4C) == 0x02000000u));
    assert(!p.isFatefulEncounter());

    setRibbonWord(p, 0x01000000u); // bit 24: National Ribbon
    assert(!p.ribbonChampionBattle());
    assert(!p.ribbonChampionRegional());
    assert(!p.ribbonChampionNational());
    assert(!p.ribbonCountry());
    assert(p.ribbonNational());
    assert(p.ribbonEarth() == (p.rd32(0x4C) == 0x02000000u));
    assert(!p.isFatefulEncounter());

    // Bit 25 is Earth Ribbon. It is intentionally not aliased to any fixed
    // Event3 reader because Earth can be earned later through Gen III
    // GameCube cross-transfer history.
    setRibbonWord(p, 0x02000000u);
    assert(!p.ribbonChampionBattle());
    assert(!p.ribbonChampionRegional());
    assert(!p.ribbonChampionNational());
    assert(!p.ribbonCountry());
    assert(!p.ribbonNational());
    assert(p.ribbonEarth() == (p.rd32(0x4C) == 0x02000000u));
    assert(!p.isFatefulEncounter());

    // Fateful/obedience is bit 31 of the same word and must remain independent.
    setRibbonWord(p, 0x80000000u);
    assert(!p.ribbonChampionBattle());
    assert(!p.ribbonChampionRegional());
    assert(!p.ribbonChampionNational());
    assert(!p.ribbonCountry());
    assert(!p.ribbonNational());
    assert(!p.ribbonEarth());
    assert(p.isFatefulEncounter());

    // Exhaustively isolate every stored bit, including unused/reserved bits.
    for (unsigned bit = 0; bit < 32; ++bit) {
        setRibbonWord(p, uint32_t{1} << bit);
        assert(p.ribbonChampionBattle() == (bit == 20));
        assert(p.ribbonChampionRegional() == (bit == 21));
        assert(p.ribbonChampionNational() == (bit == 22));
        assert(p.ribbonCountry() == (bit == 23));
        assert(p.ribbonNational() == (bit == 24));
        assert(p.ribbonEarth() == (bit == 25));
        assert(p.isFatefulEncounter() == (bit == 31));
    }

    std::cout << "Gen III persisted Event3 ribbon bit readers: PASS\n";
}
