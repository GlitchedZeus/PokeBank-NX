#pragma once

#include <cstdint>

namespace Legality::Gen4Release {

// Pinned PKHeX EncounterVerifier restrictions for retail Generation IV.
// Language 8 is Korean in the stored PK4 language field.
inline constexpr uint8_t KoreanLanguage = 8;

// Oak's Letter / Flower Paradise Shaymin was never distributed in Korea.
// Apply this only after a direct Gen IV static encounter identity is proven.
constexpr bool staticEncounterUnreleased(uint16_t species,
                                         uint8_t language) noexcept {
    return species == 492 && language == KoreanLanguage;
}

// Korean Gen IV games never received the Shaymin event that opened Seabreak Path /
// Flower Paradise, so an egg-origin PK4 cannot legitimately hatch at either area.
constexpr bool eggHatchLocationUnreleased(uint8_t language,
                                          uint16_t metLocation) noexcept {
    return language == KoreanLanguage &&
           (metLocation == 63 || metLocation == 85);
}

} // namespace Legality::Gen4Release
