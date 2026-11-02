#ifndef POKEBANK_TEXT_TEXT_FIELD_CAPABILITY_H
#define POKEBANK_TEXT_TEXT_FIELD_CAPABILITY_H
// One question the shared keyboard, the editors and every name writer ask a format about one text
// field: can it store this exact text, and if so, what bytes? (docs/architecture/
// SHARED_UX_REQUIREMENTS.md §2.1; Round 14 P0-1 / P0-2 / P0-3.)
//
// THE RULE THIS EXISTS TO ENFORCE: a writer never truncates, substitutes or drops. A name is
// either stored so that decoding it gives back the requested text (Gen III Latin width folded:
// the same bytes are half-width on an international cartridge and full-width on a Japanese one),
// or it is refused with a reason and the destination bytes are untouched. Before this, four
// writers each kept "the part that fit": the Gen III encoder stopped at the first byte the table
// lacked (a FireRed Egg OT "A×B" was exported as "A"), the Gen IV codec cut at the cap and wrote
// '?' for missing glyphs, and the UTF-16 setString cut in code units, splitting a surrogate pair.
//
// The capability is the ONE check: the writers below call check() before touching bytes, so the
// UI's verdict and the writer's cannot disagree. Device build: -fno-rtti -fno-exceptions; no
// virtual dispatch is needed by callers that hold a concrete field, and no allocation happens
// beyond the UTF-8 -> UTF-16 conversion.
#include "Utils/Gen3Text.h"
#include "Utils/Gen4TextCodec.h"
#include "Utils/StringHelpers.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace PokeBank::Text {

enum class FieldKind : uint8_t {
    Nickname, OriginalTrainer, TrainerName, RivalName, BoxName,  // save-native
    BankBoxName, BackupName, SaveFilePath,                       // app-owned
    SearchQuery,                                                 // never stored
};

enum class Refusal : uint8_t {
    None,
    Empty,
    TooLong,
    UnsupportedGlyph,   // a character the format's table has no glyph for (incl. non-BMP)
    MalformedText,      // invalid UTF-8 / lone surrogate / embedded NUL
    FieldReadOnly,
    RoundTripMismatch,  // encode -> decode would not give the requested text back
};

struct TextVerdict {
    Refusal refusal = Refusal::None;
    uint16_t characters = 0;     // code points in the request
    uint16_t nativeUnits = 0;    // bytes (Gen III) or 16-bit units (Gen IV+) the encoding needs
    uint16_t firstBadIndex = 0;  // code-unit index of the first refused character
    char32_t firstBad = 0;
    [[nodiscard]] bool ok() const noexcept { return refusal == Refusal::None; }
};

/// One user-facing sentence per refusal. Callers may prefix the field name.
[[nodiscard]] inline const char* reason(Refusal refusal) noexcept {
    switch (refusal) {
        case Refusal::None: return "";
        case Refusal::Empty: return "The name cannot be empty.";
        case Refusal::TooLong: return "The name is too long for this game.";
        case Refusal::UnsupportedGlyph: return "This game cannot store one of those characters.";
        case Refusal::MalformedText: return "The text is not valid.";
        case Refusal::FieldReadOnly: return "This name cannot be changed in this save.";
        case Refusal::RoundTripMismatch:
            return "The name would not read back unchanged in this game, so nothing was changed.";
    }
    return "The name was refused.";
}

/// UTF-8 -> UTF-16 that refuses rather than skips: Utils::utf8ToUtf16 drops malformed bytes
/// silently, which would itself be a substitution. Returns false (and leaves `out` empty) on any
/// malformed sequence or embedded NUL.
[[nodiscard]] inline bool strictUtf16(std::string_view utf8, std::u16string& out) {
    out.clear();
    const std::string text(utf8);
    if (text.find('\0') != std::string::npos) return false;
    std::u16string wide = Utils::utf8ToUtf16(text);
    if (Utils::utf16ToUtf8(wide) != text) return false;
    out = std::move(wide);
    return true;
}

/// Number of code points in well-formed UTF-16.
[[nodiscard]] inline std::size_t codePoints(const std::u16string& text) noexcept {
    std::size_t count = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char16_t unit = text[i];
        if (unit >= 0xD800 && unit <= 0xDBFF && i + 1 < text.size()) ++i;
        ++count;
    }
    return count;
}

class TextFieldCapability {
public:
    virtual ~TextFieldCapability() = default;
    [[nodiscard]] virtual FieldKind kind() const noexcept = 0;
    [[nodiscard]] virtual bool editable() const noexcept { return true; }
    [[nodiscard]] virtual uint16_t maxCharacters() const noexcept = 0;
    [[nodiscard]] virtual uint16_t maxNativeUnits() const noexcept = 0;
    /// One character in isolation (drives key greying). Agrees with check().
    [[nodiscard]] virtual bool glyphAllowed(char32_t c) const noexcept = 0;
    /// Whole-string validation of UTF-16 text, no side effects.
    [[nodiscard]] virtual TextVerdict check16(const std::u16string& text) const = 0;
    /// Whole-string validation of UTF-8 text (what the keyboard returns), no side effects.
    [[nodiscard]] TextVerdict check(std::string_view utf8) const {
        std::u16string wide;
        if (!strictUtf16(utf8, wide)) return refused(Refusal::MalformedText);
        return check16(wide);
    }

protected:
    [[nodiscard]] static TextVerdict refused(Refusal r, std::size_t index = 0, char32_t c = 0) noexcept {
        TextVerdict v;
        v.refusal = r;
        v.firstBadIndex = static_cast<uint16_t>(index);
        v.firstBad = c;
        return v;
    }
};

// ---------------------------------------------------------------------------------------------
// Gen III (RSE / FRLG): one byte per glyph through the table of the TEXT language (the record's
// own language, except an Egg's OT, which is in the save's charset: Pokemon3FRLG::otTextLanguage),
// 0xFF terminator. `maxChars` is the game's character cap for that language; `fieldBytes` is the
// native field width (a full-width name has no terminator).
// ---------------------------------------------------------------------------------------------
class Gen3NameField final : public TextFieldCapability {
public:
    Gen3NameField(FieldKind kind, uint8_t textLanguage, std::size_t maxChars, std::size_t fieldBytes,
                  bool allowEmpty = true) noexcept
        : kind_(kind), language_(textLanguage),
          maxChars_(maxChars < fieldBytes ? maxChars : fieldBytes), fieldBytes_(fieldBytes),
          allowEmpty_(allowEmpty) {}

    [[nodiscard]] FieldKind kind() const noexcept override { return kind_; }
    [[nodiscard]] uint16_t maxCharacters() const noexcept override { return static_cast<uint16_t>(maxChars_); }
    [[nodiscard]] uint16_t maxNativeUnits() const noexcept override { return static_cast<uint16_t>(fieldBytes_); }
    [[nodiscard]] uint8_t textLanguage() const noexcept { return language_; }
    [[nodiscard]] bool glyphAllowed(char32_t c) const noexcept override {
        return c != 0 && c <= 0xFFFF &&
               Utils::charToGen3(static_cast<char16_t>(c), language_) != Utils::GEN3_TERMINATOR;
    }

    [[nodiscard]] TextVerdict check16(const std::u16string& text) const override {
        TextVerdict v;
        v.characters = static_cast<uint16_t>(codePoints(text));
        if (text.empty() && !allowEmpty_) return refused(Refusal::Empty);
        for (std::size_t i = 0; i < text.size(); ++i) {
            const char16_t c = text[i];
            if (c >= 0xD800 && c <= 0xDFFF) {   // non-BMP: no Gen III glyph
                char32_t cp = c;
                if (c <= 0xDBFF && i + 1 < text.size())
                    cp = 0x10000 + ((static_cast<char32_t>(c) - 0xD800) << 10) + (text[i + 1] - 0xDC00);
                return refused(Refusal::UnsupportedGlyph, i, cp);
            }
            if (!glyphAllowed(c)) return refused(Refusal::UnsupportedGlyph, i, c);
        }
        if (text.size() > maxChars_) return refused(Refusal::TooLong, maxChars_);
        // Read-back: what the cartridge will draw must be the requested name (width folded).
        std::array<uint8_t, 16> scratch{};   // every Gen III name field is at most 10 bytes
        if (fieldBytes_ > scratch.size()) return refused(Refusal::TooLong);
        scratch.fill(Utils::GEN3_TERMINATOR);
        for (std::size_t i = 0; i < text.size(); ++i) scratch[i] = Utils::charToGen3(text[i], language_);
        if (Utils::gen3FoldWidth(decode(std::span<const uint8_t>(scratch.data(), fieldBytes_))) !=
            Utils::gen3FoldWidth(text))
            return refused(Refusal::RoundTripMismatch);
        v.nativeUnits = static_cast<uint16_t>(text.size());
        return v;
    }

    /// All-or-nothing: on refusal `out` is untouched. Writes the glyph bytes and one 0xFF
    /// terminator when the name is shorter than the field; bytes after it are left as they are
    /// (the games leave trash there, and an unedited round trip must not rewrite it).
    [[nodiscard]] TextVerdict encode16(const std::u16string& text, std::span<uint8_t> out) const {
        if (out.size() < fieldBytes_) return refused(Refusal::TooLong);
        const TextVerdict v = check16(text);
        if (!v.ok()) return v;
        for (std::size_t i = 0; i < text.size(); ++i) out[i] = Utils::charToGen3(text[i], language_);
        if (text.size() < fieldBytes_) out[text.size()] = Utils::GEN3_TERMINATOR;
        return v;
    }

    [[nodiscard]] std::u16string decode(std::span<const uint8_t> in) const {
        std::u16string text;
        for (std::size_t i = 0; i < in.size() && i < fieldBytes_; ++i) {
            if (in[i] == Utils::GEN3_TERMINATOR) break;
            if (const char16_t c = Utils::gen3ToChar(in[i], language_)) text += c;
        }
        return text;
    }

private:
    FieldKind kind_;
    uint8_t language_;
    std::size_t maxChars_;
    std::size_t fieldBytes_;
    bool allowEmpty_;
};

// ---------------------------------------------------------------------------------------------
// Gen IV (DPPt / HGSS): one 16-bit table code per character (Utils/Gen4Text.h, international +
// Hangul), 0xFFFF terminator. The game's gender-sign width rule is part of the encoding.
// ---------------------------------------------------------------------------------------------
class Gen4NameField final : public TextFieldCapability {
public:
    Gen4NameField(FieldKind kind, uint8_t language, std::size_t maxChars, std::size_t slotCount,
                  bool allowEmpty = true) noexcept
        : kind_(kind), language_(language),
          maxChars_(slotCount == 0 ? 0 : (maxChars < slotCount - 1 ? maxChars : slotCount - 1)),
          slotCount_(slotCount), allowEmpty_(allowEmpty) {}

    [[nodiscard]] FieldKind kind() const noexcept override { return kind_; }
    [[nodiscard]] uint16_t maxCharacters() const noexcept override { return static_cast<uint16_t>(maxChars_); }
    [[nodiscard]] uint16_t maxNativeUnits() const noexcept override { return static_cast<uint16_t>(slotCount_); }
    [[nodiscard]] bool glyphAllowed(char32_t c) const noexcept override {
        return c != 0 && c <= 0xFFFF && Utils::gen4GlyphEncodable(static_cast<char16_t>(c));
    }
    [[nodiscard]] TextVerdict check16(const std::u16string& text) const override {
        TextVerdict v;
        v.characters = static_cast<uint16_t>(codePoints(text));
        if (text.empty() && !allowEmpty_) return refused(Refusal::Empty);
        for (std::size_t i = 0; i < text.size(); ++i)
            if (!glyphAllowed(text[i])) return refused(Refusal::UnsupportedGlyph, i, text[i]);
        if (text.size() > maxChars_) return refused(Refusal::TooLong, maxChars_);
        const auto encoded = Utils::encodeGen4Field(text, slotCount_, maxChars_, language_);
        if (encoded.size() != slotCount_ * 2 || Utils::decodeGen4Field(encoded) != text)
            return refused(Refusal::RoundTripMismatch);
        v.nativeUnits = static_cast<uint16_t>(text.size());
        return v;
    }

private:
    FieldKind kind_;
    uint8_t language_;
    std::size_t maxChars_;
    std::size_t slotCount_;
    bool allowEmpty_;
};

// ---------------------------------------------------------------------------------------------
// Raw UTF-16LE fields (Gen VII+ legacy Switch titles: nickname / OT / HT / trainer / box names).
// The cap is in UTF-16 CODE UNITS, as the game counts it (PKHeX StringConverter8.SetString), so a
// non-BMP character costs two and is never split; one unit is always reserved for the terminator.
// ---------------------------------------------------------------------------------------------
class Utf16NameField final : public TextFieldCapability {
public:
    Utf16NameField(FieldKind kind, std::size_t maxUnits, std::size_t fieldBytes) noexcept
        : kind_(kind),
          maxUnits_(fieldBytes / 2 == 0 ? 0 : (maxUnits < fieldBytes / 2 - 1 ? maxUnits : fieldBytes / 2 - 1)),
          fieldBytes_(fieldBytes) {}

    [[nodiscard]] FieldKind kind() const noexcept override { return kind_; }
    [[nodiscard]] uint16_t maxCharacters() const noexcept override { return static_cast<uint16_t>(maxUnits_); }
    [[nodiscard]] uint16_t maxNativeUnits() const noexcept override { return static_cast<uint16_t>(maxUnits_); }
    [[nodiscard]] bool glyphAllowed(char32_t c) const noexcept override {
        return c != 0 && c <= 0x10FFFF && !(c >= 0xD800 && c <= 0xDFFF);
    }
    [[nodiscard]] TextVerdict check16(const std::u16string& text) const override {
        TextVerdict v;
        v.characters = static_cast<uint16_t>(codePoints(text));
        for (std::size_t i = 0; i < text.size(); ++i) {
            const char16_t unit = text[i];
            if (unit == 0) return refused(Refusal::MalformedText, i);
            if (unit >= 0xD800 && unit <= 0xDBFF) {
                if (i + 1 >= text.size() || text[i + 1] < 0xDC00 || text[i + 1] > 0xDFFF)
                    return refused(Refusal::MalformedText, i, unit);
                ++i;
            } else if (unit >= 0xDC00 && unit <= 0xDFFF) {
                return refused(Refusal::MalformedText, i, unit);
            }
        }
        if (text.size() > maxUnits_) return refused(Refusal::TooLong, maxUnits_);
        v.nativeUnits = static_cast<uint16_t>(text.size());
        return v;
    }
    /// All-or-nothing write through Utils::setStringChecked: on refusal `dest` is untouched.
    [[nodiscard]] bool write(uint8_t* dest, const std::u16string& text) const {
        if (dest == nullptr || !check16(text).ok()) return false;
        return Utils::setStringChecked(dest, fieldBytes_, text, maxUnits_) ==
               Utils::CheckedTextResult::Accepted;
    }

private:
    FieldKind kind_;
    std::size_t maxUnits_;
    std::size_t fieldBytes_;
};

}  // namespace PokeBank::Text

#endif
