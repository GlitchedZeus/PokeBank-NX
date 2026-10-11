#include "Legality/Gen34SameParentEggHistory.h"
#include <array>
#include <cassert>
#include <iostream>
namespace H=Legality::Gen34SameParentEggHistory;
void runCases() {
const H::Input pair{
    "firered_gba",1,1001,1002,true,
    {33,45,0,0},{80,345,15,33},{33,45,0,0},
    {false,false,true,false},{false,false,false,true},
    true,false,{80,345,15,33},
    1,1,H::PairProof::IndependentlyCompatible
};
const auto sameFather=H::evaluate(pair);
assert(sameFather.status==H::Status::CandidateCompatible);
assert(sameFather.eggSources==2 && sameFather.machineSources==1);
assert(sameFather.sharedSources==1);
assert((sameFather.generated==std::array<uint16_t,4>{80,345,15,33}));
const auto different=[&] {
    auto p=pair;
    p.father={80,15,33,0};
    p.verifiedMachine={false,true,false,false};
    p.verifiedSharedLevel={false,false,true,false};
    return H::evaluate(p);
}();
assert(different.status==H::Status::CandidateMismatch);
assert(different.eggSources==1);
const auto unknown=[&] {
    auto p=pair;p.fatherId=0;return H::evaluate(p);
}();
assert(unknown.status==H::Status::UnknownParents);
const auto noMotherMove=[&] {
    auto p=pair;p.mother={45,0,0,0};return H::evaluate(p);
}();
assert(noMotherMove.sharedSources==0);
assert(noMotherMove.status==H::Status::CandidateMismatch);
const auto ruby=[&] {
    auto p=pair;p.game="ruby_gba";return H::evaluate(p);
}();
// Ruby also permits #345 in the pinned dataset: do not invent a
// supposed Ruby/FRLG difference for this shared Bulbasaur move.
assert(Legality::Gen34EggMove::isEggMove("ruby_gba",1,345));
assert(ruby.status==H::Status::CandidateCompatible);
const auto rubyWithGen4OnlyMove=[&] {
    auto p=pair;p.game="ruby_gba";
    // Leaf Storm #437 is a Gen IV addition, not a Ruby egg move.
    p.father={80,437,15,33};
    return H::evaluate(p);
}();
assert(!Legality::Gen34EggMove::isEggMove("ruby_gba",1,437));
assert(rubyWithGen4OnlyMove.status==H::Status::CandidateMismatch);
const auto volt=[&] {
    H::Input p{};p.game="emerald_gba";p.species=172;
    p.fatherId=1;p.motherId=2;p.snapshotsKnown=true;
    p.fatherSpecies=25;p.motherSpecies=25;
    p.pairProof=H::PairProof::IndependentlyCompatible;
    p.base={84,204,0,0};p.lightBallKnown=true;p.lightBall=true;
    p.observed={84,204,344,0};return H::evaluate(p);
}();
assert(volt.status==H::Status::CandidateCompatible);
const auto unknownLight=[&] {
    H::Input p{};p.game="emerald_gba";p.species=172;
    p.fatherId=1;p.motherId=2;p.snapshotsKnown=true;
    p.fatherSpecies=25;p.motherSpecies=25;
    p.pairProof=H::PairProof::IndependentlyCompatible;
    p.base={84,204,0,0};p.observed={84,204,0,0};
    return H::evaluate(p);
}();
assert(unknownLight.status==H::Status::UnknownLightBall);
const auto missingEligibility=[&] {
    auto p=pair;p.pairProof=H::PairProof::Unknown;
    return H::evaluate(p);
}();
assert(missingEligibility.status==H::Status::UnknownPairEligibility);
const auto explicitlyIncompatible=[&] {
    auto p=pair;p.pairProof=H::PairProof::IndependentlyIncompatible;
    return H::evaluate(p);
}();
assert(explicitlyIncompatible.status==H::Status::CandidateMismatch);
const auto missingParentSpecies=[&] {
    auto p=pair;p.fatherSpecies=0;
    return H::evaluate(p);
}();
assert(missingParentSpecies.status==H::Status::UnknownPairEligibility);
const auto impossibleGen3Baby=[&] {
    auto p=pair;p.species=387;return H::evaluate(p);
}();
assert(impossibleGen3Baby.status==H::Status::Unsupported);
const auto impossibleGen3Father=[&] {
    auto p=pair;p.fatherSpecies=493;return H::evaluate(p);
}();
assert(impossibleGen3Father.status==H::Status::CandidateMismatch);
const auto gen4ParentRange=[&] {
    auto p=pair;p.game="diamond_nds";
    p.fatherSpecies=493;p.motherSpecies=493;
    return H::evaluate(p);
}();
assert(gen4ParentRange.status!=H::Status::UnknownPairEligibility);

} // runCases
int main() {
    runCases();
    std::cout<<"Gen III/IV same-parent move witness: PASS\n";
}
