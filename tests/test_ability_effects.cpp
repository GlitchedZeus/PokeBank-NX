// NEW-13 (Round 3): the Ability picker shows ability names and short effects (UI/AbilityEffects.h)
// instead of "Slot 1 / Slot 2" in Generation III, IV and V. Host-only.
#include "UI/AbilityEffects.h"
#include "UI/SharedMenuGeometry.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <string>

// Trainer::getAbilityName forwards to Names::getAbilityName (src/Trainer/Trainer.cpp).
namespace Names { const char* getAbilityName(uint16_t abilityId); }   // src/Names/AbilityNames.cpp
namespace Trainer { inline const char* getAbilityName(uint16_t id) { return Names::getAbilityName(id); } }

namespace {
namespace AE = UI::AbilityEffects;
namespace G = PokeBank::UIModel::SharedMenuGeometry;

std::string read(const char* path) {
    std::ifstream in(path);
    assert(in && "source readable");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void testTableMatchesNativeIds() {
    // Every Gen III-V id carries its own name (no off-by-one) and an effect from its generation on.
    for (uint16_t id = 1; id <= AE::kLastGen5; ++id) {
        const auto& e = AE::kTable[id];
        assert(e.name == Trainer::getAbilityName(id));
        const int since = AE::introducedIn(id);
        for (int gen = 3; gen <= 5; ++gen) {
            const auto text = AE::effect(id, gen);
            assert(gen < since ? text.empty() : !text.empty());
            assert(text.size() <= 48);                        // one short line
            assert(text.find("Slot") == std::string_view::npos);
        }
    }
    assert(AE::introducedIn(76) == 3 && AE::introducedIn(77) == 4 && AE::introducedIn(123) == 4 &&
           AE::introducedIn(124) == 5 && AE::introducedIn(164) == 5 && AE::introducedIn(165) == 0);
    assert(AE::effect(0, 5).empty() && AE::effect(165, 5).empty());
    std::cout << "table matches native ids ok\n";
}

void testGenerationDifferences() {
    // Gen V changed these; Gen III-IV keep the earlier effect.
    assert(AE::effect(5, 4) != AE::effect(5, 5));            // Sturdy: OHKO only before Gen V
    assert(AE::effect(31, 3) != AE::effect(31, 5));          // Lightning Rod: no immunity before Gen V
    assert(AE::effect(114, 4) != AE::effect(114, 5));        // Storm Drain
    assert(AE::effect(24, 3).find("1/16") != std::string_view::npos && AE::effect(24, 5).find("1/8") != std::string_view::npos);
    assert(AE::effect(57, 3).find("Minus ally") != std::string_view::npos);   // Plus needs Minus before Gen V
    assert(AE::effect(1, 3) != AE::effect(1, 5));             // Stench
    assert(AE::effect(65, 3) == AE::effect(65, 5));           // Overgrow unchanged
    std::cout << "generation differences ok\n";
}

void testDrawbacksOnly() {
    std::set<uint16_t> flagged;
    for (uint16_t id = 1; id <= AE::kLastGen5; ++id)
        if (AE::drawback(id)) flagged.insert(id);
    // Truant, Stall, Klutz, Slow Start, Defeatist: the only red rows.
    assert((flagged == std::set<uint16_t>{54, 100, 103, 112, 129}));
    assert(AE::isDrawbackEffect(AE::effect(54, 3)) && !AE::isDrawbackEffect(AE::effect(65, 3)));
    assert(!AE::isDrawbackEffect(""));
    std::cout << "drawbacks only ok\n";
}

void testRowsFitTheWindow() {
    // Name (Body) + effect (Caption) fit one row of the wide two-column value list.
    const auto l = G::layout(G::MenuType::LocationPicker, 3);
    const int rowW = l.frame.w - 44;
    for (uint16_t id = 1; id <= AE::kLastGen5; ++id)
        for (int gen = AE::introducedIn(id); gen <= 5; ++gen) {
            const std::string name = std::string(Trainer::getAbilityName(id)) + (gen == 5 ? " (Hidden)" : "");
            const auto text = AE::effect(id, gen);
            assert(G::textWidth(name, G::Text::Body) + 24 + G::textWidth(text, G::Text::Caption) <= rowW - 32);
            const std::string row = AE::abilityEffectRow(name, id, gen);
            assert(AE::nameOf(row) == name && row.size() == name.size() + AE::kSplit.size() + text.size());
        }
    assert(AE::abilityEffectRow("Overgrow", 0, 3) == "Overgrow" && AE::nameOf("Overgrow") == "Overgrow");
    std::cout << "rows fit the window ok\n";
}

void testWiring() {
    const std::string gen3 = read("src/UI/Gen3SharedPokemonSurface.inc");
    const std::string gen4 = read("src/UI/Gen4SharedPokemonSurface.inc");
    const std::string gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    const std::string shared = read("src/UI/SharedPokemonEditorSurface.inc");
    assert(gen3.find("UI::AbilityEffects::abilityEffectRow(Trainer::getAbilityName(ability), ability, 3)") != std::string::npos);
    assert(gen3.find("\"Slot \" + std::to_string(value) + \" - \"") == std::string::npos);
    assert(gen4.find("UI::AbilityEffects::abilityEffectRow(Trainer::getAbilityName(abilityId), abilityId, 4)") != std::string::npos);
    assert(gen4.find("\" (Slot \" + std::to_string(value + 1)") == std::string::npos);
    assert(gen4.find("\"Ability Slot \"") == std::string::npos);
    assert(gen5.find("UI::AbilityEffects::abilityEffectRow(name, ability, 5)") != std::string::npos);
    assert(gen5.find("\" (Slot 2)\"") == std::string::npos && gen5.find("\" (Slot 1)\"") == std::string::npos);
    assert(gen5.find("\"Ability -> \" + UI::AbilityEffects::nameOf(") != std::string::npos);   // review row: name only
    for (const std::string* src : {&gen3, &shared}) {
        assert(src->find("target == PickerTarget::MetLocation || target == PickerTarget::Ability") != std::string::npos);
        assert(src->find("UI::AbilityEffects::isDrawbackEffect(effect) ? Colors::Error") != std::string::npos);
    }
    std::cout << "Gen III / IV / V ability picker wiring ok\n";
}
} // namespace

int main() {
    testTableMatchesNativeIds();
    testGenerationDifferences();
    testDrawbacksOnly();
    testRowsFitTheWindow();
    testWiring();
    std::cout << "test_ability_effects: all passed\n";
    return 0;
}
