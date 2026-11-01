// Feature A (standalone Pokemon files), Gen III slice: .pk3 / .ek3 import into an EMPTY box slot of
// the staged workspace and non-mutating export, for Ruby, Sapphire, Emerald, FireRed and LeafGreen.
// Synthetic saves and records only. The save builder is the one pinned by
// tests/test_gen3_staged_pokemon_editor.cpp (makeLogical / writeSlot / fixture, copied because it
// lives in that test's anonymous namespace); the RTC footer is the 16-byte mGBA/VBA-M one from
// tests/test_gen3_trainer_edit.cpp; the Gen IV record for the wrong-generation refusal comes from
// tests/fuzz_synthetic_saves.h.
#include "Encryption/Encryption3FRLG.h"
#include "Integration/Gen3/Gen3SaveValidation.h"
#include "Integration/Gen3/Gen3StagedPokemonEditor.h"
#include "PokemonFile/NativePokemonFile.h"
#include "Pokemon/Pokemon3FRLG.h"
#include "fuzz_synthetic_saves.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <span>
#include <string>
#include <vector>

using namespace PokeVault::Integration::Gen3;
namespace F = PokeBank::PokemonFile;

namespace {

using Bytes = std::vector<uint8_t>;
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

// --- records -------------------------------------------------------------------------------

struct RecordSpec {
    uint16_t species = 25;
    uint8_t language = 2;
    std::u16string nickname = u"PIKACHU";
    std::u16string ot = u"RED";
    uint16_t item = 13;
    uint32_t pid = 0x12345678u;
    bool egg = false;
};

// PKHeX plaintext (decrypted, canonical order, checksum at 0x1C) of an 80-byte stored record, or
// a 100-byte party record whose tail holds computed stats, current HP, status and a mail id.
Bytes plainRecord(SourceGame game, const RecordSpec& spec, bool party = false,
                  const std::function<void(Pokemon::Pokemon3FRLG&)>& tweak = {}) {
    std::vector<uint8_t> blank(party ? 100 : 80, 0);
    Pokemon::Pokemon3FRLG p(std::span<const std::byte>(
        reinterpret_cast<const std::byte*>(blank.data()), blank.size()));
    p.setPID(spec.pid);
    p.setTID16(12345);
    p.setSID16(54321);
    p.setSpecies(spec.species);
    p.setLanguage(spec.language);
    p.setOTName(spec.ot);
    p.setNickname(spec.nickname);
    p.setHeldItem(spec.item);
    p.setFriendship(70);
    p.setOriginGame(origin(game));
    p.setBall(4);
    p.setMetLevel(20);
    p.setMetLocation(1);
    p.setLevel(20);
    p.setMove(0, 33); // Tackle
    p.setMovePPUps(0, 1);
    p.setMovePP(0, 40);
    for (int i = 0; i < 6; ++i) {
        p.setIV(i, static_cast<uint8_t>(10 + i));
        p.setEV(i, static_cast<uint8_t>(20 + i));
    }
    if (spec.egg) p.setEgg(true);
    if (party) {
        p.recalculateStats();
        p.setStatHPCurrent(static_cast<uint16_t>(p.statHPMax() - 3));
    }
    if (tweak) tweak(p);
    Bytes out(p.getData().size());
    std::memcpy(out.data(), p.getData().data(), out.size());
    if (party) { out[0x50] = 8; out[0x55] = 0xA5; } // status and mail id: kept as given
    assert(Encryption::checksum3FRLG(std::as_bytes(std::span<const uint8_t>(out).first(80))) ==
           static_cast<uint16_t>(out[0x1C] | out[0x1D] << 8));
    return out;
}

Bytes encrypted(const Bytes& plain) {
    std::byte* e = Encryption::encryptArray3FRLG(std::as_bytes(std::span<const uint8_t>(plain)));
    Bytes out(plain.size());
    std::memcpy(out.data(), e, out.size());
    delete[] e;
    return out;
}

Bytes decryptedOf(std::span<const uint8_t> raw) {
    std::byte* d = Encryption::decryptArray3FRLG(std::as_bytes(raw));
    Bytes out(raw.size());
    std::memcpy(out.data(), d, out.size());
    delete[] d;
    return out;
}

// Re-stamps the 0x1C checksum after a raw byte change (to build records only a validator refuses).
void restamp(Bytes& plain) {
    write16(plain, 0x1C, Encryption::checksum3FRLG(std::as_bytes(std::span<const uint8_t>(plain).first(80))));
}

F::Decoded decodeOk(const Bytes& bytes, const char* name) {
    std::string error;
    auto d = F::decode(bytes, name, error);
    if (!d) std::cerr << name << ": " << error << "\n";
    assert(d && error.empty() && d->generation == 3);
    return *d;
}

// --- saves (tests/test_gen3_staged_pokemon_editor.cpp makeLogical / writeSlot / fixture) --------

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
    const size_t flags = family == Family::FRLG ? 0xee0 : family == Family::Emerald ? 0x1270 : 0x1220;
    const unsigned firstBadge = family == Family::FRLG ? 0x820 : family == Family::Emerald ? 0x867 : 0x807;
    for (unsigned i = 0; i < 8; ++i)
        if ((0xa5u >> i) & 1u) {
            const auto position = flags + (firstBadge + i) / 8;
            logical[1 + position / Detail::kSectorDataSize][position % Detail::kSectorDataSize] |=
                static_cast<uint8_t>(1u << ((firstBadge + i) % 8));
        }
    // Box 1 / Slot 1 holds a Pokemon (the occupied-slot refusal and the export source).
    const auto mon = encrypted(plainRecord(game, {}));
    for (std::size_t i = 0; i < mon.size(); ++i) {
        const std::size_t position = 4 + i;
        logical[5 + position / Detail::kSectorDataSize][position % Detail::kSectorDataSize] = mon[i];
    }
    return logical;
}

void writeSlot(Bytes& save, uint8_t slot, uint32_t counter, uint8_t rotation, Family family, SourceGame game) {
    auto logical = makeLogical(family, game);
    for (std::size_t physical = 0; physical < Detail::kSectorCount; ++physical) {
        const uint16_t id = static_cast<uint16_t>((physical + rotation) % Detail::kSectorCount);
        const std::size_t offset = Detail::kSlotBases[slot] + physical * Detail::kSectorSize;
        std::copy(logical[id].begin(), logical[id].end(), save.begin() + static_cast<std::ptrdiff_t>(offset));
        write16(save, offset + 0xFF4, id);
        write16(save, offset + 0xFF6, Detail::sectorChecksum(
            std::span<const uint8_t>(save).subspan(offset, Detail::kChunkLengths[id])));
        write32(save, offset + 0xFF8, Detail::kSectorSignature);
        write32(save, offset + 0xFFC, counter);
    }
}

// Slot 1 (counter 9, rotated) is the active slot; Party 1 holds a real 100-byte record.
Bytes fixture(Family family, SourceGame game) {
    Bytes save(Detail::kSaveSize, 0);
    writeSlot(save, 0, 7, 0, family, game);
    writeSlot(save, 1, 9, 5, family, game);
    const auto selected = Detail::validateSlot(save, 1);
    assert(selected.valid);
    const auto sector = selected.logicalSectorOffsets[1];
    const auto body = sector + Detail::partyDataOffset(game);
    save[sector + Detail::partyCountOffset(game)] = 1;
    const auto party = encrypted(plainRecord(game, {.species = 6, .nickname = u"CHARIZARD", .pid = 0x0BADF00Du}, true));
    std::copy(party.begin(), party.end(), save.begin() + static_cast<std::ptrdiff_t>(body));
    write16(save, sector + 0xFF6, Detail::sectorChecksum(
        std::span<const uint8_t>(save).subspan(sector, Detail::kChunkLengths[1])));
    return save;
}

const std::array<uint8_t, 16> kRtcFooter{{0x52, 0x54, 0x43, 0x01, 0x02, 0x03, 0x04, 0x05,
                                          0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D}};

bool hasRow(const StagedPokemonEditor& e, const std::string& label, const std::string& before,
            const std::string& after) {
    for (const auto& row : e.pendingChanges())
        if (row.label == label && row.beforeValue == before && row.afterValue == after) return true;
    return false;
}

// Strict reparse of a finalized image: the parser accepts it, a fresh editor opens it, and the
// slot holds exactly `raw` (encrypted) whose decrypted form is `plain80`.
void assertFinalized(const Bytes& finalized, SourceGame game, std::size_t box, std::size_t slot,
                     const Bytes& plain80) {
    assert(parse(finalized, game));
    std::string error;
    auto reopened = StagedPokemonEditor::create(finalized, game, error);
    assert(reopened && error.empty());
    const auto record = reopened->boxedPokemon(box, slot, error);
    assert(record && error.empty());
    assert(decryptedOf(record->encryptedBytes) == plain80);
    const auto exported = reopened->exportBoxPokemonFile(box, slot, error);
    assert(exported && *exported == plain80);
}

// --- per-game coverage ---------------------------------------------------------------------

void successPaths(SourceGame game, Family family) {
    const Bytes source = fixture(family, game);
    const Bytes immutable = source;
    std::string error;

    // Plain stored .pk3: placed byte for byte, shown in Review, finalized, strictly reparsed.
    {
        auto editor = StagedPokemonEditor::create(source, game, error);
        assert(editor && !editor->hasPendingChanges());
        const Bytes plain = plainRecord(game, {.species = 150, .nickname = u"MEWTWO", .item = 0, .pid = 0x8642ACE1u});
        const Bytes fileBytes = plain;
        const auto file = decodeOk(fileBytes, "mewtwo.pk3");
        assert(!file.wasEncrypted && !file.party);
        const auto fileCopy = file.plain;
        assert(editor->stageImportBoxPokemon(1, 3, file, error) && error.empty());
        assert(file.plain == fileCopy && fileBytes == plain);   // the input is never touched
        const auto record = editor->boxedPokemon(1, 3, error);
        assert(record && record->species == 150 && record->nickname == "MEWTWO" && record->pid == 0x8642ACE1u);
        assert(decryptedOf(record->encryptedBytes) == plain);
        assert(record->encryptedBytes.size() == 80 &&
               std::equal(record->encryptedBytes.begin(), record->encryptedBytes.end(), encrypted(plain).begin()));
        assert(hasRow(*editor, "Box 2 / Slot 4 / Import Pokémon", "Empty", "Mewtwo Lv 20"));
        assert(hasRow(*editor, "Box 2 / Slot 4 / Nickname", "", "MEWTWO"));
        assert(hasRow(*editor, "Box 2 / Slot 4 / OT / TID / SID", "", "RED / 12345 / 54321"));
        assert(hasRow(*editor, "Box 2 / Slot 4 / Language", "", "English"));
        for (const auto& row : editor->pendingChanges()) assert(row.box == 1 && row.slot == 3 && !row.party);
        assert(editor->originalBytes() == immutable && source == immutable);
        const auto finalized = editor->finalizedBytes(error);
        assert(!finalized.empty() && error.empty() && finalized.size() == source.size());
        assert(finalized != source);
        assertFinalized(finalized, game, 1, 3, plain);
        // Box 1 / Slot 1 and Party 1 are untouched.
        auto reopened = StagedPokemonEditor::create(finalized, game, error);
        assert(reopened->boxedPokemon(0, 0, error)->encryptedBytes ==
               StagedPokemonEditor::create(source, game, error)->boxedPokemon(0, 0, error)->encryptedBytes);
        assert(source == immutable);
    }

    // Encrypted .ek3: the box slot holds the file's own bytes.
    {
        auto editor = StagedPokemonEditor::create(source, game, error);
        const Bytes plain = plainRecord(game, {.species = 384, .nickname = u"RAYQUAZA", .item = 0, .pid = 0x11223344u});
        const Bytes ek3 = encrypted(plain);
        const auto file = decodeOk(ek3, "rayquaza.ek3");
        assert(file.wasEncrypted && file.plain == plain);
        assert(editor->stageImportBoxPokemon(13, 29, file, error) && error.empty());
        const auto record = editor->boxedPokemon(13, 29, error);
        assert(record && std::equal(ek3.begin(), ek3.end(), record->encryptedBytes.begin()));
        assert(hasRow(*editor, "Box 14 / Slot 30 / Import Pokémon", "Empty", "Rayquaza Lv 20"));
        assertFinalized(editor->finalizedBytes(error), game, 13, 29, plain);
    }

    // Party-sized files (.pk3 plain and .ek3 encrypted): the slot takes the first 80 bytes.
    for (const char* name : {"party.pk3", "party.ek3"}) {
        auto editor = StagedPokemonEditor::create(source, game, error);
        const Bytes plain = plainRecord(game, {.species = 254, .nickname = u"SCEPTILE", .item = 0, .pid = 0x55667788u}, true);
        const bool enc = std::string(name).ends_with(".ek3");
        const auto file = decodeOk(enc ? encrypted(plain) : plain, name);
        assert(file.party && file.wasEncrypted == enc && file.plain == plain);
        assert(editor->stageImportBoxPokemon(2, 0, file, error) && error.empty());
        const Bytes stored(plain.begin(), plain.begin() + 80);
        assert(decryptedOf(editor->boxedPokemon(2, 0, error)->encryptedBytes) == stored);
        assert(hasRow(*editor, "Box 3 / Slot 1 / Source", "",
                      std::string(enc ? "Encrypted" : "Plain") + " party Generation III file, kept byte for byte"));
        assertFinalized(editor->finalizedBytes(error), game, 2, 0, stored);
    }

    // Japanese record in every game (Gen III storage is language-independent: pret pokeemerald
    // struct BoxPokemon carries its own `language`, and only name display differs). Same rule as
    // Clone, which copies any language. An Egg is accepted too (Clone copies Eggs).
    {
        auto editor = StagedPokemonEditor::create(source, game, error);
        const Bytes jp = plainRecord(game, {.language = 1, .nickname = u"ピカチュウ", .ot = u"サトシ", .item = 0, .pid = 0x01020304u});
        assert(jp[0x12] == 1);
        assert(editor->stageImportBoxPokemon(3, 5, decodeOk(jp, "jp.pk3"), error) && error.empty());
        assert(hasRow(*editor, "Box 4 / Slot 6 / Language", "", "Japanese"));
        const Bytes egg = plainRecord(game, {.species = 258, .language = 1, .nickname = u"タマゴ", .item = 0, .pid = 0x0A0B0C0Du, .egg = true});
        assert(editor->stageImportBoxPokemon(3, 6, decodeOk(egg, "egg.pk3"), error) && error.empty());
        const auto finalized = editor->finalizedBytes(error);
        assertFinalized(finalized, game, 3, 5, jp);
        assertFinalized(finalized, game, 3, 6, egg);
    }

    // Discard undoes an import entirely.
    {
        auto editor = StagedPokemonEditor::create(source, game, error);
        assert(editor->stageImportBoxPokemon(1, 3, decodeOk(plainRecord(game, {}), "a.pk3"), error));
        assert(editor->hasPendingChanges() && editor->stagedBytes() != source);
        editor->discard();
        assert(!editor->hasPendingChanges() && editor->stagedBytes() == source);
        assert(!editor->boxedPokemon(1, 3, error) && error.empty());
        assert(editor->finalizedBytes(error) == source);
    }

    // RTC-footered save: footer carried verbatim, image strictly reparses.
    {
        Bytes rtc = source;
        rtc.insert(rtc.end(), kRtcFooter.begin(), kRtcFooter.end());
        const Bytes rtcImmutable = rtc;
        auto editor = StagedPokemonEditor::create(rtc, game, error);
        assert(editor && error.empty());
        const Bytes plain = plainRecord(game, {.species = 1, .nickname = u"BULBASAUR", .pid = 0x31415926u});
        assert(editor->stageImportBoxPokemon(0, 1, decodeOk(encrypted(plain), "b.ek3"), error) && error.empty());
        const auto finalized = editor->finalizedBytes(error);
        assert(finalized.size() == rtc.size());
        assert(std::equal(kRtcFooter.begin(), kRtcFooter.end(), finalized.end() - 16));
        assertFinalized(finalized, game, 0, 1, plain);
        assert(rtc == rtcImmutable && editor->originalBytes() == rtcImmutable);
    }
}

void exportRoundTrip(SourceGame game, Family family) {
    const Bytes source = fixture(family, game);
    std::string error;
    auto editor = StagedPokemonEditor::create(source, game, error);
    const Bytes staged = editor->stagedBytes();

    // Box: export -> decode -> import into an empty slot -> same record, same stored bytes.
    const auto box = editor->exportBoxPokemonFile(0, 0, error);
    assert(box && error.empty() && box->size() == 80);
    assert(*box == plainRecord(game, {}));
    assert(editor->stagedBytes() == staged && !editor->hasPendingChanges()); // export is read-only
    const auto boxFile = decodeOk(*box, "export.pk3");
    assert(!boxFile.wasEncrypted && !boxFile.party && boxFile.plain == *box);
    assert(editor->stageImportBoxPokemon(0, 2, boxFile, error) && error.empty());
    assert(editor->boxedPokemon(0, 2, error)->encryptedBytes == editor->boxedPokemon(0, 0, error)->encryptedBytes);

    // Party: 100 bytes, the party-only tail exactly as stored (status 8, mail id 0xA5).
    const auto party = editor->exportPartyPokemonFile(0, error);
    assert(party && error.empty() && party->size() == 100);
    const Bytes expectedParty = plainRecord(game, {.species = 6, .nickname = u"CHARIZARD", .pid = 0x0BADF00Du}, true);
    assert(*party == expectedParty && (*party)[0x50] == 8 && (*party)[0x55] == 0xA5);
    const auto partyFile = decodeOk(*party, "party.pk3");
    assert(partyFile.party && partyFile.plain == *party);
    assert(editor->stageImportBoxPokemon(0, 4, partyFile, error) && error.empty());
    const auto roundTrip = editor->exportBoxPokemonFile(0, 4, error);
    assert(roundTrip && std::equal(roundTrip->begin(), roundTrip->end(), party->begin()));

    // Export reads the open workspace: the staged import is exported, the original never changes.
    assert(editor->originalBytes() == source);

    // Export refusals.
    auto fresh = StagedPokemonEditor::create(source, game, error);
    assert(!fresh->exportBoxPokemonFile(0, 1, error) && error.find("empty") != std::string::npos);
    assert(!fresh->exportBoxPokemonFile(14, 0, error) && error.find("outside") != std::string::npos);
    assert(!fresh->exportBoxPokemonFile(0, 30, error) && !error.empty());
    assert(!fresh->exportPartyPokemonFile(1, error) && !error.empty());
    assert(!fresh->exportPartyPokemonFile(6, error) && !error.empty());
    assert(fresh->stagedBytes() == source);
}

void refusals(SourceGame game, Family family) {
    const Bytes source = fixture(family, game);
    std::string error;
    auto editor = StagedPokemonEditor::create(source, game, error);

    const auto refused = [&](const F::Decoded& file, std::size_t box, std::size_t slot, const char* needle) {
        error.clear();
        const bool ok = editor->stageImportBoxPokemon(box, slot, file, error);
        if (ok || error.find(needle) == std::string::npos)
            std::cerr << "expected refusal containing '" << needle << "', got '" << error << "'\n";
        assert(!ok && error.find(needle) != std::string::npos);
        assert(editor->stagedBytes() == source && !editor->hasPendingChanges());
    };
    const auto manual = [](Bytes plain, bool party = false) {
        F::Decoded d;
        d.format = F::Format::PK3;
        d.generation = 3;
        d.party = party;
        d.plain = std::move(plain);
        return d;
    };
    const auto withRaw = [&](const std::function<void(Bytes&)>& change) {
        Bytes plain = plainRecord(game, {});
        change(plain);
        restamp(plain);
        return plain;
    };
    const Bytes good = plainRecord(game, {});

    // Slot placement.
    refused(decodeOk(good, "a.pk3"), 0, 0, "already holds a Pokemon");
    refused(decodeOk(good, "a.pk3"), 14, 0, "outside the native 14 x 30");
    refused(decodeOk(good, "a.pk3"), 0, 30, "outside the native 14 x 30");

    // A slot holding a record with an unused internal species index (252-276) reads as "no
    // Pokemon" but is not empty: Import refuses to write over it.
    {
        Bytes odd = source;
        const auto selected = Detail::validateSlot(odd, 1);
        const auto sector = selected.logicalSectorOffsets[5];
        const auto glitch = encrypted(withRaw([](Bytes& p) { write16(p, 0x20, 260); }));
        std::copy(glitch.begin(), glitch.end(), odd.begin() + static_cast<std::ptrdiff_t>(sector + 4 + 80 * 7));
        write16(odd, sector + 0xFF6, Detail::sectorChecksum(
            std::span<const uint8_t>(odd).subspan(sector, Detail::kChunkLengths[5])));
        auto oddEditor = StagedPokemonEditor::create(odd, game, error);
        assert(oddEditor);
        assert(!oddEditor->boxedPokemon(0, 7, error) && error.empty());
        // Round 11 H32: the shared Clone carry reads this to show the slot as protected before A.
        assert(oddEditor->holdsUnreadableRecord(0, 7) && !oddEditor->holdsUnreadableRecord(0, 8));
        assert(!oddEditor->stageImportBoxPokemon(0, 7, decodeOk(good, "a.pk3"), error) &&
               error.find("unreadable record") != std::string::npos);
        assert(oddEditor->stagedBytes() == odd && !oddEditor->hasPendingChanges());
        // The same guard protects Create, Clone and Move placement (pre-existing gap found here).
        BoxPokemonCreate create; create.species = 25; create.level = 5; create.language = 2;
        assert(!oddEditor->stageAddBoxPokemon(0, 7, create, error) &&
               error.find("unreadable record") != std::string::npos);
        assert(!oddEditor->stageCloneBoxPokemon(0, 0, 0, 7, error) &&
               error.find("unreadable record") != std::string::npos);
        assert(oddEditor->beginSparseMove(0, {0}, error));
        assert(!oddEditor->placeSparseMove(0, 7, error) && error.find("unreadable record") != std::string::npos);
        assert(oddEditor->cancelSparseMove(error));
        assert(oddEditor->stagedBytes() == odd && !oddEditor->hasPendingChanges());
    }

    // Wrong generation: a real Gen IV record decoded by the codec, and hand-built Gen I/II/V shapes.
    {
        const auto pk4 = qasynth::gen4::stored(25);
        const Bytes pk4Bytes(reinterpret_cast<const uint8_t*>(pk4.data()),
                             reinterpret_cast<const uint8_t*>(pk4.data()) + pk4.size());
        std::string decodeError;
        const auto file = F::decode(pk4Bytes, "pika.pk4", decodeError);
        assert(file && file->generation == 4);
        refused(*file, 1, 0, "Generation 4 Pokemon file");
        for (const auto& [format, gen] : {std::pair{F::Format::PK1, 1}, std::pair{F::Format::PK2, 2},
                                          std::pair{F::Format::PK5, 5}}) {
            F::Decoded d;
            d.format = format;
            d.generation = gen;
            d.plain.assign(80, 0);
            refused(d, 1, 0, ("Generation " + std::to_string(gen)).c_str());
        }
        F::Decoded mismatch = manual(good);
        mismatch.format = F::Format::PK4;   // generation 3 but not a PK3 record
        refused(mismatch, 1, 0, "takes only .pk3 / .ek3");
    }

    // Shape and integrity (a caller-built Decoded is not trusted).
    refused(manual(Bytes(good.begin(), good.end()), true), 1, 0, "80-byte stored or 100-byte party");
    refused(manual(Bytes(99, 0)), 1, 0, "80-byte stored or 100-byte party");
    {
        Bytes bad = good;
        bad[0x30] ^= 0x40;
        refused(manual(bad), 1, 0, "checksum");
        refused(manual(withRaw([](Bytes& p) { p[0x13] |= 1; })), 1, 0, "Bad Egg");
    }

    // Species: an unused internal index (252-276) and the out-of-table 412 decode, but the editor
    // has no Gen III species for them.
    refused(decodeOk(withRaw([](Bytes& p) { write16(p, 0x20, 260); }), "a.pk3"), 1, 0, "no valid Generation III species");
    refused(decodeOk(withRaw([](Bytes& p) { write16(p, 0x20, 412); }), "a.pk3"), 1, 0, "no valid Generation III species");

    // Held items: not a Gen III held item; never released as a held item (Safari Ball, Berry Juice).
    refused(decodeOk(withRaw([](Bytes& p) { write16(p, 0x22, 52); }), "a.pk3"), 1, 0, "is not a Generation III held item");
    refused(decodeOk(withRaw([](Bytes& p) { write16(p, 0x22, 400); }), "a.pk3"), 1, 0, "is not a Generation III held item");
    refused(decodeOk(withRaw([](Bytes& p) { write16(p, 0x22, 5); }), "a.pk3"), 1, 0, "never released as a held item");
    refused(decodeOk(withRaw([](Bytes& p) { write16(p, 0x22, 44); }), "a.pk3"), 1, 0, "never released as a held item");

    // Other Create field validators.
    refused(decodeOk(withRaw([](Bytes& p) { p[0x12] = 6; }), "a.pk3"), 1, 0, "not a Generation III language");
    refused(decodeOk(withRaw([](Bytes& p) { p[0x12] = 0; }), "a.pk3"), 1, 0, "not a Generation III language");
    refused(decodeOk(withRaw([](Bytes& p) { p[0x44] = 0x05; }), "a.pk3"), 1, 0, "Pokerus");
    refused(decodeOk(plainRecord(game, {}, false, [](auto& p) { p.setBall(0); }), "a.pk3"), 1, 0, "Ball");
    refused(decodeOk(plainRecord(game, {}, false, [](auto& p) { p.setBall(13); }), "a.pk3"), 1, 0, "Ball");
    refused(decodeOk(plainRecord(game, {}, false, [](auto& p) { p.setMetLevel(101); }), "a.pk3"), 1, 0, "met level");
    refused(decodeOk(withRaw([](Bytes& p) { for (int i = 0; i < 6; ++i) p[0x38 + i] = 100; }), "a.pk3"), 1, 0, "EV total");
    refused(decodeOk(withRaw([](Bytes& p) { write16(p, 0x2E, 355); }), "a.pk3"), 1, 0, "outside the native move table");
    refused(decodeOk(withRaw([](Bytes& p) { p[0x34] = 99; }), "a.pk3"), 1, 0, "exceeds the exact Gen III maximum");
    refused(decodeOk(withRaw([](Bytes& p) { p[0x35] = 5; }), "a.pk3"), 1, 0, "Empty Generation III move slots");

    // A carried sparse move blocks staging, as for every other operation.
    assert(editor->beginSparseMove(0, {0}, error));
    error.clear();
    assert(!editor->stageImportBoxPokemon(1, 0, decodeOk(good, "a.pk3"), error) &&
           error.find("carried") != std::string::npos);
    assert(editor->cancelSparseMove(error));
    assert(editor->stagedBytes() == source && !editor->hasPendingChanges());

    // After every refusal the editor still imports into the same slot.
    assert(editor->stageImportBoxPokemon(1, 0, decodeOk(good, "a.pk3"), error) && error.empty());
    assert(editor->originalBytes() == source);
}

void runGame(SourceGame game, Family family) {
    successPaths(game, family);
    exportRoundTrip(game, family);
    refusals(game, family);
}

} // namespace

int main() {
    runGame(SourceGame::RubyGBA, Family::RS);
    runGame(SourceGame::SapphireGBA, Family::RS);
    runGame(SourceGame::EmeraldGBA, Family::Emerald);
    runGame(SourceGame::FireRedGBA, Family::FRLG);
    runGame(SourceGame::LeafGreenGBA, Family::FRLG);
    std::cout << "Gen III standalone Pokemon file import/export (R/S/E/FR/LG): PASS\n";
    return 0;
}
