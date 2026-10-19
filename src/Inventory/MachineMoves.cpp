#include "Inventory/MachineMoves.h"

#include "Enums/GameVersion.h"
#include "Inventory/ClassicInventoryCatalog.h"
#include "Names/MoveBattleStats.h"
#include "Names/MoveBattleStats5.h"
#include "Names/MoveNames.h"
#include "Names/MoveTypes34.h"
#include "Names/MachineDisplay.h"

#include <algorithm>
#include <array>

namespace PokeVault::Inventory::Machines {
namespace {

namespace QP = QuantityPolicy;
using Enums::GameVersion;

// Display order of each family's machine items (TMs, then HMs). Item ids are the native ids the
// repo's catalogs use: ClassicInventoryCatalog kG1Machines / kG2Machines / kG3Machines,
// Gen4BagCatalog (328-427), Gen5Bag BagCatalog5::kMachine.
template <std::size_t N>
constexpr std::array<uint16_t, N> range(uint16_t first) {
    std::array<uint16_t, N> out{};
    for (std::size_t i = 0; i < N; ++i) out[i] = static_cast<uint16_t>(first + i);
    return out;
}
constexpr auto kGen1Items = [] {
    std::array<uint16_t, 55> out{};
    for (std::size_t i = 0; i < 50; ++i) out[i] = static_cast<uint16_t>(201 + i);  // TM01-50
    for (std::size_t i = 0; i < 5; ++i) out[50 + i] = static_cast<uint16_t>(196 + i);  // HM01-05
    return out;
}();
constexpr auto kGen3Items = range<58>(289);  // TM01-50 = 289-338, HM01-08 = 339-346
constexpr auto kGen4Items = range<100>(328); // TM01-92 = 328-419, HM01-08 = 420-427
constexpr auto kGen5Items = [] {
    std::array<uint16_t, 101> out{};
    for (std::size_t i = 0; i < 92; ++i) out[i] = static_cast<uint16_t>(328 + i);  // TM01-92
    out[92] = 618; out[93] = 619; out[94] = 620;                                   // TM93-95
    for (std::size_t i = 0; i < 6; ++i) out[95 + i] = static_cast<uint16_t>(420 + i);  // HM01-06
    return out;
}();

ClassicGame classic(Game game) noexcept { return static_cast<ClassicGame>(static_cast<uint8_t>(game)); }

GameVersion group(Game game) noexcept {
    switch (game) {
        case Game::Red: case Game::Blue: case Game::Yellow: return GameVersion::RBY;
        case Game::Gold: case Game::Silver: case Game::Crystal: return GameVersion::GSC;
        case Game::Ruby: case Game::Sapphire: case Game::Emerald:
        case Game::FireRed: case Game::LeafGreen: return GameVersion::FRLG;  // shared Gen III data
        case Game::Diamond: case Game::Pearl: return GameVersion::DP;
        case Game::Platinum: return GameVersion::PT;
        case Game::HeartGold: case Game::SoulSilver: return GameVersion::HGSS;
        case Game::Black: case Game::White: return GameVersion::BW;
        case Game::Black2: case Game::White2: return GameVersion::B2W2;
    }
    return GameVersion::Invalid;
}

uint16_t maxMove(int generation) noexcept {
    switch (generation) {
        case 1: return 165;
        case 2: return 251;
        case 3: return 354;
        case 4: return 467;
        default: return 559;
    }
}

// Gen I-III have no per-move category: damaging moves are physical or special by type.
// Physical: Normal, Fighting, Flying, Poison, Ground, Rock, Bug, Ghost, Steel (PKHeX 0-8).
constexpr Category categoryForType(uint8_t type) noexcept {
    return type <= 8 ? Category::Physical : Category::Special;
}

constexpr uint16_t kHiddenPower = 237;

} // namespace

// ONE machine table for the whole app: Names::getMachineDescriptor / Names::getTMMove (the same
// data the bag labels and machine-disc art use). R/S/E resolve through the shared Gen III (FRLG)
// group. An id counts only when it is a native TM/HM of this exact game (ItemQuantityPolicy).
Machine machine(Game game, uint16_t itemId) noexcept {
    if (static_cast<std::size_t>(game) >= QP::kGameCount || itemId == 0) return {};
    const auto kind = QP::itemKind(game, itemId);
    if (kind != QP::ItemKind::TM && kind != QP::ItemKind::HM) return {};
    const auto d = Names::getMachineDescriptor(group(game), itemId);
    if (!d || (d.kind != Names::MachineKind::TM && d.kind != Names::MachineKind::HM)) return {};
    return {d.kind == Names::MachineKind::TM ? Kind::TM : Kind::HM, d.number, d.moveId};
}

std::span<const uint16_t> machineItems(Game game) noexcept {
    if (static_cast<std::size_t>(game) >= QP::kGameCount) return {};
    switch (QP::generation(game)) {
        case 1: return kGen1Items;
        case 2: return addableItems(classic(game), ClassicPocket::TMHM);  // TM01-50 then HM01-07
        case 3: return kGen3Items;
        case 4: return kGen4Items;
        default: return kGen5Items;
    }
}

MoveDetail moveDetail(Game game, uint16_t moveId) noexcept {
    MoveDetail d;
    if (static_cast<std::size_t>(game) >= QP::kGameCount) return d;
    const int gen = QP::generation(game);
    if (moveId == 0 || moveId > maxMove(gen)) return d;
    d.valid = true;
    d.moveId = moveId;
    if (const char* name = Names::getMoveName(moveId)) d.name = name;
    d.typeVaries = moveId == kHiddenPower && gen >= 2;

    if (gen == 5) {
        const auto* s = Names::getMoveBattleStats5(moveId);
        if (!s) return MoveDetail{};
        d.type = s->type;
        d.category = s->category == 0 ? Category::Physical
                   : s->category == 1 ? Category::Special : Category::Status;
        if (d.category == Category::Status) d.powerKind = PowerKind::None;
        else if (s->power == Names::kMoveValueUndefined5 || d.typeVaries) d.powerKind = PowerKind::Variable;
        else { d.powerKind = PowerKind::Fixed; d.power = s->power; }
        d.alwaysHits = s->accuracy == Names::kMoveValueUndefined5;
        d.accuracy = d.alwaysHits ? 0 : s->accuracy;
        d.pp = s->pp;
        return d;
    }

    // Gen I-IV decomp tables: power 0 = no power (non-damaging), 1 = damaging but not a base-power
    // value; accuracy 0 = never misses. D/P Hypnosis (70%, 60% elsewhere) comes from getMoveBattleStats(DP).
    const auto* s = Names::getMoveBattleStats(moveId, group(game));
    if (!s) return MoveDetail{};
    d.pp = s->pp;
    d.alwaysHits = s->accuracy == 0;
    d.accuracy = s->accuracy;
    if (s->power == 0) d.powerKind = PowerKind::None;
    else if (s->power == 1) d.powerKind = PowerKind::Variable;
    else { d.powerKind = PowerKind::Fixed; d.power = s->power; }

    if (gen == 4) {
        const auto& tc = Names::kMoveTypeCategory4[moveId];
        d.type = tc.type;
        d.category = tc.category == 0 ? Category::Physical
                   : tc.category == 1 ? Category::Special : Category::Status;
        return d;
    }
    d.type = gen == 1 ? Names::kMoveType1[moveId] : gen == 2 ? Names::kMoveType2[moveId]
                                                             : Names::kMoveType3[moveId];
    d.categoryByType = true;
    d.category = d.powerKind == PowerKind::None || d.type == kTypeUnknown ? Category::Status
                                                                          : categoryForType(d.type);
    return d;
}

MoveDetail machineMoveDetail(Game game, uint16_t itemId) noexcept {
    const auto m = machine(game, itemId);
    return m ? moveDetail(game, m.moveId) : MoveDetail{};
}

} // namespace PokeVault::Inventory::Machines
