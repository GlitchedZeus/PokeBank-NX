#ifndef POKEBANK_NAMES_SPECIES_NAMES_H
#define POKEBANK_NAMES_SPECIES_NAMES_H

#include <cstddef>
#include <cstdint>

namespace Names {
    const char* getSpeciesName(uint16_t speciesId);
    const char* getSpeciesNameLocalized(uint16_t speciesId, std::size_t languageIndex);
}

#endif
