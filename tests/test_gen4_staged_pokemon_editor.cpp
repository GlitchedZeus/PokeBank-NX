#include "Encryption/Encryption4.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Integration/Gen4/Gen4BagCatalog.h"
#include "Integration/Gen4/Gen4ExportTransaction.h"
#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Integration/Gen4/Gen4StagedTrainerEditor.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Utils/CRC16.h"
#include "Utils/Gen4TextCodec.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <vector>
#include <unistd.h>

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
    assert(mon->setMove(0, 105)); // Recover
    assert(mon->pp()[0] == 10 && mon->ppUps()[0] == 0);
    assert(editor->commitBoxPokemon(0, 0, *mon, &error));
    assert(error.empty());
    assert(editor->hasChanges());
    assert(source == original);
    const auto fields=PokeVault::Integration::Gen4::describeStagedReviewChanges(*editor);
    assert(fields.size()>4); // Every field remains reachable across Review pages.
    assert(std::any_of(fields.begin(),fields.end(),[](const auto& row) {
        return row.first=="Box 1 / Slot 1 / Move 1" && row.second.ends_with(" -> Recover");
    }));
    assert(std::any_of(fields.begin(),fields.end(),[](const auto& row) {
        return row.first=="Box 1 / Slot 1 / IV HP" && row.second.ends_with(" -> 31");
    }));
    assert(std::any_of(fields.begin(),fields.end(),[](const auto& row) {
        return row.first=="Box 1 / Slot 1 / EV Atk" && row.second.ends_with(" -> 200");
    }));

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
    assert(parsed->box(0, 0).moves()[0] == 105);
    assert(parsed->box(0, 0).pp()[0] == 10);
    assert(parsed->box(0, 0).ppUps()[0] == 0);

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
        assert(created->nickname() == u"PIPLUP");
        assert(!created->isNicknamed());
        const auto createdBytes = created->originalEncryptedBytes();
        const auto draftBytes = draft->encryptedBytes();
        assert(createdBytes.size() == draftBytes.size());
        assert(std::equal(createdBytes.begin(), createdBytes.end(), draftBytes.begin()));

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

void testCloneAndReleaseTransactions() {
    for (const auto layout : {Layout::DiamondPearl, Layout::Platinum,
                              Layout::HeartGoldSoulSilver}) {
        auto source = makeSave(layout);
        const auto original = source;
        std::string error;
        auto editor = Gen4StagedPokemonEditor::create(
            source, layout, gameId(layout), &error);
        assert(editor && error.empty());

        const auto sourceRecord = editor->boxedPokemon(0, 0, &error);
        assert(sourceRecord && error.empty());
        const auto sourceBytes = sourceRecord->originalEncryptedBytes();

        assert(editor->stageCloneBoxPokemon(0, 0, 0, 1, &error));
        assert(error.empty());
        assert(editor->hasChanges());
        assert(source == original);

        const auto clone = editor->boxedPokemon(0, 1, &error);
        assert(clone && error.empty());
        assert(std::equal(clone->originalEncryptedBytes().begin(),
                          clone->originalEncryptedBytes().end(), sourceBytes.begin()));

        // Clone cannot silently overwrite an occupied slot.
        const auto beforeRefusal = editor->stagedBytes();
        assert(!editor->stageCloneBoxPokemon(0, 0, 0, 1, &error));
        assert(!error.empty());
        assert(editor->stagedBytes() == beforeRefusal);
        error.clear();

        // Release zeroes only the staged stored-PK4 slot and leaves the clone intact.
        assert(editor->stageReleaseBoxPokemon(0, 0, &error));
        assert(error.empty());
        assert(!editor->boxedPokemon(0, 0, &error));
        error.clear();
        const auto cloneAfterRelease = editor->boxedPokemon(0, 1, &error);
        assert(cloneAfterRelease && error.empty());

        const auto finalBytes = editor->finalizedBytes(&error);
        assert(!finalBytes.empty() && error.empty());
        const auto reparsed = Gen4ReadOnlySave::parse(
            finalBytes, layout, gameId(layout), &error);
        assert(reparsed && error.empty());
        assert(reparsed->box(0, 0).valid() && reparsed->box(0, 0).empty());
        assert(reparsed->box(0, 1).valid() && !reparsed->box(0, 1).empty());
        assert(source == original);

        editor->discard();
        assert(!editor->hasChanges());
        assert(editor->finalizedBytes(&error) == original);
    }
}

void testMoveSwapAndCrossBoxTransactions() {
    auto original = makeSave(Layout::Platinum);
    auto editor = Gen4StagedPokemonEditor::create(original, Layout::Platinum, "platinum_nds");
    assert(editor);
    std::string error;
    const auto first = editor->boxedPokemon(0, 0, &error);
    assert(first && error.empty());
    const auto firstBytes = first->originalEncryptedBytes();
    const auto untouched = editor->stagedBytes();

    assert(editor->stageMoveBoxPokemon(0, 0, 0, 1, &error) && error.empty());
    assert(!editor->boxedPokemon(0, 0, &error));
    const auto moved = editor->boxedPokemon(0, 1, &error);
    assert(moved && std::equal(moved->originalEncryptedBytes().begin(),
                               moved->originalEncryptedBytes().end(), firstBytes.begin()));
    assert(editor->originalBytes() == original);
    const auto postMove = editor->stagedBytes();
    {
        // S04 follow-up: Review lists a staged Move as ONE line, not "X -> (empty)" plus
        // "(empty) -> X". The staged bytes are the same two-slot transaction.
        const auto lines = PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor);
        assert(lines.size() == 1);
        assert(lines[0].rfind("Moved ", 0) == 0);
        assert(PokeVault::Integration::Gen4::describeStagedReviewChanges(*editor).size()==1);
        assert(lines[0].find(": Box 1 slot 1 -> Box 1 slot 2") != std::string::npos);
    }
    assert(!editor->stageMoveBoxPokemon(0, 0, 1, 0, &error));
    assert(editor->stagedBytes() == postMove);
    assert(!editor->stageMoveBoxPokemon(0, 1, 18, 0, &error));
    assert(editor->stagedBytes() == postMove);
    error.clear();

    // Populate another slot using the already-validated native Create primitive,
    // then prove occupied-destination swap retains BOTH complete PK4 records.
    auto draft = editor->createBoxDraft(0, 2, 25, &error);
    assert(draft && error.empty());
    assert(editor->stageCreateBoxPokemon(0, 2, *draft, &error));
    const auto other = editor->boxedPokemon(0, 2, &error);
    assert(other);
    const auto otherBytes = other->originalEncryptedBytes();
    assert(editor->stageMoveBoxPokemon(0, 1, 0, 2, &error) && error.empty());
    const auto a = editor->boxedPokemon(0, 1, &error);
    const auto b = editor->boxedPokemon(0, 2, &error);
    assert(a && b);
    assert(std::equal(a->originalEncryptedBytes().begin(), a->originalEncryptedBytes().end(), otherBytes.begin()));
    assert(std::equal(b->originalEncryptedBytes().begin(), b->originalEncryptedBytes().end(), firstBytes.begin()));

    // Cross-box move and cancellation/discard leave original source immutable.
    assert(editor->stageMoveBoxPokemon(0, 2, 1, 0, &error) && error.empty());
    assert(!editor->boxedPokemon(0, 2, &error));
    assert(editor->boxedPokemon(1, 0, &error));
    {
        // Net effect against the ORIGINAL: Box 1/1's Pokemon now lives in Box 2/1 (one Moved
        // line, destination before the source in slot order is still paired), and the created
        // Pokemon sits in Box 1/2 (one plain line).
        const auto lines = PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor);
        assert(lines.size() == 2);
        const auto movedLine = std::find_if(lines.begin(), lines.end(), [](const std::string& l) {
            return l.rfind("Moved ", 0) == 0;
        });
        assert(movedLine != lines.end());
        assert(movedLine->find(": Box 1 slot 1 -> Box 2 slot 1") != std::string::npos);
    }
    assert(editor->originalBytes() == original);
    editor->discard();
    assert(editor->stagedBytes() == untouched);
    assert(!editor->hasChanges());
}

void testSharedHoldYGroupPlacementAtomicity() {
    // Gen IV now uses the existing Gen I-III Y-hold shared carrier. Unlike
    // the packed generations, native PK4 slots are sparse: a carried group
    // must be prepared on a detached staged editor and published all at once.
    auto original = makeSave(Layout::Platinum);
    auto editor = Gen4StagedPokemonEditor::create(
        original, Layout::Platinum, "platinum_nds");
    assert(editor);
    std::string error;
    auto secondDraft = editor->createBoxDraft(0, 1, 4, &error);
    assert(secondDraft && error.empty());
    assert(editor->stageCreateBoxPokemon(0, 1, *secondDraft, &error));
    const auto baseline = editor->stagedBytes();
    const auto first = editor->boxedPokemon(0, 0, &error);
    const auto second = editor->boxedPokemon(0, 1, &error);
    assert(first && second);
    const auto firstBytes = first->originalEncryptedBytes();
    const auto secondBytes = second->originalEncryptedBytes();

    // First placement on the candidate succeeds; if the second placement
    // fails, throwing away the candidate must preserve every baseline byte.
    {
        auto candidate = *editor;
        assert(candidate.stageMoveBoxPokemon(0, 0, 1, 0, &error));
        assert(!candidate.stageMoveBoxPokemon(0, 1, 18, 1, &error));
        assert(editor->stagedBytes() == baseline);
        assert(editor->boxedPokemon(0, 0, &error));
        assert(editor->boxedPokemon(0, 1, &error));
    }

    // Valid nonoverlapping group: preserve the exact encrypted PK4 payloads,
    // leave the original emulator image untouched, and make both old source
    // slots empty. This models a single published shared-overlay Y drop.
    {
        auto candidate = *editor;
        assert(candidate.stageMoveBoxPokemon(0, 0, 1, 0, &error));
        assert(candidate.stageMoveBoxPokemon(0, 1, 1, 1, &error));
        *editor = std::move(candidate);
    }
    const auto movedFirst = editor->boxedPokemon(1, 0, &error);
    const auto movedSecond = editor->boxedPokemon(1, 1, &error);
    assert(movedFirst && movedSecond);
    assert(std::equal(firstBytes.begin(), firstBytes.end(),
                      movedFirst->originalEncryptedBytes().begin()));
    assert(std::equal(secondBytes.begin(), secondBytes.end(),
                      movedSecond->originalEncryptedBytes().begin()));
    assert(!editor->boxedPokemon(0, 0, &error));
    assert(!editor->boxedPokemon(0, 1, &error));
    assert(editor->originalBytes() == original);

    editor->discard();
    assert(editor->stagedBytes() == original);
    assert(!editor->hasChanges());
}

void testGen4BagQuantityBoundsAndSourceImmutability() {
    // Run against three strict dual-partition retail-layout fixtures, not
    // only an isolated presenter/source-code string contract. Exercise the
    // same backend called after numeric-keypad confirmation.
    namespace Bag = PokeVault::Integration::Gen4;
    const size_t items = static_cast<size_t>(Bag::BagPocket::Items);
    const size_t keyItems = static_cast<size_t>(Bag::BagPocket::KeyItems);
    const size_t machines = static_cast<size_t>(Bag::BagPocket::Machines);
    for (const auto layout : {Layout::DiamondPearl, Layout::Platinum,
                              Layout::HeartGoldSoulSilver}) {
        const auto original = makeSave(layout);
        std::string error;
        auto editor = Gen4StagedPokemonEditor::create(
            original, layout, gameId(layout), &error);
        assert(editor && error.empty());
        assert(Bag::gen4BagItemAllowed(layout, items, 68));
        assert(Bag::gen4BagItemAllowed(layout, keyItems, 434));
        assert(Bag::gen4BagItemAllowed(layout, machines, 328));
        assert(Bag::gen4BagItemAllowed(layout, machines, 420));
        assert(Bag::gen4BagMaxQuantity(items, 68) == 999);
        assert(Bag::gen4BagMaxQuantity(keyItems, 434) == 1);
        assert(Bag::gen4BagMaxQuantity(machines, 328) == 99);
        assert(Bag::gen4BagMaxQuantity(machines, 420) == 1);

        assert(editor->stageBagAdd(items, 68, 777, &error) && error.empty());
        assert(editor->stageBagAdd(machines, 328, 42, &error) && error.empty());
        assert(editor->stageBagAdd(machines, 420, 1, &error) && error.empty());
        assert(editor->stageBagAdd(keyItems, 434, 1, &error) && error.empty());

        // Invalid keyboard-style inputs are byte-for-byte inert. A count of
        // zero must never be a disguised Delete action.
        const auto prior = editor->stagedBytes();
        assert(!editor->stageBagAdd(items, 69, 0, &error));
        assert(!editor->stageBagAdd(machines, 329, 100, &error));
        assert(!editor->stageBagAdd(machines, 421, 2, &error));
        assert(!editor->stageBagAdd(keyItems, 435, 2, &error));
        assert(!editor->stageBagQuantity(items, 0, 0, &error));
        assert(!editor->stageBagQuantity(machines, 0, 100, &error));
        assert(!editor->stageBagQuantity(machines, 1, 2, &error));
        assert(!editor->stageBagQuantity(keyItems, 0, 2, &error));
        assert(editor->stagedBytes() == prior);
        error.clear();

        // Valid edits change ONLY app-owned staging and reparse into exact
        // native items; the original dual-partition emulator save is intact.
        assert(editor->stageBagQuantity(items, 0, 900, &error) && error.empty());
        assert(editor->stageBagQuantity(machines, 0, 99, &error) && error.empty());
        assert(editor->stageBagQuantity(machines, 1, 1, &error) && error.empty());
        const auto finished = editor->finalizedBytes(&error);
        assert(!finished.empty() && error.empty());
        const auto parsed = Gen4ReadOnlySave::parse(
            finished, layout, gameId(layout), &error);
        assert(parsed && error.empty());
        const auto bag = Bag::decodeReadOnlyBag(*parsed);
        assert(bag);
        assert((*bag)[items].size() == 1 && (*bag)[items][0].itemId == 68 &&
               (*bag)[items][0].count == 900);
        assert((*bag)[machines].size() == 2);
        assert((*bag)[machines][0].itemId == 328 &&
               (*bag)[machines][0].count == 99);
        assert((*bag)[machines][1].itemId == 420 &&
               (*bag)[machines][1].count == 1);
        assert((*bag)[keyItems].size() == 1 &&
               (*bag)[keyItems][0].count == 1);
        assert(editor->originalBytes() == original);
        // Bag-only changes are listed for Review and the manifest (one line per changed item).
        const auto bagLines = PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor);
        assert(bagLines.size() == 4);
        const auto hasLine = [&](const std::string& text) {
            return std::find(bagLines.begin(), bagLines.end(), text) != bagLines.end();
        };
        assert(hasLine("Bag Items: Black Flute 0 -> 900"));
        assert(hasLine("Bag TM/HM: TM01 — Focus Punch 0 -> 99"));
        assert(hasLine("Bag TM/HM: HM01 — Cut 0 -> 1"));
        assert(hasLine("Bag Key Items: Sticker Case 0 -> 1"));
        editor->discard();
        assert(editor->stagedBytes() == original);
        assert(PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor).empty());
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

std::vector<uint8_t> slurp(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(in), {});
}

void testVerifiedWorkingCopyExport() {
    namespace fs = std::filesystem;
    using PokeVault::Integration::Common::ExportFailurePoint;
    const fs::path root = fs::temp_directory_path() /
        ("pokebank-g4-export-" + std::to_string(::getpid()));
    fs::remove_all(root);
    for (const auto layout : {Layout::DiamondPearl, Layout::Platinum,
                              Layout::HeartGoldSoulSilver}) {
        auto source = makeSave(layout);
        const auto original = source;
        std::string error;
        auto editor = Gen4StagedPokemonEditor::create(source, layout, gameId(layout), &error);
        assert(editor && error.empty());

        PokeVault::Integration::Gen4::Gen4ExportRequest request;
        request.rootDirectory = root.string();
        request.directoryStem = gameId(layout) + "_20261014-120000";
        request.sourcePath = "sdmc:/retroarch/saves/fixture.sav";
        request.timestamp = "20261014-120000";

        // Nothing staged: refused before any filesystem work.
        auto none = PokeVault::Integration::Gen4::publishVerifiedGen4Export(*editor, request);
        assert(!none.success && none.error.find("No pending") != std::string::npos);
        assert(!fs::exists(root / request.directoryStem));

        assert(editor->stageCloneBoxPokemon(0, 0, 0, 1, &error));
        const auto lines = PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor);
        assert(lines.size() == 1 && lines[0].rfind("Box 1 slot 2: (empty) -> ", 0) == 0);

        // Every injected failure leaves no published or temporary directory behind.
        for (auto point : {ExportFailurePoint::AfterBackupWrite, ExportFailurePoint::AfterEditedWrite,
                           ExportFailurePoint::AfterReadbackVerify,
                           ExportFailurePoint::AfterManifestWrite}) {
            auto faulty = request;
            faulty.failurePoint = point;
            auto failed = PokeVault::Integration::Gen4::publishVerifiedGen4Export(*editor, faulty);
            assert(!failed.success && failed.directory.empty());
            assert(!fs::exists(root / request.directoryStem));
            for (const auto& entry : fs::directory_iterator(root))
                assert(entry.path().filename().string().find(".tmp") == std::string::npos);
        }

        auto exported = PokeVault::Integration::Gen4::publishVerifiedGen4Export(*editor, request);
        assert(exported.success && exported.error.empty());
        assert(exported.directory == (root / request.directoryStem).string());
        assert(exported.changedBytes > 0);
        const fs::path dir(exported.directory);
        assert(slurp(dir / "original_backup.sav") == original);
        const auto edited = slurp(dir / "edited.sav");
        assert(edited == editor->stagedBytes() && edited != original);
        const auto reparsed = Gen4ReadOnlySave::parse(edited, layout, gameId(layout), &error);
        assert(reparsed && !reparsed->box(0, 1).empty());
        std::ifstream manifestIn(dir / "EDIT_MANIFEST.txt");
        const std::string manifest((std::istreambuf_iterator<char>(manifestIn)), {});
        assert(manifest.find("GAME_ID=" + gameId(layout)) != std::string::npos);
        assert(manifest.find("SOURCE_SHA256=" + exported.originalSha256) != std::string::npos);
        // No Open snapshot: the backup is the payload and no container facts are claimed.
        assert(manifest.find("SOURCE_CONTAINER=") == std::string::npos);
        assert(manifest.find("ORIGINAL_BACKUP_IS=") == std::string::npos);
        assert(manifest.find("EDITED_SHA256=" + exported.editedSha256) != std::string::npos);
        assert(manifest.find("STRICT_REPARSE=VERIFIED") != std::string::npos);
        assert(manifest.find("- Box 1 slot 2: (empty) -> ") != std::string::npos);

        // A second export never overwrites the first; the source bytes never changed.
        auto again = PokeVault::Integration::Gen4::publishVerifiedGen4Export(*editor, request);
        assert(again.success && again.directory == exported.directory + "-2");
        assert(source == original);
    }
    fs::remove_all(root);
}

// Phase 4: Party and Box edits exported SEPARATELY through the real transaction. Each export's
// edited bytes differ from the source only inside that record and its own block CRC (the other
// block is byte-identical), the review lines name only that surface, racing destinations and a
// mid-publication failure leave no residue, and malformed inputs never open a working copy.
void testPhase4PartyBoxExportSeparation() {
    namespace fs = std::filesystem;
    using PokeVault::Integration::Common::ExportFailurePoint;
    const fs::path root = fs::temp_directory_path() /
        ("pokebank-g4-phase4-" + std::to_string(::getpid()));
    fs::remove_all(root);
    fs::create_directories(root);
    auto entries = [&] {
        return static_cast<size_t>(std::distance(fs::directory_iterator(root), fs::directory_iterator()));
    };
    for (const auto layout : {Layout::DiamondPearl, Layout::Platinum, Layout::HeartGoldSoulSilver}) {
        const auto sp = spec(layout);
        const auto original = makeSave(layout);
        // makeSave stamps partition 0 newest for both blocks.
        const size_t partyBegin = sp.party, partyEnd = partyBegin + Encryption::SIZE_PARTY4;
        const size_t generalCrc = sp.generalSize - 2;
        const size_t boxBegin = sp.storageStart + sp.boxData;
        const size_t boxEnd = boxBegin + Encryption::SIZE_STORED4;
        const size_t storageCrc = sp.storageStart + sp.storageSize - 2;

        for (const bool partyEdit : {true, false}) {
            auto source = original;
            std::string error;
            auto editor = Gen4StagedPokemonEditor::create(source, layout, gameId(layout), &error);
            assert(editor && error.empty());
            if (partyEdit) {
                auto party = editor->editablePartyPokemon(0, &error);
                assert(party && party->setFriendship(201));
                assert(editor->commitPartyPokemon(0, *party, &error));
            } else {
                auto boxed = editor->editableBoxPokemon(0, 0, &error);
                assert(boxed && boxed->setFriendship(202));
                assert(editor->commitBoxPokemon(0, 0, *boxed, &error));
            }
            const auto lines = PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor);
            assert(lines.size() == 1);
            assert(lines[0].rfind(partyEdit ? "Party slot 1: " : "Box 1 slot 1: ", 0) == 0);

            PokeVault::Integration::Gen4::Gen4ExportRequest request;
            request.rootDirectory = root.string();
            request.directoryStem = gameId(layout) + (partyEdit ? "_party" : "_box");
            request.sourcePath = "sdmc:/drastic/backup/fixture.dsv";
            request.timestamp = "20261015-120000";
            for (auto point : {ExportFailurePoint::AfterFirstPublishedFile,
                               ExportFailurePoint::DestinationAppearsAsEmptyDirectory,
                               ExportFailurePoint::DestinationAppearsAsFile}) {
                auto faulty = request;
                faulty.failurePoint = point;
                auto failed = PokeVault::Integration::Gen4::publishVerifiedGen4Export(*editor, faulty);
                assert(!failed.success && failed.directory.empty());
                const fs::path raced = root / request.directoryStem;
                if (point == ExportFailurePoint::DestinationAppearsAsEmptyDirectory) {
                    assert(fs::is_directory(raced) && fs::is_empty(raced));
                    fs::remove(raced);
                } else if (point == ExportFailurePoint::DestinationAppearsAsFile) {
                    assert(fs::is_regular_file(raced) && fs::file_size(raced) == 0);
                    fs::remove(raced);
                }
                assert(entries() == 0);
            }

            auto exported = PokeVault::Integration::Gen4::publishVerifiedGen4Export(*editor, request);
            assert(exported.success);
            const fs::path dir(exported.directory);
            assert(slurp(dir / "original_backup.sav") == original);
            const auto edited = slurp(dir / "edited.sav");
            assert(edited.size() == original.size());
            bool recordChanged = false;
            for (size_t i = 0; i < edited.size(); ++i) {
                if (edited[i] == original[i]) continue;
                if (partyEdit) {
                    assert((i >= partyBegin && i < partyEnd) || i == generalCrc || i == generalCrc + 1);
                    recordChanged = recordChanged || (i >= partyBegin && i < partyEnd);
                } else {
                    assert((i >= boxBegin && i < boxEnd) || i == storageCrc || i == storageCrc + 1);
                    recordChanged = recordChanged || (i >= boxBegin && i < boxEnd);
                }
            }
            assert(recordChanged);
            auto reparsed = Gen4ReadOnlySave::parse(edited, layout, gameId(layout), &error);
            assert(reparsed && !reparsed->recovered());
            assert(reparsed->nativePartySlots()[0].friendship() == (partyEdit ? 201 : 70));
            assert(reparsed->box(0, 0).friendship() == (partyEdit ? 70 : 202));
            std::ifstream manifestIn(dir / "EDIT_MANIFEST.txt");
            const std::string manifest((std::istreambuf_iterator<char>(manifestIn)), {});
            assert(manifest.find(partyEdit ? "- Party slot 1: " : "- Box 1 slot 1: ") != std::string::npos);
            assert(manifest.find(partyEdit ? "- Box 1 slot 1: " : "- Party slot 1: ") == std::string::npos);
            assert(source == original);
            fs::remove_all(dir);
            assert(entries() == 0);
        }

        // Malformed, truncated, oversized and doubly-corrupt inputs never open a working copy.
        std::string error;
        auto truncated = original;
        truncated.resize(original.size() - 1);
        assert(!Gen4StagedPokemonEditor::create(truncated, layout, gameId(layout), &error));
        auto oversized = original;
        oversized.push_back(0);
        assert(!Gen4StagedPokemonEditor::create(oversized, layout, gameId(layout), &error));
        assert(!Gen4StagedPokemonEditor::create(std::vector<uint8_t>{}, layout, gameId(layout), &error));
        auto bothBroken = original;
        bothBroken[sp.storageStart + sp.boxData] ^= 1;
        bothBroken[PARTITION + sp.storageStart + sp.boxData] ^= 1;
        assert(!Gen4StagedPokemonEditor::create(bothBroken, layout, gameId(layout), &error));
        // The wrong exact game for these bytes is a mismatch, not a working copy.
        const auto wrongLayout = layout == Layout::Platinum ? Layout::DiamondPearl : Layout::Platinum;
        assert(!Gen4StagedPokemonEditor::create(original, wrongLayout, gameId(wrongLayout), &error));
    }
    fs::remove_all(root);
}

// Review lines describe Pokemon changes only: a Pokemon created and then released in this session
// leaves a differently-encoded empty slot, which must not appear as "(empty) (edited)"; a real
// release still reads "<species> -> (empty)".
void testReviewLinesSkipEmptyToEmpty() {
    for (const auto layout : {Layout::DiamondPearl, Layout::Platinum, Layout::HeartGoldSoulSilver}) {
        auto source = makeSave(layout);
        std::string error;
        auto editor = Gen4StagedPokemonEditor::create(source, layout, gameId(layout), &error);
        assert(editor);
        auto draft = editor->createBoxDraft(0, 1, 393, &error);
        assert(draft && editor->stageCreateBoxPokemon(0, 1, *draft, &error));
        assert(editor->stageReleaseBoxPokemon(0, 1, &error));
        for (const auto& line : PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor))
            assert(line.find("(empty) (edited)") == std::string::npos);
        assert(PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor).empty());
        assert(editor->stageReleaseBoxPokemon(0, 0, &error));
        const auto lines = PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor);
        assert(lines.size() == 1 && lines[0].rfind("Box 1 slot 1: ", 0) == 0 &&
               lines[0].find("-> (empty)") != std::string::npos);
    }
}

// QA-F8: a slot that started empty holds an encrypted blank in retail saves. Releasing a clone
// placed there restores those original bytes, so the net no-op leaves the working copy (and any
// export) byte-identical to the source and no review line; releasing a source Pokemon still
// clears its slot.
void testReleaseRestoresOriginalEmptyRecord() {
    for (const auto layout : {Layout::DiamondPearl, Layout::Platinum, Layout::HeartGoldSoulSilver}) {
        const auto source = makeSave(layout);
        std::string error;
        auto editor = Gen4StagedPokemonEditor::create(source, layout, gameId(layout), &error);
        assert(editor);
        assert(editor->stageCloneBoxPokemon(0, 0, 1, 0, &error));
        assert(editor->hasChanges());
        assert(editor->stageReleaseBoxPokemon(1, 0, &error));
        assert(!editor->hasChanges());
        assert(editor->stagedBytes() == source);
        assert(PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor).empty());
        const auto finalized = editor->finalizedBytes(&error);
        assert(finalized == source);

        // The original Pokemon's slot was never empty: Release still empties it.
        assert(editor->stageReleaseBoxPokemon(0, 0, &error));
        assert(editor->hasChanges());
        const auto reparsed = Gen4ReadOnlySave::parse(editor->stagedBytes(), layout, gameId(layout), &error);
        assert(reparsed && reparsed->box(0, 0).valid() && reparsed->box(0, 0).empty());
        assert(PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor).size() == 1);
    }
}

// A synthetic DeSmuME .dsv container: payload, then the documented 40-byte footer (six u32 values
// and the 16-byte "|-DESMUME SAVE-|" cookie). Values are fixture data, not a real save.
std::vector<uint8_t> dsvContainer(const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> file = payload;
    const uint32_t words[6] = {static_cast<uint32_t>(payload.size()), static_cast<uint32_t>(payload.size()),
                               0, 0, 0, 0};
    for (uint32_t word : words)
        for (int i = 0; i < 4; ++i) file.push_back(static_cast<uint8_t>(word >> (8 * i)));
    const std::string cookie = "|-DESMUME SAVE-|";
    file.insert(file.end(), cookie.begin(), cookie.end());
    return file;
}

void spitBytes(const std::filesystem::path& path, const std::vector<uint8_t>& bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

// Gen IV export with an Open snapshot: a raw or .dsv source that is unchanged exports; one that
// changed after Open (content, size or mtime) is refused before anything is written, and the
// source file itself is never touched.
void testExportRefusesStaleSource() {
    namespace fs = std::filesystem;
    namespace G4 = PokeVault::Integration::Gen4;
    const fs::path root = fs::temp_directory_path() / ("pokebank-g4-stale-" + std::to_string(::getpid()));
    fs::remove_all(root);
    fs::create_directories(root / "src");
    for (const auto container : {G4::Gen4SourceContainer::Raw, G4::Gen4SourceContainer::Dsv}) {
        const bool dsv = container == G4::Gen4SourceContainer::Dsv;
        const auto payload = makeSave(Layout::Platinum);
        const auto fileBytes = dsv ? dsvContainer(payload) : payload;
        const fs::path sourcePath = root / "src" / (dsv ? "platinum.dsv" : "platinum.sav");
        spitBytes(sourcePath, fileBytes);
        const auto sourceTime = fs::last_write_time(sourcePath);
        std::string error;

        // Snapshot shape checks: the container must match the file, and the payload must lead it.
        const auto wrong = dsv ? G4::Gen4SourceContainer::Raw : G4::Gen4SourceContainer::Dsv;
        assert(!G4::snapshotGen4Source(sourcePath.string(), wrong, payload, &error) && !error.empty());
        auto otherPayload = payload;
        otherPayload[0x10] ^= 0xFF;
        assert(!G4::snapshotGen4Source(sourcePath.string(), container, otherPayload, &error));
        const auto snapshot = G4::snapshotGen4Source(sourcePath.string(), container, payload, &error);
        assert(snapshot && error.empty());
        assert(snapshot->fileSize == fileBytes.size() && snapshot->container == container);
        assert(std::string(G4::gen4SourceContainerLabel(container)) == (dsv ? "dsv-footer" : "raw-nds-battery"));
        assert(snapshot->fileSha256 == PokeVault::Integration::Common::sha256Hex(fileBytes));

        auto editor = Gen4StagedPokemonEditor::create(payload, Layout::Platinum, "platinum_nds", &error);
        assert(editor && editor->stageCloneBoxPokemon(0, 0, 0, 1, &error));

        G4::Gen4ExportRequest request;
        request.rootDirectory = (root / "out").string();
        request.sourcePath = sourcePath.string();
        request.timestamp = "20261015-120000";
        request.sourceSnapshot = snapshot;
        int attempt = 0;
        auto refused = [&](const std::vector<uint8_t>& replacement, bool restoreTime) {
            spitBytes(sourcePath, replacement);
            if (restoreTime) fs::last_write_time(sourcePath, sourceTime);
            request.directoryStem = std::string(dsv ? "dsv" : "raw") + "_stale_" + std::to_string(attempt++);
            const auto result = G4::publishVerifiedGen4Export(*editor, request);
            assert(!result.success && result.directory.empty());
            assert(result.error.find("reopen it before exporting") != std::string::npos ||
                   result.error.find("no longer begins") != std::string::npos);
            assert(!fs::exists(fs::path(request.rootDirectory) / request.directoryStem));
            assert(slurp(sourcePath) == replacement);
        };

        // Same size, one footer/payload byte different, mtime restored: the hash catches it.
        auto flipped = fileBytes;
        flipped.back() ^= 0x01;
        refused(flipped, true);
        // Truncated (a .dsv losing its footer, a raw file losing its tail).
        refused(std::vector<uint8_t>(fileBytes.begin(), fileBytes.end() - 40), true);
        // Identical bytes, but rewritten later: the mtime alone is enough to refuse.
        spitBytes(sourcePath, fileBytes);
        fs::last_write_time(sourcePath, sourceTime + std::chrono::seconds(5));
        request.directoryStem = std::string(dsv ? "dsv" : "raw") + "_touched";
        auto touched = G4::publishVerifiedGen4Export(*editor, request);
        assert(!touched.success && touched.error.find("changed after it was opened") != std::string::npos);
        // Gone entirely.
        fs::remove(sourcePath);
        request.directoryStem = std::string(dsv ? "dsv" : "raw") + "_missing";
        auto missing = G4::publishVerifiedGen4Export(*editor, request);
        assert(!missing.success && missing.error.find("missing or unreadable") != std::string::npos);

        // Restored exactly as opened: the export proceeds and the source stays byte-identical.
        spitBytes(sourcePath, fileBytes);
        fs::last_write_time(sourcePath, sourceTime);
        request.directoryStem = std::string(dsv ? "dsv" : "raw") + "_ok";
        const auto exported = G4::publishVerifiedGen4Export(*editor, request);
        assert(exported.success && exported.error.empty());
        const fs::path out(exported.directory);
        // The backup is the source FILE verbatim (footer included); edited.sav is the raw payload.
        const fs::path backupPath = out / (dsv ? "original_backup.dsv" : "original_backup.sav");
        assert(slurp(backupPath) == fileBytes);
        assert(!dsv || !fs::exists(out / "original_backup.sav"));
        assert(!fs::exists(out / "edited.dsv"));
        assert(slurp(out / "edited.sav") == editor->stagedBytes());
        assert(exported.backupSha256 == PokeVault::Integration::Common::sha256Hex(fileBytes));
        assert(exported.originalSha256 == PokeVault::Integration::Common::sha256Hex(payload));
        std::ifstream manifestIn(out / "EDIT_MANIFEST.txt");
        const std::string manifest((std::istreambuf_iterator<char>(manifestIn)), {});
        auto has = [&](const std::string& line) { return manifest.find(line + "\n") != std::string::npos; };
        assert(has(std::string("SOURCE_CONTAINER=") + (dsv ? "dsv-footer" : "raw-nds-battery")));
        assert(has("EDITED_CONTAINER=raw-nds-battery"));
        assert(has("SOURCE_SHA256_SCOPE=0x80000_SAV4_PAYLOAD"));
        assert(has("SOURCE_FILE_SIZE=" + std::to_string(fileBytes.size())));
        assert(has("SOURCE_FILE_SHA256=" + exported.backupSha256));
        assert(has("ORIGINAL_BACKUP_IS=VERBATIM_SOURCE_FILE"));
        assert(has("SOURCE_SIZE=" + std::to_string(payload.size())));
        assert(has(std::string("ORIGINAL_BACKUP=") + (dsv ? "original_backup.dsv" : "original_backup.sav")));
        assert(has("EDITED_SAVE=edited.sav"));
        assert(slurp(sourcePath) == fileBytes);
    }
    fs::remove_all(root);
}

// R2-BUG-002: Trainer Name / Money staged in the selected General block of every Gen IV game.
// Edit -> verified export -> strict reparse: the fields come back as entered, every byte outside
// the trainer name / money fields and the selected General CRC is identical (the other General
// copy and both Storage copies included), Review lists both changes, Discard restores the source
// image, and the source buffer itself never changes.
void testTrainerNameMoneyRoundTrip() {
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() /
        ("pokebank-g4-trainer-" + std::to_string(::getpid()));
    fs::remove_all(root);
    struct Case { Layout layout; uint8_t rom; const char* id; int newerGeneral; };
    const Case cases[] = {
        {Layout::DiamondPearl, 10, "diamond_nds", 0}, {Layout::DiamondPearl, 11, "pearl_nds", 1},
        {Layout::Platinum, 12, "platinum_nds", 1},
        {Layout::HeartGoldSoulSilver, 7, "heartgold_nds", 0},
        {Layout::HeartGoldSoulSilver, 8, "soulsilver_nds", 1},
    };
    for (const auto& c : cases) {
        auto source = makeSave(c.layout, c.rom, c.newerGeneral);
        const auto original = source;
        std::string error;
        auto editor = Gen4StagedPokemonEditor::create(source, c.layout, c.id, &error);
        assert(editor && error.empty());
        assert(editor->trainerNameEditable());
        const auto before = editor->stagedTrainer(&error);
        assert(before && before->name == u"Kylie" && before->money == 0);

        // Refusals change nothing.
        assert(!editor->stageTrainerMoney(1000000, &error) && !error.empty());
        assert(!editor->stageTrainerName(u"", &error));
        assert(!editor->stageTrainerName(u"Eightchr", &error));
        assert(!editor->stageTrainerName(u"A\U0001F600", &error));
        assert(editor->stagedBytes() == original && !editor->hasChanges());

        // Through the Trainer page's editor: only the changed fields are staged.
        PokeVault::Integration::Gen4::Gen4StagedTrainerEditor trainerEditor(*editor);
        assert(trainerEditor.available() && trainerEditor.trainerName() == "Kylie");
        assert(trainerEditor.stage("Kylie", 0, error) && !editor->hasChanges());
        assert(trainerEditor.stage("Ash", 123456, error) && error.empty());
        assert(trainerEditor.trainerName() == "Ash" && trainerEditor.money() == 123456);
        const auto after = editor->stagedTrainer(&error);
        assert(after && after->name == u"Ash" && after->money == 123456);
        assert(after->tid == before->tid && after->sid == before->sid && after->language == before->language);

        // Footprint: only the trainer name (16 bytes) and money (4 bytes) fields and the selected
        // General block's CRC differ.
        const auto sp = spec(c.layout);
        const size_t general = static_cast<size_t>(c.newerGeneral) * PARTITION;
        const size_t trainer = general + sp.trainer;
        const size_t crc = general + sp.generalSize - 2;
        const auto& staged = editor->stagedBytes();
        for (size_t i = 0; i < staged.size(); ++i) {
            if (staged[i] == original[i]) continue;
            const bool allowed = (i >= trainer && i < trainer + 16) ||
                                 (i >= trainer + 0x14 && i < trainer + 0x18) || i == crc || i == crc + 1;
            assert(allowed);
        }
        const size_t otherGeneral = static_cast<size_t>(1 - c.newerGeneral) * PARTITION;
        assert(std::equal(staged.begin() + static_cast<std::ptrdiff_t>(otherGeneral),
                          staged.begin() + static_cast<std::ptrdiff_t>(otherGeneral + sp.generalSize),
                          original.begin() + static_cast<std::ptrdiff_t>(otherGeneral)));
        // Name written PKHeX ClearZero style: "Ash", one terminator, the rest zero.
        assert(staged[trainer + 6] == 0xFF && staged[trainer + 7] == 0xFF);
        for (size_t i = trainer + 8; i < trainer + 16; ++i) assert(staged[i] == 0);

        const auto lines = PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor);
        assert(std::find(lines.begin(), lines.end(), "Trainer name: Kylie -> Ash") != lines.end());
        assert(std::find(lines.begin(), lines.end(), "Money: 0 -> 123456") != lines.end());

        PokeVault::Integration::Gen4::Gen4ExportRequest request;
        request.rootDirectory = root.string();
        request.directoryStem = std::string(c.id) + "_20261019-120000";
        request.sourcePath = "sdmc:/retroarch/saves/fixture.sav";
        request.timestamp = "20261019-120000";
        auto exported = PokeVault::Integration::Gen4::publishVerifiedGen4Export(*editor, request);
        assert(exported.success && exported.error.empty());
        const auto edited = slurp(fs::path(exported.directory) / "edited.sav");
        assert(edited == staged);
        const auto reparsed = Gen4ReadOnlySave::parse(edited, c.layout, c.id, &error);
        assert(reparsed && !reparsed->recovered());
        assert(reparsed->trainer().name == u"Ash" && reparsed->trainer().money == 123456);
        assert(slurp(fs::path(exported.directory) / "original_backup.sav") == original);
        assert(source == original);

        editor->discard();
        assert(editor->stagedBytes() == original);
    }

    // Japanese (language 1): the name stays read-only; money still stages.
    auto japanese = makeSave(Layout::Platinum, 12);
    for (int partition = 0; partition < 2; ++partition) {
        const size_t base = static_cast<size_t>(partition) * PARTITION;
        japanese[base + spec(Layout::Platinum).trainer + 0x19] = 1;
        stamp(japanese, base, spec(Layout::Platinum).generalSize, spec(Layout::Platinum).footerSize,
              partition == 0 ? 20 : 10);
    }
    std::string error;
    auto editor = Gen4StagedPokemonEditor::create(japanese, Layout::Platinum, "platinum_nds", &error);
    assert(editor && !editor->trainerNameEditable());
    assert(!editor->stageTrainerName(u"Ash", &error) && editor->stagedBytes() == japanese);
    assert(editor->stageTrainerMoney(999999, &error) && editor->stagedTrainer()->money == 999999);
    fs::remove_all(root);
}

// P3 (Round 5): Trainer Gender (+0x18) and Play Time (+0x22 u16 hours, +0x24 minutes, +0x25
// seconds; PKSM-Core Sav4) staged in the selected General block of every Gen IV game. Byte-exact:
// only those bytes and the selected General CRC change; the other General copy and Storage are
// identical; Review lists both; export -> strict reparse returns the values; Discard restores the
// source image; the source buffer is never written. Out-of-range input and a non-native stored
// gender byte are refused without changing anything.
void testTrainerGenderPlayTimeRoundTrip() {
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() /
        ("pokebank-g4-trainer-p3-" + std::to_string(::getpid()));
    fs::remove_all(root);
    struct Case { Layout layout; uint8_t rom; const char* id; int newerGeneral; };
    const Case cases[] = {
        {Layout::DiamondPearl, 10, "diamond_nds", 0}, {Layout::DiamondPearl, 11, "pearl_nds", 1},
        {Layout::Platinum, 12, "platinum_nds", 1},
        {Layout::HeartGoldSoulSilver, 7, "heartgold_nds", 0},
        {Layout::HeartGoldSoulSilver, 8, "soulsilver_nds", 1},
    };
    for (const auto& c : cases) {
        auto source = makeSave(c.layout, c.rom, c.newerGeneral);
        const auto original = source;
        std::string error;
        auto editor = Gen4StagedPokemonEditor::create(source, c.layout, c.id, &error);
        assert(editor && error.empty());
        const auto before = editor->stagedTrainer(&error);
        assert(before && before->gender == 1);   // the fixture's stored Female
        assert(editor->trainerGenderEditable());

        assert(!editor->stageTrainerGender(2, &error) && !error.empty());
        assert(!editor->stageTrainerPlayTime(1000, 0, 0, &error));
        assert(!editor->stageTrainerPlayTime(1, 60, 0, &error));
        assert(!editor->stageTrainerPlayTime(1, 0, 60, &error));
        assert(editor->stagedBytes() == original && !editor->hasChanges());

        PokeVault::Integration::Gen4::Gen4StagedTrainerEditor trainer(*editor);
        assert(trainer.genderEditable() && trainer.gender() == 1);
        assert(trainer.stageGender(1, error) && !editor->hasChanges());   // unchanged: nothing staged
        assert(trainer.stageGender(0, error) && trainer.gender() == 0);
        assert(trainer.stagePlayTime({517, 42, 9, false}, error) && error.empty());
        const auto time = trainer.playTime();
        assert(time && time->hours == 517 && time->minutes == 42 && time->seconds == 9);

        const auto sp = spec(c.layout);
        const size_t general = static_cast<size_t>(c.newerGeneral) * PARTITION;
        const size_t base = general + sp.trainer;
        const size_t crc = general + sp.generalSize - 2;
        const auto& staged = editor->stagedBytes();
        assert(staged[base + 0x18] == 0);
        assert(staged[base + 0x22] == (517 & 0xFF) && staged[base + 0x23] == (517 >> 8) &&
               staged[base + 0x24] == 42 && staged[base + 0x25] == 9);
        for (size_t i = 0; i < staged.size(); ++i) {
            if (staged[i] == original[i]) continue;
            const bool allowed = i == base + 0x18 || (i >= base + 0x22 && i < base + 0x26) ||
                                 i == crc || i == crc + 1;
            assert(allowed);
        }
        const size_t otherGeneral = static_cast<size_t>(1 - c.newerGeneral) * PARTITION;
        assert(std::equal(staged.begin() + static_cast<std::ptrdiff_t>(otherGeneral),
                          staged.begin() + static_cast<std::ptrdiff_t>(otherGeneral + sp.generalSize),
                          original.begin() + static_cast<std::ptrdiff_t>(otherGeneral)));

        const auto lines = PokeVault::Integration::Gen4::describeStagedSlotChanges(*editor);
        assert(std::find(lines.begin(), lines.end(), "Trainer gender: Female -> Male") != lines.end());
        const std::string playLine = "Play time: " + PokeVault::Integration::Common::playTimeText(
            PokeVault::Integration::Common::nativePlayTime(before->playedHours, before->playedMinutes,
                                                           before->playedSeconds)) + " -> 517:42:09";
        assert(std::find(lines.begin(), lines.end(), playLine) != lines.end());

        PokeVault::Integration::Gen4::Gen4ExportRequest request;
        request.rootDirectory = root.string();
        request.directoryStem = std::string(c.id) + "_20261024-120000";
        request.sourcePath = "sdmc:/retroarch/saves/fixture.sav";
        request.timestamp = "20261024-120000";
        auto exported = PokeVault::Integration::Gen4::publishVerifiedGen4Export(*editor, request);
        assert(exported.success && exported.error.empty());
        const auto edited = slurp(fs::path(exported.directory) / "edited.sav");
        assert(edited == staged);
        const auto reparsed = Gen4ReadOnlySave::parse(edited, c.layout, c.id, &error);
        assert(reparsed && !reparsed->recovered());
        assert(reparsed->trainer().gender == 0 && reparsed->trainer().playedHours == 517 &&
               reparsed->trainer().playedMinutes == 42 && reparsed->trainer().playedSeconds == 9);
        assert(reparsed->trainer().name == before->name && reparsed->trainer().money == before->money);
        assert(slurp(fs::path(exported.directory) / "original_backup.sav") == original);
        assert(source == original);

        editor->discard();
        assert(editor->stagedBytes() == original);
    }

    // A stored gender byte that is not a native 0 / 1 is never offered or rewritten.
    auto odd = makeSave(Layout::Platinum, 12);
    for (int partition = 0; partition < 2; ++partition) {
        const size_t base = static_cast<size_t>(partition) * PARTITION;
        odd[base + spec(Layout::Platinum).trainer + 0x18] = 7;
        stamp(odd, base, spec(Layout::Platinum).generalSize, spec(Layout::Platinum).footerSize,
              partition == 0 ? 20 : 10);
    }
    std::string error;
    auto editor = Gen4StagedPokemonEditor::create(odd, Layout::Platinum, "platinum_nds", &error);
    assert(editor && !editor->trainerGenderEditable());
    assert(!editor->stageTrainerGender(0, &error) && editor->stagedBytes() == odd);
    fs::remove_all(root);
}

int main() {
    testTrainerNameMoneyRoundTrip();
    testTrainerGenderPlayTimeRoundTrip();
    testLayout(Layout::DiamondPearl);
    testLayout(Layout::Platinum);
    testLayout(Layout::HeartGoldSoulSilver, 7, false);
    testLayout(Layout::HeartGoldSoulSilver, 8, true);
    testMixedPartitionMutationFootprint();
    testEmptySlotCreateTransaction();
    testCloneAndReleaseTransactions();
    testReleaseRestoresOriginalEmptyRecord();
    testExportRefusesStaleSource();
    testMoveSwapAndCrossBoxTransactions();
    testSharedHoldYGroupPlacementAtomicity();
    testGen4BagQuantityBoundsAndSourceImmutability();
    testNoOpAndFailureRollback();
    testShedinjaPartyHpRule();
    testRecoveredAndMismatchRemainReadOnly();
    testVerifiedWorkingCopyExport();
    testPhase4PartyBoxExportSeparation();
    testReviewLinesSkipEmptyToEmpty();
    std::cout << "Gen IV staged boxed Pokemon editor PASS\n";
    return 0;
}
