#include "Integration/Gen7/Gen7NativeSession.h"

#include "Enums/GameVersion.h"
#include "Enums/LanguageID.h"
#include "Integration/Gen7/Gen7Readouts.h"
#include "Integration/Gen7/Gen7Stats.h"
#include "Pokemon/PokemonTypes.h"
#include "Utils/StringHelpers.h"

#include <algorithm>
#include <map>
#include <utility>

namespace PokeVault::Integration::Gen7 {
namespace {

std::string slotLabel(Modern::SlotRef s) {
    if (s.region == Modern::Region::Party) return "Party " + std::to_string(s.slot + 1);
    return "Box " + std::to_string(s.box + 1) + " / Slot " + std::to_string(s.slot + 1);
}

constexpr RecordRegion nativeRegion(Modern::Region r) noexcept {
    return r == Modern::Region::Party ? RecordRegion::Party : RecordRegion::Box;
}

std::string gameTitle(std::string_view id) {
    if (id == "sun_3ds") return "Sun";
    if (id == "moon_3ds") return "Moon";
    if (id == "ultrasun_3ds") return "Ultra Sun";
    if (id == "ultramoon_3ds") return "Ultra Moon";
    return std::string(id);
}

// Pockets whose stacks the staged bag may change; Key Items, TMs and Z-Crystals are story
// pockets (Gen7StagedBag refuses them) and the Roto Powers pocket exists in USUM only.
constexpr bool editablePocket(BagPocket7 p) noexcept {
    return p == BagPocket7::Items || p == BagPocket7::Medicine || p == BagPocket7::Berries ||
           p == BagPocket7::BattleItems;
}

constexpr std::array<const char*, 6> StatNames = {"HP", "Atk", "Def", "Spe", "SpA", "SpD"};

} // namespace

Modern::OpenedSession Gen7NativeSession::open(std::span<const uint8_t> bytes, std::string_view assignedExactGame) {
    Modern::OpenedSession out;
    std::string error;
    auto save = Gen7Save::parse(bytes, assignedExactGame, &error);  // SignaturePolicy::Require
    if (!save) {
        out.diagnostic = error.empty() ? "Gen VII save failed strict validation" : error;
        return out;
    }
    if (!assignedExactGame.empty() && save->exactGameId() != assignedExactGame) {
        out.diagnostic = "Validated Gen VII title differs from the assigned game; reassign explicitly";
        return out;
    }
    out.session = std::make_unique<Gen7NativeSession>(std::move(*save));
    return out;
}

Gen7NativeSession::Gen7NativeSession(Gen7Save source)
    : session_(std::move(source)), capabilities_(Capability::find(session_.source().exactGameId())) {
    // Every Gen VII exact id is registered; a missing row would be a programming error that the
    // registry test catches, so fall back to the Sun row rather than dereferencing null.
    if (!capabilities_) capabilities_ = Capability::find("sun_3ds");
}

Enums::GameVersion Gen7NativeSession::gameGroup() const noexcept {
    return session_.source().family() == Layout::Family::SunMoon ? Enums::GameVersion::SM : Enums::GameVersion::USUM;
}

std::string Gen7NativeSession::boxName(size_t box) const {
    if (const auto name = Readouts::boxName(session_.source(), box); name && !name->empty()) {
        // A native name made only of unpaired surrogates decodes to nothing; never show a blank label.
        if (auto utf8 = Utils::utf16ToUtf8(*name); !utf8.empty()) return utf8;
    }
    return "Box " + std::to_string(box + 1);
}

SlotRecord Gen7NativeSession::original(Modern::SlotRef slot) const {
    const auto n = toNative(slot);
    if (!n.canonical()) return {};
    return n.region == RecordRegion::Party ? session_.source().party(n.slot)
                                           : session_.source().box(n.box, n.slot);
}

std::optional<Modern::RecordView> Gen7NativeSession::view(Modern::SlotRef slot) const {
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
    // PK7 = Gen6::PK6::Pokemon6: every accessor below sits at the same offset in PKHeX PK7.cs.
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
    } else if (const auto s = Stats::compute(*mon)) {  // hyper-training aware (Gen7Stats.h)
        v.stats = *s;
        v.currentHp = v.stats[0];
    }
    for (size_t i = 0; i < 4; ++i) {
        v.moves[i] = mon->move(i);
        v.pp[i] = mon->movePP(i);
        v.ppUps[i] = mon->movePPUps(i);
        v.relearnMoves[i] = PK7::read16(mon->plain(), 0x6A + 2 * i);  // PK7.cs RelearnMove1..4 0x6A..0x70
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
                 mon->plain()[0xD2], mon->plain()[0xD3]};  // PK7.cs EggMonth 0xD2, EggDay 0xD3
    const auto types = Pokemon::getPokemonTypes(v.species, v.form, gameGroup());
    v.types = {static_cast<uint8_t>(types.type1), static_cast<uint8_t>(types.type2)};
    // Gen VII-native facts, read-only in the shared editor (no verified writer exists for them).
    v.nativeFacts.push_back({"Current handler", mon->currentHandler() == 0 ? "Original Trainer" : "Handling Trainer"});
    v.nativeFacts.push_back({"Ability slot", std::to_string(mon->abilityNumber())});
    v.nativeFacts.push_back({"Encryption constant", std::to_string(mon->encryptionConstant())});
    // Hyper Training flags (PK7.cs HyperTrainFlags 0xDE, read through Gen7Stats::hyperTrained).
    std::string hyper;
    for (size_t i = 0; i < 6; ++i)
        if (Stats::hyperTrained(*mon, i)) hyper += (hyper.empty() ? "" : ", ") + std::string(StatNames[i]);
    v.nativeFacts.push_back({"Hyper training", hyper.empty() ? std::string("None") : hyper});
    return v;
}

bool Gen7NativeSession::originalOccupied(Modern::SlotRef slot) const {
    return original(slot).occupied();
}

bool Gen7NativeSession::emptyDestination(Modern::SlotRef slot) const {
    if (slot.region != Modern::Region::Box) return false;
    // The same "verified blank" Gen7StagedSession::cloneToBox demands: decodes, species 0, all-zero plain.
    const auto rec = original(slot);
    return rec.state == SlotState::Empty && rec.pokemon && rec.pokemon->allZero() && !stagedAt(slot);
}

bool Gen7NativeSession::stagedAt(Modern::SlotRef slot) const {
    return session_.isStaged(toNative(slot));
}

std::optional<Modern::SlotRef> Gen7NativeSession::firstEmptyBoxSlot() const {
    for (size_t b = 0; b < Layout::BoxCount; ++b)
        for (size_t s = 0; s < Layout::BoxSlots; ++s)
            if (const auto at = Modern::SlotRef::boxed(b, s); emptyDestination(at)) return at;
    return std::nullopt;
}

Modern::FieldState Gen7NativeSession::fieldState(Modern::Field field, Modern::Region region) const noexcept {
    return Gen7EditorProvider::fieldState(field, nativeRegion(region));
}

bool Gen7NativeSession::stageValue(Modern::SlotRef slot, Modern::Field field, uint32_t value, size_t index,
                                   std::string& error) {
    return session_.stageValue(toNative(slot), field, value, index, &error);
}

bool Gen7NativeSession::stageText(Modern::SlotRef slot, Modern::Field field, std::string_view utf8, std::string& error) {
    return session_.stageText(toNative(slot), field, utf8, &error);
}

uint32_t Gen7NativeSession::fieldMaximum(Modern::Field field) const noexcept {
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

std::vector<uint16_t> Gen7NativeSession::heldItemChoices() const {
    std::vector<uint16_t> ids{0};
    const auto f = session_.source().family();
    const auto lim = session_.limits();
    for (const size_t pocket : {static_cast<size_t>(BagPocket7::Items), static_cast<size_t>(BagPocket7::Medicine),
                                static_cast<size_t>(BagPocket7::Berries)})
        for (const uint16_t id : bagPocketItems7(f, pocket))
            if (id != 0 && id <= lim.maxHeldItem && gen7BagItemAddable(f, pocket, id) &&
                std::find(ids.begin(), ids.end(), id) == ids.end())
                ids.push_back(id);
    return ids;
}

std::vector<uint16_t> Gen7NativeSession::ballChoices() const {
    std::vector<uint16_t> balls;
    for (uint16_t b = 1; b <= session_.limits().maxBall; ++b) balls.push_back(b);
    return balls;
}

int Gen7NativeSession::maxPP(uint16_t, uint8_t) const noexcept {
    // Moves are read-only for Gen VII (no pinned SM/USUM PP table in this build); 0 = unknown.
    return 0;
}

bool Gen7NativeSession::beginDraft(Modern::SlotRef slot, std::string& error) {
    const auto n = toNative(slot);
    if (!n.canonical()) {
        error = "Gen VII draft target is not a canonical slot";
        return false;
    }
    const auto mon = session_.current(n);
    if (!mon || mon->empty()) {
        error = "Gen VII draft requires a valid occupied record";
        return false;
    }
    if (draft_) discardDraft();  // a stale bracket is never silently kept
    draftSnapshot_ = session_.snapshotEntry(n);
    draft_ = slot;
    error.clear();
    return true;
}

bool Gen7NativeSession::draftDirty() const noexcept {
    return draft_ && !session_.entryMatches(toNative(*draft_), draftSnapshot_);
}

void Gen7NativeSession::discardDraft() {
    if (!draft_) return;
    session_.restoreEntry(toNative(*draft_), draftSnapshot_);
    draft_.reset();
}

Modern::OperationSupport Gen7NativeSession::cloneSupport(Modern::SlotRef from) const {
    const auto rec = original(from);
    if (!rec.occupied() || stagedAt(from))
        return {false, "Clone needs an unmodified, occupied source record"};
    if (!firstEmptyBoxSlot()) return {false, "No verified blank box slot is available"};
    return {true, {}};
}

Modern::OperationSupport Gen7NativeSession::moveSupport(Modern::SlotRef from) const {
    if (from.region != Modern::Region::Box)
        return {false, "Party removal is unsupported for Gen VII (party count/compaction not verified); Move is box-to-box"};
    return cloneSupport(from);
}

Modern::OperationSupport Gen7NativeSession::removeSupport(Modern::SlotRef at) const {
    if (at.region != Modern::Region::Box)
        return {false, "Party removal is unsupported for Gen VII (party count/compaction not verified)"};
    if (!original(at).occupied() || stagedAt(at))
        return {false, "Remove needs an unmodified, occupied box record"};
    return {true, {}};
}

Modern::OperationSupport Gen7NativeSession::createSupport(Modern::SlotRef) const {
    // Gen7StagedSession has no Create path; the registry row names the lane's prerequisite.
    return {false, capabilities_->status(Capability::Workflow::Create).reason};
}

bool Gen7NativeSession::clone(Modern::SlotRef from, Modern::SlotRef to, std::string& error) {
    if (draft_ && (*draft_ == from || *draft_ == to)) {
        error = "Gen VII slot has an open editor draft; keep or discard it before Clone";
        return false;
    }
    if (to.region != Modern::Region::Box) {
        error = "Gen VII clone destination must be a box slot";
        return false;
    }
    return session_.cloneToBox(toNative(from), to.box, to.slot, &error);
}

bool Gen7NativeSession::move(Modern::SlotRef from, Modern::SlotRef to, std::string& error) {
    if (draft_ && (*draft_ == from || *draft_ == to)) {
        error = "Gen VII slot has an open editor draft; keep or discard it before Move";
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

bool Gen7NativeSession::remove(Modern::SlotRef at, std::string& error) {
    if (draft_ && *draft_ == at) {
        error = "Gen VII slot has an open editor draft; keep or discard it before Remove";
        return false;
    }
    if (at.region != Modern::Region::Box) {
        error = "Party removal is unsupported for Gen VII";
        return false;
    }
    return session_.removeFromBox(at.box, at.slot, &error);
}

bool Gen7NativeSession::bagAvailable() const {
    return decodeBag7(session_.source().family(), sourceItemsBlock7(session_.source())).has_value();
}

std::vector<Modern::BagPocketView> Gen7NativeSession::bag() const {
    std::vector<Modern::BagPocketView> out;
    const auto f = session_.source().family();
    std::optional<ValidatedBag7> decoded;
    if (const auto* staged = session_.stagedBag()) decoded = staged->current();
    else decoded = decodeBag7(f, sourceItemsBlock7(session_.source()));
    if (!decoded) return out;
    for (size_t p = 0; p < BagPocket7Count; ++p) {
        const auto layout = bagPocketLayout7(f, p);
        if (layout.slots == 0) continue;  // Roto Powers: USUM only; it is the last native pocket
        Modern::BagPocketView view;
        view.name = bagPocketName7(p);
        view.maxQuantity = layout.maxCount;
        view.editable = editablePocket(static_cast<BagPocket7>(p));
        if (!view.editable) view.reason = "Key Items, TMs and Z-Crystals are story pockets; shown read-only";
        for (const auto& item : (*decoded)[p]) view.items.push_back({item.itemId, item.count, false, false});
        out.push_back(std::move(view));
    }
    return out;
}

std::vector<uint16_t> Gen7NativeSession::bagAddChoices(size_t pocket) const {
    std::vector<uint16_t> ids;
    const auto f = session_.source().family();
    if (pocket >= BagPocket7Count) return ids;
    for (const uint16_t id : bagPocketItems7(f, pocket))
        if (gen7BagItemAddable(f, pocket, id)) ids.push_back(id);
    return ids;
}

bool Gen7NativeSession::bagSetQuantity(size_t pocket, size_t row, uint16_t quantity, std::string& error) {
    auto* b = session_.bag(&error);
    return b && b->setQuantity(pocket, row, quantity, &error);
}
bool Gen7NativeSession::bagRemove(size_t pocket, size_t row, std::string& error) {
    auto* b = session_.bag(&error);
    return b && b->remove(pocket, row, &error);
}
bool Gen7NativeSession::bagAdd(size_t pocket, uint16_t itemId, uint16_t quantity, std::string& error) {
    auto* b = session_.bag(&error);
    return b && b->add(pocket, itemId, quantity, &error);
}
void Gen7NativeSession::discardBag() {
    if (auto* b = session_.bag()) b->discard();
}

std::vector<Modern::TrainerFact> Gen7NativeSession::trainerFacts() const {
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

std::string Gen7NativeSession::trainerName() const {
    const auto name = Utils::utf16ToUtf8(session_.source().trainer().rawName);
    return name.empty() ? std::string("Unknown") : name;
}

uint8_t Gen7NativeSession::trainerGender() const noexcept {
    // MyStatus +5, the byte Gen7Save::trainer() reads; no std::u16string is built in a noexcept member.
    const auto g = session_.source().bytes()[myStatusOffset() + 5];
    return g <= 1 ? g : 0;
}

Modern::DexProgress Gen7NativeSession::pokedex() const {
    const auto dex = Readouts::dexProgress(session_.source());
    return {dex.seen, dex.caught, dex.total};
}

size_t Gen7NativeSession::changeCount() const noexcept {
    return session_.changeCount() + (session_.bagDirty() ? 1 : 0);
}

std::vector<Modern::ReviewRow> Gen7NativeSession::reviewRows() const {
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
        if (const auto* b = session_.stagedBag()) {
            const auto f = session_.source().family();
            const auto before = decodeBag7(f, b->original());
            const auto after = decodeBag7(f, b->staged());
            if (before && after) {
                for (size_t p = 0; p < BagPocket7Count; ++p) {
                    std::map<uint16_t, uint16_t> was, now;
                    for (const auto& i : (*before)[p]) was[i.itemId] = i.count;
                    for (const auto& i : (*after)[p]) now[i.itemId] = i.count;
                    for (const auto& [id, count] : now)
                        if (!was.count(id) || was[id] != count)
                            rows.push_back({std::string("Bag / ") + bagPocketName7(p) + " / item " + std::to_string(id),
                                            std::to_string(was.count(id) ? was[id] : 0) + " -> " + std::to_string(count)});
                    for (const auto& [id, count] : was)
                        if (!now.count(id))
                            rows.push_back({std::string("Bag / ") + bagPocketName7(p) + " / item " + std::to_string(id),
                                            std::to_string(count) + " -> 0"});
                }
            }
        }
    }
    return rows;
}

void Gen7NativeSession::discardAll() {
    draft_.reset();
    for (const auto& change : session_.changes()) (void)session_.undo(change.slot);
    if (session_.bagDirty()) discardBag();
}

Modern::OperationSupport Gen7NativeSession::exportSupport() const {
    // Blocked whether or not anything is staged: Gen7StagedSession has no signed working copy and
    // the registry row for this title names why (SAV7 MemeCrypto re-signing lacks evidence).
    return {false, capabilities_->status(Capability::Workflow::Export).reason};
}

Common::VerifiedExportResult Gen7NativeSession::exportWorkingCopy(const Modern::ExportRequest& request) const {
    // Refuse before touching the filesystem: no directory, no backup, no edited copy, no manifest.
    (void)request;
    Common::VerifiedExportResult result;
    result.success = false;
    result.error = std::string(exportSupport().reason);
    return result;
}

} // namespace PokeVault::Integration::Gen7
