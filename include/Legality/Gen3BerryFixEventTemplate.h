#pragma once

#include <cstdint>
#include <string_view>

namespace Legality::Gen3BerryFixEvent {

constexpr bool matchesTemplate(uint16_t species, uint16_t tid16, uint16_t sid16,
                               uint8_t originGame, uint8_t language,
                               uint8_t otGender, uint8_t metLevel,
                               uint16_t metLocation, uint8_t ball,
                               bool isEgg, bool fateful,
                               std::u16string_view otName) noexcept {
    if (species != 263 || sid16 != 0 || originGame != 1 ||
        metLevel != 5 || metLocation != 255 || ball != 4 ||
        isEgg || fateful)
        return false;

    if (language == 2 && tid16 == 30317) {
        return (otGender == 1 && otName == u"RUBY") ||
               (otGender == 0 && otName == u"SAPHIRE");
    }

    if (language == 1 && tid16 == 21121) {
        return (otGender == 1 && otName == u"ルビー") ||
               (otGender == 0 && otName == u"サファイア");
    }

    return false;
}

} // namespace Legality::Gen3BerryFixEvent
