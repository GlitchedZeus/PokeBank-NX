#pragma once
// Region geometry of the Gen I lists the staged writers touch (party, current box, the two box banks,
// Bag / PC item lists, player name, money, play time). Every number is the one the strict parser
// validates (Gen1ReadOnlySave.cpp kInternational / kJapanese), cross-checked against PKHeX 8e3eb3d4:
//   SAV1Offsets.cs INT / JPN      Party 0x2F2C / 0x2ED5, CurrentBox 0x30C0 / 0x302D,
//                                 CurrentBoxIndex 0x284C / 0x2842, ChecksumOfs 0x3523 / 0x3594,
//                                 Items 0x25C9 / 0x25C4, PCItems 0x27E6 / 0x27DC,
//                                 Money 0x25F3 / 0x25EE, PlayTime 0x2CED / 0x2CA0, OT (name) 0x2598
//   SAV1.cs                       BoxCount 12 / 8, BoxSlotCount 20 / 30, StringLength 11 / 6
//                                 (GBPKML.StringLengthNotJapan / StringLengthJapanese),
//                                 SIZE_BOX_LIST = (2 * StringLength + 33 + 1) * slots + 2,
//                                 SIZE_PARTY_LIST = (2 * StringLength + 44 + 1) * 6 + 2,
//                                 GetBoxRawDataOffset: boxes [0, n/2) at 0x4000, the rest at 0x6000
//   PokeList1 / PokeCrypto.cs     list = count, species[slots], 0xFF, bodies, OT names, nicknames;
//                                 a single-entry .pk1 is 69 (SIZE_1ULIST) / 59 (SIZE_1JLIST) bytes
// and pret/pokered (English, structure only): wBoxDataStart/wBoxCount, box_struct 33 / party_struct
// 44 bytes, NAME_LENGTH 11, MONS_PER_BOX 20, the per-bank "all boxes" + per-box checksums written by
// engine/menus/save.asm (CalcIndividualBoxCheckSums). Japanese pokered has no public disassembly;
// the Japanese values are PKHeX's and the parser's (both agree), and the parser's bank checksums are
// the international scheme scaled to 4 boxes per bank (bank checksum at bank + 4 * 0x566 = 0x1598).
//
// PKHeX at 8e3eb3d4 rewrites only the main checksum (SAV1.SetChecksums); it does not refresh the
// bank checksums. The strict parser (and the game's own "box data corrupted" check) validates them,
// so the staged writers refresh both.

#include "Integration/Gen1/Gen1ReadOnlySave.h"

#include <cstddef>

namespace PokeVault::Integration::Gen1 {

struct StorageLayout {
    RegionLayout region;
    std::size_t boxCount;
    std::size_t capacity;        // Pokemon per box
    std::size_t stringLength;    // OT / nickname field bytes (characters + 0x50)
    std::size_t nicknameMax;     // characters
    std::size_t trainerNameMax;  // characters (player name and OT)
    std::size_t boxSize;         // one box list
    std::size_t bankBoxesSize;   // the boxes of one bank; the bank checksum follows
    std::size_t currentBox;
    std::size_t currentBoxIndex; // bit 7: banks initialized
    std::size_t party;
    std::size_t checksum;
    std::size_t mainLength;      // main checksum covers 0x2598 .. 0x2598 + mainLength - 1
    std::size_t bag;             // Bag list (count, 20 pairs, 0xFF)
    std::size_t pcItems;         // PC item list (count, 50 pairs, 0xFF)
    std::size_t money;           // 3 bytes BCD
    std::size_t playTime;        // hours, maxed flag, minutes, seconds, frames

    static constexpr std::size_t kMainStart = 0x2598;
    static constexpr std::size_t kNameOffset = 0x2598;
    static constexpr std::size_t kStoredBody = 33;
    static constexpr std::size_t kPartyBody = 44;
    static constexpr std::size_t kBagListSize = 1 + 20 * 2 + 1;
    static constexpr std::size_t kPcListSize = 1 + 50 * 2 + 1;

    constexpr std::size_t boxesPerBank() const noexcept { return boxCount / 2; }
    constexpr std::size_t bankOffset(std::size_t box) const noexcept {
        return (box < boxesPerBank() ? 0x4000 : 0x6000) + (box % boxesPerBank()) * boxSize;
    }
    constexpr std::size_t boxOffset(const Metadata& m, std::size_t box) const noexcept {
        return box == m.currentBox ? currentBox : bankOffset(box);
    }
    // Box list: count, capacity species markers, 0xFF, then bodies, OT names, nicknames.
    constexpr std::size_t bodyOffset(std::size_t list, std::size_t slot) const noexcept {
        return list + capacity + 2 + slot * kStoredBody;
    }
    constexpr std::size_t otOffset(std::size_t list, std::size_t slot) const noexcept {
        return list + capacity + 2 + capacity * kStoredBody + slot * stringLength;
    }
    constexpr std::size_t nickOffset(std::size_t list, std::size_t slot) const noexcept {
        return list + capacity + 2 + capacity * (kStoredBody + stringLength) + slot * stringLength;
    }
    // Party list: count, six species markers, 0xFF, six 44-byte bodies, OT names, nicknames.
    constexpr std::size_t partyBody(std::size_t slot) const noexcept { return party + 8 + slot * kPartyBody; }
    constexpr std::size_t partyOT(std::size_t slot) const noexcept {
        return party + 8 + 6 * kPartyBody + slot * stringLength;
    }
    constexpr std::size_t partyNick(std::size_t slot) const noexcept {
        return party + 8 + 6 * (kPartyBody + stringLength) + slot * stringLength;
    }
    // Single-entry .pk1: count 1, species, 0xFF, 44-byte body, OT, nickname.
    constexpr std::size_t pk1Size() const noexcept { return 3 + kPartyBody + 2 * stringLength; }
    constexpr std::size_t pk1OT() const noexcept { return 3 + kPartyBody; }
    constexpr std::size_t pk1Nick() const noexcept { return 3 + kPartyBody + stringLength; }
};

inline constexpr StorageLayout kInternationalStorage{
    RegionLayout::International, 12, 20, 11, 10, 7, 0x462, 0x1A4C, 0x30C0, 0x284C, 0x2F2C, 0x3523, 0x0F8B,
    0x25C9, 0x27E6, 0x25F3, 0x2CED,
};
inline constexpr StorageLayout kJapaneseStorage{
    RegionLayout::Japanese, 8, 30, 6, 5, 5, 0x566, 0x1598, 0x302D, 0x2842, 0x2ED5, 0x3594, 0x0FFC,
    0x25C4, 0x27DC, 0x25EE, 0x2CA0,
};

constexpr const StorageLayout& storageLayout(RegionLayout region) noexcept {
    return region == RegionLayout::Japanese ? kJapaneseStorage : kInternationalStorage;
}

// The layout arithmetic must reproduce PKHeX's list sizes and the save's fixed offsets.
namespace detail {
constexpr bool consistent(const StorageLayout& l) {
    return l.boxSize == (2 * l.stringLength + StorageLayout::kStoredBody + 1) * l.capacity + 2 &&
           l.bankBoxesSize == l.boxesPerBank() * l.boxSize &&
           l.party + (2 * l.stringLength + StorageLayout::kPartyBody + 1) * 6 + 2 == l.currentBox &&
           l.currentBox + l.boxSize + 1 == l.checksum &&
           StorageLayout::kMainStart + l.mainLength == l.checksum &&
           l.bag + StorageLayout::kBagListSize == l.money &&
           l.pcItems + StorageLayout::kPcListSize == l.currentBoxIndex &&
           0x6000 + l.bankBoxesSize + 1 + l.boxesPerBank() <= 0x8000;
}
}  // namespace detail
static_assert(detail::consistent(kInternationalStorage));
static_assert(detail::consistent(kJapaneseStorage));
static_assert(kInternationalStorage.pk1Size() == 69 && kJapaneseStorage.pk1Size() == 59);

}  // namespace PokeVault::Integration::Gen1
