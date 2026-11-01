#ifndef UI_SYSTEM_FONT_FALLBACKS_H
#define UI_SYSTEM_FONT_FALLBACKS_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace UI::SystemFontFallbacks {

// The console's shared fonts that the UI adds as fallbacks behind Nunito and the two romfs symbol
// fonts (PKSEFramebuffer). Every Switch carries them, so nothing is bundled.
//
// Order is the selection rule: fontstash draws each code point with the FIRST fallback that has a
// glyph for it. Standard comes first so kana and kanji keep their Japanese shapes (the Korean font
// also carries Han characters, drawn in Korean style); Korean comes last and so only adds what no
// earlier font has, which is Hangul (Round 11 H27: Korean names drew as "[][][][][]").
enum class SharedFont : uint8_t { Standard, Korean };

struct Fallback {
    SharedFont font;
    const char* face;   // NanoVG face name; distinct per font
};

inline constexpr std::array<Fallback, 2> kOrder{{
    {SharedFont::Standard, "jp"},
    {SharedFont::Korean, "ko"},
}};

// Two romfs symbol fonts plus the shared fonts must fit fontstash's per-font fallback table
// (FONS_MAX_FALLBACKS, nanovg/fontstash.h).
inline constexpr size_t kRomfsFallbacks = 2;
inline constexpr size_t kFontstashMaxFallbacks = 20;
static_assert(kRomfsFallbacks + kOrder.size() <= kFontstashMaxFallbacks);

struct Registered {
    int japanese = -1;   // NanoVG font id, or -1 when the console font could not be loaded
    int korean = -1;
};

// Loads each shared font in kOrder and registers the ones that load. `load(font, face)` returns a
// NanoVG font id, or a negative value when the console font is unavailable; `addFallback(id)`
// appends it to the primary font's fallbacks. A font that fails is skipped and the others are
// still registered, so the app draws exactly as before for any script it cannot load -- missing
// glyphs, never a crash.
template <class Load, class AddFallback>
Registered registerAll(Load&& load, AddFallback&& addFallback) {
    Registered out;
    for (const Fallback& f : kOrder) {
        const int id = load(f.font, f.face);
        if (id < 0) continue;
        addFallback(id);
        (f.font == SharedFont::Standard ? out.japanese : out.korean) = id;
    }
    return out;
}

} // namespace UI::SystemFontFallbacks

#endif
