// L1 Stage 3: four distinct original GBA handheld PID/IV generation
// families replayed FORWARD and joined to the same physical source/slot/
// level/nature. All seeds and physical witnesses here are synthetic tests,
// not original-retail capture attestations.
#include "Legality/Gen3OriginalWildPidIvStage.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace H=Legality::Gen3OriginalWildPidIvStage;
namespace J=Legality::Gen3OriginalWildJointHistory;
namespace W=Legality::Gen3OriginalWildSourceStage;
namespace P=Legality::Gen3PidIv;
namespace R=Legality::Gen3OriginalWildRngWitness;

uint32_t independentStep(uint32_t s) {
    return static_cast<uint32_t>((uint64_t(s)*1103515245ull+24691ull)&0xffffffffull);
}
H::Replay independentPidIv(uint32_t seed,P::Method method) {
    uint32_t s=independentStep(seed);
    const uint32_t lo=s>>16;
    if(method==P::Method::Method3)s=independentStep(s);
    s=independentStep(s);
    const uint32_t pid=lo|((s>>16)<<16);
    if(method==P::Method::Method2)s=independentStep(s);
    s=independentStep(s);
    const uint16_t a=static_cast<uint16_t>((s>>16)&0x7fffu);
    if(method==P::Method::Method4)s=independentStep(s);
    s=independentStep(s);
    const uint16_t b=static_cast<uint16_t>((s>>16)&0x7fffu);
    return {pid,{static_cast<uint8_t>(a&31u),
                 static_cast<uint8_t>((a>>5)&31u),
                 static_cast<uint8_t>((a>>10)&31u),
                 static_cast<uint8_t>(b&31u),
                 static_cast<uint8_t>((b>>5)&31u),
                 static_cast<uint8_t>((b>>10)&31u)}};
}
uint8_t slot(W::Method method,W::Rod rod,uint32_t seed) {
    const uint16_t roll=static_cast<uint16_t>((independentStep(seed)>>16)%100);
    constexpr unsigned land[]={20,40,50,60,70,80,85,90,94,98,99,100};
    constexpr unsigned water[]={60,90,95,99,100};
    constexpr unsigned old[]={70,100},good[]={60,80,100},super[]={40,80,95,99,100};
    const unsigned *steps=land;unsigned count=12,offset=0;
    if(method==W::Method::Surf || method==W::Method::RockSmash){steps=water;count=5;}
    if(method==W::Method::Fishing) {
        if(rod==W::Rod::Old){steps=old;count=2;}
        else if(rod==W::Rod::Good){steps=good;count=3;offset=2;}
        else {steps=super;count=5;offset=5;}
    }
    for(unsigned i=0;i<count;++i)if(roll<steps[i])return static_cast<uint8_t>(offset+i);
    return 255;
}
H::Input make(std::size_t group,std::uint32_t iteration,P::Method method) {
    const auto& g=W::kGroups[group];
    H::Input x{};
    auto& c=x.sourceAndRng;
    c.physicalCaptureIdentity=80001;
    c.originalSaveIdentity=80002;
    c.originalCaptureIdentityIndependentlyAttested=true;
    c.sourceAndRngBelongToSameCaptureIndependentlyAttested=true;
    c.source.originalGame=g.game;
    c.source.exactGroupIndex=group;
    c.source.originalMap=g.map;
    c.source.originalMethod=g.method;
    c.source.originalRod=g.method==W::Method::Fishing?W::Rod::Super:W::Rod::Unknown;
    c.source.originalGameAttested=c.source.originalGroupAndSlotAttested=true;
    c.source.originalMapAndMethodAttested=c.source.originalSpeciesAttested=true;
    c.source.originalLevelAttested=c.source.originalRodAttested=true;
    c.stage.exactOriginalGame=g.game;
    c.stage.exactGroupIndex=group;
    c.stage.rod=c.source.originalRod;
    c.stage.originalLead=R::Lead::NoLead;
    c.stage.preSlotSeed=iteration*10591u+static_cast<uint32_t>(group)*83u+9u;
    c.stage.originalGameAndGroupAttested=c.stage.originalSlotSeedAttested=true;
    c.stage.originalSlotAndLevelAttested=c.stage.originalLeadAttested=true;
    c.stage.originalNatureSeedAttested=c.stage.originalNatureAttested=true;
    c.stage.originalSafariContextKnown=c.stage.originalSafariPokeblockPathRuledOut=true;
    c.stage.attestedSlot=slot(g.method,c.stage.rod,c.stage.preSlotSeed);
    assert(c.stage.attestedSlot<W::slotCount(g));
    const auto row=W::decode(g,c.stage.attestedSlot);
    c.stage.attestedLevel=static_cast<uint8_t>(
        row.minLevel+((independentStep(independentStep(c.stage.preSlotSeed))>>16)%
                        (row.maxLevel-row.minLevel+1)));
    c.source.physicalSlot=c.stage.attestedSlot;
    c.source.originalLevel=c.stage.attestedLevel;
    c.source.originalSpecies=W::kSpecies[row.speciesIndex];
    x.frameCaptureIdentity=c.physicalCaptureIdentity;
    x.frameOriginalSaveIdentity=c.originalSaveIdentity;
    x.originalPrePidSeed=iteration*27749u+static_cast<uint32_t>(group)*271u+113u;
    x.originalPidIvMethod=method;
    const auto generated=independentPidIv(x.originalPrePidSeed,method);
    x.originalPid=generated.pid;
    x.originalIvs=generated.ivs;
    x.originalFrameIdentityIndependentlyAttested=true;
    x.originalPidIvSnapshotIndependentlyAttested=true;
    x.originalMethodFamilyIndependentlyAttested=true;
    x.independentOriginalPrePidSeedAttested=true;
    x.sameOriginalCaptureIndependentlyAttested=true;
    const uint8_t desired=static_cast<uint8_t>(x.originalPid%25u);
    // Independently supply an earlier original nature frame whose roll is
    // consistent with the already fixed, independently replayed PID.
    bool found=false;
    for(uint32_t seed=0;seed<4096;++seed) {
        if(((independentStep(seed)>>16)%25u)==desired) {
            c.stage.preNatureSeed=seed;c.stage.attestedNature=desired;
            found=true;break;
        }
    }
    assert(found);
    return x;
}
int main() {
    unsigned positives=0,local=0,unknown=0;
    for(std::size_t group=0;group<W::kGroups.size();++group) {
        for(uint32_t iteration=0;iteration<64;++iteration) {
            for(auto method:{P::Method::Method1,P::Method::Method2,
                             P::Method::Method3,P::Method::Method4}) {
                auto x=make(group,iteration,method);
                assert(H::ordinary(method));
                assert(H::replay(x.originalPrePidSeed,method).pid==x.originalPid);
                assert(H::replay(x.originalPrePidSeed,method).ivs==x.originalIvs);
                const auto original=x;
                const auto verified=H::evaluate(x);
                assert(verified.status==H::Status::ConditionalOriginalSourceRngPidIvStagesOnly);
                assert(verified.method==method && verified.frameSeed==x.originalPrePidSeed);
                assert(!verified.globalInvalidSupported && !verified.globallyLegalSupported);
                assert(x.originalIvs==original.originalIvs && x.originalPid==original.originalPid);
                positives+=7;
                auto bad=x;bad.originalPid^=1u;
                assert(H::evaluate(bad).status==H::Status::CandidateOriginalPidIvContradicted);
                ++local;
                bad=x;bad.originalIvs[2]^=1u;
                assert(H::evaluate(bad).status==H::Status::CandidateOriginalPidIvContradicted);
                ++local;
                bad=x;
                bad.sourceAndRng.stage.attestedNature=
                    static_cast<uint8_t>((bad.sourceAndRng.stage.attestedNature+1u)%25u);
                // New claimed nature needs its own coherent original frame.
                for(uint32_t seed=0;seed<4096;++seed) {
                    if(((independentStep(seed)>>16)%25u)==
                       bad.sourceAndRng.stage.attestedNature) {
                        bad.sourceAndRng.stage.preNatureSeed=seed;break;
                    }
                }
                assert(H::evaluate(bad).status==H::Status::CandidateOriginalNatureContradicted);
                ++local;
                bad=x;bad.sourceAndRng.source.originalSpecies=
                    x.sourceAndRng.source.originalSpecies=="PIDGEY"?"RATTATA":"PIDGEY";
                assert(H::evaluate(bad).status==H::Status::CandidateOriginalWildStageContradicted);
                ++local;
                bad=x;bad.sameOriginalCaptureIndependentlyAttested=false;
                assert(H::evaluate(bad).status==H::Status::UnknownDifferentCapturedCreature);
                ++unknown;
                bad=x;bad.frameCaptureIdentity^=1u;
                assert(H::evaluate(bad).status==H::Status::UnknownDifferentCapturedCreature);
                ++unknown;
                bad=x;bad.independentOriginalPrePidSeedAttested=false;
                assert(H::evaluate(bad).status==H::Status::UnknownIndependentPidIvFrame);
                ++unknown;
                bad=x;bad.originalIvs[0]=255u;
                assert(H::evaluate(bad).status==H::Status::UnknownMalformedIvs);
                ++unknown;
                bad=x;bad.originalPidIvMethod=P::Method::Method1Roamer;
                assert(H::evaluate(bad).status==H::Status::UnknownOriginalMethod);
                ++unknown;
                bad=x;bad.sourceAndRng.stage.originalNatureSeedAttested=false;
                assert(H::evaluate(bad).status==H::Status::UnknownOriginalWildStages);
                ++unknown;
            }
        }
    }
    assert(positives==33u*64u*4u*7u);
    assert(local==33u*64u*4u*4u);
    assert(unknown==33u*64u*4u*6u);
    std::cout<<"L1 Stage3 GBA Method1-4 original same-capture native frames: "
             <<positives<<" positive controls, "<<local<<" local mismatches, "
             <<unknown<<" Unknown controls PASS\n";
}
