/**
 * TypeNames.h - Pokemon Type Name Lookup
 *
 * Provides type name lookup by type ID.
 * Type IDs match the MoveType enum (0-17).
 */

#ifndef NAMES_TYPE_NAMES_H
#define NAMES_TYPE_NAMES_H

#include <cstdint>

namespace Names {
    /**
     * Gets the name of a Pokemon type by ID.
     * @param typeId Type ID (0-17)
     * @return Type name string (e.g., "Normal", "Fire", "Water")
     */
    const char* getTypeName(uint8_t typeId);

    /**
     * Gets the total number of types.
     * @return Number of types (18)
     */
    constexpr uint8_t getTypeCount() { return 18; }
}

#endif
