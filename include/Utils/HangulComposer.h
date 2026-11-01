#ifndef UTILS_HANGUL_COMPOSER_H
#define UTILS_HANGUL_COMPOSER_H

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// Hangul syllable composition for the in-app keyboard's Korean page (Round 11 H27), the standard
// two-set (Dubeolsik) behaviour: each key is one compatibility jamo (U+3131-U+3163), and a key
// typed right after a Hangul character the same keyboard run produced combines with it --
//   ㄱ + ㅏ -> 가,  가 + ㄱ -> 각,  각 + ㅅ -> 갃,  ㅗ-syllable + ㅏ -> ㅘ-syllable,
//   각 + ㅏ -> 가가 (a final consonant moves to the next syllable), 닭 + ㅏ -> 달가.
// It only builds Unicode text. Whether a game can STORE that text is the writer's decision (Gen IV:
// Pokemon4Mutable round-trips through the Gen 4 character table and refuses the rest). The keyboard
// offers this page only for a Gen IV Korean-language Pokemon (see Utils::promptKoreanName).
namespace Utils::Hangul {

inline constexpr char32_t kSyllableBase = 0xAC00;
inline constexpr char32_t kSyllableLast = 0xD7A3;
inline constexpr int kVowelCount = 21;
inline constexpr int kFinalCount = 28;

// Compatibility jamo for the 19 initials, 21 vowels and 27 finals (index 0 of finals = none).
inline constexpr std::array<char32_t, 19> kInitials{{
    0x3131, 0x3132, 0x3134, 0x3137, 0x3138, 0x3139, 0x3141, 0x3142, 0x3143, 0x3145,
    0x3146, 0x3147, 0x3148, 0x3149, 0x314A, 0x314B, 0x314C, 0x314D, 0x314E}};
inline constexpr std::array<char32_t, kFinalCount> kFinals{{
    0,      0x3131, 0x3132, 0x3133, 0x3134, 0x3135, 0x3136, 0x3137, 0x3139, 0x313A,
    0x313B, 0x313C, 0x313D, 0x313E, 0x313F, 0x3140, 0x3141, 0x3142, 0x3144, 0x3145,
    0x3146, 0x3147, 0x3148, 0x314A, 0x314B, 0x314C, 0x314D, 0x314E}};
inline constexpr char32_t kFirstVowel = 0x314F;   // ㅏ; vowels are contiguous to ㅣ U+3163

struct Pair { char32_t first, second, combined; };
// Final + consonant -> double final.
inline constexpr std::array<Pair, 11> kDoubleFinals{{
    {0x3131, 0x3145, 0x3133}, {0x3134, 0x3148, 0x3135}, {0x3134, 0x314E, 0x3136},
    {0x3139, 0x3131, 0x313A}, {0x3139, 0x3141, 0x313B}, {0x3139, 0x3142, 0x313C},
    {0x3139, 0x3145, 0x313D}, {0x3139, 0x314C, 0x313E}, {0x3139, 0x314D, 0x313F},
    {0x3139, 0x314E, 0x3140}, {0x3142, 0x3145, 0x3144}}};
// Vowel + vowel -> compound vowel.
inline constexpr std::array<Pair, 7> kCompoundVowels{{
    {0x3157, 0x314F, 0x3158}, {0x3157, 0x3150, 0x3159}, {0x3157, 0x3163, 0x315A},
    {0x315C, 0x3153, 0x315D}, {0x315C, 0x3154, 0x315E}, {0x315C, 0x3163, 0x315F},
    {0x3161, 0x3163, 0x3162}}};

[[nodiscard]] constexpr int indexOf(const auto& table, char32_t c, int from = 0) noexcept {
    for (int i = from; i < static_cast<int>(table.size()); ++i)
        if (table[static_cast<size_t>(i)] == c) return i;
    return -1;
}
[[nodiscard]] constexpr int initialIndex(char32_t c) noexcept { return indexOf(kInitials, c); }
[[nodiscard]] constexpr int finalIndex(char32_t c) noexcept { return c == 0 ? -1 : indexOf(kFinals, c, 1); }
[[nodiscard]] constexpr int vowelIndex(char32_t c) noexcept {
    return c >= kFirstVowel && c < kFirstVowel + kVowelCount ? static_cast<int>(c - kFirstVowel) : -1;
}
[[nodiscard]] constexpr bool isJamoKey(char32_t c) noexcept { return initialIndex(c) >= 0 || vowelIndex(c) >= 0; }
[[nodiscard]] constexpr bool isSyllable(char32_t c) noexcept { return c >= kSyllableBase && c <= kSyllableLast; }
[[nodiscard]] constexpr char32_t syllable(int initial, int vowel, int final) noexcept {
    return kSyllableBase + static_cast<char32_t>((initial * kVowelCount + vowel) * kFinalCount + final);
}
[[nodiscard]] constexpr char32_t combine(const auto& pairs, char32_t first, char32_t second) noexcept {
    for (const Pair& p : pairs)
        if (p.first == first && p.second == second) return p.combined;
    return 0;
}

// What `previous` (the character before the caret, typed in this run) becomes when `jamo` is typed
// after it: one or two characters that replace it, or nothing when `jamo` is simply appended.
[[nodiscard]] inline std::optional<std::u32string> compose(char32_t previous, char32_t jamo) {
    const int v = vowelIndex(jamo);
    if (isSyllable(previous)) {
        const int index = static_cast<int>(previous - kSyllableBase);
        const int l = index / (kVowelCount * kFinalCount);
        const int pv = (index / kFinalCount) % kVowelCount;
        const int t = index % kFinalCount;
        if (v < 0) {
            // A consonant becomes (or extends) the final.
            const char32_t next = t == 0 ? jamo : combine(kDoubleFinals, kFinals[static_cast<size_t>(t)], jamo);
            const int nt = next ? finalIndex(next) : -1;
            if (nt < 0) return std::nullopt;
            return std::u32string(1, syllable(l, pv, nt));
        }
        if (t == 0) {
            const char32_t compound = combine(kCompoundVowels, kFirstVowel + static_cast<char32_t>(pv), jamo);
            if (!compound) return std::nullopt;
            return std::u32string(1, syllable(l, vowelIndex(compound), 0));
        }
        // A vowel after a final: the final (or a double final's second half) starts a new syllable.
        int keep = 0;
        char32_t moved = kFinals[static_cast<size_t>(t)];
        for (const Pair& p : kDoubleFinals)
            if (p.combined == moved) { keep = finalIndex(p.first); moved = p.second; break; }
        const int nl = initialIndex(moved);
        if (nl < 0) return std::nullopt;
        return std::u32string{syllable(l, pv, keep), syllable(nl, v, 0)};
    }
    if (v >= 0) {
        if (const int l = initialIndex(previous); l >= 0) return std::u32string(1, syllable(l, v, 0));
        if (vowelIndex(previous) >= 0)
            if (const char32_t compound = combine(kCompoundVowels, previous, jamo))
                return std::u32string(1, compound);
    }
    return std::nullopt;
}

// UTF-8 helpers for the keyboard, which keeps its text as UTF-8.
[[nodiscard]] inline char32_t decodeOne(std::string_view s) noexcept {
    if (s.empty()) return 0;
    const auto b = [&](size_t i) { return static_cast<char32_t>(static_cast<unsigned char>(s[i])); };
    if (b(0) < 0x80) return s.size() == 1 ? b(0) : 0;
    if ((b(0) & 0xE0) == 0xC0 && s.size() == 2) return ((b(0) & 0x1F) << 6) | (b(1) & 0x3F);
    if ((b(0) & 0xF0) == 0xE0 && s.size() == 3)
        return ((b(0) & 0x0F) << 12) | ((b(1) & 0x3F) << 6) | (b(2) & 0x3F);
    return 0;   // Hangul and jamo are all three-byte; anything else never composes
}
[[nodiscard]] inline std::string encode(std::u32string_view text) {
    std::string out;
    for (const char32_t c : text) {
        if (c < 0x80) out.push_back(static_cast<char>(c));
        else if (c < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (c >> 6)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (c >> 12)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
    }
    return out;
}

// The Korean page, two-set layout: three rows of jamo, lower then shifted (tense consonants and
// ㅒ / ㅖ). The rows are what KeyboardDialog lays out; every key is a jamo compose() accepts.
struct Row { std::string_view lower, upper; };
inline constexpr std::array<Row, 3> kRows{{
    {"ㅂㅈㄷㄱㅅㅛㅕㅑㅐㅔ", "ㅃㅉㄸㄲㅆㅛㅕㅑㅒㅖ"},
    {"ㅁㄴㅇㄹㅎㅗㅓㅏㅣ", "ㅁㄴㅇㄹㅎㅗㅓㅏㅣ"},
    {"ㅋㅌㅊㅍㅠㅜㅡ", "ㅋㅌㅊㅍㅠㅜㅡ"},
}};

} // namespace Utils::Hangul

#endif
