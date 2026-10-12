#pragma once
// L1 / E07: exact, separately attested Emerald Safari row and lead/Method-H
// seed. This joins ONE candidate source row to the same PID/IV history;
// it does not exhaust Emerald wild encounters, trades, eggs or events.
#include "Legality/Gen3SafariMethodHEvidence.h"
#include <array>
#include <cstddef>
#include <cstdint>
namespace Legality::Gen3MethodHOriginalStage {
enum class Lead : uint8_t { Unknown, NoLead, SynchronizeSuccess };
enum class Status : uint8_t {
    UnknownSource, UnknownRow, UnknownLead, UnknownPidIvStage,
    UnsupportedPidIvFamily, CandidateRowContradicted,
    CandidatePidIvContradicted, UnsupportedLeadHistory,
    KnownConditionalEmeraldSafariHistory
};
struct Candidate {
    std::size_t rowIndex=0;
    uint16_t originalSpecies=0,originalLocation=0;
    uint8_t originalLevel=0,originalForm=0;
    uint32_t originalPid=0,prePidSeed=0;
    std::array<uint8_t,6> originalIVs{};
    Lead lead=Lead::Unknown;
    bool originalEmeraldSafariIndependentlyVerified=false;
    bool originalRowIndependentlyVerified=false;
    bool originalLeadIndependentlyVerified=false;
    bool originalPidIvStageIndependentlyVerified=false;
};
struct Result {
    Status status=Status::UnknownSource;
    Gen3SafariMethodH::Path matchedPath=Gen3SafariMethodH::Path::None;
    uint32_t matchedFrame=0;
};
inline Result evaluate(const Candidate& c) noexcept {
    namespace M=Gen3SafariMethodH;
    if(!c.originalEmeraldSafariIndependentlyVerified)return {};
    if(!c.originalRowIndependentlyVerified ||
       c.rowIndex>=std::size(Gen3Safari::kGen3SafariEntries))
        return {Status::UnknownRow};
    if(!c.originalLeadIndependentlyVerified || c.lead==Lead::Unknown)
        return {Status::UnknownLead};
    if(!c.originalPidIvStageIndependentlyVerified)
        return {Status::UnknownPidIvStage};
    const auto& row=Gen3Safari::kGen3SafariEntries[c.rowIndex];
    if(row.game!=static_cast<uint8_t>(Gen3Safari::Game::Emerald))
        return {Status::UnknownRow}; // alien source is not a negative proof
    if(row.species!=c.originalSpecies || row.location!=c.originalLocation ||
       row.form!=c.originalForm || c.originalLevel<row.minLevel ||
       c.originalLevel>row.maxLevel)
        return {Status::CandidateRowContradicted};
    if(!Gen3PidIv::validStoredIVs(c.originalIVs))
        return {Status::UnknownPidIvStage};
    const auto correlation=Gen3PidIv::analyze(c.originalPid,c.originalIVs);
    if(!M::supportedCorrelation(correlation.method))
        return {Status::UnsupportedPidIvFamily};
    if(correlation.originSeed!=c.prePidSeed)
        return {Status::CandidatePidIvContradicted};
    const uint8_t nature=static_cast<uint8_t>(
        M::Detail::sequentialPid(c.prePidSeed)%25u);
    M::Detail::CandidateMatch match{};
    if(c.lead==Lead::NoLead)
        match=M::Detail::matchCandidate(
            row,c.originalLevel,c.prePidSeed,nature,true);
    else if(c.lead==Lead::SynchronizeSuccess)
        match=M::Detail::matchEmeraldSynchronize(
            row,c.originalLevel,c.prePidSeed);
    if(match.path==M::Path::None)
        return {Status::UnsupportedLeadHistory};
    return {Status::KnownConditionalEmeraldSafariHistory,
            match.path,match.frameSeed};
}
} // namespace Legality::Gen3MethodHOriginalStage
