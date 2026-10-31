#ifndef POKEBANK_GEN5_SOURCE_PATH_IDENTITY_H
#define POKEBANK_GEN5_SOURCE_PATH_IDENTITY_H

#include <string>
#include <string_view>
#include <vector>

namespace PokeVault::Integration::Gen5 {
// Ownership comparison for the case-insensitive Switch SD mount when stat
// does not supply a usable inode. Never rewrite the actual I/O path or infer
// ownership from trainer names/save contents. POSIX host paths stay distinct.
[[nodiscard]] inline std::string sdOwnershipPath(std::string_view path) {
    if(path.find('\0')!=std::string_view::npos)return {};
    std::string normalized;
    normalized.reserve(path.size());
    for(unsigned char ch:path) {
        if(ch=='\\')ch='/';
        if(ch>='A' && ch<='Z')ch=static_cast<unsigned char>(ch+'a'-'A');
        normalized.push_back(static_cast<char>(ch));
    }
    if(!normalized.starts_with("sdmc:/"))return {};
    std::vector<std::string_view> parts;
    for(size_t start=6;start<normalized.size();) {
        const auto end=normalized.find('/',start);
        const auto part=std::string_view(normalized).substr(start,
            end==std::string::npos?normalized.size()-start:end-start);
        if(part=="..") {
            if(parts.empty())return {}; // Never traverse above this mount.
            parts.pop_back();
        } else if(!part.empty() && part!=".")parts.push_back(part);
        if(end==std::string::npos)break;
        start=end+1;
    }
    std::string out="sdmc:/";
    for(const auto part:parts) {
        if(out.size()>6)out+='/';
        out+=part;
    }
    return out;
}
[[nodiscard]] inline bool sameSdOwnershipPath(std::string_view a,std::string_view b) {
    const auto key=sdOwnershipPath(a);
    return !key.empty() && key==sdOwnershipPath(b);
}
[[nodiscard]] inline bool gen5BindingGame(std::string_view game) noexcept {
    return game=="black_nds" || game=="white_nds" ||
           game=="black2_nds" || game=="white2_nds";
}
} // namespace PokeVault::Integration::Gen5
#endif
