/**
 * Gen4LegalityMoves.cpp - evolution chains and move sources for PK4.
 *
 * Ports of PKHeX.Core (pinned in Gen4LegalityData.h): EvolutionTree.Evolves4 (reverse
 * lineage and forward methods), EvolutionMethod.Check, EvolutionGroup4,
 * EvolutionChain.GetEvolutionChainsAllGens, EvolutionUtil, LearnVerifier,
 * LearnVerifierHistory, LearnVerifierEgg, LearnVerifierRelearn.VerifyEggMoveset,
 * LearnGroup4, LearnSource4DP/Pt/HGSS, MoveBreed / MoveBreed4 and
 * EvolutionRestrictions.IsValidEvolutionWithMove.
 */
#include "Gen4LegalityInternal.h"

#include <algorithm>
#include <mutex>

namespace Legality::Gen4 {

namespace {
// EvolutionType values used by EvolutionMethod.Check
enum : uint8_t {
    kEvoTrade = 5, kEvoTradeHeldItem = 6, kEvoLevelUpBeauty = 16, kEvoUseItemMale = 17, kEvoUseItemFemale = 18,
    kEvoLevelUpMale = 23, kEvoLevelUpFemale = 24, kEvoLevelUpFormFemale1 = 34,
};
enum : uint16_t {
    kMoveWhirlpool = 250, kMoveDefog = 432, kMoveVoltTackle = 344, kMoveBlastBurn = 307, kMoveHydroCannon = 308,
    kMoveFrenzyPlant = 338, kMoveDracoMeteor = 434, kMoveOverheat = 315, kMoveHydroPump = 56, kMoveBlizzard = 59,
    kMoveAirSlash = 403, kMoveLeafStorm = 437, kMoveMimic = 102, kMoveDoubleHit = 458, kMoveRollout = 205,
    kMoveAncientPower = 246, kMoveStruggle = 165, kMoveChatter = 448,
};

std::span<const D::Evolution> evolutionsOf(uint16_t species) noexcept {
    if (species >= D::kEvolutionCount) return {};
    return {D::kEvolutions + D::kEvolutionStart[species], D::kEvolutions + D::kEvolutionStart[species + 1]};
}

// --------------------------------------------------------------- reverse lineage (Evolves4)
struct Link { uint16_t species = 0; uint8_t form = 0; const D::Evolution* method = nullptr; };
struct Node { Link first, second; };
constexpr std::size_t kMaxForms = 32;

const std::vector<Node>& lineage() {
    static const std::vector<Node> nodes = [] {
        std::vector<Node> n(D::kEvolutionCount * kMaxForms);
        for (uint16_t s = 1; s <= kMaxSpecies4; ++s) {
            const uint8_t fc = personalAt(Table::DP, s).formCount();
            for (uint8_t f = 0; f < fc; ++f) {
                for (const auto& evo : evolutionsOf(s)) {
                    if (evo.species == 0) break;
                    const uint8_t dForm = evo.form == 255 ? f : evo.form;
                    if (evo.species >= D::kEvolutionCount || dForm >= kMaxForms) continue;
                    Node& node = n[evo.species * kMaxForms + dForm];
                    const Link link{s, f, &evo};
                    if (node.first.species == 0) node.first = link;
                    else if (node.second.species == 0) node.second = link;
                }
            }
        }
        return n;
    }();
    return nodes;
}

const Node* nodeOf(uint16_t species, uint8_t form) noexcept {
    if (species >= D::kEvolutionCount || form >= kMaxForms) return nullptr;
    return &lineage()[species * kMaxForms + form];
}

enum class EvoCheck : uint8_t { Valid, Insufficient, Bad };

EvoCheck checkMethod(const D::Evolution& m, const Pk4& pk, uint8_t lvl, uint8_t levelMin) noexcept {
    if (m.levelUp == 0) {
        // ValidNotLevelUp: PK4.IsUntraded is false, so trade evolutions always pass.
        if (m.type == kEvoUseItemMale) return pk.gender == 0 ? EvoCheck::Valid : EvoCheck::Bad;
        if (m.type == kEvoUseItemFemale) return pk.gender == 1 ? EvoCheck::Valid : EvoCheck::Bad;
        return EvoCheck::Valid;
    }
    switch (m.type) {
    case kEvoLevelUpMale: if (pk.gender != 0) return EvoCheck::Bad; break;
    case kEvoLevelUpFemale: if (pk.gender != 1) return EvoCheck::Bad; break;
    case kEvoLevelUpFormFemale1: if (pk.gender != 1 || pk.form != 1) return EvoCheck::Bad; break;
    case kEvoLevelUpBeauty: if (pk.contest[1] < m.argument) return EvoCheck::Bad; break;
    default: break;
    }
    if (lvl < m.level) return EvoCheck::Insufficient;
    if (m.levelUp == 0) return EvoCheck::Valid;
    if (m.level == 0 && lvl < 2) return EvoCheck::Insufficient;
    if (lvl < levelMin + m.levelUp) return EvoCheck::Insufficient;
    return EvoCheck::Valid;
}

bool tryDevolve(const Evo& head, const Pk4& pk, uint8_t currentMax, uint8_t levelMin, Evo& out) noexcept {
    const Node* node = nodeOf(head.species, head.form);
    if (!node) return false;
    for (const Link* link : {&node->first, &node->second}) {
        if (link->species == 0) return false;
        if (checkMethod(*link->method, pk, currentMax, levelMin) != EvoCheck::Valid) continue;
        out = Evo{link->species, link->form, link->method->levelUp, currentMax, link->method->level, link->method->type};
        return true;
    }
    return false;
}

bool tryEvolve(const Evo& head, const Evo& next, const Pk4& pk, uint8_t currentMax, uint8_t levelMin, Evo& out) noexcept {
    for (const auto& m : evolutionsOf(head.species)) {
        if (m.species != next.species) continue;
        const uint8_t expectForm = m.form == 255 ? head.form : m.form;
        if (next.form != expectForm) continue;
        if (checkMethod(m, pk, currentMax, levelMin) != EvoCheck::Valid) continue;
        out = Evo{next.species, next.form, m.levelUp, currentMax,
                  std::max(static_cast<uint8_t>(levelMin + m.levelUp), m.level), m.type};
        return true;
    }
    return false;
}

bool isBetterDevolution(const Evo& a, const Evo& ref) noexcept {
    if (ref.species == 0) return true;
    return a.levelMin + a.levelUpRequired < ref.levelMin + ref.levelUpRequired;
}
bool isBetterEvolution(const Evo& a, const Evo& ref) noexcept {
    if (ref.method == kSentinel) return true;
    if (a.levelMin + a.levelUpRequired > ref.levelMin + ref.levelUpRequired) return false;
    return a.levelMax > ref.levelMax;
}

void cleanDevolve(std::span<Evo> result, uint8_t levelMin) noexcept {
    uint8_t req = 0, method = 0;
    for (std::size_t k = result.size(); k-- > 0;) {
        Evo& evo = result[k];
        const uint8_t nextMin = evo.levelMin, nextReq = evo.levelUpRequired, nextMethod = evo.method;
        evo.levelMin = std::min(evo.levelMax, levelMin);
        evo.levelUpRequired = req;
        evo.method = method;
        levelMin = std::max(nextMin, levelMin);
        req = nextReq;
        method = nextMethod;
    }
}

std::vector<Evo> setHistory(std::span<const Evo> result) {
    std::size_t start = 0, count = 0;
    for (const auto& evo : result) {
        if (evo.method == kSentinel) {
            start += count + 1;
            count = 0;
        } else if (isPresentInGame(Table::HGSS, evo.species, evo.form)) {
            count++;
        } else if (count == 0) {
            start++;
        } else {
            break;
        }
    }
    if (count == 0 || start >= result.size()) return {};
    std::vector<Evo> arr(result.begin() + static_cast<std::ptrdiff_t>(start),
                         result.begin() + static_cast<std::ptrdiff_t>(std::min(result.size(), start + count)));
    // CleanEvolve
    std::size_t i = 1;
    for (; i < arr.size(); i++) {
        const Evo next = arr[i - 1];
        const int newMax = next.levelMax;
        Evo& evo = arr[i];
        if (evo.levelMin > newMax - next.levelUpRequired) break;
        evo.levelMax = static_cast<uint8_t>(newMax);
    }
    arr.resize(i);
    return arr;
}
} // namespace

std::vector<Evo> evolutionHistory(const Pk4& pk, const EncInfo& enc, uint8_t currentLevel) {
    const uint8_t min = std::max(pk.metLevel, enc.levelMin);
    const uint8_t max = currentLevel;
    std::array<Evo, 3> chain{};
    std::size_t length = 0;
    if (pk.isEgg) {
        chain[0] = Evo{pk.species, pk.form, 0, max, max, 0};
        length = 1;
    } else {
        chain[0] = Evo{pk.species, pk.form, 0, max, 0, 0};
        for (std::size_t i = 1; i < chain.size(); ++i) {
            const Evo prev = chain[i - 1];
            Evo evo{};
            if (!tryDevolve(prev, pk, prev.levelMax, min, evo)) continue;
            if (isBetterDevolution(evo, chain[i])) chain[i] = evo;
        }
        // encSpecies != 0: length is IndexOf(encSpecies) + 1
        std::size_t idx = chain.size();
        for (std::size_t i = 0; i < chain.size(); ++i) if (chain[i].species == enc.species) { idx = i; break; }
        length = idx == chain.size() ? 0 : idx + 1;
        if (length == 0) return {};
        cleanDevolve(std::span<Evo>(chain.data(), length), min);
    }
    // When the record is an egg, GetOriginChain returns the single entry without devolving;
    // IndexOf(encSpecies) still bounds it.
    if (pk.isEgg) {
        if (chain[0].species != enc.species) return {};
    }
    std::span<Evo> c(chain.data(), length);
    for (auto& evo : c) {
        if (evo.species == enc.species) break;
        evo.method = kSentinel;
    }
    for (std::size_t i = length - 1; i >= 1; --i) {
        Evo& dest = c[i - 1];
        const Evo devolved = c[i];
        Evo evo{};
        if (!tryEvolve(devolved, dest, pk, max, devolved.levelMin, evo)) {
            if (dest.method == kSentinel) break;
            continue;
        }
        if (isBetterEvolution(evo, dest)) dest = evo;
    }
    return setHistory(c);
}

std::vector<Evo> originChain(const Pk4& pk, uint8_t currentLevel) {
    const uint8_t max = currentLevel;
    const uint8_t min = (pk.isEgg || pk.eggLocation() != 0) ? 1 : pk.metLevel;
    if (pk.isEgg) return {Evo{pk.species, pk.form, 0, max, max, 0}};
    std::array<Evo, 3> r{};
    r[0] = Evo{pk.species, pk.form, 0, max, 0, 0};
    for (std::size_t i = 1; i < r.size(); ++i) {
        const Evo prev = r[i - 1];
        Evo evo{};
        if (!tryDevolve(prev, pk, prev.levelMax, min, evo)) continue;
        if (isBetterDevolution(evo, r[i])) r[i] = evo;
    }
    // EvolutionUtil.Discard against PersonalTable.HGSS
    for (int i = 0; i < static_cast<int>(r.size()); ++i) {
        if (r[static_cast<std::size_t>(i)].species == 0) break;
        if (isPresentInGame(Table::HGSS, r[static_cast<std::size_t>(i)].species, r[static_cast<std::size_t>(i)].form)) continue;
        for (std::size_t k = static_cast<std::size_t>(i) + 1; k < r.size(); ++k) r[k - 1] = r[k];
        r.back() = Evo{};
        --i; // re-check this index
    }
    std::size_t count = 0;
    while (count < r.size() && r[count].species != 0) ++count;
    cleanDevolve(std::span<Evo>(r.data(), count), min);
    return std::vector<Evo>(r.begin(), r.begin() + static_cast<std::ptrdiff_t>(count));
}

std::pair<uint16_t, uint8_t> baseSpeciesForm(uint16_t species, uint8_t form) noexcept {
    uint16_t s = species;
    uint8_t f = form;
    for (int guard = 0; guard < 8; ++guard) {
        const Node* node = nodeOf(s, f);
        if (!node || node->first.species == 0) break;
        s = node->first.species;
        f = node->first.form;
    }
    return {s, f};
}

bool hasVisited(const std::vector<Evo>& history, uint16_t species) noexcept {
    for (const auto& e : history) if (e.species == species) return true;
    return false;
}

// ======================================================================= learnsets
Learnset learnset(Table t, uint16_t species, uint8_t form) noexcept {
    const uint16_t idx = formIndex(t, species, form);
    const D::LevelMove* moves = t == Table::DP ? D::kLevelUpDP : (t == Table::Pt ? D::kLevelUpPt : D::kLevelUpHGSS);
    const uint16_t* start = t == Table::DP ? D::kLevelUpDPStart : (t == Table::Pt ? D::kLevelUpPtStart : D::kLevelUpHGSSStart);
    const std::size_t count = t == Table::DP ? D::kLevelUpDPCount : (t == Table::Pt ? D::kLevelUpPtCount : D::kLevelUpHGSSCount);
    if (idx >= count) return {};
    return {moves + start[idx], moves + start[idx + 1]};
}

bool canInheritMoves(uint16_t species) noexcept {
    return species < 494 && (D::kBreed[species].flags & 4) != 0;
}

namespace {
bool tryLevelLearn(const Learnset& ls, uint16_t move, uint8_t& level) noexcept {
    for (auto* m = ls.begin; m != ls.end; ++m) {
        if (m->move == move) { level = m->level; return true; }
    }
    level = 0;
    return false;
}
bool learnsetHas(const Learnset& ls, uint16_t move) noexcept {
    uint8_t level = 0;
    return tryLevelLearn(ls, move, level);
}
/// Learnset.GetBaseEggMoves
std::vector<uint16_t> baseEggMoves(const Learnset& ls, uint8_t level) {
    std::size_t count = 0;
    for (auto* m = ls.begin; m != ls.end; ++m) {
        if (m->level > level) break;
        count++;
    }
    std::vector<uint16_t> r;
    const std::size_t start = count > 4 ? count - 4 : 0;
    for (std::size_t i = start; i < count; ++i) r.push_back(ls.begin[i].move);
    return r;
}
std::span<const uint16_t> eggMoves(bool hgss, uint16_t species) noexcept {
    const uint16_t* data = hgss ? D::kEggMovesHGSS : D::kEggMovesDPPt;
    const uint16_t* start = hgss ? D::kEggMovesHGSSStart : D::kEggMovesDPPtStart;
    const std::size_t count = hgss ? D::kEggMovesHGSSCount : D::kEggMovesDPPtCount;
    if (species >= count) return {};
    return {data + start[species], data + start[species + 1]};
}
template <typename C>
int indexIn(const C& list, uint16_t move) noexcept {
    int i = 0;
    for (auto m : list) {
        if (m == move) return i;
        ++i;
    }
    return -1;
}
bool contains(std::span<const uint16_t> list, uint16_t move) noexcept { return indexIn(list, move) >= 0; }

bool isTM(const Personal& pi, uint16_t move) noexcept {
    const int index = indexIn(std::span<const uint16_t>(D::kMachineTM, 92), move);
    return index >= 0 && index < 92 && pi.tmhm(index);
}
bool isHM(const Personal& pi, uint16_t move, bool hgss) noexcept {
    const int index = indexIn(std::span<const uint16_t>(hgss ? D::kMachineHMHGSS : D::kMachineHMDPPt, 8), move);
    return index >= 0 && index < 8 && pi.tmhm(92 + index);
}
bool isTypeTutor(uint16_t species, uint16_t move) noexcept {
    static constexpr uint16_t kBlast[] = {6, 157, 257, 392};
    static constexpr uint16_t kHydro[] = {9, 160, 260, 395};
    static constexpr uint16_t kFrenzy[] = {3, 154, 254, 389};
    static constexpr uint16_t kDraco[] = {147, 148, 149, 230, 329, 330, 334, 371, 372, 373, 380, 381, 384, 443, 444, 445, 483, 484, 487};
    switch (move) {
    case kMoveBlastBurn: return std::find(std::begin(kBlast), std::end(kBlast), species) != std::end(kBlast);
    case kMoveHydroCannon: return std::find(std::begin(kHydro), std::end(kHydro), species) != std::end(kHydro);
    case kMoveFrenzyPlant: return std::find(std::begin(kFrenzy), std::end(kFrenzy), species) != std::end(kFrenzy);
    case kMoveDracoMeteor: return std::find(std::begin(kDraco), std::end(kDraco), species) != std::end(kDraco);
    default: return false;
    }
}
bool isSpecialTutorHGSS(uint16_t species, uint8_t form, uint16_t move) noexcept {
    const int index = indexIn(std::span<const uint16_t>(D::kTutorMoves, 52), move);
    if (index < 0) return false;
    const uint16_t record = formIndex(Table::HGSS, species, form);
    if (record >= D::kPersonalHGSSCount) return false;
    return ((D::kTutorBitsHGSS[record * 7 + static_cast<std::size_t>(index >> 3)] >> (index & 7)) & 1) != 0;
}
bool isEnhancedTutor(uint16_t evoSpecies, const Pk4& pk, uint16_t move, bool past) noexcept {
    if (evoSpecies != kRotom) return false;
    switch (move) {
    case kMoveOverheat: return past || pk.form == 1;
    case kMoveHydroPump: return past || pk.form == 2;
    case kMoveBlizzard: return past || pk.form == 3;
    case kMoveAirSlash: return past || pk.form == 4;
    case kMoveLeafStorm: return past || pk.form == 5;
    default: return false;
    }
}

/// LearnSource4HGSS.GetCanLearn (all sources) / Pt (level-up and machines) / DP (level-up).
LearnMethod canLearnHGSS(const Pk4& pk, const Evo& evo, uint16_t move, bool past) noexcept {
    uint8_t level = 0;
    if (tryLevelLearn(learnset(Table::HGSS, evo.species, evo.form), move, level) && level <= evo.levelMax)
        return LearnMethod::LevelUp;
    const Personal pi = personal(Table::HGSS, evo.species, evo.form);
    if (isTM(pi, move)) return LearnMethod::TMHM;
    if (isHM(pi, move, true)) return LearnMethod::TMHM; // format 4: every HM counts
    if (isTypeTutor(evo.species, move)) return LearnMethod::Tutor;
    if (isSpecialTutorHGSS(evo.species, evo.form, move)) return LearnMethod::Tutor;
    if (isEnhancedTutor(evo.species, pk, move, past)) return LearnMethod::Tutor;
    return LearnMethod::None;
}
LearnMethod canLearnPt(const Evo& evo, uint16_t move) noexcept {
    uint8_t level = 0;
    if (tryLevelLearn(learnset(Table::Pt, evo.species, evo.form), move, level) && level <= evo.levelMax)
        return LearnMethod::LevelUp;
    const Personal pi = personal(Table::Pt, evo.species, evo.form);
    if (isTM(pi, move)) return LearnMethod::TMHM;
    if (isHM(pi, move, false)) return LearnMethod::TMHM;
    return LearnMethod::None;
}
LearnMethod canLearnDP(const Evo& evo, uint16_t move) noexcept {
    uint8_t level = 0;
    if (tryLevelLearn(learnset(Table::DP, evo.species, evo.form), move, level) && level <= evo.levelMax)
        return LearnMethod::LevelUp;
    return LearnMethod::None;
}

void checkInternal(MoveResults& result, const std::array<uint16_t, 4>& current, const Pk4& pk, const Evo& evo, bool past) noexcept {
    if (evo.species > kMaxSpecies4) return;
    for (int i = 3; i >= 0; --i) {
        auto& r = result[static_cast<std::size_t>(i)];
        if (r.valid()) continue;
        const uint16_t move = current[static_cast<std::size_t>(i)];
        LearnMethod chk = canLearnHGSS(pk, evo, move, past);
        if (chk == LearnMethod::None) chk = canLearnPt(evo, move);
        if (chk == LearnMethod::None) chk = canLearnDP(evo, move);
        if (chk != LearnMethod::None) r = {chk, 0};
    }
}

bool allParsed(const MoveResults& r) noexcept {
    for (const auto& x : r) if (!x.parsed()) return false;
    return true;
}

/// LearnGroup4.Check
bool learnGroup4Check(MoveResults& result, const std::array<uint16_t, 4>& current, const Pk4& pk,
                      const std::vector<Evo>& evos, const EncInfo& enc, bool past) {
    for (const auto& evo : evos) {
        if (evo.species == kDeoxys || evo.species == kGiratina || evo.species == kShaymin) {
            if (evo.species > kMaxSpecies4) continue;
            const uint8_t fc = personal(Table::HGSS, evo.species, evo.form).formCount();
            for (uint8_t f = 0; f < fc; ++f) {
                Evo e = evo;
                e.form = f;
                checkInternal(result, current, pk, e, past);
            }
        } else {
            checkInternal(result, current, pk, evo, past);
        }
    }
    if (enc.kind == EncKind::Egg) {
        // CheckEncounterMoves: HG/SS (<= SS) use HG/SS data, Pt uses Pt, D/P use DP.
        const bool hgss = enc.version <= D::kSS;
        const Table t = hgss ? Table::HGSS : (enc.version == D::kPt ? Table::Pt : Table::DP);
        const auto eggs = eggMoves(hgss, enc.species);
        const Learnset inherit = canInheritMoves(enc.species) ? learnset(t, enc.species, enc.form) : Learnset{};
        for (int i = 3; i >= 0; --i) {
            auto& r = result[static_cast<std::size_t>(i)];
            if (r.valid()) continue;
            const uint16_t move = current[static_cast<std::size_t>(i)];
            if (contains(eggs, move)) r = {LearnMethod::EggMove, 0};
            else if (learnsetHas(inherit, move)) r = {LearnMethod::InheritLevelUp, 0};
            else if (move == kMoveVoltTackle && enc.species == kPichu) r = {LearnMethod::SpecialEgg, 0};
        }
    }
    if (enc.species == kNincada && evos.size() == 2 && evos[0].species == kShedinja && !allParsed(result)) {
        const Evo& nincada = evos.back();
        const Learnset ninjask = learnset(Table::Pt, kNinjask, 0);
        for (std::size_t i = 0; i < 4; ++i) {
            if (result[i].valid()) continue;
            const uint16_t move = current[i];
            if (move == 0) break;
            uint8_t level = 0;
            if (!tryLevelLearn(ninjask, move, level)) continue;
            if (!nincada.insideLevelRange(level)) continue;
            result[i] = {LearnMethod::ShedinjaEvo, 0};
            break;
        }
    }
    return allParsed(result);
}

/// LearnVerifierHistory.MarkAndIterate (LearnGroup4 has no previous group for a Gen IV encounter).
void markAndIterate(MoveResults& result, const std::array<uint16_t, 4>& current, const EncInfo& enc, const Pk4& pk,
                    const std::vector<Evo>& history, bool past) {
    if (enc.hasMoves()) {
        for (auto move : enc.moves) {
            if (move == 0) break;
            const int index = indexIn(current, move);
            if (index != -1) result[static_cast<std::size_t>(index)] = {LearnMethod::Initial, 0};
        }
    }
    if (learnGroup4Check(result, current, pk, history, enc, past)) return;
    for (auto& r : result) if (!r.parsed()) r = {LearnMethod::Unobtainable, 0};
}

// --------------------------------------------------------------- MoveBreed4
enum : uint8_t { kSrcNone = 0, kSrcBase = 1, kSrcFatherEgg = 2, kSrcFatherTM = 3, kSrcParentLevelUp = 4, kSrcMax = 5, kSrcVoltTackle = 6 };

struct BreedInfo {
    std::span<uint8_t> actual;
    std::span<uint8_t> possible;
    Learnset learn;
    std::span<const uint16_t> moves;
};

bool verifyBaseMoves(const BreedInfo& info) {
    int count = 0;
    for (auto x : info.actual) {
        if (x == kSrcBase) count++;
        else break;
    }
    const auto& moves = info.moves;
    const auto base = baseEggMoves(info.learn, 1);
    const int baseLen = static_cast<int>(base.size());
    if (baseLen < count) return false;
    if (moves.back() == 0 && count != baseLen) return false;
    for (int i = count - 1, b = baseLen - 1; i >= 0; i--, b--) {
        if (base[static_cast<std::size_t>(b)] != moves[static_cast<std::size_t>(i)]) return false;
    }
    if (baseLen == count) return true;
    for (int i = count; i < static_cast<int>(info.actual.size()); i++) {
        const bool isBase = (info.possible[static_cast<std::size_t>(i)] & (1 << kSrcBase)) != 0;
        if (!isBase) continue;
        const int baseIndex = indexIn(base, moves[static_cast<std::size_t>(i)]);
        const int min = static_cast<int>(moves.size()) - baseLen + baseIndex;
        if (i < min + count) return false;
    }
    return true;
}

bool recurseMovesForOrigin(const BreedInfo& info, int start, int type = kSrcMax - 1) {
    int i = start;
    do {
        const int unpeel = type - 1;
        if (unpeel != 0 && recurseMovesForOrigin(info, i, unpeel)) return true;
        const uint8_t permit = info.possible[static_cast<std::size_t>(i)];
        if ((permit & (1 << type)) == 0) return false;
        info.actual[static_cast<std::size_t>(i)] = static_cast<uint8_t>(type);
    } while (--i >= 0);
    return verifyBaseMoves(info);
}
} // namespace

bool moveBreed4Validate(uint16_t species, uint8_t version, std::span<const uint16_t> moves, std::span<uint8_t> origins) {
    int count = indexIn(moves, 0);
    if (count == 0) return false;
    if (count == -1) count = static_cast<int>(moves.size());
    const bool hgss = isHGSS(version);
    const Table t = hgss ? Table::HGSS : ((version == D::kD || version == D::kP) ? Table::DP : Table::Pt);
    const Learnset learn = learnset(t, species, 0);
    const Personal pi = personal(t, species, 0);
    std::fill(origins.begin(), origins.end(), static_cast<uint8_t>(0));
    std::vector<uint8_t> possible(static_cast<std::size_t>(count), 0);
    const BreedInfo info{origins, std::span<uint8_t>(possible), learn, moves};
    if (species == kPichu && moves[static_cast<std::size_t>(count - 1)] == kMoveVoltTackle)
        origins[static_cast<std::size_t>(--count)] = kSrcVoltTackle;
    bool valid;
    if (count == 0) {
        valid = verifyBaseMoves(info);
    } else {
        const bool inherit = canInheritMoves(species);
        const auto eggs = eggMoves(hgss, species);
        const auto base = baseEggMoves(learn, 1);
        for (int i = 0; i < count; i++) {
            const uint16_t move = moves[static_cast<std::size_t>(i)];
            uint8_t& p = possible[static_cast<std::size_t>(i)];
            if (indexIn(base, move) >= 0) p |= 1 << kSrcBase;
            if (inherit && learnsetHas(learn, move)) p |= 1 << kSrcParentLevelUp;
            if (contains(eggs, move)) p |= 1 << kSrcFatherEgg;
            if (isTM(pi, move)) p |= 1 << kSrcFatherTM;
            if (isHM(pi, move, hgss)) p |= 1 << kSrcFatherTM;
        }
        valid = recurseMovesForOrigin(info, count - 1);
    }
    if (!valid) {
        // CleanResult
        for (std::size_t i = 0; i < possible.size(); i++) {
            const uint8_t poss = possible[i];
            if (poss == 0) continue;
            if (origins[i] != 0) continue;
            for (int j = 0; j < kSrcMax; j++) {
                if ((poss & (1 << j)) == 0) continue;
                origins[i] = static_cast<uint8_t>(j);
                break;
            }
        }
    }
    return valid;
}

namespace {
LearnMethod eggSourceMethod(uint8_t origin) noexcept {
    switch (origin) {
    case kSrcBase: return LearnMethod::Initial;
    case kSrcFatherEgg: return LearnMethod::EggMove;
    case kSrcFatherTM: return LearnMethod::TMHM;
    case kSrcParentLevelUp: return LearnMethod::InheritLevelUp;
    case kSrcVoltTackle: return LearnMethod::SpecialEgg;
    default: return LearnMethod::None;
    }
}

/// MoveBreed.GetExpectedMoves (generation 4).
void expectedMoves(const std::array<uint16_t, 4>& moves, const EncInfo& e, std::array<uint16_t, 4>& result) {
    std::array<uint8_t, 4> origins{};
    if (moveBreed4Validate(e.species, e.version, moves, origins)) { result = moves; return; }
    struct Order { uint16_t move = 0; uint8_t origin = 0; };
    std::array<Order, 4> expected{};
    int count = 0;
    for (int i = 3; i >= 0; i--) {
        const uint8_t origin = origins[static_cast<std::size_t>(i)];
        if (origin == 0) continue;
        int insert = 0;
        for (; insert < count; insert++) if (origin < expected[static_cast<std::size_t>(insert)].origin) break;
        if (insert < count) {
            for (int k = 3; k > insert; k--) expected[static_cast<std::size_t>(k)] = expected[static_cast<std::size_t>(k - 1)];
        }
        expected[static_cast<std::size_t>(insert)] = {moves[static_cast<std::size_t>(i)], origin};
        count++;
    }
    for (std::size_t i = 0; i < 4; i++) result[i] = expected[i].move;
    std::array<uint8_t, 4> temp{};
    if (moveBreed4Validate(e.species, e.version, result, temp)) return;
    const Table t = isHGSS(e.version) ? Table::HGSS : (e.version == D::kPt ? Table::Pt : Table::DP);
    const auto base = baseEggMoves(learnset(t, e.species, e.form), 1);
    // RebuildMoves
    std::array<uint16_t, 4> notBase{};
    int notBaseCount = 0;
    for (const auto& o : expected) {
        if (o.origin == 0) continue;
        if (indexIn(base, o.move) < 0) notBase[static_cast<std::size_t>(notBaseCount++)] = o.move;
    }
    int baseCount = 4 - notBaseCount;
    if (baseCount > static_cast<int>(base.size())) baseCount = static_cast<int>(base.size());
    int ctr = 0;
    for (; ctr < baseCount; ctr++) result[static_cast<std::size_t>(ctr)] = base[base.size() - static_cast<std::size_t>(baseCount) + static_cast<std::size_t>(ctr)];
    for (int i = 0; i < notBaseCount; i++) result[static_cast<std::size_t>(ctr++)] = notBase[static_cast<std::size_t>(i)];
    for (; ctr < 4; ctr++) result[static_cast<std::size_t>(ctr)] = 0;
    temp = {};
    if (moveBreed4Validate(e.species, e.version, result, temp)) return;
    for (std::size_t i = 0; i < 4; i++) result[i] = i < base.size() ? base[i] : 0;
}

/// LearnVerifierRelearn.VerifyEggMoveset (generation 4).
void verifyEggMoveset(const EncInfo& e, MoveResults& result, const std::array<uint16_t, 4>& moves) {
    std::array<uint8_t, 4> origins{};
    const bool valid = moveBreed4Validate(e.species, e.version, moves, origins);
    if (valid) {
        for (int i = 3; i >= 0; i--) {
            const auto k = static_cast<std::size_t>(i);
            result[k] = moves[k] == 0 ? MoveResult{LearnMethod::Empty, 0} : MoveResult{eggSourceMethod(origins[k]), 0};
        }
    } else {
        std::array<uint16_t, 4> expected{};
        expectedMoves(moves, e, expected);
        moveBreed4Validate(e.species, e.version, expected, origins);
        for (int i = 3; i >= 0; i--) {
            const auto k = static_cast<std::size_t>(i);
            if (moves[k] != expected[k]) result[k] = {LearnMethod::UnobtainableExpect, expected[k]};
            else if (moves[k] == 0) result[k] = {LearnMethod::Empty, 0};
            else result[k] = {eggSourceMethod(origins[k]), 0};
        }
    }
    // IsAnyMoveDuplicate
    const uint16_t m1 = moves[0], m2 = moves[1], m3 = moves[2], m4 = moves[3];
    int dupe = -1;
    if (m1 != 0 && m1 == m2) dupe = 1;
    else if (m3 != 0 && (m1 == m3 || m2 == m3)) dupe = 2;
    else if (m4 != 0 && (m1 == m4 || m2 == m4 || m3 == m4)) dupe = 3;
    if (dupe != -1) result[static_cast<std::size_t>(dupe)] = {LearnMethod::Duplicate, 0};
}

void verifyMovesInitial(MoveResults& result, const std::array<uint16_t, 4>& current, std::span<const uint16_t> initial) {
    for (std::size_t i = 0; i < initial.size() && i < 4; i++) {
        if (current[i] != initial[i]) result[i] = {LearnMethod::UnobtainableExpect, initial[i]};
        else if (current[i] == 0) result[i] = {LearnMethod::Empty, 0};
        else result[i] = {LearnMethod::Initial, 0};
    }
    for (std::size_t i = initial.size(); i < 4; i++)
        result[i] = current[i] == 0 ? MoveResult{LearnMethod::Empty, 0} : MoveResult{LearnMethod::Unobtainable, 0};
}
} // namespace

MoveResults verifyMoves(const Pk4& pk, const EncInfo& enc, const std::vector<Evo>& history) {
    MoveResults result{};
    const std::array<uint16_t, 4>& current = pk.moves;
    if (pk.isEgg) {
        if (enc.kind == EncKind::Egg) {
            verifyEggMoveset(enc, result, current);
        } else if (enc.hasMoves()) {
            verifyMovesInitial(result, current, enc.moves);
        } else if (enc.version != 0 && enc.version < 0xFF) {
            const Table t = learnTable(enc.version);
            const auto initial = baseEggMoves(learnset(t, enc.species, enc.form), enc.levelMin);
            verifyMovesInitial(result, current, initial);
        }
    } else if (enc.species == kSmeargle) {
        for (int i = 3; i >= 0; --i) {
            const auto k = static_cast<std::size_t>(i);
            const uint16_t move = current[k];
            if (move == 0) result[k] = {LearnMethod::Empty, 0};
            else if (move <= 467 && move != kMoveStruggle && move != kMoveChatter) result[k] = {LearnMethod::Sketch, 0};
            else result[k] = {LearnMethod::Unobtainable, 0};
        }
    } else {
        for (int i = 3; i >= 0; --i) {
            const auto k = static_cast<std::size_t>(i);
            if (current[k] != 0) break;
            result[k] = {LearnMethod::Empty, 0};
        }
        markAndIterate(result, current, enc, pk, history, false);
    }
    // Finalize
    bool emptySlot = false;
    for (std::size_t i = 0; i < 4; i++) {
        const uint16_t move = current[i];
        if (move == 0) { emptySlot = true; continue; }
        if (emptySlot) {
            for (std::size_t j = i; j-- > 0;) {
                if (current[j] != 0) break;
                result[j] = {LearnMethod::EmptyInvalid, 0};
            }
            emptySlot = false;
            continue;
        }
        for (std::size_t j = 3; j > i; j--) {
            if (current[j] != move) continue;
            result[i] = {LearnMethod::Duplicate, 0};
            break;
        }
    }
    if (current[0] == 0) result[0] = {LearnMethod::EmptyInvalid, 0};
    return result;
}

bool isValidEvolutionWithMove(const Pk4& pk, const EncInfo& enc, const std::vector<Evo>& history, const MoveResults& moves) {
    const uint16_t species = pk.species;
    if (enc.species == species) return true;
    uint16_t move = 0;
    switch (species) {
    case 122: case 185: move = kMoveMimic; break;          // Mr. Mime, Sudowoodo
    case 424: move = kMoveDoubleHit; break;                 // Ambipom
    case 463: move = kMoveRollout; break;                   // Lickilicky
    case 465: case 469: case 473: move = kMoveAncientPower; break; // Tangrowth, Yanmega, Mamoswine
    default: return true;
    }
    bool slot = false;
    for (const auto& m : moves) {
        const auto method = m.method;
        if (!(method == LearnMethod::EggMove || method == LearnMethod::InheritLevelUp || method == LearnMethod::SpecialEgg)) { slot = true; break; }
    }
    if (!slot) return false;
    // PruneKeepPreEvolutions(species)
    std::vector<Evo> pruned = history;
    for (std::size_t i = 0; i < history.size(); ++i) {
        if (history[i].species != species) continue;
        pruned.assign(history.begin() + static_cast<std::ptrdiff_t>(i + 1), history.end());
        break;
    }
    // MemoryPermissions.GetCanKnowMove with LearnOption.AtAnyTimeChain (IsPast).
    MoveResults r{};
    const std::array<uint16_t, 4> one{move, 0, 0, 0};
    // MarkAndIterate over a single-slot span: only slot 0 is meaningful.
    if (enc.hasMoves()) {
        for (auto m : enc.moves) {
            if (m == 0) break;
            if (m == move) { r[0] = {LearnMethod::Initial, 0}; break; }
        }
    }
    if (r[0].valid()) return true;
    // Remaining slots of the 4-wide buffer are marked parsed so only slot 0 decides.
    for (std::size_t i = 1; i < 4; ++i) r[i] = {LearnMethod::Empty, 0};
    learnGroup4Check(r, one, pk, pruned, enc, true);
    return r[0].valid();
}

} // namespace Legality::Gen4
