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
    /// One character -> its Gen 4 code, or GEN4_TERMINATOR when the game has no glyph for it.
    /// (Round 14 P0-2: this used to answer the game's '?' code, so a name with one missing glyph
    /// was stored with a question mark in its place. A writer now refuses instead.)
    inline uint16_t encodeGen4CodePoint(char16_t c, bool halfWidth) noexcept {
        if (halfWidth) c = unnormalizeGen4Gender(c);
        if (c == u'\u2019') return 0x01B3;
        return charToGen4(static_cast<uint16_t>(c));
    }
    /// True when the character has a Gen 4 glyph in at least one width.
    inline bool gen4GlyphEncodable(char16_t c) noexcept {
        return encodeGen4CodePoint(c, false) != GEN4_TERMINATOR ||
               encodeGen4CodePoint(c, true) != GEN4_TERMINATOR;
    }
    /// All-or-nothing field encoder (PKHeX StringConverter4 ClearZero shape: the text, one 0xFFFF,
    /// then zeros). Returns an EMPTY vector -- never a prefix, never a '?' -- when the text is longer
    /// than `maxCharacters` (or the field), or any character has no glyph. Callers must treat an
    /// empty result as a refusal and leave the destination untouched.
    inline std::vector<std::byte> encodeGen4Field(const std::u16string& value,
                                                  size_t slotCount,
                                                  size_t maxCharacters,
                                                  uint8_t language) {
        if (slotCount == 0 || value.size() > maxCharacters || value.size() > slotCount - 1) return {};
        std::vector<std::byte> out(slotCount * 2, std::byte{0});
        const size_t count = value.size();
        const bool halfWidth = language == static_cast<uint8_t>(Enums::LanguageID::Korean) ||
            !gen4IsFullWidth(value);
        for (size_t i = 0; i < count; ++i) {
            const uint16_t encoded = encodeGen4CodePoint(value[i], halfWidth);
            if (encoded == GEN4_TERMINATOR) return {};
            out[i * 2] = static_cast<std::byte>(encoded & 0xFF);
            out[i * 2 + 1] = static_cast<std::byte>(encoded >> 8);
        }
        out[count * 2] = std::byte{0xFF};
        out[count * 2 + 1] = std::byte{0xFF};
        return out;
    }
}

#endif
