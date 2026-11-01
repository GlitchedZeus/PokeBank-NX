#include "Integration/Gen9/Gen9SvNativeSession.h"

#include "Enums/GameVersion.h"
#include "Pokemon/PokemonTypes.h"
#include "Utils/StringHelpers.h"

#include <utility>

namespace PokeVault::Integration::Gen9Sv {
namespace {

std::string slotLabel(Modern::SlotRef s) {
    if (s.region == Modern::Region::Party) return "Party " + std::to_string(s.slot + 1);
    return "Box " + std::to_string(s.box + 1) + " / Slot " + std::to_string(s.slot + 1);
}

constexpr Region nativeRegion(Modern::Region r) noexcept {
    return r == Modern::Region::Party ? Region::Party : Region::Box;
}

std::string teamFact(const Gen9SvSave& save, size_t box, size_t slot) {
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

Modern::OpenedSession Gen9SvNativeSession::open(std::span<const uint8_t> bytes, std::string_view assignedExactGame) {
    Modern::OpenedSession out;
    std::string error;
    auto save = Gen9SvSave::parse(bytes, assignedExactGame, &error);
    if (!save) {
        out.diagnostic = error.empty() ? "Scarlet/Violet save failed strict validation" : error;
        return out;
    }
    if (!assignedExactGame.empty() && save->exactGameId() != assignedExactGame) {
        out.diagnostic = "Validated Scarlet/Violet title differs from the assigned game; reassign explicitly";
        return out;
    }
    out.session = std::make_unique<Gen9SvNativeSession>(std::move(*save));
    return out;
}

Gen9SvNativeSession::Gen9SvNativeSession(Gen9SvSave source)
    : session_(std::move(source)), capabilities_(Capability::find(session_.source().exactGameId())) {
    if (!capabilities_) capabilities_ = Capability::find("scarlet_switch");
}

std::optional<Modern::RecordView> Gen9SvNativeSession::view(Modern::SlotRef slot) const {
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
    v.partyRecord = true;  // Scarlet/Violet stores party-format PB8 (cached stats) in boxes too
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
    v.hyperTrainFlags = static_cast<uint8_t>(mon->plain()[0x126] & 0x3F);  // PK9.cs HyperTrainFlags 0x126
    v.language = plain[0xD5];  // PK9 language (PKHeX PK9.cs), not the PK8 offset
    v.friendship = mon->originalTrainerFriendship();
    v.pokerus = mon->pokerus();
    v.originGame = mon->originVersion();
    v.metLevel = mon->metLevel();
    v.metLocation = PK8::read16(plain, 0x122);
    v.eggLocation = PK8::read16(plain, 0x120);
    // PKSM-Core PK8.cpp: egg date 0x119..0x11B, met date 0x11C..0x11E (year - 2000).
    v.metDate = {static_cast<uint16_t>(plain[0x11C] ? 2000 + plain[0x11C] : 0), plain[0x11D], plain[0x11E]};
    v.eggDate = {static_cast<uint16_t>(plain[0x119] ? 2000 + plain[0x119] : 0), plain[0x11A], plain[0x11B]};
    const auto types = Pokemon::getPokemonTypes(v.species, v.form, Enums::GameVersion::SV);
    v.types = {static_cast<uint8_t>(types.type1), static_cast<uint8_t>(types.type2)};
    // Scarlet/Violet facts, read-only in the shared editor (no verified writer for them).
    if (mon->statNature() != mon->nature())
        v.nativeFacts.push_back({"Stat nature (mint)", std::to_string(mon->statNature())});
    if (slot.region == Modern::Region::Box)
        v.nativeFacts.push_back({"Battle team", teamFact(session_.source(), slot.box, slot.slot)});
    v.nativeFacts.push_back({"Tera Type (original / override)", std::to_string(plain[0x94]) + " / " + std::to_string(plain[0x95])});
    v.nativeFacts.push_back({"Scale", std::to_string(plain[0x4A])});
    v.nativeFacts.push_back({"Current handler", mon->currentHandler() == 0 ? "Original Trainer" : "Handling Trainer"});
    v.nativeFacts.push_back({"Ability slot", std::to_string(mon->abilityNumber())});
    v.nativeFacts.push_back({"Encryption constant", std::to_string(mon->encryptionConstant())});
    return v;
}

bool Gen9SvNativeSession::originalOccupied(Modern::SlotRef slot) const {
    return session_.source().at(toNative(slot)).occupied();
}

bool Gen9SvNativeSession::emptyDestination(Modern::SlotRef slot) const {
    if (slot.region != Modern::Region::Box) return false;
    const auto n = toNative(slot);
    return session_.source().at(n).blank() && !session_.isStaged(n) && session_.storageBlockedReason(n).empty();
}

std::optional<Modern::SlotRef> Gen9SvNativeSession::firstEmptyBoxSlot() const {
    for (size_t b = 0; b < Layout::BoxCount; ++b)
        for (size_t s = 0; s < Layout::BoxSlots; ++s)
            if (const auto at = Modern::SlotRef::boxed(b, s); emptyDestination(at)) return at;
    return std::nullopt;
}

Modern::FieldState Gen9SvNativeSession::fieldState(Modern::Field field, Modern::Region region) const noexcept {
    return Gen8EditorProvider::fieldState(field, nativeRegion(region));
}

bool Gen9SvNativeSession::stageValue(Modern::SlotRef slot, Modern::Field field, uint32_t value, size_t index,
                                   std::string& error) {
    (void)index;
    return session_.stageValue(toNative(slot), field, value, &error);
}

bool Gen9SvNativeSession::stageText(Modern::SlotRef slot, Modern::Field field, std::string_view utf8, std::string& error) {
    return session_.stageText(toNative(slot), field, utf8, &error);
}

uint32_t Gen9SvNativeSession::fieldMaximum(Modern::Field field) const noexcept {
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

std::vector<uint16_t> Gen9SvNativeSession::heldItemChoices() const {
    std::vector<uint16_t> ids{0};
    for (uint32_t id = 1; id <= 0xFFFF; ++id)
        if (rules().heldItemAllowed(id)) ids.push_back(static_cast<uint16_t>(id));
        else if (id > 4096) break;
    return ids;
}

std::vector<uint16_t> Gen9SvNativeSession::ballChoices() const {
    return {rules().balls.begin(), rules().balls.end()};
}


bool Gen9SvNativeSession::beginDraft(Modern::SlotRef slot, std::string& error) {
    const auto n = toNative(slot);
    if (!n.canonical()) {
        error = "Scarlet/Violet draft target is not a canonical slot";
        return false;
    }
    const auto mon = session_.current(n);
    if (!mon || mon->empty()) {
        error = "Scarlet/Violet draft requires a valid occupied record";
        return false;
    }
    if (draft_) discardDraft();
    draftSnapshot_ = session_.snapshotEntry(n);
    draft_ = slot;
    error.clear();
    return true;
}

bool Gen9SvNativeSession::draftDirty() const noexcept {
    return draft_ && !session_.entryMatches(toNative(*draft_), draftSnapshot_);
}

void Gen9SvNativeSession::discardDraft() {
    if (!draft_) return;
    session_.restoreEntry(toNative(*draft_), draftSnapshot_);
    draft_.reset();
}

Modern::OperationSupport Gen9SvNativeSession::cloneSupport(Modern::SlotRef from) const {
    if (from.region != Modern::Region::Box)
        return {false, "Scarlet/Violet Clone copies box records only (party deposit/heal rules are not verified)"};
    if (!originalOccupied(from) || stagedAt(from)) return {false, "Clone needs an unmodified, occupied source record"};
    if (!firstEmptyBoxSlot()) return {false, "No verified blank, team-free box slot is available"};
    return {true, {}};
}

Modern::OperationSupport Gen9SvNativeSession::moveSupport(Modern::SlotRef from) const {
    if (from.region != Modern::Region::Box)
        return {false, "Party storage changes are unsupported for Scarlet/Violet; Move is box-to-box"};
    if (const auto reason = session_.storageBlockedReason(toNative(from)); !reason.empty()) return {false, reason};
    return cloneSupport(from);
}

Modern::OperationSupport Gen9SvNativeSession::removeSupport(Modern::SlotRef at) const {
    if (const auto reason = session_.storageBlockedReason(toNative(at)); !reason.empty()) return {false, reason};
    if (!originalOccupied(at) || stagedAt(at)) return {false, "Remove needs an unmodified, occupied box record"};
    return {true, {}};
}

bool Gen9SvNativeSession::clone(Modern::SlotRef from, Modern::SlotRef to, std::string& error) {
    if (draft_ && (*draft_ == from || *draft_ == to)) {
        error = "Scarlet/Violet slot has an open editor draft; keep or discard it before Clone";
        return false;
    }
    if (to.region != Modern::Region::Box) {
        error = "Scarlet/Violet clone destination must be a box slot";
        return false;
    }
    return session_.cloneToBox(toNative(from), toNative(to), &error);
}

bool Gen9SvNativeSession::move(Modern::SlotRef from, Modern::SlotRef to, std::string& error) {
    if (draft_ && (*draft_ == from || *draft_ == to)) {
        error = "Scarlet/Violet slot has an open editor draft; keep or discard it before Move";
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

bool Gen9SvNativeSession::remove(Modern::SlotRef at, std::string& error) {
    if (draft_ && *draft_ == at) {
        error = "Scarlet/Violet slot has an open editor draft; keep or discard it before Remove";
        return false;
    }
    if (at.region != Modern::Region::Box) {
        error = "Party removal is unsupported for Scarlet/Violet";
        return false;
    }
    return session_.removeFromBox(toNative(at), &error);
}

std::vector<Modern::TrainerFact> Gen9SvNativeSession::trainerFacts() const {
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
    facts.push_back({"Battle teams", teams ? std::to_string(teams->size()) + " box references" : "Unreadable (box changes refused)"});
    return facts;
}

std::string Gen9SvNativeSession::trainerName() const {
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

uint8_t Gen9SvNativeSession::trainerGender() const noexcept {
    const auto g = session_.source().myStatus().data[Layout::MyGender];
    return g <= 1 ? g : 0;
}

uint16_t Gen9SvNativeSession::trainerId() const noexcept { return PK8::read16(session_.source().myStatus().data, Layout::MyTid); }

uint16_t Gen9SvNativeSession::secretId() const noexcept { return PK8::read16(session_.source().myStatus().data, Layout::MySid); }

std::vector<Modern::ReviewRow> Gen9SvNativeSession::reviewRows() const {
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

Modern::OperationSupport Gen9SvNativeSession::exportSupport() const {
    if (!session_.hasChanges()) return {false, "Export Copy needs at least one staged change"};
    return {true, {}};
}

Common::VerifiedExportResult Gen9SvNativeSession::exportWorkingCopy(const Modern::ExportRequest& request) const {
    Gen9SvExportRequest r;
    r.rootDirectory = request.rootDirectory;
    r.sourcePath = request.sourcePath;
    r.timestamp = request.timestamp;
    r.applicationSha = request.applicationSha;
    return session_.exportWorkingCopy(r);
}

} // namespace PokeVault::Integration::Gen9Sv
