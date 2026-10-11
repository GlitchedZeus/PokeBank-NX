#pragma once
// Evidence-only Gen III -> IV move-stage classification for detached PK4.
// A source-game's broad compatibility table is not a reconstruction of
// acquisition timing, parents, transfer routes or actual trade history.
#include "Integration/Gen3/Gen3LearnsetTable.h"
#include "Integration/Gen4/Gen4MoveCompatibility.h"
#include <array>
#include <cstdint>

namespace Legality::Gen34PalParkMoves {
using Source=PokeVault::Integration::Gen3::SourceGame;

struct Result {
    uint8_t storedMoves=0;
    uint8_t originalGbaCompatible=0;
    uint8_t otherGbaCompatible=0;
    uint8_t gen4StorageOnly=0;
    uint8_t gen4CompatibleOnly=0;
    uint8_t unmodeled=0;
    constexpr bool allAudited() const noexcept {
        return storedMoves!=0 && unmodeled==0;
    }
};

// A native Generation III PK3 cannot store move IDs above 354. An accepted
// native PK4 with such a move therefore requires post-migration acquisition
// in any legitimate Gen III-origin historical route. Do NOT infer which
// tutor, event, game, transfer or parent produced that move.
inline Result classify(Source original,uint16_t species,uint8_t form,
                       const std::array<uint16_t,4>& moves) noexcept {
    using namespace PokeVault::Integration;
    Result r{};
    for(uint16_t m:moves) {
        if(!m)continue;
        ++r.storedMoves;
        const bool g3Direct=m<=354 &&
            Gen3::Learnset::directlyLearnable(original,species,m);
        const bool g3Some=m<=354 &&
            Gen3::Learnset::learnableSomewhere(species,m);
        const bool g4Some=
            Gen4MoveCompatibility::anywhereInGen4(species,form,m);
        if(m>354)++r.gen4StorageOnly;
        if(g3Direct)++r.originalGbaCompatible;
        else if(g3Some)++r.otherGbaCompatible;
        else if(g4Some)++r.gen4CompatibleOnly;
        else ++r.unmodeled;
    }
    return r;
}
} // namespace Legality::Gen34PalParkMoves
