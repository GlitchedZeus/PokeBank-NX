#ifndef POKEBANK_INVENTORY_MACHINE_MOVES_H
#define POKEBANK_INVENTORY_MACHINE_MOVES_H

// R2-BUG-005: per-game TM/HM -> move table for all 20 Gen I-V games, plus that generation's move
// facts (name, type, category, power, accuracy, PP) for the shared grid item picker's detail
// area (Lane P renders it). Pure query API: no UI, no save I/O.
//
// Sources (pinned; each table is cited where it is defined):
//  * Gen I    pret pokered d2704a63 (ClassicInventoryCatalog kG1MachineMoves; MoveBattleStats Gen1,
//             MoveTypes34 kMoveType1)
//  * Gen II   pret pokecrystal e058e4f5 (kG2MachineMoves; MoveBattleStats Gen2, kMoveType2)
//  * Gen III  pret pokeemerald c587cc9e (kG3MachineMoves - R/S/E and FR/LG share TM01-50 / HM01-08;
//             MoveBattleStats Gen3, kMoveType3)
//  * Gen IV   PKHeX TMMoves (Names/TMMoves.cpp GEN4_TM_MOVES, HM05 Defog in D/P/Pt, Whirlpool in
//             HG/SS); pret pokeheartgold 9d8b7591 waza_tbl (MoveBattleStats Gen4; D/P Hypnosis is 70%, 60% elsewhere)
//  * Gen V    PKHeX 6501f0ab PersonalInfo5BW.MachineMoves (TM01-95, HM01-06; B2W2 uses the same
//             list), item ids from Gen5Bag.h BagCatalog5::kMachine; MoveBattleStats5 (PKHeX
//             MoveInfo5 + Showdown 3a766f2c) for the move facts
// What differs by generation is handled here, never by the caller: the same TM number teaches
// different moves (TM01 is Mega Punch in Gen I, DynamicPunch in Gen II, Focus Punch in Gen III/IV,
// Hone Claws in Gen V); Gen I/II types (Bite is Normal in Gen I, Dark from Gen II); Gen I/II
// power/accuracy values; the Gen I-III type-based physical/special split.

#include "Inventory/ItemQuantityPolicy.h"

#include <cstdint>
#include <span>
#include <string_view>

namespace PokeVault::Inventory::Machines {

using QuantityPolicy::Game;

enum class Kind : uint8_t { None, TM, HM };

struct Machine {
    Kind kind = Kind::None;
    uint16_t number = 0;  // TM05 -> 5, HM03 -> 3
    uint16_t moveId = 0;  // the move this machine teaches in this exact game
    [[nodiscard]] explicit constexpr operator bool() const noexcept {
        return kind != Kind::None && moveId != 0;
    }
};

// None for an item id that is not a TM/HM in this game.
[[nodiscard]] Machine machine(Game game, uint16_t itemId) noexcept;

// The game's machine item ids in display order: TM01.. then HM01..
[[nodiscard]] std::span<const uint16_t> machineItems(Game game) noexcept;

enum class Category : uint8_t { Physical, Special, Status };
enum class PowerKind : uint8_t {
    None,      // non-damaging: no power ("—")
    Fixed,     // `power` is the base power
    Variable,  // damaging, but not a base-power value (Seismic Toss, Hidden Power, Return, ...)
};

inline constexpr uint8_t kTypeUnknown = 255;  // "???" (Curse in Gen II-IV)

struct MoveDetail {
    bool valid = false;
    uint16_t moveId = 0;
    std::string_view name;            // English move name (modern spelling)
    uint8_t type = kTypeUnknown;      // PKHeX type order: 0 Normal .. 8 Steel .. 16 Dark, 17 Fairy
    bool typeVaries = false;          // Hidden Power (Gen II+): type comes from IVs
    Category category = Category::Status;
    bool categoryByType = false;      // Gen I-III: physical/special follows the move's type
    PowerKind powerKind = PowerKind::None;
    uint8_t power = 0;                // meaningful only when powerKind == Fixed
    bool alwaysHits = false;          // never misses / no accuracy check
    uint8_t accuracy = 0;             // percent; meaningful only when !alwaysHits
    uint8_t pp = 0;                   // base PP in this generation
};

// That generation's facts for a move id (invalid for 0 or an id the generation does not have).
[[nodiscard]] MoveDetail moveDetail(Game game, uint16_t moveId) noexcept;

// machine(game, itemId) -> moveDetail(game, moveId); invalid for a non-machine item.
[[nodiscard]] MoveDetail machineMoveDetail(Game game, uint16_t itemId) noexcept;

} // namespace PokeVault::Inventory::Machines

#endif
