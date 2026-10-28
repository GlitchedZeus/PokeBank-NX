// Inherited-bug fix (PKSE 9a9a731, upstream issue #135; owner Oct 28 October campaign): building a
// Legends: Arceus working copy must not inflate Pokedex research. Trainer8LA::updatePokedexBlock ran
// the "obtained" registration for EVERY stored Pokemon on every build, so each save of the working
// copy -- including the post-image of every Bank <-> save true Move -- added +1 to "Number caught"
// for every species already in the boxes. Only a Pokemon that is genuinely new since the save was
// read (or since the previous build) may be registered; one already present changes nothing.
//
// Synthetic blocks only (no real save): a zeroed Zukan block, encrypted-blank party and box blocks.
#include "Trainer/Trainer8LA.h"
#include "Encryption/Encryption8LA.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>

using Trainer::Trainer8LA;

namespace {
constexpr size_t kZukanSize = 0x1E460;   // PLA_BLOCK_SIZE
constexpr size_t kResearchBase = 0x70, kResearchSize = 0x58;

std::vector<uint8_t> blankSlots(size_t count, size_t slotSize) {
    std::vector<std::byte> zero(Encryption::SIZE_PARTY8_LA, std::byte{0});
    std::byte* enc = Encryption::encryptArray8LA(std::span<const std::byte>(zero.data(), zero.size()), 0);
    std::vector<uint8_t> one(reinterpret_cast<uint8_t*>(enc), reinterpret_cast<uint8_t*>(enc) + slotSize);
    delete[] enc;
    std::vector<uint8_t> out;
    for (size_t i = 0; i < count; ++i) out.insert(out.end(), one.begin(), one.end());
    return out;
}

std::vector<::Save::Block> freshBlocks() {
    std::vector<::Save::Block> blocks;
    blocks.push_back({static_cast<uint32_t>(Trainer::POKEDEX8_LA), Enums::SCTypeCode::Object, Enums::SCTypeCode::None,
                      std::vector<uint8_t>(kZukanSize, 0)});
    blocks.push_back({static_cast<uint32_t>(Trainer::PARTY8_LA), Enums::SCTypeCode::Object, Enums::SCTypeCode::None,
                      blankSlots(6, Encryption::SIZE_PARTY8_LA)});
    blocks.push_back({static_cast<uint32_t>(Trainer::BOX8_LA), Enums::SCTypeCode::Object, Enums::SCTypeCode::None,
                      blankSlots(Trainer::BOX_COUNT8_LA * 30, Encryption::SIZE_STORED8_LA)});
    return blocks;
}

std::vector<uint8_t> zukan(const Trainer8LA& t) {
    for (const auto& b : t.getBlocks()) if (b.key == Trainer::POKEDEX8_LA) return b.data;
    assert(false);
    return {};
}

uint16_t caught(const std::vector<uint8_t>& z, uint16_t species) {
    const size_t o = kResearchBase + species * kResearchSize + 0x0A;
    return static_cast<uint16_t>(z[o] | (z[o + 1] << 8));
}

std::unique_ptr<::Pokemon::Pokemon> mon(const Trainer8LA& t, uint16_t species, uint32_t ec) {
    auto pk = t.createBlankPokemon();
    pk->setSpecies(species);
    pk->setEncryptionConstant(ec);
    pk->setPID(ec ^ 0x13572468u);
    return pk;
}
}  // namespace

int main() {
    constexpr uint16_t kPikachu = 25, kEevee = 133;
    // A Pokemon placed in the box this session is a genuine arrival: it counts once.
    Trainer8LA first(freshBlocks());
    first.boxes[0][0] = mon(first, kPikachu, 0x11111111u);
    first.updateBoxBlock();
    first.updatePokedexBlock();
    assert(caught(zukan(first), kPikachu) == 1);
    first.updatePokedexBlock();                       // building again adds nothing
    assert(caught(zukan(first), kPikachu) == 1);

    // Reopen the written blocks: that Pikachu is now already in the save.
    Trainer8LA reopened(first.getBlocks());
    assert(reopened.boxes[0][0] && reopened.boxes[0][0]->speciesID() == kPikachu);
    const auto before = zukan(reopened);
    reopened.updatePokedexBlock();
    reopened.updatePokedexBlock();
    assert(zukan(reopened) == before);                // unedited builds: Zukan byte-identical

    // Moving a stored Pokemon between boxes is not an arrival.
    reopened.swapBoxSlots(0, 0, 3, 7);
    reopened.updateBoxBlock();
    reopened.updatePokedexBlock();
    assert(zukan(reopened) == before);

    // A second Pikachu (different EC, e.g. a clone or a Bank arrival) counts exactly once, and a
    // new species registers once too.
    reopened.boxes[1][0] = mon(reopened, kPikachu, 0x22222222u);
    reopened.boxes[1][1] = mon(reopened, kEevee, 0x33333333u);
    reopened.updateBoxBlock();
    reopened.updatePokedexBlock();
    auto after = zukan(reopened);
    assert(caught(after, kPikachu) == 2 && caught(after, kEevee) == 1);
    reopened.updatePokedexBlock();
    assert(zukan(reopened) == after);

    // An identical clone (same species and EC as one already present) is still one more Pokemon.
    reopened.boxes[2][0] = mon(reopened, kEevee, 0x33333333u);
    reopened.updateBoxBlock();
    reopened.updatePokedexBlock();
    after = zukan(reopened);
    assert(caught(after, kEevee) == 2 && caught(after, kPikachu) == 2);

    std::cout << "Legends: Arceus research counts change only for new arrivals (PKSE 9a9a731): PASS\n";
    return 0;
}
