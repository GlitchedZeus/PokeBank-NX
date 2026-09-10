/*
 * PKSM-Core-backed, read-only Gen III adapter for PokeBank NX.
 *
 * PokeBank performs stricter sector/signature/counter/checksum validation before PKSM-Core sees
 * a private copy. The selected release is also family-checked before any Pokemon are exposed.
 */
#if !defined(__SWITCH__) && !defined(POKEBANK_GEN3_SELECTIVE_PORT_TEST)

#include "Integration/Gen3/PKSMGen3Adapter.h"
#include "Integration/Gen3/FRLGReadModel.h"
#include "Integration/Gen3/Gen3SaveValidation.h"

#include "pkx/PK3.hpp"
#include "pkx/PKX.hpp"
#include "sav/Sav3.hpp"
#include "sav/SavE.hpp"
#include "sav/SavFRLG.hpp"
#include "sav/SavRS.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <utility>

namespace PokeVault::Integration::Gen3 {
    namespace {
        std::shared_ptr<u8[]> copyForCore(std::span<const uint8_t> bytes) {
            std::shared_ptr<u8[]> copy(new u8[bytes.size()], std::default_delete<u8[]>());
            std::copy(bytes.begin(), bytes.end(), copy.get());
            return copy;
        }

        std::shared_ptr<u8[]> copyForCoreSlot(std::span<const uint8_t> bytes, uint8_t activeSlot) {
            auto copy = copyForCore(bytes);
            // PKSM-Core's slot picker is intentionally more permissive than PokeBank's. Hide the
            // rejected slot only in this private engine copy so Core cannot override our strict
            // newest-VALID selection. The source buffer remains byte-for-byte untouched.
            const uint8_t otherSlot = activeSlot == 0 ? 1 : 0;
            for (size_t physical = 0; physical < Detail::kSectorCount; ++physical) {
                const size_t idOffset = Detail::kSlotBases[otherSlot] +
                                        physical * Detail::kSectorSize + 0xFF4;
                copy[idOffset] = 0xFF;
                copy[idOffset + 1] = 0xFF;
            }
            return copy;
        }

        std::unique_ptr<pksm::Sav3> makeCore(SourceGame game,
                                              const std::shared_ptr<u8[]>& bytes) {
            switch (game) {
                case SourceGame::FireRedGBA:
                case SourceGame::LeafGreenGBA:
                    return std::make_unique<pksm::SavFRLG>(bytes);
                case SourceGame::RubyGBA:
                case SourceGame::SapphireGBA:
                    return std::make_unique<pksm::SavRS>(bytes);
                case SourceGame::EmeraldGBA:
                    return std::make_unique<pksm::SavE>(bytes);
            }
            return nullptr;
        }

        Detail::FRLGReadModelResult readModelFor(
            std::span<const uint8_t> bytes, SourceGame game,
            const std::array<size_t, Detail::kSectorCount>& offsets) noexcept {
            return Detail::isFRLG(game) ? Detail::readFRLGModel(bytes, offsets) :
                Detail::readRSEModel(bytes, offsets, game == SourceGame::EmeraldGBA);
        }

        std::array<pksm::Stat, 6> stats() noexcept {
            return {pksm::Stat::HP, pksm::Stat::ATK, pksm::Stat::DEF,
                    pksm::Stat::SPD, pksm::Stat::SPATK, pksm::Stat::SPDEF};
        }
    }

    struct ReadOnlySave::Impl {
        std::vector<uint8_t> original;
        std::shared_ptr<u8[]> coreBytes;
        std::unique_ptr<pksm::Sav3> core;
        SaveMetadata metadata;
        std::array<size_t, Detail::kSectorCount> sectorOffsets{};
        Detail::FRLGReadModelResult readModel;
        mutable SaveError enumerationError = SaveError::None;

        Impl(std::span<const uint8_t> bytes, SourceGame sourceGame, uint8_t activeSlot,
             uint32_t saveCounter,
             const std::array<size_t, Detail::kSectorCount>& offsets)
            : original(bytes.begin(), bytes.end()),
              coreBytes(copyForCoreSlot(original, activeSlot)),
              core(makeCore(sourceGame, coreBytes)),
              metadata{sourceGame, Gen3::sourceGameId(sourceGame), activeSlot, saveCounter,
                       core ? core->partyCount() : static_cast<uint8_t>(0xFF),
                       core ? static_cast<uint8_t>(core->maxBoxes()) : static_cast<uint8_t>(0), 30},
              sectorOffsets(offsets), readModel(readModelFor(original, sourceGame, offsets)) {}

        std::vector<uint8_t> readLogical(uint8_t firstSector, size_t logical, size_t length) const {
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
    const TrainerRecord& ReadOnlySave::trainer() const noexcept { return impl_->readModel.trainer; }
    const std::vector<InventoryPouchRecord>& ReadOnlySave::inventory() const noexcept {
        return impl_->readModel.inventory;
    }

    std::vector<PokemonRecord> ReadOnlySave::party() const {
        impl_->enumerationError = SaveError::None;
        std::vector<PokemonRecord> result;
        const size_t base = Detail::partyDataOffset(impl_->metadata.sourceGame);
        for (uint8_t slot = 0; slot < impl_->metadata.partyCount; ++slot) {
            auto raw = impl_->readLogical(1, base + static_cast<size_t>(slot) * 100, 100);
            auto pokemon = impl_->core->pkm(slot);
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
                auto pokemon = impl_->core->pkm(box, slot);
                auto parsed = impl_->record(std::move(pokemon),
                    {PokemonLocation::Kind::Box, box, slot}, std::move(raw));
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

        auto coreProbe = copyForCoreSlot(bytes, active);
        if (!pksm::Sav3::isValid(coreProbe))
            return {nullptr, SaveError::CoreRejected, "pinned PKSM-Core rejected the validated save"};

        auto impl = std::make_unique<ReadOnlySave::Impl>(
            bytes, sourceGame, active, selected.counter, selected.logicalSectorOffsets);
        if (!impl->core)
            return {nullptr, SaveError::CoreRejected, "could not construct pinned PKSM-Core Gen III reader"};
        if (impl->readModel.error != SaveError::None) {
            const SaveError error = impl->readModel.error;
            return {nullptr, error, std::string(errorMessage(error))};
        }
        const uint8_t rawPartyCount = bytes[selected.logicalSectorOffsets[1] +
            Detail::partyCountOffset(sourceGame)];
        if (impl->metadata.partyCount != rawPartyCount)
            return {nullptr, SaveError::CoreRejected, "PKSM-Core party count disagrees with validated save"};
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
            case SaveError::CoreRejected: return "PKSM-Core rejected the save or Pokemon";
            case SaveError::MalformedPokemon: return "Pokemon checksum is invalid";
            case SaveError::InvalidInventory: return "invalid Generation III inventory";
        }
        return "unknown Generation III parse error";
    }
}

#endif
