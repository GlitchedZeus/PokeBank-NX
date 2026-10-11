#pragma once
// RNG-001: retail Generation III ordinary-daycare IV inheritance STAGE only.
// Forward replay requires the RNG state at the InheritIVs entry boundary
// plus known initial child and two parent IV arrays. It does NOT reconstruct
// the pending PID, encounter, parent pair, or prove a real egg existed.
// Unknown parents stay Unknown; a mismatched known stage is NOT globally Invalid.
#include <array>
#include <cstdint>
#include <string_view>

namespace Legality::Gen3EggIVInheritanceStage {
enum class Family : uint8_t { Unknown, RubySapphire, Emerald, FireRedLeafGreen };
enum class Status : uint8_t { UnsupportedGame, UnknownParentHistory, MalformedIVDomain, ReplayedStage };
enum class CandidateCheck : uint8_t {
    UnsupportedGame, UnknownParentHistory, MalformedIVDomain, Matched, CandidateMismatch
};
constexpr Family family(std::string_view id) noexcept {
    if(id=="ruby_gba"||id=="sapphire_gba") return Family::RubySapphire;
    if(id=="emerald_gba")return Family::Emerald;
    if(id=="firered_gba"||id=="leafgreen_gba")
        return Family::FireRedLeafGreen;
    return Family::Unknown;
}
struct Result {
    Status status=Status::UnsupportedGame;
    uint32_t finalSeed=0;
    std::array<uint16_t,6> draws{};
    std::array<uint8_t,3> selectedStats{};
    std::array<uint8_t,3> parentChoices{};
    std::array<uint8_t,6> childIVs{};
    uint8_t distinctInheritedStats=0;
    constexpr bool matches(const std::array<uint8_t,6>& observed) const noexcept {
        return status==Status::ReplayedStage && childIVs==observed;
    }
};
constexpr uint32_t next(uint32_t state) noexcept {
    return static_cast<uint32_t>(state*0x41C64E6Du+0x6073u);
}
// Exactly six LCRNG calls in the verified inheritance stage: three
// selected-stat and three donor draws. This inverse cannot reconstruct a
// seed from only the child's stored IV/PID; the stage-ending state must
// be independently known. It does not traverse the pending PID stage.
constexpr uint32_t rewindSix(uint32_t finalSeed) noexcept {
    uint32_t seed=finalSeed;
    for(uint8_t i=0;i<6;++i)
        seed=static_cast<uint32_t>(seed*0xEEB9EB65u+0x0A3561A1u);
    return seed;
}
constexpr bool validIVs(const std::array<uint8_t,6>& values) noexcept {
    for(uint8_t value:values)if(value>31)return false;
    return true;
}
constexpr Result replay(std::string_view game,uint32_t seed,
                        const std::array<uint8_t,6>& initial,
                        const std::array<uint8_t,6>& parent0,
                        const std::array<uint8_t,6>& parent1,
                        bool knownParents) noexcept {
    const Family f=family(game);
    if(f==Family::Unknown)return {};
    if(!knownParents)return {Status::UnknownParentHistory};
    if(!validIVs(initial)||!validIVs(parent0)||!validIVs(parent1))
        return {Status::MalformedIVDomain};
    Result result{};
    result.status=Status::ReplayedStage;
    result.childIVs=initial;
    std::array<uint8_t,6> available{0,1,2,3,4,5};
    for(uint8_t i=0;i<3;++i) {
        seed=next(seed);
        const uint16_t roll=static_cast<uint16_t>(seed>>16);
        result.draws[i]=roll;
        const uint8_t chosen=available[roll % (6u-i)];
        result.selectedStats[i]=chosen;
        // Real retail removal differences: Emerald removes loop position i;
        // RS/FRLG remove the selected stat VALUE as an array index. Never
        // use the optional repaired variant for forensic retail history.
        const uint8_t removal=f==Family::Emerald ? i : chosen;
        available[removal]=0xFFu;
        uint8_t compacted=0;
        for(uint8_t j=0;j<6;++j)
            if(available[j]!=0xFFu)
                available[compacted++]=available[j];
        // Deliberately preserve compacted stale tail, per retail behavior.
    }
    for(uint8_t i=0;i<3;++i) {
        seed=next(seed);
        const uint16_t roll=static_cast<uint16_t>(seed>>16);
        result.draws[3u+i]=roll;
        result.parentChoices[i]=static_cast<uint8_t>(roll % 2u);
    }
    std::array<bool,6> used{};
    for(uint8_t i=0;i<3;++i) {
        const uint8_t stat=result.selectedStats[i];
        if(!used[stat]) {
            used[stat]=true;
            ++result.distinctInheritedStats;
        }
        result.childIVs[stat]=
            result.parentChoices[i] ? parent1[stat] : parent0[stat];
    }
    result.finalSeed=seed;
    return result;
}
// Validate one EXPLICIT known-parent AND known final-RNG-state stage.
// A mismatch falsifies only that candidate history, never every possible
// unknown pair, seed, pending PID, breeding mode or game distribution.
constexpr CandidateCheck verifyKnownStage(
        std::string_view game,uint32_t knownFinalSeed,
        const std::array<uint8_t,6>& observedChild,
        const std::array<uint8_t,6>& initialChild,
        const std::array<uint8_t,6>& parent0,
        const std::array<uint8_t,6>& parent1,
        bool knownParents) noexcept {
    if(family(game)==Family::Unknown)return CandidateCheck::UnsupportedGame;
    if(!knownParents)return CandidateCheck::UnknownParentHistory;
    if(!validIVs(observedChild)||!validIVs(initialChild)||
       !validIVs(parent0)||!validIVs(parent1))
        return CandidateCheck::MalformedIVDomain;
    const auto result=replay(game,rewindSix(knownFinalSeed),
                             initialChild,parent0,parent1,true);
    if(result.status!=Status::ReplayedStage)
        return CandidateCheck::UnsupportedGame;
    return (result.finalSeed==knownFinalSeed &&
            result.matches(observedChild)) ?
        CandidateCheck::Matched:CandidateCheck::CandidateMismatch;
}
} // namespace Legality::Gen3EggIVInheritanceStage
