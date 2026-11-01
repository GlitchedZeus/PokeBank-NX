// Text rules the Gen V (PK5) legality analyzer needs, following PKHeX.Core at the
// commit pinned in Gen5LegalityData.h: Gen V string decoding, species-name lookups
// in every language, and PKHeX's word filter (Switch and 3DS regular expressions
// plus the Gen V dictionary).
//
// .NET invariant casing, char.IsNumber and char.IsWhiteSpace are modelled only for a
// fixed set of UTF-16 units (inDomain); tools/pkhex_gen5_diff/run.sh compares every
// unit of that set with .NET. Callers treat text with any other unit as unknown.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace Legality::Gen5Text {

/// UTF-16 units whose casing, number and white-space classes this module models.
bool inDomain(char16_t c) noexcept;
bool inDomain(std::u16string_view text) noexcept;

/// .NET char.ToLowerInvariant / ToUpperInvariant / IsNumber / IsWhiteSpace for a unit
/// inside the domain (a unit outside it is returned unchanged / reported false).
char16_t toLowerInvariant(char16_t c) noexcept;
char16_t toUpperInvariant(char16_t c) noexcept;
bool isNumber(char16_t c) noexcept;
bool isWhiteSpace(char16_t c) noexcept;

/// PKHeX StringConverter5.LoadString: units up to the first 0xFFFF or 0, with the
/// half-width gender symbols 0x246D / 0x246E read as U+2642 / U+2640.
std::u16string load5(std::span<const uint16_t> units);

/// PKHeX TextNormalizer.Normalize (Switch and 3DS filters).
std::u16string normalizeConsole(std::u16string_view text);
/// PKHeX WordFilter5.Normalize (Gen V dictionary).
std::u16string normalize5(std::u16string_view text);

/// PKHeX Language.GetAvailableGameLanguages for a generation's context (3, 4, 5, 7).
std::span<const uint8_t> gameLanguages(uint8_t generation) noexcept;

/// PKHeX SpeciesName.GetSpeciesName: the name list entry ("" when there is none).
std::u16string_view speciesName(uint16_t species, uint8_t language) noexcept;
/// PKHeX SpeciesName.GetSpeciesNameGeneration for generation 3, 4 or 5.
std::u16string speciesNameGeneration(uint16_t species, uint8_t language, uint8_t generation);
/// PKHeX SpeciesName.GetEggName(language, 5).
std::u16string_view eggName(uint8_t language) noexcept;
/// PKHeX SpeciesName.TryGetSpeciesAnyLanguage (case-sensitive); 0 when no language has it.
uint16_t speciesAnyLanguage(std::u16string_view name, uint8_t generation);
/// PKHeX SpeciesName.TryGetSpeciesAnyLanguageCaseInsensitive; the name must be in the domain.
uint16_t speciesAnyLanguageCaseInsensitive(std::u16string_view name, uint8_t generation);
/// PKHeX SpeciesName.IsNicknamedAnyLanguage.
bool isNicknamedAnyLanguage(uint16_t species, std::u16string_view name, uint8_t generation);

enum class Filter : uint8_t { None, Switch, Console3DS, Gen5 };
struct FilterHit {
    Filter list = Filter::None;
    std::size_t index = 0; // pattern or word index in that list
};
/// PKHeX WordFilter.IsFiltered(message, Gen5, original) with PKHeX's default settings
/// (every past filter checked). `originGeneration` is 3, 4 or 5; the message must be
/// inside the domain.
FilterHit wordFilter(std::u16string_view message, uint8_t originGeneration);
/// PKHeX TrainerNameVerifier.ContainsTooManyNumbers.
/// Newly introduced modern lookback pattern. A hit means historical applicability
/// is unverified; it must never alone produce Invalid for a Gen III-V record.
std::u16string_view unverifiedLookbackPattern(std::u16string_view message, uint8_t originGeneration);
bool tooManyNumbers(std::u16string_view text, uint8_t originGeneration);

/// Pattern of a filter list as stored (lower-cased); empty when out of range.
std::u16string_view filterPattern(Filter list, std::size_t index) noexcept;
/// Number of stored Switch / 3DS patterns the matcher could not parse (expected 0).
std::size_t unsupportedPatterns();

} // namespace Legality::Gen5Text
