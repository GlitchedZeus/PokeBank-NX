#ifndef POKEBANK_GEN7_BATTLE_TEAMS_H
#define POKEBANK_GEN7_BATTLE_TEAMS_H

#include "Integration/Gen7/Gen7Readouts.h"
#include "Integration/Gen7/Gen7Save.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

// Sun/Moon/Ultra Sun/Ultra Moon Battle Box teams, read-only. Source, pinned PKHeX 6501f0ab
// (verified for the PKSE 1.2.2 audit, which flagged that Gen VII had no team guard):
//   Saves/Substructures/Gen7/BoxLayout7.cs, block 13 (SaveBlockAccessor7SM/7USUM `Block(sav, 13)`):
//     BattleBoxFlags 0x4C4: 6 teams x 6 signed int16, (box << 8) | slot, negative = empty slot.
//     GetIsTeamLocked(team): byte PCBackgrounds(0x5C0) - TeamCount(6) - team == 1.
//   Saves/SAV7.cs ReloadBattleTeams: a Sun/Moon demo save (BoxLayout bytes [0, 0x4C4) all zero) has no
//     teams; GetBoxSlotFlags marks every referenced slot as a battle-team slot (+ Locked when locked).
// A pointer outside 32 boxes x 30 slots makes the whole team table unreadable, and callers then refuse
// every box storage change (fail closed), exactly like the Sword/Shield TeamIndexes8 guard.
namespace PokeVault::Integration::Gen7::BattleTeams {

inline constexpr size_t TeamCount = 6;
inline constexpr size_t TeamSize = 6;
inline constexpr size_t BattleBoxFlags = 0x4C4;
inline constexpr size_t PCBackgrounds = 0x5C0;

struct Member {
    size_t team = 0, box = 0, slot = 0;
    bool locked = false;
};

struct Table {
    bool readable = false;
    std::vector<Member> members;
    [[nodiscard]] const Member* at(size_t box, size_t slot) const noexcept {
        const Member* found = nullptr;
        for (const auto& m : members)
            if (m.box == box && m.slot == slot && (!found || (m.locked && !found->locked))) found = &m;
        return found;
    }
};

[[nodiscard]] inline Table read(const Gen7Save& save) {
    Table out;
    const auto& block = Layout::blocks(save.family())[Readouts::BoxLayoutBlock];
    const auto bytes = save.bytes();
    if (block.offset + PCBackgrounds > bytes.size() || block.offset + BattleBoxFlags + TeamCount * TeamSize * 2 > bytes.size())
        return out;   // unreadable
    if (save.family() == Layout::Family::SunMoon) {
        bool demo = true;
        for (size_t i = 0; i < BattleBoxFlags && demo; ++i) demo = bytes[block.offset + i] == 0;
        if (demo) {
            out.readable = true;   // PKHeX: demo saves carry no team data
            return out;
        }
    }
    for (size_t i = 0; i < TeamCount * TeamSize; ++i) {
        const auto raw = static_cast<int16_t>(PK7::read16(bytes, block.offset + BattleBoxFlags + 2 * i));
        if (raw < 0) continue;
        const size_t box = static_cast<size_t>(raw) >> 8, slot = static_cast<size_t>(raw) & 0xFF;
        if (box >= Layout::BoxCount || slot >= Layout::BoxSlots) {
            out.members.clear();
            return out;   // unreadable: refuse every box storage change
        }
        const size_t team = i / TeamSize;
        out.members.push_back({team, box, slot, bytes[block.offset + PCBackgrounds - TeamCount - team] == 1});
    }
    out.readable = true;
    return out;
}

} // namespace PokeVault::Integration::Gen7::BattleTeams

#endif
