#include "Names/MachineDisplay.h"

#include "Names/MoveNames.h"
#include "Names/TMMoves.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

namespace Names {
namespace {

MachineDescriptor make(Enums::GameVersion game, uint16_t itemId, MachineKind kind,
                       uint16_t number, uint8_t width) noexcept {
    const uint16_t move = getTMMove(game, itemId);
    if (move == 0) return {};
    return {kind, number, width, move};
}

std::string lower(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

} // namespace

MachineDescriptor getMachineDescriptor(Enums::GameVersion game, uint16_t itemId) noexcept {
    using Enums::GameVersion;
    switch (game) {
        case GameVersion::GSC: {
            static constexpr uint8_t ids[] = {
                191,192,193,194,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,
                212,213,214,215,216,217,218,219,221,222,223,224,225,226,227,228,229,230,231,232,
                233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,
            };
            for (uint16_t i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
                if (ids[i] != itemId) continue;
                if (i < 50) return make(game, itemId, MachineKind::TM, static_cast<uint16_t>(i + 1), 2);
                return make(game, itemId, MachineKind::HM, static_cast<uint16_t>(i - 49), 2);
            }
            return {};
        }

        case GameVersion::FR:
        case GameVersion::LG:
        case GameVersion::FRLG:
            if (itemId >= 289 && itemId <= 338) return make(game, itemId, MachineKind::TM, itemId - 288, 2);
            if (itemId >= 339 && itemId <= 346) return make(game, itemId, MachineKind::HM, itemId - 338, 2);
            return {};

        case GameVersion::GP:
        case GameVersion::GE:
        case GameVersion::GG:
            if (itemId >= 328 && itemId <= 387) return make(game, itemId, MachineKind::TM, itemId - 327, 2);
            return {};

        case GameVersion::SW:
        case GameVersion::SH:
        case GameVersion::SWSH:
            if (itemId == 1230) return make(game, itemId, MachineKind::TM, 0, 2);
            if (itemId >= 328 && itemId <= 419) return make(game, itemId, MachineKind::TM, itemId - 327, 2);
            if (itemId >= 618 && itemId <= 620) return make(game, itemId, MachineKind::TM, 93 + (itemId - 618), 2);
            if (itemId >= 690 && itemId <= 693) return make(game, itemId, MachineKind::TM, 96 + (itemId - 690), 2);
            if (itemId >= 1130 && itemId <= 1229) return make(game, itemId, MachineKind::TR, itemId - 1130, 2);
            return {};

        case GameVersion::BD:
        case GameVersion::SP:
        case GameVersion::BDSP:
            if (itemId >= 328 && itemId <= 419) return make(game, itemId, MachineKind::TM, itemId - 327, 2);
            if (itemId >= 420 && itemId <= 427) return make(game, itemId, MachineKind::TM, 93 + (itemId - 420), 3);
            return {};

        case GameVersion::SL:
        case GameVersion::VL:
        case GameVersion::SV:
            if (itemId == 1230) return make(game, itemId, MachineKind::TM, 0, 3);
            if (itemId >= 328 && itemId <= 419) return make(game, itemId, MachineKind::TM, itemId - 327, 3);
            if (itemId >= 618 && itemId <= 620) return make(game, itemId, MachineKind::TM, 93 + (itemId - 618), 3);
            if (itemId >= 690 && itemId <= 693) return make(game, itemId, MachineKind::TM, 96 + (itemId - 690), 3);
            if (itemId >= 2160 && itemId <= 2289) return make(game, itemId, MachineKind::TM, 100 + (itemId - 2160), 3);
            return {};

        case GameVersion::ZA:
            if (itemId >= 328 && itemId <= 419) return make(game, itemId, MachineKind::TM, itemId - 327, 3);
            if (itemId >= 618 && itemId <= 620) return make(game, itemId, MachineKind::TM, 93 + (itemId - 618), 3);
            if (itemId >= 690 && itemId <= 693) return make(game, itemId, MachineKind::TM, 96 + (itemId - 690), 3);
            if (itemId == 2160) return make(game, itemId, MachineKind::TM, 100, 3);
            if (itemId >= 2162 && itemId <= 2221) return make(game, itemId, MachineKind::TM, 101 + (itemId - 2162), 3);
            return {};

        case GameVersion::PLA:
        case GameVersion::RBY:
        default:
            return {};
    }
}

std::string machineIdentifier(const MachineDescriptor& machine) {
    if (!machine) return {};
    const char* prefix = machine.kind == MachineKind::TM ? "TM" :
                         machine.kind == MachineKind::HM ? "HM" :
                         machine.kind == MachineKind::TR ? "TR" : "";
    std::ostringstream out;
    out << prefix << std::setw(machine.numberWidth) << std::setfill('0') << machine.number;
    return out.str();
}

std::string machineDisplayLabel(Enums::GameVersion game, uint16_t itemId, std::string_view fallbackLabel) {
    const auto machine = getMachineDescriptor(game, itemId);
    if (!machine) return std::string(fallbackLabel);
    return machineIdentifier(machine) + " — " + getMoveName(machine.moveId);
}

bool machineDisplayMatches(Enums::GameVersion game, uint16_t itemId,
                           std::string_view fallbackLabel, std::string_view query) {
    if (query.empty()) return true;
    const std::string q = lower(query);
    const std::string display = lower(machineDisplayLabel(game, itemId, fallbackLabel));
    if (display.find(q) != std::string::npos) return true;
    const auto machine = getMachineDescriptor(game, itemId);
    if (!machine) return lower(fallbackLabel).find(q) != std::string::npos;
    return std::to_string(machine.number).find(q) != std::string::npos ||
           lower(getMoveName(machine.moveId)).find(q) != std::string::npos;
}

} // namespace Names
