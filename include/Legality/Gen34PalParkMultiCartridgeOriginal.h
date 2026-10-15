#pragma once
// L3 Stage 5: original source adapter for a candidate-only Pal Park crossing.
// Method tables validate possibilities; historical player receipts remain
// independently supplied and are never invented from PK3/PK4 fields.
#include "Legality/Gen34PalParkMultiCartridgeCore.h"
#include "Legality/Gen3GbaJointMethodHistory.h"
#include "Legality/Gen4PlatinumNativeMethodData.h"
#include "Legality/Gen4PlatinumPinnedMachines.h"
#include <array>
#include <span>

namespace Legality::Gen34PalParkMultiCartridgeOriginal {
namespace P=Gen34PalParkMultiCartridgeCore;
namespace H=Gen3GbaJointMethodHistoryCore;
namespace O=Gen3GbaJointMethodHistory;
namespace D=Gen4PlatinumNativeMethodData;
namespace M=Gen4PlatinumPinnedMachines;
struct PlatinumEvidence {
    std::array<P::Offer,128> offers{};
    std::array<P::Machine,12> machines{};
    std::array<std::uint16_t,32> tutors{};
    std::uint16_t offerCount=0;
    std::uint8_t machineCount=0,tutorCount=0;
    bool pinned=false;
    [[nodiscard]] constexpr P::Catalog catalog(std::uint16_t species) const noexcept {
        return {P::Game::Platinum,species,
            std::span<const P::Offer>(offers.data(),offerCount),
            std::span<const P::Machine>(machines.data(),machineCount),
            std::span<const std::uint16_t>(tutors.data(),tutorCount),
            pinned,pinned,pinned};
    }
};
[[nodiscard]] constexpr PlatinumEvidence originalPlatinum(std::uint16_t species) noexcept {
    PlatinumEvidence evidence{};
    // Index lookup: GCC 13 rejects a null test on the address of an
    // inline constexpr array element inside a constant expression.
    const auto index=D::rowIndex(species);
    if(index>=D::kRows.size())return evidence;
    const auto* row=&D::kRows[index];
    if(!row->offerCount||row->offerCount>evidence.offers.size()||
       row->tutorCount>evidence.tutors.size()||M::kPinned.size()>evidence.machines.size())
        return evidence;
    evidence.offerCount=row->offerCount;
    for(std::uint16_t i=0;i<row->offerCount;++i) {
        const auto packed=D::packedOffer(species,i);
        evidence.offers[i]={static_cast<std::uint8_t>(packed>>9),
                            static_cast<std::uint16_t>(packed&511u)};
    }
    for(const auto& machine:M::kPinned) {
        evidence.machines[evidence.machineCount++]={
            machine.hidden,machine.number,machine.move,
            M::compatible(species,machine.hidden,machine.number,machine.move)};
    }
    evidence.tutorCount=row->tutorCount;
    for(std::uint8_t i=0;i<row->tutorCount;++i)
        evidence.tutors[i]=D::kTutors[row->tutorStart+i];
    evidence.pinned=true;
    return evidence;
}
[[nodiscard]] constexpr P::Result evaluate(const P::Input& asserted) noexcept {
    P::Result unresolved{};
    if(!asserted.gba.phaseCount||
       asserted.gba.phaseCount>asserted.gba.phases.size())return unresolved;
    // Replace every caller catalog with an independently fingerprinted retail
    // source table. Source flags inside the caller's packet are not trusted.
    std::array<O::Evidence,5> canonical{};
    P::Input packet=asserted;
    for(std::uint8_t i=0;i<packet.gba.phaseCount;++i) {
        auto& p=packet.gba.phases[i];
        const auto& h=p.history;
        canonical[i]=O::originalEvidence(h.game,h.species);
        if(!canonical[i].pinned)return unresolved;
        p.catalog={h.game,h.species,
            std::span<const H::Offer>(canonical[i].offers.data(),canonical[i].count),
            std::span<const std::uint16_t>(
                Gen3GbaOriginalMachineData::kMachineMove.data(),
                Gen3GbaOriginalMachineData::kMachineMove.size()),
            canonical[i].machines,true,true,true,true};
    }
    // Only the independently pinned Platinum 12-species native source is
    // currently complete enough to certify a *post-arrival method*.
    // Unmodeled D/P/HG/SS methods remain Unknown in Core preflight; the
    // immediately witnessed, unchanged Pal Park arrival still has meaning.
    const auto platinum=originalPlatinum(packet.species);
    std::array<P::Catalog,1> catalogs{};
    std::uint8_t count=0;
    if(platinum.pinned)catalogs[count++]=platinum.catalog(packet.species);
    return P::evaluate(packet,std::span<const P::Catalog>(catalogs.data(),count));
}
static_assert(originalPlatinum(1).offerCount==14);
static_assert(originalPlatinum(386).offerCount==14);
static_assert(!originalPlatinum(252).pinned);
static_assert(originalPlatinum(1).machines[5].number==53);
static_assert(originalPlatinum(1).machines[5].move==412);
} // namespace Legality::Gen34PalParkMultiCartridgeOriginal
