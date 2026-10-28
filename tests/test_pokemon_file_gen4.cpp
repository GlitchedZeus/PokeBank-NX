// Feature A (owner Oct 28): standalone .pk4 / .ek4 files against the Gen IV staged editor.
// Import places a decoded PK4 into an EMPTY box slot of the app-owned staged image (Review /
// Discard / finalize unchanged); export returns the PKHeX plaintext PK4 (PKHeX PK4
// DecryptedBoxData / DecryptedPartyData) that PokemonFile::decode() reads back.
// Synthetic saves only (tests/fuzz_synthetic_saves.h qasynth::gen4, the builder pinned from
// tests/test_gen4_staged_pokemon_editor.cpp makeSave()); no real save, ROM or key.
#include "Encryption/Encryption4.h"
#include "Integration/Gen4/Gen4ExportTransaction.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "PokemonFile/NativePokemonFile.h"
#include "fuzz_synthetic_saves.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace F = PokeBank::PokemonFile;
namespace S = qasynth::gen4;
using PokeVault::Integration::Gen4::Gen4ReadOnlySave;
using PokeVault::Integration::Gen4::Gen4StagedPokemonEditor;
using PokeVault::Integration::Gen4::Layout;
using Bytes = std::vector<uint8_t>;

namespace {

struct Game {
    S::Kind kind;
    Layout layout;
    const char* id;
    uint8_t rom; // trainer version byte the exact assignment checks (test_gen4_staged_pokemon_editor)
};

constexpr Game kGames[] = {
    {S::Kind::DiamondPearl, Layout::DiamondPearl, "diamond_nds", 10},
    {S::Kind::DiamondPearl, Layout::DiamondPearl, "pearl_nds", 11},
    {S::Kind::Platinum, Layout::Platinum, "platinum_nds", 12},
    {S::Kind::HeartGoldSoulSilver, Layout::HeartGoldSoulSilver, "heartgold_nds", 7},
    {S::Kind::HeartGoldSoulSilver, Layout::HeartGoldSoulSilver, "soulsilver_nds", 8},
};

void w16(Bytes& b, size_t o, uint16_t v) {
    b[o] = static_cast<uint8_t>(v);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
}
void w32(Bytes& b, size_t o, uint32_t v) {
    w16(b, o, static_cast<uint16_t>(v));
    w16(b, o + 2, static_cast<uint16_t>(v >> 16));
}
uint16_t r16(std::span<const uint8_t> b, size_t o) {
    return static_cast<uint16_t>(b[o] | (b[o + 1] << 8));
}
Bytes toBytes(std::span<const std::byte> in) {
    Bytes out(in.size());
    std::transform(in.begin(), in.end(), out.begin(),
                   [](std::byte v) { return static_cast<uint8_t>(v); });
    return out;
}
void reseal(Bytes& plain) {
    w16(plain, 0x06, Encryption::checksum4(std::as_bytes(std::span<const uint8_t>(plain).first(0x88))));
}

struct Spec4 {
    uint16_t species = 1;
    uint8_t form = 0;
    uint16_t item = 0;
    uint16_t move1 = 33;
    uint8_t language = 2;
    uint32_t pid = 0xA1B2C3D4u;
};

// A plaintext stored PK4 (PKHeX PK4 layout) built from the shared synthetic record with a few
// distinctive fields. Bytes the importer must keep verbatim, including "unused" ones.
Bytes plainPk4(const Spec4& s = {}) {
    Bytes b = toBytes(S::decryptedStored(s.species));
    w32(b, 0x00, s.pid);
    w16(b, 0x0A, s.item);
    b[0x17] = s.language;
    w16(b, 0x28, s.move1);
    w16(b, 0x2A, 45);
    b[0x30] = 35;
    b[0x31] = 40;
    w32(b, 0x38, 0x1234ABCDu & 0x3FFFFFFFu);           // IVs (no egg / nickname flags)
    b[0x40] = static_cast<uint8_t>((s.form << 3) | 0x04); // form + genderless bit
    b[0x5F] = 10;                                       // origin: Diamond
    b[0x7C] = 0x5A;                                     // an unused byte must survive
    reseal(b);
    return b;
}
// PKHeX PK4 party record: stored 136 bytes + plaintext battle-stat cache (0x88..0xEB).
Bytes plainPartyPk4(const Spec4& s = {}) {
    Bytes b = plainPk4(s);
    b.resize(Encryption::SIZE_PARTY4, 0);
    b[0x8C] = 17;
    w16(b, 0x8E, 40);
    w16(b, 0x90, 44);
    for (size_t o = 0x92; o < 0x9C; o += 2) w16(b, o, 30);
    b[0xE0] = 0x77;
    return b;
}
Bytes ek4(const Bytes& plain) {
    return toBytes(Encryption::encryptArray4(std::as_bytes(std::span<const uint8_t>(plain))));
}

F::Decoded decodeOk(const Bytes& file, const char* name) {
    std::string error;
    auto d = F::decode(file, name, error);
    assert(d && error.empty());
    return *d;
}
F::Decoded manual(const Bytes& plain, int generation = 4, F::Format format = F::Format::PK4) {
    F::Decoded d;
    d.format = format;
    d.generation = generation;
    d.party = plain.size() == Encryption::SIZE_PARTY4;
    d.plain = plain;
    return d;
}

Gen4StagedPokemonEditor openEditor(const Bytes& source, const Game& g) {
    std::string error;
    auto editor = Gen4StagedPokemonEditor::create(source, g.layout, g.id, &error);
    if (!editor) std::cerr << g.id << ": " << error << "\n";
    assert(editor && error.empty());
    return std::move(*editor);
}

Bytes slotPlain(const Gen4StagedPokemonEditor& editor, size_t box, size_t slot) {
    std::string error;
    const auto pokemon = editor.boxedPokemon(box, slot, &error);
    assert(pokemon && error.empty());
    return toBytes(pokemon->decryptedBytes());
}

bool hasRow(const std::vector<std::pair<std::string, std::string>>& rows,
            const std::string& key, const std::string& valuePart = {}) {
    return std::any_of(rows.begin(), rows.end(), [&](const auto& r) {
        return r.first == key && r.second.find(valuePart) != std::string::npos;
    });
}

// Refusal: false, a reason containing `why`, and no staged byte changed.
void refuse(Gen4StagedPokemonEditor& editor, size_t box, size_t slot, const F::Decoded& file,
            const std::string& why) {
    const auto before = editor.stagedBytes();
    const auto plainBefore = file.plain;
    std::string error;
    assert(!editor.stageImportBoxPokemon(box, slot, file, &error));
    if (error.find(why) == std::string::npos) {
        std::cerr << "expected refusal containing '" << why << "', got '" << error << "'\n";
        assert(false);
    }
    assert(editor.stagedBytes() == before);
    assert(file.plain == plainBefore);
}

// 1. Success path in every game: .pk4, .ek4, party-sized .pk4/.ek4; Review; finalize + strict
//    reparse; byte footprint limited to the imported slots + the selected Storage CRC; discard;
//    source immutability. The newer Storage copy alternates so both partitions are exercised.
void testImportSuccessAllGames() {
    int variant = 0;
    for (const auto& g : kGames) {
        for (int storagePart = 0; storagePart < 2; ++storagePart, ++variant) {
            const auto source = S::save(g.kind, g.rom, variant % 2, storagePart);
            const auto pristine = source;
            auto editor = openEditor(source, g);

            const Bytes boxPlain = plainPk4({.species = 1, .item = 1});
            const Bytes boxFile = boxPlain;
            const Bytes encFile = ek4(plainPk4({.species = 4, .pid = 0x0BADF00Du}));
            const Bytes partyPlain = plainPartyPk4({.species = 7, .pid = 0x13572468u});
            const Bytes partyFile = partyPlain;
            const Bytes partyEnc = ek4(plainPartyPk4({.species = 152, .pid = 0x2468ACE1u}));
            const auto filesBefore = std::vector<Bytes>{boxFile, encFile, partyFile, partyEnc};

            const auto dBox = decodeOk(boxFile, "bulbasaur.pk4");
            const auto dEnc = decodeOk(encFile, "charmander.ek4");
            const auto dParty = decodeOk(partyFile, "squirtle.pk4");
            const auto dPartyEnc = decodeOk(partyEnc, "chikorita.ek4");
            assert(!dBox.wasEncrypted && dEnc.wasEncrypted && dParty.party && dPartyEnc.party);

            std::string error;
            assert(editor.canImportPokemonFile(dBox, &error) && error.empty());
            assert(editor.stageImportBoxPokemon(0, 5, dBox, &error) && error.empty());
            assert(editor.stageImportBoxPokemon(0, 6, dEnc, &error) && error.empty());
            assert(editor.stageImportBoxPokemon(1, 0, dParty, &error) && error.empty());
            assert(editor.stageImportBoxPokemon(17, 29, dPartyEnc, &error) && error.empty());
            assert(editor.hasChanges());

            // Every stored byte kept as given (no reroll / repair); party files contribute 136 bytes.
            assert(slotPlain(editor, 0, 5) == boxPlain);
            assert(slotPlain(editor, 0, 6) == dEnc.plain);
            assert(slotPlain(editor, 1, 0) == Bytes(partyPlain.begin(), partyPlain.begin() + 0x88));
            assert(slotPlain(editor, 17, 29) == Bytes(dPartyEnc.plain.begin(), dPartyEnc.plain.begin() + 0x88));
            assert(slotPlain(editor, 0, 5)[0x7C] == 0x5A); // an unused byte survived verbatim

            // Review: the existing derived-diff Review shows each import; new records get CG74 detail rows.
            const auto lines = PokeVault::Integration::Gen4::describeStagedSlotChanges(editor);
            assert(std::find(lines.begin(), lines.end(), "Box 1 slot 6: (empty) -> Bulbasaur #1") != lines.end());
            assert(std::find(lines.begin(), lines.end(), "Box 1 slot 7: (empty) -> Charmander #4") != lines.end());
            assert(std::find(lines.begin(), lines.end(), "Box 2 slot 1: (empty) -> Squirtle #7") != lines.end());
            assert(std::find(lines.begin(), lines.end(), "Box 18 slot 30: (empty) -> Chikorita #152") != lines.end());
            const auto rows = PokeVault::Integration::Gen4::describeStagedReviewChanges(editor);
            assert(hasRow(rows, "Staged change", "Box 1 slot 6: (empty) -> Bulbasaur #1"));
            assert(hasRow(rows, "Box 1 / Slot 6 / OT / TID / SID", "ASH / 12345 / 54321"));
            assert(hasRow(rows, "Box 1 / Slot 6 / Held Item", "Master Ball"));
            assert(hasRow(rows, "Box 1 / Slot 6 / Moves", "Tackle, Growl"));

            // Finalize: strict reparse with each record byte-identical (decrypted) in its slot.
            const auto finalBytes = editor.finalizedBytes(&error);
            assert(!finalBytes.empty() && error.empty());
            const auto reparsed = Gen4ReadOnlySave::parse(finalBytes, g.layout, g.id, &error);
            assert(reparsed && error.empty());
            assert(reparsed->assignmentStatus() == PokeVault::Integration::Gen4::AssignmentStatus::Match);
            assert(!reparsed->recovered());
            assert(toBytes(reparsed->box(0, 5).decryptedBytes()) == boxPlain);
            assert(toBytes(reparsed->box(0, 6).decryptedBytes()) == dEnc.plain);
            assert(toBytes(reparsed->box(1, 0).decryptedBytes()) ==
                   Bytes(partyPlain.begin(), partyPlain.begin() + 0x88));
            assert(toBytes(reparsed->box(17, 29).decryptedBytes()) ==
                   Bytes(dPartyEnc.plain.begin(), dPartyEnc.plain.begin() + 0x88));
            assert(reparsed->box(0, 0).valid() && !reparsed->box(0, 0).empty()); // existing untouched

            // Footprint: only the four slots of the SELECTED Storage copy and its CRC changed;
            // General blocks and the other Storage copy are untouched (as Clone/Create do).
            const auto sp = S::spec(g.kind);
            const size_t storage = static_cast<size_t>(storagePart) * S::kPartition + sp.storageStart;
            const auto slotAt = [&](size_t box, size_t slot) {
                return storage + sp.boxData + box * sp.boxStride + slot * Encryption::SIZE_STORED4;
            };
            const size_t slots[] = {slotAt(0, 5), slotAt(0, 6), slotAt(1, 0), slotAt(17, 29)};
            const size_t crc = storage + sp.storageSize - 2;
            for (size_t i = 0; i < finalBytes.size(); ++i) {
                if (finalBytes[i] == pristine[i]) continue;
                bool allowed = i >= crc && i < crc + 2;
                for (const size_t s : slots) allowed = allowed || (i >= s && i < s + Encryption::SIZE_STORED4);
                assert(allowed);
            }

            // Source / original / files never change.
            assert(source == pristine);
            assert(editor.originalBytes() == pristine);
            assert((std::vector<Bytes>{boxFile, encFile, partyFile, partyEnc}) == filesBefore);

            // Discard undoes every import.
            editor.discard();
            assert(!editor.hasChanges());
            assert(editor.finalizedBytes(&error) == pristine);
            assert(PokeVault::Integration::Gen4::describeStagedReviewChanges(editor).empty());
            assert(!editor.boxedPokemon(0, 5, &error));
        }
    }
}

// 2. Every refusal, per game where it differs.
void testImportRefusals() {
    for (const auto& g : kGames) {
        const auto source = S::save(g.kind, g.rom);
        const auto pristine = source;
        auto editor = openEditor(source, g);
        const bool dp = g.layout == Layout::DiamondPearl;
        const bool hgss = g.layout == Layout::HeartGoldSoulSilver;
        const auto good = decodeOk(plainPk4(), "a.pk4");

        // Wrong generation: a real .pk5 (decoder-proven Gen V content) and a Gen III record.
        Bytes pk5(136, 0);
        w32(pk5, 0, 0x2468ACE0u);
        w16(pk5, 0x08, 25);
        w32(pk5, 0x10, 5000);
        pk5[0x5F] = 20; // White
        w16(pk5, 0x80, 0x5555);
        w16(pk5, 0x06, Encryption::checksum4(std::as_bytes(std::span<const uint8_t>(pk5))));
        const auto d5 = decodeOk(pk5, "pikachu.pk5");
        assert(d5.generation == 5);
        refuse(editor, 0, 1, d5, "Generation 5 Pokemon file");
        refuse(editor, 0, 1, manual(Bytes(80, 0), 3, F::Format::PK3), "Generation 3 Pokemon file");
        refuse(editor, 0, 1, manual(plainPk4(), 4, F::Format::PK5), "accepts only .pk4");

        // Slot: occupied, out of range (box and slot).
        refuse(editor, 0, 0, good, "occupied");
        refuse(editor, 18, 0, good, "outside native 18 x 30");
        refuse(editor, 0, 30, good, "outside native 18 x 30");

        // Malformed records (decode() refuses these too; the editor re-checks defensively).
        refuse(editor, 0, 1, manual(Bytes(100, 0)), "not 136 (box) or 236 (party)");
        Bytes badSum = plainPk4();
        badSum[0x20] ^= 1;
        refuse(editor, 0, 1, manual(badSum), "checksum does not match");
        Bytes zero = plainPk4({.species = 0});
        refuse(editor, 0, 1, manual(zero), "holds no Pokemon");
        Bytes sanity = plainPk4();
        w16(sanity, 0x04, 1); // sanity / Battle Revolution flag
        refuse(editor, 0, 1, manual(sanity), "strict PK4");
        Bytes beyond = plainPk4({.species = 494});
        refuse(editor, 0, 1, manual(beyond), "does not exist in");

        // Forms not in this game's personal table (PKHeX PersonalTable.DP / Pt / HGSS).
        const auto rotomWash = decodeOk(plainPk4({.species = 479, .form = 1}), "rotom.pk4");
        const auto pichuSpiky = decodeOk(plainPk4({.species = 172, .form = 1}), "pichu.pk4");
        const auto giratinaOrigin =
            decodeOk(plainPk4({.species = 487, .form = 1, .item = 112}), "giratina.pk4");
        std::string error;
        if (dp) {
            refuse(editor, 0, 1, rotomWash, "Rotom #479 form 1 does not exist in Diamond/Pearl");
            refuse(editor, 0, 1, giratinaOrigin, "form 1 does not exist in Diamond/Pearl");
        } else {
            assert(editor.canImportPokemonFile(rotomWash, &error) && error.empty());
            assert(editor.canImportPokemonFile(giratinaOrigin, &error) && error.empty());
        }
        if (!hgss)
            refuse(editor, 0, 1, pichuSpiky, "Pichu #172 form 1 does not exist in");
        else
            assert(editor.canImportPokemonFile(pichuSpiky, &error) && error.empty());
        refuse(editor, 0, 1, decodeOk(plainPk4({.species = 201, .form = 28}), "unown.pk4"),
               "form 28 does not exist");
        // Shaymin Sky Forme never persists in a Gen IV box (PKHeX FormVerifier; setForm rule).
        refuse(editor, 0, 1, decodeOk(plainPk4({.species = 492, .form = 1}), "shaymin.pk4"),
               dp ? "form 1 does not exist in Diamond/Pearl" : "Shaymin Sky Forme");

        // Held items: Griseous Orb is Pt/HGSS only; Safari Ball (5) and Apricorns (485+) are not holdable.
        const auto orb = decodeOk(plainPk4({.species = 25, .item = 112}), "orb.pk4");
        if (dp) refuse(editor, 0, 1, orb, "Held item #112 is not a holdable item in Diamond/Pearl");
        else assert(editor.canImportPokemonFile(orb, &error) && error.empty());
        refuse(editor, 0, 1, decodeOk(plainPk4({.item = 5}), "b.pk4"), "Held item #5");
        refuse(editor, 0, 1, decodeOk(plainPk4({.item = 485}), "c.pk4"), "Held item #485");

        // Moves past Shadow Force (467) are not Gen IV moves.
        refuse(editor, 0, 1, decodeOk(plainPk4({.move1 = 468}), "d.pk4"), "Move #468");
        // Language ids 0 / 6 are not game languages (Enums::groupHasLanguage).
        refuse(editor, 0, 1, decodeOk(plainPk4({.language = 0}), "e.pk4"), "Language id 0");
        refuse(editor, 0, 1, decodeOk(plainPk4({.language = 6}), "f.pk4"), "Language id 6");

        assert(!editor.hasChanges());
        assert(source == pristine);
    }
}

// 3. Export is non-mutating and decode() returns exactly the record; export -> decode -> import
//    round-trips byte-identically (box and party, plain .pk4 and .ek4).
void testExportRoundTrip() {
    for (const auto& g : kGames) {
        const auto source = S::save(g.kind, g.rom);
        const auto pristine = source;
        auto editor = openEditor(source, g);
        std::string error;

        const auto box = editor.exportBoxPokemonFile(0, 0, &error);
        assert(box && error.empty() && box->size() == Encryption::SIZE_STORED4);
        Bytes expected = toBytes(S::decryptedStored()); // the synthetic slot's plaintext...
        reseal(expected);                               // ...with the checksum encryptArray4 stored
        assert(*box == expected);                       // PKHeX plaintext PK4, canonical ABCD
        assert(r16(*box, 0x06) == Encryption::checksum4(std::as_bytes(std::span<const uint8_t>(*box))));
        const auto party = editor.exportPartyPokemonFile(0, &error);
        assert(party && error.empty() && party->size() == Encryption::SIZE_PARTY4);
        assert(Bytes(party->begin(), party->begin() + 0x88) == *box); // same synthetic record
        assert((*party)[0x8C] == 20 && r16(*party, 0x90) == 35);       // battle cache as stored
        assert(!editor.hasChanges() && editor.stagedBytes() == pristine);

        // Refusals: empty slot, out of range, no such party member.
        assert(!editor.exportBoxPokemonFile(0, 1, &error) && error.find("empty") != std::string::npos);
        assert(!editor.exportBoxPokemonFile(18, 0, &error) && !error.empty());
        assert(!editor.exportPartyPokemonFile(1, &error) && !error.empty());

        const auto dBox = decodeOk(*box, "pikachu.pk4");
        assert(dBox.plain == *box && !dBox.party && dBox.generation == 4);
        const auto dParty = decodeOk(*party, "pikachu-party.pk4");
        assert(dParty.plain == *party && dParty.party);
        const auto dEk = decodeOk(ek4(*box), "pikachu.ek4");
        assert(dEk.plain == *box && dEk.wasEncrypted);

        assert(editor.stageImportBoxPokemon(2, 3, dBox, &error) && error.empty());
        assert(editor.stageImportBoxPokemon(2, 4, dParty, &error) && error.empty());
        assert(editor.stageImportBoxPokemon(2, 5, dEk, &error) && error.empty());
        for (size_t slot = 3; slot <= 5; ++slot) {
            assert(slotPlain(editor, 2, slot) == *box);
            const auto again = editor.exportBoxPokemonFile(2, slot, &error); // exports staged view
            assert(again && *again == *box);
        }
        // Encrypted slot bytes identical to the source slot (no reroll, no reshuffle).
        const auto a = editor.boxedPokemon(0, 0, &error), b = editor.boxedPokemon(2, 3, &error);
        assert(a && b && std::equal(a->originalEncryptedBytes().begin(), a->originalEncryptedBytes().end(),
                                    b->originalEncryptedBytes().begin(), b->originalEncryptedBytes().end()));
        // Review: a record already in the save gets its slot line only (no Create detail rows).
        const auto rows = PokeVault::Integration::Gen4::describeStagedReviewChanges(editor);
        assert(hasRow(rows, "Staged change", "Box 3 slot 4: (empty) -> Pikachu #25"));
        assert(!hasRow(rows, "Box 3 / Slot 4 / OT / TID / SID"));

        const auto finalBytes = editor.finalizedBytes(&error);
        assert(!finalBytes.empty() && error.empty());
        const auto reparsed = Gen4ReadOnlySave::parse(finalBytes, g.layout, g.id, &error);
        assert(reparsed && error.empty() && toBytes(reparsed->box(2, 4).decryptedBytes()) == *box);
        assert(source == pristine && editor.originalBytes() == pristine);
    }
}

// 4. A record exported from one game group imports into another only when that game's validators
//    accept it: Platinum's Rotom-Wash goes to HGSS, not to Diamond/Pearl.
void testCrossGroupRoundTrip() {
    const auto pt = S::save(S::Kind::Platinum, kGames[2].rom);
    auto ptEditor = openEditor(pt, kGames[2]);
    std::string error;
    assert(ptEditor.stageImportBoxPokemon(0, 1, decodeOk(plainPk4({.species = 479, .form = 1}), "r.pk4"), &error));
    const auto exported = ptEditor.exportBoxPokemonFile(0, 1, &error);
    assert(exported);
    const auto decoded = decodeOk(*exported, "rotom.pk4");

    const auto hg = S::save(S::Kind::HeartGoldSoulSilver, kGames[3].rom);
    auto hgEditor = openEditor(hg, kGames[3]);
    assert(hgEditor.stageImportBoxPokemon(4, 4, decoded, &error) && error.empty());
    assert(slotPlain(hgEditor, 4, 4) == *exported);

    const auto dpSave = S::save(S::Kind::DiamondPearl, kGames[0].rom);
    auto dpEditor = openEditor(dpSave, kGames[0]);
    refuse(dpEditor, 4, 4, decoded, "does not exist in Diamond/Pearl");
}

} // namespace

int main() {
    testImportSuccessAllGames();
    testImportRefusals();
    testExportRoundTrip();
    testCrossGroupRoundTrip();
    std::cout << "Gen IV standalone Pokemon file import/export PASS\n";
    return 0;
}
