#include "Pokemon/SpeciesMoveDefaults.h"

#include <algorithm>

namespace PokeBank::Moves {
namespace {
#include "LevelUpLearnsets15Data.inc"

constexpr std::span<const uint32_t> kNone{};

const LevelUpIndex15* findRow(LevelUpTable15 table, uint16_t species, uint8_t form) noexcept {
    const auto t = static_cast<uint8_t>(table);
    const auto* begin = std::begin(kLevelUpIndex15);
    const auto* end = std::end(kLevelUpIndex15);
    // Rows are sorted by (table, species, form) by the generator.
    const auto* it = std::lower_bound(begin, end, t, [&](const LevelUpIndex15& row, uint8_t) {
        return row.table < t || (row.table == t && (row.species < species ||
                                                    (row.species == species && row.form < form)));
    });
    if (it != end && it->table == t && it->species == species && it->form == form) return it;
    return nullptr;
}
} // namespace

LevelUpTable15 levelUpTableForGen3Version(uint8_t version) noexcept {
    switch (version) {
        case 1: case 2: return LevelUpTable15::RubySapphire;
        case 3: return LevelUpTable15::Emerald;
        case 4: return LevelUpTable15::FireRed;
        case 5: return LevelUpTable15::LeafGreen;
        default: return LevelUpTable15::Count;
    }
}

LevelUpTable15 levelUpTableForGen45Version(uint8_t version) noexcept {
    switch (version) {
        case 10: case 11: return LevelUpTable15::DiamondPearl;
        case 12: return LevelUpTable15::Platinum;
        case 7: case 8: return LevelUpTable15::HeartGoldSoulSilver;
        case 20: case 21: return LevelUpTable15::BlackWhite;
        case 22: case 23: return LevelUpTable15::Black2White2;
        default: return LevelUpTable15::Count;
    }
}

std::span<const uint32_t> levelUpLearnset(LevelUpTable15 table, uint16_t species, uint8_t form) noexcept {
    if (table >= LevelUpTable15::Count || species == 0) return kNone;
    const LevelUpIndex15* row = findRow(table, species, form);
    if (!row && form != 0) row = findRow(table, species, 0);
    if (!row) return kNone;
    return std::span<const uint32_t>(kLevelUpPool15 + row->start, row->count);
}

DefaultMoves defaultMoves(LevelUpTable15 table, uint16_t species, uint8_t form, uint8_t level) noexcept {
    DefaultMoves out;
    const auto rows = levelUpLearnset(table, species, form);
    if (rows.empty()) return out;
    out.known = true;
    for (const uint32_t entry : rows) {
        const auto [moveLevel, move] = decodeLevelUp(entry);
        if (moveLevel > level || move == 0) continue;
        const auto held = out.moves.begin() + out.count;
        if (std::find(out.moves.begin(), held, move) != held) continue;
        if (out.count < 4) {
            out.moves[out.count++] = move;
        } else {
            std::rotate(out.moves.begin(), out.moves.begin() + 1, out.moves.end());
            out.moves[3] = move;
        }
    }
    return out;
}

} // namespace PokeBank::Moves
