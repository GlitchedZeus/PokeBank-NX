#include "Inventory/ItemQuantityPolicy.h"

#include "Inventory/ClassicInventoryCatalog.h"
#include "Integration/Gen4/Gen4BagCatalog.h"
#include "Integration/Gen5/Gen5Bag.h"

#include <algorithm>
#include <array>

namespace PokeVault::Inventory::QuantityPolicy {
namespace {

namespace G4 = PokeVault::Integration::Gen4;
namespace G5 = PokeVault::Integration::Gen5;

constexpr std::array<Pocket, 6> kGen1Pockets{{
    Pocket::Items, Pocket::Medicine, Pocket::Balls, Pocket::KeyItems, Pocket::TMHM, Pocket::PCItems,
}};
constexpr std::array<Pocket, 5> kGen2Pockets{{
    Pocket::Items, Pocket::Balls, Pocket::KeyItems, Pocket::TMHM, Pocket::PCItems,
}};
constexpr std::array<Pocket, 6> kGen3Pockets{{
    Pocket::Items, Pocket::KeyItems, Pocket::Balls, Pocket::TMHM, Pocket::Berries, Pocket::PCItems,
}};
// Gen4ReadOnlyInventory.h BagPocket order.
constexpr std::array<Pocket, 8> kGen4Pockets{{
    Pocket::Items, Pocket::KeyItems, Pocket::TMHM, Pocket::Mail,
    Pocket::Medicine, Pocket::Berries, Pocket::Balls, Pocket::BattleItems,
}};
// Gen5Bag.h BagPocket5 order (Poké Balls live in Items in Gen V).
constexpr std::array<Pocket, 5> kGen5Pockets{{
    Pocket::Items, Pocket::KeyItems, Pocket::TMHM, Pocket::Medicine, Pocket::Berries,
}};

// Story-exclusive pairs. Advisory only.
// Bulbapedia "Mt. Moon": the player takes either the Dome Fossil or the Helix
// Fossil (R/B/Y ids 41/42, Names/ItemNames G1; FR/LG ids 358/357, G3).
constexpr std::array<uint16_t, 2> kRbyFossils{{41, 42}};
constexpr std::array<uint16_t, 2> kFrlgFossils{{357, 358}};
// Bulbapedia "Rydel's Cycles": in R/S/E the Mach Bike (259) and Acro Bike (272)
// are swapped, never held together. Bulbapedia "Mirage Tower" / "Route 111": the
// player takes either the Root Fossil (286) or the Claw Fossil (287).
constexpr std::array<uint16_t, 2> kRseBikes{{259, 272}};
constexpr std::array<uint16_t, 2> kRseFossils{{286, 287}};

bool isClassic(Game game) noexcept { return game <= Game::LeafGreen; }

ClassicGame toClassic(Game game) noexcept {
    // Enumerators are declared in the same order as ClassicGame.
    return static_cast<ClassicGame>(static_cast<uint8_t>(game));
}

bool toClassicPocket(Pocket pocket, ClassicPocket& out) noexcept {
    switch (pocket) {
        case Pocket::Items: out = ClassicPocket::Items; return true;
        case Pocket::Medicine: out = ClassicPocket::Medicines; return true;
        case Pocket::Balls: out = ClassicPocket::Balls; return true;
        case Pocket::KeyItems: out = ClassicPocket::KeyItems; return true;
        case Pocket::TMHM: out = ClassicPocket::TMHM; return true;
        case Pocket::Berries: out = ClassicPocket::Berries; return true;
        case Pocket::PCItems: out = ClassicPocket::PCItems; return true;
        case Pocket::Mail:
        case Pocket::BattleItems: return false;
    }
    return false;
}

G4::Layout gen4Layout(Game game) noexcept {
    if (game == Game::Platinum) return G4::Layout::Platinum;
    if (game == Game::HeartGold || game == Game::SoulSilver) return G4::Layout::HeartGoldSoulSilver;
    return G4::Layout::DiamondPearl;
}

G5::SaveFamily gen5Family(Game game) noexcept {
    return (game == Game::Black2 || game == Game::White2) ? G5::SaveFamily::Black2White2
                                                          : G5::SaveFamily::BlackWhite;
}

bool gen4PocketIndex(Pocket pocket, std::size_t& out) noexcept {
    const auto it = std::find(kGen4Pockets.begin(), kGen4Pockets.end(), pocket);
    if (it == kGen4Pockets.end()) return false;
    out = static_cast<std::size_t>(it - kGen4Pockets.begin());
    return true;
}

bool gen5PocketIndex(Pocket pocket, std::size_t& out) noexcept {
    const auto it = std::find(kGen5Pockets.begin(), kGen5Pockets.end(), pocket);
    if (it == kGen5Pockets.end()) return false;
    out = static_cast<std::size_t>(it - kGen5Pockets.begin());
    return true;
}

bool gen5Released(uint16_t id) noexcept {
    return std::find(G5::BagCatalog5::kUnreleased.begin(), G5::BagCatalog5::kUnreleased.end(), id) ==
           G5::BagCatalog5::kUnreleased.end();
}

// Native pocket membership (released items only), straight from the repo catalogs.
bool isPocketMember(Game game, Pocket pocket, uint16_t id) noexcept {
    if (id == 0) return false;
    if (isClassic(game)) {
        ClassicPocket classic{};
        return toClassicPocket(pocket, classic) && Inventory::isAddableItem(toClassic(game), classic, id);
    }
    if (generation(game) == 4) {
        std::size_t index = 0;
        return gen4PocketIndex(pocket, index) && G4::gen4BagItemAllowed(gen4Layout(game), index, id);
    }
    std::size_t index = 0;
    return gen5PocketIndex(pocket, index) && G5::gen5BagItemInPocket(gen5Family(game), index, id) &&
           gen5Released(id);
}

bool inSpan(std::span<const uint16_t> values, uint16_t id) noexcept {
    return std::find(values.begin(), values.end(), id) != values.end();
}

// Not every catalog pocket is a subset of the PC list (e.g. R/B/Y id 50, the
// unused duplicate "PP Up", sits in Medicines only), so check them all.
bool inAnyClassicPocket(ClassicGame game, uint16_t id) noexcept {
    for (const auto pocket : Inventory::supportedPockets(game))
        if (inSpan(addableItems(game, pocket), id)) return true;
    return false;
}

} // namespace

int generation(Game game) noexcept {
    if (game <= Game::Yellow) return 1;
    if (game <= Game::Crystal) return 2;
    if (game <= Game::LeafGreen) return 3;
    if (game <= Game::SoulSilver) return 4;
    return 5;
}

std::string_view gameId(Game game) noexcept {
    switch (game) {
        case Game::Red: return "red_gb";
        case Game::Blue: return "blue_gb";
        case Game::Yellow: return "yellow_gb";
        case Game::Gold: return "gold_gbc";
        case Game::Silver: return "silver_gbc";
        case Game::Crystal: return "crystal_gbc";
        case Game::Ruby: return "ruby_gba";
        case Game::Sapphire: return "sapphire_gba";
        case Game::Emerald: return "emerald_gba";
        case Game::FireRed: return "firered_gba";
        case Game::LeafGreen: return "leafgreen_gba";
        case Game::Diamond: return "diamond_nds";
        case Game::Pearl: return "pearl_nds";
        case Game::Platinum: return "platinum_nds";
        case Game::HeartGold: return "heartgold_nds";
        case Game::SoulSilver: return "soulsilver_nds";
        case Game::Black: return "black_nds";
        case Game::White: return "white_nds";
        case Game::Black2: return "black2_nds";
        case Game::White2: return "white2_nds";
    }
    return {};
}

bool gameFromId(std::string_view id, Game& out) noexcept {
    for (std::size_t i = 0; i < kGameCount; ++i) {
        const auto game = static_cast<Game>(i);
        if (gameId(game) == id) {
            out = game;
            return true;
        }
    }
    return false;
}

std::span<const Pocket> pockets(Game game) noexcept {
    switch (generation(game)) {
        case 1: return kGen1Pockets;
        case 2: return kGen2Pockets;
        case 3: return kGen3Pockets;
        case 4: return kGen4Pockets;
        default: return kGen5Pockets;
    }
}

bool supportsPocket(Game game, Pocket pocket) noexcept {
    const auto list = pockets(game);
    return std::find(list.begin(), list.end(), pocket) != list.end();
}

uint16_t nativePocketCap(Game game, Pocket pocket) noexcept {
    if (!supportsPocket(game, pocket)) return 0;
    // Key Items never stack: Gen II stores an ID-only list
    // (Gen2ReadOnlyInventory.cpp appendKeyList); Gen III-V PKHeX MaxCount 1.
    // Gen I has no native key pocket; its logical Key Items are unique items.
    if (pocket == Pocket::KeyItems) return 1;
    switch (game) {
        case Game::Red: case Game::Blue: case Game::Yellow:
            // PlayerBag1.cs: bag (20 slots) and PC (50 slots) both MaxCount 99;
            // Gen1ReadOnlyInventory.cpp kMaxQuantity = 99.
            return 99;
        case Game::Gold: case Game::Silver: case Game::Crystal:
            // PlayerBag2.cs: TMHM/Items/Balls/PC MaxCount 99;
            // Gen2ReadOnlyInventory.cpp rejects TM/HM quantity > 99.
            return 99;
        case Game::Ruby: case Game::Sapphire: case Game::Emerald:
            // PlayerBag3RS.cs / PlayerBag3E.cs: Items 99, Balls 99, TMHMs 99,
            // Berries 999, PCItems 999.
            return (pocket == Pocket::Berries || pocket == Pocket::PCItems) ? 999 : 99;
        case Game::FireRed: case Game::LeafGreen:
            // PlayerBag3FRLG.cs: Items, Balls, TMHMs, Berries and PCItems all 999.
            return 999;
        case Game::Diamond: case Game::Pearl: case Game::Platinum:
        case Game::HeartGold: case Game::SoulSilver:
            // PlayerBag4DP/Pt/HGSS.cs: TMHMs 99, every other pouch 999;
            // Gen4BagCatalog.h gen4BagMaxQuantity agrees.
            return pocket == Pocket::TMHM ? 99 : 999;
        case Game::Black: case Game::White: case Game::Black2: case Game::White2:
            // PlayerBag5BW/B2W2.cs: TMHMs 1 (TMs are reusable), others 999;
            // Gen5Bag.h bagPocketLayout5 agrees.
            return pocket == Pocket::TMHM ? 1 : 999;
    }
    return 0;
}

ItemKind itemKind(Game game, uint16_t id) noexcept {
    if (id == 0) return ItemKind::Invalid;
    switch (generation(game)) {
        case 1: {
            const auto g = toClassic(game);
            // pokered numbering (Names/ItemNames G1, ClassicInventoryCatalog
            // classicMachineMove): HM01-05 = 196-200, TM01-50 = 201-250.
            if (id >= 196 && id <= 200) return ItemKind::HM;
            if (id >= 201 && id <= 250) return ItemKind::TM;
            if (inSpan(addableItems(g, ClassicPocket::KeyItems), id)) return ItemKind::KeyItem;
            return inAnyClassicPocket(g, id) ? ItemKind::Regular : ItemKind::Invalid;
        }
        case 2: {
            const auto g = toClassic(game);
            // PKHeX ItemStorage2.Machine: TMs are the first 50 entries (191-242),
            // HM01-07 = 243-249.
            if (inSpan(addableItems(g, ClassicPocket::TMHM), id))
                return id >= 243 ? ItemKind::HM : ItemKind::TM;
            if (inSpan(addableItems(g, ClassicPocket::KeyItems), id)) return ItemKind::KeyItem;
            return inAnyClassicPocket(g, id) ? ItemKind::Regular : ItemKind::Invalid;
        }
        case 3: {
            const auto g = toClassic(game);
            // PKHeX ItemStorage3RS IsTM / IsHM: TM01-50 = 289-338, HM01-08 = 339-346.
            if (id >= 289 && id <= 338) return ItemKind::TM;
            if (id >= 339 && id <= 346) return ItemKind::HM;
            if (inSpan(addableItems(g, ClassicPocket::KeyItems), id)) return ItemKind::KeyItem;
            return inAnyClassicPocket(g, id) ? ItemKind::Regular : ItemKind::Invalid;
        }
        case 4: {
            // Gen4BagCatalog.h: Machines pouch 328-427; gen4BagMaxQuantity
            // treats 420-427 (HM01-08) as max 1.
            const auto layout = gen4Layout(game);
            if (id >= 328 && id <= 419) return ItemKind::TM;
            if (id >= 420 && id <= 427) return ItemKind::HM;
            if (G4::gen4BagItemAllowed(layout, static_cast<std::size_t>(G4::BagPocket::KeyItems), id))
                return ItemKind::KeyItem;
            for (std::size_t p = 0; p < G4::BagPocketCount; ++p)
                if (G4::gen4BagItemAllowed(layout, p, id)) return ItemKind::Regular;
            return ItemKind::Invalid;
        }
        default: {
            // Gen5Bag.h BagCatalog5::kMachine: TM01-92 = 328-419, TM93-95 =
            // 618-620, HM01-06 = 420-425.
            const auto family = gen5Family(game);
            if (!gen5Released(id)) return ItemKind::Invalid;
            if (id >= 420 && id <= 425) return ItemKind::HM;
            if (G5::gen5BagItemInPocket(family, static_cast<std::size_t>(G5::BagPocket5::Machines), id))
                return ItemKind::TM;
            if (G5::gen5BagItemInPocket(family, static_cast<std::size_t>(G5::BagPocket5::KeyItems), id))
                return ItemKind::KeyItem;
            for (std::size_t p = 0; p < G5::BagPocket5Count; ++p)
                if (G5::gen5BagItemInPocket(family, p, id)) return ItemKind::Regular;
            return ItemKind::Invalid;
        }
    }
}

ItemQuantityRule quantityRule(Game game, Pocket pocket, uint16_t itemId) noexcept {
    ItemQuantityRule rule{};
    if (static_cast<std::size_t>(game) >= kGameCount) {
        rule.reason = "Unknown game";
        return rule;
    }
    if (!supportsPocket(game, pocket)) {
        rule.reason = "This game has no such pocket";
        return rule;
    }
    if (!isPocketMember(game, pocket, itemId)) {
        rule.reason = itemId == 0 ? "Empty item id" : "Item is not native to this game and pocket";
        return rule;
    }
    rule.valid = true;
    rule.kind = itemKind(game, itemId);
    rule.minimum = 1;

    const auto unique = [&rule](std::string_view why) {
        rule.unique = true;
        rule.editable = false;
        rule.maximum = 1;
        rule.reason = why;
        return rule;
    };
    switch (rule.kind) {
        case ItemKind::KeyItem:
            // Gen I: Bulbapedia "Key Item" / pokered IsKeyItem - one of each,
            // cannot be tossed. Gen II: ID-only key list. Gen III-V: PKHeX Key
            // pouch MaxCount 1. Holds in the PC too (RS / GSC / RBY PC lists).
            return unique("Key items exist only once");
        case ItemKind::HM:
            // PKHeX PlayerBag1/2/3*/4* GetMaxCount: HM -> 1; Gen V Machines MaxCount 1.
            return unique("HMs are reusable and exist only once");
        case ItemKind::TM:
            if (generation(game) == 5)
                // PlayerBag5BW/B2W2.cs TMHMs MaxCount 1; Bulbapedia "TM": reusable from Gen V.
                return unique("Gen V TMs are reusable and exist only once");
            break; // Gen I-IV TMs are single-use: stack to the native cap.
        case ItemKind::Regular:
        case ItemKind::Invalid:
            break;
    }
    rule.maximum = nativePocketCap(game, pocket);
    rule.editable = rule.maximum > 1;
    rule.reason = rule.kind == ItemKind::TM ? "Single-use TM; stacks to the pocket maximum"
                                            : "Stacks to the pocket maximum";
    return rule;
}

bool mayAddItem(Game game, Pocket pocket, uint16_t itemId, uint32_t heldAlready) noexcept {
    const auto rule = quantityRule(game, pocket, itemId);
    if (!rule.valid) return false;
    if (rule.unique) return heldAlready == 0;
    return heldAlready < rule.maximum;
}

std::span<const uint16_t> exclusiveWith(Game game, uint16_t itemId) noexcept {
    const auto pick = [itemId](std::span<const uint16_t> group) -> std::span<const uint16_t> {
        return inSpan(group, itemId) ? group : std::span<const uint16_t>{};
    };
    switch (game) {
        case Game::Red: case Game::Blue: case Game::Yellow:
            return pick(kRbyFossils);
        case Game::FireRed: case Game::LeafGreen:
            return pick(kFrlgFossils);
        case Game::Ruby: case Game::Sapphire: case Game::Emerald: {
            const auto bikes = pick(kRseBikes);
            return bikes.empty() ? pick(kRseFossils) : bikes;
        }
        default:
            return {};
    }
}

} // namespace PokeVault::Inventory::QuantityPolicy
