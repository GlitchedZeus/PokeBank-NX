#include "Integration/Gen4/Gen4StagedPokemonEditor.h"

#include "Encryption/Encryption4.h"
#include "Utils/CRC16.h"

#include <algorithm>
#include <utility>

namespace PokeVault::Integration::Gen4 {
namespace {

struct GeneralGeometry {
    size_t size = 0;
    size_t footerSize = 0;
    size_t partyOffset = 0;
};

GeneralGeometry generalGeometry(Layout layout) noexcept {
    switch (layout) {
        case Layout::DiamondPearl: return {0xC100, 0x14, 0x98};
        case Layout::Platinum: return {0xCF2C, 0x14, 0xA0};
        case Layout::HeartGoldSoulSilver: return {0xF628, 0x10, 0x98};
    }
    return {};
}

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
