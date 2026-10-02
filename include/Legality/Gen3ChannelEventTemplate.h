#pragma once

#include <cstdint>
#include <string_view>

namespace Legality::Gen3ChannelEvent {

constexpr bool isRubySapphireOrigin(uint8_t originGame) noexcept {
    // PK3 stored version values used by the existing Channel RNG matcher:
    // 1 = Sapphire, 2 = Ruby.
    return originGame == 1 || originGame == 2;
}

constexpr bool matchesTemplate(uint16_t species, uint16_t tid16,
                               uint8_t originGame, uint8_t metLevel,
                               uint16_t metLocation, uint8_t ball,
                               bool isEgg, bool fateful,
                               std::u16string_view otName) noexcept {
    // Pinned PKHeX EncountersWC3 Channel Jirachi:
    // species 385, level-5 distribution with stored met level 0,
    // event met location 255, TID 40122, OT "CHANNEL", Ruby/Sapphire,
    // Poke Ball, non-egg, non-fateful. SID and OT gender are RNG-derived
    // and are checked by Gen3ChannelPidIvCorrelation.
    return species == 385 &&
           tid16 == 40122 &&
           isRubySapphireOrigin(originGame) &&
           metLevel == 0 &&
           metLocation == 255 &&
           ball == 4 &&
           !isEgg &&
           !fateful &&
           otName == u"CHANNEL";
}

} // namespace Legality::Gen3ChannelEvent
