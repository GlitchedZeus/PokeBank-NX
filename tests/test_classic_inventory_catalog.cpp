#include "Inventory/ClassicInventoryCatalog.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>

using namespace PokeVault::Inventory;

static bool has(ClassicGame game, ClassicPocket pocket, uint16_t id) {
    const auto items = addableItems(game, pocket);
    return std::find(items.begin(), items.end(), id) != items.end();
}

int main() {
    assert(classicGameFromId("yellow_gb") == ClassicGame::Yellow);
    assert(classicGameFromId("gold_gbc") == ClassicGame::Gold);
    assert(classicGameFromId("emerald_gba") == ClassicGame::Emerald);
    assert(classicGameFromId("firered_gba") == ClassicGame::FireRed);
    assert(!classicGameFromId("unknown"));

    assert(supportsPocket(ClassicGame::Yellow, ClassicPocket::Medicines));
    assert(!supportsPocket(ClassicGame::Yellow, ClassicPocket::Berries));
    assert(!supportsPocket(ClassicGame::Gold, ClassicPocket::Berries));
    assert(supportsPocket(ClassicGame::Emerald, ClassicPocket::Berries));

    // Raw id 13 is Ice Heal in Gen I, not modern Dusk Ball. The catalog never interprets
    // modern item ids in Gen I and therefore cannot inject a future-generation Dusk Ball.
    assert(has(ClassicGame::Yellow, ClassicPocket::Medicines, 13));
    assert(displayItemName(ClassicGame::Yellow, ClassicPocket::Medicines, 13) == "Ice Heal");
    assert(!has(ClassicGame::Yellow, ClassicPocket::Balls, 13));
    assert(!has(ClassicGame::Gold, ClassicPocket::Balls, 13));
    assert(!has(ClassicGame::Emerald, ClassicPocket::Balls, 13)); // Gen III ball ids end at 12.

    // Raw ID 5 is the ordinary ball in all three titles; GS Ball stays ID 115.
    for (auto game : {ClassicGame::Gold, ClassicGame::Silver, ClassicGame::Crystal}) {
        assert(has(game, ClassicPocket::Balls, 5));
        assert(displayItemName(game, ClassicPocket::Balls, 5) == "Poké Ball");
        assert(displayItemName(game, ClassicPocket::PCItems, 5) == "Poké Ball");
        assert(displayItemName(game, ClassicPocket::KeyItems, 115) == "GS BALL");
        assert(displayItemName(game, ClassicPocket::PCItems, 115) == "GS BALL");
    }

    assert(has(ClassicGame::Gold, ClassicPocket::KeyItems, 54));
    assert(!has(ClassicGame::Gold, ClassicPocket::KeyItems, 70));
    assert(has(ClassicGame::Crystal, ClassicPocket::KeyItems, 70));

    assert(has(ClassicGame::Ruby, ClassicPocket::KeyItems, 259));
    assert(!has(ClassicGame::FireRed, ClassicPocket::KeyItems, 259));
    assert(has(ClassicGame::FireRed, ClassicPocket::KeyItems, 349));
    assert(!has(ClassicGame::Ruby, ClassicPocket::KeyItems, 349));
    assert(has(ClassicGame::Emerald, ClassicPocket::KeyItems, 375));

    assert(quantityRule(ClassicGame::Gold, ClassicPocket::KeyItems, 54).maximum == 1);
    assert(!quantityRule(ClassicGame::Gold, ClassicPocket::KeyItems, 54).editable);
    assert(quantityRule(ClassicGame::Gold, ClassicPocket::Items, 18).maximum == 99);

    assert(classicMachineMove(ClassicGame::Yellow, 201) == 5);   // TM01 Mega Punch
    assert(classicMachineMove(ClassicGame::Gold, 191) == 223);   // TM01 Dynamic Punch
    assert(classicMachineMove(ClassicGame::Ruby, 289) == 264);   // TM01 Focus Punch
    assert(classicMachineMove(ClassicGame::FireRed, 289) == 264);
    // Four Gen V-era IDs had leaked into the original Gen III machine table.
    // All GBA titles must use only moves from their own retail-era TM/HM pool.
    for (auto g : {ClassicGame::Ruby, ClassicGame::Sapphire,
                   ClassicGame::Emerald, ClassicGame::FireRed, ClassicGame::LeafGreen}) {
        assert(classicMachineMove(g, 298) == 237); // TM10 Hidden Power, not later-gen Volt Switch
        assert(classicMachineMove(g, 331) == 290); // TM43 Secret Power, never Flame Charge
        assert(classicMachineMove(g, 337) == 289); // TM49 Snatch, never Scald
        assert(classicMachineMove(g, 346) == 291); // HM08 Dive, not Shadow Force
        assert(displayItemName(g, ClassicPocket::TMHM, 331) == "TM43 — Secret Power");
        assert(displayItemName(g, ClassicPocket::PCItems, 331) == "TM43 — Secret Power");
        assert(displayItemName(g, ClassicPocket::PCItems, 337) == "TM49 — Snatch");
    }
    // Native machine identity + move names must also work in Gen I and II PC storage.
    assert(displayItemName(ClassicGame::Yellow, ClassicPocket::PCItems, 201) ==
           "TM01 — Mega Punch");
    assert(displayItemName(ClassicGame::Crystal, ClassicPocket::PCItems, 193) ==
           "TM03 — Curse");
    assert(classicMachineMove(ClassicGame::Yellow, 201) != classicMachineMove(ClassicGame::Gold, 191));
    assert(displayItemName(ClassicGame::Yellow, ClassicPocket::TMHM, 201).find("TM01") == 0);
    assert(displayItemName(ClassicGame::Gold, ClassicPocket::TMHM, 191).find("TM01") == 0);
    assert(displayItemName(ClassicGame::FireRed, ClassicPocket::TMHM, 289).find("TM01") == 0);

    // Exhaustive retail-era machine coverage (including PC Items). The same
    // native machine ID must teach the same in-generation move in both picker
    // and PC views. Modern TM assignments must never contaminate Gen III.
    for (auto game : {ClassicGame::Red,ClassicGame::Blue,ClassicGame::Yellow,
                      ClassicGame::Gold,ClassicGame::Silver,ClassicGame::Crystal,
                      ClassicGame::Ruby,ClassicGame::Sapphire,ClassicGame::Emerald,
                      ClassicGame::FireRed,ClassicGame::LeafGreen}) {
        const auto machines=addableItems(game,ClassicPocket::TMHM);
        const std::size_t expected=game<=ClassicGame::Yellow ? 55 :
                                   game<=ClassicGame::Crystal ? 57 : 58;
        assert(machines.size()==expected);
        const uint16_t maxMove=game<=ClassicGame::Yellow ? 165 :
                               game<=ClassicGame::Crystal ? 251 : 354;
        for(const auto nativeId:machines) {
            const uint16_t move=classicMachineMove(game,nativeId);
            assert(move>0 && move<=maxMove);
            const auto bagName=displayItemName(game,ClassicPocket::TMHM,nativeId);
            const auto pcName=displayItemName(game,ClassicPocket::PCItems,nativeId);
            assert(bagName==pcName);
            assert(bagName.find("TM")==0 || bagName.find("HM")==0);
            assert(bagName.find(" — ")!=std::string::npos);
        }
    }

    std::cout << "classic inventory catalog: PASS\n";
    return 0;
}
