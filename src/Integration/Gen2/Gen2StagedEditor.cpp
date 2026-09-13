#include "Integration/Gen2/Gen2StagedEditor.h"

#include "Integration/Gen2/Gen2PersonalData.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Experience.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <sstream>
#include <utility>

namespace PokeVault::Integration::Gen2 {
namespace {

struct Layout {
    std::size_t trainerName;
    std::size_t trainerNameBytes;
    std::size_t money;
    std::size_t items;
    std::size_t balls;
    std::size_t currentBoxCopy;
    std::size_t checksumEnd;
    std::size_t checksumPrimary;
    std::size_t checksumSecondary;
    uint8_t boxCapacity;
    uint8_t boxCount;
    uint8_t stringLength;
    uint8_t splitAtBox;
};

constexpr Layout kIntlGS{
    0x200B, 11, 0x23DB, 0x241F, 0x2464, 0x2D6C, 0x2D68, 0x2D69, 0x7E6D,
    20, 14, 11, 7,
};
constexpr Layout kIntlCrystal{
    0x200B, 11, 0x23DC, 0x2420, 0x2465, 0x2D10, 0x2B82, 0x2D0D, 0x1F0D,
    20, 14, 11, 7,
};

const Layout& layoutFor(VersionFamily family) noexcept {
    return family == VersionFamily::Crystal ? kIntlCrystal : kIntlGS;
}

constexpr std::size_t kStoredBodySize = 32;

std::size_t boxListLength(const Layout& layout) noexcept {
    return 1 + (static_cast<std::size_t>(layout.boxCapacity) + 1) +
           static_cast<std::size_t>(layout.boxCapacity) * kStoredBodySize +
           2 * static_cast<std::size_t>(layout.boxCapacity) * layout.stringLength;
}

std::size_t boxStride(const Layout& layout) noexcept {
    return boxListLength(layout) + 2;
}

std::size_t boxStart(const Layout& layout, std::size_t box) noexcept {
    if (box < layout.splitAtBox) return 0x4000 + box * boxStride(layout);
    return 0x6000 + (box - layout.splitAtBox) * boxStride(layout);
}

std::size_t boxBodyStart(const Layout& layout, std::size_t box) noexcept {
    return boxStart(layout, box) + 1 + (static_cast<std::size_t>(layout.boxCapacity) + 1);
}

std::size_t boxOTStart(const Layout& layout, std::size_t box) noexcept {
    return boxBodyStart(layout, box) + static_cast<std::size_t>(layout.boxCapacity) * kStoredBodySize;
}

std::size_t boxNicknameStart(const Layout& layout, std::size_t box) noexcept {
    return boxOTStart(layout, box) + static_cast<std::size_t>(layout.boxCapacity) * layout.stringLength;
}

constexpr std::array<uint8_t, 131> kGeneralItems{
    3,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,26,27,28,29,
    30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,46,47,48,49,51,52,53,57,
    60,62,63,64,65,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,
    91,92,93,94,95,96,97,98,99,101,102,103,104,105,106,107,108,109,110,111,
    112,113,114,117,118,119,121,122,123,124,125,126,131,132,138,139,140,143,
    144,146,150,151,152,156,158,163,167,168,169,170,172,173,174,180,181,182,
    183,184,185,186,187,188,189,
};
constexpr std::array<uint8_t, 11> kBallItems{1,2,4,5,157,159,160,161,164,165,166};

// Generation II base PP table, move IDs 0..251. Pinned against PKHeX
// MoveInfo2.cs at 77dcd3a7895bceaafbbff12d25bdf77c1acd8ca5.
constexpr std::array<uint8_t, 252> kGen2BasePP{
    0,35,25,10,15,20,20,15,15,15,35,30,5,10,30,30,35,35,20,15,
    20,20,10,20,30,5,25,15,15,15,25,20,5,35,15,20,20,20,15,30,
    35,20,20,30,25,40,20,15,20,20,20,30,25,15,30,25,5,15,10,5,
    20,20,20,5,35,20,25,20,20,20,15,20,10,10,40,25,10,35,30,15,
    20,40,10,15,30,15,20,10,15,10,5,10,10,25,10,20,40,30,30,20,
    20,15,10,40,15,20,30,20,20,10,40,40,30,30,30,20,30,10,10,20,
    5,10,30,20,20,20,5,15,10,20,15,15,35,20,15,10,20,30,15,40,
    20,15,10,5,10,30,10,15,20,15,40,40,10,5,15,10,10,10,15,30,
    30,10,10,20,10,1,1,10,10,10,5,15,25,15,10,15,30,5,40,15,
    10,25,10,30,10,20,10,10,10,10,10,20,5,40,5,5,15,5,10,5,
    15,10,5,10,20,20,40,15,10,20,20,25,5,15,10,5,20,15,20,25,
    20,5,30,5,10,20,40,5,20,40,20,15,35,10,5,5,5,15,5,20,
    5,5,15,20,10,5,5,15,15,15,15,10,
};

bool contains(std::span<const uint8_t> values, uint8_t value) noexcept {
    return std::find(values.begin(), values.end(), value) != values.end();
}

bool allowedInPocket(InventoryPocket pocket, uint8_t itemId) noexcept {
    switch (pocket) {
        case InventoryPocket::Items: return contains(kGeneralItems, itemId);
        case InventoryPocket::Balls: return contains(kBallItems, itemId);
        default: return false;
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

void writeBE16(std::vector<uint8_t>& bytes, std::size_t offset, uint16_t value) noexcept {
    bytes[offset] = static_cast<uint8_t>(value >> 8);
    bytes[offset + 1] = static_cast<uint8_t>(value & 0xFF);
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
    if (count > capacity || bytes[offset + 1 + 2 * count] != 0xFF) return false;
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

bool repairForStrictReload(std::vector<uint8_t>& bytes, const Metadata& metadata,
                           std::string& error) {
    const auto& layout = layoutFor(metadata.family);
    if (metadata.payloadSize > bytes.size() || layout.checksumSecondary + 1 >= metadata.payloadSize) {
        error = "staged Gen II payload cannot be repaired safely";
        return false;
    }
    auto payload = std::span<uint8_t>(bytes.data(), metadata.payloadSize);
    const uint16_t checksum = calculateChecksum(payload, metadata.region, metadata.family);
    writeLE16(bytes, layout.checksumPrimary, checksum);
    writeLE16(bytes, layout.checksumSecondary, checksum);

    std::vector<uint8_t> payloadCopy(bytes.begin(), bytes.begin() +
        static_cast<std::ptrdiff_t>(metadata.payloadSize));
    if (!synchronizeInternationalMirrors(payloadCopy, metadata.family, error)) return false;
    std::copy(payloadCopy.begin(), payloadCopy.end(), bytes.begin());
    return true;
}

bool encodeInternationalASCII(std::string_view name, std::size_t maxChars, std::size_t fieldBytes,
                              std::vector<uint8_t>& encoded, std::string& error,
                              std::string_view fieldLabel) {
    error.clear();
    if (name.empty()) {
        error = std::string(fieldLabel) + " cannot be empty";
        return false;
    }
    encoded.assign(fieldBytes, 0x50);
    std::size_t input = 0;
    std::size_t output = 0;
    while (input < name.size()) {
        if (output >= maxChars) {
            error = std::string(fieldLabel) + " exceeds the Generation II international length limit";
            return false;
        }
        const unsigned char c = static_cast<unsigned char>(name[input]);
        uint8_t value = 0;
        std::size_t consumed = 1;
        if (c >= 'A' && c <= 'Z') value = static_cast<uint8_t>(0x80 + c - 'A');
        else if (c >= 'a' && c <= 'z') value = static_cast<uint8_t>(0xA0 + c - 'a');
        else if (c >= '0' && c <= '9') value = static_cast<uint8_t>(0xF6 + c - '0');
        else if (c < 0x80) {
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
                    error = std::string(fieldLabel) +
                        " contains a character not representable by the conservative Gen II encoder";
                    return false;
            }
        } else if (input + 2 < name.size() &&
                   static_cast<unsigned char>(name[input]) == 0xE2 &&
                   static_cast<unsigned char>(name[input + 1]) == 0x99 &&
                   static_cast<unsigned char>(name[input + 2]) == 0x82) {
            value = 0xEF;
            consumed = 3;
        } else if (input + 2 < name.size() &&
                   static_cast<unsigned char>(name[input]) == 0xE2 &&
                   static_cast<unsigned char>(name[input + 1]) == 0x99 &&
                   static_cast<unsigned char>(name[input + 2]) == 0x80) {
            value = 0xF5;
            consumed = 3;
        } else if (input + 2 < name.size() &&
                   static_cast<unsigned char>(name[input]) == 0xE2 &&
                   static_cast<unsigned char>(name[input + 1]) == 0x80 &&
                   static_cast<unsigned char>(name[input + 2]) == 0x99) {
            value = 0xE0;
            consumed = 3;
        } else {
            error = std::string(fieldLabel) +
                " contains a character not representable by the conservative Gen II encoder";
            return false;
        }
        encoded[output++] = value;
        input += consumed;
    }
    return true;
}

bool validHeldItem(uint8_t item) noexcept {
    if (item == 0) return true;
    const auto name = gen2ItemName(item);
    return !name.empty() && name != "TERU-SAMA";
}

uint8_t maxPP(uint8_t move, uint8_t ppUps) noexcept {
    if (move == 0 || move > kMaxGen2Move || ppUps > 3) return 0;
    const uint8_t base = kGen2BasePP[move];
    return static_cast<uint8_t>(std::min<unsigned>(63u, base + (static_cast<unsigned>(base) * ppUps) / 5u));
}

std::string array4(const std::array<uint8_t, 4>& values) {
    std::ostringstream os;
    os << static_cast<unsigned>(values[0]) << '/' << static_cast<unsigned>(values[1]) << '/'
       << static_cast<unsigned>(values[2]) << '/' << static_cast<unsigned>(values[3]);
    return os.str();
}

std::string statExpText(const std::array<uint16_t, 5>& values) {
    std::ostringstream os;
    os << values[0] << '/' << values[1] << '/' << values[2] << '/' << values[3] << '/' << values[4];
    return os.str();
}

uint8_t semanticGender(const PokemonRecord& pokemon) noexcept {
    return static_cast<uint8_t>(genderFromAttackDV(pokemon.species, pokemon.dvs[1]));
}

bool samePokemon(const PokemonRecord& a, const PokemonRecord& b) noexcept {
    return a.species == b.species && a.heldItem == b.heldItem && a.moves == b.moves &&
           a.trainerId == b.trainerId && a.experience == b.experience &&
           a.statExperience == b.statExperience && a.dvs == b.dvs && a.pp == b.pp &&
           a.ppUps == b.ppUps && a.friendship == b.friendship && a.pokerus == b.pokerus &&
           a.caughtData == b.caughtData && a.level == b.level && a.originalTrainer == b.originalTrainer &&
           a.nickname == b.nickname && a.isEgg == b.isEgg && a.shiny == b.shiny &&
           semanticGender(a) == semanticGender(b);
}

std::string pokemonMismatchDetail(const PokemonRecord& reparsed, const PokemonRecord& expected) {
    std::ostringstream os;
    auto add = [&](std::string_view field, auto actual, auto wanted) {
        if (actual == wanted) return;
        if (os.tellp() > 0) os << "; ";
        os << field << " expected=" << +wanted << " reparsed=" << +actual;
    };
    add("species", reparsed.species, expected.species);
    add("heldItem", reparsed.heldItem, expected.heldItem);
    add("trainerId", reparsed.trainerId, expected.trainerId);
    add("experience", reparsed.experience, expected.experience);
    for (std::size_t i = 0; i < 4; ++i) {
        add("move" + std::to_string(i + 1), reparsed.moves[i], expected.moves[i]);
        add("pp" + std::to_string(i + 1), reparsed.pp[i], expected.pp[i]);
        add("ppUps" + std::to_string(i + 1), reparsed.ppUps[i], expected.ppUps[i]);
    }
    for (std::size_t i = 0; i < 5; ++i) {
        add("statExperience" + std::to_string(i), reparsed.statExperience[i], expected.statExperience[i]);
        add("dv" + std::to_string(i), reparsed.dvs[i], expected.dvs[i]);
    }
    add("friendship", reparsed.friendship, expected.friendship);
    add("pokerus", reparsed.pokerus, expected.pokerus);
    add("caughtData", reparsed.caughtData, expected.caughtData);
    add("level", reparsed.level, expected.level);
    add("isEgg", reparsed.isEgg, expected.isEgg);
    add("shiny", reparsed.shiny, expected.shiny);
    add("gender", semanticGender(reparsed), semanticGender(expected));
    if (reparsed.originalTrainer != expected.originalTrainer) {
        if (os.tellp() > 0) os << "; ";
        os << "originalTrainer expected='" << expected.originalTrainer
           << "' reparsed='" << reparsed.originalTrainer << "'";
    }
    if (reparsed.nickname != expected.nickname) {
        if (os.tellp() > 0) os << "; ";
        os << "nickname expected='" << expected.nickname
           << "' reparsed='" << reparsed.nickname << "'";
    }
    return os.str();
}

bool validBoxLocation(const Metadata& metadata, const Layout& layout,
                      std::size_t box, std::size_t slot, std::string& error) {
    if (box >= layout.boxCount || box >= metadata.boxCount) {
        error = "Generation II box index is outside the validated save";
        return false;
    }
    if (slot >= layout.boxCapacity || slot >= metadata.boxCapacity) {
        error = "Generation II box slot is outside the validated save";
        return false;
    }
    return true;
}

void writePokemonBody(std::vector<uint8_t>& bytes, std::size_t offset,
                      const PokemonRecord& p) noexcept {
    bytes[offset] = static_cast<uint8_t>(p.species);
    bytes[offset + 1] = p.heldItem;
    for (std::size_t i = 0; i < 4; ++i) bytes[offset + 2 + i] = p.moves[i];
    writeBE16(bytes, offset + 6, p.trainerId);
    writeBE24(bytes, offset + 8, p.experience);
    for (std::size_t i = 0; i < 5; ++i) writeBE16(bytes, offset + 11 + i * 2, p.statExperience[i]);
    bytes[offset + 21] = static_cast<uint8_t>((p.dvs[1] << 4) | p.dvs[2]); // atk/def
    bytes[offset + 22] = static_cast<uint8_t>((p.dvs[3] << 4) | p.dvs[4]); // speed/special
    for (std::size_t i = 0; i < 4; ++i)
        bytes[offset + 23 + i] = static_cast<uint8_t>((p.ppUps[i] << 6) | (p.pp[i] & 0x3F));
    bytes[offset + 27] = p.friendship;
    bytes[offset + 28] = p.pokerus;
    writeBE16(bytes, offset + 29, p.caughtData);
    bytes[offset + 31] = p.level;
}

std::array<uint8_t, 4> editableDVs(const PokemonRecord& p) noexcept {
    return {p.dvs[1], p.dvs[2], p.dvs[3], p.dvs[4]};
}

void putEditableDVs(PokemonRecord& p, const std::array<uint8_t, 4>& dvs) noexcept {
    p.dvs[1] = dvs[0];
    p.dvs[2] = dvs[1];
    p.dvs[3] = dvs[2];
    p.dvs[4] = dvs[3];
    p.dvs[0] = StagedEditor::derivedHPDV(dvs);
    p.shiny = StagedEditor::isShinyDVs(dvs);
    p.gender = static_cast<uint8_t>(genderFromAttackDV(p.species, dvs[0]));
}

} // namespace

StagedEditor::StagedEditor(const ReadOnlySave& source)
    : metadata_(source.metadata()),
      sourceGame_(source.metadata().sourceGame),
      original_(source.sourceBytes().begin(), source.sourceBytes().end()),
      staged_(original_),
      originalTrainerName_(source.trainer().name),
      trainerName_(originalTrainerName_),
      trainerId_(source.trainer().trainerId),
      originalMoney_(source.trainer().money),
      money_(originalMoney_) {
    capabilities_.add(SaveEdit::Capability::TrainerIdentity)
                 .add(SaveEdit::Capability::Money)
                 .add(SaveEdit::Capability::Inventory)
                 .add(SaveEdit::Capability::Balls)
                 .add(SaveEdit::Capability::Medicine)
                 .add(SaveEdit::Capability::BattleItems)
                 .add(SaveEdit::Capability::BoxPokemon)
                 .add(SaveEdit::Capability::PokemonEditing)
                 .add(SaveEdit::Capability::PokemonCreation)
                 .add(SaveEdit::Capability::PokemonClone)
                 .add(SaveEdit::Capability::PokemonShinyToggle)
                 .add(SaveEdit::Capability::MachineMoveNames);
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
        layout.money + 3 > source.metadata().payloadSize ||
        boxStart(layout, layout.boxCount - 1) + boxListLength(layout) > source.metadata().payloadSize) {
        error = "Gen II editable offsets do not fit the validated payload";
        return nullptr;
    }
    return std::unique_ptr<StagedEditor>(new StagedEditor(source));
}

bool StagedEditor::encodeTrainerName(std::string_view name, std::vector<uint8_t>& encoded,
                                     std::string& error) const {
    return encodeInternationalASCII(name, kInternationalTrainerNameMax,
                                    layoutFor(metadata_.family).trainerNameBytes,
                                    encoded, error, "trainer name");
}

bool StagedEditor::encodePokemonName(std::string_view name, std::vector<uint8_t>& encoded,
                                     std::string& error) const {
    return encodeInternationalASCII(name, kInternationalPokemonNameMax,
                                    layoutFor(metadata_.family).stringLength,
                                    encoded, error, "Pokemon name");
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
        error = "this Generation II pocket is not editable in the staged editor";
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

uint8_t StagedEditor::gen2MoveBasePP(uint16_t move) noexcept {
    return move <= kMaxGen2Move ? kGen2BasePP[move] : 0;
}

bool StagedEditor::isShinyDVs(const std::array<uint8_t, 4>& dvs) noexcept {
    return dvs[1] == 10 && dvs[2] == 10 && dvs[3] == 10 && (dvs[0] & 0x02) != 0;
}

uint8_t StagedEditor::derivedHPDV(const std::array<uint8_t, 4>& dvs) noexcept {
    return static_cast<uint8_t>(((dvs[0] & 1) << 3) | ((dvs[1] & 1) << 2) |
                                ((dvs[2] & 1) << 1) | (dvs[3] & 1));
}

std::optional<PokemonRecord> StagedEditor::boxedPokemon(std::size_t box, std::size_t slot,
                                                         std::string& error) const {
    error.clear();
    const auto& layout = layoutFor(metadata_.family);
    if (!validBoxLocation(metadata_, layout, box, slot, error)) return std::nullopt;

    std::vector<uint8_t> checked = staged_;
    if (!repairForStrictReload(checked, metadata_, error)) return std::nullopt;
    const auto parsed = parse(checked, sourceGame_);
    if (!parsed) {
        error = "staged Gen II box failed strict parse: " + parsed.detail;
        return std::nullopt;
    }
    if (box >= parsed.save->boxes().size() || slot >= parsed.save->boxes()[box].slots.size() ||
        !parsed.save->boxes()[box].slots[slot]) {
        error = "Generation II box slot is empty";
        return std::nullopt;
    }
    PokemonRecord normalized = *parsed.save->boxes()[box].slots[slot];
    // Gender is not stored independently in Gen II PK2 data. The read-only parser deliberately
    // leaves PokemonRecord::gender unresolved; staged-editor semantics resolve it from species
    // and Attack DV so a serialize/reparse cycle compares the actual retail meaning.
    normalized.gender = semanticGender(normalized);
    return normalized;
}

bool StagedEditor::syncCurrentBoxCopy(std::size_t box, std::string& error) {
    error.clear();
    if (box != metadata_.currentBox) return true;
    const auto& layout = layoutFor(metadata_.family);
    const std::size_t start = boxStart(layout, box);
    const std::size_t length = boxListLength(layout);
    if (!rangeFits(start, start + length, metadata_.payloadSize) ||
        layout.currentBoxCopy + length > metadata_.payloadSize) {
        error = "Generation II current-box cache cannot be synchronized safely";
        return false;
    }
    copyRange(staged_, start, start + length, layout.currentBoxCopy);
    return true;
}

void StagedEditor::rememberPokemonExpectation(std::size_t box, std::size_t slot,
                                              const PokemonRecord& expected) {
    auto it = std::find_if(pokemonExpectations_.begin(), pokemonExpectations_.end(),
        [box, slot](const PokemonExpectation& value) {
            return value.box == box && value.slot == slot;
        });
    if (it == pokemonExpectations_.end()) pokemonExpectations_.push_back({box, slot, expected});
    else it->expected = expected;
}

bool StagedEditor::stageBoxPokemonEdit(std::size_t box, std::size_t slot,
                                       const BoxPokemonEdit& edit, std::string& error) {
    error.clear();
    auto currentOpt = boxedPokemon(box, slot, error);
    if (!currentOpt) return false;
    const PokemonRecord before = *currentOpt;
    if (before.isEgg) {
        error = "egg editing is not enabled in the first Generation II Pokemon editor milestone";
        return false;
    }
    PokemonRecord after = before;

    if (edit.species) {
        if (*edit.species == 0 || *edit.species > kMaxGen2Species || !personalRecord(*edit.species)) {
            error = "species is not a valid Generation II species";
            return false;
        }
        after.species = *edit.species;
    }
    if (edit.nickname) {
        std::vector<uint8_t> ignored;
        if (!encodePokemonName(*edit.nickname, ignored, error)) return false;
        after.nickname = *edit.nickname;
    }
    if (edit.otName) {
        std::vector<uint8_t> ignored;
        if (!encodePokemonName(*edit.otName, ignored, error)) return false;
        after.originalTrainer = *edit.otName;
    }
    if (edit.heldItem) {
        if (!validHeldItem(*edit.heldItem)) {
            error = "held item is not a usable Generation II item id";
            return false;
        }
        after.heldItem = *edit.heldItem;
    }
    if (edit.trainerId) after.trainerId = *edit.trainerId;
    if (edit.friendship) after.friendship = *edit.friendship;
    if (edit.pokerus) after.pokerus = *edit.pokerus;
    if (edit.caughtData) after.caughtData = *edit.caughtData;

    if (edit.statExperience) after.statExperience = *edit.statExperience;
    if (edit.dvs) {
        for (uint8_t dv : *edit.dvs) {
            if (dv > 15) {
                error = "Generation II DVs must be in the range 0..15";
                return false;
            }
        }
        putEditableDVs(after, *edit.dvs);
    }

    if (edit.level && edit.experience) {
        error = "set either level or experience in one Pokemon edit, not both";
        return false;
    }
    const auto* personal = personalRecord(after.species);
    if (!personal) {
        error = "Generation II personal data is unavailable for the requested species";
        return false;
    }
    if (edit.level) {
        if (*edit.level < 1 || *edit.level > 100) {
            error = "Generation II level must be in the range 1..100";
            return false;
        }
        after.level = *edit.level;
        after.experience = Pokemon::getExpForLevel(after.level, personal->experienceGrowth);
    } else if (edit.experience) {
        const uint32_t maxExperience = Pokemon::getExpForLevel(100, personal->experienceGrowth);
        if (*edit.experience > maxExperience) {
            error = "experience exceeds this species' Generation II level-100 threshold";
            return false;
        }
        after.experience = *edit.experience;
        after.level = Pokemon::getLevelFromExp(after.experience, personal->experienceGrowth);
    } else if (edit.species) {
        const uint32_t maxExperience = Pokemon::getExpForLevel(100, personal->experienceGrowth);
        if (after.experience > maxExperience) after.experience = maxExperience;
        after.level = Pokemon::getLevelFromExp(after.experience, personal->experienceGrowth);
    }

    if (edit.moves) {
        after.moves = *edit.moves;
        if (!edit.ppUps) after.ppUps = {0,0,0,0};
        if (!edit.pp) {
            for (std::size_t i = 0; i < 4; ++i) after.pp[i] = gen2MoveBasePP(after.moves[i]);
        }
    }
    if (edit.ppUps) after.ppUps = *edit.ppUps;
    if (edit.pp) after.pp = *edit.pp;
    for (std::size_t i = 0; i < 4; ++i) {
        if (after.moves[i] > kMaxGen2Move) {
            error = "move id is outside the Generation II move range";
            return false;
        }
        if (after.ppUps[i] > 3) {
            error = "Generation II PP Ups are limited to 0..3";
            return false;
        }
        if (after.moves[i] == 0) {
            if (after.pp[i] != 0 || after.ppUps[i] != 0) {
                error = "an empty Generation II move slot must have zero PP and zero PP Ups";
                return false;
            }
        } else if (after.pp[i] > maxPP(after.moves[i], after.ppUps[i])) {
            error = "move PP exceeds the Generation II maximum for its move/PP-Up state";
            return false;
        }
    }

    // Species changes can alter the DV-derived gender; there is no independent gender byte in PK2.
    putEditableDVs(after, editableDVs(after));

    std::vector<uint8_t> nicknameBytes;
    std::vector<uint8_t> otBytes;
    if (!encodePokemonName(after.nickname, nicknameBytes, error) ||
        !encodePokemonName(after.originalTrainer, otBytes, error)) return false;

    const auto& layout = layoutFor(metadata_.family);
    if (!validBoxLocation(metadata_, layout, box, slot, error)) return false;
    const std::size_t list = boxStart(layout, box);
    const uint8_t count = staged_[list];
    if (slot >= count) {
        error = "cannot edit an empty Generation II box slot";
        return false;
    }

    const auto stagedBackup = staged_;
    const auto changesBackup = changes_;
    const auto expectationsBackup = pokemonExpectations_;
    const std::size_t body = boxBodyStart(layout, box) + slot * kStoredBodySize;
    writePokemonBody(staged_, body, after);
    staged_[list + 1 + slot] = static_cast<uint8_t>(after.species);
    std::copy(otBytes.begin(), otBytes.end(), staged_.begin() +
              static_cast<std::ptrdiff_t>(boxOTStart(layout, box) + slot * layout.stringLength));
    std::copy(nicknameBytes.begin(), nicknameBytes.end(), staged_.begin() +
              static_cast<std::ptrdiff_t>(boxNicknameStart(layout, box) + slot * layout.stringLength));
    if (!syncCurrentBoxCopy(box, error)) {
        staged_ = stagedBackup;
        return false;
    }

    auto verified = boxedPokemon(box, slot, error);
    if (!verified || !samePokemon(*verified, after)) {
        if (error.empty()) {
            const std::string mismatch = verified ? pokemonMismatchDetail(*verified, after)
                                                  : std::string("reparse unavailable");
            error = "staged Generation II Pokemon edit failed semantic round-trip";
            if (!mismatch.empty()) error += ": " + mismatch;
        }
        staged_ = stagedBackup;
        changes_ = changesBackup;
        pokemonExpectations_ = expectationsBackup;
        return false;
    }
    rememberPokemonExpectation(box, slot, *verified);

    const std::string prefix = "box:" + std::to_string(box) + ":" + std::to_string(slot) + ":";
    const std::string labelPrefix = "Box " + std::to_string(box + 1) + " Slot " + std::to_string(slot + 1) + " ";
    if (before.species != verified->species)
        setChange(prefix + "species", labelPrefix + "Species", std::to_string(before.species), std::to_string(verified->species));
    if (before.nickname != verified->nickname)
        setChange(prefix + "nickname", labelPrefix + "Nickname", before.nickname, verified->nickname);
    if (before.level != verified->level || before.experience != verified->experience)
        setChange(prefix + "level_exp", labelPrefix + "Level / Experience",
                  std::to_string(before.level) + " / " + std::to_string(before.experience),
                  std::to_string(verified->level) + " / " + std::to_string(verified->experience));
    if (before.heldItem != verified->heldItem)
        setChange(prefix + "held", labelPrefix + "Held Item", std::string(gen2ItemName(before.heldItem)),
                  verified->heldItem == 0 ? "NONE" : std::string(gen2ItemName(verified->heldItem)));
    if (before.moves != verified->moves)
        setChange(prefix + "moves", labelPrefix + "Moves", array4(before.moves), array4(verified->moves));
    if (before.pp != verified->pp || before.ppUps != verified->ppUps)
        setChange(prefix + "pp", labelPrefix + "Move PP", array4(before.pp), array4(verified->pp));
    if (editableDVs(before) != editableDVs(*verified))
        setChange(prefix + "dvs", labelPrefix + "DVs", array4(editableDVs(before)), array4(editableDVs(*verified)));
    if (before.statExperience != verified->statExperience)
        setChange(prefix + "statexp", labelPrefix + "Stat Experience",
                  statExpText(before.statExperience), statExpText(verified->statExperience));
    if (before.originalTrainer != verified->originalTrainer)
        setChange(prefix + "ot", labelPrefix + "OT Name", before.originalTrainer, verified->originalTrainer);
    if (before.trainerId != verified->trainerId)
        setChange(prefix + "tid", labelPrefix + "Trainer ID", std::to_string(before.trainerId), std::to_string(verified->trainerId));
    if (before.friendship != verified->friendship)
        setChange(prefix + "friendship", labelPrefix + "Friendship", std::to_string(before.friendship), std::to_string(verified->friendship));
    if (before.pokerus != verified->pokerus)
        setChange(prefix + "pokerus", labelPrefix + "Pokerus", std::to_string(before.pokerus), std::to_string(verified->pokerus));
    if (before.caughtData != verified->caughtData)
        setChange(prefix + "caught", labelPrefix + "Caught/Met Data", std::to_string(before.caughtData), std::to_string(verified->caughtData));
    return true;
}

bool StagedEditor::stageBoxPokemonShiny(std::size_t box, std::size_t slot, bool shiny,
                                        std::string& error) {
    auto currentOpt = boxedPokemon(box, slot, error);
    if (!currentOpt) return false;
    if (currentOpt->isEgg) {
        error = "egg shiny editing is not enabled in this milestone";
        return false;
    }
    const bool wasShiny = currentOpt->shiny;
    if (wasShiny == shiny) return true;
    auto dvs = editableDVs(*currentOpt);
    const PokemonGender oldGender = genderFromAttackDV(currentOpt->species, dvs[0]);

    if (shiny) {
        constexpr std::array<uint8_t, 8> candidates{2,3,6,7,10,11,14,15};
        bool found = false;
        uint8_t best = 0;
        int bestDistance = 99;
        for (uint8_t candidate : candidates) {
            const PokemonGender newGender = genderFromAttackDV(currentOpt->species, candidate);
            if ((oldGender == PokemonGender::Male || oldGender == PokemonGender::Female) &&
                newGender != oldGender) continue;
            const int distance = std::abs(static_cast<int>(candidate) - static_cast<int>(dvs[0]));
            if (!found || distance < bestDistance) {
                found = true;
                best = candidate;
                bestDistance = distance;
            }
        }
        if (!found) {
            error = "this Pokemon's current DV-derived gender cannot be preserved with a legal Generation II shiny DV combination";
            return false;
        }
        dvs = {best, 10, 10, 10};
    } else {
        // Preserve Attack DV so DV-derived gender is unchanged; breaking one required 10 is sufficient.
        dvs[1] = dvs[1] == 10 ? 9 : dvs[1];
        if (isShinyDVs(dvs)) dvs[2] = 9;
    }

    BoxPokemonEdit edit;
    edit.dvs = dvs;
    if (!stageBoxPokemonEdit(box, slot, edit, error)) return false;
    const std::string key = "box:" + std::to_string(box) + ":" + std::to_string(slot) + ":shiny";
    setChange(key, "Box " + std::to_string(box + 1) + " Slot " + std::to_string(slot + 1) + " Shiny",
              wasShiny ? "Yes" : "No", shiny ? "Yes" : "No");
    return true;
}

bool StagedEditor::stageCloneBoxPokemon(std::size_t sourceBox, std::size_t sourceSlot,
                                        std::size_t destinationBox, std::size_t& destinationSlot,
                                        std::string& error) {
    error.clear();
    auto source = boxedPokemon(sourceBox, sourceSlot, error);
    if (!source) return false;
    if (source->isEgg) {
        error = "egg cloning is not enabled in this milestone";
        return false;
    }
    const auto& layout = layoutFor(metadata_.family);
    if (destinationBox >= layout.boxCount || destinationBox >= metadata_.boxCount) {
        error = "clone destination box is outside the validated Gen II save";
        return false;
    }
    const std::size_t list = boxStart(layout, destinationBox);
    const uint8_t count = staged_[list];
    if (count >= layout.boxCapacity) {
        error = "clone destination box is full";
        return false;
    }
    destinationSlot = count;

    std::vector<uint8_t> otBytes, nicknameBytes;
    if (!encodePokemonName(source->originalTrainer, otBytes, error) ||
        !encodePokemonName(source->nickname, nicknameBytes, error)) return false;

    const auto stagedBackup = staged_;
    const auto changesBackup = changes_;
    const auto expectationsBackup = pokemonExpectations_;
    staged_[list] = static_cast<uint8_t>(count + 1);
    staged_[list + 1 + destinationSlot] = static_cast<uint8_t>(source->species);
    staged_[list + 1 + destinationSlot + 1] = 0xFF;
    writePokemonBody(staged_, boxBodyStart(layout, destinationBox) + destinationSlot * kStoredBodySize, *source);
    std::copy(otBytes.begin(), otBytes.end(), staged_.begin() + static_cast<std::ptrdiff_t>(
        boxOTStart(layout, destinationBox) + destinationSlot * layout.stringLength));
    std::copy(nicknameBytes.begin(), nicknameBytes.end(), staged_.begin() + static_cast<std::ptrdiff_t>(
        boxNicknameStart(layout, destinationBox) + destinationSlot * layout.stringLength));
    if (!syncCurrentBoxCopy(destinationBox, error)) {
        staged_ = stagedBackup;
        return false;
    }
    auto verified = boxedPokemon(destinationBox, destinationSlot, error);
    if (!verified || !samePokemon(*verified, *source)) {
        if (error.empty()) error = "cloned Generation II Pokemon failed semantic round-trip";
        staged_ = stagedBackup;
        changes_ = changesBackup;
        pokemonExpectations_ = expectationsBackup;
        return false;
    }
    rememberPokemonExpectation(destinationBox, destinationSlot, *verified);
    setChange("box:" + std::to_string(destinationBox) + ":" + std::to_string(destinationSlot) + ":clone",
              "Box " + std::to_string(destinationBox + 1) + " Slot " + std::to_string(destinationSlot + 1),
              "Empty", "Cloned " + verified->nickname + " Lv. " + std::to_string(verified->level));
    return true;
}

bool StagedEditor::stageAddBoxPokemon(std::size_t destinationBox, const BoxPokemonCreate& pokemon,
                                      std::size_t& destinationSlot, std::string& error) {
    error.clear();
    if (pokemon.species == 0 || pokemon.species > kMaxGen2Species || !personalRecord(pokemon.species)) {
        error = "new Pokemon species is outside the Generation II range";
        return false;
    }
    if (pokemon.level < 1 || pokemon.level > 100) {
        error = "new Pokemon level must be in the range 1..100";
        return false;
    }
    if (!validHeldItem(pokemon.heldItem)) {
        error = "new Pokemon held item is not a usable Generation II item id";
        return false;
    }
    for (uint8_t dv : pokemon.dvs) {
        if (dv > 15) {
            error = "new Pokemon DVs must be in the range 0..15";
            return false;
        }
    }
    for (std::size_t i = 0; i < 4; ++i) {
        if (pokemon.moves[i] > kMaxGen2Move || pokemon.ppUps[i] > 3) {
            error = "new Pokemon move or PP-Up value is outside the Generation II range";
            return false;
        }
    }

    PokemonRecord created;
    created.species = pokemon.species;
    created.heldItem = pokemon.heldItem;
    created.moves = pokemon.moves;
    created.trainerId = pokemon.trainerId.value_or(trainerId_);
    created.level = pokemon.level;
    created.statExperience = pokemon.statExperience;
    created.friendship = pokemon.friendship;
    created.pokerus = pokemon.pokerus;
    created.caughtData = pokemon.caughtData;
    created.nickname = pokemon.nickname.empty() ? std::string(Names::getSpeciesName(pokemon.species)) : pokemon.nickname;
    created.originalTrainer = pokemon.otName.empty() ? trainerName_ : pokemon.otName;
    created.partyRecord = false;
    created.isEgg = false;
    const auto* personal = personalRecord(created.species);
    created.experience = Pokemon::getExpForLevel(created.level, personal->experienceGrowth);
    putEditableDVs(created, pokemon.dvs);
    created.ppUps = pokemon.ppUps;
    for (std::size_t i = 0; i < 4; ++i) {
        created.pp[i] = pokemon.pp[i];
        if (created.moves[i] == 0) {
            if (created.pp[i] != 0 || created.ppUps[i] != 0) {
                error = "empty move slots on a new Pokemon must have zero PP and zero PP Ups";
                return false;
            }
        } else {
            if (created.pp[i] == 0) created.pp[i] = gen2MoveBasePP(created.moves[i]);
            if (created.pp[i] > maxPP(created.moves[i], created.ppUps[i])) {
                error = "new Pokemon PP exceeds the Generation II maximum";
                return false;
            }
        }
    }

    std::vector<uint8_t> otBytes, nicknameBytes;
    if (!encodePokemonName(created.originalTrainer, otBytes, error) ||
        !encodePokemonName(created.nickname, nicknameBytes, error)) return false;

    const auto& layout = layoutFor(metadata_.family);
    if (destinationBox >= layout.boxCount || destinationBox >= metadata_.boxCount) {
        error = "add destination box is outside the validated Gen II save";
        return false;
    }
    const std::size_t list = boxStart(layout, destinationBox);
    const uint8_t count = staged_[list];
    if (count >= layout.boxCapacity) {
        error = "add destination box is full";
        return false;
    }
    destinationSlot = count;

    const auto stagedBackup = staged_;
    const auto changesBackup = changes_;
    const auto expectationsBackup = pokemonExpectations_;
    staged_[list] = static_cast<uint8_t>(count + 1);
    staged_[list + 1 + destinationSlot] = static_cast<uint8_t>(created.species);
    staged_[list + 1 + destinationSlot + 1] = 0xFF;
    writePokemonBody(staged_, boxBodyStart(layout, destinationBox) + destinationSlot * kStoredBodySize, created);
    std::copy(otBytes.begin(), otBytes.end(), staged_.begin() + static_cast<std::ptrdiff_t>(
        boxOTStart(layout, destinationBox) + destinationSlot * layout.stringLength));
    std::copy(nicknameBytes.begin(), nicknameBytes.end(), staged_.begin() + static_cast<std::ptrdiff_t>(
        boxNicknameStart(layout, destinationBox) + destinationSlot * layout.stringLength));
    if (!syncCurrentBoxCopy(destinationBox, error)) {
        staged_ = stagedBackup;
        return false;
    }
    auto verified = boxedPokemon(destinationBox, destinationSlot, error);
    if (!verified || !samePokemon(*verified, created)) {
        if (error.empty()) error = "new Generation II Pokemon failed semantic round-trip";
        staged_ = stagedBackup;
        changes_ = changesBackup;
        pokemonExpectations_ = expectationsBackup;
        return false;
    }
    rememberPokemonExpectation(destinationBox, destinationSlot, *verified);
    setChange("box:" + std::to_string(destinationBox) + ":" + std::to_string(destinationSlot) + ":add",
              "Box " + std::to_string(destinationBox + 1) + " Slot " + std::to_string(destinationSlot + 1),
              "Empty", "Added " + verified->nickname + " Lv. " + std::to_string(verified->level));
    return true;
}

void StagedEditor::discard() noexcept {
    staged_ = original_;
    trainerName_ = originalTrainerName_;
    money_ = originalMoney_;
    changes_.clear();
    pokemonExpectations_.clear();
}

std::vector<uint8_t> StagedEditor::finalizedBytes(std::string& error) const {
    error.clear();
    std::vector<uint8_t> result = staged_;
    const auto& layout = layoutFor(metadata_.family);
    if (!repairForStrictReload(result, metadata_, error)) return {};

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
    for (const auto& expectation : pokemonExpectations_) {
        if (expectation.box >= reparsed.save->boxes().size() ||
            expectation.slot >= reparsed.save->boxes()[expectation.box].slots.size() ||
            !reparsed.save->boxes()[expectation.box].slots[expectation.slot] ||
            !samePokemon(*reparsed.save->boxes()[expectation.box].slots[expectation.slot], expectation.expected)) {
            error = "final staged Gen II export did not semantically round-trip a Pokemon edit";
            return {};
        }
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
