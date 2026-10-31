#include "Integration/Gen7b/Gen7bSave.h"

#include <algorithm>

namespace PokeVault::Integration::Gen7b {

PokeListHeader Gen7bSave::readHeader(std::span<const uint8_t> bytes) noexcept {
    PokeListHeader h;
    for (size_t i = 0; i < Layout::PartyPointers; ++i) h.party[i] = PK6::read16(bytes, Layout::Header + 2 * i);
    h.starter = PK6::read16(bytes, Layout::Header + Layout::StarterPointer);
    h.count = PK6::read16(bytes, Layout::Header + Layout::CountField);
    return h;
}

StorageModel Gen7bSave::readModel(std::span<const uint8_t> bytes) {
    StorageModel m;
    m.header = readHeader(bytes);
    m.records.resize(Layout::Slots);
    for (size_t i = 0; i < Layout::Slots; ++i)
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(Layout::recordOffset(i)), Layout::RecordSize,
                    m.records[i].begin());
    return m;
}

std::optional<PK6::Pokemon6> Gen7bSave::decode(const RawRecord& raw) {
    return PK6::Pokemon6::decode(std::span<const uint8_t>(raw.data(), raw.size()));
}

bool Gen7bSave::validateModel(const StorageModel& m, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (m.records.size() != Layout::Slots) return fail("Let's Go storage is not 1000 records");
    const auto& h = m.header;
    if (h.count == 0 || h.count > Layout::Slots) return fail("Let's Go stored Pokemon count is outside 1..1000");
    for (size_t i = 0; i < Layout::Slots; ++i) {
        const auto& raw = m.records[i];
        const uint32_t ec = PK6::read32(std::span<const uint8_t>(raw.data(), raw.size()), 0);
        if (i < h.count) {
            const auto mon = decode(raw);
            if (!mon || mon->empty())
                return fail("Let's Go storage record " + std::to_string(i + 1) + " inside the stored count is not a valid PB7");
        } else if (ec != 0) {
            return fail("Let's Go storage is not gapless (a record after the stored count is occupied)");
        }
    }
    // Party pointers: compact (members first), each inside the list, distinct, at least one.
    bool sawEmpty = false;
    size_t members = 0;
    for (size_t i = 0; i < Layout::PartyPointers; ++i) {
        const auto p = h.party[i];
        if (p == Layout::EmptyPointer) { sawEmpty = true; continue; }
        if (sawEmpty) return fail("Let's Go party pointers are not compact");
        if (p >= h.count) return fail("Let's Go party pointer is outside the stored list");
        for (size_t j = 0; j < i; ++j)
            if (h.party[j] == p) return fail("Let's Go party pointers repeat a storage record");
        ++members;
    }
    if (members == 0) return fail("Let's Go party is empty");
    if (h.starter != Layout::EmptyPointer && h.starter >= h.count)
        return fail("Let's Go partner pointer is outside the stored list");
    if (error) error->clear();
    return true;
}

std::optional<Gen7bSave> Gen7bSave::parse(std::span<const uint8_t> bytes, std::string_view assignedExactGame,
                                          std::string* error) {
    const auto fail = [&](const std::string& why) -> std::optional<Gen7bSave> {
        if (error) *error = why;
        return std::nullopt;
    };
    if (const auto why = Save::LGPEReadValidation::validate(bytes); !why.empty()) return fail(std::string(why));
    const uint8_t version = bytes[Layout::Version];
    const auto gameId = Layout::gameIdForVersion(version);
    if (gameId.empty()) return fail("Let's Go MyStatus version is neither Pikachu (42) nor Eevee (43)");
    if (!assignedExactGame.empty() && assignedExactGame != gameId)
        return fail("Let's Go version does not match the assigned exact game");
    Gen7bSave out;
    out.bytes_.assign(bytes.begin(), bytes.end());
    out.gameId_ = std::string(gameId);
    out.model_ = readModel(bytes);
    std::string why;
    if (!validateModel(out.model_, &why)) return fail(why);
    if (error) error->clear();
    return out;
}

TrainerFacts Gen7bSave::trainer() const {
    TrainerFacts t;
    const std::span<const uint8_t> d(bytes_);
    t.tid = PK6::read16(d, Layout::Tid);
    t.sid = PK6::read16(d, Layout::Sid);
    t.version = d[Layout::Version];
    t.gender = d[Layout::Gender];
    t.language = d[Layout::Language];
    for (size_t i = 0; i < Layout::OtNameUnits; ++i) {
        const uint16_t ch = PK6::read16(d, Layout::OtName + 2 * i);
        if (ch == 0) break;
        t.name.push_back(static_cast<char16_t>(ch));
    }
    return t;
}

} // namespace PokeVault::Integration::Gen7b
