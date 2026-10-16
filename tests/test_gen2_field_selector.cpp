// Divergence item 5: Gen II Pokerus, Crystal Met Time and Crystal OT Gender use a selector
// (open on the current value, Up/Down, A/tap apply, B cancel, focus kept) instead of cycling in
// place. The choices are exactly the native values the previous cycling produced.
#include "UI/Gen2FieldSelector.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace S = PokeBank::UIModel::Gen2FieldSelector;
namespace Native = PokeBank::UIModel::Gen2Native;
using C = Native::CrystalOtGenderChoice;

namespace {
std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void has(const std::string& t, const char* n) {
    if (t.find(n) == std::string::npos) std::cerr << "Missing: " << n << '\n';
    assert(t.find(n) != std::string::npos);
}
void lacks(const std::string& t, const char* n) {
    if (t.find(n) != std::string::npos) std::cerr << "Forbidden: " << n << '\n';
    assert(t.find(n) == std::string::npos);
}
} // namespace

int main() {
    // Pokerus: the same three values the old cycle produced, opened on the current one.
    {
        auto s = S::pokerus(0);
        assert(s.open() && s.count == 3 && s.row == 0);
        assert(s.choices[1].value == Native::cyclePokerusState(0));          // None -> Infected
        const uint8_t infected = 0x3A;                                          // strain 3, 10 days
        s = S::pokerus(infected);
        assert(s.row == 1 && s.choices[1].value == infected);                 // keeps the raw byte
        assert(s.choices[2].value == Native::cyclePokerusState(infected));   // Infected -> Cured
        s = S::pokerus(0x30);                                                   // cured strain 3
        assert(s.row == 2 && s.choices[2].value == 0x30);
        // Navigation moves; B cancels with no value; A applies the highlighted value.
        auto r = S::step(s, true, false, false, false);
        assert(r.outcome == S::Outcome::Moved && s.row == 1 && s.open());
        r = S::step(s, false, false, false, true);
        assert(r.outcome == S::Outcome::Cancelled && !s.open());
        s = S::pokerus(0);
        r = S::step(s, false, true, false, false);
        r = S::step(s, false, false, true, false);
        assert(r.outcome == S::Outcome::Apply && r.value == Native::encodePokerus(1, 1) && !s.open());
        // A tap selects and applies that row.
        s = S::pokerus(0x3A);
        r = S::step(s, false, false, false, false, 0);
        assert(r.outcome == S::Outcome::Apply && r.value == 0);
        // Closed selector does nothing.
        assert(S::step(s, true, true, true, true).outcome == S::Outcome::NotOpen);
    }
    // Met Time: recorded history offers only the encounter's allowed times; disabled rows are
    // skipped by navigation and never apply; unrecorded pending values may be Unknown.
    {
        auto s = S::metTime(2, true, static_cast<uint8_t>(1u << 2 | 1u << 3)); // Day, Night allowed
        assert(s.count == 3 && !s.choices[0].enabled && s.choices[0].value == 1);
        assert(s.choices[s.row].value == 2);
        auto r = S::step(s, false, true, false, false);
        assert(s.choices[s.row].value == 3);
        r = S::step(s, false, true, false, false);                // wraps past disabled Morning
        assert(s.choices[s.row].value == 2);
        r = S::step(s, false, false, false, false, 0);            // tap on disabled Morning
        assert(r.outcome == S::Outcome::Swallowed && s.open());
        s = S::metTime(0, false, 0);
        assert(s.count == 4 && s.choices[0].value == 0 && s.row == 0);
        for (int i = 0; i < 4; ++i) assert(s.choices[static_cast<std::size_t>(i)].enabled);
        r = S::step(s, false, false, false, false, 3);
        assert(r.outcome == S::Outcome::Apply && r.value == 3);
    }
    // OT Gender: recorded = Male/Female; unrecorded adds Unknown.
    {
        auto s = S::otGender(C::Female, true);
        assert(s.count == 2 && s.choices[s.row].value == static_cast<uint8_t>(C::Female));
        auto r = S::step(s, true, false, true, false);            // A applies the current row first
        assert(r.outcome == S::Outcome::Apply && r.value == static_cast<uint8_t>(C::Female));
        s = S::otGender(C::Unknown, false);
        assert(s.count == 3 && s.row == 0);
        r = S::step(s, false, false, false, false, 2);
        assert(r.outcome == S::Outcome::Apply && r.value == static_cast<uint8_t>(C::Female));
    }

    // Live route: activation opens the selector (no in-place cycling); the selector owns input
    // ahead of the re-roll prompt, field taps and B exit; apply re-encodes caught data only when
    // history is recorded.
    const auto unified = read("src/UI/Gen2UnifiedPokemonWorkspace.inc");
    has(unified, "selector = PokeBank::UIModel::Gen2FieldSelector::pokerus(p.pokerus);");
    has(unified, "PokeBank::UIModel::Gen2FieldSelector::metTime(state.crystalMetTime, false, 0)");
    has(unified, "PokeBank::UIModel::Gen2FieldSelector::otGender(state.crystalOtGender, caught.present)");
    lacks(unified, "cyclePokerusState(");
    lacks(unified, "cycleUnrecordedCrystalOtGender(");
    lacks(unified, "cycleRecordedCrystalOtGender(");
    lacks(unified, "(state.crystalMetTime + 1) % 4");
    const auto editable = unified.substr(unified.find("return handleUnifiedMoveEditor(screen, state, down, held, stickX, stickY);"));
    const auto gate = editable.find("if (layer == Layers::Layer::FieldSelector) {");
    assert(gate != std::string::npos);
    assert(gate < editable.find("Reroll::step(unified.reroll,"));
    assert(gate < editable.find("const int touchedDetail = updateUnifiedDetailsTouch("));
    assert(gate < editable.find("if (down & HidNpadButton_B) {"));
    const auto apply = unified.substr(unified.find("void applyFieldSelector("),
        unified.find("void drawFieldSelector(") - unified.find("void applyFieldSelector("));
    assert(apply.find("if (!caught.present) return;") < apply.find("p.caughtData = Gen2Native::encodeCrystalCaughtData(caught);"));
    const auto hw = read("src/UI/Gen2HardwareWorkspaceFix.inc");
    size_t guards = 0;
    for (auto at = hw.find("unified.selector.open()"); at != std::string::npos; at = hw.find("unified.selector.open()", at + 1)) ++guards;
    assert(guards == 2);
    has(read("src/UI/Gen2HardwareFinalFix.inc"), "if (unified.selector.open()) drawFieldSelector(screen, fb, unified.selector);");

    std::cout << "Gen II Pokerus / Met Time / OT Gender selectors: PASS\n";
}
