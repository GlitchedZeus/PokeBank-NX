#include "Integration/Gen8/Gen8StagedSession.h"
#include "Integration/Gen8/Gen8RecordEdits.h"

#ifdef __SWITCH__
#include "Utils/PokeBankPaths.h"
#endif

#include <array>

namespace PokeVault::Integration::Gen8 {
namespace {

std::string slotLabel(SlotRef s) {
    if (s.region == RecordRegion::Party) return "Party slot " + std::to_string(s.slot + 1);
    return "Box " + std::to_string(s.box + 1) + " slot " + std::to_string(s.slot + 1);
}

size_t recordOffset(SlotRef s) {
    return s.region == RecordRegion::Party ? Gen8Save::partyRecordOffset(s.slot)
                                           : Gen8Save::boxRecordOffset(s.box, s.slot);
}

using Gen8Edits::allowedName;
using Gen8Edits::editMask;

bool heldItemInRange(uint32_t value) noexcept { return value <= Gen8EditorProvider::Limits.maxHeldItem; }

const TeamSlot* teamAt(const Gen8Save& save, size_t box, size_t slot) noexcept {
    const auto& teams = save.teams();
    if (!teams) return nullptr;
    const TeamSlot* found = nullptr;
    for (const auto& t : *teams) {
        if (t.box != box || t.slot != slot) continue;
        if (t.locked) return &t;  // a locked reference dominates
        found = &t;
    }
    return found;
}

} // namespace

std::string_view Gen8StagedSession::editBlockedReason(SlotRef slot) const noexcept {
    if (slot.region == RecordRegion::Party) return {};
    if (!source_.teams())
        return "Sword/Shield battle-team blocks could not be read; box changes are refused";
    if (const auto* t = teamAt(source_, slot.box, slot.slot); t && t->locked)
        return "This Pokemon is in a locked Sword/Shield battle team; unlock the team in game first";
    return {};
}

std::string_view Gen8StagedSession::storageBlockedReason(SlotRef slot) const noexcept {
    if (slot.region == RecordRegion::Party)
        return "Party storage changes are unsupported for Sword/Shield (party count/compaction not verified)";
    if (const auto reason = editBlockedReason(slot); !reason.empty()) return reason;
    if (teamAt(source_, slot.box, slot.slot))
        return "This box slot is referenced by a Sword/Shield battle team; remove it from the team in game first";
    return {};
}

SlotRecord Gen8StagedSession::original(SlotRef slot) const {
    if (!slot.canonical()) return {};
    return slot.region == RecordRegion::Party ? source_.party(slot.slot) : source_.box(slot.box, slot.slot);
}

std::optional<PK8::Pokemon8> Gen8StagedSession::current(SlotRef slot) const {
    if (const auto it = entries_.find(slot); it != entries_.end()) return it->second.record;
    return original(slot).pokemon;
}

void Gen8StagedSession::commitEdit(SlotRef slot, PK8::Pokemon8 next, std::string note) {
    const auto base = original(slot);
    auto it = entries_.find(slot);
    if (it == entries_.end()) {
        it = entries_.emplace(slot, Entry{ChangeKind::Edit, std::move(next), std::nullopt, {}}).first;
    } else {
        it->second.record = std::move(next);
    }
    it->second.notes.push_back(std::move(note));
    // An edit that restores the original record exactly is no longer pending.
    if (it->second.kind == ChangeKind::Edit && base.pokemon) {
        const auto a = base.pokemon->plain();
        const auto b = it->second.record.plain();
        if (std::equal(a.begin(), a.end(), b.begin(), b.end())) entries_.erase(it);
    }
}

bool Gen8StagedSession::stageValue(SlotRef slot, Field field, uint32_t value, size_t, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (!slot.canonical()) return fail("Sword/Shield edit target is not a canonical slot");
    if (!Gen8EditorProvider::editable(field, slot.region))
        return fail("Sword/Shield field is not editable in this build");
    if (const auto reason = editBlockedReason(slot); !reason.empty()) return fail(std::string(reason));
    if (const auto it = entries_.find(slot); it != entries_.end() && it->second.kind == ChangeKind::Remove)
        return fail("Sword/Shield cannot edit a slot staged for removal");
    auto rec = current(slot);
    if (!rec || rec->empty()) return fail("Sword/Shield edit requires a valid occupied PK8");
    std::string note = slotLabel(slot) + ": ";
    switch (field) {
        case Field::HeldItem:
            if (!heldItemInRange(value)) return fail("Sword/Shield held item exceeds the game's item range");
            note += "held item " + std::to_string(rec->heldItem()) + " -> " + std::to_string(value);
            rec->setHeldItem(static_cast<uint16_t>(value));
            break;
        case Field::TrainerId:
        case Field::SecretId: {
            if (value > 0xFFFF) return fail("Sword/Shield trainer ID half out of range");
            const bool tid = field == Field::TrainerId;
            note += std::string(tid ? "TID16 " : "SID16 ") + std::to_string(tid ? rec->tid() : rec->sid()) +
                    " -> " + std::to_string(value);
            tid ? rec->setTid(static_cast<uint16_t>(value)) : rec->setSid(static_cast<uint16_t>(value));
            break;
        }
        case Field::FatefulEncounter:
            if (value > 1) return fail("Sword/Shield fateful encounter is a single bit");
            note += std::string("fateful encounter -> ") + (value ? "yes" : "no");
            rec->setFatefulEncounter(value != 0);
            break;
        case Field::Pokerus:
            if (value > 0xFF) return fail("Sword/Shield Pokerus byte out of range");
            note += "Pokerus " + std::to_string(rec->pokerus()) + " -> " + std::to_string(value);
            rec->setPokerus(static_cast<uint8_t>(value));
            break;
        case Field::Friendship:
            if (value > 0xFF) return fail("Sword/Shield friendship out of range");
            // The shared Friendship field is the current trainer's value; only the original-trainer
            // byte is supported (handling-trainer friendship is not).
            if (rec->currentHandler() != 0)
                return fail("Sword/Shield handling-trainer friendship editing is unsupported");
            note += "friendship " + std::to_string(rec->originalTrainerFriendship()) + " -> " + std::to_string(value);
            rec->setOriginalTrainerFriendship(static_cast<uint8_t>(value));
            break;
        case Field::Ball:
            if (value < Gen8EditorProvider::Limits.minBall || value > Gen8EditorProvider::Limits.maxBall)
                return fail("Sword/Shield ball out of range");
            note += "ball " + std::to_string(rec->ball()) + " -> " + std::to_string(value);
            rec->setBall(static_cast<uint8_t>(value));
            break;
        case Field::OriginalTrainerGender:
            if (value > 1) return fail("Sword/Shield OT gender is a single bit");
            note += "OT gender -> " + std::to_string(value);
            rec->setOriginalTrainerGender(static_cast<uint8_t>(value));
            break;
        default:
            return fail("Sword/Shield field has no numeric staged editor");
    }
    commitEdit(slot, std::move(*rec), std::move(note));
    if (error) error->clear();
    return true;
}

bool Gen8StagedSession::stageText(SlotRef slot, Field field, std::string_view text, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (field != Field::Nickname && field != Field::OriginalTrainer)
        return fail("Sword/Shield text editing supports Nickname and OT name only");
    if (!slot.canonical()) return fail("Sword/Shield edit target is not a canonical slot");
    if (!Gen8EditorProvider::editable(field, slot.region))
        return fail("Sword/Shield field is not editable in this build");
    if (const auto reason = editBlockedReason(slot); !reason.empty()) return fail(std::string(reason));
    if (const auto it = entries_.find(slot); it != entries_.end() && it->second.kind == ChangeKind::Remove)
        return fail("Sword/Shield cannot edit a slot staged for removal");
    if (!allowedName(text)) return fail("Sword/Shield names must be 1-12 printable ASCII characters");
    auto rec = current(slot);
    if (!rec || rec->empty()) return fail("Sword/Shield edit requires a valid occupied PK8");
    const std::u16string wide(text.begin(), text.end());
    std::string note = slotLabel(slot);
    if (field == Field::Nickname) {
        if (rec->isEgg()) return fail("Sword/Shield egg nicknames are unsupported");
        rec->setRawName(PK8::NicknameOffset, wide);
        rec->setNicknamedFlag(true);
        note += ": nickname -> " + std::string(text);
    } else {
        rec->setRawName(PK8::OriginalTrainerOffset, wide);
        note += ": OT name -> " + std::string(text);
    }
    commitEdit(slot, std::move(*rec), std::move(note));
    if (error) error->clear();
    return true;
}

bool Gen8StagedSession::cloneToBox(SlotRef from, size_t box, size_t slot, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    const SlotRef to = SlotRef::boxed(box, slot);
    if (!from.canonical() || !to.canonical() || from == to)
        return fail("Sword/Shield clone requires canonical, distinct source and destination");
    if (from.region != RecordRegion::Box)
        return fail("Sword/Shield clone source must be a box record (party deposit/heal rules are not verified)");
    if (entries_.count(from) || entries_.count(to))
        return fail("Sword/Shield clone requires an unmodified source and destination");
    if (const auto reason = storageBlockedReason(to); !reason.empty()) return fail(std::string(reason));
    const auto src = original(from);
    const auto dst = original(to);
    if (!src.occupied() || !src.pokemon) return fail("Sword/Shield clone source is not a valid occupied PK8");
    if (!dst.blank()) return fail("Sword/Shield clone destination must be a verified blank box slot");
    const auto check = PK8::Pokemon8::decode(src.pokemon->serialize());
    if (!check || check->species() != src.pokemon->species() || check->pid() != src.pokemon->pid())
        return fail("Sword/Shield clone failed strict PK8 verification");
    entries_.emplace(to, Entry{ChangeKind::Clone, *src.pokemon, from, {slotLabel(to) + ": clone of " + slotLabel(from)}});
    if (error) error->clear();
    return true;
}

bool Gen8StagedSession::removeFromBox(size_t box, size_t slot, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    const SlotRef at = SlotRef::boxed(box, slot);
    if (!at.canonical()) return fail("Sword/Shield removal target is not a canonical box slot");
    if (entries_.count(at)) return fail("Sword/Shield removal requires an unmodified box slot");
    if (const auto reason = storageBlockedReason(at); !reason.empty()) return fail(std::string(reason));
    if (!original(at).occupied()) return fail("Sword/Shield removal requires one valid occupied box record");
    entries_.emplace(at, Entry{ChangeKind::Remove, PK8::Pokemon8::blank(PK8::PartySize), std::nullopt,
                               {slotLabel(at) + ": removed"}});
    if (error) error->clear();
    return true;
}

std::vector<StagedChange> Gen8StagedSession::changes() const {
    std::vector<StagedChange> out;
    for (const auto& [slot, entry] : entries_) {
        StagedChange c;
        c.slot = slot;
        c.kind = entry.kind;
        c.cloneSource = entry.cloneSource;
        if (const auto base = original(slot); base.pokemon)
            c.originalPlain.assign(base.pokemon->plain().begin(), base.pokemon->plain().end());
        c.stagedPlain.assign(entry.record.plain().begin(), entry.record.plain().end());
        PK8::write16(c.stagedPlain, 0x06, PK8::checksum(c.stagedPlain));
        c.notes = entry.notes;
        out.push_back(std::move(c));
    }
    return out;
}

std::vector<std::string> Gen8StagedSession::changeLines() const {
    std::vector<std::string> lines;
    for (const auto& [slot, entry] : entries_) lines.insert(lines.end(), entry.notes.begin(), entry.notes.end());
    return lines;
}

std::optional<std::vector<uint8_t>> Gen8StagedSession::buildWorkingCopy(std::string* error) const {
    const auto fail = [&](const std::string& why) -> std::optional<std::vector<uint8_t>> {
        if (error) *error = why;
        return std::nullopt;
    };
    if (!hasChanges()) return fail("Sword/Shield working copy needs at least one staged change");
    auto blocks = source_.blocks();  // decrypted copy; the source model is untouched
    const auto partyAt = source_.indexOf(Layout::KeyParty);
    const auto boxAt = source_.indexOf(Layout::KeyBox);
    if (!partyAt || !boxAt) return fail("Sword/Shield Party/Box blocks are missing");
    for (const auto& [slot, entry] : entries_) {
        if (entry.record.size() != Layout::RecordSize) return fail("Sword/Shield staged record geometry mismatch");
        auto& data = blocks[slot.region == RecordRegion::Party ? *partyAt : *boxAt].data;
        const size_t offset = recordOffset(slot);
        if (offset + Layout::RecordSize > data.size()) return fail("Sword/Shield staged record escapes its block");
        const auto encrypted = entry.record.serialize();
        std::copy(encrypted.begin(), encrypted.end(), data.begin() + static_cast<std::ptrdiff_t>(offset));
    }
    auto out = Swish::encrypt(blocks);
    std::string why;
    if (!verifyWorkingCopy(source_, out, changes(), &why)) return fail(why);
    if (error) error->clear();
    return out;
}

Common::VerifiedExportResult Gen8StagedSession::exportWorkingCopy(const Gen8ExportRequest& request) const {
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
    const std::string fixedRoot = PokeBank::Paths::exportsRoot() + "/gen8";
    if (r.rootDirectory.empty()) r.rootDirectory = fixedRoot;
    if (r.rootDirectory != fixedRoot) {
        result.error = "Sword/Shield export cannot leave app-owned exports/gen8";
        return result;
    }
#endif
    if (r.rootDirectory.empty() || request.timestamp.empty()) {
        result.error = "Sword/Shield export needs an app-owned root and a timestamp";
        return result;
    }
    r.directoryStem = gameId + "_" + request.timestamp;
    r.generationLabel = "Generation VIII";
    // The game's file is named `main` (no extension); the copy is published as edited.bin with
    // original_backup.bin beside it. Restoring it is a manual, owner-verified step.
    r.fileExtension = ".bin";
    r.sourcePath = request.sourcePath;
    r.gameId = gameId;
    r.saveFormat = "Sword/Shield main (SwishCrypto, " + std::to_string(source_.bytes().size()) + " bytes)";
    r.timestamp = request.timestamp;
    r.applicationSha = request.applicationSha;
    r.originalBytes = source_.bytes();
    r.editedBytes = *edited;
    r.changeLines = changeLines();
    r.failurePoint = request.failurePoint;
    const auto expected = changes();
    const Gen8Save& baseline = source_;
    r.validator = [&baseline, &expected](std::span<const uint8_t> bytes, std::string& why) {
        return verifyWorkingCopy(baseline, bytes, expected, &why);
    };
    return Common::publishVerifiedStagedExport(r);
}

bool verifyWorkingCopy(const Gen8Save& source, std::span<const uint8_t> edited,
                       const std::vector<StagedChange>& changes, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (edited.size() != source.bytes().size()) return fail("Sword/Shield working copy size changed");
    std::string parseError;
    const auto reparsed = Gen8Save::parse(edited, source.exactGameId(), &parseError);
    if (!reparsed) return fail("Sword/Shield working copy failed strict reparse: " + parseError);
    if (reparsed->partyCount() != source.partyCount()) return fail("Sword/Shield working copy changed party count");
    {
        const auto canonical = Swish::encrypt(reparsed->blocks());
        if (!std::equal(canonical.begin(), canonical.end(), edited.begin(), edited.end()))
            return fail("Sword/Shield working copy is not the canonical encryption of its blocks");
    }
    const auto& before = source.blocks();
    const auto& after = reparsed->blocks();
    if (before.size() != after.size()) return fail("Sword/Shield working copy changed the block count");
    const auto partyAt = source.indexOf(Layout::KeyParty);
    const auto boxAt = source.indexOf(Layout::KeyBox);
    if (!partyAt || !boxAt) return fail("Sword/Shield Party/Box blocks are missing");

    // Per-byte allowance inside the Party and Box block payloads (decrypted space).
    std::vector<uint8_t> partyAllowed(before[*partyAt].data.size(), 0);
    std::vector<uint8_t> boxAllowed(before[*boxAt].data.size(), 0);
    for (const auto& c : changes) {
        if (!c.slot.canonical()) return fail("Sword/Shield change has a noncanonical slot");
        auto& allowed = c.slot.region == RecordRegion::Party ? partyAllowed : boxAllowed;
        const size_t offset = recordOffset(c.slot);
        if (offset + Layout::RecordSize > allowed.size()) return fail("Sword/Shield change escapes its block");
        std::fill_n(allowed.begin() + static_cast<std::ptrdiff_t>(offset), Layout::RecordSize, uint8_t{0xFF});

        const auto rec = c.slot.region == RecordRegion::Party ? reparsed->party(c.slot.slot)
                                                              : reparsed->box(c.slot.box, c.slot.slot);
        if (!rec.pokemon) return fail("Sword/Shield staged record does not decode strictly");
        const auto plain = rec.pokemon->plain();
        if (!std::equal(plain.begin(), plain.end(), c.stagedPlain.begin(), c.stagedPlain.end()))
            return fail("Sword/Shield staged record differs from the reviewed change");
        const auto sourceRec = c.slot.region == RecordRegion::Party ? source.party(c.slot.slot)
                                                                    : source.box(c.slot.box, c.slot.slot);
        if (!sourceRec.pokemon || !std::equal(c.originalPlain.begin(), c.originalPlain.end(),
                                              sourceRec.pokemon->plain().begin(), sourceRec.pokemon->plain().end()))
            return fail("Sword/Shield baseline record differs from the immutable source");
        // Team protection is re-checked against the immutable source, not trusted from the session.
        const auto* team = c.slot.region == RecordRegion::Box ? teamAt(source, c.slot.box, c.slot.slot) : nullptr;
        if (c.slot.region == RecordRegion::Box && !source.teams())
            return fail("Sword/Shield box change without readable battle-team blocks");

        switch (c.kind) {
            case ChangeKind::Edit: {
                if (!sourceRec.occupied() || rec.pokemon->empty()) return fail("Sword/Shield edit must keep an occupied record");
                if (team && team->locked) return fail("Sword/Shield edit touched a locked battle-team member");
                const auto mask = editMask(c.slot.region);
                for (size_t i = 0; i < plain.size(); ++i)
                    if ((plain[i] ^ c.originalPlain[i]) & ~mask[i])
                        return fail("Sword/Shield edit touched a non-whitelisted PK8 byte at 0x" + std::to_string(i));
                break;
            }
            case ChangeKind::Clone: {
                if (c.slot.region != RecordRegion::Box || !c.cloneSource || !sourceRec.blank() || team)
                    return fail("Sword/Shield clone must target a blank, team-free box slot");
                const auto& from = *c.cloneSource;
                if (from.region != RecordRegion::Box) return fail("Sword/Shield clone donor must be a box record");
                const auto donor = source.box(from.box, from.slot);
                if (!donor.occupied() || !donor.pokemon) return fail("Sword/Shield clone donor is invalid");
                const auto d = donor.pokemon->plain();
                const auto mask = editMask(RecordRegion::Box);
                for (size_t i = 0; i < plain.size(); ++i)
                    if ((plain[i] ^ d[i]) & ~mask[i])
                        return fail("Sword/Shield clone differs from its immutable donor outside editable fields");
                break;
            }
            case ChangeKind::Remove:
                if (c.slot.region != RecordRegion::Box || !sourceRec.occupied() || !rec.blank() || team)
                    return fail("Sword/Shield removal must blank an occupied, team-free box slot");
                break;
        }
    }
    for (size_t i = 0; i < before.size(); ++i) {
        const auto& a = before[i];
        const auto& b = after[i];
        if (a.key != b.key || a.type != b.type || a.subtype != b.subtype || a.data.size() != b.data.size())
            return fail("Sword/Shield working copy changed a block's key, type or size");
        const std::vector<uint8_t>* allowed = i == *partyAt ? &partyAllowed : i == *boxAt ? &boxAllowed : nullptr;
        for (size_t j = 0; j < a.data.size(); ++j)
            if ((a.data[j] ^ b.data[j]) & ~(allowed ? (*allowed)[j] : uint8_t{0}))
                return fail("Sword/Shield working copy changed a non-whitelisted byte in block 0x" +
                            std::to_string(a.key) + " at " + std::to_string(j));
    }
    if (error) error->clear();
    return true;
}

} // namespace PokeVault::Integration::Gen8
