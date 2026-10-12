// L5 Stage 4: pinned PKHeX source-rule differential, NOT a PKHeX binary run.
// Real PokeBank NX Gen12TimeCapsuleEvidence.h is compiled and executed.
// Reference method semantics independently reimplemented from PKHeX at
// 6501f0ab46e8f8ca048539dbaf8cae8cb104e722 (see runner blob manifest).
// Neither source-modeled verdict proves a whole-Pokemon Legal/Invalid result.
#include "Legality/Gen12TimeCapsuleEvidence.h"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#ifdef NDEBUG
#error L5_PARITY_TEST_MUST_ENABLE_ASSERTIONS
#endif
namespace {
namespace NX=Legality::Gen12TimeCapsule;
enum class Grade:uint8_t { NoContradiction, Invalid, Incomplete };
enum class Diff:uint8_t { BothClean, BothInvalid, ReferenceOnlyInvalid,
                         NXOnlyInvalid, HistoryIncomplete, SuspectedOverstatement };
struct EVInput {
    uint8_t gen=3;
    bool egg=false;
    std::array<uint16_t,6> ev{};
    std::array<uint16_t,5> gb{};
    bool originalEncounterWitness=false;
    bool encounteredAtLevel100=false;
    bool expUnchangedSinceEncounter=false;
};
struct Counters {uint64_t comparisons=0,assertions=0, clean=0,invalid=0,
    referenceOnly=0,nxOnly=0,historicalUnknown=0,suspect=0,trade=0,domain=0;};
#define CHECK(COND) do {++c.assertions;if(!(COND)){std::cerr<<"Stage4 fail line "<<__LINE__<<": "<<#COND<<'\n';std::abort();}}while(false)
constexpr uint32_t sum(const EVInput& x) {
    uint32_t total=0;for(auto v:x.ev)total+=v;return total;
}
constexpr bool anyGBStatExp(const EVInput& x){for(auto v:x.gb)if(v)return true;return false;}
constexpr bool anyNonVitaminGB(const EVInput& x){
    for(auto v:x.gb) { if(v>25600 || v%2560!=0)return true; }
    return false;
}
constexpr bool missingGBStat(const EVInput& x){for(auto v:x.gb)if(v==0)return true;return false;}
constexpr bool allVitamins34(const EVInput& x){
    for(auto v:x.ev) { if(v!=0&&(v>100||v%10!=0))return false; }
    return true;
}
// Independently read PKHeX EffortValueVerifier.cs + EffortValues.cs.
// Reference is a *bounded one-method judgement*, NOT full PKHeX legality.
constexpr Grade pkhexEffortModel(const EVInput& x){
    if(x.egg)return (x.gen<=2?anyGBStatExp(x):sum(x)!=0)?Grade::Invalid:Grade::NoContradiction;
    if(x.gen<=2)return anyNonVitaminGB(x)&&missingGBStat(x)?Grade::Invalid:Grade::NoContradiction;
    if(sum(x)>510)return Grade::Invalid;
    if(x.gen==3||x.gen==4){
        if(allVitamins34(x))return Grade::NoContradiction;
        if(!x.originalEncounterWitness)return Grade::Incomplete;
        if(x.encounteredAtLevel100||x.expUnchangedSinceEncounter)return Grade::Invalid;
    }
    return Grade::NoContradiction;
}
// Accepted L5 Legality.cpp source contract, explicitly not an executed analyzer.
// Gen I/II wrapper modern EVs are neutral; exact original statExp isn't read.
constexpr Grade nxAcceptedSourceModel(const EVInput& x){
    if(x.gen<=2)return Grade::NoContradiction;
    for(auto v:x.ev)if(v>252)return Grade::Invalid;
    if(sum(x)>510)return Grade::Invalid;
    if(x.egg&&sum(x)!=0)return Grade::Invalid;
    return Grade::NoContradiction;
}
constexpr Diff classify(Grade ref,Grade nx){
    if(ref==Grade::Incomplete)
        return nx==Grade::Invalid?Diff::SuspectedOverstatement:Diff::HistoryIncomplete;
    if(ref==Grade::Invalid&&nx==Grade::Invalid)return Diff::BothInvalid;
    if(ref==Grade::NoContradiction&&nx==Grade::NoContradiction)return Diff::BothClean;
    if(ref==Grade::Invalid)return Diff::ReferenceOnlyInvalid;
    return Diff::NXOnlyInvalid;
}
void compare(const EVInput& x,Counters& c){
    const auto ref=pkhexEffortModel(x),nx=nxAcceptedSourceModel(x);
    const auto diff=classify(ref,nx);
    CHECK(ref!=Grade::Incomplete || diff==Diff::HistoryIncomplete||diff==Diff::SuspectedOverstatement);
    CHECK((ref==Grade::Invalid&&nx==Grade::Invalid)==(diff==Diff::BothInvalid));
    ++c.comparisons;
    switch(diff){
        case Diff::BothClean:++c.clean;break;
        case Diff::BothInvalid:++c.invalid;break;
        case Diff::ReferenceOnlyInvalid:++c.referenceOnly;break;
        case Diff::NXOnlyInvalid:++c.nxOnly;break;
        case Diff::HistoryIncomplete:++c.historicalUnknown;break;
        case Diff::SuspectedOverstatement:++c.suspect;break;
    }
}
void checksEV(Counters& c){
    // All possible 16-bit Gen I and Gen II stored stat-exp values, five
    // distinct positions, both egg states for Gen II and source-free history.
    for(unsigned gen=1;gen<=2;++gen){
        EVInput x{};x.gen=static_cast<uint8_t>(gen);
        for(uint32_t v=0;v<=65535;++v){
            for(unsigned place=0;place<5;++place){
                x.gb={};x.gb[place]=static_cast<uint16_t>(v);
                x.egg=false;compare(x,c);
                if(gen==2){x.egg=true;compare(x,c);}
            }
        }
        // Nonzero vitamin-only values versus incomplete enemy-gain stats.
        for(uint16_t v:{uint16_t{0},uint16_t{2560},uint16_t{25600},uint16_t{25601},uint16_t{65535}}){
            x.egg=false;x.gb.fill(v);compare(x,c);
            CHECK(pkhexEffortModel(x)==Grade::NoContradiction);
        }
    }
    // Exhaustive 256x256 native two-stat EV values for both PK3 and PK4,
    // paired with source-free, attested level 100, EXP unchanged, and
    // an ordinary possible history. Original encounter evidence for these
    // synthetic records is hypothetical, not an authenticated retail receipt.
    for(unsigned gen=3;gen<=4;++gen){
        EVInput x{};x.gen=static_cast<uint8_t>(gen);
        for(unsigned hp=0;hp<256;++hp)for(unsigned atk=0;atk<256;++atk){
            x.ev={static_cast<uint16_t>(hp),static_cast<uint16_t>(atk),0,0,0,0};
            x.egg=false;
            x.originalEncounterWitness=false;compare(x,c);
            x.originalEncounterWitness=true;
            x.encounteredAtLevel100=true;compare(x,c);
            x.encounteredAtLevel100=false;
            x.expUnchangedSinceEncounter=true;compare(x,c);
            x.expUnchangedSinceEncounter=false;compare(x,c);
            x.egg=true;compare(x,c);
        }
    }
    // Explicit counterexamples to the unqualified "all EV over 252 is
    // impossible for Gen III/IV" assumption; source-time route remains unknown.
    for(unsigned gen:{3u,4u})for(unsigned high:{253u,254u,255u}){
        EVInput x{};x.gen=static_cast<uint8_t>(gen);
        x.ev={static_cast<uint16_t>(high),0,0,0,0,0};
        CHECK(classify(pkhexEffortModel(x),nxAcceptedSourceModel(x))==Diff::SuspectedOverstatement);
        x.originalEncounterWitness=true;x.encounteredAtLevel100=true;
        CHECK(classify(pkhexEffortModel(x),nxAcceptedSourceModel(x))==Diff::BothInvalid);
    }
}
constexpr std::array<uint16_t,22> evolved={169,182,186,196,197,199,208,212,230,233,242,
    462,463,464,465,466,467,470,471,474,700,900};
constexpr bool pkhexGBCanVisit(uint16_t species){
    // In PKHeX GBRestrictions, 0 is accepted by species <=151 expression.
    // PK0 is an empty slot, NOT a valid encounter: evaluate only species>=1.
    if(species<=151)return true;
    for(auto e:evolved)if(e==species)return true;
    return false;
}
void checksHistorical(Counters& c){
    for(uint32_t id=0;id<=65535;++id){
        const auto species=static_cast<uint16_t>(id);
        const bool reference=pkhexGBCanVisit(species);
        const bool nx=NX::canVisitGen1Species(species);
        if(species==0){CHECK(reference&&!nx);++c.domain;continue;}
        // PKHeX includes Gen III+ descendants for modern transfer chains.
        // Native PK2 cannot contain species above 251; preserve that scope.
        if(species>251){CHECK(!nx);++c.domain;}
        else CHECK(reference==nx);
        for(bool egg:{false,true})for(uint16_t caught:{uint16_t{0},uint16_t{1},uint16_t{65535}}){
            const bool eligible=species<=251&&reference&&!egg&&caught==0;
            CHECK(NX::couldOriginateGen1(species,egg,caught)==eligible);
            // PKHeX also excludes Korean saves. NX helper does not have
            // a Korean input parameter; do not claim parity for Korean cases.
            const bool koreanUnmodeled=eligible;
            if(koreanUnmodeled)++c.historicalUnknown;
            ++c.trade;
        }
        for(const auto move:{uint16_t{0},uint16_t{165},uint16_t{166},uint16_t{251},uint16_t{252}}){
            const std::array<uint16_t,4> moves{33,move,0,0};
            const auto state=NX::classifyCurrentTrade(species,false,moves);
            CHECK((state==NX::CurrentTrade::Compatible)==(species<=151&&move<=165));
            if(move>251 && species<=151)
                CHECK(state==NX::CurrentTrade::InvalidGen2MoveId);
            if(species>151)CHECK(state==NX::CurrentTrade::SpeciesNotInGen1);
            // A Gen II evolution of Gen I ancestor is historically possible,
            // even though current back-trade with the evolved species is not.
            if(species>151&&species<=251&&pkhexGBCanVisit(species))
                CHECK(NX::canVisitGen1Species(species));
            ++c.trade;
        }
    }
}
void negativeControls(Counters& c){
    CHECK(classify(Grade::Invalid,Grade::NoContradiction)==Diff::ReferenceOnlyInvalid);
    CHECK(classify(Grade::NoContradiction,Grade::Invalid)==Diff::NXOnlyInvalid);
    CHECK(classify(Grade::Incomplete,Grade::NoContradiction)==Diff::HistoryIncomplete);
    CHECK(classify(Grade::Incomplete,Grade::Invalid)==Diff::SuspectedOverstatement);
    CHECK(classify(Grade::Invalid,Grade::Invalid)==Diff::BothInvalid);
    CHECK(classify(Grade::NoContradiction,Grade::NoContradiction)==Diff::BothClean);
    CHECK(c.comparisons==1638410u);
    CHECK(c.trade==720885u);
    CHECK(c.domain==65285u);
    CHECK(c.historicalUnknown>0&&c.suspect>0&&c.referenceOnly>0&&c.nxOnly>0);
    CHECK(c.clean+c.invalid+c.referenceOnly+c.nxOnly+c.historicalUnknown+c.suspect>=c.comparisons);
}
}
int main(){Counters c{};checksEV(c);checksHistorical(c);negativeControls(c);
    std::cout<<"L5 Stage4 source-modeled pinned EV and true NX Time Capsule: comparisons="
             <<c.comparisons<<" clean="<<c.clean<<" both_invalid="<<c.invalid
             <<" reference_only="<<c.referenceOnly<<" nx_only="<<c.nxOnly
             <<" incomplete="<<c.historicalUnknown<<" suspected="<<c.suspect
             <<" history_checks="<<c.trade<<" domain="<<c.domain
             <<" assertions="<<c.assertions
             <<" runtime_pkhex=0 runtime_analyzer=0\n";
}
