#ifndef NAMES_MACHINE_DISPLAY_H
#define NAMES_MACHINE_DISPLAY_H

#include <cstdint>
#include <string>
#include <string_view>

#include "Enums/GameVersion.h"

namespace Names {

enum class MachineKind : uint8_t { None, TM, HM, TR };

struct MachineDescriptor {
    MachineKind kind = MachineKind::None;
    uint16_t number = 0;
    uint8_t numberWidth = 2;
    uint16_t moveId = 0;

    constexpr explicit operator bool() const noexcept {
        return kind != MachineKind::None && moveId != 0;
    }
};

// Interpret a stored item id in the context of the exact game/group. The item id is never mutated.
MachineDescriptor getMachineDescriptor(Enums::GameVersion game, uint16_t itemId) noexcept;
std::string machineIdentifier(const MachineDescriptor& machine);
std::string machineDisplayLabel(Enums::GameVersion game, uint16_t itemId, std::string_view fallbackLabel);

// Reusable case-insensitive filter helper for machine lists. Matches identifier/number/move/full label.
bool machineDisplayMatches(Enums::GameVersion game, uint16_t itemId,
                           std::string_view fallbackLabel, std::string_view query);

} // namespace Names

#endif
