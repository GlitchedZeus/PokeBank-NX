#include "Pokemon/Pokemon2ReadOnly.h"

#include "Pokemon/BaseStatsGen89.h"
#include "Utils/StringHelpers.h"

namespace Pokemon {

Pokemon2ReadOnly::Pokemon2ReadOnly(
    const PokeVault::Integration::Gen2::PokemonRecord& record)
    : record_(record) {
    dataSize = record_.rawBodySize;
    if (dataSize == 0 || dataSize > record_.rawBody.size()) {
        dataSize = 0;
        data = {};
    } else {
        buffer = new std::byte[dataSize];
        for (size_t i = 0; i < dataSize; ++i)
            buffer[i] = static_cast<std::byte>(record_.rawBody[i]);
        data = std::span<std::byte>(buffer, dataSize);
    }

    if (const auto* personal = PokeVault::Integration::Gen2::personalRecord(record_.species))
        personal_ = *personal;

    using PokeVault::Integration::Gen2::PokemonGender;
    switch (PokeVault::Integration::Gen2::genderFromAttackDV(record_.species, record_.dvs[1])) {
        case PokemonGender::Male: gender_ = 0; break;
        case PokemonGender::Female: gender_ = 1; break;
        case PokemonGender::Genderless: gender_ = 2; break;
        case PokemonGender::Unknown: gender_ = 2; break;
    }
}

const char* Pokemon2ReadOnly::species() const noexcept {
    return getSpeciesNameGen89(speciesID());
}

std::u16string Pokemon2ReadOnly::nickname() const {
    return Utils::utf8ToUtf16(record_.nickname);
}

std::u16string Pokemon2ReadOnly::otName() const {
    return Utils::utf8ToUtf16(record_.originalTrainer);
}

} // namespace Pokemon
