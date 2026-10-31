#include "Integration/Gen8a/Gen8aSession.h"

#include "Enums/Ball.h"
#include "Names/ItemPresence.h"

#ifdef __SWITCH__
#include "Utils/PokeBankPaths.h"
#endif

#include <algorithm>
#include <array>

namespace PokeVault::Integration::Gen8a {
namespace {

std::string slotLabel(SlotRef s) {
    if (s.region == Region::Party) return "Party slot " + std::to_string(s.slot + 1);
    return "Box " + std::to_string(s.box + 1) + " slot " + std::to_string(s.slot + 1);
}

// Per-byte whitelist inside a decrypted PA8 (PKHeX PA8.cs offsets), derived from fieldState().
std::array<uint8_t, PA8::PartySize> editMask() {
    std::array<uint8_t, PA8::PartySize> m{};
    const auto range = [&](size_t a, size_t b) { for (size_t i = a; i < b; ++i) m[i] = 0xFF; };
    m[0x06] = m[0x07] = 0xFF;
    if (editable(Field::HeldItem)) range(0x0A, 0x0C);
    if (editable(Field::TrainerId)) range(0x0C, 0x0E);
    if (editable(Field::SecretId)) range(0x0E, 0x10);
    if (editable(Field::FatefulEncounter)) m[0x22] |= 0x01;
    if (editable(Field::Nickname)) { range(PA8::NicknameOffset, PA8::NicknameOffset + PA8::NameBytes); m[0x97] |= 0x80; }
    if (editable(Field::OriginalTrainer)) range(PA8::OriginalTrainerOffset, PA8::OriginalTrainerOffset + PA8::NameBytes);
    if (editable(Field::Friendship)) m[0x12A] = 0xFF;
    if (editable(Field::Ball)) m[0x137] = 0xFF;
    if (editable(Field::OriginalTrainerGender)) m[0x13D] |= 0x80;
    return m;
}
bool withinMask(std::span<const uint8_t> a, std::span<const uint8_t> b, size_t* bad) {
    if (a.size() != b.size()) return false;
    const auto m = editMask();
    for (size_t i = 0; i < a.size(); ++i)
        if ((a[i] ^ b[i]) & ~m[i]) { if (bad) *bad = i; return false; }
    return true;
}
bool allowedName(std::string_view t) {
    if (t.empty() || t.size() > PA8::NameCapacity) return false;
    bool visible = false;
    for (unsigned char c : t) {
        if (c < 0x20 || c > 0x7E) return false;
        if (c != ' ') visible = true;
    }
    return visible;
}
bool heldItemAllowed(uint32_t id) {
    return id == 0 || (id <= Names::ITEM_PRESENCE_MAX_ID && Names::isHeldItemPresent(static_cast<uint16_t>(id), Enums::GameVersion::PLA));
}
bool ballAllowed(uint32_t b) {
    const auto list = Enums::getBallList(Enums::GameVersion::PLA);
    return std::find(list.begin(), list.end(), b) != list.end();
}
Record decodeAt(const std::vector<Swish::Block>& blocks, size_t partyIdx, size_t boxIdx, SlotRef s) {
    Record r;
    const auto& data = blocks[s.region == Region::Party ? partyIdx : boxIdx].data;
    if (!s.canonical() || s.offset() + s.recordSize() > data.size()) return r;
    r.pokemon = PA8::Pokemon8a::decode(std::span<const uint8_t>(data).subspan(s.offset(), s.recordSize()));
    r.valid = r.pokemon.has_value();
    return r;
}

} // namespace

std::optional<Gen8aSave> Gen8aSave::parse(std::span<const uint8_t> bytes, std::string_view assigned, std::string* error) {
    const auto fail = [&](const std::string& why) -> std::optional<Gen8aSave> {
        if (error) *error = why;
        return std::nullopt;
    };
    if (std::find(Layout::Sizes.begin(), Layout::Sizes.end(), bytes.size()) == Layout::Sizes.end())
        return fail("not a Legends: Arceus main size (SaveUtil.SIZE_G8LA)");
    Swish::ParseStatus status{};
    auto blocks = Swish::decrypt(bytes, &status);
    if (!blocks) return fail(std::string("Legends: Arceus ") + Swish::parseStatusText(status));
    Gen8aSave out;
    out.bytes_.assign(bytes.begin(), bytes.end());
    out.blocks_ = std::move(*blocks);
    const auto find = [&](uint32_t key, size_t minimum, size_t& idx, const char* name) -> std::string {
        for (size_t i = 0; i < out.blocks_.size(); ++i)
            if (out.blocks_[i].key == key) {
                if (!out.blocks_[i].bytePayload()) return std::string("Legends: Arceus ") + name + " block is not a byte payload";
                if (out.blocks_[i].data.size() < minimum) return std::string("Legends: Arceus ") + name + " block is truncated";
                idx = i;
                return {};
            }
        return std::string("Legends: Arceus ") + name + " block is missing";
    };
    if (auto why = find(Layout::KeyMyStatus, Layout::MyName + Layout::MyNameBytes, out.myStatus_, "MyStatus"); !why.empty()) return fail(why);
    if (auto why = find(Layout::KeyParty, Layout::PartyCountOffset + 1, out.party_, "Party"); !why.empty()) return fail(why);
    if (auto why = find(Layout::KeyBox, Layout::BoxCount * Layout::BoxSlots * PA8::StoredSize, out.box_, "Box"); !why.empty()) return fail(why);
    if (out.blocks_[out.myStatus_].data[Layout::MyGame] != Layout::VersionPLA)
        return fail("Legends: Arceus MyStatus game is not Legends: Arceus (47)");
    if (!assigned.empty() && assigned != "legends_arceus_switch") return fail("Legends: Arceus save does not match the assigned exact game");
    out.partyCount_ = out.blocks_[out.party_].data[Layout::PartyCountOffset];
    if (out.partyCount_ == 0 || out.partyCount_ > Layout::PartySlots) return fail("Legends: Arceus party count is outside 1..6");
    for (size_t i = 0; i < out.partyCount_; ++i)
        if (!out.at(SlotRef::party(i)).occupied()) return fail("Legends: Arceus declared party slot is not a valid occupied PA8");
    if (error) error->clear();
    return out;
}

Record Gen8aSave::at(SlotRef slot) const { return decodeAt(blocks_, party_, box_, slot); }

std::optional<PA8::Pokemon8a> Gen8aStagedSession::current(SlotRef slot) const {
    if (const auto it = entries_.find(slot); it != entries_.end()) return it->second.record;
    return source_.at(slot).pokemon;
}

void Gen8aStagedSession::commitEdit(SlotRef slot, PA8::Pokemon8a next, std::string note) {
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

bool Gen8aStagedSession::stageValue(SlotRef slot, Field field, uint32_t value, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (!slot.canonical()) return fail("Legends: Arceus edit target is not a canonical slot");
    if (!editable(field)) return fail("Legends: Arceus field is not editable in this build");
    if (const auto it = entries_.find(slot); it != entries_.end() && it->second.kind == ChangeKind::Remove)
        return fail("Legends: Arceus cannot edit a slot staged for removal");
    auto rec = current(slot);
    if (!rec || rec->empty()) return fail("Legends: Arceus edit requires a valid occupied PA8");
    std::string note = slotLabel(slot) + ": ";
    switch (field) {
        case Field::HeldItem:
            if (value > 0xFFFF || !heldItemAllowed(value)) return fail("Legends: Arceus held item is not holdable in this game");
            note += "held item " + std::to_string(rec->heldItem()) + " -> " + std::to_string(value);
            rec->setHeldItem(static_cast<uint16_t>(value));
            break;
        case Field::TrainerId:
        case Field::SecretId: {
            if (value > 0xFFFF) return fail("Legends: Arceus trainer ID half out of range");
            const bool tid = field == Field::TrainerId;
            note += std::string(tid ? "TID16 " : "SID16 ") + std::to_string(tid ? rec->tid() : rec->sid()) + " -> " + std::to_string(value);
            tid ? rec->setTid(static_cast<uint16_t>(value)) : rec->setSid(static_cast<uint16_t>(value));
            break;
        }
        case Field::FatefulEncounter:
            if (value > 1) return fail("Legends: Arceus fateful encounter is a single bit");
            note += std::string("fateful encounter -> ") + (value ? "yes" : "no");
            rec->setFatefulEncounter(value != 0);
            break;
        case Field::Friendship:
            if (value > 0xFF) return fail("Legends: Arceus friendship out of range");
            if (rec->currentHandler() != 0) return fail("Legends: Arceus handling-trainer friendship editing is unsupported");
            note += "friendship " + std::to_string(rec->originalTrainerFriendship()) + " -> " + std::to_string(value);
            rec->setOriginalTrainerFriendship(static_cast<uint8_t>(value));
            break;
        case Field::Ball:
            if (!ballAllowed(value)) return fail("Legends: Arceus ball is not a Hisui ball");
            note += "ball " + std::to_string(rec->ball()) + " -> " + std::to_string(value);
            rec->setBall(static_cast<uint8_t>(value));
            break;
        case Field::OriginalTrainerGender:
            if (value > 1) return fail("Legends: Arceus OT gender is a single bit");
            note += "OT gender -> " + std::to_string(value);
            rec->setOriginalTrainerGender(static_cast<uint8_t>(value));
            break;
        default:
            return fail("Legends: Arceus field has no numeric staged editor");
    }
    commitEdit(slot, std::move(*rec), std::move(note));
    if (error) error->clear();
    return true;
}

bool Gen8aStagedSession::stageText(SlotRef slot, Field field, std::string_view text, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (field != Field::Nickname && field != Field::OriginalTrainer) return fail("Legends: Arceus text editing supports Nickname and OT name only");
    if (!slot.canonical()) return fail("Legends: Arceus edit target is not a canonical slot");
    if (!allowedName(text)) return fail("Legends: Arceus names must be 1-12 printable ASCII characters");
    if (const auto it = entries_.find(slot); it != entries_.end() && it->second.kind == ChangeKind::Remove)
        return fail("Legends: Arceus cannot edit a slot staged for removal");
    auto rec = current(slot);
    if (!rec || rec->empty()) return fail("Legends: Arceus edit requires a valid occupied PA8");
    const std::u16string wide(text.begin(), text.end());
    std::string note = slotLabel(slot) + ": ";
    if (field == Field::Nickname) {
        if (rec->isEgg()) return fail("Legends: Arceus egg nicknames are unsupported");
        rec->setRawName(PA8::NicknameOffset, wide);
        rec->setNicknamedFlag(true);
        note += "nickname -> " + std::string(text);
    } else {
        rec->setRawName(PA8::OriginalTrainerOffset, wide);
        note += "OT name -> " + std::string(text);
    }
    commitEdit(slot, std::move(*rec), std::move(note));
    if (error) error->clear();
    return true;
}

bool Gen8aStagedSession::cloneToBox(SlotRef from, SlotRef to, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (!from.canonical() || !to.canonical() || from == to) return fail("Legends: Arceus clone requires distinct canonical slots");
    if (from.region != Region::Box || to.region != Region::Box)
        return fail("Legends: Arceus clone copies box records into box slots (party conversion is not verified)");
    if (entries_.count(from) || entries_.count(to)) return fail("Legends: Arceus clone requires an unmodified source and destination");
    const auto src = source_.at(from), dst = source_.at(to);
    if (!src.occupied()) return fail("Legends: Arceus clone source is not a valid occupied PA8");
    if (!dst.blank()) return fail("Legends: Arceus clone destination must be a verified blank box slot");
    entries_.emplace(to, Entry{ChangeKind::Clone, *src.pokemon, from, {slotLabel(to) + ": clone of " + slotLabel(from)}});
    if (error) error->clear();
    return true;
}

bool Gen8aStagedSession::removeFromBox(SlotRef at, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (!at.canonical() || at.region != Region::Box) return fail("Legends: Arceus removal is offered for box records only");
    if (entries_.count(at)) return fail("Legends: Arceus removal requires an unmodified box slot");
    if (!source_.at(at).occupied()) return fail("Legends: Arceus removal requires one valid occupied box record");
    entries_.emplace(at, Entry{ChangeKind::Remove, PA8::Pokemon8a::blank(PA8::StoredSize), std::nullopt, {slotLabel(at) + ": removed"}});
    if (error) error->clear();
    return true;
}

Gen8aStagedSession::EntrySnapshot Gen8aStagedSession::snapshotEntry(SlotRef slot) const {
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

void Gen8aStagedSession::restoreEntry(SlotRef slot, const EntrySnapshot& s) {
    if (!s.present || !s.record) {
        entries_.erase(slot);
        return;
    }
    entries_.insert_or_assign(slot, Entry{s.kind, *s.record, s.cloneSource, s.notes});
}

bool Gen8aStagedSession::entryMatches(SlotRef slot, const EntrySnapshot& s) const {
    const auto it = entries_.find(slot);
    if (it == entries_.end()) return !s.present;
    if (!s.present || !s.record) return false;
    const auto a = it->second.record.plain(), b = s.record->plain();
    return it->second.kind == s.kind && std::equal(a.begin(), a.end(), b.begin(), b.end()) &&
           it->second.cloneSource == s.cloneSource && it->second.notes == s.notes;
}

std::vector<StagedChange> Gen8aStagedSession::changes() const {
    std::vector<StagedChange> out;
    for (const auto& [slot, e] : entries_) {
        StagedChange c;
        c.slot = slot;
        c.kind = e.kind;
        c.cloneSource = e.cloneSource;
        if (const auto base = source_.at(slot); base.pokemon) c.originalPlain.assign(base.pokemon->plain().begin(), base.pokemon->plain().end());
        c.stagedPlain.assign(e.record.plain().begin(), e.record.plain().end());
        PA8::write16(c.stagedPlain, 0x06, PA8::checksum(c.stagedPlain));
        c.notes = e.notes;
        out.push_back(std::move(c));
    }
    return out;
}

std::vector<std::string> Gen8aStagedSession::changeLines() const {
    std::vector<std::string> lines;
    for (const auto& [slot, e] : entries_) lines.insert(lines.end(), e.notes.begin(), e.notes.end());
    return lines;
}

std::optional<std::vector<uint8_t>> Gen8aStagedSession::buildWorkingCopy(std::string* error) const {
    const auto fail = [&](const std::string& why) -> std::optional<std::vector<uint8_t>> {
        if (error) *error = why;
        return std::nullopt;
    };
    if (!hasChanges()) return fail("Legends: Arceus working copy needs at least one staged change");
    auto blocks = source_.blocks();
    for (const auto& [slot, e] : entries_) {
        if (e.record.size() != slot.recordSize()) return fail("Legends: Arceus staged record geometry mismatch");
        auto& data = blocks[slot.region == Region::Party ? source_.partyIndex() : source_.boxIndex()].data;
        const auto enc = e.record.serialize();
        std::copy(enc.begin(), enc.end(), data.begin() + static_cast<std::ptrdiff_t>(slot.offset()));
    }
    auto out = Swish::encrypt(blocks);
    std::string why;
    if (!verifyWorkingCopy(source_, out, changes(), &why)) return fail(why);
    if (error) error->clear();
    return out;
}

Common::VerifiedExportResult Gen8aStagedSession::exportWorkingCopy(const Gen8aExportRequest& request) const {
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
    const std::string fixedRoot = PokeBank::Paths::exportsRoot() + "/pla";
    if (r.rootDirectory.empty()) r.rootDirectory = fixedRoot;
    if (r.rootDirectory != fixedRoot) {
        result.error = "Legends: Arceus export cannot leave app-owned exports/pla";
        return result;
    }
#endif
    if (r.rootDirectory.empty() || request.timestamp.empty()) {
        result.error = "Legends: Arceus export needs an app-owned root and a timestamp";
        return result;
    }
    r.directoryStem = "legends_arceus_switch_" + request.timestamp;
    r.generationLabel = "Generation VIII";
    r.fileExtension = ".bin";
    r.sourcePath = request.sourcePath;
    r.gameId = "legends_arceus_switch";
    r.saveFormat = "Legends: Arceus main (SwishCrypto, " + std::to_string(source_.bytes().size()) + " bytes)";
    r.timestamp = request.timestamp;
    r.applicationSha = request.applicationSha;
    r.originalBytes = source_.bytes();
    r.editedBytes = *edited;
    r.changeLines = changeLines();
    const auto expected = changes();
    const Gen8aSave& baseline = source_;
    r.validator = [&baseline, expected](std::span<const uint8_t> bytes, std::string& why) {
        return verifyWorkingCopy(baseline, bytes, expected, &why);
    };
    return Common::publishVerifiedStagedExport(r);
}

bool verifyWorkingCopy(const Gen8aSave& source, std::span<const uint8_t> edited, const std::vector<StagedChange>& changes,
                       std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (edited.size() != source.bytes().size()) return fail("Legends: Arceus working copy size changed");
    std::string why;
    const auto reparsed = Gen8aSave::parse(edited, source.exactGameId(), &why);
    if (!reparsed) return fail("Legends: Arceus working copy failed strict reparse: " + why);
    if (reparsed->partyCount() != source.partyCount()) return fail("Legends: Arceus working copy changed party count");
    const auto canonical = Swish::encrypt(reparsed->blocks());
    if (!std::equal(canonical.begin(), canonical.end(), edited.begin(), edited.end()))
        return fail("Legends: Arceus working copy is not the canonical encryption of its blocks");
    const auto& before = source.blocks();
    const auto& after = reparsed->blocks();
    if (before.size() != after.size()) return fail("Legends: Arceus working copy changed the block count");
    std::vector<uint8_t> partyAllowed(before[source.partyIndex()].data.size(), 0), boxAllowed(before[source.boxIndex()].data.size(), 0);
    for (const auto& c : changes) {
        if (!c.slot.canonical()) return fail("Legends: Arceus change has a noncanonical slot");
        auto& allowed = c.slot.region == Region::Party ? partyAllowed : boxAllowed;
        std::fill_n(allowed.begin() + static_cast<std::ptrdiff_t>(c.slot.offset()), c.slot.recordSize(), uint8_t{1});
        const auto rec = reparsed->at(c.slot);
        if (!rec.pokemon) return fail("Legends: Arceus staged record does not decode strictly");
        const auto plain = rec.pokemon->plain();
        if (!std::equal(plain.begin(), plain.end(), c.stagedPlain.begin(), c.stagedPlain.end()))
            return fail("Legends: Arceus staged record differs from the reviewed change");
        const auto src = source.at(c.slot);
        if (!src.pokemon || !std::equal(c.originalPlain.begin(), c.originalPlain.end(), src.pokemon->plain().begin(), src.pokemon->plain().end()))
            return fail("Legends: Arceus baseline record differs from the immutable source");
        size_t bad = 0;
        switch (c.kind) {
            case ChangeKind::Edit:
                if (!src.occupied() || rec.pokemon->empty()) return fail("Legends: Arceus edit must keep an occupied record");
                if (!withinMask(c.originalPlain, plain, &bad))
                    return fail("Legends: Arceus edit touched a non-whitelisted PA8 byte at 0x" + std::to_string(bad));
                break;
            case ChangeKind::Clone: {
                if (c.slot.region != Region::Box || !c.cloneSource || c.cloneSource->region != Region::Box || !src.blank())
                    return fail("Legends: Arceus clone must copy a box record into a blank box slot");
                const auto donor = source.at(*c.cloneSource);
                if (!donor.occupied() || !withinMask(donor.pokemon->plain(), plain, &bad))
                    return fail("Legends: Arceus clone differs from its immutable donor outside editable fields");
                break;
            }
            case ChangeKind::Remove:
                if (c.slot.region != Region::Box || !src.occupied() || !rec.blank())
                    return fail("Legends: Arceus removal must blank an occupied box slot");
                break;
        }
    }
    for (size_t i = 0; i < before.size(); ++i) {
        const auto& a = before[i];
        const auto& b = after[i];
        if (a.key != b.key || a.type != b.type || a.subtype != b.subtype || a.data.size() != b.data.size())
            return fail("Legends: Arceus working copy changed a block's key, type or size");
        const std::vector<uint8_t>* allowed = i == source.partyIndex() ? &partyAllowed : i == source.boxIndex() ? &boxAllowed : nullptr;
        for (size_t j = 0; j < a.data.size(); ++j)
            if (a.data[j] != b.data[j] && !(allowed && (*allowed)[j]))
                return fail("Legends: Arceus working copy changed a non-whitelisted byte in block 0x" + std::to_string(a.key));
    }
    if (error) error->clear();
    return true;
}

} // namespace PokeVault::Integration::Gen8a
