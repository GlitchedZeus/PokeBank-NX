#pragma once

#include <cstdint>

namespace Legality::Gen3MethodHSlot {

enum class Type : uint8_t {
    Grass,
    Surf,
    OldRod,
    GoodRod,
    SuperRod,
    RockSmash,
    SwarmFish50,
    SwarmGrass50,
};

inline constexpr uint8_t kInvalid = 0xFF;

struct Range {
    uint8_t min = kInvalid;
    uint8_t max = kInvalid;

    constexpr bool valid() const noexcept {
        return min != kInvalid && max != kInvalid;
    }

    constexpr bool contains(uint8_t roll) const noexcept {
        return valid() && roll >= min && roll <= max;
    }
};

constexpr uint8_t regular(uint32_t roll) noexcept {
    return roll < 20 ? 0 :
           roll < 40 ? 1 :
           roll < 50 ? 2 :
           roll < 60 ? 3 :
           roll < 70 ? 4 :
           roll < 80 ? 5 :
           roll < 85 ? 6 :
           roll < 90 ? 7 :
           roll < 94 ? 8 :
           roll < 98 ? 9 :
           roll < 99 ? 10 :
           roll == 99 ? 11 : kInvalid;
}

constexpr uint8_t surf(uint32_t roll) noexcept {
    return roll < 60 ? 0 :
           roll < 90 ? 1 :
           roll < 95 ? 2 :
           roll < 99 ? 3 :
           roll == 99 ? 4 : kInvalid;
}

constexpr uint8_t oldRod(uint32_t roll) noexcept {
    return roll < 70 ? 0 : roll <= 99 ? 1 : kInvalid;
}

constexpr uint8_t goodRod(uint32_t roll) noexcept {
    return roll < 60 ? 0 : roll < 80 ? 1 : roll <= 99 ? 2 : kInvalid;
}

constexpr uint8_t superRod(uint32_t roll) noexcept {
    return roll < 40 ? 0 :
           roll < 80 ? 1 :
           roll < 95 ? 2 :
           roll < 99 ? 3 :
           roll == 99 ? 4 : kInvalid;
}

// Mirrors pinned PKHeX SlotMethodH.GetSlot. The input is the raw 16-bit RNG
// half; each normal encounter type reduces it modulo 100 before slot selection.
constexpr uint8_t get(Type type, uint32_t rand) noexcept {
    const uint32_t roll = rand % 100u;
    switch (type) {
        case Type::Grass:        return regular(roll);
        case Type::Surf:         return surf(roll);
        case Type::OldRod:       return oldRod(roll);
        case Type::GoodRod:      return goodRod(roll);
        case Type::SuperRod:     return superRod(roll);
        case Type::RockSmash:    return surf(roll);
        case Type::SwarmFish50:
        case Type::SwarmGrass50: return roll < 50 ? 0 : kInvalid;
    }
    return kInvalid;
}

constexpr Range grassRange(uint8_t slot) noexcept {
    switch (slot) {
        case 0: return {0, 19};
        case 1: return {20, 39};
        case 2: return {40, 49};
        case 3: return {50, 59};
        case 4: return {60, 69};
        case 5: return {70, 79};
        case 6: return {80, 84};
        case 7: return {85, 89};
        case 8: return {90, 93};
        case 9: return {94, 97};
        case 10: return {98, 98};
        case 11: return {99, 99};
        default: return {};
    }
}

constexpr Range surfRange(uint8_t slot) noexcept {
    switch (slot) {
        case 0: return {0, 59};
        case 1: return {60, 89};
        case 2: return {90, 94};
        case 3: return {95, 98};
        case 4: return {99, 99};
        default: return {};
    }
}

constexpr Range oldRodRange(uint8_t slot) noexcept {
    switch (slot) {
        case 0: return {0, 69};
        case 1: return {70, 99};
        default: return {};
    }
}

constexpr Range goodRodRange(uint8_t slot) noexcept {
    switch (slot) {
        case 0: return {0, 59};
        case 1: return {60, 79};
        case 2: return {80, 99};
        default: return {};
    }
}

// PKHeX's pinned GetRangeSuperRod table/comments say 40/30/25/4/1, but its
// forward GetSuperRod function says 40/40/15/4/1. The decompiled Emerald
// encounter data independently confirms 40/40/15/4/1, so these inverse ranges
// intentionally follow the game and the forward function rather than the stale
// inverse helper.
constexpr Range superRodRange(uint8_t slot) noexcept {
    switch (slot) {
        case 0: return {0, 39};
        case 1: return {40, 79};
        case 2: return {80, 94};
        case 3: return {95, 98};
        case 4: return {99, 99};
        default: return {};
    }
}

constexpr Range range(Type type, uint8_t slot) noexcept {
    switch (type) {
        case Type::Grass:        return grassRange(slot);
        case Type::Surf:
        case Type::RockSmash:    return surfRange(slot);
        case Type::OldRod:       return oldRodRange(slot);
        case Type::GoodRod:      return goodRodRange(slot);
        case Type::SuperRod:     return superRodRange(slot);
        case Type::SwarmFish50:
        case Type::SwarmGrass50: return slot == 0 ? Range{0, 49} : Range{};
    }
    return {};
}

} // namespace Legality::Gen3MethodHSlot
