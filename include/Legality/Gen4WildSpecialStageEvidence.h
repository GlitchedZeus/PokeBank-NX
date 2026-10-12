#pragma once
// L1 / E13 (+E15 interface): bounded exact-source HGSS special wild routes.
// No encounter is certified without independently attested original context.
// Existing Method K verifier searches only a bounded nature reversal; its
// false result is Unknown, not global Invalid.
#include "Legality/Gen4WildEncounter.h"
#include "Legality/Gen4WildRngCorrelation.h"
#include "Legality/Gen4NatureReversalReplay.h"
#include "Legality/Gen4PidIvCorrelation.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace Legality::Gen4WildSpecialStageEvidence {
enum class Status : uint8_t {
    UnknownOriginalSource,UnknownOriginalRow,UnknownCaptureTimeContext,
    UnknownLeadOrRngFamily,UnknownOriginalNatureStage,
    CandidateRowContradicted,CandidatePidIvContradicted,BoundedCandidateUnresolved,
    KnownConditionalSpecialEncounter
};
enum class Category : uint8_t {
    Unknown,Fishing,RockSmash,Headbutt,BugContest,Safari
};
constexpr Category category(uint8_t method) noexcept {
    if(method>=10 && method<=14)return Category::Safari;
    if(method>=2 && method<=4)return Category::Fishing;
    if(method==5)return Category::RockSmash;
    if(method==6||method==7)return Category::Headbutt;
    if(method==8)return Category::BugContest;
    return Category::Unknown;
}
struct Candidate {
    std::string_view exactGame{};
    std::size_t rowIndex=0;
    uint16_t originalSpecies=0,originalLocation=0;
    uint8_t originalLevel=0,originalForm=0;
    uint32_t pid=0,prePidSeed=0;
    std::array<uint8_t,6> originalIVs{};
    bool originalWildSourceIndependentlyAttested=false;
    bool originalRowIndependentlyAttested=false;
    bool captureTimeAreaAndMethodConditionsIndependentlyAttested=false;
    bool originalNoLeadAndMethodKIndependentlyAttested=false;
    bool originalPidIvStageIndependentlyAttested=false;
    // HGSS Safari blocks/time, fishing activation/swarm, Rock Smash area
    // rate, Headbutt group, contest day and minimum-31 rerolls must all be
    // established separately; current save/world state cannot prove them.
};
struct Result {
    Status status=Status::UnknownOriginalSource;
    Category method=Category::Unknown;
    Gen4WildRng::Method matchedRng=Gen4WildRng::Method::None;
};
inline Result evaluate(const Candidate& c) noexcept {
    const auto game=Gen4Wild::gameForId(c.exactGame);
    if(game!=Gen4Wild::Game::HeartGold&&
       game!=Gen4Wild::Game::SoulSilver)return {};
    if(!c.originalWildSourceIndependentlyAttested)return {};
    if(!c.originalRowIndependentlyAttested ||
       c.rowIndex>=std::size(Gen4Wild::kPackedGen4WildEncounters))
        return {Status::UnknownOriginalRow};
    const auto row=Gen4Wild::kPackedGen4WildEncounters[c.rowIndex];
    if(Gen4Wild::game(row)!=game)return {Status::UnknownOriginalRow};
    const auto method=category(Gen4Wild::method(row));
    if(method==Category::Unknown)return {Status::UnknownOriginalRow};
    if(!c.captureTimeAreaAndMethodConditionsIndependentlyAttested)
        return {Status::UnknownCaptureTimeContext,method};
    if(!c.originalNoLeadAndMethodKIndependentlyAttested)
        return {Status::UnknownLeadOrRngFamily,method};
    if(!c.originalPidIvStageIndependentlyAttested)
        return {Status::UnknownOriginalNatureStage,method};
    if(Gen4Wild::species(row)!=c.originalSpecies ||
       Gen4Wild::location(row)!=c.originalLocation ||
       !Gen4Wild::levelMatches(row,c.originalLevel) ||
       !Gen4Wild::formMatches(Gen4Wild::form(row),c.originalForm))
        return {Status::CandidateRowContradicted,method};
    // One original Method-1 seed must generate the SAME stored PID and IVs.
    // A matching slot/nature alone is not an attested captured specimen.
    if(!Gen3PidIv::validStoredIVs(c.originalIVs))
        return {Status::UnknownOriginalNatureStage,method};
    if(Gen4WildRng::sequentialPid(c.prePidSeed)!=c.pid)
        return {Status::CandidatePidIvContradicted,method};
    const auto pidiv=Gen4PidIv::analyze(c.pid,c.originalIVs);
    if(!pidiv.matched() || pidiv.originSeed!=c.prePidSeed)
        return {Status::CandidatePidIvContradicted,method};
    auto result=Gen4WildRng::matchNoLeadRow(
        true,row,c.prePidSeed,c.pid,c.originalLevel);
    if(!result.matched())
        result=Gen4WildRng::matchAcceptedNoLeadRerollRow(
            true,row,c.prePidSeed,c.pid,c.originalLevel);
    if(!result.matched())
        return {Status::BoundedCandidateUnresolved,method};
    return {Status::KnownConditionalSpecialEncounter,method,result.method};
}
} // namespace Legality::Gen4WildSpecialStageEvidence
