#pragma once
// L4 V18 — event held item, ability, nature and form source history.
// PK4 native fields are immutable observations, NOT proof of retained
// original gift metadata or original source game's ability/form rules.
// Reference: pinned PKHeX PK4.cs Block A 0x0A/0x15, Block B 0x40;
// G4PKM.cs Nature = PID % 25 (6501f0ab...).
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Legality/Gen4FormEvidence.h"
#include <cstdint>

namespace Legality::Gen4EventOtherMetadataEvidence {
enum class FieldStatus : uint8_t {
    UnknownNative, UnknownOriginal, UnknownTransformation,
    CandidateCompatible, CandidateContradicted
};
struct Source {
    bool originalGiftIndependentlyVerified=false;
    bool originalItemKnown=false;
    uint16_t originalItem=0;
    bool itemHistoryUnchanged=false;

    bool originalAbilityKnown=false;
    uint8_t originalAbility=0;
    bool speciesAndAbilityHistoryUnchanged=false;

    bool originalFormKnown=false;
    uint8_t originalForm=0;
    bool formHistoryUnchanged=false;

    bool originalNatureKnown=false;
    uint8_t originalNature=0;
    bool originalPidHistoryUnchanged=false;
};
struct Observed {
    uint16_t species=0;
    uint16_t item=0;
    uint8_t ability=0;
    uint8_t form=0;
    uint8_t nature=0;
    uint32_t pid=0;
};
struct Result {
    FieldStatus item=FieldStatus::UnknownNative;
    FieldStatus ability=FieldStatus::UnknownNative;
    FieldStatus form=FieldStatus::UnknownNative;
    FieldStatus nature=FieldStatus::UnknownNative;
    bool nativePresent=false;
    bool anyContradiction() const noexcept {
        return item==FieldStatus::CandidateContradicted ||
               ability==FieldStatus::CandidateContradicted ||
               form==FieldStatus::CandidateContradicted ||
               nature==FieldStatus::CandidateContradicted;
    }
    bool allConditionalMatches() const noexcept {
        return nativePresent &&
               item==FieldStatus::CandidateCompatible &&
               ability==FieldStatus::CandidateCompatible &&
               form==FieldStatus::CandidateCompatible &&
               nature==FieldStatus::CandidateCompatible;
    }
};
constexpr FieldStatus compare(bool originalKnown,bool unchanged,
                              uint32_t original,uint32_t observed) noexcept {
    if(!originalKnown)return FieldStatus::UnknownOriginal;
    if(!unchanged)return FieldStatus::UnknownTransformation;
    return original==observed ? FieldStatus::CandidateCompatible
                             : FieldStatus::CandidateContradicted;
}
constexpr Result evaluate(bool nativePresent,const Observed& obs,
                          const Source& source={}) noexcept {
    Result out{};
    if(!nativePresent)return out;
    out.nativePresent=true;
    if(!source.originalGiftIndependentlyVerified) {
        out.item=out.ability=out.form=out.nature=FieldStatus::UnknownOriginal;
        return out;
    }
    out.item=compare(source.originalItemKnown,source.itemHistoryUnchanged,
                     source.originalItem,obs.item);
    out.ability=compare(source.originalAbilityKnown,
                        source.speciesAndAbilityHistoryUnchanged,
                        source.originalAbility,obs.ability);
    out.form=compare(source.originalFormKnown,source.formHistoryUnchanged,
                     source.originalForm,obs.form);
    // A native Gen IV nature is an exact PID modulo 25, not a later
    // Gen VI+ explicit nature byte or mint. Require original PID identity
    // before assuming the present nature is original-gift nature.
    out.nature=compare(source.originalNatureKnown &&
                       source.originalNature<25u,
                       source.originalPidHistoryUnchanged &&
                       obs.nature==obs.pid%25u,
                       source.originalNature,obs.nature);
    return out;
}
inline Result evaluateNative(const Pokemon::Pokemon4ReadOnly& pokemon,
                             const Source& source={}) noexcept {
    if(!pokemon.valid() || pokemon.empty())return {};
    const Observed obs{
        pokemon.species(),pokemon.heldItem(),pokemon.ability(),
        pokemon.form(),static_cast<uint8_t>(pokemon.pid()%25u),pokemon.pid()
    };
    // A stored native ability can differ after evolution, and some forms
    // are changed without changing gift origin. Those historical facts
    // are not inferred here; the independently verified Source supplies
    // comparability. Do not trust native sourceGroup() as original game.
    return evaluate(true,obs,source);
}
static_assert(compare(true,true,0,0)==FieldStatus::CandidateCompatible);
static_assert(compare(true,true,0,1)==FieldStatus::CandidateContradicted);
static_assert(compare(true,false,1,2)==FieldStatus::UnknownTransformation);
static_assert(compare(false,true,1,2)==FieldStatus::UnknownOriginal);
} // namespace Legality::Gen4EventOtherMetadataEvidence
