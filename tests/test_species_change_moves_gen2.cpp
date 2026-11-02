// Generation II species change and Create moves / history: Gold, Silver, Crystal through the real
// Session::setSpecies (the one function both Gen II species-apply paths call) and the real
// StagedEditor, ending in a strict export -> reparse. Round 11 (owner H28 / Extra 5 / Extra 6 / H25)
// replaces the round 3 R2-BUG-004 refill and the Oct 26 seeded Create moves (C14): a new Create
// record starts with no moves; a species change keeps only the moves the new species can use in
// this exact game, with their PP and PP Ups, and never adds one; a Create draft's Pokerus and a
// no-longer-valid Crystal caught location reset, while an existing Pokemon keeps both; Add refuses
// a moveless Pokemon. The same species changes nothing; Discard restores; only the edited box's
// bytes change in the export.
#include "fixtures/gsc_pokemon_fixture.h"
#include "Integration/Gen2/Gen2MoveCompatibility.h"
#include "UI/Gen2PokemonSession.h"
#include "UI/Gen2PokemonPickerModel.h"
#include "UI/SpeciesChangeLevelPolicy.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>
#include <vector>

namespace Rules = PokeBank::UIModel::Gen2PokemonEditor;
using Rules::Session;
using Rules::SessionMode;

namespace {

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}

// Every kept move came from `before` with its own PP / PP Ups, is usable by the new species, and
// the empty slots are at the end with PP 0 / PP Ups 0.
void checkKeptMoves(const PokemonRecord& before, const PokemonRecord& after, SourceGame game,
                    const std::string& what) {
    bool empty = false;
    for (std::size_t i = 0; i < 4; ++i) {
        const auto move = after.moves[i];
        if (move == 0) {
            empty = true;
            require(after.pp[i] == 0 && after.ppUps[i] == 0, what + ": empty slot PP 0 / PP Ups 0");
            continue;
        }
        require(!empty, what + ": kept moves move up");
        const auto at = std::find(before.moves.begin(), before.moves.end(), move);
        require(at != before.moves.end(), what + ": no move is ever added");
        const auto from = static_cast<std::size_t>(at - before.moves.begin());
        require(after.pp[i] == before.pp[from] && after.ppUps[i] == before.ppUps[from],
                what + ": PP and PP Ups travel with the move");
        require(MoveCompatibility::canLearnMove(game, after.species, move), what + ": kept move is compatible");
    }
    for (const auto move : before.moves)
        if (move && MoveCompatibility::canLearnMove(game, after.species, move))
            require(std::find(after.moves.begin(), after.moves.end(), move) != after.moves.end(),
                    what + ": a compatible move is kept");
}

void run(const L& layout, SourceGame game, std::string_view sourceId) {
    const bool crystal = game == SourceGame::Crystal;
    const auto raw = fixture(layout, true);
    auto parsed = parse(raw, game);
    require(static_cast<bool>(parsed), "fixture parses");
    std::string error;

    // Box 1 slot 0 is a non-current box, so its export bytes are isolated.
    auto baseEditor = StagedEditor::create(*parsed.save, error);
    require(static_cast<bool>(baseEditor), "editor");
    const auto normalized = baseEditor->finalizedBytes(error);
    auto entry = *baseEditor->boxedPokemon(1, 0, error);
    require(entry.species == 25, "fixture Pikachu");

    // Transitions with different learnsets, Edit and Create sessions.
    const uint16_t chain[] = {26, 22, 1, 152, 133, 22, 249, 25};
    for (const auto mode : {SessionMode::Edit, SessionMode::Create}) {
        Session s;
        s.begin(entry, mode);
        s.working.moves = {84, 45, 98, 0};  // Thunder Shock, Growl, Quick Attack
        s.working.pp = {30, 40, 30, 0};
        s.working.ppUps = {3, 2, 1, 0};
        for (const uint16_t species : chain) {
            const auto before = s.working;
            require(s.setSpecies(species, sourceId), "setSpecies " + std::to_string(species));
            require(s.working.level == PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(sourceId, species),
                    "level policy unchanged");
            checkKeptMoves(before, s.working, game, std::string(sourceId) + " -> " + std::to_string(species));
            for (const auto move : s.working.moves)
                require(move != 84 || species == 25 || species == 26, "Thunder Shock never survives a change away from Pikachu");
            // Same species: nothing changes, even after hand-edited moves.
            require(s.setMove(3, 33), "hand-edit slot 4");
            const auto again = s.working;
            require(s.setSpecies(species, sourceId), "reselect");
            require(Rules::sameEditableRecord(again, s.working), "same species changes nothing");
        }
        // Unknown game: nothing is provably compatible, so the slots are cleared -- never kept blindly.
        Session unknown;
        unknown.begin(entry, mode);
        unknown.working.moves = {84, 45, 0, 0};
        require(unknown.setSpecies(22, "not_a_gen2_game"), "unknown-game species change");
        require(unknown.working.moves == (std::array<uint8_t, 4>{}) && unknown.working.pp == (std::array<uint8_t, 4>{}),
                "no data: empty slots");
    }

    // Round 11 (owner H28 / Extra 5 / Extra 6; C14 reversed by the owner): a new Create record
    // starts with NO moves and a species change does not fill any in. Session::add refuses until a
    // move is chosen, staging nothing; then the staged record and Create Review carry the choice.
    {
        auto createEditor = StagedEditor::create(*parsed.save, error);
        require(static_cast<bool>(createEditor), "create editor");
        const auto beforeBytes = createEditor->finalizedBytes(error);
        PokemonRecord fresh = entry;
        fresh.level = 5;
        fresh.moves = {};
        fresh.pp = {};
        fresh.ppUps = {};
        fresh.pokerus = 0;
        fresh.caughtData = 0;
        Session create;
        create.begin(fresh, SessionMode::Create);
        require(create.setSpecies(24, sourceId) && create.working.moves == (std::array<uint8_t, 4>{}),
                "blank Create stays blank after a species change");
        require(create.setSpecies(25, sourceId) && create.working.moves == (std::array<uint8_t, 4>{}) &&
                create.working.pp == (std::array<uint8_t, 4>{}), "Create starts with no moves (C14 reversed)");
        require(create.setLevel(5), "Create level 5");
        std::size_t slot = 99;
        require(!create.add(*createEditor, 2, slot, error) && error == "Choose at least one move before Add",
                "Add refuses a moveless Pokemon with the shared message: " + error);
        require(createEditor->finalizedBytes(error) == beforeBytes && !createEditor->hasPendingChanges(),
                "a refused Add stages nothing");
        require(create.mode == SessionMode::Create, "the draft stays open");
        require(create.setMove(0, 84) && create.setMove(1, 45), "user picks Thunder Shock, Growl");
        require(create.add(*createEditor, 2, slot, error), "Create add: " + error);
        const auto added = createEditor->boxedPokemon(2, slot, error);
        require(added && added->species == 25 && added->level == 5 &&
                added->moves == (std::array<uint8_t, 4>{84, 45, 0, 0}) &&
                added->pp == (std::array<uint8_t, 4>{30, 40, 0, 0}), "staged Create carries the chosen moves");
        bool movesRow = false;
        for (const auto& change : createEditor->pendingChanges())
            movesRow |= change.label == "Box 3 Slot " + std::to_string(slot + 1) + " / Moves" &&
                        change.afterValue == "Thunder Shock, Growl";
        require(movesRow, "Create Review shows a Moves row");
    }

    // H25: a Create draft's Pokerus and a Crystal caught location that is no encounter of the new
    // species reset; a still-valid one stays; an existing Pokemon keeps both.
    {
        namespace Native = PokeBank::UIModel::Gen2Native;
        Native::CrystalCaughtData caught;
        caught.present = true; caught.timeOfDay = 2; caught.levelCode = 16; caught.location = 50;
        const uint16_t caughtRaw = Native::encodeCrystalCaughtData(caught);  // a Crystal Pikachu location
        for (const auto mode : {SessionMode::Create, SessionMode::Edit}) {
            PokemonRecord p = entry;
            p.moves = {84, 45, 0, 0};
            p.pp = {30, 40, 0, 0};
            p.pokerus = 0x21;
            p.caughtData = caughtRaw;
            Session s;
            s.begin(p, mode);
            require(s.setSpecies(24, sourceId), "Pikachu -> Arbok");
            const bool create = mode == SessionMode::Create;
            require(s.working.pokerus == (create ? 0 : 0x21), "Pokerus resets only for a Create draft");
            const bool locationValidForArbok = false;  // no Crystal template puts Arbok at location 50
            require(s.working.caughtData == (create && !locationValidForArbok ? 0 : caughtRaw),
                    "Create clears a location Arbok was never met at; Edit keeps its recorded history");
            // Round 11 follow-up: a Create reset is visible (caught data is optional in Crystal, so
            // it says "cleared", not "needs a new choice"); Edit resets nothing and says nothing.
            require(s.speciesChangeResetStatus() ==
                        (create ? std::string("Species changed: Pokérus cleared and Met Location cleared") : std::string()),
                    "reset status: " + s.speciesChangeResetStatus());
            // Edit: the kept Crystal caught data is flagged (editor), never cleared; Gold / Silver
            // carry no caught data to judge.
            const auto flags = s.historyFlags(sourceId);
            if (create || !crystal) require(flags.empty(), "no flag for a Create draft or Gold / Silver");
            else
                require(!flags.empty() && flags.front() == "Met Location is not a listed catch for Arbok (history kept)",
                        "Crystal Edit flag: " + (flags.empty() ? std::string("(none)") : flags.front()));
        }
        if (crystal) {
            // Edit Pikachu -> Arbok with Crystal caught data and Pokerus: Keep -> Review (caution note)
            // -> export -> strict reparse keeps Pokerus and the caught data byte for byte.
            auto historyEditor = StagedEditor::create(*parsed.save, error);
            PokemonRecord p = entry;
            p.moves = {84, 92, 0, 0};   // Thunder Shock, Toxic (TM06: Arbok keeps it)
            p.pp = {30, 10, 0, 0};
            p.pokerus = 0x21;
            p.caughtData = caughtRaw;
            Session seed;
            seed.begin(entry, SessionMode::Edit);
            seed.working = p;   // stage the history first, as an earlier session would have
            require(seed.keep(*historyEditor, 1, 0, error), "stage Pikachu with caught data: " + error);
            const auto seeded = *historyEditor->boxedPokemon(1, 0, error);
            require(seeded.caughtData == caughtRaw && seeded.pokerus == 0x21, "seeded history");
            Session e;
            e.begin(seeded, SessionMode::Edit);
            require(e.setSpecies(24, sourceId), "Edit Pikachu -> Arbok");
            const auto flags = e.historyFlags(sourceId);
            require(!flags.empty() && e.missingRequiredFields().empty(), "flagged, and Keep is not refused");
            require(e.keep(*historyEditor, 1, 0, error), "keep Arbok: " + error);
            bool cautionRow = false;
            for (const auto& change : historyEditor->pendingChanges())
                cautionRow |= change.label == "Box 2 Slot 1 Caution" && change.beforeValue.empty() &&
                              change.afterValue == PokeBank::UIModel::HistoryConsistency::joined(flags);
            require(cautionRow, "Review carries the caution note on Arbok's entry");
            const auto exported = historyEditor->finalizedBytes(error);
            require(!exported.empty() && error.empty(), "export: " + error);
            auto reread = parse(exported, game);
            require(static_cast<bool>(reread), "strict reparse");
            auto rereadEditor = StagedEditor::create(*reread.save, error);
            const auto arbok = rereadEditor->boxedPokemon(1, 0, error);
            require(arbok && arbok->species == 24 && arbok->caughtData == caughtRaw && arbok->pokerus == 0x21,
                    "reparsed Arbok kept its Pokerus and Crystal caught data byte for byte");
        }
        if (crystal) {
            // Ekans -> Arbok at location 52, where Crystal has both: the caught data stays.
            Native::CrystalCaughtData both = caught;
            both.location = 52;
            PokemonRecord p = entry;
            p.species = 23;
            p.moves = {84, 45, 0, 0};
            p.pokerus = 0x21;
            p.caughtData = Native::encodeCrystalCaughtData(both);
            Session s;
            s.begin(p, SessionMode::Create);
            require(s.setSpecies(24, sourceId) && s.working.caughtData == p.caughtData && s.working.pokerus == 0,
                    "Create: a location still valid for Arbok is kept; Pokerus still resets");
        }
    }

    // Round 11 follow-up (independent QA finding 1): every species 1-251 has at least one Crystal Met
    // Location row, so no species x Crystal combination is empty. Crystal caught data stays optional
    // (Create is not refused without it); a chosen row still round-trips through Add -> export ->
    // strict reparse.
    // Round 12 (R12-004 / R12-005, owner H08 "pokemon crysal now has massive amounts for met lcoation
    // for paokmon that arnt legit"; updated deliberately): the chooser opens on the DEFAULT list --
    // verified Crystal templates of the species / its pre-evolutions that the Pokémon's level
    // reaches, the Gift / Event markers, then ONE "Other location (unverified)..." row -- which opens
    // the UNVERIFIED list (every other landmark, plus templates above the level, each saying why).
    if (crystal) {
        namespace Picker = PokeBank::UIModel::Gen2PokemonPicker;
        namespace Native = PokeBank::UIModel::Gen2Native;
        namespace Rows = PokeBank::UIModel::ExactGameMetLocations;
        namespace Policy = PokeVault::Integration::MetLocationPolicy;
        std::size_t empty = 0, rows = 0, templates = 0, markers = 0, others = 0, levelRows = 0, staged = 0,
                    markersOnly = 0;
        for (uint16_t species = 1; species <= 251; ++species) {
            const std::string what = "Crystal " + std::to_string(species);
            const uint8_t level = PokeBank::UIModel::SpeciesChangeLevelPolicy::defaultLevel(sourceId, species);
            Picker::Model model;
            model.openLocation(sourceId, species, 0, 0, level);
            if (model.locationCount() == 0) { ++empty; continue; }
            require(!model.unverifiedList && model.unverifiedEntryAt(model.locationCount() - 1),
                    what + ": the default list ends with 'Other location (unverified)...'");
            std::size_t verifiedHere = 0, markersHere = 0;
            for (int row = 0; row < model.locationCount(); ++row) {
                model.index = row;
                const auto text = Picker::locationRowTextAt(model, row);
                require(text.detail.find("legal") == std::string::npos, what + ": no row says legal");
                const bool unverified = text.detail.find("unverified") != std::string::npos;
                if (model.unverifiedEntrySelected()) {
                    require(text.place == Policy::kUnverifiedListRowPlace && !model.locationChoice() &&
                            !model.otherLocationChoice(), what + ": the entry row is not a location");
                } else if (const auto* encounter = model.locationChoice();
                           encounter && unverified &&
                           text.detail.find(PokeVault::Integration::GiftStaticEncounters::kAccessUnverified) != std::string::npos) {
                    // Owner decision (Nov 2, updated deliberately): Celebi (GS Ball) is the encounter in
                    // the game, listed but never verified; its note reads Incomplete.
                    ++templates;
                    require(Rows::crystalUnverifiedNote(sourceId, species, static_cast<uint8_t>(encounter->location),
                                                        static_cast<uint8_t>(encounter->minLevel), level).find("Incomplete") == 0,
                            what + ": a prerequisite-dependent row reads Incomplete");
                } else if (const auto* encounter = model.locationChoice()) {
                    ++templates; ++verifiedHere;
                    // (an Odd Egg template is hatched at landmark 0, Crystal's own egg record)
                    // R12-005: never a template above the Pokémon's level.
                    require(!unverified && text.detail.find("Lv. ") != std::string::npos &&
                            encounter->minLevel <= level &&
                            (encounter->location == 0 || Rows::crystalNativeCaughtLocation(encounter->location)),
                            what + ": a verified Crystal template the level reaches: " + text.detail);
                } else {
                    ++markers; ++markersHere;
                    const auto other = model.otherLocationChoice();
                    require(other && unverified && Rows::crystalMarkerLocation(*other) &&
                            text.detail.find("marker") != std::string::npos,
                            what + ": a default non-template row is a labelled Gift / Event marker: " + text.detail);
                }
                ++rows;
            }
            // Never empty: no verified row here still shows the markers (unless verified there).
            require(markersHere + verifiedHere > 0 && markersHere <= 2, what + ": markers shown");
            if (verifiedHere == 0) {
                ++markersOnly;
                require(markersHere == 2, what + ": no verified row -> Gift and Event markers");
            }
            // The unverified list is reachable and labelled.
            Picker::Model all;
            all.openLocation(sourceId, species, 0, 0, level, true);
            require(all.unverifiedList && !all.unverifiedEntry && all.locationCount() > 0, what + ": unverified list");
            for (int row = 0; row < all.locationCount(); ++row) {
                all.index = row;
                const auto text = Picker::locationRowTextAt(all, row);
                const auto other = all.otherLocationChoice();
                // (a level-blocked Odd Egg template keeps Crystal's own egg landmark 0; Round 13, owner
                // decision 7: a level-blocked gift / trade data row may sit at the Gift landmark 0x7E)
                require(!all.locationChoice() && other &&
                        (!Rows::crystalMarkerLocation(*other) || all.levelBlockedAt(row)) &&
                        (Rows::crystalNativeCaughtLocation(*other) || (*other == 0 && all.levelBlockedAt(row))),
                        what + ": an unverified row is a landmark, never a template");
                if (const auto* blocked = all.levelBlockedAt(row)) {
                    ++levelRows;
                    require(blocked->minLevel > level && text.detail.find(Policy::kLevelBelowEncounter) != std::string::npos,
                            what + ": a template above the level says why: " + text.detail);
                } else {
                    ++others;
                    require(text.detail == Rows::kOtherPlaceDetail, what + ": other landmark unverified: " + text.detail);
                }
                ++rows;
            }
            // Create with a usable move: no caught data is fine (optional); a default row and an
            // unverified row chosen each round-trip, and the editor / Review line says unverified.
            uint8_t move = 0;
            for (uint16_t m = 1; m <= 251 && !move; ++m)
                if (PokeVault::Integration::Gen2::MoveCompatibility::canLearnMove(game, species, m)) move = static_cast<uint8_t>(m);
            require(move != 0, "a learnable Crystal move exists for " + std::to_string(species));
            for (const bool fromUnverified : {false, true}) {
                auto createEditor = StagedEditor::create(*parsed.save, error);
                PokemonRecord fresh = entry;
                fresh.level = 5;
                fresh.moves = {};
                fresh.pp = {};
                fresh.ppUps = {};
                fresh.pokerus = 0;
                fresh.caughtData = 0;
                Session create;
                create.begin(fresh, SessionMode::Create);
                require(create.setSpecies(species, sourceId) && create.setMove(0, move),
                        "Crystal Create " + std::to_string(species));
                require(create.setLevel(level), what + ": the Create level the lists were built for");
                require(create.missingRequiredFields().empty(), "Crystal caught data stays optional");
                Picker::Model& list = fromUnverified ? all : model;
                const int real = list.locationCount() - (list.unverifiedEntry ? 1 : 0);
                list.index = species % real;
                Native::CrystalCaughtData caught;
                caught.present = true;
                caught.timeOfDay = 1;
                caught.levelCode = static_cast<uint8_t>(std::clamp<int>(level, 2, 63));
                if (const auto* encounter = list.locationChoice()) {
                    // Round 13 (owner decisions 7 / 8; updated deliberately): the overlay's one rule
                    // for a verified row's caught data (a gift / trade at the Gift landmark is written
                    // as Crystal writes it: level 0, time 0; a timed row writes its first time).
                    caught = Picker::caughtForTemplate(*encounter, 0, false).caught;
                } else {
                    caught.location = *list.otherLocationChoice();
                }
                create.working.caughtData = Native::encodeCrystalCaughtData(caught);
                const auto note = Rows::crystalUnverifiedNote(sourceId, species, caught.location, caught.levelCode,
                                                              create.working.level);
                // (a landmark that also has a verified row the level reaches reads verified exactly when
                // the level code fits that row)
                const bool verifiedPlace = std::any_of(model.encounterChoices.begin(), model.encounterChoices.end(),
                    [&](const auto& e) { return e.location == caught.location; });
                require(fromUnverified ? (note.find("unverified") != std::string::npos || (note.empty() && verifiedPlace))
                                       : (list.locationChoice() ? (note.empty() || note.find("Incomplete") == 0)   // (Nov 2: GS Ball Celebi)
                                                               : note.find("marker") != std::string::npos),
                        what + ": the editor / Review line matches the list chosen from: " + note);
                std::size_t slot = 99;
                require(create.add(*createEditor, 2, slot, error), "Crystal Create add " + std::to_string(species) + ": " + error);
                bool reviewNote = false;
                for (const auto& change : createEditor->pendingChanges())
                    reviewNote |= change.label.find("Met evidence") != std::string::npos && change.afterValue == note;
                require(reviewNote == !note.empty(), what + ": Review carries the unverified note exactly when unverified");
                const auto out = createEditor->finalizedBytes(error);
                require(!out.empty() && error.empty(), "Crystal export: " + error);
                auto reread = parse(out, game);
                require(static_cast<bool>(reread), "Crystal strict reparse");
                auto rereadEditor = StagedEditor::create(*reread.save, error);
                const auto back = rereadEditor->boxedPokemon(2, slot, error);
                require(back && back->species == species &&
                        Native::decodeCrystalCaughtData(back->caughtData).location == caught.location,
                        "the chosen Crystal location round-trips for " + std::to_string(species));
                ++staged;
            }
        }
        require(empty == 0 && templates > 0 && markers > 0 && others > 0 && levelRows > 0,
                "Crystal species with no Met Location row: " + std::to_string(empty));
        std::cout << "crystal_gbc: 251 species, " << rows << " rows (" << templates << " verified, " << markers
                  << " markers, " << others << " unverified landmarks, " << levelRows << " above the level), "
                  << staged << " staged and reparsed, " << markersOnly << " markers only, " << empty << " empty\n";

        // The owner's example: Crystal Arbok. Before Round 12 its chooser listed 104 rows (14
        // templates + every other landmark); now only verified Arbok / Ekans rows its level reaches.
        Picker::Model arbok;
        arbok.openLocation(sourceId, 24, 0, 0, 5);
        Picker::Model arbokHigh;
        arbokHigh.openLocation(sourceId, 24, 0, 0, 30);
        std::cout << "Crystal Arbok: default list " << arbok.locationCount() << " rows at Lv. 5, "
                  << arbokHigh.locationCount() << " rows at Lv. 30 (was 104)\n";
        require(arbokHigh.locationCount() < 104 && arbokHigh.unverifiedEntrySelected() == false,
                "Crystal Arbok default list is short");
        bool viaEkans = false;
        for (int row = 0; row < arbokHigh.locationCount(); ++row)
            viaEkans |= Picker::locationRowTextAt(arbokHigh, row).detail.find("via Ekans") != std::string::npos;
        require(viaEkans, "Crystal Arbok is offered its Ekans catches (pre-evolution)");
        for (int row = 0; row < arbok.locationCount(); ++row) {
            arbok.index = row;
            if (const auto* e = arbok.locationChoice()) require(e->minLevel <= 5, "Lv. 5 Arbok: nothing above Lv. 5");
        }
    }

    // An existing Pokemon whose species change leaves no usable move cannot be kept moveless.
    {
        auto editEditor = StagedEditor::create(*parsed.save, error);
        PokemonRecord p = entry;
        p.moves = {84, 0, 0, 0};
        p.pp = {30, 0, 0, 0};
        Session s;
        s.begin(p, SessionMode::Edit);
        require(s.setSpecies(129, sourceId) && s.working.moves == (std::array<uint8_t, 4>{}),
                "Magikarp cannot use Thunder Shock");
        require(!s.keep(*editEditor, 1, 0, error) && error == "Choose at least one move before Keep",
                "Keep refused with the shared message: " + error);
        require(!editEditor->hasPendingChanges(), "nothing staged");
    }

    // Pikachu -> Fearow, Keep, Review, export -> strict reparse; only box 1 bytes move.
    auto editor = StagedEditor::create(*parsed.save, error);
    Session s;
    PokemonRecord taught = entry;
    taught.moves = {84, 45, 98, 0};
    taught.pp = {30, 40, 30, 0};
    taught.ppUps = {1, 2, 3, 0};
    s.begin(taught, SessionMode::Edit);
    require(s.setSpecies(22, sourceId), "Pikachu -> Fearow");
    require(std::find(s.working.moves.begin(), s.working.moves.end(), 84) == s.working.moves.end() &&
            s.working.moves[0] == 45, "Fearow: Thunder Shock cleared, Growl kept");
    const auto fearow = s.working;
    require(s.keep(*editor, 1, 0, error), "keep: " + error);
    require(!editor->pendingChanges().empty(), "Review lists the change");
    const auto staged = editor->boxedPokemon(1, 0, error);
    require(staged && staged->species == 22 && staged->moves == fearow.moves && staged->pp == fearow.pp &&
            staged->ppUps == fearow.ppUps, "staged record carries the new moves");
    const auto out = editor->finalizedBytes(error);
    require(!out.empty() && error.empty(), "export");
    require(differsOnlyInRange(normalized, out, boxStart(1), boxStart(1) + listLen(20, 32, 11)),
            "no byte outside the edited box changes");
    auto reloaded = parse(out, game);
    require(static_cast<bool>(reloaded), "strict reparse");
    auto check = StagedEditor::create(*reloaded.save, error);
    const auto back = check->boxedPokemon(1, 0, error);
    require(back && back->species == 22 && back->moves == fearow.moves && back->pp == fearow.pp &&
            back->ppUps == fearow.ppUps, "reparsed Fearow has its kept moves");
    checkKeptMoves(taught, *back, game, "reparsed");
    const auto other = check->boxedPokemon(0, 0, error);
    require(other && other->species == 25, "the other box is untouched");
    require(raw == fixture(layout, true), "source bytes never change");

    // Discard restores the original record exactly.
    editor->discard();
    require(!editor->hasPendingChanges(), "Discard clears Review");
    require(std::equal(editor->stagedBytes().begin(), editor->stagedBytes().end(), editor->originalBytes().begin()),
            "Discard restores every byte");
}

// The UI's Create entry is a TrainerViewScreen member path (no host harness): pin that it no longer
// seeds moves (Round 11) and that its Add asks the session's shared required-fields check.
void createEntryStartsBlank() {
    std::ifstream in("src/UI/Gen2PokemonEditorFoundation.inc", std::ios::binary);
    const std::string src{std::istreambuf_iterator<char>(in), {}};
    const auto begin = src.find("bool beginCreate(TrainerViewScreen& screen, int box) {");
    require(begin != std::string::npos, "beginCreate found");
    const auto body = src.substr(begin, src.find("\n}\n", begin) - begin);
    require(body.find("state.working = defaultCreateRecord(*editor);") != std::string::npos &&
            body.find("seedNativeMoves") == std::string::npos, "beginCreate starts with the moveless default");
    const auto stage = src.find("bool stageCreate(TrainerViewScreen& screen) {");
    const auto stageBody = src.substr(stage, src.find("\n}\n", stage) - stage);
    require(stageBody.find("state.missingRequiredFields()") < stageBody.find("state.add("),
            "stageCreate checks the required fields before Add");
}

} // namespace

int main() {
    createEntryStartsBlank();
    run(GS, SourceGame::Gold, "gold_gbc");
    run(GS, SourceGame::Silver, "silver_gbc");
    run(C, SourceGame::Crystal, "crystal_gbc");
    std::cout << "Gen II blank Create, species change keeps compatible moves, Create history reset (Gold, "
                 "Silver, Crystal; keep, Review, export, reparse, Discard): PASS\n";
    return 0;
}
