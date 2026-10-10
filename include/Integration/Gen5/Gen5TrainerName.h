#ifndef POKEBANK_GEN5_TRAINER_NAME_H
#define POKEBANK_GEN5_TRAINER_NAME_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace PokeVault::Integration::Gen5 {

// Display conversion only. Gen V trainer names are up to seven UTF-16 code
// units in the validated Trainer block. Malformed/unsupported code units
// return no label, rather than inventing a trainer identity or emitting
// invalid UTF-8 into the Save Instances UI.
[[nodiscard]] inline std::optional<std::string> displayTrainerName(
    std::u16string_view raw) {
    if(raw.empty() || raw.size()>7)return std::nullopt;
    std::string out;
    out.reserve(raw.size()*3);
    for(size_t i=0;i<raw.size();++i) {
        char32_t c=raw[i];
        if(c>=0xD800 && c<=0xDBFF) {
            if(i+1>=raw.size())return std::nullopt;
            const char32_t lo=raw[++i];
            if(lo<0xDC00 || lo>0xDFFF)return std::nullopt;
            c=0x10000+((c-0xD800)<<10)+(lo-0xDC00);
        } else if(c>=0xDC00 && c<=0xDFFF) {
            return std::nullopt;
        }
        // Remove terminal/control/private-use/noncharacter text rather than
        // displaying arbitrary glyphs as the trainer's actual name.
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

} // namespace PokeVault::Integration::Gen5
#endif
