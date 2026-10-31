#ifndef POKEBANK_GEN1_MOVE_COMPATIBILITY_H
#define POKEBANK_GEN1_MOVE_COMPATIBILITY_H

#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace PokeVault::Integration::Gen1 {

// Kid-safe Generation I move-compatibility filter for normal creation/edit pickers.
// This intentionally answers a narrower question than full legality:
// "is this move obtainable by this species in this R/B/Y ruleset through the
// pinned level-up / initial / TM / HM model, including proven Gen I ancestors?"
// Encounter/event/tradeback legality remains a separate deferred concern.
class MoveCompatibility final {
public:
    static bool canLearnMove(SourceGame game, uint16_t species, uint16_t move) noexcept;
    static std::vector<uint8_t> compatibleMoves(SourceGame game, uint16_t species);
    static std::optional<std::size_t> firstIncompatible(
        SourceGame game, uint16_t species, const std::array<uint8_t,4>& moves) noexcept;
};

} // namespace PokeVault::Integration::Gen1

#endif
