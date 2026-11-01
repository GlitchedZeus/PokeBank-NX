#include "Integration/Gen8b/Gen8bNativeSession.h"

#include "Enums/GameVersion.h"
#include "Pokemon/PokemonTypes.h"
#include "Utils/StringHelpers.h"

#include <utility>

namespace PokeVault::Integration::Gen8b {
namespace {

std::string slotLabel(Modern::SlotRef s) {
    if (s.region == Modern::Region::Party) return "Party " + std::to_string(s.slot + 1);
    return "Box " + std::to_string(s.box + 1) + " / Slot " + std::to_string(s.slot + 1);
}

constexpr RecordRegion nativeRegion(Modern::Region r) noexcept {
    return r == Modern::Region::Party ? RecordRegion::Party : RecordRegion::Box;
}

std::string teamFact(const Gen8bSave& save, size_t box, size_t slot) {
    const auto& teams = save.teams();
    if (!teams) return "Unknown (team data unreadable)";
    std::string out;
    for (const auto& t : *teams) {
        if (t.box != box || t.slot != slot) continue;
        if (!out.empty()) out += ", ";
        out += "Team " + std::to_string(t.team + 1) + (t.locked ? " (locked)" : "");
    }
    return out.empty() ? std::string("None") : out;
}

} // namespace

Modern::OpenedSession Gen8bNativeSession::open(std::span<const uint8_t> bytes, std::string_view assignedExactGame) {
    Modern::OpenedSession out;
    std::string error;
    auto save = Gen8bSave::parse(bytes, assignedExactGame, &error);
    if (!save) {
        out.diagnostic = error.empty() ? "BD/SP save failed strict validation" : error;
        return out;
    }
    if (!assignedExactGame.empty() && save->exactGameId() != assignedExactGame) {
        out.diagnostic = "Validated BD/SP title differs from the assigned game; reassign explicitly";
        return out;
    }
    out.session = std::make_unique<Gen8bNativeSession>(std::move(*save));
    return out;
}

Gen8bNativeSession::Gen8bNativeSession(Gen8bSave source)
    : session_(std::move(source)), capabilities_(Capability::find(session_.source().exactGameId())) {
    if (!capabilities_) capabilities_ = Capability::find("brilliant_diamond_switch");
}

std::optional<Modern::RecordView> Gen8bNativeSession::view(Modern::SlotRef slot) const {
    const auto n = toNative(slot);
    if (!n.canonical()) return std::nullopt;
    Modern::RecordView v;
    const auto mon = session_.current(n);
    if (!mon) {
        v.valid = false;  // quarantined malformed record: shown as invalid, never overwritten
        return v;
    }
    v.valid = true;
    v.empty = mon->empty();
    if (v.empty) return v;
    v.partyRecord = true;  // BD/SP stores party-format PB8 (cached stats) in boxes too
    v.species = mon->species();
    v.form = static_cast<uint8_t>(mon->form());
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
    v.statNature = mon->statNature();
    v.ability = mon->ability();
    v.abilityNumber = mon->abilityNumber();
    for (size_t i = 0; i < 6; ++i) {
        v.ivs[i] = mon->iv(i);
        v.evs[i] = mon->ev(i);
        v.stats[i] = mon->stat(i);  // HPmax, Atk, Def, Spe, SpA, SpD (cached by the game)
    }
    v.currentHp = mon->currentHp();
    const auto plain = mon->plain();
    for (size_t i = 0; i < 4; ++i) {
        v.moves[i] = mon->move(i);
        v.pp[i] = plain[0x7A + i];      // PK8 Move1_PP..Move4_PP
        v.ppUps[i] = plain[0x7E + i];   // PK8 Move1_PPUps..Move4_PPUps
        v.relearnMoves[i] = PK8::read16(plain, 0x82 + 2 * i);
    }
    v.ball = mon->ball();
    v.hyperTrainFlags = static_cast<uint8_t>(mon->plain()[0x126] & 0x3F);  // G8PKM.cs (PB8) HyperTrainFlags 0x126
    v.language = mon->language();
    v.friendship = mon->originalTrainerFriendship();
    v.pokerus = mon->pokerus();
    v.originGame = mon->originVersion();
    v.metLevel = mon->metLevel();
    v.metLocation = PK8::read16(plain, 0x122);
    v.eggLocation = PK8::read16(plain, 0x120);
    // PKSM-Core PK8.cpp: egg date 0x119..0x11B, met date 0x11C..0x11E (year - 2000).
    v.metDate = {static_cast<uint16_t>(plain[0x11C] ? 2000 + plain[0x11C] : 0), plain[0x11D], plain[0x11E]};
    v.eggDate = {static_cast<uint16_t>(plain[0x119] ? 2000 + plain[0x119] : 0), plain[0x11A], plain[0x11B]};
    const auto types = Pokemon::getPokemonTypes(v.species, v.form, Enums::GameVersion::BDSP);
    v.types = {static_cast<uint8_t>(types.type1), static_cast<uint8_t>(types.type2)};
    // BD/SP facts, read-only in the shared editor (no verified writer for them).
    if (mon->statNature() != mon->nature())
        v.nativeFacts.push_back({"Stat nature (mint)", std::to_string(mon->statNature())});
    if (slot.region == Modern::Region::Box)
        v.nativeFacts.push_back({"Battle team", teamFact(session_.source(), slot.box, slot.slot)});
    v.nativeFacts.push_back({"Current handler", mon->currentHandler() == 0 ? "Original Trainer" : "Handling Trainer"});
    v.nativeFacts.push_back({"Ability slot", std::to_string(mon->abilityNumber())});
    v.nativeFacts.push_back({"Encryption constant", std::to_string(mon->encryptionConstant())});
    return v;
}

bool Gen8bNativeSession::originalOccupied(Modern::SlotRef slot) const {
    return session_.original(toNative(slot)).occupied();
}

bool Gen8bNativeSession::emptyDestination(Modern::SlotRef slot) const {
    if (slot.region != Modern::Region::Box) return false;
    const auto n = toNative(slot);
    return session_.original(n).blank() && !session_.isStaged(n) && session_.storageBlockedReason(n).empty();
}

std::optional<Modern::SlotRef> Gen8bNativeSession::firstEmptyBoxSlot() const {
    for (size_t b = 0; b < Layout::BoxCount; ++b)
        for (size_t s = 0; s < Layout::BoxSlots; ++s)
            if (const auto at = Modern::SlotRef::boxed(b, s); emptyDestination(at)) return at;
    return std::nullopt;
}

Modern::FieldState Gen8bNativeSession::fieldState(Modern::Field field, Modern::Region region) const noexcept {
    return Gen8EditorProvider::fieldState(field, nativeRegion(region));
}

bool Gen8bNativeSession::stageValue(Modern::SlotRef slot, Modern::Field field, uint32_t value, size_t index,
                                   std::string& error) {
    (void)index;
    return session_.stageValue(toNative(slot), field, value, &error);
}

bool Gen8bNativeSession::stageText(Modern::SlotRef slot, Modern::Field field, std::string_view utf8, std::string& error) {
    return session_.stageText(toNative(slot), field, utf8, &error);
}

uint32_t Gen8bNativeSession::fieldMaximum(Modern::Field field) const noexcept {
    switch (field) {
        case Modern::Field::HeldItem: return 0xFFFF;
        case Modern::Field::Ball: return 26;
        case Modern::Field::TrainerId:
        case Modern::Field::SecretId: return 0xFFFF;
        case Modern::Field::Friendship:
        case Modern::Field::Pokerus: return 0xFF;
        case Modern::Field::IV: return 31;
        case Modern::Field::EV: return 252;
        case Modern::Field::Nature: return 24;
        case Modern::Field::Level: return 100;
        default: return 0;
    }
}

std::vector<uint16_t> Gen8bNativeSession::heldItemChoices() const {
    std::vector<uint16_t> ids{0};
    for (uint32_t id = 1; id <= 0xFFFF; ++id)
        if (rules().heldItemAllowed(id)) ids.push_back(static_cast<uint16_t>(id));
        else if (id > 4096) break;
    return ids;
}

std::vector<uint16_t> Gen8bNativeSession::ballChoices() const {
    return {rules().balls.begin(), rules().balls.end()};
}

std::string Gen8bNativeSession::boxName(size_t box) const {
    const auto name = Utils::utf16ToUtf8(session_.source().boxName(box));
    return name.empty() ? "Box " + std::to_string(box + 1) : name;
}

bool Gen8bNativeSession::beginDraft(Modern::SlotRef slot, std::string& error) {
    const auto n = toNative(slot);
    if (!n.canonical()) {
        error = "BD/SP draft target is not a canonical slot";
        return false;
    }
    const auto mon = session_.current(n);
    if (!mon || mon->empty()) {
        error = "BD/SP draft requires a valid occupied record";
        return false;
    }
    if (draft_) discardDraft();
    draftSnapshot_ = session_.snapshotEntry(n);
    draft_ = slot;
    error.clear();
    return true;
}

bool Gen8bNativeSession::draftDirty() const noexcept {
    return draft_ && !session_.entryMatches(toNative(*draft_), draftSnapshot_);
}

void Gen8bNativeSession::discardDraft() {
    if (!draft_) return;
    session_.restoreEntry(toNative(*draft_), draftSnapshot_);
    draft_.reset();
}

Modern::OperationSupport Gen8bNativeSession::cloneSupport(Modern::SlotRef from) const {
    if (from.region != Modern::Region::Box)
        return {false, "BD/SP Clone copies box records only (party deposit/heal rules are not verified)"};
    if (!originalOccupied(from) || stagedAt(from)) return {false, "Clone needs an unmodified, occupied source record"};
    if (!firstEmptyBoxSlot()) return {false, "No verified blank, team-free box slot is available"};
    return {true, {}};
}

Modern::OperationSupport Gen8bNativeSession::moveSupport(Modern::SlotRef from) const {
    if (from.region != Modern::Region::Box)
        return {false, "Party storage changes are unsupported for BD/SP; Move is box-to-box"};
    if (const auto reason = session_.storageBlockedReason(toNative(from)); !reason.empty()) return {false, reason};
    return cloneSupport(from);
}

Modern::OperationSupport Gen8bNativeSession::removeSupport(Modern::SlotRef at) const {
    if (const auto reason = session_.storageBlockedReason(toNative(at)); !reason.empty()) return {false, reason};
    if (!originalOccupied(at) || stagedAt(at)) return {false, "Remove needs an unmodified, occupied box record"};
    return {true, {}};
}

bool Gen8bNativeSession::clone(Modern::SlotRef from, Modern::SlotRef to, std::string& error) {
    if (draft_ && (*draft_ == from || *draft_ == to)) {
        error = "BD/SP slot has an open editor draft; keep or discard it before Clone";
        return false;
    }
    if (to.region != Modern::Region::Box) {
        error = "BD/SP clone destination must be a box slot";
        return false;
    }
    return session_.cloneToBox(toNative(from), to.box, to.slot, &error);
}

bool Gen8bNativeSession::move(Modern::SlotRef from, Modern::SlotRef to, std::string& error) {
    if (draft_ && (*draft_ == from || *draft_ == to)) {
        error = "BD/SP slot has an open editor draft; keep or discard it before Move";
        return false;
    }
    if (const auto support = moveSupport(from); !support.available) {
        error = std::string(support.reason);
        return false;
    }
    if (!clone(from, to, error)) return false;
    if (!session_.removeFromBox(from.box, from.slot, &error)) {
        (void)session_.undo(toNative(to));  // atomic: no half-moved record survives
        return false;
    }
    return true;
}

bool Gen8bNativeSession::remove(Modern::SlotRef at, std::string& error) {
    if (draft_ && *draft_ == at) {
        error = "BD/SP slot has an open editor draft; keep or discard it before Remove";
        return false;
    }
    if (at.region != Modern::Region::Box) {
        error = "Party removal is unsupported for BD/SP";
        return false;
    }
    return session_.removeFromBox(at.box, at.slot, &error);
}

std::vector<Modern::TrainerFact> Gen8bNativeSession::trainerFacts() const {
    const auto t = session_.source().trainer();
    const uint32_t id32 = (static_cast<uint32_t>(t.sid) << 16) | t.tid;
    std::vector<Modern::TrainerFact> facts;
    facts.push_back({"Name", trainerName()});
    facts.push_back({"Game", std::string(capabilities_->title)});
    // Gen VII+ display IDs (PKHeX ITrainerID32: TrainerTID7 = ID32 % 1e6, TrainerSID7 = ID32 / 1e6).
    facts.push_back({"Trainer ID", std::to_string(id32 % 1000000u)});
    facts.push_back({"Secret ID", std::to_string(id32 / 1000000u)});
    facts.push_back({"TID16 / SID16", std::to_string(t.tid) + " / " + std::to_string(t.sid)});
    facts.push_back({"Gender", t.gender == 0 ? "Male" : t.gender == 1 ? "Female" : "Unknown"});
    facts.push_back({"Save revision", std::string(session_.source().revision())});
    const auto& teams = session_.source().teams();
    facts.push_back({"Battle teams", teams ? std::to_string(teams->size()) + " box references" : "Unreadable (box changes refused)"});
    return facts;
}

std::string Gen8bNativeSession::trainerName() const {
    const auto name = Utils::utf16ToUtf8(session_.source().trainer().name);
    return name.empty() ? std::string("Unknown") : name;
}

uint8_t Gen8bNativeSession::trainerGender() const noexcept {
    return session_.source().bytes()[Layout::Male] == 1 ? 0 : 1;   // MyStatus8b.Male
}

uint16_t Gen8bNativeSession::trainerId() const noexcept { return PK8::read16(session_.source().bytes(), Layout::Tid); }

uint16_t Gen8bNativeSession::secretId() const noexcept { return PK8::read16(session_.source().bytes(), Layout::Sid); }

std::vector<Modern::ReviewRow> Gen8bNativeSession::reviewRows() const {
    std::vector<Modern::ReviewRow> rows;
    for (const auto& change : session_.changes()) {
        const std::string where = slotLabel(fromNative(change.slot));
        switch (change.kind) {
            case ChangeKind::Clone:
                rows.push_back({where, "Clone of " + (change.cloneSource ? slotLabel(fromNative(*change.cloneSource)) : std::string("?"))});
                break;
            case ChangeKind::Remove:
                rows.push_back({where, "Removed (staged copy only; source unchanged)"});
                break;
            case ChangeKind::Edit:
                for (const auto& note : change.notes) {
                    const auto colon = note.find(": ");
                    rows.push_back({where, colon == std::string::npos ? note : note.substr(colon + 2)});
                }
                break;
        }
    }
    return rows;
}

Modern::OperationSupport Gen8bNativeSession::exportSupport() const {
    if (!session_.hasChanges()) return {false, "Export Copy needs at least one staged change"};
    return {true, {}};
}

Common::VerifiedExportResult Gen8bNativeSession::exportWorkingCopy(const Modern::ExportRequest& request) const {
    Gen8bExportRequest r;
    r.rootDirectory = request.rootDirectory;
    r.sourcePath = request.sourcePath;
    r.timestamp = request.timestamp;
    r.applicationSha = request.applicationSha;
    return session_.exportWorkingCopy(r);
}

} // namespace PokeVault::Integration::Gen8b
