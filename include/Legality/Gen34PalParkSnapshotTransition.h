#pragma once
// TRANSFER-001: exact independently witnessed pre-Pal-Park and immediate
// Gen IV arrival stage. An origin-game marker alone is not transfer proof.
// Gen III HM-known original candidates CANNOT migrate until the HM is
// removed before this stage. No transfer history is stored as a full
// native PK4 move timeline; candidate-local only, never global Invalid.
#include "Legality/Gen34PalParkHMIds.h"
#include <array>
#include <cstdint>
#include <string_view>
namespace Legality::Gen34PalParkSnapshotTransition {
enum class Status : uint8_t {
    UnsupportedGames, UnknownStageWitness, MalformedCandidateSnapshot,
    CandidateContradicted, CandidateConsistent
};
struct Input {
    std::string_view sourceGame{}, destinationGame{};
    uint16_t species=0;
    bool independentlyVerifiedPreTransfer=false;
    bool independentlyVerifiedImmediateArrival=false;
    bool unchangedSpeciesThroughArrivalVerified=false;
    std::array<uint16_t,4> outboundGbaMoves{}, immediateNdsMoves{};
};
struct Result {
    Status status=Status::UnknownStageWitness;
    uint8_t prohibitedHMs=0, retainedSlots=0;
};
constexpr bool sourceGame(std::string_view game) noexcept {
    return game=="ruby_gba"||game=="sapphire_gba"||
        game=="emerald_gba"||game=="firered_gba"||
        game=="leafgreen_gba";
}
constexpr bool destGame(std::string_view game) noexcept {
    return game=="diamond_nds"||game=="pearl_nds"||
        game=="platinum_nds"||game=="heartgold_nds"||
        game=="soulsilver_nds";
}
constexpr bool structural(const std::array<uint16_t,4>& moves,
                          uint16_t ceiling) noexcept {
    bool hadEmpty=false;
    for(uint8_t i=0;i<4;++i) {
        if(!moves[i]){hadEmpty=true;continue;}
        if(moves[i]>ceiling || hadEmpty)return false;
        for(uint8_t j=0;j<i;++j)if(moves[i]==moves[j])return false;
    }
    return true;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result result{};
    if(!sourceGame(in.sourceGame)||!destGame(in.destinationGame) ||
       in.species<1||in.species>386) {
        result.status=Status::UnsupportedGames;return result;
    }
    if(!in.independentlyVerifiedPreTransfer ||
       !in.independentlyVerifiedImmediateArrival ||
       !in.unchangedSpeciesThroughArrivalVerified)return result;
    if(!structural(in.outboundGbaMoves,354) ||
       !structural(in.immediateNdsMoves,467)) {
        result.status=Status::MalformedCandidateSnapshot;return result;
    }
    for(uint16_t move:in.outboundGbaMoves)
        if(Gen34PalParkHMIds::isGen3HM(move))
            ++result.prohibitedHMs;
    if(result.prohibitedHMs) {
        result.status=Status::CandidateContradicted;return result;
    }
    if(in.outboundGbaMoves!=in.immediateNdsMoves) {
        result.status=Status::CandidateContradicted;return result;
    }
    for(uint16_t move:in.outboundGbaMoves)
        if(move)++result.retainedSlots;
    result.status=Status::CandidateConsistent;
    return result;
}
} // namespace Legality::Gen34PalParkSnapshotTransition
