#include "Integration/Gen2/Gen2MoveCompatibility.h"

namespace PokeVault::Integration::Gen2 {
namespace {
#include "Gen2MoveCompatibilityData.inc"

const uint8_t* rowFor(SourceGame game, uint16_t species) noexcept {
    if (species < 1 || species > 251) return nullptr;

    const std::array<uint8_t, 8064>* table = nullptr;
    switch (game) {
        case SourceGame::Gold:
        case SourceGame::Silver:
            table = &kGen2GSCompatibility;
            break;
        case SourceGame::Crystal:
            table = &kGen2CrystalCompatibility;
            break;
        default:
            return nullptr;
    }

    return table->data() + (static_cast<std::size_t>(species) * kGen2MoveCompatibilityWidth);
}
} // namespace

bool MoveCompatibility::canLearnMove(SourceGame game, uint16_t species, uint16_t move) noexcept {
    if (move == 0) return species >= 1 && species <= 251;
    if (move > 251) return false;
    const auto* row = rowFor(game, species);
    if (!row) return false;
    return (row[move >> 3U] & static_cast<uint8_t>(1U << (move & 7U))) != 0;
}

std::vector<uint8_t> MoveCompatibility::compatibleMoves(SourceGame game, uint16_t species) {
    std::vector<uint8_t> result;
    if (!rowFor(game, species)) return result;
    result.reserve(128);
    result.push_back(0);
    for (uint16_t move = 1; move <= 251; ++move) {
        if (canLearnMove(game, species, move))
            result.push_back(static_cast<uint8_t>(move));
    }
    return result;
}

std::optional<std::size_t> MoveCompatibility::firstIncompatible(
    SourceGame game, uint16_t species, const std::array<uint8_t, 4>& moves) noexcept {
    if (!rowFor(game, species)) return 0;
    for (std::size_t i = 0; i < moves.size(); ++i) {
        if (!canLearnMove(game, species, moves[i])) return i;
    }
    return std::nullopt;
}

} // namespace PokeVault::Integration::Gen2
