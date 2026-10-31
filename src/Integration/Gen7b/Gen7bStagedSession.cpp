#include "Integration/Gen7b/Gen7bStagedSession.h"

#ifdef __SWITCH__
#include "Utils/PokeBankPaths.h"
#endif

#include <algorithm>

namespace PokeVault::Integration::Gen7b {
namespace {

std::string slotLabel(size_t index) {
    return "Box " + std::to_string(index / Layout::BoxSlots + 1) + " / Slot " + std::to_string(index % Layout::BoxSlots + 1);
}

// Per-byte whitelist inside a decrypted PB7, derived from fieldState(). Offsets: PKSM-Core PB7.cpp.
std::array<uint8_t, Layout::RecordSize> editMask() {
    std::array<uint8_t, Layout::RecordSize> m{};
    const auto range = [&](size_t from, size_t to) { for (size_t i = from; i < to; ++i) m[i] = 0xFF; };
    m[0x06] = m[0x07] = 0xFF;  // checksum
    if (editable(Field::TrainerId)) range(0x0C, 0x0E);
    if (editable(Field::SecretId)) range(0x0E, 0x10);
    if (editable(Field::FatefulEncounter)) m[0x1D] |= 0x01;
    if (editable(Field::Nickname)) { range(0x40, 0x5A); m[0x77] |= 0x80; }
    if (editable(Field::OriginalTrainer)) range(0xB0, 0xCA);
    if (editable(Field::Ball)) m[0xDC] = 0xFF;
    if (editable(Field::OriginalTrainerGender)) m[0xDD] |= 0x80;
    return m;
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

RawRecord toRaw(const std::vector<uint8_t>& encrypted) {
    RawRecord r{};
    std::copy_n(encrypted.begin(), std::min(encrypted.size(), r.size()), r.begin());
    return r;
}

struct Identity {
    uint32_t ec = 0, pid = 0;
    bool operator==(const Identity&) const = default;
};
std::optional<Identity> identityAt(const StorageModel& m, uint16_t pointer) {
    if (pointer == Layout::EmptyPointer || pointer >= m.header.count) return std::nullopt;
    const auto mon = Gen7bSave::decode(m.records[pointer]);
    if (!mon) return std::nullopt;
    return Identity{mon->encryptionConstant(), mon->pid()};
}

} // namespace

std::optional<StorageModel> Gen7bStagedSession::replay(const StorageModel& base, const std::vector<Op>& ops,
                                                       std::string* error) {
    const auto fail = [&](const std::string& why) -> std::optional<StorageModel> {
        if (error) *error = why;
        return std::nullopt;
    };
    StorageModel m = base;
    for (const auto& op : ops) {
        auto& h = m.header;
        switch (op.kind) {
            case OpKind::Edit: {
                if (op.index >= h.count) return fail("Let's Go edit targets an empty record");
                if (op.plain.size() != Layout::RecordSize) return fail("Let's Go edit record has the wrong size");
                auto plain = op.plain;
                PK6::write16(plain, 0x06, PK6::checksum(plain));
                m.records[op.index] = toRaw(PK6::encrypt(plain));
                break;
            }
            case OpKind::Clone: {
                if (op.index >= h.count) return fail("Let's Go clone donor is empty");
                if (h.count >= Layout::Slots) return fail("Let's Go storage is full (1000 Pokemon)");
                m.records[h.count] = m.records[op.index];
                ++h.count;
                break;
            }
            case OpKind::Remove: {
                const size_t k = op.index;
                if (k >= h.count) return fail("Let's Go release targets an empty record");
                if (h.inParty(k)) return fail("Let's Go party members cannot be released here");
                if (h.isPartner(k)) return fail("Let's Go partner Pokemon can never be released");
                // Shift k+1..count-1 down; the vacated last index takes the blank that followed the
                // list (or the encrypted zero record when storage was full), never the released bytes.
                const RawRecord blank = h.count < Layout::Slots ? m.records[h.count]
                                                                : toRaw(PK6::encrypt(std::vector<uint8_t>(Layout::RecordSize, 0)));
                for (size_t i = k; i + 1 < h.count; ++i) m.records[i] = m.records[i + 1];
                m.records[h.count - 1] = blank;
                for (auto& p : h.party)
                    if (p != Layout::EmptyPointer && p > k) --p;
                if (h.starter != Layout::EmptyPointer && h.starter > k) --h.starter;
                --h.count;
                break;
            }
        }
    }
    if (error) error->clear();
    return m;
}

std::optional<PK6::Pokemon6> Gen7bStagedSession::record(size_t index) const {
    if (index >= Layout::Slots) return std::nullopt;
    if (index >= current_.header.count) return PK6::Pokemon6::blank(Layout::RecordSize);
    return Gen7bSave::decode(current_.records[index]);
}

bool Gen7bStagedSession::changedAt(size_t index) const noexcept {
    if (index >= Layout::Slots) return false;
    return current_.records[index] != source_.model().records[index];
}

bool Gen7bStagedSession::apply(Op op, std::string* error) {
    std::vector<Op> one{op};
    auto next = replay(current_, one, error);
    if (!next) return false;
    std::string why;
    if (!Gen7bSave::validateModel(*next, &why)) {
        if (error) *error = "Let's Go staged storage would be invalid: " + why;
        return false;
    }
    current_ = std::move(*next);
    ops_.push_back(std::move(op));
    if (error) error->clear();
    return true;
}

std::string_view Gen7bStagedSession::cloneBlocked(size_t from) const noexcept {
    if (from >= current_.header.count) return "Clone needs an occupied record";
    if (current_.header.count >= Layout::Slots) return "Let's Go storage is full (1000 Pokemon)";
    return {};
}

std::string_view Gen7bStagedSession::removeBlocked(size_t at) const noexcept {
    const auto& h = current_.header;
    if (at >= h.count) return "Release needs an occupied record";
    if (h.isPartner(at)) return "Your partner Pokemon can never be released";
    if (h.inParty(at)) return "This Pokemon is in your party; release is offered for box Pokemon only";
    return {};
}

bool Gen7bStagedSession::stageValue(size_t index, Field field, uint32_t value, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (!editable(field)) return fail("Let's Go field is not editable in this build");
    auto rec = record(index);
    if (!rec || rec->empty()) return fail("Let's Go edit requires a valid occupied PB7");
    std::string note = slotLabel(index) + ": ";
    switch (field) {
        case Field::TrainerId:
        case Field::SecretId: {
            if (value > 0xFFFF) return fail("Let's Go trainer ID half out of range");
            const bool tid = field == Field::TrainerId;
            note += std::string(tid ? "TID16 " : "SID16 ") + std::to_string(tid ? rec->tid() : rec->sid()) + " -> " +
                    std::to_string(value);
            tid ? rec->setTid(static_cast<uint16_t>(value)) : rec->setSid(static_cast<uint16_t>(value));
            break;
        }
        case Field::FatefulEncounter:
            if (value > 1) return fail("Let's Go fateful encounter is a single bit");
            note += std::string("fateful encounter -> ") + (value ? "yes" : "no");
            rec->setFatefulEncounter(value != 0);
            break;
        case Field::Ball:
            if (!validBall(value)) return fail("Let's Go balls are Poke, Great, Ultra, Master and Premier");
            note += "ball " + std::to_string(rec->ball()) + " -> " + std::to_string(value);
            rec->setBall(static_cast<uint8_t>(value));
            break;
        case Field::OriginalTrainerGender:
            if (value > 1) return fail("Let's Go OT gender is a single bit");
            note += "OT gender -> " + std::to_string(value);
            rec->setOriginalTrainerGender(static_cast<uint8_t>(value));
            break;
        default:
            return fail("Let's Go field has no numeric staged editor");
    }
    const auto plain = rec->plain();
    return apply({OpKind::Edit, index, std::vector<uint8_t>(plain.begin(), plain.end()), std::move(note)}, error);
}

bool Gen7bStagedSession::stageText(size_t index, Field field, std::string_view text, std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    if (field != Field::Nickname && field != Field::OriginalTrainer)
        return fail("Let's Go text editing supports Nickname and OT name only");
    if (!allowedName(text)) return fail("Let's Go names must be 1-12 printable ASCII characters");
    auto rec = record(index);
    if (!rec || rec->empty()) return fail("Let's Go edit requires a valid occupied PB7");
    const std::u16string wide(text.begin(), text.end());
    std::string note = slotLabel(index);
    if (field == Field::Nickname) {
        if (rec->isEgg()) return fail("Let's Go egg nicknames are unsupported");
        rec->setRawName(PK6::NicknameOffset, wide);
        rec->setNicknamedFlag(true);
        note += ": nickname -> " + std::string(text);
    } else {
        rec->setRawName(PK6::OriginalTrainerOffset, wide);
        note += ": OT name -> " + std::string(text);
    }
    const auto plain = rec->plain();
    return apply({OpKind::Edit, index, std::vector<uint8_t>(plain.begin(), plain.end()), std::move(note)}, error);
}

bool Gen7bStagedSession::cloneAppend(size_t from, std::string* error) {
    if (const auto why = cloneBlocked(from); !why.empty()) {
        if (error) *error = std::string(why);
        return false;
    }
    const size_t to = current_.header.count;
    return apply({OpKind::Clone, from, {}, slotLabel(to) + ": clone of " + slotLabel(from)}, error);
}

bool Gen7bStagedSession::remove(size_t at, std::string* error) {
    if (const auto why = removeBlocked(at); !why.empty()) {
        if (error) *error = std::string(why);
        return false;
    }
    return apply({OpKind::Remove, at, {}, slotLabel(at) + ": released (later Pokemon move up one slot)"}, error);
}

bool Gen7bStagedSession::undoLastAt(size_t index) {
    if (ops_.empty()) return false;
    const auto& last = ops_.back();
    const size_t touched = last.kind == OpKind::Clone ? static_cast<size_t>(current_.header.count - 1) : last.index;
    if (touched != index) return false;
    truncate(ops_.size() - 1);
    return true;
}

void Gen7bStagedSession::truncate(size_t mark) {
    if (mark >= ops_.size()) return;
    ops_.resize(mark);
    current_ = *replay(source_.model(), ops_);  // ops were valid when applied; the prefix stays valid
}

std::vector<std::string> Gen7bStagedSession::changeLines() const {
    std::vector<std::string> lines;
    for (const auto& op : ops_) lines.push_back(op.note);
    return lines;
}

namespace {
void writeModel(std::vector<uint8_t>& out, const StorageModel& m) {
    for (size_t i = 0; i < Layout::PartyPointers; ++i) PK6::write16(out, Layout::Header + 2 * i, m.header.party[i]);
    PK6::write16(out, Layout::Header + Layout::StarterPointer, m.header.starter);
    PK6::write16(out, Layout::Header + Layout::CountField, m.header.count);
    for (size_t i = 0; i < Layout::Slots; ++i)
        std::copy(m.records[i].begin(), m.records[i].end(), out.begin() + static_cast<std::ptrdiff_t>(Layout::recordOffset(i)));
    for (const auto& b : Save::LGPEReadValidation::CONSUMED_BLOCKS) {
        if (b.key != Layout::HeaderKey && b.key != Layout::StorageKey) continue;
        const uint16_t crc = Save::LGPEReadValidation::crc16Arc(std::span<const uint8_t>(out).subspan(b.offset, b.size));
        PK6::write16(out, Layout::checksumOffset(b.key), crc);
    }
}
} // namespace

std::optional<std::vector<uint8_t>> Gen7bStagedSession::buildWorkingCopy(std::string* error) const {
    if (!hasChanges()) {
        if (error) *error = "Let's Go working copy needs at least one staged change";
        return std::nullopt;
    }
    std::vector<uint8_t> out(source_.bytes().begin(), source_.bytes().end());
    writeModel(out, current_);
    std::string why;
    if (!verifyWorkingCopy(source_, out, ops_, &why)) {
        if (error) *error = why;
        return std::nullopt;
    }
    if (error) error->clear();
    return out;
}

Common::VerifiedExportResult Gen7bStagedSession::exportWorkingCopy(const Gen7bExportRequest& request) const {
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
    const std::string fixedRoot = PokeBank::Paths::exportsRoot() + "/lgpe";
    if (r.rootDirectory.empty()) r.rootDirectory = fixedRoot;
    if (r.rootDirectory != fixedRoot) {
        result.error = "Let's Go export cannot leave app-owned exports/lgpe";
        return result;
    }
#endif
    if (r.rootDirectory.empty() || request.timestamp.empty()) {
        result.error = "Let's Go export needs an app-owned root and a timestamp";
        return result;
    }
    r.directoryStem = gameId + "_" + request.timestamp;
    r.generationLabel = "Generation VII";
    r.fileExtension = ".bin";
    r.sourcePath = request.sourcePath;
    r.gameId = gameId;
    r.saveFormat = "Let's Go main (SAV7b, " + std::to_string(source_.bytes().size()) + " bytes)";
    r.timestamp = request.timestamp;
    r.applicationSha = request.applicationSha;
    r.originalBytes = source_.bytes();
    r.editedBytes = *edited;
    r.changeLines = changeLines();
    r.failurePoint = request.failurePoint;
    const Gen7bSave& baseline = source_;
    const auto ops = ops_;
    r.validator = [&baseline, ops](std::span<const uint8_t> bytes, std::string& why) {
        return verifyWorkingCopy(baseline, bytes, ops, &why);
    };
    return Common::publishVerifiedStagedExport(r);
}

bool verifyWorkingCopy(const Gen7bSave& source, std::span<const uint8_t> edited, const std::vector<Op>& ops,
                       std::string* error) {
    const auto fail = [&](const std::string& why) { if (error) *error = why; return false; };
    const auto original = source.bytes();
    if (edited.size() != original.size()) return fail("Let's Go working copy size changed");
    std::string why;
    const auto reparsed = Gen7bSave::parse(edited, source.exactGameId(), &why);
    if (!reparsed) return fail("Let's Go working copy failed strict reparse: " + why);

    // Bytes outside the PokeListHeader block, the storage block and their two checksum words.
    std::vector<uint8_t> allowed(edited.size(), 0);
    std::fill_n(allowed.begin() + static_cast<std::ptrdiff_t>(Layout::Header), Layout::HeaderBlockSize, uint8_t{1});
    std::fill_n(allowed.begin() + static_cast<std::ptrdiff_t>(Layout::Storage), Layout::Slots * Layout::RecordSize, uint8_t{1});
    for (const uint32_t key : {Layout::HeaderKey, Layout::StorageKey}) {
        allowed[Layout::checksumOffset(key)] = 1;
        allowed[Layout::checksumOffset(key) + 1] = 1;
    }
    for (size_t i = 0; i < edited.size(); ++i)
        if (edited[i] != original[i] && !allowed[i])
            return fail("Let's Go working copy changed a byte outside storage at offset " + std::to_string(i));
    // Header bytes past the 16 known ones stay identical.
    for (size_t i = Layout::Header + 0x10; i < Layout::Header + Layout::HeaderBlockSize; ++i)
        if (edited[i] != original[i]) return fail("Let's Go working copy changed reserved PokeListHeader bytes");

    // Independent semantic replay: every op re-checked against the model it applied to.
    StorageModel m = source.model();
    const auto mask = editMask();
    for (const auto& op : ops) {
        if (op.kind == OpKind::Edit) {
            if (op.index >= m.header.count) return fail("Let's Go edit targets an empty record");
            const auto before = Gen7bSave::decode(m.records[op.index]);
            if (!before || op.plain.size() != Layout::RecordSize) return fail("Let's Go edit baseline is invalid");
            auto after = op.plain;
            PK6::write16(after, 0x06, PK6::checksum(after));
            const auto b = before->plain();
            for (size_t i = 0; i < Layout::RecordSize; ++i)
                if ((after[i] ^ b[i]) & ~mask[i])
                    return fail("Let's Go edit touched a non-whitelisted PB7 byte at 0x" + std::to_string(i));
        } else if (op.kind == OpKind::Remove) {
            if (m.header.inParty(op.index) || m.header.isPartner(op.index))
                return fail("Let's Go release of a party member or the partner");
        }
        auto next = Gen7bStagedSession::replay(m, {op}, &why);
        if (!next) return fail(why);
        // Party members and partner stay the same Pokemon through any compaction.
        for (size_t i = 0; i < Layout::PartyPointers; ++i)
            if (identityAt(m, m.header.party[i]) != identityAt(*next, next->header.party[i]))
                return fail("Let's Go party pointer no longer names the same Pokemon");
        if (identityAt(m, m.header.starter) != identityAt(*next, next->header.starter))
            return fail("Let's Go partner pointer no longer names the same Pokemon");
        m = std::move(*next);
    }
    if (!(reparsed->model() == m)) return fail("Let's Go working copy differs from the reviewed operations");
    if (error) error->clear();
    return true;
}

} // namespace PokeVault::Integration::Gen7b
