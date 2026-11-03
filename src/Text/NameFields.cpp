#include "Text/NameFields.h"

#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen2/Gen2StagedEditor.h"
#include "Utils/StringHelpers.h"

#include <array>
#include <vector>

namespace PokeBank::Text {

namespace {
bool cjkLanguage(uint8_t language) noexcept {
    return language == kLanguageJapanese || language == kLanguageKorean;
}
}  // namespace

std::size_t nameMaxCharacters(const FieldContext& c) noexcept {
    const bool pokemonName = c.kind == FieldKind::Nickname;
    switch (c.format) {
        case Format::Gen1:
        case Format::Gen2:
            if (c.japaneseLayout) return 5;
            return pokemonName ? 10 : 7;
        case Format::Gen3:
            if (c.kind == FieldKind::BoxName) return 8;
            if (c.kind == FieldKind::TrainerName) return 7;
            if (c.language == kLanguageJapanese) return 5;
            return pokemonName ? 10 : 7;
        case Format::Gen4:
        case Format::Gen5:
            if (c.kind == FieldKind::BoxName) return 8;
            if (cjkLanguage(c.language)) return 5;
            return pokemonName ? 10 : 7;
    }
    return 0;
}

bool Gen12NameField::encodes(const std::string& utf8, std::size_t maxChars) const {
    std::string error;
    if (format_ == Format::Gen1) {
        namespace G1 = PokeVault::Integration::Gen1;
        std::array<uint8_t, 11> out{};
        return G1::StagedPokemonEditor::encodeRegionName(
            japanese_ ? G1::RegionLayout::Japanese : G1::RegionLayout::International, utf8, maxChars, out, error);
    }
    namespace G2 = PokeVault::Integration::Gen2;
    std::vector<uint8_t> out;
    return G2::encodeGen2Name(japanese_ ? G2::RegionLayout::Japanese : G2::RegionLayout::International,
                              maxChars, utf8, out, error);
}

bool Gen12NameField::glyphAllowed(char32_t c) const noexcept {
    if (c == 0 || c > 0xFFFF || (c >= 0xD800 && c <= 0xDFFF)) return false;
    const std::u16string one(1, static_cast<char16_t>(c));
    return encodes(Utils::utf16ToUtf8(one), maxChars_);
}

TextVerdict Gen12NameField::check16(const std::u16string& text) const {
    TextVerdict v;
    v.characters = static_cast<uint16_t>(codePoints(text));
    if (text.empty()) return allowEmpty_ ? v : refused(Refusal::Empty);
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char16_t c = text[i];
        if (c >= 0xD800 && c <= 0xDFFF) return refused(Refusal::UnsupportedGlyph, i, c);
        if (!glyphAllowed(c)) return refused(Refusal::UnsupportedGlyph, i, c);
    }
    if (text.size() > maxChars_) return refused(Refusal::TooLong, maxChars_);
    if (!encodes(Utils::utf16ToUtf8(text), maxChars_)) return refused(Refusal::RoundTripMismatch);
    v.nativeUnits = static_cast<uint16_t>(text.size() + 1);
    return v;
}

std::unique_ptr<TextFieldCapability> makeNameField(const FieldContext& c) {
    const std::size_t maxChars = nameMaxCharacters(c);
    switch (c.format) {
        case Format::Gen1:
        case Format::Gen2:
            return std::make_unique<Gen12NameField>(c.format, c.kind, c.japaneseLayout, maxChars, c.allowEmpty);
        case Format::Gen3: {
            const std::size_t fieldBytes = c.kind == FieldKind::Nickname ? 10
                                         : c.kind == FieldKind::BoxName  ? 9
                                                                         : 7;
            return std::make_unique<Gen3NameField>(c.kind, c.language, maxChars, fieldBytes, c.allowEmpty);
        }
        case Format::Gen4: {
            const std::size_t slots = c.kind == FieldKind::Nickname ? 11 : c.kind == FieldKind::BoxName ? 9 : 8;
            return std::make_unique<Gen4NameField>(c.kind, c.language, maxChars, slots, c.allowEmpty);
        }
        case Format::Gen5: {
            const std::size_t slots = c.kind == FieldKind::Nickname ? 11 : c.kind == FieldKind::BoxName ? 9 : 8;
            return std::make_unique<Gen5NameField>(c.kind, c.language, maxChars, slots, c.allowEmpty);
        }
    }
    return std::make_unique<Gen3NameField>(c.kind, c.language, maxChars, 10, false);
}

}  // namespace PokeBank::Text
