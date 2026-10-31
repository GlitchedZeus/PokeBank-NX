#ifndef POKEBANK_GEN2_MOVE_COMPATIBILITY_H
#define POKEBANK_GEN2_MOVE_COMPATIBILITY_H

#include "Integration/Gen2/Gen2ReadOnlySave.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace PokeVault::Integration::Gen2 {

// Exact-game Generation II move-compatibility filter for normal Create/Edit
// and passive preservation diagnostics. This intentionally answers a narrower
// question than full encounter legality. Existing unusual records may be
// preserved by callers; new normal-editor commits should require compatibility.
class MoveCompatibility final {
public:
    static bool canLearnMove(SourceGame game, uint16_t species, uint16_t move) noexcept;
    static std::vector<uint8_t> compatibleMoves(SourceGame game, uint16_t species);
    static std::optional<std::size_t> firstIncompatible(
        SourceGame game, uint16_t species, const std::array<uint8_t, 4>& moves) noexcept;
};

} // namespace PokeVault::Integration::Gen2

#endif
