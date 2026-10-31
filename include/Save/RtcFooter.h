#pragma once
#include <cstddef>
namespace PokeVault::Save {
inline bool isKnownRtcFooterSize(std::size_t size) noexcept {
    return size == 7 || (size >= 0x0C && size <= 0x30 && (size & 1u) == 0);
}
inline bool splitRtcPayloadSize(std::size_t totalSize, std::size_t payloadSize,
                                std::size_t& footerSize) noexcept {
    footerSize = 0;
    if (totalSize == payloadSize) return true;
    if (totalSize < payloadSize) return false;
    footerSize = totalSize - payloadSize;
    return isKnownRtcFooterSize(footerSize);
}
} // namespace PokeVault::Save
