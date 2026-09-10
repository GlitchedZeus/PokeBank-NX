#include "Legacy/FRLGReadOnlyTrainer.h"

#include "Pokemon/Pokemon3FRLG.h"

#include <cstddef>
#include <span>

namespace PokeVault::Legacy {
    namespace {
        std::unique_ptr<Pokemon::Pokemon> makePokemon(
            const Integration::Gen3::PokemonRecord& record, size_t expectedSize,
            std::string& error) {
            if (record.originalBytes.size() != expectedSize || !record.checksumValid ||
                !record.byteIdenticalRoundTrip) {
                error = "validated Gen III record failed the read-only view-model contract";
                return nullptr;
            }
            const auto bytes = std::span<const std::byte>(
                reinterpret_cast<const std::byte*>(record.originalBytes.data()),
                record.originalBytes.size());
            auto pokemon = std::make_unique<Pokemon::Pokemon3FRLG>(bytes);
            if (!pokemon->checksumValid() || pokemon->speciesID() != record.species ||
                pokemon->pid() != record.pid || pokemon->tid16() != record.tid ||
                pokemon->sid16() != record.sid) {
                error = "Gen III view-model cross-check disagreed with the strict adapter";
                return nullptr;
            }
            return pokemon;
        }

        bool supportedGen3Id(std::string_view id) noexcept {
            return id == "ruby_gba" || id == "sapphire_gba" || id == "emerald_gba" ||
                   id == "firered_gba" || id == "leafgreen_gba";
        }

        bool isFRLGId(std::string_view id) noexcept {
            return id == "firered_gba" || id == "leafgreen_gba";
        }

        void populateInventoryItems(
            const std::vector<Integration::Gen3::InventoryPouchRecord>& strictInventory,
            std::vector<std::vector<::Trainer::InventoryItem>>& items) {
            items.resize(strictInventory.size());
            for (size_t pouch = 0; pouch < strictInventory.size(); ++pouch) {
                items[pouch].reserve(strictInventory[pouch].items.size());
                for (const auto& item : strictInventory[pouch].items) {
                    if (item.itemId == 0 || item.count == 0) continue;
                    items[pouch].push_back({item.itemId, item.count, false, false});
                }
            }
        }
    }

    FRLGReadOnlyTrainer::FRLGReadOnlyTrainer(
        const Integration::Gen3::SaveMetadata& metadata)
        : boxCount_(metadata.boxCount), slotsPerBox_(metadata.slotsPerBox),
          sourceGameId_(metadata.sourceGameId) {
        trainerName.clear();
        money = ID32 = TID = SID = 0;
        TID16 = SID16 = 0;
        trainerGender = 0;
        saveRevision = 0;
        saveRevisionString = "Base";
        gameVersionString.clear();
        currentBox = 0;
        boxes.resize(boxCount_);
        boxNames.reserve(boxCount_);
        for (size_t box = 0; box < boxCount_; ++box)
            boxNames.push_back("Box " + std::to_string(box + 1));
        boxNameDirty.assign(boxCount_, false);
    }

    std::unique_ptr<FRLGReadOnlyTrainer> FRLGReadOnlyTrainer::create(
        const Integration::Gen3::ReadOnlySave& save, std::string& error) {
        error.clear();
        const auto& metadata = save.metadata();
        if (!supportedGen3Id(metadata.sourceGameId) || metadata.boxCount != 14 ||
            metadata.slotsPerBox != 30 || metadata.partyCount > ::Trainer::MAX_PARTY_SLOTS) {
            error = "unsupported Gen III source metadata";
            return nullptr;
        }
        auto trainer = std::unique_ptr<FRLGReadOnlyTrainer>(new FRLGReadOnlyTrainer(metadata));
        if (!trainer->populate(save, error)) return nullptr;
        return trainer;
    }

    bool FRLGReadOnlyTrainer::populate(
        const Integration::Gen3::ReadOnlySave& save, std::string& error) {
        const auto& strictTrainer = save.trainer();
        trainerName = strictTrainer.name;
        trainerGender = strictTrainer.gender;
        TID16 = strictTrainer.tid16;
        SID16 = strictTrainer.sid16;
        ID32 = strictTrainer.id32;
        TID = TID16;
        SID = SID16;
        money = strictTrainer.money;

        const auto& strictInventory = save.inventory();
        items.clear();
        if (isFRLGId(sourceGameId_)) {
            // FRLG inventory is part of its already accepted device contract. Preserve the strict
            // six-pouch requirement rather than weakening accepted FRLG behavior.
            if (strictInventory.size() != 6) {
                error = "strict FRLG adapter returned an invalid inventory layout";
                return false;
            }
            populateInventoryItems(strictInventory, items);
        } else {
            // RSE inventory was added after the original read-only trainer bridge. The old bridge
            // treated any newly exposed RSE inventory as an error and returned nullptr, which made
            // every otherwise-valid Ruby/Sapphire/Emerald save bounce back to the main menu.
            //
            // A valid six-pouch RSE model is now passed through exactly like FRLG. If the optional
            // RSE inventory model is unavailable, leave items empty and continue exposing trainer,
            // party, boxes, source details and refresh. The Items panel renders that state as
            // "Inventory unavailable" instead of making the whole save unusable.
            if (strictInventory.size() == 6)
                populateInventoryItems(strictInventory, items);
        }

        const auto partyRecords = save.party();
        if (save.lastEnumerationError() != Integration::Gen3::SaveError::None) {
            error = "strict Gen III adapter could not enumerate the party";
            return false;
        }
        party.resize(save.metadata().partyCount);
        for (const auto& record : partyRecords) {
            if (record.location.kind != Integration::Gen3::PokemonLocation::Kind::Party ||
                record.location.slot >= party.size()) {
                error = "strict Gen III adapter returned an invalid party location";
                return false;
            }
            auto pokemon = makePokemon(record, 100, error);
            if (!pokemon) return false;
            party[record.location.slot] = std::move(pokemon);
        }

        const auto boxRecords = save.boxes();
        if (save.lastEnumerationError() != Integration::Gen3::SaveError::None) {
            error = "strict Gen III adapter could not enumerate storage boxes";
            return false;
        }
        for (const auto& record : boxRecords) {
            if (record.location.kind != Integration::Gen3::PokemonLocation::Kind::Box ||
                record.location.box >= boxes.size() || record.location.slot >= slotsPerBox_) {
                error = "strict Gen III adapter returned an invalid box location";
                return false;
            }
            auto pokemon = makePokemon(record, 80, error);
            if (!pokemon) return false;
            boxes[record.location.box][record.location.slot] = std::move(pokemon);
        }
        return true;
    }
}
