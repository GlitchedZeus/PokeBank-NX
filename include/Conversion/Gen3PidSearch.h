#ifndef CONVERSION_GEN3_PID_SEARCH_H
#define CONVERSION_GEN3_PID_SEARCH_H

#include <cstddef>
#include <cstdint>
#include <optional>

namespace Conversion::Gen3PidSearch {
    struct Traits {
        uint8_t nature = 0;       // 0..24
        uint8_t gender = 0;       // 0 male, 1 female, 2 genderless
        uint8_t genderRatio = 0;  // Gen III personal ratio
        uint8_t abilityBit = 0;   // PID bit 0: 0 slot 1, 1 slot 2
        uint32_t tid32 = 0;
        bool shiny = false;
    };

    inline uint8_t genderForPid(uint32_t pid, uint8_t ratio) noexcept {
        if (ratio == 255) return 2;
        if (ratio == 254) return 1;
        if (ratio == 0) return 0;
        return static_cast<uint8_t>((pid & 0xFF) < ratio ? 1 : 0);
    }

    inline std::optional<uint32_t> find(uint32_t startPid,
                                        const Traits& wanted,
                                        std::size_t maxAttempts = 1000000) noexcept {
        const uint16_t tsv = static_cast<uint16_t>((wanted.tid32 & 0xFFFF) ^ (wanted.tid32 >> 16));
        uint32_t candidate = startPid;
        for (std::size_t i = 0; i < maxAttempts; ++i) {
            const uint16_t psv = static_cast<uint16_t>((candidate & 0xFFFF) ^ (candidate >> 16));
            const bool shiny = static_cast<uint16_t>(psv ^ tsv) < 8;
            if ((candidate % 25) == wanted.nature
                && genderForPid(candidate, wanted.genderRatio) == wanted.gender
                && shiny == wanted.shiny
                && (candidate & 1u) == wanted.abilityBit) {
                return candidate;
            }
            candidate = candidate * 0x41C64E6Du + 0x00006073u;
        }
        return std::nullopt;
    }
}

#endif
