// SPDX-License-Identifier: AGPL-3.0-or-later
// Read-only original Gen I scripted gift acquisition evidence.
// Exact original sources (not mutable source-game master):
// pret/pokered@af519899719f0754965776faac0e836a3b906e6d
//   scripts/MtMoonPokecenter.asm blob da3d3bc5fbd07130f6a6c4da686b3e0eaa024b93
//   scripts/CeladonMansionRoofHouse.asm blob 12eff10c28799de278ee2cafa3f5ef743c98d166
// pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382
//   scripts/MtMoonPokecenter_2.asm blob ba5650a235b40c8c9bd41afba72448b3a7b465a3
//   scripts/CeruleanMelaniesHouse.asm blob 8b8095e012a6f6e38e2313907228a966ecd11c31
//   scripts/Route24.asm blob 8e2931ba5cde850e53a3e7a7e265ff15462fc724
//   scripts/VermilionCity_2.asm blob c6cd9478558dc212e88bb4a6a5339eb4ff154237
//   scripts/CeladonMansionRoofHouse.asm blob 330efe0c7801790f31569eb82c673face2a25d39
// Historical conditions are NOT inferred from a detached present-day PK1.
// No result here is a global Legal or Invalid verdict.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace Legality::Gen1PretGiftEventStage {
enum class Gate : uint8_t { OneTimeObject, PaidMagikarp, YellowHappiness147,
                            YellowRoute24, YellowThunderBadge };
struct Scene {
 uint8_t gameMask;
 std::string_view map;
 uint16_t species;
 uint8_t level;
 Gate gate;
 uint16_t sourcePrice;
};
inline constexpr std::array<Scene,5> kScenes{{
 {7,"MtMoonPokecenter",129,5,Gate::PaidMagikarp,500},
 {7,"CeladonMansionRoofHouse",133,25,Gate::OneTimeObject,0},
 {4,"CeruleanMelaniesHouse",1,10,Gate::YellowHappiness147,0},
 {4,"Route24",4,10,Gate::YellowRoute24,0},
 {4,"VermilionCity",7,10,Gate::YellowThunderBadge,0}
}};
constexpr uint8_t mask(std::string_view game) noexcept {
 return game=="red_gb"?1:game=="blue_gb"?2:game=="yellow_gb"?4:0;
}
enum class Status : uint8_t {
 UnknownOriginalGame,UnknownOriginalScene,UnknownOriginalMap,
 UnknownOriginalSpecies,UnknownOriginalLevel,UnknownOriginalEventFlag,
 UnknownOriginalGiftReceipt,UnknownOriginalSceneCondition,
 UnknownOriginalPurchaseReceipt,UnknownOriginalPayment,
 CandidateLocalForeignGame,CandidateLocalMapMismatch,
 CandidateLocalSpeciesMismatch,CandidateLocalLevelMismatch,
 CandidateLocalAlreadyClaimed,CandidateLocalGiftNotAccepted,
 CandidateLocalHappinessBelowThreshold,CandidateLocalBadgeMissing,
 CandidateLocalPaymentMismatch,ConditionalSourceSceneOnly
};
struct Witness {
 std::string_view game{};
 std::size_t sourceIndex=0;
 std::string_view map{};
 uint16_t species=0;
 uint8_t level=0;
 uint16_t paidMoney=0;
 uint8_t pikachuHappiness=0;
 bool thunderBadge=false;
 bool gameAttested=false,sceneAttested=false,mapAttested=false;
 bool speciesAttested=false,levelAttested=false;
 // Flags and successful GivePokemon result AT ORIGINAL REDEMPTION TIME.
 bool priorUnclaimedAttested=false,priorUnclaimed=false;
 bool nativeGiftReceiptAttested=false,nativeGiftSucceeded=false;
 bool originalPaymentAttested=false;
 bool originalPikachuHappinessAttested=false;
 bool originalThunderBadgeAttested=false;
};
struct Result {
 Status status=Status::UnknownOriginalGame;
 uint16_t sourceSpecies=0;
 uint8_t sourceLevel=0;
 bool globalInvalidSupported=false,globallyLegalSupported=false;
};
constexpr Result evaluate(const Witness& w) noexcept {
 const uint8_t gameMask=mask(w.game);
 if(!w.gameAttested||!gameMask)return {};
 if(!w.sceneAttested||w.sourceIndex>=kScenes.size())
     return {Status::UnknownOriginalScene};
 const Scene& s=kScenes[w.sourceIndex];
 if((s.gameMask&gameMask)==0)
     return {Status::CandidateLocalForeignGame,s.species,s.level};
 if(!w.mapAttested||w.map.empty())
     return {Status::UnknownOriginalMap,s.species,s.level};
 if(!w.speciesAttested||w.species==0||w.species>151)
     return {Status::UnknownOriginalSpecies,s.species,s.level};
 if(!w.levelAttested||w.level==0||w.level>100)
     return {Status::UnknownOriginalLevel,s.species,s.level};
 if(w.map!=s.map)
     return {Status::CandidateLocalMapMismatch,s.species,s.level};
 if(w.species!=s.species)
     return {Status::CandidateLocalSpeciesMismatch,s.species,s.level};
 if(w.level!=s.level)
     return {Status::CandidateLocalLevelMismatch,s.species,s.level};
 if(!w.priorUnclaimedAttested)
     return {Status::UnknownOriginalEventFlag,s.species,s.level};
 if(!w.priorUnclaimed)
     return {Status::CandidateLocalAlreadyClaimed,s.species,s.level};
 // The four original source gates cannot be deduced from a subsequent save.
 switch(s.gate) {
 case Gate::YellowHappiness147:
     if(!w.originalPikachuHappinessAttested)
        return {Status::UnknownOriginalSceneCondition,s.species,s.level};
     if(w.pikachuHappiness<147)
        return {Status::CandidateLocalHappinessBelowThreshold,s.species,s.level};
     break;
 case Gate::YellowThunderBadge:
     if(!w.originalThunderBadgeAttested)
        return {Status::UnknownOriginalSceneCondition,s.species,s.level};
     if(!w.thunderBadge)
        return {Status::CandidateLocalBadgeMissing,s.species,s.level};
     break;
 case Gate::PaidMagikarp:
     if(!w.originalPaymentAttested)
        return {Status::UnknownOriginalPayment,s.species,s.level};
     if(w.paidMoney!=s.sourcePrice)
        return {Status::CandidateLocalPaymentMismatch,s.species,s.level};
     break;
 case Gate::OneTimeObject: case Gate::YellowRoute24:
     break;
 }
 if(!w.nativeGiftReceiptAttested)
     return {s.gate==Gate::PaidMagikarp?
       Status::UnknownOriginalPurchaseReceipt:Status::UnknownOriginalGiftReceipt,
       s.species,s.level};
 if(!w.nativeGiftSucceeded)
     return {Status::CandidateLocalGiftNotAccepted,s.species,s.level};
 return {Status::ConditionalSourceSceneOnly,s.species,s.level};
}
static_assert(kScenes.size()==5);
static_assert(kScenes[0].sourcePrice==500);
static_assert(kScenes[2].gameMask==4&&kScenes[2].species==1);
} // namespace Legality::Gen1PretGiftEventStage
