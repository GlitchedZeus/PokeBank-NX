#ifndef POKEBANK_BOX_SUMMARY_RADAR_H
#define POKEBANK_BOX_SUMMARY_RADAR_H

#include "UI/PokemonEditorFoundationContract.h"

#include <array>
#include <cstdint>
#include <optional>

// Which battle-stat radar the Box summary (quick view) draws for the selected record. The quick
// view must show the same graph the editor and View show (SharedPokemonShell::drawDataAndGraph:
// StatsRadar + BattleStatRadarModel, adaptive "Scale N", HP/Atk/Def/Spe/SpA/SpD clockwise), so
// every shipped Gen I-V editor routes here to its shared renderer:
//   Gen I   -> StatsRadar::drawGen1Labeled (one Special axis)
//   Gen II  -> StatsRadar::drawGen2Labeled (calculated stats from the DVs)
//   Gen III -> shared six-stat radar (RSE and FRLG records both report the FRLG game group)
//   Gen IV  -> shared six-stat radar
//   Gen V   -> shared six-stat radar (Round 11 G5-RADAR: it used to fall through to the inherited
//              fixed-255 hexagon, so low-level Pokemon all drew the same tiny blob)
// Only formats without a shared editor (the inherited Gen VII-IX loaders) keep the inherited
// hexagon; moving them is a separate, deliberate change.
namespace PokeBank::UIModel::BoxSummaryRadar {

enum class Route : uint8_t { Gen1Special, Gen2Calculated, SharedSixStat, InheritedHexagon };

using PokemonEditorFoundation::Generation;

// pk1Record / gscRecord: the no-RTTI adapter tags of the record itself. frlgGroup: the record's
// game group is FRLG (every Gen III adapter). exactGeneration: the opened save's exact
// capability generation, if it has one.
[[nodiscard]] constexpr Route route(bool pk1Record, bool gscRecord, bool frlgGroup,
                                    std::optional<Generation> exactGeneration) noexcept {
    if (pk1Record) return Route::Gen1Special;
    if (gscRecord) return Route::Gen2Calculated;
    if (frlgGroup) return Route::SharedSixStat;
    if (exactGeneration && (*exactGeneration == Generation::Gen3 ||
                            *exactGeneration == Generation::Gen4 ||
                            *exactGeneration == Generation::Gen5))
        return Route::SharedSixStat;
    return Route::InheritedHexagon;
}

// The six values in the shared radar's order, read from the record's own display getters. For
// Gen V these are Pokemon5ReadOnlyView's displayStats5 values, the same function the editor's
// Gen5SharedReadOnlySurface::stats and View use, so the quick view is numerically identical.
template <class Record>
[[nodiscard]] std::array<uint16_t,6> sixStats(const Record& p) {
    return {p.statHPMax(), p.statATK(), p.statDEF(), p.statSPE(), p.statSPA(), p.statSPD()};
}

} // namespace PokeBank::UIModel::BoxSummaryRadar
#endif
