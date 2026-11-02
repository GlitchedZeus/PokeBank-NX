#include "Integration/Gen8/Gen8TeamIndexes.h"
#include "Integration/Gen9/Gen9ZaNativeSession.h"

#include "Enums/GameVersion.h"
#include "Pokemon/PokemonTypes.h"
#include "Utils/StringHelpers.h"

#include <utility>

namespace PokeVault::Integration::Gen9Za {
namespace {

std::string slotLabel(Modern::SlotRef s) {
    if (s.region == Modern::Region::Party) return "Party " + std::to_string(s.slot + 1);
    return "Box " + std::to_string(s.box + 1) + " / Slot " + std::to_string(s.slot + 1);
}

constexpr Region nativeRegion(Modern::Region r) noexcept {
    return r == Modern::Region::Party ? Region::Party : Region::Box;
}

std::string teamFact(const Gen9ZaSave& save, size_t box, size_t slot) {
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

Modern::OpenedSession Gen9ZaNativeSession::open(std::span<const uint8_t> bytes, std::string_view assignedExactGame) {
    Modern::OpenedSession out;
    std::string error;
    auto save = Gen9ZaSave::parse(bytes, assignedExactGame, &error);
    if (!save) {
        out.diagnostic = error.empty() ? "Legends: Z-A save failed strict validation" : error;
        return out;
    }
    if (!assignedExactGame.empty() && save->exactGameId() != assignedExactGame) {
        out.diagnostic = "Validated Legends: Z-A title differs from the assigned game; reassign explicitly";
        return out;
    }
    out.session = std::make_unique<Gen9ZaNativeSession>(std::move(*save));
    return out;
}

Gen9ZaNativeSession::Gen9ZaNativeSession(Gen9ZaSave source)
    : session_(std::move(source)), capabilities_(Capability::find(session_.source().exactGameId())) {
    if (!capabilities_) capabilities_ = Capability::find("legends_za_switch");
}

std::optional<Modern::RecordView> Gen9ZaNativeSession::view(Modern::SlotRef slot) const {
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
    v.partyRecord = true;  // Legends: Z-A stores party-format PB8 (cached stats) in boxes too
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
    v.hyperTrainFlags = static_cast<uint8_t>(mon->plain()[0x126] & 0x3F);  // PA9.cs HyperTrainFlags 0x126
    v.language = plain[0xD5];  // PA9 language (PKHeX PA9.cs), not the PK8 offset
    v.friendship = mon->originalTrainerFriendship();
    v.pokerus = mon->pokerus();
    v.originGame = mon->originVersion();
    v.metLevel = mon->metLevel();
    v.metLocation = PK8::read16(plain, 0x122);
    v.eggLocation = PK8::read16(plain, 0x120);
    // PKSM-Core PK8.cpp: egg date 0x119..0x11B, met date 0x11C..0x11E (year - 2000).
    v.metDate = {static_cast<uint16_t>(plain[0x11C] ? 2000 + plain[0x11C] : 0), plain[0x11D], plain[0x11E]};
    v.eggDate = {static_cast<uint16_t>(plain[0x119] ? 2000 + plain[0x119] : 0), plain[0x11A], plain[0x11B]};
    const auto types = Pokemon::getPokemonTypes(v.species, v.form, Enums::GameVersion::ZA);
    v.types = {static_cast<uint8_t>(types.type1), static_cast<uint8_t>(types.type2)};
    // Legends: Z-A facts, read-only in the shared editor (no verified writer for them).
    if (mon->statNature() != mon->nature())
        v.nativeFacts.push_back({"Stat nature (mint)", std::to_string(mon->statNature())});
    if (slot.region == Modern::Region::Box)
        v.nativeFacts.push_back({"Battle team", teamFact(session_.source(), slot.box, slot.slot)});
    v.nativeFacts.push_back({"Alpha", plain[0x23] ? "Yes" : "No"});
    v.nativeFacts.push_back({"Save revision", std::to_string(session_.source().revision())});
    v.nativeFacts.push_back({"Current handler", mon->currentHandler() == 0 ? "Original Trainer" : "Handling Trainer"});
    v.nativeFacts.push_back({"Ability slot", std::to_string(mon->abilityNumber())});
    v.nativeFacts.push_back({"Encryption constant", std::to_string(mon->encryptionConstant())});
    return v;
}

bool Gen9ZaNativeSession::originalOccupied(Modern::SlotRef slot) const {
    return session_.source().at(toNative(slot)).occupied();
}

bool Gen9ZaNativeSession::emptyDestination(Modern::SlotRef slot) const {
    if (slot.region != Modern::Region::Box) return false;
    const auto n = toNative(slot);
    return session_.source().at(n).blank() && !session_.isStaged(n) && session_.storageBlockedReason(n).empty();
}

std::optional<Modern::SlotRef> Gen9ZaNativeSession::firstEmptyBoxSlot() const {
    for (size_t b = 0; b < Layout::BoxCount; ++b)
        for (size_t s = 0; s < Layout::BoxSlots; ++s)
            if (const auto at = Modern::SlotRef::boxed(b, s); emptyDestination(at)) return at;
    return std::nullopt;
}

Modern::FieldState Gen9ZaNativeSession::fieldState(Modern::Field field, Modern::Region region) const noexcept {
    return Gen8EditorProvider::fieldState(field, nativeRegion(region));
}

bool Gen9ZaNativeSession::stageValue(Modern::SlotRef slot, Modern::Field field, uint32_t value, size_t index,
                                   std::string& error) {
    (void)index;
    return session_.stageValue(toNative(slot), field, value, &error);
}

bool Gen9ZaNativeSession::stageText(Modern::SlotRef slot, Modern::Field field, std::string_view utf8, std::string& error) {
    return session_.stageText(toNative(slot), field, utf8, &error);
}

uint32_t Gen9ZaNativeSession::fieldMaximum(Modern::Field field) const noexcept {
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

std::vector<uint16_t> Gen9ZaNativeSession::heldItemChoices() const {
    std::vector<uint16_t> ids{0};
    for (uint32_t id = 1; id <= 0xFFFF; ++id)
        if (rules().heldItemAllowed(id)) ids.push_back(static_cast<uint16_t>(id));
        else if (id > 4096) break;
    return ids;
}

std::vector<uint16_t> Gen9ZaNativeSession::ballChoices() const {
    return {rules().balls.begin(), rules().balls.end()};
}


bool Gen9ZaNativeSession::beginDraft(Modern::SlotRef slot, std::string& error) {
    const auto n = toNative(slot);
    if (!n.canonical()) {
        error = "Legends: Z-A draft target is not a canonical slot";
        return false;
    }
    const auto mon = session_.current(n);
    if (!mon || mon->empty()) {
        error = "Legends: Z-A draft requires a valid occupied record";
        return false;
    }
    if (draft_) discardDraft();
    draftSnapshot_ = session_.snapshotEntry(n);
    draft_ = slot;
    error.clear();
    return true;
}

bool Gen9ZaNativeSession::draftDirty() const noexcept {
    return draft_ && !session_.entryMatches(toNative(*draft_), draftSnapshot_);
}

void Gen9ZaNativeSession::discardDraft() {
    if (!draft_) return;
    session_.restoreEntry(toNative(*draft_), draftSnapshot_);
    draft_.reset();
}

Modern::OperationSupport Gen9ZaNativeSession::cloneSupport(Modern::SlotRef from) const {
    if (from.region != Modern::Region::Box)
        return {false, "Legends: Z-A Clone copies box records only (party deposit/heal rules are not verified)"};
    if (!originalOccupied(from) || stagedAt(from)) return {false, "Clone needs an unmodified, occupied source record"};
    if (!firstEmptyBoxSlot()) return {false, "No verified blank, team-free box slot is available"};
    return {true, {}};
}

Modern::OperationSupport Gen9ZaNativeSession::moveSupport(Modern::SlotRef from) const {
    if (from.region != Modern::Region::Box)
        return {false, "Party storage changes are unsupported for Legends: Z-A; Move is box-to-box"};
    if (const auto reason = session_.storageBlockedReason(toNative(from)); !reason.empty()) return {false, reason};
    return cloneSupport(from);
}

Modern::OperationSupport Gen9ZaNativeSession::removeSupport(Modern::SlotRef at) const {
    if (const auto reason = session_.storageBlockedReason(toNative(at)); !reason.empty()) return {false, reason};
    if (!originalOccupied(at) || stagedAt(at)) return {false, "Remove needs an unmodified, occupied box record"};
    return {true, {}};
}

bool Gen9ZaNativeSession::clone(Modern::SlotRef from, Modern::SlotRef to, std::string& error) {
    if (draft_ && (*draft_ == from || *draft_ == to)) {
        error = "Legends: Z-A slot has an open editor draft; keep or discard it before Clone";
        return false;
    }
    if (to.region != Modern::Region::Box) {
        error = "Legends: Z-A clone destination must be a box slot";
        return false;
    }
    return session_.cloneToBox(toNative(from), toNative(to), &error);
}

bool Gen9ZaNativeSession::move(Modern::SlotRef from, Modern::SlotRef to, std::string& error) {
    if (draft_ && (*draft_ == from || *draft_ == to)) {
        error = "Legends: Z-A slot has an open editor draft; keep or discard it before Move";
        return false;
    }
    if (const auto support = moveSupport(from); !support.available) {
        error = std::string(support.reason);
        return false;
    }
    if (!clone(from, to, error)) return false;
    if (!session_.removeFromBox(toNative(from), &error)) {
        (void)session_.undo(toNative(to));  // atomic: no half-moved record survives
        return false;
    }
    return true;
}

bool Gen9ZaNativeSession::remove(Modern::SlotRef at, std::string& error) {
    if (draft_ && *draft_ == at) {
        error = "Legends: Z-A slot has an open editor draft; keep or discard it before Remove";
        return false;
    }
    if (at.region != Modern::Region::Box) {
        error = "Party removal is unsupported for Legends: Z-A";
        return false;
    }
    return session_.removeFromBox(toNative(at), &error);
}

std::vector<Modern::TrainerFact> Gen9ZaNativeSession::trainerFacts() const {
    const uint16_t tid = trainerId(), sid = secretId();
    const uint32_t id32 = (static_cast<uint32_t>(sid) << 16) | tid;
    std::vector<Modern::TrainerFact> facts;
    facts.push_back({"Name", trainerName()});
    facts.push_back({"Game", std::string(capabilities_->title)});
    facts.push_back({"Trainer ID", std::to_string(id32 % 1000000u)});
    facts.push_back({"Secret ID", std::to_string(id32 / 1000000u)});
    facts.push_back({"TID16 / SID16", std::to_string(tid) + " / " + std::to_string(sid)});
    facts.push_back({"Gender", trainerGender() == 0 ? "Male" : "Female"});
    const auto& teams = session_.source().teams();
    facts.push_back({"Battle teams", Gen8::teamFactText(teams, session_.source().teamDiagnostic())});
    return facts;
}

std::string Gen9ZaNativeSession::trainerName() const {
    const auto& d = session_.source().myStatus().data;
    std::u16string name;
    for (size_t i = 0; i + 1 < Layout::MyNameBytes; i += 2) {
        const uint16_t ch = PK8::read16(d, Layout::MyName + i);
        if (ch == 0) break;
        name.push_back(static_cast<char16_t>(ch));
    }
    const auto out = Utils::utf16ToUtf8(name);
    return out.empty() ? std::string("Unknown") : out;
}

uint8_t Gen9ZaNativeSession::trainerGender() const noexcept {
    const auto g = session_.source().myStatus().data[Layout::MyGender];
    return g <= 1 ? g : 0;
}

uint16_t Gen9ZaNativeSession::trainerId() const noexcept { return PK8::read16(session_.source().myStatus().data, Layout::MyTid); }

uint16_t Gen9ZaNativeSession::secretId() const noexcept { return PK8::read16(session_.source().myStatus().data, Layout::MySid); }

std::vector<Modern::ReviewRow> Gen9ZaNativeSession::reviewRows() const {
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

Modern::OperationSupport Gen9ZaNativeSession::exportSupport() const {
    if (!session_.hasChanges()) return {false, "Export Copy needs at least one staged change"};
    return {true, {}};
}

Common::VerifiedExportResult Gen9ZaNativeSession::exportWorkingCopy(const Modern::ExportRequest& request) const {
    Gen9ZaExportRequest r;
    r.rootDirectory = request.rootDirectory;
    r.sourcePath = request.sourcePath;
    r.timestamp = request.timestamp;
    r.applicationSha = request.applicationSha;
    return session_.exportWorkingCopy(r);
}

} // namespace PokeVault::Integration::Gen9Za
