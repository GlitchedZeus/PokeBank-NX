#pragma once
// Evidence-only Gen III -> IV move-stage classification for detached PK4.
// A source-game's broad compatibility table is not a reconstruction of
// acquisition timing, parents, transfer routes or actual trade history.
#include "Integration/Gen3/Gen3LearnsetTable.h"
#include "Integration/Gen4/Gen4MoveCompatibility.h"
#include "Legality/Gen4TransferEvidence.h"
#include "Legality/Gen34PalParkSpecies.h"
#include <array>
#include <cstdint>

namespace Legality::Gen34PalParkMoves {
using Source=PokeVault::Integration::Gen3::SourceGame;

// Positive move-stage provenance requires a structurally consistent Pal
// Park marker AND the stored Gen III Ball bytes. Merely finding the marker
// in one PK4 field cannot justify source/migration provenance when the
// other split fields contradict it.
constexpr bool verifiedTransferFields(
        uint16_t metDP,uint16_t metExtended,
        uint8_t ballDPPt,uint8_t ballHGSS,bool egg) noexcept {
    const auto e=Gen4Transfer::classifyStoredFields(
        3,metDP,metExtended,egg);
    return (e==Gen4Transfer::Evidence::PalParkDiamondPearlFields ||
            e==Gen4Transfer::Evidence::PalParkPtHgssFields) &&
           Gen4Transfer::validPalParkBallFields(e,ballDPPt,ballHGSS);
}

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
    const uint16_t gbaSpecies=Gen34PalParkSpecies::nearestGen3Ancestor(species);
    // A Gen IV-only species without any GBA-era ancestral source cannot
    // produce a positive move-history candidate from a Gen III origin.
    if(gbaSpecies==0)return r;
    for(uint16_t m:moves) {
        if(!m)continue;
        ++r.storedMoves;
        const bool g3Direct=m<=354 &&
            Gen3::Learnset::directlyLearnable(original,gbaSpecies,m);
        const bool g3Some=m<=354 &&
            Gen3::Learnset::learnableSomewhere(gbaSpecies,m);
        const bool g4Some=
            Gen4MoveCompatibility::anywhereInGen4(species,form,m);
        // Only IDs inside the native Gen IV move domain can serve as
        // positive *post-migration* stage evidence. A malformed PK4 id
        // >=468 is structurally impossible and belongs to the unmodeled
        // category, not to a legitimate Gen IV acquisition path.
        if(m>354 && m<=467)++r.gen4StorageOnly;
        if(g3Direct)++r.originalGbaCompatible;
        else if(g3Some)++r.otherGbaCompatible;
        else if(g4Some)++r.gen4CompatibleOnly;
        else ++r.unmodeled;
    }
    return r;
}
// Exact lower bound for how many independently audited source-game pools
// must cover all four current moves of a migrated Gen III-origin Pokemon.
// This is a COVERAGE bound over union tables, not a feasible tutor/parent/
// evolution/trade route. Any unmodeled move makes the bound unresolved.
enum class JointBound : uint8_t {
    Empty,
    OnePool,
    MultiplePools,
    Unresolved,
};

struct JointResult {
    JointBound bound=JointBound::Unresolved;
    uint8_t minimumPools=0;
    bool requiresBothEras=false;
};

inline uint8_t jointMask(uint16_t species,uint8_t form,uint16_t move) noexcept {
    if(move==0 || move>467)return 0;
    using namespace PokeVault::Integration;
    uint8_t mask=0;
    const uint16_t gbaSpecies=Gen34PalParkSpecies::nearestGen3Ancestor(species);
    if(gbaSpecies==0)return 0; // No legitimate Gen III Pal Park source.
    if(move<=354) {
        if(Gen3::Learnset::directlyLearnable(Gen3::SourceGame::RubyGBA,
                                           gbaSpecies,move))mask|=1u;
        if(Gen3::Learnset::directlyLearnable(Gen3::SourceGame::EmeraldGBA,
                                           gbaSpecies,move))mask|=2u;
        if(Gen3::Learnset::directlyLearnable(Gen3::SourceGame::FireRedGBA,
                                           gbaSpecies,move))mask|=4u;
    }
    if(Gen4MoveCompatibility::direct("diamond_nds",species,form,move))
        mask|=8u;
    if(Gen4MoveCompatibility::direct("platinum_nds",species,form,move))
        mask|=16u;
    if(Gen4MoveCompatibility::direct("heartgold_nds",species,form,move))
        mask|=32u;
    return mask;
}

inline JointResult classifyJoint(
        uint16_t species,uint8_t form,
        const std::array<uint16_t,4>& moves) noexcept {
    std::array<uint8_t,4> masks{};
    uint8_t used=0,minimum=5;
    bool gbaExclusive=false,ndsExclusive=false;
    for(uint16_t move:moves) {
        if(move==0)continue;
        const uint8_t mask=jointMask(species,form,move);
        if(mask==0)return {}; // Includes ID >467 and unsupported events.
        masks[used++]=mask;
        if((mask & 0x38u)==0)gbaExclusive=true;
        if((mask & 0x07u)==0)ndsExclusive=true;
    }
    if(used==0)return {JointBound::Empty,0,false};
    // Only 6 exact pinned families exist (RS, E, FRLG, DP, Pt, HGSS).
    // Exhaustive finite set cover is constant-time: 63 subsets x 4 slots.
    for(uint8_t subset=1;subset<=63;++subset) {
        bool covers=true;
        for(uint8_t i=0;i<used;++i) {
            if((subset & masks[i])==0){covers=false;break;}
        }
        if(!covers)continue;
        uint8_t count=0;
        for(uint8_t b=subset;b;b>>=1)
            count+=static_cast<uint8_t>(b&1u);
        if(count<minimum)minimum=count;
    }
    if(minimum==5)return {};
    return {
        minimum==1?JointBound::OnePool:JointBound::MultiplePools,
        minimum,
        gbaExclusive && ndsExclusive
    };
}

} // namespace Legality::Gen34PalParkMoves
