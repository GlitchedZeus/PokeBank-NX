/*
 * PKSM-Core-backed, read-only Gen III adapter for PokeBank NX.
 *
 * Derived integration work. PKSM-Core copyright belongs to its contributors and is
 * distributed under GPLv3 with additional terms 7.b and 7.c. This wrapper performs
 * stricter pre-validation than pinned PKSM-Core Sav3::isValid(): every sector signature,
 * counter, and checksum is verified before Core sees a private copy of the bytes.
 */
#if !defined(__SWITCH__) && !defined(POKEBANK_GEN3_SELECTIVE_PORT_TEST)

#include "Integration/Gen3/PKSMGen3Adapter.h"

#include "pkx/PK3.hpp"
#include "pkx/PKX.hpp"
#include "sav/Sav3.hpp"
#include "sav/SavFRLG.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <utility>

namespace PokeVault::Integration::Gen3 {
    namespace {
        constexpr size_t kSaveSize = 0x20000;
        constexpr size_t kSectorSize = 0x1000;
        constexpr size_t kSectorDataSize = 0xF80;
        constexpr size_t kSectorCount = 14;
        constexpr size_t kSlotBases[2] = {0x0000, 0xE000};
        constexpr uint32_t kSectorSignature = 0x08012025;
        constexpr std::array<size_t, kSectorCount> kChunkLengths = {
            0xF2C, 0xF80, 0xF80, 0xF80, 0xF08, 0xF80, 0xF80,
            0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0xF80, 0x7D0,
        };

        uint16_t read16(std::span<const uint8_t> bytes, size_t offset) noexcept {
            return static_cast<uint16_t>(bytes[offset]) |
                   static_cast<uint16_t>(bytes[offset + 1] << 8);
        }

        uint32_t read32(std::span<const uint8_t> bytes, size_t offset) noexcept {
            return static_cast<uint32_t>(read16(bytes, offset)) |
                   (static_cast<uint32_t>(read16(bytes, offset + 2)) << 16);
        }

        uint16_t sectorChecksum(std::span<const uint8_t> data) noexcept {
            uint32_t sum = 0;
            for (size_t offset = 0; offset < data.size(); offset += 4) {
                uint32_t word = data[offset];
                if (offset + 1 < data.size()) word |= static_cast<uint32_t>(data[offset + 1]) << 8;
                if (offset + 2 < data.size()) word |= static_cast<uint32_t>(data[offset + 2]) << 16;
                if (offset + 3 < data.size()) word |= static_cast<uint32_t>(data[offset + 3]) << 24;
                sum += word;
            }
            return static_cast<uint16_t>((sum & 0xFFFFu) + (sum >> 16));
        }

        struct SlotValidation {
            bool valid = false;
            SaveError error = SaveError::None;
            uint32_t counter = 0;
            std::array<size_t, kSectorCount> logicalSectorOffsets{};
        };

        SlotValidation validateSlot(std::span<const uint8_t> bytes, uint8_t slot) noexcept {
            SlotValidation result;
            result.logicalSectorOffsets.fill(std::numeric_limits<size_t>::max());
            const size_t slotBase = kSlotBases[slot];
            bool counterSet = false;

            for (size_t physical = 0; physical < kSectorCount; ++physical) {
                const size_t offset = slotBase + physical * kSectorSize;
                const uint16_t id = read16(bytes, offset + 0xFF4);
                if (id >= kSectorCount) {
                    result.error = SaveError::InvalidSectorId;
                    return result;
                }
                if (result.logicalSectorOffsets[id] != std::numeric_limits<size_t>::max()) {
                    result.error = SaveError::DuplicateSector;
                    return result;
                }
                if (read32(bytes, offset + 0xFF8) != kSectorSignature) {
                    result.error = SaveError::BadSectorSignature;
                    return result;
                }

                const uint32_t counter = read32(bytes, offset + 0xFFC);
                if (!counterSet) {
                    result.counter = counter;
                    counterSet = true;
                } else if (counter != result.counter) {
                    result.error = SaveError::MismatchedSectorCounters;
                    return result;
                }

                const auto payload = bytes.subspan(offset, kChunkLengths[id]);
                if (read16(bytes, offset + 0xFF6) != sectorChecksum(payload)) {
                    result.error = SaveError::BadSectorChecksum;
                    return result;
                }
                result.logicalSectorOffsets[id] = offset;
            }

            if (std::ranges::find(result.logicalSectorOffsets,
                    std::numeric_limits<size_t>::max()) != result.logicalSectorOffsets.end()) {
                result.error = SaveError::MissingSector;
                return result;
            }
            result.valid = true;
            return result;
        }

        bool counterIsNewer(uint32_t lhs, uint32_t rhs) noexcept {
            return static_cast<int32_t>(lhs - rhs) > 0;
        }

        std::shared_ptr<u8[]> copyForCore(std::span<const uint8_t> bytes) {
            std::shared_ptr<u8[]> copy(new u8[bytes.size()], std::default_delete<u8[]>());
            std::copy(bytes.begin(), bytes.end(), copy.get());
            return copy;
        }

        std::shared_ptr<u8[]> copyForCoreSlot(std::span<const uint8_t> bytes, uint8_t activeSlot) {
            auto copy = copyForCore(bytes);
            // PKSM-Core's pinned slot picker only considers sector IDs and counters. Our stricter
            // validator may correctly reject the numerically newer slot for a bad signature or
            // checksum, so hide that rejected slot in the private engine copy. Source bytes and
            // the caller's buffer remain untouched.
            const uint8_t otherSlot = activeSlot == 0 ? 1 : 0;
            for (size_t physical = 0; physical < kSectorCount; ++physical) {
                const size_t idOffset = kSlotBases[otherSlot] + physical * kSectorSize + 0xFF4;
                copy[idOffset] = 0xFF;
                copy[idOffset + 1] = 0xFF;
            }
            return copy;
        }

        std::array<pksm::Stat, 6> stats() noexcept {
            return {pksm::Stat::HP, pksm::Stat::ATK, pksm::Stat::DEF,
                    pksm::Stat::SPD, pksm::Stat::SPATK, pksm::Stat::SPDEF};
        }
    }

    struct ReadOnlySave::Impl {
        std::vector<uint8_t> original;
        std::shared_ptr<u8[]> coreBytes;
        pksm::SavFRLG core;
        SaveMetadata metadata;
        std::array<size_t, kSectorCount> sectorOffsets{};
        mutable SaveError enumerationError = SaveError::None;

        Impl(std::span<const uint8_t> bytes, SourceGame sourceGame, uint8_t activeSlot,
             uint32_t saveCounter, const std::array<size_t, kSectorCount>& offsets)
            : original(bytes.begin(), bytes.end()),
              coreBytes(copyForCoreSlot(original, activeSlot)),
              core(coreBytes),
              metadata{sourceGame, Gen3::sourceGameId(sourceGame), activeSlot, saveCounter,
                       core.partyCount(), static_cast<uint8_t>(core.maxBoxes()), 30},
              sectorOffsets(offsets) {}

        std::vector<uint8_t> readLogical(uint8_t firstSector, size_t logical,
                                         size_t length) const {
            std::vector<uint8_t> out(length);
            for (size_t index = 0; index < length; ++index) {
                const size_t position = logical + index;
                const size_t sectorDelta = position / kSectorDataSize;
                const size_t sectorId = static_cast<size_t>(firstSector) + sectorDelta;
                if (sectorId >= kSectorCount) return {};
                out[index] = original[sectorOffsets[sectorId] + position % kSectorDataSize];
            }
            return out;
        }

        PokemonRecord record(std::unique_ptr<pksm::PKX> pokemon, PokemonLocation location,
                             std::vector<uint8_t> originalBytes) const {
            PokemonRecord out;
            out.location = location;
            out.originalBytes = std::move(originalBytes);
            if (!pokemon) {
                enumerationError = SaveError::CoreRejected;
                return out;
            }

            out.checksumValid = !pokemon->isEncrypted();
            if (!out.checksumValid) {
                enumerationError = SaveError::MalformedPokemon;
                return out;
            }
            out.species = static_cast<uint16_t>(pokemon->species());
            out.pid = pokemon->PID();
            out.tid = pokemon->TID();
            out.sid = pokemon->SID();
            out.experience = pokemon->experience();
            out.heldItem = pokemon->heldItem();
            out.heldItemGen3 = static_cast<const pksm::PK3*>(pokemon.get())->heldItem3();
            for (uint8_t index = 0; index < 4; ++index) {
                out.moves[index] = static_cast<uint16_t>(pokemon->move(index));
                out.pp[index] = pokemon->PP(index);
            }
            const auto statOrder = stats();
            for (size_t index = 0; index < statOrder.size(); ++index) {
                out.ivs[index] = pokemon->iv(statOrder[index]);
                out.evs[index] = static_cast<uint8_t>(pokemon->ev(statOrder[index]));
            }
            out.nickname = pokemon->nickname();
            out.otName = pokemon->otName();

            auto roundTrip = pokemon->clone();
            if (roundTrip) {
                roundTrip->encrypt();
                const auto serialized = roundTrip->rawData();
                out.byteIdenticalRoundTrip = serialized.size() == out.originalBytes.size() &&
                    std::equal(serialized.begin(), serialized.end(), out.originalBytes.begin());
            }
            return out;
        }
    };

    ReadOnlySave::ReadOnlySave(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
    ReadOnlySave::ReadOnlySave(ReadOnlySave&&) noexcept = default;
    ReadOnlySave& ReadOnlySave::operator=(ReadOnlySave&&) noexcept = default;
    ReadOnlySave::~ReadOnlySave() = default;

    const SaveMetadata& ReadOnlySave::metadata() const noexcept { return impl_->metadata; }

    std::vector<PokemonRecord> ReadOnlySave::party() const {
        impl_->enumerationError = SaveError::None;
        std::vector<PokemonRecord> result;
        for (uint8_t slot = 0; slot < impl_->metadata.partyCount; ++slot) {
            auto raw = impl_->readLogical(1, 0x38 + static_cast<size_t>(slot) * 100, 100);
            auto pokemon = impl_->core.pkm(slot);
            auto parsed = impl_->record(std::move(pokemon),
                {PokemonLocation::Kind::Party, 0, slot}, std::move(raw));
            if (impl_->enumerationError != SaveError::None) break;
            if (parsed.species != 0) result.push_back(std::move(parsed));
        }
        return result;
    }

    std::vector<PokemonRecord> ReadOnlySave::boxes() const {
        impl_->enumerationError = SaveError::None;
        std::vector<PokemonRecord> result;
        for (uint8_t box = 0; box < impl_->metadata.boxCount; ++box) {
            for (uint8_t slot = 0; slot < impl_->metadata.slotsPerBox; ++slot) {
                const size_t logical = 4 +
                    (static_cast<size_t>(box) * impl_->metadata.slotsPerBox + slot) * 80;
                auto raw = impl_->readLogical(5, logical, 80);
                auto pokemon = impl_->core.pkm(box, slot);
                auto parsed = impl_->record(std::move(pokemon),
                    {PokemonLocation::Kind::Box, box, slot}, std::move(raw));
                if (impl_->enumerationError != SaveError::None) return result;
                if (parsed.species != 0) result.push_back(std::move(parsed));
            }
        }
        return result;
    }

    SaveError ReadOnlySave::lastEnumerationError() const noexcept {
        return impl_->enumerationError;
    }

    ParseResult parse(std::span<const uint8_t> bytes, SourceGame sourceGame) {
        if (bytes.size() != kSaveSize) {
            return {nullptr, SaveError::WrongSize, "Gen III GBA save must be exactly 128 KiB"};
        }

        const SlotValidation slots[2] = {validateSlot(bytes, 0), validateSlot(bytes, 1)};
        if (!slots[0].valid && !slots[1].valid) {
            const SaveError error = slots[0].error != SaveError::None ?
                slots[0].error : slots[1].error;
            return {nullptr, error, std::string(errorMessage(error))};
        }
        const uint8_t active = !slots[0].valid ? 1 : !slots[1].valid ? 0 :
            (counterIsNewer(slots[1].counter, slots[0].counter) ? 1 : 0);
        const auto& selected = slots[active];

        const size_t block0 = selected.logicalSectorOffsets[0];
        if (read32(bytes, block0 + 0xAC) != 1) {
            return {nullptr, SaveError::UnsupportedGame,
                    "valid Gen III save is not the FireRed/LeafGreen family"};
        }

        auto coreProbe = copyForCore(bytes);
        if (!pksm::Sav3::isValid(coreProbe)) {
            return {nullptr, SaveError::CoreRejected, "pinned PKSM-Core rejected the validated save"};
        }

        auto impl = std::make_unique<ReadOnlySave::Impl>(
            bytes, sourceGame, active, selected.counter, selected.logicalSectorOffsets);
        if (impl->metadata.partyCount > 6) {
            return {nullptr, SaveError::InvalidPartyCount, "party count exceeds six"};
        }
        return {std::unique_ptr<ReadOnlySave>(new ReadOnlySave(std::move(impl))),
                SaveError::None, {}};
    }

    std::string_view sourceGameId(SourceGame game) noexcept {
        switch (game) {
            case SourceGame::FireRedGBA: return "firered_gba";
            case SourceGame::LeafGreenGBA: return "leafgreen_gba";
        }
        return "";
    }

    std::string_view errorMessage(SaveError error) noexcept {
        switch (error) {
            case SaveError::None: return "no error";
            case SaveError::WrongSize: return "wrong save size";
            case SaveError::InvalidSectorId: return "invalid sector id";
            case SaveError::DuplicateSector: return "duplicate sector id (another sector is missing)";
            case SaveError::MissingSector: return "missing sector";
            case SaveError::BadSectorSignature: return "bad sector signature";
            case SaveError::MismatchedSectorCounters: return "sector counters do not match";
            case SaveError::BadSectorChecksum: return "bad sector checksum";
            case SaveError::UnsupportedGame: return "unsupported Generation III game";
            case SaveError::InvalidPartyCount: return "invalid party count";
            case SaveError::CoreRejected: return "PKSM-Core rejected the save or Pokemon";
            case SaveError::MalformedPokemon: return "Pokemon checksum is invalid";
        }
        return "unknown Generation III parse error";
    }
}

#endif
