/**
 * Trainer.cpp - Trainer/Save File Data Management Implementation
 *
 * This file implements the Trainer class for managing Pokemon save file data.
 * Pokemon encryption/decryption logic has been moved to Pokemon/Gen8Encryption.cpp
 * and Pokemon class implementations have been moved to Pokemon/Pokemon8SWSH.cpp, Pokemon/Pokemon7LGPE.cpp, etc.
 */

#include <cstdint>
#include <cstddef>
#include <vector>
#include <cstring>
#include <span>
#include <algorithm>

#include "Utils/Logger.h"
#include "Trainer/Trainer.h"
#include "Encryption/Encryption.h"

// Forward declarations for Names namespace functions
namespace Names {
    extern const char* getItemName(uint16_t itemId);
    extern const char* getNatureName(uint8_t natureId);
    extern const char* getSpeciesName(uint16_t speciesId);
    extern const char* getAbilityName(uint16_t abilityId);
    extern size_t getItemCount();
}

// Wrapper functions in Trainer namespace that forward to Names namespace
namespace Trainer {
    const char* getItemName(uint16_t itemId) {
        return Names::getItemName(itemId);
    }

    const char* getNatureName(uint8_t natureId) {
        return Names::getNatureName(natureId);
    }

    const char* getSpeciesName(uint16_t speciesId) {
        return Names::getSpeciesName(speciesId);
    }

    const char* getAbilityName(uint16_t abilityId) {
        return Names::getAbilityName(abilityId);
    }

    size_t getItemCount() {
        return Names::getItemCount();
    }
}