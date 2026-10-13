#include "Legality/Gen1NativeExperienceEvidence.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Pokemon/Experience.h"

namespace Legality::Gen1NativeExperience {
Result evaluate(const Gen1::PokemonRecord& r,Source source) noexcept {
    Result out{};
    if (!source.strictReadOnlyPK1IndependentlyVerified) return out;
    const auto& bytes=r.rawBody;
    const auto expectedSize=r.partyRecord?44u:33u;
    if (r.rawBodySize!=expectedSize ||
        !r.rawSpecies || bytes[0]!=r.rawSpecies ||
        r.species<1 || r.species>151 ||
        Gen1::gen1InternalToNational(r.rawSpecies)!=r.species ||
        r.experience>0xFFFFFFu) {
        out.status=Status::InvalidDecodedSourceField;return out;
    }
    const std::uint32_t storedExperience=
        (static_cast<std::uint32_t>(bytes[0x0E])<<16u)|
        (static_cast<std::uint32_t>(bytes[0x0F])<<8u)|bytes[0x10];
    const std::uint8_t storedLevel=bytes[r.partyRecord?0x21:0x03];
    if (storedExperience!=r.experience ||
        storedLevel!=r.level || storedLevel>100) {
        out.status=Status::InvalidDecodedSourceField;return out;
    }
    if (!source.originalGen1SpeciesGrowthIndependentlyVerified ||
        !source.normalLevelExperienceHistoryIndependentlyVerified ||
        storedLevel==0) {
        out.status=Status::UnknownOriginalProgression;return out;
    }
    out.personalGrowthRate=Gen1::StagedPokemonEditor::growthRate(r.species);
    // Gen I's original personal table contains only 0,3,4,5. Do not
    // let Experience.cpp's default growth-0 fallback invent a proof if
    // the supposedly original source has an unsupported table entry.
    if (out.personalGrowthRate!=0 && out.personalGrowthRate!=3 &&
        out.personalGrowthRate!=4 && out.personalGrowthRate!=5) {
        out.status=Status::UnknownOriginalProgression;return out;
    }
    out.minimumExperienceAtLevel=
        Pokemon::getExpForLevel(storedLevel,out.personalGrowthRate);
    out.exclusiveExperienceAtNextLevel=storedLevel==100?0x1000000u:
        Pokemon::getExpForLevel(static_cast<std::uint8_t>(storedLevel+1),
                                out.personalGrowthRate);
    out.levelFromExperience=
        Pokemon::getLevelFromExp(r.experience,out.personalGrowthRate);
    if(r.experience<out.minimumExperienceAtLevel ||
       r.experience>=out.exclusiveExperienceAtNextLevel ||
       out.levelFromExperience!=storedLevel) {
        out.status=Status::ContradictedAttestedOriginalProgression;
        return out;
    }
    out.status=Status::ConditionalLevelExperienceMatch;
    return out;
}
} // namespace Legality::Gen1NativeExperience
