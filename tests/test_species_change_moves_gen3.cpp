// R2-BUG-004, Generation III: Ruby, Sapphire, Emerald, FireRed, LeafGreen through the real
// Session::setSpecies (the Gen III species picker's only apply) and the real StagedPokemonEditor,
// ending in a strict export -> reparse. A species change brings the new species' native moves for
// the EXACT game (pret pokeruby / pokeemerald / pokefirered GiveMonInitialMoveset; FireRed and
// LeafGreen differ) with base PP and no PP Ups; the same species changes nothing; Discard
// restores; every field other than the species-dependent ones survives.
#include "Encryption/Encryption3FRLG.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Names/MoveInfo.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "Pokemon/SpeciesMoveDefaults.h"
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
namespace Moves = PokeBank::Moves;
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

void checkNative(const StagedPokemonRecord& p, SourceGame game, const std::string& what) {
    const auto d = Moves::defaultMoves(Moves::levelUpTableForGen3Version(origin(game)), p.species, 0, p.level);
    require(d.known && p.moves == d.moves, what + ": native default moves");
    for (std::size_t i = 0; i < 4; ++i) {
        require(p.ppUps[i] == 0, what + ": PP Ups 0");
        require(p.pp[i] == Names::getMoveBasePP(p.moves[i], Enums::GameVersion::FRLG), what + ": base PP");
    }
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

    // Several transitions with different learnsets, Edit sessions.
    Session s;
    s.begin(entry, Mode::Edit);
    for (const uint16_t species : {22, 1, 252, 133, 386, 51, 25}) {
        require(s.setSpecies(species, id), id + " setSpecies " + std::to_string(species));
        checkNative(s.working, game, id + " -> " + std::to_string(species));
        require(species == 25 || std::find(s.working.moves.begin(), s.working.moves.end(), 84) == s.working.moves.end(),
                "Thunder Shock never survives a change away from Pikachu");
        s.working.moves[3] = 33;  // a hand edit after the change
        s.working.pp[3] = 35;
        const auto before = s.working;
        require(s.setSpecies(species, id), "reselect");
        require(s.working.moves == before.moves && s.working.pp == before.pp && s.working.ppUps == before.ppUps &&
                s.working.level == before.level, "same species changes nothing");
    }

    // Pikachu -> Fearow, Keep, export -> strict reparse.
    Session fearow;
    fearow.begin(entry, Mode::Edit);
    require(fearow.setSpecies(22, id), "Pikachu -> Fearow");
    const auto wanted = fearow.working;
    require(fearow.keep(*editor, 0, 0, error), id + " keep: " + error);
    const auto staged = *editor->boxedPokemon(0, 0, error);
    require(staged.species == 22 && staged.moves == wanted.moves && staged.pp == wanted.pp &&
            staged.ppUps == wanted.ppUps, "staged record carries the new moves");
    sameIdentity(staged, entry, id + " staged");
    const auto out = editor->finalizedBytes(error);
    require(!out.empty() && error.empty(), id + " export: " + error);
    auto reparsed = StagedPokemonEditor::create(out, game, error);
    require(reparsed && error.empty(), id + " strict reparse: " + error);
    const auto back = *reparsed->boxedPokemon(0, 0, error);
    require(back.species == 22 && back.moves == wanted.moves && back.pp == wanted.pp && back.ppUps == wanted.ppUps,
            "reparsed Fearow");
    checkNative(back, game, id + " reparsed");
    sameIdentity(back, entry, id + " reparsed");
    require(source == immutable, "source bytes never change");

    // Discard restores the original record.
    editor->discard();
    const auto restored = *editor->boxedPokemon(0, 0, error);
    require(restored.species == 25 && restored.moves == entry.moves && restored.pp == entry.pp &&
            restored.ppUps == entry.ppUps, "Discard restores the original moves");

    // Create: Pikachu Lv5 (empty moves) -> Fearow, Add, reparse.
    Session create;
    create.beginCreate(editor->trainer(), origin(game));
    require(create.setSpecies(22, id), "Create species change");
    checkNative(create.working, game, id + " Create");
    std::size_t slot = 1;
    require(create.add(*editor, 0, slot, error), id + " add: " + error);
    const auto added = *editor->boxedPokemon(0, slot, error);
    require(added.species == 22 && added.moves == create.working.moves, "created Fearow carries the native moves");
    editor->discard();
}

} // namespace

int main() {
    run(SourceGame::RubyGBA, Family::RS);
    run(SourceGame::SapphireGBA, Family::RS);
    run(SourceGame::EmeraldGBA, Family::Emerald);
    run(SourceGame::FireRedGBA, Family::FRLG);
    run(SourceGame::LeafGreenGBA, Family::FRLG);

    // Exact-game routing through the session. At the species-change level policy's levels no
    // species differs FireRed vs LeafGreen or Ruby/Sapphire vs Emerald (their tables differ only
    // at levels the policy never picks: test_species_change_moves_gen15 checks the FR / LG
    // Deoxys and Dugtrio tables directly), but FireRed vs Emerald Charizard does.
    Session fr, em;
    StagedPokemonRecord base{};
    base.species = 5;
    base.level = 5;
    fr.begin(base, Mode::Edit);
    em.begin(base, Mode::Edit);
    require(fr.setSpecies(6, "firered_gba") && em.setSpecies(6, "emerald_gba"), "Charmeleon -> Charizard");
    require(fr.working.level == em.working.level, "same level policy");
    require(fr.working.moves == Moves::defaultMoves(Moves::LevelUpTable15::FireRed, 6, 0, fr.working.level).moves &&
            em.working.moves == Moves::defaultMoves(Moves::LevelUpTable15::Emerald, 6, 0, em.working.level).moves &&
            fr.working.moves != em.working.moves, "FireRed and Emerald get their own Charizard moves");
    // No source id: the draft's own origin game decides; no origin either: empty slots.
    Session origin3;
    base.originGame = 3;
    origin3.begin(base, Mode::Edit);
    require(origin3.setSpecies(22) &&
            origin3.working.moves == Moves::defaultMoves(Moves::LevelUpTable15::Emerald, 22, 0, origin3.working.level).moves,
            "origin game fallback");
    Session none;
    base.originGame = 0;
    base.moves = {84, 45, 0, 0};
    none.begin(base, Mode::Edit);
    require(none.setSpecies(22) && none.working.moves == (std::array<uint16_t, 4>{}), "no game: empty slots");

    std::cout << "Gen III species change resets moves (Ruby, Sapphire, Emerald, FireRed, LeafGreen; keep, add, "
                 "export, reparse, Discard; exact-game routing): PASS\n";
    return 0;
}
