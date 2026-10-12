#include "Legality/Gen3EggPIDCreationHistory.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace H=Legality::Gen3EggPIDCreationHistory;
H::Input ordinary(const char* game,uint32_t pending,uint32_t collection,
                  uint16_t emeraldVBlank=0) {
    H::Input in{};
    in.game=game;
    in.originalUnhatchedEggIndependentlyVerified=true;
    in.ordinaryDaycarePairIndependentlyVerified=true;
    in.ordinaryDaycarePairEligible=true;
    in.originalPhysicalDepositOrderIndependentlyVerified=true;
    in.physicalSlots[0]={100,25,1,7,false,true,true};
    in.physicalSlots[1]={200,25,0,11,false,true,true};
    in.originalCreationStageIndependentlyVerified=true;
    in.originalCollectionStageIndependentlyVerified=true;
    in.originalParentStageIdentity=0x7777;
    in.originalCreationStageIdentity=0x7777;
    in.originalCollectionStageIdentity=0x7777;
    in.pendingLCRNGStateIndependentlyVerified=true;
    in.pendingLCRNGState=pending;
    in.collectionLCRNGStateIndependentlyVerified=true;
    in.collectionLCRNGState=collection;
    in.originalEmeraldVBlankCounterIndependentlyVerified=true;
    in.originalEmeraldVBlankCounter=emeraldVBlank;
    return in;
}
void setObserved(H::Input& in,uint32_t pid) {
    in.originalUnhatchedEggPID=pid;
}
int main() {
    // Fixed vectors hand-computed from pinned original Random() recurrence:
    // seed*1103515245+24691, high16 output. These synthetic stage vectors
    // are NOT authenticated archival retail eggs.
    struct Fixture {uint32_t pending,collection,pid,finalPending,finalCollection;};
    constexpr std::array<Fixture,4> rs{{
        {0,0,0x00000001u,0x00006073u,0x00006073u},
        {1,1,0x41C641C7u,0x41C6AEE0u,0x41C6AEE0u},
        {0x12345678u,0x89ABCDEFu,0xECBF0B72u,0x0B71C18Bu,0xECBFE136u},
        {0xFFFFFFFFu,0,0x0000BE3Bu,0xBE3A1206u,0x00006073u}
    }};
    unsigned checks=0;
    for(const char* game:{"ruby_gba","sapphire_gba","firered_gba","leafgreen_gba"}) {
        for(const auto& f:rs) {
            auto x=ordinary(game,f.pending,f.collection);
            setObserved(x,f.pid);
            const auto out=H::evaluate(x);
            assert(out.status==H::Status::ConditionallyMatchedOriginalEggPID);
            assert(out.finalPersonality==f.pid);
            assert(out.pendingPersonality==(f.pid&65535u));
            assert(out.finalPendingLCRNGState==f.finalPending);
            assert(out.finalCollectionLCRNGState==f.finalCollection);
            assert(out.pendingRNGDraws==1 && out.collectionRNGDraws==1);
            assert(!out.natureInheritanceActivated && out.originalNatureAttempts==0);
            // RS and FRLG do not use Emerald Everstone logic.
            x.physicalSlots[0].heldEverstoneAtCreation=true;
            assert(H::evaluate(x).status==H::Status::ConditionallyMatchedOriginalEggPID);
            x.originalEmeraldVBlankCounterIndependentlyVerified=false;
            assert(H::evaluate(x).status==H::Status::ConditionallyMatchedOriginalEggPID);
            x.originalUnhatchedEggPID^=1;
            assert(H::evaluate(x).status==H::Status::OriginalEggPIDContradicted);
            ++checks;
        }
    }
    struct Emerald {
        uint32_t seed;
        uint16_t vblank;
        uint32_t normalPid,everstonePid,everstoneEndRegular,everstoneEndRng2;
        uint16_t attempts,draws;
        bool activated;
    };
    constexpr std::array<Emerald,4> emerald{{
        {0,0,0x00000001u,0xE2CCAFC5u,0xAFC58AC9u,0xE2CCA5EEu,6,7,true},
        {1,1,0x41C641C7u,0x37B90C2Du,0x0C2D60E8u,0x37B99529u,8,9,true},
        {0x12345678u,0xCDEFu,0x34F00B72u,0xC2AF10E8u,0x10E8F964u,0xC2AF4E98u,11,12,true},
        {0xFFFFFFFFu,0,0x0000BE3Bu,0x000026DCu,0x26DBE101u,0x00006073u,1,2,false}
    }};
    for(const auto& f:emerald) {
        auto x=ordinary("emerald_gba",f.seed,0xFFFFFFFFu,f.vblank);
        setObserved(x,f.normalPid);
        const auto normal=H::evaluate(x);
        assert(normal.status==H::Status::ConditionallyMatchedOriginalEggPID);
        assert(normal.finalPersonality==f.normalPid);
        assert(normal.pendingPersonality==f.normalPid);
        assert(normal.pendingRNGDraws==1 && normal.collectionRNGDraws==0);
        assert(normal.originalNatureAttempts==1);
        assert(!normal.natureInheritanceActivated);
        // Original female holds Everstone at egg CREATION.
        x.physicalSlots[0].heldEverstoneAtCreation=true;
        setObserved(x,f.everstonePid);
        const auto nature=H::evaluate(x);
        assert(nature.status==H::Status::ConditionallyMatchedOriginalEggPID);
        assert(nature.finalPersonality==f.everstonePid);
        assert(nature.finalPendingLCRNGState==f.everstoneEndRegular);
        assert(nature.finalSecondRNGState==f.everstoneEndRng2);
        assert(nature.originalNatureAttempts==f.attempts);
        assert(nature.pendingRNGDraws==f.draws);
        assert(nature.natureInheritanceActivated==f.activated);
        assert(!nature.natureSearchExhausted);
        assert(nature.collectionRNGDraws==0);
        x.originalUnhatchedEggPID^=1;
        assert(H::evaluate(x).status==H::Status::OriginalEggPIDContradicted);
        ++checks;
    }
    // Exact retail coin threshold: high16 == 0x7FFE accepts, 0x7FFF
    // does not. These seed inversions are independently derived.
    {
        auto below=ordinary("emerald_gba",0xB36B61A1u,0,0);
        below.physicalSlots[0].heldEverstoneAtCreation=true;
        assert(H::evaluate(below).natureInheritanceActivated);
        auto at=ordinary("emerald_gba",0x9ED061A1u,0,0);
        at.physicalSlots[0].heldEverstoneAtCreation=true;
        assert(!H::evaluate(at).natureInheritanceActivated);
        auto above=ordinary("emerald_gba",0x8A3561A1u,0,0);
        above.physicalSlots[0].heldEverstoneAtCreation=true;
        assert(!H::evaluate(above).natureInheritanceActivated);
        checks+=3;
    }
    // The original donor for an ordinary Ditto pair is Ditto, even when
    // the female present in a physical slot currently holds Everstone.
    {
        auto x=ordinary("emerald_gba",0,0,0);
        x.physicalSlots[0].heldEverstoneAtCreation=true;
        x.physicalSlots[1].species=132;
        x.physicalSlots[1].gender=2;
        setObserved(x,1);
        assert(H::evaluate(x).status==H::Status::ConditionallyMatchedOriginalEggPID);
        assert(H::evaluate(x).originalEverstoneDonor==1);
        auto two=x;two.physicalSlots[0].species=132;
        assert(H::evaluate(two).status==H::Status::IncompatibleOriginalDaycarePair);
        checks+=2;
    }
    for(const char* game:{"ruby_gba","emerald_gba","firered_gba"}) {
        auto x=ordinary(game,0,0);
        x.originalUnhatchedEggIndependentlyVerified=false;
        assert(H::evaluate(x).status==H::Status::UnknownOriginalEgg);
        x=ordinary(game,0,0);x.ordinaryDaycarePairIndependentlyVerified=false;
        assert(H::evaluate(x).status==H::Status::UnknownOriginalParentPair);
        x=ordinary(game,0,0);x.originalPhysicalDepositOrderIndependentlyVerified=false;
        assert(H::evaluate(x).status==H::Status::UnknownOriginalParentPair);
        x=ordinary(game,0,0);x.ordinaryDaycarePairEligible=false;
        assert(H::evaluate(x).status==H::Status::IncompatibleOriginalDaycarePair);
        x=ordinary(game,0,0);x.physicalSlots[0].snapshotIdentity=0;
        assert(H::evaluate(x).status==H::Status::UnknownParentSnapshots);
        x=ordinary(game,0,0);x.physicalSlots[0].gender=3;
        assert(H::evaluate(x).status==H::Status::UnknownParentSnapshots);
        x=ordinary(game,0,0);x.physicalSlots[0].snapshotIdentity=200;
        assert(H::evaluate(x).status==H::Status::UnknownParentSnapshots);
        x=ordinary(game,0,0);x.originalCreationStageIndependentlyVerified=false;
        assert(H::evaluate(x).status==H::Status::UnknownOriginalStage);
        x=ordinary(game,0,0);x.originalCollectionStageIdentity=0x8888;
        assert(H::evaluate(x).status==H::Status::IncoherentOriginalStage);
        x=ordinary(game,0,0);x.pendingLCRNGStateIndependentlyVerified=false;
        assert(H::evaluate(x).status==H::Status::UnknownPendingRNG);
        if(H::family(game)==H::Family::Emerald) {
            x=ordinary(game,0,0);x.physicalSlots[0].originalHeldItemIndependentlyVerified=false;
            assert(H::evaluate(x).status==H::Status::UnknownCreationItems);
            x=ordinary(game,0,0);x.originalEmeraldVBlankCounterIndependentlyVerified=false;
            assert(H::evaluate(x).status==H::Status::UnknownEverstoneVBlankSeed);
        } else {
            x=ordinary(game,0,0);x.collectionLCRNGStateIndependentlyVerified=false;
            assert(H::evaluate(x).status==H::Status::UnknownCollectionRNG);
        }
        ++checks;
    }
    // Record identity may be source Gen III, not an unspecified later egg.
    auto outside=ordinary("emerald_gba",0,0);
    outside.game="platinum_nds";
    assert(H::evaluate(outside).status==H::Status::UnsupportedGame);
    outside.game="heartgold_nds";
    assert(H::evaluate(outside).status==H::Status::UnsupportedGame);
    // Bounded seeded phase: run real full source-stage conditional replay,
    // not infer a PID or any original parents from an unpaired PK3.
    for(const char* game:{"ruby_gba","sapphire_gba","emerald_gba",
                          "firered_gba","leafgreen_gba"}) {
        for(uint32_t seed=0;seed<4096;++seed) {
            auto x=ordinary(game,seed,seed^0x13572468u,
                            static_cast<uint16_t>(seed));
            auto out=H::evaluate(x);
            assert(out.status==H::Status::OriginalEggPIDContradicted ||
                   out.status==H::Status::ConditionallyMatchedOriginalEggPID);
            assert(out.pendingRNGDraws>=1 &&
                   out.pendingRNGDraws<=2402);
            assert(out.originalNatureAttempts<=2401);
            assert(out.collectionRNGDraws==(H::family(game)==H::Family::Emerald?0:1));
            setObserved(x,out.finalPersonality);
            const auto valid=H::evaluate(x);
            assert(valid.status==H::Status::ConditionallyMatchedOriginalEggPID);
            assert(valid.finalPersonality==out.finalPersonality);
            if(H::family(game)==H::Family::Emerald) {
                x.physicalSlots[0].heldEverstoneAtCreation=true;
                const auto branch=H::evaluate(x);
                assert(branch.originalNatureAttempts>=1 &&
                       branch.originalNatureAttempts<=2401);
                assert(branch.pendingRNGDraws<=2402);
                setObserved(x,branch.finalPersonality);
                assert(H::evaluate(x).status==H::Status::ConditionallyMatchedOriginalEggPID);
            }
            ++checks;
        }
    }
    std::cout<<"L2 B07 Gen III source-game egg PID/Everstone: "
             <<checks<<" seeded and fixed cases plus Unknown controls PASS\n";
}
