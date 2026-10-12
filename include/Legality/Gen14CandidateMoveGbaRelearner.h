#pragma once
// L3 M07/M12: original GBA level-up-move relearner menu source behavior.
// Sources pinned: pret/pokeruby@63a8cbf0016b351a4e68f7036fa0b77e23d2f2c1,
// src/pokemon_3.c GetMoveTutorMoves (Ruby/Sapphire);
// pret/pokeemerald@5eff78649e7170a877b961ef0b3da13b81a16038
// and pret/pokefirered@c75f352304d529f6ba92d4f74b9cf8b5c3810788,
// src/pokemon.c GetMoveRelearnerMoves. Exactly versioned species table,
// ordered deduplication, exclude known/currently unavailable higher-level moves.
// Source visit, access and actual player teaching are caller-attested, NEVER
// inferred from a detached PK3. All five games independently source-pinned.
#include "Legality/Gen14CandidateMoveGbaLevelUpTimeline.h"
#include <array>
#include <cstdint>
namespace Legality::Gen14CandidateMoveGbaRelearner {
namespace C=Gen14CandidateMoveChronology;
namespace L=Gen3GbaVersionedLevelUpTimeline;
using Game=C::Game;
enum class Status : uint8_t { Unresolved, CandidateConsistent, CandidateContradicted, Unsupported };
struct Input {
    uint32_t candidateId=0;
    Game game=Game::Unknown;
    uint16_t nationalSpecies=0;
    uint8_t level=0;
    std::array<uint16_t,4> beforeMoves{}, afterMoves{};
    uint16_t selectedMove=0;
    uint8_t replacementSlot=0;
    bool originalPokemonAndSpeciesVerified=false;
    bool originalGameAndServiceAccessVerified=false;
    bool exactLevelAndFourSlotsVerified=false;
    bool exactPlayerChoiceVerified=false;
};
struct Result {
    Status status=Status::Unresolved;
    std::array<uint16_t,32> offers{};
    std::array<uint16_t,4> rebuilt{};
    uint8_t offerCount=0, skippedKnown=0, skippedDuplicate=0, skippedHighLevel=0;
};
constexpr bool sourceSupported(Game game,uint16_t species) noexcept {
    return C::generation(game)==3 && L::supported(game,species);
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(C::generation(in.game)!=3 || !in.nationalSpecies ||
       in.nationalSpecies>386 || !in.level || in.level>100 ||
       !in.selectedMove || in.selectedMove>354) {
        out.status=Status::Unsupported;return out;
    }
    if(!in.candidateId || !in.originalPokemonAndSpeciesVerified ||
       !in.originalGameAndServiceAccessVerified ||
       !in.exactLevelAndFourSlotsVerified || !in.exactPlayerChoiceVerified ||
       !sourceSupported(in.game,in.nationalSpecies) ||
       !C::structural(in.beforeMoves,354) ||
       !C::structural(in.afterMoves,354))return out;
    // Same ordering and filtering as the pinned retail function:
    // read original current-species table, level threshold, skip known
    // four slots, skip duplicates among the offered menu.
    for(uint16_t i=0;i<L::offerCount(in.game,in.nationalSpecies);++i) {
        const auto offer=L::offerAt(in.game,in.nationalSpecies,i);
        if(offer.level>in.level) {++out.skippedHighLevel;continue;}
        if(C::contains(in.beforeMoves,offer.move)) {
            ++out.skippedKnown;continue;
        }
        bool seen=false;
        for(uint8_t j=0;j<out.offerCount;++j)
            if(out.offers[j]==offer.move) {seen=true;break;}
        if(seen) {++out.skippedDuplicate;continue;}
        if(out.offerCount>=out.offers.size())return out;
        out.offers[out.offerCount++]=offer.move;
    }
    bool selected=false;
    for(uint8_t i=0;i<out.offerCount;++i)
        if(out.offers[i]==in.selectedMove){selected=true;break;}
    if(!selected) {
        out.status=Status::CandidateContradicted;return out;
    }
    out.rebuilt=in.beforeMoves;
    const uint8_t open=C::freeSlot(out.rebuilt);
    if(in.replacementSlot>=4 || (open<4 && in.replacementSlot!=open)) {
        out.status=Status::CandidateContradicted;return out;
    }
    out.rebuilt[in.replacementSlot]=in.selectedMove;
    out.status=out.rebuilt==in.afterMoves?
        Status::CandidateConsistent:Status::CandidateContradicted;
    return out;
}
static_assert(sourceSupported(Game::Ruby,1));
static_assert(sourceSupported(Game::Sapphire,4));
static_assert(sourceSupported(Game::Emerald,386));
static_assert(sourceSupported(Game::LeafGreen,4));
static_assert(sourceSupported(Game::FireRed,133));
} // namespace Legality::Gen14CandidateMoveGbaRelearner
