#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Integration/Gen4/Gen4ReadOnlyInventory.h"
#include "Integration/Gen4/Gen4BagCatalog.h"

#include "Encryption/Encryption4.h"
#include "Names/Gen4HeldItemCatalog.h"
#include "Names/SpeciesNames.h"
#include "Utils/CRC16.h"
#include "Utils/Gen4Text.h"
#include "Enums/LanguageID.h"

#include <algorithm>
#include <array>
#include <utility>

namespace PokeVault::Integration::Gen4 {
namespace {

struct GeneralGeometry {
    size_t size = 0;
    size_t footerSize = 0;
    size_t partyOffset = 0;
    size_t trainerOffset = 0; // PKHeX SAV4*.Trainer1, same values as Gen4ReadOnlySave's spec
};

GeneralGeometry generalGeometry(Layout layout) noexcept {
    switch (layout) {
        case Layout::DiamondPearl: return {0xC100, 0x14, 0x98, 0x64};
        case Layout::Platinum: return {0xCF2C, 0x14, 0xA0, 0x68};
        case Layout::HeartGoldSoulSilver: return {0xF628, 0x10, 0x98, 0x64};
    }
    return {};
}

constexpr size_t kTrainerNameField = 0x00;  // 8 u16 units
constexpr size_t kTrainerNameUnits = 8;
constexpr size_t kTrainerMoneyField = 0x14; // u32
constexpr size_t kTrainerGenderField = 0x18;   // u8, 0 male / 1 female (PKSM-Core Sav4::gender)
constexpr size_t kTrainerPlayTimeField = 0x22; // u16 hours, u8 minutes, u8 seconds (Sav4::played*)

struct StorageGeometry {
    size_t size = 0;
    size_t footerSize = 0;
    size_t boxDataOffset = 0;
    size_t boxStride = 0;
};

StorageGeometry storageGeometry(Layout layout) noexcept {
    switch (layout) {
        case Layout::DiamondPearl:
            return {0x121E0, 0x14, 0x4, 0xFF0};
        case Layout::Platinum:
            return {0x121E4, 0x14, 0x4, 0xFF0};
        case Layout::HeartGoldSoulSilver:
            return {0x12310, 0x10, 0x0, 0x1000};
    }
    return {};
}

void write16(std::vector<uint8_t>& bytes, size_t offset, uint16_t value) noexcept {
    if (offset + 1 >= bytes.size()) return;
    bytes[offset] = static_cast<uint8_t>(value);
    bytes[offset + 1] = static_cast<uint8_t>(value >> 8);
}

void setError(std::string* error, std::string value) {
    if (error) *error = std::move(value);
}

const char* layoutGameName(Layout layout) noexcept {
    switch (layout) {
        case Layout::DiamondPearl: return "Diamond/Pearl";
        case Layout::Platinum: return "Platinum";
        case Layout::HeartGoldSoulSilver: return "HeartGold/SoulSilver";
    }
    return "this Generation IV game";
}

std::string speciesLabel(uint16_t species) {
    const char* name = Names::getSpeciesName(species);
    return std::string(name ? name : "Species") + " #" + std::to_string(species);
}

// PK4 plaintext (PKHeX PK4): checksum u16 at 0x06 over 0x08..0x87.
uint16_t plainU16(std::span<const uint8_t> bytes, size_t offset) noexcept {
    return static_cast<uint16_t>(bytes[offset] | (static_cast<uint16_t>(bytes[offset + 1]) << 8));
}

} // namespace

Gen4StagedPokemonEditor::Gen4StagedPokemonEditor(
    std::vector<uint8_t> source, Layout layout, std::string exactGameId,
    Enums::GameVersion sourceGroup) noexcept
    : original_(source), staged_(std::move(source)), layout_(layout),
      exactGameId_(std::move(exactGameId)), sourceGroup_(sourceGroup) {}

std::optional<Gen4StagedPokemonEditor> Gen4StagedPokemonEditor::create(
    std::span<const uint8_t> source, Layout layout, std::string_view exactGameId,
    std::string* error) {
    if (exactGameId.empty()) {
        setError(error, "Gen IV staged editing requires an exact assigned game identity");
        return std::nullopt;
    }

    std::string parseError;
    auto parsed = Gen4ReadOnlySave::parse(source, layout, exactGameId, &parseError);
    if (!parsed) {
        setError(error, parseError.empty()
            ? "Gen IV staged source failed strict validation" : parseError);
        return std::nullopt;
    }
    if (parsed->assignmentStatus() != AssignmentStatus::Match) {
        setError(error, "Gen IV staged source does not match its exact game assignment");
        return std::nullopt;
    }
    if (parsed->recovered()) {
        setError(error,
            "Recovered older-copy Gen IV sources remain read-only and cannot start staged editing");
        return std::nullopt;
    }

    if (error) error->clear();
    return Gen4StagedPokemonEditor(
        std::vector<uint8_t>(source.begin(), source.end()), layout,
        std::string(exactGameId), parsed->rawFamily());
}

std::optional<Gen4ReadOnlySave> Gen4StagedPokemonEditor::reparse(
    std::string* error) const {
    std::string parseError;
    auto parsed = Gen4ReadOnlySave::parse(
        staged_, layout_, exactGameId_, &parseError);
    if (!parsed) {
        setError(error, parseError.empty()
            ? "staged Gen IV save failed strict reparse" : parseError);
        return std::nullopt;
    }
    if (parsed->assignmentStatus() != AssignmentStatus::Match) {
        setError(error, "staged Gen IV save lost exact game assignment");
        return std::nullopt;
    }
    if (parsed->recovered()) {
        setError(error, "staged Gen IV save unexpectedly reparsed as a recovered older copy");
        return std::nullopt;
    }
    if (error) error->clear();
    return parsed;
}

bool Gen4StagedPokemonEditor::originalEmptyRecordAt(
    const Gen4ReadOnlySave& stagedParse, size_t box, size_t slot, size_t offset) const {
    if (original_.size() != staged_.size()) return false;
    const auto original = Gen4ReadOnlySave::parse(original_, layout_, exactGameId_);
    if (!original) return false;
    if (original->storageSelection().offset != stagedParse.storageSelection().offset) return false;
    const auto originalOffset = boxRecordOffset(*original, box, slot);
    if (!originalOffset || *originalOffset != offset) return false;
    const auto& record = original->box(box, slot);
    return record.valid() && record.empty();
}

std::optional<size_t> Gen4StagedPokemonEditor::boxRecordOffset(
    const Gen4ReadOnlySave& parsed, size_t box, size_t slot) const noexcept {
    if (box >= 18 || slot >= 30) return std::nullopt;
    const auto geometry = storageGeometry(layout_);
    if (geometry.size == 0) return std::nullopt;

    const size_t offset = parsed.storageSelection().offset +
        geometry.boxDataOffset + box * geometry.boxStride +
        slot * Encryption::SIZE_STORED4;
    if (offset + Encryption::SIZE_STORED4 > staged_.size())
        return std::nullopt;
    if (offset + Encryption::SIZE_STORED4 >
        parsed.storageSelection().offset + geometry.size)
        return std::nullopt;
    return offset;
}

std::optional<size_t> Gen4StagedPokemonEditor::partyRecordOffset(
    const Gen4ReadOnlySave& parsed, size_t slot) const noexcept {
    if (slot >= 6 || slot >= parsed.partyCount()) return std::nullopt;
    const auto geometry = generalGeometry(layout_);
    if (geometry.size == 0) return std::nullopt;
    const size_t offset = parsed.generalSelection().offset +
        geometry.partyOffset + slot * Encryption::SIZE_PARTY4;
    if (offset + Encryption::SIZE_PARTY4 > staged_.size()) return std::nullopt;
    if (offset + Encryption::SIZE_PARTY4 >
        parsed.generalSelection().offset + geometry.size) return std::nullopt;
    return offset;
}

std::optional<Pokemon::Pokemon4ReadOnly> Gen4StagedPokemonEditor::boxedPokemon(
    size_t box, size_t slot, std::string* error) const {
    auto parsed = reparse(error);
    if (!parsed) return std::nullopt;
    if (box >= 18 || slot >= 30) {
        setError(error, "Gen IV box/slot index is outside the native 18 x 30 storage");
        return std::nullopt;
    }
    const auto& pokemon = parsed->box(box, slot);
    if (!pokemon.valid() || pokemon.empty()) {
        setError(error, pokemon.empty()
            ? "selected Gen IV box slot is empty"
            : "selected Gen IV PK4 failed strict validation");
        return std::nullopt;
    }
    if (error) error->clear();
    return pokemon;
}

std::optional<Pokemon::Pokemon4Mutable>
Gen4StagedPokemonEditor::editableBoxPokemon(
    size_t box, size_t slot, std::string* error) const {
    auto pokemon = boxedPokemon(box, slot, error);
    if (!pokemon) return std::nullopt;
    return Pokemon::Pokemon4Mutable::fromEncrypted(
        pokemon->originalEncryptedBytes(), sourceGroup_, error);
}

std::optional<Pokemon::Pokemon4ReadOnly> Gen4StagedPokemonEditor::partyPokemon(
    size_t slot, std::string* error) const {
    auto parsed = reparse(error);
    if (!parsed) return std::nullopt;
    if (slot >= parsed->partyCount() || slot >= 6) {
        setError(error, "Generation IV party slot is outside the occupied party");
        return std::nullopt;
    }
    const auto party = parsed->nativePartySlots();
    if (slot >= party.size()) {
        setError(error, "Generation IV party slot is outside the native party records");
        return std::nullopt;
    }
    const auto& pokemon = party[slot];
    if (!pokemon.valid() || pokemon.empty() || !pokemon.isParty()) {
        setError(error, pokemon.empty()
            ? "selected Generation IV party slot is empty"
            : "selected Generation IV party PK4 failed strict validation");
        return std::nullopt;
    }
    if (error) error->clear();
    return pokemon;
}

std::optional<Pokemon::Pokemon4Mutable>
Gen4StagedPokemonEditor::editablePartyPokemon(
    size_t slot, std::string* error) const {
    auto pokemon = partyPokemon(slot, error);
    if (!pokemon) return std::nullopt;
    return Pokemon::Pokemon4Mutable::fromEncrypted(
        pokemon->originalEncryptedBytes(), sourceGroup_, error);
}

std::optional<Pokemon::Pokemon4Mutable>
Gen4StagedPokemonEditor::createBoxDraft(
    size_t box, size_t slot, uint16_t species, std::string* error) const {
    auto parsed = reparse(error);
    if (!parsed) return std::nullopt;
    if (box >= 18 || slot >= 30) {
        setError(error, "Gen IV Create draft target is outside native storage");
        return std::nullopt;
    }
    const auto& destination = parsed->box(box, slot);
    if (!destination.valid() || !destination.empty()) {
        setError(error, destination.valid()
            ? "Gen IV Create draft target is already occupied"
            : "Gen IV Create draft target failed strict slot validation");
        return std::nullopt;
    }

    const auto exact = parsed->assignedExactGame();
    const auto origin = static_cast<uint8_t>(exact);
    if (Enums::getGameGroup(exact) != sourceGroup_) {
        setError(error, "Gen IV Create draft lost exact target game identity");
        return std::nullopt;
    }

    const auto& trainer = parsed->trainer();
    Pokemon::Pokemon4CreateDefaults defaults;
    defaults.species = species;
    defaults.tid = trainer.tid;
    defaults.sid = trainer.sid;
    defaults.language = Enums::safeLanguageForGroup(sourceGroup_, trainer.language);
    defaults.otGender = static_cast<uint8_t>(trainer.gender & 1u);
    defaults.originVersion = origin;
    defaults.level = 5;
    defaults.ball = 4; // native Poké Ball id
    defaults.metLevel = 5;
    defaults.metLocation = 0; // UI must choose a real encounter location before legality claims.
    defaults.otName = trainer.name;
    return Pokemon::Pokemon4Mutable::createStored(defaults, sourceGroup_, error);
}

bool Gen4StagedPokemonEditor::refreshStorageCrc(
    const Gen4ReadOnlySave& parsed, std::string* error) {
    const auto geometry = storageGeometry(layout_);
    const size_t offset = parsed.storageSelection().offset;
    if (geometry.size == 0 || geometry.footerSize >= geometry.size ||
        offset + geometry.size > staged_.size()) {
        setError(error, "Gen IV Storage block geometry is outside the staged save");
        return false;
    }

    const uint16_t crc = Utils::crc16ccitt(
        staged_.data() + offset, geometry.size - geometry.footerSize);
    write16(staged_, offset + geometry.size - 2, crc);
    return true;
}

bool Gen4StagedPokemonEditor::refreshGeneralCrc(
    const Gen4ReadOnlySave& parsed, std::string* error) {
    const auto geometry = generalGeometry(layout_);
    const size_t offset = parsed.generalSelection().offset;
    if (geometry.size == 0 || geometry.footerSize >= geometry.size ||
        offset + geometry.size > staged_.size()) {
        setError(error, "Gen IV General block geometry is outside the staged save");
        return false;
    }
    const uint16_t crc = Utils::crc16ccitt(
        staged_.data() + offset, geometry.size - geometry.footerSize);
    write16(staged_, offset + geometry.size - 2, crc);
    return true;
}

bool Gen4StagedPokemonEditor::commitBoxPokemon(
    size_t box, size_t slot, const Pokemon::Pokemon4Mutable& pokemon,
    std::string* error) {
    auto parsedBefore = reparse(error);
    if (!parsedBefore) return false;

    const auto offset = boxRecordOffset(*parsedBefore, box, slot);
    if (!offset) {
        setError(error, "Gen IV box/slot index is outside the selected Storage block");
        return false;
    }

    const auto encrypted = pokemon.encryptedBytes();
    if (encrypted.size() != Encryption::SIZE_STORED4) {
        setError(error, "Gen IV boxed edit did not serialize to a 0x88-byte PK4");
        return false;
    }
    Pokemon::Pokemon4ReadOnly candidate(encrypted, sourceGroup_);
    if (!candidate.valid() || candidate.empty()) {
        setError(error, "Gen IV boxed edit did not serialize to a valid occupied PK4");
        return false;
    }

    bool identical = true;
    for (size_t i = 0; i < encrypted.size(); ++i) {
        if (staged_[*offset + i] != static_cast<uint8_t>(encrypted[i])) {
            identical = false;
            break;
        }
    }
    if (identical) {
        if (error) error->clear();
        return true;
    }

    const auto backup = staged_;
    for (size_t i = 0; i < encrypted.size(); ++i)
        staged_[*offset + i] = static_cast<uint8_t>(encrypted[i]);

    if (!refreshStorageCrc(*parsedBefore, error)) {
        staged_ = backup;
        return false;
    }

    auto parsedAfter = reparse(error);
    if (!parsedAfter) {
        staged_ = backup;
        return false;
    }

    const auto& reparsed = parsedAfter->box(box, slot);
    if (!reparsed.valid() || reparsed.empty() ||
        !std::equal(reparsed.originalEncryptedBytes().begin(),
                    reparsed.originalEncryptedBytes().end(), encrypted.begin())) {
        staged_ = backup;
        setError(error, "Gen IV staged edit failed exact PK4 reparse verification");
        return false;
    }

    if (error) error->clear();
    return true;
}

bool Gen4StagedPokemonEditor::stageCreateBoxPokemon(
    size_t box, size_t slot, const Pokemon::Pokemon4Mutable& pokemon,
    std::string* error) {
    auto parsedBefore = reparse(error);
    if (!parsedBefore) return false;

    if (box >= 18 || slot >= 30) {
        setError(error, "Gen IV Create target is outside the native 18 x 30 storage");
        return false;
    }
    const auto& destination = parsedBefore->box(box, slot);
    if (!destination.valid()) {
        setError(error, "Gen IV Create target failed strict slot validation");
        return false;
    }
    if (!destination.empty()) {
        setError(error, "Gen IV Create refuses to overwrite an occupied box slot");
        return false;
    }

    const auto offset = boxRecordOffset(*parsedBefore, box, slot);
    if (!offset) {
        setError(error, "Gen IV Create target is outside the selected Storage block");
        return false;
    }

    const auto encrypted = pokemon.encryptedBytes();
    if (encrypted.size() != Encryption::SIZE_STORED4) {
        setError(error, "Gen IV Create draft did not serialize to a 0x88-byte stored PK4");
        return false;
    }
    Pokemon::Pokemon4ReadOnly candidate(encrypted, sourceGroup_);
    if (!candidate.valid() || candidate.empty() || candidate.isParty()) {
        setError(error, "Gen IV Create draft is not a valid occupied stored PK4");
        return false;
    }

    const auto backup = staged_;
    for (size_t i = 0; i < encrypted.size(); ++i)
        staged_[*offset + i] = static_cast<uint8_t>(encrypted[i]);

    if (!refreshStorageCrc(*parsedBefore, error)) {
        staged_ = backup;
        return false;
    }

    auto parsedAfter = reparse(error);
    if (!parsedAfter) {
        staged_ = backup;
        return false;
    }
    const auto& reparsed = parsedAfter->box(box, slot);
    if (!reparsed.valid() || reparsed.empty() ||
        !std::equal(reparsed.originalEncryptedBytes().begin(),
                    reparsed.originalEncryptedBytes().end(), encrypted.begin())) {
        staged_ = backup;
        setError(error, "Gen IV staged Create failed exact PK4 reparse verification");
        return false;
    }

    if (error) error->clear();
    return true;
}

bool Gen4StagedPokemonEditor::stageCloneBoxPokemon(
    size_t sourceBox, size_t sourceSlot,
    size_t destinationBox, size_t destinationSlot,
    std::string* error) {
    auto parsedBefore = reparse(error);
    if (!parsedBefore) return false;
    if (sourceBox >= 18 || destinationBox >= 18 ||
        sourceSlot >= 30 || destinationSlot >= 30) {
        setError(error, "Generation IV Clone box/slot is outside native 18 x 30 storage");
        return false;
    }

    const auto& source = parsedBefore->box(sourceBox, sourceSlot);
    const auto& destination = parsedBefore->box(destinationBox, destinationSlot);
    if (!source.valid() || source.empty()) {
        setError(error, "Generation IV Clone source is empty or invalid");
        return false;
    }
    if (!destination.valid() || !destination.empty()) {
        setError(error, destination.valid()
            ? "Generation IV Clone destination is occupied"
            : "Generation IV Clone destination failed strict slot validation");
        return false;
    }
    const auto destinationOffset =
        boxRecordOffset(*parsedBefore, destinationBox, destinationSlot);
    if (!destinationOffset) {
        setError(error, "Generation IV Clone destination is outside the selected Storage block");
        return false;
    }

    const auto sourceBytes = source.originalEncryptedBytes();
    if (sourceBytes.size() != Encryption::SIZE_STORED4) {
        setError(error, "Generation IV Clone source is not a stored 0x88-byte PK4");
        return false;
    }

    const auto backup = staged_;
    for (size_t i = 0; i < sourceBytes.size(); ++i)
        staged_[*destinationOffset + i] = static_cast<uint8_t>(sourceBytes[i]);

    if (!refreshStorageCrc(*parsedBefore, error)) {
        staged_ = backup;
        return false;
    }
    auto parsedAfter = reparse(error);
    if (!parsedAfter) {
        staged_ = backup;
        return false;
    }
    const auto& cloned = parsedAfter->box(destinationBox, destinationSlot);
    if (!cloned.valid() || cloned.empty() ||
        !std::equal(cloned.originalEncryptedBytes().begin(),
                    cloned.originalEncryptedBytes().end(), sourceBytes.begin())) {
        staged_ = backup;
        setError(error, "Generation IV staged Clone failed exact PK4 reparse verification");
        return false;
    }
    if (error) error->clear();
    return true;
}

// Feature A: standalone .pk4 / .ek4 import. The decoder (PokemonFile::decode) already proved the
// file is a Gen IV PK4 by content (checksum in exactly one reading, PKHeX GetFormat45); this adds
// the target game's own structural validators and never edits a byte of the record.
bool Gen4StagedPokemonEditor::canImportPokemonFile(
    const PokeBank::PokemonFile::Decoded& file, std::string* error) const {
    if (file.generation != 4 || file.format != PokeBank::PokemonFile::Format::PK4) {
        setError(error, "This is a Generation " + std::to_string(file.generation) +
            " Pokemon file; a Generation IV save accepts only .pk4 / .ek4 records "
            "(no conversion between generations)");
        return false;
    }
    if (file.plain.size() != Encryption::SIZE_STORED4 &&
        file.plain.size() != Encryption::SIZE_PARTY4) {
        setError(error, "The .pk4 record is not 136 (box) or 236 (party) bytes");
        return false;
    }
    // The stored 136 bytes; a party record's trailing battle-stat cache is not stored in a box.
    const std::span<const uint8_t> stored(file.plain.data(), Encryption::SIZE_STORED4);
    const auto storedBytes = std::as_bytes(stored);
    if (Encryption::checksum4(storedBytes) != plainU16(stored, 0x06)) {
        setError(error, "The .pk4 record's checksum does not match its data");
        return false;
    }
    const auto encrypted = Encryption::encryptArray4(storedBytes);
    const Pokemon::Pokemon4ReadOnly record(encrypted, sourceGroup_);
    if (!record.valid() || record.isParty() ||
        !std::equal(record.decryptedBytes().begin(), record.decryptedBytes().end(),
                    storedBytes.begin(), storedBytes.end())) {
        setError(error, "The .pk4 record failed strict PK4 checksum/sanity validation");
        return false;
    }
    if (record.empty()) {
        setError(error, "The .pk4 record holds no Pokemon (species 0)");
        return false;
    }

    const std::string game = layoutGameName(layout_);
    const uint16_t species = record.species();
    // Species and form must exist in THIS game's personal table (PKHeX PersonalTable.DP / .Pt /
    // .HGSS): e.g. Rotom appliance forms and Giratina Origin are Platinum+, Spiky-eared Pichu is
    // HeartGold/SoulSilver only. Same tables and stored-form rule as Pokemon4Mutable::setForm.
    if (species > 493 || record.personal().hp == 0) {
        setError(error, speciesLabel(species) + " does not exist in " + game);
        return false;
    }
    const auto editable = Pokemon::Pokemon4Mutable::fromEncrypted(encrypted, sourceGroup_, error);
    if (!editable) return false;
    if (record.form() >= editable->formCount()) {
        setError(error, speciesLabel(species) + " form " + std::to_string(record.form()) +
            " does not exist in " + game);
        return false;
    }
    // PKHeX FormVerifier: Shaymin Sky Forme cannot be kept in a Gen IV PC box (setForm rule).
    if (species == 492 && record.form() != 0) {
        setError(error, "Shaymin Sky Forme cannot be stored in a Generation IV PC box");
        return false;
    }
    // Holdable items per game: PKHeX ItemStorage4DP / ItemStorage4Pt GetAllHeld (setHeldItem rule).
    if (record.heldItem() != 0 && !Names::isGen4HeldItemPresent(record.heldItem(), sourceGroup_)) {
        setError(error, "Held item #" + std::to_string(record.heldItem()) +
            " is not a holdable item in " + game);
        return false;
    }
    // Native PK4 move range ends at Shadow Force (467), PKHeX Legal.MaxMoveID_4 (setMove rule).
    for (const auto move : record.moves()) {
        if (move > 467) {
            setError(error, "Move #" + std::to_string(move) +
                " is outside the Generation IV move list");
            return false;
        }
    }
    if (!Enums::groupHasLanguage(sourceGroup_, record.language())) {
        setError(error, "Language id " + std::to_string(record.language()) +
            " is not a Generation IV game language");
        return false;
    }
    if (error) error->clear();
    return true;
}

bool Gen4StagedPokemonEditor::stageImportBoxPokemon(
    size_t box, size_t slot, const PokeBank::PokemonFile::Decoded& file, std::string* error) {
    if (!canImportPokemonFile(file, error)) return false;
    auto parsedBefore = reparse(error);
    if (!parsedBefore) return false;
    if (box >= 18 || slot >= 30) {
        setError(error, "Generation IV Import target is outside native 18 x 30 storage");
        return false;
    }
    const auto& destination = parsedBefore->box(box, slot);
    if (!destination.valid() || !destination.empty()) {
        setError(error, destination.valid()
            ? "Generation IV Import refuses to overwrite an occupied box slot"
            : "Generation IV Import target failed strict slot validation");
        return false;
    }
    const auto offset = boxRecordOffset(*parsedBefore, box, slot);
    if (!offset) {
        setError(error, "Generation IV Import target is outside the selected Storage block");
        return false;
    }

    const auto plain = std::as_bytes(std::span<const uint8_t>(file.plain.data(),
                                                              Encryption::SIZE_STORED4));
    const auto encrypted = Encryption::encryptArray4(plain);
    if (encrypted.size() != Encryption::SIZE_STORED4) {
        setError(error, "Generation IV Import did not encrypt to a 0x88-byte stored PK4");
        return false;
    }

    const auto backup = staged_;
    for (size_t i = 0; i < encrypted.size(); ++i)
        staged_[*offset + i] = static_cast<uint8_t>(encrypted[i]);

    if (!refreshStorageCrc(*parsedBefore, error)) {
        staged_ = backup;
        return false;
    }
    auto parsedAfter = reparse(error);
    if (!parsedAfter) {
        staged_ = backup;
        return false;
    }
    const auto& imported = parsedAfter->box(box, slot);
    if (!imported.valid() || imported.empty() ||
        !std::equal(imported.originalEncryptedBytes().begin(),
                    imported.originalEncryptedBytes().end(),
                    encrypted.begin(), encrypted.end()) ||
        !std::equal(imported.decryptedBytes().begin(), imported.decryptedBytes().end(),
                    plain.begin(), plain.end())) {
        staged_ = backup;
        setError(error, "Generation IV staged Import failed exact PK4 reparse verification");
        return false;
    }
    if (error) error->clear();
    return true;
}

std::optional<std::vector<uint8_t>> Gen4StagedPokemonEditor::exportBoxPokemonFile(
    size_t box, size_t slot, std::string* error) const {
    const auto pokemon = boxedPokemon(box, slot, error);
    if (!pokemon) return std::nullopt;
    const auto plain = pokemon->decryptedBytes();
    if (plain.size() != Encryption::SIZE_STORED4) {
        setError(error, "Generation IV box record is not a stored 0x88-byte PK4");
        return std::nullopt;
    }
    std::vector<uint8_t> out(plain.size());
    std::transform(plain.begin(), plain.end(), out.begin(),
                   [](std::byte value) { return static_cast<uint8_t>(value); });
    if (error) error->clear();
    return out;
}

std::optional<std::vector<uint8_t>> Gen4StagedPokemonEditor::exportPartyPokemonFile(
    size_t slot, std::string* error) const {
    const auto pokemon = partyPokemon(slot, error);
    if (!pokemon) return std::nullopt;
    const auto plain = pokemon->decryptedBytes();
    if (plain.size() != Encryption::SIZE_PARTY4) {
        setError(error, "Generation IV party record is not a 0xEC-byte party PK4");
        return std::nullopt;
    }
    std::vector<uint8_t> out(plain.size());
    std::transform(plain.begin(), plain.end(), out.begin(),
                   [](std::byte value) { return static_cast<uint8_t>(value); });
    if (error) error->clear();
    return out;
}

bool Gen4StagedPokemonEditor::stageMoveBoxPokemon(
    size_t sourceBox, size_t sourceSlot,
    size_t destinationBox, size_t destinationSlot,
    std::string* error) {
    auto parsedBefore = reparse(error);
    if (!parsedBefore) return false;
    if (sourceBox >= 18 || destinationBox >= 18 ||
        sourceSlot >= 30 || destinationSlot >= 30) {
        setError(error, "Gen IV Move box/slot is outside native 18 x 30 storage");
        return false;
    }
    const auto& source = parsedBefore->box(sourceBox, sourceSlot);
    const auto& destination = parsedBefore->box(destinationBox, destinationSlot);
    if (!source.valid() || source.empty() || !destination.valid()) {
        setError(error, "Gen IV Move requires a valid occupied source and valid destination");
        return false;
    }
    if (sourceBox == destinationBox && sourceSlot == destinationSlot) {
        if (error) error->clear();
        return true;
    }
    const auto from = boxRecordOffset(*parsedBefore, sourceBox, sourceSlot);
    const auto to = boxRecordOffset(*parsedBefore, destinationBox, destinationSlot);
    if (!from || !to || *from == *to) {
        setError(error, "Gen IV Move target is outside native Storage blocks");
        return false;
    }

    // Source and destination raw bytes (including any unknown stored fields)
    // are swapped verbatim. We never rebuild or lose a PK4 field.
    const auto backup = staged_;
    constexpr size_t recordSize = Encryption::SIZE_STORED4;
    for (size_t i = 0; i < recordSize; ++i)
        std::swap(staged_[*from + i], staged_[*to + i]);

    if (!refreshStorageCrc(*parsedBefore, error)) {
        staged_ = backup;
        return false;
    }
    auto parsedAfter = reparse(error);
    if (!parsedAfter) {
        staged_ = backup;
        return false;
    }
    const auto& moved = parsedAfter->box(destinationBox, destinationSlot);
    const auto& replaced = parsedAfter->box(sourceBox, sourceSlot);
    // Validate the complete native 0x88-byte records against the backup,
    // including empty slots whose read-only wrapper may not expose a
    // nonempty encrypted-record span. Never infer an empty PK4 from its ID.
    const bool identicalDestination = std::equal(
        staged_.begin() + static_cast<std::ptrdiff_t>(*to),
        staged_.begin() + static_cast<std::ptrdiff_t>(*to + recordSize),
        backup.begin() + static_cast<std::ptrdiff_t>(*from));
    const bool identicalSource = std::equal(
        staged_.begin() + static_cast<std::ptrdiff_t>(*from),
        staged_.begin() + static_cast<std::ptrdiff_t>(*from + recordSize),
        backup.begin() + static_cast<std::ptrdiff_t>(*to));
    if (!moved.valid() || moved.empty() || !replaced.valid() ||
        replaced.empty() != destination.empty() ||
        !identicalDestination || !identicalSource) {
        staged_ = backup;
        setError(error, "Gen IV Move/swap failed native PK4 read-back verification");
        return false;
    }
    if (error) error->clear();
    return true;
}

bool Gen4StagedPokemonEditor::stageReleaseBoxPokemon(
    size_t box, size_t slot, std::string* error) {
    auto parsedBefore = reparse(error);
    if (!parsedBefore) return false;
    if (box >= 18 || slot >= 30) {
        setError(error, "Generation IV Release box/slot is outside native 18 x 30 storage");
        return false;
    }
    const auto& source = parsedBefore->box(box, slot);
    if (!source.valid() || source.empty()) {
        setError(error, "Generation IV Release source is empty or invalid");
        return false;
    }
    const auto offset = boxRecordOffset(*parsedBefore, box, slot);
    if (!offset) {
        setError(error, "Generation IV Release target is outside the selected Storage block");
        return false;
    }

    // A slot that started empty gets its original empty record back (retail empty box slots
    // hold an encrypted blank, not zeros), so clone + release of the clone is byte-identical to
    // the source. A slot that held a Pokemon in the source is zero-filled, which reparses as
    // empty. The original slot only counts when the source selected the same Storage copy.
    const auto backup = staged_;
    const auto restore = originalEmptyRecordAt(*parsedBefore, box, slot, *offset);
    if (restore)
        std::copy_n(original_.begin() + static_cast<std::ptrdiff_t>(*offset),
                    Encryption::SIZE_STORED4,
                    staged_.begin() + static_cast<std::ptrdiff_t>(*offset));
    else
        std::fill_n(staged_.begin() + static_cast<std::ptrdiff_t>(*offset),
                    Encryption::SIZE_STORED4, uint8_t{0});

    if (!refreshStorageCrc(*parsedBefore, error)) {
        staged_ = backup;
        return false;
    }
    auto parsedAfter = reparse(error);
    if (!parsedAfter) {
        staged_ = backup;
        return false;
    }
    const auto& released = parsedAfter->box(box, slot);
    if (!released.valid() || !released.empty()) {
        staged_ = backup;
        setError(error, "Generation IV staged Release failed empty-slot reparse verification");
        return false;
    }
    if (error) error->clear();
    return true;
}

bool Gen4StagedPokemonEditor::commitPartyPokemon(
    size_t slot, const Pokemon::Pokemon4Mutable& pokemon,
    std::string* error) {
    auto parsedBefore = reparse(error);
    if (!parsedBefore) return false;

    const auto offset = partyRecordOffset(*parsedBefore, slot);
    if (!offset) {
        setError(error, "Generation IV party slot is outside the selected General block");
        return false;
    }

    const auto encrypted = pokemon.encryptedBytes();
    if (encrypted.size() != Encryption::SIZE_PARTY4) {
        setError(error, "Generation IV party edit did not serialize to a 0xEC-byte PK4");
        return false;
    }
    Pokemon::Pokemon4ReadOnly candidate(encrypted, sourceGroup_);
    if (!candidate.valid() || candidate.empty() || !candidate.isParty()) {
        setError(error, "Generation IV party edit did not serialize to a valid occupied party PK4");
        return false;
    }

    bool identical = true;
    for (size_t i = 0; i < encrypted.size(); ++i) {
        if (staged_[*offset + i] != static_cast<uint8_t>(encrypted[i])) {
            identical = false;
            break;
        }
    }
    if (identical) {
        if (error) error->clear();
        return true;
    }

    const auto backup = staged_;
    for (size_t i = 0; i < encrypted.size(); ++i)
        staged_[*offset + i] = static_cast<uint8_t>(encrypted[i]);

    if (!refreshGeneralCrc(*parsedBefore, error)) {
        staged_ = backup;
        return false;
    }

    auto parsedAfter = reparse(error);
    if (!parsedAfter) {
        staged_ = backup;
        return false;
    }
    const auto party = parsedAfter->nativePartySlots();
    if (slot >= parsedAfter->partyCount() || slot >= party.size()) {
        staged_ = backup;
        setError(error, "Generation IV staged party edit changed the party layout");
        return false;
    }
    const auto& reparsed = party[slot];
    if (!reparsed.valid() || reparsed.empty() || !reparsed.isParty() ||
        !std::equal(reparsed.originalEncryptedBytes().begin(),
                    reparsed.originalEncryptedBytes().end(), encrypted.begin())) {
        staged_ = backup;
        setError(error, "Generation IV staged party edit failed exact PK4 reparse verification");
        return false;
    }

    if (error) error->clear();
    return true;
}

bool Gen4StagedPokemonEditor::stageBagQuantity(
    size_t pocket,size_t visibleIndex,uint16_t quantity,std::string* error) {
    // No removal by accidentally setting zero, no arbitrary item creation,
    // no source writes and no writes to a recovered General partition.
    if(quantity==0 || quantity>999 || pocket>=BagPocketCount) {
        setError(error,"Gen IV item quantity must be 1..999 in a native pocket");
        return false;
    }
    auto before=reparse(error);
    if(!before)return false;
    const auto previousBag=decodeReadOnlyBag(*before);
    if(!previousBag || visibleIndex>=(*previousBag)[pocket].size()) {
        setError(error,"Gen IV item row is unvalidated or out of range");
        return false;
    }
    const auto& expected=(*previousBag)[pocket][visibleIndex];
    if(!gen4BagItemAllowed(layout_,pocket,expected.itemId) ||
       quantity>gen4BagMaxQuantity(pocket,expected.itemId)) {
        setError(error,"Gen IV item quantity exceeds this native pocket or item limit");
        return false;
    }
    const auto slots=bagLayout(layout_)[pocket];
    const size_t base=before->generalSelection().offset;
    const size_t blockSize=generalGeometry(layout_).size;
    if(slots.offset>blockSize || slots.slots>(blockSize-slots.offset)/4 ||
       base>staged_.size() || blockSize>staged_.size()-base) {
        setError(error,"Gen IV selected General pocket region is invalid");
        return false;
    }
    size_t populated=0;
    size_t target=0;
    bool found=false;
    for(size_t slot=0;slot<slots.slots;++slot) {
        const size_t at=base+slots.offset+slot*4;
        const uint16_t id=uint16_t(staged_[at]) | (uint16_t(staged_[at+1])<<8);
        const uint16_t count=uint16_t(staged_[at+2]) | (uint16_t(staged_[at+3])<<8);
        if(id==0 || id==0xFFFF || count==0)continue;
        if(populated++==visibleIndex) {
            if(id!=expected.itemId || count!=expected.count) {
                setError(error,"Gen IV item identity or quantity changed during selection");
                return false;
            }
            target=at;
            found=true;
            break;
        }
    }
    if(!found) {
        setError(error,"Gen IV inventory stack no longer exists");
        return false;
    }
    if(quantity==expected.count) {
        if(error)error->clear();
        return true;
    }

    auto old=staged_;
    write16(staged_,target+2,quantity);
    if(!refreshGeneralCrc(*before,error)) {
        staged_=std::move(old);
        return false;
    }
    const auto after=reparse(error);
    if(!after) {
        staged_=std::move(old);
        return false;
    }
    const auto nextBag=decodeReadOnlyBag(*after);
    if(!nextBag || (*nextBag)[pocket].size()!=(*previousBag)[pocket].size() ||
       (*nextBag)[pocket][visibleIndex].itemId!=expected.itemId ||
       (*nextBag)[pocket][visibleIndex].count!=quantity) {
        staged_=std::move(old);
        setError(error,"Gen IV bag change failed exact native pouch reparse");
        return false;
    }
    // Verify the COMPLETE staged image: only quantity's two bytes and the
    // selected General footer checksum may differ from the baseline image.
    const size_t checksumAt=base+blockSize-2;
    for(size_t i=0;i<staged_.size();++i) {
        const bool permitted=(i==target+2 || i==target+3 ||
                              i==checksumAt || i==checksumAt+1);
        if(!permitted && staged_[i]!=old[i]) {
            staged_=std::move(old);
            setError(error,"Gen IV item edit unexpectedly changed unrelated save bytes");
            return false;
        }
    }
    if(error)error->clear();
    return true;
}

bool Gen4StagedPokemonEditor::stageBagRemove(
    size_t pocket,size_t visibleIndex,std::string* error) {
    // This edits app-owned staged bytes only. The UI MUST ask for a separate
    // destructive confirmation; never treat A or count=0 as a removal.
    if(pocket>=BagPocketCount) {
        setError(error,"Gen IV item removal references an invalid native pocket");
        return false;
    }
    const auto before=reparse(error);
    if(!before)return false;
    const auto oldBag=decodeReadOnlyBag(*before);
    if(!oldBag || visibleIndex>=(*oldBag)[pocket].size()) {
        setError(error,"Gen IV item removal requires a validated existing stack");
        return false;
    }
    const auto expected=(*oldBag)[pocket][visibleIndex];
    const auto spec=bagLayout(layout_)[pocket];
    const size_t base=before->generalSelection().offset;
    const size_t blockSize=generalGeometry(layout_).size;
    if(spec.offset>blockSize || spec.slots>(blockSize-spec.offset)/4 ||
       base>staged_.size() || blockSize>staged_.size()-base) {
        setError(error,"Gen IV selected General pocket geometry is invalid");
        return false;
    }
    // The game lists a pocket up to its FIRST slot with no item or a zero quantity
    // (pret pokediamond/pokeplatinum/pokeheartgold bag list builders) and removal
    // packs the pocket (PocketCompaction / Pocket_SortEmpty). Packing is only safe
    // when the app's list and the game's list are the same stacks: a pocket whose
    // stacks sit after an empty slot, or that holds an item ID 0xFFFF stack, is
    // pre-existing native damage. Refuse rather than rearrange it.
    const auto slotAt=[&](size_t slot){ return base+spec.offset+slot*4; };
    const auto read16=[&](size_t at){
        return uint16_t(uint16_t(staged_[at]) | (uint16_t(staged_[at+1])<<8));
    };
    size_t gameListed=0;
    while(gameListed<spec.slots &&
          read16(slotAt(gameListed))!=0 && read16(slotAt(gameListed)+2)!=0)++gameListed;
    for(size_t slot=0;slot<spec.slots;++slot) {
        const uint16_t id=read16(slotAt(slot));
        const uint16_t quantity=read16(slotAt(slot)+2);
        const bool inGameList=slot<gameListed;
        const bool appVisible=id!=0 && id!=0xFFFF && quantity!=0;
        if(inGameList!=appVisible) {
            setError(error,"Gen IV Remove refused: this pocket already has an empty or "
                           "unreadable slot before other items. It is left unchanged.");
            return false;
        }
    }
    size_t foundAt=0;
    size_t populated=0;
    bool found=false;
    for(size_t slot=0;slot<gameListed;++slot) {
        const size_t at=slotAt(slot);
        const uint16_t id=read16(at);
        const uint16_t quantity=read16(at+2);
        if(populated++==visibleIndex) {
            if(id!=expected.itemId || quantity!=expected.count) {
                setError(error,"Gen IV item removal selection changed");
                return false;
            }
            foundAt=at;
            found=true;
            break;
        }
    }
    if(!found) {
        setError(error,"Gen IV item removal selected a missing native stack");
        return false;
    }
    auto backup=staged_;
    // Native behaviour (pret Bag_TakeItem / Bag_TryRemoveItem -> pocket compaction): a removed
    // stack never leaves a hole, since a hole would hide every later stack in game. Shift the
    // listed stacks after it up one slot and blank the last listed slot. Slots past the game's
    // list (ghost or filler data) are not listed in game either way and stay byte-identical.
    const size_t listEnd=slotAt(gameListed);
    for(size_t at=foundAt;at+4<listEnd;at+=4)
        for(size_t k=0;k<4;++k)staged_[at+k]=staged_[at+4+k];
    for(size_t k=0;k<4;++k)staged_[listEnd-4+k]=0;
    if(!refreshGeneralCrc(*before,error)) {
        staged_=std::move(backup);
        return false;
    }
    const auto after=reparse(error);
    const auto newBag=after?decodeReadOnlyBag(*after):std::nullopt;
    if(!newBag || (*newBag)[pocket].size()+1!=(*oldBag)[pocket].size()) {
        staged_=std::move(backup);
        setError(error,"Gen IV removal failed strict native bag reparse");
        return false;
    }
    for(size_t i=0;i<BagPocketCount;++i) {
        const auto& prior=(*oldBag)[i];
        const auto& next=(*newBag)[i];
        const size_t offset=(i==pocket)?1:0;
        if(next.size()+offset!=prior.size()) {
            staged_=std::move(backup);
            setError(error,"Gen IV item removal changed another pocket");
            return false;
        }
        for(size_t n=0;n<next.size();++n) {
            const size_t index=(i==pocket && n>=visibleIndex)?n+1:n;
            if(prior[index].itemId!=next[n].itemId ||
               prior[index].count!=next[n].count) {
                staged_=std::move(backup);
                setError(error,"Gen IV item removal modified an unrelated stack");
                return false;
            }
        }
    }
    // Audit every byte, not merely the displayed pouch. Only the listed slots
    // from the removed stack on and the verified General CRC may change.
    const size_t crcAt=base+blockSize-2;
    for(size_t i=0;i<staged_.size();++i) {
        if(i>=foundAt && i<listEnd)continue;
        if(i==crcAt || i==crcAt+1)continue;
        if(staged_[i]!=backup[i]) {
            staged_=std::move(backup);
            setError(error,"Gen IV item removal modified unrelated save bytes");
            return false;
        }
    }
    if(error)error->clear();
    return true;
}

bool Gen4StagedPokemonEditor::stageBagAdd(
    size_t pocket,uint16_t itemId,uint16_t quantity,std::string* error) {
    if(!gen4BagItemAllowed(layout_,pocket,itemId) ||
       quantity==0 || quantity>gen4BagMaxQuantity(pocket,itemId)) {
        setError(error,"Gen IV Add rejected: unsupported game, pouch, item or quantity");
        return false;
    }
    const auto before=reparse(error);
    if(!before)return false;
    const auto previousBag=decodeReadOnlyBag(*before);
    if(!previousBag) {
        setError(error,"Gen IV Add requires a completely validated native bag");
        return false;
    }
    const auto& prior=(*previousBag)[pocket];
    for(const auto& stack:prior) {
        if(stack.itemId==itemId) {
            setError(error,"Gen IV item already exists in this native pocket; edit quantity instead");
            return false;
        }
    }
    const auto spec=bagLayout(layout_)[pocket];
    const size_t base=before->generalSelection().offset;
    const size_t blockSize=generalGeometry(layout_).size;
    if(spec.offset>blockSize || spec.slots>(blockSize-spec.offset)/4 ||
       base>staged_.size() || blockSize>staged_.size()-base) {
        setError(error,"Gen IV Add selected native pocket geometry is invalid");
        return false;
    }
    // Append after the last occupied native slot so the shared visible item
    // order remains stable. Never reuse nonzero-ID/zero-quantity ghost slots
    // or overwrite a slot whose bytes are not a native blank sentinel.
    size_t afterLast=0;
    for(size_t slot=0;slot<spec.slots;++slot) {
        const size_t at=base+spec.offset+slot*4;
        const uint16_t id=uint16_t(staged_[at]) | (uint16_t(staged_[at+1])<<8);
        const uint16_t count=uint16_t(staged_[at+2]) | (uint16_t(staged_[at+3])<<8);
        if(id!=0 && id!=0xFFFF && count!=0)afterLast=slot+1;
    }
    size_t target=0;
    bool found=false;
    for(size_t slot=afterLast;slot<spec.slots;++slot) {
        const size_t at=base+spec.offset+slot*4;
        const uint16_t id=uint16_t(staged_[at]) | (uint16_t(staged_[at+1])<<8);
        const uint16_t count=uint16_t(staged_[at+2]) | (uint16_t(staged_[at+3])<<8);
        if((id==0 && count==0) ||
           (id==0xFFFF && count==0xFFFF)) {
            target=at;
            found=true;
            break;
        }
    }
    if(!found) {
        setError(error,"Gen IV Add refused: native pouch has no verified trailing empty slot");
        return false;
    }

    auto backup=staged_;
    write16(staged_,target,itemId);
    write16(staged_,target+2,quantity);
    if(!refreshGeneralCrc(*before,error)) {
        staged_=std::move(backup);
        return false;
    }
    const auto after=reparse(error);
    const auto current=after?decodeReadOnlyBag(*after):std::nullopt;
    if(!current || (*current)[pocket].size()!=prior.size()+1 ||
       (*current)[pocket].back().itemId!=itemId ||
       (*current)[pocket].back().count!=quantity) {
        staged_=std::move(backup);
        setError(error,"Gen IV Add failed exact native pocket reparse");
        return false;
    }
    for(size_t i=0;i<BagPocketCount;++i) {
        const auto& oldPocket=(*previousBag)[i];
        const auto& newPocket=(*current)[i];
        if(newPocket.size()!=oldPocket.size()+(i==pocket?1:0)) {
            staged_=std::move(backup);
            setError(error,"Gen IV Add changed another native pocket");
            return false;
        }
        for(size_t n=0;n<oldPocket.size();++n) {
            if(oldPocket[n].itemId!=newPocket[n].itemId ||
               oldPocket[n].count!=newPocket[n].count) {
                staged_=std::move(backup);
                setError(error,"Gen IV Add changed an existing item identity");
                return false;
            }
        }
    }
    const size_t crcAt=base+blockSize-2;
    for(size_t i=0;i<staged_.size();++i) {
        if(i>=target && i<target+4)continue;
        if(i==crcAt || i==crcAt+1)continue;
        if(staged_[i]!=backup[i]) {
            staged_=std::move(backup);
            setError(error,"Gen IV Add changed unrelated original save bytes");
            return false;
        }
    }
    if(error)error->clear();
    return true;
}

std::optional<size_t> Gen4StagedPokemonEditor::trainerRecordOffset(
    const Gen4ReadOnlySave& parsed) const noexcept {
    const auto geometry = generalGeometry(layout_);
    const size_t base = parsed.generalSelection().offset;
    // The whole trainer struct read by Gen4ReadOnlySave (0x26 bytes) must sit inside the selected
    // General block, before its footer.
    if (geometry.size == 0 || geometry.trainerOffset + 0x26 > geometry.size - geometry.footerSize ||
        base > staged_.size() || geometry.size > staged_.size() - base) return std::nullopt;
    return base + geometry.trainerOffset;
}

template <class Verify>
bool Gen4StagedPokemonEditor::stageTrainerBytes(size_t field, std::span<const uint8_t> bytes,
                                                Verify&& verify, std::string* error) {
    auto before = reparse(error);
    if (!before) return false;
    const auto base = trainerRecordOffset(*before);
    if (!base) {
        setError(error, "Gen IV trainer structure is outside the selected General block");
        return false;
    }
    auto backup = staged_;
    std::copy(bytes.begin(), bytes.end(), staged_.begin() + static_cast<std::ptrdiff_t>(*base + field));
    if (!refreshGeneralCrc(*before, error)) {
        staged_ = std::move(backup);
        return false;
    }
    auto after = reparse(error);
    if (!after || after->generalSelection().offset != before->generalSelection().offset ||
        !verify(after->trainer())) {
        if (after) setError(error, "staged Gen IV trainer field did not reparse as entered");
        staged_ = std::move(backup);
        return false;
    }
    if (error) error->clear();
    return true;
}

std::optional<TrainerReadOnly> Gen4StagedPokemonEditor::stagedTrainer(std::string* error) const {
    auto parsed = reparse(error);
    if (!parsed) return std::nullopt;
    return parsed->trainer();
}

bool Gen4StagedPokemonEditor::trainerNameEditable() const {
    const auto trainer = stagedTrainer();
    if (!trainer) return false;
    using L = Enums::LanguageID;
    switch (static_cast<L>(trainer->language)) {
        case L::English: case L::French: case L::Italian: case L::German: case L::Spanish:
            return true;
        default:
            return false;
    }
}

bool Gen4StagedPokemonEditor::stageTrainerName(const std::u16string& name, std::string* error) {
    if (!trainerNameEditable()) {
        setError(error, "This Gen IV save's trainer name language is read-only here");
        return false;
    }
    if (name.empty() || name.size() > kTrainerNameMax) {
        setError(error, "Gen IV trainer name must be 1-7 characters");
        return false;
    }
    if (!Utils::gen4CanEncode(name)) {
        setError(error, "Gen IV trainer name has a character the game cannot store");
        return false;
    }
    std::array<uint8_t, kTrainerNameUnits * 2> field{};
    Utils::writeGen4Field(field.data(), kTrainerNameUnits, name, kTrainerNameMax);
    return stageTrainerBytes(kTrainerNameField, field,
        [&](const TrainerReadOnly& trainer) { return trainer.name == name; }, error);
}

bool Gen4StagedPokemonEditor::stageTrainerMoney(uint32_t money, std::string* error) {
    if (money > kMaxMoney) {
        setError(error, "Gen IV money must be 0-999999");
        return false;
    }
    const std::array<uint8_t, 4> field{static_cast<uint8_t>(money), static_cast<uint8_t>(money >> 8),
                                       static_cast<uint8_t>(money >> 16), static_cast<uint8_t>(money >> 24)};
    return stageTrainerBytes(kTrainerMoneyField, field,
        [&](const TrainerReadOnly& trainer) { return trainer.money == money; }, error);
}

bool Gen4StagedPokemonEditor::trainerGenderEditable() const {
    const auto base = [&]() -> std::optional<size_t> {
        const auto parsed = reparse();
        return parsed ? trainerRecordOffset(*parsed) : std::nullopt;
    }();
    // Only a stored 0 / 1 byte is a native choice; any other value stays read-only, never rewritten.
    return base && staged_[*base + kTrainerGenderField] <= 1;
}

bool Gen4StagedPokemonEditor::stageTrainerGender(uint8_t gender, std::string* error) {
    if (gender > 1) {
        setError(error, "Gen IV trainer gender must be Male or Female");
        return false;
    }
    if (!trainerGenderEditable()) {
        setError(error, "This save's trainer gender byte is not a native Male / Female value");
        return false;
    }
    const std::array<uint8_t, 1> field{gender};
    return stageTrainerBytes(kTrainerGenderField, field,
        [&](const TrainerReadOnly& trainer) { return trainer.gender == gender; }, error);
}

bool Gen4StagedPokemonEditor::stageTrainerPlayTime(uint16_t hours, uint8_t minutes, uint8_t seconds,
                                                   std::string* error) {
    if (hours > kMaxPlayedHours || minutes > 59 || seconds > 59) {
        setError(error, "Gen IV play time must be 0:00:00-999:59:59");
        return false;
    }
    const std::array<uint8_t, 4> field{static_cast<uint8_t>(hours), static_cast<uint8_t>(hours >> 8),
                                       minutes, seconds};
    return stageTrainerBytes(kTrainerPlayTimeField, field,
        [&](const TrainerReadOnly& trainer) {
            return trainer.playedHours == hours && trainer.playedMinutes == minutes &&
                   trainer.playedSeconds == seconds;
        }, error);
}

std::vector<uint8_t> Gen4StagedPokemonEditor::finalizedBytes(
    std::string* error) const {
    if (staged_.size() != original_.size()) {
        setError(error, "Gen IV staged save size changed unexpectedly");
        return {};
    }
    if (!reparse(error)) return {};
    if (error) error->clear();
    return staged_;
}

} // namespace PokeVault::Integration::Gen4
