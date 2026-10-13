#pragma once
// L5 QA-only release evidence lattice. No production legality analyzer linkage.
// A candidate contradiction is never an independently proven global Invalid.
#include <array>
#include <cstdint>
namespace Legality::Audit::L5FinalGate {
enum class Clearance : std::uint8_t { NotApplicable, Research, NativeMethodObserved,
    ReferenceMethodObserved, CompleteRetailParity };
enum class Scope : std::uint8_t { Unknown, CandidateOnly, StoredInvariantNeedsRuntime,
    ExhaustivelyContradictedOriginal };
enum class Domain : std::uint8_t { NativeStored, Encounter, Breeding, Moves,
    Event, Transfer, SourceUnresolved };
struct Cell {
    bool nativeApplicable=true, nativeAnalyzed=false, originalSaveAuthenticated=false;
    bool fullOriginalHistoryAuthenticated=false, executablePKHeXCompared=false;
    bool sameNativeFormat=true, sameGameAndMethod=true;
    bool fullCandidateUniverseEnumerated=false, allAlternateSourcesExcluded=false;
    bool allRuntimeComparisonsAgree=false;
};
struct Site {
    Domain domain=Domain::SourceUnresolved;
    bool exactNativeFormatConfirmed=false, directStoredInvariantProven=false;
    bool originalSourceAuthenticated=false, allOriginalRoutesAuthenticated=false;
    bool allAlternateAcquisitionsExcluded=false;
    bool candidateContradiction=false, consistentAlternativeFound=false;
    bool unexaminedRouteExists=true;
};
constexpr Clearance grade(const Cell& c) noexcept {
    if(!c.nativeApplicable)return Clearance::NotApplicable;
    if(!c.nativeAnalyzed)return Clearance::Research;
    if(!c.executablePKHeXCompared)return Clearance::NativeMethodObserved;
    if(!c.originalSaveAuthenticated||!c.fullOriginalHistoryAuthenticated||
       !c.sameNativeFormat||!c.sameGameAndMethod||!c.fullCandidateUniverseEnumerated||
       !c.allAlternateSourcesExcluded||!c.allRuntimeComparisonsAgree)
        return Clearance::ReferenceMethodObserved;
    return Clearance::CompleteRetailParity;
}
constexpr Scope scope(const Site& s) noexcept {
    if(!s.exactNativeFormatConfirmed || !s.candidateContradiction)
        return Scope::Unknown;
    if(s.domain==Domain::NativeStored && s.directStoredInvariantProven)
        return Scope::StoredInvariantNeedsRuntime;
    if(s.consistentAlternativeFound || s.unexaminedRouteExists ||
       !s.originalSourceAuthenticated || !s.allOriginalRoutesAuthenticated ||
       !s.allAlternateAcquisitionsExcluded)
        return Scope::CandidateOnly;
    return Scope::ExhaustivelyContradictedOriginal;
}
// Even exhaustive source-specific contradictions do not establish that the
// full production analyzer has emitted a justified global Invalid verdict.
struct Dashboard {
    std::array<Cell,336> cells{};
    std::array<Site,38> emitted{};
    std::uint16_t nativeApplicable=0,notApplicable=0;
    std::uint16_t fullyCertified=0,methodObserved=0;
    std::uint16_t sitesNeedingRuntime=0,sitesCandidateOnly=0,sitesExhausted=0;
    bool anyOriginalPairedRetailHistory=false;
    bool runtimeFullAnalyzerExecuted=false;
};
struct Summary {
    std::uint16_t nativeApplicable=0,notApplicable=0,fullyCertified=0;
    std::uint16_t methodObserved=0,sitesNeedingRuntime=0;
    std::uint16_t sitesCandidateOnly=0,sitesExhausted=0;
    bool releaseApproved=false;
};
constexpr Summary summarize(const Dashboard& d) noexcept {
    Summary r{};
    for(const Cell& c:d.cells){
        if(c.nativeApplicable)++r.nativeApplicable; else ++r.notApplicable;
        const auto x=grade(c);
        if(x==Clearance::CompleteRetailParity)++r.fullyCertified;
        if(x==Clearance::NativeMethodObserved || x==Clearance::ReferenceMethodObserved)
            ++r.methodObserved;
    }
    for(const Site& site:d.emitted){
        const auto s=scope(site);
        if(s==Scope::StoredInvariantNeedsRuntime)++r.sitesNeedingRuntime;
        if(s==Scope::CandidateOnly)++r.sitesCandidateOnly;
        if(s==Scope::ExhaustivelyContradictedOriginal)++r.sitesExhausted;
    }
    r.releaseApproved=r.notApplicable+r.nativeApplicable==336 &&
        r.fullyCertified==r.nativeApplicable &&
        d.anyOriginalPairedRetailHistory && d.runtimeFullAnalyzerExecuted &&
        r.sitesNeedingRuntime==0 && r.sitesCandidateOnly==0 &&
        r.sitesExhausted==0;
    return r;
}
static_assert(grade(Cell{})==Clearance::Research);
static_assert(scope(Site{})==Scope::Unknown);
} // namespace Legality::Audit::L5FinalGate
