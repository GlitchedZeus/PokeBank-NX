// Species change and Create moves, Gen I-V. Part 1 of the host coverage: the native level-up data
// and algorithm for all 20 Gen I-V games (kept as table tests: the R2-BUG-004 data stays correct and
// pinned), the shared keep-learnable policy, and the apply-path contracts. Round 11 (owner H28 /
// Extra 5 / Extra 6) replaced the round 3 refill: no editor fills moves in any more -- a new Create
// draft starts blank and a species change keeps only usable moves, never adding one -- and every
// Keep/Add asks the shared CreateRequiredFields rule. The per-generation record round trips are
// tests/test_species_change_moves_gen1.cpp ... _gen5.cpp; the pure policies are
// tests/test_species_change_history_policy.cpp.
//
// Sources behind the data (tools/gen_levelup_learnsets15.py, cross-checked at generation):
// pret pokered / pokeyellow / pokegold / pokecrystal / pokeruby / pokeemerald / pokefirered and
// PKHeX 6501f0ab lvlmove_* resources.
#include "Pokemon/SpeciesMoveDefaults.h"
#include "UI/SpeciesChangeMoves.h"

#include <algorithm>
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
        require(moves(t, 25, 5) == (std::array<uint16_t, 4>{84, 45, 0, 0}), "Gen I Pikachu Lv5 native level-up moves");
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

// The shared policy over real data: an old moveset (Pikachu's native Lv 30 moves) changed to a
// species that shares only some of them keeps exactly those, never one of the new species' own.
void sharedPolicy() {
    const auto pikachu = moves(T::Crystal, 25, 30);
    const auto raichuOwn = moves(T::Crystal, 26, 30);
    const std::array<uint8_t, 4> pp{11, 12, 13, 14}, ups{1, 2, 3, 0};
    const std::set<uint16_t> learns{pikachu[1], pikachu[3]};
    const auto s = R::keepLearnable(pikachu, pp, ups, [&](uint16_t m) { return learns.count(m) != 0; });
    require(s.moves == (std::array<uint16_t, 4>{pikachu[1], pikachu[3], 0, 0}) &&
            s.pp == (std::array<uint8_t, 4>{12, 14, 0, 0}) && s.ppUps == (std::array<uint8_t, 4>{2, 0, 0, 0}),
            "only the learnable old moves stay, with their own PP / PP Ups, moved up");
    for (const auto m : raichuOwn)
        if (m && std::find(pikachu.begin(), pikachu.end(), m) == pikachu.end())
            require(std::find(s.moves.begin(), s.moves.end(), m) == s.moves.end(), "no new species' default is added");
    std::array<uint16_t, 4> blank{};
    std::array<uint8_t, 4> zero{};
    R::refreshArrays(blank, zero, zero, [](uint16_t) { return true; });
    require(blank == (std::array<uint16_t, 4>{}), "a blank draft stays blank");
}

// The five apply paths call the shared keep-learnable policy only on a real species change, no
// editor fills moves in, and every Keep/Add entry asks the shared required-fields rule.
void applyPathContracts() {
    const auto gen1 = read("src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc");
    const auto g1 = functionBody(gen1, "bool ux3ApplySpeciesPicker(TrainerViewScreen& screen)");
    require(g1.find("writeArrays(\n                ux3SpeciesChangeMoves(screen, species, state.draft.moves, state.draft.pp, state.draft.ppUps)") != std::string::npos,
            "Gen I Add draft keeps its own usable moves");
    require(g1.find("writeEdit(\n                ux3SpeciesChangeMoves(screen, species, pokemon->moves, pokemon->pp, pokemon->ppUps), edit)") != std::string::npos,
            "Gen I Edit keeps the record's own usable moves");
    require(g1.find("CreateRequiredFields::forEdit(pokemon->moves, *edit.moves).move") < g1.find("ux2CommitEdit("),
            "Gen I Edit refuses (before staging) a change that leaves no move");
    const auto g1Helper = functionBody(gen1, "PokeBank::UIModel::SpeciesMoveDefaults::Slots ux3SpeciesChangeMoves(");
    require(g1Helper.find("SpeciesMoveDefaults::gen1(editor(screen), species, moves, pp, ppUps)") != std::string::npos,
            "Gen I ux3 uses the one Gen I binding");
    const auto gen1Binding = read("include/UI/Gen1SpeciesChangeMoves.h");
    require(gen1Binding.find("MoveCompatibility::canLearnMove(game, species, move)") != std::string::npos &&
            gen1Binding.find("keepLearnable(") != std::string::npos,
            "Gen I binding: exact-game Gen I compatibility, keep-learnable");
    // Every other Gen I species-apply path (QA CG19) goes through the same binding, only on a real change.
    const auto cleanup2 = read("src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc");
    const auto ux2 = functionBody(cleanup2, "bool ux2ApplySpeciesPicker(TrainerViewScreen& screen)");
    require(ux2.find("if (selected != state.draft.species)\n            PokeBank::UIModel::SpeciesMoveDefaults::writeArrays(PokeBank::UIModel::SpeciesMoveDefaults::gen1(editor(screen), selected, state.draft.moves, state.draft.pp, state.draft.ppUps)") != std::string::npos &&
            ux2.find("if (pokemon && selected != pokemon->species)\n            PokeBank::UIModel::SpeciesMoveDefaults::writeEdit(PokeBank::UIModel::SpeciesMoveDefaults::gen1(editor(screen), selected, pokemon->moves, pokemon->pp, pokemon->ppUps), edit);") != std::string::npos,
            "Gen I ux2ApplySpeciesPicker keeps usable moves (Add and Edit)");
    for (const char* path : {"src/UI/Gen1PokemonEditorOverlayUX.inc", "src/UI/Gen1PokemonEditorOverlay.cpp"}) {
        const auto text = read(path);
        const auto apply = functionBody(text, "bool applyPicker(TrainerViewScreen& screen)");
        require(apply.find("if (species != state.draft.species)\n                PokeBank::UIModel::SpeciesMoveDefaults::writeArrays(\n                    PokeBank::UIModel::SpeciesMoveDefaults::gen1(e, species, state.draft.moves, state.draft.pp, state.draft.ppUps),") != std::string::npos &&
                apply.find("if (*edit.species != pokemon->species)\n            PokeBank::UIModel::SpeciesMoveDefaults::writeEdit(\n                PokeBank::UIModel::SpeciesMoveDefaults::gen1(e, *edit.species, pokemon->moves, pokemon->pp, pokemon->ppUps), edit);") != std::string::npos,
                std::string(path) + " applyPicker keeps usable moves (Add and Edit)");
    }
    // Both Gen I updates sit inside the species-changed branches.
    require(g1.find("if (species != state.draft.species) {") < g1.find("writeArrays(") &&
            g1.find("if (species != pokemon->species) {") < g1.find("writeEdit("), "Gen I: same species changes nothing");

    const auto gen2 = read("include/UI/Gen2PokemonSession.h");
    const auto g2 = functionBody(gen2, "bool setSpecies(uint16_t species, std::string_view sourceGameId = {}) noexcept");
    require(g2.find("if (species == working.species) return true;") < g2.find("SpeciesMoveDefaults::refreshArrays("),
            "Gen II: same species returns before the update");
    require(g2.find("Gen2::MoveCompatibility::canLearnMove(*game, species, move)") != std::string::npos &&
            g2.find("SpeciesChangeHistoryPolicy::reconcile(") != std::string::npos,
            "Gen II: exact-game compatibility and the shared history policy");

    const auto gen3 = read("include/UI/Gen3SharedPokemonSession.h");
    const auto g3 = functionBody(gen3, "bool setSpecies(uint16_t species, std::string_view sourceGameId = {}) noexcept");
    require(g3.find("if (species == working.species) return true;") < g3.find("SpeciesMoveDefaults::refreshArrays("),
            "Gen III: same species returns before the update");
    require(g3.find("exactGame(sourceGameId, working.originGame)") != std::string::npos &&
            g3.find("SpeciesChangeHistoryPolicy::reconcile(") != std::string::npos,
            "Gen III: exact game (source id, else origin) and the shared history policy");
    const auto gen3Surface = read("src/UI/Gen3SharedPokemonSurface.inc");
    // Round 11 follow-up (updated pin): the Create reset status is now resetStatus() naming what was
    // reset (Session::speciesChangeResetStatus), and every Met Location row -- template, other
    // native location, marker, keep-stored -- is applied by Session::applyMetLocationRow, which
    // records the explicit choice (the old pin only covered the template branch).
    require(gen3Surface.find("state.session.setSpecies(state.speciesPreview, screen.sourceGameId)") != std::string::npos &&
            gen3Surface.find("state.session.speciesChangeResetStatus()") != std::string::npos &&
            gen3Surface.find("state.session.applyMetLocationRow(picker, value, applied)") != std::string::npos,
            "Gen III picker applies through Session::setSpecies; the Met picker records the choice");
    {
        const auto apply = functionBody(gen3, "bool applyMetLocationRow(const MetLocationPicker& picker, uint16_t row,");
        require(apply.find("noteMetLocationChosen();") != std::string::npos &&
                apply.find("isOtherNativeRow(row)") < apply.find("noteMetLocationChosen();"),
                "Gen III: every applied Met Location row (other native and markers too) is an explicit choice");
    }

    const auto gen4 = read("src/UI/Gen4SharedPokemonSurface.inc");
    const auto g4 = functionBody(gen4, "bool applyGen4Picker(TrainerViewScreen& screen, State& state, PickerTarget target,");
    require(g4.find("state.session.changeSpecies(value, state.speciesPreviewShiny, screen.sourceGameId,") != std::string::npos,
            "Gen IV picker applies through Session::changeSpecies");
    require(g4.find("Could not reconcile Species/Shiny while preserving PID-linked traits") != std::string::npos,
            "Gen IV rollback message kept");
    const auto gen4Session = read("include/UI/Gen4SharedPokemonSession.h");
    const auto g4s = functionBody(gen4Session, "bool changeSpecies(uint16_t value, bool shinyWanted, std::string_view exactGameId,");
    require(g4s.find("if (ok && value != previousSpecies) {") < g4s.find("SpeciesMoveDefaults::setMoves(*working,") &&
            g4s.find("SpeciesChangeHistoryPolicy") != std::string::npos,
            "Gen IV: moves and history only on a real species change");
    require(g4s.find("SpeciesMoveDefaults::setMoves(") < g4s.find("if (!ok) {\n            auto restored"),
            "Gen IV: a refused write rolls the whole draft back");

    const auto gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    const auto g5 = functionBody(gen5, "static bool applySpecies(TrainerViewScreen& screen, EditorSurface::EditorState& state,");
    require(g5.find("PokeBank::UIModel::SpeciesMoveDefaults::stageMoves<Field::Move, Field::MovePPUps, Field::MovePP>(probe,\n                    PokeVault::Integration::Gen5::speciesChangeMoves5(screen.sourceGameId, species, 0, p), error)") != std::string::npos,
            "Gen V edit stages the kept moves on the probe in replay order");
    require(g5.find("if (species != p.species()) {\n            // Round 11") != std::string::npos,
            "Gen V: update only on a real species change");
    require(g5.find("backendState.recreateCreateSpecies(") != std::string::npos &&
            g5.find("backendState.speciesRefusal(species)") != std::string::npos, "Gen V pinned calls kept");
    const auto gen5State = read("include/Integration/Gen5/Gen5SharedScreenState.h");
    const auto rebuild = functionBody(gen5State, "                                             bool& encounterReset)");
    require(rebuild.find("if(species!=p->species()) {") < rebuild.find("speciesChangeMoves5("),
            "Gen V Create rebuild: a new species keeps only usable moves; same species keeps the draft's");
    const auto keepWith = functionBody(gen5State, "[[nodiscard]] bool keepWithPresentation(Workspace& workspace,");
    require(keepWith.find("missingRequiredFields()") < keepWith.find("if(createPending_ && !draft_.dirty())"),
            "Gen V Keep asks the shared required-fields rule first");

    // No editor fills moves in: no UI path calls the native default-moveset helpers any more.
    for (const char* path : {"src/UI/Gen1PokemonEditorOverlayUXCleanup3.inc", "src/UI/Gen1PokemonEditorOverlayUXCleanup2.inc",
                             "src/UI/Gen1PokemonEditorOverlayUX.inc", "src/UI/Gen1PokemonEditorOverlay.cpp",
                             "src/UI/Gen2PokemonEditorFoundation.inc", "include/UI/Gen2PokemonSession.h",
                             "include/UI/Gen3SharedPokemonSession.h", "src/UI/Gen4SharedPokemonSurface.inc",
                             "include/UI/Gen4SharedPokemonSession.h", "src/UI/Gen5SharedReadOnlySurface.inc",
                             "include/Integration/Gen5/Gen5SharedScreenState.h", "include/UI/Gen1SpeciesChangeMoves.h",
                             "include/UI/SpeciesChangeMoves.h"}) {
        const auto text = read(path);
        for (const char* banned : {"defaultMoves(", "forSpecies(", "seedNativeMoves", "kSharedCreateMove"})
            require(text.find(banned) == std::string::npos, std::string(path) + " never auto-fills moves (" + banned + ")");
    }

    // A new Create draft starts with no moves in every generation (Gen III / IV never had any).
    const auto gen1Editor = read("include/Integration/Gen1/Gen1StagedPokemonEditor.h");
    require(gen1Editor.find("std::array<uint8_t,4> moves{}, pp{}, ppUps{}, dvs{};") != std::string::npos,
            "Gen I Create default has no moves");
    require(gen5State.find("r.moves={};\n        r.draftWithoutMoves=true;") != std::string::npos,
            "Gen V Create default has no moves");

    // Every Keep/Add entry consults the shared required-fields rule before staging.
    const auto ux2Add = functionBody(cleanup2, "bool ux2StageAdd(TrainerViewScreen& screen)");
    require(ux2Add.find("CreateRequiredFields::forCreate(1, false, state.draft.moves)") < ux2Add.find("e->stageAdd("),
            "Gen I ux2/ux3 Stage Add");
    const auto uxAdd = functionBody(read("src/UI/Gen1PokemonEditorOverlayUX.inc"), "bool stageAdd(TrainerViewScreen& screen)");
    require(uxAdd.find("CreateRequiredFields::forCreate(1, false, state.draft.moves)") < uxAdd.find("e->stageAdd("),
            "Gen I older overlay Stage Add");
    const auto overlay = read("src/UI/Gen1PokemonEditorOverlay.cpp");
    require(overlay.find("CreateRequiredFields::forCreate(1, false, state.draft.moves)") < overlay.find("if (!e->stageAdd("),
            "Gen I oldest overlay Add");
    const auto legacyGen2 = functionBody(read("src/UI/TrainerViewScreenGSCOverlay.inc"), "bool stageAddPokemon(TrainerViewScreen& screen, int box)");
    require(legacyGen2.find("CreateRequiredFields::forCreate(2, false, create.moves)") < legacyGen2.find("stageAddBoxPokemon("),
            "Gen II legacy prompt Add");
    for (const auto& [text, fn] : {std::pair{gen2, std::string("bool add(Gen2::StagedEditor& editor,size_t box,size_t& slot,std::string& error)")},
                                   std::pair{gen2, std::string("bool keep(Gen2::StagedEditor& editor,size_t box,size_t slot,std::string& error,bool party=false)")},
                                   std::pair{gen3, std::string("bool add(Gen3::StagedPokemonEditor& editor, std::size_t box, std::size_t slot,")},
                                   std::pair{gen3, std::string("bool keep(Gen3::StagedPokemonEditor& editor, std::size_t box, std::size_t slot,")},
                                   std::pair{gen4Session, std::string("bool keepCreate(Gen4::Gen4StagedPokemonEditor& editor,")},
                                   std::pair{gen4Session, std::string("bool keep(Gen4::Gen4StagedPokemonEditor& editor,")},
                                   std::pair{gen4Session, std::string("bool keepParty(Gen4::Gen4StagedPokemonEditor& editor,")}}) {
        const auto body = functionBody(text, fn);
        require(body.find("missingRequiredFields(") != std::string::npos, fn + " asks the required-fields rule");
    }
}

} // namespace

int main() {
    tableRouting();
    everySpeciesEveryLevel();
    knownNativeMovesets();
    sharedPolicy();
    applyPathContracts();
    require(failures == 0, "all species-change move checks");
    std::cout << "Species change moves: 20 games, 13 native tables, every species x level 1-100 vs PKHeX "
                 "SetEncounterMoves; keep-learnable policy; five apply paths, no auto-fill, required fields: PASS\n";
    return 0;
}
