#ifndef POKEBANK_GEN8_TEAM_INDEXES_H
#define POKEBANK_GEN8_TEAM_INDEXES_H

// One battle-team reader for every SwishCrypto title with TeamIndexes8 (Sword/Shield, Scarlet/Violet,
// Legends: Z-A). Pinned PKHeX 6501f0ab Saves/Substructures/Gen8/SWSH/TeamIndexes8.cs reads
// `Indexes.Data` (6 teams x 6 int16, (box << 8) | slot, negative = empty) and `Locks.Data` (bit `team`
// of byte 0) as RAW BYTES, whatever SCTypeCode the block carries. PKSE 1.2.2 (Trainer.h
// battleTeamBlockHoldsSlot) does the same on real saves.
//
// Owner hardware 2026-11-01 (G69-HW-007): a real Legends: Z-A save showed "Battle teams: Unreadable".
// The previous readers additionally required an Object or Array<Byte> block, so a block stored as e.g.
// Array<Int16> or a single Byte/UInt32 value was refused although its bytes are exactly what PKHeX
// reads. That type gate is the most likely cause (unconfirmed without the owner's save); the strict
// parts stay: both keys present, >= 72 index bytes, >= 1 lock byte, every pointer inside the
// title's boxes, otherwise the table is unreadable and every box storage change is refused. The
// reason is kept so the Trainer screen can say exactly why.

#include "Integration/Gen8/Gen8Save.h"
#include "Integration/Gen8/Gen8SwishCrypto.h"

#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen8 {

struct TeamTable {
    std::optional<std::vector<TeamSlot>> teams;   // nullopt = unreadable
    std::string diagnostic;                        // why unreadable (empty when readable)
};

[[nodiscard]] inline const char* swishTypeName(Swish::TypeCode t) noexcept {
    switch (t) {
        case Swish::TypeCode::Bool1: return "Bool1";
        case Swish::TypeCode::Bool2: return "Bool2";
        case Swish::TypeCode::Bool3: return "Bool3";
        case Swish::TypeCode::Object: return "Object";
        case Swish::TypeCode::Array: return "Array";
        case Swish::TypeCode::Byte: return "Byte";
        case Swish::TypeCode::UInt16: return "UInt16";
        case Swish::TypeCode::UInt32: return "UInt32";
        case Swish::TypeCode::UInt64: return "UInt64";
        case Swish::TypeCode::SByte: return "SByte";
        case Swish::TypeCode::Int16: return "Int16";
        case Swish::TypeCode::Int32: return "Int32";
        case Swish::TypeCode::Int64: return "Int64";
        case Swish::TypeCode::Single: return "Single";
        case Swish::TypeCode::Double: return "Double";
        default: return "None";
    }
}

[[nodiscard]] inline std::string describeBlock(const Swish::Block& b) {
    std::string type = swishTypeName(b.type);
    if (b.type == Swish::TypeCode::Array) type += std::string("<") + swishTypeName(b.subtype) + ">";
    return type + ", " + std::to_string(b.data.size()) + " bytes";
}

[[nodiscard]] inline TeamTable readTeamIndexes(const Swish::Block* indexes, const Swish::Block* locks,
                                               size_t boxCount, size_t boxSlots) {
    constexpr size_t TeamCount = 6, TeamSize = 6, IndexBytes = TeamCount * TeamSize * 2;
    TeamTable out;
    if (!indexes) { out.diagnostic = "team index block 0x33F39467 missing"; return out; }
    if (!locks) { out.diagnostic = "team lock block 0x605EBC30 missing"; return out; }
    if (indexes->data.size() < IndexBytes) {
        out.diagnostic = "team index block is " + describeBlock(*indexes) + "; 72 bytes needed";
        return out;
    }
    if (locks->data.empty()) {
        out.diagnostic = "team lock block is " + describeBlock(*locks) + "; no lock byte";
        return out;
    }
    std::vector<TeamSlot> teams;
    for (size_t i = 0; i < TeamCount * TeamSize; ++i) {
        const auto raw = static_cast<int16_t>(static_cast<uint16_t>(indexes->data[2 * i] | (indexes->data[2 * i + 1] << 8)));
        if (raw < 0) continue;
        const size_t box = static_cast<size_t>(raw) >> 8, slot = static_cast<size_t>(raw) & 0xFF;
        if (box >= boxCount || slot >= boxSlots) {
            char hex[8];
            std::snprintf(hex, sizeof(hex), "%04X", static_cast<unsigned>(static_cast<uint16_t>(raw)));
            out.diagnostic = "team " + std::to_string(i / TeamSize + 1) + " pointer 0x" + hex + " (box " +
                             std::to_string(box + 1) + ", slot " + std::to_string(slot + 1) + ") is outside " +
                             std::to_string(boxCount) + " x " + std::to_string(boxSlots);
            return out;
        }
        const size_t team = i / TeamSize;
        teams.push_back({team, box, slot, ((locks->data[0] >> team) & 1) != 0});
    }
    out.teams = std::move(teams);
    return out;
}

// The Trainer-screen text for a team table.
[[nodiscard]] inline std::string teamFactText(const std::optional<std::vector<TeamSlot>>& teams, const std::string& why) {
    if (teams) return std::to_string(teams->size()) + " box references";
    return "Unreadable: " + (why.empty() ? std::string("unknown reason") : why) + " (box changes refused)";
}

} // namespace PokeVault::Integration::Gen8

#endif
