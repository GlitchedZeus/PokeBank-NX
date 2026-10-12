#pragma once
// L4 V09 — bounded original Ranger Manaphy gift egg candidate history.
// Pinned application: Gen4RangerManaphy.h, Gen4HatchLocationEvidence.h,
// Pokemon4ReadOnly; source helper itself traces PKHeX Method-1/ARNG behavior.
// A matching native egg is NOT proof of real Ranger distribution receipt.
// Any unresolved recipient/origin history stays Unknown, NEVER global Invalid.
#include "Legality/Gen4RangerManaphy.h"
#include "Legality/Gen4HatchLocationEvidence.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include <array>
#include <cstdint>

namespace Legality::Gen4EventRangerGiftHistory {
namespace Ranger=Gen4RangerManaphy;
namespace Hatch=Gen4Hatch;
enum class Status : uint8_t {
    UnavailableNative, UnknownOriginalGift,
    UnknownSourceGame, UnknownHatchGame, UnknownRecipientHistory,
    CandidateStructuralMismatch, CandidateHatchLocationMismatch,
    CandidatePidIvMismatch, ConditionalGiftCandidate
};
struct SourceEvidence {
    // Must be supported independently for the candidate to be classed as
    // one conditional origin witness. No recovered source receipt in PK4.
    bool rangerGiftOriginIndependentlyAttested=false;
    bool originalRecipientTrainerKnown=false;
    uint16_t originalRecipientTid=0;
    uint16_t originalRecipientSid=0;
    bool hatchGameHistoryKnown=false;
    uint8_t hatchGameVersion=0;
};
struct Input {
    Ranger::Candidate observed{};
    uint32_t pid=0;
    std::array<uint8_t,6> ivs{};
    uint8_t originVersion=0;
    uint16_t currentTid=0;
    uint16_t currentSid=0;
};
struct Result {
    Status status=Status::UnavailableNative;
    Ranger::PidResult rng{};
    Hatch::HatchResult hatch=Hatch::HatchResult::UnknownOrigin;
    bool candidateStructurallyMatched=false;
};
constexpr Result evaluate(bool nativeAvailable,const Input& in,
                          const SourceEvidence& source={}) noexcept {
    Result out{};
    if(!nativeAvailable)return out;
    if(!Ranger::matches(in.observed)) {
        out.status=Status::CandidateStructuralMismatch;return out;
    }
    out.candidateStructurallyMatched=true;
    if(!source.rangerGiftOriginIndependentlyAttested) {
        out.status=Status::UnknownOriginalGift;return out;
    }
    // Ranger's original egg gift was initially intended for D/P and later
    // Platinum; avoid assuming an HGSS-origin Ranger redemption mechanism
    // merely from current storage. Other unknown historical routes stay Unknown.
    if(in.originVersion!=10 && in.originVersion!=11 && in.originVersion!=12) {
        out.status=Status::UnknownSourceGame;return out;
    }
    if(!in.observed.isEgg) {
        if(!source.hatchGameHistoryKnown) {
            out.status=Status::UnknownHatchGame;return out;
        }
        out.hatch=Hatch::classifyHatchedEgg(
            source.hatchGameVersion,in.observed.eggLocation,in.observed.metLocation);
        // Direct-origin Ranger eggs use location 3001, not a normal
        // breeding egg. The hatch location nevertheless must be compatible
        // with the separately attested hatching game's map.
        if(in.observed.eggLocation==Ranger::LocationRanger4) {
            if(!Hatch::maskForOriginVersion(source.hatchGameVersion)) {
                out.status=Status::UnknownHatchGame;return out;
            }
            if(!Hatch::isValidForOrigin(in.observed.metLocation,
                                      source.hatchGameVersion)) {
                out.status=Status::CandidateHatchLocationMismatch;return out;
            }
            // Ordinary-egg check sees Ranger's special origin 3001 as
            // outside its bounded table; replace that intermediate
            // classification after the Ranger-specific proof succeeds.
            out.hatch=Hatch::HatchResult::ValidForOrigin;
        } else if(out.hatch==Hatch::HatchResult::UnknownOrigin) {
            out.status=Status::UnknownHatchGame;return out;
        } else if(out.hatch==Hatch::HatchResult::InvalidKnownOrigin) {
            out.status=Status::CandidateHatchLocationMismatch;return out;
        }
    }
    // For a traded Ranger gift, a detached native PK4's trainer ID is not
    // necessarily the original receiving trainer. An arbitrary visible
    // TID/SID must never be assumed to be the original anti-shiny context.
    if(!source.originalRecipientTrainerKnown) {
        out.status=Status::UnknownRecipientHistory;return out;
    }
    out.rng=Ranger::analyzePidIv(in.observed,in.pid,in.ivs,
                                source.originalRecipientTid,
                                source.originalRecipientSid);
    out.status=out.rng.matched()
        ? Status::ConditionalGiftCandidate : Status::CandidatePidIvMismatch;
    return out;
}
inline Result evaluateNative(const Pokemon::Pokemon4ReadOnly& pokemon,
                             const SourceEvidence& source={}) noexcept {
    if(!pokemon.valid() || pokemon.empty())return {};
    Ranger::Candidate gift{};
    gift.species=pokemon.species();
    gift.language=pokemon.language();
    gift.gender=pokemon.gender();
    gift.isEgg=pokemon.isEgg();
    gift.eggLocation=pokemon.eggLocationExtended()!=0
        ? pokemon.eggLocationExtended():pokemon.eggLocationDP();
    gift.metLocation=pokemon.metLocationExtended()!=0
        ? pokemon.metLocationExtended():pokemon.metLocationDP();
    gift.ball=pokemon.ballDPPt()>pokemon.ballHGSS()
        ? pokemon.ballDPPt():pokemon.ballHGSS();
    gift.fateful=pokemon.fatefulEncounter();
    return evaluate(true,{
        gift,pokemon.pid(),pokemon.ivs(),pokemon.originVersion(),
        pokemon.tid(),pokemon.sid()
    },source);
}
} // namespace Legality::Gen4EventRangerGiftHistory
