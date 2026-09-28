#include "Encryption/Encryption4.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Utils/CRC16.h"
#include "Utils/Gen4TextCodec.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

using PokeVault::Integration::Gen4::Gen4ReadOnlySave;
using PokeVault::Integration::Gen4::Gen4StagedPokemonEditor;
using PokeVault::Integration::Gen4::Layout;

namespace {

constexpr size_t SAVE_SIZE = 0x80000;
constexpr size_t PARTITION = 0x40000;
constexpr uint32_t MAGIC = 0x20060623;

struct Spec {
    size_t generalSize;
    size_t storageStart;
    size_t storageSize;
    size_t footerSize;
    size_t trainer;
    size_t party;
    size_t boxData;
    size_t boxStride;
};

Spec spec(Layout layout) {
    if (layout == Layout::DiamondPearl)
        return {0xC100, 0xC100, 0x121E0, 0x14, 0x64, 0x98, 4, 0xFF0};
    if (layout == Layout::Platinum)
        return {0xCF2C, 0xCF2C, 0x121E4, 0x14, 0x68, 0xA0, 4, 0xFF0};
    return {0xF628, 0xF700, 0x12310, 0x10, 0x64, 0x98, 0, 0x1000};
}

void w16(std::vector<uint8_t>& b, size_t o, uint16_t v) {
    b[o] = static_cast<uint8_t>(v);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
}
void w32(std::vector<uint8_t>& b, size_t o, uint32_t v) {
    b[o] = static_cast<uint8_t>(v);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
    b[o + 2] = static_cast<uint8_t>(v >> 16);
    b[o + 3] = static_cast<uint8_t>(v >> 24);
}
void wb16(std::vector<std::byte>& b, size_t o, uint16_t v) {
    b[o] = static_cast<std::byte>(v);
    b[o + 1] = static_cast<std::byte>(v >> 8);
}
void wb32(std::vector<std::byte>& b, size_t o, uint32_t v) {
    b[o] = static_cast<std::byte>(v);
    b[o + 1] = static_cast<std::byte>(v >> 8);
    b[o + 2] = static_cast<std::byte>(v >> 16);
    b[o + 3] = static_cast<std::byte>(v >> 24);
}
void copy(std::vector<uint8_t>& dst, size_t off, std::span<const std::byte> src) {
    for (size_t i = 0; i < src.size(); ++i)
        dst[off + i] = static_cast<uint8_t>(src[i]);
}
void copy(std::vector<std::byte>& dst, size_t off, std::span<const std::byte> src) {
    std::copy(src.begin(), src.end(),
              dst.begin() + static_cast<std::ptrdiff_t>(off));
}

std::vector<std::byte> occupiedPk4(uint16_t species = 25) {
    std::vector<std::byte> d(Encryption::SIZE_STORED4, std::byte{0});
    wb32(d, 0x00, 0x12345678u);
    wb16(d, 0x08, species);
    wb16(d, 0x0A, 1);
    wb16(d, 0x0C, 12345);
    wb16(d, 0x0E, 54321);
    wb32(d, 0x10, 10000);
    d[0x14] = std::byte{70};
    d[0x15] = std::byte{9};
    d[0x17] = std::byte{2};
    auto nickname = Utils::encodeGen4Field(u"PIKA", 11, 10, 2);
    copy(d, 0x48, nickname);
    auto ot = Utils::encodeGen4Field(u"ASH", 8, 7, 2);
    copy(d, 0x68, ot);
    d[0x83] = std::byte{4};
    d[0x84] = std::byte{25};
    d[0x86] = std::byte{17};
    return Encryption::encryptArray4(d);
}

std::vector<std::byte> occupiedPartyPk4(uint16_t species = 25) {
    std::vector<std::byte> d(Encryption::SIZE_PARTY4, std::byte{0});
    const auto stored = occupiedPk4(species);
    const auto decoded = Encryption::decryptArray4(stored);
    std::copy(decoded.begin(), decoded.end(), d.begin());
    d[0x8C] = std::byte{20};
    wb16(d, 0x8E, 35);
    wb16(d, 0x90, 35);
    wb16(d, 0x92, 25);
    wb16(d, 0x94, 20);
    wb16(d, 0x96, 30);
    wb16(d, 0x98, 25);
    wb16(d, 0x9A, 20);
    return Encryption::encryptArray4(d);
}

void stamp(std::vector<uint8_t>& save, size_t off, size_t len,
           size_t footer, uint32_t major) {
    const size_t end = off + len;
    w32(save, end - 0x14, major);
    w32(save, end - 0x10, 1);
    w32(save, end - 0x0C, static_cast<uint32_t>(len));
    w32(save, end - 0x08, MAGIC);
    w16(save, end - 2,
        Utils::crc16ccitt(save.data() + off, len - footer));
}

std::vector<uint8_t> makeSave(Layout layout, uint8_t romCode = 7,
                                  int newerGeneral = 0, int newerStorage = 0) {
    const auto sp = spec(layout);
    std::vector<uint8_t> save(SAVE_SIZE, 0xFF);
    const auto blank = Encryption::blankRecord4(Encryption::SIZE_STORED4);
    const auto occupied = occupiedPk4();
    const auto party = occupiedPartyPk4();

    for (int partition = 0; partition < 2; ++partition) {
        const size_t base = static_cast<size_t>(partition) * PARTITION;
        std::fill(save.begin() + static_cast<std::ptrdiff_t>(base),
                  save.begin() + static_cast<std::ptrdiff_t>(base + sp.generalSize), 0);
        const size_t storage = base + sp.storageStart;
        std::fill(save.begin() + static_cast<std::ptrdiff_t>(storage),
                  save.begin() + static_cast<std::ptrdiff_t>(storage + sp.storageSize), 0);

        auto trainerName = Utils::encodeGen4Field(u"Kylie", 8, 7, 2);
        copy(save, base + sp.trainer, trainerName);
        w16(save, base + sp.trainer + 0x10, 12150);
        w16(save, base + sp.trainer + 0x12, 22558);
        save[base + sp.trainer + 0x18] = 1;
        save[base + sp.trainer + 0x19] = 2;
        save[base + sp.trainer + 0x1C] = romCode;
        save[base + sp.party - 4] = 1;
        copy(save, base + sp.party, party);

        for (size_t box = 0; box < 18; ++box) {
            const size_t boxBase = storage + sp.boxData + box * sp.boxStride;
            for (size_t slot = 0; slot < 30; ++slot)
                copy(save, boxBase + slot * Encryption::SIZE_STORED4, blank);
            if (layout == Layout::HeartGoldSoulSilver) {
                std::fill(
                    save.begin() + static_cast<std::ptrdiff_t>(boxBase + 0xFF0),
                    save.begin() + static_cast<std::ptrdiff_t>(boxBase + 0x1000),
                    static_cast<uint8_t>(0xC0 + box));
            }
        }
        copy(save, storage + sp.boxData, occupied);

        if (layout == Layout::HeartGoldSoulSilver)
            save[storage + 0x12000] = 0;
        else
            save[storage] = 0;

        stamp(save, base, sp.generalSize, sp.footerSize,
              partition == newerGeneral ? 20 : 10);
        stamp(save, storage, sp.storageSize, sp.footerSize,
              partition == newerStorage ? 30 : 15);
    }
    return save;
}

std::string gameId(Layout layout, bool soulSilver = false) {
    if (layout == Layout::DiamondPearl) return "diamond_nds";
    if (layout == Layout::Platinum) return "platinum_nds";
    return soulSilver ? "soulsilver_nds" : "heartgold_nds";
}

void testLayout(Layout layout, uint8_t romCode = 7, bool soulSilver = false) {
    auto source = makeSave(layout, romCode);
    const auto original = source;
    std::string error;
    auto editor = Gen4StagedPokemonEditor::create(
        source, layout, gameId(layout, soulSilver), &error);
    assert(editor && error.empty());
    assert(source == original);
    assert(!editor->hasChanges());

    auto mon = editor->editableBoxPokemon(0, 0, &error);
    assert(mon && error.empty());
    const uint8_t beforeNature = mon->nature();
    assert(mon->setFriendship(222));
    assert(mon->setIV(0, 31));
    assert(mon->setEV(1, 200));
    assert(mon->setNature(static_cast<uint8_t>((beforeNature + 1) % 25)));
    assert(editor->commitBoxPokemon(0, 0, *mon, &error));
    assert(error.empty());
    assert(editor->hasChanges());
    assert(source == original);

    const auto finalBytes = editor->finalizedBytes(&error);
    assert(!finalBytes.empty() && error.empty());
    assert(finalBytes.size() == original.size());

    auto parsed = Gen4ReadOnlySave::parse(
        finalBytes, layout, gameId(layout, soulSilver), &error);
    assert(parsed && error.empty());
    assert(parsed->box(0, 0).valid());
    assert(parsed->box(0, 0).friendship() == 222);
    assert(parsed->box(0, 0).ivs()[0] == 31);
    assert(parsed->box(0, 0).evs()[1] == 200);
    assert(parsed->box(0, 0).pid() % 25 ==
           static_cast<uint8_t>((beforeNature + 1) % 25));

    // Party editing uses the same mutable PK4 core but commits a full 0xEC party
    // record into the selected General block, refreshes its CRC and keeps live stats coherent.
    auto partyMon = editor->editablePartyPokemon(0, &error);
    assert(partyMon && error.empty() && partyMon->isParty());
    const auto partyBefore = partyMon->encryptedBytes();
    assert(partyMon->setLevel(30));
    assert(partyMon->setIV(0, 31));
    assert(partyMon->setEV(1, 180));
    assert(editor->commitPartyPokemon(0, *partyMon, &error));
    assert(error.empty());
    assert(source == original);

    const auto partyFinal = editor->finalizedBytes(&error);
    assert(!partyFinal.empty() && error.empty());
    auto partyParsed = Gen4ReadOnlySave::parse(
        partyFinal, layout, gameId(layout, soulSilver), &error);
    assert(partyParsed && error.empty());
    const auto nativeParty = partyParsed->nativePartySlots();
    assert(nativeParty.size() == 6);
    assert(nativeParty[0].valid() && nativeParty[0].isParty());
    assert(nativeParty[0].partyLevel() == 30);
    assert(nativeParty[0].ivs()[0] == 31);
    assert(nativeParty[0].evs()[1] == 180);
    assert(nativeParty[0].maxHP() > 0);
    assert(nativeParty[0].currentHP() <= nativeParty[0].maxHP());
    assert(nativeParty[0].originalEncryptedBytes().size() == Encryption::SIZE_PARTY4);
    assert(!std::equal(partyBefore.begin(), partyBefore.end(),
                       nativeParty[0].originalEncryptedBytes().begin()));

    // The inactive partition is not rewritten as a side effect.
    assert(std::equal(finalBytes.begin() + static_cast<std::ptrdiff_t>(PARTITION),
                      finalBytes.end(),
                      original.begin() + static_cast<std::ptrdiff_t>(PARTITION)));

    if (layout == Layout::HeartGoldSoulSilver) {
        const auto beforeParsed = Gen4ReadOnlySave::parse(
            original, layout, gameId(layout, soulSilver));
        assert(beforeParsed);
        for (size_t box = 0; box < 18; ++box) {
            const auto beforePad = beforeParsed->hgssBoxPadding(box);
            const auto afterPad = parsed->hgssBoxPadding(box);
            assert(beforePad.size() == 0x10 && afterPad.size() == 0x10);
            assert(std::equal(beforePad.begin(), beforePad.end(), afterPad.begin()));
        }
    }

    editor->discard();
    assert(!editor->hasChanges());
    assert(editor->finalizedBytes(&error) == original);
}

void testMixedPartitionMutationFootprint() {
    for (const auto layout : {Layout::DiamondPearl, Layout::Platinum,
                              Layout::HeartGoldSoulSilver}) {
        const uint8_t romCode = layout == Layout::HeartGoldSoulSilver ? 7 : 7;
        const auto sp = spec(layout);
        for (int generalPart = 0; generalPart < 2; ++generalPart) {
            for (int storagePart = 0; storagePart < 2; ++storagePart) {
                auto source = makeSave(layout, romCode, generalPart, storagePart);
                const auto original = source;
                std::string error;
                auto editor = Gen4StagedPokemonEditor::create(
                    source, layout, gameId(layout), &error);
                assert(editor && error.empty());

                auto party = editor->editablePartyPokemon(0, &error);
                assert(party && error.empty());
                assert(party->setFriendship(211));
                assert(editor->commitPartyPokemon(0, *party, &error) && error.empty());

                auto boxed = editor->editableBoxPokemon(0, 0, &error);
                assert(boxed && error.empty());
                assert(boxed->setFriendship(199));
                assert(editor->commitBoxPokemon(0, 0, *boxed, &error) && error.empty());

                const auto finalBytes = editor->finalizedBytes(&error);
                assert(!finalBytes.empty() && error.empty());
                assert(source == original);

                const size_t generalBase = static_cast<size_t>(generalPart) * PARTITION;
                const size_t partyBegin = generalBase + sp.party;
                const size_t partyEnd = partyBegin + Encryption::SIZE_PARTY4;
                const size_t generalCrcBegin = generalBase + sp.generalSize - 2;
                const size_t generalCrcEnd = generalCrcBegin + 2;

                const size_t storageBase = static_cast<size_t>(storagePart) * PARTITION + sp.storageStart;
                const size_t boxBegin = storageBase + sp.boxData;
                const size_t boxEnd = boxBegin + Encryption::SIZE_STORED4;
                const size_t storageCrcBegin = storageBase + sp.storageSize - 2;
                const size_t storageCrcEnd = storageCrcBegin + 2;

                bool partyChanged = false;
                bool boxChanged = false;
                for (size_t i = 0; i < finalBytes.size(); ++i) {
                    if (finalBytes[i] == original[i]) continue;
                    const bool inParty = i >= partyBegin && i < partyEnd;
                    const bool inGeneralCrc = i >= generalCrcBegin && i < generalCrcEnd;
                    const bool inBox = i >= boxBegin && i < boxEnd;
                    const bool inStorageCrc = i >= storageCrcBegin && i < storageCrcEnd;
                    assert(inParty || inGeneralCrc || inBox || inStorageCrc);
                    partyChanged = partyChanged || inParty;
                    boxChanged = boxChanged || inBox;
                }
                assert(partyChanged && boxChanged);

                auto reparsed = Gen4ReadOnlySave::parse(
                    finalBytes, layout, gameId(layout), &error);
                assert(reparsed && error.empty());
                assert(reparsed->generalSelection().partition == generalPart);
                assert(reparsed->storageSelection().partition == storagePart);
                assert(reparsed->nativePartySlots()[0].friendship() == 211);
                assert(reparsed->box(0, 0).friendship() == 199);
            }
        }
    }
}

void testEmptySlotCreateTransaction() {
    for (const auto layout : {Layout::DiamondPearl, Layout::Platinum,
                              Layout::HeartGoldSoulSilver}) {
        auto source = makeSave(layout, 7);
        const auto original = source;
        std::string error;
        auto editor = Gen4StagedPokemonEditor::create(
            source, layout, gameId(layout), &error);
        assert(editor && error.empty());

        auto draft = editor->createBoxDraft(0, 1, 393, &error);
        assert(draft && error.empty());
        assert(draft->species() == 393);
        assert(draft->tid() == 12150 && draft->sid() == 22558);
        assert(draft->language() == 2);
        assert(editor->stageCreateBoxPokemon(0, 1, *draft, &error));
        assert(error.empty());
        assert(source == original);

        auto created = editor->boxedPokemon(0, 1, &error);
        assert(created && error.empty());
        assert(created->species() == 393);
        assert(created->tid() == 12150 && created->sid() == 22558);
        assert(created->originalTrainerName() == u"Kylie");
        assert(created->originalEncryptedBytes() == draft->encryptedBytes());

        // Create must never replace an occupied target.
        const auto stagedBeforeRefusal = editor->stagedBytes();
        assert(!editor->createBoxDraft(0, 0, 393, &error));
        assert(!error.empty());
        error.clear();
        assert(!editor->stageCreateBoxPokemon(0, 0, *draft, &error));
        assert(!error.empty());
        assert(editor->stagedBytes() == stagedBeforeRefusal);

        const auto finalBytes = editor->finalizedBytes(&error);
        assert(!finalBytes.empty() && error.empty());
        auto reparsed = Gen4ReadOnlySave::parse(
            finalBytes, layout, gameId(layout), &error);
        assert(reparsed && error.empty());
        assert(!reparsed->box(0, 1).empty());
        assert(source == original);
    }
}

void testNoOpAndFailureRollback() {
    auto source = makeSave(Layout::Platinum);
    auto editor = Gen4StagedPokemonEditor::create(
        source, Layout::Platinum, "platinum_nds");
    assert(editor);
    auto mon = editor->editableBoxPokemon(0, 0);
    assert(mon);
    assert(editor->commitBoxPokemon(0, 0, *mon));
    assert(!editor->hasChanges());
    auto party = editor->editablePartyPokemon(0);
    assert(party && party->isParty());
    assert(editor->commitPartyPokemon(0, *party));
    assert(!editor->hasChanges());

    auto corrupt = mon->encryptedBytes();
    corrupt[0x20] ^= std::byte{1};
    Pokemon::Pokemon4ReadOnly invalid(corrupt, Enums::GameVersion::PT);
    assert(!invalid.valid());

    // Empty/out-of-range slots fail without touching staged bytes.
    const auto before = editor->stagedBytes();
    std::string error;
    assert(!editor->editableBoxPokemon(99, 0, &error));
    assert(editor->stagedBytes() == before);
}

void testShedinjaPartyHpRule() {
    std::string error;
    auto shedinja = Pokemon::Pokemon4Mutable::fromEncrypted(
        occupiedPartyPk4(292), Enums::GameVersion::PT, &error);
    assert(shedinja && error.empty() && shedinja->isParty());

    // Shedinja is the hard Gen III+ HP-formula exception. Stat-affecting Party edits
    // must never turn it into an ordinary calculated-HP Pokémon.
    assert(shedinja->setLevel(50));
    assert(shedinja->setIV(0, 31));
    assert(shedinja->setEV(0, 252));

    const auto encrypted = shedinja->encryptedBytes();
    Pokemon::Pokemon4ReadOnly reparsed(encrypted, Enums::GameVersion::PT);
    assert(reparsed.valid() && reparsed.isParty());
    assert(reparsed.partyLevel() == 50);
    assert(reparsed.maxHP() == 1);
    assert(reparsed.currentHP() == 1);
}

void testRecoveredAndMismatchRemainReadOnly() {
    auto recovered = makeSave(Layout::DiamondPearl);
    // Partition 0 is nominally newest. Break its General payload without restamping:
    // strict parser falls back to the older valid General and marks recovery state.
    recovered[1] ^= 1;
    std::string error;
    assert(!Gen4StagedPokemonEditor::create(
        recovered, Layout::DiamondPearl, "diamond_nds", &error));
    assert(error.find("Recovered older-copy") != std::string::npos);

    auto hg = makeSave(Layout::HeartGoldSoulSilver, 7);
    error.clear();
    assert(!Gen4StagedPokemonEditor::create(
        hg, Layout::HeartGoldSoulSilver, "soulsilver_nds", &error));
    assert(error.find("does not match") != std::string::npos);
}

} // namespace

int main() {
    testLayout(Layout::DiamondPearl);
    testLayout(Layout::Platinum);
    testLayout(Layout::HeartGoldSoulSilver, 7, false);
    testLayout(Layout::HeartGoldSoulSilver, 8, true);
    testMixedPartitionMutationFootprint();
    testEmptySlotCreateTransaction();
    testNoOpAndFailureRollback();
    testShedinjaPartyHpRule();
    testRecoveredAndMismatchRemainReadOnly();
    std::cout << "Gen IV staged boxed Pokemon editor PASS\n";
    return 0;
}
