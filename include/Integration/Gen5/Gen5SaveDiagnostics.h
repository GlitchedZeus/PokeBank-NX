#ifndef POKEBANK_GEN5_SAVE_DIAGNOSTICS_H
#define POKEBANK_GEN5_SAVE_DIAGNOSTICS_H

#include "Integration/Gen5/Gen5BlankSlot.h"
#include "Integration/Gen5/Gen5Pokedex.h"
#include "Integration/Gen5/Gen5ReadOnlySave.h"
#include "Integration/Gen5/Gen5SaveBlockLayout.h"
#include "Integration/Gen5/Gen5Trainer.h"
#include "Integration/Gen5/Gen5TrainerName.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace PokeVault::Integration::Gen5 {

// Structured, read-only evidence about BOTH retail copies of a SAV5 image.
// Valid block checksums only prove that a copy is internally consistent; they
// do not prove it holds the player's current game. These states keep a blank
// or stale copy from being shown as "Trainer 0 / Party 0 / Dex 0".
//
// Layout facts (cited, pinned PKHeX 6501f0ab): BW copy 0x24000, B2W2 copy
// 0x26000 (SaveUtil.SIZE_G5BW / SIZE_G5B2W2); block tables per
// SaveBlockAccessor5BW/B2W2; PlayerData block 27, Misc block 52, party block
// 26 (count at +4), boxes from 0x400 with stride 0x1000 (SAV5.GetBoxOffset).
// PKHeX itself always reads the first copy; this code never picks a copy by
// guesswork either -- it reports evidence and the user chooses.
enum class CopyState5 : uint8_t {
    ChecksumInvalid,      // one or more retail blocks fail CRC16-CCITT
    UnknownVersion,       // checksums valid but the game-version byte is not 20..23
    NewGameNoTrainer,     // valid new-game save: no name, TID=SID=0, play time 0, no Pokemon
    NoTrainerIdentity,    // valid, but TID=SID=0 and an empty name: blank/initial copy
    UnreadableTrainerName,// trainer present but the name holds unsupported code units
    InvalidPokemonRecords,// a declared party PK5 fails validation (boxed ones are quarantined)
    GenuinelyEmpty,       // trainer present, no Pokemon anywhere
    Populated,            // trainer present and at least one Pokemon
};

[[nodiscard]] constexpr const char* copyStateLabel5(CopyState5 s) noexcept {
    switch (s) {
        case CopyState5::ChecksumInvalid: return "checksums invalid";
        case CopyState5::UnknownVersion: return "unknown game version";
        case CopyState5::NewGameNoTrainer: return "no trainer data (new-game save, no trainer yet)";
        case CopyState5::NoTrainerIdentity: return "no trainer data (blank or initial copy)";
        case CopyState5::UnreadableTrainerName: return "trainer name unreadable";
        case CopyState5::InvalidPokemonRecords: return "invalid Pokemon records";
        case CopyState5::GenuinelyEmpty: return "valid, no Pokemon yet";
        case CopyState5::Populated: return "valid, populated";
    }
    return "unknown";
}
// A copy the shared editor may open without hiding missing data.
[[nodiscard]] constexpr bool copyOpenable5(CopyState5 s) noexcept {
    return s == CopyState5::Populated || s == CopyState5::GenuinelyEmpty ||
           s == CopyState5::UnreadableTrainerName || s == CopyState5::NewGameNoTrainer;
}

struct CopyReport5 {
    SaveFamily family = SaveFamily::BlackWhite;
    size_t offset = 0;
    size_t blocksValid = 0;
    size_t blocksTotal = 0;
    uint8_t versionByte = 0;
    uint8_t genderByte = 0;
    bool trainerIdPresent = false;     // TID or SID nonzero (values are not reported)
    size_t nameLength = 0;
    bool nameDecodes = false;
    uint8_t partyCountByte = 0;
    size_t validPartyRecords = 0;
    size_t invalidPartyRecords = 0;
    size_t occupiedBoxRecords = 0;
    size_t blankEncryptedBoxSlots = 0; // decrypt to an all-zero PK5 (encrypted blank)
    size_t zeroFilledBoxSlots = 0;     // raw all-zero bytes
    size_t invalidBoxRecords = 0;
    size_t dexSeen = 0, dexCaught = 0;
    uint16_t playedHours = 0;
    uint8_t playedMinutes = 0, playedSeconds = 0;
    CopyState5 state = CopyState5::ChecksumInvalid;
};

struct SaveReport5 {
    std::string container;          // "raw-nds-battery" / "desmume-dsv"
    size_t normalizedSize = 0;
    std::vector<CopyReport5> copies; // only families whose version byte or blocks match
    bool copiesIdentical = false;
    // The copy the shared editor would open with the user's selection, and why.
    std::optional<size_t> chosenIndex;
    std::string chosenReason;
};

namespace detail5diag {
template <size_t N>
[[nodiscard]] inline size_t countValid(std::span<const uint8_t> part,
                                       const std::array<LayoutInfo::SaveBlock, N>& layout) {
    size_t matched = 0;
    for (const auto& b : layout) {
        if (b.offset + b.length > part.size() || b.checksumOffset + 2 > part.size() ||
            b.mirrorOffset + 2 > part.size())
            continue;
        const uint16_t crc = Utils::crc16ccitt(part.data() + b.offset, b.length);
        if (crc == Crypto::read16(part, b.checksumOffset) && crc == Crypto::read16(part, b.mirrorOffset))
            ++matched;
    }
    return matched;
}
[[nodiscard]] inline bool allZero(std::span<const uint8_t> b) {
    for (const auto v : b) if (v) return false;
    return true;
}
} // namespace detail5diag

[[nodiscard]] inline CopyReport5 inspectCopy5(std::span<const uint8_t> image, SaveFamily family,
                                              size_t offset) {
    CopyReport5 r;
    r.family = family;
    r.offset = offset;
    const size_t size = family == SaveFamily::BlackWhite ? LayoutInfo::BlackWhiteCopySize
                                                         : LayoutInfo::Black2White2CopySize;
    if (offset + size > image.size()) return r;
    const auto part = image.subspan(offset, size);
    if (family == SaveFamily::BlackWhite) {
        r.blocksTotal = LayoutInfo::BlackWhite.size();
        r.blocksValid = detail5diag::countValid(part, LayoutInfo::BlackWhite);
    } else {
        r.blocksTotal = LayoutInfo::Black2White2.size();
        r.blocksValid = detail5diag::countValid(part, LayoutInfo::Black2White2);
    }
    const auto& player = trainerBlock5(family, TrainerLayout5::PlayerBlockIndex);
    const auto pd = part.subspan(player.offset, player.length);
    r.versionByte = pd[TrainerLayout5::GameOffset];
    r.genderByte = pd[TrainerLayout5::GenderOffset];
    r.trainerIdPresent = Crypto::read16(pd, TrainerLayout5::TidOffset) != 0 ||
                         Crypto::read16(pd, TrainerLayout5::SidOffset) != 0;
    std::u16string name;
    for (size_t i = 0; i < TrainerLayout5::MaxNameChars; ++i) {
        const uint16_t ch = Crypto::read16(pd, TrainerLayout5::NameOffset + i * 2);
        if (ch == 0 || ch == 0xFFFF) break;
        name.push_back(static_cast<char16_t>(ch));
    }
    r.nameLength = name.size();
    r.nameDecodes = !name.empty() && displayTrainerName(name).has_value();
    r.playedHours = Crypto::read16(pd, TrainerLayout5::HoursOffset);
    r.playedMinutes = pd[TrainerLayout5::MinutesOffset];
    r.playedSeconds = pd[TrainerLayout5::SecondsOffset];
    r.partyCountByte = part[LayoutInfo::PartyOffset + 4];
    for (size_t i = 0; i < LayoutInfo::PartySlots; ++i) {
        const auto rec = part.subspan(LayoutInfo::PartyOffset + 8 + i * Crypto::PartySize, Crypto::PartySize);
        if (i >= r.partyCountByte) continue;
        const Pokemon5ReadOnly pk(rec);
        if (pk.valid() && !pk.empty()) ++r.validPartyRecords;
        else ++r.invalidPartyRecords;
    }
    for (size_t box = 0; box < LayoutInfo::BoxCount; ++box)
        for (size_t slot = 0; slot < LayoutInfo::BoxSlots; ++slot) {
            const auto rec = part.subspan(LayoutInfo::BoxOffset + box * LayoutInfo::BoxStride +
                                          slot * Crypto::StoredSize, Crypto::StoredSize);
            if (detail5diag::allZero(rec)) { ++r.zeroFilledBoxSlots; continue; }
            if (nativeBlankSlot5(rec)) { ++r.blankEncryptedBoxSlots; continue; }
            const Pokemon5ReadOnly pk(rec);
            if (pk.valid() && !pk.empty()) ++r.occupiedBoxRecords;
            else ++r.invalidBoxRecords;
        }
    // Pokedex (Zukan5): caught at +0x08, four seen regions from +0x5C.
    const size_t dexIndex = family == SaveFamily::BlackWhite ? DexLayout5::BlackWhiteBlockIndex
                                                             : DexLayout5::Black2White2BlockIndex;
    const auto& dexBlock = trainerBlock5(family, dexIndex);
    const auto dex = part.subspan(dexBlock.offset, dexBlock.length);
    for (size_t bit = 0; bit < DexLayout5::MaxSpecies; ++bit) {
        const auto flag = [&](size_t ofs) { return (dex[ofs + (bit >> 3)] >> (bit & 7)) & 1u; };
        if (flag(DexLayout5::CaughtOffset)) ++r.dexCaught;
        for (size_t region = 0; region < DexLayout5::SeenRegions; ++region)
            if (flag(DexLayout5::SeenOffset + region * DexLayout5::RegionSize)) { ++r.dexSeen; break; }
    }

    const bool versionOk = family == SaveFamily::BlackWhite ? (r.versionByte == 20 || r.versionByte == 21)
                                                            : (r.versionByte == 22 || r.versionByte == 23);
    if (r.blocksValid != r.blocksTotal) r.state = CopyState5::ChecksumInvalid;
    else if (!versionOk) r.state = CopyState5::UnknownVersion;
    else if (newGameNoTrainer5(r.nameLength, r.trainerIdPresent ? 1 : 0, 0, r.playedHours,
                               r.playedMinutes, r.playedSeconds) &&
             r.partyCountByte == 0 && r.occupiedBoxRecords == 0 && r.invalidBoxRecords == 0)
        r.state = CopyState5::NewGameNoTrainer;
    else if (!r.trainerIdPresent && r.nameLength == 0) r.state = CopyState5::NoTrainerIdentity;
    // Damaged boxed records are quarantined (Gen IV parity) and counted, not
    // fatal; a damaged DECLARED party member makes the copy unreadable.
    else if (r.invalidPartyRecords > 0 || r.partyCountByte > LayoutInfo::PartySlots)
        r.state = CopyState5::InvalidPokemonRecords;
    else if (!r.nameDecodes) r.state = CopyState5::UnreadableTrainerName;
    else if (r.validPartyRecords == 0 && r.occupiedBoxRecords == 0) r.state = CopyState5::GenuinelyEmpty;
    else r.state = CopyState5::Populated;
    return r;
}

// Inspect every plausible copy of a normalized 0x80000 image. A family is
// reported when either copy has a matching version byte or any valid block.
[[nodiscard]] inline SaveReport5 inspectSave5(std::span<const uint8_t> image, std::string container,
                                              SaveCopySelection selection = SaveCopySelection::Automatic) {
    SaveReport5 report;
    report.container = std::move(container);
    report.normalizedSize = image.size();
    if (image.size() != LayoutInfo::FullSaveSize) {
        report.chosenReason = "normalized image is not 0x80000 bytes";
        return report;
    }
    for (const auto family : {SaveFamily::BlackWhite, SaveFamily::Black2White2}) {
        const size_t size = family == SaveFamily::BlackWhite ? LayoutInfo::BlackWhiteCopySize
                                                             : LayoutInfo::Black2White2CopySize;
        const auto primary = inspectCopy5(image, family, 0);
        const auto backup = inspectCopy5(image, family, size);
        const auto relevant = [&](const CopyReport5& c) {
            const bool versionMatch = family == SaveFamily::BlackWhite
                ? (c.versionByte == 20 || c.versionByte == 21) : (c.versionByte == 22 || c.versionByte == 23);
            return versionMatch || c.blocksValid == c.blocksTotal;
        };
        if (relevant(primary) || relevant(backup)) {
            report.copies.push_back(primary);
            report.copies.push_back(backup);
            if (primary.blocksValid == primary.blocksTotal && backup.blocksValid == backup.blocksTotal)
                report.copiesIdentical = std::equal(image.begin(), image.begin() + static_cast<std::ptrdiff_t>(size),
                                                    image.begin() + static_cast<std::ptrdiff_t>(size));
        }
    }
    std::string error;
    const auto parsed = Gen5ReadOnlySave::parse(image, {}, &error, selection);
    if (!parsed) {
        report.chosenReason = error.empty() ? "no copy validates" : error;
        return report;
    }
    for (size_t i = 0; i < report.copies.size(); ++i)
        if (report.copies[i].family == parsed->family() && report.copies[i].offset == parsed->selectedCopyOffset())
            report.chosenIndex = i;
    report.chosenReason = selection == SaveCopySelection::Primary ? "Primary chosen explicitly" :
                          selection == SaveCopySelection::Backup ? "Backup chosen explicitly" :
                          report.copiesIdentical ? "both copies valid and identical" :
                          "only one copy validates";
    return report;
}

// The selected copy's state, or ChecksumInvalid when nothing was chosen.
[[nodiscard]] inline CopyState5 chosenState5(const SaveReport5& r) {
    return r.chosenIndex ? r.copies[*r.chosenIndex].state : CopyState5::ChecksumInvalid;
}
// Index of the other copy of the chosen family that is openable, if any.
[[nodiscard]] inline std::optional<size_t> openableAlternative5(const SaveReport5& r) {
    if (!r.chosenIndex) return std::nullopt;
    const auto& chosen = r.copies[*r.chosenIndex];
    for (size_t i = 0; i < r.copies.size(); ++i)
        if (i != *r.chosenIndex && r.copies[i].family == chosen.family && copyOpenable5(r.copies[i].state) &&
            r.copies[i].state != CopyState5::NewGameNoTrainer)
            return i;
    return std::nullopt;
}

// Shared gate for every Gen V open path (Save Instances rows, Home card
// preview, assigned open, generic reopen). Valid checksums do not prove the
// copy holds the player's game: a blank/initial copy (no TID/SID/name) or one
// whose declared party holds invalid PK5 records is refused with an explicit
// reason instead of opening as "Trainer 0 / Party 0 / Dex 0". The other copy
// is described, never silently substituted. Empty result: copy is openable.
[[nodiscard]] inline std::string unopenableCopyDiagnostic5(std::span<const uint8_t> image,
                                                           const Gen5ReadOnlySave& save,
                                                           SaveCopySelection selection,
                                                           CopyState5* stateOut = nullptr) {
    const auto report = inspectSave5(image, "normalized", selection);
    const auto state = chosenState5(report);
    if (stateOut) *stateOut = state;
    // A new-game copy opens (read-only presentation says "no trainer yet"),
    // unless the other copy holds a real game: then it is a stale copy.
    const bool staleNewGame = state == CopyState5::NewGameNoTrainer && openableAlternative5(report);
    if (copyOpenable5(state) && !staleNewGame) return {};
    std::string out = std::string(save.selectedBackupPartition() ? "Backup" : "Primary") +
                      " copy: " + copyStateLabel5(state);
    if (const auto other = openableAlternative5(report)) {
        const auto& alt = report.copies[*other];
        out += std::string("; the ") + (alt.offset == 0 ? "Primary" : "Backup") +
               " copy is " + copyStateLabel5(alt.state) + " - choose it explicitly";
    } else {
        for (const auto& c : report.copies)
            if (c.family == save.family() && c.offset != save.selectedCopyOffset())
                out += std::string("; the other copy is ") + copyStateLabel5(c.state);
    }
    return out;
}

// Plain-text report with no personal data: no name text, no IDs, no raw bytes.
[[nodiscard]] inline std::string formatSaveReport5(const SaveReport5& r, std::string_view gameId) {
    std::string out = "PokeBank NX Gen V save diagnostics\n";
    out += "ASSIGNED_GAME=" + std::string(gameId) + "\n";
    out += "CONTAINER=" + r.container + "\n";
    out += "NORMALIZED_SIZE=" + std::to_string(r.normalizedSize) + "\n";
    out += "COPIES_IDENTICAL=" + std::string(r.copiesIdentical ? "YES" : "NO") + "\n";
    out += "CHOSEN=" + (r.chosenIndex ? std::to_string(*r.chosenIndex) : std::string("NONE")) + "\n";
    out += "CHOSEN_REASON=" + r.chosenReason + "\n";
    for (size_t i = 0; i < r.copies.size(); ++i) {
        const auto& c = r.copies[i];
        const std::string p = "COPY" + std::to_string(i) + "_";
        out += p + "FAMILY=" + (c.family == SaveFamily::BlackWhite ? "BW" : "B2W2") + "\n";
        out += p + "ROLE=" + (c.offset == 0 ? "Primary" : "Backup") + "\n";
        out += p + "BLOCKS_VALID=" + std::to_string(c.blocksValid) + "/" + std::to_string(c.blocksTotal) + "\n";
        out += p + "VERSION_BYTE=" + std::to_string(c.versionByte) + "\n";
        out += p + "GENDER_BYTE=" + std::to_string(c.genderByte) + "\n";
        out += p + "TRAINER_ID_PRESENT=" + (c.trainerIdPresent ? "YES" : "NO") + "\n";
        out += p + "NAME_LENGTH=" + std::to_string(c.nameLength) + "\n";
        out += p + "NAME_DECODES=" + (c.nameDecodes ? "YES" : "NO") + "\n";
        out += p + "PLAY_TIME=" + std::to_string(c.playedHours) + ":" + std::to_string(c.playedMinutes) +
               ":" + std::to_string(c.playedSeconds) + "\n";
        out += p + "PARTY_COUNT_BYTE=" + std::to_string(c.partyCountByte) + "\n";
        out += p + "PARTY_VALID=" + std::to_string(c.validPartyRecords) +
               " INVALID=" + std::to_string(c.invalidPartyRecords) + "\n";
        out += p + "BOX_OCCUPIED=" + std::to_string(c.occupiedBoxRecords) +
               " BLANK_ENCRYPTED=" + std::to_string(c.blankEncryptedBoxSlots) +
               " ZERO_FILLED=" + std::to_string(c.zeroFilledBoxSlots) +
               " INVALID=" + std::to_string(c.invalidBoxRecords) + "\n";
        out += p + "DEX_SEEN=" + std::to_string(c.dexSeen) + " CAUGHT=" + std::to_string(c.dexCaught) + "\n";
        out += p + "STATE=" + copyStateLabel5(c.state) + "\n";
    }
    return out;
}

} // namespace PokeVault::Integration::Gen5
#endif
