#pragma once
// L2 Gen I only: read-only original PK1 fields, DVs, Stat Experience,
// unified Special and source-specific battle-stat comparison.
// This is NOT a Gen I breeding/ability/gender/nature/held-item verifier.
// No modern IV/EV attributes can invalidate native PK1.
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Legality/Gen1NativeExperienceEvidence.h"
#include <array>
#include <cstdint>
namespace Legality::Gen1NativeDVStat {
namespace G1=PokeVault::Integration::Gen1;
enum class Status : std::uint8_t {
    UnknownSource, InvalidNativeRecord, UnknownLevelContext,
    UnresolvedPartyCache, ConditionalNativeFields,
    ConditionalPartyStatMatch, ContradictedAttestedPartyStats,
    ContradictedAttestedLevelExperience
};
struct SourceContext {
    // A strict PK1 parser validated the original 0x21/0x2C body.
    bool strictPK1SourceIndependentlyVerified=false;
    // Party stats may be stale following Stat Exp gains, leveling, or
    // in-battle bonuses. A claim of "this is a party" is NOT sufficient.
    bool sameLevelAndStatExpAtLastCalculationIndependentlyVerified=false;
    bool ordinaryUnmodifiedPartyStatCalculationIndependentlyVerified=false;
    // Does not assert R/B/Y original acquisition history from a tradeback.
    bool nativeLevelHistoryIndependentlyVerified=false;
    // Opt-in only. Detached original PK1 EXP cannot certify ordinary history.
    bool originalExperienceProgressionIndependentlyVerified=false;
    bool originalSpeciesGrowthSourceIndependentlyVerified=false;
};
struct Result {
    Status status=Status::UnknownSource;
    std::uint8_t derivedHPDV=0;
    std::array<std::uint8_t,4> storedDVs{};
    std::array<std::uint16_t,5> statExperience{};
    G1::BattleStats calculated{};
    bool comparedCachedPartyStats=false;
    bool boxLevelDiffersFromPartyLevel=false;
    Gen1NativeExperience::Result experienceEvidence{};
};
constexpr std::uint8_t hpDV(std::array<std::uint8_t,4> stored) noexcept {
    // PK1 DV16: high to low nibbles Atk, Def, Speed, Special.
    return static_cast<std::uint8_t>(((stored[0]&1u)<<3u)|
       ((stored[1]&1u)<<2u)|((stored[2]&1u)<<1u)|(stored[3]&1u));
}
constexpr std::array<std::uint8_t,4> unpackDVs(std::uint16_t packed) noexcept {
    return {static_cast<std::uint8_t>((packed>>12u)&15u),
            static_cast<std::uint8_t>((packed>>8u)&15u),
            static_cast<std::uint8_t>((packed>>4u)&15u),
            static_cast<std::uint8_t>(packed&15u)};
}
constexpr std::uint16_t packedDVs(std::array<std::uint8_t,4> dv) noexcept {
    return static_cast<std::uint16_t>((dv[0]<<12u)|(dv[1]<<8u)|
                                      (dv[2]<<4u)|dv[3]);
}
constexpr std::uint16_t bigEndian16(const std::array<std::uint8_t,44>& bytes,
                                    std::size_t offset) noexcept {
    return static_cast<std::uint16_t>((static_cast<unsigned>(bytes[offset])<<8u)|
                                       bytes[offset+1]);
}
constexpr std::uint32_t bigEndian24(const std::array<std::uint8_t,44>& bytes,
                                    std::size_t offset) noexcept {
    return (static_cast<std::uint32_t>(bytes[offset])<<16u)|
           (static_cast<std::uint32_t>(bytes[offset+1])<<8u)|bytes[offset+2];
}
Result evaluate(const G1::PokemonRecord& record,SourceContext source) noexcept;
static_assert(hpDV({15,15,15,15})==15);
static_assert(hpDV({8,8,8,8})==0);
static_assert(unpackDVs(0xA987u)==
              (std::array<std::uint8_t,4>{10,9,8,7}));
static_assert(packedDVs({10,9,8,7})==0xA987u);
} // namespace Legality::Gen1NativeDVStat
