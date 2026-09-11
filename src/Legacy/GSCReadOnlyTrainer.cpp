#include "Legacy/GSCReadOnlyTrainer.h"

#include "Integration/Gen2/Gen2ReadOnlyInventory.h"
#include "Pokemon/Pokemon2ReadOnly.h"
#include "Trainer/Inventory.h"

#include <string_view>
#include <vector>

namespace PokeVault::Legacy {
namespace {
bool supportedGSCId(std::string_view id) noexcept {
    return id == "gold_gbc" || id == "silver_gbc" || id == "crystal_gbc";
}
}

GSCReadOnlyTrainer::GSCReadOnlyTrainer(const Integration::Gen2::Metadata& metadata)
    : Trainer::Trainer(std::vector<Save::Block>{}),
      boxCount_(metadata.boxCount),
      slotsPerBox_(metadata.boxCapacity),
      sourceGameId_(metadata.sourceGameId),
      region_(metadata.region),
      family_(metadata.family),
      hasStoredTrainerGender_(metadata.family == Integration::Gen2::VersionFamily::Crystal) {
    trainerName.clear();
    money = 0;
    ID32 = 0;
    trainerGender = 0;
    TID16 = 0;
    SID16 = 0;
    TID = 0;
    SID = 0;
    saveRevision = 0;
    saveRevisionString.clear();
    gameVersionString.clear();
    currentBox = metadata.currentBox;
    items.clear();
    boxes.resize(boxCount_);
    boxNames.reserve(boxCount_);
    boxNameDirty.assign(boxCount_, false);
}

std::unique_ptr<GSCReadOnlyTrainer> GSCReadOnlyTrainer::create(
    const Integration::Gen2::ReadOnlySave& save, std::string& error) {
    error.clear();
    const auto& metadata = save.metadata();
    if (!supportedGSCId(metadata.sourceGameId)) {
        error = "unsupported Generation II source identity";
        return nullptr;
    }
    const size_t expectedBoxes = metadata.region == Integration::Gen2::RegionLayout::Japanese ? 9u : 14u;
    const size_t expectedSlots = metadata.region == Integration::Gen2::RegionLayout::Japanese ? 30u : 20u;
    if (metadata.boxCount != expectedBoxes || metadata.boxCapacity != expectedSlots ||
        save.boxes().size() != expectedBoxes || save.party().size() > ::Trainer::MAX_PARTY_SLOTS) {
        error = "strict Generation II source metadata is inconsistent";
        return nullptr;
    }
    if (metadata.currentBox >= metadata.boxCount) {
        error = "strict Generation II current-box index is out of range";
        return nullptr;
    }
    for (const auto& box : save.boxes()) {
        if (box.slots.size() != expectedSlots) {
            error = "strict Generation II box geometry is inconsistent";
            return nullptr;
        }
    }
    auto trainer = std::unique_ptr<GSCReadOnlyTrainer>(new GSCReadOnlyTrainer(metadata));
    if (!trainer->populate(save, error)) return nullptr;
    return trainer;
}

bool GSCReadOnlyTrainer::populate(
    const Integration::Gen2::ReadOnlySave& save, std::string& error) {
    const auto& strictTrainer = save.trainer();
    trainerName = strictTrainer.name;
    money = strictTrainer.money;
    TID16 = strictTrainer.trainerId;
    TID = TID16;
    ID32 = TID16; // Generation II has no SID.
    SID16 = 0;
    SID = 0;
    trainerGender = strictTrainer.gender.value_or(0);

    // Inventory is optional to the core save-open contract. If this independent pouch decoder sees
    // malformed counts/terminators/ids, Trainer/Party/Boxes remain usable and Items reports inventory
    // unavailable instead of rejecting or repairing the source save.
    items.clear();
    const auto inventory = Integration::Gen2::decodeInventory(
        save.payloadBytes(), save.metadata().region, save.metadata().family);
    if (inventory.available) {
        items.resize(Integration::Gen2::kInventoryCategoryCount);
        auto copyInventory = [](const auto& src, auto& dst) {
            dst.reserve(src.size());
            for (const auto& item : src)
                dst.push_back(::Trainer::InventoryItem{item.itemId, item.quantity, false, false});
        };
        copyInventory(inventory.tmhm, items[0]);
        copyInventory(inventory.items, items[1]);
        copyInventory(inventory.keyItems, items[2]);
        copyInventory(inventory.balls, items[3]);
        copyInventory(inventory.pcItems, items[4]);
    }

    party.clear();
    party.reserve(save.party().size());
    for (const auto& record : save.party()) {
        if (!record.partyRecord || record.rawBodySize != 48 || record.species == 0) {
            error = "strict Generation II adapter returned an invalid party record";
            return false;
        }
        party.push_back(std::make_unique<Pokemon::Pokemon2ReadOnly>(
            record, save.metadata().sourceGame, save.metadata().region));
    }

    boxNames.clear();
    for (size_t box = 0; box < boxes.size(); ++box) {
        const auto& strictBox = save.boxes()[box];
        boxNames.push_back(strictBox.name.empty() ? "Box " + std::to_string(box + 1) : strictBox.name);
        for (size_t slot = 0; slot < slotsPerBox_; ++slot) {
            if (!strictBox.slots[slot]) continue;
            const auto& record = *strictBox.slots[slot];
            if (record.partyRecord || record.rawBodySize != 32 || record.species == 0) {
                error = "strict Generation II adapter returned an invalid boxed record";
                return false;
            }
            boxes[box][slot] = std::make_unique<Pokemon::Pokemon2ReadOnly>(
                record, save.metadata().sourceGame, save.metadata().region);
        }
    }
    return true;
}

} // namespace PokeVault::Legacy
