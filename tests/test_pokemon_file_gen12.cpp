// Feature A (owner Oct 28): standalone .pk1 / .pk2 files in the Gen I and Gen II staged editors.
// Import places a decoded file into the first empty box slot as a staged change only; export builds
// the PKHeX single-entry list of a box or Party Pokemon without touching anything. Synthetic saves
// only (tests/fixtures/gsc_pokemon_fixture.h, the R/B/Y editor fixture of test_rby_pokemon_editor.cpp,
// and the Japanese layouts of test_rby_gen1_adapter.cpp / test_gsc_gen2_adapter.cpp).
#include "fixtures/gsc_pokemon_fixture.h"
#include "Integration/Gen1/Gen1StagedPokemonEditor.h"
#include "Integration/Gen2/Gen2BattleStats.h"
#include "Integration/Gen2/Gen2HeldItems.h"
#include "PokemonFile/NativePokemonFile.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <span>
#include <string>
#include <tuple>
#include <vector>

namespace F = PokeBank::PokemonFile;
namespace G1 = PokeVault::Integration::Gen1;
using Bytes = std::vector<uint8_t>;

namespace {
bool has(const std::string& text, const char* part) { return text.find(part) != std::string::npos; }
F::Decoded decoded(const Bytes& b, const char* name) {
    std::string e;
    const auto d = F::decode(b, name, e);
    if (!d) std::cerr << "decode refused: " << e << "\n";
    assert(d);
    return *d;
}
}  // namespace

// ---------------------------------------------------------------------------------------------
// Generation I (R/B/Y international). Fixture: test_rby_pokemon_editor.cpp, plus one Party Pokemon.
namespace rby {
uint8_t sum(const Bytes& v, size_t start, size_t n) { unsigned s = 0; for (size_t i = start; i < start + n; ++i) s += v[i]; return (~s) & 255; }
size_t bank(size_t b) { return (b < 6 ? 0x4000 : 0x6000) + (b % 6) * 0x462; }
void checksums(Bytes& v) {
    v[0x3523] = sum(v, 0x2598, 0xF8B);
    if (v[0x284C] & 0x80)
        for (size_t b : {size_t(0x4000), size_t(0x6000)}) {
            v[b + 0x1A4C] = sum(v, b, 0x1A4C);
            for (size_t i = 0; i < 6; ++i) v[b + 0x1A4D + i] = sum(v, b + i * 0x462, 0x462);
        }
}
void be16(Bytes& v, size_t o, uint16_t x) { v[o] = uint8_t(x >> 8); v[o + 1] = uint8_t(x); }
constexpr size_t Party = 0x2F2C, PartyBody = Party + 8, PartyOT = PartyBody + 6 * 44, PartyNick = PartyOT + 6 * 11;
size_t otAt(size_t box, size_t s) { return box + 22 + 20 * 33 + s * 11; }
size_t nickAt(size_t box, size_t s) { return box + 22 + 20 * 44 + s * 11; }

Bytes fixture(G1::SourceGame game) {
    Bytes v(0x8000, 0);
    v[0x2598] = 0x91; v[0x2599] = 0x84; v[0x259A] = 0x83; v[0x259B] = 0x50;  // RED
    v[0x2605] = 0x12; v[0x2606] = 0x34;
    v[0x25CA] = v[0x27E7] = 0xFF;
    v[0x284C] = 0x82;                                                       // current box 3, banks initialized
    v[0x29C3] = game == G1::SourceGame::Yellow ? 0x54 : 0x99;
    for (size_t b = 0; b < 12; ++b) v[bank(b) + 1] = 0xFF;
    const size_t box = 0x30C0, body = box + 22;
    v[box] = 1; v[box + 1] = 0x54; v[box + 2] = 0xFF;                       // Pikachu, internal 0x54
    v[body] = 0x54; v[body + 1] = 0; v[body + 2] = 20; v[body + 3] = 5;
    v[body + 5] = v[body + 6] = 23; v[body + 7] = 190; v[body + 8] = 33;
    v[body + 12] = 0x12; v[body + 13] = 0x34; v[body + 16] = 125;
    v[body + 27] = 0x88; v[body + 28] = 0x88; v[body + 29] = 35;
    std::fill_n(v.begin() + box + 682, 11, 0x50); v[box + 682] = 0x91; v[box + 683] = 0x84; v[box + 684] = 0x83;
    std::fill_n(v.begin() + box + 902, 11, 0x50); v[box + 902] = 0x8F; v[box + 903] = 0x88; v[box + 904] = 0x8A; v[box + 905] = 0x80;
    std::copy_n(v.begin() + box, 0x462, v.begin() + bank(2));
    // Party: Bulbasaur L10 whose box level byte (0x03) is stale at 5, as after levelling in the Party.
    v[Party] = 1; v[Party + 1] = 0x99; v[Party + 2] = 0xFF;
    const size_t p = PartyBody;
    v[p] = 0x99; be16(v, p + 1, 25); v[p + 3] = 5; v[p + 5] = 0x16; v[p + 6] = 0x03; v[p + 7] = 45;
    v[p + 8] = 33; v[p + 9] = 45; v[p + 12] = 0x12; v[p + 13] = 0x34;
    const uint32_t exp = Pokemon::getExpForLevel(10, G1::StagedPokemonEditor::growthRate(1));
    v[p + 14] = uint8_t(exp >> 16); v[p + 15] = uint8_t(exp >> 8); v[p + 16] = uint8_t(exp);
    v[p + 27] = 0xA9; v[p + 28] = 0x87; v[p + 29] = 35; v[p + 30] = 40; v[p + 0x21] = 10;
    const auto stats = G1::StagedPokemonEditor::calculateBattleStats(1, 10, {10, 9, 8, 7}, {}).asArray();
    for (size_t i = 0; i < 5; ++i) be16(v, p + 0x22 + i * 2, stats[i]);
    std::fill_n(v.begin() + PartyOT, 11, 0x50); v[PartyOT] = 0x91; v[PartyOT + 1] = 0x84; v[PartyOT + 2] = 0x83;
    std::fill_n(v.begin() + PartyNick, 11, 0x50);
    for (size_t i = 0; i < 5; ++i) v[PartyNick + i] = std::array<uint8_t, 5>{0x81, 0x94, 0x8B, 0x81, 0x80}[i];  // BULBA
    checksums(v);
    return v;
}

// Japanese R/B/Y layout (PKHeX SAV1Offsets, as pinned in test_rby_gen1_adapter.cpp JPN).
Bytes japaneseFixture() {
    Bytes v(0x8000, 0);
    std::fill_n(v.begin() + 0x2598, 6, 0x50); v[0x2598] = 0x80; v[0x2599] = 0x81;
    v[0x2842] = 2;                                         // current box 3, banks not initialized
    v[0x2ED5] = 0; v[0x2ED6] = 0xFF;                       // empty Party
    v[0x302D] = 0; v[0x302E] = 0xFF;                       // empty current box
    for (size_t b = 0; b < 8; ++b) { const size_t o = (b < 4 ? 0x4000 : 0x6000) + (b % 4) * 0x566; v[o + 1] = 0xFF; }
    v[0x3594] = sum(v, 0x2598, 0x0FFC);
    return v;
}

Bytes staged(const G1::StagedPokemonEditor& e) { return {e.stagedBytes().begin(), e.stagedBytes().end()}; }

struct Snapshot {
    Bytes bytes; size_t changes;
    explicit Snapshot(const G1::StagedPokemonEditor& e) : bytes(staged(e)), changes(e.pendingChanges().size()) {}
    bool same(const G1::StagedPokemonEditor& e) const { return staged(e) == bytes && e.pendingChanges().size() == changes; }
};

void refuse(G1::StagedPokemonEditor& e, size_t box, size_t slot, const F::Decoded& file, const char* reason) {
    const Snapshot before(e);
    std::string error;
    assert(!e.stageImport(box, slot, file, error));
    if (!has(error, reason)) std::cerr << "unexpected refusal: " << error << "\n";
    assert(has(error, reason));
    assert(before.same(e));
}

void run(G1::SourceGame game) {
    const auto raw = fixture(game);
    const auto source = G1::parse(raw, game);
    assert(source);
    std::string error;
    auto e = G1::StagedPokemonEditor::create(*source.save, error);
    assert(e && source.save->party().size() == 1);

    // Export a box Pokemon: 33 stored bytes, then the party fields pokered _MoveMon computes.
    const auto boxRecord = *e->boxedPokemon(2, 0, error);
    const auto file = e->exportBoxPokemonFile(2, 0, error);
    assert(file && file->size() == 69 && !e->hasPendingChanges() && staged(*e) == raw);
    assert((*file)[0] == 1 && (*file)[1] == 0x54 && (*file)[2] == 0xFF);
    assert(std::equal(boxRecord.rawBody.begin(), boxRecord.rawBody.begin() + 33, file->begin() + 3));
    assert((*file)[3 + 0x21] == 5);
    const auto stats = G1::StagedPokemonEditor::calculateBattleStats(25, 5, {8, 8, 8, 8}, {}).asArray();
    for (size_t i = 0; i < 5; ++i) assert((((*file)[3 + 0x22 + i * 2] << 8) | (*file)[3 + 0x23 + i * 2]) == stats[i]);
    assert(std::equal(file->begin() + 47, file->begin() + 58, raw.begin() + otAt(0x30C0, 0)));
    assert(std::equal(file->begin() + 58, file->end(), raw.begin() + nickAt(0x30C0, 0)));
    const auto pk1 = decoded(*file, "pika.pk1");
    assert(pk1.generation == 1 && !pk1.japanese && pk1.plain == *file);

    // Import into an empty non-current box and into the current box.
    assert(e->stageImport(0, 0, pk1, error));
    assert(e->stageImport(2, 1, pk1, error));
    for (const auto& [b, s] : {std::pair<size_t, size_t>{0, 0}, {2, 1}}) {
        const auto placed = e->boxedPokemon(b, s, error);
        assert(placed && std::equal(placed->rawBody.begin(), placed->rawBody.begin() + 33, boxRecord.rawBody.begin()));
        assert(placed->nickname == "PIKA" && placed->originalTrainer == "RED");
    }
    // Review lists it like a Create.
    const auto& changes = e->pendingChanges();
    const auto row = std::find_if(changes.begin(), changes.end(), [](const auto& c) { return c.key == "0/0/presence"; });
    assert(row != changes.end() && row->beforeValue == "Empty" && has(row->afterValue, "Pikachu") && has(row->label, "Box 1 Slot 1"));
    assert(std::any_of(changes.begin(), changes.end(), [](const auto& c) { return c.key == "2/1/presence"; }));
    // Re-export of the imported Pokemon is byte-identical to the file.
    assert(e->exportBoxPokemonFile(0, 0, error) == *file);

    // Finalize: strict reparse holds the imported bytes in the slot body and string arrays.
    const auto out = e->finalizedBytes(error);
    assert(!out.empty());
    const auto reparsed = G1::parse(out, game);
    assert(reparsed);
    for (const auto& [b, s, at] : {std::tuple<size_t, size_t, size_t>{0, 0, bank(0)}, {2, 1, 0x30C0}}) {
        const auto& p = reparsed.save->boxes()[b].slots[s];
        assert(p && std::equal(p->rawBody.begin(), p->rawBody.begin() + 33, file->begin() + 3));
        assert(std::equal(file->begin() + 47, file->begin() + 58, out.begin() + otAt(at, s)));
        assert(std::equal(file->begin() + 58, file->end(), out.begin() + nickAt(at, s)));
    }
    assert(std::equal(out.begin() + 0x30C0, out.begin() + 0x30C0 + 0x462, out.begin() + bank(2)));
    // The source never changes.
    assert(std::equal(raw.begin(), raw.end(), e->originalBytes().begin()));
    assert(std::equal(raw.begin(), raw.end(), source.save->sourceBytes().begin()));
    assert(pk1.plain == *file);

    // Discard removes the staged import.
    e->discard();
    assert(!e->hasPendingChanges() && staged(*e) == raw && !e->boxedPokemon(0, 0, error));

    // Party export copies the stored 44-byte body; importing it into the PC applies pokered's
    // PARTY_TO_BOX level copy (body 0x21 -> 0x03).
    const auto partyFile = e->exportPartyPokemonFile(0, error);
    assert(partyFile && partyFile->size() == 69 && staged(*e) == raw);
    assert(std::equal(partyFile->begin() + 3, partyFile->begin() + 47, raw.begin() + PartyBody));
    assert(std::equal(partyFile->begin() + 47, partyFile->begin() + 58, raw.begin() + PartyOT));
    assert(std::equal(partyFile->begin() + 58, partyFile->end(), raw.begin() + PartyNick));
    assert(!e->exportPartyPokemonFile(1, error) && !error.empty());
    assert(e->stageImport(0, 0, decoded(*partyFile, "bulba.pk1"), error));
    const auto deposited = *e->boxedPokemon(0, 0, error);
    assert(deposited.species == 1 && deposited.level == 10 && deposited.rawBody[3] == 10 && deposited.nickname == "BULBA");
    for (size_t i = 0; i < 33; ++i) if (i != 3) assert(deposited.rawBody[i] == (*partyFile)[3 + i]);
    e->discard();

    // aa4b72e: a list whose party block is zero is a valid box record; the box level byte stays.
    Bytes zeroParty = *file;
    std::fill(zeroParty.begin() + 3 + 33, zeroParty.begin() + 47, 0);
    assert(e->stageImport(0, 0, decoded(zeroParty, "zero.pk1"), error));
    assert(e->boxedPokemon(0, 0, error)->level == 5);
    e->discard();

    // Refusals leave the staged image and Review untouched.
    Bytes pk2(73, 0x50); pk2[0] = 1; pk2[1] = 25; pk2[2] = 0xFF; std::fill_n(pk2.begin() + 3, 48, 0); pk2[3] = 25;
    refuse(*e, 0, 0, decoded(pk2, "x.pk2"), "Only a .pk1");
    Bytes jp(59, 0x50); jp[0] = 1; jp[1] = 0x54; jp[2] = 0xFF; std::fill_n(jp.begin() + 3, 44, 0); jp[3] = 0x54;
    std::copy_n(file->begin() + 3, 33, jp.begin() + 3);
    refuse(*e, 0, 0, decoded(jp, "jp.pk1"), "Japanese");
    refuse(*e, 2, 0, pk1, "occupied");
    refuse(*e, 0, 1, pk1, "first empty slot");
    refuse(*e, 12, 0, pk1, "out of range");
    refuse(*e, 0, 20, pk1, "out of range");
    {
        uint8_t missingNo = 0;
        for (unsigned i = 1; i <= 190 && !missingNo; ++i) if (G1::gen1InternalToNational(uint8_t(i)) == 0) missingNo = uint8_t(i);
        assert(missingNo);
        Bytes bad = *file; bad[1] = bad[3] = missingNo;
        refuse(*e, 0, 0, decoded(bad, "m.pk1"), "not a valid Generation I box Pokemon");
    }
    {
        Bytes bad = zeroParty; bad[3 + 16] = 250;                          // EXP 250 is level 6, not 5
        refuse(*e, 0, 0, decoded(bad, "lv.pk1"), "level and experience");
        bad = *file; bad[3 + 0x21] = 9;                                    // party level disagrees with EXP
        refuse(*e, 0, 0, decoded(bad, "lv.pk1"), "level and experience");
        bad = *file; bad[3 + 8] = 166;                                     // move id outside Gen I
        refuse(*e, 0, 0, decoded(bad, "mv.pk1"), "Move/PP");
        bad = *file; bad[3 + 29] = 36;                                     // Tackle PP above 35
        refuse(*e, 0, 0, decoded(bad, "pp.pk1"), "Move/PP");
        F::Decoded broken = pk1; broken.plain[1] = 0x99;                   // hand-made, list/body disagree
        refuse(*e, 0, 0, broken, "single-Pokemon list");
    }
    for (size_t i = 0; i < 20; ++i) { auto slot = e->appendSlot(5, error); assert(slot); assert(e->stageAdd(5, *slot, {}, error)); }
    refuse(*e, 5, 19, pk1, "full");
    e->discard();
    // A carried (packed-move) Pokemon must be placed first.
    assert(e->beginPackedMove(2, 0, error));
    refuse(*e, 0, 0, pk1, "carried");
    assert(!e->exportBoxPokemonFile(2, 0, error) && has(error, "carried"));
    assert(e->cancelPackedMove(error));
    assert(staged(*e) == raw && !e->hasPendingChanges());
    assert(!e->exportBoxPokemonFile(0, 0, error) && !error.empty());  // empty slot
}

void japaneseSave() {
    const auto raw = japaneseFixture();
    const auto parsed = G1::parse(raw, G1::SourceGame::Red);
    assert(parsed && parsed.save->metadata().region == G1::RegionLayout::Japanese);
    std::string error;
    // Japanese Gen I saves stay read-only: no staged editor, so nothing can be imported.
    assert(!G1::StagedPokemonEditor::create(*parsed.save, error) && !error.empty());
}
}  // namespace rby

// ---------------------------------------------------------------------------------------------
// Generation II (G/S/C international). Fixture: tests/fixtures/gsc_pokemon_fixture.h.
namespace gsc {
constexpr size_t kOT = 3 + 48, kNick = kOT + 11;
size_t partyNames(const L& l) { return l.party + 8 + 6 * 48; }

struct Snapshot {
    Bytes bytes; size_t changes;
    explicit Snapshot(const StagedEditor& e) : bytes(e.stagedBytes().begin(), e.stagedBytes().end()), changes(e.pendingChanges().size()) {}
    bool same(const StagedEditor& e) const {
        return std::equal(bytes.begin(), bytes.end(), e.stagedBytes().begin()) && e.pendingChanges().size() == changes;
    }
};

void refuse(StagedEditor& e, size_t box, size_t slot, const F::Decoded& file, const char* reason) {
    const Snapshot before(e);
    std::string error;
    assert(!e.stageImportBoxPokemon(box, slot, file, error));
    if (!has(error, reason)) std::cerr << "unexpected refusal: " << error << "\n";
    assert(has(error, reason));
    assert(before.same(e));
}

void run(const L& l, SourceGame game) {
    auto raw = fixture(l);
    fullBox(raw, 3);
    checksum(raw, l);
    const auto source = parse(raw, game);
    assert(source);
    std::string error;
    auto e = StagedEditor::create(*source.save, error);
    assert(e);
    const auto isRaw = [&] { return std::equal(raw.begin(), raw.end(), e->stagedBytes().begin()); };

    // Export a box Pokemon: 32 stored bytes, then pokecrystal's PC_WITHDRAW party fields.
    const auto boxRecord = *e->boxedPokemon(0, 0, error);
    const auto file = e->exportBoxPokemonFile(0, 0, error);
    assert(file && file->size() == 73 && isRaw() && !e->hasPendingChanges());
    assert((*file)[0] == 1 && (*file)[1] == 25 && (*file)[2] == 0xFF);
    assert(std::equal(boxRecord.rawBody.begin(), boxRecord.rawBody.begin() + 32, file->begin() + 3));
    const auto stats = calculateBattleStats(25, 15, {7, 10, 10, 10}, {1, 2, 3, 4, 5}).asArray();
    const auto rd = [&](const Bytes& b, size_t o) { return uint16_t((b[3 + o] << 8) | b[3 + o + 1]); };
    assert((*file)[3 + 31] == 15 && (*file)[3 + 32] == 0 && (*file)[3 + 33] == 0 && rd(*file, 34) == stats[0]);
    for (size_t i = 0; i < 6; ++i) assert(rd(*file, 36 + i * 2) == stats[i]);
    assert(std::equal(file->begin() + kOT, file->begin() + kNick, raw.begin() + boxOT(0)));
    assert(std::equal(file->begin() + kNick, file->end(), raw.begin() + boxNick(0)));
    const auto pk2 = decoded(*file, "pika.pk2");
    assert(pk2.generation == 2 && !pk2.japanese && pk2.plain == *file);

    // Import into an empty box and into the current box (0); Review shows each import.
    size_t placedSlot = 99;
    assert(e->stageImportBoxPokemon(2, 0, pk2, error));
    assert(e->stageImportBoxPokemon(0, 1, pk2, error));
    for (const auto& [b, s] : {std::pair<size_t, size_t>{2, 0}, {0, 1}}) {
        const auto placed = e->boxedPokemon(b, s, error);
        assert(placed && std::equal(placed->rawBody.begin(), placed->rawBody.begin() + 32, boxRecord.rawBody.begin()));
        assert(placed->nickname == "PIKA" && placed->originalTrainer == "OT" && !placed->isEgg);
        const auto& changes = e->pendingChanges();
        const std::string key = "box:" + std::to_string(b) + ":" + std::to_string(s) + ":import";
        const auto row = std::find_if(changes.begin(), changes.end(), [&](const auto& c) { return c.key == key; });
        assert(row != changes.end() && row->beforeValue == "Empty" && has(row->afterValue, "Imported PIKA Lv. 15"));
    }
    assert(e->exportBoxPokemonFile(2, 0, error) == *file);
    // A clone after an import still works on the same staged image.
    assert(e->stageCloneBoxPokemon(2, 0, 2, placedSlot, error) && placedSlot == 1);

    const auto out = e->finalizedBytes(error);
    assert(!out.empty());
    const auto reparsed = parse(out, game);
    assert(reparsed);
    for (const auto& [b, s] : {std::pair<size_t, size_t>{2, 0}, {0, 1}}) {
        const auto& p = reparsed.save->boxes()[b].slots[s];
        assert(p && std::equal(p->rawBody.begin(), p->rawBody.begin() + 32, file->begin() + 3));
        assert(std::equal(file->begin() + kOT, file->begin() + kNick, out.begin() + boxOT(b) + s * 11));
        assert(std::equal(file->begin() + kNick, file->end(), out.begin() + boxNick(b) + s * 11));
    }
    assert(std::equal(raw.begin(), raw.end(), e->originalBytes().begin()));
    assert(std::equal(raw.begin(), raw.end(), source.save->sourceBytes().begin()));
    assert(pk2.plain == *file);

    e->discard();
    assert(isRaw() && !e->hasPendingChanges() && !e->boxedPokemon(2, 0, error));

    // Party export copies the stored 48-byte body; it imports into the PC unchanged.
    const auto partyFile = e->exportPartyPokemonFile(0, error);
    assert(partyFile && partyFile->size() == 73 && isRaw());
    assert(std::equal(partyFile->begin() + 3, partyFile->begin() + 51, raw.begin() + l.party + 8));
    assert(std::equal(partyFile->begin() + kOT, partyFile->begin() + kNick, raw.begin() + partyNames(l)));
    assert(std::equal(partyFile->begin() + kNick, partyFile->end(), raw.begin() + partyNames(l) + 66));
    assert(!e->exportPartyPokemonFile(1, error) && !error.empty());
    assert(e->stageImportBoxPokemon(2, 0, decoded(*partyFile, "party.pk2"), error));
    assert(std::equal(partyFile->begin() + 3, partyFile->begin() + 35, e->boxedPokemon(2, 0, error)->rawBody.begin()));
    e->discard();

    // Egg: list species 0xFD, body keeps the species; imported as an Egg exactly as stored.
    Bytes eggFile = *file;
    eggFile[1] = 0xFD;
    text(eggFile, kNick, 11, "EGG");
    eggFile[3 + 27] = 10;                                                   // friendship = hatch cycles
    const auto egg = decoded(eggFile, "egg.pk2");
    assert(e->stageImportBoxPokemon(2, 0, egg, error));
    const auto stagedEgg = *e->boxedPokemon(2, 0, error);
    assert(stagedEgg.isEgg && stagedEgg.species == 25 && stagedEgg.nickname == "EGG" && stagedEgg.friendship == 10);
    assert(std::equal(eggFile.begin() + 3, eggFile.begin() + 35, stagedEgg.rawBody.begin()));
    assert(has(e->pendingChanges().back().afterValue, "Imported Egg"));
    const auto eggOut = e->finalizedBytes(error);
    const auto eggParsed = parse(eggOut, game);
    assert(eggParsed && eggParsed.save->boxes()[2].slots[0]->isEgg && eggOut[boxStart(2) + 1] == 0xFD);
    const auto eggBack = e->exportBoxPokemonFile(2, 0, error);
    assert(eggBack && (*eggBack)[1] == 0xFD && rd(*eggBack, 34) == 0);          // PC_WITHDRAW: an Egg has 0 HP
    assert(std::equal(eggBack->begin(), eggBack->begin() + 37, eggFile.begin()));
    assert(std::equal(eggBack->begin() + 39, eggBack->end(), eggFile.begin() + 39));
    assert(decoded(*eggBack, "egg2.pk2").plain == *eggBack);
    e->discard();

    // Refusals leave the staged image and Review untouched.
    Bytes pk1(69, 0x50); pk1[0] = 1; pk1[1] = 0x54; pk1[2] = 0xFF; std::fill_n(pk1.begin() + 3, 44, 0); pk1[3] = 0x54;
    refuse(*e, 2, 0, decoded(pk1, "x.pk1"), "Only a .pk2");
    Bytes jp(63, 0x50); jp[0] = 1; jp[1] = 25; jp[2] = 0xFF; std::copy_n(file->begin() + 3, 48, jp.begin() + 3);
    refuse(*e, 2, 0, decoded(jp, "jp.pk2"), "Japanese");
    refuse(*e, 0, 0, pk2, "occupied");
    refuse(*e, 2, 1, pk2, "first empty slot");
    refuse(*e, 14, 0, pk2, "outside the validated save");
    refuse(*e, 2, 20, pk2, "outside the validated save");
    refuse(*e, 3, 19, pk2, "full");
    {
        F::Decoded bad = pk2; bad.plain[1] = bad.plain[3] = 252;            // hand-made: species past 251
        refuse(*e, 2, 0, bad, "not a valid Generation II box Pokemon");
        Bytes b = *file; b[3 + 9] = 0x13; b[3 + 10] = 0x88;                  // EXP 5000 is level 17, not 15
        refuse(*e, 2, 0, decoded(b, "lv.pk2"), "level and experience");
        b = *file; b[3 + 31] = 101;                                          // level above 100 (strict parser)
        refuse(*e, 2, 0, decoded(b, "lv.pk2"), "not a valid Generation II box Pokemon");
        uint8_t item = 0;
        for (unsigned i = 1; i < 256 && !item; ++i) if (!selectableHeldItem(uint8_t(i))) item = uint8_t(i);
        b = *file; b[3 + 1] = item;
        refuse(*e, 2, 0, decoded(b, "it.pk2"), "held item");
        b = *file; b[3 + 2] = 252;
        refuse(*e, 2, 0, decoded(b, "mv.pk2"), "move id");
        b = *file; b[3 + 23] = 36;                                           // Tackle PP above 35
        refuse(*e, 2, 0, decoded(b, "pp.pk2"), "move PP");
        b = *file; b[3 + 28] = 0x05;                                         // Pokerus strain 0 with days
        refuse(*e, 2, 0, decoded(b, "rus.pk2"), "Pokerus");
        F::Decoded broken = pk2; broken.plain[1] = 26;                       // hand-made, list/body disagree
        refuse(*e, 2, 0, broken, "single-Pokemon list");
    }
    assert(e->beginPackedMove(0, 0, error));
    refuse(*e, 2, 0, pk2, "carried");
    assert(!e->exportPartyPokemonFile(0, error) && has(error, "carried"));
    assert(e->cancelPackedMove(error));
    assert(isRaw() && !e->hasPendingChanges());
    assert(!e->exportBoxPokemonFile(2, 0, error) && !error.empty());           // empty slot
}

// Japanese Gold/Silver layout as pinned in test_gsc_gen2_adapter.cpp (JGS).
void japaneseSave() {
    constexpr size_t money = 0x23BC, names = 0x2708, party = 0x283E, current = 0x2D10, end = 0x2C8B, sum = 0x2D0D;
    constexpr size_t cap = 30, str = 6, boxes = 9, split = 6;
    const size_t stride = 1 + (cap + 1) + cap * 32 + 2 * cap * str + 2;
    Bytes d(0x8000, 0);
    d[0x2009] = 0x12; d[0x200A] = 0x34; std::fill_n(d.begin() + 0x200B, str, 0x50); d[0x200B] = 0x80;
    d[money] = 0x01; d[money + 1] = 0xE2; d[money + 2] = 0x40;
    for (size_t b = 0; b < boxes; ++b) {
        const size_t o = b < split ? 0x4000 + b * stride : 0x6000 + (b - split) * stride;
        d[o] = 0; d[o + 1] = 0xFF;
        std::fill_n(d.begin() + names + b * 9, 9, 0x50); d[names + b * 9] = uint8_t(0x80 + b);
    }
    d[current] = 0; d[current + 1] = 0xFF;
    d[party] = 0; d[party + 1] = 0xFF;
    uint16_t s = 0; for (size_t i = 0x2009; i <= end; ++i) s = uint16_t(s + d[i]);
    d[sum] = uint8_t(s); d[sum + 1] = uint8_t(s >> 8);
    const auto parsed = parse(d, SourceGame::Gold);
    assert(parsed && parsed.save->metadata().region == RegionLayout::Japanese);
    std::string error;
    // Japanese Gen II saves stay read-only: no staged editor, so nothing can be imported.
    assert(!StagedEditor::create(*parsed.save, error) && !error.empty());
}
}  // namespace gsc

int main() {
    for (const auto game : {G1::SourceGame::Red, G1::SourceGame::Blue, G1::SourceGame::Yellow}) rby::run(game);
    rby::japaneseSave();
    gsc::run(GS, SourceGame::Gold);
    gsc::run(GS, SourceGame::Silver);
    gsc::run(C, SourceGame::Crystal);
    gsc::japaneseSave();
    std::cout << "Standalone .pk1/.pk2 import and export in the Gen I/II staged editors: PASS\n";
    return 0;
}
