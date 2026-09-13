#include "Integration/Gen1/Gen1MoveCompatibility.h"

#include <algorithm>

namespace PokeVault::Integration::Gen1 {
namespace {
#include "Gen1MoveCompatibilityData.inc"

const uint64_t* wordsFor(SourceGame game, uint16_t species) noexcept {
    if (species < 1 || species > 151) return nullptr;
    if (game == SourceGame::Yellow) {
        for (const auto& overrideRow : kCompatibleMovesYOverrides)
            if (overrideRow.species == species) return overrideRow.words;
    }
    return kCompatibleMovesRB[species];
}
} // namespace

bool MoveCompatibility::canLearnMove(SourceGame game, uint16_t species, uint16_t move) noexcept {
    if (move == 0) return true; // Empty move slot is always a valid normal-editor choice.
    if (move > 165) return false;
    const auto* words = wordsFor(game, species);
    if (!words) return false;
    const unsigned word = move >> 6;
    const unsigned bit = move & 63u;
    return (words[word] & (uint64_t{1} << bit)) != 0;
}

std::vector<uint8_t> MoveCompatibility::compatibleMoves(SourceGame game, uint16_t species) {
    std::vector<uint8_t> result;
    result.reserve(96);
    result.push_back(0);
    if (species < 1 || species > 151) return result;
    for (uint16_t move = 1; move <= 165; ++move)
        if (canLearnMove(game, species, move)) result.push_back(static_cast<uint8_t>(move));
    return result;
}

std::optional<std::size_t> MoveCompatibility::firstIncompatible(
    SourceGame game, uint16_t species, const std::array<uint8_t,4>& moves) noexcept {
    for (std::size_t i = 0; i < moves.size(); ++i)
        if (!canLearnMove(game, species, moves[i])) return i;
    return std::nullopt;
}

} // namespace PokeVault::Integration::Gen1
