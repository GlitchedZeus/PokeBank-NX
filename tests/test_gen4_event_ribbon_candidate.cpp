#include "Legality/Gen4EventRibbonCandidate.h"
#include <array>
#include <cassert>
#include <iostream>
namespace E=Legality::Gen4EventRibbonCandidate;
constexpr auto source=Legality::Gen4EventTemplate::Candidate{
    4,7200,58364,0x0f199b50u,40,3053,16,0,0,1,7,0,true
};
constexpr E::Requirement evidence{true,{1u,2u,4u},{8u,16u,32u}};
static_assert(E::evaluate(source,67,true,{1u,2u,4u},evidence)
              .status==E::Status::CandidateCompatible);
static_assert(E::evaluate(source,67,true,{0u,2u,4u},evidence)
              .status==E::Status::CandidateContradicted);
static_assert(E::evaluate(source,67,true,{1u,18u,4u},evidence)
              .status==E::Status::CandidateContradicted);
static_assert(E::evaluate(source,0,true,{1u,2u,4u},evidence)
              .status==E::Status::IdentityMismatch);
static_assert(E::evaluate(source,67,false,{1u,2u,4u},evidence)
              .status==E::Status::UnavailableRibbonData);
static_assert(E::evaluate(source,67,true,{1u,2u,4u},{})
              .status==E::Status::UnknownEventRibbonTemplate);
constexpr auto conflicting=[]{
    auto r=evidence;r.mustNotHave[0]=1u;return E::evaluate(
        source,67,true,{1u,2u,4u},r);
}();
static_assert(conflicting.status==E::Status::ConflictingRequirement);
constexpr auto evolved=[] {
    auto c=source;c.species=5;return E::evaluate(
        c,67,true,{1u,2u,4u},evidence);
}();
static_assert(evolved.status==E::Status::CandidateCompatible);
int main(){
    assert(E::evaluate(source,67,true,{1u,2u,4u},evidence)
           .status==E::Status::CandidateCompatible);
    std::cout<<"Native Gen IV event raw-ribbon candidate evidence: PASS\n";
}
