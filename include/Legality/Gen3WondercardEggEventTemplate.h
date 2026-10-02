#pragma once

#include "Legality/Gen3PidIvCorrelation.h"

#include <array>
#include <cstdint>

namespace Legality::Gen3WondercardEggEvent {

struct Entry {
    uint16_t species;
    uint8_t originMask; // bit0 Emerald, bit1 FireRed, bit2 LeafGreen
    std::array<uint16_t, 4> moves;
};

inline constexpr uint8_t kEmerald = 1u << 0;
inline constexpr uint8_t kFireRed = 1u << 1;
inline constexpr uint8_t kLeafGreen = 1u << 2;
inline constexpr uint8_t kFRLG = kFireRed | kLeafGreen;
inline constexpr uint8_t kEFL = kEmerald | kFireRed | kLeafGreen;

inline constexpr std::array<Entry, 25> kEntries{{
    // PCJP Egg Pokemon Present / PCNY Wish Eggs -- FR/LG.
    {43,  kFRLG, {71, 73, 0, 0}},
    {52,  kFRLG, {10, 45, 80, 0}},
    {60,  kFRLG, {145, 186, 0, 0}},
    {69,  kFRLG, {22, 298, 0, 0}},
    {83,  kFRLG, {281, 273, 0, 0}},
    {96,  kFRLG, {187, 273, 0, 0}},
    {102, kFRLG, {230, 273, 0, 0}},
    {108, kFRLG, {215, 273, 0, 0}},
    {113, kFRLG, {230, 273, 0, 0}},
    {115, kFRLG, {281, 273, 0, 0}},

    // PokePark Wondercard eggs -- Emerald / FireRed / LeafGreen.
    {54,  kEFL, {346, 10, 39, 300}},
    {172, kEFL, {84, 204, 266, 0}},
    {174, kEFL, {47, 204, 111, 321}},
    {222, kEFL, {33, 300, 0, 0}},
    {276, kEFL, {64, 45, 116, 297}},
    {283, kEFL, {145, 300, 0, 0}},
    {293, kEFL, {1, 253, 298, 0}},
    {300, kEFL, {45, 33, 39, 205}},
    {311, kEFL, {45, 86, 346, 0}},
    {312, kEFL, {45, 86, 300, 0}},
    {325, kEFL, {150, 253, 0, 0}},
    {327, kEFL, {33, 253, 47, 0}},
    {331, kEFL, {40, 43, 71, 227}},
    {341, kEFL, {145, 346, 0, 0}},
    {360, kEFL, {150, 204, 227, 321}},
}};

struct Candidate {
    uint16_t species = 0;
    uint8_t originGame = 0;
    uint8_t language = 0;
    uint8_t metLevel = 0;
    uint16_t metLocation = 0;
    uint8_t ball = 0;
    bool isEgg = false;
    bool fateful = false;
    std::array<uint16_t, 4> moves{};
};

constexpr uint8_t originBit(uint8_t originGame) noexcept {
    switch (originGame) {
        case 3: return kEmerald;
        case 4: return kFireRed;
        case 5: return kLeafGreen;
        default: return 0;
    }
}

constexpr bool allowedMethod(Gen3PidIv::Method method) noexcept {
    // Pinned EncounterGift3 Method_2 compatibility:
    // native Method 2, or Method 1 / Method 4 via VBlank timing.
    return method == Gen3PidIv::Method::Method1 ||
           method == Gen3PidIv::Method::Method2 ||
           method == Gen3PidIv::Method::Method4;
}

constexpr bool matches(const Candidate& c,
                       const Gen3PidIv::Result& rng) noexcept {
    if (!allowedMethod(rng.method) ||
        c.language != 1 ||
        c.metLevel != 0 ||
        c.metLocation != 255 ||
        c.ball != 4 ||
        !c.isEgg ||
        !c.fateful)
        return false;

    const uint8_t bit = originBit(c.originGame);
    if (bit == 0)
        return false;

    for (const auto& row : kEntries) {
        if (row.species == c.species &&
            (row.originMask & bit) != 0 &&
            row.moves == c.moves)
            return true;
    }
    return false;
}

} // namespace Legality::Gen3WondercardEggEvent
