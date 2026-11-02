#pragma once

// ONE "is this an initialized game save?" rule, Generation I-V (Round 15, owner decision Nov 2:
// "A file without an initialized game save or valid trainer information should not be treated as
// an editable save merely because the emulator created a file. However, don't reject valid
// initialized saves just because one optional trainer field is empty. Validate against each game's
// native format and initialization requirements. Creating a completely new save must be a
// separate, explicitly supported operation.").
//
// Two layers, both required:
//  1. The game's own native validity marker, already enforced by each strict parser before a save
//     is ever "ready". These are how the games themselves tell "no save" from "a save":
//       Gen I   (pokered engine/menus/save.asm LoadSAV / SAVCheckSum): the main-data checksum at
//               sMainDataCheckSum must match, otherwise the game shows no CONTINUE.
//       Gen II  (pokecrystal engine/menus/save.asm CheckPrimarySaveFile / ValidateSave): the game
//               checks SAVE_CHECK_VALUE_1/2 and the main-data checksum; PokeBank's strict parser
//               (Gen2ReadOnlySave.cpp checksumValid) enforces the 0x2009.. main-data checksum.
//       Gen III (pokeemerald src/save.c GetSaveValidStatus): every sector footer carries the
//               0x08012025 signature and a valid sector checksum.
//       Gen IV  (pret/pokeplatinum save footers, PKHeX SAV4): General / Storage block footers carry
//               the block size, the 0x20060623 (international) magic and a CRC-16-CCITT.
//       Gen V   (PKHeX SAV5 BlockInfo): every block's CRC-16-CCITT and its footer mirror match.
//     An emulator-created battery file (all 0x00 / all 0xFF, or a size-only stub) fails these.
//  2. Trainer initialization (this header): a structurally valid save whose trainer was never
//     written -- NO name AND TID 0 AND SID 0 AND play time 0:00:00 (or no valid timer) -- is the
//     state a game leaves before the player names the character (Gen V writes valid blocks on
//     first boot before naming; see Gen5TrainerName.h newGameNoTrainer5, observed on hardware as
//     PBX-TRN-003). It is NOT an editable save. All four must be empty: a real save that merely
//     lacks one optional field (TID 0, SID 0, an unusual name, 0:00 timer) still opens.
// Pure, no I/O. Creating a brand-new save is a separate operation PokeBank does not offer here.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "Integration/Common/NativePlayTime.h"

namespace PokeVault::Source::SaveInitialization {

inline constexpr std::string_view kNotInitializedReason =
    "not an initialized save: the game was never saved with a trainer";

struct TrainerFacts {
    std::size_t nameLength = 0;
    uint32_t tid = 0;
    uint32_t sid = 0;
    bool playTimeZero = true;   // 0:00:00, or no valid timer at all
};

[[nodiscard]] constexpr bool initialized(const TrainerFacts& facts) noexcept {
    return !(facts.nameLength == 0 && facts.tid == 0 && facts.sid == 0 && facts.playTimeZero);
}

[[nodiscard]] inline bool zeroTime(const std::optional<Integration::Common::NativePlayTime>& time) noexcept {
    return !time || (time->hours == 0 && time->minutes == 0 && time->seconds == 0 && !time->maximum);
}

// Gen I / II / III TrainerRecord (name is the decoded string, empty at a leading terminator).
template <class Record>
[[nodiscard]] TrainerFacts classicFacts(const Record& trainer, uint32_t tid, uint32_t sid = 0) {
    return {trainer.name.size(), tid, sid, zeroTime(trainer.playTime)};
}

// Gen IV TrainerReadOnly / Gen V Trainer5View-like records (UTF-16 names, h/m/s timer).
[[nodiscard]] constexpr TrainerFacts dsFacts(std::size_t nameLength, uint32_t tid, uint32_t sid,
                                             uint32_t hours, uint32_t minutes, uint32_t seconds) noexcept {
    return {nameLength, tid, sid, hours == 0 && minutes == 0 && seconds == 0};
}

} // namespace PokeVault::Source::SaveInitialization
