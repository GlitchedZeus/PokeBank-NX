/**
 * ModernNickname.h - the un-nicknamed name rule shared by the Gen VI-IX analyzers.
 *
 * NicknameVerifier.VerifyUnNicknamed: a record whose nickname flag is clear must carry its
 * species' name. Species names, the Generation 7 Chinese renames, the Generation 6/7 language
 * lists and the species whose encounters relax the rule (fixed-nickname trades and gifts, and
 * fixed-language mystery gifts, with every evolution of them) are generated from the pinned
 * PKHeX.Core by tools/pkhex_gen67_diff/oracle ("dumpnames") into ModernNicknameData.inc.
 */
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace Legality::ModernNickname {

/// Which string codec and name rules apply.
enum class Family : uint8_t {
    Gen6,     // PK6: StringConverter6, the name may be any language's once evolved or a traded egg
    Gen7,     // PK7: StringConverter7 (Chinese private glyphs), as Gen6 plus Chinese
    Beluga,   // PB7: plain UTF-16, the record's language, Generation 7 names
    Switch8,  // PK8, PB8, PA8: plain UTF-16, the record's language
    Switch9,  // PK9, PA9: plain UTF-16, the record's language
};

enum class Result : uint8_t {
    NoIssue,       // matches, or the rule does not decide this record
    Empty,         // NickLengthShort: no characters at all
    NotSpecies,    // NickMatchLanguageFail: not the species name in the record's language
    NotAnyName,    // NickMatchLanguageFail: not the species name in any of the game's languages
};

struct Input {
    Family family;
    std::span<const uint8_t> nickname;  // the 26-byte nickname field
    uint16_t species;                   // national species id
    uint8_t language;
    bool isEgg;
    bool isNicknamed;
    uint16_t eggLocation;               // PK6/PK7 only
};

Result check(const Input& in) noexcept;

/// The Invalid text for a result, or nullptr for NoIssue.
const char* message(Result result) noexcept;

/// The species' name in a language (Generation 6 and 8+ spelling); empty when unknown.
std::u16string_view speciesName(uint16_t species, uint8_t language) noexcept;

/// Whether an encounter of the species' line relaxes the rule (the rule then never decides it).
bool relaxed(uint16_t species) noexcept;

} // namespace Legality::ModernNickname
