#pragma once

#include <cstdint>

namespace Legality::Gen4CuteCharmPid {

struct EncounterIdentity {
    uint16_t species = 0;
    uint8_t gender = 2;
    bool deriveGenderFromPid = false;
};

constexpr bool isAzurillBufferedMale(uint32_t pid) noexcept {
    return pid >= 0xC8u && pid <= 0xE0u;
}

constexpr uint8_t genderFromPid(uint32_t pid, uint8_t genderRatio) noexcept {
    if (genderRatio == 0)
        return 0;
    if (genderRatio == 0xFE)
        return 1;
    if (genderRatio == 0xFF)
        return 2;
    return static_cast<uint8_t>((pid & 0xFFu) < genderRatio ? 1 : 0);
}

// Mirrors PKHeX CuteCharm4's Gen IV evolution/gender-ratio edge handling.
constexpr EncounterIdentity remapEncounterIdentity(
    uint16_t currentSpecies, uint8_t currentGender, uint32_t pid) noexcept {
    switch (currentSpecies) {
        case 292: return {290, 2, true};  // Shedinja <- Nincada
        case 413: return {412, 1, false}; // Wormadam <- Burmy
        case 414: return {412, 0, false}; // Mothim <- Burmy
        case 416: return {415, 1, false}; // Vespiquen <- Combee
        case 475: return {281, 0, false}; // Gallade <- Kirlia
        case 478: return {361, 1, false}; // Froslass <- Snorunt
        case 183:
        case 184:
            if (isAzurillBufferedMale(pid))
                return {298, 0, false}; // Marill/Azumarill <- male Azurill collision
            break;
        default:
            break;
    }
    return {currentSpecies, currentGender, false};
}

constexpr uint32_t expectedMalePid(uint8_t genderRatio, uint8_t nature) noexcept {
    constexpr uint32_t NatureCount = 25;
    const uint32_t base = NatureCount *
        ((static_cast<uint32_t>(genderRatio) / NatureCount) + 1u);
    return base + nature;
}

// This checks the deterministic Cute Charm PID surface only.
// Method J/K encounter-slot and lead-frame validation remains a separate layer.
constexpr bool matchesSurface(uint32_t pid, uint8_t gender,
                              uint8_t genderRatio) noexcept {
    if (pid > 0xFFu)
        return false;
    if (genderRatio >= 0xFEu)
        return false;

    const uint8_t nature = static_cast<uint8_t>(pid % 25u);
    if (gender == 0)
        return pid == expectedMalePid(genderRatio, nature);
    if (gender == 1)
        return pid < 25u;
    return false;
}

} // namespace Legality::Gen4CuteCharmPid
