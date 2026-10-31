#include "Integration/Gen8b/Gen8bStagedSession.h"

#include "Enums/Ball.h"
#include "Names/ItemPresence.h"

#ifdef __SWITCH__
#include "Utils/PokeBankPaths.h"
#endif

#include <algorithm>

namespace PokeVault::Integration::Gen8b {
namespace {

std::string slotLabel(SlotRef s) {
    if (s.region == RecordRegion::Party) return "Party slot " + std::to_string(s.slot + 1);
    return "Box " + std::to_string(s.box + 1) + " slot " + std::to_string(s.slot + 1);
}
size_t offsetOf(SlotRef s) {
    return s.region == RecordRegion::Party ? Layout::partyOffset(s.slot) : Layout::boxOffset(s.box, s.slot);
}
const TeamSlot* teamAt(const Gen8bSave& save, size_t box, size_t slot) noexcept {
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
        "BD/SP",
        [](uint32_t id) { return id <= Names::ITEM_PRESENCE_MAX_ID && Names::isHeldItemPresent(static_cast<uint16_t>(id), Enums::GameVersion::BDSP); },
        Enums::getBallList(Enums::GameVersion::BDSP)};
    return r;
}

std::string_view Gen8bStagedSession::editBlockedReason(SlotRef slot) const noexcept {
    if (slot.region == RecordRegion::Party) return {};
    if (!source_.teams()) return "BD/SP battle-team data could not be read; box changes are refused";
    if (const auto* t = teamAt(source_, slot.box, slot.slot); t && t->locked)
        return "This Pokemon is in a locked BD/SP battle team; unlock the team in game first";
    return {};
}

std::string_view Gen8bStagedSession::storageBlockedReason(SlotRef slot) const noexcept {
    if (slot.region == RecordRegion::Party)
        return "Party storage changes are unsupported for BD/SP (party count/compaction not verified)";
    if (const auto r = editBlockedReason(slot); !r.empty()) return r;
    if (teamAt(source_, slot.box, slot.slot))
        return "This box slot is referenced by a BD/SP battle team; remove it from the team in game first";
    return {};
}

SlotRecord Gen8bStagedSession::original(SlotRef slot) const {
    if (!slot.canonical()) return {};
    return slot.region == RecordRegion::Party ? source_.party(slot.slot) : source_.box(slot.box, slot.slot);
}

std::optional<PK8::Pokemon8> Gen8bStagedSession::current(SlotRef slot) const {
    if (const auto it = entries_.find(slot); it != entries_.end()) return it->second.record;
    return original(slot).pokemon;
}

void Gen8bStagedSession::commitEdit(SlotRef slot, PK8::Pokemon8 next, std::string note) {
    const auto base = original(slot);
    auto it = entries_.find(slot);
    if (it == entries_.end()) it = entries_.emplace(slot, Entry{ChangeKind::Edit, std::move(next), std::nullopt, {}}).first;
    else it->second.record = std::move(next);
    it->second.notes.push_back(std::move(note));
    if (it->second.kind == ChangeKind::Edit && base.pokemon) {
        const auto a = base.pokemon->plain(), b = it->second.record.plain();
        if (std::equal(a.begin(), a.end(), b.begin(), b.end())) entries_.erase(it);
    }
}

bool Gen8bStagedSession::stageValue(SlotRef slot, Field field, uint32_t value, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (!slot.canonical()) return fail("BD/SP edit target is not a canonical slot");
    if (const auto r = editBlockedReason(slot); !r.empty()) return fail(std::string(r));
    if (const auto it = entries_.find(slot); it != entries_.end() && it->second.kind == ChangeKind::Remove)
        return fail("BD/SP cannot edit a slot staged for removal");
    auto rec = current(slot);
    if (!rec || rec->empty()) return fail("BD/SP edit requires a valid occupied PB8");
    std::string note = slotLabel(slot) + ": ", why;
    if (!Gen8Edits::applyValue(*rec, field, value, rules(), note, why)) return fail(why);
    commitEdit(slot, std::move(*rec), std::move(note));
    if (error) error->clear();
    return true;
}

bool Gen8bStagedSession::stageText(SlotRef slot, Field field, std::string_view text, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (!slot.canonical()) return fail("BD/SP edit target is not a canonical slot");
    if (const auto r = editBlockedReason(slot); !r.empty()) return fail(std::string(r));
    if (const auto it = entries_.find(slot); it != entries_.end() && it->second.kind == ChangeKind::Remove)
        return fail("BD/SP cannot edit a slot staged for removal");
    auto rec = current(slot);
    if (!rec || rec->empty()) return fail("BD/SP edit requires a valid occupied PB8");
    std::string note = slotLabel(slot) + ": ", why;
    if (!Gen8Edits::applyText(*rec, field, text, rules(), note, why)) return fail(why);
    commitEdit(slot, std::move(*rec), std::move(note));
    if (error) error->clear();
    return true;
}

bool Gen8bStagedSession::cloneToBox(SlotRef from, size_t box, size_t slot, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    const SlotRef to = SlotRef::boxed(box, slot);
    if (!from.canonical() || !to.canonical() || from == to) return fail("BD/SP clone requires distinct canonical slots");
    if (from.region != RecordRegion::Box) return fail("BD/SP clone source must be a box record (party deposit rules are not verified)");
    if (entries_.count(from) || entries_.count(to)) return fail("BD/SP clone requires an unmodified source and destination");
    if (const auto r = storageBlockedReason(to); !r.empty()) return fail(std::string(r));
    const auto src = original(from), dst = original(to);
    if (!src.occupied() || !src.pokemon) return fail("BD/SP clone source is not a valid occupied PB8");
    if (!dst.blank()) return fail("BD/SP clone destination must be a verified blank box slot");
    entries_.emplace(to, Entry{ChangeKind::Clone, *src.pokemon, from, {slotLabel(to) + ": clone of " + slotLabel(from)}});
    if (error) error->clear();
    return true;
}

bool Gen8bStagedSession::removeFromBox(size_t box, size_t slot, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    const SlotRef at = SlotRef::boxed(box, slot);
    if (!at.canonical()) return fail("BD/SP removal target is not a canonical box slot");
    if (entries_.count(at)) return fail("BD/SP removal requires an unmodified box slot");
    if (const auto r = storageBlockedReason(at); !r.empty()) return fail(std::string(r));
    if (!original(at).occupied()) return fail("BD/SP removal requires one valid occupied box record");
    entries_.emplace(at, Entry{ChangeKind::Remove, PK8::Pokemon8::blank(PK8::PartySize), std::nullopt, {slotLabel(at) + ": removed"}});
    if (error) error->clear();
    return true;
}

Gen8bStagedSession::EntrySnapshot Gen8bStagedSession::snapshotEntry(SlotRef slot) const {
    EntrySnapshot out;
    const auto it = entries_.find(slot);
    if (it == entries_.end()) return out;
    out.present = true;
    out.kind = it->second.kind;
    out.record = it->second.record;
    out.cloneSource = it->second.cloneSource;
    out.notes = it->second.notes;
    return out;
}

void Gen8bStagedSession::restoreEntry(SlotRef slot, const EntrySnapshot& s) {
    if (!s.present || !s.record) {
        entries_.erase(slot);
        return;
    }
    entries_.insert_or_assign(slot, Entry{s.kind, *s.record, s.cloneSource, s.notes});
}

bool Gen8bStagedSession::entryMatches(SlotRef slot, const EntrySnapshot& s) const {
    const auto it = entries_.find(slot);
    if (it == entries_.end()) return !s.present;
    if (!s.present || !s.record) return false;
    const auto a = it->second.record.plain(), b = s.record->plain();
    return it->second.kind == s.kind && std::equal(a.begin(), a.end(), b.begin(), b.end()) &&
           it->second.cloneSource == s.cloneSource && it->second.notes == s.notes;
}

std::vector<StagedChange> Gen8bStagedSession::changes() const {
    std::vector<StagedChange> out;
    for (const auto& [slot, e] : entries_) {
        StagedChange c;
        c.slot = slot;
        c.kind = e.kind;
        c.cloneSource = e.cloneSource;
        if (const auto base = original(slot); base.pokemon)
            c.originalPlain.assign(base.pokemon->plain().begin(), base.pokemon->plain().end());
        c.stagedPlain.assign(e.record.plain().begin(), e.record.plain().end());
        PK8::write16(c.stagedPlain, 0x06, PK8::checksum(c.stagedPlain));
        c.notes = e.notes;
        out.push_back(std::move(c));
    }
    return out;
}

std::vector<std::string> Gen8bStagedSession::changeLines() const {
    std::vector<std::string> lines;
    for (const auto& [slot, e] : entries_) lines.insert(lines.end(), e.notes.begin(), e.notes.end());
    return lines;
}

std::optional<std::vector<uint8_t>> Gen8bStagedSession::buildWorkingCopy(std::string* error) const {
    const auto fail = [&](const std::string& why) -> std::optional<std::vector<uint8_t>> {
        if (error) *error = why;
        return std::nullopt;
    };
    if (!hasChanges()) return fail("BD/SP working copy needs at least one staged change");
    std::vector<uint8_t> out(source_.bytes().begin(), source_.bytes().end());
    for (const auto& [slot, e] : entries_) {
        if (e.record.size() != Layout::RecordSize) return fail("BD/SP staged record geometry mismatch");
        const auto enc = e.record.serialize();
        std::copy(enc.begin(), enc.end(), out.begin() + static_cast<std::ptrdiff_t>(offsetOf(slot)));
    }
    Gen8bSave::refreshHash(out);
    std::string why;
    if (!verifyWorkingCopy(source_, out, changes(), &why)) return fail(why);
    if (error) error->clear();
    return out;
}

Common::VerifiedExportResult Gen8bStagedSession::exportWorkingCopy(const Gen8bExportRequest& request) const {
    Common::VerifiedExportResult result;
    std::string error;
    const auto edited = buildWorkingCopy(&error);
    if (!edited) {
        result.error = error;
        return result;
    }
    const std::string gameId(source_.exactGameId());
    Common::VerifiedExportRequest r;
    r.rootDirectory = request.rootDirectory;
#ifdef __SWITCH__
    const std::string fixedRoot = PokeBank::Paths::exportsRoot() + "/bdsp";
    if (r.rootDirectory.empty()) r.rootDirectory = fixedRoot;
    if (r.rootDirectory != fixedRoot) {
        result.error = "BD/SP export cannot leave app-owned exports/bdsp";
        return result;
    }
#endif
    if (r.rootDirectory.empty() || request.timestamp.empty()) {
        result.error = "BD/SP export needs an app-owned root and a timestamp";
        return result;
    }
    r.directoryStem = gameId + "_" + request.timestamp;
    r.generationLabel = "Generation VIII";
    r.fileExtension = ".bin";
    r.sourcePath = request.sourcePath;
    r.gameId = gameId;
    r.saveFormat = "BD/SP SaveData (SAV8BS rev " + std::string(source_.revision()) + ", " + std::to_string(source_.bytes().size()) + " bytes)";
    r.timestamp = request.timestamp;
    r.applicationSha = request.applicationSha;
    r.originalBytes = source_.bytes();
    r.editedBytes = *edited;
    r.changeLines = changeLines();
    r.failurePoint = request.failurePoint;
    const auto expected = changes();
    const Gen8bSave& baseline = source_;
    r.validator = [&baseline, expected](std::span<const uint8_t> bytes, std::string& why) {
        return verifyWorkingCopy(baseline, bytes, expected, &why);
    };
    return Common::publishVerifiedStagedExport(r);
}

bool verifyWorkingCopy(const Gen8bSave& source, std::span<const uint8_t> edited, const std::vector<StagedChange>& changes,
                       std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    const auto original = source.bytes();
    if (edited.size() != original.size()) return fail("BD/SP working copy size changed");
    std::string why;
    const auto reparsed = Gen8bSave::parse(edited, source.exactGameId(), &why);
    if (!reparsed) return fail("BD/SP working copy failed strict reparse: " + why);
    if (reparsed->partyCount() != source.partyCount()) return fail("BD/SP working copy changed party count");
    std::vector<uint8_t> allowed(edited.size(), 0);
    std::fill_n(allowed.begin() + static_cast<std::ptrdiff_t>(Layout::HashOffset), Layout::HashBytes, uint8_t{1});
    for (const auto& c : changes) {
        if (!c.slot.canonical()) return fail("BD/SP change has a noncanonical slot");
        std::fill_n(allowed.begin() + static_cast<std::ptrdiff_t>(offsetOf(c.slot)), Layout::RecordSize, uint8_t{1});
        const auto rec = c.slot.region == RecordRegion::Party ? reparsed->party(c.slot.slot) : reparsed->box(c.slot.box, c.slot.slot);
        if (!rec.pokemon) return fail("BD/SP staged record does not decode strictly");
        const auto plain = rec.pokemon->plain();
        if (!std::equal(plain.begin(), plain.end(), c.stagedPlain.begin(), c.stagedPlain.end()))
            return fail("BD/SP staged record differs from the reviewed change");
        const auto src = c.slot.region == RecordRegion::Party ? source.party(c.slot.slot) : source.box(c.slot.box, c.slot.slot);
        if (!src.pokemon || !std::equal(c.originalPlain.begin(), c.originalPlain.end(), src.pokemon->plain().begin(), src.pokemon->plain().end()))
            return fail("BD/SP baseline record differs from the immutable source");
        const auto* team = c.slot.region == RecordRegion::Box ? teamAt(source, c.slot.box, c.slot.slot) : nullptr;
        if (c.slot.region == RecordRegion::Box && !source.teams()) return fail("BD/SP box change without readable battle teams");
        size_t bad = 0;
        switch (c.kind) {
            case ChangeKind::Edit:
                if (!src.occupied() || rec.pokemon->empty()) return fail("BD/SP edit must keep an occupied record");
                if (team && team->locked) return fail("BD/SP edit touched a locked battle-team member");
                if (!Gen8Edits::withinMask(c.originalPlain, plain, c.slot.region, &bad))
                    return fail("BD/SP edit touched a non-whitelisted PB8 byte at 0x" + std::to_string(bad));
                break;
            case ChangeKind::Clone: {
                if (c.slot.region != RecordRegion::Box || !c.cloneSource || !src.blank() || team ||
                    c.cloneSource->region != RecordRegion::Box)
                    return fail("BD/SP clone must copy a box record into a blank, team-free box slot");
                const auto donor = source.box(c.cloneSource->box, c.cloneSource->slot);
                if (!donor.occupied() || !Gen8Edits::withinMask(donor.pokemon->plain(), plain, RecordRegion::Box, &bad))
                    return fail("BD/SP clone differs from its immutable donor outside editable fields");
                break;
            }
            case ChangeKind::Remove:
                if (c.slot.region != RecordRegion::Box || !src.occupied() || !rec.blank() || team)
                    return fail("BD/SP removal must blank an occupied, team-free box slot");
                break;
        }
    }
    for (size_t i = 0; i < edited.size(); ++i)
        if (edited[i] != original[i] && !allowed[i])
            return fail("BD/SP working copy changed a non-whitelisted byte at offset " + std::to_string(i));
    if (error) error->clear();
    return true;
}

} // namespace PokeVault::Integration::Gen8b
