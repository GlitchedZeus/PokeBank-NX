#include "Legality/Gen14CandidateMoveJointEvolutionHistory.h"
#include "Legality/Gen3GbaOriginalMachineData.h"
#include <array>
#include <cassert>
#include <iostream>
namespace H=Legality::Gen14CandidateMoveJointEvolutionHistory;
namespace D=Legality::Gen3GbaOriginalMachineData;
using G=H::Game;using A=H::Action;using M=H::Machine;using S=H::Status;

// Original complete per-species stage excerpts transcribed from pinned pret
// Ruby/Sapphire and FRLG sources. Machine compatibility below uses the exact
// existing committed full 386-species original RSE/FRLG bitmap header.
struct Original {
    static constexpr std::array<H::Offer,11> bulba{{
        {1,33},{4,45},{7,73},{10,22},{15,77},{15,79},
        {20,75},{25,230},{32,74},{39,235},{46,76}}};
    static constexpr std::array<H::Offer,10> charRse{{
        {1,10},{1,45},{7,52},{13,108},{19,99},{25,184},
        {31,53},{37,163},{43,82},{49,83}}};
    static constexpr std::array<H::Offer,10> charFrlg{{
        {1,10},{1,45},{7,52},{13,232},{19,108},{25,184},
        {31,53},{37,163},{43,82},{49,83}}};
    static constexpr std::array<H::Offer,11> charmeleonRse{{
        {1,10},{1,45},{1,52},{7,52},{13,108},{20,99},
        {27,184},{34,53},{41,163},{48,82},{55,83}}};
    static constexpr std::array<H::Offer,11> charmeleonFrlg{{
        {1,10},{1,45},{1,52},{7,52},{13,232},{20,108},
        {27,184},{34,53},{41,163},{48,82},{55,83}}};
    static constexpr std::array<H::Offer,13> charizardRse{{
        {1,10},{1,45},{1,52},{1,108},{7,52},{13,108},
        {20,99},{27,184},{34,53},{36,17},{44,163},{54,82},{64,83}}};
    static constexpr std::array<H::Offer,14> charizardFrlg{{
        {1,257},{1,10},{1,45},{1,52},{1,232},{7,52},{13,232},
        {20,108},{27,184},{34,53},{36,17},{44,163},{54,82},{64,83}}};
    static constexpr bool supported(G g,uint16_t s) noexcept {
        return H::isGen3(g) && (s==1 || s==4 || s==5 || s==6);
    }
    static constexpr uint16_t offerCount(G g,uint16_t s) noexcept {
        if(!supported(g,s))return 0;
        if(s==1)return 11;
        if(s==4)return 10;
        if(s==5)return 11;
        return g==G::FireRed || g==G::LeafGreen?14:13;
    }
    static constexpr H::Offer offerAt(G g,uint16_t s,uint16_t index) noexcept {
        if(index>=offerCount(g,s))return {};
        if(s==1)return bulba[index];
        bool frlg=g==G::FireRed || g==G::LeafGreen;
        if(s==4)return frlg?charFrlg[index]:charRse[index];
        if(s==5)return frlg?charmeleonFrlg[index]:charmeleonRse[index];
        return frlg?charizardFrlg[index]:charizardRse[index];
    }
    static constexpr bool supportsMachine(G g,uint16_t s,M type,uint8_t n,
                                          uint16_t move) noexcept {
        if(!supported(g,s))return false;
        if((type==M::TM && (!n || n>50)) ||
           (type==M::HM && (!n || n>8)))return false;
        const unsigned bit=type==M::TM?n-1:n+49;
        const auto mask=(g==G::FireRed || g==G::LeafGreen)
            ?D::kFireRedLeafGreenMasks[s]:D::kRubySapphireMasks[s];
        return (mask&(uint64_t{1}<<bit)) && D::kMachineMove[bit]==move;
    }
};
static_assert(Original::offerAt(G::Ruby,4,3).move==108);
static_assert(Original::offerAt(G::FireRed,4,3).move==232);
static_assert(Original::offerAt(G::Ruby,6,9).move==17);
static_assert(Original::offerAt(G::FireRed,6,10).move==17);
static_assert(Original::supportsMachine(G::Ruby,1,M::TM,6,92));
static_assert(!Original::supportsMachine(G::Ruby,5,M::HM,2,19));
static_assert(Original::supportsMachine(G::Ruby,6,M::HM,2,19));
static_assert(Original::supportsMachine(G::FireRed,1,M::TM,10,237));

constexpr H::Path evolveWithItems(G g=G::Ruby) {
    H::Path p{};p.candidateId=3001;p.origin=g;p.observedGame=g;
    p.originalSpecies=5;p.observedSpecies=6;
    p.initialLevel=33;p.observedLevel=38;
    p.initialMoves={10,45,108,184};p.observedMoves={19,45,17,92};
    p.originalSnapshotVerified=p.samePokemonVerified=true;
    p.everyLevelUpDecisionVerified=p.wholeClaimedHistoryVerified=true;
    p.count=5;
    p.steps[0]={g,34,A::OfferedDecline,53,0,0,M::TM,0,G::Unknown,0,true,false,0};
    p.steps[1]={g,36,A::Evolve,0,0,6,M::TM,0,G::Unknown,0,true,true,0};
    p.steps[2]={g,36,A::OfferedLearn,17,2,0,M::TM,0,G::Unknown,0,true,false,0};
    p.steps[3]={g,37,A::TeachMachine,92,3,0,M::TM,6,g,6,true,true,401};
    p.steps[4]={g,38,A::TeachMachine,19,0,0,M::HM,2,g,6,true,true,0};
    return p;
}
constexpr H::Path bulbasaurDoubleOffer() {
    H::Path p{};p.candidateId=3002;p.origin=G::Ruby;p.observedGame=G::Ruby;
    p.originalSpecies=1;p.observedSpecies=1;p.initialLevel=14;p.observedLevel=15;
    p.initialMoves={33,45,73,22};p.observedMoves={77,79,92,237};
    p.originalSnapshotVerified=p.samePokemonVerified=true;
    p.everyLevelUpDecisionVerified=p.wholeClaimedHistoryVerified=true;
    p.count=4;
    p.steps[0]={G::Ruby,15,A::OfferedLearn,77,0,0,M::TM,0,G::Unknown,0,true,false,0};
    p.steps[1]={G::Ruby,15,A::OfferedLearn,79,1,0,M::TM,0,G::Unknown,0,true,false,0};
    p.steps[2]={G::Ruby,15,A::TeachMachine,92,2,0,M::TM,6,G::Ruby,1,true,true,111};
    p.steps[3]={G::Ruby,15,A::TeachMachine,237,3,0,M::TM,10,G::Ruby,1,true,true,112};
    return p;
}
constexpr H::Path tradedCharmander(G destination=G::FireRed,uint16_t offered=232) {
    H::Path p{};p.candidateId=3003;p.origin=G::Ruby;p.observedGame=destination;
    p.originalSpecies=4;p.observedSpecies=4;p.initialLevel=12;p.observedLevel=13;
    p.initialMoves={10,45,52,0};p.observedMoves={10,45,52,offered};
    p.originalSnapshotVerified=p.samePokemonVerified=true;
    p.everyLevelUpDecisionVerified=p.wholeClaimedHistoryVerified=true;
    p.count=2;
    p.steps[0]={destination,12,A::VisitGame,0,0,0,M::TM,0,G::Unknown,0,true,false,0};
    p.steps[1]={destination,13,A::OfferedLearn,offered,3,0,M::TM,0,G::Unknown,0,true,false,0};
    return p;
}
static_assert(H::evaluate<Original>(evolveWithItems()).status==S::CandidateConsistent);
static_assert(H::evaluate<Original>(evolveWithItems()).machines==2);
static_assert(H::evaluate<Original>(evolveWithItems()).evolutions==1);
static_assert(H::evaluate<Original>(evolveWithItems(G::FireRed)).status==S::CandidateConsistent);
static_assert(H::evaluate<Original>(bulbasaurDoubleOffer()).status==S::CandidateConsistent);
static_assert(H::evaluate<Original>(bulbasaurDoubleOffer()).offers==2);
static_assert(H::evaluate<Original>(tradedCharmander()).status==S::CandidateConsistent);
static_assert([]{auto p=tradedCharmander(G::LeafGreen,232);
    return H::evaluate<Original>(p).status==S::CandidateConsistent;}());
static_assert([]{auto p=tradedCharmander(G::FireRed,108);
    return H::evaluate<Original>(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=bulbasaurDoubleOffer();
    p.steps[2]=p.steps[1];p.steps[1]={G::Ruby,15,A::TeachMachine,92,2,0,M::TM,6,G::Ruby,1,true,true,111};
    return H::evaluate<Original>(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=bulbasaurDoubleOffer();p.steps[3].machineNumber=6;
    return H::evaluate<Original>(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=bulbasaurDoubleOffer();p.steps[3].receiptId=111;
    return H::evaluate<Original>(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=bulbasaurDoubleOffer();p.steps[3].receiptId=0;
    return H::evaluate<Original>(p).status==S::Unresolved;}());
static_assert([]{auto p=bulbasaurDoubleOffer();p.steps[3].receiptSpecies=4;
    return H::evaluate<Original>(p).status==S::Unresolved;}());
static_assert([]{auto p=bulbasaurDoubleOffer();p.steps[3].receiptGame=G::Emerald;
    return H::evaluate<Original>(p).status==S::Unresolved;}());
static_assert([]{auto p=bulbasaurDoubleOffer();p.steps[0].slot=4;
    return H::evaluate<Original>(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=bulbasaurDoubleOffer();p.count=1;
    return H::evaluate<Original>(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=bulbasaurDoubleOffer();p.steps[3].methodReceiptVerified=false;
    return H::evaluate<Original>(p).status==S::Unresolved;}());
static_assert([]{auto p=bulbasaurDoubleOffer();p.steps[3].action=A::UnsupportedRoute;
    return H::evaluate<Original>(p).status==S::Unresolved;}());
static_assert([]{auto p=bulbasaurDoubleOffer();p.originalSnapshotVerified=false;
    return H::evaluate<Original>(p).status==S::Unresolved;}());
static_assert([]{auto p=evolveWithItems();p.steps[1].level=35;
    return H::evaluate<Original>(p).status==S::Unresolved;}());
static_assert([]{auto p=evolveWithItems();p.steps[1].targetSpecies=94;
    return H::evaluate<Original>(p).status==S::Unresolved;}());
static_assert([]{auto p=evolveWithItems();p.steps[1].methodReceiptVerified=false;
    return H::evaluate<Original>(p).status==S::Unresolved;}());
static_assert([]{auto p=evolveWithItems();p.steps[2].move=97;
    return H::evaluate<Original>(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=evolveWithItems();p.steps[4].receiptSpecies=5;
    return H::evaluate<Original>(p).status==S::Unresolved;}());
static_assert([]{auto p=evolveWithItems();p.steps[4].level=37;
    p.steps[3].level=38;
    return H::evaluate<Original>(p).status==S::Unresolved;}());
static_assert([]{auto p=evolveWithItems();p.steps[4].machineNumber=8;
    return H::evaluate<Original>(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=evolveWithItems();p.steps[4].slot=3;
    return H::evaluate<Original>(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=evolveWithItems();p.count=4;p.observedLevel=37;
    p.observedMoves={10,45,17,92};
    return H::evaluate<Original>(p).status==S::CandidateConsistent;}());
static_assert([]{auto p=evolveWithItems();
    p.steps[4]={G::Ruby,35,A::TeachMachine,19,0,0,M::HM,2,G::Ruby,5,true,true,0};
    return H::evaluate<Original>(p).status==S::Unresolved;}());
static_assert([]{auto p=evolveWithItems();
    p.steps[4]={G::Ruby,39,A::UnsupportedRoute,0,0,0,M::TM,0,G::Unknown,0,true,false,0};
    return H::evaluate<Original>(p).status==S::Unresolved;}());
// A fully attested Fly TM/HM method is available only AFTER the original
// Charmeleon has evolved; misplacing the receipt before evolution fails.
static_assert([]{auto p=evolveWithItems();p.count=4;
    p.steps[1]={G::Ruby,35,A::TeachMachine,19,0,0,M::HM,2,G::Ruby,5,true,true,0};
    p.steps[2]={G::Ruby,36,A::Evolve,0,0,6,M::TM,0,G::Unknown,0,true,true,0};
    p.steps[3]={G::Ruby,36,A::OfferedLearn,17,2,0,M::TM,0,G::Unknown,0,true,false,0};
    return H::evaluate<Original>(p).status==S::CandidateContradicted;}());
// A delayed level-37 evolution does not retroactively trigger the new
// form's level-36 Wing Attack move offer.
static_assert([]{auto p=evolveWithItems();p.steps[1].level=37;
    p.steps[2].level=37;
    return H::evaluate<Original>(p).status==S::CandidateContradicted;}());
// A verified Move Deleter shifts all later slots and cannot duplicate or
// resurrect a forgotten move. An unverified deletion stays Unknown.
static_assert([]{auto p=bulbasaurDoubleOffer();p.count=5;p.observedLevel=16;
    p.steps[4]={G::Ruby,16,A::DeleteMove,92,2,0,M::TM,0,G::Unknown,0,true,true,0};
    p.observedMoves={77,79,237,0};
    return H::evaluate<Original>(p).status==S::CandidateConsistent;}());
static_assert([]{auto p=bulbasaurDoubleOffer();p.count=5;p.observedLevel=16;
    p.steps[4]={G::Ruby,16,A::DeleteMove,92,2,0,M::TM,0,G::Unknown,0,true,false,0};
    p.observedMoves={77,79,237,0};
    return H::evaluate<Original>(p).status==S::Unresolved;}());
static_assert([]{auto p=bulbasaurDoubleOffer();p.count=5;p.observedLevel=16;
    p.steps[4]={G::Ruby,16,A::DeleteMove,33,2,0,M::TM,0,G::Unknown,0,true,true,0};
    p.observedMoves={77,79,237,0};
    return H::evaluate<Original>(p).status==S::CandidateContradicted;}());
// Original observed species and game are part of the final identity, not
// inferred from compatible moves alone.
static_assert([]{auto p=evolveWithItems();p.observedSpecies=5;
    return H::evaluate<Original>(p).status==S::CandidateContradicted;}());
static_assert([]{auto p=tradedCharmander();p.steps[0].originalActionVerified=false;
    return H::evaluate<Original>(p).status==S::Unresolved;}());
static_assert([]{auto p=tradedCharmander();p.observedGame=G::Ruby;
    return H::evaluate<Original>(p).status==S::CandidateContradicted;}());

constexpr H::Alternatives unknownAlternatives(){
    H::Alternatives a{};a.count=2;a.allGenuineHistoriesEnumerated=true;
    a.paths[0]=bulbasaurDoubleOffer();a.paths[0].steps[3].machineNumber=6;
    a.paths[1]=bulbasaurDoubleOffer();a.paths[1].originalSnapshotVerified=false;
    return a;
}
static_assert(H::evaluate<Original>(unknownAlternatives())==S::Unresolved);
static_assert([]{auto a=unknownAlternatives();a.paths[1]=bulbasaurDoubleOffer();
    return H::evaluate<Original>(a)==S::CandidateConsistent;}());
static_assert([]{auto a=unknownAlternatives();a.paths[1]=a.paths[0];
    return H::evaluate<Original>(a)==S::CandidateContradicted;}());
static_assert([]{auto a=unknownAlternatives();a.paths[1]=a.paths[0];
    a.allGenuineHistoriesEnumerated=false;
    return H::evaluate<Original>(a)==S::Unresolved;}());
static_assert([]{auto a=unknownAlternatives();a.paths[1].candidateId=123456;
    return H::evaluate<Original>(a)==S::Unresolved;}());

int main(){
    constexpr std::array<G,5> games{{G::Ruby,G::Sapphire,G::Emerald,G::FireRed,G::LeafGreen}};
    for(G game:games){
        assert(H::evaluate<Original>(evolveWithItems(game)).status==S::CandidateConsistent);
        const auto traded=tradedCharmander(game,
            game==G::FireRed||game==G::LeafGreen?232:108);
        assert(H::evaluate<Original>(traded).status==
               (game==G::Ruby?S::Unresolved:S::CandidateConsistent));
    }
    uint32_t negatives=0,unknowns=0,positives=0;
    for(uint8_t slot=0;slot<4;++slot)for(uint16_t move=1;move<=354;++move){
        auto p=bulbasaurDoubleOffer();p.steps[3].slot=slot;p.steps[3].move=move;
        auto status=H::evaluate<Original>(p).status;
        if(status==S::CandidateConsistent)++positives;
        else if(status==S::CandidateContradicted)++negatives;
        else ++unknowns;
    }
    assert(negatives>1300 && positives==1 && unknowns==0);
    // Independent alternative routes are never globally exhausted just
    // because one source-specific hypothetical sequence was contradicted.
    assert(H::evaluate<Original>(unknownAlternatives())==S::Unresolved);
    std::cout<<"L3 same-individual evolution + original machines: "<<positives
             <<" fuzzed positives, "<<negatives<<" local negatives: PASS\n";
}
