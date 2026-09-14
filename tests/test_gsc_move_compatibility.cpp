#include "Integration/Gen2/Gen2MoveCompatibility.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>

using PokeVault::Integration::Gen2::MoveCompatibility;
using PokeVault::Integration::Gen2::SourceGame;

int main() {
    // Chikorita starts with Tackle in the Gen II rulesets; prove all exact-game
    // selectors reach their intended table.
    constexpr uint16_t chikorita = 152;
    constexpr uint16_t tackle = 33;
    assert(MoveCompatibility::canLearnMove(SourceGame::Gold, chikorita, tackle));
    assert(MoveCompatibility::canLearnMove(SourceGame::Silver, chikorita, tackle));
    assert(MoveCompatibility::canLearnMove(SourceGame::Crystal, chikorita, tackle));

    // Chikorita cannot learn Fly; normal Create/Edit must reject it.
    constexpr uint16_t fly = 19;
    assert(!MoveCompatibility::canLearnMove(SourceGame::Gold, chikorita, fly));
    assert(!MoveCompatibility::canLearnMove(SourceGame::Silver, chikorita, fly));

    // Gold and Silver intentionally share one table, while Crystal is a
    // separately generated ruleset. Require at least one real exact-game delta
    // so a future accidental alias cannot silently pass.
    bool foundCrystalOnly = false;
    uint16_t deltaSpecies = 0;
    uint16_t deltaMove = 0;
    for (uint16_t species = 1; species <= 251 && !foundCrystalOnly; ++species) {
        for (uint16_t move = 1; move <= 251; ++move) {
            const bool gs = MoveCompatibility::canLearnMove(SourceGame::Gold, species, move);
            assert(gs == MoveCompatibility::canLearnMove(SourceGame::Silver, species, move));
            const bool crystal = MoveCompatibility::canLearnMove(SourceGame::Crystal, species, move);
            if (!gs && crystal) {
                foundCrystalOnly = true;
                deltaSpecies = species;
                deltaMove = move;
                break;
            }
        }
    }
    assert(foundCrystalOnly);
    assert(!MoveCompatibility::canLearnMove(SourceGame::Gold, deltaSpecies, deltaMove));
    assert(!MoveCompatibility::canLearnMove(SourceGame::Silver, deltaSpecies, deltaMove));
    assert(MoveCompatibility::canLearnMove(SourceGame::Crystal, deltaSpecies, deltaMove));

    // Empty slots are valid. Corrupt/out-of-range species, move and game values
    // fail closed for populated moves without indexing outside committed data.
    assert(MoveCompatibility::canLearnMove(SourceGame::Gold, chikorita, 0));
    assert(MoveCompatibility::canLearnMove(SourceGame::Crystal, 0, 0));
    assert(!MoveCompatibility::canLearnMove(SourceGame::Gold, 0, tackle));
    assert(!MoveCompatibility::canLearnMove(SourceGame::Gold, 252, tackle));
    assert(!MoveCompatibility::canLearnMove(SourceGame::Gold, chikorita, 252));
    assert(!MoveCompatibility::canLearnMove(static_cast<SourceGame>(0xFF), chikorita, tackle));

    const std::array<uint8_t, 4> compatibleMoves{static_cast<uint8_t>(tackle), 0, 0, 0};
    assert(!MoveCompatibility::firstIncompatible(SourceGame::Gold, chikorita, compatibleMoves));

    const std::array<uint8_t, 4> unusualMoves{static_cast<uint8_t>(tackle),
                                               static_cast<uint8_t>(fly), 0, 0};
    const auto unusual = MoveCompatibility::firstIncompatible(SourceGame::Gold, chikorita, unusualMoves);
    assert(unusual && *unusual == 1);

    const auto pool = MoveCompatibility::compatibleMoves(SourceGame::Crystal, chikorita);
    assert(!pool.empty() && pool.front() == 0);

    std::cout << "GSC exact-game move compatibility tests passed; Crystal-only delta species="
              << deltaSpecies << " move=" << deltaMove << "\n";
    return 0;
}
