/**
 * Gen4LegalityEncounters.cpp - encounter search for PK4.
 *
 * Ports of PKHeX.Core (pinned in Gen4LegalityData.h): EncounterEnumerator4,
 * EncounterGenerator4 (deferrals, the wild-lead queue and the last-resort yield),
 * the IsMatchExact / GetMatchRating / IsCompatible members of EncounterSlot4,
 * EncounterStatic4, EncounterStatic4Pokewalker, EncounterTrade4PID,
 * EncounterTrade4RanchGift, PCD/PGT and EncounterEgg4, and PGT.IsRangerManaphy.
 * The generator is evaluated eagerly; EncounterFinder only peeks one element ahead
 * and every successful peek is followed by a move to that element, so the eager
 * list, the ManualFlag and the final lead are the same as PKHeX's lazy sequence.
 */
#include "Gen4LegalityInternal.h"

#include <algorithm>

namespace Legality::Gen4 {

namespace {

enum Rating : int { kMatch = 0, kDeferred = 1, kDeferredErrors = 2, kPartialMatch = 3, kMaxNotMatch = 4 };
enum : uint8_t { kBallPoke = 4, kBallSafari = 5, kBallSport = 24 };
enum : uint16_t { kLocDaycare4 = 2000, kLocLinkTrade4 = 2002, kLocFaraway4 = 3002, kLocRanger4 = 3001,
                  kLocLinkTrade4NPC = 2001, kLocPokewalker4 = 233, kLocMarsh = 52, kLocSafariHGSS = 202,
                  kLocRuinsOfAlph = 209 };
enum : uint16_t { kMunchlax = 446, kMilotic = 350 };

bool isSafariBallRequired(uint16_t loc) noexcept { return loc == kLocMarsh || loc == kLocSafariHGSS; }

// ------------------------------------------------------------------ gift decoding
Gift parseGift(const uint8_t* rec) noexcept {
    Gift g;
    g.cardType = static_cast<uint16_t>(rec[0] | (rec[1] << 8));
    (void)read(std::span<const uint8_t>(rec + 8, 236), g.pk);
    return g;
}

uint8_t pkCurrentLevel(const Pk4& p) noexcept {
    return levelFromExp(p.exp, personal(Table::HGSS, p.species, p.form).expGrowth());
}

bool isContestBelow(const Pk4& pk, const std::array<uint8_t, 6>& initial) noexcept {
    for (std::size_t i = 0; i < 6; ++i) if (pk.contest[i] < initial[i]) return true;
    return false;
}

// ------------------------------------------------------------------ roamers
bool roamerBit64(uint64_t permit, uint16_t location, int first) noexcept {
    const int value = static_cast<int>(location) - first;
    if (static_cast<unsigned>(value) >= 64u) return false;
    return (permit & (uint64_t{1} << value)) != 0;
}
bool roamerBit32(uint32_t permit, uint16_t location, int first) noexcept {
    const int value = static_cast<int>(location) - first;
    if (static_cast<unsigned>(value) >= 32u) return false;
    return (permit & (uint32_t{1} << value)) != 0;
}
constexpr int kFirstS = 16, kFirstJ = 177, kFirstH = 149;
bool roamerGrass(uint16_t location, uint16_t met) noexcept {
    switch (location) {
    case kFirstS: return roamerBit64(0x2'8033FFFFull, met, kFirstS);
    case kFirstJ: return roamerBit32(0x0003E7FFu, met, kFirstJ);
    case kFirstH: return roamerBit32(0x0AB3FFFFu, met, kFirstH);
    default: return false;
    }
}
bool roamerWater(uint16_t location, uint16_t met) noexcept {
    switch (location) {
    case kFirstS: return roamerBit64(0x2'803E3B9Eull, met, kFirstS);
    case kFirstJ: return roamerBit32(0x0001E06Eu, met, kFirstJ);
    case kFirstH: return roamerBit32(0x0ABC1B28u, met, kFirstH);
    default: return false;
    }
}
constexpr uint8_t kTileGrass = 2, kTileWater = 7;

// ------------------------------------------------------------------ per-kind matching
bool slotMatchExact(const D::Area4& a, const D::Slot4& s, const Pk4& pk, const Evo& evo) noexcept {
    // HGSS's ordinary encounter bank at map section 202 is a placeholder.
    // With the Safari system flag, the native land/surf/fishing dispatch uses
    // FieldSystem_GenerateSafariEncounter and its ten-slot area tables. Without
    // that flag the battle cannot supply the Safari Ball required here. Do not
    // let a placeholder surf/rod slot bypass the real Safari level/frame check.
    // pret/pokeheartgold@9d8b759: src/field/encounter_check.c.
    if (isHGSS(a.version) && a.location == kLocSafariHGSS && a.type < SlotRef::kSafariGrass)
        return false;
    if (s.form != evo.form && s.species != kBurmy) {
        if (s.form < 30) return false; // IsRandomUnspecificForm (FormDynamic)
    }
    const SlotRef ref{&a, &s};
    if (pk.format != 4) {
        // A PK5's met level is the transfer level: the slot only has to be reachable.
        if (evo.levelMax < s.levelMin) return false;
    } else if (!(s.levelMin <= pk.metLevel && pk.metLevel <= s.levelMax)) {
        // RNGFrameNotFound4 is Invalid by default, so Pressure slots pass on to the lead check.
        if (a.type != SlotRef::kGrass || pk.metLevel != ref.pressureLevel()) return false;
    }
    if (a.type == SlotRef::kHoneyTree && s.species == kMunchlax && !isMunchlaxTree(pk.tid | (uint32_t{pk.sid} << 16), a.location))
        return false;
    return true;
}
int slotRating(const D::Area4& a, const D::Slot4& s, const Pk4& pk) noexcept {
    const uint8_t ball = pk.ball();
    if ((ball == kBallSafari) != isSafariBallRequired(a.location)) return kPartialMatch;
    if ((ball == kBallSport) != (a.type == SlotRef::kBugContest)) {
        if (pk.species != kShedinja || ball != kBallPoke) return kPartialMatch;
    }
    if (s.species == kWurmple && pk.species != kWurmple && !isWurmpleEvoValid(pk.pid, pk.species)) return kPartialMatch;
    if (s.species == kUnown && !isUnownFormValid(pk, pk.form, a.location == kLocRuinsOfAlph)) return kPartialMatch;
    return kMatch;
}
uint8_t slotFixedBall(const D::Area4& a) noexcept {
    if (a.type == SlotRef::kBugContest) return kBallSport;
    return isSafariBallRequired(a.location) ? kBallSafari : 0;
}

bool staticMatchExact5(const D::Static4& t, const Pk4& pk, const Evo& evo) noexcept {
    // EncounterStatic4.IsMatchExact for a transferred entity: Poke Transfer keeps only the D/P egg
    // location (a Pt/HG/SS-only egg location reads as Faraway Place), the met location is lost and
    // the met level is the transfer level.
    const bool isEggTemplate = t.eggLocation != 0;
    const uint16_t eggLoc = pk.eggLocation();
    if (!isEggTemplate) {
        if (eggLoc != t.eggLocation) return false;
    } else if (eggLoc != kLocLinkTrade4) {
        const uint16_t cmp = (t.eggLocation > 2010 && t.eggLocation < 3000) ? static_cast<uint16_t>(kLocFaraway4) : uint16_t{t.eggLocation};
        if (eggLoc != cmp) return false;
    }
    if (t.level > evo.levelMax) return false;
    if (t.gender != 0xFF && pk.gender != t.gender) return false;
    if (t.form != evo.form && !isFormChangeable(t.species)) return false;
    return true;
}

bool staticMatchExact(const D::Static4& t, const Pk4& pk, const Evo& evo) noexcept {
    if (pk.format != 4) return staticMatchExact5(t, pk, evo);
    const bool isEggTemplate = t.eggLocation != 0;
    const uint16_t eggLoc = pk.eggLocation();
    if (!isEggTemplate) {
        if (eggLoc != t.eggLocation) return false;
    } else if (!pk.isEgg) {
        if (eggLoc != t.eggLocation && eggLoc != kLocLinkTrade4) return false;
    } else if (eggLoc != t.eggLocation) {
        return false;
    }
    const uint16_t met = pk.metLocation();
    if (isEggTemplate) {
        if (pk.isEgg && met != t.location && met != kLocLinkTrade4) return false;
    } else if (!t.roaming) {
        if (met != t.location) return false;
    } else {
        bool ok = false;
        if (pk.groundTile == kTileGrass) ok = roamerGrass(t.location, met);
        else if (pk.groundTile == kTileWater) ok = roamerWater(t.location, met);
        if (!ok) return false;
    }
    if (pk.metLevel != (isEggTemplate ? 0 : t.level)) return false;
    if (t.gender != 0xFF && pk.gender != t.gender) return false;
    if (t.form != evo.form && !isFormChangeable(t.species)) return false;
    return true;
}
int staticRating(const D::Static4& t, const Pk4& pk) noexcept {
    const bool gift = t.ball == kBallPoke;
    return gift && pk.ball() != t.ball ? kPartialMatch : kMatch;
}

bool walkerMatchExact(const D::Walker4& t, const Pk4& pk, const Evo& evo) noexcept {
    if (pk.eggLocation() != 0) return false;
    if (pk.format != 4) {
        if (evo.levelMax < t.level) return false; // transfer location is verified later
    } else {
        if (pk.metLocation() != kLocPokewalker4) return false;
        if (pk.metLevel != t.level) return false;
    }
    if (pk.gender != t.gender) {
        if (pk.species != t.species && t.species == kAzurill && t.gender == 1) {
            if (genderFromPid(pk.pid, 0xBF) != t.gender) return false;
        }
    }
    if (evo.form != 0 && !isFormChangeable(t.species)) return false;
    return true;
}
int walkerRating(const Pk4& pk) noexcept {
    uint32_t seeds[kSeedBuffer];
    if (pk.ball() != kBallPoke || lcrng::getSeedsIVs(seeds, pk.ivs) == 0) return kPartialMatch;
    if (pk.isShiny()) return kDeferredErrors;
    return kMatch;
}

bool tradeMatchExact(const D::Trade4& t, const Pk4& pk, const Evo& evo) noexcept {
    const bool metUnset = t.metLocation == 0;
    const bool transferred = pk.format != 4;
    if (transferred) { if (evo.levelMax < t.level) return false; }
    else if (!metUnset) { if (pk.metLevel != t.level) return false; }
    else if (pk.metLevel < t.level) return false;
    if (pk.tid != t.tid || pk.sid != t.sid) return false;
    const uint16_t location = metUnset ? static_cast<uint16_t>(kLocLinkTrade4NPC) : uint16_t{t.metLocation};
    if (!transferred && pk.metLocation() != location) return false;
    for (std::size_t i = 0; i < 6; ++i) if (static_cast<int>(pk.ivs[i]) != t.ivs[i]) return false;
    if (pk.pid != t.pid) return false;
    if (pk.nature() != t.pid % 25) return false; // a PK5 stores its nature
    if (evo.form != 0 && !isFormChangeable(t.species)) return false;
    if (pk.otGender != t.otGender) return false;
    if (pk.eggLocation() != 0) return false;
    const std::array<uint8_t, 6> c{t.contest, t.contest, t.contest, t.contest, t.contest, 0};
    if (isContestBelow(pk, c)) return false;
    return true;
}

bool ranchMatchExact(const D::Ranch4& t, const Pk4& pk, const Evo& evo) noexcept {
    if (pk.format != 4) { if (evo.levelMax < t.level) return false; }
    else if (pk.metLevel != t.metLevel) return false;
    if (pk.tid != 1000 || pk.sid != t.sid) return false;
    if (pk.gender != t.gender) return false;
    if (t.fateful) { if (pk.isShiny()) return false; }
    else if (t.pid != pk.pid) return false;
    if (pk.format == 4 && pk.metLocation() != t.location) return false;
    if (evo.form != t.form && !isFormChangeable(t.species)) return false;
    if (pk.otGender != t.otGender) return false;
    if (pk.eggLocation() != t.eggLocation) return false;
    if (pk.isEgg) return false;
    return true;
}

bool pcdMatchExact(const Gift& g, const Pk4& pk, const Evo& evo) noexcept {
    const Pk4& wc = g.pk;
    if (!wc.isEgg) {
        if (wc.tid != pk.tid || wc.sid != pk.sid) return false;
        if (!decodeEqualOT(wc, pk)) return false;
        if (wc.otGender != pk.otGender) return false;
        if (wc.language != 0 && wc.language != pk.language) return false;
        if (pk.format != 4) {
            // Transferred: the met location is checked with the transfer rules.
            if (pkCurrentLevel(wc) > pk.metLevel) return false;
            if (pk.eggLocation() != g.eggLocation()) return false;
        } else {
            if (wc.eggLocation() + 3000 != pk.metLocation()) return false;
            if (pkCurrentLevel(wc) != pk.metLevel) return false;
        }
    } else {
        if (wc.eggLocation() + 3000 != pk.eggLocation() && pk.eggLocation() != kLocLinkTrade4) return false;
        if (pkCurrentLevel(wc) != pk.metLevel) return false;
        if (pk.isEgg && pk.format != 4) return false;
    }
    if (wc.form != evo.form && !isFormChangeable(wc.species)) return false;
    if (wc.ball() != pk.ball()) return false;
    if (wc.otGender < 3 && wc.otGender != pk.otGender) return false;
    if (wc.species == kMilotic && isContestBelow(pk, wc.contest)) return false;
    if (wc.pid == 1) {
        if (pk.isShiny()) return false;
    } else if (wc.gender != pk.gender) {
        return false;
    }
    return true;
}
int pcdRating(const Gift& g, const Pk4& pk) noexcept {
    return g.version() != pk.version ? kPartialMatch : kMatch;
}

bool isRangerManaphy(const Pk4& pk) noexcept {
    if (pk.species != kManaphy) return false;
    if (pk.language >= 8) return false;
    const uint16_t egg = pk.eggLocation();
    if (!pk.isEgg) return egg == kLocLinkTrade4 || egg == kLocRanger4;
    if (pk.format != 4) return false; // cannot be an egg if it must be transferred
    if (egg != kLocRanger4) return false;
    const uint16_t met = pk.metLocation();
    return met == kLocLinkTrade4 || met == 0;
}

bool isG4ManaphyPidValid(PidType type, const Pk4& pk) noexcept {
    const bool tradedEgg = pk.eggLocation() == kLocLinkTrade4;
    if (type == PidType::Method_1) {
        if (pk.isEgg) return !pk.isShiny();
        return tradedEgg || !pk.isShiny();
    }
    if (type != PidType::G4MGAntiShiny) return false;
    if (!pk.isEgg && tradedEgg) return true;
    // ARNG.Prev
    const uint32_t pid = pk.pid * 0x9638806Du + 0x69C77F93u;
    const uint32_t tsv = static_cast<uint32_t>(pk.tid ^ pk.sid) >> 3;
    const uint32_t psv = ((pid & 0xFFFFu) ^ (pid >> 16)) >> 3;
    return tsv == psv;
}

// ------------------------------------------------------------------ the enumerator
struct Enumerator {
    const Pk4& pk;
    const std::vector<Evo>& chain;
    std::vector<Candidate> out;
    Candidate deferred{};
    bool hasDeferred = false;
    int rating = kMaxNotMatch;

    template <class Exact, class Rate>
    void consider(uint16_t species, const Candidate& c, Exact&& exact, Rate&& rate) {
        for (const Evo& evo : chain) {
            if (evo.species != species) continue;
            if (!exact(evo)) break;
            const int r = rate();
            if (r == kMatch) { out.push_back(c); }
            else if (r < rating) { deferred = c; hasDeferred = true; rating = r; }
            break;
        }
    }

    void events() {
        for (std::size_t i = 0; i < D::kPcdCount; ++i) {
            const Gift g = parseGift(D::kPcd + i * D::kPcdRecordSize);
            Candidate c{EncKind::Pcd, static_cast<int>(i), -1, g.species(), 0, Lead::Invalid};
            consider(g.species(), c, [&](const Evo& evo) { return pcdMatchExact(g, pk, evo); },
                     [&] { return pcdRating(g, pk); });
        }
    }
    void statics(int list) {
        for (int i = D::kStaticListStart[list]; i < D::kStaticListStart[list + 1]; ++i) {
            const auto& t = D::kStatics[i];
            consider(t.species, Candidate{EncKind::Static, i, -1, t.species, 0, Lead::Invalid},
                     [&](const Evo& evo) { return staticMatchExact(t, pk, evo); }, [&] { return staticRating(t, pk); });
        }
    }
    void walkers() {
        for (std::size_t i = 0; i < D::kWalkerCount; ++i) {
            const auto& t = D::kWalkers[i];
            consider(t.species, Candidate{EncKind::Walker, static_cast<int>(i), -1, t.species, 0, Lead::Invalid},
                     [&](const Evo& evo) { return walkerMatchExact(t, pk, evo); }, [&] { return walkerRating(pk); });
        }
    }
    void trades(uint8_t list) {
        for (std::size_t i = 0; i < D::kTradeCount; ++i) {
            const auto& t = D::kTrades[i];
            if (t.list != list) continue;
            consider(t.species, Candidate{EncKind::Trade, static_cast<int>(i), -1, t.species, 0, Lead::Invalid},
                     [&](const Evo& evo) { return tradeMatchExact(t, pk, evo); }, [] { return int{kMatch}; });
        }
    }
    void ranch() {
        for (std::size_t i = 0; i < D::kRanchCount; ++i) {
            const auto& t = D::kRanch[i];
            consider(t.species, Candidate{EncKind::Ranch, static_cast<int>(i), -1, t.species, 0, Lead::Invalid},
                     [&](const Evo& evo) { return ranchMatchExact(t, pk, evo); }, [] { return int{kMatch}; });
        }
    }
    void slots() {
        if (pk.isEgg || pk.eggLocation() != 0) return; // CanBeWildEncounter
        const uint16_t met = pk.metLocation();
        for (std::size_t ai = 0; ai < D::kAreaCount; ++ai) {
            const auto& a = D::kAreas[ai];
            // A PK5 lost its original met location (EncounterEnumerator4 hasOriginalLocation).
            if (a.version != pk.version || (pk.format == 4 && a.location != met)) continue;
            for (uint16_t k = 0; k < a.slotCount; ++k) {
                const int si = a.slotStart + k;
                const auto& s = D::kSlots[si];
                consider(s.species, Candidate{EncKind::Slot, si, static_cast<int>(ai), s.species, 0, Lead::Invalid},
                         [&](const Evo& evo) { return slotMatchExact(a, s, pk, evo); }, [&] { return slotRating(a, s, pk); });
            }
        }
    }
    void staticsForVersion() {
        switch (pk.version) {
        case D::kD: statics(D::StaticD); statics(D::StaticDP); statics(D::StaticDPPt); break;
        case D::kP: statics(D::StaticP); statics(D::StaticDP); statics(D::StaticDPPt); break;
        case D::kPt: statics(D::StaticPt); statics(D::StaticDPPt); break;
        case D::kHG: statics(D::StaticHG); statics(D::StaticHGSS); walkers(); break;
        case D::kSS: statics(D::StaticSS); statics(D::StaticHGSS); walkers(); break;
        default: break;
        }
    }

    bool tryGetEgg(Candidate& egg) const {
        const Evo& devolved = chain.back();
        if (!devolved.insideLevelRange(1)) return false;
        const auto [species, form] = baseSpeciesForm(devolved.species, devolved.form);
        if (species != devolved.species && !(devolved.species < 494 && (D::kBreed[devolved.species].flags & 2) != 0)) return false;
        if (species >= 494 || (D::kBreed[species].flags & 1) == 0) return false;
        if (form != 0 && (species == kPikachu || species == kEevee || species == kPichu)) return false;
        if (!isPresentInGame(Table::HGSS, species, form)) return false;
        egg = Candidate{EncKind::Egg, -1, -1, species, pk.version, Lead::Invalid};
        return true;
    }
    bool tryGetSplit(const Candidate& other, Candidate& split) const {
        const Evo* devolved = &chain.back();
        if (other.species == devolved->species) {
            if (chain.size() < 2) return false;
            devolved = &chain[chain.size() - 2];
        }
        if (devolved->species >= 494 || (D::kBreed[devolved->species].flags & 2) == 0) return false;
        split = other;
        split.species = devolved->species;
        return true;
    }

    void run() {
        if (chain.empty()) return;
        if (pk.fateful) {
            if (isRangerManaphy(pk)) {
                out.push_back(Candidate{EncKind::RangerManaphy, -1, -1, kManaphy, 0, Lead::Invalid});
                return;
            }
            events();
            if (!out.empty()) return;
        }
        const uint16_t eggLoc = pk.eggLocation();
        const bool bred = eggLoc == kLocDaycare4 || eggLoc == kLocLinkTrade4 ||
                          (eggLoc == kLocFaraway4 && (pk.version == D::kPt || pk.version == D::kHG || pk.version == D::kSS));
        Candidate egg{};
        if (bred && tryGetEgg(egg)) {
            out.push_back(egg);
            Candidate split{};
            if (tryGetSplit(egg, split)) out.push_back(split);
        }
        if (pk.version == D::kD) { ranch(); trades(0); }
        else if (isHGSS(pk.version)) trades(1);
        else trades(0);

        const uint8_t ball = pk.ball();
        const bool mustBeSlot = eggLoc == 0 && (ball == kBallSport || ball == kBallSafari);
        if (mustBeSlot) {
            slots();
            staticsForVersion();
            return; // StaticEnd breaks without the Fallback
        }
        staticsForVersion();
        slots();
        if (hasDeferred) out.push_back(deferred);
    }
};

// ------------------------------------------------------------------ generator helpers
uint32_t groundTileAllowed(const Candidate& c, bool& hasTile) noexcept {
    hasTile = true;
    if (c.kind == EncKind::Slot) return D::kAreas[c.area].groundTile;
    if (c.kind == EncKind::Static) return D::kStatics[c.index].groundTile;
    hasTile = false;
    return 0;
}

uint8_t fixedBall(const Candidate& c) noexcept {
    switch (c.kind) {
    case EncKind::Slot: return slotFixedBall(D::kAreas[c.area]);
    case EncKind::Static: return D::kStatics[c.index].ball;
    case EncKind::Walker: case EncKind::Trade: case EncKind::Egg: case EncKind::RangerManaphy: return kBallPoke;
    case EncKind::Ranch: return D::kRanch[c.index].ball;
    case EncKind::Pcd: return pcdGift(c.index).ball();
    default: return 0;
    }
}

enum class Compat : uint8_t { Match, NotIdeal, Mismatch };

Compat typeCompatible(const Candidate& c, const Pk4& pk, PidType type) noexcept {
    auto m = [](bool b) { return b ? Compat::Match : Compat::Mismatch; };
    switch (c.kind) {
    case EncKind::Slot: {
        const SlotRef ref{&D::kAreas[c.area], &D::kSlots[c.index]};
        if (type == PidType::Method_1) return Compat::Match;
        if (type == PidType::ChainShiny) return m(pk.isShiny() && ref.canUseRadar());
        if (type == PidType::CuteCharm) return m(isCuteCharmValid(ref.slot->species, pk));
        return Compat::Mismatch;
    }
    case EncKind::Static: {
        const auto& t = D::kStatics[c.index];
        if (t.species == kPichu) return m(type == PidType::Pokewalker);
        if (t.shiny == kShinyAlways) return m(type == PidType::ChainShiny);
        if (type == PidType::Method_1) return Compat::Match;
        if (type == PidType::CuteCharm) return m(isCuteCharmValid(t.species, pk));
        return Compat::Mismatch;
    }
    case EncKind::Walker: {
        if (type == PidType::Pokewalker) return Compat::Match;
        if (type == PidType::CuteCharm) return m(isCuteCharm(pk, pk.pid) && isCuteCharmValid(D::kWalkers[c.index].species, pk));
        return Compat::Mismatch;
    }
    case EncKind::Pcd: case EncKind::RangerManaphy: {
        const Gift g = c.kind == EncKind::Pcd ? pcdGift(c.index) : rangerManaphy();
        if (g.isManaphyEgg()) return m(isG4ManaphyPidValid(type, pk));
        if (g.pk.pid != 1 && type == PidType::G5MGShiny) return Compat::Match;
        return m(type == PidType::None);
    }
    case EncKind::Egg: return m(type == PidType::None); // EggRandomAnyType4 is false by default
    default: return m(type == PidType::None);           // Trade4PID / RanchGift: not IRandomCorrelation
    }
}

enum class Deferral : uint8_t { None, PIDIVDefer, SlotNumber, Ball, Tile, PIDIV };

} // namespace

// ======================================================================= public
Gift pcdGift(int index) noexcept {
    if (index < 0 || static_cast<std::size_t>(index) >= D::kPcdCount) return {};
    return parseGift(D::kPcd + static_cast<std::size_t>(index) * D::kPcdRecordSize);
}

Gift rangerManaphy() noexcept {
    Gift g;
    g.cardType = 7;
    g.pk.pid = 1; // ItemSubID = 1 lands on the PK4 PID: never shiny
    return g;
}

bool isFormChangeable(uint16_t species) noexcept {
    switch (species) {
    case kBurmy: case kRotom: case kDeoxys: case 483: case 484: case kGiratina: case kShaymin: case kArceus:
        return true;
    default:
        return false;
    }
}

Search searchEncounters(const Pk4& pk, uint8_t currentLevel) {
    if (!isGen4Version(pk.version)) {
        Search s;
        s.pidiv = analyzePidIv(pk);
        return s;
    }
    return searchEncounters(pk, originChain(pk, currentLevel));
}

Search searchEncounters(const Pk4& pk, const std::vector<Evo>& chain) {
    Search s;
    s.pidiv = analyzePidIv(pk);
    if (!isGen4Version(pk.version)) return s;
    if (chain.empty()) return s;
    Enumerator e{pk, chain, {}, {}, false, kMaxNotMatch};
    e.run();

    Deferral deferType = Deferral::None;
    bool hasDefer = false;
    Candidate deferCand{};
    auto defer = [&](Deferral type, const Candidate& c) {
        if (type >= deferType && hasDefer) return;
        deferType = type;
        deferCand = c;
        hasDefer = true;
    };
    std::vector<std::pair<LeadSeed, Candidate>> queue;
    const uint8_t ball = pk.ball();
    for (const Candidate& c : e.out) {
        bool hasTile = false;
        const uint32_t allowed = groundTileAllowed(c, hasTile);
        const bool tileOk = hasTile ? isGroundTileAllowed(allowed, pk.groundTile) : pk.groundTile == 0;
        if (!tileOk) { defer(Deferral::Tile, c); continue; }
        const Compat compat = typeCompatible(c, pk, s.pidiv.type);
        if (compat != Compat::Match) {
            defer(compat == Compat::NotIdeal ? Deferral::PIDIVDefer : Deferral::PIDIV, c);
            continue;
        }
        const uint8_t fb = fixedBall(c);
        const bool ballOk = fb == kBallSafari ? ball == kBallSafari
                          : fb == kBallSport ? ball == kBallSport
                          : (ball != kBallSafari && ball != kBallSport);
        if (!ballOk) { defer(Deferral::Ball, c); continue; }
        if (c.kind != EncKind::Slot) {
            if (ball == kBallSafari || ball == kBallSport) defer(Deferral::Ball, c);
            else s.list.push_back(c);
            continue;
        }
        const SlotRef ref{&D::kAreas[c.area], &D::kSlots[c.index]};
        // LeadFinder.GetLevelConstraint: the met level for a PK4; for a PK5 the chain entry of the
        // slot species clamped to the slot's level range.
        uint8_t levelMin = pk.metLevel, levelMax = pk.metLevel;
        if (pk.format != 4) {
            for (const Evo& evo : chain) {
                if (evo.species != ref.slot->species) continue;
                levelMin = std::max(evo.levelMin, ref.levelMin());
                levelMax = std::min(evo.levelMax, ref.levelMax());
                break;
            }
        }
        const LeadSeed lead = leadInfo4(pk, ref, s.pidiv, levelMin, levelMax);
        if (!lead.valid()) { defer(Deferral::SlotNumber, c); continue; }
        Candidate withLead = c;
        withLead.lead = lead.lead;
        auto it = std::find_if(queue.begin(), queue.end(),
                               [&](const auto& q) { return lead.lead > q.first.lead; });
        queue.insert(it, {lead, withLead});
    }
    for (const auto& q : queue) s.list.push_back(q.second);
    if (!queue.empty()) return s;
    if (!hasDefer) return s;
    if (deferType == Deferral::PIDIV) s.manualFlag = 1;
    else if (deferType == Deferral::SlotNumber) s.manualFlag = 2;
    s.list.push_back(deferCand);
    return s;
}

int matchRating(const Template& t, const Pk4& pk) {
    switch (t.kind) {
    case EncKind::Slot: return slotRating(*t.area, *t.slot, pk);
    case EncKind::Static: return staticRating(D::kStatics[t.index], pk);
    case EncKind::Walker: return walkerRating(pk);
    case EncKind::Trade: case EncKind::Ranch: case EncKind::RangerManaphy: return kMatch;
    case EncKind::Pcd: return pcdRating(t.gift, pk);
    default: return -1; // EncounterEgg4 is not IEncounterMatch
    }
}

Template makeTemplate(const Candidate& c, const Pk4& pk) {
    Template t;
    t.kind = c.kind;
    t.index = c.index;
    switch (c.kind) {
    case EncKind::Slot: {
        const auto& a = D::kAreas[c.area];
        const auto& s = D::kSlots[c.index];
        t.area = &a; t.slot = &s;
        t.species = s.species; t.form = s.form; t.levelMin = s.levelMin; t.levelMax = s.levelMax;
        t.version = a.version; t.location = a.location; t.fixedBall = slotFixedBall(a);
        t.ability = 0; t.shiny = kShinyRandom;
        break;
    }
    case EncKind::Static: {
        const auto& s = D::kStatics[c.index];
        t.species = s.species; t.form = s.form; t.levelMin = t.levelMax = s.level; t.version = s.version;
        t.location = s.location; t.eggLocation = s.eggLocation; t.isEgg = s.eggLocation != 0;
        t.fixedBall = s.ball; t.ability = s.ability; t.shiny = s.shiny; t.fateful = s.fateful != 0;
        t.gender = s.gender; t.nature = s.nature; t.heldItem = s.heldItem;
        for (std::size_t i = 0; i < 4; ++i) t.moves[i] = s.moves[i];
        break;
    }
    case EncKind::Walker: {
        const auto& s = D::kWalkers[c.index];
        t.species = s.species; t.levelMin = t.levelMax = s.level; t.version = D::kVerHGSS;
        t.location = kLocPokewalker4; t.fixedBall = kBallPoke; t.ability = 0; t.shiny = kShinyNever;
        t.gender = s.gender;
        for (std::size_t i = 0; i < 4; ++i) t.moves[i] = s.moves[i];
        break;
    }
    case EncKind::Trade: {
        const auto& s = D::kTrades[c.index];
        t.species = s.species; t.levelMin = s.level; t.levelMax = s.metLocation == 0 ? s.level : 100;
        t.version = s.version; t.location = s.metLocation == 0 ? static_cast<uint16_t>(kLocLinkTrade4NPC) : uint16_t{s.metLocation};
        t.fixedBall = kBallPoke; t.ability = s.ability; t.shiny = kShinyFixed; t.gender = s.gender;
        t.nature = static_cast<uint8_t>(s.pid % 25);
        for (std::size_t i = 0; i < 4; ++i) t.moves[i] = s.moves[i];
        break;
    }
    case EncKind::Ranch: {
        const auto& s = D::kRanch[c.index];
        t.species = s.species; t.form = s.form; t.levelMin = t.levelMax = s.level; t.version = D::kD;
        t.location = s.location; t.eggLocation = s.eggLocation; t.fixedBall = s.ball; t.ability = s.ability;
        t.fateful = s.fateful != 0; t.shiny = t.fateful ? kShinyNever : kShinyFixed; t.gender = s.gender;
        t.nature = t.fateful ? 25 : static_cast<uint8_t>(s.pid % 25);
        for (std::size_t i = 0; i < 4; ++i) t.moves[i] = s.moves[i];
        break;
    }
    case EncKind::Pcd: case EncKind::RangerManaphy: {
        t.gift = c.kind == EncKind::Pcd ? pcdGift(c.index) : rangerManaphy();
        const Gift& g = t.gift;
        t.species = g.species(); t.form = g.isManaphyEgg() ? 0 : g.pk.form;
        t.levelMin = t.levelMax = g.level(); t.version = g.version();
        if (c.kind == EncKind::Pcd) { t.location = g.location(); t.eggLocation = g.eggLocation(); }
        t.isEgg = g.isEgg(); t.fixedBall = g.ball();
        t.ability = g.isManaphyEgg() ? 0 : ((g.pk.pid & 1) ? 2 : 1);
        t.shiny = g.isEgg() ? kShinyRandom : g.pk.pid == 1 ? kShinyNever : g.pk.isShiny() ? kShinyAlways : kShinyNever;
        t.fateful = g.fateful(); t.heldItem = g.pk.heldItem;
        t.gender = g.pk.gender;
        if (!g.isManaphyEgg()) t.moves = g.pk.moves;
        break;
    }
    case EncKind::Egg: {
        t.species = c.species; t.levelMin = t.levelMax = 1; t.version = c.version;
        t.location = isHGSS(c.version) ? 182 : 4; t.eggLocation = kLocDaycare4; t.isEgg = true;
        t.fixedBall = kBallPoke; t.ability = 0; t.shiny = kShinyRandom;
        break;
    }
    case EncKind::Invalid: default: {
        t.kind = EncKind::Invalid;
        t.species = pk.species; t.form = pk.form; t.levelMin = pk.metLevel;
        t.levelMax = pkCurrentLevel(pk); t.isEgg = pk.isEgg || pk.eggDate[2] != 0; t.version = pk.version;
        t.ability = -1; t.shiny = kShinyRandom;
        break;
    }
    }
    return t;
}

EncInfo encInfo(const Template& t) {
    EncInfo e;
    e.kind = t.kind;
    e.species = t.species;
    e.form = t.form;
    e.levelMin = t.levelMin;
    e.levelMax = t.levelMax;
    e.version = t.version;
    e.moves = t.moves;
    e.isEggTemplate = t.isEgg;
    return e;
}

} // namespace Legality::Gen4
