// Stage 3: end-to-end independent original RSE/FRLG source/slot/level/
// nature capture witness, not retail authenticity and never global Invalid.
#include "Legality/Gen3OriginalWildJointHistory.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
namespace J=Legality::Gen3OriginalWildJointHistory;
namespace W=Legality::Gen3OriginalWildSourceStage;
namespace R=Legality::Gen3OriginalWildRngWitness;
uint32_t step(uint32_t s) {
    return static_cast<uint32_t>((uint64_t(s)*1103515245ull+24691ull)&0xffffffffull);
}
uint8_t independentlyChosenSlot(W::Method method,W::Rod rod,uint32_t rng) {
    const uint8_t value=static_cast<uint8_t>((rng>>16)%100u);
    constexpr unsigned grass[]={20,40,50,60,70,80,85,90,94,98,99,100};
    constexpr unsigned water[]={60,90,95,99,100};
    constexpr unsigned old[]={70,100},good[]={60,80,100},super[]={40,80,95,99,100};
    if(method==W::Method::Grass)
        for(unsigned i=0;i<12;++i)if(value<grass[i])return i;
    if(method==W::Method::Surf || method==W::Method::RockSmash)
        for(unsigned i=0;i<5;++i)if(value<water[i])return i;
    if(method==W::Method::Fishing) {
        const unsigned* thresholds=rod==W::Rod::Old?old:rod==W::Rod::Good?good:super;
        const unsigned count=rod==W::Rod::Old?2:rod==W::Rod::Good?3:5;
        const unsigned offset=rod==W::Rod::Old?0:rod==W::Rod::Good?2:5;
        for(unsigned i=0;i<count;++i)
            if(value<thresholds[i])return static_cast<uint8_t>(offset+i);
    }
    return 255;
}
J::Candidate captured(std::size_t groupIndex,uint32_t iteration) {
    const auto& g=W::kGroups[groupIndex];
    J::Candidate c{};
    c.physicalCaptureIdentity=12345u;
    c.originalSaveIdentity=9876u;
    c.originalCaptureIdentityIndependentlyAttested=true;
    c.sourceAndRngBelongToSameCaptureIndependentlyAttested=true;
    c.source.originalGame=g.game;
    c.source.exactGroupIndex=groupIndex;
    c.source.originalMap=g.map;
    c.source.originalMethod=g.method;
    c.source.originalGameAttested=true;
    c.source.originalGroupAndSlotAttested=true;
    c.source.originalMapAndMethodAttested=true;
    c.source.originalSpeciesAttested=true;
    c.source.originalLevelAttested=true;
    c.source.originalRodAttested=true;
    c.stage.exactOriginalGame=g.game;
    c.stage.exactGroupIndex=groupIndex;
    c.stage.originalLead=R::Lead::NoLead;
    c.stage.rod=g.method==W::Method::Fishing?W::Rod::Super:W::Rod::Unknown;
    c.stage.preSlotSeed=iteration*74917u+static_cast<uint32_t>(groupIndex)*33u+1u;
    c.stage.preNatureSeed=iteration*19267u+static_cast<uint32_t>(groupIndex)*277u+71u;
    c.stage.originalGameAndGroupAttested=true;
    c.stage.originalSlotSeedAttested=true;
    c.stage.originalSlotAndLevelAttested=true;
    c.stage.originalLeadAttested=true;
    c.stage.originalNatureSeedAttested=true;
    c.stage.originalNatureAttested=true;
    c.stage.originalSafariContextKnown=true;
    c.stage.originalSafariPokeblockPathRuledOut=true;
    const auto first=step(c.stage.preSlotSeed);
    const uint8_t slot=independentlyChosenSlot(g.method,c.stage.rod,first);
    assert(slot<W::slotCount(g));
    c.stage.attestedSlot=slot;
    const auto row=W::decode(g,slot);
    c.stage.attestedLevel=static_cast<uint8_t>(
       row.minLevel+((step(first)>>16)%(row.maxLevel-row.minLevel+1)));
    c.stage.attestedNature=static_cast<uint8_t>((step(c.stage.preNatureSeed)>>16)%25u);
    c.source.physicalSlot=slot;
    c.source.originalLevel=c.stage.attestedLevel;
    c.source.originalSpecies=W::kSpecies[row.speciesIndex];
    c.source.originalRod=c.stage.rod;
    return c;
}
int main() {
    unsigned passed=0, local=0, unknown=0;
    static_assert(J::kMaxCandidates==32);
    for(std::size_t i=0;i<W::kGroups.size();++i) {
        for(uint32_t trial=0;trial<256;++trial) {
            auto c=captured(i,trial);
            auto result=J::evaluate(c);
            assert(result.status==J::Status::ConditionalSourceAndStageOnly);
            assert(result.source==W::Status::SourceTemplateCompatibleHistoryUnverified);
            assert(result.rng==R::Status::StageOnlyCompatibleHistoryUnverified);
            assert(result.physicalSlot==c.stage.attestedSlot);
            assert(result.originalLevel==c.stage.attestedLevel);
            assert(result.nature==c.stage.attestedNature);
            assert(!result.globalInvalidSupported && !result.globallyLegalSupported);
            passed+=7;
            auto bad=c;bad.source.originalSpecies=
                c.source.originalSpecies=="PIDGEY"?"RATTATA":"PIDGEY";
            assert(J::evaluate(bad).status==J::Status::CandidateOriginalSourceContradicted);
            ++local;
            bad=c;bad.stage.attestedNature=
                static_cast<uint8_t>((c.stage.attestedNature+1u)%25);
            assert(J::evaluate(bad).status==J::Status::CandidateOriginalRngContradicted);
            ++local;
            bad=c;bad.source.originalLevel=
                static_cast<uint8_t>(c.source.originalLevel==100?99:c.source.originalLevel+1u);
            assert(J::evaluate(bad).status==J::Status::UnknownConflictingWitness);
            ++unknown;
            bad=c;bad.stage.attestedSlot=static_cast<uint8_t>(
                (c.stage.attestedSlot+1u)%W::slotCount(W::kGroups[i]));
            assert(J::evaluate(bad).status==J::Status::UnknownConflictingWitness);
            ++unknown;
            bad=c;bad.stage.exactGroupIndex=(i+1u)%W::kGroups.size();
            assert(J::evaluate(bad).status==J::Status::UnknownConflictingWitness);
            ++unknown;
            bad=c;bad.sourceAndRngBelongToSameCaptureIndependentlyAttested=false;
            assert(J::evaluate(bad).status==J::Status::UnknownSameCaptureLink);
            ++unknown;
            bad=c;bad.physicalCaptureIdentity=0;
            assert(J::evaluate(bad).status==J::Status::UnknownOriginalRoot);
            ++unknown;
            bad=c;bad.originalSaveIdentity=0;
            assert(J::evaluate(bad).status==J::Status::UnknownOriginalRoot);
            ++unknown;
            bad=c;bad.stage.originalNatureSeedAttested=false;
            assert(J::evaluate(bad).status==J::Status::UnknownOriginalStage);
            ++unknown;
            bad=c;bad.source.originalSpecies="INVALID_SOURCE_SPECIES";
            assert(J::evaluate(bad).status==J::Status::UnknownOriginalSource);
            ++unknown;
            bad=c;bad.source.originalMethod=static_cast<W::Method>(255u);
            assert(J::evaluate(bad).status==J::Status::UnknownOriginalSource);
            ++unknown;
            if(c.source.originalMethod==W::Method::Fishing) {
                bad=c;bad.source.originalRod=static_cast<W::Rod>(255u);
                assert(J::evaluate(bad).status==J::Status::UnknownConflictingWitness);
                ++unknown;
            }
            J::Hypothesis alternatives[3]{};
            alternatives[0]={1,c};
            alternatives[1]={2,c};
            alternatives[1].candidate.source.originalSpecies=
                c.source.originalSpecies=="PIDGEY"?"RATTATA":"PIDGEY";
            alternatives[2]={3,c};
            alternatives[2].candidate.stage.originalLeadAttested=false;
            auto mix=J::evaluateUniverse(12345,true,alternatives);
            assert(mix.status==J::UniverseStatus::OneConditionalSameCaptureRoute);
            assert(mix.compatible==1 && mix.locallyContradicted==1 && mix.unresolved==1);
            assert(!mix.globalInvalidSupported);
            passed+=3;
            alternatives[0].candidate.source=alternatives[1].candidate.source;
            mix=J::evaluateUniverse(12345,true,alternatives);
            assert(mix.status==J::UniverseStatus::UnknownUnresolvedHistories);
            assert(mix.compatible==0 && mix.locallyContradicted==2 && mix.unresolved==1);
            passed+=2;
            alternatives[2].candidate.stage.originalLeadAttested=true;
            mix=J::evaluateUniverse(12345,true,alternatives);
            assert(mix.status==J::UniverseStatus::ProvidedRoutesLocallyContradictedOnly);
            assert(!mix.globalInvalidSupported && !mix.globallyLegalSupported);
            passed+=2;
            alternatives[2].independentlyIdentifiedRoute=1;
            assert(J::evaluateUniverse(12345,true,alternatives).status==
                   J::UniverseStatus::UnknownDuplicateCaptureWitness);
            ++unknown;
        }
    }
    auto first=captured(0,0);
    std::array<J::Hypothesis,33> over{};
    for(std::size_t i=0;i<over.size();++i)over[i]={i+1,first};
    assert(J::evaluateUniverse(12345,true,over).status==J::UniverseStatus::UnknownCapacity);
    assert(J::evaluateUniverse(0,true,std::span<const J::Hypothesis>()).status==
           J::UniverseStatus::UnknownOriginalRoot);
    assert(J::evaluateUniverse(12345,true,std::span<const J::Hypothesis>()).status==
           J::UniverseStatus::UnknownUnresolvedHistories);
    assert(passed>100000 && local>10000 && unknown>70000);
    std::cout<<"L1 Stage3 same-capture original GBA: passed="<<passed
             <<" local="<<local<<" unknown="<<unknown
             <<" (never global Invalid) PASS\n";
}
