#pragma once
// L3 Gen I/II boundary: strict parser RECORD snapshots, never source receipts.
// A detached PK1/PK2 cannot authenticate its original cartridge, trainer
// choices, consumed TMs, egg parent, evolution or Time Capsule visit.
#include "Legality/Gen12CandidateMoveMultiStage.h"
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include <array>
#include <cstdint>
namespace Legality::Gen12CandidateMoveNativeSnapshot {
namespace H=Gen12CandidateMoveMultiStage;
struct Snapshot {
    bool structurallyUsable=false;
    bool isEgg=false;
    bool verifiedPhysicalSaveContext=false;
    H::Game game=H::Game::Unknown;
    std::uint16_t species=0;
    std::uint8_t level=0;
    std::array<std::uint16_t,4> moves{};
};
constexpr H::Game gameOf(PokeVault::Integration::Gen1::SourceGame game) noexcept {
    using G=PokeVault::Integration::Gen1::SourceGame;
    switch(game) {
    case G::Red:return H::Game::Red;
    case G::Blue:return H::Game::Blue;
    case G::Yellow:return H::Game::Yellow;
    }
    return H::Game::Unknown;
}
constexpr H::Game gameOf(PokeVault::Integration::Gen2::SourceGame game) noexcept {
    using G=PokeVault::Integration::Gen2::SourceGame;
    switch(game) {
    case G::Gold:return H::Game::Gold;
    case G::Silver:return H::Game::Silver;
    case G::Crystal:return H::Game::Crystal;
    }
    return H::Game::Unknown;
}
inline Snapshot fromVerifiedPK1(const PokeVault::Integration::Gen1::PokemonRecord& r,
                               PokeVault::Integration::Gen1::SourceGame game,
                               bool physicalSourceVerified) noexcept {
    Snapshot s{};
    s.verifiedPhysicalSaveContext=physicalSourceVerified;
    s.game=physicalSourceVerified?gameOf(game):H::Game::Unknown;
    s.species=r.species;s.level=r.level;
    for(std::uint8_t i=0;i<4;++i)s.moves[i]=r.moves[i];
    s.structurallyUsable=r.species>=1 && r.species<=151 &&
        r.level>=1 && r.level<=100 && H::validSlots(s.moves,165);
    return s;
}
inline Snapshot fromVerifiedPK2(const PokeVault::Integration::Gen2::PokemonRecord& r,
                               PokeVault::Integration::Gen2::SourceGame game,
                               bool physicalSourceVerified) noexcept {
    Snapshot s{};
    s.verifiedPhysicalSaveContext=physicalSourceVerified;
    s.game=physicalSourceVerified?gameOf(game):H::Game::Unknown;
    s.species=r.species;s.level=r.level;s.isEgg=r.isEgg;
    for(std::uint8_t i=0;i<4;++i)s.moves[i]=r.moves[i];
    s.structurallyUsable=!r.isEgg && r.species>=1 && r.species<=251 &&
        r.level>=1 && r.level<=100 && H::validSlots(s.moves,251);
    return s;
}
inline H::Path emptyWitnessSkeleton(std::uint32_t candidateId,
                                   const Snapshot& original,
                                   const Snapshot& observed) noexcept {
    H::Path path{};
    if(!candidateId||!original.structurallyUsable||!observed.structurallyUsable||
       !original.verifiedPhysicalSaveContext||
       !observed.verifiedPhysicalSaveContext||
       !H::generation(original.game)||!H::generation(observed.game))
        return path;
    path.candidateId=candidateId;
    path.initialSpecies=original.species;
    path.observedSpecies=observed.species;
    path.originalGame=original.game;path.observedGame=observed.game;
    path.initialLevel=original.level;path.observedLevel=observed.level;
    path.initialMoves=original.moves;path.observedMoves=observed.moves;
    // Crucial: even two individually validated snapshots are NOT a verified
    // same-Pokémon identity, acquisition timeline or transfer event.
    return path;
}
} // namespace Legality::Gen12CandidateMoveNativeSnapshot
