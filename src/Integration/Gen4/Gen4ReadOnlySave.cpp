#include "Integration/Gen4/Gen4ReadOnlySave.h"

#include "Encryption/Encryption4.h"
#include "Utils/CRC16.h"
#include "Utils/Gen4TextCodec.h"

#include <array>

namespace PokeVault::Integration::Gen4 {
namespace {
    constexpr size_t SAVE_SIZE = 0x80000;
    constexpr size_t PARTITION_SIZE = 0x40000;
    constexpr uint32_t MAGIC_INTL = 0x20060623;
    constexpr uint32_t MAGIC_KOR = 0x20070903;
    constexpr size_t BOX_COUNT = 18;
    constexpr size_t BOX_SLOTS = 30;
    constexpr size_t SINNOH_BOX_STRIDE = BOX_SLOTS * Encryption::SIZE_STORED4; // 0xFF0

    struct Spec {
        size_t generalSize;
        size_t storageStart;
        size_t storageSize;
        size_t footerSize;
        size_t trainerOffset;
        size_t partyOffset;
        size_t pokedexOffset;
        size_t boxDataOffset;
        size_t boxStride;
        size_t currentBoxOffset;
        size_t boxNamesOffset;
        Enums::GameVersion group;
    };

    Spec specFor(Layout layout) {
        switch (layout) {
            case Layout::DiamondPearl:
                return {0xC100, 0xC100, 0x121E0, 0x14, 0x64, 0x98, 0x12DC,
                        0x4, SINNOH_BOX_STRIDE, 0, 4 + BOX_COUNT * SINNOH_BOX_STRIDE,
                        Enums::GameVersion::DP};
            case Layout::Platinum:
                return {0xCF2C, 0xCF2C, 0x121E4, 0x14, 0x68, 0xA0, 0x1328,
                        0x4, SINNOH_BOX_STRIDE, 0, 4 + BOX_COUNT * SINNOH_BOX_STRIDE,
                        Enums::GameVersion::PT};
            case Layout::HeartGoldSoulSilver:
                return {0xF628, 0xF700, 0x12310, 0x10, 0x64, 0x98, 0x12B8,
                        0, 0x1000, 0x12000, 0x12008,
                        Enums::GameVersion::HGSS};
        }
        return {};
    }

    uint16_t read16(std::span<const uint8_t> b, size_t o) noexcept {
        if (o + 1 >= b.size()) return 0;
        return static_cast<uint16_t>(b[o] | (static_cast<uint16_t>(b[o + 1]) << 8));
    }
    uint32_t read32(std::span<const uint8_t> b, size_t o) noexcept {
        if (o + 3 >= b.size()) return 0;
        return static_cast<uint32_t>(b[o]) |
               (static_cast<uint32_t>(b[o + 1]) << 8) |
               (static_cast<uint32_t>(b[o + 2]) << 16) |
               (static_cast<uint32_t>(b[o + 3]) << 24);
    }

    int compareCounter(uint32_t first, uint32_t second) noexcept {
        if (first == 0xFFFFFFFFu && second != 0xFFFFFFFEu) return 1;
        if (second == 0xFFFFFFFFu && first != 0xFFFFFFFEu) return 0;
        if (first > second) return 0;
        if (first < second) return 1;
        return 2;
    }

    struct Candidate {
        size_t offset = 0;
        size_t size = 0;
        bool valid = false;
        uint32_t major = 0;
        uint32_t minor = 0;
    };

    Candidate inspect(std::span<const uint8_t> source, size_t offset,
                      size_t size, size_t footerSize) noexcept {
        Candidate c{offset, size, false, 0, 0};
        if (offset + size > source.size() || size < 0x14 || footerSize > size) return c;
        const size_t end = offset + size;
        c.major = read32(source, end - 0x14);
        c.minor = read32(source, end - 0x10);
        if (read32(source, end - 0x0C) != size) return c;
        const uint32_t magic = read32(source, end - 0x08);
        if (magic != MAGIC_INTL && magic != MAGIC_KOR) return c;
        const uint16_t stored = read16(source, end - 0x02);
        const uint16_t actual = Utils::crc16ccitt(source.data() + offset, size - footerSize);
        c.valid = stored == actual;
        return c;
    }

    int preferred(const Candidate& a, const Candidate& b) noexcept {
        const int major = compareCounter(a.major, b.major);
        if (major != 2) return major;
        const int minor = compareCounter(a.minor, b.minor);
        return minor == 1 ? 1 : 0;
    }

    std::optional<BlockSelection> selectPair(const Candidate& a, const Candidate& b) {
        const int nominal = preferred(a, b);
        if (!a.valid && !b.valid) return std::nullopt;
        int selected;
        if (a.valid && b.valid) selected = preferred(a, b);
        else selected = a.valid ? 0 : 1;
        const Candidate& c = selected == 0 ? a : b;
        return BlockSelection{c.offset, static_cast<uint8_t>(selected),
                              selected != nominal, c.major, c.minor};
    }

    Enums::GameVersion assignedVersion(std::string_view id) noexcept {
        if (id == "diamond_nds") return Enums::GameVersion::D;
        if (id == "pearl_nds") return Enums::GameVersion::P;
        if (id == "platinum_nds") return Enums::GameVersion::Pt;
        if (id == "heartgold_nds") return Enums::GameVersion::HG;
        if (id == "soulsilver_nds") return Enums::GameVersion::SS;
        return Enums::GameVersion::Invalid;
    }

    std::span<const std::byte> byteSpan(const std::vector<uint8_t>& source, size_t offset, size_t length) {
        return {reinterpret_cast<const std::byte*>(source.data() + offset), length};
    }
}

AssignmentStatus Gen4ReadOnlySave::validateAssignment(Layout layout,
                                                      Enums::GameVersion exactGameFromSave,
                                                      std::string_view assignedGameId,
                                                      Enums::GameVersion* assignedExact) noexcept {
    const Enums::GameVersion assigned = assignedVersion(assignedGameId);
    if (assignedExact) *assignedExact = assigned;
    if (assignedGameId.empty()) return AssignmentStatus::Unspecified;
    if (assigned == Enums::GameVersion::Invalid) return AssignmentStatus::Mismatch;

    switch (layout) {
        case Layout::DiamondPearl:
            return assigned == Enums::GameVersion::D || assigned == Enums::GameVersion::P
                ? AssignmentStatus::Match : AssignmentStatus::Mismatch;
        case Layout::Platinum:
            return assigned == Enums::GameVersion::Pt
                ? AssignmentStatus::Match : AssignmentStatus::Mismatch;
        case Layout::HeartGoldSoulSilver:
            return assigned == exactGameFromSave
                ? AssignmentStatus::Match : AssignmentStatus::Mismatch;
    }
    return AssignmentStatus::Mismatch;
}

std::optional<Gen4ReadOnlySave> Gen4ReadOnlySave::parse(std::span<const uint8_t> source,
                                                        Layout layout,
                                                        std::string_view assignedGameId,
                                                        std::string* error) {
    const auto setError = [&](std::string value) {
        if (error) *error = std::move(value);
    };
    if (source.size() != SAVE_SIZE) {
        setError("Gen IV save must be exactly 0x80000 bytes");
        return std::nullopt;
    }

    const Spec spec = specFor(layout);
    const Candidate general0 = inspect(source, 0, spec.generalSize, spec.footerSize);
    const Candidate general1 = inspect(source, PARTITION_SIZE, spec.generalSize, spec.footerSize);
    const auto general = selectPair(general0, general1);
    if (!general) {
        setError("no CRC-valid Gen IV General block candidate");
        return std::nullopt;
    }

    const Candidate storage0 = inspect(source, spec.storageStart, spec.storageSize, spec.footerSize);
    const Candidate storage1 = inspect(source, PARTITION_SIZE + spec.storageStart,
                                       spec.storageSize, spec.footerSize);
    const auto storage = selectPair(storage0, storage1);
    if (!storage) {
        setError("no CRC-valid Gen IV Storage block candidate");
        return std::nullopt;
    }

    Gen4ReadOnlySave out;
    out.layout_ = layout;
    out.rawFamily_ = spec.group;
    out.general_ = *general;
    out.storage_ = *storage;
    out.storageOffset_ = storage->offset;
    out.source_.assign(source.begin(), source.end());

    const size_t trainerBase = general->offset + spec.trainerOffset;
    if (trainerBase + 0x26 > general->offset + spec.generalSize) {
        setError("trainer structure outside selected General block");
        return std::nullopt;
    }
    out.trainer_.name = Utils::decodeGen4Field(byteSpan(out.source_, trainerBase, 16));
    out.trainer_.tid = read16(out.source_, trainerBase + 0x10);
    out.trainer_.sid = read16(out.source_, trainerBase + 0x12);
    out.trainer_.money = read32(out.source_, trainerBase + 0x14);
    out.trainer_.gender = out.source_[trainerBase + 0x18] & 1u;
    out.trainer_.language = out.source_[trainerBase + 0x19];
    out.trainer_.badges = out.source_[trainerBase + 0x1A];
    out.trainer_.romCode = out.source_[trainerBase + 0x1C];
    out.trainer_.playedHours = read16(out.source_, trainerBase + 0x22);
    out.trainer_.playedMinutes = out.source_[trainerBase + 0x24];
    out.trainer_.playedSeconds = out.source_[trainerBase + 0x25];

    if (layout == Layout::Platinum) {
        out.exactGameFromSave_ = Enums::GameVersion::Pt;
    } else if (layout == Layout::HeartGoldSoulSilver) {
        if (out.trainer_.romCode == static_cast<uint8_t>(Enums::GameVersion::HG))
            out.exactGameFromSave_ = Enums::GameVersion::HG;
        else if (out.trainer_.romCode == static_cast<uint8_t>(Enums::GameVersion::SS))
            out.exactGameFromSave_ = Enums::GameVersion::SS;
        else {
            setError("HGSS layout has invalid Trainer ROMCode");
            return std::nullopt;
        }
    } else {
        out.exactGameFromSave_ = Enums::GameVersion::Invalid; // raw DP cannot prove D vs P.
    }

    out.assignmentStatus_ = validateAssignment(layout, out.exactGameFromSave_,
                                                assignedGameId, &out.assignedExactGame_);

    const size_t partyCountOffset = general->offset + spec.partyOffset - 4;
    const uint8_t partyCount = out.source_[partyCountOffset];
    if (partyCount > 6) {
        setError("Gen IV party count exceeds six");
        return std::nullopt;
    }
    out.partyCount_ = partyCount;
    out.diagnostics_.declaredPartyCount = partyCount;
    out.party_.reserve(6);
    for (uint8_t slot = 0; slot < 6; ++slot) {
        const size_t offset = general->offset + spec.partyOffset +
                              static_cast<size_t>(slot) * Encryption::SIZE_PARTY4;
        if (offset + Encryption::SIZE_PARTY4 > general->offset + spec.generalSize) {
            setError("party record outside selected General block");
            return std::nullopt;
        }
        out.party_.emplace_back(byteSpan(out.source_, offset, Encryption::SIZE_PARTY4), spec.group);
        if (slot < partyCount) {
            if (out.party_.back().valid()) ++out.diagnostics_.validPartyRecords;
            else ++out.diagnostics_.invalidPartyRecords;
        }
    }

    out.boxes_.reserve(BOX_COUNT * BOX_SLOTS);
    for (size_t box = 0; box < BOX_COUNT; ++box) {
        const size_t boxBase = storage->offset + spec.boxDataOffset + box * spec.boxStride;
        for (size_t slot = 0; slot < BOX_SLOTS; ++slot) {
            const size_t offset = boxBase + slot * Encryption::SIZE_STORED4;
            if (offset + Encryption::SIZE_STORED4 > storage->offset + spec.storageSize) {
                setError("box record outside selected Storage block");
                return std::nullopt;
            }
            out.boxes_.emplace_back(byteSpan(out.source_, offset, Encryption::SIZE_STORED4), spec.group);
            const auto& parsed = out.boxes_.back();
            if (!parsed.valid()) ++out.diagnostics_.invalidBoxRecords;
            else if (!parsed.empty()) ++out.diagnostics_.occupiedBoxRecords;
        }
    }

    const size_t current = storage->offset + spec.currentBoxOffset;
    if (current >= out.source_.size()) {
        setError("current-box field outside selected Storage block");
        return std::nullopt;
    }
    // PKHeX SAV4Sinnoh and PKSM-Core both read only Storage[0] for D/P/Pt CurrentBox;
    // the field is 32-bit aligned, but the semantic value is one byte.
    const uint32_t currentBox = out.source_[current];
    if (currentBox >= BOX_COUNT) {
        setError("Gen IV current-box index exceeds seventeen");
        return std::nullopt;
    }
    out.currentBox_ = static_cast<uint8_t>(currentBox);

    out.boxNames_.reserve(BOX_COUNT);
    for (size_t box = 0; box < BOX_COUNT; ++box) {
        const size_t offset = storage->offset + spec.boxNamesOffset + box * 40;
        if (offset + 40 > storage->offset + spec.storageSize) {
            setError("box-name field outside selected Storage block");
            return std::nullopt;
        }
        out.boxNames_.push_back(Utils::decodeGen4Field(byteSpan(out.source_, offset, 40)));
    }

    if (error) error->clear();
    return out;
}


DexProgress Gen4ReadOnlySave::dexProgress() const noexcept {
    DexProgress out{};
    const Spec spec = specFor(layout_);
    const size_t base = general_.offset + spec.pokedexOffset;
    // Zukan4: u32 magic, then 0x40 bytes Caught followed by 0x40 bytes Seen.
    const size_t caught = base + 4;
    const size_t seen = caught + 0x40;
    if (seen + 0x40 > source_.size()) return out;

    for (uint16_t species = 1; species <= out.total; ++species) {
        const uint16_t index = static_cast<uint16_t>(species - 1);
        const uint8_t mask = static_cast<uint8_t>(1u << (index & 7));
        if (source_[seen + (index >> 3)] & mask) ++out.seen;
        if (source_[caught + (index >> 3)] & mask) ++out.caught;
    }
    return out;
}

const Pokemon::Pokemon4ReadOnly& Gen4ReadOnlySave::box(size_t boxIndex, size_t slotIndex) const {
    // Native builds disable exceptions. Return an invalid, immutable record for bad indices.
    static const Pokemon::Pokemon4ReadOnly invalid;
    if (boxIndex >= BOX_COUNT || slotIndex >= BOX_SLOTS) return invalid;
    return boxes_[boxIndex * BOX_SLOTS + slotIndex];
}

std::span<const uint8_t> Gen4ReadOnlySave::hgssBoxPadding(size_t boxIndex) const noexcept {
    if (layout_ != Layout::HeartGoldSoulSilver || boxIndex >= BOX_COUNT) return {};
    const size_t offset = storageOffset_ + boxIndex * 0x1000 + 0xFF0;
    if (offset + 0x10 > source_.size()) return {};
    return {source_.data() + offset, 0x10};
}

}
