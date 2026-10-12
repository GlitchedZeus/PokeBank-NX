#pragma once
// L3 M06 / M12: exact ordered level-up offers within ONE verified, unchanged
// GBA species/game stage. All 386 Emerald national species are drawn from the
// accepted pinned source table. Ruby/Sapphire and FR/LG are conservatively
// limited to nine independently transcribed source-verified species; an
// unmodeled species is UNKNOWN, not an impossible move history.
// Original source revisions: pokeruby@63a8cbf0016b351a4e68f7036fa0b77e23d2f2c1,
// pokeemerald@5eff78649e7170a877b961ef0b3da13b81a16038,
// pokefirered@c75f352304d529f6ba92d4f74b9cf8b5c3810788.
// L3 contains no Pokémon/save mutation and no negative global legality rule.
#include "Legality/Gen14CandidateMoveChronology.h"
#include "Legality/Gen3EmeraldLevelUpEvidence.h"
#include "Legality/Gen14CandidateMoveGbaStarterTables.h"
#include <array>
#include <cstdint>

namespace Legality::Gen3GbaVersionedLevelUpTimeline {
namespace C=Gen14CandidateMoveChronology;
using Game=C::Game;
enum class Decision : uint8_t { AutoLearn, ReplaceSlot, Decline, AlreadyKnown };
enum class Status : uint8_t {
    Unresolved, CandidateConsistent, CandidateContradicted, Unsupported
};
struct Step {
    uint8_t level=0;
    uint16_t offeredMove=0;
    Decision decision=Decision::Decline;
    uint8_t slot=0;
};
struct Input {
    Game game=Game::Unknown;
    uint16_t nationalSpecies=0;
    uint8_t initialLevel=0, observedLevel=0;
    std::array<uint16_t,4> initialMoves{}, observedMoves{};
    bool initialSnapshotVerified=false;
    bool sameSpeciesAndGameContinuityVerified=false;
    bool noOtherMoveChangesVerified=false;
    bool allPlayerOfferDecisionsVerified=false;
    std::array<Step,32> steps{};
    uint8_t stepCount=0;
};
struct Result {
    Status status=Status::Unresolved;
    std::array<uint16_t,4> rebuilt{};
    uint8_t offers=0, learned=0, declined=0, replaced=0, alreadyKnown=0;
};
struct Offer { uint8_t level; uint16_t move; };
// Each ordering reflects the exact source learnset, including same-level
// double offers. Do not deduplicate: retail offers depend on order.
inline constexpr std::array<Offer,11> kBulbasaur{{
    {1,33},{4,45},{7,73},{10,22},{15,77},{15,79},
    {20,75},{25,230},{32,74},{39,235},{46,76}
}};
inline constexpr std::array<Offer,10> kCharmanderRse{{
    {1,10},{1,45},{7,52},{13,108},{19,99},{25,184},
    {31,53},{37,163},{43,82},{49,83}
}};
inline constexpr std::array<Offer,10> kCharmanderFrlg{{
    {1,10},{1,45},{7,52},{13,232},{19,108},{25,184},
    {31,53},{37,163},{43,82},{49,83}
}};
constexpr bool supported(Game game,uint16_t species) noexcept {
    if(C::generation(game)!=3 || !species || species>386)return false;
    if(game==Game::Emerald)return true;
    return species>=1 && species<=9;
}
constexpr uint16_t offerCount(Game game,uint16_t species) noexcept {
    if(!supported(game,species))return 0;
    if(game==Game::Emerald)
        return static_cast<uint16_t>(Gen3EmeraldLevelUp::kEmeraldLevelOffsets[species+1]
                                     -Gen3EmeraldLevelUp::kEmeraldLevelOffsets[species]);
    switch(species) {
    case 1:return static_cast<uint16_t>(kBulbasaur.size());
    case 2:return static_cast<uint16_t>(Gen14CandidateMoveGbaStarterTables::kIvysaur.size());
    case 3:return static_cast<uint16_t>(Gen14CandidateMoveGbaStarterTables::kVenusaur.size());
    case 4:return static_cast<uint16_t>(kCharmanderRse.size());
    case 5:return static_cast<uint16_t>(Gen14CandidateMoveGbaStarterTables::kCharmeleonRse.size());
    case 6:return static_cast<uint16_t>((game==Game::FireRed || game==Game::LeafGreen)?
        Gen14CandidateMoveGbaStarterTables::kCharizardFrlg.size():
        Gen14CandidateMoveGbaStarterTables::kCharizardRse.size());
    case 7:return static_cast<uint16_t>(Gen14CandidateMoveGbaStarterTables::kSquirtle.size());
    case 8:return static_cast<uint16_t>(Gen14CandidateMoveGbaStarterTables::kWartortle.size());
    case 9:return static_cast<uint16_t>(Gen14CandidateMoveGbaStarterTables::kBlastoise.size());
    default:return 0;
    }
}
constexpr Offer offerAt(Game game,uint16_t species,uint16_t index) noexcept {
    if(index>=offerCount(game,species))return {};
    if(game==Game::Emerald) {
        const auto packed=Gen3EmeraldLevelUp::kEmeraldPackedLevelMoves[
            Gen3EmeraldLevelUp::kEmeraldLevelOffsets[species]+index];
        return {static_cast<uint8_t>(packed>>9),
                static_cast<uint16_t>(packed&511u)};
    }
    namespace S=Gen14CandidateMoveGbaStarterTables;
    const bool frlg=game==Game::FireRed || game==Game::LeafGreen;
    if(species==1)return kBulbasaur[index];
    if(species==4)return frlg?kCharmanderFrlg[index]:kCharmanderRse[index];
    S::Offer offer{};
    switch(species) {
    case 2:offer=S::kIvysaur[index];break;
    case 3:offer=S::kVenusaur[index];break;
    case 5:offer=frlg?S::kCharmeleonFrlg[index]:S::kCharmeleonRse[index];break;
    case 6:offer=frlg?S::kCharizardFrlg[index]:S::kCharizardRse[index];break;
    case 7:offer=S::kSquirtle[index];break;
    case 8:offer=S::kWartortle[index];break;
    case 9:offer=S::kBlastoise[index];break;
    default:return {};
    }
    return {offer.level,offer.move};
}
constexpr Result evaluate(const Input& in) noexcept {
    Result out{};
    if(C::generation(in.game)!=3 || !in.nationalSpecies ||
       in.nationalSpecies>386 || !in.initialLevel ||
       in.observedLevel<in.initialLevel || in.observedLevel>100) {
        out.status=Status::Unsupported;return out;
    }
    // These stages cannot be sourced from an isolated PK3 alone.
    if(!in.initialSnapshotVerified ||
       !in.sameSpeciesAndGameContinuityVerified ||
       !in.noOtherMoveChangesVerified ||
       !in.allPlayerOfferDecisionsVerified ||
       in.stepCount>in.steps.size() ||
       !C::structural(in.initialMoves,354) ||
       !C::structural(in.observedMoves,354) ||
       !supported(in.game,in.nationalSpecies))return out;
    out.rebuilt=in.initialMoves;
    const uint16_t count=offerCount(in.game,in.nationalSpecies);
    for(uint16_t i=0;i<count;++i) {
        const Offer offered=offerAt(in.game,in.nationalSpecies,i);
        if(offered.level<=in.initialLevel || offered.level>in.observedLevel)
            continue;
        // Missing a source-verified offer is UNKNOWN until the witness
        // genuinely provides all player decisions. Never accept a timeline
        // solely because its final slot values happen to match.
        if(out.offers>=in.stepCount || out.offers>=in.steps.size())return out;
        const Step step=in.steps[out.offers++];
        if(step.level!=offered.level || step.offeredMove!=offered.move) {
            out.status=Status::CandidateContradicted;return out;
        }
        const bool already=C::contains(out.rebuilt,offered.move);
        const uint8_t open=C::freeSlot(out.rebuilt);
        switch(step.decision) {
        case Decision::AlreadyKnown:
            if(!already) {out.status=Status::CandidateContradicted;return out;}
            ++out.alreadyKnown;break;
        case Decision::AutoLearn:
            if(already || open>=4 || step.slot!=open) {
                out.status=Status::CandidateContradicted;return out;
            }
            out.rebuilt[open]=offered.move;++out.learned;break;
        case Decision::ReplaceSlot:
            if(already || open<4 || step.slot>=4) {
                out.status=Status::CandidateContradicted;return out;
            }
            out.rebuilt[step.slot]=offered.move;
            ++out.learned;++out.replaced;break;
        case Decision::Decline:
            // Retail auto-learns into empty slots, so Decline only applies
            // after all four slots are filled and the offered move is new.
            if(already || open<4) {
                out.status=Status::CandidateContradicted;return out;
            }
            ++out.declined;break;
        }
    }
    if(out.offers!=in.stepCount) {
        out.status=Status::CandidateContradicted;return out;
    }
    out.status=out.rebuilt==in.observedMoves?
        Status::CandidateConsistent:Status::CandidateContradicted;
    return out;
}
// At compile time, verify that the accepted full Emerald generated table
// and the independently source-pinned RS starter rows really agree for all
// nine evolved/original forms. The retail FRLG variant is intentionally NOT
// asserted equal for Charmander, Charmeleon and Charizard.
constexpr bool starterFamilySourceParity() noexcept {
    for(uint16_t species=1;species<=9;++species) {
        const auto count=offerCount(Game::Ruby,species);
        if(!count || count!=offerCount(Game::Sapphire,species) ||
           count!=offerCount(Game::Emerald,species))return false;
        for(uint16_t index=0;index<count;++index) {
            const Offer rs=offerAt(Game::Ruby,species,index);
            const Offer sap=offerAt(Game::Sapphire,species,index);
            const Offer em=offerAt(Game::Emerald,species,index);
            if(rs.level!=sap.level || rs.move!=sap.move ||
               rs.level!=em.level || rs.move!=em.move)return false;
        }
        const auto frCount=offerCount(Game::FireRed,species);
        if(frCount!=offerCount(Game::LeafGreen,species))return false;
        for(uint16_t index=0;index<frCount;++index) {
            const Offer fr=offerAt(Game::FireRed,species,index);
            const Offer lg=offerAt(Game::LeafGreen,species,index);
            if(fr.level!=lg.level || fr.move!=lg.move)return false;
            if(species!=4 && species!=5 && species!=6) {
                const Offer rs=offerAt(Game::Ruby,species,index);
                if(fr.level!=rs.level || fr.move!=rs.move)return false;
            }
        }
    }
    return true;
}
static_assert(starterFamilySourceParity());
static_assert(offerCount(Game::Emerald,1)==11);
static_assert(offerAt(Game::Ruby,4,3).move==108);
static_assert(offerAt(Game::FireRed,4,3).move==232);
static_assert(offerAt(Game::LeafGreen,4,3).level==13);
static_assert(offerCount(Game::Ruby,2)==13);
static_assert(offerCount(Game::LeafGreen,6)==14);
static_assert(offerCount(Game::Emerald,6)==13);
static_assert(offerAt(Game::FireRed,6,0).move==257); // FRLG Heat Wave
static_assert(offerAt(Game::Emerald,6,0).move==10);  // RSE Scratch
static_assert(offerAt(Game::Sapphire,8,12).move==56); // Hydro Pump
static_assert(offerAt(Game::FireRed,9,13).level==68);
static_assert(offerAt(Game::Ruby,5,4).move==108); // Smokescreen
static_assert(offerAt(Game::LeafGreen,5,4).move==232); // Metal Claw
static_assert(!supported(Game::Ruby,133));
static_assert(supported(Game::Emerald,386));
} // namespace Legality::Gen3GbaVersionedLevelUpTimeline
