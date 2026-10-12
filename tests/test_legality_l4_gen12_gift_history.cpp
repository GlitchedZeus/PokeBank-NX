#include "Legality/Gen12GiftHistoryEvidence.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace E = Legality::Gen12GiftHistoryEvidence;

template<class T> constexpr E::Datum<T> known(T v) { return {true, v}; }
constexpr E::Name name(uint8_t first) {
    E::Name result{};
    result[0] = first;
    result[1] = 0x50; // test-only terminal byte, not an archival OT name
    return result;
}
constexpr E::GiftRow hypothetical(E::Game game = E::Game::Gold,
                                  E::Kind kind = E::Kind::NPCTrade) {
    E::GiftRow x{};
    x.game = game; x.kind = kind;
    x.originalMechanicIndependentlyVerified = true;
    x.originalRowIndependentlyVerified = true;
    x.originalDistributionAttested = true;
    x.originalReceiptKnown = true;
    x.levelRule = kind == E::Kind::NPCTrade ? E::LevelRule::OfferedTradeLevel : E::LevelRule::Fixed;
    x.species = known<uint16_t>(25);
    x.level = known<uint8_t>(10);
    x.trainerId = known<uint16_t>(43121);
    x.trainerName = known(name('A'));
    x.nickname = known(name('B'));
    x.moves = known(E::Moves4{1, 2, 3, 0});
    return x;
}
constexpr E::Observation observed(E::Game game = E::Game::Gold) {
    E::Observation o{};
    o.game = game;
    o.species = known<uint16_t>(25);
    o.level = known<uint8_t>(10);
    o.trainerId = known<uint16_t>(43121);
    o.trainerName = known(name('A'));
    o.nickname = known(name('B'));
    o.moves = known(E::Moves4{1, 2, 3, 0});
    return o;
}
constexpr E::RouteWitness unchanged() {
    E::RouteWitness h{};
    h.speciesHistory = E::History::Unchanged;
    h.trainerNameHistory = E::History::Unchanged;
    h.nicknameHistory = E::History::Unchanged;
    h.moveHistory = E::History::Unchanged;
    h.offeredTradeLevel = known<uint8_t>(10);
    h.timeCapsule = E::Crossing::NoCrossingVerified;
    return h;
}
static_assert(E::evaluate(hypothetical(), observed(), unchanged()).disposition ==
              E::Disposition::ConditionalCandidate);
static_assert(!E::evaluate(hypothetical(), observed(), unchanged()).authenticated);
static_assert(E::evaluate(hypothetical(E::Game::Red,E::Kind::PCNYGift),observed(),unchanged()).disposition ==
              E::Disposition::Unknown);

int main() {
    using D = E::Disposition;
    auto s = hypothetical(); auto o = observed(); auto h = unchanged();
    auto result = E::evaluate(s, o, h);
    assert(result.disposition == D::ConditionalCandidate && !result.authenticated);
    // Specific original-source failures cannot globally invalidate a Pokemon.
    for (unsigned mask = 0; mask < 16; ++mask) {
        auto x = s;
        x.originalMechanicIndependentlyVerified = (mask & 1u) != 0;
        x.originalRowIndependentlyVerified = (mask & 2u) != 0;
        x.originalDistributionAttested = (mask & 4u) != 0;
        x.originalReceiptKnown = (mask & 8u) != 0;
        x.kind = E::Kind::StadiumGift;
        x.levelRule = E::LevelRule::Fixed;
        auto y = o; y.trainerId.value++;
        auto r = E::evaluate(x, y, h);
        assert(r.disposition == (mask == 15 ? D::CandidateContradicted : D::Unknown));
        if (mask != 15) assert(r.contradicted == 0);
    }
    o.trainerId.value++;
    result = E::evaluate(s,o,h);
    assert(result.disposition == D::CandidateContradicted && (result.contradicted & E::TrainerId));
    o=observed(); o.level.value=9;
    assert(E::evaluate(s,o,h).contradicted & E::Level);
    o=observed(); h=unchanged(); h.offeredTradeLevel.known=false;
    assert(E::evaluate(s,o,h).disposition == D::Unknown);
    h.offeredTradeLevel=known<uint8_t>(12);
    assert(E::evaluate(s,o,h).contradicted & E::Level);
    s.levelRule=E::LevelRule::Fixed; // invalid assumption: NPC trade level is offered-mon level
    assert(E::evaluate(s,o,h).disposition == D::Unknown);
    s=hypothetical();
    o=observed(); o.level.known=false;
    assert(E::evaluate(s,o,h).disposition == D::Unknown);
    o=observed(); o.species.value=26;
    result=E::evaluate(s,o,h);
    assert(result.contradicted & E::Species);
    h.speciesHistory=E::History::Unknown;
    assert(!(E::evaluate(s,o,h).contradicted & E::Species));
    h.speciesHistory=E::History::IndependentlyVerifiedChange;
    h.independentlyVerifiedEvolutionEndpoint=known<uint16_t>(26);
    assert(!(E::evaluate(s,o,h).contradicted & E::Species));
    h.independentlyVerifiedEvolutionEndpoint=known<uint16_t>(27);
    assert(E::evaluate(s,o,h).contradicted & E::Species);
    o=observed(); h=unchanged(); o.trainerName.value=name('C');
    assert(E::evaluate(s,o,h).contradicted & E::TrainerName);
    h.trainerNameHistory=E::History::Unknown;
    assert(E::evaluate(s,o,h).disposition == D::Unknown);
    o=observed(); h=unchanged(); o.nickname.value=name('D');
    assert(E::evaluate(s,o,h).contradicted & E::Nickname);
    h.nicknameHistory=E::History::IndependentlyVerifiedChange;
    assert(!(E::evaluate(s,o,h).contradicted & E::Nickname));
    // Four moves being learnable separately is not a witnessed move history.
    o=observed(); h=unchanged(); o.moves.value={1,2,100,0};
    assert(E::evaluate(s,o,h).contradicted & E::Moves);
    h.moveHistory=E::History::Unknown;
    assert(E::evaluate(s,o,h).disposition == D::Unknown);
    h.moveHistory=E::History::IndependentlyVerifiedChange;
    h.independentlyVerifiedFinalMoves=known(E::Moves4{1,2,100,0});
    assert(!(E::evaluate(s,o,h).contradicted & E::Moves));
    o=observed(E::Game::Red); s=hypothetical(E::Game::Crystal,E::Kind::Gift);
    h=unchanged();
    assert(E::evaluate(s,o,h).disposition == D::Unknown);
    h.timeCapsule=E::Crossing::SnapshotVerified;
    h.crossingSpecies=known<uint16_t>(25);
    h.crossingMoves=known(E::Moves4{1,2,165,0});
    assert(E::evaluate(s,o,h).disposition == D::ConditionalCandidate);
    h.crossingMoves.value[2]=166;
    assert(E::evaluate(s,o,h).contradicted & E::Capsule);
    h.crossingMoves.value[2]=165; h.crossingSpecies.value=152;
    assert(E::evaluate(s,o,h).contradicted & E::Capsule);
    // Present-day Gen II move 166 does not disprove an earlier legal crossing.
    s=hypothetical(E::Game::Red,E::Kind::Gift);
    o=observed(E::Game::Crystal); o.moves.value[0]=166;
    h=unchanged(); h.moveHistory=E::History::Unknown;
    h.timeCapsule=E::Crossing::SnapshotVerified;
    h.crossingSpecies=known<uint16_t>(25);
    h.crossingMoves=known(E::Moves4{1,2,3,0});
    assert(!(E::evaluate(s,o,h).contradicted & E::Capsule));
    // Exhaust all native species and four-move boundary conditions for the
    // *crossing snapshot*, not a mutable future observed party.
    uint32_t states=0, forbidden=0;
    s=hypothetical(E::Game::Gold,E::Kind::Gift);
    o=observed(E::Game::Red); h=unchanged();
    h.timeCapsule=E::Crossing::SnapshotVerified;
    for(uint16_t species=0;species<=252;++species)
    for(uint16_t move=0;move<=252;++move) {
        h.crossingSpecies=known(species);
        h.crossingMoves=known(E::Moves4{1,2,move,0});
        const bool allowed=species>=1 && species<=151 && move<=165;
        const auto r=E::evaluate(s,o,h);
        assert(((r.contradicted & E::Capsule)==0)==allowed);
        assert(!r.authenticated);
        ++states; if (!allowed) ++forbidden;
    }
    assert(states==64009 && forbidden>0);
    std::cout << "L4 Gen I/II gift history: " << states << " crossing combinations, "
              << forbidden << " candidate-only contradictions: PASS\n";
}
