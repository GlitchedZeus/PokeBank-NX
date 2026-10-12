#pragma once
// L1 E06/E08: exact Ruby/Sapphire/FireRed/LeafGreen ORIGINAL Safari-source
// hypothesis. Source-game, area and physical row must be independently
// attested; this cannot reconstruct an evolved/traded creature's capture stage.
// A negative is only about this indexed, no-lead candidate; not global Invalid.
#include "Legality/Gen3SafariMethodHEvidence.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace Legality::Gen3SafariIndexedOriginalHistory {
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownOriginalSave, UnknownOriginalAreaOrRow,
    UnknownLeadStage, UnknownOriginalPidIv, UnknownOriginalBall,
    UnsupportedOriginalMethod, ForeignRowNotEvidence,
    CandidateRowContradicted, CandidateBallContradicted,
    CandidatePidIvContradicted, BoundedCandidateUnresolved,
    ConditionalOriginalSafariHistory
};
struct Candidate {
    std::string_view exactOriginalGame{};
    std::size_t originalRowIndex=0;
    uint16_t originalSpecies=0,originalMetLocation=0;
    uint8_t originalForm=0,originalMetLevel=0,originalBall=0;
    uint32_t originalPid=0,attestedPrePidSeed=0;
    std::array<uint8_t,6> originalIVs{};
    bool originalSaveIndependentlyAttested=false;
    bool originalPhysicalAreaAndRowIndependentlyAttested=false;
    bool originalNoLeadIndependentlyAttested=false;
    bool originalPidIvStageIndependentlyAttested=false;
    bool originalBallIndependentlyAttested=false;
    bool originalHandheldMethodFamilyIndependentlyAttested=false;
};
struct Result {
    Status status=Status::UnknownOriginalGame;
    Gen3SafariMethodH::Path path=Gen3SafariMethodH::Path::None;
    uint32_t frameSeed=0;
    uint16_t originalAreaIndex=0;
};
inline Result evaluate(const Candidate& c) noexcept {
    namespace G=Gen3Safari;
    namespace H=Gen3SafariMethodH;
    const auto game=G::gameForId(c.exactOriginalGame);
    if(game!=G::Game::Ruby && game!=G::Game::Sapphire &&
       game!=G::Game::FireRed && game!=G::Game::LeafGreen)
        return {};
    if(!c.originalSaveIndependentlyAttested)return {Status::UnknownOriginalSave};
    if(!c.originalPhysicalAreaAndRowIndependentlyAttested ||
       c.originalRowIndex>=std::size(G::kGen3SafariEntries))
        return {Status::UnknownOriginalAreaOrRow};
    const auto row=G::kGen3SafariEntries[c.originalRowIndex];
    // A row sourced from another exact game does not establish any negative
    // history for THIS original creature, even if its species/location agree.
    if(row.game!=static_cast<uint8_t>(game))
        return {Status::ForeignRowNotEvidence};
    if(!c.originalNoLeadIndependentlyAttested)
        return {Status::UnknownLeadStage};
    if(!c.originalPidIvStageIndependentlyAttested)
        return {Status::UnknownOriginalPidIv};
    if(!c.originalBallIndependentlyAttested)
        return {Status::UnknownOriginalBall};
    if(row.species!=c.originalSpecies ||
       row.location!=c.originalMetLocation ||
       row.form!=c.originalForm ||
       !G::levelMatches(row,c.originalMetLevel))
        return {Status::CandidateRowContradicted};
    if(c.originalBall!=G::kSafariBall)
        return {Status::CandidateBallContradicted};
    if(!c.originalHandheldMethodFamilyIndependentlyAttested ||
       !Gen3PidIv::validStoredIVs(c.originalIVs))
        return {Status::UnsupportedOriginalMethod};
    const auto correlation=Gen3PidIv::analyze(c.originalPid,c.originalIVs);
    if(!H::supportedCorrelation(correlation.method))
        return {Status::UnsupportedOriginalMethod};
    if(correlation.originSeed!=c.attestedPrePidSeed)
        return {Status::CandidatePidIvContradicted};
    const uint32_t replayPid=correlation.method==Gen3PidIv::Method::Method3 ?
        H::Detail::method3Pid(correlation.originSeed) :
        H::Detail::sequentialPid(correlation.originSeed);
    if(replayPid!=c.originalPid)
        return {Status::CandidatePidIvContradicted};
    const uint8_t nature=static_cast<uint8_t>(
        H::Detail::sequentialPid(c.attestedPrePidSeed)%25u);
    const bool hoenn=game==G::Game::Ruby||game==G::Game::Sapphire;
    const auto frame=H::Detail::matchCandidate(
        row,c.originalMetLevel,c.attestedPrePidSeed,nature,hoenn);
    if(frame.path==H::Path::None)
        return {Status::BoundedCandidateUnresolved};
    return {Status::ConditionalOriginalSafariHistory,frame.path,
            frame.frameSeed,row.areaIndex};
}
} // namespace Legality::Gen3SafariIndexedOriginalHistory
