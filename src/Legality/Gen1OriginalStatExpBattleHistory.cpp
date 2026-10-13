#include "Legality/Gen1OriginalStatExpBattleHistory.h"
#include <array>
#include <cstdint>
namespace Legality::Gen1OriginalStatExpBattleHistory {
namespace {
// The EXACT existing Gen I base-stat source; do not derive from
// modern game data or duplicate a second private table.
#include "../Integration/Gen1/Gen1PersonalData.inc"
constexpr bool knownGame(G::SourceGame game) noexcept {
    return game==G::SourceGame::Red || game==G::SourceGame::Blue ||
           game==G::SourceGame::Yellow;
}
constexpr std::uint16_t satAdd(std::uint16_t old,std::uint8_t gain) noexcept {
    const auto sum=static_cast<std::uint32_t>(old)+gain;
    return static_cast<std::uint16_t>(sum>65535u?65535u:sum);
}
static_assert(satAdd(65520,35)==65535);
static_assert(satAdd(65534,0)==65534);
static_assert(satAdd(0,100)==100);
}
Result evaluate(const G::PokemonRecord& p,const History& h) noexcept {
    Result out{};
    if(!h.strictPK1IndependentlyVerified)return out;
    const auto n=p.partyRecord?44u:33u;
    if(p.rawBodySize!=n || p.species<1 || p.species>151 ||
       !p.rawSpecies || p.rawBody[0]!=p.rawSpecies ||
       G::gen1InternalToNational(p.rawSpecies)!=p.species) {
        out.status=Status::MalformedAttestedRecord;return out;
    }
    for(unsigned i=0;i<5;++i){
        const auto o=0x11u+2u*i;
        const auto raw=static_cast<std::uint16_t>(
            (static_cast<unsigned>(p.rawBody[o])<<8u)|
             p.rawBody[o+1u]);
        if(p.statExperience[i]!=raw){
            out.status=Status::MalformedAttestedRecord;return out;
        }
    }
    if(!h.originalInitialSnapshotIndependentlyAttested ||
       !knownGame(h.sourceGame) || h.count>h.events.size()){
        out.status=Status::UnknownOriginalTimeline;return out;
    }
    out.replayed=h.originalBeforeStatExp;
    std::uint64_t previousSequence=0;
    for(unsigned index=0;index<h.count;++index){
        const auto& e=h.events[index];
        if(!e.originalEventId || !e.originalMonotonicEventSequence ||
           !e.originalDefeatIndependentlyAttested ||
           !e.originalOrderingIndependentlyAttested ||
           !e.sourceSpeciesIdentityIndependentlyAttested ||
           !e.originalAfterStatExpIndependentlyAttested ||
           !e.singleEligibleRecipientIndependentlyAttested ||
           !e.expAllAbsentIndependentlyAttested ||
           !knownGame(e.originalGame) || e.originalGame!=h.sourceGame ||
           e.defeatedNationalSpecies<1 || e.defeatedNationalSpecies>151 ||
           e.originalMonotonicEventSequence<=previousSequence) {
            out.status=Status::UnknownOriginalTimeline;return out;
        }
        for(unsigned j=0;j<index;++j)
            if(h.events[j].originalEventId==e.originalEventId){
                out.status=Status::UnknownOriginalTimeline;return out;
            }
        previousSequence=e.originalMonotonicEventSequence;
        const auto& base=kPersonal[e.defeatedNationalSpecies];
        for(unsigned stat=0;stat<5;++stat)
            out.replayed[stat]=satAdd(out.replayed[stat],base[stat]);
        ++out.eventsReplayed;
        if(out.replayed!=e.originalAfterStatExp){
            out.status=Status::ContradictedAttestedEvent;
            out.firstContradictionEvent=static_cast<std::uint16_t>(index+1u);
            return out;
        }
    }
    if(!h.allOriginalStatExpEventsIndependentlyEnumerated){
        out.status=Status::IncompleteEventCoverage;return out;
    }
    out.status=out.replayed==p.statExperience?
        Status::ConditionalOriginalBattleHistory:
        Status::ContradictedAttestedEvent;
    if(out.status==Status::ContradictedAttestedEvent)
        out.firstContradictionEvent=static_cast<std::uint16_t>(h.count+1u);
    return out;
}
} // namespace Legality::Gen1OriginalStatExpBattleHistory
