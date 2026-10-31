#include "Integration/Gen6/Gen6NativeSession.h"

#include "Enums/GameVersion.h"
#include "Enums/LanguageID.h"
#include "Integration/Gen6/Gen6Readouts.h"
#include "Integration/Gen6/Gen6Stats.h"
#include "Pokemon/PokemonTypes.h"
#include "Utils/StringHelpers.h"

#include <algorithm>
#include <map>
#include <utility>

namespace PokeVault::Integration::Gen6 {
namespace {

std::string slotLabel(Modern::SlotRef s) {
    if (s.region == Modern::Region::Party) return "Party " + std::to_string(s.slot + 1);
    return "Box " + std::to_string(s.box + 1) + " / Slot " + std::to_string(s.slot + 1);
}

constexpr RecordRegion nativeRegion(Modern::Region r) noexcept {
    return r == Modern::Region::Party ? RecordRegion::Party : RecordRegion::Box;
}

std::string gameTitle(std::string_view id) {
    if (id == "x_3ds") return "X";
    if (id == "y_3ds") return "Y";
    if (id == "omegaruby_3ds") return "Omega Ruby";
    if (id == "alphasapphire_3ds") return "Alpha Sapphire";
    return std::string(id);
}

} // namespace

Modern::OpenedSession Gen6NativeSession::open(std::span<const uint8_t> bytes, std::string_view assignedExactGame) {
    Modern::OpenedSession out;
    std::string error;
    auto save = Gen6Save::parse(bytes, assignedExactGame, &error);
    if (!save) {
        out.diagnostic = error.empty() ? "Gen VI save failed strict validation" : error;
        return out;
    }
    if (!assignedExactGame.empty() && save->exactGameId() != assignedExactGame) {
        out.diagnostic = "Validated Gen VI title differs from the assigned game; reassign explicitly";
        return out;
    }
    out.session = std::make_unique<Gen6NativeSession>(std::move(*save));
    return out;
}

Gen6NativeSession::Gen6NativeSession(Gen6Save source)
    : session_(std::move(source)), capabilities_(Capability::find(session_.source().exactGameId())) {
    // Every Gen VI exact id is registered; a missing row would be a programming error that
    // the registry test catches, so fall back to the X row rather than dereferencing null.
    if (!capabilities_) capabilities_ = Capability::find("x_3ds");
}

Enums::GameVersion Gen6NativeSession::gameGroup() const noexcept {
    return session_.source().family() == Layout::Family::XY ? Enums::GameVersion::XY : Enums::GameVersion::ORAS;
}

std::string Gen6NativeSession::boxName(size_t box) const {
    if (const auto name = Readouts::boxName(session_.source(), box); name && !name->empty())
        return Utils::utf16ToUtf8(*name);
    return "Box " + std::to_string(box + 1);
}

SlotRecord Gen6NativeSession::original(Modern::SlotRef slot) const {
    const auto n = toNative(slot);
    if (!n.canonical()) return {};
    return n.region == RecordRegion::Party ? session_.source().party(n.slot)
                                           : session_.source().box(n.box, n.slot);
}

std::optional<Modern::RecordView> Gen6NativeSession::view(Modern::SlotRef slot) const {
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
    v.partyRecord = mon->partyForm();
    v.species = mon->species();
    v.form = mon->form();
    v.gender = mon->gender();
    v.shiny = mon->shiny();
    v.egg = mon->isEgg();
    v.nicknamed = mon->isNicknamed();
    v.fatefulEncounter = mon->fatefulEncounter();
    v.experience = mon->experience();
    v.level = v.partyRecord ? mon->statLevel() : Stats::level(*mon).value_or(0);
    if (v.level == 0) v.level = Stats::level(*mon).value_or(0);
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
    }
    if (const auto p = Stats::personal(mon->species(), mon->form()))
        v.baseStats = {p->hp, p->atk, p->def, p->spe, p->spa, p->spd};
    if (v.partyRecord) {
        v.currentHp = mon->stat(0);
        for (size_t i = 0; i < 6; ++i) v.stats[i] = mon->stat(i + 1);
    } else if (const auto s = Stats::compute(*mon)) {
        v.stats = *s;
        v.currentHp = v.stats[0];
    }
    for (size_t i = 0; i < 4; ++i) {
        v.moves[i] = mon->move(i);
        v.pp[i] = mon->movePP(i);
        v.ppUps[i] = mon->movePPUps(i);
        v.relearnMoves[i] = PK6::read16(mon->plain(), 0x6A + 2 * i);
    }
    v.ball = mon->ball();
    v.language = mon->language();
    v.friendship = mon->originalTrainerFriendship();
    v.pokerus = mon->pokerus();
    v.originGame = mon->originVersion();
    v.metLevel = mon->metLevel();
    v.metLocation = mon->metLocation();
    v.eggLocation = mon->eggLocation();
    v.metDate = {static_cast<uint16_t>(mon->metYear() ? 2000 + mon->metYear() : 0), mon->metMonth(), mon->metDay()};
    v.eggDate = {static_cast<uint16_t>(mon->eggYear() ? 2000 + mon->eggYear() : 0),
                 mon->plain()[0xD2], mon->plain()[0xD3]};
    const auto types = Pokemon::getPokemonTypes(v.species, v.form, gameGroup());
    v.types = {static_cast<uint8_t>(types.type1), static_cast<uint8_t>(types.type2)};
    // Gen VI-native facts, read-only in the shared editor (no verified writer exists for them).
    v.nativeFacts.push_back({"Current handler", mon->currentHandler() == 0 ? "Original Trainer" : "Handling Trainer"});
    v.nativeFacts.push_back({"Ability slot", std::to_string(mon->abilityNumber())});
    v.nativeFacts.push_back({"Encryption constant", std::to_string(mon->encryptionConstant())});
    return v;
}

bool Gen6NativeSession::originalOccupied(Modern::SlotRef slot) const {
    return original(slot).occupied();
}

bool Gen6NativeSession::emptyDestination(Modern::SlotRef slot) const {
    if (slot.region != Modern::Region::Box) return false;
    return original(slot).blank() && !stagedAt(slot);
}

bool Gen6NativeSession::stagedAt(Modern::SlotRef slot) const {
    const auto n = toNative(slot);
    for (const auto& c : session_.changes())
        if (c.slot == n) return true;
    return false;
}

std::optional<Modern::SlotRef> Gen6NativeSession::firstEmptyBoxSlot() const {
    for (size_t b = 0; b < Layout::BoxCount; ++b)
        for (size_t s = 0; s < Layout::BoxSlots; ++s)
            if (const auto at = Modern::SlotRef::boxed(b, s); emptyDestination(at)) return at;
    return std::nullopt;
}

Modern::FieldState Gen6NativeSession::fieldState(Modern::Field field, Modern::Region region) const noexcept {
    return Gen6EditorProvider::fieldState(field, nativeRegion(region));
}

bool Gen6NativeSession::stageValue(Modern::SlotRef slot, Modern::Field field, uint32_t value, size_t index,
                                   std::string& error) {
    return session_.stageValue(toNative(slot), field, value, index, &error);
}

bool Gen6NativeSession::stageText(Modern::SlotRef slot, Modern::Field field, std::string_view utf8, std::string& error) {
    return session_.stageText(toNative(slot), field, utf8, &error);
}

uint32_t Gen6NativeSession::fieldMaximum(Modern::Field field) const noexcept {
    const auto lim = session_.limits();
    switch (field) {
        case Modern::Field::HeldItem: return lim.maxHeldItem;
        case Modern::Field::Ball: return lim.maxBall;
        case Modern::Field::IV: return lim.maxIV;
        case Modern::Field::EV: return lim.maxEV;
        case Modern::Field::Nature: return lim.maxNature;
        case Modern::Field::Level: return 100;
        case Modern::Field::TrainerId:
        case Modern::Field::SecretId: return 0xFFFF;
        case Modern::Field::Friendship:
        case Modern::Field::Pokerus: return 0xFF;
        default: return 0;
    }
}

std::vector<uint16_t> Gen6NativeSession::heldItemChoices() const {
    std::vector<uint16_t> ids{0};
    const auto f = session_.source().family();
    const auto lim = session_.limits();
    for (const size_t pocket : {static_cast<size_t>(BagPocket6::Items), static_cast<size_t>(BagPocket6::Medicine),
                                static_cast<size_t>(BagPocket6::Berries)})
        for (const uint16_t id : bagPocketItems6(f, pocket))
            if (id != 0 && id <= lim.maxHeldItem && gen6BagItemAddable(f, pocket, id) &&
                std::find(ids.begin(), ids.end(), id) == ids.end())
                ids.push_back(id);
    return ids;
}

std::vector<uint16_t> Gen6NativeSession::ballChoices() const {
    std::vector<uint16_t> balls;
    for (uint16_t b = 1; b <= session_.limits().maxBall; ++b) balls.push_back(b);
    return balls;
}

int Gen6NativeSession::maxPP(uint16_t, uint8_t) const noexcept {
    // Moves are read-only for Gen VI (no pinned XY/ORAS PP table in this build); 0 = unknown.
    return 0;
}

bool Gen6NativeSession::beginDraft(Modern::SlotRef slot, std::string& error) {
    const auto n = toNative(slot);
    if (!n.canonical()) {
        error = "Gen VI draft target is not a canonical slot";
        return false;
    }
    const auto mon = session_.current(n);
    if (!mon || mon->empty()) {
        error = "Gen VI draft requires a valid occupied record";
        return false;
    }
    if (draft_) discardDraft();  // a stale bracket is never silently kept
    draftSnapshot_ = session_.snapshotEntry(n);
    draft_ = slot;
    error.clear();
    return true;
}

bool Gen6NativeSession::draftDirty() const noexcept {
    return draft_ && !session_.entryMatches(toNative(*draft_), draftSnapshot_);
}

void Gen6NativeSession::discardDraft() {
    if (!draft_) return;
    session_.restoreEntry(toNative(*draft_), draftSnapshot_);
    draft_.reset();
}

Modern::OperationSupport Gen6NativeSession::cloneSupport(Modern::SlotRef from) const {
    const auto rec = original(from);
    if (!rec.occupied() || stagedAt(from))
        return {false, "Clone needs an unmodified, occupied source record"};
    if (!firstEmptyBoxSlot()) return {false, "No verified blank box slot is available"};
    return {true, {}};
}

Modern::OperationSupport Gen6NativeSession::moveSupport(Modern::SlotRef from) const {
    if (from.region != Modern::Region::Box)
        return {false, "Party removal is unsupported for Gen VI (party count/compaction not verified); Move is box-to-box"};
    return cloneSupport(from);
}

Modern::OperationSupport Gen6NativeSession::removeSupport(Modern::SlotRef at) const {
    if (at.region != Modern::Region::Box)
        return {false, "Party removal is unsupported for Gen VI (party count/compaction not verified)"};
    if (!original(at).occupied() || stagedAt(at))
        return {false, "Remove needs an unmodified, occupied box record"};
    return {true, {}};
}

Modern::OperationSupport Gen6NativeSession::createSupport(Modern::SlotRef) const {
    return {false, Gen6StagedSession::createCapability().reason};
}

bool Gen6NativeSession::clone(Modern::SlotRef from, Modern::SlotRef to, std::string& error) {
    if (to.region != Modern::Region::Box) {
        error = "Gen VI clone destination must be a box slot";
        return false;
    }
    return session_.cloneToBox(toNative(from), to.box, to.slot, &error);
}

bool Gen6NativeSession::move(Modern::SlotRef from, Modern::SlotRef to, std::string& error) {
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

bool Gen6NativeSession::remove(Modern::SlotRef at, std::string& error) {
    if (at.region != Modern::Region::Box) {
        error = "Party removal is unsupported for Gen VI";
        return false;
    }
    return session_.removeFromBox(at.box, at.slot, &error);
}

bool Gen6NativeSession::bagAvailable() const {
    return decodeBag6(session_.source().family(), sourceItemsBlock6(session_.source())).has_value();
}

std::vector<Modern::BagPocketView> Gen6NativeSession::bag() const {
    std::vector<Modern::BagPocketView> out;
    const auto f = session_.source().family();
    std::optional<ValidatedBag6> decoded;
    if (const auto* staged = const_cast<Gen6StagedSession&>(session_).bag()) decoded = staged->current();
    else decoded = decodeBag6(f, sourceItemsBlock6(session_.source()));
    if (!decoded) return out;
    for (size_t p = 0; p < BagPocket6Count; ++p) {
        Modern::BagPocketView view;
        view.name = bagPocketName6(p);
        view.maxQuantity = bagPocketLayout6(f, p).maxCount;
        const auto kind = static_cast<BagPocket6>(p);
        view.editable = kind == BagPocket6::Items || kind == BagPocket6::Medicine || kind == BagPocket6::Berries;
        if (!view.editable) view.reason = "Key Items and TMs/HMs are story pockets; shown read-only";
        for (const auto& item : (*decoded)[p]) view.items.push_back({item.itemId, item.count, false, false});
        out.push_back(std::move(view));
    }
    return out;
}

std::vector<uint16_t> Gen6NativeSession::bagAddChoices(size_t pocket) const {
    std::vector<uint16_t> ids;
    const auto f = session_.source().family();
    if (pocket >= BagPocket6Count) return ids;
    for (const uint16_t id : bagPocketItems6(f, pocket))
        if (gen6BagItemAddable(f, pocket, id)) ids.push_back(id);
    return ids;
}

bool Gen6NativeSession::bagSetQuantity(size_t pocket, size_t row, uint16_t quantity, std::string& error) {
    auto* b = session_.bag(&error);
    return b && b->setQuantity(pocket, row, quantity, &error);
}
bool Gen6NativeSession::bagRemove(size_t pocket, size_t row, std::string& error) {
    auto* b = session_.bag(&error);
    return b && b->remove(pocket, row, &error);
}
bool Gen6NativeSession::bagAdd(size_t pocket, uint16_t itemId, uint16_t quantity, std::string& error) {
    auto* b = session_.bag(&error);
    return b && b->add(pocket, itemId, quantity, &error);
}
void Gen6NativeSession::discardBag() {
    if (auto* b = session_.bag()) b->discard();
}

std::vector<Modern::TrainerFact> Gen6NativeSession::trainerFacts() const {
    const auto t = session_.source().trainer();
    std::vector<Modern::TrainerFact> facts;
    facts.push_back({"Name", trainerName()});
    facts.push_back({"Game", gameTitle(session_.source().exactGameId())});
    facts.push_back({"Trainer ID", std::to_string(t.tid)});
    facts.push_back({"Secret ID", std::to_string(t.sid)});
    facts.push_back({"Gender", t.gender == 0 ? "Male" : t.gender == 1 ? "Female" : "Unknown"});
    facts.push_back({"Language", Enums::getLanguageName(t.language)});
    facts.push_back({"Play Time", std::to_string(t.playedHours) + "h " + std::to_string(t.playedMinutes) + "m " +
                                  std::to_string(t.playedSeconds) + "s"});
    facts.push_back({"Boxes unlocked", std::to_string(Readouts::unlockedBoxes(session_.source()))});
    return facts;
}

std::string Gen6NativeSession::trainerName() const {
    const auto name = Utils::utf16ToUtf8(session_.source().trainer().rawName);
    return name.empty() ? std::string("Unknown") : name;
}

uint8_t Gen6NativeSession::trainerGender() const noexcept {
    const auto g = session_.source().trainer().gender;
    return g <= 1 ? g : 0;
}

Modern::DexProgress Gen6NativeSession::pokedex() const {
    const auto dex = Readouts::dexProgress(session_.source());
    return {dex.seen, dex.caught, dex.total};
}

size_t Gen6NativeSession::changeCount() const noexcept {
    return session_.changeCount() + (session_.bagDirty() ? 1 : 0);
}

std::vector<Modern::ReviewRow> Gen6NativeSession::reviewRows() const {
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
    if (session_.bagDirty()) {
        if (const auto* b = const_cast<Gen6StagedSession&>(session_).bag()) {
            const auto f = session_.source().family();
            const auto before = decodeBag6(f, b->original());
            const auto after = decodeBag6(f, b->staged());
            if (before && after) {
                for (size_t p = 0; p < BagPocket6Count; ++p) {
                    std::map<uint16_t, uint16_t> was, now;
                    for (const auto& i : (*before)[p]) was[i.itemId] = i.count;
                    for (const auto& i : (*after)[p]) now[i.itemId] = i.count;
                    for (const auto& [id, count] : now)
                        if (!was.count(id) || was[id] != count)
                            rows.push_back({std::string("Bag / ") + bagPocketName6(p) + " / item " + std::to_string(id),
                                            std::to_string(was.count(id) ? was[id] : 0) + " -> " + std::to_string(count)});
                    for (const auto& [id, count] : was)
                        if (!now.count(id))
                            rows.push_back({std::string("Bag / ") + bagPocketName6(p) + " / item " + std::to_string(id),
                                            std::to_string(count) + " -> 0"});
                }
            }
        }
    }
    return rows;
}

Modern::OperationSupport Gen6NativeSession::exportSupport() const {
    if (!session_.hasChanges()) return {false, "Export Copy needs at least one staged change"};
    return {true, {}};
}

Common::VerifiedExportResult Gen6NativeSession::exportWorkingCopy(const Modern::ExportRequest& request) const {
    Gen6ExportRequest r;
    r.rootDirectory = request.rootDirectory;
    r.sourcePath = request.sourcePath;
    r.timestamp = request.timestamp;
    r.applicationSha = request.applicationSha;
    return session_.exportWorkingCopy(r);
}

} // namespace PokeVault::Integration::Gen6
