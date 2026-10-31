#ifndef POKEBANK_GEN5_NATIVE_TEXT_H
#define POKEBANK_GEN5_NATIVE_TEXT_H

#include "Encryption/Encryption5.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// PK5 nickname 0x48: 11 UTF-16 units, max 10 characters.
// PK5 original trainer name 0x68: 8 units, max 7 characters.
// This first text-editing tranche permits native ASCII only, not lossy
// UTF-8-to-Pokemon glyph approximation. Non-ASCII source text is preserved.
[[nodiscard]] constexpr size_t nameOffset5(bool nickname) noexcept {
    return nickname ? 0x48 : 0x68;
}
[[nodiscard]] constexpr size_t nameCapacity5(bool nickname) noexcept {
    return nickname ? 11 : 8;
}
[[nodiscard]] constexpr size_t nameMaxLength5(bool nickname) noexcept {
    return nameCapacity5(nickname)-1;
}

[[nodiscard]] inline std::optional<std::string> asciiName5(
    std::span<const uint8_t> plain,bool nickname) {
    const size_t off=nameOffset5(nickname),cap=nameCapacity5(nickname);
    if(plain.size()<off+cap*2)return std::nullopt;
    std::string result;
    for(size_t i=0;i<cap;++i) {
        const uint16_t code=Crypto::read16(plain,off+i*2);
        if(code==0 || code==0xFFFF)break;
        if(code<0x20 || code>0x7E)return std::nullopt;
        result.push_back(static_cast<char>(code));
    }
    return result;
}
[[nodiscard]] inline bool nativeNameBytesDiffer5(
    std::span<const uint8_t> a,std::span<const uint8_t> b,bool nickname) {
    const size_t off=nameOffset5(nickname),end=off+nameCapacity5(nickname)*2;
    if(a.size()<end || b.size()<end)return true;
    return !std::equal(a.begin()+static_cast<std::ptrdiff_t>(off),
                       a.begin()+static_cast<std::ptrdiff_t>(end),
                       b.begin()+static_cast<std::ptrdiff_t>(off));
}
[[nodiscard]] inline bool encodeAsciiName5(std::vector<uint8_t>& plain,
                                           bool nickname,
                                           std::string_view text) {
    const size_t off=nameOffset5(nickname),cap=nameCapacity5(nickname);
    if(plain.size()<off+cap*2 || text.empty() ||
       text.size()>nameMaxLength5(nickname))return false;
    for(const unsigned char c:text)
        if(c<0x20 || c>0x7E)return false;
    // Full fixed-width canonical name: ASCII UCS-2 plus 0xFFFF terminators.
    for(size_t i=0;i<cap;++i)
        Crypto::write16(plain,off+2*i,
            i<text.size()?static_cast<uint16_t>(
                static_cast<unsigned char>(text[i])):uint16_t{0xFFFF});
    if(nickname) {
        const uint32_t iv=Crypto::read32(plain,0x38)|0x80000000u;
        Crypto::write16(plain,0x38,static_cast<uint16_t>(iv));
        Crypto::write16(plain,0x3A,static_cast<uint16_t>(iv>>16));
    }
    return true;
}
// A Unicode name written as PKHeX 8e3eb3d4 StringConverter5.SetString does for PK5.Nickname /
// OriginalTrainerName (StringConverterOption.None): one UTF-16 unit per character, a 0xFFFF
// terminator, and the units after it left as they are. Text that is not full width (or any Korean
// text) stores ♂/♀ as the game's half-width U+246D/U+246E (StringConverter4Util.UnNormalizeGender-
// Symbol). Does not touch the nickname flag. Used for language-default names (M1 D3); free Unicode
// entry waits for the verified Gen V glyph rules.
[[nodiscard]] inline bool writeUnicodeName5(std::vector<uint8_t>& plain, bool nickname,
                                            std::u16string_view text, uint8_t language) {
    const size_t off=nameOffset5(nickname),cap=nameCapacity5(nickname);
    if(plain.size()<off+cap*2 || text.empty() || text.size()>nameMaxLength5(nickname))return false;
    bool fullWidth=false;
    for(const char16_t c:text) {
        if((c>>12)==0 || (c>>12)==0xE || c==u'\u2640' || c==u'\u2642')continue;
        fullWidth=true;
        break;
    }
    const bool halfWidth=language==8 || !fullWidth;
    for(size_t i=0;i<text.size();++i) {
        char16_t c=text[i];
        if(c==0 || c==0xFFFF)return false;
        if(halfWidth && c==u'\u2642')c=u'\u246D';
        if(halfWidth && c==u'\u2640')c=u'\u246E';
        Crypto::write16(plain,off+2*i,static_cast<uint16_t>(c));
    }
    if(text.size()<cap)Crypto::write16(plain,off+2*text.size(),0xFFFF);
    return true;
}
} // namespace PokeVault::Integration::Gen5
#endif
