#pragma once
// META-001: explicit availability and five native Gen IV Sinnoh contest
// rank-prefix checks. Ranks are four consecutive bits, low to high:
// Normal, Great, Ultra, Master. Do not infer missing data as zero.
#include <array>
#include <cstdint>

namespace Legality::Gen4ContestRibbon {
enum class Status : uint8_t { Unavailable, Consistent, RankGap };
struct Result {
    Status status=Status::Unavailable;
    uint8_t presentCategories=0;
    uint8_t gapCategoriesMask=0;
    constexpr bool known() const noexcept {
        return status!=Status::Unavailable;
    }
};
constexpr Result evaluate(bool available,
                          const std::array<uint8_t,5>& ranks) noexcept {
    if(!available) return {};
    Result result{};
    result.status=Status::Consistent;
    for(uint8_t i=0;i<ranks.size();++i) {
        const uint8_t mask=static_cast<uint8_t>(ranks[i]&0x0Fu);
        if(!mask)continue;
        ++result.presentCategories;
        // Valid prefixes: 0000,0001,0011,0111,1111.
        if((mask & static_cast<uint8_t>(mask+1u))!=0)
            result.gapCategoriesMask|=static_cast<uint8_t>(1u<<i);
    }
    if(result.gapCategoriesMask)
        result.status=Status::RankGap;
    return result;
}
} // namespace Legality::Gen4ContestRibbon
