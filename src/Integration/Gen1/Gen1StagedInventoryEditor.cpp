#include "Integration/Gen1/Gen1StagedInventoryEditor.h"

#include <algorithm>

namespace PokeVault::Integration::Gen1 {
namespace {

constexpr std::size_t kMainStart = 0x2598;
constexpr std::size_t kIntlChecksum = 0x3523;
constexpr std::size_t kIntlMainLength = 0x0F8B;
constexpr std::size_t kJpnChecksum = 0x3594;
constexpr std::size_t kJpnMainLength = 0x0FFC;
constexpr std::size_t kIntlBag = 0x25C9;
constexpr std::size_t kIntlPc = 0x27E6;
constexpr std::size_t kJpnBag = 0x25C4;
constexpr std::size_t kJpnPc = 0x27DC;
constexpr std::size_t kBagCapacity = 20;
constexpr std::size_t kPcCapacity = 50;
constexpr uint8_t kTerminator = 0xFF;

std::size_t blockOffset(RegionLayout region, bool pc) noexcept {
    if (region == RegionLayout::Japanese) return pc ? kJpnPc : kJpnBag;
    return pc ? kIntlPc : kIntlBag;
}

std::size_t blockCapacity(bool pc) noexcept { return pc ? kPcCapacity : kBagCapacity; }

Inventory::ClassicGame inventoryGame(SourceGame game) noexcept {
    switch (game) {
        case SourceGame::Red: return Inventory::ClassicGame::Red;
        case SourceGame::Blue: return Inventory::ClassicGame::Blue;
        case SourceGame::Yellow: return Inventory::ClassicGame::Yellow;
    }
    return Inventory::ClassicGame::Red;
}

bool isPcPocket(Inventory::ClassicPocket pocket) noexcept {
    return pocket == Inventory::ClassicPocket::PCItems;
}

} // namespace

StagedInventoryEditor::StagedInventoryEditor(const ReadOnlySave& source, Inventory::ClassicGame game)
    : game_(game), region_(source.metadata().region), sourceGame_(source.metadata().sourceGame),
      originalBytes_(source.sourceBytes().begin(), source.sourceBytes().end()), stagedBytes_(originalBytes_) {}

std::unique_ptr<StagedInventoryEditor> StagedInventoryEditor::create(const ReadOnlySave& source,
                                                                     std::string& error) {
    error.clear();
    auto editor = std::unique_ptr<StagedInventoryEditor>(
        new StagedInventoryEditor(source, inventoryGame(source.metadata().sourceGame)));
    ReadOnlyInventory decoded;
    if (!editor->decode(decoded, error)) return nullptr;
    return editor;
}

bool StagedInventoryEditor::decode(ReadOnlyInventory& out, std::string& error) const {
    out = decodeInventory(stagedBytes_, region_);
    if (!out.available) {
        error = out.error.empty() ? "Generation I staged inventory is malformed" : out.error;
        return false;
    }
    error.clear();
    return true;
}

std::vector<InventoryEntry> StagedInventoryEditor::entries(Inventory::ClassicPocket pocket,
                                                            std::string& error) const {
    error.clear();
    if (!Inventory::supportsPocket(game_, pocket)) {
        error = "Inventory category is not supported by this game";
        return {};
    }
    ReadOnlyInventory decoded;
    if (!decode(decoded, error)) return {};
    const auto& source = isPcPocket(pocket) ? decoded.pcItems : decoded.bag;
    if (isPcPocket(pocket)) return source;

    std::vector<InventoryEntry> filtered;
    for (const auto& entry : source) {
        if (Inventory::isAddableItem(game_, pocket, entry.itemId)) filtered.push_back(entry);
    }
    return filtered;
}

bool StagedInventoryEditor::rewrite(bool pc, const std::vector<InventoryEntry>& values,
                                    std::string& error) {
    const std::size_t capacity = blockCapacity(pc);
    if (values.size() > capacity) {
        error = pc ? "No free slots in PC Items" : "No free slots in Bag";
        return false;
    }
    const std::size_t offset = blockOffset(region_, pc);
    const std::size_t blockSize = 1 + capacity * 2 + 1;
    if (offset + blockSize > stagedBytes_.size()) {
        error = "Generation I inventory block exceeds staged save";
        return false;
    }
    stagedBytes_[offset] = static_cast<uint8_t>(values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        stagedBytes_[offset + 1 + i * 2] = values[i].itemId;
        stagedBytes_[offset + 2 + i * 2] = values[i].quantity;
    }
    stagedBytes_[offset + 1 + values.size() * 2] = kTerminator;
    return true;
}

void StagedInventoryEditor::recordChange(Inventory::ClassicPocket pocket, uint16_t itemId,
                                         uint16_t before, uint16_t after) {
    auto it = std::find_if(changes_.begin(), changes_.end(), [=](const InventoryChange& change) {
        return change.pocket == pocket && change.itemId == itemId;
    });
    if (it == changes_.end()) {
        if (before != after) changes_.push_back({pocket, itemId, before, after});
        return;
    }
    it->after = after;
    if (it->before == it->after) changes_.erase(it);
}

bool StagedInventoryEditor::stageSetQuantity(Inventory::ClassicPocket pocket, uint16_t itemId,
                                              uint16_t quantity, std::string& error) {
    error.clear();
    if (!Inventory::supportsPocket(game_, pocket)) {
        error = "Inventory category is not supported by this game";
        return false;
    }
    if (!Inventory::isAddableItem(game_, pocket, itemId)) {
        error = "Item is not valid for this game and inventory category";
        return false;
    }
    const auto rule = Inventory::quantityRule(game_, pocket, itemId);
    if (quantity != 0 && (quantity < rule.minimum || quantity > rule.maximum)) {
        error = "Item quantity is outside the valid range";
        return false;
    }

    ReadOnlyInventory decoded;
    if (!decode(decoded, error)) return false;
    const bool pc = isPcPocket(pocket);
    auto values = pc ? decoded.pcItems : decoded.bag;
    auto it = std::find_if(values.begin(), values.end(), [=](const InventoryEntry& entry) {
        return entry.itemId == itemId;
    });
    const uint16_t before = it == values.end() ? 0 : it->quantity;

    if (quantity == 0) {
        if (it == values.end()) return true;
        values.erase(it);
    } else if (it != values.end()) {
        it->quantity = static_cast<uint8_t>(quantity);
    } else {
        if (values.size() >= blockCapacity(pc)) {
            error = pc ? "No free slots in PC Items" : "No free slots in this Bag";
            return false;
        }
        values.push_back({static_cast<uint8_t>(itemId), static_cast<uint8_t>(quantity)});
    }

    const auto beforeBytes = stagedBytes_;
    if (!rewrite(pc, values, error)) {
        stagedBytes_ = beforeBytes;
        return false;
    }
    ReadOnlyInventory roundTrip;
    if (!decode(roundTrip, error)) {
        stagedBytes_ = beforeBytes;
        return false;
    }
    const auto& check = pc ? roundTrip.pcItems : roundTrip.bag;
    const auto verify = std::find_if(check.begin(), check.end(), [=](const InventoryEntry& entry) {
        return entry.itemId == itemId;
    });
    if ((quantity == 0 && verify != check.end()) ||
        (quantity != 0 && (verify == check.end() || verify->quantity != quantity))) {
        stagedBytes_ = beforeBytes;
        error = "Generation I inventory edit failed semantic round-trip validation";
        return false;
    }
    recordChange(pocket, itemId, before, quantity);
    return true;
}

void StagedInventoryEditor::discard() noexcept {
    stagedBytes_ = originalBytes_;
    changes_.clear();
}

std::vector<uint8_t> StagedInventoryEditor::finalize(std::string& error) const {
    error.clear();
    std::vector<uint8_t> candidate = stagedBytes_;
    const std::size_t checksum = region_ == RegionLayout::Japanese ? kJpnChecksum : kIntlChecksum;
    const std::size_t length = region_ == RegionLayout::Japanese ? kJpnMainLength : kIntlMainLength;
    if (candidate.size() != kRawSaveSize || kMainStart + length > candidate.size() || checksum >= candidate.size()) {
        error = "Generation I staged save size/layout is invalid";
        return {};
    }
    candidate[checksum] = calculateDiff8(std::span<const uint8_t>(candidate).subspan(kMainStart, length));

    const auto strict = parse(candidate, sourceGame_);
    if (!strict) {
        error = "Generation I strict reload failed after inventory serialization: " + strict.detail;
        return {};
    }
    const auto decoded = decodeInventory(candidate, region_);
    if (!decoded.available) {
        error = "Generation I inventory reload failed after serialization: " + decoded.error;
        return {};
    }
    if (candidate.size() != originalBytes_.size()) {
        error = "Generation I finalized save size changed";
        return {};
    }
    return candidate;
}

} // namespace PokeVault::Integration::Gen1
