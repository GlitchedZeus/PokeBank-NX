#include "Legality/Gen34SameParentEggHistory.h"
#include <array>
#include <cassert>
#include <iostream>
namespace H=Legality::Gen34SameParentEggHistory;
constexpr H::Input pair{
    "firered_gba",1,1001,1002,true,
    {33,45,0,0},{80,345,15,33},{33,45,0,0},
    {false,false,true,false},{false,false,false,true},
    true,false,{80,345,15,33}
};
constexpr auto sameFather=H::evaluate(pair);
static_assert(sameFather.status==H::Status::CandidateCompatible);
static_assert(sameFather.eggSources==2 && sameFather.machineSources==1);
static_assert(sameFather.sharedSources==1);
static_assert(sameFather.generated==std::array<uint16_t,4>{80,345,15,33});
constexpr auto different=[] {
    auto p=pair;
    p.father={80,15,33,0};
    p.verifiedMachine={false,true,false,false};
    p.verifiedSharedLevel={false,false,true,false};
    return H::evaluate(p);
}();
static_assert(different.status==H::Status::CandidateMismatch);
static_assert(different.eggSources==1);
constexpr auto unknown=[] {
    auto p=pair;p.fatherId=0;return H::evaluate(p);
}();
static_assert(unknown.status==H::Status::UnknownParents);
constexpr auto noMotherMove=[] {
    auto p=pair;p.mother={45,0,0,0};return H::evaluate(p);
}();
static_assert(noMotherMove.sharedSources==0);
static_assert(noMotherMove.status==H::Status::CandidateMismatch);
constexpr auto ruby=[] {
    auto p=pair;p.game="ruby_gba";return H::evaluate(p);
}();
static_assert(!Legality::Gen34EggMove::isEggMove("ruby_gba",1,345));
static_assert(ruby.status==H::Status::CandidateMismatch);
constexpr auto volt=[] {
    H::Input p{};p.game="emerald_gba";p.species=172;
    p.fatherId=1;p.motherId=2;p.snapshotsKnown=true;
    p.base={84,204,0,0};p.lightBallKnown=true;p.lightBall=true;
    p.observed={84,204,344,0};return H::evaluate(p);
}();
static_assert(volt.status==H::Status::CandidateCompatible);
constexpr auto unknownLight=[] {
    H::Input p{};p.game="emerald_gba";p.species=172;
    p.fatherId=1;p.motherId=2;p.snapshotsKnown=true;
    p.base={84,204,0,0};p.observed={84,204,0,0};
    return H::evaluate(p);
}();
static_assert(unknownLight.status==H::Status::UnknownLightBall);
int main() {
    assert(sameFather.status==H::Status::CandidateCompatible);
    assert(different.status==H::Status::CandidateMismatch);
    std::cout<<"Gen III/IV same-parent move witness: PASS\n";
}
