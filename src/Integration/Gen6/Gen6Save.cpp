#include "Integration/Gen6/Gen6Save.h"

#include "Utils/CRC16.h"

namespace PokeVault::Integration::Gen6 {

std::optional<size_t> Gen6Save::owningBlock(Layout::Family f, size_t offset, size_t length) noexcept {
    const auto table = Layout::blocks(f);
    for (size_t id = 0; id < table.size(); ++id) {
        const auto& block = table[id];
        if (offset >= block.offset && offset + length <= size_t{block.offset} + block.length)
            return id;
    }
    return std::nullopt;
}

uint16_t Gen6Save::computeBlockChecksum(std::span<const uint8_t> save, Layout::Family f,
                                        size_t blockId) noexcept {
    const auto& block = Layout::blocks(f)[blockId];
    return Utils::crc16ccitt(save.data() + block.offset, block.length);
}

uint16_t Gen6Save::storedBlockChecksum(std::span<const uint8_t> save, Layout::Family f,
                                       size_t blockId) noexcept {
    return PK6::read16(save, Layout::checksumOffset(f, blockId));
}

void Gen6Save::refreshBlockChecksum(std::span<uint8_t> save, Layout::Family f,
                                    size_t blockId) noexcept {
    PK6::write16(save, Layout::checksumOffset(f, blockId), computeBlockChecksum(save, f, blockId));
}

size_t Gen6Save::validBlockCount(std::span<const uint8_t> save, Layout::Family f) noexcept {
    if (save.size() != Layout::saveSize(f)) return 0;
    size_t valid = 0;
    for (size_t id = 0; id < Layout::blocks(f).size(); ++id)
        if (computeBlockChecksum(save, f, id) == storedBlockChecksum(save, f, id)) ++valid;
    return valid;
}

std::optional<Gen6Save> Gen6Save::parse(std::span<const uint8_t> bytes,
                                        std::string_view assignedExactGame, std::string* error) {
    const auto fail = [&](std::string message) -> std::optional<Gen6Save> {
        if (error) *error = std::move(message);
        return std::nullopt;
    };

    Layout::Family family;
    if (bytes.size() == Layout::XYSize) family = Layout::Family::XY;
    else if (bytes.size() == Layout::ORASSize) family = Layout::Family::ORAS;
    else if (bytes.size() == Layout::ORASDemoSize)
        return fail("Gen VI ORAS Special Demo saves are unsupported");
    else
        return fail("Gen VI requires an exact 0x65600-byte XY or 0x76000-byte ORAS save");

    const size_t meta = Layout::metadataOffset(family);
    if (PK6::read32(bytes, meta + Layout::MagicWithinMetadata) != Layout::BeefMagic)
        return fail("Gen VI save footer has no BEEF block-metadata magic");

    const auto table = Layout::blocks(family);
    const size_t valid = validBlockCount(bytes, family);
    if (valid != table.size())
        return fail("Gen VI save block checksum mismatch (" + std::to_string(valid) + "/" +
                    std::to_string(table.size()) + " blocks valid)");

    const uint8_t version = bytes[Layout::MyStatusOffset + 4];
    if (!Layout::versionMatchesFamily(version, family))
        return fail("Gen VI MyStatus game version disagrees with the save size/block table");
    const std::string_view gameId = Layout::gameIdForVersion(version);
    if (!assignedExactGame.empty() && assignedExactGame != gameId)
        return fail("Gen VI exact game identity does not match the assigned source");

    Gen6Save out;
    out.bytes_.assign(bytes.begin(), bytes.end());
    out.family_ = family;
    out.version_ = version;
    out.gameId_ = std::string(gameId);

    const uint8_t count = bytes[partyCountOffset()];
    if (count > Layout::PartySlots) return fail("Gen VI party count exceeds six");
    out.partyCount_ = count;
    for (size_t i = 0; i < Layout::PartySlots; ++i) {
        const auto rec = out.party(i);
        if (i < count) {
            if (!rec.occupied() || !rec.pokemon || !rec.pokemon->partyForm())
                return fail("Gen VI declared party slot " + std::to_string(i + 1) +
                            " is not a valid occupied PK6");
        } else if (rec.state == SlotState::Invalid) {
            ++out.diagnostics_.invalidUndeclaredPartyRecords;
        }
    }

    for (size_t b = 0; b < Layout::BoxCount; ++b)
        for (size_t s = 0; s < Layout::BoxSlots; ++s) {
            switch (out.box(b, s).state) {
                case SlotState::Occupied: ++out.diagnostics_.occupiedBoxRecords; break;
                case SlotState::Empty: ++out.diagnostics_.emptyBoxRecords; break;
                case SlotState::Invalid: ++out.diagnostics_.invalidBoxRecords; break;
            }
        }

    for (size_t id = 0; id < table.size(); ++id) {
        const size_t entry = meta + Layout::EntriesWithinMetadata + id * Layout::EntrySize;
        if (PK6::read32(bytes, entry) != table[id].length || PK6::read16(bytes, entry + 4) != id)
            ++out.diagnostics_.metadataMismatches;
    }

    if (error) error->clear();
    return out;
}

SlotRecord Gen6Save::record(size_t offset, size_t size) const {
    SlotRecord out;
    out.encrypted = std::span<const uint8_t>(bytes_).subspan(offset, size);
    out.pokemon = PK6::Pokemon6::decode(out.encrypted, &out.status);
    if (!out.pokemon) out.state = SlotState::Invalid;
    else out.state = out.pokemon->empty() ? SlotState::Empty : SlotState::Occupied;
    return out;
}

SlotRecord Gen6Save::party(size_t slot) const {
    if (slot >= Layout::PartySlots) return {};
    return record(partyRecordOffset(slot), PK6::PartySize);
}

SlotRecord Gen6Save::box(size_t box, size_t slot) const {
    if (box >= Layout::BoxCount || slot >= Layout::BoxSlots) return {};
    return record(boxRecordOffset(family_, box, slot), PK6::StoredSize);
}

TrainerReadOnly Gen6Save::trainer() const {
    TrainerReadOnly t;
    const size_t base = Layout::MyStatusOffset;
    t.tid = PK6::read16(bytes_, base + 0);
    t.sid = PK6::read16(bytes_, base + 2);
    t.version = bytes_[base + 4];
    t.gender = bytes_[base + 5];
    t.language = bytes_[base + 0x2D];
    for (size_t i = 0; i < 13; ++i) {
        const uint16_t ch = PK6::read16(bytes_, base + 0x48 + 2 * i);
        if (ch == 0) break;
        t.rawName.push_back(static_cast<char16_t>(ch));
    }
    // PlayTime block (6): u16 hours, u8 minutes, u8 seconds (PKSM-Core Sav6.cpp).
    t.playedHours = PK6::read16(bytes_, Layout::PlayTimeOffset);
    t.playedMinutes = bytes_[Layout::PlayTimeOffset + 2];
    t.playedSeconds = bytes_[Layout::PlayTimeOffset + 3];
    return t;
}

} // namespace PokeVault::Integration::Gen6
