#include "Legality/Gen14EventCoordinatorRoutes.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
namespace A=Legality::Gen14EventCoordinatorRoutes;
namespace W=Legality::Gen3DistributionWitnessEvidence;
namespace G=Legality::Gen4MysteryGiftContainerEvidence;
namespace H=Legality::Gen4MysteryGiftHistoryEvidence;
int main(){
    unsigned cases=0;
    W::Result g3{};g3.nativeAvailable=true;
    g3.source=W::SourceStatus::ConditionalSource;
    for(uint32_t mismatch=0;mismatch<(1u<<W::FIELD_COUNT);++mismatch){
        g3.mismatchMask=mismatch;
        g3.unknownMask=0;
        const auto r=A::classifyGen3(g3);
        assert(r.state==(mismatch?A::LaneState::CandidateExcluded:A::LaneState::ConditionalEvidence));
        assert(r.contradicted==mismatch&&!r.authenticated&&!r.globallyInvalid);
        ++cases;
    }
    g3.mismatchMask=0;
    for(unsigned u=1;u<=W::FIELD_COUNT;++u){
        g3.unknownMask=(1u<<u)-1u;
        const auto r=A::classifyGen3(g3);
        assert(r.state==A::LaneState::UnknownHistory);
        assert(r.unknown==g3.unknownMask);
        ++cases;
    }
    g3.unknownMask=0;
    for(unsigned s=0;s<=static_cast<unsigned>(W::SourceStatus::RouteContradicted);++s){
        g3.source=static_cast<W::SourceStatus>(s);
        const auto r=A::classifyGen3(g3);
        assert(r.state==(g3.source==W::SourceStatus::ConditionalSource?
            A::LaneState::ConditionalEvidence:A::LaneState::UnknownSource));
        ++cases;
    }
    H::Result g4{};G::Projection projection{};
    for(unsigned t=0;t<=15;++t){
        projection.type=static_cast<G::GiftType>(t);
        projection.delivery=G::delivery(projection.type);
        g4.status=H::Status::ConditionalCandidate;
        const auto r=A::classifyGen4(g4,projection);
        const bool pokemon=projection.delivery==G::Delivery::DirectPokemon ||
                           projection.delivery==G::Delivery::RangerManaphyEgg;
        assert(r.state==(pokemon?A::LaneState::ConditionalEvidence:A::LaneState::UnsupportedNonGift));
        assert(!r.authenticated&&!r.globallyInvalid);
        ++cases;
    }
    projection.delivery=G::Delivery::DirectPokemon;
    const std::array<H::Status,6> states={
        H::Status::UnavailableNative,H::Status::UnknownOriginalSource,
        H::Status::UnknownRangerReceipt,H::Status::CandidateContradicted,
        H::Status::UnresolvedHistory,H::Status::ConditionalCandidate};
    const std::array<A::LaneState,6> expected={
        A::LaneState::Unavailable,A::LaneState::UnknownSource,
        A::LaneState::UnknownHistory,A::LaneState::CandidateExcluded,
        A::LaneState::UnknownHistory,A::LaneState::ConditionalEvidence};
    for(size_t i=0;i<states.size();++i) {
        g4.status=states[i];g4.contradicted=7;g4.unknown=3;
        const auto r=A::classifyGen4(g4,projection);
        assert(r.state==expected[i]);
        assert(r.authenticated==false&&r.globallyInvalid==false);
        ++cases;
    }
    // A failed candidate cannot override missing corroboration from the
    // other lane or become global validity or invalidity.
    for(unsigned outcome=0;outcome<=static_cast<unsigned>(A::J::Outcome::ConditionalCandidate);++outcome){
        A::J::Result joint{};joint.outcome=static_cast<A::J::Outcome>(outcome);
        for(unsigned lane=0;lane<=static_cast<unsigned>(A::LaneState::ConditionalEvidence);++lane){
            A::Gate gate{};gate.state=static_cast<A::LaneState>(lane);
            const auto observed=A::reconcile(joint,gate);
            if(joint.outcome!=A::J::Outcome::ConditionalCandidate)
                assert(observed==joint.outcome);
            ++cases;
        }
    }
    std::cout<<"L4 Stage 5 G3/G4 family and cross-lane severity isolation: "<<cases<<" PASS\n";
}
