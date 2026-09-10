/*
 * Exception-free native Generation III implementation behind PKSMGen3Adapter.
 * This selectively ports the proven read-only PK3 semantics and contains no write/resign path.
 */
#if defined(__SWITCH__) || defined(POKEBANK_GEN3_SELECTIVE_PORT_TEST)

#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Integration/Gen3/FRLGReadModel.h"
#include "Integration/Gen3/Gen3SaveValidation.h"

#include "Encryption/Encryption3FRLG.h"
#include "Names/ItemNames.h"
#include "Pokemon/SpeciesConverter3.h"
#include "Utils/Gen3Text.h"
#include "Utils/StringHelpers.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <utility>

namespace PokeVault::Integration::Gen3 {
    namespace {
        std::string decodeName(std::span<const uint8_t> bytes, size_t offset, size_t length) {
            std::u16string text;
            for (size_t index = 0; index < length; ++index) {
                const uint8_t value = bytes[offset + index];
                if (value == Utils::GEN3_TERMINATOR) break;
                if (const char16_t character = Utils::gen3ToChar(value)) text.push_back(character);
            }
            return Utils::utf16ToUtf8(text);
        }

        Detail::FRLGReadModelResult readModelFor(
            std::span<const uint8_t> bytes, SourceGame game,
            const std::array<size_t, Detail::kSectorCount>& offsets) noexcept {
            return Detail::isFRLG(game) ? Detail::readFRLGModel(bytes, offsets) :
                Detail::readRSEModel(bytes, offsets, game == SourceGame::EmeraldGBA);
        }
    }

    struct ReadOnlySave::Impl {
        std::vector<uint8_t> original;
        SaveMetadata metadata;
        std::array<size_t, Detail::kSectorCount> sectorOffsets{};
        Detail::FRLGReadModelResult readModel;
        mutable SaveError enumerationError = SaveError::None;

        Impl(std::span<const uint8_t> bytes, SourceGame sourceGame, uint8_t activeSlot,
             uint32_t saveCounter,
             const std::array<size_t, Detail::kSectorCount>& offsets)
            : original(bytes.begin(), bytes.end()),
              metadata{sourceGame, Gen3::sourceGameId(sourceGame), activeSlot, saveCounter,
                       0, 14, 30},
              sectorOffsets(offsets), readModel(readModelFor(original, sourceGame, offsets)) {
            const auto count = readLogical(1, Detail::partyCountOffset(sourceGame), 1);
            metadata.partyCount = count.empty() ? 0xFF : count.front();
        }

        std::vector<uint8_t> readLogical(uint8_t firstSector, size_t logical,
                                         size_t length) const {
            std::vector<uint8_t> out(length);
            for (size_t index = 0; index < length; ++index) {
                const size_t position = logical + index;
                const size_t sectorId = static_cast<size_t>(firstSector) +
                                        position / Detail::kSectorDataSize;
                if (sectorId >= Detail::kSectorCount) return {};
                out[index] = original[sectorOffsets[sectorId] +
                                      position % Detail::kSectorDataSize];
            }
            return out;
        }

        PokemonRecord record(std::vector<uint8_t> raw, PokemonLocation location) const {
            PokemonRecord out;
            out.location = location;
            out.originalBytes = std::move(raw);
            if (out.originalBytes.size() != 80 && out.originalBytes.size() != 100) {
                enumerationError = SaveError::CoreRejected;
                return out;
            }
            const auto encrypted = std::span<const std::byte>(
                reinterpret_cast<const std::byte*>(out.originalBytes.data()),
                out.originalBytes.size());
            std::unique_ptr<std::byte[]> decrypted(Encryption::decryptArray3FRLG(encrypted));
            const auto bytes = std::span<const uint8_t>(
                reinterpret_cast<const uint8_t*>(decrypted.get()), out.originalBytes.size());
            out.checksumValid = Detail::read16(bytes, 0x1C) == Encryption::checksum3FRLG(
                std::span<const std::byte>(decrypted.get(), out.originalBytes.size()));
            if (!out.checksumValid) {
                enumerationError = SaveError::MalformedPokemon;
                return out;
            }

            out.pid = Detail::read32(bytes, 0x00);
            out.tid = Detail::read16(bytes, 0x04);
            out.sid = Detail::read16(bytes, 0x06);
            out.species = Pokemon::gen3InternalToNational(Detail::read16(bytes, 0x20));
            out.heldItemGen3 = Detail::read16(bytes, 0x22);
            out.heldItem = Names::itemG3ToModern(out.heldItemGen3);
            out.experience = Detail::read32(bytes, 0x24);
            for (size_t index = 0; index < 4; ++index) {
                out.moves[index] = Detail::read16(bytes, 0x2C + index * 2);
                out.pp[index] = bytes[0x34 + index];
            }
            for (size_t index = 0; index < 6; ++index) out.evs[index] = bytes[0x38 + index];
            const uint32_t packedIvs = Detail::read32(bytes, 0x48);
            for (size_t index = 0; index < 6; ++index)
                out.ivs[index] = static_cast<uint8_t>((packedIvs >> (index * 5)) & 0x1F);
            out.nickname = decodeName(bytes, 0x08, 10);
            out.otName = decodeName(bytes, 0x14, 7);

            std::unique_ptr<std::byte[]> serialized(Encryption::encryptArray3FRLG(
                std::span<const std::byte>(decrypted.get(), out.originalBytes.size())));
            out.byteIdenticalRoundTrip = std::memcmp(serialized.get(), out.originalBytes.data(),
                                                     out.originalBytes.size()) == 0;
            return out;
        }
    };

    ReadOnlySave::ReadOnlySave(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}
    ReadOnlySave::ReadOnlySave(ReadOnlySave&&) noexcept = default;
    ReadOnlySave& ReadOnlySave::operator=(ReadOnlySave&&) noexcept = default;
    ReadOnlySave::~ReadOnlySave() = default;

    const SaveMetadata& ReadOnlySave::metadata() const noexcept { return impl_->metadata; }
    const TrainerRecord& ReadOnlySave::trainer() const noexcept { return impl_->readModel.trainer; }
    const std::vector<InventoryPouchRecord>& ReadOnlySave::inventory() const noexcept {
        return impl_->readModel.inventory;
    }

    std::vector<PokemonRecord> ReadOnlySave::party() const {
        impl_->enumerationError = SaveError::None;
        std::vector<PokemonRecord> result;
        const size_t base = Detail::partyDataOffset(impl_->metadata.sourceGame);
        for (uint8_t slot = 0; slot < impl_->metadata.partyCount; ++slot) {
            auto parsed = impl_->record(
                impl_->readLogical(1, base + static_cast<size_t>(slot) * 100, 100),
                {PokemonLocation::Kind::Party, 0, slot});
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
                auto parsed = impl_->record(impl_->readLogical(5, logical, 80),
                    {PokemonLocation::Kind::Box, box, slot});
                if (impl_->enumerationError != SaveError::None) return result;
                if (parsed.species != 0) result.push_back(std::move(parsed));
            }
        }
        return result;
    }

    SaveError ReadOnlySave::lastEnumerationError() const noexcept { return impl_->enumerationError; }

    ParseResult parse(std::span<const uint8_t> bytes, SourceGame sourceGame) {
        if (bytes.size() != Detail::kSaveSize)
            return {nullptr, SaveError::WrongSize, "Gen III GBA save must be exactly 128 KiB"};
        if (!Detail::sourceGameSupported(sourceGame))
            return {nullptr, SaveError::UnsupportedGame, "unsupported Generation III source identity"};

        const Detail::SlotValidation slots[2] = {
            Detail::validateSlot(bytes, 0), Detail::validateSlot(bytes, 1)};
        if (!slots[0].valid && !slots[1].valid) {
            const SaveError error = slots[0].error != SaveError::None ? slots[0].error : slots[1].error;
            return {nullptr, error, std::string(errorMessage(error))};
        }
        const uint8_t active = Detail::selectActiveSlot(slots);
        const auto& selected = slots[active];
        if (!Detail::familyMatches(sourceGame, Detail::detectFamily(bytes, selected)))
            return {nullptr, SaveError::UnsupportedGame,
                    "validated Gen III save family does not match the selected source release"};

        auto impl = std::make_unique<ReadOnlySave::Impl>(
            bytes, sourceGame, active, selected.counter, selected.logicalSectorOffsets);
        if (impl->readModel.error != SaveError::None) {
            const SaveError error = impl->readModel.error;
            return {nullptr, error, std::string(errorMessage(error))};
        }
        if (impl->metadata.partyCount > 6)
            return {nullptr, SaveError::InvalidPartyCount, "party count exceeds six"};
        return {std::unique_ptr<ReadOnlySave>(new ReadOnlySave(std::move(impl))),
                SaveError::None, {}};
    }

    std::string_view sourceGameId(SourceGame game) noexcept {
        switch (game) {
            case SourceGame::FireRedGBA: return "firered_gba";
            case SourceGame::LeafGreenGBA: return "leafgreen_gba";
            case SourceGame::RubyGBA: return "ruby_gba";
            case SourceGame::SapphireGBA: return "sapphire_gba";
            case SourceGame::EmeraldGBA: return "emerald_gba";
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
            case SaveError::CoreRejected: return "native Generation III reader rejected the data";
            case SaveError::MalformedPokemon: return "Pokemon checksum is invalid";
            case SaveError::InvalidInventory: return "invalid FireRed/LeafGreen inventory";
        }
        return "unknown Generation III parse error";
    }
}

#endif
