#ifndef UTILS_GEN4_TEXT_CODEC_H
#define UTILS_GEN4_TEXT_CODEC_H

#include "Enums/LanguageID.h"
#include "Utils/Gen4Text.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace Utils {
    inline constexpr char16_t GEN4_MALE = u'\u2642';
    inline constexpr char16_t GEN4_FEMALE = u'\u2640';
    inline constexpr char16_t GEN4_HALF_MALE = u'\u246D';
    inline constexpr char16_t GEN4_HALF_FEMALE = u'\u246E';

    inline char16_t normalizeGen4Gender(char16_t c) noexcept {
        if (c == GEN4_HALF_MALE) return GEN4_MALE;
        if (c == GEN4_HALF_FEMALE) return GEN4_FEMALE;
        return c;
    }
    inline char16_t unnormalizeGen4Gender(char16_t c) noexcept {
        if (c == GEN4_MALE) return GEN4_HALF_MALE;
        if (c == GEN4_FEMALE) return GEN4_HALF_FEMALE;
        return c;
    }
    inline bool gen4IsFullWidth(const std::u16string& value) noexcept {
        for (char16_t c : value) {
            const uint16_t u = static_cast<uint16_t>(c);
            if ((u >> 12) == 0 || (u >> 12) == 0xE) continue;
            if (c == GEN4_MALE || c == GEN4_FEMALE) continue;
            return true;
        }
        return false;
    }
    inline std::u16string decodeGen4Field(std::span<const std::byte> bytes) {
        std::u16string result;
        result.reserve(bytes.size() / 2);
        for (size_t offset = 0; offset + 1 < bytes.size(); offset += 2) {
            const auto* p = reinterpret_cast<const uint8_t*>(bytes.data() + offset);
            const uint16_t stored = static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
            if (stored == 0 || stored == GEN4_TERMINATOR) break;
            const uint16_t decoded = gen4ToChar(stored);
            if (decoded == 0) break;
            result.push_back(normalizeGen4Gender(static_cast<char16_t>(decoded)));
        }
        return result;
    }
    inline uint16_t encodeGen4CodePoint(char16_t c, bool halfWidth) noexcept {
        if (halfWidth) c = unnormalizeGen4Gender(c);
        if (c == u'\u2019') return 0x01B3;
        const uint16_t encoded = charToGen4(static_cast<uint16_t>(c));
        return encoded == GEN4_TERMINATOR ? 0x01AC : encoded;
    }
    inline std::vector<std::byte> encodeGen4Field(const std::u16string& value,
                                                  size_t slotCount,
                                                  size_t maxCharacters,
                                                  uint8_t language) {
        std::vector<std::byte> out(slotCount * 2, std::byte{0});
        if (slotCount == 0) return out;
        const bool halfWidth =
            language == static_cast<uint8_t>(Enums::LanguageID::Korean) || !gen4IsFullWidth(value);
        const size_t count = std::min({value.size(), maxCharacters, slotCount - 1});
        for (size_t i = 0; i < count; ++i) {
            const uint16_t encoded = encodeGen4CodePoint(value[i], halfWidth);
            out[i * 2] = static_cast<std::byte>(encoded & 0xFF);
            out[i * 2 + 1] = static_cast<std::byte>(encoded >> 8);
        }
        out[count * 2] = std::byte{0xFF};
        out[count * 2 + 1] = std::byte{0xFF};
        return out;
    }
}

#endif
