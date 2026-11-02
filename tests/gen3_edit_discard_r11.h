// Round 11 H24 (owner PASS on the inferred Round 10 build 3634d1e6, Gen III / Editor): "In Gen III stage
// species/held item/move/stat change on a dispensable Pokemon; inspect Review then discard. Expected:
// only intended fields change in preview; original Pokemon returns exactly after discard."
//
// Permanent regression for that PASS (tests/test_hw_round1_regression_gate.py GUARDS_ROUND11['H24']),
// built and run inside test_species_change_moves_gen3 (an existing host-test / host-sanitize target that
// already compiles the Gen III editor and Session), so no Makefile.host* change is needed: a change there
// would make the bound Round 13 candidate stale for the NRO audit's staleness guard.
// for Ruby, Sapphire, Emerald, FireRed and LeafGreen, through the editor's real paths: the Gen III
// Session (the species picker's setSpecies, the held-item / move / IV rows writing `working`) and
// Session::keep -> StagedPokemonEditor::stageBoxPokemonEdit.
//   1. A draft is a preview: until Keep, the staged save is byte-identical to the source.
//   2. Keep stages exactly one Pokemon: Review lists only Box 1 / Slot 1 rows, and only the rows a
//      species / held item / move / IV edit can produce (Species, its Level / Experience and the
//      history notes, Held Item, the Move / PP rows, the IV row). Identity rows (OT, Trainer ID, PID,
//      Nature, Gender, Ability, Shiny, Ball, Met, Friendship, Pokerus, EVs, Language) never appear.
//   3. Every other box record is byte-identical after a strict finalize -> reparse, and the edited
//      record keeps its identity (PID, OT, IDs, EVs, Ball, Met, Friendship, Pokerus, origin, language).
//   4. Discard restores the original exactly: staged bytes == source, no pending change, the record's
//      encrypted bytes equal the original record's; the caller's source vector never changes.
// The level a species change leaves is deliberately NOT asserted here (owner Round 13 decision 9 is
// changing that rule); only that it is reported as a Level / Experience row when it changes.
// The save is synthetic, built in code (the same layout tests/test_species_change_moves_gen3.cpp
// builds); no real save, ROM or Pokemon file is read.
#pragma once
#include "Encryption/Encryption3FRLG.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "UI/Gen3SharedPokemonSession.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace r11_h24 {

using namespace PokeVault::Integration::Gen3;
using PokeBank::UIModel::Gen3SharedEditor::Session;
using Mode = PokeBank::UIModel::Gen3SharedEditor::Mode;

inline int failures = 0;
inline void require(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAILED: " << what << '\n';
        ++failures;
    }
}

enum class Family { RS, Emerald, FRLG };
using LogicalSectors = std::array<std::array<uint8_t, Detail::kSectorSize>, Detail::kSectorCount>;

inline void write16(std::span<uint8_t> bytes, std::size_t offset, uint16_t value) {
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}
inline void write32(std::span<uint8_t> bytes, std::size_t offset, uint32_t value) {
    write16(bytes, offset, static_cast<uint16_t>(value));
    write16(bytes, offset + 2, static_cast<uint16_t>(value >> 16));
}
inline uint8_t origin(SourceGame game) {
    switch (game) {
        case SourceGame::SapphireGBA: return 1;
        case SourceGame::RubyGBA: return 2;
        case SourceGame::EmeraldGBA: return 3;
        case SourceGame::FireRedGBA: return 4;
        case SourceGame::LeafGreenGBA: return 5;
    }
    return 0;
}
inline const char* sourceId(SourceGame game) {
    switch (game) {
        case SourceGame::RubyGBA: return "ruby_gba";
        case SourceGame::SapphireGBA: return "sapphire_gba";
        case SourceGame::EmeraldGBA: return "emerald_gba";
        case SourceGame::FireRedGBA: return "firered_gba";
        case SourceGame::LeafGreenGBA: return "leafgreen_gba";
    }
    return "";
}
// A nicknamed Pikachu (so a species change never renames it), Lv20, holding a Potion.
inline std::array<uint8_t, 80> pikachu(SourceGame game, uint32_t pid) {
    std::array<uint8_t, 80> blank{};
    Pokemon::Pokemon3FRLG p(std::span<const std::byte>(reinterpret_cast<const std::byte*>(blank.data()), blank.size()));
    p.setPID(pid);
    p.setTID16(12345);
    p.setSID16(54321);
    p.setSpecies(25);
    p.setLanguage(2);
    p.setOTName(u"RED");
    p.setNickname(u"SPARKY");
    p.setHeldItem(13);
    p.setFriendship(70);
    p.setOriginGame(origin(game));
    p.setBall(4);
    p.setMetLevel(20);
    p.setMetLocation(1);
    p.setPokerus(0x21);
    p.setLevel(20);
    p.setMove(0, 84);   // Thunder Shock
    p.setMovePP(0, 30);
    p.setMove(1, 45);   // Growl
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
// Box 1 slot 1: the Pikachu to edit; Box 1 slot 2: a neighbour that must never change.
inline LogicalSectors makeLogical(Family family, SourceGame game) {
    LogicalSectors logical{};
    constexpr uint32_t key = 0xA1B2C3D4;
    if (family == Family::FRLG) {
        write32(logical[0], 0xAC, 1);
        write32(logical[0], 0xF20, key);
        write32(logical[1], 0x0290, key);
    } else if (family == Family::Emerald) {
        write32(logical[0], 0xAC, key);
        logical[0][0xEE0] = 0x42;
        write32(logical[1], 0x0490, key);
    } else {
        write32(logical[0], 0xAC, 0);
        write32(logical[1], 0x0490, 0);
    }
    write16(logical[0], 0x0A, 12345);
    write16(logical[0], 0x0C, 54321);
    const std::array<std::array<uint8_t, 80>, 2> mons{pikachu(game, 0x12345678u), pikachu(game, 0x0BADF00Du)};
    for (std::size_t m = 0; m < mons.size(); ++m)
        for (std::size_t i = 0; i < 80; ++i) {
            const std::size_t position = 4 + m * 80 + i;
            logical[5 + position / Detail::kSectorDataSize][position % Detail::kSectorDataSize] = mons[m][i];
        }
    return logical;
}
inline void writeSlot(std::vector<uint8_t>& save, uint8_t slot, uint32_t counter, uint8_t rotation, Family family,
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
inline std::vector<uint8_t> fixture(Family family, SourceGame game) {
    std::vector<uint8_t> save(Detail::kSaveSize, 0);
    writeSlot(save, 0, 7, 0, family, game);
    writeSlot(save, 1, 9, 5, family, game);
    return save;
}

inline bool startsWith(const std::string& s, const std::string& prefix) { return s.rfind(prefix, 0) == 0; }

// Review rows a species / held item / move / IV edit may produce for Box 1 / Slot 1.
inline bool intendedRow(const std::string& label) {
    const std::string at = "Box 1 / Slot 1 ";
    if (!startsWith(label, at)) return false;
    const std::string field = label.substr(at.size());
    static const char* const allowed[] = {"Species", "Level", "Experience", "Held Item", "HP IV", "Caution"};
    for (const char* a : allowed)
        if (field == a) return true;
    // Moves and their PP follow the species change and the hand-picked move.
    if (startsWith(field, "Move ") || startsWith(field, "PP ") || startsWith(field, "PP Ups ")) return true;
    // Round 12 R12-004 / R12-005: a changed level may add the one "unverified Met" note (createdDetail).
    return field.find("Met") != std::string::npos && field != "Met Location" && field != "Met Level";
}

inline void run(SourceGame game, Family family) {
    const std::string id = sourceId(game);
    auto source = fixture(family, game);
    const auto immutable = source;
    std::string error;
    auto editor = StagedPokemonEditor::create(source, game, error);
    require(editor && error.empty(), id + ": editor: " + error);
    if (!editor) return;
    const auto entryOpt = editor->boxedPokemon(0, 0, error);
    const auto neighbourOpt = editor->boxedPokemon(0, 1, error);
    require(entryOpt && entryOpt->species == 25 && neighbourOpt && neighbourOpt->species == 25,
            id + ": fixture holds two Pikachu");
    if (!entryOpt || !neighbourOpt) return;
    const auto entry = *entryOpt;

    // 1. Edit draft: species (Pikachu -> Raichu, which keeps Thunder Shock and Growl), held item,
    //    a hand-picked move and one IV. Nothing reaches the staged save before Keep.
    Session s;
    s.begin(entry, Mode::Edit);
    require(s.setSpecies(26, id), id + ": species picker applies Raichu");
    s.working.heldItem = 14;            // Antidote (was Potion)
    s.working.moves[2] = 85;            // Thunderbolt, as the move picker applies it
    s.working.pp[2] = 15;
    s.working.ivs[0] = 31;              // HP IV (was 10)
    require(s.dirty(), id + ": the draft is dirty");
    require(editor->stagedBytes() == immutable && !editor->hasPendingChanges(),
            id + ": the draft is a preview: nothing staged before Keep");

    // 2. Keep: one Pokemon staged; Review lists only its intended rows.
    require(s.keep(*editor, 0, 0, error), id + ": Keep: " + error);
    const auto& changes = editor->pendingChanges();
    bool species = false, held = false, move = false, iv = false;
    for (const auto& c : changes) {
        require(c.box == 0 && c.slot == 0 && !c.party, id + ": Review row outside Box 1 / Slot 1: " + c.label);
        require(intendedRow(c.label), id + ": unintended Review row " + c.label + " (" + c.beforeValue + " -> " +
                                          c.afterValue + ")");
        species = species || c.label == "Box 1 / Slot 1 Species";
        held = held || c.label == "Box 1 / Slot 1 Held Item";
        move = move || c.label == "Box 1 / Slot 1 Move 3";
        iv = iv || c.label == "Box 1 / Slot 1 HP IV";
    }
    require(species && held && move && iv, id + ": Review lists Species, Held Item, Move 3 and HP IV");

    // 3. Strict finalize -> reparse: the edit is there, its identity is intact, the neighbour is not touched.
    const auto out = editor->finalizedBytes(error);
    require(!out.empty() && error.empty(), id + ": finalize: " + error);
    auto reparsed = StagedPokemonEditor::create(out, game, error);
    require(reparsed && error.empty(), id + ": strict reparse: " + error);
    if (reparsed) {
        const auto after = reparsed->boxedPokemon(0, 0, error);
        require(after && after->species == 26 && after->heldItem == 14 && after->moves[2] == 85 &&
                    after->ivs[0] == 31, id + ": reparsed record carries the four edits");
        if (after) {
            require(after->pid == entry.pid && after->otName == entry.otName && after->tid == entry.tid &&
                        after->sid == entry.sid && after->nickname == entry.nickname && after->evs == entry.evs &&
                        after->ball == entry.ball && after->metLocation == entry.metLocation &&
                        after->metLevel == entry.metLevel && after->friendship == entry.friendship &&
                        after->pokerus == entry.pokerus && after->originGame == entry.originGame &&
                        after->language == entry.language && after->nature == entry.nature &&
                        after->gender == entry.gender && after->shiny == entry.shiny,
                    id + ": unrelated fields of the edited Pokemon are unchanged");
            require(std::equal(entry.ivs.begin() + 1, entry.ivs.end(), after->ivs.begin() + 1),
                    id + ": only the edited IV changed");
        }
        auto pristine = StagedPokemonEditor::create(immutable, game, error);
        require(pristine != nullptr, id + ": source editor: " + error);
        for (std::size_t box = 0; pristine && box < 14; ++box)
            for (std::size_t slot = 0; slot < 30; ++slot) {
                if (box == 0 && slot == 0) continue;
                std::string e1, e2, e3;
                const auto a = editor->boxedPokemon(box, slot, e1);
                const auto b = reparsed->boxedPokemon(box, slot, e2);
                const auto o = pristine->boxedPokemon(box, slot, e3);
                const bool same = a.has_value() == o.has_value() && b.has_value() == o.has_value() &&
                                  (!o || (a->encryptedBytes == o->encryptedBytes && b->encryptedBytes == o->encryptedBytes));
                if (!same) require(false, id + ": Box " + std::to_string(box + 1) + " / Slot " +
                                              std::to_string(slot + 1) + " changed");
            }
    }
    require(source == immutable && editor->originalBytes() == immutable, id + ": the source bytes never change");

    // 4. Discard: the original Pokemon returns exactly.
    editor->discard();
    require(editor->stagedBytes() == immutable && !editor->hasPendingChanges(),
            id + ": Discard restores the staged save byte for byte");
    const auto restored = editor->boxedPokemon(0, 0, error);
    require(restored && restored->encryptedBytes == entry.encryptedBytes && restored->species == 25 &&
                restored->heldItem == 13 && restored->moves == entry.moves && restored->ivs == entry.ivs,
            id + ": Discard returns the original Pokemon exactly");
    require(source == immutable, id + ": source unchanged after Discard");
    std::cout << "  " << id << ": species / held item / move / IV edit -> Review -> Discard checked\n";
}

// Every Gen III game; returns the number of failed checks (0 = PASS).
inline int runAll() {
    run(SourceGame::RubyGBA, Family::RS);
    run(SourceGame::SapphireGBA, Family::RS);
    run(SourceGame::EmeraldGBA, Family::Emerald);
    run(SourceGame::FireRedGBA, Family::FRLG);
    run(SourceGame::LeafGreenGBA, Family::FRLG);
    if (failures) {
        std::cerr << failures << " Round 11 H24 check(s) failed\n";
        return failures;
    }
    std::cout << "Round 11 H24 Gen III edit -> Review -> Discard (R/S/E/FR/LG): PASS\n";
    return 0;
}

} // namespace r11_h24
