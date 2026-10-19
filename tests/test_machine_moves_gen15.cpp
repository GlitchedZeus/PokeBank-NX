// R2-BUG-005: per-game TM/HM -> move table and that generation's move facts, all 20 Gen I-V games.
// Expected values are the pinned sources cited in include/Inventory/MachineMoves.h.
#include "Inventory/MachineMoves.h"

#include "Inventory/ClassicInventoryCatalog.h"
#include "Names/ItemNames.h"
#include "Names/MoveNames.h"
#include "Names/MachineDisplay.h"
#include "UI/ClassicMachineArtwork.h"
#include "UI/ItemSpriteKey.h"

#include <cstdio>
#include <fstream>
#include <set>
#include <string>
#include <initializer_list>
#include <string_view>

using namespace PokeVault::Inventory::Machines;
using G = PokeVault::Inventory::QuantityPolicy::Game;
namespace QP = PokeVault::Inventory::QuantityPolicy;

static int failures = 0;
#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (!(cond)) {                                                                    \
            ++failures;                                                                   \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                                 \
    } while (0)

static void expectMachine(G game, uint16_t item, Kind kind, uint16_t number, uint16_t move,
                          std::string_view name) {
    const auto m = machine(game, item);
    CHECK(static_cast<bool>(m));
    CHECK(m.kind == kind && m.number == number && m.moveId == move);
    const auto d = machineMoveDetail(game, item);
    CHECK(d.valid && d.moveId == move && d.name == name);
    if (!(m.kind == kind && m.number == number && m.moveId == move && d.name == name))
        std::fprintf(stderr, "  ^ game=%d item=%u got kind=%d no=%u move=%u name=%.*s\n",
                     static_cast<int>(game), item, static_cast<int>(m.kind), m.number, m.moveId,
                     static_cast<int>(d.name.size()), d.name.data());
}

static void sameNumberDifferentMoves() {
    // TM01 per generation.
    for (G g : {G::Red, G::Blue, G::Yellow}) expectMachine(g, 201, Kind::TM, 1, 5, "Mega Punch");
    for (G g : {G::Gold, G::Silver, G::Crystal}) expectMachine(g, 191, Kind::TM, 1, 223, "Dynamic Punch");
    for (G g : {G::Ruby, G::Sapphire, G::Emerald, G::FireRed, G::LeafGreen})
        expectMachine(g, 289, Kind::TM, 1, 264, "Focus Punch");
    for (G g : {G::Diamond, G::Pearl, G::Platinum, G::HeartGold, G::SoulSilver})
        expectMachine(g, 328, Kind::TM, 1, 264, "Focus Punch");
    for (G g : {G::Black, G::White, G::Black2, G::White2}) expectMachine(g, 328, Kind::TM, 1, 468, "Hone Claws");
    // HM05 per generation (and Gen IV per game).
    expectMachine(G::Red, 200, Kind::HM, 5, 148, "Flash");
    expectMachine(G::Crystal, 247, Kind::HM, 5, 148, "Flash");
    expectMachine(G::Crystal, 249, Kind::HM, 7, 127, "Waterfall");
    expectMachine(G::Emerald, 343, Kind::HM, 5, 148, "Flash");
    expectMachine(G::FireRed, 346, Kind::HM, 8, 291, "Dive");
    for (G g : {G::Diamond, G::Pearl, G::Platinum}) expectMachine(g, 424, Kind::HM, 5, 432, "Defog");
    for (G g : {G::HeartGold, G::SoulSilver}) expectMachine(g, 424, Kind::HM, 5, 250, "Whirlpool");
    for (G g : {G::Black, G::White, G::Black2, G::White2}) {
        expectMachine(g, 424, Kind::HM, 5, 127, "Waterfall");
        expectMachine(g, 425, Kind::HM, 6, 291, "Dive");
        expectMachine(g, 618, Kind::TM, 93, 528, "Wild Charge");
        expectMachine(g, 619, Kind::TM, 94, 249, "Rock Smash");
        expectMachine(g, 620, Kind::TM, 95, 555, "Snarl");
        expectMachine(g, 419, Kind::TM, 92, 433, "Trick Room");
        CHECK(!machine(g, 426));  // Gen V has only six HMs
        CHECK(!machine(g, 427));
    }
    // Gen IV TM92 / HM08.
    expectMachine(G::Platinum, 419, Kind::TM, 92, 433, "Trick Room");
    expectMachine(G::HeartGold, 427, Kind::HM, 8, 431, "Rock Climb");
    // Non-machines and out-of-range ids.
    for (std::size_t i = 0; i < QP::kGameCount; ++i) {
        const auto g = static_cast<G>(i);
        CHECK(!machine(g, 0));
        CHECK(!machine(g, 0xFFFF));
        CHECK(!machineMoveDetail(g, 0).valid);
    }
    CHECK(!machine(G::Red, 20));     // Potion
    CHECK(!machine(G::Gold, 195));   // gap in the Gen II machine ids
    CHECK(!machine(G::Gold, 220));
    CHECK(!machine(G::Emerald, 347));
    CHECK(!machine(G::Diamond, 428));
}

static void everyGameEveryMachine() {
    const std::size_t expected[] = {55, 55, 55, 57, 57, 57, 58, 58, 58, 58, 58,
                                    100, 100, 100, 100, 100, 101, 101, 101, 101};
    for (std::size_t i = 0; i < QP::kGameCount; ++i) {
        const auto g = static_cast<G>(i);
        const auto items = machineItems(g);
        CHECK(items.size() == expected[i]);
        uint16_t tm = 0, hm = 0;
        bool hmSeen = false;
        for (const auto item : items) {
            const auto m = machine(g, item);
            CHECK(static_cast<bool>(m));
            if (m.kind == Kind::TM) {
                CHECK(!hmSeen);               // TMs first
                CHECK(m.number == ++tm);      // TM01, TM02, ... in order
            } else {
                hmSeen = true;
                CHECK(m.number == ++hm);
            }
            const auto d = machineMoveDetail(g, item);
            CHECK(d.valid && !d.name.empty());
            CHECK(d.pp > 0);
            // Every machine's item is a native TM/HM pocket member of this exact game.
            CHECK(QP::quantityRule(g, QP::Pocket::TMHM, item).valid);
        }
        // The full native TM/HM pocket is covered (no machine missing from the table).
        std::size_t pocketMachines = 0;
        for (uint32_t id = 1; id < 1000; ++id)
            if (QP::itemKind(g, static_cast<uint16_t>(id)) == QP::ItemKind::TM ||
                QP::itemKind(g, static_cast<uint16_t>(id)) == QP::ItemKind::HM)
                ++pocketMachines;
        CHECK(pocketMachines == items.size());
    }
}

static void generationFacts() {
    // Bite: Normal in Gen I, Dark (special by type) in Gen II/III, Dark physical in Gen IV/V.
    {
        const auto d = moveDetail(G::Red, 44);
        CHECK(d.type == 0 && d.categoryByType && d.category == Category::Physical);
        CHECK(d.powerKind == PowerKind::Fixed && d.power == 60 && d.accuracy == 100 && d.pp == 25);
    }
    for (G g : {G::Gold, G::Emerald}) {
        const auto d = moveDetail(g, 44);
        CHECK(d.type == 16 && d.categoryByType && d.category == Category::Special);
    }
    for (G g : {G::Platinum, G::Black}) {
        const auto d = moveDetail(g, 44);
        CHECK(d.type == 16 && !d.categoryByType && d.category == Category::Physical);
    }
    // Karate Chop: Normal in Gen I, Fighting from Gen II.
    CHECK(moveDetail(G::Yellow, 2).type == 0);
    CHECK(moveDetail(G::Crystal, 2).type == 1);
    // Explosion: 170 in Gen I, 250 from Gen II.
    CHECK(moveDetail(G::Blue, 153).power == 170);
    CHECK(moveDetail(G::Silver, 153).power == 250);
    // Non-damaging moves have no power; variable-damage moves are not 0 power.
    {
        const auto sd = moveDetail(G::Red, 14);
        CHECK(sd.powerKind == PowerKind::None && sd.category == Category::Status);
        const auto toss = moveDetail(G::Red, 69);
        CHECK(toss.powerKind == PowerKind::Variable && toss.category == Category::Physical);
        const auto sdV = moveDetail(G::White2, 14);
        CHECK(sdV.powerKind == PowerKind::None && sdV.category == Category::Status);
    }
    // Hypnosis: 70% in Diamond/Pearl only.
    CHECK(moveDetail(G::Diamond, 95).accuracy == 70 && moveDetail(G::Pearl, 95).accuracy == 70);
    CHECK(moveDetail(G::Platinum, 95).accuracy == 60 && moveDetail(G::HeartGold, 95).accuracy == 60);
    // Curse is "???" and non-damaging in Gen II.
    {
        const auto curse = moveDetail(G::Gold, 174);
        CHECK(curse.type == kTypeUnknown && curse.category == Category::Status && curse.powerKind == PowerKind::None);
    }
    // Hidden Power: absent in Gen I; type and power vary from Gen II.
    CHECK(!moveDetail(G::Red, 237).valid);
    for (G g : {G::Gold, G::Ruby, G::Diamond, G::Black}) {
        const auto hp = moveDetail(g, 237);
        CHECK(hp.valid && hp.typeVaries && hp.powerKind == PowerKind::Variable);
    }
    // Never-miss moves (Aerial Ace / Swift from Gen III).
    CHECK(moveDetail(G::Emerald, 332).alwaysHits);
    CHECK(moveDetail(G::Black, 129).alwaysHits);
    // Move id ranges per generation.
    CHECK(moveDetail(G::Red, 165).valid && !moveDetail(G::Red, 166).valid);
    CHECK(moveDetail(G::Gold, 251).valid && !moveDetail(G::Gold, 252).valid);
    CHECK(moveDetail(G::FireRed, 354).valid && !moveDetail(G::FireRed, 355).valid);
    CHECK(moveDetail(G::SoulSilver, 467).valid && !moveDetail(G::SoulSilver, 468).valid);
    CHECK(moveDetail(G::White, 559).valid && !moveDetail(G::White, 560).valid);
    CHECK(!moveDetail(G::Black, 0).valid);
    // R/S/E use the same Gen III facts as FR/LG.
    for (uint16_t move = 1; move <= 354; ++move) {
        const auto e = moveDetail(G::Emerald, move);
        const auto f = moveDetail(G::FireRed, move);
        CHECK(e.type == f.type && e.power == f.power && e.accuracy == f.accuracy && e.pp == f.pp);
    }
    // B2W2 TMs/HMs are the BW ones.
    for (const auto item : machineItems(G::Black))
        CHECK(machine(G::Black, item).moveId == machine(G::White2, item).moveId);
}

// The app has ONE machine table (Names::getMachineDescriptor / getTMMove); the classic bag's own
// copy (ClassicInventoryCatalog classicMachineMove) must agree with it for every Gen I-III machine.
static void oneMachineTable() {
    for (std::size_t i = 0; i <= static_cast<std::size_t>(G::LeafGreen); ++i) {
        const auto g = static_cast<G>(i);
        for (const auto item : machineItems(g))
            CHECK(machine(g, item).moveId ==
                  PokeVault::Inventory::classicMachineMove(static_cast<PokeVault::Inventory::ClassicGame>(i), item));
    }
}

static std::set<std::string> pinnedArt() {
    std::ifstream in("tools/pinned_item_sprite_names.txt");
    std::set<std::string> out;
    for (std::string line; std::getline(in, line);) if (!line.empty()) out.insert(line);
    return out;
}

// QA Part 9 (0faf7e9c): every Gen V TM/HM has its exact label and a real disc icon; ids 420-425 are
// HM01-06 in Gen V while BD/SP keeps TM93-98; the seven Gen V feathers resolve to pinned art.
static void gen5LabelsAndIcons() {
    const auto art = pinnedArt();
    CHECK(art.size() > 500);
    using Enums::GameVersion;
    for (const auto version : {GameVersion::BW, GameVersion::B2W2, GameVersion::B, GameVersion::W,
                               GameVersion::B2, GameVersion::W2}) {
        const G game = version == GameVersion::B || version == GameVersion::BW ? G::Black
                     : version == GameVersion::W ? G::White
                     : version == GameVersion::B2 || version == GameVersion::B2W2 ? G::Black2 : G::White2;
        for (const auto item : machineItems(game)) {
            const auto m = machine(game, item);
            char expected[8];
            std::snprintf(expected, sizeof expected, "%s%02u", m.kind == Kind::HM ? "HM" : "TM", m.number);
            const std::string label = Names::machineDisplayLabel(version, item, Names::getItemName(item));
            CHECK(label.rfind(expected, 0) == 0);
            CHECK(label.find(Names::getMoveName(m.moveId)) != std::string::npos);
            const std::string key = UI::ClassicMachineArtwork::artKey(version, item);
            CHECK(!key.empty() && art.count(key) == 1);
            CHECK(key.rfind(m.kind == Kind::HM ? "hm-" : "tm-", 0) == 0);
            if (key.empty() || art.count(key) != 1 || label.rfind(expected, 0) != 0)
                std::fprintf(stderr, "  ^ item=%u label='%s' art='%s'\n", item, label.c_str(), key.c_str());
        }
    }
    // Same native id 420, different games: HM01 in Gen V, TM93 in BD/SP (unchanged).
    CHECK(Names::machineDisplayLabel(GameVersion::B2W2, 420, Names::getItemName(420)).rfind("HM01", 0) == 0);
    CHECK(Names::machineDisplayLabel(GameVersion::BW, 425, Names::getItemName(425)).rfind("HM06", 0) == 0);
    CHECK(Names::machineDisplayLabel(GameVersion::BDSP, 420, Names::getItemName(420)).rfind("TM093", 0) == 0 ||
          Names::machineDisplayLabel(GameVersion::BDSP, 420, Names::getItemName(420)).rfind("TM93", 0) == 0);
    CHECK(std::string(Names::getItemName(420)) == "TM93");  // shared BD/SP row text untouched
    // Gen V TM93-95 are 618-620, the same disc art rule as every other TM.
    CHECK(UI::ClassicMachineArtwork::artKey(GameVersion::BW, 618) == "tm-electric");  // Wild Charge
    CHECK(UI::ClassicMachineArtwork::artKey(GameVersion::B2W2, 620) == "tm-dark");    // Snarl
    // Gen V feathers 565-571 resolve to the pinned *-wing art.
    for (uint16_t id = 565; id <= 571; ++id) {
        const auto slug = UI::ItemSpriteKey::slug(Names::getItemName(id));
        CHECK(art.count(slug) == 1);
        CHECK(slug.size() > 5 && slug.compare(slug.size() - 5, 5, "-wing") == 0);
    }
}

int main() {
    sameNumberDifferentMoves();
    everyGameEveryMachine();
    generationFacts();
    oneMachineTable();
    gen5LabelsAndIcons();
    if (failures != 0) {
        std::fprintf(stderr, "test_machine_moves_gen15: %d failure(s)\n", failures);
        return 1;
    }
    std::puts("test_machine_moves_gen15: per-game TM/HM moves and move facts PASS (20 games)");
    return 0;
}
