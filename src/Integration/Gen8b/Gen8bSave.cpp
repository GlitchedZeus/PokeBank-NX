#include "Integration/Gen8b/Gen8bSave.h"

#include "Utils/MD5.h"

#include <algorithm>

namespace PokeVault::Integration::Gen8b {

bool Gen8bSave::hashValid(std::span<const uint8_t> bytes) {
    if (bytes.size() < Layout::HashOffset + Layout::HashBytes) return false;
    std::vector<uint8_t> probe(bytes.begin(), bytes.end());
    std::fill_n(probe.begin() + static_cast<std::ptrdiff_t>(Layout::HashOffset), Layout::HashBytes, uint8_t{0});
    std::array<uint8_t, Layout::HashBytes> digest{};
    Utils::md5(probe.data(), probe.size(), digest.data());
    return std::equal(digest.begin(), digest.end(), bytes.begin() + static_cast<std::ptrdiff_t>(Layout::HashOffset));
}

void Gen8bSave::refreshHash(std::vector<uint8_t>& bytes) {
    if (bytes.size() < Layout::HashOffset + Layout::HashBytes) return;
    std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(Layout::HashOffset), Layout::HashBytes, uint8_t{0});
    std::array<uint8_t, Layout::HashBytes> digest{};
    Utils::md5(bytes.data(), bytes.size(), digest.data());
    std::copy(digest.begin(), digest.end(), bytes.begin() + static_cast<std::ptrdiff_t>(Layout::HashOffset));
}

std::optional<Gen8bSave> Gen8bSave::parse(std::span<const uint8_t> bytes, std::string_view assignedExactGame,
                                          std::string* error) {
    const auto fail = [&](const std::string& why) -> std::optional<Gen8bSave> {
        if (error) *error = why;
        return std::nullopt;
    };
    const Layout::Revision* revision = nullptr;
    for (const auto& r : Layout::Revisions)
        if (r.size == bytes.size()) revision = &r;
    if (!revision) return fail("not a BD/SP save size (SaveUtil.SIZE_G8BDSP_0..3)");
    if (PK8::read32(bytes, 0) != revision->version)
        return fail("BD/SP revision word does not match the file size (Gem8Version)");
    if (!hashValid(bytes)) return fail("BD/SP whole-file MD5 does not match");
    const auto gameId = Layout::gameIdForRom(bytes[Layout::RomCode]);
    if (gameId.empty()) return fail("BD/SP MyStatus ROM code is neither Brilliant Diamond (0) nor Shining Pearl (1)");
    if (!assignedExactGame.empty() && assignedExactGame != gameId)
        return fail("BD/SP version does not match the assigned exact game");

    Gen8bSave out;
    out.bytes_.assign(bytes.begin(), bytes.end());
    out.gameId_ = std::string(gameId);
    out.revision_ = revision->label;
    out.partyCount_ = bytes[Layout::PartyCount];
    if (out.partyCount_ == 0 || out.partyCount_ > Layout::PartySlots) return fail("BD/SP party count is outside 1..6");
    for (size_t i = 0; i < out.partyCount_; ++i)
        if (!out.party(i).occupied()) return fail("BD/SP declared party slot is not a valid occupied PB8");

    std::vector<TeamSlot> teams;
    bool valid = true;
    const uint8_t locks = bytes[Layout::LockedTeam];
    for (size_t i = 0; i < Layout::TeamCount * Layout::TeamSlotCount; ++i) {
        const auto raw = static_cast<int16_t>(PK8::read16(bytes, Layout::TeamPositions + 2 * i));
        if (raw < 0) continue;
        const size_t box = static_cast<size_t>(raw) >> 8, slot = static_cast<size_t>(raw) & 0xFF;
        if (box >= Layout::BoxCount || slot >= Layout::BoxSlots) { valid = false; break; }
        const size_t team = i / Layout::TeamSlotCount;
        teams.push_back({team, box, slot, ((locks >> team) & 1) != 0});
    }
    if (valid) out.teams_ = std::move(teams);
    if (error) error->clear();
    return out;
}

SlotRecord Gen8bSave::record(std::span<const uint8_t> encrypted) {
    SlotRecord r;
    r.pokemon = PK8::Pokemon8::decode(encrypted, &r.status);
    if (!r.pokemon) r.state = SlotState::Invalid;
    else r.state = r.pokemon->empty() ? SlotState::Empty : SlotState::Occupied;
    return r;
}

SlotRecord Gen8bSave::party(size_t slot) const {
    if (slot >= Layout::PartySlots) return {};
    return record(std::span<const uint8_t>(bytes_).subspan(Layout::partyOffset(slot), Layout::RecordSize));
}

SlotRecord Gen8bSave::box(size_t box, size_t slot) const {
    if (box >= Layout::BoxCount || slot >= Layout::BoxSlots) return {};
    return record(std::span<const uint8_t>(bytes_).subspan(Layout::boxOffset(box, slot), Layout::RecordSize));
}

TrainerFacts Gen8bSave::trainer() const {
    TrainerFacts t;
    const std::span<const uint8_t> d(bytes_);
    for (size_t i = 0; i + 1 < Layout::OtBytes; i += 2) {
        const uint16_t ch = PK8::read16(d, Layout::MyStatus + i);
        if (ch == 0) break;
        t.name.push_back(static_cast<char16_t>(ch));
    }
    t.tid = PK8::read16(d, Layout::Tid);
    t.sid = PK8::read16(d, Layout::Sid);
    t.gender = d[Layout::Male] == 1 ? 0 : 1;
    return t;
}

std::u16string Gen8bSave::boxName(size_t box) const {
    std::u16string out;
    if (box >= Layout::BoxCount) return out;
    const std::span<const uint8_t> d(bytes_);
    for (size_t i = 0; i + 1 < Layout::BoxNameBytes; i += 2) {
        const uint16_t ch = PK8::read16(d, Layout::BoxLayout + box * Layout::BoxNameBytes + i);
        if (ch == 0) break;
        out.push_back(static_cast<char16_t>(ch));
    }
    return out;
}

} // namespace PokeVault::Integration::Gen8b
