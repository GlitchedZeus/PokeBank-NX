#ifndef POKEBANK_GEN6_READOUTS_H
#define POKEBANK_GEN6_READOUTS_H

#include "Integration/Gen6/Gen6Save.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

// Read-only Gen VI box names and Pokedex progress. Nothing here writes a byte.
// Sources, pinned PKHeX 6501f0ab:
//   Saves/Substructures/Gen6/BoxLayout6.cs  block 12 (0x4400): 31 names of 0x22 bytes
//     (UTF-16, SAV6.LongStringLength), wallpapers at 0x41E, unlocked count 0x43E,
//     current box 0x43F. PKSM-Core aa22d7a4 Sav6::boxName reads PCLayout + 0x22*box.
//   Saves/Substructures/PokeDex/Zukan6.cs   block 20 (0x15000): caught bits at 0x08,
//     four seen regions of 0x60 bytes at 0x68, bit = species-1 (species 1..721);
//     Zukan6XY foreign-obtained flags at 0x64C (0x54 bytes, species 1..649).
namespace PokeVault::Integration::Gen6::Readouts {

inline constexpr size_t BoxLayoutBlock = 12;
inline constexpr size_t BoxNameBytes = 0x22;
inline constexpr size_t UnlockedOffset = 0x43E;
inline constexpr size_t CurrentBoxOffset = 0x43F;
inline constexpr size_t ZukanBlock = 20;
inline constexpr size_t ZukanCaught = 0x08;
inline constexpr size_t ZukanSeen = 0x68;
inline constexpr size_t ZukanRegion = 0x60;
inline constexpr size_t ZukanForeignXY = 0x64C;
inline constexpr uint16_t MaxSpecies = 721;
inline constexpr uint16_t MaxForeignSpecies = 649;

// Raw UTF-16 box name (up to 17 units, stops at the terminator); nullopt for a bad box.
[[nodiscard]] inline std::optional<std::u16string> boxName(const Gen6Save& save, size_t box) {
    if (box >= Layout::BoxCount) return std::nullopt;
    const auto base = Layout::blocks(save.family())[BoxLayoutBlock].offset + box * BoxNameBytes;
    std::u16string out;
    for (size_t i = 0; i < BoxNameBytes / 2; ++i) {
        const uint16_t ch = PK6::read16(save.bytes(), base + 2 * i);
        if (ch == 0) break;
        out.push_back(static_cast<char16_t>(ch));
    }
    return out;
}

[[nodiscard]] inline uint8_t unlockedBoxes(const Gen6Save& save) {
    return save.bytes()[Layout::blocks(save.family())[BoxLayoutBlock].offset + UnlockedOffset];
}
[[nodiscard]] inline uint8_t currentBox(const Gen6Save& save) {
    return save.bytes()[Layout::blocks(save.family())[BoxLayoutBlock].offset + CurrentBoxOffset];
}

struct DexProgress6 {
    uint16_t seen = 0;
    uint16_t caught = 0;
    uint16_t foreignObtained = 0;  // X/Y only; reported separately, never merged
    uint16_t total = MaxSpecies;
};

[[nodiscard]] inline bool dexBit(const Gen6Save& save, size_t offset, uint16_t species) {
    const auto base = Layout::blocks(save.family())[ZukanBlock].offset + offset;
    const size_t bit = species - 1u;
    return (save.bytes()[base + (bit >> 3)] >> (bit & 7)) & 1;
}

[[nodiscard]] inline bool dexCaught(const Gen6Save& save, uint16_t species) {
    return species >= 1 && species <= MaxSpecies && dexBit(save, ZukanCaught, species);
}
[[nodiscard]] inline bool dexSeen(const Gen6Save& save, uint16_t species) {
    if (species < 1 || species > MaxSpecies) return false;
    for (size_t region = 0; region < 4; ++region)
        if (dexBit(save, ZukanSeen + region * ZukanRegion, species)) return true;
    return false;
}

[[nodiscard]] inline DexProgress6 dexProgress(const Gen6Save& save) {
    DexProgress6 p;
    for (uint16_t s = 1; s <= MaxSpecies; ++s) {
        if (dexSeen(save, s)) ++p.seen;
        if (dexCaught(save, s)) ++p.caught;
        if (save.family() == Layout::Family::XY && s <= MaxForeignSpecies && dexBit(save, ZukanForeignXY, s))
            ++p.foreignObtained;
    }
    return p;
}

} // namespace PokeVault::Integration::Gen6::Readouts

#endif
