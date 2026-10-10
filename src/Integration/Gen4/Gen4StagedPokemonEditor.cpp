#include "Integration/Gen4/Gen4StagedPokemonEditor.h"
#include "Integration/Gen4/Gen4ReadOnlyInventory.h"
#include "Integration/Gen4/Gen4BagCatalog.h"

#include "Encryption/Encryption4.h"
#include "Utils/CRC16.h"
#include "Enums/LanguageID.h"

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
    const auto expectedMoved = source.originalEncryptedBytes();
    const auto expectedReplaced = destination.originalEncryptedBytes();
    if (!moved.valid() || moved.empty() || !replaced.valid() ||
        replaced.empty() != destination.empty() ||
        expectedMoved.size() != recordSize || expectedReplaced.size() != recordSize ||
        !std::equal(moved.originalEncryptedBytes().begin(),
                    moved.originalEncryptedBytes().end(), expectedMoved.begin()) ||
        !std::equal(replaced.originalEncryptedBytes().begin(),
                    replaced.originalEncryptedBytes().end(), expectedReplaced.begin())) {
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

    const auto backup = staged_;
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
    size_t foundAt=0;
    size_t populated=0;
    bool found=false;
    for(size_t slot=0;slot<spec.slots;++slot) {
        const size_t at=base+spec.offset+slot*4;
        const uint16_t id=uint16_t(staged_[at]) | (uint16_t(staged_[at+1])<<8);
        const uint16_t quantity=uint16_t(staged_[at+2]) | (uint16_t(staged_[at+3])<<8);
        if(id==0 || id==0xFFFF || quantity==0)continue;
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
    write16(staged_,foundAt,0);
    write16(staged_,foundAt+2,0);
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
    // Audit every byte, not merely the displayed pouch. Only the native
    // four-byte item slot and the verified General CRC may change.
    const size_t crcAt=base+blockSize-2;
    for(size_t i=0;i<staged_.size();++i) {
        if(i>=foundAt && i<foundAt+4)continue;
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
