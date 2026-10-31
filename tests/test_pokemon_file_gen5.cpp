// Feature A (owner Oct 28): standalone .pk5 / .ek5 files for Black/White and Black 2/White 2.
// Import lands only in the staged workspace (Review / Keep / Discard / Export Copy unchanged), is
// independently re-verified by Review and by the export-candidate builder, and the export of a
// save's Pokemon is the PKHeX plaintext PK5 that the standalone decoder reads back. Synthetic saves
// only (fixture shape of tests/test_gen5_staged_save_candidate.cpp, PKHeX SaveBlockAccessor5BW/B2W2).
#include "Integration/Gen5/Gen5PokemonFile.h"
#include "Integration/Gen5/Gen5ReviewRows.h"
#include "Integration/Gen5/Gen5StagedSaveCandidate.h"
#include "PokemonFile/NativePokemonFile.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace G = PokeVault::Integration::Gen5;
namespace C = PokeVault::Integration::Gen5::Crypto;
namespace L = PokeVault::Integration::Gen5::LayoutInfo;
namespace F = PokeBank::PokemonFile;
using Bytes = std::vector<uint8_t>;
using Region = G::Gen5StagedPokemonWorkspace::Region;
using Kind = G::Gen5StagedPokemonWorkspace::ChangeKind;
using Field = G::StagedPokemon5Record::Field;

namespace {

void write32(std::span<uint8_t> s, size_t o, uint32_t v) {
    C::write16(s, o, static_cast<uint16_t>(v));
    C::write16(s, o + 2, static_cast<uint16_t>(v >> 16));
}
bool contains(const std::string& text, std::string_view needle) {
    return text.find(needle) != std::string::npos;
}

struct Spec {
    uint16_t species = 25;
    uint8_t form = 0;
    uint16_t item = 0;
    uint8_t version = 20;
    uint16_t metLocation = 4;   // a native BW/B2W2 route (Gen5EncounterLocations.h)
    uint8_t language = 2;
    uint8_t ball = 4;
    uint8_t nature = 3;
    uint16_t move = 33;         // Tackle
    uint32_t pid = 0x2468ACE1u;
    bool party = false;
};

// PKHeX plaintext PK5 (PK5.cs field offsets), checksum at 0x06.
Bytes pk5(const Spec& s) {
    Bytes b(s.party ? C::PartySize : C::StoredSize, 0);
    write32(b, 0, s.pid);
    C::write16(b, 0x08, s.species);
    C::write16(b, 0x0A, s.item);
    C::write16(b, 0x0C, 12345);
    C::write16(b, 0x0E, 54321);
    write32(b, 0x10, 1000);
    b[0x14] = 70;
    b[0x15] = 9;
    b[0x17] = s.language;
    b[0x18] = 4;                                  // HP EV
    C::write16(b, 0x28, s.move);
    b[0x30] = 35;
    write32(b, 0x38, 0x0012345u);                 // IVs, not nicknamed, not an egg
    b[0x40] = static_cast<uint8_t>(s.form << 3);  // male, form
    b[0x41] = s.nature;
    const char16_t name[] = u"SPARK";
    for (size_t i = 0; i < 5; ++i) C::write16(b, 0x48 + i * 2, name[i]);
    C::write16(b, 0x48 + 10, 0xFFFF);
    b[0x5F] = s.version;
    const char16_t ot[] = u"ASH";
    for (size_t i = 0; i < 3; ++i) C::write16(b, 0x68 + i * 2, ot[i]);
    C::write16(b, 0x68 + 6, 0xFFFF);
    b[0x7B] = 11; b[0x7C] = 3; b[0x7D] = 9;       // met 2011-03-09
    C::write16(b, 0x80, s.metLocation);
    b[0x83] = s.ball;
    b[0x84] = 5;
    if (s.party) {
        b[0x8C] = 12;                              // party level cache
        C::write16(b, 0x8E, 30);                   // current HP
        C::write16(b, 0x90, 30);                   // max HP
    }
    C::write16(b, 0x06, C::checksum(b));
    return b;
}
Bytes stored(const Bytes& plain) { return Bytes(plain.begin(), plain.begin() + C::StoredSize); }

void stamp(std::span<uint8_t> copy, std::span<const L::SaveBlock> blocks) {
    for (const auto& b : blocks) {
        const uint16_t crc = Utils::crc16ccitt(copy.data() + b.offset, b.length);
        C::write16(copy, b.checksumOffset, crc);
        C::write16(copy, b.mirrorOffset, crc);
    }
}

struct Game {
    G::SaveFamily family;
    std::string_view id;
    uint8_t version;
    bool backup;
};
size_t copySize(G::SaveFamily f) { return f == G::SaveFamily::BlackWhite ? L::BlackWhiteCopySize : L::Black2White2CopySize; }
std::span<const L::SaveBlock> blocks(G::SaveFamily f) {
    return f == G::SaveFamily::BlackWhite ? std::span<const L::SaveBlock>(L::BlackWhite)
                                          : std::span<const L::SaveBlock>(L::Black2White2);
}

// One party Pokemon, box 1 slot 1 occupied, slot 3 the retail ENCRYPTED blank, slot 6 a
// species-zero "ghost" record that holds data (not a native blank), every other slot raw zeros.
// Only the selected copy is filled; the other copy stays zero (its CRCs fail), so the selection is
// the explicit one under test.
Bytes save(const Game& g) {
    const size_t base = g.backup ? copySize(g.family) : 0;
    Bytes sav(L::FullSaveSize, 0);
    sav[base + 0x19400 + 0x1F] = g.version;
    sav[base + L::PartyOffset + 4] = 1;
    Spec party; party.party = true; party.species = 495; party.version = g.version; party.pid = 0x11112222u;
    const auto p = C::encryptCandidate(pk5(party));
    std::copy(p.begin(), p.end(), sav.begin() + static_cast<std::ptrdiff_t>(base + L::PartyOffset + 8));
    Spec boxed; boxed.species = 501; boxed.version = g.version; boxed.pid = 0x33334444u;
    const auto b = C::encryptCandidate(pk5(boxed));
    std::copy(b.begin(), b.end(), sav.begin() + static_cast<std::ptrdiff_t>(base + L::BoxOffset));
    const auto& blank = G::encryptedBlankBoxSlot5();
    std::copy(blank.begin(), blank.end(),
              sav.begin() + static_cast<std::ptrdiff_t>(base + L::BoxOffset + 2 * C::StoredSize));
    Bytes ghost(C::StoredSize, 0);
    C::write16(ghost, 0x0A, 1);
    const auto ge = C::encryptCandidate(ghost);
    std::copy(ge.begin(), ge.end(),
              sav.begin() + static_cast<std::ptrdiff_t>(base + L::BoxOffset + 5 * C::StoredSize));
    stamp(std::span<uint8_t>(sav).subspan(base, copySize(g.family)), blocks(g.family));
    return sav;
}

G::SaveCopySelection selection(const Game& g) {
    return g.backup ? G::SaveCopySelection::Backup : G::SaveCopySelection::Primary;
}

F::Decoded decodeOk(const Bytes& file, const char* name) {
    std::string e;
    const auto d = F::decode(file, name, e);
    assert(d && e.empty());
    return *d;
}

// The record now in a box slot of `s`, decrypted.
Bytes slotPlain(const G::Gen5ReadOnlySave& s, size_t box, size_t slot) {
    const auto pk = s.boxPokemon(box, slot);
    assert(pk && pk->valid());
    const auto d = pk->decodedBytes();
    return Bytes(d.begin(), d.end());
}

void importSuccessAndCandidate(const Game& g) {
    const auto input = save(g);
    const auto snapshot = input;
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(input, g.id, &error, selection(g));
    assert(parsed && parsed->selectedBackupPartition() == g.backup);
    G::Gen5StagedPokemonWorkspace ws(*parsed);

    Spec boxSpec; boxSpec.species = 151; boxSpec.version = g.version; boxSpec.item = 1;
    Spec partySpec = boxSpec; partySpec.party = true; partySpec.species = 249; partySpec.pid = 0x0BADF00Du;
    const Bytes boxPlain = pk5(boxSpec), partyPlain = pk5(partySpec);
    const Bytes boxPk5 = boxPlain, boxEk5 = C::encryptCandidate(boxPlain);
    const Bytes partyPk5 = partyPlain, partyEk5 = C::encryptCandidate(partyPlain);

    const auto dBoxPk5 = decodeOk(boxPk5, "mew.pk5");
    const auto dBoxEk5 = decodeOk(boxEk5, "mew.ek5");
    const auto dPartyPk5 = decodeOk(partyPk5, "lugia.pk5");
    const auto dPartyEk5 = decodeOk(partyEk5, "lugia.ek5");
    assert(!dBoxPk5.wasEncrypted && dBoxEk5.wasEncrypted && !dBoxPk5.party && dPartyPk5.party);
    assert(dBoxPk5.plain == boxPlain && dBoxEk5.plain == boxPlain);
    assert(dPartyPk5.plain == partyPlain && dPartyEk5.plain == partyPlain);

    // Slot 2 is raw zeros, slot 3 the retail encrypted blank: both native empty encodings.
    assert(G::importPokemonFile5(ws, dBoxPk5, 0, 1, &error) && error.empty());
    assert(G::importPokemonFile5(ws, dBoxEk5, 0, 2, &error));
    assert(G::importPokemonFile5(ws, dPartyPk5, 1, 0, &error));
    assert(G::importPokemonFile5(ws, dPartyEk5, 23, 29, &error));
    assert(ws.hasChanges() && ws.changedRecordCount() == 4);
    for (const auto& [box, slot, expect] : {std::tuple{size_t{0}, size_t{1}, boxPlain},
                                            std::tuple{size_t{0}, size_t{2}, boxPlain},
                                            std::tuple{size_t{1}, size_t{0}, stored(partyPlain)},
                                            std::tuple{size_t{23}, size_t{29}, stored(partyPlain)}}) {
        const auto view = ws.viewBox(box, slot);
        assert(view && view->valid() && !view->empty() && !view->partyRecord());
        const auto d = view->decodedBytes();
        assert(Bytes(d.begin(), d.end()) == expect);   // the file's record, byte for byte
    }

    // Review: every import is its own row, independently verified, showing its values.
    const auto pending = ws.pendingReview();
    assert(pending.size() == 4);
    for (const auto& c : pending) {
        assert(c.kind == Kind::ImportBox && c.location.region == Region::Box);
        assert(c.importedRecord.size() == C::StoredSize && c.replacementOriginal == c.after);
        assert(G::nativeBlankSlot5(c.before) && c.cloneSourceOriginal.empty() && !c.createRequest);
        const auto v = G::verifyStagedReview(c, &error);
        assert(v && error.empty() && v->fields.size() == 1 && v->fields[0].field == Field::ImportBox);
        assert(!v->created.empty());
    }
    assert(G::groupReviewRows5(pending).size() == 4);

    // An imported Pokemon is editable through the same shared field model; Review replays the edit
    // on top of the imported baseline.
    assert(ws.stageBox(0, 1, Field::Friendship, 0, 200, &error));
    {
        const auto again = ws.pendingReview();
        const auto it = std::find_if(again.begin(), again.end(), [](const auto& c) { return c.location.slot == 1 && c.location.box == 0; });
        assert(it != again.end() && it->after != it->replacementOriginal);
        const auto v = G::verifyStagedReview(*it, &error);
        assert(v && v->fields.size() == 2 && v->fields[1].field == Field::Friendship && v->fields[1].after == 200);
    }

    // Export Copy candidate: strict reparse of the same copy, records byte-identical, CRCs valid
    // (parse is strict), nothing outside the selected copy touched.
    const auto out = G::buildStagedSaveCandidate(ws, &error);
    assert(out && error.empty() && out->changedRecords == 4);
    assert(out->selectedCopyOffset == (g.backup ? copySize(g.family) : 0));
    const auto reparsed = G::Gen5ReadOnlySave::parse(out->bytes, g.id, &error, selection(g));
    assert(reparsed && reparsed->selectedBackupPartition() == g.backup);
    assert(reparsed->partyCount() == 1);
    Bytes edited = boxPlain;
    edited[0x14] = 200;
    C::write16(edited, 0x06, C::checksum(edited));
    assert(slotPlain(*reparsed, 0, 1) == edited);
    assert(slotPlain(*reparsed, 0, 2) == boxPlain);
    assert(slotPlain(*reparsed, 1, 0) == stored(partyPlain));
    assert(slotPlain(*reparsed, 23, 29) == stored(partyPlain));
    {
        const auto kept = reparsed->boxPokemon(0, 0), was = parsed->boxPokemon(0, 0);
        const auto k = kept->originalEncryptedBytes(), w = was->originalEncryptedBytes();
        assert(k.size() == C::StoredSize && std::equal(k.begin(), k.end(), w.begin(), w.end()));
    }
    const size_t base = g.backup ? copySize(g.family) : 0;
    for (size_t i = 0; i < input.size(); ++i)
        if (i < base || i >= base + copySize(g.family)) assert(out->bytes[i] == input[i]);
    assert(G::buildStagedSaveCandidateForObservedSource(ws, input, &error));

    // Discard undoes every import.
    ws.discardAll();
    assert(!ws.hasChanges() && ws.pendingReview().empty());
    assert(ws.viewBox(0, 1)->empty() && ws.viewBox(23, 29)->empty());
    assert(!G::buildStagedSaveCandidate(ws, &error));

    // Undo of one import, and Release of a staged import, return the slot to its own native blank.
    assert(G::importPokemonFile5(ws, dBoxPk5, 0, 1, &error));
    assert(ws.stagedOperand({Region::Box, 0, 1}));
    assert(ws.cloneStagedToEmptyBox({Region::Box, 0, 1}, 0, 2, &error));
    {
        const auto twin = ws.pendingReview();
        assert(twin.size() == 2 && twin[1].kind == Kind::ImportBox);
        for (const auto& c : twin) assert(G::verifyStagedReview(c, &error));
        assert(G::buildStagedSaveCandidate(ws, &error));
    }
    assert(ws.releaseStagedBox(0, 2, &error) && ws.undoBoxReplacement(0, 1) && !ws.hasChanges());

    // Source save bytes and the decoded file records never change.
    assert(input == snapshot);
    assert(std::equal(input.begin(), input.end(), ws.sourceSave().sourceBytes().begin()));
    assert(dBoxPk5.plain == boxPlain && dBoxEk5.plain == boxPlain);
    assert(dPartyPk5.plain == partyPlain && dPartyEk5.plain == partyPlain);
}

void refusals(const Game& g) {
    const auto input = save(g);
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(input, g.id, &error, selection(g));
    assert(parsed);
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    const bool b2w2 = g.family == G::SaveFamily::Black2White2;
    Spec ok; ok.version = g.version;
    const auto good = decodeOk(pk5(ok), "ok.pk5");

    const auto refused = [&](const Bytes& plain, size_t box, size_t slot, std::string_view why) {
        std::string e;
        const bool staged = ws.importToEmptyBox(plain, box, slot, &e);
        if (staged || !contains(e, why)) {
            std::cerr << "expected refusal '" << why << "', got '" << e << "'\n";
            assert(false);
        }
        assert(!ws.hasChanges());
    };

    // Wrong generation: a real decoded .pk4 / .pk3 file. Never converted.
    {
        Bytes pk4(136, 0);
        write32(pk4, 0, 0x2468ACE0u);
        C::write16(pk4, 0x08, 133);
        write32(pk4, 0x10, 5000);
        pk4[0x5F] = 10;
        C::write16(pk4, 0x06, C::checksum(pk4));
        const auto d4 = decodeOk(pk4, "eevee.pk4");
        assert(d4.generation == 4);
        assert(!G::importPokemonFile5(ws, d4, 0, 1, &error) && contains(error, "never converted"));
        F::Decoded d3;
        d3.format = F::Format::PK3; d3.generation = 3; d3.plain.assign(80, 0);
        assert(!G::importPokemonFile5(ws, d3, 0, 1, &error) && contains(error, "Generation V"));
        auto mismatched = good;
        mismatched.party = true;   // says party, holds 136 bytes
        assert(!G::importPokemonFile5(ws, mismatched, 0, 1, &error) && contains(error, "wrong size"));
        assert(!ws.hasChanges());
    }

    // Slot refusals.
    refused(good.plain, 0, 0, "occupied");
    refused(good.plain, 24, 0, "outside");
    refused(good.plain, 0, 30, "outside");
    refused(good.plain, 0, 5, "not a native blank");
    assert(ws.importToEmptyBox(good.plain, 0, 1, nullptr));   // stage once ...
    {
        std::string e;
        assert(!ws.importToEmptyBox(good.plain, 0, 1, &e) && contains(e, "already holds a staged change"));
        assert(ws.changedRecordCount() == 1);
        assert(ws.undoBoxReplacement(0, 1) && !ws.hasChanges());
    }

    // Malformed records.
    {
        Bytes p = pk5(ok); p[0x14] ^= 1;
        refused(p, 0, 1, "checksum");
        p = pk5(ok); C::write16(p, 4, 1); C::write16(p, 6, C::checksum(p));
        refused(p, 0, 1, "sanity");
        Spec s = ok; s.species = 0;
        refused(pk5(s), 0, 1, "no Generation V species");
        s = ok; s.species = 650;
        refused(pk5(s), 0, 1, "no Generation V species");
        refused(Bytes(100, 0), 0, 1, "136 or 220");
        s = ok; s.version = 10;   // Diamond origin with a Gen IV met location
        refused(pk5(s), 0, 1, "Generation IV");
        s = ok; s.version = 0; s.metLocation = 30001;
        refused(pk5(s), 0, 1, "origin game");
        s = ok; s.move = 560;
        refused(pk5(s), 0, 1, "move");
        s = ok; s.nature = 25;
        refused(pk5(s), 0, 1, "nature");
        s = ok; s.ball = 26;
        refused(pk5(s), 0, 1, "Ball");
        s = ok; s.language = 0;
        refused(pk5(s), 0, 1, "language");
        s = ok; s.metLocation = 3;
        refused(pk5(s), 0, 1, "met location");
        s = ok; s.item = 639;
        refused(pk5(s), 0, 1, "held item");
    }

    // Title rules from the existing validators: Kyurem form 2 (646) and Landorus form 1 (645) exist
    // only in the B2W2 personal table; item 638 is only in the B2W2 key-item catalog and 623 only in
    // the BW one (Gen5Bag.h kKeyB2W2 / kKeyBW).
    {
        Spec kyurem = ok; kyurem.species = 646; kyurem.form = 2;
        Spec therian = ok; therian.species = 645; therian.form = 1;
        Spec glass = ok; glass.item = 638;
        Spec light = ok; light.item = 623;
        assert(G::personal5(646, 2, true) && !G::personal5(646, 2, false));
        assert(G::personal5(645, 1, true) && !G::personal5(645, 1, false));
        if (b2w2) {
            assert(ws.importToEmptyBox(pk5(kyurem), 0, 1, &error));
            assert(ws.importToEmptyBox(pk5(therian), 0, 2, &error));
            assert(ws.importToEmptyBox(pk5(glass), 0, 3, &error));
            for (const auto& c : ws.pendingReview()) assert(G::verifyStagedReview(c, &error));
            assert(G::buildStagedSaveCandidate(ws, &error));
            ws.discardAll();
            refused(pk5(light), 0, 1, "does not exist in Black 2/White 2");
        } else {
            refused(pk5(kyurem), 0, 1, "does not exist in Black/White");
            refused(pk5(therian), 0, 1, "does not exist in Black/White");
            refused(pk5(glass), 0, 1, "does not exist in Black/White");
            assert(ws.importToEmptyBox(pk5(light), 0, 1, &error));
            ws.discardAll();
        }
    }
    assert(std::equal(input.begin(), input.end(), ws.sourceSave().sourceBytes().begin()));
}

void tamperedReview(const Game& g) {
    const auto input = save(g);
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(input, g.id, &error, selection(g));
    assert(parsed);
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    Spec s; s.version = g.version; s.species = 143;
    const Bytes plain = pk5(s);
    assert(ws.importToEmptyBox(plain, 0, 1, &error));
    const auto pending = ws.pendingReview();
    assert(pending.size() == 1);
    const auto& real = pending[0];
    assert(G::verifyStagedReview(real, &error));

    const auto rejects = [&](G::Gen5StagedPokemonWorkspace::PendingChange c, int line = __builtin_LINE()) {
        std::string e;
        if (G::verifyStagedReview(c, &e) || e.empty()) {
            std::cerr << "tampered Review change accepted (test line " << line << ")\n";
            assert(false);
        }
    };
    // The record shown is not the record imported.
    auto c = real; c.importedRecord[0x14] ^= 1; rejects(c);
    // The imported record is consistent with itself, but the slot result is something else.
    c = real; { Bytes other = plain; other[0x14] = 1; C::write16(other, 6, C::checksum(other));
        c.importedRecord = other; } rejects(c);
    // The resulting slot bytes were altered after import (no field edit explains them).
    c = real; c.after[20] ^= 0x40; rejects(c);
    // A valid record whose difference no editor field explains (met date, encounter type).
    c = real; { Bytes other = plain; other[0x7D] = 10; other[0x85] = 5; C::write16(other, 6, C::checksum(other));
        c.after = C::encryptCandidate(other); } rejects(c);
    // An authorized edit on top of the import is a verified field change, not an import mismatch.
    c = real; { Bytes other = plain; other[0x41] = 7; C::write16(other, 6, C::checksum(other));
        c.after = C::encryptCandidate(other);
        const auto v = G::verifyStagedReview(c, &error);
        assert(v && v->fields.size() == 2 && v->fields[1].field == Field::Nature); }
    // The slot was not empty.
    {
        const auto occupied = parsed->boxPokemon(0, 0);
        const auto bytes = occupied->originalEncryptedBytes();
        c = real; c.before.assign(bytes.begin(), bytes.end());
        rejects(c);
    }
    // A ghost (species-zero data) "empty" slot is not a native blank.
    c = real; { Bytes ghost(C::StoredSize, 0); C::write16(ghost, 0x0A, 1); c.before = C::encryptCandidate(ghost); }
    rejects(c);
    // Missing / truncated / party-sized imported record.
    c = real; c.importedRecord.clear(); rejects(c);
    c = real; c.importedRecord.resize(C::PartySize); rejects(c);
    // Unrelated evidence riding along.
    c = real; c.cloneSourceOriginal = real.after; rejects(c);
    c = real; c.kind = Kind::CreateBox; rejects(c);
    c = real; c.kind = Kind::CloneBox; rejects(c);
    c = real; c.kind = Kind::FieldEdit; rejects(c);
    c = real; c.kind = Kind::RemoveBox; rejects(c);
    c = real; c.location = {Region::Party, 0, 0}; rejects(c);
    c = real; c.location = {Region::Box, 24, 0}; rejects(c);
    // A self-consistent certificate for a record this title cannot hold (a BW Review must reject
    // Black Kyurem even when every byte agrees).
    {
        Spec k = s; k.species = 646; k.form = 2;
        const Bytes kp = pk5(k);
        c = real; c.importedRecord = kp; c.replacementOriginal = C::encryptCandidate(kp); c.after = c.replacementOriginal;
        c.black2White2 = false;
        rejects(c);
        c.black2White2 = true;
        assert(G::verifyStagedReview(c, &error));
    }
    // Clone / Create / Field-edit changes must never carry an imported record.
    {
        G::Gen5StagedPokemonWorkspace other(*parsed);
        assert(other.cloneToEmptyBox({Region::Box, 0, 0}, 0, 1, &error));
        assert(other.stageBox(0, 0, Field::Friendship, 0, 99, &error));
        for (auto change : other.pendingReview()) {
            assert(G::verifyStagedReview(change, &error));
            change.importedRecord = plain;
            change.importedRecord.resize(C::StoredSize);
            std::string e;
            assert(!G::verifyStagedReview(change, &e));
        }
    }
    assert(std::equal(input.begin(), input.end(), ws.sourceSave().sourceBytes().begin()));
}

void exportRoundTrip(const Game& g) {
    const auto input = save(g);
    const auto snapshot = input;
    std::string error;
    const auto parsed = G::Gen5ReadOnlySave::parse(input, g.id, &error, selection(g));
    assert(parsed);

    // Box export: 136 bytes, PKHeX plaintext, decodes as the same record (.pk5 and as .ek5).
    const auto boxed = G::exportPokemonFile5(*parsed, {Region::Box, 0, 0}, &error);
    assert(boxed && error.empty() && boxed->size() == C::StoredSize);
    assert(C::read16(*boxed, 0x08) == 501 && C::read16(*boxed, 0x06) == C::checksum(*boxed));
    const auto dBox = decodeOk(*boxed, "oshawott.pk5");
    assert(dBox.generation == 5 && dBox.format == F::Format::PK5 && !dBox.party && !dBox.wasEncrypted);
    assert(dBox.plain == *boxed);
    const auto dBoxEnc = decodeOk(C::encryptCandidate(*boxed), "oshawott.ek5");
    assert(dBoxEnc.wasEncrypted && dBoxEnc.plain == *boxed);
    // The save holds that record encrypted; the export is its decryption, nothing else.
    {
        const auto source = parsed->boxPokemon(0, 0);
        const auto bytes = source->originalEncryptedBytes();
        assert(C::encryptCandidate(*boxed) == Bytes(bytes.begin(), bytes.end()));
    }

    // Party export: 220 bytes as stored, with its party extension.
    const auto party = G::exportPokemonFile5(*parsed, {Region::Party, 0, 0}, &error);
    assert(party && party->size() == C::PartySize && (*party)[0x8C] == 12);
    const auto dParty = decodeOk(*party, "snivy.pk5");
    assert(dParty.party && dParty.plain == *party);

    // Refusals: empty slot, undeclared party slot, out-of-range slot, ghost record.
    assert(!G::exportPokemonFile5(*parsed, {Region::Box, 0, 1}, &error) && contains(error, "empty"));
    assert(!G::exportPokemonFile5(*parsed, {Region::Party, 0, 1}, &error) && contains(error, "outside"));
    assert(!G::exportPokemonFile5(*parsed, {Region::Box, 24, 0}, &error));
    assert(!G::exportPokemonFile5(*parsed, {Region::Party, 1, 0}, &error));
    assert(!G::exportPokemonFile5(*parsed, {Region::Box, 0, 5}, &error));

    // Round trip: export -> decode -> import -> candidate -> strict reparse -> export again.
    G::Gen5StagedPokemonWorkspace ws(*parsed);
    assert(G::importPokemonFile5(ws, dBox, 0, 1, &error));
    assert(G::importPokemonFile5(ws, dParty, 0, 2, &error));
    const auto out = G::buildStagedSaveCandidate(ws, &error);
    assert(out);
    const auto reparsed = G::Gen5ReadOnlySave::parse(out->bytes, g.id, &error, selection(g));
    assert(reparsed);
    const auto again = G::exportPokemonFile5(*reparsed, {Region::Box, 0, 1}, &error);
    assert(again && *again == *boxed);
    const auto fromParty = G::exportPokemonFile5(*reparsed, {Region::Box, 0, 2}, &error);
    assert(fromParty && *fromParty == stored(*party));
    // The original Pokemon is still where it was.
    assert(G::exportPokemonFile5(*reparsed, {Region::Box, 0, 0}, &error) == boxed);

    // Export never changes the source.
    assert(input == snapshot);
    assert(std::equal(input.begin(), input.end(), parsed->sourceBytes().begin()));
}

}  // namespace

int main() {
    const std::array<Game, 4> games{{
        {G::SaveFamily::BlackWhite, "white_nds", 20, false},
        {G::SaveFamily::BlackWhite, "black_nds", 21, true},
        {G::SaveFamily::Black2White2, "white2_nds", 22, false},
        {G::SaveFamily::Black2White2, "black2_nds", 23, true},
    }};
    for (const auto& g : games) {
        importSuccessAndCandidate(g);
        refusals(g);
        tamperedReview(g);
        exportRoundTrip(g);
    }
    std::cout << "Gen V standalone Pokemon files (.pk5/.ek5): BW/B2W2 primary+backup import to staged "
                 "box, refusals, Review re-verification, candidate reparse, export round trip PASS\n";
}
