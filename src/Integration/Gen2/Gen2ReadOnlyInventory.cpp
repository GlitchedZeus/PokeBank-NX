#include "Integration/Gen2/Gen2ReadOnlyInventory.h"

#include <algorithm>
#include <array>

namespace PokeVault::Integration::Gen2 {
namespace {

struct InventoryLayout {
    std::size_t tmhm;
    std::size_t items;
    std::size_t keyItems;
    std::size_t balls;
    std::size_t pcItems;
};

constexpr InventoryLayout layoutFor(RegionLayout region, VersionFamily family) noexcept {
    if (region == RegionLayout::International) {
        return family == VersionFamily::Crystal
            ? InventoryLayout{0x23E7, 0x2420, 0x244A, 0x2465, 0x247F}
            : InventoryLayout{0x23E6, 0x241F, 0x2449, 0x2464, 0x247E};
    }
    return family == VersionFamily::Crystal
        ? InventoryLayout{0x23C9, 0x2402, 0x242C, 0x2447, 0x2461}
        : InventoryLayout{0x23C7, 0x2400, 0x242A, 0x2445, 0x245F};
}

constexpr std::array<uint8_t, 57> kMachineIds{
    191,192,193,194,196,197,198,199,
    200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,
    221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,
    243,244,245,246,247,248,249
};

// Exact Generation II raw item namespace from the Gen II oracle / pret pokecrystal item table.
// Unused retail IDs intentionally retain their TERU-SAMA identity instead of falling through to a
// later-generation National item mapping.
constexpr std::array<std::string_view, 191> kBaseNames{
    "NONE","MASTER BALL","ULTRA BALL","BRIGHTPOWDER","GREAT BALL","# BALL","TERU-SAMA","BICYCLE",
    "MOON STONE","ANTIDOTE","BURN HEAL","ICE HEAL","AWAKENING","PARLYZ HEAL","FULL RESTORE","MAX POTION",
    "HYPER POTION","SUPER POTION","POTION","ESCAPE ROPE","REPEL","MAX ELIXER","FIRE STONE","THUNDERSTONE",
    "WATER STONE","TERU-SAMA","HP UP","PROTEIN","IRON","CARBOS","LUCKY PUNCH","CALCIUM","RARE CANDY",
    "X ACCURACY","LEAF STONE","METAL POWDER","NUGGET","# DOLL","FULL HEAL","REVIVE","MAX REVIVE",
    "GUARD SPEC.","SUPER REPEL","MAX REPEL","DIRE HIT","TERU-SAMA","FRESH WATER","SODA POP","LEMONADE",
    "X ATTACK","TERU-SAMA","X DEFEND","X SPEED","X SPECIAL","COIN CASE","ITEMFINDER","TERU-SAMA",
    "EXP.SHARE","OLD ROD","GOOD ROD","SILVER LEAF","SUPER ROD","PP UP","ETHER","MAX ETHER","ELIXER",
    "RED SCALE","SECRETPOTION","S.S.TICKET","MYSTERY EGG","CLEAR BELL","SILVER WING","MOOMOO MILK",
    "QUICK CLAW","PSNCUREBERRY","GOLD LEAF","SOFT SAND","SHARP BEAK","PRZCUREBERRY","BURNT BERRY",
    "ICE BERRY","POISON BARB","KING'S ROCK","BITTER BERRY","MINT BERRY","RED APRICORN","TINYMUSHROOM",
    "BIG MUSHROOM","SILVERPOWDER","BLU APRICORN","TERU-SAMA","AMULET COIN","YLW APRICORN","GRN APRICORN",
    "CLEANSE TAG","MYSTIC WATER","TWISTEDSPOON","WHT APRICORN","BLACKBELT","BLK APRICORN","TERU-SAMA",
    "PNK APRICORN","BLACKGLASSES","SLOWPOKETAIL","PINK BOW","STICK","SMOKE BALL","NEVERMELTICE","MAGNET",
    "MIRACLEBERRY","PEARL","BIG PEARL","EVERSTONE","SPELL TAG","RAGECANDYBAR","GS BALL","BLUE CARD",
    "MIRACLE SEED","THICK CLUB","FOCUS BAND","TERU-SAMA","ENERGYPOWDER","ENERGY ROOT","HEAL POWDER",
    "REVIVAL HERB","HARD STONE","LUCKY EGG","CARD KEY","MACHINE PART","EGG TICKET","LOST ITEM","STARDUST",
    "STAR PIECE","BASEMENT KEY","PASS","TERU-SAMA","TERU-SAMA","TERU-SAMA","CHARCOAL","BERRY JUICE",
    "SCOPE LENS","TERU-SAMA","TERU-SAMA","METAL COAT","DRAGON FANG","TERU-SAMA","LEFTOVERS","TERU-SAMA",
    "TERU-SAMA","TERU-SAMA","MYSTERYBERRY","DRAGON SCALE","BERSERK GENE","TERU-SAMA","TERU-SAMA","TERU-SAMA",
    "SACRED ASH","HEAVY BALL","FLOWER MAIL","LEVEL BALL","LURE BALL","FAST BALL","TERU-SAMA","LIGHT BALL",
    "FRIEND BALL","MOON BALL","LOVE BALL","NORMAL BOX","GORGEOUS BOX","SUN STONE","POLKADOT BOW","TERU-SAMA",
    "UP-GRADE","BERRY","GOLD BERRY","SQUIRTBOTTLE","TERU-SAMA","PARK BALL","RAINBOW WING","TERU-SAMA",
    "BRICK PIECE","SURF MAIL","LITEBLUEMAIL","PORTRAITMAIL","LOVELY MAIL","EON MAIL","MORPH MAIL",
    "BLUESKY MAIL","MUSIC MAIL","MIRAGE MAIL","TERU-SAMA"
};

bool appendPairList(std::span<const uint8_t> data, std::size_t offset, std::size_t capacity,
                    std::vector<InventoryItem>& out, std::string& detail) {
    const std::size_t bytesNeeded = 1 + 2 * capacity + 1;
    if (offset > data.size() || bytesNeeded > data.size() - offset) {
        detail = "inventory pair pouch extends beyond payload";
        return false;
    }
    const uint8_t count = data[offset];
    if (count > capacity) {
        detail = "inventory pair pouch count exceeds capacity";
        return false;
    }
    const std::size_t terminator = offset + 1 + 2 * count;
    if (data[terminator] != 0xFF) {
        detail = "inventory pair pouch is missing 0xFF terminator";
        return false;
    }
    out.clear();
    out.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const uint8_t id = data[offset + 1 + 2 * i];
        const uint8_t qty = data[offset + 2 + 2 * i];
        if (id == 0 || id == 0xFF || qty == 0 || qty > 99) {
            detail = "inventory pair pouch contains an invalid id/quantity";
            return false;
        }
        out.push_back(InventoryItem{id, qty, std::string(gen2ItemName(id))});
    }
    return true;
}

bool appendKeyList(std::span<const uint8_t> data, std::size_t offset,
                   std::vector<InventoryItem>& out, std::string& detail) {
    constexpr std::size_t capacity = 26;
    const std::size_t bytesNeeded = 1 + capacity + 1;
    if (offset > data.size() || bytesNeeded > data.size() - offset) {
        detail = "key-item pouch extends beyond payload";
        return false;
    }
    const uint8_t count = data[offset];
    if (count > capacity) {
        detail = "key-item pouch count exceeds capacity";
        return false;
    }
    if (data[offset + 1 + count] != 0xFF) {
        detail = "key-item pouch is missing 0xFF terminator";
        return false;
    }
    out.clear();
    out.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const uint8_t id = data[offset + 1 + i];
        if (id == 0 || id == 0xFF) {
            detail = "key-item pouch contains an invalid id";
            return false;
        }
        out.push_back(InventoryItem{id, 1, std::string(gen2ItemName(id))});
    }
    return true;
}

} // namespace

std::string_view gen2ItemName(uint8_t itemId) noexcept {
    if (itemId < kBaseNames.size()) return kBaseNames[itemId];
    static constexpr std::array<std::string_view, 57> names{
        "TM01","TM02","TM03","TM04","TM05","TM06","TM07","TM08","TM09","TM10",
        "TM11","TM12","TM13","TM14","TM15","TM16","TM17","TM18","TM19","TM20",
        "TM21","TM22","TM23","TM24","TM25","TM26","TM27","TM28","TM29","TM30",
        "TM31","TM32","TM33","TM34","TM35","TM36","TM37","TM38","TM39","TM40",
        "TM41","TM42","TM43","TM44","TM45","TM46","TM47","TM48","TM49","TM50",
        "HM01","HM02","HM03","HM04","HM05","HM06","HM07"
    };
    for (std::size_t i = 0; i < kMachineIds.size(); ++i)
        if (kMachineIds[i] == itemId) return names[i];
    return "TERU-SAMA";
}

InventoryRecord decodeInventory(
    std::span<const uint8_t> payload, RegionLayout region, VersionFamily family) {
    InventoryRecord result;
    const auto l = layoutFor(region, family);
    if (l.tmhm + kMachineIds.size() > payload.size()) {
        result.detail = "TM/HM pouch extends beyond payload";
        return result;
    }

    for (std::size_t i = 0; i < kMachineIds.size(); ++i) {
        const uint8_t qty = payload[l.tmhm + i];
        if (qty > 99) {
            result.detail = "TM/HM pouch contains quantity greater than 99";
            return result;
        }
        if (i >= 50 && qty > 1) {
            result.detail = "HM pouch contains quantity greater than one";
            return result;
        }
        if (qty != 0) {
            const uint8_t id = kMachineIds[i];
            result.tmhm.push_back(InventoryItem{id, qty, std::string(gen2ItemName(id))});
        }
    }

    if (!appendPairList(payload, l.items, 20, result.items, result.detail)) return result;
    if (!appendKeyList(payload, l.keyItems, result.keyItems, result.detail)) return result;
    if (!appendPairList(payload, l.balls, 12, result.balls, result.detail)) return result;
    if (!appendPairList(payload, l.pcItems, 50, result.pcItems, result.detail)) return result;

    result.available = true;
    result.detail = "validated read-only Generation II inventory";
    return result;
}

} // namespace PokeVault::Integration::Gen2
