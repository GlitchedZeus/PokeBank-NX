#pragma once
// L1 Stage 5: bounded, read-only original Gen IV acquisition histories.
// Each route independently attests one physical save and capture identity;
// paths are NOT interchangeable by sharing PK4 PID/species/IVs. No result
// supports global Invalid or Legal, even when all supplied paths disagree.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
namespace Legality::Gen4OriginalAcquisitionReconciliation {
inline constexpr std::size_t kCapacity=16;
enum class Game : uint8_t { Unknown,Diamond,Pearl,Platinum,HeartGold,SoulSilver };
enum class Family : uint8_t {
 Unknown,SinnohOrdinary,SinnohHoney,SinnohRadar,SinnohChainShiny,
 HgssHeadbutt,HgssFishing,HgssSafari,HgssBugContest,HgssRockSmash,
 StaticGift,GiftEgg,Roamer
};
enum class Stage : uint8_t { Unknown,SourceTemplate,OriginalRngMatched,
                             CandidateContradiction,BoundedExhausted };
enum class Status : uint8_t {
 UnknownRoot,UnknownOriginalGame,UnknownOriginalSource,UnknownOriginalContext,
 UnknownStage,UnknownSearchBound,UnknownAlternativeHistory,
 CandidateLocalContradiction,ConditionalOriginalSourceOnly,
 ConditionalOriginalRngStageOnly
};
struct Candidate {
 uint64_t originalSaveIdentity=0,physicalCreatureIdentity=0,routeIdentity=0;
 Game game=Game::Unknown;
 Family family=Family::Unknown;
 Stage stage=Stage::Unknown;
 bool originalSaveAndCreatureIndependentlyAttested=false;
 bool originalRouteAndFamilyIndependentlyAttested=false;
 bool originalSourceRowIndependentlyAttested=false;
 bool originalCaptureContextIndependentlyAttested=false;
 bool originalPidIvStageIndependentlyAttested=false;
 // These flags describe independently archived acquisition-time evidence,
 // NOT current save state, current trainer, a detached PK4 or inferred RNG.
 bool originalHoneySlatherTimerAndTreeAttested=false;
 bool originalHoneyTrainerForMunchlaxAttested=false;
 bool originalRadarActivationAttested=false;
 bool originalRadarChainHistoryAttested=false;
 bool originalTreePositionAndGroupAttested=false;
 bool originalFishingRodTimeAndSwarmAttested=false;
 bool originalSafariBlocksAndTimeAttested=false;
 bool originalBugContestDayAndRerollAttested=false;
 bool originalRockSmashAreaAndRateAttested=false;
 bool originalGiftOrEggSceneAttested=false;
 bool originalRoamerActivationAndRouteAttested=false;
};
struct Result {
 Status status=Status::UnknownRoot;
 bool globalInvalidSupported=false,globallyLegalSupported=false;
};
constexpr bool recognizedGame(Game g) noexcept {
 return g==Game::Diamond||g==Game::Pearl||g==Game::Platinum||
        g==Game::HeartGold||g==Game::SoulSilver;
}
constexpr bool hgss(Game g) noexcept {return g==Game::HeartGold||g==Game::SoulSilver;}
constexpr bool recognizedFamily(Family f) noexcept {
 return f>=Family::SinnohOrdinary && f<=Family::Roamer;
}
constexpr bool pathMatchesGame(Game g,Family f) noexcept {
 if(!recognizedGame(g)||!recognizedFamily(f))return false;
 if(f>=Family::SinnohOrdinary&&f<=Family::SinnohChainShiny)return !hgss(g);
 if(f>=Family::HgssHeadbutt&&f<=Family::HgssRockSmash)return hgss(g);
 return true;
}
constexpr bool originalContextComplete(const Candidate& c) noexcept {
 switch(c.family) {
 case Family::SinnohOrdinary:return true;
 case Family::SinnohHoney:return c.originalHoneySlatherTimerAndTreeAttested &&
                                  c.originalHoneyTrainerForMunchlaxAttested;
 case Family::SinnohRadar:return c.originalRadarActivationAttested;
 case Family::SinnohChainShiny:return c.originalRadarActivationAttested &&
                                      c.originalRadarChainHistoryAttested;
 case Family::HgssHeadbutt:return c.originalTreePositionAndGroupAttested;
 case Family::HgssFishing:return c.originalFishingRodTimeAndSwarmAttested;
 case Family::HgssSafari:return c.originalSafariBlocksAndTimeAttested;
 case Family::HgssBugContest:return c.originalBugContestDayAndRerollAttested;
 case Family::HgssRockSmash:return c.originalRockSmashAreaAndRateAttested;
 case Family::StaticGift:case Family::GiftEgg:
     return c.originalGiftOrEggSceneAttested;
 case Family::Roamer:return c.originalRoamerActivationAndRouteAttested;
 default:return false;
 }
}
constexpr Result evaluate(const Candidate& c) noexcept {
 if(!c.originalSaveIdentity || !c.physicalCreatureIdentity ||
    !c.originalSaveAndCreatureIndependentlyAttested)return {};
 if(!recognizedGame(c.game))return {Status::UnknownOriginalGame};
 if(!c.routeIdentity || !c.originalRouteAndFamilyIndependentlyAttested ||
    !pathMatchesGame(c.game,c.family) || !c.originalSourceRowIndependentlyAttested)
     return {Status::UnknownOriginalSource};
 if(!c.originalCaptureContextIndependentlyAttested ||
    !originalContextComplete(c))return {Status::UnknownOriginalContext};
 switch(c.stage) {
 case Stage::SourceTemplate:return {Status::ConditionalOriginalSourceOnly};
 case Stage::OriginalRngMatched:
     return {c.originalPidIvStageIndependentlyAttested?
         Status::ConditionalOriginalRngStageOnly:Status::UnknownStage};
 case Stage::CandidateContradiction:
     return {c.originalPidIvStageIndependentlyAttested?
         Status::CandidateLocalContradiction:Status::UnknownStage};
 case Stage::BoundedExhausted:return {Status::UnknownSearchBound};
 default:return {Status::UnknownStage};
 }
}
enum class UniverseStatus : uint8_t {
 UnknownRoot,UnknownCapacity,UnknownRouteIdentity,UnknownAlternatives,
 ProvidedPathsLocallyContradictedOnly,OneConditionalSourcePath
};
struct UniverseResult {
 UniverseStatus status=UniverseStatus::UnknownRoot;
 uint8_t compatible=0,contradicted=0,unknown=0;
 bool globalInvalidSupported=false,globallyLegalSupported=false;
};
constexpr UniverseResult reconcile(uint64_t specimenRoot,bool rootAttested,
                                    std::span<const Candidate> routes) noexcept {
 UniverseResult out{};
 if(!specimenRoot||!rootAttested)return out;
 if(routes.size()>kCapacity){out.status=UniverseStatus::UnknownCapacity;return out;}
 for(std::size_t i=0;i<routes.size();++i){
     if(!routes[i].routeIdentity ||
        routes[i].physicalCreatureIdentity!=specimenRoot){
         out.status=UniverseStatus::UnknownRouteIdentity;return out;
     }
     for(std::size_t j=0;j<i;++j)
         if(routes[i].routeIdentity==routes[j].routeIdentity){
             out.status=UniverseStatus::UnknownRouteIdentity;return out;
         }
 }
 for(const auto& c:routes){
     const auto r=evaluate(c).status;
     if(r==Status::ConditionalOriginalSourceOnly ||
        r==Status::ConditionalOriginalRngStageOnly)++out.compatible;
     else if(r==Status::CandidateLocalContradiction)++out.contradicted;
     else ++out.unknown;
 }
 if(out.compatible)out.status=UniverseStatus::OneConditionalSourcePath;
 else if(out.unknown||routes.empty())out.status=UniverseStatus::UnknownAlternatives;
 else out.status=UniverseStatus::ProvidedPathsLocallyContradictedOnly;
 return out;
}
static_assert(kCapacity==16);
} // namespace Legality::Gen4OriginalAcquisitionReconciliation
