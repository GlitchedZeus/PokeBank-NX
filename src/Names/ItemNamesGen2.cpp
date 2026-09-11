#include "Names/ItemNames.h"

#include <array>
#include <string_view>

namespace Names {
namespace {
// Native Gen II item-id text pinned to PKSM-Core
// aa22d7a4f87c0351baf7da5962ba5acd01039a7c strings/eng/items2.txt
// (blob c78641c8f9755d6bf4cbf768390f735028d3781c). Unused/Teru-sama ids are
// deliberately represented as nullptr so they can never inherit a modern item name.
constexpr std::array<const char*, 255> kItem2Names{{
    "(None)", "Master Ball", "Ultra Ball", "BrightPowder", "Great Ball", "Poké Ball", nullptr, "Bicycle",
    "Moon Stone", "Antidote", "Burn Heal", "Ice Heal", "Awakening", "Parlyz Heal", "Full Restore", "Max Potion",
    "Hyper Potion", "Super Potion", "Potion", "Escape Rope", "Repel", "Max Elixer", "Fire Stone", "Thunder Stone",
    "Water Stone", nullptr, "HP Up", "Protein", "Iron", "Carbos", "Lucky Punch", "Calcium",
    "Rare Candy", "X Accuracy", "Leaf Stone", "Metal Powder", "Nugget", "Poké Doll", "Full Heal", "Revive",
    "Max Revive", "Guard Spec.", "Super Repel", "Max Repel", "Dire Hit", nullptr, "Fresh Water", "Soda Pop",
    "Lemonade", "X Attack", nullptr, "X Defend", "X Speed", "X Special", "Coin Case", "Itemfinder",
    nullptr, "Exp Share", "Old Rod", "Good Rod", "Silver Leaf", "Super Rod", "PP Up", "Ether",
    "Max Ether", "Elixer", "Red Scale", "SecretPotion", "S.S. Ticket", "Mystery Egg", "Clear Bell", "Silver Wing",
    "Moomoo Milk", "Quick Claw", "PSNCureBerry", "Gold Leaf", "Soft Sand", "Sharp Beak", "PRZCureBerry", "Burnt Berry",
    "Ice Berry", "Poison Barb", "King's Rock", "Bitter Berry", "Mint Berry", "Red Apricorn", "TinyMushroom", "Big Mushroom",
    "SilverPowder", "Blu Apricorn", nullptr, "Amulet Coin", "Ylw Apricorn", "Grn Apricorn", "Cleanse Tag", "Mystic Water",
    "TwistedSpoon", "Wht Apricorn", "Black Belt", "Blk Apricorn", nullptr, "Pnk Apricorn", "BlackGlasses", "SlowpokeTail",
    "Pink Bow", "Stick", "Smoke Ball", "NeverMeltIce", "Magnet", "MiracleBerry", "Pearl", "Big Pearl",
    "Everstone", "Spell Tag", "RageCandyBar", "GS Ball", "Blue Card", "Miracle Seed", "Thick Club", "Focus Band",
    nullptr, "EnergyPowder", "Energy Root", "Heal Powder", "Revival Herb", "Hard Stone", "Lucky Egg", "Card Key",
    "Machine Part", "Egg Ticket", "Lost Item", "Stardust", "Star Piece", "Basement Key", "Pass", nullptr,
    nullptr, nullptr, "Charcoal", "Berry Juice", "Scope Lens", nullptr, nullptr, "Metal Coat",
    "Dragon Fang", nullptr, "Leftovers", nullptr, nullptr, nullptr, "MysteryBerry", "Dragon Scale",
    "Berserk Gene", nullptr, nullptr, nullptr, "Sacred Ash", "Heavy Ball", "Flower Mail", "Level Ball",
    "Lure Ball", "Fast Ball", nullptr, "Light Ball", "Friend Ball", "Moon Ball", "Love Ball", "Normal Box",
    "Gorgeous Box", "Sun Stone", "Polkadot Bow", nullptr, "Up-Grade", "Berry", "Gold Berry", "SquirtBottle",
    nullptr, "Park Ball", "Rainbow Wing", nullptr, "Brick Piece", "Surf Mail", "Litebluemail", "Portraitmail",
    "Lovely Mail", "Eon Mail", "Morph Mail", "Bluesky Mail", "Music Mail", "Mirage Mail", nullptr, "TM01",
    "TM02", "TM03", "TM04", nullptr, "TM05", "TM06", "TM07", "TM08",
    "TM09", "TM10", "TM11", "TM12", "TM13", "TM14", "TM15", "TM16",
    "TM17", "TM18", "TM19", "TM20", "TM21", "TM22", "TM23", "TM24",
    "TM25", "TM26", "TM27", "TM28", nullptr, "TM29", "TM30", "TM31",
    "TM32", "TM33", "TM34", "TM35", "TM36", "TM37", "TM38", "TM39",
    "TM40", "TM41", "TM42", "TM43", "TM44", "TM45", "TM46", "TM47",
    "TM48", "TM49", "TM50", "HM01", "HM02", "HM03", "HM04", "HM05",
    "HM06", "HM07", nullptr, nullptr, nullptr, nullptr, nullptr,
}};
static_assert(kItem2Names.size() == 255);
}

const char* getItemNameG2(uint16_t itemId) {
    if (itemId >= kItem2Names.size() || itemId == 0) return "???";
    const char* name = kItem2Names[itemId];
    return name && name[0] ? name : "???";
}

} // namespace Names
