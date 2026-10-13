// SPDX-License-Identifier: AGPL-3.0-or-later
// Exact original Gen I evolution ANCESTRY for encounter source histories.
// Source-pinned pret/pokered and pret/pokeyellow evos_moves.asm, 72 edges.
// This is not a move learnset, true trade history or evolution legality.
#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace Legality::Gen1PretEvolutionStage {
enum class PretEvolutionMethod : uint8_t { Level, Item, Trade };
struct PretEvolutionEdge {
 uint16_t originalSpecies, evolvedSpecies;
 uint8_t minimumEvolutionLevel;
 PretEvolutionMethod method;
 uint8_t gameMask;
};
#include "Legality/Gen1PretEvolutionEdges.inc"
constexpr uint8_t gameMask(std::string_view game) noexcept {
 return game=="red_gb"?1:game=="blue_gb"?2:game=="yellow_gb"?4:0;
}
constexpr bool reachable(uint16_t original,uint16_t present,uint8_t mask) noexcept {
 if(!mask||!(mask==1||mask==2||mask==4))return false;
 if(original==0||present==0||original>151||present>151)return false;
 if(original==present)return true;
 for(const auto& edge:kPretEvolutionEdges) {
   if(edge.originalSpecies!=original||!(edge.gameMask&mask))continue;
   if(edge.evolvedSpecies==present)return true;
   for(const auto& next:kPretEvolutionEdges) {
     if(next.originalSpecies==edge.evolvedSpecies &&
        next.evolvedSpecies==present && (next.gameMask&mask))
        return true;
   }
 }
 return false;
}
enum class Status : uint8_t {
 UnknownOriginalGame,UnknownOriginalSpecies,UnknownCurrentSpecies,
 UnknownLaterGenerationEvolution,UnknownIndependentlyAttestedLineage,
 CandidateLocalAncestryMismatch,SourceGraphCompatibleHistoryUnverified
};
struct Witness {
 std::string_view originalGame{};
 uint16_t originalSpecies=0, currentSpecies=0;
 bool originalGameIndependentlyAttested=false;
 bool originalSpeciesIndependentlyAttested=false;
 bool currentSpeciesIndependentlyAttested=false;
 bool evolutionHistoryIndependentlyAttested=false;
};
struct Result {
 Status status=Status::UnknownOriginalGame;
 bool globallyLegalSupported=false,globalInvalidSupported=false;
};
constexpr Result evaluate(const Witness& w) noexcept {
 const uint8_t mask=gameMask(w.originalGame);
 if(!mask||!w.originalGameIndependentlyAttested)return {};
 if(!w.originalSpeciesIndependentlyAttested||
    !w.originalSpecies||w.originalSpecies>151)
    return {Status::UnknownOriginalSpecies};
 if(!w.currentSpeciesIndependentlyAttested||!w.currentSpecies)
    return {Status::UnknownCurrentSpecies};
 if(w.currentSpecies>151)return {Status::UnknownLaterGenerationEvolution};
 if(w.originalSpecies!=w.currentSpecies &&
    !w.evolutionHistoryIndependentlyAttested)
    return {Status::UnknownIndependentlyAttestedLineage};
 if(!reachable(w.originalSpecies,w.currentSpecies,mask))
    return {Status::CandidateLocalAncestryMismatch};
 return {Status::SourceGraphCompatibleHistoryUnverified};
}
static_assert(kPretEvolutionEdges.size()==72);
static_assert(reachable(133,134,1)&&reachable(133,135,2)&&reachable(133,136,4));
static_assert(!reachable(134,135,1)&&!reachable(135,136,4));
static_assert(reachable(10,12,1)&&!reachable(13,12,1));
} // namespace Legality::Gen1PretEvolutionStage
