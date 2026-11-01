// Round 11 shared species-change / Create-draft policy (owner H25, H28, H07, Extra 5 / Extra 6):
//  - SpeciesChangeHistoryPolicy: Create resets Pokerus and a Met Location no longer valid for the
//    new species (a valid ancestry location stays); Edit keeps the existing record's history.
//  - SpeciesChangeMoves keepLearnable / writers: a species change never adds a move, keeps each
//    learnable move with its own PP and PP Ups, clears the rest, and a blank draft stays blank.
//  - CreateRequiredFields: Met Location in Gen III-V only, at least one move everywhere, one
//    message naming everything missing; no move-count rule.
// The per-generation wiring is covered by tests/test_species_change_moves_gen1.cpp ... _gen5.cpp
// and tests/test_gen4_create_species_change_history.cpp.
#include "UI/CreateRequiredFields.h"
#include "UI/HistoryConsistency.h"
#include "UI/SpeciesChangeHistoryPolicy.h"
#include "UI/SpeciesChangeMoves.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <vector>

namespace {

namespace H = PokeBank::UIModel::SpeciesChangeHistoryPolicy;
namespace R = PokeBank::UIModel::SpeciesMoveDefaults;
namespace C = PokeBank::UIModel::CreateRequiredFields;

int failures = 0;
void require(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
    assert(ok);
}

void historyPolicy() {
    int asked = 0;
    const auto valid = [&] { ++asked; return true; };
    const auto invalid = [&] { ++asked; return false; };

    // Create: Pokerus always resets; a chosen location is kept only while still valid.
    auto d = H::reconcile(H::Mode::Create, true, invalid);
    require(d.resetPokerus && d.resetMetLocation, "Create: Pikachu Trophy Garden -> Arbok resets Met and Pokerus");
    d = H::reconcile(H::Mode::Create, true, valid);
    require(d.resetPokerus && !d.resetMetLocation, "Create: a valid (ancestry) location is kept");
    asked = 0;
    d = H::reconcile(H::Mode::Create, false, invalid);
    require(d.resetPokerus && !d.resetMetLocation && asked == 0,
            "Create: no chosen location -> nothing to judge, Pokerus still resets");

    // Edit: an existing Pokemon's own history is never cleared, whatever the tables say.
    asked = 0;
    for (const bool chosen : {false, true}) {
        d = H::reconcile(H::Mode::Edit, chosen, invalid);
        require(!d.resetPokerus && !d.resetMetLocation, "Edit keeps Met Location and Pokerus");
    }
    require(asked == 0, "Edit never consults the wild tables");
    // Round 11 follow-up (updated pin): the Create reset is visible and names what was reset; the old
    // "choose a new legitimate encounter" read as a legality claim the wild tables cannot make.
    require(std::string(H::kChooseEncounterStatus) == "Species changed: Met Location needs a new choice",
            "the Met Location reset status");
    require(H::resetStatus(true, true) == "Species changed: Pokérus cleared and Met Location needs a new choice",
            "Pokerus and Met Location reset named together");
    require(H::resetStatus(false, true) == H::kChooseEncounterStatus, "Met Location only");
    require(H::resetStatus(true, false) == "Species changed: Pokérus cleared", "Pokerus only");
    require(H::resetStatus(false, false).empty(), "nothing reset: no status (never a silent claim)");
    require(H::resetStatus(true, true, false) == "Species changed: Pokérus cleared and Met Location cleared",
            "Crystal caught data is optional: cleared, not demanded");
    for (const bool p : {false, true})
        for (const bool m : {false, true})
            for (const bool required : {false, true}) {
                const auto text = H::resetStatus(p, m, required);
                require(text.find("legal") == std::string::npos && text.find("illegal") == std::string::npos,
                        "a reset status never makes a legality claim");
            }
}

// The pure H25 history check (UI/HistoryConsistency.h): flags only, never a verdict.
void historyConsistency() {
    namespace HC = PokeBank::UIModel::HistoryConsistency;
    HC::Facts f;
    f.species = "Arbok";
    require(HC::flags(f).empty(), "nothing recorded, nothing judged");
    f.metLocationRecorded = true;
    f.originKnown = true;
    f.catchDataAvailable = true;
    f.listedCatch = false;
    auto out = HC::flags(f);
    require(out.size() == 1 && out[0] == "Met Location is not a listed catch for Arbok (history kept)",
            "the owner's Trophy Garden Arbok: not a listed catch, kept");
    f.listedCatch = true;
    require(HC::flags(f).empty(), "a listed catch (species or ancestry) is not flagged");
    f.listedCatch = false;
    f.marker = true;
    require(HC::flags(f).empty(), "a trade / egg / event marker is not a catch claim");
    f.marker = false;
    f.catchDataAvailable = false;
    require(HC::flags(f).empty(), "no catch data for the origin: nothing to compare");
    f.catchDataAvailable = true;
    f.nativeToOrigin = false;
    out = HC::flags(f);
    require(out.size() == 1 && out[0] == "Origin game has no record of this Met Location (history kept)",
            "a location outside the origin game's own list (and no catch claim on top)");
    f.originKnown = false;
    require(HC::flags(f).size() == 1 && HC::flags(f)[0].find("not a listed catch") != std::string::npos,
            "an unknown origin list is never judged as foreign");
    f = {};
    f.species = "Raichu";
    f.levelsKnown = true;
    f.metLevel = 30;
    f.level = 22;
    out = HC::flags(f);
    require(out.size() == 1 && out[0] == "Met Level is above the current level (history kept)", "met level above level");
    f.metLevel = 0;
    require(HC::flags(f).empty(), "met level 0 (hatched / not recorded) is not judged");
    f.metLevel = 22;
    require(HC::flags(f).empty(), "equal levels are fine");
    f.metLevel = 30;
    f.levelsKnown = false;
    require(HC::flags(f).empty(), "unknown levels are not judged");
    // Several flags join into one caution line, in a fixed order.
    f = {};
    f.species = "Arbok";
    f.metLocationRecorded = f.originKnown = f.catchDataAvailable = f.levelsKnown = true;
    f.metLevel = 40;
    f.level = 22;
    const auto line = HC::joined(HC::flags(f));
    require(line == "Met Location is not a listed catch for Arbok (history kept); "
                    "Met Level is above the current level (history kept)", "joined: " + line);
    require(HC::joined({}).empty(), "no flags: no line");
    require(std::string(HC::kCautionLabel) == "Caution", "caution label");
    // Wording: suspect / kept as recorded, never a legality verdict.
    for (const auto& text : {HC::notListedCatch("Pikachu"), std::string(HC::kNotNativeToOrigin),
                             std::string(HC::kMetLevelAboveLevel)}) {
        require(text.find("legal") == std::string::npos && text.find("Illegal") == std::string::npos,
                "no legality wording: " + text);
        require(text.find("(history kept)") != std::string::npos, "says the history was kept: " + text);
    }
}

struct FakeGen4 {
    std::array<uint16_t, 4> moves{9, 9, 9, 9};
    std::array<uint8_t, 4> pp{1, 1, 1, 1}, ups{2, 2, 2, 2};
    int refuseAt = -1;
    bool setMove(std::size_t slot, uint16_t move) {
        if (static_cast<int>(slot) == refuseAt) return false;
        moves[slot] = move; pp[slot] = move ? 40 : 0; ups[slot] = 0;   // native: base PP, no Ups
        return true;
    }
    bool setPP(std::size_t slot, uint8_t v) { pp[slot] = v; return true; }
    bool setPPUps(std::size_t slot, uint8_t v) { ups[slot] = v; return true; }
};
struct MoveOnlyGen4 {
    std::array<uint16_t, 4> moves{9, 9, 9, 9};
    bool setMove(std::size_t slot, uint16_t move) { moves[slot] = move; return true; }
};
enum class FakeField { Move, Ups, PP };
struct FakeGen5 {
    std::vector<std::tuple<FakeField, std::size_t, uint32_t>> log;
    bool stage(FakeField f, std::size_t slot, uint32_t value, std::string&) {
        log.emplace_back(f, slot, value);
        return true;
    }
};
struct FakeEdit {
    std::optional<std::array<uint8_t, 4>> moves, pp, ppUps;
};

void movePolicy() {
    // Pikachu (Thunder Shock 84, Growl 45, Quick Attack 98, Thunder 87) -> a species that learns
    // only Growl and Thunder: those two are kept with their own PP / PP Ups and move up.
    const std::set<uint16_t> learns{45, 87};
    const auto canLearn = [&](uint16_t m) { return learns.count(m) != 0; };
    const std::array<uint8_t, 4> moves{84, 45, 98, 87}, pp{30, 12, 30, 7}, ups{1, 3, 0, 2};
    const auto s = R::keepLearnable(moves, pp, ups, canLearn);
    require(s.moves == (std::array<uint16_t, 4>{45, 87, 0, 0}), "incompatible moves cleared, compatible kept and compacted");
    require(s.pp == (std::array<uint8_t, 4>{12, 7, 0, 0}) && s.ppUps == (std::array<uint8_t, 4>{3, 2, 0, 0}),
            "kept moves keep their PP and PP Ups; empty slots 0/0");

    // A blank Create draft stays blank, whatever the new species could learn: nothing is added.
    const std::array<uint16_t, 4> none{};
    const std::array<uint8_t, 4> zero{};
    const auto blank = R::keepLearnable(none, zero, zero, [](uint16_t) { return true; });
    require(blank.moves == none && blank.pp == zero && blank.ppUps == zero, "blank stays blank");
    // Nothing learnable: every slot cleared (the old species' moves never carry over).
    const auto cleared = R::keepLearnable(moves, pp, ups, [](uint16_t) { return false; });
    require(cleared.moves == none, "no learnable move: all four slots empty");

    std::array<uint8_t, 4> m8 = moves, p8 = pp, u8 = ups;
    R::refreshArrays(m8, p8, u8, canLearn);
    require(m8 == (std::array<uint8_t, 4>{45, 87, 0, 0}) && p8 == (std::array<uint8_t, 4>{12, 7, 0, 0}) &&
            u8 == (std::array<uint8_t, 4>{3, 2, 0, 0}), "array writer applies the same slots");

    FakeEdit edit;
    R::writeEdit(s, edit);
    require(edit.moves && edit.pp && edit.ppUps && *edit.moves == m8 && *edit.pp == p8 && *edit.ppUps == u8,
            "edit request carries all three arrays");

    FakeGen4 g4;
    require(R::setMoves(g4, s) && g4.moves == s.moves && g4.pp == s.pp && g4.ups == s.ppUps,
            "Gen IV writer: move, then the kept PP Ups and PP");
    MoveOnlyGen4 bare;
    require(R::setMoves(bare, s) && bare.moves == s.moves, "a move-only record still gets every slot");
    FakeGen4 refusing;
    refusing.refuseAt = 2;
    require(!R::setMoves(refusing, s), "Gen IV writer reports a refusal (caller rolls back)");

    FakeGen5 g5;
    std::string error;
    require(R::stageMoves<FakeField::Move, FakeField::Ups, FakeField::PP>(g5, s, error), "Gen V stager");
    require(g5.log.size() == 12, "Gen V: three stages per slot");
    for (std::size_t slot = 0; slot < 4; ++slot) {
        const auto& [f0, s0, v0] = g5.log[slot * 3];
        const auto& [f1, s1, v1] = g5.log[slot * 3 + 1];
        const auto& [f2, s2, v2] = g5.log[slot * 3 + 2];
        require(f0 == FakeField::Move && f1 == FakeField::Ups && f2 == FakeField::PP &&
                s0 == slot && s1 == slot && s2 == slot, "Gen V replay order Move, PP Ups, PP");
        require(v0 == s.moves[slot] && v1 == s.ppUps[slot] && v2 == s.pp[slot], "Gen V staged values");
    }
}

void requiredFields() {
    const std::array<uint16_t, 4> none{}, one{33, 0, 0, 0}, last{0, 0, 0, 45};
    require(!C::hasAnyMove(none) && C::hasAnyMove(one) && C::hasAnyMove(last), "any slot counts");
    for (int gen = 1; gen <= 5; ++gen) {
        const bool met = gen >= 3;
        require(C::metLocationRequired(gen) == met, "Met Location required only in Gen III-V");
        const auto blank = C::forCreate(gen, false, none);
        require(blank.move && blank.metLocation == met, "blank draft: move (+ Met in III-V) missing");
        require(!C::forCreate(gen, true, one).any(), "chosen Met and one move: nothing missing");
        // No move-count rule: one move is enough at any level.
        require(!C::forCreate(gen, true, last).any(), "a single move in slot 4 is enough");
    }
    require(!C::forCreate(1, false, one).any() && !C::forCreate(2, false, one).any(),
            "Gen I / Gen II never need a Met Location (Crystal caught data stays optional)");
    require(C::message(C::forCreate(5, false, one)) == "Choose a Met Location before Keep",
            "Gen V H07 message unchanged");
    require(C::message(C::forCreate(4, true, none)) == "Choose at least one move before Keep", "move only");
    require(C::message(C::forCreate(3, false, none)) == "Choose a Met Location and at least one move before Keep",
            "one message names both");
    require(C::message(C::forCreate(1, false, none), "Add") == "Choose at least one move before Add", "Gen I/II wording");
    require(C::message({}).empty(), "nothing missing: no message");

    // Edit: an existing Pokemon is refused only when this session took its last move away.
    require(C::forEdit(one, none).move, "species change left no move: Keep refused");
    require(!C::forEdit(none, none).any(), "a stored moveless record is not blamed for its own history");
    require(!C::forEdit(one, last).any(), "a changed moveset with a move is fine");
}

} // namespace

int main() {
    historyPolicy();
    historyConsistency();
    movePolicy();
    requiredFields();
    require(failures == 0, "all Round 11 species/Create policy checks");
    std::cout << "Round 11 species-change history, move and Create required-field policy: PASS\n";
    return 0;
}
