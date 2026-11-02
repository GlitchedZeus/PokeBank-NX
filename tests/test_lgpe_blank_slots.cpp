// Inherited-bug fix (PKSE 9a9a731; owner Oct 28 October campaign): writing Let's Go storage must not
// zero the game's own empty-slot records. Trainer7LGPE::updateBoxBlock memset every slot after the
// packed list to 0, so an UNEDITED working copy -- and the post-image of every Bank <-> save true
// Move -- rewrote every empty slot (upstream measured 239,164 changed bytes on one save). The game
// writes an empty slot as encryption constant 0 followed by non-zero bytes. PKHeX CompressStorage
// packs the occupied slots and moves the file's own empty records behind them, in order.
//
// Synthetic blocks only: two occupied slots and 998 distinctive "game blank" records.
#include "Trainer/Trainer7LGPE.h"
#include "Encryption/Encryption7LGPE.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

namespace T = ::Trainer;

namespace {
constexpr size_t kTotal = T::BOX_COUNT7_LGPE * T::SLOTS_PER_BOX7_LGPE;   // 1000
constexpr size_t kSlot = T::SIZE_PARTY7_LGPE;

std::vector<uint8_t> occupied(uint16_t species, uint32_t ec) {
    std::vector<std::byte> zero(kSlot, std::byte{0});
    std::byte* blankEnc = Encryption::encryptArray7LGPE(std::span<const std::byte>(zero.data(), kSlot), 0);
    ::Pokemon::Pokemon7LGPE pk(std::span<const std::byte>(blankEnc, kSlot));
    delete[] blankEnc;
    pk.setSpecies(species);
    pk.setEncryptionConstant(ec);
    pk.setPID(ec ^ 0x0BADF00Du);
    std::byte* enc = Encryption::encryptArray7LGPE(pk.getData(), ec);
    std::vector<uint8_t> out(reinterpret_cast<uint8_t*>(enc), reinterpret_cast<uint8_t*>(enc) + kSlot);
    delete[] enc;
    return out;
}

// A game-style blank: EC 0, then bytes that differ per slot so any reordering is visible.
std::vector<uint8_t> gameBlank(size_t slot) {
    std::vector<uint8_t> b(kSlot);
    for (size_t i = 4; i < kSlot; ++i) b[i] = static_cast<uint8_t>(0x40 + slot * 7 + i);
    for (size_t i = 0; i < 4; ++i) b[i] = 0;
    b[4] = static_cast<uint8_t>(slot); b[5] = static_cast<uint8_t>(slot >> 8);
    return b;
}

std::vector<Save::Block> blocks(std::vector<uint8_t>& storage) {
    storage.clear();
    const auto a = occupied(25, 0x11111111u), b = occupied(133, 0x22222222u);
    storage.insert(storage.end(), a.begin(), a.end());
    storage.insert(storage.end(), b.begin(), b.end());
    for (size_t s = 2; s < kTotal; ++s) { const auto g = gameBlank(s); storage.insert(storage.end(), g.begin(), g.end()); }
    std::vector<uint8_t> header(0x12, 0);
    for (size_t i = 0; i < 7; ++i) { header[i * 2] = 0xE9; header[i * 2 + 1] = 0x03; }   // 1001: empty
    header[0x0E] = 2;                                                                      // packed count
    return {{static_cast<uint32_t>(T::POKE_LIST_HEADER7_LGPE), Enums::SCTypeCode::Object, Enums::SCTypeCode::None, header},
            {static_cast<uint32_t>(T::POKE_LIST_POKEMON7_LGPE), Enums::SCTypeCode::Object, Enums::SCTypeCode::None, storage}};
}

const std::vector<uint8_t>& storageOf(const T::Trainer7LGPE& t) {
    for (const auto& b : t.getBlocks()) if (b.key == T::POKE_LIST_POKEMON7_LGPE) return b.data;
    assert(false);
    static std::vector<uint8_t> none;
    return none;
}

bool sameSlot(const std::vector<uint8_t>& a, size_t ia, const std::vector<uint8_t>& b, size_t ib) {
    return std::equal(a.begin() + ia * kSlot, a.begin() + (ia + 1) * kSlot, b.begin() + ib * kSlot);
}
}  // namespace

int main() {
    std::vector<uint8_t> original;
    // Unedited write: storage is byte-identical (no game blank is zeroed).
    {
        T::Trainer7LGPE t(blocks(original));
        assert(t.boxes[0][0] && t.boxes[0][0]->speciesID() == 25 && !t.boxes[0][2]);
        t.updateBoxBlock();
        assert(storageOf(t) == original);
    }
    // A Pokemon leaves storage (e.g. a true Move to the Bank): the survivor packs to slot 0, the
    // file's blank records follow in their original order, and the one extra free slot gets a copy
    // of a genuine game blank -- never zeros, and never the departed Pokemon's old bytes.
    {
        T::Trainer7LGPE t(blocks(original));
        t.boxes[0][0].reset();
        t.updateBoxBlock();
        const auto& out = storageOf(t);
        assert(out.size() == original.size());
        assert(sameSlot(out, 0, original, 1));                        // Eevee packed forward
        for (size_t s = 1; s + 1 < kTotal; ++s) assert(sameSlot(out, s, original, s + 1));
        assert(sameSlot(out, kTotal - 1, original, 2));               // copy of a genuine blank
        for (size_t s = 0; s < kTotal; ++s)
            assert(!std::all_of(out.begin() + s * kSlot, out.begin() + (s + 1) * kSlot, [](uint8_t v) { return v == 0; }));
        const T::Trainer7LGPE reread(t.getBlocks());
        assert(reread.boxes[0][0] && reread.boxes[0][0]->speciesID() == 133 && !reread.boxes[0][1]);
    }
    std::cout << "Let's Go storage keeps the game's own empty-slot records (PKSE 9a9a731): PASS\n";
    return 0;
}
