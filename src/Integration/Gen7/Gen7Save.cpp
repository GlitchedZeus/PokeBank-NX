#include "Integration/Gen7/Gen7Save.h"

#include "Utils/CRC16.h"
#include "Utils/SHA256.h"

extern "C" {
#include "memecrypto.h"
}

#include <algorithm>

namespace PokeVault::Integration::Gen7 {
namespace {

std::vector<uint8_t> withSignatureCleared(std::span<const uint8_t> save, Layout::Family f) {
    std::vector<uint8_t> copy(save.begin(), save.end());
    const size_t at = Layout::signatureOffset(f);
    std::fill_n(copy.begin() + static_cast<std::ptrdiff_t>(at), Layout::SignatureLength, uint8_t{0});
    return copy;
}

uint16_t crcOf(std::span<const uint8_t> save, Layout::Family f, size_t id) {
    const auto& b = Layout::blocks(f)[id];
    return Utils::crc16Invert(save.data() + b.offset, b.length);
}

} // namespace

uint16_t Gen7Save::computeBlockChecksum(std::span<const uint8_t> save, Layout::Family f, size_t blockId) {
    if (blockId != Layout::MemeCryptoBlock) return crcOf(save, f, blockId);
    const auto cleared = withSignatureCleared(save, f);
    return crcOf(cleared, f, blockId);
}

size_t Gen7Save::validBlockCount(std::span<const uint8_t> save, Layout::Family f) {
    if (save.size() != Layout::saveSize(f)) return 0;
    const auto cleared = withSignatureCleared(save, f);
    size_t valid = 0;
    for (size_t id = 0; id < Layout::blocks(f).size(); ++id)
        if (crcOf(cleared, f, id) == PK7::read16(save, Layout::checksumOffset(f, id))) ++valid;
    return valid;
}

std::array<uint8_t, 32> Gen7Save::checksumTableHash(std::span<const uint8_t> save, Layout::Family f) {
    std::array<uint8_t, 32> out{};
    Utils::SHA256 sha;
    sha.update(save.data() + Layout::metadataOffset(f), Layout::signedTableLength(f));
    sha.finalize(out.data());
    return out;
}

bool Gen7Save::verifySignature(std::span<const uint8_t> save, Layout::Family f,
                               std::array<uint8_t, Layout::SignatureLength>* payload) {
    if (save.size() != Layout::saveSize(f)) return false;
    std::array<uint8_t, Layout::SignatureLength> input{};
    std::array<uint8_t, Layout::SignatureLength> output{};
    std::copy_n(save.begin() + static_cast<std::ptrdiff_t>(Layout::signatureOffset(f)), input.size(), input.begin());
    if (!memecrypto_verify(input.data(), output.data(), static_cast<int>(input.size()))) return false;
    const auto hash = checksumTableHash(save, f);
    if (!std::equal(hash.begin(), hash.end(), output.begin())) return false;
    if (payload) *payload = output;
    return true;
}

std::optional<Gen7Save> Gen7Save::parse(std::span<const uint8_t> bytes, std::string_view assignedExactGame,
                                        std::string* error, SignaturePolicy policy) {
    const auto fail = [&](std::string message) -> std::optional<Gen7Save> {
        if (error) *error = std::move(message);
        return std::nullopt;
    };
    Layout::Family family;
    if (bytes.size() == Layout::SMSize) family = Layout::Family::SunMoon;
    else if (bytes.size() == Layout::USUMSize) family = Layout::Family::UltraSunUltraMoon;
    else return fail("Gen VII requires an exact 0x6BE00-byte SM or 0x6CC00-byte USUM save");

    const size_t meta = Layout::metadataOffset(family);
    if (PK7::read32(bytes, meta + Layout::MagicWithinMetadata) != Layout::BeefMagic)
        return fail("Gen VII save footer has no BEEF block-metadata magic");
    const auto table = Layout::blocks(family);
    const size_t valid = validBlockCount(bytes, family);
    if (valid != table.size())
        return fail("Gen VII save block checksum mismatch (" + std::to_string(valid) + "/" +
                    std::to_string(table.size()) + " blocks valid)");

    const size_t status = table[Layout::MyStatusBlock].offset;
    const uint8_t version = bytes[status + 4];
    if (!Layout::versionMatchesFamily(version, family))
        return fail("Gen VII MyStatus game version disagrees with the save size/block table");
    const std::string_view gameId = Layout::gameIdForVersion(version);
    if (!assignedExactGame.empty() && assignedExactGame != gameId)
        return fail("Gen VII exact game identity does not match the assigned source");

    std::array<uint8_t, Layout::SignatureLength> payload{};
    const bool signed_ = verifySignature(bytes, family, &payload);
    if (!signed_ && policy == SignaturePolicy::Require)
        return fail("Gen VII MemeCrypto signature does not verify against the checksum table");

    Gen7Save out;
    out.bytes_.assign(bytes.begin(), bytes.end());
    out.family_ = family;
    out.gameId_ = std::string(gameId);
    out.signatureVerified_ = signed_;
    out.diagnostics_.signatureTail =
        std::all_of(payload.begin() + 32, payload.end() - 8, [](uint8_t b) { return b == 0; })
            ? SignatureTail::Zero : SignatureTail::NonZero;

    const uint8_t count = bytes[partyCountOffset(family)];
    if (count > Layout::PartySlots) return fail("Gen VII party count exceeds six");
    out.partyCount_ = count;
    for (size_t i = 0; i < count; ++i) {
        const auto rec = out.party(i);
        if (!rec.occupied() || !rec.pokemon->partyForm())
            return fail("Gen VII declared party slot " + std::to_string(i + 1) + " is not a valid occupied PK7");
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
        if (PK7::read32(bytes, entry) != table[id].length || PK7::read16(bytes, entry + 4) != id)
            ++out.diagnostics_.metadataMismatches;
    }
    if (error) error->clear();
    return out;
}

SlotRecord Gen7Save::record(size_t offset, size_t size) const {
    SlotRecord out;
    out.pokemon = PK7::Pokemon6::decode(std::span<const uint8_t>(bytes_).subspan(offset, size), &out.status);
    if (!out.pokemon) out.state = SlotState::Invalid;
    else out.state = out.pokemon->empty() ? SlotState::Empty : SlotState::Occupied;
    return out;
}

SlotRecord Gen7Save::party(size_t slot) const {
    if (slot >= Layout::PartySlots) return {};
    return record(partyRecordOffset(family_, slot), Layout::PartyRecordSize);
}

SlotRecord Gen7Save::box(size_t box, size_t slot) const {
    if (box >= Layout::BoxCount || slot >= Layout::BoxSlots) return {};
    return record(boxRecordOffset(family_, box, slot), Layout::BoxRecordSize);
}

TrainerReadOnly Gen7Save::trainer() const {
    TrainerReadOnly t;
    const size_t base = Layout::blocks(family_)[Layout::MyStatusBlock].offset;
    t.tid = PK7::read16(bytes_, base + 0);
    t.sid = PK7::read16(bytes_, base + 2);
    t.version = bytes_[base + 4];
    t.gender = bytes_[base + 5];
    t.language = bytes_[base + 0x35];
    for (size_t i = 0; i < 13; ++i) {
        const uint16_t ch = PK7::read16(bytes_, base + 0x38 + 2 * i);
        if (ch == 0) break;
        t.rawName.push_back(static_cast<char16_t>(ch));
    }
    const size_t play = Layout::blocks(family_)[Layout::PlayTimeBlock].offset;
    t.playedHours = PK7::read16(bytes_, play);
    t.playedMinutes = bytes_[play + 2];
    t.playedSeconds = bytes_[play + 3];
    return t;
}

} // namespace PokeVault::Integration::Gen7
