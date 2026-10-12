#include "Legality/Gen4DiamondPearlEggPidHistory.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace DP=Legality::Gen4DiamondPearlEggPidHistory;
// Original Diamond/Pearl overlay assembly: ov05_021EC8A8,
// ov05_021EC944, ov05_Daycare_CreateEgg; original math_util.c
// LCRandom / PRandom. Synthetic source-stage witnesses, NOT retail eggs.
DP::Input base(std::span<const uint32_t> mt,const char* game="diamond_nds") {
    DP::Input x{};
    x.game=game;
    x.originalDaycarePairEligibilityVerified=true;
    x.eligibleOrdinaryDaycarePair=true;
    x.originalParentSnapshotsVerified=true;
    x.originalDaycareSlotOrderVerified=true;
    x.creationTimeItemsIndependentlyVerified=true;
    x.parents[0]={25,1,7,229,true}; // female, Everstone, nature=7
    x.parents[1]={25,0,11,0,true}; // male, different nature
    x.creationStageIndependentlyVerified=true;
    x.collectionStageIndependentlyVerified=true;
    x.parentStageToken=0x44;
    x.mtStageToken=0x44;x.collectionStageToken=0x44;
    x.creationLCRNGStateIndependentlyVerified=true;
    x.creationLCRNGState=0; // first 16-bit LCRNG draw == 0 (<0x7fff)
    x.consecutiveMTOutputsIndependentlyVerified=true;
    x.exactMTConsumptionIndependentlyVerified=true;
    x.mtOutputs=mt;
    x.originalParentLanguagesIndependentlyVerified=true;
    x.originalParentLanguages={2,2};
    x.originalCollectionTrainerIdIndependentlyVerified=true;
    x.originalCollectionTrainerId=0;
    x.originalUnhatchedEggPidIndependentlyVerified=true;
    x.originalUnhatchedEggPid=32;
    return x;
}
int main() {
    unsigned checks=0;
    constexpr std::array<uint32_t,3> trial{0,1,32};
    for(const char* game:{"diamond_nds","pearl_nds"}) {
        auto known=base(trial,game);
        const auto yes=DP::evaluate(known);
        assert(yes.status==DP::Status::ConditionalOriginalEggPIDMatched);
        assert(yes.pendingPid==32 && yes.originalFinalPid==32);
        assert(yes.mtConsumed==3 && yes.lcConsumed==1);
        assert(yes.everstoneNatureProcced && yes.everstoneDonor==0);
        assert(yes.finalLCRNGState==0x00006073u);
        auto mismatch=known;mismatch.originalUnhatchedEggPid=33;
        assert(DP::evaluate(mismatch).status==DP::Status::OriginalEggPIDContradicted);
        // Original cutover is strict: 0x7FFE inherits; 0x7FFF fails.
        auto below=known;below.creationLCRNGState=0xB36B61A1u;
        assert(DP::evaluate(below).status==DP::Status::ConditionalOriginalEggPIDMatched);
        auto at=known;at.creationLCRNGState=0x9ED061A1u;
        at.mtOutputs=std::span<const uint32_t>(trial.data()+2,1);
        assert(DP::evaluate(at).status==DP::Status::ConditionalOriginalEggPIDMatched);
        assert(!DP::evaluate(at).everstoneNatureProcced);
        auto noStone=known;noStone.parents[0].heldItem=0;
        noStone.mtOutputs=std::span<const uint32_t>(trial.data(),1);
        noStone.originalUnhatchedEggPid=0;
        assert(DP::evaluate(noStone).status==DP::Status::ConditionalOriginalEggPIDMatched);
        assert(DP::evaluate(noStone).lcConsumed==0);
        auto noNature=known;noNature.parents[0].pidIndependentlyVerified=false;
        assert(DP::evaluate(noNature).status==DP::Status::UnknownParentNature);
        // A two-Ditto branch exists in retail control flow, but two
        // Ditto cannot be an eligible ordinary daycare pair (BREED-003).
        // An inconsistent positive pair attestation cannot bypass this.
        constexpr std::array<uint32_t,1> single{32};
        auto doubleDitto=base(single,game);
        doubleDitto.parents[0]={132,2,7,0,true};
        doubleDitto.parents[1]={132,2,32,229,true};
        assert(DP::evaluate(doubleDitto).status==DP::Status::IncompatibleDaycarePair);
        auto noDonor=base(single,game);
        noDonor.parents[0].gender=0;
        assert(DP::evaluate(noDonor).status==DP::Status::IncompatibleDaycarePair);
        // No missing/contradictory provenance may become a global Invalid.
        auto noPair=known;noPair.originalDaycarePairEligibilityVerified=false;
        assert(DP::evaluate(noPair).status==DP::Status::UnknownEligibleDaycarePair);
        auto badPair=known;badPair.eligibleOrdinaryDaycarePair=false;
        assert(DP::evaluate(badPair).status==DP::Status::IncompatibleDaycarePair);
        auto badGender=known;badGender.parents[0].gender=3;
        assert(DP::evaluate(badGender).status==DP::Status::UnknownParentCreationSnapshots);
        auto emptySpecies=known;emptySpecies.parents[1].species=0;
        assert(DP::evaluate(emptySpecies).status==DP::Status::UnknownParentCreationSnapshots);
        auto noParents=known;noParents.originalParentSnapshotsVerified=false;
        assert(DP::evaluate(noParents).status==DP::Status::UnknownParentCreationSnapshots);
        auto noOrder=known;noOrder.originalDaycareSlotOrderVerified=false;
        assert(DP::evaluate(noOrder).status==DP::Status::UnknownPhysicalSlotOrder);
        auto noItems=known;noItems.creationTimeItemsIndependentlyVerified=false;
        assert(DP::evaluate(noItems).status==DP::Status::UnknownCreationHeldItems);
        auto noEvent=known;noEvent.collectionStageToken=0;
        assert(DP::evaluate(noEvent).status==DP::Status::UnknownSourceStage);
        auto wrongEvent=known;wrongEvent.collectionStageToken=1;
        assert(DP::evaluate(wrongEvent).status==DP::Status::IncoherentSourceStage);
        auto noRng=known;noRng.creationLCRNGStateIndependentlyVerified=false;
        assert(DP::evaluate(noRng).status==DP::Status::UnknownLCRNGState);
        auto noMt=known;noMt.consecutiveMTOutputsIndependentlyVerified=false;
        assert(DP::evaluate(noMt).status==DP::Status::UnknownMTTranscript);
        auto notExact=known;notExact.exactMTConsumptionIndependentlyVerified=false;
        assert(DP::evaluate(notExact).status==DP::Status::UnknownMTTranscript);
        auto incomplete=known;incomplete.mtOutputs=std::span<const uint32_t>(trial.data(),2);
        assert(DP::evaluate(incomplete).status==DP::Status::IncompleteMTTranscript);
        auto extra=known;
        constexpr std::array<uint32_t,4> withExtra{0,1,32,57};
        extra.mtOutputs=withExtra;
        assert(DP::evaluate(extra).status==DP::Status::MTConsumptionContradicted);
        auto noLang=known;noLang.originalParentLanguagesIndependentlyVerified=false;
        assert(DP::evaluate(noLang).status==DP::Status::UnknownCollectionLanguages);
        auto noTrainer=known;noTrainer.originalCollectionTrainerIdIndependentlyVerified=false;
        assert(DP::evaluate(noTrainer).status==DP::Status::UnknownCollectionTrainer);
        auto noEgg=known;noEgg.originalUnhatchedEggPidIndependentlyVerified=false;
        assert(DP::evaluate(noEgg).status==DP::Status::UnknownOriginalChildPID);
        checks+=21;
    }
    // Exhaustion: the original ARM loop consumes 2401 MT candidates,
    // then retains the last even when its nature is still wrong.
    constexpr auto rejects=[] {
        std::array<uint32_t,2401> a{};
        for(auto& x:a)x=1;
        return a;
    }();
    auto fallback=base(rejects);
    fallback.originalUnhatchedEggPid=1;
    auto exhausted=DP::evaluate(fallback);
    assert(exhausted.status==DP::Status::ConditionalOriginalEggPIDMatched);
    assert(exhausted.retailMTFallback && exhausted.mtConsumed==2401);
    auto shortened=fallback;shortened.mtOutputs=std::span<const uint32_t>(rejects.data(),2400);
    assert(DP::evaluate(shortened).status==DP::Status::IncompleteMTTranscript);
    // Collection changes ONLY the PID when original parent languages
    // differ. PRandom(x) = x*1812433253 + 1 (mod 2^32), up to 4.
    constexpr std::array<uint32_t,1> accepted{32};
    auto international=base(accepted);
    international.originalParentLanguages={1,2};
    international.originalUnhatchedEggPid=0x2CBD24FCu;
    const auto intl=DP::evaluate(international);
    assert(intl.status==DP::Status::ConditionalOriginalEggPIDMatched);
    assert(intl.internationalRerolls==4);
    assert(intl.pendingPid==32 && intl.originalFinalPid==0x2CBD24FCu);
    auto early=base(accepted);
    early.parents[0].heldItem=0;
    constexpr std::array<uint32_t,1> zero{0};
    early.mtOutputs=zero;early.originalUnhatchedEggPid=0;
    early.originalParentLanguages={1,2};
    assert(DP::evaluate(early).internationalRerolls==0);
    assert(DP::evaluate(early).status==DP::Status::ConditionalOriginalEggPIDMatched);
    // Original D/P international collection reroll witnesses. These
    // fixed 32-bit outputs and counts independently encode the source
    // PRandom recurrence and early-stop shiny condition; no unverified
    // original parent language is silently copied from the native child.
    struct LanguageWitness {
        uint32_t pending,originalTrainer,expected;
        uint8_t rerolls;
    };
    constexpr std::array<LanguageWitness,9> languageWitnesses{{
        {0u,0u,0u,0u},
        {0u,0x00000020u,0xDBFFE6DCu,4u},
        {1u,0u,1u,0u},
        {1u,0x00000020u,0xDE85D0CDu,4u},
        {32u,0u,0x2CBD24FCu,4u},
        {32u,0x00000020u,32u,0u},
        {0x12345678u,0x12345678u,0x12345678u,0u},
        {0x12345678u,0u,0x7E5085D4u,4u},
        {0xDEADBEEFu,0u,0xB0832CDBu,4u}
    }};
    unsigned languageChecks=0;
    for(const char* game:{"diamond_nds","pearl_nds"}) {
        for(const auto& witness:languageWitnesses) {
            const std::array<uint32_t,1> observedMT{witness.pending};
            auto candidate=base(observedMT,game);
            candidate.parents[0].heldItem=0;
            candidate.originalParentLanguages={1,2};
            candidate.originalCollectionTrainerId=witness.originalTrainer;
            candidate.originalUnhatchedEggPid=witness.expected;
            const auto result=DP::evaluate(candidate);
            assert(result.status==DP::Status::ConditionalOriginalEggPIDMatched);
            assert(result.pendingPid==witness.pending);
            assert(result.originalFinalPid==witness.expected);
            assert(result.internationalRerolls==witness.rerolls);
            // No Masuda/international reroll when original languages
            // agree, even for an original PID not shiny to this trainer.
            candidate.originalParentLanguages={2,2};
            candidate.originalUnhatchedEggPid=witness.pending;
            const auto domestic=DP::evaluate(candidate);
            assert(domestic.status==DP::Status::ConditionalOriginalEggPIDMatched);
            assert(domestic.originalFinalPid==witness.pending);
            assert(domestic.internationalRerolls==0);
            candidate.originalParentLanguagesIndependentlyVerified=false;
            assert(DP::evaluate(candidate).status==DP::Status::UnknownCollectionLanguages);
            candidate.originalParentLanguagesIndependentlyVerified=true;
            candidate.originalCollectionTrainerIdIndependentlyVerified=false;
            assert(DP::evaluate(candidate).status==DP::Status::UnknownCollectionTrainer);
            ++languageChecks;
        }
    }
    assert(languageChecks==18);
    auto wrongGame=base(accepted,"platinum_nds");
    assert(DP::evaluate(wrongGame).status==DP::Status::UnsupportedGame);
    std::cout<<"L2 B13 original Diamond/Pearl PID history: "<<checks+7
             <<" source-assembly controls PASS (conditional only)\n";
}
