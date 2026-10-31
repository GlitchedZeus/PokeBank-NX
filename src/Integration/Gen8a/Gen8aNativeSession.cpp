#include "Integration/Gen8a/Gen8aNativeSession.h"

#include "Enums/Ball.h"
#include "Enums/GameVersion.h"
#include "Names/ItemPresence.h"
#include "Pokemon/PokemonTypes.h"
#include "Utils/StringHelpers.h"

namespace PokeVault::Integration::Gen8a {
namespace {
std::string label(Modern::SlotRef s) {
    if (s.region == Modern::Region::Party) return "Party " + std::to_string(s.slot + 1);
    return "Box " + std::to_string(s.box + 1) + " / Slot " + std::to_string(s.slot + 1);
}
} // namespace

Modern::OpenedSession Gen8aNativeSession::open(std::span<const uint8_t> bytes, std::string_view assigned) {
    Modern::OpenedSession out;
    std::string error;
    auto save = Gen8aSave::parse(bytes, assigned, &error);
    if (!save) {
        out.diagnostic = error.empty() ? "Legends: Arceus save failed strict validation" : error;
        return out;
    }
    out.session = std::make_unique<Gen8aNativeSession>(std::move(*save));
    return out;
}

Gen8aNativeSession::Gen8aNativeSession(Gen8aSave source)
    : session_(std::move(source)), capabilities_(Capability::find("legends_arceus_switch")) {}

std::optional<Modern::RecordView> Gen8aNativeSession::view(Modern::SlotRef slot) const {
    const auto n = toNative(slot);
    if (!n.canonical()) return std::nullopt;
    Modern::RecordView v;
    const auto mon = session_.current(n);
    if (!mon) return v;
    v.valid = true;
    v.empty = mon->empty();
    if (v.empty) return v;
    v.partyRecord = n.region == Region::Party;
    v.species = mon->species();
    v.form = mon->form();
    v.gender = mon->gender();
    v.shiny = mon->shiny();
    v.egg = mon->isEgg();
    v.nicknamed = mon->isNicknamed();
    v.fatefulEncounter = mon->fatefulEncounter();
    v.experience = mon->experience();
    v.level = mon->statLevel();
    v.nickname = Utils::utf16ToUtf8(mon->rawName(PA8::NicknameOffset));
    v.otName = Utils::utf16ToUtf8(mon->rawName(PA8::OriginalTrainerOffset));
    v.otGender = mon->originalTrainerGender();
    v.tid = mon->tid();
    v.sid = mon->sid();
    v.pid = mon->pid();
    v.encryptionConstant = mon->encryptionConstant();
    v.heldItem = mon->heldItem();
    v.nature = mon->nature();
    v.statNature = mon->statNature();
    v.ability = mon->ability();
    v.abilityNumber = mon->abilityNumber();
    for (size_t i = 0; i < 6; ++i) {
        v.ivs[i] = mon->iv(i);
        v.stats[i] = mon->stat(i);
    }
    v.currentHp = mon->currentHp();
    for (size_t i = 0; i < 4; ++i) v.moves[i] = mon->move(i);
    v.ball = mon->ball();
    v.language = mon->language();
    v.friendship = mon->originalTrainerFriendship();
    v.originGame = mon->originVersion();
    v.metLevel = mon->metLevel();
    const auto types = Pokemon::getPokemonTypes(v.species, v.form, Enums::GameVersion::PLA);
    v.types = {static_cast<uint8_t>(types.type1), static_cast<uint8_t>(types.type2)};
    static constexpr const char* Names6[6] = {"HP", "Atk", "Def", "Spe", "SpA", "SpD"};
    std::string gv;
    for (size_t i = 0; i < 6; ++i) gv += std::string(i ? " " : "") + Names6[i] + " " + std::to_string(mon->effortLevel(i));
    v.nativeFacts.push_back({"Effort Levels", gv});
    v.nativeFacts.push_back({"Alpha", mon->alpha() ? "Yes" : "No"});
    v.nativeFacts.push_back({"Noble", mon->noble() ? "Yes" : "No"});
    v.nativeFacts.push_back({"Alpha move", std::to_string(mon->alphaMove())});
    v.nativeFacts.push_back({"Height / Weight scalar", std::to_string(mon->heightScalar()) + " / " + std::to_string(mon->weightScalar())});
    v.nativeFacts.push_back({"Encryption constant", std::to_string(mon->encryptionConstant())});
    return v;
}

std::optional<Modern::SlotRef> Gen8aNativeSession::firstEmptyBoxSlot() const {
    for (size_t b = 0; b < Layout::BoxCount; ++b)
        for (size_t s = 0; s < Layout::BoxSlots; ++s)
            if (const auto at = Modern::SlotRef::boxed(b, s); emptyDestination(at)) return at;
    return std::nullopt;
}

uint32_t Gen8aNativeSession::fieldMaximum(Modern::Field field) const noexcept {
    switch (field) {
        case Modern::Field::HeldItem: return 0xFFFF;
        case Modern::Field::Ball: return 37;
        case Modern::Field::TrainerId:
        case Modern::Field::SecretId: return 0xFFFF;
        case Modern::Field::Friendship: return 0xFF;
        case Modern::Field::EffortLevel: return 10;
        default: return 0;
    }
}

std::vector<uint16_t> Gen8aNativeSession::heldItemChoices() const {
    std::vector<uint16_t> ids{0};
    for (uint16_t id = 1; id <= Names::ITEM_PRESENCE_MAX_ID; ++id)
        if (Names::isHeldItemPresent(id, Enums::GameVersion::PLA)) ids.push_back(id);
    return ids;
}

std::vector<uint16_t> Gen8aNativeSession::ballChoices() const {
    const auto list = Enums::getBallList(Enums::GameVersion::PLA);
    return {list.begin(), list.end()};
}

bool Gen8aNativeSession::beginDraft(Modern::SlotRef slot, std::string& error) {
    const auto n = toNative(slot);
    const auto mon = n.canonical() ? session_.current(n) : std::nullopt;
    if (!mon || mon->empty()) {
        error = "Legends: Arceus draft requires a valid occupied record";
        return false;
    }
    if (draft_) discardDraft();
    draftSnapshot_ = session_.snapshotEntry(n);
    draft_ = slot;
    error.clear();
    return true;
}

void Gen8aNativeSession::discardDraft() {
    if (!draft_) return;
    session_.restoreEntry(toNative(*draft_), draftSnapshot_);
    draft_.reset();
}

Modern::OperationSupport Gen8aNativeSession::cloneSupport(Modern::SlotRef from) const {
    if (from.region != Modern::Region::Box)
        return {false, "Legends: Arceus Clone/Move copy box records only (party conversion is not verified)"};
    if (!originalOccupied(from) || stagedAt(from)) return {false, "Clone needs an unmodified, occupied source record"};
    if (!firstEmptyBoxSlot()) return {false, "No verified blank box slot is available"};
    return {true, {}};
}

Modern::OperationSupport Gen8aNativeSession::removeSupport(Modern::SlotRef at) const {
    if (at.region != Modern::Region::Box)
        return {false, "Party removal is unsupported for Legends: Arceus (party count/compaction not verified)"};
    if (!originalOccupied(at) || stagedAt(at)) return {false, "Remove needs an unmodified, occupied box record"};
    return {true, {}};
}

bool Gen8aNativeSession::clone(Modern::SlotRef from, Modern::SlotRef to, std::string& error) {
    if (draft_ && (*draft_ == from || *draft_ == to)) {
        error = "Legends: Arceus slot has an open editor draft; keep or discard it before Clone";
        return false;
    }
    return session_.cloneToBox(toNative(from), toNative(to), &error);
}

bool Gen8aNativeSession::move(Modern::SlotRef from, Modern::SlotRef to, std::string& error) {
    if (draft_ && (*draft_ == from || *draft_ == to)) {
        error = "Legends: Arceus slot has an open editor draft; keep or discard it before Move";
        return false;
    }
    if (const auto s = moveSupport(from); !s.available) {
        error = std::string(s.reason);
        return false;
    }
    if (!session_.cloneToBox(toNative(from), toNative(to), &error)) return false;
    if (!session_.removeFromBox(toNative(from), &error)) {
        (void)session_.undo(toNative(to));
        return false;
    }
    return true;
}

bool Gen8aNativeSession::remove(Modern::SlotRef at, std::string& error) {
    if (draft_ && *draft_ == at) {
        error = "Legends: Arceus slot has an open editor draft; keep or discard it before Remove";
        return false;
    }
    return session_.removeFromBox(toNative(at), &error);
}

std::vector<Modern::TrainerFact> Gen8aNativeSession::trainerFacts() const {
    const auto& d = session_.source().myStatus().data;
    const uint16_t tid = PA8::read16(d, Layout::MyTid), sid = PA8::read16(d, Layout::MySid);
    const uint32_t id32 = (static_cast<uint32_t>(sid) << 16) | tid;
    return {{"Name", trainerName()},
            {"Game", "Legends Arceus"},
            {"Trainer ID", std::to_string(id32 % 1000000u)},
            {"Secret ID", std::to_string(id32 / 1000000u)},
            {"TID16 / SID16", std::to_string(tid) + " / " + std::to_string(sid)},
            {"Gender", trainerGender() == 0 ? "Male" : "Female"}};
}

std::string Gen8aNativeSession::trainerName() const {
    const auto& d = session_.source().myStatus().data;
    std::u16string name;
    for (size_t i = 0; i + 1 < Layout::MyNameBytes; i += 2) {
        const uint16_t ch = PA8::read16(d, Layout::MyName + i);
        if (ch == 0) break;
        name.push_back(static_cast<char16_t>(ch));
    }
    const auto out = Utils::utf16ToUtf8(name);
    return out.empty() ? std::string("Unknown") : out;
}

uint8_t Gen8aNativeSession::trainerGender() const noexcept {
    const auto g = session_.source().myStatus().data[Layout::MyGender];
    return g <= 1 ? g : 0;
}

std::vector<Modern::ReviewRow> Gen8aNativeSession::reviewRows() const {
    std::vector<Modern::ReviewRow> rows;
    for (const auto& c : session_.changes()) {
        const Modern::SlotRef at = c.slot.region == Region::Party ? Modern::SlotRef::party(c.slot.slot)
                                                                  : Modern::SlotRef::boxed(c.slot.box, c.slot.slot);
        if (c.kind == ChangeKind::Clone && c.cloneSource)
            rows.push_back({label(at), "Clone of " + label(Modern::SlotRef::boxed(c.cloneSource->box, c.cloneSource->slot))});
        else if (c.kind == ChangeKind::Remove)
            rows.push_back({label(at), "Removed (staged copy only; source unchanged)"});
        else
            for (const auto& note : c.notes) {
                const auto colon = note.find(": ");
                rows.push_back({label(at), colon == std::string::npos ? note : note.substr(colon + 2)});
            }
    }
    return rows;
}

Common::VerifiedExportResult Gen8aNativeSession::exportWorkingCopy(const Modern::ExportRequest& request) const {
    return session_.exportWorkingCopy({request.rootDirectory, request.sourcePath, request.timestamp, request.applicationSha});
}

} // namespace PokeVault::Integration::Gen8a
