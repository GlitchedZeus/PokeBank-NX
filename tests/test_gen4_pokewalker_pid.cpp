#include "Legality/Gen4PokewalkerPid.h"

#include <cassert>
#include <cstdint>
#include <iostream>

namespace {
constexpr uint32_t id32(uint16_t tid, uint16_t sid) {
    return static_cast<uint32_t>(tid) | (static_cast<uint32_t>(sid) << 16);
}
}

int main() {
    using Legality::Gen4PokewalkerPid::matches;

    // PKHeX PIDIVTests Pokewalker vectors; Pikachu gender ratio is 0x7F.
    assert(matches(0x71FFFF5Au, id32(49017, 12807), 0x71FFFF5Au % 25, 1, 0x7F));
    assert(matches(0xC0000001u, id32(17398, 31936), 0xC0000001u % 25, 1, 0x7F));
    assert(matches(0x2FFFFF5Eu, id32(27008, 42726), 0x2FFFFF5Eu % 25, 1, 0x7F));
    assert(matches(0x59FFFFFEu, id32(51223, 28044), 0x59FFFFFEu % 25, 0, 0x7F));

    assert(!matches(0x71FFFF5Bu, id32(49017, 12807), 0x71FFFF5Au % 25, 1, 0x7F));

    std::cout << "Gen IV Pokewalker PID correlation: PASS\n";
}
