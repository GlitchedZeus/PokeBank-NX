#ifndef POKEBANK_GEN5_BAG_H
#define POKEBANK_GEN5_BAG_H

#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5SaveBlockLayout.h"
#include "Trainer/Inventory.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// Native Black/White/Black 2/White 2 player bag (save block 25, MyItem).
// Source: pinned PKHeX 6501f0ab, PKHeX.Core/Items/Bags/PlayerBag5BW.cs,
// PlayerBag5B2W2.cs, Items/ItemStorage5*.cs and SaveBlockAccessor5*.cs
// (Items = Block(25)). The item tables below are the native pouch ID lists;
// this is a structural editing catalog, NOT a story/ownership legality check.
// Each slot is two little-endian u16 values: item ID, then quantity.
enum class BagPocket5 : size_t { Items, KeyItems, Machines, Medicine, Berries };
inline constexpr size_t BagPocket5Count=5;
inline constexpr size_t ItemsBlockIndex=25;

struct BagPocket5Layout {
    size_t offset=0;   // relative to the Items block
    size_t slots=0;    // native slots PKHeX reads/writes (= pouch ID list size)
    uint16_t maxCount=0;
};

namespace BagCatalog5 {
inline constexpr std::array<uint16_t,261> kGeneral{{
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
    13, 14, 15, 16, 55, 56, 57, 58, 59, 60, 61, 62,
    63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74,
    75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86,
    87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98,
    99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
    111, 112, 116, 117, 118, 119, 135, 136, 137, 138, 139, 140,
    141, 142, 143, 144, 145, 146, 147, 148, 213, 214, 215, 216,
    217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228,
    229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240,
    241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252,
    253, 254, 255, 256, 257, 258, 259, 260, 261, 262, 263, 264,
    265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276,
    277, 278, 279, 280, 281, 282, 283, 284, 285, 286, 287, 288,
    289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300,
    301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312,
    313, 314, 315, 316, 317, 318, 319, 320, 321, 322, 323, 324,
    325, 326, 327, 492, 493, 494, 495, 496, 497, 498, 499, 500,
    537, 538, 539, 540, 541, 542, 543, 544, 545, 546, 547, 548,
    549, 550, 551, 552, 553, 554, 555, 556, 557, 558, 559, 560,
    561, 562, 563, 564, 571, 572, 573, 575, 576, 577, 580, 581,
    582, 583, 584, 585, 586, 587, 588, 589, 590,
}};

inline constexpr std::array<uint16_t,101> kMachine{{
    328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 338, 339,
    340, 341, 342, 343, 344, 345, 346, 347, 348, 349, 350, 351,
    352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363,
    364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375,
    376, 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, 387,
    388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399,
    400, 401, 402, 403, 404, 405, 406, 407, 408, 409, 410, 411,
    412, 413, 414, 415, 416, 417, 418, 419, 618, 619, 620, 420,
    421, 422, 423, 424, 425,
}};

inline constexpr std::array<uint16_t,47> kMedicine{{
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28,
    29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
    53, 54, 134, 504, 565, 566, 567, 568, 569, 570, 591,
}};

inline constexpr std::array<uint16_t,64> kBerry{{
    149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
    161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172,
    173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184,
    185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196,
    197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208,
    209, 210, 211, 212,
}};

inline constexpr std::array<uint16_t,17> kUnreleased{{
    5, 16, 260, 261, 262, 263, 264, 492, 493, 494, 495, 496,
    497, 498, 499, 500, 576,
}};

inline constexpr std::array<uint16_t,19> kKeyBW{{
    437, 442, 447, 450, 465, 466, 471, 504, 533, 574, 578, 579,
    616, 617, 621, 623, 624, 625, 626,
}};

inline constexpr std::array<uint16_t,27> kKeyB2W2{{
    437, 442, 447, 450, 453, 458, 465, 466, 471, 504, 578, 616,
    617, 621, 626, 627, 628, 629, 630, 631, 632, 633, 634, 635,
    636, 637, 638,
}};

} // namespace BagCatalog5

[[nodiscard]] constexpr size_t itemsBlockLength(SaveFamily family) noexcept {
    return family==SaveFamily::BlackWhite?LayoutInfo::BlackWhite[ItemsBlockIndex].length:
                                          LayoutInfo::Black2White2[ItemsBlockIndex].length;
}
[[nodiscard]] constexpr size_t itemsBlockOffset(SaveFamily family) noexcept {
    return family==SaveFamily::BlackWhite?LayoutInfo::BlackWhite[ItemsBlockIndex].offset:
                                          LayoutInfo::Black2White2[ItemsBlockIndex].offset;
}

[[nodiscard]] constexpr BagPocket5Layout bagPocketLayout5(SaveFamily family,size_t pocket) noexcept {
    const bool b2w2=family==SaveFamily::Black2White2;
    switch(static_cast<BagPocket5>(pocket)) {
        case BagPocket5::Items:    return {0x000,BagCatalog5::kGeneral.size(),999};
        case BagPocket5::KeyItems: return {0x4D8,b2w2?BagCatalog5::kKeyB2W2.size():BagCatalog5::kKeyBW.size(),1};
        case BagPocket5::Machines: return {0x624,BagCatalog5::kMachine.size(),1};
        case BagPocket5::Medicine: return {0x7D8,BagCatalog5::kMedicine.size(),999};
        case BagPocket5::Berries:  return {0x898,BagCatalog5::kBerry.size(),999};
    }
    return {};
}

[[nodiscard]] constexpr const char* bagPocketName5(size_t pocket) noexcept {
    constexpr std::array<const char*,BagPocket5Count> names{{
        "Items","Key Items","TMs & HMs","Medicine","Berries"}};
    return pocket<names.size()?names[pocket]:"Unknown Pocket";
}

namespace detail5 {
template<size_t N>
[[nodiscard]] constexpr bool contains(const std::array<uint16_t,N>& list,uint16_t id) noexcept {
    for(const auto v:list)if(v==id)return true;
    return false;
}
} // namespace detail5

// True when `id` is a native member of this pocket for this family.
[[nodiscard]] constexpr bool gen5BagItemInPocket(SaveFamily family,size_t pocket,uint16_t id) noexcept {
    switch(static_cast<BagPocket5>(pocket)) {
        case BagPocket5::Items:    return detail5::contains(BagCatalog5::kGeneral,id);
        case BagPocket5::KeyItems: return family==SaveFamily::Black2White2
                                       ? detail5::contains(BagCatalog5::kKeyB2W2,id)
                                       : detail5::contains(BagCatalog5::kKeyBW,id);
        case BagPocket5::Machines: return detail5::contains(BagCatalog5::kMachine,id);
        case BagPocket5::Medicine: return detail5::contains(BagCatalog5::kMedicine,id);
        case BagPocket5::Berries:  return detail5::contains(BagCatalog5::kBerry,id);
    }
    return false;
}

// Add choices: native pocket member, released in Gen V, and never a Key Item or
// TM/HM (those come from story progress; adding them can break game flags).
[[nodiscard]] constexpr bool gen5BagItemAddable(SaveFamily family,size_t pocket,uint16_t id) noexcept {
    if(pocket==static_cast<size_t>(BagPocket5::KeyItems) ||
       pocket==static_cast<size_t>(BagPocket5::Machines))return false;
    return gen5BagItemInPocket(family,pocket,id) &&
           !detail5::contains(BagCatalog5::kUnreleased,id);
}

[[nodiscard]] inline std::vector<uint16_t> gen5BagChoices(SaveFamily family,size_t pocket) {
    std::vector<uint16_t> ids;
    for(uint16_t id=1;id<=638;++id)
        if(gen5BagItemAddable(family,pocket,id))ids.push_back(id);
    return ids;
}

using ValidatedBag5=std::array<std::vector<Trainer::InventoryItem>,BagPocket5Count>;

// Decode one Items block (exactly itemsBlockLength(family) bytes). Any slot
// holding an item that is not native to its pocket, or an out-of-range count,
// fails the whole decode: an unrecognized bag is shown as unavailable, never
// guessed. A zero ID or zero quantity is an empty slot.
[[nodiscard]] inline std::optional<ValidatedBag5> decodeBag5(SaveFamily family,
                                                             std::span<const uint8_t> block) {
    if(block.size()!=itemsBlockLength(family))return std::nullopt;
    ValidatedBag5 bag{};
    for(size_t pocket=0;pocket<BagPocket5Count;++pocket) {
        const auto spec=bagPocketLayout5(family,pocket);
        if(spec.offset+spec.slots*4>block.size())return std::nullopt;
        for(size_t slot=0;slot<spec.slots;++slot) {
            const size_t at=spec.offset+slot*4;
            const uint16_t id=uint16_t(block[at]|(block[at+1]<<8));
            const uint16_t count=uint16_t(block[at+2]|(block[at+3]<<8));
            if(id==0 || count==0)continue;
            if(!gen5BagItemInPocket(family,pocket,id) || count>spec.maxCount)
                return std::nullopt;
            bag[pocket].push_back(Trainer::InventoryItem{id,count,false,false});
        }
    }
    return bag;
}

// The selected, CRC-validated copy's Items block. Never reads the other copy.
[[nodiscard]] inline std::span<const uint8_t> sourceItemsBlock(const Gen5ReadOnlySave& save) noexcept {
    const auto bytes=save.sourceBytes();
    const size_t at=save.selectedCopyOffset()+itemsBlockOffset(save.family());
    const size_t len=itemsBlockLength(save.family());
    if(at>bytes.size() || len>bytes.size()-at)return {};
    return bytes.subspan(at,len);
}

[[nodiscard]] inline std::optional<ValidatedBag5> decodeBag5(const Gen5ReadOnlySave& save) {
    return decodeBag5(save.family(),sourceItemsBlock(save));
}

// App-owned copy of the Items block. Every operation re-decodes the result
// and audits that only the targeted 4-byte slot changed; failures restore the
// prior bytes. It never touches a source file or the other save copy.
class Gen5StagedBag {
public:
    static std::optional<Gen5StagedBag> create(const Gen5ReadOnlySave& save,std::string* error=nullptr) {
        const auto block=sourceItemsBlock(save);
        if(block.empty() || !decodeBag5(save.family(),block)) {
            if(error)*error="Gen V bag is not a fully recognized native bag";
            return std::nullopt;
        }
        return Gen5StagedBag(save.family(),std::vector<uint8_t>(block.begin(),block.end()));
    }

    [[nodiscard]] SaveFamily family() const noexcept { return family_; }
    [[nodiscard]] std::span<const uint8_t> original() const noexcept { return original_; }
    [[nodiscard]] std::span<const uint8_t> staged() const noexcept { return staged_; }
    [[nodiscard]] bool dirty() const noexcept { return staged_!=original_; }
    [[nodiscard]] ValidatedBag5 current() const { return *decodeBag5(family_,staged_); }
    [[nodiscard]] ValidatedBag5 baseline() const { return *decodeBag5(family_,original_); }
    void discard() noexcept { staged_=original_; }

    [[nodiscard]] bool setQuantity(size_t pocket,size_t visibleIndex,uint16_t quantity,
                                   std::string* error=nullptr) {
        if(pocket>=BagPocket5Count || quantity==0 ||
           quantity>bagPocketLayout5(family_,pocket).maxCount)
            return fail(error,"Gen V item quantity must be 1..the pocket maximum");
        size_t at=0;
        Trainer::InventoryItem expected{};
        if(!locate(pocket,visibleIndex,at,expected,error))return false;
        if(quantity==expected.count) { if(error)error->clear(); return true; }
        return commit(at,expected.itemId,quantity,pocket,error);
    }

    // Removal is a separate, explicitly confirmed UI action; never count 0.
    [[nodiscard]] bool remove(size_t pocket,size_t visibleIndex,std::string* error=nullptr) {
        if(pocket==static_cast<size_t>(BagPocket5::KeyItems) ||
           pocket==static_cast<size_t>(BagPocket5::Machines))
            return fail(error,"Gen V Key Items and TMs/HMs are story items; removal is disabled");
        size_t at=0;
        Trainer::InventoryItem expected{};
        if(!locate(pocket,visibleIndex,at,expected,error))return false;
        return commit(at,0,0,pocket,error);
    }

    // Appends after the last occupied slot of the pocket (stable order).
    [[nodiscard]] bool add(size_t pocket,uint16_t itemId,uint16_t quantity,std::string* error=nullptr) {
        if(pocket>=BagPocket5Count || !gen5BagItemAddable(family_,pocket,itemId) ||
           quantity==0 || quantity>bagPocketLayout5(family_,pocket).maxCount)
            return fail(error,"Gen V Add rejected: item is not addable to this native pocket");
        const auto bag=current();
        for(const auto& stack:bag[pocket])
            if(stack.itemId==itemId)
                return fail(error,"Gen V item already exists in this pocket; edit quantity instead");
        const auto spec=bagPocketLayout5(family_,pocket);
        size_t afterLast=0;
        for(size_t slot=0;slot<spec.slots;++slot) {
            const size_t at=spec.offset+slot*4;
            if(read16(at)!=0 && read16(at+2)!=0)afterLast=slot+1;
        }
        for(size_t slot=afterLast;slot<spec.slots;++slot) {
            const size_t at=spec.offset+slot*4;
            if(read16(at)==0 && read16(at+2)==0)
                return commit(at,itemId,quantity,pocket,error);
        }
        return fail(error,"Gen V Add refused: this pocket has no empty native slot");
    }

private:
    Gen5StagedBag(SaveFamily family,std::vector<uint8_t> block)
        : family_(family),original_(block),staged_(std::move(block)) {}

    static bool fail(std::string* error,const char* why) {
        if(error)*error=why;
        return false;
    }
    [[nodiscard]] uint16_t read16(size_t at) const noexcept {
        return uint16_t(staged_[at]|(staged_[at+1]<<8));
    }
    bool locate(size_t pocket,size_t visibleIndex,size_t& at,
                Trainer::InventoryItem& expected,std::string* error) const {
        if(pocket>=BagPocket5Count)return fail(error,"Gen V bag pocket is out of range");
        const auto bag=current();
        if(visibleIndex>=bag[pocket].size())
            return fail(error,"Gen V item row is out of range");
        expected=bag[pocket][visibleIndex];
        const auto spec=bagPocketLayout5(family_,pocket);
        size_t populated=0;
        for(size_t slot=0;slot<spec.slots;++slot) {
            const size_t pos=spec.offset+slot*4;
            if(read16(pos)==0 || read16(pos+2)==0)continue;
            if(populated++==visibleIndex) {
                if(read16(pos)!=expected.itemId || read16(pos+2)!=expected.count)
                    return fail(error,"Gen V item changed during selection");
                at=pos;
                return true;
            }
        }
        return fail(error,"Gen V item stack no longer exists");
    }
    bool commit(size_t at,uint16_t id,uint16_t count,size_t pocket,std::string* error) {
        auto backup=staged_;
        staged_[at]=uint8_t(id);
        staged_[at+1]=uint8_t(id>>8);
        staged_[at+2]=uint8_t(count);
        staged_[at+3]=uint8_t(count>>8);
        const auto before=decodeBag5(family_,backup);
        const auto after=decodeBag5(family_,staged_);
        bool ok=before && after;
        for(size_t i=0;ok && i<staged_.size();++i)
            if((i<at || i>=at+4) && staged_[i]!=backup[i])ok=false;
        for(size_t p=0;ok && p<BagPocket5Count;++p)
            if(p!=pocket && (*after)[p].size()!=(*before)[p].size())ok=false;
        if(!ok) {
            staged_=std::move(backup);
            return fail(error,"Gen V bag change failed exact native pouch reparse");
        }
        if(error)error->clear();
        return true;
    }

    SaveFamily family_;
    std::vector<uint8_t> original_;
    std::vector<uint8_t> staged_;
};

} // namespace PokeVault::Integration::Gen5
#endif
