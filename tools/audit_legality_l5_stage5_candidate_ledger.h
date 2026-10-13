#pragma once
// L5 QA-only: cross-lane evidence composition. No production legality verdict.
// A candidate contradiction is not equivalent to global Severity::Invalid.
#include <array>
#include <cstdint>
namespace Legality::Audit::L5CandidateLedger {
enum class Claim : std::uint8_t { Absent, Compatible, Contradicted, Unsupported };
enum Lane : std::uint8_t { Encounter=1, Breeding=2, Moves=4, Event=8 };
enum class Status : std::uint8_t {
    Unresolved, AuthenticatedCandidateConsistent, AllEnumeratedCandidateRoutesContradicted
};
struct Packet {
    std::uint64_t pokemon=0, originalRoute=0, originalSource=0;
    bool originalReceiptAttested=false;
    bool samePokemonHistoryAttested=false;
    bool completeStageHistory=false;
    std::uint8_t required=0;
    std::array<Claim,4> lane{};
};
struct Input {
    std::uint64_t pokemon=0;
    std::array<Packet,16> packets{};
    std::uint8_t count=0;
    bool allOriginalRoutesAttested=false;
    bool allAlternativeAcquisitionRoutesExcluded=false;
    bool boundedEnumerationProvenComplete=false;
};
struct Result {
    Status status=Status::Unresolved;
    std::uint8_t compatible=0,contradicted=0,unresolved=0,duplicateConflicts=0;
    std::uint8_t authenticatedDistinctRoutes=0;
    bool globalInvalid=false; // Informational QA gate can never emit Invalid.
};
constexpr bool equivalent(const Packet& a,const Packet& b) noexcept {
    return a.pokemon==b.pokemon && a.originalRoute==b.originalRoute &&
           a.originalSource==b.originalSource &&
           a.samePokemonHistoryAttested==b.samePokemonHistoryAttested &&
           a.completeStageHistory==b.completeStageHistory &&
           a.required==b.required && a.lane==b.lane;
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(!in.pokemon||!in.count||in.count>in.packets.size()) return out;
    // Authenticated identity is an independent original route, not a
    // post-hoc combination of unrelated evidence from four separate lanes.
    for(std::uint8_t i=0;i<in.count;++i){
        const Packet& p=in.packets[i];
        if(!p.originalReceiptAttested)continue;
        bool prior=false;
        for(std::uint8_t j=0;j<i;++j){
            const auto& q=in.packets[j];
            if(q.originalReceiptAttested && p.originalRoute==q.originalRoute &&
               p.pokemon==q.pokemon && p.originalSource==q.originalSource)
                prior=true;
        }
        if(prior)continue;
        ++out.authenticatedDistinctRoutes;
        bool conflict=false;
        for(std::uint8_t j=i+1;j<in.count;++j){
            const auto& q=in.packets[j];
            if(q.originalReceiptAttested && p.originalRoute==q.originalRoute &&
               p.pokemon==q.pokemon && p.originalSource==q.originalSource &&
               !equivalent(p,q))conflict=true;
        }
        if(conflict){++out.duplicateConflicts;++out.unresolved;continue;}
        if(!p.originalRoute||!p.originalSource||p.pokemon!=in.pokemon||
           !p.samePokemonHistoryAttested||!p.completeStageHistory||
           !p.required||(p.required&~15u)){
            ++out.unresolved;continue;
        }
        bool unknown=false,contradiction=false;
        for(unsigned lane=0;lane<4;++lane){
            const auto claim=p.lane[lane];
            const bool required=(p.required & (1u<<lane))!=0;
            if(required){
                if(claim==Claim::Absent || claim==Claim::Unsupported)unknown=true;
                if(claim==Claim::Contradicted)contradiction=true;
            }else if(claim!=Claim::Absent)unknown=true;
        }
        if(unknown)++out.unresolved;
        else if(contradiction)++out.contradicted;
        else ++out.compatible;
    }
    // A second unattested distinct route is evidence that alternatives are
    // unclosed, even if one authenticated route is locally contradicted.
    for(std::uint8_t i=0;i<in.count;++i){
        const Packet& p=in.packets[i];
        if(p.originalReceiptAttested)continue;
        bool represented=false;
        for(std::uint8_t j=0;j<in.count;++j){
            const auto& q=in.packets[j];
            if(q.originalReceiptAttested && p.originalRoute==q.originalRoute &&
               p.originalSource==q.originalSource && p.pokemon==q.pokemon)
               represented=true;
        }
        if(!represented)++out.unresolved;
    }
    if(out.compatible)
        out.status=Status::AuthenticatedCandidateConsistent;
    else if(out.contradicted && !out.unresolved &&
            in.allOriginalRoutesAttested &&
            in.allAlternativeAcquisitionRoutesExcluded &&
            in.boundedEnumerationProvenComplete)
        out.status=Status::AllEnumeratedCandidateRoutesContradicted;
    return out;
}
static_assert([]{
    Input i{};i.pokemon=7;i.count=1;
    auto& p=i.packets[0];p.pokemon=7;p.originalRoute=8;p.originalSource=9;
    p.originalReceiptAttested=p.samePokemonHistoryAttested=p.completeStageHistory=true;
    p.required=Encounter|Moves;
    p.lane={Claim::Compatible,Claim::Absent,Claim::Compatible,Claim::Absent};
    return evaluate(i).status==Status::AuthenticatedCandidateConsistent;
}());
} // namespace Legality::Audit::L5CandidateLedger
