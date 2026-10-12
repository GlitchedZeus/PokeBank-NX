#include "Legality/Gen12CandidateMoveNativeSnapshot.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
namespace N=Legality::Gen12CandidateMoveNativeSnapshot;
namespace H=Legality::Gen12CandidateMoveMultiStage;
int main() {
    using namespace PokeVault::Integration;
    std::uint32_t checks=0;
    for(auto g:{Gen1::SourceGame::Red,Gen1::SourceGame::Blue,Gen1::SourceGame::Yellow}) {
        Gen1::PokemonRecord strict{};
        strict.species=1;strict.level=5;strict.moves={33,45,0,0};
        const auto originalMoves=strict.moves;
        const auto detached=N::fromVerifiedPK1(strict,g,false);
        assert(detached.structurallyUsable&&!detached.verifiedPhysicalSaveContext);
        assert(detached.game==H::Game::Unknown);
        assert(N::emptyWitnessSkeleton(100,detached,detached).candidateId==0);
        const auto retained=N::fromVerifiedPK1(strict,g,true);
        assert(retained.structurallyUsable&&retained.game!=H::Game::Unknown);
        const auto candidate=N::emptyWitnessSkeleton(100,retained,retained);
        assert(candidate.candidateId==100);
        assert(candidate.initialMoves==candidate.observedMoves);
        assert(!candidate.initialSnapshotAttested &&
               !candidate.sameIndividualAttested &&
               !candidate.everyInterveningActionAttested);
        assert(H::evaluate(candidate).status==H::Status::Unknown);
        assert(strict.moves==originalMoves);
        checks+=10;
    }
    for(auto g:{Gen2::SourceGame::Gold,Gen2::SourceGame::Silver,Gen2::SourceGame::Crystal}) {
        Gen2::PokemonRecord strict{};
        strict.species=1;strict.level=5;strict.moves={33,45,0,0};
        const auto originalMoves=strict.moves;
        const auto detached=N::fromVerifiedPK2(strict,g,false);
        assert(detached.structurallyUsable&&!detached.verifiedPhysicalSaveContext);
        assert(detached.game==H::Game::Unknown);
        const auto retained=N::fromVerifiedPK2(strict,g,true);
        const auto candidate=N::emptyWitnessSkeleton(101,retained,retained);
        assert(candidate.candidateId==101);
        assert(!candidate.sameIndividualAttested && !candidate.timeCapsulePolicyAttested);
        assert(H::evaluate(candidate).status==H::Status::Unknown);
        strict.isEgg=true;
        const auto egg=N::fromVerifiedPK2(strict,g,true);
        assert(egg.isEgg && !egg.structurallyUsable);
        assert(N::emptyWitnessSkeleton(101,retained,egg).candidateId==0);
        assert(strict.moves==originalMoves);
        checks+=9;
    }
    for(unsigned i=0;i<256;++i) {
        Gen1::PokemonRecord p1{};
        p1.species=1;p1.level=5;p1.moves={33,45,0,0};
        p1.moves[2]=static_cast<std::uint8_t>(i);
        const auto x=N::fromVerifiedPK1(p1,Gen1::SourceGame::Red,true);
        const bool expected=i==0||(i<=165&&i!=33&&i!=45);
        assert(x.structurallyUsable==expected);
        Gen2::PokemonRecord p2{};
        p2.species=1;p2.level=5;p2.moves={33,45,0,0};
        p2.moves[2]=static_cast<std::uint8_t>(i);
        const auto y=N::fromVerifiedPK2(p2,Gen2::SourceGame::Gold,true);
        assert(y.structurallyUsable==(i==0||(i<=251&&i!=33&&i!=45)));
        checks+=2;
    }
    std::cout<<"L3 native PK1/PK2 source-free candidate boundary: "<<checks
             <<" assertions PASS (no source or parent authenticity inferred)\n";
}
