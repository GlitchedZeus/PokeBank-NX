#include "Integration/Gen6/Gen6StagedSession.h"
#include "Integration/Gen6/Gen6Stats.h"

#ifdef __SWITCH__
#include "Utils/PokeBankPaths.h"
#endif

#include <algorithm>
#include <array>
#include <set>

namespace PokeVault::Integration::Gen6 {
namespace {

constexpr uint16_t SpeciesFurfrou = 676;
constexpr uint16_t SpeciesHoopa = 720;
constexpr std::array<const char*, 6> StatNames = {"HP", "Atk", "Def", "Spe", "SpA", "SpD"};

std::string slotLabel(SlotRef s) {
    if (s.region == RecordRegion::Party) return "Party slot " + std::to_string(s.slot + 1);
    return "Box " + std::to_string(s.box + 1) + " slot " + std::to_string(s.slot + 1);
}

size_t recordOffset(Layout::Family family, SlotRef s) {
    return s.region == RecordRegion::Party ? Gen6Save::partyRecordOffset(s.slot)
                                           : Gen6Save::boxRecordOffset(family, s.box, s.slot);
}
size_t recordSize(SlotRef s) {
    return s.region == RecordRegion::Party ? PK6::PartySize : PK6::StoredSize;
}

// Per-byte bitmask of what a field edit may change inside a decrypted PK6,
// mirroring Gen6EditorProvider's Editable set for that region exactly.
std::array<uint8_t, PK6::PartySize> editMask(RecordRegion region) {
    std::array<uint8_t, PK6::PartySize> m{};
    const auto range = [&](size_t from, size_t to) { for (size_t i = from; i < to; ++i) m[i] = 0xFF; };
    using Gen6EditorProvider::editable;
    m[0x06] = m[0x07] = 0xFF;  // checksum is always recomputed
    if (editable(Field::HeldItem, region)) range(0x0A, 0x0C);
    if (editable(Field::TrainerId, region)) range(0x0C, 0x0E);
    if (editable(Field::SecretId, region)) range(0x0E, 0x10);
    if (editable(Field::Level, region)) range(0x10, 0x14);  // EXP
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

// True when a battle-stat input (EXP, nature, EVs, IV bits 0..29) differs.
bool statInputsChanged(std::span<const uint8_t> a, std::span<const uint8_t> b) {
    for (size_t i = 0x10; i < 0x14; ++i) if (a[i] != b[i]) return true;
    if (a[0x1C] != b[0x1C]) return true;
    for (size_t i = 0x1E; i < 0x24; ++i) if (a[i] != b[i]) return true;
    return (PK6::read32(a, 0x74) & 0x3FFFFFFFu) != (PK6::read32(b, 0x74) & 0x3FFFFFFFu);
}

bool allowedName(std::string_view text) {
    if (text.empty() || text.size() > PK6::NameCapacity) return false;
    bool visible = false;
    for (unsigned char c : text) {
        if (c < 0x20 || c > 0x7E) return false;
        if (c != ' ') visible = true;
    }
    return visible;
}

} // namespace

SlotRecord Gen6StagedSession::original(SlotRef slot) const {
    if (!slot.canonical()) return {};
    return slot.region == RecordRegion::Party ? source_.party(slot.slot)
                                              : source_.box(slot.box, slot.slot);
}

std::optional<PK6::Pokemon6> Gen6StagedSession::current(SlotRef slot) const {
    if (const auto it = entries_.find(slot); it != entries_.end()) return it->second.record;
    auto rec = original(slot);
    return rec.pokemon;
}

void Gen6StagedSession::commitEdit(SlotRef slot, PK6::Pokemon6 next, std::string note) {
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

bool Gen6StagedSession::stageValue(SlotRef slot, Field field, uint32_t value, size_t index,
                                   std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (!slot.canonical()) return fail("Gen VI edit target is not a canonical slot");
    if (!Gen6EditorProvider::editable(field, slot.region))
        return fail("Gen VI field is not editable for this record region");
    if (const auto it = entries_.find(slot); it != entries_.end() && it->second.kind == ChangeKind::Remove)
        return fail("Gen VI cannot edit a slot staged for removal");
    auto rec = current(slot);
    if (!rec || rec->empty()) return fail("Gen VI edit requires a valid occupied PK6");
    const auto lim = limits();
    std::string note = slotLabel(slot) + ": ";

    switch (field) {
        case Field::HeldItem:
            if (value > lim.maxHeldItem) return fail("Gen VI held item exceeds the game's item range");
            note += "held item " + std::to_string(rec->heldItem()) + " -> " + std::to_string(value);
            rec->setHeldItem(static_cast<uint16_t>(value));
            break;
        case Field::TrainerId:
        case Field::SecretId: {
            if (value > 0xFFFF) return fail("Gen VI trainer ID out of range");
            const bool tid = field == Field::TrainerId;
            note += std::string(tid ? "TID " : "SID ") + std::to_string(tid ? rec->tid() : rec->sid()) +
                    " -> " + std::to_string(value);
            tid ? rec->setTid(static_cast<uint16_t>(value)) : rec->setSid(static_cast<uint16_t>(value));
            break;
        }
        case Field::Nature:
            if (value > lim.maxNature) return fail("Gen VI nature out of range");
            note += "nature " + std::to_string(rec->nature()) + " -> " + std::to_string(value);
            rec->setNature(static_cast<uint8_t>(value));
            break;
        case Field::FatefulEncounter:
            if (value > 1) return fail("Gen VI fateful encounter is a single bit");
            note += std::string("fateful encounter -> ") + (value ? "yes" : "no");
            rec->setFatefulEncounter(value != 0);
            break;
        case Field::EV: {
            if (index >= 6 || value > lim.maxEV) return fail("Gen VI EV stat/value out of range");
            uint32_t total = value;
            for (size_t i = 0; i < 6; ++i) if (i != index) total += rec->ev(i);
            if (total > lim.maxEVTotal) return fail("Gen VI EV total would exceed 510");
            note += std::string("EV ") + StatNames[index] + " " + std::to_string(rec->ev(index)) +
                    " -> " + std::to_string(value);
            rec->setEv(index, static_cast<uint8_t>(value));
            break;
        }
        case Field::IV:
            if (index >= 6 || value > lim.maxIV) return fail("Gen VI IV stat/value out of range");
            note += std::string("IV ") + StatNames[index] + " " + std::to_string(rec->iv(index)) +
                    " -> " + std::to_string(value);
            rec->setIv(index, static_cast<uint8_t>(value));
            break;
        case Field::Level: {
            const auto exp = Stats::experienceForLevel(*rec, static_cast<uint8_t>(value > 255 ? 0 : value));
            if (value < 1 || value > 100 || !exp)
                return fail("Gen VI level must be 1-100 for a species/form with verified growth data");
            note += "level " + std::to_string(Stats::level(*rec).value_or(0)) + " -> " + std::to_string(value);
            rec->setExperience(*exp);
            break;
        }
        case Field::Pokerus:
            if (value > 0xFF) return fail("Gen VI Pokerus byte out of range");
            note += "Pokerus " + std::to_string(rec->pokerus()) + " -> " + std::to_string(value);
            rec->setPokerus(static_cast<uint8_t>(value));
            break;
        case Field::Friendship:
            if (value > 0xFF) return fail("Gen VI friendship out of range");
            // The shared Friendship field means the current trainer's value; only
            // the original-trainer byte is supported (handler friendship is not).
            if (rec->currentHandler() != 0)
                return fail("Gen VI handling-trainer friendship editing is unsupported");
            note += "friendship " + std::to_string(rec->originalTrainerFriendship()) + " -> " +
                    std::to_string(value);
            rec->setOriginalTrainerFriendship(static_cast<uint8_t>(value));
            break;
        case Field::Ball:
            if (value < 1 || value > lim.maxBall) return fail("Gen VI ball out of range");
            note += "ball " + std::to_string(rec->ball()) + " -> " + std::to_string(value);
            rec->setBall(static_cast<uint8_t>(value));
            break;
        case Field::OriginalTrainerGender:
            if (value > 1) return fail("Gen VI OT gender is a single bit");
            note += "OT gender -> " + std::to_string(value);
            rec->setOriginalTrainerGender(static_cast<uint8_t>(value));
            break;
        default:
            return fail("Gen VI field has no numeric staged editor");
    }
    if (slot.region == RecordRegion::Party) {
        const auto base = original(slot);
        if (base.pokemon && statInputsChanged(base.pokemon->plain(), rec->plain())) {
            if (!Stats::refreshParty(*rec))
                return fail("Gen VI party stats cannot be recomputed for this species/form");
            note += " (party stats recomputed)";
        } else if (base.pokemon) {
            // Inputs back at their original values: restore the original cached block.
            std::array<uint16_t, 6> stats{};
            for (size_t i = 0; i < 6; ++i) stats[i] = base.pokemon->stat(i + 1);
            rec->setPartyStats(base.pokemon->statLevel(), base.pokemon->stat(0), stats);
        }
    }
    commitEdit(slot, std::move(*rec), std::move(note));
    if (error) error->clear();
    return true;
}

bool Gen6StagedSession::stageText(SlotRef slot, Field field, std::string_view text, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (field != Field::Nickname && field != Field::OriginalTrainer)
        return fail("Gen VI text editing supports Nickname and OT name only");
    if (!slot.canonical()) return fail("Gen VI edit target is not a canonical slot");
    if (!Gen6EditorProvider::editable(field, slot.region))
        return fail("Gen VI field is not editable for this record region");
    if (const auto it = entries_.find(slot); it != entries_.end() && it->second.kind == ChangeKind::Remove)
        return fail("Gen VI cannot edit a slot staged for removal");
    if (!allowedName(text))
        return fail("Gen VI names must be 1-12 printable ASCII characters");
    auto rec = current(slot);
    if (!rec || rec->empty()) return fail("Gen VI edit requires a valid occupied PK6");
    std::u16string wide(text.begin(), text.end());
    std::string note = slotLabel(slot);
    if (field == Field::Nickname) {
        if (rec->isEgg()) return fail("Gen VI egg nicknames are unsupported");
        rec->setRawName(PK6::NicknameOffset, wide);
        rec->setNicknamedFlag(true);
        note += ": nickname -> " + std::string(text);
    } else {
        rec->setRawName(PK6::OriginalTrainerOffset, wide);
        note += ": OT name -> " + std::string(text);
    }
    commitEdit(slot, std::move(*rec), std::move(note));
    if (error) error->clear();
    return true;
}

bool Gen6StagedSession::cloneToBox(SlotRef from, size_t box, size_t slot, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    const SlotRef to = SlotRef::boxed(box, slot);
    if (!from.canonical() || !to.canonical() || from == to)
        return fail("Gen VI clone requires canonical, distinct source and destination");
    if (entries_.count(from) || entries_.count(to))
        return fail("Gen VI clone requires an unmodified source and destination");
    const auto src = original(from);
    const auto dst = original(to);
    if (!src.occupied() || !src.pokemon) return fail("Gen VI clone source is not a valid occupied PK6");
    if (!dst.blank()) return fail("Gen VI clone destination must be a verified blank box slot");
    auto boxed = src.pokemon->partyForm() ? src.pokemon->boxed() : *src.pokemon;
    // PKHeX SAV6.SetPKM silently resets these forms when boxing; refuse instead of mutating.
    if (boxed.form() != 0 && (boxed.species() == SpeciesFurfrou || boxed.species() == SpeciesHoopa))
        return fail("Gen VI boxed Furfrou/Hoopa must be in their base form");
    const auto check = PK6::Pokemon6::decode(boxed.serialize());
    if (!check || check->species() != src.pokemon->species() || check->pid() != src.pokemon->pid())
        return fail("Gen VI boxed clone failed strict PK6 verification");
    entries_.emplace(to, Entry{ChangeKind::Clone, std::move(boxed), from,
                               {slotLabel(to) + ": clone of " + slotLabel(from)}});
    if (error) error->clear();
    return true;
}

bool Gen6StagedSession::removeFromBox(size_t box, size_t slot, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    const SlotRef at = SlotRef::boxed(box, slot);
    if (!at.canonical()) return fail("Gen VI removal target is not a canonical box slot");
    if (entries_.count(at)) return fail("Gen VI removal requires an unmodified box slot");
    const auto rec = original(at);
    if (!rec.occupied()) return fail("Gen VI removal requires one valid occupied box record");
    entries_.emplace(at, Entry{ChangeKind::Remove, PK6::Pokemon6::blank(PK6::StoredSize), std::nullopt,
                               {slotLabel(at) + ": removed"}});
    if (error) error->clear();
    return true;
}

std::vector<StagedChange> Gen6StagedSession::changes() const {
    std::vector<StagedChange> out;
    for (const auto& [slot, entry] : entries_) {
        StagedChange c;
        c.slot = slot;
        c.kind = entry.kind;
        c.cloneSource = entry.cloneSource;
        const auto base = original(slot);
        if (base.pokemon) c.originalPlain.assign(base.pokemon->plain().begin(), base.pokemon->plain().end());
        c.stagedPlain.assign(entry.record.plain().begin(), entry.record.plain().end());
        PK6::write16(c.stagedPlain, 0x06, PK6::checksum(c.stagedPlain));
        c.notes = entry.notes;
        out.push_back(std::move(c));
    }
    return out;
}

std::vector<std::string> Gen6StagedSession::changeLines() const {
    std::vector<std::string> lines;
    for (const auto& [slot, entry] : entries_)
        lines.insert(lines.end(), entry.notes.begin(), entry.notes.end());
    return lines;
}

std::optional<std::vector<uint8_t>> Gen6StagedSession::buildWorkingCopy(std::string* error) const {
    const auto fail = [&](const std::string& why) -> std::optional<std::vector<uint8_t>> {
        if (error) *error = why;
        return std::nullopt;
    };
    if (entries_.empty()) return fail("Gen VI working copy needs at least one staged change");
    const auto family = source_.family();
    std::vector<uint8_t> out(source_.bytes().begin(), source_.bytes().end());
    std::set<size_t> touched;
    for (const auto& [slot, entry] : entries_) {
        const size_t offset = recordOffset(family, slot);
        const size_t size = recordSize(slot);
        if (entry.record.size() != size) return fail("Gen VI staged record geometry mismatch");
        const auto block = Gen6Save::owningBlock(family, offset, size);
        if (!block) return fail("Gen VI staged record escapes its checksum block");
        const auto encrypted = entry.record.serialize();
        std::copy(encrypted.begin(), encrypted.end(), out.begin() + static_cast<std::ptrdiff_t>(offset));
        touched.insert(*block);
    }
    for (const size_t id : touched) Gen6Save::refreshBlockChecksum(out, family, id);
    std::string why;
    if (!verifyWorkingCopy(source_, out, changes(), &why)) return fail(why);
    if (error) error->clear();
    return out;
}

Common::VerifiedExportResult Gen6StagedSession::exportWorkingCopy(const Gen6ExportRequest& request) const {
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
    // On hardware the destination is fixed to PokeBank NX's own exports tree.
    const std::string fixedRoot = PokeBank::Paths::exportsRoot() + "/gen6";
    if (r.rootDirectory.empty()) r.rootDirectory = fixedRoot;
    if (r.rootDirectory != fixedRoot) {
        result.error = "Gen VI export cannot leave app-owned exports/gen6";
        return result;
    }
#endif
    if (r.rootDirectory.empty() || request.timestamp.empty()) {
        result.error = "Gen VI export needs an app-owned root and a timestamp";
        return result;
    }
    r.directoryStem = gameId + "_" + request.timestamp;
    r.generationLabel = "Generation VI";
    r.fileExtension = ".sav";
    r.sourcePath = request.sourcePath;
    r.gameId = gameId;
    r.saveFormat = source_.family() == Layout::Family::XY ? "SAV6 X/Y main (0x65600)"
                                                           : "SAV6 Omega Ruby/Alpha Sapphire main (0x76000)";
    r.timestamp = request.timestamp;
    r.applicationSha = request.applicationSha;
    r.originalBytes = source_.bytes();
    r.editedBytes = *edited;
    r.changeLines = changeLines();
    r.failurePoint = request.failurePoint;
    const auto expected = changes();
    const Gen6Save& baseline = source_;
    r.validator = [&baseline, &expected](std::span<const uint8_t> bytes, std::string& why) {
        return verifyWorkingCopy(baseline, bytes, expected, &why);
    };
    return Common::publishVerifiedStagedExport(r);
}

bool verifyWorkingCopy(const Gen6Save& source, std::span<const uint8_t> edited,
                       const std::vector<StagedChange>& changes, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    const auto original = source.bytes();
    if (edited.size() != original.size()) return fail("Gen VI working copy size changed");
    std::string parseError;
    const auto reparsed = Gen6Save::parse(edited, source.exactGameId(), &parseError);
    if (!reparsed) return fail("Gen VI working copy failed strict reparse: " + parseError);
    if (reparsed->partyCount() != source.partyCount()) return fail("Gen VI working copy changed party count");

    const auto family = source.family();
    std::vector<uint8_t> allowed(edited.size(), 0);  // per-byte bitmask in ENCRYPTED space
    std::set<size_t> blocks;
    for (const auto& c : changes) {
        if (!c.slot.canonical()) return fail("Gen VI change has a noncanonical slot");
        const size_t offset = recordOffset(family, c.slot);
        const size_t size = recordSize(c.slot);
        const auto block = Gen6Save::owningBlock(family, offset, size);
        if (!block) return fail("Gen VI change escapes its checksum block");
        blocks.insert(*block);
        // Encrypted bytes of a re-encrypted record may all change; the semantic
        // whitelist is enforced below on the decrypted plain record.
        std::fill_n(allowed.begin() + static_cast<std::ptrdiff_t>(offset), size, uint8_t{0xFF});

        const auto rec = c.slot.region == RecordRegion::Party ? reparsed->party(c.slot.slot)
                                                              : reparsed->box(c.slot.box, c.slot.slot);
        if (!rec.pokemon) return fail("Gen VI staged record does not decode strictly");
        const auto plain = rec.pokemon->plain();
        if (!std::equal(plain.begin(), plain.end(), c.stagedPlain.begin(), c.stagedPlain.end()))
            return fail("Gen VI staged record differs from the reviewed change");
        if (c.originalPlain.size() != plain.size()) return fail("Gen VI baseline record size mismatch");
        const auto sourceRec = c.slot.region == RecordRegion::Party ? source.party(c.slot.slot)
                                                                    : source.box(c.slot.box, c.slot.slot);
        if (!sourceRec.pokemon ||
            !std::equal(c.originalPlain.begin(), c.originalPlain.end(), sourceRec.pokemon->plain().begin(),
                        sourceRec.pokemon->plain().end()))
            return fail("Gen VI baseline record differs from the immutable source");

        switch (c.kind) {
            case ChangeKind::Edit: {
                if (!sourceRec.occupied() || rec.pokemon->empty())
                    return fail("Gen VI edit must keep an occupied record");
                const auto mask = editMask(c.slot.region);
                for (size_t i = 0; i < PK6::StoredSize; ++i)
                    if ((plain[i] ^ c.originalPlain[i]) & ~mask[i])
                        return fail("Gen VI edit touched a non-whitelisted PK6 byte at 0x" +
                                    std::to_string(i));
                if (c.slot.region == RecordRegion::Party) {
                    // Cached party block: unchanged unless a stat input changed, in which
                    // case it must equal an exact recomputation from the staged inputs.
                    std::span<const uint8_t> expected(c.originalPlain);
                    std::optional<PK6::Pokemon6> refreshed;
                    if (statInputsChanged(c.originalPlain, plain)) {
                        refreshed = rec.pokemon;
                        if (!Stats::refreshParty(*refreshed))
                            return fail("Gen VI party stats cannot be recomputed for verification");
                        expected = refreshed->plain();
                    }
                    if (!std::equal(plain.begin() + static_cast<std::ptrdiff_t>(PK6::StoredSize), plain.end(),
                                    expected.begin() + static_cast<std::ptrdiff_t>(PK6::StoredSize), expected.end()))
                        return fail("Gen VI party cached stats are not the exact recomputation");
                }
                break;
            }
            case ChangeKind::Clone: {
                if (c.slot.region != RecordRegion::Box || !c.cloneSource || !sourceRec.blank())
                    return fail("Gen VI clone must target a blank box slot");
                const auto& from = *c.cloneSource;
                const auto donor = from.region == RecordRegion::Party ? source.party(from.slot)
                                                                      : source.box(from.box, from.slot);
                if (!donor.occupied() || !donor.pokemon) return fail("Gen VI clone donor is invalid");
                // A staged clone may carry later field edits, held to the boxed edit mask.
                const auto d = donor.pokemon->plain();
                const auto mask = editMask(RecordRegion::Box);
                if (plain.size() != PK6::StoredSize) return fail("Gen VI clone is not a boxed record");
                for (size_t i = 0; i < PK6::StoredSize; ++i)
                    if ((plain[i] ^ d[i]) & ~mask[i])
                        return fail("Gen VI clone differs from its immutable donor outside editable fields");
                break;
            }
            case ChangeKind::Remove:
                if (c.slot.region != RecordRegion::Box || !sourceRec.occupied() || !rec.blank())
                    return fail("Gen VI removal must blank an occupied box slot");
                break;
        }
    }
    for (const size_t id : blocks) {
        const size_t at = Layout::checksumOffset(family, id);
        allowed[at] = allowed[at + 1] = 0xFF;
    }
    for (size_t i = 0; i < edited.size(); ++i)
        if ((edited[i] ^ original[i]) & ~allowed[i])
            return fail("Gen VI working copy changed a non-whitelisted save byte at offset " +
                        std::to_string(i));
    if (error) error->clear();
    return true;
}

} // namespace PokeVault::Integration::Gen6
