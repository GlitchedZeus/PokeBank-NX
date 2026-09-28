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

std::vector<std::byte> occupiedPk4() {
    std::vector<std::byte> d(Encryption::SIZE_STORED4, std::byte{0});
    wb32(d, 0x00, 0x12345678u);
    wb16(d, 0x08, 25);
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

std::vector<uint8_t> makeSave(Layout layout, uint8_t romCode = 7) {
    const auto sp = spec(layout);
    std::vector<uint8_t> save(SAVE_SIZE, 0xFF);
    const auto blank = Encryption::blankRecord4(Encryption::SIZE_STORED4);
    const auto occupied = occupiedPk4();

    for (int partition = 0; partition < 2; ++partition) {
        const size_t base = static_cast<size_t>(partition) * PARTITION;
        std::fill(save.begin() + static_cast<std::ptrdiff_t>(base),
                  save.begin() + static_cast<std::ptrdiff_t>(base + sp.generalSize), 0);
        const size_t storage = base + sp.storageStart;
        std::fill(save.begin() + static_cast<std::ptrdiff_t>(storage),
                  save.begin() + static_cast<std::ptrdiff_t>(storage + sp.storageSize), 0);

        save[base + sp.trainer + 0x19] = 2;
        save[base + sp.trainer + 0x1C] = romCode;
        save[base + sp.party - 4] = 0;

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
              partition == 0 ? 20 : 10);
        stamp(save, storage, sp.storageSize, sp.footerSize,
              partition == 0 ? 30 : 15);
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

void testNoOpAndFailureRollback() {
    auto source = makeSave(Layout::Platinum);
    auto editor = Gen4StagedPokemonEditor::create(
        source, Layout::Platinum, "platinum_nds");
    assert(editor);
    auto mon = editor->editableBoxPokemon(0, 0);
    assert(mon);
    assert(editor->commitBoxPokemon(0, 0, *mon));
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
    testNoOpAndFailureRollback();
    testRecoveredAndMismatchRemainReadOnly();
    std::cout << "Gen IV staged boxed Pokemon editor PASS\n";
    return 0;
}
