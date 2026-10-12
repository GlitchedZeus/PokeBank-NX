#include "Legality/Gen14CandidateMovePalParkNativeJoint.h"
#include <array>
#include <cassert>
#include <iostream>
namespace B=Legality::Gen14CandidateMovePalParkNativeJoint;
namespace H=Legality::Gen14CandidateMoveJointEvolutionHistory;
using G=B::Game;using A=B::Action;using S=B::Status;
// Full original RSE/FRLG Bulbasaur row, source pins pokeruby@63a8cbf /
// pokefirered@c75f352. Original game sources retained separately.
struct OriginalGba {
    static constexpr std::array<H::Offer,11> kBulba{{
        {1,33},{4,45},{7,73},{10,22},{15,77},{15,79},
        {20,75},{25,230},{32,74},{39,235},{46,76}
    }};
    static constexpr bool supported(H::Game g,uint16_t sp) {
        return H::isGen3(g) && sp==1;
    }
    static constexpr uint16_t offerCount(H::Game g,uint16_t sp) {
        return supported(g,sp)?static_cast<uint16_t>(kBulba.size()):0;
    }
    static constexpr H::Offer offerAt(H::Game g,uint16_t sp,uint16_t n) {
        return n<offerCount(g,sp)?kBulba[n]:H::Offer{};
    }
    static constexpr bool supportsMachine(H::Game,uint16_t,H::Machine,uint8_t,uint16_t) {
        return false;
    }
};
// Complete original Platinum Bulbasaur (National #1) level-up source row.
// pret/pokeplatinum@fe1ea418, res/pokemon/bulbasaur/data.json.
struct Platinum {
    static constexpr std::array<B::Offer,14> levels{{
        {1,33},{3,45},{7,73},{9,22},{13,77},{13,79},
        {15,36},{19,75},{21,230},{25,74},{27,38},
        {31,388},{33,235},{37,402}
    }};
    static constexpr bool supported(G game,uint16_t species) {
        return game==G::Platinum && species==1;
    }
    static constexpr uint16_t offerCount(G g,uint16_t sp) {
        return supported(g,sp)?static_cast<uint16_t>(levels.size()):0;
    }
    static constexpr B::Offer offerAt(G g,uint16_t sp,uint16_t n) {
        return n<offerCount(g,sp)?levels[n]:B::Offer{};
    }
    static constexpr bool machineKnown(G g,bool hm,uint8_t number) {
        return g==G::Platinum && ((hm && number==1)||(!hm && (number==6||number==53)));
    }
    static constexpr bool machineCompatible(G g,uint16_t sp,bool hm,uint8_t number,uint16_t move) {
        return supported(g,sp) && ((hm&&number==1&&move==15)||
            (!hm&&number==6&&move==92)||(!hm&&number==53&&move==412));
    }
    static constexpr bool tutorCompatible(G g,uint16_t sp,uint16_t move) {
        return supported(g,sp) && (move==235||move==189);
    }
};
constexpr std::array<H::Game,5> gba{H::Game::Ruby,H::Game::Sapphire,H::Game::Emerald,H::Game::FireRed,H::Game::LeafGreen};
constexpr std::array<G,5> nds{G::Diamond,G::Pearl,G::Platinum,G::HeartGold,G::SoulSilver};
constexpr B::Input base(H::Game origin,G target){
    B::Input a{};a.candidateId=8055;a.species=1;
    a.arrivalGame=target;a.observedGame=target;a.arrivalLevel=20;a.observedLevel=20;
    a.samePokemonThroughTransferVerified=true;
    a.preTransferSnapshotVerified=true;a.immediateArrivalSnapshotVerified=true;
    a.nativeActionsCompleteVerified=true;
    auto& p=a.departure;p.candidateId=a.candidateId;
    p.origin=origin;p.observedGame=origin;p.originalSpecies=1;p.observedSpecies=1;
    p.initialLevel=20;p.observedLevel=20;
    p.initialMoves={33,45,73,22};p.observedMoves=p.initialMoves;
    p.originalSnapshotVerified=p.samePokemonVerified=true;
    p.everyLevelUpDecisionVerified=p.wholeClaimedHistoryVerified=true;
    a.recordedDeparture=p.observedMoves;a.recordedArrival=p.observedMoves;
    a.observedMoves=p.observedMoves;return a;
}
constexpr B::Input taught(){
    auto a=base(H::Game::Ruby,G::Platinum);
    a.observedLevel=21;a.count=4;
    a.steps[0]={G::Platinum,21,A::OfferDecline,230,0,false,0,0,true,true};
    a.steps[1]={G::Platinum,21,A::Machine,412,0,false,53,1001,true,true};
    a.steps[2]={G::Platinum,21,A::Tutor,235,1,false,0,0,true,true};
    a.steps[3]={G::Platinum,21,A::Reminder,230,2,false,0,0,true,true};
    a.observedMoves={412,235,230,22};return a;
}
static_assert(B::evaluate<OriginalGba,Platinum>(taught()).status==S::Consistent);
static_assert(B::evaluate<OriginalGba,Platinum>(taught()).verifiedOffers==1);
static_assert(B::evaluate<OriginalGba,Platinum>(taught()).verifiedMachines==1);
static_assert(B::evaluate<OriginalGba,Platinum>(taught()).verifiedTutors==1);
static_assert(B::evaluate<OriginalGba,Platinum>(taught()).verifiedReminders==1);
static_assert([]{auto a=taught();a.steps[0].move=22;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Contradicted;}());
static_assert([]{auto a=taught();a.steps[1].machineNumber=24;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Unresolved;}());
static_assert([]{auto a=taught();a.steps[1].itemReceipt=0;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Unresolved;}());
static_assert([]{auto a=taught();a.steps[3].methodSourceVerified=false;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Unresolved;}());
static_assert([]{auto a=taught();a.steps[1].move=92;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Contradicted;}());
static_assert([]{auto a=taught();a.steps[1].slot=3;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Consistent;}()==false); // final moves mismatch
static_assert([]{auto a=taught();a.steps[2].move=467;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Contradicted;}());
static_assert([]{auto a=taught();a.preTransferSnapshotVerified=false;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Unresolved;}());
static_assert([]{auto a=taught();a.recordedArrival={33,45,73,76};
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Contradicted;}());
static_assert([]{auto a=taught();a.departure.initialMoves={15,45,73,22};
    a.departure.observedMoves=a.departure.initialMoves;
    a.recordedDeparture=a.departure.initialMoves;
    a.recordedArrival=a.recordedDeparture;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Contradicted;}());
static_assert([]{auto a=taught();a.steps[1].game=G::HeartGold;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Unresolved;}());
static_assert([]{auto a=taught();a.nativeActionsCompleteVerified=false;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Unresolved;}());
constexpr B::Input duplicateTM(){
    auto a=base(H::Game::Ruby,G::Platinum);
    a.count=2;a.steps[0]={G::Platinum,20,A::Machine,92,0,false,6,700,true,true};
    a.steps[1]={G::Platinum,20,A::Machine,412,1,false,53,700,true,true};
    a.observedMoves={92,412,73,22};return a;
}
static_assert(B::evaluate<OriginalGba,Platinum>(duplicateTM()).status==S::Contradicted);
static_assert([]{auto a=duplicateTM();a.steps[1].itemReceipt=701;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Consistent;}());
constexpr B::Input hmRemoved(){
    auto a=base(H::Game::Sapphire,G::Platinum);
    auto& p=a.departure;
    p.initialMoves={15,45,73,22};p.count=1;
    p.steps[0]={H::Game::Sapphire,20,H::Action::DeleteMove,15,0,0,H::Machine::TM,0,H::Game::Unknown,0,true,true,0};
    p.observedMoves={45,73,22,0};a.recordedDeparture=p.observedMoves;
    a.recordedArrival=a.observedMoves=p.observedMoves;
    return a;
}
static_assert(B::evaluate<OriginalGba,Platinum>(hmRemoved()).status==S::Consistent);
static_assert([]{auto a=hmRemoved();a.departure.steps[0].methodReceiptVerified=false;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Unresolved;}());
static_assert([]{auto a=base(H::Game::Ruby,G::Diamond);a.observedLevel=21;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Unresolved;}());
static_assert([]{auto a=base(H::Game::Ruby,G::Platinum);a.observedLevel=21;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Contradicted;}());
// A Gen III HM removed by an independently witnessed Move Deleter may be
// re-taught from the real Gen IV HM item AFTER Pal Park. It cannot be
// treated as a Gen III HM illegally carried across the migration edge.
static_assert([]{auto a=hmRemoved();a.count=1;
    a.steps[0]={G::Platinum,20,A::Machine,15,3,true,1,0,true,true};
    a.observedMoves={45,73,22,15};
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Consistent;}());
// A real game visit at the same level does not cause fictional level-up
// prompts in either game. The unmodeled destination cannot verify NEW moves.
static_assert([]{auto a=base(H::Game::Ruby,G::Platinum);
    a.count=1;a.steps[0]={G::HeartGold,20,A::VisitGame,0,0,false,0,0,true,false};
    a.observedGame=G::HeartGold;
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Consistent;}());
static_assert([]{auto a=base(H::Game::Ruby,G::Platinum);
    a.count=2;a.steps[0]={G::HeartGold,20,A::VisitGame,0,0,false,0,0,true,false};
    a.steps[1]={G::HeartGold,20,A::Machine,92,0,false,6,100,true,true};
    return B::evaluate<OriginalGba,Platinum>(a).status==S::Unresolved;}());
int main() {
    uint32_t accepted=0,mutations=0,contradicted=0;
    for(auto origin:gba)for(auto destination:nds){
        const auto a=base(origin,destination);
        assert((B::evaluate<OriginalGba,Platinum>(a).status==S::Consistent));++accepted;
        auto b=a;b.samePokemonThroughTransferVerified=false;
        assert((B::evaluate<OriginalGba,Platinum>(b).status==S::Unresolved));
    }
    for(uint16_t move=1;move<=467;++move)for(uint8_t slot=0;slot<4;++slot){
        auto a=base(H::Game::Ruby,G::Platinum);
        a.count=1;
        a.steps[0]={G::Platinum,20,A::Machine,move,slot,false,53,100,true,true};
        if(move==412&&slot==0)a.observedMoves={412,45,73,22};
        const auto result=B::evaluate<OriginalGba,Platinum>(a);
        if(move==412&&slot==0){assert(result.status==S::Consistent);++accepted;}
        else {assert(result.status==S::Contradicted);++contradicted;}
        ++mutations;
    }
    assert(accepted==26&&mutations==1868&&contradicted==1867);
    std::cout<<"L3 Stage4 Pal Park 25 routes, 1868 four-slot machine mutations: PASS\n";
}
