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
    require(fearow.working.moves == (std::array<uint16_t, 4>{45, 0, 0, 0}) && fearow.working.pp[0] == entry.pp[1],
            "Fearow: Thunder Shock cleared, Growl kept with its PP");
    const auto wanted = fearow.working;
    require(fearow.keep(*editor, 0, 0, error), id + " keep: " + error);
    const auto staged = *editor->boxedPokemon(0, 0, error);
    require(staged.species == 22 && staged.moves == wanted.moves && staged.pp == wanted.pp &&
            staged.ppUps == wanted.ppUps, "staged record carries the kept moves");
    sameIdentity(staged, entry, id + " staged");
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
    require(create.metLocationChoosable() == choosable, "choosable follows the exact-game templates");
    require(!create.add(*editor, 0, slot, error) &&
            error == (choosable ? "Choose a Met Location and at least one move before Keep"
                                : "Choose at least one move before Keep"), id + " blank Add refused: " + error);
    require(editor->finalizedBytes(error) == beforeAdd && !editor->hasPendingChanges(), "a refused Add stages nothing");
    create.working.moves[0] = 64;  // Peck, as the move picker applies it
    create.working.pp[0] = 35;
    if (choosable) {
        require(!create.add(*editor, 0, slot, error) && error == "Choose a Met Location before Keep",
                id + " Met Location still required: " + error);
        create.working.metLocation = fearowEncounters.front().encounter.location;
        require(create.setEncounterMetLevel(fearowEncounters.front().encounter.minLevel), "met level");
        create.noteMetLocationChosen();
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
        bool arbokThere = false;
        for (const auto& c : Encounter::forGameSpeciesWithGen3Provenance(id, 24))
            arbokThere |= c.encounter.location == met;
        if (!arbokThere)
            require(h.encounterReset && !h.metLocationChosen && h.working.metLocation == 0 && h.working.metLevel == 0,
                    "Create: Arbok was never met at Pikachu's location -> encounter cleared");
    }
    const auto horsea = Encounter::forGameSpeciesWithGen3Provenance(id, 116);
    if (!horsea.empty()) {
        Session h;
        h.beginCreate(editor->trainer(), origin(game));
        require(h.setSpecies(116, id), "Create Horsea");
        h.working.metLocation = horsea.front().encounter.location;
        require(h.setEncounterMetLevel(horsea.front().encounter.minLevel), "Horsea met level");
        h.noteMetLocationChosen();
        bool seadraThere = false;   // through Seadra's own templates or, without any, Horsea ancestry
        for (const auto& c : Encounter::forGameSpeciesWithGen3Provenance(id, 117))
            seadraThere |= c.encounter.location == h.working.metLocation && c.encounter.containsLevel(h.working.metLevel);
        require(h.setSpecies(117, id), "Create Horsea -> Seadra");
        require(h.encounterReset == !seadraThere && h.metLocationChosen == seadraThere,
                "Create: the Horsea location is kept exactly when it is valid for Seadra");
        if (Encounter::forGameSpecies(id, 117).empty())
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

} // namespace

int main() {
    run(SourceGame::RubyGBA, Family::RS);
    run(SourceGame::SapphireGBA, Family::RS);
    run(SourceGame::EmeraldGBA, Family::Emerald);
    run(SourceGame::FireRedGBA, Family::FRLG);
    run(SourceGame::LeafGreenGBA, Family::FRLG);

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

    std::cout << "Gen III blank Create, species change keeps usable moves, Create history reset (Ruby, "
                 "Sapphire, Emerald, FireRed, LeafGreen; keep, add, export, reparse, Discard): PASS\n";
    return 0;
}
