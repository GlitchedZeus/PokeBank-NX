// Crystal Met Time is edited inline: A cycles the caught-time code in place (like Pokerus), no
// popup. The code is the 2-bit time field of Crystal's caught data (PK2 0x1D; pret/pokecrystal
// SetCaughtData, PKHeX PK2 CaughtData): 1 Morning, 2 Day, 3 Night, 0 unknown. Cycling wraps,
// skips times the recorded encounter disallows, and never touches the level / OT gender /
// location bits. Gold/Silver have no caught data and never show the field.
#include "fixtures/gsc_pokemon_fixture.h"
#include "UI/Gen2FieldSelector.h"
#include "UI/Gen2NativePresentation.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace {
namespace Sel = PokeBank::UIModel::Gen2FieldSelector;
namespace Native = PokeBank::UIModel::Gen2Native;

constexpr uint8_t bit(uint8_t code) { return static_cast<uint8_t>(1u << code); }

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in && "run from the repository root");
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void cycles() {
    // Pending (no caught history yet): Unknown -> Morning -> Day -> Night -> Unknown.
    static_assert(Sel::nextMetTime(0, false, 0) == 1);
    static_assert(Sel::nextMetTime(1, false, 0) == 2);
    static_assert(Sel::nextMetTime(2, false, 0) == 3);
    static_assert(Sel::nextMetTime(3, false, 0) == 0);
    // Recorded, every time allowed: Morning -> Day -> Night -> Morning (Unknown is never offered).
    constexpr uint8_t all = bit(1) | bit(2) | bit(3);
    static_assert(Sel::nextMetTime(1, true, all) == 2);
    static_assert(Sel::nextMetTime(2, true, all) == 3);
    static_assert(Sel::nextMetTime(3, true, all) == 1);
    // Recorded, encounter allows Day and Night only: the disallowed Morning is skipped.
    constexpr uint8_t dayNight = bit(2) | bit(3);
    static_assert(Sel::nextMetTime(2, true, dayNight) == 3);
    static_assert(Sel::nextMetTime(3, true, dayNight) == 2);
    static_assert(Sel::nextMetTime(1, true, dayNight) == 2); // a disallowed current moves to the next allowed
    // A single allowed time stays put; no allowed time keeps the stored value.
    static_assert(Sel::nextMetTime(3, true, bit(3)) == 3);
    static_assert(Sel::nextMetTime(2, true, 0) == 2);
    // Every result is a choice the selector itself offers (same labels as the old popup).
    for (uint8_t code = 0; code <= 3; ++code) {
        const uint8_t next = Sel::nextMetTime(code, false, 0);
        const auto s = Sel::metTime(next, false, 0);
        assert(std::string(s.choices[static_cast<std::size_t>(s.row)].label) == Native::crystalMetTimeName(next));
    }
}

void stagedExportRoundTrip() {
    // Recorded caught data: level 12, OT female, location 33, time Day.
    Native::CrystalCaughtData caught{true, 2, 12, true, 33};
    const uint16_t original = Native::encodeCrystalCaughtData(caught);
    constexpr uint8_t all = bit(1) | bit(2) | bit(3);
    uint8_t time = caught.timeOfDay;
    for (int step = 0; step < 4; ++step) {
        time = Sel::nextMetTime(time, true, all);
        auto next = Native::decodeCrystalCaughtData(original);
        next.timeOfDay = time;
        const uint16_t staged = Native::encodeCrystalCaughtData(next);
        // Only the two time bits (14-15) change.
        assert((staged & 0x3FFF) == (original & 0x3FFF));
        assert(((staged >> 14) & 3) == time);

        auto raw = fixture(C, true);
        const auto source = raw;
        auto parsed = parse(raw, SourceGame::Crystal);
        assert(parsed);
        std::string error;
        auto editor = StagedEditor::create(*parsed.save, error);
        assert(editor && error.empty());
        BoxPokemonEdit edit;
        edit.caughtData = staged;
        assert(editor->stageBoxPokemonEdit(0, 0, edit, error));
        const auto out = editor->finalizedBytes(error);
        assert(!out.empty() && error.empty());
        auto reopened = parse(out, SourceGame::Crystal);
        assert(reopened);
        auto reopenedEditor = StagedEditor::create(*reopened.save, error);
        assert(reopenedEditor && error.empty());
        const auto back = reopenedEditor->boxedPokemon(0, 0, error);
        assert(back && back->caughtData == staged);
        const auto decoded = Native::decodeCrystalCaughtData(back->caughtData);
        assert(decoded.timeOfDay == time && decoded.levelCode == 12 && decoded.originalTrainerFemale &&
               decoded.location == 33);
        assert(raw == source); // the source save is never written
    }
    assert(time == 3); // Day -> Night -> Morning -> Day -> Night
}

void productionRoute() {
    const auto unified = read("src/UI/Gen2UnifiedPokemonWorkspace.inc");
    const auto row = unified.find("if (focus.row == 9) {");
    assert(row != std::string::npos);
    const auto block = unified.substr(row, 900);
    // Inline: computes the next value and applies it; no selector popup is opened.
    assert(block.find("Selector::nextMetTime(") != std::string::npos);
    assert(block.find("applyFieldSelector(screen, state, Selector::Field::MetTime, next)") != std::string::npos);
    assert(block.find(".selector =") == std::string::npos);
    // Only Crystal reaches the caught-history rows; View never reaches field activation.
    const auto gate = unified.rfind("if (focus.row >= 9 && screen.sourceGameId == \"crystal_gbc\")", row);
    assert(gate != std::string::npos && row - gate < 600);
}
} // namespace

int main() {
    cycles();
    productionRoute();
    stagedExportRoundTrip();
    std::cout << "Crystal Met Time inline cycle, encoding and staged/export round trip: PASS\n";
    return 0;
}
