// R2-BUG-004: a species change resets all four moves to the new species' native moves.
// Part 1 of the host coverage (data, algorithm, all 20 Gen I-V games, apply-path contracts).
// The per-generation record round trips are tests/test_species_change_moves_gen1.cpp ... _gen5.cpp.
//
// Sources behind the data (tools/gen_levelup_learnsets15.py, cross-checked at generation):
// pret pokered / pokeyellow / pokegold / pokecrystal / pokeruby / pokeemerald / pokefirered and
// PKHeX 6501f0ab lvlmove_* resources.
#include "Pokemon/SpeciesMoveDefaults.h"
#include "UI/SpeciesChangeMoves.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <tuple>
#include <vector>

namespace {

namespace M = PokeBank::Moves;
namespace R = PokeBank::UIModel::SpeciesMoveDefaults;
using T = M::LevelUpTable15;

int failures = 0;
void require(bool ok, const std::string& what) {
    if (!ok) { std::cerr << "FAILED: " << what << '\n'; ++failures; }
    assert(ok);
}

std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    require(static_cast<bool>(in), std::string("open ") + path);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

// Text of the function starting at `signature`, up to the next line that starts a sibling
// definition at the same indentation (good enough for these .inc / header functions).
std::string functionBody(const std::string& text, const std::string& signature) {
    const auto start = text.find(signature);
    require(start != std::string::npos, "found " + signature);
    int depth = 0;
    bool opened = false;
    for (std::size_t i = text.find('{', start + signature.size()); i < text.size(); ++i) {
        if (text[i] == '{') { ++depth; opened = true; }
        if (text[i] == '}' && --depth == 0 && opened) return text.substr(start, i - start + 1);
    }
    return text.substr(start);
}

// PKHeX Learnset.SetEncounterMoves, written as PKHeX does (ring buffer + rotation), as an
// independent reference for the shift-list implementation under test. Gen I level-0 rows (base
// moves) are included first, as pret WriteMonMoves seeds them before the level-up walk.
std::array<uint16_t, 4> pkhexReference(std::span<const uint32_t> rows, uint8_t level) {
    std::array<uint16_t, 4> moves{};
    int ctr = 0;
    for (const uint32_t entry : rows) {
        const auto [req, move] = M::decodeLevelUp(entry);
        if (req > level) continue;
        bool held = false;
        for (const auto m : moves) held = held || m == move;
        if (!held) moves[static_cast<std::size_t>((ctr++) & 3)] = move;
    }
    if (ctr > 4) {
        const int rotation = ctr % 4;
        for (int i = 0; i < rotation; ++i) {
            const auto first = moves[0];
            for (int j = 0; j < 3; ++j) moves[static_cast<std::size_t>(j)] = moves[static_cast<std::size_t>(j + 1)];
            moves[3] = first;
        }
    }
    return moves;
}

struct Game { const char* name; T table; uint16_t maxSpecies; };

// All 20 Gen I-V games, each routed through the selector the editor uses for it.
std::vector<Game> games() {
    return {
        {"Red", M::levelUpTableGen1(false), 151},
        {"Blue", M::levelUpTableGen1(false), 151},
        {"Yellow", M::levelUpTableGen1(true), 151},
        {"Gold", M::levelUpTableGen2(false), 251},
        {"Silver", M::levelUpTableGen2(false), 251},
        {"Crystal", M::levelUpTableGen2(true), 251},
        {"Ruby", M::levelUpTableForGen3Version(R::gen3VersionForSourceId("ruby_gba", 0)), 386},
        {"Sapphire", M::levelUpTableForGen3Version(R::gen3VersionForSourceId("sapphire_gba", 0)), 386},
        {"Emerald", M::levelUpTableForGen3Version(R::gen3VersionForSourceId("emerald_gba", 0)), 386},
        {"FireRed", M::levelUpTableForGen3Version(R::gen3VersionForSourceId("firered_gba", 0)), 386},
        {"LeafGreen", M::levelUpTableForGen3Version(R::gen3VersionForSourceId("leafgreen_gba", 0)), 386},
        {"Diamond", R::gen4TableForGroup(Enums::GameVersion::DP), 493},
        {"Pearl", R::gen4TableForGroup(Enums::GameVersion::DP), 493},
        {"Platinum", R::gen4TableForGroup(Enums::GameVersion::PT), 493},
        {"HeartGold", R::gen4TableForGroup(Enums::GameVersion::HGSS), 493},
        {"SoulSilver", R::gen4TableForGroup(Enums::GameVersion::HGSS), 493},
        {"Black", M::levelUpTableForGen45Version(21), 649},
        {"White", M::levelUpTableForGen45Version(20), 649},
        {"Black 2", M::levelUpTableForGen45Version(23), 649},
        {"White 2", M::levelUpTableForGen45Version(22), 649},
    };
}

void tableRouting() {
    const auto g = games();
    require(g.size() == 20, "20 Gen I-V games");
    const T expected[20] = {T::RedBlue, T::RedBlue, T::Yellow, T::GoldSilver, T::GoldSilver, T::Crystal,
                            T::RubySapphire, T::RubySapphire, T::Emerald, T::FireRed, T::LeafGreen,
                            T::DiamondPearl, T::DiamondPearl, T::Platinum, T::HeartGoldSoulSilver,
                            T::HeartGoldSoulSilver, T::BlackWhite, T::BlackWhite, T::Black2White2, T::Black2White2};
    for (std::size_t i = 0; i < 20; ++i) require(g[i].table == expected[i], std::string(g[i].name) + " table");
    require(R::gen5Table(false) == T::BlackWhite && R::gen5Table(true) == T::Black2White2, "Gen V table selector");
    // Gen IV / V origin bytes (PKHeX GameVersion) map to the same tables.
    require(M::levelUpTableForGen45Version(10) == T::DiamondPearl && M::levelUpTableForGen45Version(11) == T::DiamondPearl &&
            M::levelUpTableForGen45Version(12) == T::Platinum && M::levelUpTableForGen45Version(7) == T::HeartGoldSoulSilver &&
            M::levelUpTableForGen45Version(8) == T::HeartGoldSoulSilver, "Gen IV origin bytes");
    // Unknown games select no table at all.
    require(M::levelUpTableForGen3Version(0) == T::Count && M::levelUpTableForGen45Version(9) == T::Count &&
            R::gen4TableForGroup(Enums::GameVersion::FRLG) == T::Count, "unknown game: no table");
    require(R::gen3VersionForSourceId("not_a_game", 4) == 4, "Gen III falls back to the draft's own origin");
}

// Every species of every game has a learnset; the default moveset matches PKHeX's own
// algorithm at every level; moves are distinct, at most four, learnable at or below the level,
// and inside the generation's move range.
void everySpeciesEveryLevel() {
    constexpr uint16_t kMaxMove[] = {165, 165, 251, 251, 354, 354, 354, 354, 467, 467, 467, 559, 559};
    std::set<int> seenTables;
    for (const auto& g : games()) {
        if (!seenTables.insert(static_cast<int>(g.table)).second) continue;
        const uint16_t maxMove = kMaxMove[static_cast<int>(g.table)];
        for (uint16_t species = 1; species <= g.maxSpecies; ++species) {
            const auto rows = M::levelUpLearnset(g.table, species);
            require(!rows.empty(), std::string(g.name) + " species " + std::to_string(species) + " has a learnset");
            for (uint8_t level = 1; level <= 100; ++level) {
                const auto d = M::defaultMoves(g.table, species, 0, level);
                require(d.known, "known");
                require(d.moves == pkhexReference(rows, level),
                        std::string(g.name) + " species " + std::to_string(species) + " L" + std::to_string(level) +
                            ": default moveset == PKHeX SetEncounterMoves");
                std::set<uint16_t> distinct;
                for (std::size_t i = 0; i < 4; ++i) {
                    const auto move = d.moves[i];
                    if (i >= d.count) { require(move == 0, "empty tail"); continue; }
                    require(move >= 1 && move <= maxMove, "move in generation range");
                    require(distinct.insert(move).second, "no duplicate moves");
                    bool learnable = false;
                    for (const auto e : rows) {
                        const auto [req, m] = M::decodeLevelUp(e);
                        learnable = learnable || (m == move && req <= level);
                    }
                    require(learnable, "every default move is learned at or below the level");
                }
            }
        }
        // Past the dex: no row, no moves.
        const auto none = M::defaultMoves(g.table, static_cast<uint16_t>(g.maxSpecies + 1), 0, 50);
        require(!none.known && none.count == 0 && none.moves == (std::array<uint16_t, 4>{}),
                std::string(g.name) + ": species past the dex gets empty slots");
    }
    require(seenTables.size() == 13, "13 native tables cover 20 games");
}

std::array<uint16_t, 4> moves(T table, uint16_t species, uint8_t level, uint8_t form = 0) {
    return M::defaultMoves(table, species, form, level).moves;
}

// Native values read straight from the pinned sources.
void knownNativeMovesets() {
    // The owner's report: Yellow (and Red/Blue) Pikachu Lv5 -> Fearow Lv5.
    // pokeyellow / pokered base_stats/pikachu.asm: THUNDERSHOCK (84), GROWL (45).
    // base_stats/fearow.asm: PECK (64), GROWL (45), LEER (43); evos_moves Fearow learns LEER at 9.
    for (const T t : {T::Yellow, T::RedBlue}) {
        require(moves(t, 25, 5) == (std::array<uint16_t, 4>{84, 45, 0, 0}), "Gen I Pikachu Lv5 = Create default");
        require(moves(t, 22, 5) == (std::array<uint16_t, 4>{64, 45, 43, 0}), "Gen I Fearow Lv5: Peck, Growl, Leer");
    }
    // Yellow differs from Red/Blue: Pikachu learns TAIL_WHIP at 6 in Yellow (RB: THUNDER_WAVE 9).
    require(moves(T::Yellow, 25, 6) == (std::array<uint16_t, 4>{84, 45, 39, 0}), "Yellow Pikachu Lv6 Tail Whip");
    require(moves(T::RedBlue, 25, 6) == (std::array<uint16_t, 4>{84, 45, 0, 0}), "Red/Blue Pikachu Lv6");
    // Gen II pokecrystal evos_attacks: Pikachu 1 THUNDERSHOCK, 1 GROWL, 6 TAIL_WHIP, 8 THUNDER_WAVE.
    require(moves(T::Crystal, 25, 8) == (std::array<uint16_t, 4>{84, 45, 39, 86}), "Crystal Pikachu Lv8");
    // Gen III pokeemerald Bulbasaur: 1 TACKLE, 4 GROWL, 7 LEECH_SEED, 10 VINE_WHIP ... 46 SOLAR_BEAM.
    require(moves(T::Emerald, 1, 10) == (std::array<uint16_t, 4>{33, 45, 73, 22}), "Emerald Bulbasaur Lv10");
    require(moves(T::Emerald, 1, 100) == (std::array<uint16_t, 4>{230, 74, 235, 76}), "Emerald Bulbasaur Lv100");
    // pokefirered Dugtrio: the FIRERED and LEAFGREEN blocks order Scratch / Sand-Attack differently.
    require(moves(T::FireRed, 51, 1) == (std::array<uint16_t, 4>{161, 10, 28, 45}), "FireRed Dugtrio");
    require(moves(T::LeafGreen, 51, 1) == (std::array<uint16_t, 4>{161, 28, 10, 45}), "LeafGreen Dugtrio");
    // pokefirered Deoxys: FireRed (Attack) vs LeafGreen (Defense) level-up moves differ.
    require(moves(T::FireRed, 386, 50) != moves(T::LeafGreen, 386, 50), "FR/LG Deoxys differ");
    // Gen IV/V forms with their own learnsets (PKHeX personal FormStatsIndex).
    require(moves(T::Platinum, 386, 50, 1) != moves(T::Platinum, 386, 50, 0), "Pt Deoxys-Attack has its own moves");
    require(moves(T::Black2White2, 646, 75, 1) != moves(T::Black2White2, 646, 75, 0), "B2W2 White Kyurem");
    // A form with no row of its own falls back to the base form (Rotom's forms share a list).
    require(M::levelUpLearnset(T::Platinum, 479, 3).data() == M::levelUpLearnset(T::Platinum, 479, 0).data(),
            "Rotom form falls back to the base learnset");
}

// The shared policy: base PP, no PP Ups, empty slots without data, writers and stagers.
struct FakeGen4 {
    std::array<uint16_t, 4> moves{9, 9, 9, 9};
    int refuseAt = -1;
    bool setMove(std::size_t slot, uint16_t move) {
        if (static_cast<int>(slot) == refuseAt) return false;
        moves[slot] = move;
        return true;
    }
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

void sharedPolicy() {
    const auto basePP = [](uint16_t move) { return static_cast<uint8_t>(move % 40 + 5); };
    const auto s = R::forSpecies(T::Yellow, 22, 0, 5, basePP);
    require(s.known && s.moves == (std::array<uint16_t, 4>{64, 45, 43, 0}), "slots for Fearow");
    require(s.pp == (std::array<uint8_t, 4>{basePP(64), basePP(45), basePP(43), 0}), "base PP, empty slot 0 PP");
    require(s.ppUps == (std::array<uint8_t, 4>{}), "PP Ups 0");
    const auto none = R::forSpecies(T::Count, 22, 0, 5, basePP);
    require(!none.known && none.moves == (std::array<uint16_t, 4>{}) && none.pp == (std::array<uint8_t, 4>{}),
            "no table: all four slots empty, never kept");

    std::array<uint8_t, 4> m8{1, 2, 3, 4}, pp{9, 9, 9, 9}, ups{3, 3, 3, 3};
    R::writeArrays(s, m8, pp, ups);
    require(m8 == (std::array<uint8_t, 4>{64, 45, 43, 0}) && ups == (std::array<uint8_t, 4>{}) &&
            pp[3] == 0, "array writer replaces every slot");
    std::array<uint16_t, 4> m16{1, 2, 3, 4};
    require(R::refreshArrays(T::Count, 22, 0, 5, basePP, m16, pp, ups) == false &&
            m16 == (std::array<uint16_t, 4>{}), "refreshArrays without data clears the old moves");

    FakeEdit edit;
    R::writeEdit(s, edit);
    require(edit.moves && edit.pp && edit.ppUps && (*edit.moves)[0] == 64 && (*edit.ppUps)[0] == 0,
            "edit request carries all three arrays");

    FakeGen4 g4;
    require(R::setMoves(g4, s) && g4.moves == s.moves, "Gen IV writer sets all four slots, slot 4 empty");
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
        require(v0 == s.moves[slot] && v1 == 0 && v2 == s.pp[slot], "Gen V staged values");
    }
}

// The five apply paths call the shared policy, and only on a real species change.
void applyPathContracts() {
    const auto gen1 = read("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc");
    const auto g1 = functionBody(gen1, "bool ux3ApplySpeciesPicker(TrainerViewScreen& screen)");
    require(g1.find("Moves stay intact") == std::string::npos, "Gen I no longer keeps the old moves");
    require(g1.find("writeArrays(\n                ux3SpeciesChangeMoves(screen, species, state.draft.level)") != std::string::npos,
            "Gen I Add draft refreshes moves");
    require(g1.find("writeEdit(\n                ux3SpeciesChangeMoves(screen, species, *edit.level), edit)") != std::string::npos,
            "Gen I Edit refreshes moves");
    const auto g1Helper = functionBody(gen1, "PokeBank::UIModel::SpeciesMoveDefaults::Slots ux3SpeciesChangeMoves(");
    require(g1Helper.find("SpeciesMoveDefaults::gen1(editor(screen), species, level)") != std::string::npos,
            "Gen I ux3 uses the one Gen I binding");
    const auto gen1Binding = read("include/UI/Gen1SpeciesChangeMoves.h");
    require(gen1Binding.find("SourceGame::Yellow") != std::string::npos &&
            gen1Binding.find("levelUpTableGen1(yellow)") != std::string::npos &&
            gen1Binding.find("StagedPokemonEditor::moveBasePP(move)") != std::string::npos,
            "Gen I binding: Yellow vs Red/Blue table, Gen I base PP");
    // Every other Gen I species-apply path (QA CG19) goes through the same binding, only on a real change.
    const auto cleanup2 = read("src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc");
    const auto ux2 = functionBody(cleanup2, "bool ux2ApplySpeciesPicker(TrainerViewScreen& screen)");
    require(ux2.find("if (selected != state.draft.species)\n            PokeBank::UIModel::SpeciesMoveDefaults::writeArrays(PokeBank::UIModel::SpeciesMoveDefaults::gen1(editor(screen), selected, state.draft.level)") != std::string::npos &&
            ux2.find("if (pokemon && selected != pokemon->species)\n            PokeBank::UIModel::SpeciesMoveDefaults::writeEdit(PokeBank::UIModel::SpeciesMoveDefaults::gen1(editor(screen), selected, pokemon->level), edit);") != std::string::npos,
            "Gen I ux2ApplySpeciesPicker refreshes moves (Add and Edit)");
    for (const char* path : {"src/UI/Gen1PokemonEditorOverlayUX.inc", "src/UI/Gen1PokemonEditorOverlay.cpp"}) {
        const auto text = read(path);
        const auto apply = functionBody(text, "bool applyPicker(TrainerViewScreen& screen)");
        require(apply.find("if (species != state.draft.species)\n                PokeBank::UIModel::SpeciesMoveDefaults::writeArrays(\n                    PokeBank::UIModel::SpeciesMoveDefaults::gen1(e, species, state.draft.level),") != std::string::npos &&
                apply.find("if (*edit.species != pokemon->species)\n            PokeBank::UIModel::SpeciesMoveDefaults::writeEdit(\n                PokeBank::UIModel::SpeciesMoveDefaults::gen1(e, *edit.species, pokemon->level), edit);") != std::string::npos,
                std::string(path) + " applyPicker refreshes moves (Add and Edit)");
    }
    // Both Gen I refreshes sit inside the species-changed branches.
    require(g1.find("if (species != state.draft.species) {") < g1.find("writeArrays(") &&
            g1.find("if (species != pokemon->species) {") < g1.find("writeEdit("), "Gen I: same species changes nothing");

    const auto gen2 = read("include/UI/Gen2PokemonSession.h");
    const auto g2 = functionBody(gen2, "bool setSpecies(uint16_t species, std::string_view sourceGameId = {}) noexcept");
    require(g2.find("if (species == working.species) return true;") < g2.find("SpeciesMoveDefaults::refreshArrays("),
            "Gen II: same species returns before the refresh");
    require(g2.find("gen2MoveBasePP(move)") != std::string::npos && g2.find("levelUpTableGen2(") != std::string::npos,
            "Gen II: Gold/Silver vs Crystal table, Gen II base PP");

    const auto gen3 = read("include/UI/Gen3SharedPokemonSession.h");
    const auto g3 = functionBody(gen3, "bool setSpecies(uint16_t species, std::string_view sourceGameId = {}) noexcept");
    require(g3.find("if (species == working.species) return true;") < g3.find("SpeciesMoveDefaults::refreshArrays("),
            "Gen III: same species returns before the refresh");
    require(g3.find("gen3VersionForSourceId(sourceGameId, working.originGame)") != std::string::npos,
            "Gen III: exact game (FireRed / LeafGreen / Emerald / Ruby / Sapphire)");
    const auto gen3Surface = read("src/UI/Gen3SharedPokemonSurface.inc");
    require(gen3Surface.find("state.session.setSpecies(state.speciesPreview, screen.sourceGameId)") != std::string::npos,
            "Gen III picker applies through Session::setSpecies with the exact source id");

    const auto gen4 = read("src/UI/Gen4SharedPokemonSurface.inc");
    const auto g4 = functionBody(gen4, "bool applyGen4Picker(TrainerViewScreen& screen, State& state, PickerTarget target,");
    require(g4.find("if (ok && value != previousSpecies) {") < g4.find("PokeBank::UIModel::SpeciesMoveDefaults::setMoves(*state.session.working"),
            "Gen IV: refresh only on a real species change");
    require(g4.find("PokeBank::UIModel::SpeciesMoveDefaults::setMoves(") < g4.find("if (!ok) {\n            auto restored"),
            "Gen IV: a refused move write rolls the whole draft back");
    require(g4.find("Could not reconcile Species/Shiny while preserving PID-linked traits") != std::string::npos,
            "Gen IV rollback message kept");

    const auto gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    const auto g5 = functionBody(gen5, "static bool applySpecies(TrainerViewScreen& screen, EditorSurface::EditorState& state,");
    require(g5.find("PokeBank::UIModel::SpeciesMoveDefaults::stageMoves<Field::Move, Field::MovePPUps, Field::MovePP>(probe,") != std::string::npos,
            "Gen V edit stages the new moves on the probe in replay order");
    require(g5.find("if (species != p.species()) {\n            // R2-BUG-004") != std::string::npos,
            "Gen V: refresh only on a real species change");
    require(g5.find("backendState.recreateCreateSpecies(") != std::string::npos &&
            g5.find("backendState.speciesRefusal(species)") != std::string::npos, "Gen V pinned calls kept");
    const auto gen5State = read("include/Integration/Gen5/Gen5SharedScreenState.h");
    const auto rebuild = functionBody(gen5State, "[[nodiscard]] bool recreateCreateSpecies(Workspace& ws,uint32_t species,bool shiny,");
    require(rebuild.find("if(species!=p->species()) {") < rebuild.find("PokeBank::Moves::defaultMoves("),
            "Gen V Create rebuild: new species gets native moves; same species keeps the draft's");

    // Gen I Create default is untouched (and equals the native default above).
    const auto gen1Editor = read("include/Integration/Gen1/Gen1StagedPokemonEditor.h");
    require(gen1Editor.find("std::array<uint8_t,4> moves{84,45,0,0}, pp{30,40,0,0}") != std::string::npos,
            "Gen I Pikachu Lv5 Create default kept");
}

} // namespace

int main() {
    tableRouting();
    everySpeciesEveryLevel();
    knownNativeMovesets();
    sharedPolicy();
    applyPathContracts();
    require(failures == 0, "all species-change move checks");
    std::cout << "Species change resets moves (R2-BUG-004): 20 games, 13 native tables, every species x "
                 "level 1-100 vs PKHeX SetEncounterMoves, five apply paths: PASS\n";
    return 0;
}
