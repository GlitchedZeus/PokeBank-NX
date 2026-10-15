#include "Integration/Gen7/Gen7StagedSession.h"

#include <algorithm>
#include <array>
#include <set>

namespace PokeVault::Integration::Gen7 {
namespace {

constexpr std::array<const char*, 6> StatNames = {"HP", "Atk", "Def", "Spe", "SpA", "SpD"};

std::string slotLabel(SlotRef s) {
    if (s.region == RecordRegion::Party) return "Party slot " + std::to_string(s.slot + 1);
    return "Box " + std::to_string(s.box + 1) + " slot " + std::to_string(s.slot + 1);
}
size_t recordOffset(Layout::Family f, SlotRef s) {
    return s.region == RecordRegion::Party ? Gen7Save::partyRecordOffset(f, s.slot)
                                           : Gen7Save::boxRecordOffset(f, s.box, s.slot);
}
size_t recordSize(SlotRef s) {
    return s.region == RecordRegion::Party ? Layout::PartyRecordSize : Layout::BoxRecordSize;
}
std::optional<size_t> owningBlock(Layout::Family f, size_t offset, size_t length) {
    const auto t = Layout::blocks(f);
    for (size_t id = 0; id < t.size(); ++id)
        if (offset >= t[id].offset && offset + length <= size_t{t[id].offset} + t[id].length) return id;
    return std::nullopt;
}

// Same PK7 offsets as PK6 for every editable field (PKHeX PK7.cs).
std::array<uint8_t, Layout::PartyRecordSize> editMask(RecordRegion region) {
    std::array<uint8_t, Layout::PartyRecordSize> m{};
    const auto range = [&](size_t a, size_t b) { for (size_t i = a; i < b; ++i) m[i] = 0xFF; };
    using Gen7EditorProvider::editable;
    m[0x06] = m[0x07] = 0xFF;
    if (editable(Field::HeldItem, region)) range(0x0A, 0x0C);
    if (editable(Field::TrainerId, region)) range(0x0C, 0x0E);
    if (editable(Field::SecretId, region)) range(0x0E, 0x10);
    if (editable(Field::Nature, region)) m[0x1C] = 0xFF;
    if (editable(Field::FatefulEncounter, region)) m[0x1D] |= 0x01;
    if (editable(Field::EV, region)) range(0x1E, 0x24);
    if (editable(Field::Pokerus, region)) m[0x2B] = 0xFF;
    if (editable(Field::Nickname, region)) { range(0x40, 0x5A); m[0x77] |= 0x80; }
    if (editable(Field::IV, region)) { m[0x74] = m[0x75] = m[0x76] = 0xFF; m[0x77] |= 0x3F; }
    if (editable(Field::OriginalTrainer, region)) range(0xB0, 0xCA);
    if (editable(Field::Friendship, region)) m[0xCA] = 0xFF;
    if (editable(Field::Ball, region)) m[0xDC] = 0xFF;
    if (editable(Field::OriginalTrainerGender, region)) m[0xDD] |= 0x80;
    return m;
}

bool allowedName(std::string_view text) {
    if (text.empty() || text.size() > PK7::NameCapacity) return false;
    bool visible = false;
    for (unsigned char c : text) {
        if (c < 0x20 || c > 0x7E) return false;
        if (c != ' ') visible = true;
    }
    return visible;
}

} // namespace

SlotRecord Gen7StagedSession::original(SlotRef slot) const {
    if (!slot.canonical()) return {};
    return slot.region == RecordRegion::Party ? source_.party(slot.slot) : source_.box(slot.box, slot.slot);
}

std::optional<PK7::Pokemon6> Gen7StagedSession::current(SlotRef slot) const {
    if (const auto it = entries_.find(slot); it != entries_.end()) return it->second.record;
    return original(slot).pokemon;
}

void Gen7StagedSession::commitEdit(SlotRef slot, PK7::Pokemon6 next, std::string note) {
    const auto base = original(slot);
    auto it = entries_.find(slot);
    if (it == entries_.end()) it = entries_.emplace(slot, Entry{ChangeKind::Edit, std::move(next), std::nullopt, {}}).first;
    else it->second.record = std::move(next);
    it->second.notes.push_back(std::move(note));
    if (it->second.kind == ChangeKind::Edit && base.pokemon) {
        const auto a = base.pokemon->plain();
        const auto b = it->second.record.plain();
        if (std::equal(a.begin(), a.end(), b.begin(), b.end())) entries_.erase(it);
    }
}

bool Gen7StagedSession::stageValue(SlotRef slot, Field field, uint32_t value, size_t index, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (!slot.canonical()) return fail("Gen VII edit target is not a canonical slot");
    if (!Gen7EditorProvider::editable(field, slot.region)) return fail("Gen VII field is not editable for this record region");
    if (const auto it = entries_.find(slot); it != entries_.end() && it->second.kind == ChangeKind::Remove)
        return fail("Gen VII cannot edit a slot staged for removal");
    auto rec = current(slot);
    if (!rec || rec->empty()) return fail("Gen VII edit requires a valid occupied PK7");
    const auto lim = limits();
    std::string note = slotLabel(slot) + ": ";
    switch (field) {
        case Field::HeldItem:
            if (value > lim.maxHeldItem) return fail("Gen VII held item exceeds the game's item range");
            note += "held item -> " + std::to_string(value);
            rec->setHeldItem(static_cast<uint16_t>(value));
            break;
        case Field::TrainerId:
        case Field::SecretId:
            if (value > 0xFFFF) return fail("Gen VII trainer ID out of range");
            note += std::string(field == Field::TrainerId ? "TID" : "SID") + " -> " + std::to_string(value);
            field == Field::TrainerId ? rec->setTid(static_cast<uint16_t>(value)) : rec->setSid(static_cast<uint16_t>(value));
            break;
        case Field::Nature:
            if (value > lim.maxNature) return fail("Gen VII nature out of range");
            note += "nature -> " + std::to_string(value);
            rec->setNature(static_cast<uint8_t>(value));
            break;
        case Field::FatefulEncounter:
            if (value > 1) return fail("Gen VII fateful encounter is a single bit");
            note += std::string("fateful encounter -> ") + (value ? "yes" : "no");
            rec->setFatefulEncounter(value != 0);
            break;
        case Field::EV: {
            if (index >= 6 || value > lim.maxEV) return fail("Gen VII EV stat/value out of range");
            uint32_t total = value;
            for (size_t i = 0; i < 6; ++i) if (i != index) total += rec->ev(i);
            if (total > lim.maxEVTotal) return fail("Gen VII EV total would exceed 510");
            note += std::string("EV ") + StatNames[index] + " -> " + std::to_string(value);
            rec->setEv(index, static_cast<uint8_t>(value));
            break;
        }
        case Field::IV:
            if (index >= 6 || value > lim.maxIV) return fail("Gen VII IV stat/value out of range");
            note += std::string("IV ") + StatNames[index] + " -> " + std::to_string(value);
            rec->setIv(index, static_cast<uint8_t>(value));
            break;
        case Field::Pokerus:
            if (value > 0xFF) return fail("Gen VII Pokerus byte out of range");
            note += "Pokerus -> " + std::to_string(value);
            rec->setPokerus(static_cast<uint8_t>(value));
            break;
        case Field::Friendship:
            if (value > 0xFF) return fail("Gen VII friendship out of range");
            if (rec->currentHandler() != 0) return fail("Gen VII handling-trainer friendship editing is unsupported");
            note += "friendship -> " + std::to_string(value);
            rec->setOriginalTrainerFriendship(static_cast<uint8_t>(value));
            break;
        case Field::Ball:
            if (value < 1 || value > lim.maxBall) return fail("Gen VII ball out of range");
            note += "ball -> " + std::to_string(value);
            rec->setBall(static_cast<uint8_t>(value));
            break;
        case Field::OriginalTrainerGender:
            if (value > 1) return fail("Gen VII OT gender is a single bit");
            note += "OT gender -> " + std::to_string(value);
            rec->setOriginalTrainerGender(static_cast<uint8_t>(value));
            break;
        default:
            return fail("Gen VII field has no numeric staged editor");
    }
    commitEdit(slot, std::move(*rec), std::move(note));
    if (error) error->clear();
    return true;
}

bool Gen7StagedSession::stageText(SlotRef slot, Field field, std::string_view text, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (field != Field::Nickname && field != Field::OriginalTrainer)
        return fail("Gen VII text editing supports Nickname and OT name only");
    if (!slot.canonical() || !Gen7EditorProvider::editable(field, slot.region))
        return fail("Gen VII field is not editable for this slot");
    if (const auto it = entries_.find(slot); it != entries_.end() && it->second.kind == ChangeKind::Remove)
        return fail("Gen VII cannot edit a slot staged for removal");
    if (!allowedName(text)) return fail("Gen VII names must be 1-12 printable ASCII characters");
    auto rec = current(slot);
    if (!rec || rec->empty()) return fail("Gen VII edit requires a valid occupied PK7");
    const std::u16string wide(text.begin(), text.end());
    std::string note = slotLabel(slot);
    if (field == Field::Nickname) {
        if (rec->isEgg()) return fail("Gen VII egg nicknames are unsupported");
        rec->setRawName(PK7::NicknameOffset, wide);
        rec->setNicknamedFlag(true);
        note += ": nickname -> " + std::string(text);
    } else {
        rec->setRawName(PK7::OriginalTrainerOffset, wide);
        note += ": OT name -> " + std::string(text);
    }
    commitEdit(slot, std::move(*rec), std::move(note));
    if (error) error->clear();
    return true;
}

bool Gen7StagedSession::cloneToBox(SlotRef from, size_t box, size_t slot, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    const SlotRef to = SlotRef::boxed(box, slot);
    if (!from.canonical() || !to.canonical() || from == to) return fail("Gen VII clone requires canonical, distinct slots");
    if (entries_.count(from) || entries_.count(to)) return fail("Gen VII clone requires an unmodified source and destination");
    const auto src = original(from);
    const auto dst = original(to);
    if (!src.occupied()) return fail("Gen VII clone source is not a valid occupied PK7");
    if (dst.state != SlotState::Empty || !dst.pokemon->allZero())
        return fail("Gen VII clone destination must be a verified blank box slot");
    auto boxed = src.pokemon->partyForm() ? src.pokemon->boxed() : *src.pokemon;
    const auto check = PK7::Pokemon6::decode(boxed.serialize());
    if (!check || check->species() != src.pokemon->species() || check->pid() != src.pokemon->pid())
        return fail("Gen VII boxed clone failed strict PK7 verification");
    entries_.emplace(to, Entry{ChangeKind::Clone, std::move(boxed), from, {slotLabel(to) + ": clone of " + slotLabel(from)}});
    if (error) error->clear();
    return true;
}

bool Gen7StagedSession::removeFromBox(size_t box, size_t slot, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    const SlotRef at = SlotRef::boxed(box, slot);
    if (!at.canonical() || entries_.count(at)) return fail("Gen VII removal requires an unmodified canonical box slot");
    if (!original(at).occupied()) return fail("Gen VII removal requires one valid occupied box record");
    entries_.emplace(at, Entry{ChangeKind::Remove, PK7::Pokemon6::blank(), std::nullopt, {slotLabel(at) + ": removed"}});
    if (error) error->clear();
    return true;
}

std::vector<StagedChange> Gen7StagedSession::changes() const {
    std::vector<StagedChange> out;
    for (const auto& [slot, entry] : entries_) {
        StagedChange c;
        c.slot = slot;
        c.kind = entry.kind;
        c.cloneSource = entry.cloneSource;
        const auto base = original(slot);
        if (base.pokemon) c.originalPlain.assign(base.pokemon->plain().begin(), base.pokemon->plain().end());
        c.stagedPlain.assign(entry.record.plain().begin(), entry.record.plain().end());
        PK7::write16(c.stagedPlain, 0x06, PK7::checksum(c.stagedPlain));
        c.notes = entry.notes;
        out.push_back(std::move(c));
    }
    return out;
}

std::vector<std::string> Gen7StagedSession::changeLines() const {
    std::vector<std::string> lines;
    for (const auto& [slot, entry] : entries_) lines.insert(lines.end(), entry.notes.begin(), entry.notes.end());
    return lines;
}

std::optional<UnsignedCandidate> Gen7StagedSession::buildUnsignedCandidate(std::string* error) const {
    const auto fail = [&](const std::string& why) -> std::optional<UnsignedCandidate> {
        if (error) *error = why;
        return std::nullopt;
    };
    if (entries_.empty()) return fail("Gen VII candidate needs at least one staged change");
    const auto family = source_.family();
    UnsignedCandidate out;
    out.bytes.assign(source_.bytes().begin(), source_.bytes().end());
    std::set<size_t> touched;
    for (const auto& [slot, entry] : entries_) {
        const size_t offset = recordOffset(family, slot);
        const size_t size = recordSize(slot);
        if (entry.record.size() != size) return fail("Gen VII staged record geometry mismatch");
        const auto block = owningBlock(family, offset, size);
        if (!block || *block == Layout::MemeCryptoBlock) return fail("Gen VII staged record escapes its block");
        const auto encrypted = entry.record.serialize();
        std::copy(encrypted.begin(), encrypted.end(), out.bytes.begin() + static_cast<std::ptrdiff_t>(offset));
        touched.insert(*block);
    }
    for (const size_t id : touched)
        PK7::write16(out.bytes, Layout::checksumOffset(family, id), Gen7Save::computeBlockChecksum(out.bytes, family, id));
    std::string why;
    if (!verifyUnsignedCandidate(source_, out.bytes, changes(), &why)) return fail(why);
    if (error) error->clear();
    return out;
}

bool verifyUnsignedCandidate(const Gen7Save& source, std::span<const uint8_t> edited,
                             const std::vector<StagedChange>& changes, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    const auto original = source.bytes();
    if (edited.size() != original.size()) return fail("Gen VII candidate size changed");
    std::string parseError;
    const auto reparsed = Gen7Save::parse(edited, source.exactGameId(), &parseError, SignaturePolicy::Ignore);
    if (!reparsed) return fail("Gen VII candidate failed strict reparse: " + parseError);
    if (reparsed->partyCount() != source.partyCount()) return fail("Gen VII candidate changed party count");
    const auto family = source.family();
    std::vector<uint8_t> allowed(edited.size(), 0);
    std::set<size_t> blocks;
    for (const auto& c : changes) {
        if (!c.slot.canonical()) return fail("Gen VII change has a noncanonical slot");
        const size_t offset = recordOffset(family, c.slot);
        const size_t size = recordSize(c.slot);
        const auto block = owningBlock(family, offset, size);
        if (!block || *block == Layout::MemeCryptoBlock) return fail("Gen VII change escapes its block");
        blocks.insert(*block);
        std::fill_n(allowed.begin() + static_cast<std::ptrdiff_t>(offset), size, uint8_t{0xFF});
        const auto rec = c.slot.region == RecordRegion::Party ? reparsed->party(c.slot.slot)
                                                              : reparsed->box(c.slot.box, c.slot.slot);
        const auto src = c.slot.region == RecordRegion::Party ? source.party(c.slot.slot)
                                                              : source.box(c.slot.box, c.slot.slot);
        if (!rec.pokemon || !src.pokemon) return fail("Gen VII staged record does not decode strictly");
        const auto plain = rec.pokemon->plain();
        if (!std::equal(plain.begin(), plain.end(), c.stagedPlain.begin(), c.stagedPlain.end()))
            return fail("Gen VII staged record differs from the reviewed change");
        const auto base = src.pokemon->plain();
        if (!std::equal(base.begin(), base.end(), c.originalPlain.begin(), c.originalPlain.end()))
            return fail("Gen VII baseline record differs from the immutable source");
        switch (c.kind) {
            case ChangeKind::Edit: {
                if (!src.occupied() || rec.pokemon->empty()) return fail("Gen VII edit must keep an occupied record");
                const auto mask = editMask(c.slot.region);
                for (size_t i = 0; i < plain.size(); ++i)
                    if ((plain[i] ^ base[i]) & ~mask[i])
                        return fail("Gen VII edit touched a non-whitelisted PK7 byte at 0x" + std::to_string(i));
                break;
            }
            case ChangeKind::Clone: {
                if (c.slot.region != RecordRegion::Box || !c.cloneSource || src.state != SlotState::Empty ||
                    !src.pokemon->allZero())
                    return fail("Gen VII clone must target a blank box slot");
                const auto& from = *c.cloneSource;
                const auto donor = from.region == RecordRegion::Party ? source.party(from.slot) : source.box(from.box, from.slot);
                if (!donor.occupied()) return fail("Gen VII clone donor is invalid");
                // A staged clone may carry later field edits, held to the boxed edit mask.
                const auto d = donor.pokemon->plain();
                const auto mask = editMask(RecordRegion::Box);
                if (plain.size() != Layout::BoxRecordSize) return fail("Gen VII clone is not a boxed record");
                for (size_t i = 0; i < Layout::BoxRecordSize; ++i)
                    if ((plain[i] ^ d[i]) & ~mask[i])
                        return fail("Gen VII clone differs from its immutable donor outside editable fields");
                break;
            }
            case ChangeKind::Remove:
                if (c.slot.region != RecordRegion::Box || !src.occupied() || rec.state != SlotState::Empty ||
                    !rec.pokemon->allZero())
                    return fail("Gen VII removal must blank an occupied box slot");
                break;
        }
    }
    for (const size_t id : blocks) {
        const size_t at = Layout::checksumOffset(family, id);
        allowed[at] = allowed[at + 1] = 0xFF;
    }
    for (size_t i = 0; i < edited.size(); ++i)
        if ((edited[i] ^ original[i]) & ~allowed[i])
            return fail("Gen VII candidate changed a non-whitelisted save byte at offset " + std::to_string(i));
    if (error) error->clear();
    return true;
}

} // namespace PokeVault::Integration::Gen7
