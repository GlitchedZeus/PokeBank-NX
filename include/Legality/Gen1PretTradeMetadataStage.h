// SPDX-License-Identifier: AGPL-3.0-or-later
// Original English R/B/Y NPC trade identity, no invented static OT IDs.
// pret/pokered@af519899719f0754965776faac0e836a3b906e6d
//  engine/events/in_game_trades.asm blob 3107550fcd2d059a1786e8e1a68709b2c20b3377
// pret/pokeyellow@e89ead154b9968aa50eed9328ff2b38b6c194382
//  engine/events/in_game_trades.asm blob 8cde05c078dc90e6b5d032493e67c4b59fa26d1a
// Both copy the source nickname from TradeMons, OT literal "<TRAINER>",
// and make a NEW OT ID by calling Random and copying hRandomAdd bytes.
// Yellow additionally executes InGameTrade_CheckForTradeEvo after receipt;
// Yellow index 9 CUBONE -> MACHOKE can immediately become MACHAMP.
// Neither an observed current nickname nor OT is proof of original history.
#pragma once
#include <cstdint>
#include <string_view>
namespace Legality::Gen1PretTradeMetadataStage {
enum class Status : uint8_t {
 UnknownOriginalGame,UnknownOriginalSourceRow,UnknownUnreachableOffer,
 UnknownOriginalNickname,UnknownOriginalTrainerString,
 UnknownOriginalReceivedStage,UnknownOriginalPostTradeEvolution,
 CandidateLocalNicknameMismatch,CandidateLocalTrainerStringMismatch,
 CandidateLocalReceivedSpeciesMismatch,ConditionalOriginalNpcMetadataOnly
};
struct Witness {
 std::string_view originalGame{};
 std::string_view originalReceivedNickname{};
 std::string_view originalReceivedOT{};
 uint8_t originalOfferIndex=0;
 uint16_t originalPostTradeSpecies=0;
 bool originalGameAttested=false,sourceRowAttested=false;
 bool originalNicknameAttested=false,originalTrainerNameAttested=false;
 bool originalPostTradeSpeciesAttested=false;
 bool originalPostTradeEvolutionAttested=false;
 bool originalTradeEvolutionCompleted=false;
};
struct Result {
 Status status=Status::UnknownOriginalGame;
 uint16_t sourceReceivedSpecies=0,sourcePostTradeSpecies=0;
 bool originalOTIDWasSourceFixed=false;
 bool globallyLegalSupported=false,globalInvalidSupported=false;
};
constexpr uint8_t gameMask(std::string_view game) noexcept {
 return game=="red_gb"?1:game=="blue_gb"?2:
        game=="yellow_gb"?4:0;
}
constexpr uint16_t originalImmediateSpecies(uint8_t game,
       uint8_t index,uint16_t species,bool completed) noexcept {
 // The only source-listed English Yellow NPC trade that returns a
 // level-up/trade-evolution species is CUBONE for MACHOKE at slot 9.
 if(game==4&&index==9&&species==67&&completed)return 68;
 return species;
}
template<class OriginalOffer>
constexpr Result evaluate(const OriginalOffer& row,
                          const Witness& w) noexcept {
 const uint8_t game=gameMask(w.originalGame);
 if(!w.originalGameAttested||!game)return {};
 if(!w.sourceRowAttested ||
    !(row.gameMask&game) || row.originalOfferIndex!=w.originalOfferIndex)
    return {Status::UnknownOriginalSourceRow};
 if(!row.reachable)return {Status::UnknownUnreachableOffer};
 if(!w.originalNicknameAttested)
    return {Status::UnknownOriginalNickname,row.receivedSpecies};
 if(w.originalReceivedNickname!=row.nickname)
    return {Status::CandidateLocalNicknameMismatch,row.receivedSpecies};
 if(!w.originalTrainerNameAttested)
    return {Status::UnknownOriginalTrainerString,row.receivedSpecies};
 if(w.originalReceivedOT!="<TRAINER>")
    return {Status::CandidateLocalTrainerStringMismatch,row.receivedSpecies};
 if(!w.originalPostTradeSpeciesAttested ||
    w.originalPostTradeSpecies==0 || w.originalPostTradeSpecies>151)
    return {Status::UnknownOriginalReceivedStage,row.receivedSpecies};
 const bool couldEvolve=game==4&&row.originalOfferIndex==9&&
                        row.receivedSpecies==67;
 if(couldEvolve && (w.originalPostTradeSpecies==68 ||
                     w.originalTradeEvolutionCompleted) &&
    !w.originalPostTradeEvolutionAttested)
    return {Status::UnknownOriginalPostTradeEvolution,row.receivedSpecies};
 const uint16_t expected=originalImmediateSpecies(game,
     row.originalOfferIndex,row.receivedSpecies,
     couldEvolve && w.originalTradeEvolutionAttested &&
                  w.originalTradeEvolutionCompleted);
 if(w.originalPostTradeSpecies!=expected)
    return {Status::CandidateLocalReceivedSpeciesMismatch,
             row.receivedSpecies,expected};
 // Gen I original ID is random, not fixed to one Trainer ID and not
 // constrained to a numeric range narrower than uint16_t. DVs and
 // post-trade naming/tradeback remain unverified original histories.
 return {Status::ConditionalOriginalNpcMetadataOnly,
         row.receivedSpecies,expected};
}
static_assert(originalImmediateSpecies(4,9,67,true)==68);
static_assert(originalImmediateSpecies(1,9,67,true)==67);
} // namespace Legality::Gen1PretTradeMetadataStage
