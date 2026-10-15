#include "Integration/Gen8/Gen8Save.h"

namespace PokeVault::Integration::Gen8 {

std::optional<Gen8Save> Gen8Save::parse(std::span<const uint8_t> bytes, std::string_view assignedExactGame,
                                        std::string* error) {
    const auto fail = [&](const std::string& why) -> std::optional<Gen8Save> {
        if (error) *error = why;
        return std::nullopt;
    };
    if (!Layout::isSwordShieldSize(bytes.size()))
        return fail("not a Sword/Shield main size (SaveUtil.IsSizeGen8SWSH)");
    Swish::ParseStatus status{};
    auto blocks = Swish::decrypt(bytes, &status);
    if (!blocks) return fail(std::string("Sword/Shield ") + Swish::parseStatusText(status));

    Gen8Save out;
    out.bytes_.assign(bytes.begin(), bytes.end());
    out.blocks_ = std::move(*blocks);
    out.diagnostics_.blockCount = out.blocks_.size();

    const auto require = [&](uint32_t key, size_t minimum, size_t& index, const char* name) {
        const std::string label = std::string("Sword/Shield ") + name;
        const auto at = out.indexOf(key);
        std::string why;
        if (!at) why = label + " block is missing";
        else if (!out.blocks_[*at].bytePayload()) why = label + " block is not a byte payload";
        else if (out.blocks_[*at].data.size() < minimum) why = label + " block is truncated";
        if (!why.empty()) {
            if (error) *error = why;
            return false;
        }
        index = *at;
        return true;
    };
    if (!require(Layout::KeyMyStatus, Layout::MyStatusMinimum, out.myStatus_, "MyStatus")) return std::nullopt;
    if (!require(Layout::KeyParty, Layout::PartyBlockMinimum, out.party_, "Party")) return std::nullopt;
    if (!require(Layout::KeyBox, Layout::BoxBlockMinimum, out.box_, "Box")) return std::nullopt;

    const auto& status8 = out.blocks_[out.myStatus_].data;
    out.version_ = status8[Layout::MyStatusGame];
    const auto gameId = Layout::gameIdForVersion(out.version_);
    if (gameId.empty()) return fail("Sword/Shield MyStatus version is neither Sword (44) nor Shield (45)");
    if (!assignedExactGame.empty() && assignedExactGame != gameId)
        return fail("Sword/Shield version does not match the assigned exact game");
    out.gameId_ = std::string(gameId);

    out.partyCount_ = out.blocks_[out.party_].data[Layout::PartyCountOffset];
    if (out.partyCount_ > Layout::PartySlots) return fail("Sword/Shield party count exceeds 6");
    for (size_t i = 0; i < Layout::PartySlots; ++i) {
        const auto rec = out.party(i);
        if (i < out.partyCount_) {
            if (!rec.occupied()) return fail("Sword/Shield declared party slot is not a valid occupied PK8");
        } else if (rec.state == SlotState::Invalid) {
            ++out.diagnostics_.invalidUndeclaredPartyRecords;
        }
    }
    for (size_t b = 0; b < Layout::BoxCount; ++b)
        for (size_t s = 0; s < Layout::BoxSlots; ++s) {
            const auto rec = out.box(b, s);
            if (rec.occupied()) ++out.diagnostics_.occupiedBoxRecords;
            else if (rec.state == SlotState::Empty) ++out.diagnostics_.emptyBoxRecords;
            else ++out.diagnostics_.invalidBoxRecords;
        }

    // TeamIndexes8 + team lock flags; storage changes are refused without both.
    const auto* indexes = out.find(Layout::KeyTeamIndexes);
    const auto* locks = out.find(Layout::KeyTeamLocks);
    if (indexes && locks && indexes->bytePayload() && locks->bytePayload() &&
        indexes->data.size() >= Layout::TeamIndexesMinimum && !locks->data.empty()) {
        std::vector<TeamSlot> teams;
        bool valid = true;
        for (size_t i = 0; i < Layout::TeamCount * 6; ++i) {
            const auto raw = static_cast<int16_t>(PK8::read16(indexes->data, i * 2));
            if (raw < 0) continue;
            const size_t box = static_cast<size_t>(raw) >> 8;
            const size_t slot = static_cast<size_t>(raw) & 0xFF;
            if (box >= Layout::BoxCount || slot >= Layout::BoxSlots) { valid = false; break; }
            const size_t team = i / 6;
            teams.push_back({team, box, slot, ((locks->data[0] >> team) & 1) != 0});
        }
        if (valid) out.teams_ = std::move(teams);
    }
    if (error) error->clear();
    return out;
}

const Swish::Block* Gen8Save::find(uint32_t key) const noexcept {
    const auto at = indexOf(key);
    return at ? &blocks_[*at] : nullptr;
}

std::optional<size_t> Gen8Save::indexOf(uint32_t key) const noexcept {
    for (size_t i = 0; i < blocks_.size(); ++i) if (blocks_[i].key == key) return i;
    return std::nullopt;
}

SlotRecord Gen8Save::record(std::span<const uint8_t> encrypted) {
    SlotRecord r;
    r.pokemon = PK8::Pokemon8::decode(encrypted, &r.status);
    if (!r.pokemon) r.state = SlotState::Invalid;
    else r.state = r.pokemon->empty() ? SlotState::Empty : SlotState::Occupied;
    return r;
}

SlotRecord Gen8Save::party(size_t slot) const {
    if (slot >= Layout::PartySlots) return {};
    return record(std::span<const uint8_t>(blocks_[party_].data).subspan(partyRecordOffset(slot), Layout::RecordSize));
}

SlotRecord Gen8Save::box(size_t box, size_t slot) const {
    if (box >= Layout::BoxCount || slot >= Layout::BoxSlots) return {};
    return record(std::span<const uint8_t>(blocks_[box_].data).subspan(boxRecordOffset(box, slot), Layout::RecordSize));
}

TrainerReadOnly Gen8Save::trainer() const {
    const std::span<const uint8_t> d(blocks_[myStatus_].data);
    TrainerReadOnly t;
    t.tid = PK8::read16(d, Layout::MyStatusTid);
    t.sid = PK8::read16(d, Layout::MyStatusSid);
    t.version = d[Layout::MyStatusGame];
    t.gender = d[Layout::MyStatusGender];
    t.language = d[Layout::MyStatusLanguage];
    for (size_t i = 0; i + 1 < Layout::MyStatusNameBytes; i += 2) {
        const uint16_t ch = PK8::read16(d, Layout::MyStatusName + i);
        if (ch == 0) break;
        t.rawName.push_back(static_cast<char16_t>(ch));
    }
    return t;
}

} // namespace PokeVault::Integration::Gen8
