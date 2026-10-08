#ifndef POKEBANK_GEN5_READ_ONLY_SAVE_H
#define POKEBANK_GEN5_READ_ONLY_SAVE_H

#include "Integration/Gen5/Gen5SaveBlockLayout.h"
#include "Pokemon/Pokemon5ReadOnly.h"
#include "Utils/CRC16.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace PokeVault::Integration::Gen5 {

enum class SaveFamily : uint8_t { BlackWhite, Black2White2 };
enum class SaveCopySelection : uint8_t { Automatic, Primary, Backup };

struct TrainerReadOnly {
    std::u16string rawName; // raw UTF-16 codepoints; localization still pending
    uint16_t tid = 0, sid = 0;
    uint8_t gameVersion = 0, language = 0, gender = 0;
    uint16_t playedHours = 0;
    uint8_t playedMinutes = 0, playedSeconds = 0;
};
struct DexProgress { uint16_t seen = 0, caught = 0, total = 649; };
struct ReadDiagnostics {
    size_t validPartyRecords = 0;
    size_t occupiedBoxRecords = 0;
    size_t invalidBoxRecords = 0;
};

class Gen5ReadOnlySave {
public:
    // Accept exact 0x80000 NDS battery images only; normalize container formats
    // and provider identities *outside* this strict boundary, never by guesswork.
    static std::optional<Gen5ReadOnlySave> parse(
        std::span<const uint8_t> source,
        std::string_view assignedExactGame = {},
        std::string* error = nullptr,
        SaveCopySelection selection = SaveCopySelection::Automatic) {
        auto fail = [&](const char* msg) -> std::optional<Gen5ReadOnlySave> {
            if (error) *error = msg;
            return std::nullopt;
        };
        if (source.size() != LayoutInfo::FullSaveSize)
            return fail("Gen V requires a normalized 0x80000-byte NDS battery save");

        // BW and B2W2 each have adjacent primary and backup save copies.
        // Neither backup begins at Gen IV's 0x40000 partition offset.
        struct Candidate { SaveFamily family; size_t offset; };
        std::vector<Candidate> candidates;
        for (const size_t offset : {size_t{0}, LayoutInfo::BlackWhiteCopySize}) {
            if (validBlocks(source.subspan(offset, LayoutInfo::BlackWhiteCopySize),
                            LayoutInfo::BlackWhite))
                candidates.push_back({SaveFamily::BlackWhite, offset});
        }
        for (const size_t offset : {size_t{0}, LayoutInfo::Black2White2CopySize}) {
            if (validBlocks(source.subspan(offset, LayoutInfo::Black2White2CopySize),
                            LayoutInfo::Black2White2))
                candidates.push_back({SaveFamily::Black2White2, offset});
        }
        if (candidates.empty())
            return fail("Gen V save has no fully valid BW or B2W2 checksum copy");

        const SaveFamily family = candidates.front().family;
        for (const auto& c : candidates) {
            if (c.family != family)
                return fail("Gen V save layout is ambiguous between BW and B2W2");
            const uint8_t game = source[c.offset + 0x19400 + 0x1F];
            if (gameIdFromVersion(game).empty() ||
                (family == SaveFamily::BlackWhite && game != 20 && game != 21) ||
                (family == SaveFamily::Black2White2 && game != 22 && game != 23))
                return fail("Gen V exact game version disagrees with checksum layout");
            if (game != source[candidates.front().offset + 0x19400 + 0x1F])
                return fail("Gen V valid copies disagree on exact game identity");
        }

        const size_t copySize = family == SaveFamily::BlackWhite ?
            LayoutInfo::BlackWhiteCopySize : LayoutInfo::Black2White2CopySize;
        auto chosen = candidates.front();
        if (selection != SaveCopySelection::Automatic) {
            const size_t requested = selection == SaveCopySelection::Backup ? copySize : 0;
            const auto it = std::find_if(candidates.begin(), candidates.end(),
                [&](const Candidate& c) { return c.offset == requested; });
            if (it == candidates.end())
                return fail("Gen V requested save copy failed checksums or was not present");
            chosen = *it;
        } else if (candidates.size() == 2) {
            // Checksum validity alone does not establish which save is newest.
            const auto& first = candidates[0];
            const auto& second = candidates[1];
            if (!std::equal(source.begin()+static_cast<std::ptrdiff_t>(first.offset),
                            source.begin()+static_cast<std::ptrdiff_t>(first.offset+copySize),
                            source.begin()+static_cast<std::ptrdiff_t>(second.offset)))
                return fail("Gen V has two valid but different save copies; select one explicitly");
            chosen = first.offset == 0 ? first : second;
        }
        const size_t partition = chosen.offset;

        const size_t trainerBase = partition + 0x19400;
        const uint8_t game = source[trainerBase + 0x1F];
        const std::string_view id = gameIdFromVersion(game);
        if (id.empty())
            return fail("Gen V trainer game-version byte is unknown");
        if ((family == SaveFamily::BlackWhite && game != 20 && game != 21) ||
            (family == SaveFamily::Black2White2 && game != 22 && game != 23))
            return fail("Gen V exact game version disagrees with checksum layout");
        if (!assignedExactGame.empty() && id != assignedExactGame)
            return fail("Gen V exact game identity does not match the assigned source");

        const uint8_t count = source[partition + LayoutInfo::PartyOffset + 4];
        if (count > LayoutInfo::PartySlots)
            return fail("Gen V party count exceeds six slots");

        // Declared party records must be valid; empty undeclared slots may be zero.
        for (size_t i = 0; i < count; ++i) {
            const size_t at = partition + LayoutInfo::PartyOffset + 8 + i*Crypto::PartySize;
            const Pokemon5ReadOnly pk(source.subspan(at,Crypto::PartySize));
            if (!pk.valid() || pk.empty())
                return fail("Gen V declared party contains an invalid or empty PK5");
        }

        // A validated SAV checksum does not guarantee each nested PK5 was
        // correctly encrypted and checksummed. Quarantine damaged box slots,
        // just as the accepted Gen IV reader does; never invent semantics.
        ReadDiagnostics diagnostics;
        diagnostics.validPartyRecords = count;
        for (size_t box = 0; box < LayoutInfo::BoxCount; ++box) {
            for (size_t slot = 0; slot < LayoutInfo::BoxSlots; ++slot) {
                const size_t offset = partition + LayoutInfo::BoxOffset +
                                      box*LayoutInfo::BoxStride + slot*Crypto::StoredSize;
                const Pokemon5ReadOnly pk(source.subspan(offset, Crypto::StoredSize));
                if (!pk.valid()) ++diagnostics.invalidBoxRecords;
                else if (!pk.empty()) ++diagnostics.occupiedBoxRecords;
            }
        }

        Gen5ReadOnlySave out;
        out.bytes_.assign(source.begin(), source.end());
        out.base_ = partition;
        out.family_ = family;
        out.exactId_ = std::string(id);
        out.partyCount_ = count;
        out.diagnostics_ = diagnostics;
        if (error) error->clear();
        return out;
    }

    [[nodiscard]] SaveFamily family() const noexcept { return family_; }
    [[nodiscard]] std::string_view exactGameId() const noexcept { return exactId_; }
    [[nodiscard]] size_t selectedPartition() const noexcept { return base_ == 0 ? 0 : 1; }
    [[nodiscard]] size_t selectedCopyOffset() const noexcept { return base_; }
    [[nodiscard]] bool selectedBackupPartition() const noexcept { return base_ != 0; }
    [[nodiscard]] uint8_t partyCount() const noexcept { return partyCount_; }
    [[nodiscard]] const ReadDiagnostics& diagnostics() const noexcept { return diagnostics_; }
    [[nodiscard]] std::span<const uint8_t> sourceBytes() const noexcept { return bytes_; }

    [[nodiscard]] TrainerReadOnly trainer() const {
        const size_t start = base_ + 0x19400;
        TrainerReadOnly t;
        for (size_t i = 0; i < 7; ++i) {
            const uint16_t ch = Crypto::read16(bytes_, start+4+i*2);
            if (ch == 0 || ch == 0xFFFF) break;
            t.rawName.push_back(static_cast<char16_t>(ch));
        }
        t.tid = Crypto::read16(bytes_,start+0x14);
        t.sid = Crypto::read16(bytes_,start+0x16);
        t.gameVersion = bytes_[start+0x1F];
        t.language = bytes_[start+0x1E];
        t.gender = bytes_[start+0x21];
        t.playedHours = Crypto::read16(bytes_,start+0x24);
        t.playedMinutes = bytes_[start+0x26];
        t.playedSeconds = bytes_[start+0x27];
        return t;
    }

    [[nodiscard]] std::optional<Pokemon5ReadOnly> partyPokemon(size_t slot) const {
        if (slot >= LayoutInfo::PartySlots) return std::nullopt;
        const size_t offset = base_ + LayoutInfo::PartyOffset + 8 + slot*Crypto::PartySize;
        return Pokemon5ReadOnly(std::span<const uint8_t>(bytes_).subspan(offset,Crypto::PartySize));
    }
    [[nodiscard]] std::optional<Pokemon5ReadOnly> boxPokemon(size_t box, size_t slot) const {
        if (box >= LayoutInfo::BoxCount || slot >= LayoutInfo::BoxSlots) return std::nullopt;
        const size_t offset = base_ + LayoutInfo::BoxOffset +
                              box*LayoutInfo::BoxStride + slot*Crypto::StoredSize;
        return Pokemon5ReadOnly(std::span<const uint8_t>(bytes_).subspan(offset,Crypto::StoredSize));
    }

    [[nodiscard]] DexProgress dexProgress() const noexcept {
        DexProgress progress;
        const size_t block = base_ +
            (family_ == SaveFamily::BlackWhite ? size_t{0x21600} : size_t{0x21400});
        for (size_t bit=0; bit<649; ++bit) {
            const uint8_t mask = static_cast<uint8_t>(1u << (bit & 7));
            const size_t byteOffset = bit >> 3;
            if (bytes_[block+8+byteOffset] & mask) ++progress.caught;
            bool seen=false;
            for (size_t region=1; region<=4; ++region)
                if (bytes_[block+8+region*0x54+byteOffset] & mask) {
                    seen=true;
                    break;
                }
            if (seen) ++progress.seen;
        }
        return progress;
    }

    [[nodiscard]] static std::string_view gameIdFromVersion(uint8_t version) noexcept {
        // Pinned PKHeX GameVersion: W=20, B=21, W2=22, B2=23.
        switch (version) {
            case 20: return "white_nds";
            case 21: return "black_nds";
            case 22: return "white2_nds";
            case 23: return "black2_nds";
            default: return {};
        }
    }

private:
    Gen5ReadOnlySave() = default;

    template <size_t N>
    static bool validBlocks(std::span<const uint8_t> part,
                            const std::array<LayoutInfo::SaveBlock,N>& layout) noexcept {
        for (const auto& block : layout) {
            const size_t off=block.offset, len=block.length;
            if (off+len > part.size() || block.checksumOffset+2 > part.size() ||
                block.mirrorOffset+2 > part.size()) return false;
            const uint16_t crc=Utils::crc16ccitt(part.data()+off,len);
            if (crc != Crypto::read16(part,block.checksumOffset) ||
                crc != Crypto::read16(part,block.mirrorOffset)) return false;
        }
        return true;
    }

    std::vector<uint8_t> bytes_;
    std::string exactId_;
    SaveFamily family_ = SaveFamily::BlackWhite;
    size_t base_ = 0;
    uint8_t partyCount_ = 0;
    ReadDiagnostics diagnostics_{};
};
} // namespace PokeVault::Integration::Gen5
#endif
