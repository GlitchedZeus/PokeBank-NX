#include "Pokemon/Pokemon1ReadOnly.h"

#include "Pokemon/BaseStatsGen89.h"
#include "Utils/StringHelpers.h"

#include <algorithm>

namespace Pokemon {

Pokemon1ReadOnly::Pokemon1ReadOnly(
    const PokeVault::Integration::Gen1::PokemonRecord& record)
    : record_(record) {
    dataSize = record_.rawBodySize;
    if (dataSize == 0 || dataSize > record_.rawBody.size()) {
        dataSize = 0;
        data = {};
        return;
    }
    buffer = new std::byte[dataSize];
    for (size_t i = 0; i < dataSize; ++i)
        buffer[i] = static_cast<std::byte>(record_.rawBody[i]);
    data = std::span<std::byte>(buffer, dataSize);
}

const char* Pokemon1ReadOnly::species() const noexcept {
    return getSpeciesNameGen89(speciesID());
}

std::u16string Pokemon1ReadOnly::nickname() const {
    return Utils::utf8ToUtf16(record_.nickname);
}

std::u16string Pokemon1ReadOnly::otName() const {
    return Utils::utf8ToUtf16(record_.originalTrainer);
}

} // namespace Pokemon
