#include "Legality/Gen1NativeDVStatEvidence.h"
namespace Legality::Gen1NativeDVStat {
Result evaluate(const G1::PokemonRecord& r,SourceContext source) noexcept {
    Result out{};
    if(!source.strictPK1SourceIndependentlyVerified)return out;
    const bool party=r.partyRecord;
    const auto& body=r.rawBody;
    const std::size_t size=party?44u:33u;
    if(r.rawBodySize!=size || r.species==0 || r.species>151 ||
       r.rawSpecies==0 || body[0]!=r.rawSpecies ||
       G1::gen1InternalToNational(r.rawSpecies)!=r.species ||
       r.experience>0xFFFFFFu ||
       r.currentHP!=bigEndian16(body,1) ||
       r.status!=body[4] || r.type1!=body[5] || r.type2!=body[6] ||
       r.catchRate!=body[7] ||
       r.trainerId!=bigEndian16(body,0x0C) ||
       r.experience!=bigEndian24(body,0x0E)) {
        out.status=Status::InvalidNativeRecord;return out;
    }
    for(std::size_t i=0;i<4;++i){
        if(r.moves[i]!=body[8+i] ||
           r.pp[i]!=(body[0x1D+i]&0x3Fu) ||
           r.ppUps[i]!=(body[0x1D+i]>>6u)){
            out.status=Status::InvalidNativeRecord;return out;
        }
    }
    for(std::size_t i=0;i<5;++i){
        out.statExperience[i]=bigEndian16(body,0x11+2*i);
        if(r.statExperience[i]!=out.statExperience[i]) {
            out.status=Status::InvalidNativeRecord;return out;
        }
    }
    const auto packed=bigEndian16(body,0x1B);
    out.storedDVs=unpackDVs(packed);
    out.derivedHPDV=hpDV(out.storedDVs);
    if(r.dvs[0]!=out.derivedHPDV){
        out.status=Status::InvalidNativeRecord;return out;
    }
    for(std::size_t i=0;i<4;++i)
        if(r.dvs[i+1]!=out.storedDVs[i]){
            out.status=Status::InvalidNativeRecord;return out;
        }
    const auto level=party?body[0x21]:body[3];
    if(level!=r.level || level>100){
        out.status=Status::InvalidNativeRecord;return out;
    }
    if(party){
        out.boxLevelDiffersFromPartyLevel=body[3]!=body[0x21];
        const auto cached=std::array<std::uint16_t,5>{
            bigEndian16(body,0x22),bigEndian16(body,0x24),
            bigEndian16(body,0x26),bigEndian16(body,0x28),
            bigEndian16(body,0x2A)};
        if(cached!=(std::array<std::uint16_t,5>{
               r.maxHP,r.attack,r.defense,r.speed,r.special})){
            out.status=Status::InvalidNativeRecord;return out;
        }
    }
    // Level 0 can appear in nonstandard/glitch records. The existing
    // production battle-stat calculator intentionally models levels 1-100.
    if(level==0 || !source.nativeLevelHistoryIndependentlyVerified){
        out.status=Status::UnknownLevelContext;return out;
    }
    // Source-backed EXP/level proof is strictly opt-in: an archived
    // box level, post-tradeback body or a stale party record does not
    // establish the original retail level-up chronology on its own.
    if(source.originalExperienceProgressionIndependentlyVerified){
        out.experienceEvidence=Gen1NativeExperience::evaluate(r,{
            source.strictPK1SourceIndependentlyVerified,
            source.originalSpeciesGrowthSourceIndependentlyVerified,
            source.originalExperienceProgressionIndependentlyVerified});
        if(out.experienceEvidence.status==
           Gen1NativeExperience::Status::ContradictedAttestedOriginalProgression){
            out.status=Status::ContradictedAttestedLevelExperience;return out;
        }
        if(out.experienceEvidence.status!=
           Gen1NativeExperience::Status::ConditionalLevelExperienceMatch){
            out.status=Status::UnknownLevelContext;return out;
        }
    }
    out.calculated=G1::StagedPokemonEditor::calculateBattleStats(
        r.species,level,out.storedDVs,out.statExperience);
    if(out.calculated.hp==0){
        // A production source/stat-table failure is not a fabricated
        // invalid species, and cannot become global Illegal.
        out.status=Status::UnknownLevelContext;return out;
    }
    if(!party){
        out.status=Status::ConditionalNativeFields;return out;
    }
    if(!source.sameLevelAndStatExpAtLastCalculationIndependentlyVerified ||
       !source.ordinaryUnmodifiedPartyStatCalculationIndependentlyVerified){
        out.status=Status::UnresolvedPartyCache;return out;
    }
    out.comparedCachedPartyStats=true;
    if(out.calculated.asArray()!=(std::array<std::uint16_t,5>{
           r.maxHP,r.attack,r.defense,r.speed,r.special}) ||
       r.currentHP>r.maxHP){
        out.status=Status::ContradictedAttestedPartyStats;return out;
    }
    out.status=Status::ConditionalPartyStatMatch;
    return out;
}
} // namespace Legality::Gen1NativeDVStat
