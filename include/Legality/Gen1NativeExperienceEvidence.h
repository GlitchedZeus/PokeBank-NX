#pragma once
// L2 Gen I ONLY: conditional original EXP/level evidence.
// PK1 uses THREE bytes of EXP at 0x0E..0x10. The retail byte-level
// record cannot prove ordinary level acquisition or absence of glitches.
// Reuse project's Gen I personal growth column + Pokemon::Experience;
// never infer native genders, breeding, natures, held items or abilities.
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include <cstdint>

namespace Legality::Gen1NativeExperience {
namespace Gen1=PokeVault::Integration::Gen1;
enum class Status : std::uint8_t {
    UnknownSource,
    InvalidDecodedSourceField,
    UnknownOriginalProgression,
    ConditionalLevelExperienceMatch,
    ContradictedAttestedOriginalProgression
};
struct Source {
    bool strictReadOnlyPK1IndependentlyVerified=false;
    // Both facts must be independently attested to exclude nonstandard
    // level-up history, modifications, glitches and tradeback ambiguity.
    bool originalGen1SpeciesGrowthIndependentlyVerified=false;
    bool normalLevelExperienceHistoryIndependentlyVerified=false;
};
struct Result {
    Status status=Status::UnknownSource;
    std::uint8_t personalGrowthRate=0;
    std::uint8_t levelFromExperience=0;
    std::uint32_t minimumExperienceAtLevel=0;
    // At level 100 this is 2^24, exclusive: PK1 stores 24-bit EXP.
    std::uint32_t exclusiveExperienceAtNextLevel=0;
};
Result evaluate(const Gen1::PokemonRecord&,Source) noexcept;
} // namespace Legality::Gen1NativeExperience
