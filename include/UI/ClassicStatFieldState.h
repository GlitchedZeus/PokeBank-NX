#pragma once
#include "UI/SharedFieldState.h"

#include <array>
#include <cstdint>

// R2-GOLD: the Gen I / Gen II STATS grid on the ONE field-state contract (UI/SharedFieldState.h).
// Rows: 0 HP, 1 Attack, 2 Defense, 3 Speed, 4 Special. Columns: 0 DV, 1 Stat Exp, 2 Stat.
//  - HP DV is never stored: it is derived from the low bit of the Attack, Defense, Speed and
//    Special DVs (pret/pokered and pret/pokecrystal CalcStat; PKHeX PK1/PK2 IV_HP), so it is
//    ReadOnly (gold) in View, Create and Edit alike, and always shown as that derived value.
//  - The calculated battle Stat column is ReadOnly (gold) everywhere.
//  - Stored DVs (Attack..Special) and Stat Exp are Editable fields; in View they keep the same
//    field colour (View disables the editor, not the field type), so View and Edit match.
namespace PokeBank::UIModel::ClassicStatFields {

namespace FS = SharedFieldState;

inline constexpr int kHpRow = 0;
inline constexpr int kDvColumn = 0;
inline constexpr int kStatExpColumn = 1;
inline constexpr int kStatColumn = 2;

constexpr FS::State cellState(int row, int column) noexcept {
    if (column == kStatColumn) return FS::State::ReadOnly;
    if (column == kDvColumn && row == kHpRow) return FS::State::ReadOnly;
    return FS::State::Editable;
}

// The split Special Attack / Special Defense display is calculated from the one stored Special
// stat (Gen I) or the one Special DV / Stat Exp pair (Gen II): ReadOnly (gold).
inline constexpr FS::State kSplitSpecialState = FS::State::ReadOnly;

// Shiny is the inline Yes / No toggle (A flips it by rerolling DVs, Gen12DVReroll.h): an Editable
// field, so its value is never drawn gold. Gold means locked; a shiny Pokemon is marked by the
// shiny star, not by a gold value (QA CG16).
inline constexpr FS::State kShinyState = FS::State::Editable;

// HP DV from the four stored DVs (Attack, Defense, Speed, Special).
constexpr uint8_t derivedHpDv(const std::array<uint8_t, 4>& storedDvs) noexcept {
    return static_cast<uint8_t>(((storedDvs[0] & 1U) << 3U) | ((storedDvs[1] & 1U) << 2U) |
                                ((storedDvs[2] & 1U) << 1U) | (storedDvs[3] & 1U));
}

static_assert(cellState(kHpRow, kDvColumn) == FS::State::ReadOnly);
static_assert(cellState(1, kDvColumn) == FS::State::Editable);
static_assert(cellState(kHpRow, kStatExpColumn) == FS::State::Editable);
static_assert(cellState(4, kStatColumn) == FS::State::ReadOnly);
static_assert(kSplitSpecialState == FS::State::ReadOnly);
static_assert(kShinyState == FS::State::Editable);
static_assert(derivedHpDv({7, 10, 10, 10}) == 8);

} // namespace PokeBank::UIModel::ClassicStatFields
