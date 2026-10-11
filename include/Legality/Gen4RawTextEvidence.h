#pragma once
// META-001/EVENT-001: native PK4 raw text FIELD CAPABILITY only.
// A decoded Unicode name cannot reconstruct its original 0xFFFF terminator
// or trailing word sequence. This never judges Pal Park/PCD trash legality,
// and a missing terminator is not a global Invalid determination.
#include <array>
#include <cstddef>
#include <cstdint>

namespace Legality::Gen4RawTextEvidence {
enum class Status : uint8_t { Unavailable, Known };
struct Field {
    bool terminatorPresent=false;
    uint8_t terminatorIndex=0;
    bool postTerminatorNonzero=false;
};
struct Result {
    Status status=Status::Unavailable;
    Field nickname{};
    Field originalTrainer{};
    constexpr bool fullyTerminated() const noexcept {
        return status==Status::Known &&
               nickname.terminatorPresent &&
               originalTrainer.terminatorPresent;
    }
};
template <std::size_t N>
constexpr Field inspect(const std::array<std::byte,N>& bytes) noexcept {
    Field result{};
    static_assert(N%2==0);
    constexpr std::size_t units=N/2;
    for(std::size_t i=0;i<units;++i) {
        const uint16_t word=static_cast<uint16_t>(
            std::to_integer<uint8_t>(bytes[2*i]) |
            static_cast<uint16_t>(std::to_integer<uint8_t>(bytes[2*i+1]))<<8);
        if(!result.terminatorPresent && word==0xFFFFu) {
            result.terminatorPresent=true;
            result.terminatorIndex=static_cast<uint8_t>(i);
        } else if(result.terminatorPresent && word!=0) {
            result.postTerminatorNonzero=true;
        }
    }
    return result;
}
constexpr Result classify(bool available,
                          const std::array<std::byte,22>& nickname,
                          const std::array<std::byte,16>& ot) noexcept {
    if(!available)return {};
    return {Status::Known,inspect(nickname),inspect(ot)};
}
} // namespace Legality::Gen4RawTextEvidence
