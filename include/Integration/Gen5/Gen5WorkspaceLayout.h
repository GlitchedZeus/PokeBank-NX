#ifndef POKEBANK_GEN5_WORKSPACE_LAYOUT_H
#define POKEBANK_GEN5_WORKSPACE_LAYOUT_H

#include "Integration/Gen5/Gen5SharedScreenState.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace PokeVault::Integration::Gen5 {

// Gen V View/Create/Edit use the SAME three-panel shared Pokemon workspace as
// Gen I-IV (UI::SharedPokemonShell): DETAILS (portrait + scrollable rows),
// STATS (IV/EV/Stat table) and MOVES (+ native data and stat radar). This
// pure model places every staged Gen V field in exactly one panel and defines
// focus movement: L/R switch panel, Up/Down move within the panel.
enum class WorkspacePanel5 : uint8_t { Details = 0, Stats = 1, Moves = 2 };
inline constexpr size_t kWorkspacePanelCount5 = 3;

namespace WorkspaceLayout5Detail {
using Field = Gen5SharedScreenState::Field;
inline constexpr auto& kRows = Gen5SharedScreenState::kEditableRows;
[[nodiscard]] constexpr uint8_t rowOf(Field field, size_t stat = 0) noexcept {
    for (size_t i = 0; i < kRows.size(); ++i)
        if (kRows[i].field == field && kRows[i].stat == stat) return static_cast<uint8_t>(i);
    return 0xFF;
}
} // namespace WorkspaceLayout5Detail

// DETAILS: identity first (as Gen IV), then encounter/ownership rows.
inline constexpr std::array<uint8_t, 19> kDetailsRows5{{
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::Species),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::Nickname),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::Level),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::Experience),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::Nature),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::Ability),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::Gender),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::Form),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::HeldItem),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::Friendship),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::Ball),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::MetLocation),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::MetLevel),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::TrainerId),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::SecretId),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::OriginalTrainer),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::OriginalTrainerGender),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::Pokerus),
    WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::FatefulEncounter),
}};
// STATS: the shared IV | EV table, walked row-major (IV HP, EV HP, IV Atk, ...).
inline constexpr std::array<uint8_t, 12> kStatsRows5 = [] {
    std::array<uint8_t, 12> out{};
    for (size_t stat = 0; stat < 6; ++stat) {
        out[stat * 2] = WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::IV, stat);
        out[stat * 2 + 1] = WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::EV, stat);
    }
    return out;
}();
// MOVES: per slot, Move then PP then PP Ups.
inline constexpr std::array<uint8_t, 12> kMovesRows5 = [] {
    std::array<uint8_t, 12> out{};
    for (size_t slot = 0; slot < 4; ++slot) {
        out[slot * 3] = WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::Move, slot);
        out[slot * 3 + 1] = WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::MovePP, slot);
        out[slot * 3 + 2] = WorkspaceLayout5Detail::rowOf(WorkspaceLayout5Detail::Field::MovePPUps, slot);
    }
    return out;
}();

struct PanelRows5 {
    const uint8_t* rows;
    size_t count;
};
[[nodiscard]] constexpr PanelRows5 panelRows5(WorkspacePanel5 panel) noexcept {
    switch (panel) {
        case WorkspacePanel5::Details: return {kDetailsRows5.data(), kDetailsRows5.size()};
        case WorkspacePanel5::Stats: return {kStatsRows5.data(), kStatsRows5.size()};
        case WorkspacePanel5::Moves: return {kMovesRows5.data(), kMovesRows5.size()};
    }
    return {kDetailsRows5.data(), kDetailsRows5.size()};
}

struct PanelPosition5 {
    WorkspacePanel5 panel = WorkspacePanel5::Details;
    size_t index = 0;
};
// Where an editable row index (Gen5SharedScreenState::fieldRow) is drawn.
[[nodiscard]] constexpr PanelPosition5 panelPosition5(int fieldRow) noexcept {
    for (size_t p = 0; p < kWorkspacePanelCount5; ++p) {
        const auto panel = static_cast<WorkspacePanel5>(p);
        const auto rows = panelRows5(panel);
        for (size_t i = 0; i < rows.count; ++i)
            if (rows.rows[i] == fieldRow) return {panel, i};
    }
    return {};
}

// Up/Down inside the focused panel (wraps within that panel only).
[[nodiscard]] constexpr int stepWithinPanel5(int fieldRow, int delta) noexcept {
    const auto pos = panelPosition5(fieldRow);
    const auto rows = panelRows5(pos.panel);
    const int count = static_cast<int>(rows.count);
    const int next = ((static_cast<int>(pos.index) + delta) % count + count) % count;
    return rows.rows[static_cast<size_t>(next)];
}
// L/R: the adjacent panel (wrapping), keeping the relative row where possible.
[[nodiscard]] constexpr int switchPanel5(int fieldRow, int delta) noexcept {
    const auto pos = panelPosition5(fieldRow);
    const int count = static_cast<int>(kWorkspacePanelCount5);
    const auto panel = static_cast<WorkspacePanel5>(
        ((static_cast<int>(pos.panel) + delta) % count + count) % count);
    const auto rows = panelRows5(panel);
    const size_t index = pos.index < rows.count ? pos.index : rows.count - 1;
    return rows.rows[index];
}

// Rows edited through a picker or text keyboard (never +/- stepping).
[[nodiscard]] constexpr bool pickerRow5(Gen5SharedScreenState::Field f) noexcept {
    using F = Gen5SharedScreenState::Field;
    return f == F::Nature || f == F::Move || f == F::MetLocation || f == F::Ability ||
           f == F::Form || f == F::Gender || f == F::Species || f == F::Nickname ||
           f == F::OriginalTrainer;
}

namespace WorkspaceLayout5Detail {
[[nodiscard]] constexpr bool coversEveryRowOnce() noexcept {
    std::array<int, kRows.size()> seen{};
    for (size_t p = 0; p < kWorkspacePanelCount5; ++p) {
        const auto rows = panelRows5(static_cast<WorkspacePanel5>(p));
        for (size_t i = 0; i < rows.count; ++i) {
            if (rows.rows[i] >= kRows.size()) return false;
            ++seen[rows.rows[i]];
        }
    }
    for (const int count : seen)
        if (count != 1) return false;
    return true;
}
static_assert(coversEveryRowOnce(), "every staged Gen V field must appear in exactly one shared panel");
} // namespace WorkspaceLayout5Detail

} // namespace PokeVault::Integration::Gen5
#endif
