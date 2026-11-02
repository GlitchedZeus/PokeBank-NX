// Gen I / Gen II Y "Random DVs" (owner campaign phase 6): each press re-rolls the draft at once,
// with no prompt, and keeps the shiny state, the Gen II gender (Attack DV vs. the real species
// ratio from Gen2PersonalData) and the Unown letter. The live Gen I/II workspaces call it directly.
#include "UI/Gen12DVReroll.h"
#include "Integration/Gen2/Gen2PersonalData.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <string>

namespace {

namespace R = PokeBank::UIModel::Gen12DVReroll;
namespace Gen2 = PokeVault::Integration::Gen2;

int genderCode(uint16_t species, uint8_t attack) {
    return static_cast<int>(Gen2::genderFromAttackDV(species, attack));
}

// 200 presses from `start`: every press changes the DVs and keeps shiny / gender / letter.
void rerollKeeps(uint16_t species, R::DVs start, uint32_t seed, std::size_t minDistinct) {
    const auto genderOf = [species](uint8_t attack) { return genderCode(species, attack); };
    const bool shiny = R::isShiny(start);
    const int gender = genderOf(start[0]);
    const int letter = R::unownLetter(start);
    std::set<uint32_t> seen;
    R::DVs dvs = start;
    for (int press = 0; press < 200; ++press) {
        const auto result = R::reroll(dvs, seed, genderOf, species == 201);
        assert(result.changed && result.dvs != dvs); // a new roll every press
        for (const auto v : result.dvs) assert(v <= 15);
        assert(R::isShiny(result.dvs) == shiny);
        assert(genderOf(result.dvs[0]) == gender);
        if (species == 201) assert(R::unownLetter(result.dvs) == letter);
        dvs = result.dvs;
        seen.insert(static_cast<uint32_t>(dvs[0]) << 12 | dvs[1] << 8 | dvs[2] << 4 | dvs[3]);
    }
    if (seen.size() < minDistinct)
        std::cerr << "species " << species << " shiny " << shiny << ": only " << seen.size() << " distinct\n";
    assert(seen.size() >= minDistinct);
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void has(const std::string& text, const char* needle) {
    if (text.find(needle) == std::string::npos) std::cerr << "Missing: " << needle << '\n';
    assert(text.find(needle) != std::string::npos);
}
void lacks(const std::string& text, const char* needle) {
    if (text.find(needle) != std::string::npos) std::cerr << "Unexpected: " << needle << '\n';
    assert(text.find(needle) == std::string::npos);
}

} // namespace

int main() {
    // The shiny rule is the Gen1Shiny / pokecrystal CheckShininess pattern.
    for (const uint8_t a : R::kShinyAttack) assert(R::isShiny({a, 10, 10, 10}));
    assert(!R::isShiny({1, 10, 10, 10}) && !R::isShiny({2, 10, 10, 9}));

    // Gen II gender ratios (Gen2PersonalData): 1:7 Eevee (31), 1:1 Pikachu (127), 3:1 Growlithe
    // (63), 7:1-female Clefairy (191), male-only Tauros (0), female-only Chansey (254),
    // genderless Magnemite (255).
    struct Case { uint16_t species; R::DVs start; std::size_t distinct; };
    const Case cases[] = {
        {133, {12, 7, 4, 9}, 50},  // 1:7, male, non-shiny
        {133, {1, 7, 4, 9}, 50},   // 1:7, female (Attack 0-1), non-shiny
        {133, {2, 10, 10, 10}, 7}, // 1:7, male shiny: 7 other shiny Attack values
        {25, {8, 3, 3, 3}, 50},    // 1:1 male
        {25, {7, 3, 3, 3}, 50},    // 1:1 female
        {25, {10, 10, 10, 10}, 3}, // 1:1 male shiny: Attack 10, 11, 14, 15
        {25, {2, 10, 10, 10}, 3},  // 1:1 female shiny: Attack 2, 3, 6, 7
        {58, {4, 1, 1, 1}, 50},    // 3:1 male
        {58, {3, 1, 1, 1}, 50},    // 3:1 female
        {58, {6, 10, 10, 10}, 5},  // 3:1 male shiny: 6, 7, 10, 11, 14, 15
        {35, {12, 5, 5, 5}, 50},   // 7:1 female-heavy, male (Attack 12-15)
        {35, {11, 5, 5, 5}, 50},   // 7:1, female
        {35, {14, 10, 10, 10}, 1}, // 7:1 male shiny: 14 <-> 15 only
        {35, {3, 10, 10, 10}, 5},  // 7:1 female shiny: 2, 3, 6, 7, 10, 11
        {128, {5, 5, 5, 5}, 50},   // male-only
        {113, {5, 5, 5, 5}, 50},   // female-only
        {81, {5, 5, 5, 5}, 50},    // genderless
        {81, {7, 10, 10, 10}, 7},  // genderless shiny: all 8 shiny Attack values
        {201, {4, 9, 1, 12}, 20},  // Unown: letter kept
        {201, {2, 10, 10, 10}, 3}, // Unown shiny: Attack 2, 3, 10, 11 share the letter
    };
    uint32_t seed = 0xC0FFEEu;
    for (const auto& c : cases) rerollKeeps(c.species, c.start, seed, c.distinct);

    // 1:7 female cannot be shiny (shiny Attack is >= 2): a female-shiny start does not exist,
    // and asking a shiny male to stay male never yields a female.
    assert(genderCode(133, 2) == static_cast<int>(Gen2::PokemonGender::Male));

    // No alternative: the DVs stay and `changed` is false (the caller shows a message).
    const auto onlyItself = [](uint8_t attack) { return attack == 2 ? 0 : 1; };
    const auto stuck = R::reroll({2, 10, 10, 10}, seed, onlyItself);
    assert(!stuck.changed && stuck.dvs == (R::DVs{2, 10, 10, 10}));

    // Gen I: no gender; shiny state kept (Gen I stores no flag, shown under the Gen II rule).
    R::DVs gen1 = {9, 4, 12, 1};
    for (int press = 0; press < 200; ++press) {
        const auto r = R::reroll(gen1, seed);
        assert(r.changed && r.dvs != gen1 && !R::isShiny(r.dvs));
        gen1 = r.dvs;
    }
    gen1 = {3, 10, 10, 10};
    for (int press = 0; press < 200; ++press) {
        const auto r = R::reroll(gen1, seed);
        assert(r.changed && r.dvs != gen1 && R::isShiny(r.dvs));
        gen1 = r.dvs;
    }

    // Live routes: Y re-rolls immediately in both workspaces; the confirmation prompt is gone.
    const auto gen2 = read("src/UI/Gen2UnifiedPokemonWorkspace.inc");
    has(gen2, "if (down & HidNpadButton_Y) {\n        if (randomizeDVs(state))");
    const auto gen2Rules = read("src/UI/Gen2PokemonEditorFoundation.inc");
    has(gen2Rules, "Reroll::reroll(editableDVs(state.working), state.rng,");
    has(gen2Rules, "Gen2::genderFromAttackDV(species, attack)");
    has(gen2Rules, "species == 201);");
    const auto gen1Route = read("src/UI/Gen1PokemonEditorOverlayFoundation.inc");
    has(gen1Route, "if (editable && (down & HidNpadButton_Y)) {\n        // Y Random DVs re-rolls the draft at once");
    has(gen1Route, "PokeBank::UIModel::Gen12DVReroll::reroll(current, foundation.randomState);");
    for (const char* path : {"src/UI/Gen2UnifiedPokemonWorkspace.inc", "src/UI/Gen2HardwareFinalFix.inc",
                             "src/UI/Gen2HardwareWorkspaceFix.inc", "src/UI/Gen1PokemonEditorOverlayFoundation.inc",
                             "src/UI/Gen1PokemonEditorFoundationHardwareFix.inc"}) {
        const auto text = read(path);
        lacks(text, "Gen12RerollDVs");
        lacks(text, "reroll.confirming");
        lacks(text, "\"Re-roll\"");
    }
    std::cout << "Gen I/II Y Random DVs: immediate re-roll keeps shiny/gender/Unown letter PASS\n";
    return 0;
}
