#include "Legality/Gen3OriginalEggAcquisitionTranscript.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace A=Legality::Gen3OriginalEggAcquisitionTranscript;
constexpr std::uint32_t next(std::uint32_t& x) noexcept {
    x^=x<<13; x^=x>>17; x^=x<<5; return x;
}
int main(){
    std::uint64_t checks=0;
    std::uint32_t state=0x7105BA55u;
    for(auto source:{A::Family::RubySapphireFireRedLeafGreen,A::Family::Emerald}) {
        for(unsigned i=0;i<12000;++i) {
            A::Expected x{};
            x.source=source;x.stageIdentity=1000+i;
            x.physicalSlot0Snapshot=101;x.physicalSlot1Snapshot=202;
            x.pendingPersonality=source==A::Family::Emerald?next(state):(next(state)&0xffffu);
            x.finalPersonality=next(state);
            A::Transcript t{};t.supplied=true;
            t.pendingPayloadIndependentlyVerified=true;
            t.collectedPayloadIndependentlyVerified=true;
            t.originalPhysicalPairIndependentlyVerified=true;
            t.stageIdentityIndependentlyVerified=true;
            t.originalStageIdentity=x.stageIdentity;
            t.physicalSlot0Snapshot=x.physicalSlot0Snapshot;
            t.physicalSlot1Snapshot=x.physicalSlot1Snapshot;
            t.archivedPendingPayload=x.pendingPersonality;
            t.archivedCollectedPersonality=x.finalPersonality;
            assert(A::evaluate(t,x)==A::Status::ConditionallyMatched);++checks;
            auto wrong=t;wrong.archivedPendingPayload^=1u;
            assert(A::evaluate(wrong,x)==A::Status::PendingPayloadContradicted);++checks;
            wrong=t;wrong.archivedCollectedPersonality^=1u;
            assert(A::evaluate(wrong,x)==A::Status::CollectionPayloadContradicted);++checks;
            wrong=t;wrong.originalStageIdentity++;
            assert(A::evaluate(wrong,x)==A::Status::IncoherentHistoricalSource);++checks;
            wrong=t;wrong.physicalSlot0Snapshot++;
            assert(A::evaluate(wrong,x)==A::Status::IncoherentHistoricalSource);++checks;
            wrong=t;wrong.physicalSlot1Snapshot++;
            assert(A::evaluate(wrong,x)==A::Status::IncoherentHistoricalSource);++checks;
            wrong=t;wrong.supplied=false;
            assert(A::evaluate(wrong,x)==A::Status::NotProvided);++checks;
            wrong=t;wrong.pendingPayloadIndependentlyVerified=false;
            assert(A::evaluate(wrong,x)==A::Status::UnknownSourceEvidence);++checks;
            wrong=t;wrong.collectedPayloadIndependentlyVerified=false;
            assert(A::evaluate(wrong,x)==A::Status::UnknownSourceEvidence);++checks;
            wrong=t;wrong.originalPhysicalPairIndependentlyVerified=false;
            assert(A::evaluate(wrong,x)==A::Status::UnknownSourceEvidence);++checks;
            wrong=t;wrong.stageIdentityIndependentlyVerified=false;
            assert(A::evaluate(wrong,x)==A::Status::UnknownSourceEvidence);++checks;
            if(source==A::Family::RubySapphireFireRedLeafGreen){
                wrong=t;wrong.archivedPendingPayload|=0x10000u;
                assert(A::evaluate(wrong,x)==A::Status::IncoherentHistoricalSource);++checks;
            } else {
                wrong=t;wrong.archivedPendingPayload|=0x10000u;
                const auto r=A::evaluate(wrong,x);
                assert(r==A::Status::ConditionallyMatched || r==A::Status::PendingPayloadContradicted);++checks;
            }
            wrong=t;wrong.physicalSlot0Snapshot=wrong.physicalSlot1Snapshot;
            assert(A::evaluate(wrong,x)==A::Status::UnknownSourceEvidence);++checks;
        }
    }
    assert(checks==312000);
    std::cout<<"L2 original Gen III pending/collection transcript: "<<checks<<" PASS\n";
}
