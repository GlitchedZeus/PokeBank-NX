#include "Legality/Gen34OriginalJointRouteLedger.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace L7=Legality::Gen34OriginalJointRouteLedger;
namespace S7=Legality::Gen34OriginalSameRouteProof;
namespace H7=Legality::Gen34OriginalParentLineage;
static L7::Candidate validRoute(unsigned game,std::uint64_t serial) {
    L7::Candidate c{};
    c.routeId=serial+7;
    c.originalIdentity={static_cast<S7::Game>(game),serial+10,
        serial+20,serial+30,serial+40,serial+50,true,229,236};
    c.originalStages.supplied=true;
    for(auto& stage:c.originalStages.stage)
        stage={true,true,true,c.originalIdentity.game,
               c.originalIdentity.eventId,c.originalIdentity.eggSnapshotId,
               c.originalIdentity.pairId,c.originalIdentity.slot0Id,
               c.originalIdentity.slot1Id,229,236};
    c.stageVerdicts.fill(L7::Verdict::Matched);
    c.allMethodsForThisRouteIndependentlyCovered=true;
    c.depositedSpeciesIndependentlyAttested=true;
    c.depositedSpecies={25,132};
    auto& h=c.originalParentLineage;
    h.supplied=true;
    h.originalEventOrderingIndependentlyAttested=true;
    h.originalEggEventSequence=serial+1000;
    for(unsigned j=0;j<2;++j){
        auto& p=h.physicalSlots[j];
        p.supplied=true;
        p.originalOriginIndependentlyAttested=true;
        p.originalDepositIndependentlyAttested=true;
        p.originalOrigin={j?c.originalIdentity.slot1Id:c.originalIdentity.slot0Id,
                          c.originalIdentity.game,c.depositedSpecies[j]};
        p.deposited=p.originalOrigin;
    }
    return c;
}
int main(){
    std::uint64_t assertions=0;
    constexpr std::array<unsigned char,7> corrupt{3,4,7,19,127,128,255};
    for(unsigned game=1;game<=10;++game)
    for(std::uint64_t seed=1;seed<=2048;++seed){
        L7::Input in{};in.count=1;in.allOriginalRoutesIndependentlyEnumerated=true;
        in.routes[0]=validRoute(game,seed);
        assert(L7::evaluate(in).status==L7::Status::OneConditionallyMatched);++assertions;
        for(unsigned j=0;j<4;++j)for(auto bad:corrupt){
            auto x=in;
            x.routes[0].stageVerdicts[j]=static_cast<L7::Verdict>(bad);
            const auto r=L7::evaluate(x);
            assert(r.status==L7::Status::UnresolvedSourceHistory);
            assert(r.matched==0 && r.contradicted==0 && r.unknown==1);
            assertions+=2;
            // False-negative prevention: an invalid verdict combined with a
            // definite contradiction may NEVER count as exhaustive.
            x.routes[0].stageVerdicts[(j+1)%4]=L7::Verdict::Contradicted;
            assert(L7::evaluate(x).status==L7::Status::UnresolvedSourceHistory);
            ++assertions;
        }
        auto cross=in;
        cross.routes[0].originalParentLineage.physicalSlots[0].deposited.identity++;
        assert(L7::evaluate(cross).status==L7::Status::UnverifiedOrDuplicateSources);
        ++assertions;
        cross=in;
        cross.routes[0].originalParentLineage.physicalSlots[1].originalOrigin.species=999;
        assert(L7::evaluate(cross).status==L7::Status::UnverifiedOrDuplicateSources);
        ++assertions;
        cross=in;
        cross.routes[0].originalParentLineage.physicalSlots[0].transitions=5;
        assert(L7::evaluate(cross).status==L7::Status::UnverifiedOrDuplicateSources);
        ++assertions;
        cross=in;
        cross.routes[0].originalStages.stage[2].heldItemsAttested=false;
        assert(L7::evaluate(cross).status==L7::Status::UnverifiedOrDuplicateSources);
        ++assertions;
        cross=in;
        cross.routes[0].stageVerdicts[0]=L7::Verdict::Contradicted;
        assert(L7::evaluate(cross).status==L7::Status::AllAttestedLocalHistoriesContradicted);
        ++assertions;
        cross=in;cross.count=2;
        cross.routes[1]=in.routes[0];cross.routes[1].routeId++;
        cross.routes[0].stageVerdicts[0]=L7::Verdict::Contradicted;
        assert(L7::evaluate(cross).status==L7::Status::OneConditionallyMatched);
        ++assertions;
        cross.routes[1].stageVerdicts[0]=static_cast<L7::Verdict>(255);
        assert(L7::evaluate(cross).status==L7::Status::UnresolvedSourceHistory);
        ++assertions;
        cross.routes[1].routeId=cross.routes[0].routeId;
        assert(L7::evaluate(cross).status==L7::Status::UnverifiedOrDuplicateSources);
        ++assertions;
        cross.routes[1].stageVerdicts.fill(L7::Verdict::Matched);
        assert(L7::evaluate(cross).status==L7::Status::OneConditionallyMatched);
        ++assertions;
        cross=in;cross.routes[0].stageVerdicts[0]=L7::Verdict::Contradicted;
        cross.allOriginalRoutesIndependentlyEnumerated=false;
        assert(L7::evaluate(cross).status==L7::Status::UnknownExhaustiveEnumeration);
        ++assertions;
    }
    const std::uint64_t expected=10ull*2048ull*(1ull+4ull*7ull*3ull+10ull);
    assert(assertions==expected);
    std::cout << "L2 Stage7 malformed and crossed route guards: "
              << assertions << " PASS\n";
}
