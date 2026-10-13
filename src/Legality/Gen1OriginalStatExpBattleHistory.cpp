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
// Original ItemUseVitamin in pret/pokered item_effects.asm adds 10 to
// the HIGH byte only (10*256), keeping the low byte untouched. The
// original game refuses the item when high byte >= 100; successful
// item-consumption evidence at/above that threshold is contradictory.
constexpr bool applyVitamin(std::uint16_t& value) noexcept {
    if((value >> 8u)>=100u)return false;
    value=static_cast<std::uint16_t>(value+2560u);
    return true;
}
static_assert([]{std::uint16_t x=0x63ffu;return applyVitamin(x)&&x==0x6dffu;}());
static_assert([]{std::uint16_t x=0x6400u;return !applyVitamin(x)&&x==0x6400u;}());
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
           !e.originalOrderingIndependentlyAttested ||
           !e.originalAfterStatExpIndependentlyAttested ||
           !knownGame(e.originalGame) || e.originalGame!=h.sourceGame ||
           e.originalMonotonicEventSequence<=previousSequence) {
            out.status=Status::UnknownOriginalTimeline;return out;
        }
        for(unsigned j=0;j<index;++j)
            if(h.events[j].originalEventId==e.originalEventId){
                out.status=Status::UnknownOriginalTimeline;return out;
            }
        previousSequence=e.originalMonotonicEventSequence;
        if(e.kind==EventKind::Battle){
            if(!e.originalDefeatIndependentlyAttested ||
               !e.sourceSpeciesIdentityIndependentlyAttested ||
               !e.expAllAbsentIndependentlyAttested ||
               e.defeatedNationalSpecies<1 ||
               e.defeatedNationalSpecies>151){
                out.status=Status::UnknownOriginalTimeline;return out;
            }
            // DivideExpDataByNumMonsGainingExp in pret/pokered truncates
            // each of the five base-stat bytes BEFORE it awards Stat Exp.
            // An unsourced claimed divisor cannot prove impossibility.
            unsigned recipients=0;
            if(e.originalRecipientCountIndependentlyAttested){
                recipients=e.originalEligibleRecipientCount;
                if(recipients<1 || recipients>6 ||
                   (e.singleEligibleRecipientIndependentlyAttested &&
                    recipients!=1)){
                    out.status=Status::UnknownOriginalTimeline;return out;
                }
            } else if(e.singleEligibleRecipientIndependentlyAttested){
                recipients=1;
            } else {
                out.status=Status::UnknownOriginalTimeline;return out;
            }
            const auto& base=kPersonal[e.defeatedNationalSpecies];
            for(unsigned stat=0;stat<5;++stat)
                out.replayed[stat]=satAdd(out.replayed[stat],
                    static_cast<std::uint8_t>(base[stat]/recipients));
        } else if(e.kind>=EventKind::HPUp && e.kind<=EventKind::Calcium){
            if(!e.originalVitaminItemIndependentlyAttested ||
               !e.originalVitaminConsumedIndependentlyAttested){
                out.status=Status::UnknownOriginalTimeline;return out;
            }
            const unsigned stat=static_cast<unsigned>(e.kind)-
                                static_cast<unsigned>(EventKind::HPUp);
            if(!applyVitamin(out.replayed[stat])){
                out.status=Status::ContradictedAttestedEvent;
                out.firstContradictionEvent=
                    static_cast<std::uint16_t>(index+1u);
                return out;
            }
        } else {
            // Never treat malformed event-kind bits as a valid battle.
            out.status=Status::UnknownOriginalTimeline;return out;
        }
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
