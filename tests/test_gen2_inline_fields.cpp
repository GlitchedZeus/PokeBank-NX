// Gen II inline fields (owner campaign phase 10): A edits Shiny, Pokerus and Gender in place, no
// popup. Shiny reads "Yes"/"No" with no "(DV-derived)" note. Pokerus cycles None -> Infected ->
// Cured -> None with a valid byte at every step (strain != 0 whenever present), and a state the
// record started in comes back with its exact strain/days.
#include "UI/Gen2NativePresentation.h"
#include "UI/SharedEditorPresentation.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {

namespace N = PokeBank::UIModel::Gen2Native;
namespace SP = PokeBank::UIModel::SharedEditorPresentation;

// Gen II uses the shared Gen II-V rule (pokecrystal: days = (strain & 3) + 1, same as Gen III+).
bool validPokerus(uint8_t raw) { return SP::pokerusByteValid(raw); }
uint8_t cycle(uint8_t raw, uint8_t original) { return SP::nextPokerusByte(raw, original); }

const char* stateName(uint8_t raw) {
    const auto s = N::decodePokerus(raw);
    return !s.present ? "None" : s.active ? "Infected" : "Cured";
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

} // namespace

int main() {
    // Every original byte the editor accepts: the cycle visits None, Infected, Cured in order,
    // stays valid, and returns to the original byte after a full cycle.
    for (int original = 0; original < 256; ++original) {
        const auto raw = static_cast<uint8_t>(original);
        if (!validPokerus(raw)) continue;
        uint8_t value = raw;
        std::string sequence;
        for (int press = 0; press < 3; ++press) {
            const uint8_t next = cycle(value, raw);
            assert(validPokerus(next));
            const std::string from = stateName(value), to = stateName(next);
            assert((from == "None" && to == "Infected") || (from == "Infected" && to == "Cured") ||
                   (from == "Cured" && to == "None"));
            value = next;
        }
        assert(value == raw); // a full cycle restores the original strain/days exactly
        // 300 presses never produce an invalid byte.
        for (int press = 0; press < 300; ++press) {
            value = cycle(value, raw);
            assert(validPokerus(value));
        }
    }
    // A record that started clean gets the canonical infection, and its cure keeps that strain.
    assert(cycle(0, 0) == N::encodePokerus(1, 1));
    assert(cycle(N::encodePokerus(1, 1), 0) == N::encodePokerus(1, 0));
    // A record that started infected (strain 2, 3 days) is cured with strain 2, then cleared, then
    // re-infected exactly as it was.
    const uint8_t infected = N::encodePokerus(2, 3);
    assert(cycle(infected, infected) == N::encodePokerus(2, 0));
    assert(cycle(N::encodePokerus(2, 0), infected) == 0);
    assert(cycle(0, infected) == infected);

    // Live routes: Pokerus is cycled by A in place (no selector popup); Shiny reads Yes/No.
    const auto unified = read("src/UI/Gen2UnifiedPokemonWorkspace.inc");
    assert(unified.find("SharedEditorPresentation::nextPokerusByte(\n                p.pokerus, state.baseline.pokerus);") !=
           std::string::npos);
    // One implementation: Gen II no longer carries its own baseline-restoring cycle.
    assert(read("include/UI/Gen2NativePresentation.h").find("cyclePokerusState(uint8_t raw, uint8_t original)") ==
           std::string::npos);
    assert(unified.find("Gen2FieldSelector::pokerus(") == std::string::npos);
    for (const char* path : {"src/UI/Gen2UnifiedPokemonWorkspace.inc", "src/UI/Gen2PokemonEditorFoundation.inc",
                             "src/UI/Gen2HardwareFinalFix.inc"})
        assert(read(path).find("DV-derived") == std::string::npos);
    // Gender and Shiny are already A-in-place toggles on the STATS rows (no popup).
    assert(unified.find("if (focus.row == 5) { toggleShiny(screen, p); return true; }") != std::string::npos);
    assert(unified.find("chooseGender(screen, p);\n        return true;") != std::string::npos);
    std::cout << "Gen II inline Shiny / Pokerus / Gender PASS\n";
    return 0;
}
