#pragma once
// L1 E05: original Gen II roamer location-transition SOURCE graph.
// Exact pinned pret/pokegold@ef0201d8daf47e8b3ea1518eacf890f37d4cd5e8
// and pret/pokecrystal@3bc8daa4173e96a7f4011dad3922eb6fa5dad5c6
// data/wild/roammon_maps.asm: 16 routing nodes, directed permitted edges.
// Route 40 and Route 41 absent. An independently observed transition is
// required; this graph is NOT a proof of original roamer catch/save history.
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace Legality::Gen2OriginalRoamerRouteStage {
enum class Status : uint8_t {
    UnknownOriginalGame, UnknownTransition, UnknownOriginalSourceMap,
    UnknownOriginalDestinationMap, UnknownOriginalRouteWitness,
    CandidateOriginalTransitionContradicted, ConditionalSourceTransitionOnly
};
struct RouteRow {uint8_t from,to;};
inline constexpr auto kRouteEdges=std::to_array<RouteRow>({
    {29,30},
    {29,46},
    {30,29},
    {30,31},
    {31,30},
    {31,32},
    {31,36},
    {32,36},
    {32,31},
    {32,33},
    {33,32},
    {33,34},
    {34,33},
    {34,35},
    {35,34},
    {35,36},
    {36,35},
    {36,31},
    {36,32},
    {36,37},
    {37,36},
    {37,38},
    {37,42},
    {38,37},
    {38,39},
    {38,42},
    {39,38},
    {42,43},
    {42,44},
    {42,37},
    {42,38},
    {43,42},
    {43,44},
    {44,42},
    {44,43},
    {44,45},
    {45,44},
    {45,46},
    {46,45},
    {46,29},
});
struct Candidate {
    std::string_view originalGame{};
    uint8_t originalSourceMap=0,originalDestinationMap=0;
    bool originalGameAttested=false,sourceMapAttested=false;
    bool destinationMapAttested=false,transitionIndependentlyAttested=false;
};
constexpr bool knownSourceMap(uint8_t map) noexcept {
    for(const auto row:kRouteEdges)if(row.from==map)return true;
    return false;
}
constexpr Status evaluate(const Candidate& c) noexcept {
    if(!c.originalGameAttested ||
       (c.originalGame!="gold_gbc" && c.originalGame!="silver_gbc" &&
        c.originalGame!="crystal_gbc"))return Status::UnknownOriginalGame;
    if(!c.transitionIndependentlyAttested)
        return Status::UnknownTransition;
    if(!c.sourceMapAttested || !knownSourceMap(c.originalSourceMap))
        return Status::UnknownOriginalSourceMap;
    if(!c.destinationMapAttested || !knownSourceMap(c.originalDestinationMap))
        return Status::UnknownOriginalDestinationMap;
    for(const auto row:kRouteEdges)
        if(row.from==c.originalSourceMap && row.to==c.originalDestinationMap)
            return Status::ConditionalSourceTransitionOnly;
    // Contradicts this OBSERVED jump, never an arbitrary roamer's origin.
    return Status::CandidateOriginalTransitionContradicted;
}
} // namespace Legality::Gen2OriginalRoamerRouteStage