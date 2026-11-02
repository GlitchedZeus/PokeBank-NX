#include "Integration/Gen8/Gen8TeamIndexes.h"
#include "Integration/Gen9/Gen9ZaSession.h"

#include "Enums/Ball.h"
#include "Names/ItemPresence.h"

#ifdef __SWITCH__
#include "Utils/PokeBankPaths.h"
#endif

#include <algorithm>

namespace PokeVault::Integration::Gen9Za {
namespace {

std::string slotLabel(SlotRef s) {
    if (s.region == Region::Party) return "Party slot " + std::to_string(s.slot + 1);
    return "Box " + std::to_string(s.box + 1) + " slot " + std::to_string(s.slot + 1);
}
const TeamSlot* teamAt(const Gen9ZaSave& save, size_t box, size_t slot) noexcept {
    const auto& teams = save.teams();
    if (!teams) return nullptr;
    const TeamSlot* found = nullptr;
    for (const auto& t : *teams) {
        if (t.box != box || t.slot != slot) continue;
        if (t.locked) return &t;
        found = &t;
    }
    return found;
}

} // namespace

const Gen8Edits::Rules& rules() {
    static const Gen8Edits::Rules r{
        "Legends: Z-A",
        [](uint32_t id) { return id <= Names::ITEM_PRESENCE_MAX_ID && Names::isHeldItemPresent(static_cast<uint16_t>(id), Enums::GameVersion::ZA); },
        Enums::getBallList(Enums::GameVersion::ZA)};
    return r;
}

std::optional<Gen9ZaSave> Gen9ZaSave::parse(std::span<const uint8_t> bytes, std::string_view assigned, std::string* error) {
    const auto fail = [&](const std::string& why) -> std::optional<Gen9ZaSave> {
        if (error) *error = why;
        return std::nullopt;
    };
    if (!Layout::acceptedSize(bytes.size())) return fail("not a Legends: Z-A main size (SaveUtil.IsSizeGen9ZA)");
    Swish::ParseStatus status{};
    auto blocks = Swish::decrypt(bytes, &status);
    if (!blocks) return fail(std::string("Legends: Z-A ") + Swish::parseStatusText(status));
    Gen9ZaSave out;
    out.bytes_.assign(bytes.begin(), bytes.end());
    out.blocks_ = std::move(*blocks);
    const auto find = [&](uint32_t key, size_t minimum, size_t& idx, const char* name) -> std::string {
        for (size_t i = 0; i < out.blocks_.size(); ++i)
            if (out.blocks_[i].key == key) {
                if (!out.blocks_[i].bytePayload()) return std::string("Legends: Z-A ") + name + " block is not a byte payload";
                if (out.blocks_[i].data.size() < minimum) return std::string("Legends: Z-A ") + name + " block is truncated";
                idx = i;
                return {};
            }
        return std::string("Legends: Z-A ") + name + " block is missing";
    };
    if (auto why = find(Layout::KeyMyStatus, Layout::MyName + Layout::MyNameBytes, out.myStatus_, "MyStatus"); !why.empty()) return fail(why);
    if (auto why = find(Layout::KeyParty, Layout::PartySlots * Layout::PartyStride, out.party_, "Party"); !why.empty()) return fail(why);
    if (auto why = find(Layout::KeyBox, Layout::BoxCount * Layout::BoxSlots * Layout::BoxStride, out.box_, "Box"); !why.empty()) return fail(why);
    for (const auto& b : out.blocks_)
        if (b.key == Layout::KeySaveRevision && b.type == Swish::TypeCode::UInt64 && b.data.size() == 8)
            for (size_t i = 0; i < 8; ++i) out.revision_ |= static_cast<uint64_t>(b.data[i]) << (8 * i);
    const auto gameId = Layout::gameIdFor(out.blocks_[out.myStatus_].data[Layout::MyGame]);
    if (gameId.empty()) return fail("Legends: Z-A MyStatus game is not Legends: Z-A (52)");
    if (!assigned.empty() && assigned != gameId) return fail("Legends: Z-A version does not match the assigned exact game");
    out.gameId_ = std::string(gameId);
    // Party9a has no count byte: count the leading occupied slots, and require the rest to be non-occupied.
    while (out.partyCount_ < Layout::PartySlots && out.at(SlotRef::party(out.partyCount_)).occupied()) ++out.partyCount_;
    if (out.partyCount_ == 0) return fail("Legends: Z-A party is empty");
    for (size_t i = out.partyCount_; i < Layout::PartySlots; ++i)
        if (out.at(SlotRef::party(i)).occupied()) return fail("Legends: Z-A party has a gap before an occupied slot");
    // TeamIndexes8 (6 teams x 6 int16, (box << 8) | slot, negative = none) + lock bits; unreadable -> nullopt.
    const Swish::Block* indexes = nullptr;
    const Swish::Block* locks = nullptr;
    for (const auto& b : out.blocks_) {
        if (b.key == Layout::KeyTeamIndexes) indexes = &b;
        if (b.key == Layout::KeyTeamLocks) locks = &b;
    }
    auto table = Gen8::readTeamIndexes(indexes, locks, Layout::BoxCount, Layout::BoxSlots);
    out.teams_ = std::move(table.teams);
    out.teamDiagnostic_ = std::move(table.diagnostic);
    if (error) error->clear();
    return out;
}

Gen8::SlotRecord Gen9ZaSave::at(SlotRef s) const {
    Gen8::SlotRecord r;
    const auto& data = blocks_[s.region == Region::Party ? party_ : box_].data;
    if (!s.canonical() || s.offset() + PK8::PartySize > data.size()) return r;
    r.pokemon = PK8::Pokemon8::decode(std::span<const uint8_t>(data).subspan(s.offset(), PK8::PartySize), &r.status);
    r.state = !r.pokemon ? Gen8::SlotState::Invalid : r.pokemon->empty() ? Gen8::SlotState::Empty : Gen8::SlotState::Occupied;
    return r;
}

std::string_view Gen9ZaStagedSession::editBlockedReason(SlotRef slot) const noexcept {
    if (slot.region == Region::Party) return {};
    if (!source_.teams()) return "Legends: Z-A battle-team blocks could not be read; box changes are refused";
    if (const auto* t = teamAt(source_, slot.box, slot.slot); t && t->locked)
        return "This Pokemon is in a locked Legends: Z-A battle team; unlock the team in game first";
    return {};
}

std::string_view Gen9ZaStagedSession::storageBlockedReason(SlotRef slot) const noexcept {
    if (slot.region == Region::Party)
        return "Party storage changes are unsupported for Legends: Z-A (party count/compaction not verified)";
    if (const auto r = editBlockedReason(slot); !r.empty()) return r;
    if (teamAt(source_, slot.box, slot.slot))
        return "This box slot is referenced by a Legends: Z-A battle team; remove it from the team in game first";
    return {};
}

std::optional<PK8::Pokemon8> Gen9ZaStagedSession::current(SlotRef slot) const {
    if (const auto it = entries_.find(slot); it != entries_.end()) return it->second.record;
    return source_.at(slot).pokemon;
}

void Gen9ZaStagedSession::commitEdit(SlotRef slot, PK8::Pokemon8 next, std::string note) {
    const auto base = source_.at(slot);
    auto it = entries_.find(slot);
    if (it == entries_.end()) it = entries_.emplace(slot, Entry{ChangeKind::Edit, std::move(next), std::nullopt, {}}).first;
    else it->second.record = std::move(next);
    it->second.notes.push_back(std::move(note));
    if (it->second.kind == ChangeKind::Edit && base.pokemon) {
        const auto a = base.pokemon->plain(), b = it->second.record.plain();
        if (std::equal(a.begin(), a.end(), b.begin(), b.end())) entries_.erase(it);
    }
}

bool Gen9ZaStagedSession::stageValue(SlotRef slot, Field field, uint32_t value, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (!slot.canonical()) return fail("Legends: Z-A edit target is not a canonical slot");
    if (const auto r = editBlockedReason(slot); !r.empty()) return fail(std::string(r));
    if (const auto it = entries_.find(slot); it != entries_.end() && it->second.kind == ChangeKind::Remove)
        return fail("Legends: Z-A cannot edit a slot staged for removal");
    auto rec = current(slot);
    if (!rec || rec->empty()) return fail("Legends: Z-A edit requires a valid occupied PA9");
    std::string note = slotLabel(slot) + ": ", why;
    if (!Gen8Edits::applyValue(*rec, field, value, rules(), note, why)) return fail(why);
    commitEdit(slot, std::move(*rec), std::move(note));
    if (error) error->clear();
    return true;
}

bool Gen9ZaStagedSession::stageText(SlotRef slot, Field field, std::string_view text, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (!slot.canonical()) return fail("Legends: Z-A edit target is not a canonical slot");
    if (const auto r = editBlockedReason(slot); !r.empty()) return fail(std::string(r));
    if (const auto it = entries_.find(slot); it != entries_.end() && it->second.kind == ChangeKind::Remove)
        return fail("Legends: Z-A cannot edit a slot staged for removal");
    auto rec = current(slot);
    if (!rec || rec->empty()) return fail("Legends: Z-A edit requires a valid occupied PA9");
    std::string note = slotLabel(slot) + ": ", why;
    if (!Gen8Edits::applyText(*rec, field, text, rules(), note, why)) return fail(why);
    commitEdit(slot, std::move(*rec), std::move(note));
    if (error) error->clear();
    return true;
}

bool Gen9ZaStagedSession::cloneToBox(SlotRef from, SlotRef to, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (!from.canonical() || !to.canonical() || from == to) return fail("Legends: Z-A clone requires distinct canonical slots");
    if (from.region != Region::Box || to.region != Region::Box)
        return fail("Legends: Z-A clone copies box records into box slots (party deposit rules are not verified)");
    if (entries_.count(from) || entries_.count(to)) return fail("Legends: Z-A clone requires an unmodified source and destination");
    if (const auto r = storageBlockedReason(to); !r.empty()) return fail(std::string(r));
    const auto src = source_.at(from), dst = source_.at(to);
    if (!src.occupied()) return fail("Legends: Z-A clone source is not a valid occupied PA9");
    if (!dst.blank()) return fail("Legends: Z-A clone destination must be a verified blank box slot");
    entries_.emplace(to, Entry{ChangeKind::Clone, *src.pokemon, from, {slotLabel(to) + ": clone of " + slotLabel(from)}});
    if (error) error->clear();
    return true;
}

bool Gen9ZaStagedSession::removeFromBox(SlotRef at, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (!at.canonical()) return fail("Legends: Z-A removal target is not canonical");
    if (const auto r = storageBlockedReason(at); !r.empty()) return fail(std::string(r));
    if (entries_.count(at)) return fail("Legends: Z-A removal requires an unmodified box slot");
    if (!source_.at(at).occupied()) return fail("Legends: Z-A removal requires one valid occupied box record");
    entries_.emplace(at, Entry{ChangeKind::Remove, PK8::Pokemon8::blank(PK8::PartySize), std::nullopt, {slotLabel(at) + ": removed"}});
    if (error) error->clear();
    return true;
}

Gen9ZaStagedSession::EntrySnapshot Gen9ZaStagedSession::snapshotEntry(SlotRef slot) const {
    EntrySnapshot out;
    if (const auto it = entries_.find(slot); it != entries_.end()) {
        out.present = true;
        out.kind = it->second.kind;
        out.record = it->second.record;
        out.cloneSource = it->second.cloneSource;
        out.notes = it->second.notes;
    }
    return out;
}

void Gen9ZaStagedSession::restoreEntry(SlotRef slot, const EntrySnapshot& s) {
    if (!s.present || !s.record) {
        entries_.erase(slot);
        return;
    }
    entries_.insert_or_assign(slot, Entry{s.kind, *s.record, s.cloneSource, s.notes});
}

bool Gen9ZaStagedSession::entryMatches(SlotRef slot, const EntrySnapshot& s) const {
    const auto it = entries_.find(slot);
    if (it == entries_.end()) return !s.present;
    if (!s.present || !s.record) return false;
    const auto a = it->second.record.plain(), b = s.record->plain();
    return it->second.kind == s.kind && std::equal(a.begin(), a.end(), b.begin(), b.end()) &&
           it->second.cloneSource == s.cloneSource && it->second.notes == s.notes;
}

std::vector<StagedChange> Gen9ZaStagedSession::changes() const {
    std::vector<StagedChange> out;
    for (const auto& [slot, e] : entries_) {
        StagedChange c;
        c.slot = slot;
        c.kind = e.kind;
        c.cloneSource = e.cloneSource;
        if (const auto base = source_.at(slot); base.pokemon) c.originalPlain.assign(base.pokemon->plain().begin(), base.pokemon->plain().end());
        c.stagedPlain.assign(e.record.plain().begin(), e.record.plain().end());
        PK8::write16(c.stagedPlain, 0x06, PK8::checksum(c.stagedPlain));
        c.notes = e.notes;
        out.push_back(std::move(c));
    }
    return out;
}

std::vector<std::string> Gen9ZaStagedSession::changeLines() const {
    std::vector<std::string> lines;
    for (const auto& [slot, e] : entries_) lines.insert(lines.end(), e.notes.begin(), e.notes.end());
    return lines;
}

std::optional<std::vector<uint8_t>> Gen9ZaStagedSession::buildWorkingCopy(std::string* error) const {
    const auto fail = [&](const std::string& why) -> std::optional<std::vector<uint8_t>> {
        if (error) *error = why;
        return std::nullopt;
    };
    if (!hasChanges()) return fail("Legends: Z-A working copy needs at least one staged change");
    auto blocks = source_.blocks();
    for (const auto& [slot, e] : entries_) {
        auto& data = blocks[slot.region == Region::Party ? source_.partyIndex() : source_.boxIndex()].data;
        const auto enc = e.record.serialize();
        std::copy(enc.begin(), enc.end(), data.begin() + static_cast<std::ptrdiff_t>(slot.offset()));
        // SAV9ZA.WriteSlotBox: on DLC saves a written box slot is marked present (even when empty).
        if (slot.region == Region::Box && source_.revision() != 0) data[slot.offset() + Layout::PresentFlag] = 1;
    }
    auto out = Swish::encrypt(blocks);
    std::string why;
    if (!verifyWorkingCopy(source_, out, changes(), &why)) return fail(why);
    if (error) error->clear();
    return out;
}

Common::VerifiedExportResult Gen9ZaStagedSession::exportWorkingCopy(const Gen9ZaExportRequest& request) const {
    Common::VerifiedExportResult result;
    std::string error;
    const auto edited = buildWorkingCopy(&error);
    if (!edited) {
        result.error = error;
        return result;
    }
    Common::VerifiedExportRequest r;
    r.rootDirectory = request.rootDirectory;
#ifdef __SWITCH__
    const std::string fixedRoot = PokeBank::Paths::exportsRoot() + "/za";
    if (r.rootDirectory.empty()) r.rootDirectory = fixedRoot;
    if (r.rootDirectory != fixedRoot) {
        result.error = "Legends: Z-A export cannot leave app-owned exports/za";
        return result;
    }
#endif
    if (r.rootDirectory.empty() || request.timestamp.empty()) {
        result.error = "Legends: Z-A export needs an app-owned root and a timestamp";
        return result;
    }
    const std::string gameId(source_.exactGameId());
    r.directoryStem = gameId + "_" + request.timestamp;
    r.generationLabel = "Generation IX";
    r.fileExtension = ".bin";
    r.sourcePath = request.sourcePath;
    r.gameId = gameId;
    r.saveFormat = "Legends: Z-A main (SAV9ZA, revision " + std::to_string(source_.revision()) + ", " + std::to_string(source_.bytes().size()) + " bytes)";
    r.timestamp = request.timestamp;
    r.applicationSha = request.applicationSha;
    r.originalBytes = source_.bytes();
    r.editedBytes = *edited;
    r.changeLines = changeLines();
    const auto expected = changes();
    const Gen9ZaSave& baseline = source_;
    r.validator = [&baseline, expected](std::span<const uint8_t> bytes, std::string& why) {
        return verifyWorkingCopy(baseline, bytes, expected, &why);
    };
    return Common::publishVerifiedStagedExport(r);
}

namespace {
uint8_t after_box_flag(const Gen9ZaSave& save, size_t at) { return save.blocks()[save.boxIndex()].data[at]; }
} // namespace

bool verifyWorkingCopy(const Gen9ZaSave& source, std::span<const uint8_t> edited, const std::vector<StagedChange>& changes,
                       std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (edited.size() != source.bytes().size()) return fail("Legends: Z-A working copy size changed");
    std::string why;
    const auto reparsed = Gen9ZaSave::parse(edited, source.exactGameId(), &why);
    if (!reparsed) return fail("Legends: Z-A working copy failed strict reparse: " + why);
    if (reparsed->partyCount() != source.partyCount()) return fail("Legends: Z-A working copy changed party count");
    const auto canonical = Swish::encrypt(reparsed->blocks());
    if (!std::equal(canonical.begin(), canonical.end(), edited.begin(), edited.end()))
        return fail("Legends: Z-A working copy is not the canonical encryption of its blocks");
    const auto& before = source.blocks();
    const auto& after = reparsed->blocks();
    if (before.size() != after.size()) return fail("Legends: Z-A working copy changed the block count");
    std::vector<uint8_t> partyAllowed(before[source.partyIndex()].data.size(), 0), boxAllowed(before[source.boxIndex()].data.size(), 0);
    for (const auto& c : changes) {
        if (!c.slot.canonical()) return fail("Legends: Z-A change has a noncanonical slot");
        auto& allowed = c.slot.region == Region::Party ? partyAllowed : boxAllowed;
        std::fill_n(allowed.begin() + static_cast<std::ptrdiff_t>(c.slot.offset()), PK8::PartySize, uint8_t{1});
        if (c.slot.region == Region::Box && source.revision() != 0) {
            const size_t flag = c.slot.offset() + Layout::PresentFlag;
            if (after_box_flag(*reparsed, flag) != 1) return fail("Legends: Z-A DLC box slot was not marked present");
            allowed[flag] = 1;
        }
        const auto rec = reparsed->at(c.slot);
        if (!rec.pokemon) return fail("Legends: Z-A staged record does not decode strictly");
        const auto plain = rec.pokemon->plain();
        if (!std::equal(plain.begin(), plain.end(), c.stagedPlain.begin(), c.stagedPlain.end()))
            return fail("Legends: Z-A staged record differs from the reviewed change");
        const auto src = source.at(c.slot);
        if (!src.pokemon || !std::equal(c.originalPlain.begin(), c.originalPlain.end(), src.pokemon->plain().begin(), src.pokemon->plain().end()))
            return fail("Legends: Z-A baseline record differs from the immutable source");
        const auto* team = c.slot.region == Region::Box ? teamAt(source, c.slot.box, c.slot.slot) : nullptr;
        if (c.slot.region == Region::Box && !source.teams()) return fail("Legends: Z-A box change without readable battle teams");
        size_t bad = 0;
        switch (c.kind) {
            case ChangeKind::Edit:
                if (!src.occupied() || rec.pokemon->empty()) return fail("Legends: Z-A edit must keep an occupied record");
                if (team && team->locked) return fail("Legends: Z-A edit touched a locked battle-team member");
                if (!Gen8Edits::withinMask(c.originalPlain, plain, c.slot.region, &bad))
                    return fail("Legends: Z-A edit touched a non-whitelisted PA9 byte at 0x" + std::to_string(bad));
                break;
            case ChangeKind::Clone: {
                if (c.slot.region != Region::Box || !c.cloneSource || c.cloneSource->region != Region::Box || !src.blank() || team)
                    return fail("Legends: Z-A clone must copy a box record into a blank, team-free box slot");
                const auto donor = source.at(*c.cloneSource);
                if (!donor.occupied() || !Gen8Edits::withinMask(donor.pokemon->plain(), plain, Region::Box, &bad))
                    return fail("Legends: Z-A clone differs from its immutable donor outside editable fields");
                break;
            }
            case ChangeKind::Remove:
                if (c.slot.region != Region::Box || !src.occupied() || !rec.blank() || team)
                    return fail("Legends: Z-A removal must blank an occupied, team-free box slot");
                break;
        }
    }
    for (size_t i = 0; i < before.size(); ++i) {
        const auto& a = before[i];
        const auto& b = after[i];
        if (a.key != b.key || a.type != b.type || a.subtype != b.subtype || a.data.size() != b.data.size())
            return fail("Legends: Z-A working copy changed a block's key, type or size");
        const std::vector<uint8_t>* allowed = i == source.partyIndex() ? &partyAllowed : i == source.boxIndex() ? &boxAllowed : nullptr;
        for (size_t j = 0; j < a.data.size(); ++j)
            if (a.data[j] != b.data[j] && !(allowed && (*allowed)[j]))
                return fail("Legends: Z-A working copy changed a non-whitelisted byte in block 0x" + std::to_string(a.key));
    }
    if (error) error->clear();
    return true;
}

} // namespace PokeVault::Integration::Gen9Za
