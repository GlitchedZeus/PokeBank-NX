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
        int unownForm = -1;              // 0..27 when PID-derived Unown letter must survive
        bool constrainAbilityBit = true; // false for Gen III duplicate/single-ability semantics
    };

    inline uint8_t genderForPid(uint32_t pid, uint8_t ratio) noexcept {
        if (ratio == 255) return 2;
        if (ratio == 254) return 1;
        if (ratio == 0) return 0;
        return static_cast<uint8_t>((pid & 0xFF) < ratio ? 1 : 0);
    }

    inline uint8_t unownForm(uint32_t pid) noexcept {
        const uint32_t value = ((pid & 0x03000000u) >> 18) | ((pid & 0x00030000u) >> 12)
                             | ((pid & 0x00000300u) >> 6)  | (pid & 0x00000003u);
        return static_cast<uint8_t>(value % 28);
    }

    inline uint32_t withUnownFormValue(uint32_t pid, uint32_t value) noexcept {
        constexpr uint32_t formBits = 0x03030303u;
        return (pid & ~formBits)
             | ((value & 0xC0u) << 18) | ((value & 0x30u) << 12)
             | ((value & 0x0Cu) << 6)  | (value & 0x03u);
    }

    inline uint32_t stampUnownForm(uint32_t pid, int form) noexcept {
        if (form < 0 || form >= 28) return pid;
        uint32_t patterns[10] = {};
        std::size_t count = 0;
        for (uint32_t value = static_cast<uint32_t>(form); value <= 0xFFu; value += 28)
            patterns[count++] = value;
        // Choose the pattern from bits outside the form mask, so stamping is idempotent:
        // stampUnownForm(stampUnownForm(pid, f), f) == stampUnownForm(pid, f).
        const uint32_t stable = pid & ~0x03030303u;
        return withUnownFormValue(pid, patterns[(stable >> 2) % count]);
    }

    inline std::optional<uint32_t> find(uint32_t startPid,
                                        const Traits& wanted,
                                        std::size_t maxAttempts = 1000000) noexcept {
        const uint16_t tsv = static_cast<uint16_t>((wanted.tid32 & 0xFFFF) ^ (wanted.tid32 >> 16));
        uint32_t walk = startPid;
        for (std::size_t i = 0; i < maxAttempts; ++i) {
            uint32_t candidate = wanted.unownForm >= 0 ? stampUnownForm(walk, wanted.unownForm) : walk;
            const uint16_t psv = static_cast<uint16_t>((candidate & 0xFFFF) ^ (candidate >> 16));
            const bool shiny = static_cast<uint16_t>(psv ^ tsv) < 8;
            if ((candidate % 25) == wanted.nature
                && genderForPid(candidate, wanted.genderRatio) == wanted.gender
                && shiny == wanted.shiny
                && (!wanted.constrainAbilityBit || (candidate & 1u) == wanted.abilityBit)
                && (wanted.unownForm < 0 || unownForm(candidate) == wanted.unownForm)) {
                return candidate;
            }
            walk = walk * 0x41C64E6Du + 0x00006073u;
        }
        return std::nullopt;
    }
}

#endif
