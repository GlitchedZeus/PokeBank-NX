// ModernNickname.cpp - NicknameVerifier's un-nicknamed name rule for the Gen VI-IX analyzers.
// The tables are include/Legality/ModernNicknameData.inc, written by
// tools/pkhex_gen67_diff/oracle ("dumpnames") and checked against the pinned PKHeX in CI.
#include "Legality/ModernNickname.h"

#include <array>
#include <string_view>

namespace Legality::ModernNickname {
namespace {

struct NameOverride {
    uint8_t language;
    uint16_t species;
    const char16_t* name;
};

#include "Legality/ModernNicknameData.inc"

constexpr size_t kNicknameChars = 13;  // TrashCharCountNickname

bool bit(std::span<const uint8_t> bits, uint16_t index) noexcept {
    return (index >> 3) < bits.size() && ((bits[index >> 3] >> (index & 7)) & 1) != 0;
}

// SpeciesName.GetSpeciesNameGeneration for generation 6, 7 or 8+.
std::u16string_view name(uint16_t species, uint8_t language, bool gen7) noexcept {
    if (gen7) {
        for (const auto& o : kGen7Names)
            if (o.language == language && o.species == species) return o.name;
    }
    return speciesName(species, language);
}

} // namespace

std::u16string_view speciesName(uint16_t species, uint8_t language) noexcept {
    if (species > kMaxSpeciesNames || language >= 12) return {};
    return kSpeciesNames[language][species];
}

bool relaxed(uint16_t species) noexcept { return species > kMaxSpeciesNames || bit(kSpecialNickname, species); }

const char* message(Result result) noexcept {
    switch (result) {
    case Result::Empty: return "Nickname is empty";
    case Result::NotSpecies: return "The nickname flag is clear but the nickname is not the species' name in the record's language";
    case Result::NotAnyName: return "The nickname flag is clear but the nickname is not the species' name in any language";
    case Result::NoIssue: break;
    }
    return nullptr;
}

Result check(const Input& in) noexcept {
    if (in.nickname.size() < kNicknameChars * 2) return Result::NoIssue;
    // PKM.LoadString: StringConverter6/7 turn the half-width gender glyphs into U+2642/U+2640;
    // StringConverter7 also maps its private Chinese glyphs, which this rule leaves undecided.
    std::array<char16_t, kNicknameChars> text{};
    size_t length = 0;
    bool privateGlyph = false;
    const bool gen67 = in.family == Family::Gen6 || in.family == Family::Gen7;
    for (; length < kNicknameChars; ++length) {
        auto c = static_cast<char16_t>(in.nickname[2 * length] | (in.nickname[2 * length + 1] << 8));
        if (c == 0) break;
        if (gen67 && c == 0xE08E) c = 0x2642;
        if (gen67 && c == 0xE08F) c = 0x2640;
        if (in.family == Family::Gen7 && c >= kPrivateZh7First && c <= kPrivateZh7Last) privateGlyph = true;
        text[length] = c;
    }
    if (length == 0) return Result::Empty;  // NickLengthShort, whatever the flags
    if (in.isNicknamed || in.isEgg || privateGlyph) return Result::NoIssue;
    const uint8_t maxLanguage = in.family == Family::Switch9 ? 11 : 10;
    if (in.species == 0 || relaxed(in.species) || in.language == 0 || in.language == 6 || in.language > maxLanguage)
        return Result::NoIssue;

    const std::u16string_view nickname(text.data(), length);
    const bool gen7 = in.family == Family::Gen7 || in.family == Family::Beluga;
    if (nickname == name(in.species, in.language, gen7)) return Result::NoIssue;

    if (in.family == Family::Switch8 || in.family == Family::Switch9) {
        // Farfetch'd and Sirfetch'd imported through HOME keep an ASCII apostrophe.
        if ((in.species == 83 || in.species == 865) && (in.language == 2 || in.language == 4 || in.language == 7)) {
            std::u16string ascii(name(in.species, in.language, false));
            for (auto& c : ascii)
                if (c == u'’') c = u'\'';
            if (nickname == ascii) return Result::NoIssue;
        }
        return Result::NotSpecies;
    }
    if (in.family == Family::Beluga) return Result::NotSpecies;

    // PK6/PK7: an evolved record or a traded egg may carry any language's name. A species with
    // no pre-evolution in any game, never an egg, was never evolved.
    if (bit(kNoPreEvolution, in.species) && in.eggLocation == 0) return Result::NotSpecies;
    const uint16_t languages = in.family == Family::Gen6 ? kLanguages6 : kLanguages7;
    for (uint8_t l = 1; l < 12; ++l)
        if (((languages >> l) & 1) != 0 && nickname == name(in.species, l, gen7)) return Result::NoIssue;
    return Result::NotAnyName;
}

bool eggNameMatches(Family family, std::span<const uint8_t> nickname, uint8_t language) noexcept {
    if (nickname.size() < kNicknameChars * 2 || language >= 12) return true;
    const std::u16string_view expect(kEggNames[language]);
    if (expect.empty()) return true;
    size_t length = 0;
    bool same = true;
    for (; length < kNicknameChars; ++length) {
        auto c = static_cast<char16_t>(nickname[2 * length] | (nickname[2 * length + 1] << 8));
        if (c == 0) break;
        if (family == Family::Gen7 && (language == 9 || language == 10) && c == kEggNameZh7Stored) c = expect[0];
        if (length >= expect.size() || c != expect[length]) same = false;
    }
    return same && length == expect.size();
}

} // namespace Legality::ModernNickname
