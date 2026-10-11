#include "Legality/Gen3EmeraldPalParkJointPath.h"
#include <cassert>
#include <iostream>
namespace J=Legality::Gen3EmeraldPalParkJointPath;
namespace E=Legality::Gen3EmeraldLevelUpTimeline;
void runCases() {
    J::Input in{};
    auto& e=in.learning;
    e.game="emerald_gba";e.species=1;
    e.encounterLevel=5;e.observedLevel=32;
    e.initialSnapshotVerified=true;e.sameSpeciesVerified=true;
    e.sameEmeraldGameThroughoutVerified=true;
    e.noOtherMoveChangesVerified=true;e.allOffersAndDecisionsVerified=true;
    e.initialMoves={33,45,0,0};e.observedMoves={77,79,75,22};
    e.stepCount=7;
    e.steps[0]={7,73,E::Decision::AutoLearn,2};
    e.steps[1]={10,22,E::Decision::AutoLearn,3};
    e.steps[2]={15,77,E::Decision::ReplaceSlot,0};
    e.steps[3]={15,79,E::Decision::ReplaceSlot,1};
    e.steps[4]={20,75,E::Decision::ReplaceSlot,2};
    e.steps[5]={25,230,E::Decision::Decline,0};
    e.steps[6]={32,74,E::Decision::Decline,0};
    auto& p=in.migration;
    p.sourceGame="emerald_gba";p.destinationGame="platinum_nds";
    p.species=1;p.independentlyVerifiedPreTransfer=true;
    p.independentlyVerifiedImmediateArrival=true;
    p.unchangedSpeciesThroughArrivalVerified=true;
    p.outboundGbaMoves={77,79,75,22};
    p.immediateNdsMoves={77,79,75,22};
    in.sameOriginalSourceSnapshotVerified=true;
    in.originalLearningStageToken=0xAF35A4u;
    in.outboundGbaStageToken=0xAF35A4u;
    const auto positive=J::evaluate(in);
    assert(positive.status==J::Status::KnownConditionalCombinedPath);
    assert(positive.learning.offers==7);
    assert(positive.migration.retainedSlots==4);
    auto unknownShared=in;unknownShared.sameOriginalSourceSnapshotVerified=false;
    assert(J::evaluate(unknownShared).status==J::Status::UnknownSharedSource);
    auto differentStage=in;differentStage.outboundGbaStageToken=41;
    assert(J::evaluate(differentStage).status==J::Status::IncoherentStageIdentity);
    auto differentGame=in;differentGame.migration.sourceGame="firered_gba";
    assert(J::evaluate(differentGame).status==J::Status::IncoherentGameOrSpecies);
    auto differentSpecies=in;differentSpecies.migration.species=2;
    assert(J::evaluate(differentSpecies).status==J::Status::IncoherentGameOrSpecies);
    auto changedGbaMoves=in;changedGbaMoves.migration.outboundGbaMoves={77,79,22,75};
    assert(J::evaluate(changedGbaMoves).status==J::Status::IncoherentStageIdentity);
    auto changedArrival=in;changedArrival.migration.immediateNdsMoves={77,79,22,75};
    assert(J::evaluate(changedArrival).status==J::Status::TransferContradicted);
    auto unknownArrival=in;unknownArrival.migration.independentlyVerifiedImmediateArrival=false;
    assert(J::evaluate(unknownArrival).status==J::Status::UnresolvedTransfer);
    auto unknownTrainer=in;unknownTrainer.learning.noOtherMoveChangesVerified=false;
    assert(J::evaluate(unknownTrainer).status==J::Status::UnresolvedEmeraldTraining);
    auto impossibleTrainer=in;impossibleTrainer.learning.steps[3].offeredMove=76;
    assert(J::evaluate(impossibleTrainer).status==J::Status::EmeraldTrainingContradicted);
}
int main(){runCases();std::cout<<"Same-source Emerald training -> Pal Park continuity: PASS\n";}
