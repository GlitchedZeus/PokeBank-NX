/**
 * TypeNames.cpp - Pokemon Type Name Lookup Implementation
 *
 * Maps type IDs to type names.
 * Type IDs match the MoveType enum (0-17):
 * 0=Normal, 1=Fighting, 2=Flying, 3=Poison, 4=Ground, 5=Rock,
 * 6=Bug, 7=Ghost, 8=Steel, 9=Fire, 10=Water, 11=Grass,
 * 12=Electric, 13=Psychic, 14=Ice, 15=Dragon, 16=Dark, 17=Fairy
 */

#include "Names/TypeNames.h"

namespace Names {
    // Type names indexed by type ID (matches MoveType enum)
    static const char* TYPE_NAMES[] = {
        "Normal",   // 0
        "Fighting", // 1
        "Flying",   // 2
        "Poison",   // 3
        "Ground",   // 4
        "Rock",     // 5
        "Bug",      // 6
        "Ghost",    // 7
        "Steel",    // 8
        "Fire",     // 9
        "Water",    // 10
        "Grass",    // 11
        "Electric", // 12
        "Psychic",  // 13
        "Ice",      // 14
        "Dragon",   // 15
        "Dark",     // 16
        "Fairy"     // 17
    };

    const char* getTypeName(uint8_t typeId) {
        if (typeId < 18) {
            return TYPE_NAMES[typeId];
        }
        return "???";
    }
}
