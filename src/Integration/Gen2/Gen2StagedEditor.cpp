#include "Integration/Gen2/Gen2StagedEditor.h"

#include <algorithm>
#include <array>
#include <utility>

namespace PokeVault::Integration::Gen2 {
namespace {

struct Layout {
    std::size_t trainerName;
    std::size_t trainerNameBytes;
    std::size_t money;
    std::size_t items;
    std::size_t balls;
    std::size_t checksumEnd;
    std::size_t checksumPrimary;
    std::size_t checksumSecondary;
};

constexpr Layout kIntlGS{
    0x200B, 11, 0x23DB, 0x241F, 0x2464, 0x2D68, 0x2D69, 0x7E6D,
};
constexpr Layout kIntlCrystal{
    0x200B, 11, 0x23DC, 0x2420, 0x2465, 0x2B82, 0x2D0D, 0x1F0D,
};

const Layout& layoutFor(VersionFamily family) noexcept {
    return family == VersionFamily::Crystal ? kIntlCrystal : kIntlGS;
}

constexpr std::array<uint8_t, 119> kGeneralItems{
    3,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,26,27,28,29,
    30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,46,47,48,49,51,52,53,57,
    60,62,63,64,65,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,
    91,92,93,94,95,96,97,98,99,101,102,103,104,105,106,107,108,109,110,111,
    112,113,114,117,118,119,121,122,123,124,125,126,131,132,138,139,140,143,
    144,146,150,151,152,156,158,163,167,168,169,170,172,173,174,180,181,182,
    183,184,185,186,187,188,189,
};
constexpr std::array<uint8_t, 11> kBallItems{1,2,4,5,157,159,160,161,164,165,166};

bool contains(std::span<const uint8_t> values, uint8_t value) noexcept {
    return std::find(values.begin(), values.end(), value) != values.end();
}

bool allowedInPocket(InventoryPocket pocket, uint8_t itemId) noexcept {
    switch (pocket) {
        case InventoryPocket::Items:
            return contains(kGeneralItems, itemId);
        case InventoryPocket::Balls:
            return contains(kBallItems, itemId);
        default:
            return false;
    }
}

std::size_t pocketOffset(const Layout& layout, InventoryPocket pocket) noexcept {
    switch (pocket) {
        case InventoryPocket::Items: return layout.items;
        case InventoryPocket::Balls: return layout.balls;
        default: return static_cast<std::size_t>(-1);
    }
}

std::size_t pocketCapacity(InventoryPocket pocket) noexcept {
    switch (pocket) {
        case InventoryPocket::Items: return 20;
        case InventoryPocket::Balls: return 12;
        default: return 0;
    }
}

uint32_t readBE24(std::span<const uint8_t> bytes, std::size_t offset) noexcept {
    return (static_cast<uint32_t>(bytes[offset]) << 16) |
           (static_cast<uint32_t>(bytes[offset + 1]) << 8) |
           bytes[offset + 2];
}

void writeBE24(std::vector<uint8_t>& bytes, std::size_t offset, uint32_t value) noexcept {
    bytes[offset] = static_cast<uint8_t>((value >> 16) & 0xFF);
    bytes[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    bytes[offset + 2] = static_cast<uint8_t>(value & 0xFF);
}

void writeLE16(std::vector<uint8_t>& bytes, std::size_t offset, uint16_t value) noexcept {
    bytes[offset] = static_cast<uint8_t>(value & 0xFF);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}

uint16_t readLE16(std::span<const uint8_t> bytes, std::size_t offset) noexcept {
    return static_cast<uint16_t>(bytes[offset] |
        (static_cast<uint16_t>(bytes[offset + 1]) << 8));
}

struct PairEntry { uint8_t id; uint8_t quantity; };

bool readPairPocket(std::span<const uint8_t> bytes, std::size_t offset, std::size_t capacity,
                    std::vector<PairEntry>& entries) noexcept {
    if (offset >= bytes.size()) return false;
    const std::size_t bytesNeeded = 1 + 2 * capacity + 1;
    if (bytesNeeded > bytes.size() - offset) return false;
    const uint8_t count = bytes[offset];
    if (count > capacity) return false;
    if (bytes[offset + 1 + 2 * count] != 0xFF) return false;
    entries.clear();
    entries.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const uint8_t id = bytes[offset + 1 + i * 2];
        const uint8_t qty = bytes[offset + 2 + i * 2];
        if (id == 0 || id == 0xFF || qty == 0 || qty > kMaxStackQuantity) return false;
        entries.push_back({id, qty});
    }
    return true;
}

void writePairPocket(std::vector<uint8_t>& bytes, std::size_t offset, std::size_t capacity,
                     std::span<const PairEntry> entries) noexcept {
    bytes[offset] = static_cast<uint8_t>(entries.size());
    std::fill(bytes.begin() + static_cast<std::ptrdiff_t>(offset + 1),
              bytes.begin() + static_cast<std::ptrdiff_t>(offset + 1 + 2 * capacity + 1), 0);
    for (std::size_t i = 0; i < entries.size(); ++i) {
        bytes[offset + 1 + i * 2] = entries[i].id;
        bytes[offset + 2 + i * 2] = entries[i].quantity;
    }
    bytes[offset + 1 + 2 * entries.size()] = 0xFF;
}

uint8_t quantityInPairPocket(std::span<const uint8_t> bytes, std::size_t offset,
                             std::size_t capacity, uint8_t itemId) noexcept {
    std::vector<PairEntry> entries;
    if (!readPairPocket(bytes, offset, capacity, entries)) return 0;
    const auto it = std::find_if(entries.begin(), entries.end(),
        [itemId](const PairEntry& entry) { return entry.id == itemId; });
    return it == entries.end() ? 0 : it->quantity;
}

std::string itemLabel(uint8_t itemId) {
    if (itemId == kMasterBallItemId) return "Master Ball";
    if (itemId == kPokeBallItemId) return "Poke Ball";
    if (itemId == kPotionItemId) return "Potion";
    const auto name = gen2ItemName(itemId);
    return name.empty() ? ("Item " + std::to_string(itemId)) : std::string(name);
}

bool rangeFits(std::size_t start, std::size_t endExclusive, std::size_t size) noexcept {
    return start <= endExclusive && endExclusive <= size;
}

void copyRange(std::vector<uint8_t>& bytes, std::size_t sourceStart,
               std::size_t sourceEndExclusive, std::size_t destinationStart) {
    const std::size_t length = sourceEndExclusive - sourceStart;
    std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(sourceStart), length,
                bytes.begin() + static_cast<std::ptrdiff_t>(destinationStart));
}

bool synchronizeInternationalMirrors(std::vector<uint8_t>& bytes, VersionFamily family,
                                     std::string& error) {
    const std::size_t payloadSize = bytes.size();
    if (family == VersionFamily::Crystal) {
        constexpr std::size_t sourceStart = 0x2009;
        constexpr std::size_t sourceEnd = 0x2B83;
        constexpr std::size_t destination = 0x1209;
        if (!rangeFits(sourceStart, sourceEnd, payloadSize) ||
            destination + (sourceEnd - sourceStart) > payloadSize) {
            error = "Crystal mirror range does not fit the staged payload";
            return false;
        }
        copyRange(bytes, sourceStart, sourceEnd, destination);
        return true;
    }

    struct Mirror { std::size_t begin, end, destination; };
    constexpr std::array<Mirror, 5> mirrors{{
        {0x2009, 0x222F, 0x15C7},
        {0x222F, 0x23D9, 0x3D69},
        {0x23D9, 0x2856, 0x0C6B},
        {0x2856, 0x288A, 0x7E39},
        {0x288A, 0x2D69, 0x10E8},
    }};
    for (const auto& mirror : mirrors) {
        if (!rangeFits(mirror.begin, mirror.end, payloadSize) ||
            mirror.destination + (mirror.end - mirror.begin) > payloadSize) {
            error = "Gold/Silver mirror range does not fit the staged payload";
            return false;
        }
        copyRange(bytes, mirror.begin, mirror.end, mirror.destination);
    }
    return true;
}

} // namespace

StagedEditor::StagedEditor(const ReadOnlySave& source)
    : metadata_(source.metadata()),
      sourceGame_(source.metadata().sourceGame),
      original_(source.sourceBytes().begin(), source.sourceBytes().end()),
      staged_(original_),
      originalTrainerName_(source.trainer().name),
      trainerName_(originalTrainerName_),
      originalMoney_(source.trainer().money),
      money_(originalMoney_) {
    capabilities_.add(SaveEdit::Capability::TrainerIdentity)
                 .add(SaveEdit::Capability::Money)
                 .add(SaveEdit::Capability::Inventory)
                 .add(SaveEdit::Capability::Balls)
                 .add(SaveEdit::Capability::Medicine)
                 .add(SaveEdit::Capability::BattleItems);
}

std::unique_ptr<StagedEditor> StagedEditor::create(const ReadOnlySave& source, std::string& error) {
    error.clear();
    if (source.metadata().region != RegionLayout::International) {
        error = "staged Gen II editing is currently limited to validated international saves";
        return nullptr;
    }
    if (source.metadata().payloadSize < kRawSaveSize32K ||
        source.sourceBytes().size() < source.metadata().payloadSize) {
        error = "validated Gen II source has an invalid editable payload size";
        return nullptr;
    }
    const auto inventory = decodeInventory(source.payloadBytes(), source.metadata().region,
                                           source.metadata().family);
    if (!inventory.available) {
        error = "validated Gen II inventory is unavailable; staged editing remains disabled";
        return nullptr;
    }
    const auto& layout = layoutFor(source.metadata().family);
    if (layout.checksumSecondary + 1 >= source.metadata().payloadSize ||
        layout.trainerName + layout.trainerNameBytes > source.metadata().payloadSize ||
        layout.money + 3 > source.metadata().payloadSize) {
        error = "Gen II editable offsets do not fit the validated payload";
        return nullptr;
    }
    return std::unique_ptr<StagedEditor>(new StagedEditor(source));
}

bool StagedEditor::encodeTrainerName(std::string_view name, std::vector<uint8_t>& encoded,
                                     std::string& error) const {
    error.clear();
    if (name.empty()) {
        error = "trainer name cannot be empty";
        return false;
    }
    if (name.size() > kInternationalTrainerNameMax) {
        error = "Generation II international trainer names are limited to 7 characters";
        return false;
    }
    encoded.assign(layoutFor(metadata_.family).trainerNameBytes, 0x50);
    for (std::size_t i = 0; i < name.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(name[i]);
        uint8_t value = 0;
        if (c >= 'A' && c <= 'Z') value = static_cast<uint8_t>(0x80 + c - 'A');
        else if (c >= 'a' && c <= 'z') value = static_cast<uint8_t>(0xA0 + c - 'a');
        else if (c >= '0' && c <= '9') value = static_cast<uint8_t>(0xF6 + c - '0');
        else {
            switch (c) {
                case ' ': value = 0x7F; break;
                case '(': value = 0x9A; break;
                case ')': value = 0x9B; break;
                case ':': value = 0x9C; break;
                case ';': value = 0x9D; break;
                case '-': value = 0xE3; break;
                case '?': value = 0xE6; break;
                case '!': value = 0xE7; break;
                case '&': value = 0xE9; break;
                case '\'': value = 0xE0; break;
                case '.': value = 0xF2; break;
                case '/': value = 0xF3; break;
                case ',': value = 0xF4; break;
                default:
                    error = "trainer name contains a character not representable by the current Gen II encoder";
                    return false;
            }
        }
        encoded[i] = value;
    }
    return true;
}

void StagedEditor::clearChange(std::string_view key) {
    changes_.erase(std::remove_if(changes_.begin(), changes_.end(),
        [key](const StagedChange& change) { return change.key == key; }), changes_.end());
}

void StagedEditor::setChange(std::string key, std::string label,
                             std::string beforeValue, std::string afterValue) {
    clearChange(key);
    if (beforeValue == afterValue) return;
    changes_.push_back({std::move(key), std::move(label),
                        std::move(beforeValue), std::move(afterValue)});
}

bool StagedEditor::stageTrainerName(std::string_view name, std::string& error) {
    std::vector<uint8_t> encoded;
    if (!encodeTrainerName(name, encoded, error)) return false;
    const auto& layout = layoutFor(metadata_.family);
    std::copy(encoded.begin(), encoded.end(),
              staged_.begin() + static_cast<std::ptrdiff_t>(layout.trainerName));
    trainerName_ = std::string(name);
    setChange("trainer_name", "Trainer Name", originalTrainerName_, trainerName_);
    return true;
}

bool StagedEditor::stageMoney(uint32_t value, std::string& error) {
    error.clear();
    if (value > kMaxMoney) {
        error = "Generation II money exceeds retail maximum 999999";
        return false;
    }
    writeBE24(staged_, layoutFor(metadata_.family).money, value);
    money_ = value;
    setChange("money", "Money", "$" + std::to_string(originalMoney_),
              "$" + std::to_string(money_));
    return true;
}

uint8_t StagedEditor::itemQuantity(InventoryPocket pocket, uint8_t itemId) const noexcept {
    const auto& layout = layoutFor(metadata_.family);
    const auto offset = pocketOffset(layout, pocket);
    const auto capacity = pocketCapacity(pocket);
    if (offset == static_cast<std::size_t>(-1) || capacity == 0) return 0;
    return quantityInPairPocket(staged_, offset, capacity, itemId);
}

bool StagedEditor::stageItemQuantity(InventoryPocket pocket, uint8_t itemId, uint8_t quantity,
                                     std::string& error) {
    error.clear();
    if (quantity > kMaxStackQuantity) {
        error = "Generation II item quantities are limited to 99";
        return false;
    }
    if (!allowedInPocket(pocket, itemId)) {
        error = "item is not valid in this Generation II pocket";
        return false;
    }
    const auto& layout = layoutFor(metadata_.family);
    const auto offset = pocketOffset(layout, pocket);
    const auto capacity = pocketCapacity(pocket);
    if (offset == static_cast<std::size_t>(-1) || capacity == 0) {
        error = "this Generation II pocket is not editable in the first staged-editor slice";
        return false;
    }

    std::vector<PairEntry> entries;
    if (!readPairPocket(staged_, offset, capacity, entries)) {
        error = "Generation II staged pocket no longer has a valid list structure";
        return false;
    }
    const uint8_t originalQuantity = quantityInPairPocket(original_, offset, capacity, itemId);
    auto it = std::find_if(entries.begin(), entries.end(),
        [itemId](const PairEntry& entry) { return entry.id == itemId; });
    if (quantity == 0) {
        if (it != entries.end()) entries.erase(it);
    } else if (it != entries.end()) {
        it->quantity = quantity;
    } else {
        if (entries.size() >= capacity) {
            error = "Generation II pocket is full; cannot add another item";
            return false;
        }
        entries.push_back({itemId, quantity});
    }
    writePairPocket(staged_, offset, capacity, entries);

    const std::string key = "item:" + std::to_string(static_cast<unsigned>(pocket)) +
                            ":" + std::to_string(itemId);
    setChange(key, itemLabel(itemId), std::to_string(originalQuantity), std::to_string(quantity));
    return true;
}

void StagedEditor::discard() noexcept {
    staged_ = original_;
    trainerName_ = originalTrainerName_;
    money_ = originalMoney_;
    changes_.clear();
}

std::vector<uint8_t> StagedEditor::finalizedBytes(std::string& error) const {
    error.clear();
    std::vector<uint8_t> result = staged_;
    const auto& layout = layoutFor(metadata_.family);
    if (metadata_.payloadSize > result.size() || layout.checksumSecondary + 1 >= metadata_.payloadSize) {
        error = "staged Gen II payload cannot be finalized safely";
        return {};
    }

    auto payload = std::span<uint8_t>(result.data(), metadata_.payloadSize);
    const uint16_t checksum = calculateChecksum(payload, metadata_.region, metadata_.family);
    writeLE16(result, layout.checksumPrimary, checksum);
    writeLE16(result, layout.checksumSecondary, checksum);

    // PKHeX's SAV2 export contract synchronizes the retail mirror regions after writing the checksums.
    // These copies never touch an RTC footer appended after payloadSize.
    std::vector<uint8_t> payloadCopy(result.begin(), result.begin() +
                                     static_cast<std::ptrdiff_t>(metadata_.payloadSize));
    if (!synchronizeInternationalMirrors(payloadCopy, metadata_.family, error)) return {};
    std::copy(payloadCopy.begin(), payloadCopy.end(), result.begin());

    const uint16_t repaired = calculateChecksum(
        std::span<const uint8_t>(result.data(), metadata_.payloadSize), metadata_.region, metadata_.family);
    if (readLE16(result, layout.checksumPrimary) != repaired ||
        readLE16(result, layout.checksumSecondary) != repaired) {
        error = "final Gen II primary/secondary checksum verification failed";
        return {};
    }

    const auto reparsed = parse(result, sourceGame_);
    if (!reparsed) {
        error = "final staged Gen II export failed strict reload: " + reparsed.detail;
        return {};
    }
    if (reparsed.save->trainer().name != trainerName_ || reparsed.save->trainer().money != money_) {
        error = "final staged Gen II export did not round-trip trainer fields";
        return {};
    }
    if (result.size() != original_.size()) {
        error = "final staged Gen II export changed save size";
        return {};
    }
    if (metadata_.rtcFooterSize != 0) {
        const auto footerStart = metadata_.payloadSize;
        if (!std::equal(original_.begin() + static_cast<std::ptrdiff_t>(footerStart), original_.end(),
                        result.begin() + static_cast<std::ptrdiff_t>(footerStart))) {
            error = "final staged Gen II export changed the RTC footer";
            return {};
        }
    }
    return result;
}

} // namespace PokeVault::Integration::Gen2
