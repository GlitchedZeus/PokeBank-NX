#ifndef POKEBANK_GEN7_READOUTS_H
#define POKEBANK_GEN7_READOUTS_H

#include "Integration/Gen7/Gen7Save.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

// Read-only SM/USUM box names and Pokedex progress. Nothing here writes a byte.
// Sources, pinned PKHeX 6501f0ab:
//   Saves/Substructures/Gen7/BoxLayout7.cs  block 13: 32 names of 0x22 bytes
//     (SAV6.LongStringLength, UTF-16), PCFlags 0x5E0, Unlocked 0x5E1, LastViewedBox 0x5E3.
//   Saves/Substructures/PokeDex/Zukan7.cs + Zukan.cs  block 6: magic(4)+flags(4)+misc(0x80),
//     caught bits at 0x88 (0x68 bytes), four seen regions of 0x8C bytes at 0xF0, bit = species-1.
//   Species range: SM 802, USUM 807 (PKSM-Core aa22d7a4 VersionTables maxSpecies).
namespace PokeVault::Integration::Gen7::Readouts {

inline constexpr size_t ZukanBlock = 6;
inline constexpr size_t BoxLayoutBlock = 13;
inline constexpr size_t BoxNameBytes = 0x22;
inline constexpr size_t UnlockedOffset = 0x5E1;
inline constexpr size_t CurrentBoxOffset = 0x5E3;
inline constexpr size_t ZukanCaught = 0x88;
inline constexpr size_t ZukanSeen = 0xF0;
inline constexpr size_t ZukanRegion = 0x8C;

[[nodiscard]] constexpr uint16_t maxSpecies(Layout::Family f) noexcept {
    return f == Layout::Family::SunMoon ? 802 : 807;
}

[[nodiscard]] inline std::optional<std::u16string> boxName(const Gen7Save& save, size_t box) {
    if (box >= Layout::BoxCount) return std::nullopt;
    const auto base = Layout::blocks(save.family())[BoxLayoutBlock].offset + box * BoxNameBytes;
    std::u16string out;
    for (size_t i = 0; i < BoxNameBytes / 2; ++i) {
        const uint16_t ch = PK7::read16(save.bytes(), base + 2 * i);
        if (ch == 0) break;
        out.push_back(static_cast<char16_t>(ch));
    }
    return out;
}
[[nodiscard]] inline uint8_t unlockedBoxes(const Gen7Save& save) {
    return save.bytes()[Layout::blocks(save.family())[BoxLayoutBlock].offset + UnlockedOffset];
}
[[nodiscard]] inline uint8_t currentBox(const Gen7Save& save) {
    return save.bytes()[Layout::blocks(save.family())[BoxLayoutBlock].offset + CurrentBoxOffset];
}

[[nodiscard]] inline bool dexBit(const Gen7Save& save, size_t offset, uint16_t species) {
    const auto base = Layout::blocks(save.family())[ZukanBlock].offset + offset;
    const size_t bit = species - 1u;
    return (save.bytes()[base + (bit >> 3)] >> (bit & 7)) & 1;
}
[[nodiscard]] inline bool dexCaught(const Gen7Save& save, uint16_t species) {
    return species >= 1 && species <= maxSpecies(save.family()) && dexBit(save, ZukanCaught, species);
}
[[nodiscard]] inline bool dexSeen(const Gen7Save& save, uint16_t species) {
    if (species < 1 || species > maxSpecies(save.family())) return false;
    for (size_t region = 0; region < 4; ++region)
        if (dexBit(save, ZukanSeen + region * ZukanRegion, species)) return true;
    return false;
}

struct DexProgress7 {
    uint16_t seen = 0, caught = 0, total = 0;
};
[[nodiscard]] inline DexProgress7 dexProgress(const Gen7Save& save) {
    DexProgress7 p;
    p.total = maxSpecies(save.family());
    for (uint16_t s = 1; s <= p.total; ++s) {
        if (dexSeen(save, s)) ++p.seen;
        if (dexCaught(save, s)) ++p.caught;
    }
    return p;
}

} // namespace PokeVault::Integration::Gen7::Readouts

#endif
