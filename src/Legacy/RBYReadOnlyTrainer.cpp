#include "Legacy/RBYReadOnlyTrainer.h"

#include "Integration/Gen1/Gen1ReadOnlyInventory.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Trainer/Inventory.h"

#include <string_view>
#include <vector>

namespace PokeVault::Legacy {
namespace {
bool supportedRBYId(std::string_view id) noexcept {
    return id == "red_gb" || id == "blue_gb" || id == "yellow_gb";
}
}

RBYReadOnlyTrainer::RBYReadOnlyTrainer(const Integration::Gen1::Metadata& metadata)
    : Trainer::Trainer(std::vector<Save::Block>{}),
      boxCount_(metadata.region == Integration::Gen1::RegionLayout::Japanese ? 8u : 12u),
      slotsPerBox_(metadata.region == Integration::Gen1::RegionLayout::Japanese ? 30u : 20u),
      sourceGameId_(metadata.sourceGameId),
      japaneseLayout_(metadata.region == Integration::Gen1::RegionLayout::Japanese) {
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
    for (size_t box = 0; box < boxCount_; ++box)
        boxNames.push_back("Box " + std::to_string(box + 1));
    boxNameDirty.assign(boxCount_, false);
}

std::unique_ptr<RBYReadOnlyTrainer> RBYReadOnlyTrainer::create(
    const Integration::Gen1::ReadOnlySave& save, std::string& error) {
    error.clear();
    const auto& metadata = save.metadata();
    if (!supportedRBYId(metadata.sourceGameId)) {
        error = "unsupported Generation I source identity";
        return nullptr;
    }
    const size_t expectedBoxes = metadata.region == Integration::Gen1::RegionLayout::Japanese ? 8u : 12u;
    const size_t expectedSlots = metadata.region == Integration::Gen1::RegionLayout::Japanese ? 30u : 20u;
    if (save.boxes().size() != expectedBoxes || save.party().size() > ::Trainer::MAX_PARTY_SLOTS) {
        error = "strict Generation I source metadata is inconsistent";
        return nullptr;
    }
    for (const auto& box : save.boxes()) {
        if (box.slots.size() != expectedSlots) {
            error = "strict Generation I box geometry is inconsistent";
            return nullptr;
        }
    }
    auto trainer = std::unique_ptr<RBYReadOnlyTrainer>(new RBYReadOnlyTrainer(metadata));
    if (!trainer->populate(save, error)) return nullptr;
    return trainer;
}

bool RBYReadOnlyTrainer::populate(
    const Integration::Gen1::ReadOnlySave& save, std::string& error) {
    const auto& strictTrainer = save.trainer();
    trainerName = strictTrainer.name;
    money = strictTrainer.money;
    TID16 = strictTrainer.trainerId;
    TID = TID16;
    ID32 = TID16; // Gen I has no SID; low 16 bits only are meaningful and displayed.
    SID16 = 0;
    SID = 0;

    // Inventory is an optional read-only submodel, matching the accepted RSE policy: malformed
    // inventory never makes an otherwise-valid trainer/party/boxes save unusable. A valid empty
    // inventory still exposes both Gen I categories so the UI says "No items" rather than treating
    // the category as invalid.
    items.clear();
    const auto inventory = Integration::Gen1::decodeInventory(
        save.sourceBytes(), save.metadata().region);
    if (inventory.available) {
        items.resize(Integration::Gen1::kInventoryCategoryCount);
        auto copyInventory = [](const auto& src, auto& dst) {
            dst.reserve(src.size());
            for (const auto& item : src)
                dst.push_back(::Trainer::InventoryItem{item.itemId, item.quantity, false, false});
        };
        copyInventory(inventory.bag, items[0]);
        copyInventory(inventory.pcItems, items[1]);
    }

    party.clear();
    party.reserve(save.party().size());
    for (const auto& record : save.party()) {
        if (!record.partyRecord || record.rawBodySize != 44 || record.species == 0) {
            error = "strict Generation I adapter returned an invalid party record";
            return false;
        }
        party.push_back(std::make_unique<Pokemon::Pokemon1ReadOnly>(record));
    }

    if (save.boxes().size() != boxes.size()) {
        error = "strict Generation I adapter returned an invalid box count";
        return false;
    }
    for (size_t box = 0; box < boxes.size(); ++box) {
        const auto& strictBox = save.boxes()[box];
        if (strictBox.slots.size() != slotsPerBox_) {
            error = "strict Generation I adapter returned an invalid slot count";
            return false;
        }
        for (size_t slot = 0; slot < slotsPerBox_; ++slot) {
            if (!strictBox.slots[slot]) continue;
            const auto& record = *strictBox.slots[slot];
            if (record.partyRecord || record.rawBodySize != 33 || record.species == 0) {
                error = "strict Generation I adapter returned an invalid boxed record";
                return false;
            }
            boxes[box][slot] = std::make_unique<Pokemon::Pokemon1ReadOnly>(record);
        }
    }
    return true;
}

} // namespace PokeVault::Legacy
