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

/// Which keyboard pages a field offers (docs/architecture/SHARED_UX_REQUIREMENTS.md §2.1). The
/// pages come from the record's (or save's) LANGUAGE -- the script that game's own name screen
/// offers -- and the keyboard still disables every key glyphAllowed() refuses, so a page never
/// offers a character the field cannot store.
struct KeyboardProfile {
    enum class Start : uint8_t { Latin, Kana, Hangul };
    bool latin = true;            ///< ABC page (QWERTY)
    bool symbols = true;          ///< ?123 page
    bool accents = true;          ///< àé page
    bool kana = false;            ///< あ / ア / １２３ pages (Utils/Gen12KanaText.h layout)
    bool hangul = false;          ///< 한 page: two-set jamo composed into syllables
    bool fullWidthLatin = false;  ///< ABC / ?123 keys type the full-width forms (Ａ, １)
    bool fullWidthSpace = false;  ///< Space types U+3000
    Start start = Start::Latin;
};

/// The pages of a Japanese, Korean or international record's name screen.
[[nodiscard]] inline KeyboardProfile japaneseProfile(bool withLatin) noexcept {
    KeyboardProfile p;
    p.kana = true;
    p.latin = withLatin;
    p.symbols = withLatin;
    p.accents = false;
    p.fullWidthLatin = withLatin;
    p.fullWidthSpace = true;
    p.start = KeyboardProfile::Start::Kana;
    return p;
}
[[nodiscard]] inline KeyboardProfile koreanProfile() noexcept {
    KeyboardProfile p;
    p.hangul = true;
    p.start = KeyboardProfile::Start::Hangul;
    return p;
}
[[nodiscard]] inline KeyboardProfile latinProfile(bool accents = true) noexcept {
    KeyboardProfile p;
    p.accents = accents;
    return p;
}

/// Enums::LanguageID values the profiles key on.
inline constexpr uint8_t kLanguageJapanese = 1;
inline constexpr uint8_t kLanguageKorean = 8;

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
    /// The keyboard pages for this field in this record's / save's language.
    [[nodiscard]] virtual KeyboardProfile keyboard() const noexcept { return latinProfile(); }
    /// True when every code point of `utf8` passes glyphAllowed() (an empty string passes).
    /// Drives key greying: a key whose label this answers false for is drawn disabled.
    [[nodiscard]] bool glyphsAllowed(std::string_view utf8) const {
        std::u16string wide;
        if (!strictUtf16(utf8, wide)) return false;
        for (std::size_t i = 0; i < wide.size(); ++i) {
            char32_t c = wide[i];
            if (c >= 0xD800 && c <= 0xDBFF && i + 1 < wide.size())
                c = 0x10000 + ((c - 0xD800) << 10) + (wide[++i] - 0xDC00);
            if (!glyphAllowed(c)) return false;
        }
        return true;
    }
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
    /// GEN3_JP is a kana + FULL-WIDTH Latin table, so a Japanese record's name screen is kana and
    /// full-width ABC (owner Round 13 decision 1); GEN3_EN is Latin with accents. Gen III has no
    /// Korean game: never Hangul.
    [[nodiscard]] KeyboardProfile keyboard() const noexcept override {
        return Utils::gen3IsJapanese(language_) ? japaneseProfile(true) : latinProfile();
    }
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
    /// The Gen 4 code page holds hiragana, katakana, full- and half-width Latin and 2,406 Hangul
    /// syllables for every language; the PAGES follow the record's language (a Japanese record
    /// opens on kana, a Korean one on Hangul), and every key is still checked against the table.
    [[nodiscard]] KeyboardProfile keyboard() const noexcept override {
        if (language_ == kLanguageJapanese) return japaneseProfile(true);
        if (language_ == kLanguageKorean) return koreanProfile();
        return latinProfile();
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
// Gen V (BW / B2W2): PK5 and the trainer block store UTF-16, one unit per character, 0xFFFF
// terminator; text that is not full width (and all Korean text) stores ♂ / ♀ as the half-width
// U+246D / U+246E (PKHeX 8e3eb3d4 StringConverter5.SetString + StringConverter4Util).
//
// THE REPERTOIRE: UTF-16 itself stores anything, but the games draw only their font's glyphs. The
// verified source this project has for those glyphs is the Gen 4 code page (Utils/Gen4Text.h, the
// PKHeX StringConverter4Util tables): every Gen IV name reaches Gen V through Poké Transfer as the
// Unicode of that table (PKHeX PK4.ConvertToPK5 -> StringConverter4.GetString), so every glyph in it
// is one the Gen V games draw. A character outside it is refused, never approximated. This widens
// Gen V entry from printable ASCII to that table (hiragana, katakana, full-width Latin, accents,
// symbols, Hangul) -- the owner's "no ASCII-only keyboard when the game supports more".
// ---------------------------------------------------------------------------------------------
class Gen5NameField final : public TextFieldCapability {
public:
    Gen5NameField(FieldKind kind, uint8_t language, std::size_t maxChars, std::size_t slotCount,
                  bool allowEmpty = false) noexcept
        : kind_(kind), language_(language),
          maxChars_(slotCount == 0 ? 0 : (maxChars < slotCount - 1 ? maxChars : slotCount - 1)),
          slotCount_(slotCount), allowEmpty_(allowEmpty) {}

    [[nodiscard]] FieldKind kind() const noexcept override { return kind_; }
    [[nodiscard]] uint16_t maxCharacters() const noexcept override { return static_cast<uint16_t>(maxChars_); }
    [[nodiscard]] uint16_t maxNativeUnits() const noexcept override { return static_cast<uint16_t>(slotCount_); }
    [[nodiscard]] uint8_t language() const noexcept { return language_; }
    [[nodiscard]] bool glyphAllowed(char32_t c) const noexcept override {
        return c != 0 && c <= 0xFFFF && c != 0xFFFF && Utils::gen4GlyphEncodable(static_cast<char16_t>(c));
    }
    [[nodiscard]] KeyboardProfile keyboard() const noexcept override {
        if (language_ == kLanguageJapanese) return japaneseProfile(true);
        if (language_ == kLanguageKorean) return koreanProfile();
        return latinProfile();
    }
    /// The 16-bit units the game stores for `text` (gender signs remapped), without terminator.
    [[nodiscard]] std::u16string nativeUnits(const std::u16string& text) const {
        const bool halfWidth = language_ == kLanguageKorean || !Utils::gen4IsFullWidth(text);
        std::u16string out = text;
        if (halfWidth)
            for (char16_t& c : out) c = Utils::unnormalizeGen4Gender(c);
        return out;
    }
    /// What the game reads back from stored units (the half-width gender signs shown as ♂ / ♀).
    [[nodiscard]] static std::u16string readBack(const std::u16string& units) {
        std::u16string out = units;
        for (char16_t& c : out) c = Utils::normalizeGen4Gender(c);
        return out;
    }
    [[nodiscard]] TextVerdict check16(const std::u16string& text) const override {
        TextVerdict v;
        v.characters = static_cast<uint16_t>(codePoints(text));
        if (text.empty()) {
            if (!allowEmpty_) return refused(Refusal::Empty);
            return v;
        }
        for (std::size_t i = 0; i < text.size(); ++i)
            if (!glyphAllowed(text[i])) return refused(Refusal::UnsupportedGlyph, i, text[i]);
        if (text.size() > maxChars_) return refused(Refusal::TooLong, maxChars_);
        if (readBack(nativeUnits(text)) != text) return refused(Refusal::RoundTripMismatch);
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
