#pragma once
// L3 Stage 6: source-scoped Gen IV candidate move history; not a legality verdict.
// Caller supplies an independently pinned exact-game source Catalog for each visit.
// Unknown methods/provenance do not disprove an alternative historic route.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
namespace Legality::Gen4NativeOriginalHistoryCore {
enum class Game : std::uint8_t { Unknown, Diamond, Pearl, Platinum, HeartGold, SoulSilver };
enum class Action : std::uint8_t { LevelLearn, LevelDecline, LevelKnown, TM, HM, Tutor, Relearn, DeleteMove, VisitGame, Other };
enum class Status : std::uint8_t { Unknown, CandidateConsistent, CandidateContradicted };
struct Catalog {
    Game game=Game::Unknown;
    std::uint16_t species=0;
    std::span<const std::uint16_t> packedOffers{};
    bool orderedLevelSourceComplete=false;
    bool machineCompatibilitySourceComplete=false;
    std::uint64_t machineLow=0,machineHigh=0;
    std::span<const std::uint16_t> tutorMoves{};
    bool tutorSourceComplete=false;
    std::uint64_t tutorMask=0; // one bit per entry in tutorMoves
    // Numeric TM/HM to native move mapping: NO unverified cross-title copy.
    std::span<const std::uint16_t> numberedMachineMoves{}; // TM1..92/HM1..8; 0 = Unknown
    bool originalRelearnerSourceComplete=false;
    bool originalNormalFormOnly=true;
};
struct Step {
    Game game=Game::Unknown;
    std::uint8_t level=0;
    Action action=Action::Other;
    std::uint16_t move=0;
    std::uint8_t slot=0;
    std::uint8_t number=0;
    std::uint32_t receipt=0;
    bool originalActionAttested=false;
    bool sourceMethodAttested=false;
    bool samePokemonAcrossVisitAttested=false;
};
struct History {
    std::uint32_t candidate=0;
    std::uint16_t species=0;
    Game origin=Game::Unknown,observedGame=Game::Unknown;
    std::uint8_t initialLevel=0,observedLevel=0;
    std::uint8_t form=0;
    std::array<std::uint16_t,4> initialMoves{},observedMoves{};
    bool originalSnapshotAttested=false;
    bool samePokemonAttested=false;
    bool everyIntermediateActionAttested=false;
    std::array<Step,64> steps{};
    std::uint8_t count=0;
};
struct Result {
    Status status=Status::Unknown;
    std::array<std::uint16_t,4> rebuilt{};
    Game game=Game::Unknown;
    std::uint16_t offers=0,machines=0,tutors=0,reminders=0,deletions=0,visits=0;
    bool globalInvalid=false;
};
constexpr bool supported(Game g) noexcept {return g>=Game::Diamond && g<=Game::SoulSilver;}
constexpr bool validMoves(const std::array<std::uint16_t,4>& a) noexcept {
    if(!a[0])return false;
    bool gap=false;
    for(unsigned i=0;i<4;++i){if(!a[i]){gap=true;continue;}if(a[i]>467||gap)return false;
        for(unsigned j=0;j<i;++j)if(a[i]==a[j])return false;}
    return true;
}
constexpr bool contains(const std::array<std::uint16_t,4>& m,std::uint16_t v) noexcept {
    for(const auto x:m)if(x==v)return true;
    return false;
}
constexpr std::uint8_t firstFree(const std::array<std::uint16_t,4>& m) noexcept {
    for(std::uint8_t i=0;i<4;++i)if(!m[i])return i;
    return 4;
}
constexpr bool isLevel(Action a) noexcept {
    return a==Action::LevelLearn||a==Action::LevelDecline||a==Action::LevelKnown;
}
constexpr bool isTeaching(Action a) noexcept {
    return a==Action::TM||a==Action::HM||a==Action::Tutor||a==Action::Relearn;
}
constexpr std::uint8_t machineBit(Action a,std::uint8_t number) noexcept {
    return a==Action::TM?static_cast<std::uint8_t>(number-1):static_cast<std::uint8_t>(91+number);
}
constexpr bool machineCompatible(const Catalog& c,Action action,std::uint8_t number) noexcept {
    if(!c.machineCompatibilitySourceComplete||!number||
       number>(action==Action::TM?92:8))return false;
    const std::uint8_t bit=machineBit(action,number);
    return bit<64?((c.machineLow>>bit)&1ULL)!=0:((c.machineHigh>>(bit-64))&1ULL)!=0;
}
constexpr bool validSource(const Catalog& c) noexcept {
    if(!supported(c.game)||c.species<1||c.species>493)return false;
    if(c.orderedLevelSourceComplete){
        if(c.packedOffers.empty())return false;
        std::uint8_t last=0;
        for(auto x:c.packedOffers){const auto level=x>>9,move=x&511;
            if(level<1||level>100||move<1||move>467||level<last)return false;
            last=static_cast<std::uint8_t>(level);}
    }
    if(c.machineCompatibilitySourceComplete && (c.machineHigh>>36))return false;
    if(c.numberedMachineMoves.size()!=0 && c.numberedMachineMoves.size()!=100)return false;
    if(c.tutorMoves.size()>64)return false;
    for(const auto move:c.tutorMoves)if(move<1||move>467)return false;
    return true;
}
constexpr std::uint16_t packedOffer(const Catalog& c,std::size_t index) noexcept {
    return index<c.packedOffers.size()?c.packedOffers[index]:0;
}
constexpr std::size_t skipThrough(const Catalog& c,std::uint8_t level) noexcept {
    std::size_t index=0;
    if(c.orderedLevelSourceComplete)
        while(index<c.packedOffers.size() && (packedOffer(c,index)>>9)<=level)++index;
    return index;
}
constexpr bool tutorCompatible(const Catalog& c,std::uint16_t move) noexcept {
    for(std::size_t i=0;i<c.tutorMoves.size();++i)
        if(c.tutorMoves[i]==move && ((c.tutorMask>>i)&1ULL))return true;
    return false;
}
constexpr bool relearnerCompatible(const Catalog& c,std::uint8_t level,
                                    std::uint16_t move) noexcept {
    if(!c.originalRelearnerSourceComplete || !c.orderedLevelSourceComplete)return false;
    for(auto x:c.packedOffers)if((x>>9)<=level && (x&511u)==move)return true;
    return false;
}
// Provider: constexpr Catalog catalog(Game exactTitle, uint16_t normalSpecies).
template<class Source> constexpr Result evaluate(const History& h) noexcept {
    Result out{};
    if(!h.candidate||!supported(h.origin)||!supported(h.observedGame)||
       h.species<1||h.species>493||h.form!=0||!h.initialLevel||
       h.observedLevel<h.initialLevel||h.observedLevel>100||
       h.count>h.steps.size()||!h.originalSnapshotAttested||
       !h.samePokemonAttested||!h.everyIntermediateActionAttested||
       !validMoves(h.initialMoves)||!validMoves(h.observedMoves))return out;
    Catalog current=Source::catalog(h.origin,h.species);
    if(!validSource(current)||current.game!=h.origin||current.species!=h.species)return out;
    // Preflight ALL later steps so incomplete provenance cannot be masked by
    // an earlier locally contradicted candidate route.
    for(std::uint8_t i=0;i<h.count;++i){
        const Step& s=h.steps[i];
        if(!s.originalActionAttested||!s.sourceMethodAttested||!supported(s.game)||
           s.level<h.initialLevel||s.level>h.observedLevel||s.action==Action::Other||
           s.move>467||s.slot>=4)return out;
        if(s.action==Action::VisitGame){
            if(!s.samePokemonAcrossVisitAttested||s.move||s.number||s.receipt)return out;
        } else if(s.action==Action::TM||s.action==Action::HM){
            if(!s.receipt||!s.number||s.number>(s.action==Action::TM?92:8)||!s.move)return out;
        } else if(s.number||s.receipt)return out;
    }
    // Independently preflight EVERY method and title transition before
    // chronology contradictions: a later unverified source must win Unknown.
    Game preGame=h.origin;
    Catalog preSource=current;
    std::uint8_t preLevel=h.initialLevel;
    for(std::uint8_t i=0;i<h.count;++i){
        const Step& s=h.steps[i];
        if(s.level<preLevel)return out;
        if(s.level>preLevel && !preSource.orderedLevelSourceComplete)return out;
        if(s.action==Action::VisitGame){
            if(s.game==preGame)return out;
            const auto next=Source::catalog(s.game,h.species);
            if(!validSource(next)||next.game!=s.game||next.species!=h.species)return out;
            preSource=next;preGame=s.game;
        } else {
            if(s.game!=preGame)return out;
            if(isLevel(s.action) && !preSource.orderedLevelSourceComplete)return out;
            if(s.action==Action::TM||s.action==Action::HM){
                if(!preSource.machineCompatibilitySourceComplete||
                   preSource.numberedMachineMoves.size()!=100)return out;
                if(!preSource.numberedMachineMoves[machineBit(s.action,s.number)])return out;
            }
            if(s.action==Action::Tutor && !preSource.tutorSourceComplete)return out;
            if(s.action==Action::Relearn &&
               (!preSource.originalRelearnerSourceComplete||
                !preSource.orderedLevelSourceComplete))return out;
        }
        preLevel=s.level;
    }
    if(h.observedLevel>preLevel && !preSource.orderedLevelSourceComplete)return out;
    out.rebuilt=h.initialMoves;out.game=h.origin;
    std::uint8_t level=h.initialLevel;std::size_t cursor=skipThrough(current,level);
    std::array<std::uint32_t,64> consumed{};std::uint8_t consumedCount=0;
    for(std::uint8_t i=0;i<h.count;++i){
        const Step& s=h.steps[i];
        if(s.level<level)return out;
        if(s.action==Action::VisitGame){
            // Unknown intermediate level-ups in the old cartridge do not
            // become magically absent merely because the candidate visits another.
            if(s.level>level && !current.orderedLevelSourceComplete)return out;
            if(current.orderedLevelSourceComplete && cursor<current.packedOffers.size() &&
                (packedOffer(current,cursor)>>9)<=s.level){out.status=Status::CandidateContradicted;return out;}
            if(s.game==out.game)return out;
            const Catalog next=Source::catalog(s.game,h.species);
            if(!validSource(next)||next.game!=s.game||next.species!=h.species)return out;
            current=next;out.game=s.game;cursor=skipThrough(current,s.level);++out.visits;
            level=s.level;continue;
        }
        if(s.game!=out.game)return out;
        if(s.level>level && !current.orderedLevelSourceComplete)return out;
        if(isLevel(s.action)){
            if(!current.orderedLevelSourceComplete)return out;
            const auto want=packedOffer(current,cursor);
            if(!want||want!=(static_cast<std::uint16_t>(s.level)<<9|s.move)){
                out.status=Status::CandidateContradicted;return out;}
            ++cursor;++out.offers;
            const bool known=contains(out.rebuilt,s.move);
            const auto free=firstFree(out.rebuilt);
            if(s.action==Action::LevelKnown){if(!known){out.status=Status::CandidateContradicted;return out;}}
            else if(s.action==Action::LevelDecline){if(known||free<4){out.status=Status::CandidateContradicted;return out;}}
            else {if(known||(free<4&&s.slot!=free)){out.status=Status::CandidateContradicted;return out;}out.rebuilt[s.slot]=s.move;}
        } else {
            if(current.orderedLevelSourceComplete && cursor<current.packedOffers.size()&&
               (packedOffer(current,cursor)>>9)<=s.level){out.status=Status::CandidateContradicted;return out;}
            if(s.action==Action::DeleteMove){
                if(s.move||!out.rebuilt[s.slot]||!out.rebuilt[1]){out.status=Status::CandidateContradicted;return out;}
                for(std::uint8_t k=s.slot;k<3;++k)out.rebuilt[k]=out.rebuilt[k+1];
                out.rebuilt[3]=0;++out.deletions;
            } else if(isTeaching(s.action)){
                if(!s.move||contains(out.rebuilt,s.move)){out.status=Status::CandidateContradicted;return out;}
                if(s.action==Action::TM||s.action==Action::HM){
                    if(!current.machineCompatibilitySourceComplete||current.numberedMachineMoves.size()!=100)return out;
                    const auto bit=machineBit(s.action,s.number);
                    const auto pinnedMove=current.numberedMachineMoves[bit];
                    if(!pinnedMove)return out; // unpinned original item, Unknown.
                    if(pinnedMove!=s.move||!machineCompatible(current,s.action,s.number)){
                        out.status=Status::CandidateContradicted;return out;}
                    for(std::uint8_t j=0;j<consumedCount;++j)
                        if(consumed[j]==s.receipt){out.status=Status::CandidateContradicted;return out;}
                    consumed[consumedCount++]=s.receipt;++out.machines;
                } else if(s.action==Action::Tutor){
                    if(!current.tutorSourceComplete)return out;
                    if(!tutorCompatible(current,s.move)){out.status=Status::CandidateContradicted;return out;}
                    ++out.tutors;
                } else {
                    if(!current.originalRelearnerSourceComplete||!current.orderedLevelSourceComplete)return out;
                    if(!relearnerCompatible(current,s.level,s.move)){
                        out.status=Status::CandidateContradicted;return out;}
                    ++out.reminders;
                }
                const auto free=firstFree(out.rebuilt);
                if(free<4&&s.slot!=free){out.status=Status::CandidateContradicted;return out;}
                out.rebuilt[s.slot]=s.move;
            } else return out;
        }
        level=s.level;
    }
    if(out.game!=h.observedGame)return out;
    if(h.observedLevel>level&&!current.orderedLevelSourceComplete)return out;
    if(current.orderedLevelSourceComplete && cursor<current.packedOffers.size()&&
       (packedOffer(current,cursor)>>9)<=h.observedLevel){out.status=Status::CandidateContradicted;return out;}
    out.status=out.rebuilt==h.observedMoves?Status::CandidateConsistent:Status::CandidateContradicted;
    return out;
}
struct Alternatives {
    std::array<History,8> histories{};
    std::uint8_t count=0;
    bool allOriginalHistoriesAuthenticatedAndEnumerated=false;
};
template<class Source> constexpr Status evaluate(const Alternatives& a) noexcept {
    if(!a.count||a.count>a.histories.size()||!a.histories[0].candidate)return Status::Unknown;
    bool unknown=false;
    for(std::uint8_t i=0;i<a.count;++i){
        if(a.histories[i].candidate!=a.histories[0].candidate)return Status::Unknown;
        const auto result=evaluate<Source>(a.histories[i]);
        if(result.status==Status::CandidateConsistent)return result.status;
        if(result.status==Status::Unknown)unknown=true;
    }
    return unknown||!a.allOriginalHistoriesAuthenticatedAndEnumerated?Status::Unknown:Status::CandidateContradicted;
}
} // namespace Legality::Gen4NativeOriginalHistoryCore
