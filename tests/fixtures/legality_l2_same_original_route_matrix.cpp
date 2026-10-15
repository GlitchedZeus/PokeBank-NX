#include "Legality/Gen34OriginalSameRouteProof.h"
#include <cassert>
#include <cstdint>
#include <iostream>
namespace R=Legality::Gen34OriginalSameRouteProof;
int main(){
    uint64_t checks=0;
    for(unsigned game=1;game<=10;++game)for(uint64_t seed=1;seed<=1024;++seed){
        R::Expected e{static_cast<R::Game>(game),seed,seed+10000,
            seed+20000,seed+30000,seed+40000,true,
            static_cast<uint16_t>(seed&1?229:294),
            static_cast<uint16_t>(seed&2?236:0)};
        R::Input in{};in.supplied=true;
        for(auto& stage:in.stage)
            stage={true,true,true,e.game,e.eventId,e.eggSnapshotId,
                e.pairId,e.slot0Id,e.slot1Id,e.held0,e.held1};
        assert(R::evaluate(in,e)==R::Status::ConditionallySameRoute);++checks;
        for(unsigned slot=0;slot<4;++slot){
            auto x=in;x.stage[slot].eventId++;
            assert(R::evaluate(x,e)==R::Status::CrossedOriginalStages);++checks;
            x=in;x.stage[slot].eggSnapshotId++;
            assert(R::evaluate(x,e)==R::Status::CrossedOriginalStages);++checks;
            x=in;x.stage[slot].pairId++;
            assert(R::evaluate(x,e)==R::Status::CrossedOriginalStages);++checks;
            x=in;x.stage[slot].slot0Id++;
            assert(R::evaluate(x,e)==R::Status::CrossedOriginalStages);++checks;
            x=in;x.stage[slot].slot1Id++;
            assert(R::evaluate(x,e)==R::Status::CrossedOriginalStages);++checks;
            x=in;x.stage[slot].held0++;
            assert(R::evaluate(x,e)==R::Status::CrossedOriginalStages);++checks;
            x=in;x.stage[slot].held1++;
            assert(R::evaluate(x,e)==R::Status::CrossedOriginalStages);++checks;
            x=in;x.stage[slot].game=R::Game::Unknown;
            assert(R::evaluate(x,e)==R::Status::UnknownSourceEvidence);++checks;
            x=in;x.stage[slot].independentlyAttested=false;
            assert(R::evaluate(x,e)==R::Status::UnknownSourceEvidence);++checks;
            x=in;x.stage[slot].heldItemsAttested=false;
            assert(R::evaluate(x,e)==R::Status::UnknownSourceEvidence);++checks;
        }
    }
    assert(checks==419840);
    assert(R::gameFor("ruby_gba")==R::Game::Ruby);
    assert(R::gameFor("soulsilver_nds")==R::Game::SoulSilver);
    assert(R::gameFor("white_nds")==R::Game::Unknown);
    std::cout<<"L2 Stage5 original source crossing: "<<checks<<" PASS\n";
}
