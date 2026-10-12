#pragma once
// L3 Stage 7: candidate-local authenticated joint chronology for native PK1..PK4.
// No inference from detached records, no cross-lane dependencies, no save writes.
// An inconsistent *hypothesis* never authorizes a global Invalid judgment.
#include <array>
#include <cstdint>

namespace Legality::Gen14AuthenticatedMoveHistory {
enum class Game : std::uint8_t {
    Unknown, Red, Blue, Yellow, Gold, Silver, Crystal,
    Ruby, Sapphire, Emerald, FireRed, LeafGreen,
    Diamond, Pearl, Platinum, HeartGold, SoulSilver
};
constexpr std::uint8_t generation(Game g) noexcept {
    if (g>=Game::Red && g<=Game::Yellow) return 1;
    if (g>=Game::Gold && g<=Game::Crystal) return 2;
    if (g>=Game::Ruby && g<=Game::LeafGreen) return 3;
    if (g>=Game::Diamond && g<=Game::SoulSilver) return 4;
    return 0;
}
constexpr std::uint16_t maximumMove(Game g) noexcept {
    switch (generation(g)) {
    case 1:return 165;
    case 2:return 251;
    case 3:return 354;
    case 4:return 467;
    default:return 0;
    }
}
constexpr std::uint16_t maximumSpecies(Game g) noexcept {
    switch (generation(g)) {
    case 1:return 151;
    case 2:return 251;
    case 3:return 386;
    case 4:return 493;
    default:return 0;
    }
}
enum class Status : std::uint8_t { Unknown, CandidateConsistent, CandidateContradicted };
enum class Action : std::uint8_t {
    Learn, Decline, AlreadyKnown, DeleteMove, Evolve, Visit, TimeCapsule, PalPark
};
enum class Method : std::uint8_t {
    None, LevelUp, TM, HM, Tutor, Reminder, InheritedEgg, DistributedEvent
};
// Snapshot identifiers must refer to separately authenticated *immutable* bytes.
// Source files/saves are never changed by this evaluator.
struct Snapshot {
    std::uint64_t id=0, individual=0;
    std::uint32_t candidate=0;
    Game game=Game::Unknown;
    std::uint16_t species=0;
    std::uint8_t form=0, level=0;
    std::array<std::uint16_t,4> moves{};
    bool nativeRecordVerified=false, sourceSaveVerified=false;
};
// L2 supplies a separately authenticated source egg, parent IDs, exact
// original version and source-checked acquisition. This is NOT a guess from PK3/4.
struct BreedingEvidence {
    std::uint64_t evidenceId=0, eggSnapshotId=0, motherSnapshotId=0, fatherSnapshotId=0;
    std::uint64_t individual=0;
    std::uint32_t candidate=0;
    Game originalGame=Game::Unknown;
    std::uint16_t species=0, move=0;
    std::uint8_t form=0, acquisitionLevel=0;
    bool originalEggVerified=false, parentsIndependentlyVerified=false;
    bool inheritanceMethodSourceVerified=false, sameEggIndividualVerified=false;
};
// L4 supplies a separately authenticated *specific* distribution/template,
// including exact game/form, the constrained move and a valid claim level.
struct EventEvidence {
    std::uint64_t evidenceId=0, distributionId=0, originalSnapshotId=0;
    std::uint64_t individual=0;
    std::uint32_t candidate=0;
    std::uint32_t templateId=0;
    Game originalGame=Game::Unknown;
    std::uint16_t species=0, move=0;
    std::uint8_t form=0, earliestLevel=0, latestLevel=0;
    bool distributionIndependentlyVerified=false, templateIndependentlyVerified=false;
    bool templateMethodMoveVerified=false, originalEncounterVerified=false;
};
struct Step {
    Snapshot before{}, after{};
    Action action=Action::Learn;
    Method method=Method::None;
    std::uint16_t move=0;
    std::uint8_t slot=0;
    // Every source-specific machine maps exact title+species+form+item to move.
    std::uint16_t machineNumber=0;
    std::uint64_t itemReceiptId=0, crossLaneEvidenceId=0;
    bool originalActionVerified=false, methodSourceVerified=false;
    bool sameIndividualContinuityVerified=false;
    bool speciesFormTransitionSourceVerified=false;
    bool visitOrTransferVerified=false;
};
struct History {
    std::uint32_t candidate=0;
    std::uint64_t individual=0;
    Game originalGame=Game::Unknown;
    Snapshot initial{}, observed{};
    // True ONLY after Stage 1-6 L3 exact-source replay verifies mandatory offers,
    // machine mappings and acquisition ordering; never from a PK slot list.
    bool exhaustiveIntermediateChronologyVerified=false;
    bool sourceSpecificChronologyPassed=false;
    std::array<Step,64> steps{};
    std::uint8_t count=0;
    std::array<BreedingEvidence,8> breeding{};
    std::uint8_t breedingCount=0;
    std::array<EventEvidence,8> events{};
    std::uint8_t eventCount=0;
};
struct Result {
    Status status=Status::Unknown;
    std::uint8_t evaluatedSteps=0, eggClaims=0, eventClaims=0, transfers=0;
    bool globalInvalidAuthorized=false; // hard invariant
};
constexpr bool slots(const Snapshot& s) noexcept {
    if (!s.moves[0]) return false;
    bool gap=false;
    for (std::uint8_t i=0;i<4;++i) {
        const auto m=s.moves[i];
        if (!m) {gap=true;continue;}
        if (gap||m>maximumMove(s.game))return false;
        for (std::uint8_t j=0;j<i;++j)if(s.moves[j]==m)return false;
    }
    return true;
}
constexpr bool identified(const Snapshot& s, std::uint32_t candidate,
                          std::uint64_t individual) noexcept {
    return s.id&&s.candidate==candidate&&s.individual==individual&&
           s.nativeRecordVerified&&s.sourceSaveVerified&&s.level>=1&&s.level<=100&&
           s.species>=1&&s.species<=maximumSpecies(s.game)&&slots(s);
}
constexpr bool identical(const Snapshot& a,const Snapshot& b) noexcept {
    return a.id==b.id&&a.individual==b.individual&&a.candidate==b.candidate&&
           a.game==b.game&&a.species==b.species&&a.form==b.form&&a.level==b.level&&
           a.moves==b.moves&&a.nativeRecordVerified==b.nativeRecordVerified&&
           a.sourceSaveVerified==b.sourceSaveVerified;
}
constexpr bool has(const std::array<std::uint16_t,4>& a,std::uint16_t m) noexcept {
    for (auto x:a)if(x==m)return true;
    return false;
}
constexpr std::uint8_t emptySlot(const std::array<std::uint16_t,4>& a) noexcept {
    for(std::uint8_t i=0;i<4;++i)if(!a[i])return i;
    return 4;
}
constexpr bool oldHM(std::uint16_t m) noexcept {
    return m==15||m==19||m==57||m==70||m==148||m==249||m==127||m==291;
}
// Proof requirements are checked for the WHOLE packet before any contradiction:
// an unknown event or source method later in the path defeats an earlier
// hypothetical contradiction, leaving the global record unresolved.
constexpr bool evidenceComplete(const History& h) noexcept {
    if (!h.candidate||!h.individual||!generation(h.originalGame)||
        !h.exhaustiveIntermediateChronologyVerified||!h.sourceSpecificChronologyPassed||
        h.count>h.steps.size()||
        h.breedingCount>h.breeding.size()||h.eventCount>h.events.size()||
        !identified(h.initial,h.candidate,h.individual)||
        !identified(h.observed,h.candidate,h.individual)||
        h.initial.game!=h.originalGame)return false;
    Snapshot prev=h.initial;
    for(std::uint8_t i=0;i<h.count;++i) {
        const auto& s=h.steps[i];
        if(!identified(s.before,h.candidate,h.individual)||
           !identified(s.after,h.candidate,h.individual)||
           !identical(s.before,prev)||s.after.id==s.before.id||
           !s.originalActionVerified||!s.methodSourceVerified||
           !s.sameIndividualContinuityVerified||s.after.level<s.before.level||
           s.slot>=4)return false;
        for(std::uint8_t j=0;j<i;++j)
            if(s.after.id==h.steps[j].before.id||
               s.after.id==h.steps[j].after.id)return false;
        if(s.action==Action::Evolve && !s.speciesFormTransitionSourceVerified)return false;
        if((s.action==Action::Visit||s.action==Action::TimeCapsule||
            s.action==Action::PalPark)&&!s.visitOrTransferVerified)return false;
        if(s.action==Action::Learn) {
            if(!s.move||s.move>maximumMove(s.before.game)||s.method==Method::None)
                return false;
            if(s.method==Method::TM||s.method==Method::HM) {
                if(!s.itemReceiptId||!s.machineNumber||
                   s.machineNumber>(s.method==Method::TM?92:8))return false;
                for(std::uint8_t j=0;j<i;++j)
                    if(s.itemReceiptId==h.steps[j].itemReceiptId)return false;
            } else if(s.itemReceiptId||s.machineNumber)return false;
            if(s.method==Method::InheritedEgg||s.method==Method::DistributedEvent) {
                if(!s.crossLaneEvidenceId)return false;
            } else if(s.crossLaneEvidenceId)return false;
        } else if(s.action==Action::Decline||s.action==Action::AlreadyKnown) {
            if(s.method!=Method::LevelUp||!s.move||s.itemReceiptId||
               s.crossLaneEvidenceId||s.machineNumber)return false;
        } else if(s.method!=Method::None||s.move||s.itemReceiptId||
                  s.crossLaneEvidenceId||s.machineNumber)return false;
        prev=s.after;
    }
    if(!identical(prev,h.observed))return false;
    // Do not accept orphan evidence that could be imported from other paths.
    for(std::uint8_t i=0;i<h.breedingCount;++i) {
        const auto& e=h.breeding[i];
        if(!e.evidenceId||!e.eggSnapshotId||!e.motherSnapshotId||
           !e.fatherSnapshotId||e.motherSnapshotId==e.fatherSnapshotId||
           !e.originalEggVerified||!e.parentsIndependentlyVerified||
           !e.inheritanceMethodSourceVerified||!e.sameEggIndividualVerified||
           e.candidate!=h.candidate||e.individual!=h.individual||
           e.originalGame!=h.originalGame||generation(e.originalGame)<2)return false;
        for(std::uint8_t j=0;j<i;++j)if(e.evidenceId==h.breeding[j].evidenceId)return false;
        std::uint8_t matches=0;
        for(std::uint8_t j=0;j<h.count;++j)if(h.steps[j].action==Action::Learn&&
            h.steps[j].method==Method::InheritedEgg&&
            h.steps[j].crossLaneEvidenceId==e.evidenceId)++matches;
        if(matches!=1)return false;
    }
    for(std::uint8_t i=0;i<h.eventCount;++i) {
        const auto& e=h.events[i];
        if(!e.evidenceId||!e.distributionId||!e.templateId||!e.originalSnapshotId||
           !e.distributionIndependentlyVerified||!e.templateIndependentlyVerified||
           !e.templateMethodMoveVerified||!e.originalEncounterVerified||
           e.candidate!=h.candidate||e.individual!=h.individual||
           e.originalGame!=h.originalGame||!e.earliestLevel||
           e.latestLevel<e.earliestLevel||e.latestLevel>100)return false;
        for(std::uint8_t j=0;j<i;++j) {
            const auto& prior=h.events[j];
            if(e.evidenceId==prior.evidenceId)return false;
            // One event distribution may supply several original move slots,
            // but may never borrow a different template or recipient identity.
            if(e.distributionId==prior.distributionId &&
               (e.templateId!=prior.templateId||e.originalGame!=prior.originalGame||
                e.species!=prior.species||e.form!=prior.form||
                e.individual!=prior.individual||e.candidate!=prior.candidate))return false;
        }
        std::uint8_t matches=0;
        for(std::uint8_t j=0;j<h.count;++j)if(h.steps[j].action==Action::Learn&&
            h.steps[j].method==Method::DistributedEvent&&
            h.steps[j].crossLaneEvidenceId==e.evidenceId)++matches;
        if(matches!=1)return false;
    }
    for(std::uint8_t i=0;i<h.count;++i) {
        const auto& s=h.steps[i];
        if(s.action!=Action::Learn)continue;
        if(s.method==Method::InheritedEgg) {
            std::uint8_t matches=0;
            for(std::uint8_t j=0;j<h.breedingCount;++j){const auto& e=h.breeding[j];
                if(e.evidenceId!=s.crossLaneEvidenceId)continue;
                if(e.eggSnapshotId!=s.before.id||e.species!=s.before.species||
                   e.form!=s.before.form||e.originalGame!=s.before.game||
                   e.move!=s.move||e.acquisitionLevel!=s.before.level||
                   s.before.game!=h.originalGame||s.before.level!=s.after.level)return false;
                ++matches;
            }
            if(matches!=1)return false;
        }
        if(s.method==Method::DistributedEvent) {
            std::uint8_t matches=0;
            for(std::uint8_t j=0;j<h.eventCount;++j){const auto& e=h.events[j];
                if(e.evidenceId!=s.crossLaneEvidenceId)continue;
                if(e.originalSnapshotId!=s.before.id||e.species!=s.before.species||
                   e.form!=s.before.form||e.originalGame!=s.before.game||
                   e.move!=s.move||s.before.level<e.earliestLevel||
                   s.before.level>e.latestLevel||s.before.level!=s.after.level||
                   s.before.game!=h.originalGame)return false;
                ++matches;
            }
            if(matches!=1)return false;
        }
    }
    return true;
}
constexpr Result evaluate(const History& h) noexcept {
    Result out{};
    if(!evidenceComplete(h))return out;
    Snapshot prev=h.initial;
    for(std::uint8_t i=0;i<h.count;++i) {
        const Step& s=h.steps[i];
        Snapshot expected=s.before;
        if(s.action==Action::Learn) {
            if(has(expected.moves,s.move)||
               (emptySlot(expected.moves)<4 && s.slot!=emptySlot(expected.moves))) {
                out.status=Status::CandidateContradicted;return out;
            }
            expected.moves[s.slot]=s.move;
            if(s.method==Method::InheritedEgg)++out.eggClaims;
            if(s.method==Method::DistributedEvent)++out.eventClaims;
        } else if(s.action==Action::Decline) {
            if(has(expected.moves,s.move)||emptySlot(expected.moves)<4) {
                out.status=Status::CandidateContradicted;return out;
            }
        } else if(s.action==Action::AlreadyKnown) {
            if(!has(expected.moves,s.move)) {
                out.status=Status::CandidateContradicted;return out;
            }
        } else if(s.action==Action::DeleteMove) {
            if(!expected.moves[s.slot]||!expected.moves[1]) {
                out.status=Status::CandidateContradicted;return out;
            }
            for(std::uint8_t j=s.slot;j<3;++j)expected.moves[j]=expected.moves[j+1];
            expected.moves[3]=0;
        } else if(s.action==Action::Evolve) {
            if(s.before.game!=s.after.game||(s.before.species==s.after.species&&
               s.before.form==s.after.form)) {
                out.status=Status::CandidateContradicted;return out;
            }
            expected.species=s.after.species;
            expected.form=s.after.form;
        } else if(s.action==Action::Visit) {
            if(s.before.game==s.after.game||
               generation(s.before.game)!=generation(s.after.game)) {
                out.status=Status::CandidateContradicted;return out;
            }
            expected.game=s.after.game;
        } else if(s.action==Action::TimeCapsule) {
            const auto a=generation(s.before.game),b=generation(s.after.game);
            if(!((a==1&&b==2)||(a==2&&b==1))||s.before.species>151) {
                out.status=Status::CandidateContradicted;return out;
            }
            for(auto m:s.before.moves)if(m>165) {
                out.status=Status::CandidateContradicted;return out;
            }
            expected.game=s.after.game;++out.transfers;
        } else if(s.action==Action::PalPark) {
            if(generation(s.before.game)!=3||generation(s.after.game)!=4||
               s.before.species>386) {
                out.status=Status::CandidateContradicted;return out;
            }
            for(auto m:s.before.moves)if(oldHM(m)) {
                out.status=Status::CandidateContradicted;return out;
            }
            expected.game=s.after.game;++out.transfers;
        }
        expected.id=s.after.id;
        expected.level=s.after.level;
        if(!identical(expected,s.after)) {
            out.status=Status::CandidateContradicted;return out;
        }
        prev=s.after;++out.evaluatedSteps;
    }
    if(!identical(prev,h.observed))return out;
    out.status=Status::CandidateConsistent;
    return out;
}
// Candidate universe: a single contradicted path is not a global verdict;
// a remaining Unknown alternative takes precedence over all local negatives.
struct Alternatives {
    std::array<History,4> paths{};
    std::uint8_t count=0;
    bool exhaustiveUniverseIndependentlyVerified=false;
};
struct AlternativeResult {
    Status status=Status::Unknown;
    std::uint8_t consistent=0, contradicted=0, unknown=0;
    bool globalInvalidAuthorized=false;
};
constexpr AlternativeResult evaluate(const Alternatives& a) noexcept {
    AlternativeResult out{};
    if(!a.count||a.count>a.paths.size())return out;
    const auto id=a.paths[0].candidate;
    const auto individual=a.paths[0].individual;
    if(!id||!individual)return out;
    for(std::uint8_t i=0;i<a.count;++i) {
        if(a.paths[i].candidate!=id||a.paths[i].individual!=individual)return out;
        switch(evaluate(a.paths[i]).status) {
        case Status::CandidateConsistent:++out.consistent;break;
        case Status::CandidateContradicted:++out.contradicted;break;
        case Status::Unknown:++out.unknown;break;
        }
    }
    if(out.consistent)out.status=Status::CandidateConsistent;
    else if(!out.unknown&&a.exhaustiveUniverseIndependentlyVerified)
        out.status=Status::CandidateContradicted;
    return out;
}
static_assert(generation(Game::Red)==1&&generation(Game::Crystal)==2);
static_assert(generation(Game::Emerald)==3&&generation(Game::SoulSilver)==4);
static_assert(maximumMove(Game::Platinum)==467&&maximumSpecies(Game::Ruby)==386);
} // namespace Legality::Gen14AuthenticatedMoveHistory
