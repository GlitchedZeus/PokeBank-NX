// Generation III species change, Create moves and history: Ruby, Sapphire, Emerald, FireRed,
// LeafGreen through the real Session::setSpecies (the Gen III species picker's only apply) and the
// real StagedPokemonEditor, ending in a strict export -> reparse. Round 11 (owner H28 / Extra 5 /
// Extra 6 / H25 / H07) replaces the round 3 R2-BUG-004 refill: a species change keeps only the moves
// the new species can use in the EXACT game (pret-generated Gen3 Learnset), with their PP and PP
// Ups, and never adds one; Create starts blank and Add needs a chosen Met Location and a move; a
// Create draft's Pokerus and no-longer-valid encounter reset, while an existing Pokemon keeps its
// Met Location and Pokerus. The same species changes nothing; Discard restores; every field other
// than the species-dependent ones survives.
#include "Encryption/Encryption3FRLG.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Names/MoveInfo.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Integration/Gen3/Gen3LearnsetTable.h"
#include "UI/Gen3SharedPokemonSession.h"
#include "gen3_edit_discard_r11.h"   // Round 11 H24 regression (owner PASS), run from main()

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <span>
#include <string>
#include <vector>

using namespace PokeVault::Integration::Gen3;
namespace Encounter = PokeVault::Integration::EncounterGuardrails;
using PokeBank::UIModel::Gen3SharedEditor::Session;
using Mode = PokeBank::UIModel::Gen3SharedEditor::Mode;

namespace {

void require(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    assert(ok);
}

// Save fixture: the same layout tests/test_gen3_staged_pokemon_editor.cpp builds (Pikachu Lv20 in
// box 1 slot 1, both save slots valid).
enum class Family { RS, Emerald, FRLG };
using LogicalSectors = std::array<std::array<uint8_t, Detail::kSectorSize>, Detail::kSectorCount>;

void write16(std::span<uint8_t> bytes, std::size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}
void write32(std::span<uint8_t> bytes, std::size_t offset, uint32_t value) {
    write16(bytes, offset, static_cast<uint16_t>(value));
    write16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
}
uint8_t origin(SourceGame game) {
    switch (game) {
        case SourceGame::SapphireGBA: return 1;
        case SourceGame::RubyGBA: return 2;
        case SourceGame::EmeraldGBA: return 3;
        case SourceGame::FireRedGBA: return 4;
        case SourceGame::LeafGreenGBA: return 5;
    }
    return 0;
}
const char* sourceId(SourceGame game) {
    switch (game) {
        case SourceGame::RubyGBA: return "ruby_gba";
        case SourceGame::SapphireGBA: return "sapphire_gba";
        case SourceGame::EmeraldGBA: return "emerald_gba";
        case SourceGame::FireRedGBA: return "firered_gba";
        case SourceGame::LeafGreenGBA: return "leafgreen_gba";
    }
    return "";
}
std::array<uint8_t, 80> samplePokemon(SourceGame game) {
    std::array<uint8_t, 80> blank{};
    Pokemon::Pokemon3FRLG p(std::span<const std::byte>(reinterpret_cast<const std::byte*>(blank.data()), blank.size()));
    p.setPID(0x12345678u);
    p.setTID16(12345);
    p.setSID16(54321);
    p.setSpecies(25);
    p.setLanguage(2);
    p.setOTName(u"RED");
    p.setNickname(u"PIKACHU");
    p.setHeldItem(13);
    p.setFriendship(70);
    p.setOriginGame(origin(game));
    p.setBall(4);
    p.setMetLevel(20);
    p.setMetLocation(1);
    p.setPokerus(0x21);   // Round 11 follow-up: an existing Pokemon's Pokerus must survive an Edit species change
    p.setLevel(20);
    p.setMove(0, 84);  // Thunder Shock with PP Ups, the move that must not survive
    p.setMovePPUps(0, 2);
    p.setMovePP(0, 42);
    p.setMove(1, 45);
    p.setMovePP(1, 40);
    for (int i = 0; i < 6; ++i) {
        p.setIV(i, static_cast<uint8_t>(10 + i));
        p.setEV(i, static_cast<uint8_t>(20 + i));
    }
    auto* encrypted = Encryption::encryptArray3FRLG(p.getData());
    std::array<uint8_t, 80> out{};
    std::memcpy(out.data(), encrypted, out.size());
    delete[] encrypted;
    return out;
}
LogicalSectors makeLogical(Family family, SourceGame game) {
    LogicalSectors logical{};
    if (family == Family::FRLG) {
        constexpr uint32_t key = 0xA1B2C3D4;
        write32(logical[0], 0xAC, 1);
        write32(logical[0], 0xF20, key);
        write32(logical[1], 0x0290, key);
    } else if (family == Family::Emerald) {
        constexpr uint32_t key = 0xA1B2C3D4;
        write32(logical[0], 0xAC, key);
        logical[0][0xEE0] = 0x42;
        write32(logical[1], 0x0490, key);
    } else {
        write32(logical[0], 0xAC, 0);
        write32(logical[1], 0x0490, 0);
    }
    write16(logical[0], 0x0A, 12345);
    write16(logical[0], 0x0C, 54321);
    const auto mon = samplePokemon(game);
    for (std::size_t i = 0; i < mon.size(); ++i) {
        const std::size_t position = 4 + i;
        logical[5 + position / Detail::kSectorDataSize][position % Detail::kSectorDataSize] = mon[i];
    }
    return logical;
}
void writeSlot(std::vector<uint8_t>& save, uint8_t slot, uint32_t counter, uint8_t rotation, Family family,
               SourceGame game) {
    auto logical = makeLogical(family, game);
    for (std::size_t physical = 0; physical < Detail::kSectorCount; ++physical) {
        const uint16_t id = static_cast<uint16_t>((physical + rotation) % Detail::kSectorCount);
        const std::size_t offset = Detail::kSlotBases[slot] + physical * Detail::kSectorSize;
        std::copy(logical[id].begin(), logical[id].end(), save.begin() + static_cast<std::ptrdiff_t>(offset));
        write16(save, offset + 0xFF4, id);
        write16(save, offset + 0xFF6, Detail::sectorChecksum(std::span<const uint8_t>(save).subspan(offset, Detail::kChunkLengths[id])));
        write32(save, offset + 0xFF8, Detail::kSectorSignature);
        write32(save, offset + 0xFFC, counter);
    }
}
std::vector<uint8_t> fixture(Family family, SourceGame game) {
    std::vector<uint8_t> save(Detail::kSaveSize, 0);
    writeSlot(save, 0, 7, 0, family, game);
    writeSlot(save, 1, 9, 5, family, game);
    return save;
}

// Every kept move came from `before` with its own PP / PP Ups and is usable by the new species in
// this exact game; every usable old move is kept; the empty slots trail with PP 0 / PP Ups 0.
void checkKept(const StagedPokemonRecord& before, const StagedPokemonRecord& after, SourceGame game,
               const std::string& what) {
    const auto usable = [&](uint16_t move) {
        return Learnset::classify(game, after.species, move, {}) != Learnset::Availability::Invalid;
    };
    bool empty = false;
    for (std::size_t i = 0; i < 4; ++i) {
        const auto move = after.moves[i];
        if (move == 0) {
            empty = true;
            require(after.pp[i] == 0 && after.ppUps[i] == 0, what + ": empty slot 0/0");
            continue;
        }
        require(!empty, what + ": kept moves move up");
        const auto at = std::find(before.moves.begin(), before.moves.end(), move);
        require(at != before.moves.end(), what + ": no move is ever added");
        const auto from = static_cast<std::size_t>(at - before.moves.begin());
        require(after.pp[i] == before.pp[from] && after.ppUps[i] == before.ppUps[from], what + ": PP travels");
        require(usable(move), what + ": kept move is usable");
    }
    for (const auto move : before.moves)
        if (move && usable(move))
            require(std::find(after.moves.begin(), after.moves.end(), move) != after.moves.end(),
                    what + ": a usable move is kept");
}

void sameIdentity(const StagedPokemonRecord& a, const StagedPokemonRecord& b, const std::string& what) {
    require(a.ivs == b.ivs && a.evs == b.evs && a.otName == b.otName && a.tid == b.tid && a.sid == b.sid &&
            a.heldItem == b.heldItem && a.friendship == b.friendship && a.ball == b.ball &&
            a.metLevel == b.metLevel && a.metLocation == b.metLocation && a.originGame == b.originGame &&
            a.language == b.language && a.pokerus == b.pokerus, what + ": unrelated fields untouched");
}

void run(SourceGame game, Family family) {
    const std::string id = sourceId(game);
    auto source = fixture(family, game);
    const auto immutable = source;
    std::string error;
    auto editor = StagedPokemonEditor::create(source, game, error);
    require(editor && error.empty(), id + " editor: " + error);
    const auto entry = *editor->boxedPokemon(0, 0, error);
    require(entry.species == 25 && entry.moves[0] == 84, "fixture Pikachu with Thunder Shock");

    // Several transitions with different learnsets, Edit sessions: an existing Pokemon keeps its
    // Met Location / Met Level / Pokerus (sameIdentity) and only its usable moves.
    Session s;
    s.begin(entry, Mode::Edit);
    s.working.moves = {84, 45, 98, 85};  // Thunder Shock, Growl, Quick Attack, Thunderbolt
    s.working.pp = {42, 40, 30, 15};
    s.working.ppUps = {2, 0, 1, 3};
    for (const uint16_t species : {26, 22, 1, 252, 133, 386, 51, 25}) {
        const auto before = s.working;
        require(s.setSpecies(species, id), id + " setSpecies " + std::to_string(species));
        checkKept(before, s.working, game, id + " -> " + std::to_string(species));
        sameIdentity(s.working, entry, id + " Edit -> " + std::to_string(species));
        require(!s.encounterReset, "Edit never resets the encounter");
        require(species == 25 || species == 26 ||
                std::find(s.working.moves.begin(), s.working.moves.end(), 84) == s.working.moves.end(),
                "Thunder Shock never survives a change away from Pikachu / Raichu");
        s.working.moves[3] = 33;  // a hand edit after the change
        s.working.pp[3] = 35;
        const auto again = s.working;
        require(s.setSpecies(species, id), "reselect");
        require(s.working.moves == again.moves && s.working.pp == again.pp && s.working.ppUps == again.ppUps &&
                s.working.level == again.level, "same species changes nothing");
    }

    // Pikachu -> Fearow, Keep, export -> strict reparse.
    Session fearow;
    fearow.begin(entry, Mode::Edit);
    require(fearow.setSpecies(22, id), "Pikachu -> Fearow");
    // Round 11 follow-up (owner H25 clarification 2): the record keeps its own history, and the
    // misfit is flagged in the editor (Session::historyFlags, drawn as the Caution data row) and in
    // Review (one caution note on the entry). Location 1 is a Hoenn id: for an R/S/E origin it is
    // not a listed Fearow catch; for a FireRed / LeafGreen origin it is not even a Kanto place.
    const bool frlg = family == Family::FRLG;
    const std::string expectedFlag = frlg ? "Origin game has no record of this Met Location (history kept)"
                                          : "Met Location is not a listed catch for Fearow (history kept)";
    require(fearow.working.pokerus == 0x21 && fearow.working.metLocation == 1 && fearow.working.metLevel == 20 &&
            fearow.working.originGame == entry.originGame, "Edit keeps Pokerus, Met Location / Level and origin");
    {
        const auto flags = fearow.historyFlags();
        require(!flags.empty() && flags.front() == expectedFlag, id + " editor flag: " +
                (flags.empty() ? std::string("(none)") : flags.front()));
        require(fearow.missingRequiredFields().empty(), "a flag never refuses Keep");
    }
    require(fearow.working.moves == (std::array<uint16_t, 4>{45, 0, 0, 0}) && fearow.working.pp[0] == entry.pp[1],
            "Fearow: Thunder Shock cleared, Growl kept with its PP");
    const auto wanted = fearow.working;
    require(fearow.keep(*editor, 0, 0, error), id + " keep: " + error);
    const auto staged = *editor->boxedPokemon(0, 0, error);
    require(staged.species == 22 && staged.moves == wanted.moves && staged.pp == wanted.pp &&
            staged.ppUps == wanted.ppUps, "staged record carries the kept moves");
    sameIdentity(staged, entry, id + " staged");
    {
        // Review: one caution note on this Pokemon's entry, shown as the value alone.
        const StagedPokemonChange* caution = nullptr;
        for (const auto& change : editor->pendingChanges())
            if (change.label == "Box 1 / Slot 1 Caution") caution = &change;
        require(caution && caution->createdDetail && caution->beforeValue.empty() &&
                caution->afterValue.find(expectedFlag) != std::string::npos,
                id + " Review caution note: " + (caution ? caution->afterValue : std::string("(none)")));
    }
    const auto out = editor->finalizedBytes(error);
    require(!out.empty() && error.empty(), id + " export: " + error);
    auto reparsed = StagedPokemonEditor::create(out, game, error);
    require(reparsed && error.empty(), id + " strict reparse: " + error);
    const auto back = *reparsed->boxedPokemon(0, 0, error);
    require(back.species == 22 && back.moves == wanted.moves && back.pp == wanted.pp && back.ppUps == wanted.ppUps,
            "reparsed Fearow");
    checkKept(entry, back, game, id + " reparsed");
    sameIdentity(back, entry, id + " reparsed");
    require(source == immutable, "source bytes never change");

    // Discard restores the original record.
    editor->discard();
    const auto restored = *editor->boxedPokemon(0, 0, error);
    require(restored.species == 25 && restored.moves == entry.moves && restored.pp == entry.pp &&
            restored.ppUps == entry.ppUps, "Discard restores the original moves");

    // An existing Pokemon left with no usable move cannot be kept moveless.
    {
        Session magikarp;
        magikarp.begin(entry, Mode::Edit);
        require(magikarp.setSpecies(129, id) && magikarp.working.moves == (std::array<uint16_t, 4>{}),
                "Magikarp can use neither Thunder Shock nor Growl");
        require(!magikarp.keep(*editor, 0, 0, error) && error == "Choose at least one move before Keep",
                "Keep refused with the shared message: " + error);
        require(!editor->hasPendingChanges(), "nothing staged");
    }

    // Create: Pikachu Lv5 starts blank and stays blank through a species change; Add needs a chosen
    // Met Location and a move (CreateRequiredFields), then stages and reparses.
    Session create;
    create.beginCreate(editor->trainer(), origin(game));
    require(create.working.moves == (std::array<uint16_t, 4>{}), "Create starts with no moves");
    require(create.setSpecies(22, id) && create.working.moves == (std::array<uint16_t, 4>{}),
            "blank Create stays blank after a species change");
    std::size_t slot = 1;
    const auto beforeAdd = editor->finalizedBytes(error);
    // Met Location is required wherever the exact game's templates offer one for the species.
    const auto fearowEncounters = Encounter::forGameSpeciesWithGen3Provenance(id, 22);
    const bool choosable = !fearowEncounters.empty();
    // Round 11 follow-up (updated): the picker always offers the origin game's native list, so the
    // Met Location is required as an explicit choice for every species (was: only with templates).
    require(create.metLocationChoosable(), "choosable: the origin game has a native list");
    require(!create.add(*editor, 0, slot, error) &&
            error == "Choose a Met Location and at least one move before Keep", id + " blank Add refused: " + error);
    require(editor->finalizedBytes(error) == beforeAdd && !editor->hasPendingChanges(), "a refused Add stages nothing");
    create.working.moves[0] = 64;  // Peck, as the move picker applies it
    create.working.pp[0] = 35;
    require(!create.add(*editor, 0, slot, error) && error == "Choose a Met Location before Keep",
            id + " Met Location still required: " + error);
    if (choosable) {
        create.working.metLocation = fearowEncounters.front().encounter.location;
        require(create.setEncounterMetLevel(fearowEncounters.front().encounter.minLevel), "met level");
        create.noteMetLocationChosen();
    } else {
        // No Fearow template in this game: an unverified native row is still an explicit choice.
        // Round 12 (R12-004): it is in the unverified list, behind "Other location (unverified)...".
        const auto picker = create.metLocationPicker(true);
        const PokeVault::Integration::EncounterGuardrails::EncounterProvenanceChoice* applied = nullptr;
        require(picker.unverifiedList && !picker.rows.empty() &&
                create.applyMetLocationRow(picker, picker.rows.back(), applied) && !applied,
                "choose an other-native row from the unverified list");
    }
    require(create.missingRequiredFields().empty(), "nothing missing");
    require(create.add(*editor, 0, slot, error), id + " add: " + error);
    const auto added = *editor->boxedPokemon(0, slot, error);
    require(added.species == 22 && added.moves == create.working.moves &&
            added.metLocation == create.working.metLocation, "created Fearow carries the chosen moves and Met");
    editor->discard();

    // H25: a Create Pikachu with a chosen encounter and Pokerus, changed to Arbok, loses both; a
    // Horsea encounter survives a change to Seadra (the R2 provenance ancestry edge).
    const auto pikachuEncounters = Encounter::forGameSpeciesWithGen3Provenance(id, 25);
    if (!pikachuEncounters.empty()) {
        Session h;
        h.beginCreate(editor->trainer(), origin(game));
        h.working.metLocation = pikachuEncounters.front().encounter.location;
        require(h.setEncounterMetLevel(pikachuEncounters.front().encounter.minLevel), "Pikachu met level");
        h.noteMetLocationChosen();
        h.working.pokerus = 0x21;
        const auto met = h.working.metLocation;
        require(h.setSpecies(24, id), "Create Pikachu -> Arbok");
        require(h.working.pokerus == 0, "Create: Pokerus resets");
        // Round 12 (owner H08 / R12-005; updated pin): still valid exactly when it is a verified
        // template of Arbok or its pre-evolution Ekans that Arbok's level reaches (the picker's rule).
        const bool arbokThere = PokeBank::UIModel::Gen3EncounterPicker::verifiedTemplate(
            PokeBank::UIModel::Gen3EncounterPicker::templatesForOrigin(origin(game), 24), met,
            pikachuEncounters.front().encounter.minLevel, h.working.level) != nullptr;
        if (!arbokThere)
            require(h.encounterReset && !h.metLocationChosen && h.working.metLocation == 0 && h.working.metLevel == 0,
                    "Create: Arbok was never met at Pikachu's location -> encounter cleared");
        // Round 11 follow-up: the reset is visible -- the status the surface posts names it.
        require(h.speciesChangeResetStatus() ==
                    (arbokThere ? std::string("Species changed: Pokérus cleared")
                                : std::string("Species changed: Pokérus cleared and Met Location needs a new choice")),
                id + " Create reset status: " + h.speciesChangeResetStatus());
        require(h.setSpecies(24, id) && h.speciesChangeResetStatus().empty(), "a reselect resets and says nothing");
    }
    const auto horsea = Encounter::forGameSpeciesWithGen3Provenance(id, 116);
    if (!horsea.empty()) {
        Session h;
        h.beginCreate(editor->trainer(), origin(game));
        require(h.setSpecies(116, id), "Create Horsea");
        h.working.metLocation = horsea.front().encounter.location;
        require(h.setEncounterMetLevel(horsea.front().encounter.minLevel), "Horsea met level");
        h.noteMetLocationChosen();
        const auto horseaMet = h.working.metLocation;
        const auto horseaMetLevel = h.working.metLevel;
        require(h.setSpecies(117, id), "Create Horsea -> Seadra");
        // Round 12 (owner H08 / R12-005; updated pin): Seadra's own templates and its Horsea
        // ancestry both count, once Seadra's level reaches the template's minimum.
        const bool seadraThere = PokeBank::UIModel::Gen3EncounterPicker::verifiedTemplate(
            PokeBank::UIModel::Gen3EncounterPicker::templatesForOrigin(origin(game), 117), horseaMet,
            horseaMetLevel, h.working.level) != nullptr;
        require(h.encounterReset == !seadraThere && h.metLocationChosen == seadraThere,
                "Create: the Horsea location is kept exactly when it is valid for Seadra");
        if (Encounter::forGameSpecies(id, 117).empty() && h.working.level >= horseaMetLevel)
            require(seadraThere, "Create: with no Seadra templates the Horsea ancestry location stays valid");
    }
    // The same Create draft in Edit mode keeps everything (existing record's history).
    if (!pikachuEncounters.empty()) {
        StagedPokemonRecord existing = entry;
        existing.metLocation = pikachuEncounters.front().encounter.location;
        existing.metLevel = pikachuEncounters.front().encounter.minLevel;
        existing.pokerus = 0x21;
        Session e;
        e.begin(existing, Mode::Edit);
        require(e.setSpecies(24, id) && e.working.pokerus == 0x21 && e.working.metLocation == existing.metLocation &&
                e.working.metLevel == existing.metLevel && !e.encounterReset, "Edit keeps Met Location and Pokerus");
    }
}

// Round 11 follow-up (independent QA finding 1, owner rule): Met Location is an EXPLICIT choice from
// the exact game's own list. For every species 1-386 created in each of the five games (Create
// origin = that game): the picker model offers at least one choosable row; Keep is refused until a
// row is chosen (a move is already set, so only the Met Location is missing); choosing an offered
// row makes Keep possible and the stored location round-trips through stage -> reparse. Verified
// catch templates and unverified native rows are labelled distinctly. QA counted 1,267 Gen III /
// Crystal combinations with no row; the count of empty combinations must now be 0.
//
// Round 12 (R12-004 / R12-005, owner H14 "pikachu has like 87 met locaations we need just the legit
// ones", H08 "evolved,traded gifted"; updated deliberately): the picker opens on the DEFAULT list --
// only verified templates of the species / its pre-evolutions that the draft's level reaches, the
// gift egg / trade / event markers, and ONE last row "Other location (unverified)..." -- and that row
// opens the UNVERIFIED list (every other native place, plus the templates above the level, each
// saying "level is below this encounter"). Both lists are exercised row by row.
struct MetLocationCounts { std::size_t empty = 0, markersOnly = 0; };
MetLocationCounts exhaustiveMetLocation(SourceGame game, Family family) {
    namespace Picker = PokeBank::UIModel::Gen3EncounterPicker;
    namespace Rows = PokeBank::UIModel::ExactGameMetLocations;
    namespace Policy = PokeVault::Integration::MetLocationPolicy;
    const std::string id = sourceId(game);
    auto source = fixture(family, game);
    std::string error;
    auto editor = StagedPokemonEditor::create(source, game, error);
    require(editor && error.empty(), id + " editor: " + error);
    MetLocationCounts counts;
    std::size_t rowsChecked = 0, stagedChecked = 0, templateRows = 0, markerRows = 0, otherRows = 0, levelRows = 0;
    for (uint16_t species = 1; species <= 386; ++species) {
        const std::string what = id + " " + std::to_string(species);
        Session base;
        base.beginCreate(editor->trainer(), origin(game));
        require(base.setSpecies(species, id), id + " Create species " + std::to_string(species));
        const auto picker = base.metLocationPicker();
        if (picker.rows.empty()) { ++counts.empty; continue; }
        const auto unverified = base.metLocationPicker(true);
        require(!picker.unverifiedList && unverified.unverifiedList && !unverified.rows.empty(),
                what + ": both lists are offered");
        require(base.metLocationChoosable(), what + ": choosable");
        // A move the species can use in this exact game, as the move picker would offer it.
        uint16_t move = 0;
        for (uint16_t m = 1; m < Learnset::kMoveCount && !move; ++m)
            if (Learnset::directlyLearnable(game, species, m)) move = m;
        require(move != 0, what + ": a learnable move exists");
        base.working.moves[0] = move;
        base.working.pp[0] = Names::getMoveBasePP(move, Enums::GameVersion::FRLG);
        require(base.missingRequiredFields() == "Choose a Met Location before Keep",
                what + ": Keep refused until a row is chosen: " + base.missingRequiredFields());
        {
            Session refused = base;
            const auto before = editor->finalizedBytes(error);
            require(!refused.add(*editor, 0, 1, error) && error == "Choose a Met Location before Keep" &&
                    !editor->hasPendingChanges() && editor->finalizedBytes(error) == before,
                    what + ": refused Add stages nothing");
        }
        // DEFAULT list: verified templates the level reaches + markers, then the one unverified entry.
        require(picker.rows.back() == Picker::kUnverifiedListRow &&
                std::count(picker.rows.begin(), picker.rows.end(), Picker::kUnverifiedListRow) == 1,
                what + ": the default list ends with the one 'Other location (unverified)...' row");
        std::size_t verifiedHere = 0;
        std::array<bool, 3> markerCovered{};   // 253 gift egg, 254 trade, 255 event
        for (const auto row : picker.rows) {
            Session chosen = base;
            const PokeVault::Integration::EncounterGuardrails::EncounterProvenanceChoice* applied = nullptr;
            const auto text = Picker::rowText(picker.choices, row, chosen.working.originGame,
                                              base.working.metLocation, base.working.metLevel, base.working.level);
            require(text.detail.find("legal") == std::string::npos, what + ": no row says legal");
            require(row != Picker::kKeepStoredRow, "a new draft has no stored location to keep");
            if (row == Picker::kUnverifiedListRow) {
                // Not a location: it opens the unverified list and leaves the draft unchosen.
                require(text.place == Policy::kUnverifiedListRowPlace && !text.verifiedCatch &&
                        !chosen.applyMetLocationRow(picker, row, applied) && !chosen.metLocationChosen &&
                        chosen.missingRequiredFields() == "Choose a Met Location before Keep",
                        what + ": the unverified entry row is not a choice");
                continue;
            }
            require(chosen.applyMetLocationRow(picker, row, applied), what + ": apply row");
            require(chosen.metLocationChosen && chosen.missingRequiredFields().empty(),
                    what + ": any offered row makes Keep possible");
            const bool unverifiedText = text.detail.find("unverified") != std::string::npos;
            if (Rows::isOtherNativeRow(row)) {
                // Only the game's own markers, labelled as markers.
                ++markerRows;
                markerCovered[Rows::otherNativeRowLocation(row) - 253] = true;
                require(!applied && !text.verifiedCatch && unverifiedText &&
                        Rows::gen3MarkerLocation(Rows::otherNativeRowLocation(row)) &&
                        text.detail.find("marker") != std::string::npos &&
                        chosen.working.metLocation == Rows::otherNativeRowLocation(row),
                        what + ": a default non-template row is a labelled marker: " + text.detail);
            } else {
                ++templateRows; ++verifiedHere;
                if (applied && Rows::gen3MarkerLocation(applied->encounter.location))
                    markerCovered[applied->encounter.location - 253] = true;   // a verified row there instead
                // R12-005: never a template above the draft's level.
                require(applied && text.verifiedCatch && !unverifiedText && text.detail.find("Lv. ") != std::string::npos &&
                        applied->encounter.minLevel <= base.working.level &&
                        chosen.working.metLocation == applied->encounter.location &&
                        chosen.working.metLevel <= chosen.working.level &&
                        Picker::verifiedTemplate(picker.choices, chosen.working.metLocation, chosen.working.metLevel,
                                                 chosen.working.level) != nullptr,
                        what + ": a default template row is a verified catch the level reaches: " + text.detail);
            }
            require(Names::isGen3NativeMetLocation(origin(game), chosen.working.metLocation),
                    what + ": every offered row is native to the exact game");
            ++rowsChecked;
        }
        // Never empty: a species with no verified row here still shows the three markers.
        require(markerCovered == (std::array<bool, 3>{true, true, true}),
                what + ": the gift egg / trade / event markers are in the default list");
        if (verifiedHere == 0) ++counts.markersOnly;
        // UNVERIFIED list: reachable, labelled, every row an explicit choice that stays unverified.
        for (const auto row : unverified.rows) {
            Session chosen = base;
            const PokeVault::Integration::EncounterGuardrails::EncounterProvenanceChoice* applied = nullptr;
            require(row != Picker::kUnverifiedListRow && row != Picker::kKeepStoredRow, what + ": unverified rows");
            require(chosen.applyMetLocationRow(unverified, row, applied) && !applied, what + ": apply unverified row");
            require(chosen.metLocationChosen && chosen.missingRequiredFields().empty(),
                    what + ": an unverified row is still an explicit choice");
            const auto text = Picker::rowText(unverified.choices, row, chosen.working.originGame,
                                              base.working.metLocation, base.working.metLevel, base.working.level);
            require(!text.verifiedCatch && text.detail.find("legal") == std::string::npos, what + ": never verified");
            if (Rows::isOtherNativeRow(row)) {
                ++otherRows;
                require(text.detail == Rows::kOtherPlaceDetail &&
                        !Rows::gen3MarkerLocation(Rows::otherNativeRowLocation(row)),
                        what + ": an other place is labelled unverified: " + text.detail);
            } else {
                ++levelRows;   // R12-005: a template above the level, kept but moved here
                const auto* choice = Picker::choiceAt(unverified.choices, row);
                require(choice && choice->encounter.minLevel > base.working.level &&
                        text.detail.find(Policy::kLevelBelowEncounter) != std::string::npos,
                        what + ": a template above the level says why: " + text.detail);
            }
            // The editor / Origin & Legality / Review line stays unverified after the choice.
            const auto originText = Picker::encounterOriginText(chosen.metLocationPicker().choices, chosen.working.species,
                                                                chosen.working.metLocation, chosen.working.metLevel,
                                                                chosen.working.level);
            // (A place that also has a verified row the level reaches -- another method there -- reads
            // as that verified row exactly when the stored met level fits it.)
            const bool verifiedThere = Picker::verifiedTemplate(unverified.choices, chosen.working.metLocation,
                                                                chosen.working.metLevel, chosen.working.level);
            require(originText.find("unverified") != std::string::npos ||
                        (verifiedThere && Picker::reachedTemplateAt(unverified.choices, chosen.working.metLocation,
                                                                    chosen.working.level)),
                    what + ": stays unverified: " + originText);
            require(Names::isGen3NativeMetLocation(origin(game), chosen.working.metLocation),
                    what + ": every unverified row is native to the exact game");
            ++rowsChecked;
        }
        // Stage and reparse: the first and the last real default row, one rotating default row, and
        // the first and last unverified rows of every species.
        const std::size_t lastDefault = picker.rows.size() - 2;
        const std::array<std::pair<bool, std::size_t>, 5> picks{{
            {false, 0}, {false, lastDefault}, {false, species % (lastDefault + 1)},
            {true, 0}, {true, unverified.rows.size() - 1}}};
        for (std::size_t k = 0; k < picks.size(); ++k) {
            bool repeated = false;
            for (std::size_t j = 0; j < k; ++j) repeated |= picks[j] == picks[k];
            if (repeated) continue;
            const auto& list = picks[k].first ? unverified : picker;
            Session chosen = base;
            const PokeVault::Integration::EncounterGuardrails::EncounterProvenanceChoice* applied = nullptr;
            require(chosen.applyMetLocationRow(list, list.rows[picks[k].second], applied), "apply");
            const auto location = chosen.working.metLocation;
            const auto metLevel = chosen.working.metLevel;
            require(chosen.add(*editor, 0, 1, error), what + " add: " + error);
            const auto out = editor->finalizedBytes(error);
            require(!out.empty() && error.empty(), id + " export: " + error);
            auto reparsed = StagedPokemonEditor::create(out, game, error);
            require(reparsed && error.empty(), id + " strict reparse: " + error);
            const auto back = reparsed->boxedPokemon(0, 1, error);
            require(back && back->species == species && back->metLocation == location && back->metLevel == metLevel &&
                    back->originGame == origin(game),
                    what + ": the chosen Met Location round-trips");
            editor->discard();
            ++stagedChecked;
        }
    }
    require(templateRows > 0 && markerRows > 0 && otherRows > 0 && levelRows > 0,
            id + ": every kind of row is exercised");
    std::cout << id << ": 386 species, " << rowsChecked << " rows chosen (" << templateRows << " verified, "
              << markerRows << " markers, " << otherRows << " unverified places, " << levelRows
              << " above the level), " << stagedChecked << " staged and reparsed, " << counts.markersOnly
              << " markers only, " << counts.empty << " empty\n";
    return counts;
}

// Round 12 (R12-004 / R12-005): the owner's screenshots, pinned. Ruby Pikachu's default list is its
// verified Safari Zone catch, the three markers and the unverified entry (was 87 rows); an evolved
// Raichu is offered Pikachu's catch only once its level reaches it; a Lv. 5 Raichu shows markers.
void ownerExamples() {
    namespace Picker = PokeBank::UIModel::Gen3EncounterPicker;
    namespace Rows = PokeBank::UIModel::ExactGameMetLocations;
    namespace Policy = PokeVault::Integration::MetLocationPolicy;
    auto source = fixture(Family::RS, SourceGame::RubyGBA);
    std::string error;
    auto editor = StagedPokemonEditor::create(source, SourceGame::RubyGBA, error);
    require(editor && error.empty(), "ruby editor: " + error);
    Session pikachu;
    pikachu.beginCreate(editor->trainer(), 2);
    require(pikachu.setSpecies(25, "ruby_gba") && pikachu.setLevel(25), "Ruby Pikachu Lv. 25");
    const auto rows = pikachu.metLocationPicker().rows;
    const auto all = pikachu.metLocationPicker(true).rows;
    std::cout << "Ruby Pikachu Lv. 25: default list " << rows.size() << " rows (was 87), unverified list "
              << all.size() << " rows\n";
    require(rows.size() == 5 && rows.front() == 0 && rows.back() == Picker::kUnverifiedListRow &&
            rows[1] == Rows::otherNativeRow(253) && rows[2] == Rows::otherNativeRow(254) &&
            rows[3] == Rows::otherNativeRow(255), "Ruby Pikachu: Safari Zone, 3 markers, Other location");
    require(rows.size() - 1 + all.size() == 87, "every one of the 87 earlier rows is still reachable");
    Session raichu;
    raichu.beginCreate(editor->trainer(), 2);
    require(raichu.setSpecies(26, "ruby_gba") && raichu.working.level == 5, "Ruby Raichu Lv. 5");
    auto low = raichu.metLocationPicker();
    require(Picker::verifiedRowCount(low.choices, low.rows) == 0 && low.rows.size() == 4,
            "Lv. 5 Raichu: markers only (Pikachu's Safari Zone Lv. 25 is above its level)");
    const auto blocked = raichu.metLocationPicker(true);
    require(blocked.rows.front() == 0 &&
            Picker::rowText(blocked.choices, 0, 2, 0, 0, 5).detail.find(Policy::kLevelBelowEncounter) != std::string::npos,
            "the Safari Zone row is in the unverified list with its reason");
    require(raichu.setLevel(30), "Raichu Lv. 30");
    auto high = raichu.metLocationPicker();
    require(Picker::verifiedRowCount(high.choices, high.rows) == 1 &&
            Picker::rowText(high.choices, 0, 2, 0, 0, 30).detail.find("via Pikachu") != std::string::npos,
            "Lv. 30 Raichu: Pikachu's Safari Zone catch is verified ancestry");
    const PokeVault::Integration::EncounterGuardrails::EncounterProvenanceChoice* applied = nullptr;
    require(raichu.applyMetLocationRow(high, 0, applied) && applied && raichu.encounterStillValid() &&
            Picker::encounterOriginText(high.choices, 26, raichu.working.metLocation, raichu.working.metLevel,
                                        raichu.working.level) == "Pikachu → Raichu", "evolved encounter origin");
    // H25: an existing Lv. 5 Raichu whose stored history is that Lv. 25 catch keeps it (Keep stored),
    // and it reads unverified; nothing is deleted.
    StagedPokemonRecord existing = raichu.working;
    existing.level = 5;
    existing.metLevel = 25;
    Session edit;
    edit.begin(existing, Mode::Edit);
    const auto kept = edit.metLocationPicker();
    require(kept.rows.front() == Picker::kKeepStoredRow && edit.working.metLocation == existing.metLocation &&
            Picker::encounterOriginText(kept.choices, 26, existing.metLocation, 25, 5).find("unverified") != std::string::npos,
            "a stored history above the level is kept and flagged unverified");
}

} // namespace

int main() {
    run(SourceGame::RubyGBA, Family::RS);
    run(SourceGame::SapphireGBA, Family::RS);
    run(SourceGame::EmeraldGBA, Family::Emerald);
    run(SourceGame::FireRedGBA, Family::FRLG);
    run(SourceGame::LeafGreenGBA, Family::FRLG);

    std::size_t emptyCombinations = 0, markersOnly = 0;
    for (const auto& [game, family] : {std::pair{SourceGame::RubyGBA, Family::RS}, {SourceGame::SapphireGBA, Family::RS},
                                       {SourceGame::EmeraldGBA, Family::Emerald}, {SourceGame::FireRedGBA, Family::FRLG},
                                       {SourceGame::LeafGreenGBA, Family::FRLG}}) {
        const auto counts = exhaustiveMetLocation(game, family);
        emptyCombinations += counts.empty;
        markersOnly += counts.markersOnly;
    }
    require(emptyCombinations == 0, "Gen III species x game combinations with no Met Location row: " +
                                        std::to_string(emptyCombinations));
    std::cout << "Gen III species x game combinations with markers only (no verified row at the Create level): "
              << markersOnly << " of 1930\n";
    ownerExamples();

    // Exact-game routing through the session: no move is added in either game.
    StagedPokemonRecord base{};
    base.species = 5;
    base.level = 5;
    Session fr, em;
    fr.begin(base, Mode::Edit);
    em.begin(base, Mode::Edit);
    require(fr.setSpecies(6, "firered_gba") && em.setSpecies(6, "emerald_gba"), "Charmeleon -> Charizard");
    require(fr.working.moves == (std::array<uint16_t, 4>{}) && em.working.moves == (std::array<uint16_t, 4>{}),
            "a moveless Charmeleon stays moveless in both games");
    // No source id: the draft's own origin game decides; no origin either: empty slots.
    Session origin3;
    base.originGame = 3;
    base.moves = {84, 45, 0, 0};
    base.pp = {30, 40, 0, 0};
    origin3.begin(base, Mode::Edit);
    require(origin3.setSpecies(22) && origin3.working.moves == (std::array<uint16_t, 4>{45, 0, 0, 0}),
            "origin game fallback: Emerald Fearow keeps Growl only");
    Session none;
    base.originGame = 0;
    none.begin(base, Mode::Edit);
    require(none.setSpecies(22) && none.working.moves == (std::array<uint16_t, 4>{}), "no game: empty slots");

    // Round 11 H24 (owner PASS): species / held item / move / IV edit -> Review -> Discard, byte for byte.
    require(r11_h24::runAll() == 0, "Round 11 H24 Gen III edit and discard");

    std::cout << "Gen III blank Create, species change keeps usable moves, Create history reset (Ruby, "
                 "Sapphire, Emerald, FireRed, LeafGreen; keep, add, export, reparse, Discard): PASS\n";
    return 0;
}
