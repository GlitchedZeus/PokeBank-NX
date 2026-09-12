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

} // namespace

GSCReadOnlyTrainer::GSCReadOnlyTrainer(const Integration::Gen2::Metadata& metadata)
    : Trainer::Trainer(std::vector<Save::Block>{}),
      boxCount_(metadata.boxCount),
      slotsPerBox_(metadata.boxCapacity),
      sourceGameId_(metadata.sourceGameId),
      japaneseLayout_(metadata.region == Integration::Gen2::RegionLayout::Japanese),
      crystalFamily_(metadata.family == Integration::Gen2::VersionFamily::Crystal) {
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
    if (metadata.boxCount == 0 || metadata.boxCapacity == 0 ||
        metadata.boxCapacity > ::Trainer::BOX_SLOTS ||
        metadata.currentBox >= metadata.boxCount ||
        save.boxes().size() != metadata.boxCount ||
        save.party().size() > ::Trainer::MAX_PARTY_SLOTS) {
        error = "strict Generation II source metadata is inconsistent";
        return nullptr;
    }
    const size_t expectedBoxes =
        metadata.region == Integration::Gen2::RegionLayout::Japanese ? 9u : 14u;
    const size_t expectedSlots =
        metadata.region == Integration::Gen2::RegionLayout::Japanese ? 30u : 20u;
    if (metadata.boxCount != expectedBoxes || metadata.boxCapacity != expectedSlots) {
        error = "strict Generation II regional box geometry is inconsistent";
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

    // Keep the accepted read-only bridge authoritative. Staged editing is an optional sidecar built
    // from the already-validated save and owns its own byte clone; failing to create it must never
    // prevent a safe read-only save from opening.
    std::string stagedError;
    trainer->stagedEditor_ = Integration::Gen2::StagedEditor::create(save, stagedError);
    trainer->stagedEditingUnavailableReason_ = std::move(stagedError);
    return trainer;
}

bool GSCReadOnlyTrainer::populate(
    const Integration::Gen2::ReadOnlySave& save, std::string& error) {
    const auto& strictTrainer = save.trainer();
    trainerName = strictTrainer.name;
    money = strictTrainer.money;
    TID16 = strictTrainer.trainerId;
    TID = TID16;
    ID32 = TID16; // PK2 has no SID. Only the visible 16-bit Trainer ID is meaningful.
    SID16 = 0;
    SID = 0;

    // Crystal stores a validated 0/1 player-gender byte and the strict parser owns that truth.
    // Gold/Silver have no selectable player gender: their player character is fixed male, so resolve
    // that game rule here instead of reading an unrelated save byte or pretending gender is absent.
    trainerGenderAvailable_ = strictTrainer.gender.has_value();
    if (strictTrainer.gender) {
        trainerGender = *strictTrainer.gender;
    } else if (sourceGameId_ == "gold_gbc" || sourceGameId_ == "silver_gbc") {
        trainerGender = 0;
        trainerGenderAvailable_ = true;
    }

    // Inventory is deliberately optional. A malformed optional item submodel never invalidates an
    // otherwise-valid Trainer/Party/Boxes save. A valid empty inventory still exposes all five real
    // Gen II categories so the UI can render "No items in this category" rather than "Invalid".
    items.clear();
    const auto inventory = Integration::Gen2::decodeInventory(
        save.payloadBytes(), save.metadata().region, save.metadata().family);
    inventoryAvailable_ = inventory.available;
    if (inventory.available) {
        items.resize(Integration::Gen2::kInventoryPocketCount);
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
        party.push_back(std::make_unique<Pokemon::Pokemon2ReadOnly>(record));
    }

    if (save.boxes().size() != boxes.size()) {
        error = "strict Generation II adapter returned an invalid box count";
        return false;
    }
    boxNames.clear();
    boxNames.reserve(boxes.size());
    for (size_t box = 0; box < boxes.size(); ++box) {
        const auto& strictBox = save.boxes()[box];
        if (strictBox.slots.size() != slotsPerBox_) {
            error = "strict Generation II adapter returned an invalid slot count";
            return false;
        }
        // Gen II really stores these names; preserve exactly what the strict decoder returned.
        boxNames.push_back(strictBox.name);
        for (size_t slot = 0; slot < slotsPerBox_; ++slot) {
            if (!strictBox.slots[slot]) continue;
            const auto& record = *strictBox.slots[slot];
            if (record.partyRecord || record.rawBodySize != 32 || record.species == 0) {
                error = "strict Generation II adapter returned an invalid boxed record";
                return false;
            }
            boxes[box][slot] = std::make_unique<Pokemon::Pokemon2ReadOnly>(record);
        }
    }
    return true;
}

} // namespace PokeVault::Legacy
