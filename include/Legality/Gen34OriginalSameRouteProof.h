#pragma once
// L2 Stage 5: same ORIGINAL egg, deposited parents, game, event and
// creation-held items across acquisition, PID, IV and four move stages.
// Read-only conditional evidence; no detached PK3/PK4 can attest history.
#include <array>
#include <cstdint>
#include <string_view>
namespace Legality::Gen34OriginalSameRouteProof {
enum class Game : uint8_t {
    Unknown, Ruby, Sapphire, Emerald, FireRed, LeafGreen,
    Diamond, Pearl, Platinum, HeartGold, SoulSilver
};
enum class Status : uint8_t {
    NotSupplied, UnknownSourceEvidence, CrossedOriginalStages,
    ConditionallySameRoute
};
struct Stage {
    bool independentlyAttested=false;
    bool physicalOrderAttested=false;
    bool heldItemsAttested=false;
    Game game=Game::Unknown;
    uint64_t eventId=0,eggSnapshotId=0,pairId=0;
    uint64_t slot0Id=0,slot1Id=0;
    uint16_t held0=0,held1=0;
};
struct Input {
    bool supplied=false;
    // Ordered: original acquisition, pending/collected PID, inherited IV,
    // four-move construction. Historical source records, NOT hypotheses
    // assembled by comparing present-day egg stats.
    std::array<Stage,4> stage{};
};
struct Expected {
    Game game=Game::Unknown;
    uint64_t eventId=0,eggSnapshotId=0,pairId=0;
    uint64_t slot0Id=0,slot1Id=0;
    bool itemsKnown=false;
    uint16_t held0=0,held1=0;
};
// Explicit original-game ID mapping: a Gen III or IV PID witness
// cannot be joined to a different retail game's IV/move transcript.
constexpr Game gameFor(std::string_view game) noexcept {
    return game=="ruby_gba"?Game::Ruby:
           game=="sapphire_gba"?Game::Sapphire:
           game=="emerald_gba"?Game::Emerald:
           game=="firered_gba"?Game::FireRed:
           game=="leafgreen_gba"?Game::LeafGreen:
           game=="diamond_nds"?Game::Diamond:
           game=="pearl_nds"?Game::Pearl:
           game=="platinum_nds"?Game::Platinum:
           game=="heartgold_nds"?Game::HeartGold:
           game=="soulsilver_nds"?Game::SoulSilver:Game::Unknown;
}
static_assert(gameFor("heartgold_nds")==Game::HeartGold);
static_assert(gameFor("leafgreen_gba")==Game::LeafGreen);
static_assert(gameFor("white_nds")==Game::Unknown);
constexpr bool supported(Game game) noexcept {
    return game>=Game::Ruby && game<=Game::SoulSilver;
}
constexpr Status evaluate(const Input& in,const Expected& expected) noexcept {
    if(!in.supplied)return Status::NotSupplied;
    if(!supported(expected.game) || !expected.eventId ||
       !expected.eggSnapshotId || !expected.pairId ||
       !expected.slot0Id || !expected.slot1Id ||
       expected.slot0Id==expected.slot1Id)
        return Status::UnknownSourceEvidence;
    if(!expected.itemsKnown)return Status::UnknownSourceEvidence;
    for(const auto& s:in.stage) {
        if(!s.independentlyAttested || !s.physicalOrderAttested ||
           !s.heldItemsAttested || !supported(s.game) ||
           !s.eventId || !s.eggSnapshotId || !s.pairId ||
           !s.slot0Id || !s.slot1Id || s.slot0Id==s.slot1Id)
            return Status::UnknownSourceEvidence;
        if(s.game!=expected.game || s.eventId!=expected.eventId ||
           s.eggSnapshotId!=expected.eggSnapshotId ||
           s.pairId!=expected.pairId ||
           s.slot0Id!=expected.slot0Id || s.slot1Id!=expected.slot1Id ||
           s.held0!=expected.held0 || s.held1!=expected.held1)
            return Status::CrossedOriginalStages;
    }
    return Status::ConditionallySameRoute;
}
static_assert(evaluate({}, {})==Status::NotSupplied);
} // namespace Legality::Gen34OriginalSameRouteProof
