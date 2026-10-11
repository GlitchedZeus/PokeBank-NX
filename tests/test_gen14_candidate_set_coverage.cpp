#include "Legality/Gen14CandidateSetCoverage.h"
#include <cassert>
#include <iostream>
namespace A=Legality::Gen14CandidateSetCoverage;
namespace H=Legality::Gen14CandidateMoveHistory;
void runCases() {
    A::Input in{};
    in.count=2;
    in.allOriginalEncounterCandidatesEnumerated=true;
    for(uint8_t i=0;i<2;++i){
        auto& x=in.alternatives[i];
        x.originalEncounterIndependentlyGrounded=true;
        x.candidate={uint32_t(100+i),50,5,true,3};
        x.moves[0].move=85;
        x.moves[0].exhaustive=true;
        x.moves[0].count=1;
        x.moves[0].options[0]={85,uint32_t(100+i),
            H::Method::LevelUp,10,10,1,0,0,true};
    }
    // This is a genuinely supported conditional level-up path.
    auto compatible=A::evaluate(in);
    assert(compatible.status==A::Status::OneConditionalRoute);
    assert(compatible.compatible==2 && compatible.contradicted==0);
    // Removing all matching acquisition methods disproves only
    // these TWO candidates if and only if enumeration is known.
    auto contradicted=in;
    for(auto& entry:contradicted.alternatives)
        entry.moves[0].options[0].candidateId=999;
    auto exhausted=A::evaluate(contradicted);
    assert(exhausted.status==A::Status::AllGroundedCandidatesContradicted);
    assert(exhausted.contradicted==2);
    auto incompleteUniverse=contradicted;
    incompleteUniverse.allOriginalEncounterCandidatesEnumerated=false;
    assert(A::evaluate(incompleteUniverse).status==
           A::Status::UnknownSourceCandidateEnumeration);
    auto incompleteRoute=contradicted;
    incompleteRoute.alternatives[1].moves[0].exhaustive=false;
    assert(A::evaluate(incompleteRoute).status==
           A::Status::UnknownAcquisitionMethods);
    auto unknownEncounter=contradicted;
    unknownEncounter.alternatives[0].originalEncounterIndependentlyGrounded=false;
    assert(A::evaluate(unknownEncounter).status==
           A::Status::UnknownCandidateProvenance);
    auto alternative=in;
    alternative.alternatives[0].moves[0].options[0].candidateId=999;
    assert(A::evaluate(alternative).status==A::Status::OneConditionalRoute);
    auto unknownMethod=in;
    unknownMethod.alternatives[0].moves[0].options[0].methodVerified=false;
    unknownMethod.alternatives[1].moves[0].options[0].candidateId=999;
    assert(A::evaluate(unknownMethod).status==
           A::Status::UnknownAcquisitionMethods);
    auto noCandidates=in;noCandidates.count=0;
    assert(A::evaluate(noCandidates).status==A::Status::NoCandidates);
    auto capacity=in;capacity.count=9;
    assert(A::evaluate(capacity).status==A::Status::CapacityExceededUnknown);
    auto missingId=in;missingId.alternatives[1].candidate.id=0;
    assert(A::evaluate(missingId).status==A::Status::OneConditionalRoute);
    // The candidate set only handles 1..8 explicit candidates. An
    // unenumerated ninth alternative can never be silently discarded.
    auto ungrounded=in;
    ungrounded.alternatives[1].originalEncounterIndependentlyGrounded=false;
    assert(A::evaluate(ungrounded).status==A::Status::OneConditionalRoute);
}
int main(){runCases();std::cout<<"Eight-candidate exhaustive history boundaries: PASS\n";}
