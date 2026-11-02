// Phase 2: the Gen IV / Gen V bags ask Thread I's QuantityPolicy (one per-item query) and never
// exceed the native writer's cap. This pins the pocket mapping used by gen4QuantityRule /
// gen5QuantityRule to the native pocket enums, and the unique-item behaviour the UI relies on
// (no quantity editor for Key Items, Gen IV HMs, every Gen V TM/HM).
#include "Integration/Gen4/Gen4BagCatalog.h"
#include "Integration/Gen5/Gen5Bag.h"
#include "Inventory/ItemQuantityPolicy.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

namespace QP = PokeVault::Inventory::QuantityPolicy;
namespace G4 = PokeVault::Integration::Gen4;
namespace G5 = PokeVault::Integration::Gen5;

static std::string read(const char* path) {
    std::ifstream in(path, std::ios::binary);
    assert(in);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

int main() {
    // Native pocket enums the mapping arrays follow, in order.
    static_assert(static_cast<int>(G4::BagPocket::Items) == 0 && static_cast<int>(G4::BagPocket::KeyItems) == 1 &&
                  static_cast<int>(G4::BagPocket::Machines) == 2 && static_cast<int>(G4::BagPocket::Mail) == 3 &&
                  static_cast<int>(G4::BagPocket::Medicine) == 4 && static_cast<int>(G4::BagPocket::Berries) == 5 &&
                  static_cast<int>(G4::BagPocket::Balls) == 6 && static_cast<int>(G4::BagPocket::BattleItems) == 7);
    static_assert(static_cast<int>(G5::BagPocket5::Items) == 0 && static_cast<int>(G5::BagPocket5::KeyItems) == 1 &&
                  static_cast<int>(G5::BagPocket5::Machines) == 2 && static_cast<int>(G5::BagPocket5::Medicine) == 3 &&
                  static_cast<int>(G5::BagPocket5::Berries) == 4);
    const std::string gen4 = read("src/UI/Gen4SharedPokemonSurface.inc");
    const std::string gen5 = read("src/UI/Gen5SharedReadOnlySurface.inc");
    const std::string gen4Rule = read("include/UI/Gen4BagQuantity.h");
    const std::string base = read("src/UI/TrainerViewScreenBase.inc");
    assert(gen4Rule.find("QP::Pocket::Items, QP::Pocket::KeyItems, QP::Pocket::TMHM, QP::Pocket::Mail,\n"
                         "        QP::Pocket::Medicine, QP::Pocket::Berries, QP::Pocket::Balls, QP::Pocket::BattleItems}};")
           != std::string::npos);
    // One query for the surface (A / Add) and the Base Items footer (A shown only if editable).
    assert(gen4.find("PokeBank::UIModel::Gen4BagQuantity::rule(game, pocket, id)") != std::string::npos);
    assert(base.find("PokeBank::UIModel::Gen4BagQuantity::rule(") != std::string::npos);
    assert(base.find("instructions += \"  |  A: Edit Quantity  |  Y: Remove...\";") == std::string::npos);
    assert(gen5.find("QP::Pocket::Items, QP::Pocket::KeyItems, QP::Pocket::TMHM, QP::Pocket::Medicine,\n"
                     "        QP::Pocket::Berries}};") != std::string::npos);
    assert(gen4Rule.find("std::min(r.maximum, nativeCap)") != std::string::npos);
    // Gen V clamps through the shared Bag Add flow's writer bound (UI/SharedBagAddFlow.h).
    assert(gen5.find("BagAdd::decideRow(") != std::string::npos);
    assert(read("include/UI/SharedBagAddFlow.h").find("std::min(rule.maximum, writerCap)") != std::string::npos);

    // Every native Gen IV item: the policy is valid where the native catalog allows it, and the
    // effective cap (policy capped by the writer) is 1 exactly for unique items.
    for (const char* id : {"diamond_nds", "platinum_nds", "heartgold_nds"}) {
        QP::Game game{};
        assert(QP::gameFromId(id, game));
        const auto layout = std::string(id) == "heartgold_nds" ? G4::Layout::HeartGoldSoulSilver
            : std::string(id) == "platinum_nds" ? G4::Layout::Platinum : G4::Layout::DiamondPearl;
        constexpr QP::Pocket kMap[] = {QP::Pocket::Items, QP::Pocket::KeyItems, QP::Pocket::TMHM,
            QP::Pocket::Mail, QP::Pocket::Medicine, QP::Pocket::Berries, QP::Pocket::Balls, QP::Pocket::BattleItems};
        int unique = 0, editable = 0;
        for (std::size_t pocket = 0; pocket < G4::BagPocketCount; ++pocket)
            for (uint16_t item = 1; item < 540; ++item) {
                if (!G4::gen4BagItemAllowed(layout, pocket, item)) continue;
                const auto rule = QP::quantityRule(game, kMap[pocket], item);
                assert(rule.valid);
                const uint16_t cap = std::min(rule.maximum, G4::gen4BagMaxQuantity(pocket, item));
                if (pocket == 1 || (pocket == 2 && item >= 420)) { assert(cap == 1 && !(rule.editable && cap > 1)); ++unique; }
                else { assert(cap > 1 && rule.editable); ++editable; }
            }
        assert(unique > 0 && editable > 0);
    }
    // Gen V: every TM/HM is unique; ordinary pockets stack to 999.
    for (const char* id : {"black_nds", "white2_nds"}) {
        QP::Game game{};
        assert(QP::gameFromId(id, game));
        assert(QP::quantityRule(game, QP::Pocket::TMHM, 328).maximum == 1);
        assert(QP::quantityRule(game, QP::Pocket::Items, 1).maximum == 999 ||
               !QP::quantityRule(game, QP::Pocket::Items, 1).valid);
    }
    std::cout << "Gen IV/V bag quantity via QuantityPolicy (pocket mapping, unique items, native caps): PASS\n";
}
