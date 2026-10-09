#pragma once

#include <cstdint>
#include <string_view>

namespace Legality::Gen3WishmkrEvent {

constexpr bool matchesTemplate(uint16_t species, uint16_t tid16, uint16_t sid16,
                               uint8_t originGame, uint8_t language,
                               uint8_t otGender, uint8_t metLevel,
                               uint16_t metLocation, uint8_t ball,
                               bool isEgg, bool fateful,
                               std::u16string_view otName) noexcept {
    // Pinned PKHeX EncountersWC3 WISHMKR Jirachi:
    // species 385, level/met level 5, Ruby stored origin, event location 255,
    // TID 20043 / SID 0, English, male OT, OT "WISHMKR", Poke Ball,
    // non-egg and non-fateful. Its PID/IV method is BACD_R, which is
    // validated separately as regular BA-CD with a restricted 16-bit seed.
    return species == 385 &&
           tid16 == 20043 &&
           sid16 == 0 &&
           originGame == 2 &&
           language == 2 &&
           otGender == 0 &&
           metLevel == 5 &&
           metLocation == 255 &&
           ball == 4 &&
           !isEgg &&
           !fateful &&
           otName == u"WISHMKR";
}

} // namespace Legality::Gen3WishmkrEvent
