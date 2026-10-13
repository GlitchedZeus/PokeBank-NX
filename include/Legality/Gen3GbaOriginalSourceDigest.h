#pragma once
// L3 source-independent ordered learnset fingerprint algorithm.
// FNV-1a-64 over LE u16: National species, offer count, every packed offer.
// Avoid source-table union/deduplication and preserve same-level order.
#include <cstdint>
namespace Legality::Gen3GbaOriginalSourceFingerprints {
inline constexpr std::uint64_t kOffset=14695981039346656037ULL;
inline constexpr std::uint64_t kPrime=1099511628211ULL;
constexpr std::uint64_t appendByte(std::uint64_t h,std::uint8_t b) noexcept {
    return (h^b)*kPrime;
}
constexpr std::uint64_t appendWord(std::uint64_t h,std::uint16_t v) noexcept {
    return appendByte(appendByte(h,static_cast<std::uint8_t>(v)),
                      static_cast<std::uint8_t>(v>>8));
}
template<class Lookup>
constexpr std::uint64_t fingerprint(std::uint16_t species,
                                    std::uint16_t count,Lookup lookup) noexcept {
    auto h=appendWord(appendWord(kOffset,species),count);
    for(std::uint16_t i=0;i<count;++i)h=appendWord(h,lookup(i));
    return h;
}
} // namespace Legality::Gen3GbaOriginalSourceFingerprints
