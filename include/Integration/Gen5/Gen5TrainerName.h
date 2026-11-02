#ifndef POKEBANK_GEN5_TRAINER_NAME_H
#define POKEBANK_GEN5_TRAINER_NAME_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace PokeVault::Integration::Gen5 {

// A freshly initialized new-game save: the game has not yet written a
// trainer (no name, TID=SID=0, play time 0:00:00). Conservative: all four
// must hold. Its gender byte (0) is NOT a gender and must never be shown
// as "Male"; the trainer is presented as "no trainer yet".
[[nodiscard]] constexpr bool newGameNoTrainer5(size_t nameLength, uint16_t tid, uint16_t sid,
                                               uint16_t hours, uint8_t minutes,
                                               uint8_t seconds) noexcept {
    return nameLength == 0 && tid == 0 && sid == 0 && hours == 0 && minutes == 0 && seconds == 0;
}
// Display conversion only, shared by every Gen V name a screen shows: trainer names here and
// Pokemon nickname / OT names through Gen5NativeText.h displayName5 (Round 11 H27). Gen V stores
// names as UTF-16 in the record's own language (Japanese, Korean, accented Latin ...), so the
// text is shown as stored rather than through the ASCII edit gate. The half-width gender signs the
// games store (U+246D / U+246E, PKHeX StringConverter4Util) are shown as the ♂ / ♀ they mean.
// Malformed/unsupported code units return no label, rather than inventing an identity or emitting
// invalid UTF-8; the caller keeps its own fallback. `raw` excludes the terminator.
[[nodiscard]] inline std::optional<std::string> displayGen5Text(
    std::u16string_view raw, size_t maxUnits) {
    if(raw.empty() || raw.size()>maxUnits)return std::nullopt;
    std::string out;
    out.reserve(raw.size()*3);
    for(size_t i=0;i<raw.size();++i) {
        char32_t c=raw[i];
        if(c==0x246D)c=0x2642;
        else if(c==0x246E)c=0x2640;
        if(c>=0xD800 && c<=0xDBFF) {
            if(i+1>=raw.size())return std::nullopt;
            const char32_t lo=raw[++i];
            if(lo<0xDC00 || lo>0xDFFF)return std::nullopt;
            c=0x10000+((c-0xD800)<<10)+(lo-0xDC00);
        } else if(c>=0xDC00 && c<=0xDFFF) {
            return std::nullopt;
        }
        // Remove terminal/control/private-use/noncharacter text rather than
        // displaying arbitrary glyphs as the actual name.
        if(c<0x20 || (c>=0x7F && c<=0x9F) ||
           (c>=0xE000 && c<=0xF8FF) ||
           (c>=0xFDD0 && c<=0xFDEF) ||
           (c&0xFFFE)==0xFFFE || c>0x10FFFF)
            return std::nullopt;
        if(c<=0x7F)out.push_back(static_cast<char>(c));
        else if(c<=0x7FF) {
            out.push_back(static_cast<char>(0xC0|(c>>6)));
            out.push_back(static_cast<char>(0x80|(c&0x3F)));
        } else if(c<=0xFFFF) {
            out.push_back(static_cast<char>(0xE0|(c>>12)));
            out.push_back(static_cast<char>(0x80|((c>>6)&0x3F)));
            out.push_back(static_cast<char>(0x80|(c&0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0|(c>>18)));
            out.push_back(static_cast<char>(0x80|((c>>12)&0x3F)));
            out.push_back(static_cast<char>(0x80|((c>>6)&0x3F)));
            out.push_back(static_cast<char>(0x80|(c&0x3F)));
        }
    }
    return out;
}
// Gen V trainer names are up to seven UTF-16 code units in the validated Trainer block.
[[nodiscard]] inline std::optional<std::string> displayTrainerName(
    std::u16string_view raw) {
    return displayGen5Text(raw,7);
}

} // namespace PokeVault::Integration::Gen5
#endif
