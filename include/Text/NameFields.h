#ifndef POKEBANK_TEXT_NAME_FIELDS_H
#define POKEBANK_TEXT_NAME_FIELDS_H
// The ONE place a Gen I-V save-native name field's rules are chosen (docs/architecture/
// SHARED_UX_REQUIREMENTS.md §2.1 / P1-1). Every name prompt asks makeNameField() for the field it
// edits and hands the result to Utils::promptName, so the keyboard's pages, its disabled keys, its
// live length / glyph verdict and the System keyboard's check all come from the same object the
// writer's rule lives in. No table is copied here: each field wraps the codec its writer uses.
//
//   Gen I / II   Gen12NameField -> Gen1 StagedPokemonEditor::encodeRegionName / Gen2 encodeGen2Name
//                (international Latin + accents; Japanese = Utils::Gen12Kana, 5 characters; the
//                Japanese editors stay closed by Integration/Common/JapaneseEditingGate.h).
//   Gen III      Gen3NameField  (GEN3_JP / GEN3_EN by the TEXT language)
//   Gen IV       Gen4NameField  (Gen 4 code page incl. Hangul)
//   Gen V        Gen5NameField  (UTF-16, repertoire = the Gen 4 code page; see TextFieldCapability.h)
#include "Text/TextFieldCapability.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace PokeBank::Text {

enum class Format : uint8_t { Gen1, Gen2, Gen3, Gen4, Gen5 };

struct FieldContext {
    Format format = Format::Gen3;
    FieldKind kind = FieldKind::Nickname;
    /// Enums::LanguageID of the TEXT: the record's own language for a Pokemon's nickname, the
    /// OT text language for an OT (Gen III Egg: the save's charset), the save's language for a
    /// trainer name. Ignored by Gen I / II, which key on `japaneseLayout`.
    uint8_t language = 2;
    /// Gen I / II: the save is the Japanese layout (kana font, 5-character names).
    bool japaneseLayout = false;
    /// An empty answer is meaningful (a Create nickname left blank takes the species name).
    bool allowEmpty = false;
};

/// Native character caps (PKHeX 8e3eb3d4: PK1/PK2/PK3 MaxStringLength*, Legal.GetMaxLength-
/// Nickname / GetMaxLengthOT for Gen IV / V -- Japanese and Korean names hold 5 characters).
[[nodiscard]] std::size_t nameMaxCharacters(const FieldContext& context) noexcept;

/// Gen I / II names through the generation's own encoder.
class Gen12NameField final : public TextFieldCapability {
public:
    Gen12NameField(Format format, FieldKind kind, bool japanese, std::size_t maxChars,
                   bool allowEmpty = false) noexcept
        : format_(format), kind_(kind), japanese_(japanese), maxChars_(maxChars), allowEmpty_(allowEmpty) {}
    [[nodiscard]] FieldKind kind() const noexcept override { return kind_; }
    [[nodiscard]] uint16_t maxCharacters() const noexcept override { return static_cast<uint16_t>(maxChars_); }
    /// One byte per character plus the 0x50 terminator.
    [[nodiscard]] uint16_t maxNativeUnits() const noexcept override { return static_cast<uint16_t>(maxChars_ + 1); }
    [[nodiscard]] bool glyphAllowed(char32_t c) const noexcept override;
    [[nodiscard]] TextVerdict check16(const std::u16string& text) const override;
    /// Japanese: kana pages only (the Gen I / II Japanese font has no Latin letters). International:
    /// Latin, the symbols the font holds, and the accents of the European releases.
    [[nodiscard]] KeyboardProfile keyboard() const noexcept override {
        return japanese_ ? japaneseProfile(false) : latinProfile();
    }
    [[nodiscard]] bool japanese() const noexcept { return japanese_; }

private:
    [[nodiscard]] bool encodes(const std::string& utf8, std::size_t maxChars) const;
    Format format_;
    FieldKind kind_;
    bool japanese_;
    std::size_t maxChars_;
    bool allowEmpty_;
};

/// The capability of one save-native name field. Never null for a Gen I-V name kind.
[[nodiscard]] std::unique_ptr<TextFieldCapability> makeNameField(const FieldContext& context);

}  // namespace PokeBank::Text

#endif
