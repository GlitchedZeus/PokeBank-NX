// Host tests for Inventory/ItemQuantityPolicy: every Gen I-V game, every pocket.
// Item IDs are anchored to their names through the repo's own name tables so a
// wrong ID fails here instead of silently classifying the wrong item.
#include "Inventory/ItemQuantityPolicy.h"

#include "Inventory/ClassicInventoryCatalog.h"
#include "Names/ItemNames.h"

#include <cstdio>
#include <initializer_list>
#include <string>

using namespace PokeVault::Inventory::QuantityPolicy;
namespace Classic = PokeVault::Inventory;

static int failures = 0;
#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            ++failures;                                                                \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                              \
    } while (0)

static const std::initializer_list<Game> kGen1{Game::Red, Game::Blue, Game::Yellow};
static const std::initializer_list<Game> kGen2{Game::Gold, Game::Silver, Game::Crystal};
static const std::initializer_list<Game> kRSE{Game::Ruby, Game::Sapphire, Game::Emerald};
static const std::initializer_list<Game> kFRLG{Game::FireRed, Game::LeafGreen};
static const std::initializer_list<Game> kGen4{Game::Diamond, Game::Pearl, Game::Platinum,
                                               Game::HeartGold, Game::SoulSilver};
static const std::initializer_list<Game> kGen5{Game::Black, Game::White, Game::Black2, Game::White2};

static Classic::ClassicGame classic(Game g) { return static_cast<Classic::ClassicGame>(static_cast<uint8_t>(g)); }

static std::string classicName(Game g, uint16_t id) {
    return Classic::displayItemName(classic(g), Classic::ClassicPocket::PCItems, id);
}

static void expectUnique(Game g, Pocket p, uint16_t id, ItemKind kind) {
    const auto r = quantityRule(g, p, id);
    CHECK(r.valid);
    CHECK(r.unique);
    CHECK(!r.editable);
    CHECK(r.minimum == 1);
    CHECK(r.maximum == 1);
    CHECK(r.kind == kind);
    CHECK(!r.reason.empty());
    CHECK(mayAddItem(g, p, id, 0));
    CHECK(!mayAddItem(g, p, id, 1));
    if (!(r.valid && r.unique && r.maximum == 1 && r.kind == kind))
        std::fprintf(stderr, "  ^ game=%s pocket=%d id=%u\n", std::string(gameId(g)).c_str(),
                     static_cast<int>(p), static_cast<unsigned>(id));
}

static void expectStack(Game g, Pocket p, uint16_t id, uint16_t max, ItemKind kind) {
    const auto r = quantityRule(g, p, id);
    CHECK(r.valid);
    CHECK(!r.unique);
    CHECK(r.editable);
    CHECK(r.minimum == 1);
    CHECK(r.maximum == max);
    CHECK(r.kind == kind);
    CHECK(mayAddItem(g, p, id, 0));
    CHECK(mayAddItem(g, p, id, max - 1u));
    CHECK(!mayAddItem(g, p, id, max));
    if (!(r.valid && !r.unique && r.maximum == max && r.kind == kind))
        std::fprintf(stderr, "  ^ game=%s pocket=%d id=%u max=%u kind=%d\n", std::string(gameId(g)).c_str(),
                     static_cast<int>(p), static_cast<unsigned>(id), static_cast<unsigned>(r.maximum),
                     static_cast<int>(r.kind));
}

static void expectRejected(Game g, Pocket p, uint16_t id) {
    const auto r = quantityRule(g, p, id);
    CHECK(!r.valid);
    CHECK(!r.editable);
    CHECK(r.maximum == 0);
    CHECK(!mayAddItem(g, p, id, 0));
}

static void commonRejections(Game g) {
    for (const auto p : pockets(g)) {
        expectRejected(g, p, 0);
        expectRejected(g, p, 0xFFFF);
        expectRejected(g, p, 1000);
    }
}

static void testIdentity() {
    CHECK(kGameCount == 20);
    for (std::size_t i = 0; i < kGameCount; ++i) {
        const auto g = static_cast<Game>(i);
        Game back{};
        CHECK(gameFromId(gameId(g), back) && back == g);
        CHECK(!pockets(g).empty());
    }
    Game unused{};
    CHECK(!gameFromId("x_nds", unused));
    // ClassicGame shares the first eleven enumerators.
    CHECK(Classic::classicGameId(Classic::ClassicGame::LeafGreen) == gameId(Game::LeafGreen));
    CHECK(Classic::classicGameId(Classic::ClassicGame::Red) == gameId(Game::Red));
    CHECK(generation(Game::Yellow) == 1 && generation(Game::Crystal) == 2 &&
          generation(Game::LeafGreen) == 3 && generation(Game::SoulSilver) == 4 &&
          generation(Game::White2) == 5);
}

static void testGen1() {
    for (const auto g : kGen1) {
        CHECK(pockets(g).size() == 6);
        CHECK(!supportsPocket(g, Pocket::Berries));
        CHECK(classicName(g, 6) == "Bicycle");
        CHECK(classicName(g, 76) == "Old Rod" && classicName(g, 77) == "Good Rod" && classicName(g, 78) == "Super Rod");
        CHECK(classicName(g, 196).rfind("HM01", 0) == 0 && classicName(g, 200).rfind("HM05", 0) == 0);
        CHECK(classicName(g, 201).rfind("TM01", 0) == 0 && classicName(g, 250).rfind("TM50", 0) == 0);
        CHECK(classicName(g, 20) == "Potion");

        expectUnique(g, Pocket::KeyItems, 6, ItemKind::KeyItem);   // Bicycle
        for (uint16_t rod : {76, 77, 78}) expectUnique(g, Pocket::KeyItems, rod, ItemKind::KeyItem);
        expectUnique(g, Pocket::KeyItems, 45, ItemKind::KeyItem);  // Bike Voucher
        // Exp. All (75) is an ordinary stackable item: pret pokered data/items/key_items.asm:77
        // "dbit FALSE ; EXP_ALL" (171e2c2), PKHeX PlayerBag1 MaxCount 99. PKSE d2ad323 agrees.
        expectStack(g, Pocket::Items, 75, 99, ItemKind::Regular);
        expectStack(g, Pocket::PCItems, 75, 99, ItemKind::Regular);
        expectRejected(g, Pocket::KeyItems, 75);
        expectUnique(g, Pocket::PCItems, 6, ItemKind::KeyItem);    // key item in the PC stays unique
        for (uint16_t hm = 196; hm <= 200; ++hm) {
            expectUnique(g, Pocket::TMHM, hm, ItemKind::HM);
            expectUnique(g, Pocket::PCItems, hm, ItemKind::HM);
        }
        for (uint16_t tm = 201; tm <= 250; ++tm) {
            expectStack(g, Pocket::TMHM, tm, 99, ItemKind::TM);
            expectStack(g, Pocket::PCItems, tm, 99, ItemKind::TM);
        }
        expectStack(g, Pocket::Medicine, 20, 99, ItemKind::Regular); // Potion
        expectStack(g, Pocket::Balls, 4, 99, ItemKind::Regular);     // Poké Ball
        expectStack(g, Pocket::PCItems, 20, 99, ItemKind::Regular);
        expectRejected(g, Pocket::Items, 20);   // Potion is in the logical Medicine pocket
        expectRejected(g, Pocket::Berries, 20);
        CHECK(nativePocketCap(g, Pocket::Items) == 99 && nativePocketCap(g, Pocket::PCItems) == 99);
        // Dome / Helix Fossil: the player takes one at Mt. Moon.
        CHECK(classicName(g, 41) == "Dome Fossil" && classicName(g, 42) == "Helix Fossil");
        CHECK(exclusiveWith(g, 41).size() == 2 && exclusiveWith(g, 42).size() == 2);
        CHECK(exclusiveWith(g, 6).empty());
        commonRejections(g);
    }
}

static void testGen2() {
    for (const auto g : kGen2) {
        CHECK(pockets(g).size() == 5);
        CHECK(!supportsPocket(g, Pocket::Medicine) && !supportsPocket(g, Pocket::Berries));
        CHECK(classicName(g, 7) == "Bicycle");
        CHECK(classicName(g, 58) == "Old Rod" && classicName(g, 59) == "Good Rod" && classicName(g, 61) == "Super Rod");
        CHECK(classicName(g, 191).rfind("TM01", 0) == 0 && classicName(g, 242).rfind("TM50", 0) == 0);
        CHECK(classicName(g, 243).rfind("HM01", 0) == 0 && classicName(g, 249).rfind("HM07", 0) == 0);

        expectUnique(g, Pocket::KeyItems, 7, ItemKind::KeyItem);
        for (uint16_t rod : {58, 59, 61}) expectUnique(g, Pocket::KeyItems, rod, ItemKind::KeyItem);
        expectUnique(g, Pocket::PCItems, 7, ItemKind::KeyItem);
        for (uint16_t hm = 243; hm <= 249; ++hm) {
            expectUnique(g, Pocket::TMHM, hm, ItemKind::HM);
            expectUnique(g, Pocket::PCItems, hm, ItemKind::HM);
        }
        for (const auto tm : Classic::addableItems(classic(g), Classic::ClassicPocket::TMHM))
            if (tm < 243) expectStack(g, Pocket::TMHM, tm, 99, ItemKind::TM);
        expectStack(g, Pocket::Items, 18, 99, ItemKind::Regular);  // Potion
        expectStack(g, Pocket::Balls, 5, 99, ItemKind::Regular);   // Poké Ball
        expectStack(g, Pocket::PCItems, 18, 99, ItemKind::Regular);
        expectRejected(g, Pocket::TMHM, 195);   // gap in the native machine list
        expectRejected(g, Pocket::TMHM, 220);
        CHECK(nativePocketCap(g, Pocket::KeyItems) == 1);
        CHECK(exclusiveWith(g, 7).empty());
        commonRejections(g);
    }
    // GS Ball is a Crystal-only key item.
    expectUnique(Game::Crystal, Pocket::KeyItems, 115, ItemKind::KeyItem);
    expectRejected(Game::Gold, Pocket::KeyItems, 115);
}

static void testGen3() {
    using Names::getItemNameG3;
    CHECK(std::string(getItemNameG3(259)) == "Mach Bike" && std::string(getItemNameG3(272)) == "Acro Bike");
    CHECK(std::string(getItemNameG3(360)) == "Bicycle");
    CHECK(std::string(getItemNameG3(262)) == "Old Rod" && std::string(getItemNameG3(264)) == "Super Rod");
    CHECK(std::string(getItemNameG3(289)) == "TM01" && std::string(getItemNameG3(338)) == "TM50");
    CHECK(std::string(getItemNameG3(339)) == "HM01" && std::string(getItemNameG3(346)) == "HM08");
    CHECK(std::string(getItemNameG3(13)) == "Potion" && std::string(getItemNameG3(133)) == "Cheri Berry");
    CHECK(std::string(getItemNameG3(286)) == "Root Fossil" && std::string(getItemNameG3(287)) == "Claw Fossil");
    CHECK(std::string(getItemNameG3(357)) == "Helix Fossil" && std::string(getItemNameG3(358)) == "Dome Fossil");

    for (const auto g : kRSE) {
        CHECK(pockets(g).size() == 6);
        for (uint16_t bike : {259, 272}) expectUnique(g, Pocket::KeyItems, bike, ItemKind::KeyItem);
        for (uint16_t rod : {262, 263, 264}) expectUnique(g, Pocket::KeyItems, rod, ItemKind::KeyItem);
        for (uint16_t hm = 339; hm <= 346; ++hm) expectUnique(g, Pocket::TMHM, hm, ItemKind::HM);
        for (uint16_t tm = 289; tm <= 338; ++tm) expectStack(g, Pocket::TMHM, tm, 99, ItemKind::TM);
        expectStack(g, Pocket::Items, 13, 99, ItemKind::Regular);
        expectStack(g, Pocket::Balls, 4, 99, ItemKind::Regular);
        expectStack(g, Pocket::Berries, 133, 999, ItemKind::Regular);
        expectStack(g, Pocket::PCItems, 13, 999, ItemKind::Regular);
        expectStack(g, Pocket::PCItems, 289, 999, ItemKind::TM);
        // R2-CHECK-010 (pret): R/S may store HMs in the PC; Emerald refuses every important item.
        if (g == Game::Emerald) expectRejected(g, Pocket::PCItems, 339);
        else expectUnique(g, Pocket::PCItems, 339, ItemKind::HM);
        expectRejected(g, Pocket::KeyItems, 360);  // FR/LG Bicycle
        // Mach / Acro Bike and Root / Claw Fossil are story-exclusive pairs.
        CHECK(exclusiveWith(g, 259).size() == 2 && exclusiveWith(g, 272).size() == 2);
        CHECK(exclusiveWith(g, 286).size() == 2 && exclusiveWith(g, 287).size() == 2);
        CHECK(exclusiveWith(g, 262).empty());
        commonRejections(g);
    }
    // R/S PC can hold key items; Emerald's PC list cannot (PKHeX ItemStorage3RS/3E).
    expectUnique(Game::Ruby, Pocket::PCItems, 259, ItemKind::KeyItem);
    expectRejected(Game::Emerald, Pocket::PCItems, 259);
    // pokeruby sub_80A6B64 refuses importance-2 Key Items: Devon Goods, Letter, Red/Blue Orb.
    expectUnique(Game::Sapphire, Pocket::PCItems, 264, ItemKind::KeyItem);  // Super Rod (importance 1)
    for (uint16_t id : {269, 274, 276, 277}) {
        expectRejected(Game::Ruby, Pocket::PCItems, id);
        expectRejected(Game::Sapphire, Pocket::PCItems, id);
        expectUnique(Game::Ruby, Pocket::KeyItems, id, ItemKind::KeyItem);  // still valid in the bag
    }
    expectRejected(Game::Emerald, Pocket::KeyItems, 276);  // Red Orb: R/S key only
    expectUnique(Game::Ruby, Pocket::KeyItems, 276, ItemKind::KeyItem);

    for (const auto g : kFRLG) {
        expectUnique(g, Pocket::KeyItems, 360, ItemKind::KeyItem);  // Bicycle
        for (uint16_t rod : {262, 263, 264}) expectUnique(g, Pocket::KeyItems, rod, ItemKind::KeyItem);
        for (uint16_t hm = 339; hm <= 346; ++hm) expectUnique(g, Pocket::TMHM, hm, ItemKind::HM);
        for (uint16_t tm = 289; tm <= 338; ++tm) expectStack(g, Pocket::TMHM, tm, 999, ItemKind::TM);
        expectStack(g, Pocket::Items, 13, 999, ItemKind::Regular);
        expectStack(g, Pocket::Balls, 4, 999, ItemKind::Regular);
        expectStack(g, Pocket::Berries, 133, 999, ItemKind::Regular);
        expectStack(g, Pocket::PCItems, 13, 999, ItemKind::Regular);
        expectRejected(g, Pocket::KeyItems, 259);  // no Mach Bike in Kanto
        expectRejected(g, Pocket::PCItems, 289);   // FR/LG PC holds general items only
        CHECK(exclusiveWith(g, 357).size() == 2 && exclusiveWith(g, 358).size() == 2);
        CHECK(exclusiveWith(g, 360).empty());
        commonRejections(g);
    }
}

static void testGen4() {
    CHECK(std::string(Names::getItemName(450)) == "Bike");
    CHECK(std::string(Names::getItemName(445)) == "Old Rod" && std::string(Names::getItemName(447)) == "Super Rod");
    CHECK(std::string(Names::getItemName(328)) == "TM01" && std::string(Names::getItemName(419)) == "TM92");
    // The modern name table shows 420-427 as TM93-TM100 (Gen VIII+ renaming);
    // in Gen IV they are HM01-HM08 (PKHeX ItemConverter.IsItemHM4).
    CHECK(std::string(Names::getItemName(137)) == "Greet Mail");
    CHECK(std::string(Names::getItemName(55)) == "Guard Spec.");

    for (const auto g : kGen4) {
        CHECK(pockets(g).size() == 8);
        expectUnique(g, Pocket::KeyItems, 450, ItemKind::KeyItem);
        for (uint16_t rod : {445, 446, 447}) expectUnique(g, Pocket::KeyItems, rod, ItemKind::KeyItem);
        for (uint16_t hm = 420; hm <= 427; ++hm) expectUnique(g, Pocket::TMHM, hm, ItemKind::HM);
        for (uint16_t tm = 328; tm <= 419; ++tm) expectStack(g, Pocket::TMHM, tm, 99, ItemKind::TM);
        expectStack(g, Pocket::Items, 68, 999, ItemKind::Regular);        // Black Flute
        expectStack(g, Pocket::Medicine, 17, 999, ItemKind::Regular);     // Potion
        expectStack(g, Pocket::Balls, 1, 999, ItemKind::Regular);         // Master Ball
        expectStack(g, Pocket::Berries, 149, 999, ItemKind::Regular);     // Cheri Berry
        expectStack(g, Pocket::Mail, 137, 999, ItemKind::Regular);        // Greet Mail
        expectStack(g, Pocket::BattleItems, 55, 999, ItemKind::Regular); // Guard Spec.
        expectRejected(g, Pocket::Items, 17);   // Potion belongs to Medicine
        expectRejected(g, Pocket::PCItems, 17); // no PC item storage modeled for Gen IV
        expectRejected(g, Pocket::Balls, 16);   // Cherish Ball: PKHeX ItemStorage4 unreleased
        CHECK(exclusiveWith(g, 450).empty());
        commonRejections(g);
    }
    // Platinum-only key item / HG-SS Apricorn balls.
    expectUnique(Game::Platinum, Pocket::KeyItems, 467, ItemKind::KeyItem);
    expectRejected(Game::Diamond, Pocket::KeyItems, 467);
    expectStack(Game::HeartGold, Pocket::Balls, 492, 999, ItemKind::Regular);
    expectRejected(Game::Pearl, Pocket::Balls, 492);
}

static void testGen5() {
    CHECK(std::string(Names::getItemName(618)) == "TM93" && std::string(Names::getItemName(620)) == "TM95");
    CHECK(std::string(Names::getItemName(621)) == "Xtransceiver");
    CHECK(std::string(Names::getItemName(638)) == "Reveal Glass");

    for (const auto g : kGen5) {
        CHECK(pockets(g).size() == 5);
        CHECK(!supportsPocket(g, Pocket::Balls) && !supportsPocket(g, Pocket::PCItems));
        expectUnique(g, Pocket::KeyItems, 450, ItemKind::KeyItem);  // Bicycle
        expectUnique(g, Pocket::KeyItems, 447, ItemKind::KeyItem);  // Super Rod
        for (uint16_t hm = 420; hm <= 425; ++hm) expectUnique(g, Pocket::TMHM, hm, ItemKind::HM);
        for (uint16_t tm = 328; tm <= 419; ++tm) expectUnique(g, Pocket::TMHM, tm, ItemKind::TM);
        for (uint16_t tm : {618, 619, 620}) expectUnique(g, Pocket::TMHM, tm, ItemKind::TM);
        expectRejected(g, Pocket::TMHM, 426);  // Gen V has only six HMs
        expectStack(g, Pocket::Items, 1, 999, ItemKind::Regular);      // Master Ball (Items in Gen V)
        expectStack(g, Pocket::Medicine, 17, 999, ItemKind::Regular);  // Potion
        expectStack(g, Pocket::Berries, 149, 999, ItemKind::Regular);  // Cheri Berry
        expectRejected(g, Pocket::Items, 16);  // Cherish Ball: BagCatalog5::kUnreleased
        CHECK(nativePocketCap(g, Pocket::TMHM) == 1);
        commonRejections(g);
    }
    // B2W2-only key item.
    expectUnique(Game::Black2, Pocket::KeyItems, 638, ItemKind::KeyItem);
    expectRejected(Game::White, Pocket::KeyItems, 638);
}

static void testCatalogCrossCheck() {
    // Every native key item and machine in every game must be classified; no
    // key item may ever offer a quantity editor.
    for (std::size_t i = 0; i < kGameCount; ++i) {
        const auto g = static_cast<Game>(i);
        for (const auto p : pockets(g)) {
            unsigned valid = 0;
            for (uint32_t id = 0; id <= 0xFFFF; ++id) {
                const auto r = quantityRule(g, p, static_cast<uint16_t>(id));
                if (!r.valid) {
                    CHECK(!r.editable && r.maximum == 0);
                    continue;
                }
                ++valid;
                CHECK(id != 0);
                CHECK(r.kind != ItemKind::Invalid);
                CHECK(r.verified);
                CHECK(r.maximum >= 1 && r.maximum <= nativePocketCap(g, p));
                CHECK(r.editable == (r.maximum > 1));
                if (p == Pocket::KeyItems) CHECK(r.unique && r.kind == ItemKind::KeyItem);
                if (r.kind == ItemKind::KeyItem || r.kind == ItemKind::HM) CHECK(r.unique);
                if (r.kind == ItemKind::TM) CHECK(r.unique == (generation(g) == 5));
            }
            CHECK(valid > 0);
        }
        CHECK(nativePocketCap(g, Pocket::Mail) == (generation(g) == 4 ? 999 : 0));
    }
}

int main() {
    testIdentity();
    testGen1();
    testGen2();
    testGen3();
    testGen4();
    testGen5();
    testCatalogCrossCheck();
    if (failures != 0) {
        std::fprintf(stderr, "test_item_quantity_policy: %d failure(s)\n", failures);
        return 1;
    }
    std::puts("test_item_quantity_policy: OK");
    return 0;
}
