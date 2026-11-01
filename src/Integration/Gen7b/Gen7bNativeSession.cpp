#include "Integration/Gen7b/Gen7bNativeSession.h"

#include "Enums/GameVersion.h"
#include "Enums/LanguageID.h"
#include "Pokemon/PokemonTypes.h"
#include "Utils/StringHelpers.h"

#include <utility>

namespace PokeVault::Integration::Gen7b {

Modern::OpenedSession Gen7bNativeSession::open(std::span<const uint8_t> bytes, std::string_view assignedExactGame) {
    Modern::OpenedSession out;
    std::string error;
    auto save = Gen7bSave::parse(bytes, assignedExactGame, &error);
    if (!save) {
        out.diagnostic = error.empty() ? "Let's Go save failed strict validation" : error;
        return out;
    }
    if (!assignedExactGame.empty() && save->exactGameId() != assignedExactGame) {
        out.diagnostic = "Validated Let's Go title differs from the assigned game; reassign explicitly";
        return out;
    }
    out.session = std::make_unique<Gen7bNativeSession>(std::move(*save));
    return out;
}

Gen7bNativeSession::Gen7bNativeSession(Gen7bSave source)
    : session_(std::move(source)), capabilities_(Capability::find(session_.source().exactGameId())) {
    if (!capabilities_) capabilities_ = Capability::find("letsgo_pikachu_switch");
}

std::optional<size_t> Gen7bNativeSession::indexOf(Modern::SlotRef slot) const noexcept {
    if (slot.region == Modern::Region::Party) {
        if (slot.slot >= Layout::PartyPointers) return std::nullopt;
        const auto p = session_.current().header.party[slot.slot];
        if (p == Layout::EmptyPointer) return std::nullopt;
        return p;
    }
    if (slot.box >= Layout::BoxCount || slot.slot >= Layout::BoxSlots) return std::nullopt;
    return slot.box * Layout::BoxSlots + slot.slot;
}

std::optional<Modern::RecordView> Gen7bNativeSession::view(Modern::SlotRef slot) const {
    const auto index = indexOf(slot);
    if (!index) return std::nullopt;
    Modern::RecordView v;
    const auto mon = session_.record(*index);
    if (!mon) return v;  // invalid (never happens after strict parse; shown as invalid if it did)
    v.valid = true;
    v.empty = mon->empty();
    if (v.empty) return v;
    const auto p = mon->plain();
    v.partyRecord = true;  // Let's Go stores party-length PB7 everywhere
    v.species = mon->species();
    v.form = static_cast<uint8_t>(p[0x1D] >> 3);
    v.gender = mon->gender();
    v.shiny = mon->shiny();
    v.egg = mon->isEgg();
    v.nicknamed = mon->isNicknamed();
    v.fatefulEncounter = mon->fatefulEncounter();
    v.experience = mon->experience();
    v.level = mon->statLevel();
    v.nickname = Utils::utf16ToUtf8(mon->nickname());
    v.otName = Utils::utf16ToUtf8(mon->originalTrainerName());
    v.otGender = mon->originalTrainerGender();
    v.tid = mon->tid();
    v.sid = mon->sid();
    v.pid = mon->pid();
    v.encryptionConstant = mon->encryptionConstant();
    v.heldItem = mon->heldItem();
    v.nature = mon->nature();
    v.statNature = mon->nature();
    v.ability = mon->ability();
    v.abilityNumber = mon->abilityNumber();
    for (size_t i = 0; i < 6; ++i) {
        v.ivs[i] = mon->iv(i);
        v.evs[i] = mon->ev(i);
        v.stats[i] = mon->stat(i + 1);   // PB7 0xF2.. cached (HP max, Atk, Def, Spe, SpA, SpD)
    }
    v.currentHp = mon->stat(0);
    for (size_t i = 0; i < 4; ++i) {
        v.moves[i] = mon->move(i);
        v.pp[i] = mon->movePP(i);
        v.ppUps[i] = mon->movePPUps(i);
        v.relearnMoves[i] = PK6::read16(p, 0x6A + 2 * i);
    }
    v.ball = mon->ball();
    v.hyperTrainFlags = static_cast<uint8_t>(mon->plain()[0xDE] & 0x3F);   // PB7.cs HyperTrainFlags 0xDE
    v.language = mon->language();
    v.friendship = mon->originalTrainerFriendship();
    v.originGame = mon->originVersion();
    v.metLevel = mon->metLevel();
    v.metLocation = mon->metLocation();
    v.eggLocation = mon->eggLocation();
    v.metDate = {static_cast<uint16_t>(p[0xD4] ? 2000 + p[0xD4] : 0), p[0xD5], p[0xD6]};
    v.eggDate = {static_cast<uint16_t>(p[0xD1] ? 2000 + p[0xD1] : 0), p[0xD2], p[0xD3]};
    const auto types = Pokemon::getPokemonTypes(v.species, v.form, Enums::GameVersion::GG);
    v.types = {static_cast<uint8_t>(types.type1), static_cast<uint8_t>(types.type2)};
    // Let's Go native mechanics, read-only (PKSM-Core PB7.cpp offsets).
    static constexpr const char* AvNames[6] = {"HP", "Atk", "Def", "Spe", "SpA", "SpD"};
    std::string avs;
    for (size_t i = 0; i < 6; ++i)
        avs += std::string(i ? " " : "") + AvNames[i] + " " + std::to_string(p[Layout::AwakeningOffset + i]);
    v.nativeFacts.push_back({"Awakening Values", avs});
    v.nativeFacts.push_back({"Combat Power", std::to_string(PK6::read16(p, Layout::CpOffset))});
    v.nativeFacts.push_back({"Height / Weight", std::to_string(p[0x3A]) + " / " + std::to_string(p[0x3B])});
    const auto& h = session_.current().header;
    v.nativeFacts.push_back({"Partner Pokemon", h.isPartner(*index) ? "Yes" : "No"});
    v.nativeFacts.push_back({"In party", h.inParty(*index) ? "Yes" : "No"});
    v.nativeFacts.push_back({"Storage index", std::to_string(*index + 1) + " of " + std::to_string(h.count)});
    v.nativeFacts.push_back({"Encryption constant", std::to_string(mon->encryptionConstant())});
    return v;
}

bool Gen7bNativeSession::originalOccupied(Modern::SlotRef slot) const {
    if (slot.region == Modern::Region::Party) {
        if (slot.slot >= Layout::PartyPointers) return false;
        return session_.source().model().header.party[slot.slot] != Layout::EmptyPointer;
    }
    const auto index = indexOf(slot);
    return index && *index < session_.source().model().header.count;
}

bool Gen7bNativeSession::emptyDestination(Modern::SlotRef slot) const {
    if (slot.region != Modern::Region::Box) return false;
    const auto index = indexOf(slot);
    const auto count = session_.current().header.count;
    return index && *index == count && count < Layout::Slots;
}

bool Gen7bNativeSession::stagedAt(Modern::SlotRef slot) const {
    const auto index = indexOf(slot);
    return index && session_.changedAt(*index);
}

std::optional<Modern::SlotRef> Gen7bNativeSession::firstEmptyBoxSlot() const {
    const auto count = session_.current().header.count;
    if (count >= Layout::Slots) return std::nullopt;
    return boxSlotFor(count);
}

bool Gen7bNativeSession::stageValue(Modern::SlotRef slot, Modern::Field field, uint32_t value, size_t, std::string& error) {
    const auto index = indexOf(slot);
    if (!index) {
        error = "Let's Go edit target is not a storage record";
        return false;
    }
    return session_.stageValue(*index, field, value, &error);
}

bool Gen7bNativeSession::stageText(Modern::SlotRef slot, Modern::Field field, std::string_view utf8, std::string& error) {
    const auto index = indexOf(slot);
    if (!index) {
        error = "Let's Go edit target is not a storage record";
        return false;
    }
    return session_.stageText(*index, field, utf8, &error);
}

uint32_t Gen7bNativeSession::fieldMaximum(Modern::Field field) const noexcept {
    switch (field) {
        case Modern::Field::Ball: return 12;
        case Modern::Field::TrainerId:
        case Modern::Field::SecretId: return 0xFFFF;
        case Modern::Field::Friendship: return 0xFF;
        case Modern::Field::AV: return 200;
        case Modern::Field::IV: return 31;
        case Modern::Field::Level: return 100;
        default: return 0;
    }
}

bool Gen7bNativeSession::beginDraft(Modern::SlotRef slot, std::string& error) {
    const auto index = indexOf(slot);
    const auto mon = index ? session_.record(*index) : std::nullopt;
    if (!mon || mon->empty()) {
        error = "Let's Go draft requires a valid occupied record";
        return false;
    }
    if (draft_) discardDraft();
    draftMark_ = session_.mark();
    draft_ = slot;
    error.clear();
    return true;
}

void Gen7bNativeSession::discardDraft() {
    if (!draft_) return;
    session_.truncate(draftMark_);
    draft_.reset();
}

Modern::OperationSupport Gen7bNativeSession::cloneSupport(Modern::SlotRef from) const {
    const auto index = indexOf(from);
    if (!index) return {false, "Clone needs an occupied record"};
    if (const auto why = session_.cloneBlocked(*index); !why.empty()) return {false, why};
    return {true, {}};
}

Modern::OperationSupport Gen7bNativeSession::removeSupport(Modern::SlotRef at) const {
    const auto index = indexOf(at);
    if (!index) return {false, "Release needs an occupied record"};
    if (at.region == Modern::Region::Party)
        return {false, "This Pokemon is in your party; release is offered for box Pokemon only"};
    if (const auto why = session_.removeBlocked(*index); !why.empty()) return {false, why};
    return {true, {}};
}

bool Gen7bNativeSession::clone(Modern::SlotRef from, Modern::SlotRef to, std::string& error) {
    if (draft_) {
        error = "Let's Go has an open editor draft; keep or discard it before Clone";
        return false;
    }
    const auto next = firstEmptyBoxSlot();
    if (!next || to != *next) {
        error = next ? "Let's Go storage is gapless; a clone is appended at Box " + std::to_string(next->box + 1) +
                           " / Slot " + std::to_string(next->slot + 1)
                     : std::string("Let's Go storage is full (1000 Pokemon)");
        return false;
    }
    const auto index = indexOf(from);
    if (!index) {
        error = "Clone needs an occupied record";
        return false;
    }
    return session_.cloneAppend(*index, &error);
}

bool Gen7bNativeSession::remove(Modern::SlotRef at, std::string& error) {
    if (draft_) {
        error = "Let's Go has an open editor draft; keep or discard it before Release";
        return false;
    }
    if (const auto support = removeSupport(at); !support.available) {
        error = std::string(support.reason);
        return false;
    }
    return session_.remove(*indexOf(at), &error);
}

bool Gen7bNativeSession::undo(Modern::SlotRef at) {
    if (draft_) return false;  // the open draft owns the log tail until keep/discard
    const auto index = indexOf(at);
    return index && session_.undoLastAt(*index);
}

std::vector<Modern::TrainerFact> Gen7bNativeSession::trainerFacts() const {
    const auto t = session_.source().trainer();
    const uint32_t id32 = (static_cast<uint32_t>(t.sid) << 16) | t.tid;
    const auto& h = session_.current().header;
    std::vector<Modern::TrainerFact> facts;
    facts.push_back({"Name", trainerName()});
    facts.push_back({"Game", std::string(capabilities_->title)});
    facts.push_back({"Trainer ID", std::to_string(id32 % 1000000u)});
    facts.push_back({"Secret ID", std::to_string(id32 / 1000000u)});
    facts.push_back({"TID16 / SID16", std::to_string(t.tid) + " / " + std::to_string(t.sid)});
    facts.push_back({"Gender", t.gender == 0 ? "Male" : t.gender == 1 ? "Female" : "Unknown"});
    facts.push_back({"Language", Enums::getLanguageName(t.language)});
    facts.push_back({"Pokemon stored", std::to_string(h.count) + " / 1000"});
    facts.push_back({"Partner", h.starter == Layout::EmptyPointer ? "None" : "Box " + std::to_string(h.starter / Layout::BoxSlots + 1) +
                                                                           " / Slot " + std::to_string(h.starter % Layout::BoxSlots + 1)});
    return facts;
}

std::string Gen7bNativeSession::trainerName() const {
    const auto name = Utils::utf16ToUtf8(session_.source().trainer().name);
    return name.empty() ? std::string("Unknown") : name;
}

uint8_t Gen7bNativeSession::trainerGender() const noexcept {
    const auto g = session_.source().bytes()[Layout::Gender];
    return g <= 1 ? g : 0;
}

std::vector<Modern::ReviewRow> Gen7bNativeSession::reviewRows() const {
    std::vector<Modern::ReviewRow> rows;
    for (const auto& op : session_.ops()) {
        const auto colon = op.note.find(": ");
        if (colon == std::string::npos) rows.push_back({"Storage", op.note});
        else rows.push_back({op.note.substr(0, colon), op.note.substr(colon + 2)});
    }
    return rows;
}

Modern::OperationSupport Gen7bNativeSession::exportSupport() const {
    if (!session_.hasChanges()) return {false, "Export Copy needs at least one staged change"};
    return {true, {}};
}

Common::VerifiedExportResult Gen7bNativeSession::exportWorkingCopy(const Modern::ExportRequest& request) const {
    Gen7bExportRequest r;
    r.rootDirectory = request.rootDirectory;
    r.sourcePath = request.sourcePath;
    r.timestamp = request.timestamp;
    r.applicationSha = request.applicationSha;
    return session_.exportWorkingCopy(r);
}

} // namespace PokeVault::Integration::Gen7b
