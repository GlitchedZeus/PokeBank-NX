// Original Method-1 PID/IV stage seeds 12 and 35 were independently
// replayed forward through the source LCRNG. Seed 12 yields an Emerald
// Hoenn-Safari-block grass slot; seed 35 yields a Surf Synchronize slot.
// These are conditional synthetic histories, not archived retail captures.
#include "Legality/Gen3MethodHOriginalStage.h"
#include <cassert>
#include <cstddef>
#include <iostream>
int main() {
    namespace H=Legality::Gen3MethodHOriginalStage;
    namespace G=Legality::Gen3Safari;
    namespace M=Legality::Gen3SafariMethodH;
    auto find=[&](uint16_t species,uint8_t level,uint32_t seed,
                  uint8_t method,H::Lead lead)->std::size_t{
        const uint8_t nature=static_cast<uint8_t>(
            M::Detail::sequentialPid(seed)%25u);
        for(std::size_t i=0;i<std::size(G::kGen3SafariEntries);++i){
            const auto& row=G::kGen3SafariEntries[i];
            if(row.game!=static_cast<uint8_t>(G::Game::Emerald) ||
               row.species!=species || row.method!=method ||
               !G::levelMatches(row,level))continue;
            auto hit=lead==H::Lead::NoLead ?
                M::Detail::matchCandidate(row,level,seed,nature,true) :
                M::Detail::matchEmeraldSynchronize(row,level,seed);
            if(hit.path!=M::Path::None)return i;
        }
        return std::size(G::kGen3SafariEntries);
    };
    H::Candidate c{};
    c.originalEmeraldSafariIndependentlyVerified=true;
    c.originalRowIndependentlyVerified=true;
    c.originalLeadIndependentlyVerified=true;
    c.originalPidIvStageIndependentlyVerified=true;
    c.originalLocation=G::kSafariLocationRSE;
    c.originalForm=0;
    c.originalSpecies=43;
    c.originalLevel=29;
    c.originalPid=0x091D154Cu;
    c.originalIVs={22,2,22,30,15,22};
    c.prePidSeed=12u;
    c.lead=H::Lead::NoLead;
    c.rowIndex=find(43,29,c.prePidSeed,0,c.lead);
    assert(c.rowIndex<std::size(G::kGen3SafariEntries));
    const auto h=H::evaluate(c);
    assert(h.status==H::Status::KnownConditionalEmeraldSafariHistory);
    assert(h.matchedPath==M::Path::HoennSafariBlock);
    auto sync=c;
    sync.originalSpecies=54;
    sync.originalLevel=31;
    sync.originalPid=0x85B9FE1Du;
    sync.originalIVs={2,29,24,20,4,15};
    sync.prePidSeed=35u;
    sync.lead=H::Lead::SynchronizeSuccess;
    sync.rowIndex=find(54,31,sync.prePidSeed,1,sync.lead);
    assert(sync.rowIndex<std::size(G::kGen3SafariEntries));
    assert(H::evaluate(sync).status==H::Status::KnownConditionalEmeraldSafariHistory);
    assert(H::evaluate(sync).matchedPath==M::Path::EmeraldSynchronize);
    auto negative=c;negative.originalSpecies=54;
    assert(H::evaluate(negative).status==H::Status::CandidateRowContradicted);
    negative=c;negative.prePidSeed=0x12345678u;
    assert(H::evaluate(negative).status==H::Status::CandidatePidIvContradicted);
    auto unknown=c;unknown.originalLeadIndependentlyVerified=false;
    assert(H::evaluate(unknown).status==H::Status::UnknownLead);
    unknown=c;unknown.originalPidIvStageIndependentlyVerified=false;
    assert(H::evaluate(unknown).status==H::Status::UnknownPidIvStage);
    unknown=c;unknown.originalRowIndependentlyVerified=false;
    assert(H::evaluate(unknown).status==H::Status::UnknownRow);
    unknown=c;unknown.originalEmeraldSafariIndependentlyVerified=false;
    assert(H::evaluate(unknown).status==H::Status::UnknownSource);
    unknown=c;unknown.originalIVs[0]=32;
    assert(H::evaluate(unknown).status==H::Status::UnknownPidIvStage);
    // Wrong original game row is NOT proof of illegal Emerald history.
    unknown=c;
    for(std::size_t i=0;i<std::size(G::kGen3SafariEntries);++i){
        if(G::kGen3SafariEntries[i].game==
           static_cast<uint8_t>(G::Game::FireRed)){unknown.rowIndex=i;break;}
    }
    assert(H::evaluate(unknown).status==H::Status::UnknownRow);
    std::cout<<"L1 E07 Emerald exact-row Method H and Sync: PASS\n";
}
