// Gen V (Black, White, Black 2, White 2) legality for PK5 records.
//
// Rules follow PKHeX.Core at the commit pinned in Gen5LegalityData.h; the
// rule-by-rule reference is docs/legality/GEN5_PKHEX_RULE_INVENTORY.md.
// The record is read through a bounds-checked copy and never modified.
//
// Result classes: a rule that is decided from the record and pinned data is
// Invalid when broken. Anything this analyzer cannot decide is a Warning and
// leaves its coverage Partial, so the verdict stays Incomplete instead of
// Invalid. Examples: event cards that are not in the pinned database; the
// original Gen III/IV encounter (and move sources) of a transferred Pokemon;
// names with characters outside Gen5Text's modelled set; names only an in-game
// trade or event could have fixed.
#include "Legality/Gen5Legality.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdio>
#include <string>
#include <vector>

#include "Encryption/Encryption5.h"
#include "Legality/Gen5LegalityData.h"
#include "Legality/Gen5Text.h"
#include "Names/MoveNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Experience.h"

namespace Legality {
namespace Gen5 {

namespace {

uint16_t u16(std::span<const uint8_t> b, std::size_t at) noexcept {
    return at + 1 < b.size() ? static_cast<uint16_t>(b[at] | (b[at + 1] << 8)) : 0;
}
uint32_t u32(std::span<const uint8_t> b, std::size_t at) noexcept {
    return static_cast<uint32_t>(u16(b, at)) | (static_cast<uint32_t>(u16(b, at + 2)) << 16);
}

// PKHeX EggHatchLocation5.LocationPermitted5: bit (version - 20) of entry
// `location` says whether an egg can hatch there in that game.
constexpr std::array<uint8_t, 144> kHatchPermitted = [] {
    std::array<uint8_t, 144> t{};
    auto fill = [&](int from, int to, uint8_t v) { for (int i = from; i <= to; ++i) t[i] = v; };
    fill(4, 22, 15);   t[23] = 3;
    fill(24, 39, 15);  t[40] = 3;
    fill(41, 58, 15);  t[59] = 3;
    fill(60, 72, 15);  t[73] = 3;
    fill(74, 111, 15);
    t[112] = 10;       // Black Gate: Black and Black 2 only
    t[113] = 5;        // White Gate: White and White 2 only
    fill(114, 116, 15);
    fill(117, 137, 12); // Black 2 / White 2 only
    fill(139, 143, 12);
    return t;
}();

} // namespace

bool validHatchLocation(uint16_t location, uint8_t version) noexcept {
    if (version < 20 || version > 23 || location >= kHatchPermitted.size()) return false;
    return (kHatchPermitted[location] >> (version - 20)) & 1u;
}

uint16_t checksum(std::span<const uint8_t> decrypted) noexcept {
    uint32_t sum = 0;
    for (std::size_t at = 0x08; at < 0x88 && at + 1 < decrypted.size(); at += 2)
        sum += u16(decrypted, at);
    return static_cast<uint16_t>(sum);
}

bool read(std::span<const uint8_t> b, Pk5& p) noexcept {
    if (b.size() != 136 && b.size() != 220) return false;
    p = Pk5{};
    p.party = b.size() == 220;
    p.pid = u32(b, 0x00);
    p.sanity = u16(b, 0x04);
    p.checksum = u16(b, 0x06);
    p.species = u16(b, 0x08);
    p.heldItem = u16(b, 0x0A);
    p.tid = u16(b, 0x0C);
    p.sid = u16(b, 0x0E);
    p.exp = u32(b, 0x10);
    p.friendship = b[0x14];
    p.ability = b[0x15];
    p.markings = b[0x16];
    p.language = b[0x17];
    for (int i = 0; i < 6; ++i) { p.evs[i] = b[0x18 + i]; p.contest[i] = b[0x1E + i]; }
    for (int i = 0; i < 4; ++i) {
        p.moves[i] = u16(b, 0x28 + 2 * i);
        p.pp[i] = b[0x30 + i];
        p.ppUps[i] = b[0x34 + i];
    }
    const uint32_t iv32 = u32(b, 0x38);
    for (int i = 0; i < 6; ++i) p.ivs[i] = static_cast<uint8_t>((iv32 >> (5 * i)) & 31u);
    p.isEgg = (iv32 >> 30) & 1u;
    p.isNicknamed = (iv32 >> 31) & 1u;
    p.fateful = b[0x40] & 1u;
    p.gender = (b[0x40] >> 1) & 3u;
    p.form = b[0x40] >> 3;
    p.nature = b[0x41];
    p.hiddenAbility = b[0x42] & 1u;
    p.nSparkle = (b[0x42] >> 1) & 1u;
    for (int i = 0; i < 11; ++i) p.nickname[i] = u16(b, 0x48 + 2 * i);
    p.version = b[0x5F];
    for (int i = 0; i < 8; ++i) p.ot[i] = u16(b, 0x68 + 2 * i);
    for (int i = 0; i < 3; ++i) { p.eggDate[i] = b[0x78 + i]; p.metDate[i] = b[0x7B + i]; }
    p.eggLocation = u16(b, 0x7E);
    p.metLocation = u16(b, 0x80);
    p.pokerus = b[0x82];
    p.ball = b[0x83];
    p.metLevel = b[0x84] & 0x7Fu;
    p.otGender = (b[0x84] >> 7) & 1u;
    p.groundTile = b[0x85];
    p.pokeStarFame = b[0x87];
    for (int i = 0; i < 4; ++i) {
        p.ribbons[i] = b[0x24 + i];
        p.ribbons[4 + i] = b[0x3C + i];
        p.ribbons[8 + i] = b[0x60 + i];
    }
    if (p.party) p.partyLevel = b[0x8C];
    return true;
}

} // namespace Gen5

namespace {

namespace D = Gen5Data;
namespace T = Gen5Text;
using Gen5::Pk5;

// ---------------------------------------------------------------- constants

constexpr uint16_t kMaxSpecies = 649;
constexpr uint16_t kMaxMove = 559;
constexpr uint16_t kMaxItem = 638;
constexpr uint8_t kMaxAbility = 164;
constexpr uint8_t kBallPoke = 4, kBallDream = 25;

constexpr uint16_t kLinkTrade5 = 30003, kLinkTrade5NPC = 30002, kLinkTrade4 = 2002;
constexpr uint16_t kDaycare5 = 60002;
constexpr uint16_t kTransfer4 = 30001;
constexpr uint16_t kTransfer4CelebiUnused = 30010, kTransfer4CelebiUsed = 30011;
constexpr uint16_t kTransfer4CrownUnused = 30012, kTransfer4CrownUsed = 30013;
constexpr uint16_t kDreamRadar = 30015, kEntreeForest = 75;

enum Ability : int8_t { Any12H = -1, Any12 = 0, OnlyFirst = 1, OnlySecond = 2, OnlyHidden = 4 };

bool isGen5Version(uint8_t v) { return v >= 20 && v <= 23; }
uint8_t versionBit(uint8_t v) { return static_cast<uint8_t>(1u << (v - 20)); }
// GameVersion ids of Gen III/IV origin: S R E FR LG (1-5), HG SS (7-8),
// D P Pt (10-12), Colosseum/XD (15).
constexpr uint8_t kVersionCXD = 15;
bool isGen3Version(uint8_t v) { return (v >= 1 && v <= 5) || v == kVersionCXD; }
bool isGen4Version(uint8_t v) { return v == 7 || v == 8 || (v >= 10 && v <= 12); }

// PKHeX BallUseLegality.WildPokeBalls5: Master..Quick except Safari and Cherish.
bool isWildBall5(uint8_t ball) {
    return ball >= 1 && ball <= 15 && ball != 5;
}

// PKHeX AbilityBreedLegality.BanHidden5: species that cannot pass a hidden
// ability through a Gen V egg.
bool hiddenBannedFromEgg5(uint16_t species) {
    static constexpr uint16_t kBan[] = {
        1, 4, 7, 81, 92, 100, 109, 120, 128, 132, 137, 152, 155, 158, 200, 201, 236,
        252, 255, 258, 287, 311, 337, 338, 343, 351, 352, 355, 358, 374, 387, 390, 393,
        420, 433, 436, 479, 489, 495, 498, 501, 511, 513, 515, 538, 539, 562, 566, 570,
        574, 597, 599, 602, 607, 615, 622, 627, 633, 636};
    return std::find(std::begin(kBan), std::end(kBan), species) != std::end(kBan);
}

// PKHeX Breeding.IsAbleToHatchFromEgg, Gen I-V part.
bool canHatch(uint16_t species) {
    static constexpr uint16_t kNo[] = {
        132, 144, 145, 146, 150, 151, 201, 243, 244, 245, 249, 250, 251, 377, 378, 379,
        380, 381, 382, 383, 384, 385, 386, 480, 481, 482, 483, 484, 485, 486, 487, 488,
        490, 491, 492, 493, 494, 638, 639, 640, 641, 642, 643, 644, 645, 646, 647, 648, 649};
    return std::find(std::begin(kNo), std::end(kNo), species) == std::end(kNo);
}

// PKHeX Breeding.IsSplitBreedNotBabySpecies4: hatch without the incense.
bool splitBreedNotBaby(uint16_t species) {
    switch (species) {
        case 183: case 202: case 113: case 122: case 143: case 185: case 226: case 315: case 358:
            return true;
        default:
            return false;
    }
}

// UTF-16 to UTF-8 for messages (unpaired surrogates become U+FFFD).
std::string utf8(std::u16string_view text) {
    std::string out;
    for (std::size_t i = 0; i < text.size(); ++i) {
        uint32_t cp = text[i];
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < text.size() && text[i + 1] >= 0xDC00 && text[i + 1] <= 0xDFFF)
            cp = 0x10000 + ((cp - 0xD800) << 10) + (text[++i] - 0xDC00);
        else if (cp >= 0xD800 && cp <= 0xDFFF)
            cp = 0xFFFD;
        if (cp < 0x80) {
            out.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}

std::string speciesName(uint16_t s) {
    const char* n = Names::getSpeciesName(s);
    return n && *n ? n : "species " + std::to_string(s);
}
std::string moveName(uint16_t m) {
    const char* n = Names::getMoveName(m);
    return n && *n ? n : "move " + std::to_string(m);
}

// ---------------------------------------------------------------- personal

struct Personal {
    const uint8_t* rec = nullptr;
    uint8_t genderRatio() const { return rec[0x12]; }
    uint8_t hatchCycles() const { return rec[0x13]; }
    uint8_t growth() const { return rec[0x15]; }
    uint8_t ability(int i) const { return rec[0x18 + i]; }
    uint8_t formCount() const { return rec[0x20]; }
    bool tm(int i) const { return (rec[0x28 + (i >> 3)] >> (i & 7)) & 1u; }
    bool typeTutor(int i) const { return (rec[0x38] >> i) & 1u; }
};

// PKHeX PersonalInfo.FormIndex: alternate forms live after the base table.
int formIndex(const uint8_t* table, std::size_t size, std::size_t count, uint16_t species, uint8_t form) {
    if (species >= count) return -1;
    const uint8_t* rec = table + species * size;
    const uint8_t forms = rec[0x20];
    const uint16_t first = static_cast<uint16_t>(rec[0x1C] | (rec[0x1D] << 8));
    if (form == 0 || form >= forms || first == 0) return species;
    const int index = first + form - 1;
    return index < static_cast<int>(count) ? index : species;
}
int indexB2W2(uint16_t s, uint8_t f) { return formIndex(D::kPersonalB2W2, D::kPersonalB2W2Size, D::kPersonalB2W2Count, s, f); }
int indexBW(uint16_t s, uint8_t f) { return formIndex(D::kPersonalBW, D::kPersonalBWSize, D::kPersonalBWCount, s, f); }

Personal personal(uint16_t species, uint8_t form) {
    const int i = indexB2W2(species, form);
    return i < 0 ? Personal{} : Personal{D::kPersonalB2W2 + i * D::kPersonalB2W2Size};
}

// ---------------------------------------------------------------- evolution chain

// One species in the evolution chain. `child` is the stage it evolves into
// (toward the current species; -1 for the current species). `levelUp` and
// `evoLevel` describe that evolution: a level-up evolution needs at least one
// level gained after the previous stage was obtained (PKHeX EvolutionSet
// levelUp = 1), and never happens below its level argument.
struct Stage {
    uint16_t species; uint8_t form;
    int child = -1; bool levelUp = false; uint8_t evoLevel = 0;
};

// PKHeX EvolutionType.IsLevelUpRequired for the Gen V method ids.
bool levelUpRequired(uint8_t type) {
    return (type >= 1 && type <= 4) || (type >= 9 && type <= 16) || (type >= 19 && type <= 34) ||
           (type >= 36 && type <= 41);
}

// The current species and every species it can have evolved from.
std::vector<Stage> chainOf(uint16_t species, uint8_t form) {
    std::vector<Stage> chain{{species, form}};
    for (std::size_t at = 0; at < chain.size() && chain.size() < 8; ++at) {
        const uint16_t target = chain[at].species;
        for (uint16_t from = 1; from < D::kEvolutionCount; ++from) {
            bool found = false, levelUp = true;
            uint8_t level = 255;
            for (uint16_t e = D::kEvolutionStart[from]; e < D::kEvolutionStart[from + 1]; ++e) {
                const auto& evo = D::kEvolutions[e];
                if (evo.species != target) continue;
                found = true;
                // Several methods can link the same pair; keep the least demanding.
                levelUp = levelUp && levelUpRequired(evo.type);
                level = std::min(level, evo.level);
            }
            if (!found) continue;
            const bool seen = std::any_of(chain.begin(), chain.end(),
                [&](const Stage& s) { return s.species == from; });
            if (seen) continue;
            const Personal pre = personal(from, 0);
            const uint8_t preForm = pre.rec && form < pre.formCount() ? form : 0;
            chain.push_back({from, preForm, static_cast<int>(at), levelUp, level});
        }
    }
    return chain;
}

// Lowest current level a Pokemon obtained as `species` at `metLevel` can have
// reached after evolving into the chain's current species.
int minimumLevel(const std::vector<Stage>& chain, uint16_t species, int metLevel) {
    int at = -1;
    for (std::size_t i = 0; i < chain.size(); ++i) if (chain[i].species == species) { at = static_cast<int>(i); break; }
    if (at < 0) return metLevel;
    int level = metLevel;
    while (chain[at].child >= 0) {
        const auto& stage = chain[at];
        if (stage.levelUp) level = std::max(level + 1, static_cast<int>(stage.evoLevel));
        else level = std::max(level, static_cast<int>(stage.evoLevel));
        at = stage.child;
    }
    return level;
}

bool inChain(const std::vector<Stage>& chain, uint16_t species) {
    return std::any_of(chain.begin(), chain.end(), [&](const Stage& s) { return s.species == species; });
}
// PKHeX FormInfo.IsFormChangeable for a Gen V origin.
bool formChangeable(uint16_t species) {
    switch (species) {
        case 412: case 479: case 386: case 483: case 484: case 487: case 492: case 493:
        case 641: case 642: case 645: case 646: case 647: case 649: case 585: case 586:
            return true;
        default:
            return false;
    }
}

// ---------------------------------------------------------------- moves

void addLevelUp(std::bitset<kMaxMove + 1>& pool, const D::LevelMove* moves, const uint16_t* start,
                std::size_t count, int index, int maxLevel) {
    if (index < 0 || static_cast<std::size_t>(index) >= count) return;
    for (uint16_t i = start[index]; i < start[index + 1]; ++i)
        if (moves[i].level <= maxLevel && moves[i].move <= kMaxMove) pool.set(moves[i].move);
}

// PKHeX LearnGroup5: B2W2 level-up, TM/HM, type tutors, B2W2 tutors,
// enhanced tutors, then BW level-up, for one evolution stage.
void addStageMoves(std::bitset<kMaxMove + 1>& pool, uint16_t species, uint8_t form, int maxLevel, uint8_t currentForm, bool current) {
    const Personal p = personal(species, form);
    if (!p.rec) return;
    addLevelUp(pool, D::kLevelUpB2W2, D::kLevelUpB2W2Start, D::kLearnsetB2W2Count, indexB2W2(species, form), maxLevel);
    addLevelUp(pool, D::kLevelUpBW, D::kLevelUpBWStart, D::kLearnsetBWCount, indexBW(species, form), maxLevel);
    for (std::size_t i = 0; i < D::kMachineCount; ++i)
        if (p.tm(static_cast<int>(i))) pool.set(D::kMachineMoves[i]);
    for (std::size_t i = 0; i < D::kTypeTutorCount; ++i)
        if (p.typeTutor(static_cast<int>(i)) && D::kTypeTutorMoves[i] <= kMaxMove) pool.set(D::kTypeTutorMoves[i]);
    for (int t = 0; t < 4; ++t) {
        const uint8_t* bits = p.rec + 0x3C + 4 * t;
        for (std::size_t i = 0; i < D::kTutorCounts[t]; ++i)
            if ((bits[i >> 3] >> (i & 7)) & 1u) pool.set(D::kTutorMoves[t][i]);
    }
    if (species == 647) pool.set(548);  // Keldeo: Secret Sword
    if (species == 648) pool.set(547);  // Meloetta: Relic Song
    if (species == 479 && current && currentForm >= 1 && currentForm <= 5) {
        static constexpr uint16_t kRotom[] = {315, 56, 59, 403, 437};
        pool.set(kRotom[currentForm - 1]);
    }
}

// ---------------------------------------------------------------- reporting

struct Analysis {
    Report report;
    void invalid(CheckIdentifier id, std::string text) { report.issues.push_back({Severity::Invalid, std::move(text), id}); }
    void warn(CheckIdentifier id, std::string text) { report.issues.push_back({Severity::Warning, std::move(text), id}); }
    void info(CheckIdentifier id, std::string text) { report.issues.push_back({Severity::Info, std::move(text), id}); }
};

bool validDate(const std::array<uint8_t, 3>& d) {
    const int year = 2000 + d[0], month = d[1], day = d[2];
    if (d[0] > 99 || month < 1 || month > 12 || day < 1) return false;
    static constexpr int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int limit = kDays[month - 1];
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) limit = 29;
    return day <= limit;
}
bool zeroDate(const std::array<uint8_t, 3>& d) { return d[0] == 0 && d[1] == 0 && d[2] == 0; }

// ---------------------------------------------------------------- encounter templates

enum class Kind : uint8_t { Egg, Slot, Static, Roamer, N, Entree, Radar, Trade, Gift };

const char* kindName(Kind k) {
    switch (k) {
        case Kind::Egg: return "bred egg";
        case Kind::Slot: return "wild encounter";
        case Kind::Static: return "static or gift encounter";
        case Kind::Roamer: return "roaming encounter";
        case Kind::N: return "N's Pokemon";
        case Kind::Entree: return "Entree Forest / Dream World encounter";
        case Kind::Radar: return "Dream Radar encounter";
        case Kind::Trade: return "in-game trade";
        case Kind::Gift: return "Mystery Gift (PGF) event";
    }
    return "encounter";
}

struct Template {
    Kind kind{};
    uint16_t species = 0;  // encounter species (egg: the hatched species)
    int8_t ability = Any12;
    uint8_t shiny = 0;     // 0 random, 1 never, 2 always
    uint8_t fixedBall = 0;
    bool dreamBalls = false;
    int8_t gender = -1, nature = -1;
    uint32_t pid = 0;      // fixed PID (0: none)
    bool correlation = false, fixedGenderPid = false, grotto = false;
    bool egg = false;      // PKHeX IEncounterTemplate.IsEgg
    std::array<uint16_t, 4> moves{};
    const uint8_t* card = nullptr; // PGF
    int trade = -1;        // row of Gen5Data::kTrades
};

struct Candidate {
    Template t;
    std::vector<std::string> fails;
};

struct Pgf {
    const uint8_t* c;
    uint16_t tid() const { return static_cast<uint16_t>(c[0x00] | (c[0x01] << 8)); }
    uint16_t sid() const { return static_cast<uint16_t>(c[0x02] | (c[0x03] << 8)); }
    uint8_t originGame() const { return c[0x04]; }
    uint32_t pid() const { return c[0x08] | (c[0x09] << 8) | (c[0x0A] << 16) | (static_cast<uint32_t>(c[0x0B]) << 24); }
    uint8_t ribbon0() const { return c[0x0C]; }
    uint8_t ribbon1() const { return c[0x0D]; }
    uint8_t ball() const { return c[0x0E]; }
    uint16_t move(int i) const { return static_cast<uint16_t>(c[0x12 + 2 * i] | (c[0x13 + 2 * i] << 8)); }
    uint16_t species() const { return static_cast<uint16_t>(c[0x1A] | (c[0x1B] << 8)); }
    uint8_t form() const { return c[0x1C]; }
    uint8_t language() const { return c[0x1D]; }
    // PGF.Nickname (0x1E, 11 units); the card is nicknamed when it is not empty.
    std::u16string nickname() const {
        std::array<uint16_t, 11> units{};
        for (int i = 0; i < 11; ++i) units[i] = static_cast<uint16_t>(c[0x1E + 2 * i] | (c[0x1F + 2 * i] << 8));
        return Gen5Text::load5(units);
    }
    uint8_t nature() const { return c[0x34]; }
    uint8_t gender() const { return c[0x35]; }
    uint8_t abilityType() const { return c[0x36]; }
    uint8_t pidType() const { return c[0x37]; }
    uint16_t eggLocation() const { return static_cast<uint16_t>(c[0x38] | (c[0x39] << 8)); }
    uint16_t metLocation() const { return static_cast<uint16_t>(c[0x3A] | (c[0x3B] << 8)); }
    uint8_t contest(int i) const { return c[0x3D + i]; }
    uint8_t iv(int i) const { return c[0x43 + i]; }
    uint16_t otChar(int i) const { return static_cast<uint16_t>(c[0x4A + 2 * i] | (c[0x4B + 2 * i] << 8)); }
    uint8_t otGender() const { return c[0x5A]; }
    uint8_t level() const { return c[0x5B]; }
    bool isEgg() const { return c[0x5C] != 0; }
    uint8_t cardType() const { return c[0xB3]; }
};

// PK5 ribbon bits a PGF card sets: {byte in Pk5::ribbons, bit} for PGF
// RIB0 bits 0-7 then RIB1 bits 0-6.
constexpr std::array<std::array<uint8_t, 2>, 15> kPgfRibbonBits{{
    {7, 4}, {7, 5}, {7, 6}, {7, 7},  // Country, National, Earth, World
    {3, 2}, {3, 3}, {2, 3}, {2, 6},  // Classic, Premier, Event, Birthday
    {2, 7}, {3, 0}, {3, 1},          // Special, Souvenir, Wishing
    {7, 1}, {7, 2}, {7, 3}, {2, 5},  // ChampionBattle, Regional, National, World
}};

std::array<uint8_t, 12> pgfRibbons(const Pgf& g) {
    std::array<uint8_t, 12> r{};
    for (int i = 0; i < 15; ++i) {
        const bool set = i < 8 ? (g.ribbon0() >> i) & 1u : (g.ribbon1() >> (i - 8)) & 1u;
        if (set) r[kPgfRibbonBits[i][0]] |= static_cast<uint8_t>(1u << kPgfRibbonBits[i][1]);
    }
    return r;
}

uint8_t abilityNumber(const Pk5& p) {
    return p.hiddenAbility ? 4 : static_cast<uint8_t>(1u << ((p.pid >> 16) & 1u));
}

// Which personal slot (0, 1, 2 = hidden) holds the stored ability; -1 if none.
int abilityIndex(const Pk5& p, const Personal& cur) {
    if (!cur.rec) return -1;
    for (int i = 0; i < 3; ++i) if (cur.ability(i) == p.ability) return i;
    return -1;
}

// Rules every template shares: ability permission, shiny lock, ball, fixed
// gender/nature/PID, and the Gen V PID correlation.
void checkCommon(const Pk5& p, const Template& t, const Personal& cur, std::vector<std::string>& fails) {
    // PKHeX VerifyAbility345 / VerifyFixedAbility: a fixed slot matches when
    // either the stored ability or the PID's slot is that slot.
    const uint8_t number = abilityNumber(p);
    const int index = abilityIndex(p, cur);
    switch (t.ability) {
        case Any12: if (number == 4) fails.push_back("has its hidden ability, which this encounter cannot give"); break;
        case OnlyHidden: if (number != 4) fails.push_back("lacks the hidden ability this encounter always gives"); break;
        case OnlyFirst:
            if (number == 4) fails.push_back("has its hidden ability, which this encounter cannot give");
            else if (number != 1 && index != 0) fails.push_back("ability slot is not the fixed first slot");
            break;
        case OnlySecond:
            if (number == 4) fails.push_back("has its hidden ability, which this encounter cannot give");
            else if (number != 2 && index != 1) fails.push_back("ability slot is not the fixed second slot");
            break;
        default: break;
    }
    const bool shiny = Gen5::isShiny(p.pid, p.tid, p.sid);
    if (t.shiny == 1 && shiny) fails.push_back("is shiny, but this encounter is shiny-locked");
    if (t.shiny == 2 && !shiny) fails.push_back("is not shiny, but this encounter is always shiny");
    if (t.fixedBall) {
        if (p.ball != t.fixedBall) fails.push_back("ball " + std::to_string(p.ball) + " is not this encounter's ball " + std::to_string(t.fixedBall));
    } else if (!(isWildBall5(p.ball) || (t.dreamBalls && p.ball == kBallDream))) {
        fails.push_back("ball " + std::to_string(p.ball) + " cannot be used for this encounter");
    }
    if (t.gender >= 0 && p.gender != t.gender) fails.push_back("gender differs from this encounter's fixed gender");
    if (t.nature >= 0 && p.nature != t.nature) fails.push_back("nature differs from this encounter's fixed nature");
    if (t.pid && p.pid != t.pid) fails.push_back("PID differs from this encounter's fixed PID");
    if (t.grotto && shiny) fails.push_back("is shiny, but Hidden Grotto Pokemon cannot be shiny");
    if (t.correlation && !Gen5::wildPidCorrelated(p.pid, p.tid, p.sid))
        fails.push_back("PID does not have the Gen V wild PID correlation bit");
    if (t.fixedGenderPid) {
        const uint8_t low = p.pid & 0xFFu;
        if ((t.gender == 0 && low >= 254) || (t.gender == 1 && low == 0))
            fails.push_back("PID cannot come from this fixed-gender encounter");
    }
}

// ---------------------------------------------------------------- analyzer

class Gen5Analyzer {
public:
    explicit Gen5Analyzer(const Pk5& p) : p_(p) {}
    Report run();

private:
    const Pk5& p_;
    Analysis a_;
    Personal cur_;
    std::vector<Stage> chain_;
    uint8_t level_ = 1;
    bool gen5Origin_ = false;
    bool matched_ = false;
    Template match_{};
    std::vector<Template> matchAll_; // every template that matches, in search order
    bool matchUnknownEvent_ = false;

    void structure();
    void internal();
    void abilities();
    void forms();
    void eggState();
    void movesBasic();
    void encounterGen5();
    void transfer();
    void movePool();
    std::bitset<kMaxMove + 1> movesFor(const Template* t) const;
    void ribbons();
    void transferRibbons();
    void genderFromDual();
    void wurmple();
    void namesGen5();
    void namesTransfer();
    void emptyNames(const std::u16string& nick, const std::u16string& ot, bool otMatters);
    void playerNickname(const std::u16string& nick);
    void filterName(std::u16string_view text, uint8_t origin, const char* what, bool definite = true);
    bool textUnknown_ = false; // a name rule could not be decided

    std::vector<Candidate> candidates();
    void slotCandidates(std::vector<Candidate>& out);
    void staticCandidates(std::vector<Candidate>& out);
    void tradeCandidates(std::vector<Candidate>& out);
    void eggCandidates(std::vector<Candidate>& out);
    void giftCandidates(std::vector<Candidate>& out);
};

void Gen5Analyzer::structure() {
    auto& c = a_.report.coverage;
    if (p_.species > kMaxSpecies)
        a_.invalid(CheckIdentifier::Species, "Species " + std::to_string(p_.species) + " is above the Gen V maximum 649");
    if (p_.heldItem > kMaxItem)
        a_.invalid(CheckIdentifier::Items, "Held item " + std::to_string(p_.heldItem) + " is above the Gen V maximum 638");
    else if (p_.heldItem != 0 && !std::binary_search(D::kHeldItems, D::kHeldItems + D::kHeldItemCount, p_.heldItem))
        a_.invalid(CheckIdentifier::Items, "Held item " + std::to_string(p_.heldItem) + " cannot be held in Gen V");
    if (p_.ability == 0 || p_.ability > kMaxAbility)
        a_.invalid(CheckIdentifier::Ability, "Ability " + std::to_string(p_.ability) + " is outside the Gen V ability range");
    if (p_.nature > 24) a_.invalid(CheckIdentifier::Stats, "Nature " + std::to_string(p_.nature) + " is not one of the 25 natures");
    if (p_.ball == 0 || p_.ball > kBallDream)
        a_.invalid(CheckIdentifier::Origin, "Ball " + std::to_string(p_.ball) + " is outside the Gen V ball range");
    if (p_.markings > 0x3F) a_.invalid(CheckIdentifier::Misc, "Marking byte uses bits Gen V does not have");
    for (uint16_t m : p_.moves)
        if (m > kMaxMove) a_.invalid(CheckIdentifier::Moves, "Move " + std::to_string(m) + " is above the Gen V maximum 559");
    if (p_.metLevel > 100) a_.invalid(CheckIdentifier::Origin, "Met level above 100");
    gen5Origin_ = isGen5Version(p_.version);
    if (!gen5Origin_ && !isGen3Version(p_.version) && !isGen4Version(p_.version)) {
        a_.invalid(CheckIdentifier::SourceGame, "Origin game " + std::to_string(p_.version) + " cannot exist in a Gen V save");
        c.sourceGame = CoverageLevel::Partial;
    } else {
        c.sourceGame = gen5Origin_ ? CoverageLevel::Complete : CoverageLevel::Partial;
    }
    // Language 6 is unused; 0 is allowed only on Black/White in-game trades
    // (PKHeX LanguageVerifier, GetMaxLanguageID: Korean for Gen IV/V, Spanish for Gen III).
    const bool bwTrade = p_.metLocation == kLinkTrade5NPC && (p_.version == 20 || p_.version == 21);
    if (p_.language > 8 || p_.language == 6 || (p_.language == 0 && !bwTrade))
        a_.invalid(CheckIdentifier::Trainer, "Language " + std::to_string(p_.language) + " is not a Gen V language");
    else if (p_.language == 8 && isGen3Version(p_.version))
        a_.invalid(CheckIdentifier::Trainer, "Korean is not a Gen III language");
}

void Gen5Analyzer::internal() {
    // EVs (no 252 cap in Gen V).
    int evTotal = 0;
    for (uint8_t e : p_.evs) evTotal += e;
    if (evTotal > 510) a_.invalid(CheckIdentifier::Stats, "EV total " + std::to_string(evTotal) + " is above 510");

    // Pokerus: days left cannot exceed the strain's maximum.
    const uint8_t strain = p_.pokerus >> 4, days = p_.pokerus & 15u;
    if (days > (strain & 3u) + 1u)
        a_.invalid(CheckIdentifier::Misc, "Pokerus days left exceed the strain's maximum");

    // Experience and level.
    if (cur_.rec) {
        level_ = Pokemon::getLevelFromExp(p_.exp, cur_.growth());
        if (level_ == 100 && p_.exp > Pokemon::getExpForLevel(100, cur_.growth()))
            a_.invalid(CheckIdentifier::Stats, "Experience is above the level 100 total");
        if (p_.party && p_.partyLevel != level_)
            a_.warn(CheckIdentifier::Stats, "Party level " + std::to_string(p_.partyLevel) +
                       " does not match the level from experience (" + std::to_string(level_) + ")");
        if (!p_.isEgg && p_.metLevel > level_)
            a_.invalid(CheckIdentifier::Origin, "Met level " + std::to_string(p_.metLevel) +
                       " is above the current level " + std::to_string(level_));
    }

    // Gender follows the PID in Gen III-V (current species' ratio).
    if (cur_.rec) {
        const uint8_t ratio = cur_.genderRatio();
        uint8_t expected;
        if (ratio == 255) expected = 2;
        else if (ratio == 254) expected = 1;
        else if (ratio == 0) expected = 0;
        else expected = (p_.pid & 0xFFu) < ratio ? 1 : 0;
        if (p_.gender != expected)
            a_.invalid(CheckIdentifier::Stats, ratio == 255 || p_.gender == 2
                ? "Gender does not match the species' gender rule"
                : "Gender does not match the PID");
    }
    // Gen III/IV origin: the nature still comes from the PID.
    if (!gen5Origin_ && p_.nature != p_.pid % 25)
        a_.invalid(CheckIdentifier::PidRng, "Nature does not match PID % 25, required for Gen III/IV origin");

    // Dates: a met/egg location needs a real calendar date; no location needs none.
    if (p_.metLocation != 0 && !validDate(p_.metDate))
        a_.invalid(CheckIdentifier::Origin, "Met date is not a valid calendar date");
    if (p_.metLocation == 0 && !zeroDate(p_.metDate))
        a_.invalid(CheckIdentifier::Origin, "Met date is set without a met location");
    if (p_.eggLocation != 0 && !validDate(p_.eggDate))
        a_.invalid(CheckIdentifier::Egg, "Egg date is not a valid calendar date");
    if (p_.eggLocation == 0 && !zeroDate(p_.eggDate))
        a_.invalid(CheckIdentifier::Egg, "Egg date is set without an egg location");

    // Encounter type (Gen IV ground tile) is 0 for anything that did not come from Gen IV.
    if (!isGen4Version(p_.version) && p_.groundTile != 0)
        a_.invalid(CheckIdentifier::Encounter, "Gen IV encounter type is set on a Pokemon that did not come from Gen IV");

    // PokeStar Studios fame.
    if (p_.pokeStarFame != 0) {
        if (p_.isEgg) a_.invalid(CheckIdentifier::Misc, "An egg cannot have PokeStar fame");
        else if (p_.pokeStarFame % 25 != 0 && p_.pokeStarFame % 25 != 5)
            a_.invalid(CheckIdentifier::Misc, "PokeStar fame " + std::to_string(p_.pokeStarFame) + " is not a reachable value");
    }

    // Text buffers: a Gen III-V game always ends a name with 0xFFFF, so a nickname
    // fills at most 10 of its 11 units and an OT name 7 of its 8 (deliberately
    // stricter than PKHeX, which reads to the end of the buffer and allows a
    // 12-character Western OT). The PKHeX name rules are in names*().
    auto length = [](const auto& text) {
        std::size_t n = 0;
        while (n < text.size() && text[n] != 0xFFFF) ++n;
        return n;
    };
    const std::size_t nick = length(p_.nickname), ot = length(p_.ot);
    if (nick == 0) a_.invalid(CheckIdentifier::Trainer, "Nickname is empty");
    else if (nick > 10) a_.invalid(CheckIdentifier::Trainer, "Nickname is longer than 10 characters");
    if (!p_.isEgg && ot == 0) a_.invalid(CheckIdentifier::Trainer, "Original Trainer name is empty");
    else if (ot > 7) a_.invalid(CheckIdentifier::Trainer, "Original Trainer name is longer than 7 characters");
}

void Gen5Analyzer::abilities() {
    if (!cur_.rec) return;
    const uint8_t a0 = cur_.ability(0), a1 = cur_.ability(1), ah = cur_.ability(2);
    // PKHeX IsEdgeCaseBasculin: Blue-Striped Basculin has Reckless in Black/White
    // (except White's in-game trade) and from Black 2/White 2 eggs, else Rock Head.
    if (p_.species == 550 && p_.form == 1 && !p_.hiddenAbility && p_.ability != 91 && gen5Origin_) {
        if (!matched_) return;
        uint8_t expect = 69;
        if ((p_.version == 22 || p_.version == 23) && match_.kind == Kind::Egg) expect = 120;
        else if ((p_.version == 20 || p_.version == 21) && !(match_.kind == Kind::Trade)) expect = 120;
        if (p_.ability != expect) a_.invalid(CheckIdentifier::Ability, "Blue-Striped Basculin cannot have this ability from this game and encounter");
        return;
    }
    if (p_.ability != a0 && p_.ability != a1 && p_.ability != ah) {
        a_.invalid(CheckIdentifier::Ability, "Ability " + std::to_string(p_.ability) + " is not one of this species' abilities");
        return;
    }
    if (!gen5Origin_) {
        // PKHeX rejects the flag for every Gen III/IV encounter (CheckMatch) except an
        // unevolved Gen IV card (VerifyAbilityPCD compares the ability only), and a
        // fateful Gen IV Pokemon may be one.
        if (p_.hiddenAbility && p_.fateful && isGen4Version(p_.version))
            a_.warn(CheckIdentifier::Ability, "Hidden ability flag is set on a Gen IV event Pokemon; PKHeX accepts it on "
                                              "an unevolved Gen IV card, which is not identified here");
        else if (p_.hiddenAbility)
            a_.invalid(CheckIdentifier::Ability, "Hidden ability flag is set on a Gen III/IV-origin Pokemon");
        else if (p_.ability != (p_.pid & 1u ? a1 : a0) && a0 != a1)
            // Gen III species that gained a second ability later keep their old one.
            a_.warn(CheckIdentifier::Ability, "Ability does not match the PID slot; Gen III/IV ability history is not checked here");
        return;
    }
    // The PID must pick the stored ability only for encounters that let the
    // PID decide (PKHeX VerifyAbility5 / CheckMatch). Fixed-slot templates and
    // event cards are checked by the encounter match itself.
    if (!matched_ || match_.kind == Kind::Gift) return;
    if (match_.ability > 0 && match_.species == p_.species) return;
    const uint8_t number = abilityNumber(p_);
    const uint8_t expected = number == 4 ? ah : number == 2 ? a1 : a0;
    if (p_.ability == expected || (number != 4 && a0 == a1 && p_.ability == a0)) return;
    if (number == 4 && match_.ability == OnlyHidden) {
        // A hidden-ability-only template (Entree Forest, Hidden Grotto, Dream Radar...),
        // evolved since. No Gen V game is known to keep the flag with a slot 1/2 ability,
        // but nothing from the games proves it (no Gen V decompilation), and PKHeX accepts
        // the record when it matches the current species' template (AbilityVerifier
        // VerifyFixedAbility). Unproven, so a Warning: docs/legality/PKHEX_DISAGREEMENT_REVIEW.md.
        a_.report.coverage.internal = CoverageLevel::Partial;
        a_.warn(CheckIdentifier::Ability, "Hidden ability flag is set but the ability is not the hidden ability "
                                          "(not shown impossible in Gen V; not treated as Invalid)");
        return;
    }
    a_.invalid(CheckIdentifier::Ability, number == 4
        ? "Hidden ability flag is set but the ability is not the hidden ability"
        : "Ability does not match the PID's ability slot");
}

// PKHeX PIDVerifier.VerifyECPIDWurmple: a Silcoon / Beautifly / Cascoon / Dustox
// obtained as Wurmple evolved along the branch its PID's upper half chose. Silcoon
// and Cascoon can also be caught directly, so it only applies when the encounter
// is known to be Wurmple.
void Gen5Analyzer::wurmple() {
    if (p_.species < 266 || p_.species > 269) return;
    const uint32_t branch = ((p_.pid >> 16) % 10) / 5; // 0: Silcoon line, 1: Cascoon line
    if (branch == (p_.species <= 267 ? 0u : 1u)) return;
    const bool fromWurmple = matched_ && match_.species == 265;
    const bool other = std::any_of(matchAll_.begin(), matchAll_.end(), [](const Template& t) { return t.species != 265; });
    if (fromWurmple && !other) {
        a_.invalid(CheckIdentifier::PidRng, "PID evolves Wurmple into the other branch");
    } else if (fromWurmple || !gen5Origin_) {
        a_.warn(CheckIdentifier::PidRng, gen5Origin_
            ? "PID evolves Wurmple into the other branch; valid only for the matching non-Wurmple encounter"
            : "PID evolves Wurmple into the other branch; valid only if it was caught after evolving in Gen III/IV");
        if (gen5Origin_) a_.report.coverage.encounter = CoverageLevel::Partial;
    }
}

// PKHeX GenderVerifier.IsValidFixedGenderFromBiGender: Wormadam, Mothim, Vespiquen,
// Gallade and Froslass keep the gender their PID gave the two-gender species they
// were obtained as.
void Gen5Analyzer::genderFromDual() {
    switch (p_.species) {
        case 413: case 414: case 416: case 475: case 478: break;
        default: return;
    }
    if (p_.gender == 2 || !cur_.rec) return;
    auto pidGender = [&](uint16_t species) {
        const Personal pi = personal(species, 0);
        const uint8_t r = pi.rec ? pi.genderRatio() : 255;
        return r == 255 ? 2 : r == 254 ? 1 : r == 0 ? 0 : ((p_.pid & 0xFFu) < r ? 1 : 0);
    };
    if (gen5Origin_) {
        if (!matched_ || pidGender(match_.species) == p_.gender) return;
        // PKHeX checks the encounter it selected; when another matching template's
        // species fits, which one PKHeX selects is not modelled here.
        const bool other = std::any_of(matchAll_.begin(), matchAll_.end(),
                                       [&](const Template& t) { return pidGender(t.species) == p_.gender; });
        if (other) {
            a_.warn(CheckIdentifier::Stats, "Gender fits the PID only for another matching encounter species");
            a_.report.coverage.encounter = CoverageLevel::Partial;
        } else {
            a_.invalid(CheckIdentifier::Stats, "Gender does not match the PID of the " + speciesName(match_.species) +
                           " it was obtained as");
        }
        return;
    }
    // Gen III/IV origin: the original species is not known here.
    bool fromDual = false, ok = false;
    for (std::size_t i = 1; i < chain_.size(); ++i) {
        const Personal pi = personal(chain_[i].species, 0);
        if (!pi.rec || pi.genderRatio() == 0 || pi.genderRatio() >= 254) continue;
        fromDual = true;
        ok = ok || pidGender(chain_[i].species) == p_.gender;
    }
    if (fromDual && !ok)
        a_.warn(CheckIdentifier::Stats, "Gender does not match the PID of its pre-evolution; only possible if it was obtained as " +
                    speciesName(p_.species) + " in Gen III/IV");
}

void Gen5Analyzer::forms() {
    if (!cur_.rec) return;
    const uint8_t count = cur_.formCount();
    // PKHeX FormInfo.IsValidOutOfBoundsForm: Mothim keeps Burmy's cloak (forms 0-2).
    const bool mothim = p_.species == 414 && p_.form < 3;
    if (p_.form >= std::max<uint8_t>(count, 1) && !mothim) {
        a_.invalid(CheckIdentifier::Species, "Form " + std::to_string(p_.form) + " does not exist for " + speciesName(p_.species));
        return;
    }
    switch (p_.species) {
        case 351: case 421: case 648: // Castform, Cherrim, Meloetta Pirouette
            if (p_.form != 0) a_.invalid(CheckIdentifier::Species, "Battle-only form stored outside battle");
            break;
        case 555: // Darmanitan Zen Mode
            if (p_.form & 1u) a_.invalid(CheckIdentifier::Species, "Battle-only form stored outside battle");
            break;
        case 492: // Shaymin Sky Forme reverts in the PC
            if (p_.form != 0 && !p_.party) a_.invalid(CheckIdentifier::Species, "Shaymin Sky Forme cannot be stored in a box");
            break;
        case 487: // Giratina Origin Forme needs the Griseous Orb
            if ((p_.form == 1) != (p_.heldItem == 112))
                a_.invalid(CheckIdentifier::Items, "Giratina's form does not match the Griseous Orb");
            break;
        case 493: { // Arceus form follows its plate
            static constexpr uint16_t kPlates[] = {303, 306, 304, 305, 309, 308, 310, 313, 298, 299, 301, 300, 307, 302, 311, 312};
            uint8_t expected = 0;
            for (uint8_t i = 0; i < 16; ++i) if (kPlates[i] == p_.heldItem) expected = i + 1;
            if (p_.form != expected) a_.invalid(CheckIdentifier::Items, "Arceus' form does not match its held plate");
            break;
        }
        case 649: { // Genesect form follows its drive
            const uint8_t expected = p_.heldItem >= 116 && p_.heldItem <= 119 ? p_.heldItem - 115 : 0;
            if (p_.form != expected) a_.invalid(CheckIdentifier::Items, "Genesect's form does not match its held drive");
            break;
        }
        case 479: // Rotom cannot hatch in an alternate form
            if (p_.isEgg && p_.form != 0) a_.invalid(CheckIdentifier::Egg, "Rotom egg in an appliance form");
            break;
        case 201: // Gen III Unown's letter comes from its PID
            if (isGen3Version(p_.version)) {
                const uint32_t pid = p_.pid;
                const uint32_t letter = (((pid & 0x3000000u) >> 18) | ((pid & 0x30000u) >> 12) |
                                         ((pid & 0x300u) >> 6) | (pid & 3u)) % 28;
                if (p_.form != letter) a_.invalid(CheckIdentifier::Species, "Unown's form does not match its Gen III PID");
            }
            break;
        default:
            break;
    }
}

void Gen5Analyzer::eggState() {
    if (!p_.isEgg) return;
    if (!gen5Origin_) {
        a_.invalid(CheckIdentifier::Egg, "A Gen III/IV egg cannot be transferred to Gen V");
        return;
    }
    if (cur_.rec && p_.friendship > cur_.hatchCycles())
        a_.invalid(CheckIdentifier::Egg, "Egg hatch counter is above the species' hatch cycles");
    if (std::any_of(p_.evs.begin(), p_.evs.end(), [](uint8_t v) { return v != 0; }))
        a_.invalid(CheckIdentifier::Egg, "Egg has EVs");
    if (std::any_of(p_.contest.begin(), p_.contest.end(), [](uint8_t v) { return v != 0; }))
        a_.invalid(CheckIdentifier::Egg, "Egg has contest stats");
    if (p_.heldItem != 0) a_.invalid(CheckIdentifier::Egg, "Egg is holding an item");
    if (p_.pokerus != 0) a_.invalid(CheckIdentifier::Egg, "Egg has Pokerus");
    if (!p_.isNicknamed) a_.invalid(CheckIdentifier::Egg, "Egg is not flagged as nicknamed");
    if (p_.metLevel != 1) a_.invalid(CheckIdentifier::Egg, "Unhatched egg's met level is not 1");
    if (p_.eggLocation == kLinkTrade5 || p_.eggLocation == kLinkTrade5NPC)
        a_.invalid(CheckIdentifier::Egg, "Unhatched egg has a traded egg location");
    if (cur_.rec && p_.exp != Pokemon::getExpForLevel(1, cur_.growth()))
        a_.invalid(CheckIdentifier::Egg, "Egg's experience is not the level 1 total");
    for (int i = 0; i < 4; ++i) {
        if (p_.ppUps[i] != 0) a_.invalid(CheckIdentifier::Egg, "Egg has PP Ups");
        if (p_.moves[i] && p_.moves[i] <= kMaxMove && p_.pp[i] != D::kMovePP[p_.moves[i]])
            a_.invalid(CheckIdentifier::Egg, "Egg move PP is not the base PP");
    }
}

void Gen5Analyzer::movesBasic() {
    bool seenEmpty = false;
    for (int i = 0; i < 4; ++i) {
        const uint16_t m = p_.moves[i];
        if (m == 0) {
            seenEmpty = true;
            if (p_.ppUps[i] != 0) a_.invalid(CheckIdentifier::Moves, "PP Ups on an empty move slot");
            if (p_.pp[i] != 0) a_.invalid(CheckIdentifier::Moves, "PP on an empty move slot");
            continue;
        }
        if (seenEmpty) a_.invalid(CheckIdentifier::Moves, "A move follows an empty move slot");
        for (int j = 0; j < i; ++j)
            if (p_.moves[j] == m) a_.invalid(CheckIdentifier::Moves, "Duplicate move " + moveName(m));
        if (m > kMaxMove) continue;
        if (p_.ppUps[i] > 3) a_.invalid(CheckIdentifier::Moves, "More than 3 PP Ups on " + moveName(m));
        else if (m == 166 && p_.ppUps[i] != 0) a_.invalid(CheckIdentifier::Moves, "Sketch cannot have PP Ups");
        const uint8_t base = D::kMovePP[m];
        const unsigned max = base * (5u + std::min<uint8_t>(p_.ppUps[i], 3)) / 5u;
        if (p_.pp[i] > max) a_.invalid(CheckIdentifier::Moves, "PP of " + moveName(m) + " is above its maximum");
    }
    if (p_.moves[0] == 0) a_.invalid(CheckIdentifier::Moves, "First move slot is empty");
}

// ---- candidates

void Gen5Analyzer::slotCandidates(std::vector<Candidate>& out) {
    if (p_.isEgg || p_.eggLocation != 0) return;
    const uint8_t bit = versionBit(p_.version);
    for (std::size_t a = 0; a < D::kAreaCount; ++a) {
        const auto& area = D::kAreas[a];
        if (!(area.version & bit) || area.location != p_.metLocation) continue;
        for (uint16_t s = area.slotStart; s < area.slotStart + area.slotCount; ++s) {
            const auto& slot = D::kSlots[s];
            if (!inChain(chain_, slot.species)) continue;
            Candidate c;
            c.t.kind = Kind::Slot;
            c.t.species = slot.species;
            c.t.grotto = area.type == 5;
            c.t.ability = c.t.grotto ? OnlyHidden : Any12;
            c.t.correlation = !c.t.grotto;
            if (p_.metLevel < slot.minLevel || p_.metLevel > slot.maxLevel)
                c.fails.push_back("met level " + std::to_string(p_.metLevel) + " is outside the slot's " +
                                  std::to_string(slot.minLevel) + "-" + std::to_string(slot.maxLevel));
            if (slot.species == p_.species && slot.form != p_.form && !formChangeable(slot.species))
                c.fails.push_back("form differs from the wild slot's form");
            checkCommon(p_, c.t, cur_, c.fails);
            out.push_back(std::move(c));
        }
    }
}

void Gen5Analyzer::staticCandidates(std::vector<Candidate>& out) {
    const uint8_t bit = versionBit(p_.version);
    for (std::size_t i = 0; i < D::kStaticCount; ++i) {
        const auto& s = D::kStatics[i];
        if (!(s.versions & bit) || !inChain(chain_, s.species)) continue;
        const auto kind = static_cast<D::StaticKind>(s.kind);
        Candidate c;
        c.t.species = s.species;
        c.t.ability = s.ability;
        c.t.shiny = s.shiny;
        c.t.fixedBall = s.ball;
        c.t.gender = s.gender;
        // PKHeX EncounterStatic5N matches N's Pokemon by PID, not by nature.
        c.t.nature = kind == D::StaticKind::N ? -1 : s.nature;
        c.t.pid = kind == D::StaticKind::N ? s.pid : 0;
        c.t.moves = {s.move, 0, 0, 0};
        // Location decides whether this template applies at all.
        bool applies = false;
        switch (kind) {
            case D::StaticKind::Static:
                c.t.kind = Kind::Static;
                c.t.egg = s.eggLocation != 0;
                if (s.eggLocation == 0) {
                    applies = p_.metLocation == s.location && p_.eggLocation == 0 && !p_.isEgg;
                } else if (p_.isEgg) {
                    applies = p_.eggLocation == s.eggLocation &&
                              (p_.metLocation == s.location || p_.metLocation == kLinkTrade5 || p_.metLocation == kLinkTrade5NPC);
                } else {
                    applies = p_.eggLocation == s.eggLocation || p_.eggLocation == kLinkTrade5 || p_.eggLocation == kLinkTrade5NPC;
                }
                break;
            case D::StaticKind::Roamer:
                c.t.kind = Kind::Roamer;
                applies = !p_.isEgg && p_.eggLocation == 0 && p_.metLocation < 32 &&
                          ((0b10111111111111111000000000000000u >> p_.metLocation) & 1u);
                break;
            case D::StaticKind::N:
                c.t.kind = Kind::N;
                applies = !p_.isEgg && p_.eggLocation == 0 && p_.metLocation == s.location;
                break;
            case D::StaticKind::Entree:
                c.t.kind = Kind::Entree;
                c.t.dreamBalls = true;
                applies = !p_.isEgg && p_.eggLocation == 0 && p_.metLocation == kEntreeForest;
                break;
            case D::StaticKind::Radar:
                c.t.kind = Kind::Radar;
                applies = !p_.isEgg && p_.eggLocation == 0 && p_.metLocation == kDreamRadar;
                break;
        }
        if (!applies) continue;
        if (kind == D::StaticKind::Radar && s.level == 0) {
            if (p_.metLevel % 5 != 0 || p_.metLevel < 5 || p_.metLevel > 40)
                c.fails.push_back("Dream Radar met level must be a multiple of 5 from 5 to 40");
        } else if (s.eggLocation != 0) {
            // Gift eggs hatch (or wait) at level 1.
            if (p_.metLevel != 1) c.fails.push_back("gift egg's met level is not 1");
            if (!p_.isEgg && !Gen5::validHatchLocation(p_.metLocation, p_.version))
                c.fails.push_back("met location is not a place an egg can hatch in this game");
        } else if (p_.metLevel != s.level) {
            c.fails.push_back("met level " + std::to_string(p_.metLevel) + " is not the encounter level " + std::to_string(s.level));
        }
        if (s.species == p_.species && s.form != p_.form && !formChangeable(s.species))
            c.fails.push_back("form differs from the encounter's form");
        if (kind == D::StaticKind::N) {
            if (p_.tid != 2 || p_.sid != 0) c.fails.push_back("N's Pokemon have trainer ID 2 / SID 0");
            if (std::any_of(p_.ivs.begin(), p_.ivs.end(), [](uint8_t v) { return v != 30; }))
                c.fails.push_back("N's Pokemon have all IVs 30");
            if (p_.otGender) c.fails.push_back("N's Pokemon have a male OT");
        }
        // PKHeX IsWildCorrelationPID and the fixed-gender PID rule.
        c.t.correlation = kind == D::StaticKind::Static && s.shiny == 0 && s.ball != kBallPoke && s.ability != OnlyHidden;
        c.t.fixedGenderPid = s.gender == 0 || s.gender == 1; // every Gen V static kind is IFixedGender
        checkCommon(p_, c.t, cur_, c.fails);
        out.push_back(std::move(c));
    }
}

void Gen5Analyzer::tradeCandidates(std::vector<Candidate>& out) {
    if (p_.isEgg || p_.metLocation != kLinkTrade5NPC || p_.eggLocation != 0) return;
    const uint8_t bit = versionBit(p_.version);
    for (std::size_t i = 0; i < D::kTradeCount; ++i) {
        const auto& t = D::kTrades[i];
        if (!(t.versions & bit) || !inChain(chain_, t.species)) continue;
        Candidate c;
        c.t.kind = Kind::Trade;
        c.t.trade = static_cast<int>(i);
        c.t.species = t.species;
        c.t.ability = t.ability;
        c.t.fixedBall = kBallPoke;
        c.t.gender = t.gender;
        c.t.nature = t.nature;
        c.t.pid = t.pid;
        c.t.shiny = t.bw ? 0 : 1;
        c.t.fixedGenderPid = !t.bw && (t.gender == 0 || t.gender == 1); // PKHeX exempts only BW trades
        if (p_.metLevel != t.level) c.fails.push_back("met level is not the trade's level " + std::to_string(t.level));
        if ((static_cast<uint32_t>(p_.sid) << 16 | p_.tid) != t.id32) c.fails.push_back("trainer ID is not the trade partner's");
        if (p_.otGender != (t.otGender != 0)) c.fails.push_back("OT gender is not the trade partner's");
        for (int s = 0; s < 6; ++s)
            if (t.ivs[s] >= 0 && p_.ivs[s] != t.ivs[s]) { c.fails.push_back("IVs differ from the trade's fixed IVs"); break; }
        if (t.species == p_.species && t.form != p_.form && !formChangeable(t.species))
            c.fails.push_back("form differs from the trade's form");
        checkCommon(p_, c.t, cur_, c.fails);
        out.push_back(std::move(c));
    }
}

void Gen5Analyzer::eggCandidates(std::vector<Candidate>& out) {
    const bool bredLocation = p_.eggLocation == kDaycare5 || p_.eggLocation == kLinkTrade5 || p_.eggLocation == kLinkTrade5NPC;
    if (!bredLocation) return;
    // The hatched species is the chain's base, or the stage above a split-breed baby.
    std::vector<uint16_t> eggSpecies;
    const Stage base = chain_.back();
    eggSpecies.push_back(base.species);
    for (const auto& s : chain_)
        if (splitBreedNotBaby(s.species) && s.species != base.species) eggSpecies.push_back(s.species);
    for (uint16_t species : eggSpecies) {
        Candidate c;
        c.t.kind = Kind::Egg;
        c.t.egg = true;
        c.t.species = species;
        c.t.ability = Any12H;
        c.t.fixedBall = kBallPoke;
        if (!canHatch(species)) c.fails.push_back(speciesName(species) + " cannot hatch from an egg");
        if ((species == 25 || species == 133 || species == 172) && p_.form != 0 && species == p_.species)
            c.fails.push_back("this form cannot hatch from an egg");
        if (abilityNumber(p_) == 4 && hiddenBannedFromEgg5(species))
            c.fails.push_back("hidden ability cannot be passed to " + speciesName(species) + " through a Gen V egg");
        if (p_.isEgg) {
            if (p_.species != species) c.fails.push_back("an unhatched egg must be the hatching species");
            if (p_.metLocation != 0 && p_.metLocation != kLinkTrade5 && p_.metLocation != kLinkTrade5NPC)
                c.fails.push_back("unhatched egg has a met location");
        } else {
            if (p_.metLevel != 1) c.fails.push_back("hatched egg's met level is not 1");
            if (!Gen5::validHatchLocation(p_.metLocation, p_.version))
                c.fails.push_back("met location " + std::to_string(p_.metLocation) + " is not a place an egg can hatch in this game");
        }
        checkCommon(p_, c.t, cur_, c.fails);
        out.push_back(std::move(c));
    }
}

void Gen5Analyzer::giftCandidates(std::vector<Candidate>& out) {
    for (std::size_t i = 0; i < D::kPgfCount; ++i) {
        const Pgf g{D::kPgf + i * D::kPgfSize};
        if (g.cardType() != 1 || !inChain(chain_, g.species())) continue;
        // Location gate first, so unrelated cards are not reported.
        if (!g.isEgg()) {
            if (p_.metLocation != g.metLocation() || p_.eggLocation != g.eggLocation()) continue;
        } else if (p_.eggLocation != g.eggLocation() &&
                   (p_.isEgg || (p_.eggLocation != kLinkTrade5 && p_.eggLocation != kLinkTrade5NPC))) {
            continue;
        }
        Candidate c;
        c.t.kind = Kind::Gift;
        c.t.egg = g.isEgg();
        c.t.species = g.species();
        c.t.card = g.c;
        c.t.fixedBall = g.ball();
        for (int m = 0; m < 4; ++m) c.t.moves[m] = g.move(m);
        const bool shiny = Gen5::isShiny(p_.pid, p_.tid, p_.sid);
        if (!g.isEgg()) {
            if (g.tid() != p_.tid || g.sid() != p_.sid) c.fails.push_back("trainer ID is not the event's");
            for (int k = 0; k < 8; ++k) {
                if (g.otChar(k) != p_.ot[k]) { c.fails.push_back("OT name is not the event's"); break; }
                if (g.otChar(k) == 0xFFFF) break;
            }
            if (g.otGender() < 3 && (g.otGender() != 0) != p_.otGender) c.fails.push_back("OT gender is not the event's");
            if (g.pid() != 0 && g.pid() != p_.pid) c.fails.push_back("PID is not the event's fixed PID");
            if (g.pidType() == 0 && shiny) c.fails.push_back("is shiny, but the event is never shiny");
            if (g.pidType() == 2 && !shiny) c.fails.push_back("is not shiny, but the event is always shiny");
            if (g.originGame() != 0 && g.originGame() != p_.version) c.fails.push_back("origin game is not the event's");
            if (g.language() != 0 && g.language() != p_.language) c.fails.push_back("language is not the event's");
        } else if (p_.eggLocation == g.eggLocation() && g.pidType() == 0 && shiny) {
            c.fails.push_back("is shiny, but the event egg is never shiny");
        }
        // PKHeX G5MGShiny: an always-shiny gift that was not traded as an egg
        // must use the event shiny PID.
        if (g.pidType() == 2 && !(p_.eggLocation == kLinkTrade5 || p_.eggLocation == kLinkTrade5NPC)) {
            const uint32_t low = p_.pid & 0xFFFFu;
            if (low > 0xFF || p_.pid != Gen5::shinyPid(low, (p_.pid >> 16) & 1u, p_.tid, p_.sid))
                c.fails.push_back("PID is not an event shiny PID");
        }
        if (g.species() == p_.species && g.form() != p_.form && !formChangeable(g.species()))
            c.fails.push_back("form differs from the event's form");
        if (g.level() != p_.metLevel) c.fails.push_back("met level is not the event's level");
        if (g.ball() != p_.ball) c.fails.push_back("ball is not the event's ball");
        if (g.nature() != 0xFF && g.nature() != p_.nature) c.fails.push_back("nature is not the event's");
        if (g.gender() != 2 && g.gender() != p_.gender) c.fails.push_back("gender is not the event's");
        for (int k = 0; k < 6; ++k)
            if (p_.contest[k] < g.contest(k)) { c.fails.push_back("contest stats are below the event's"); break; }
        // Fixed IVs; 0xFC-0xFE ask for that many perfect IVs.
        int perfect = 0;
        for (uint8_t v : p_.ivs) perfect += v == 31;
        for (int k = 0; k < 6; ++k) {
            const uint8_t v = g.iv(k);
            if (v <= 31 && p_.ivs[k] != v) { c.fails.push_back("IVs differ from the event's fixed IVs"); break; }
            if (v >= 0xFC && v <= 0xFE && perfect < v - 0xFB) { c.fails.push_back("fewer perfect IVs than the event gives"); break; }
        }
        const uint8_t number = abilityNumber(p_);
        const uint8_t type = g.abilityType();
        if (type <= 2 && number != (1u << type)) c.fails.push_back("ability slot is not the event's fixed slot");
        if (type == 3 && number == 4) c.fails.push_back("has its hidden ability, which the event cannot give");
        if (!p_.fateful) c.fails.push_back("event Pokemon must have the fateful encounter flag");
        out.push_back(std::move(c));
    }
}

std::vector<Candidate> Gen5Analyzer::candidates() {
    std::vector<Candidate> out;
    // Same search order as PKHeX EncounterEnumerator5: events (fateful),
    // trades, eggs, statics, wild slots.
    if (p_.fateful) giftCandidates(out);
    tradeCandidates(out);
    eggCandidates(out);
    staticCandidates(out);
    slotCandidates(out);
    // An evolved Pokemon must have gained the levels its evolutions need.
    if (!p_.isEgg) {
        for (auto& c : out) {
            if (c.t.species == p_.species) continue;
            const int need = minimumLevel(chain_, c.t.species, c.t.kind == Kind::Egg ? 1 : p_.metLevel);
            if (level_ < need)
                c.fails.push_back("evolving from " + speciesName(c.t.species) + " needs level " +
                                  std::to_string(need) + ", but it is level " + std::to_string(level_));
        }
    }
    return out;
}

// PKHeX MethodFinder.GetBACDMatch for a shiny PID: the seeds whose next two LCRNG calls
// give the IVs (LCRNGReversal.GetSeedsIVs), tested against the Gen III event PID
// patterns (CommonEvent3) a shiny PID can fit. Only the BACD searches are modelled:
// their 16-bit comparisons make a chance hit about 1 in 65536; PKHeX's other searches
// compare all 32 PID bits.
bool gen3EventPattern(const Pk5& p) {
    constexpr uint32_t kMult = 0x41C64E6Du, kAdd = 0x6073u, kRMult = 0xEEB9EB65u, kRAdd = 0x0A3561A1u;
    auto next = [&](uint32_t s) { return s * kMult + kAdd; };
    auto prev = [&](uint32_t s) { return s * kRMult + kRAdd; };
    // Pk5::ivs is in storage order HP Atk Def Spe SpA SpD: the two 15-bit halves.
    const uint32_t iv1 = p.ivs[0] | (p.ivs[1] << 5) | (p.ivs[2] << 10);
    const uint32_t iv2 = p.ivs[3] | (p.ivs[4] << 5) | (p.ivs[5] << 10);
    const uint32_t pid = p.pid, idXor = p.tid ^ p.sid;
    for (uint32_t high = 0; high < 2; ++high) {
        for (uint32_t low = 0; low < 0x10000u; ++low) {
            const uint32_t first = (high << 31) | (iv1 << 16) | low;
            if (((next(first) >> 16) & 0x7FFFu) != iv2) continue;
            const uint32_t seed = prev(first);
            const uint32_t b16 = seed >> 16, a16 = prev(seed) >> 16, x16 = prev(prev(seed)) >> 16;
            const uint32_t regular = (a16 << 16) | b16;
            if (regular == pid) return true;                                              // BACD
            const uint32_t delta = pid - regular;
            if ((pid & 7u) == 0 && delta != 0 && delta < 8) return true;                  // BACD_EA
            const uint32_t antishiny = ((a16 ^ idXor ^ b16) << 16) | b16;
            if ((a16 & ~7u) != 0 && (pid & 0xFFFFu) == b16 && (antishiny >> 19) != (pid >> 19)) return true; // BACD_EAX
            const uint32_t force = (x16 << 16) | (((idXor ^ x16) & 0xFFF8u) | (b16 & 7u));
            if (x16 == pid >> 16 && force == pid) return true;                            // BACD_S
        }
    }
    return false;
}

void Gen5Analyzer::encounterGen5() {
    auto& cov = a_.report.coverage;
    const auto all = candidates();
    const Candidate* best = nullptr;
    for (const auto& c : all) {
        if (c.fails.empty()) { best = &c; break; }
        if (!best || c.fails.size() < best->fails.size()) best = &c;
    }
    if (best && best->fails.empty()) {
        matched_ = true;
        match_ = best->t;
        // Dream World entries repeat per special move; any matching one may be the source.
        for (const auto& c : all)
            if (c.fails.empty()) matchAll_.push_back(c.t);
        cov.encounter = CoverageLevel::Complete;
        cov.pidRng = CoverageLevel::Complete;
        cov.eventGift = CoverageLevel::Complete;
        a_.info(CheckIdentifier::Encounter, std::string("Matches a Gen V ") + kindName(match_.kind) +
                " (" + speciesName(match_.species) + ")");
        if (p_.fateful && match_.kind != Kind::Gift)
            a_.invalid(CheckIdentifier::EventGift, "Fateful encounter flag is set on a non-event encounter");
        if (match_.kind == Kind::N && !p_.nSparkle)
            a_.invalid(CheckIdentifier::Encounter, "N's Pokemon must have N's sparkle flag");
        if (match_.kind != Kind::N && p_.nSparkle)
            a_.invalid(CheckIdentifier::Encounter, "N's sparkle flag is set on a Pokemon that is not N's");
        if (match_.kind == Kind::Slot && match_.species == 132 && p_.pokeStarFame != 0)
            a_.invalid(CheckIdentifier::Misc, "Ditto must have zero PokeStar fame");
        if (match_.kind == Kind::Gift && Pgf{match_.card}.pidType() == 2 && !(p_.eggLocation == kLinkTrade5 || p_.eggLocation == kLinkTrade5NPC) &&
            gen3EventPattern(p_)) {
            // PKHeX MiscFatefulEncounterVerifier accepts an event shiny PID only when
            // MethodFinder.Analyze calls it G5MGShiny, and that search tries the Gen III
            // event (BACD) patterns first. A real event Pokemon can hit one by chance.
            cov.pidRng = CoverageLevel::Partial;
            a_.warn(CheckIdentifier::PidRng, "PID and IVs also fit a Gen III event RNG pattern, which PKHeX tries before the "
                                          "Gen V event shiny PID; how PKHeX rules on it is not modelled here");
        }
        return;
    }
    cov.encounter = CoverageLevel::Partial;
    cov.pidRng = CoverageLevel::Partial;
    const bool eventRange = p_.metLocation > 40000 && p_.metLocation < 50000;
    if (p_.fateful && !best) {
        // A distribution that is not in the pinned card database is unknown,
        // not proven impossible.
        matchUnknownEvent_ = true;
        cov.eventGift = CoverageLevel::Partial;
        a_.warn(CheckIdentifier::EventGift, "Fateful encounter does not match any event card in the pinned Gen V database");
        return;
    }
    cov.eventGift = p_.fateful ? CoverageLevel::Partial : CoverageLevel::Complete;
    if (!best) {
        std::string why = eventRange
            ? "Met location " + std::to_string(p_.metLocation) + " is an event location, but the fateful encounter flag is not set"
            : "No Gen V encounter gives " + speciesName(p_.species) + " at met location " + std::to_string(p_.metLocation);
        a_.invalid(CheckIdentifier::Encounter, why);
        return;
    }
    std::string why = std::string("Closest Gen V encounter (") + kindName(best->t.kind) + ", " +
                      speciesName(best->t.species) + ") does not match: ";
    for (std::size_t i = 0; i < best->fails.size(); ++i) why += (i ? "; " : "") + best->fails[i];
    a_.invalid(CheckIdentifier::Encounter, why);
}

// A Gen III/IV in-game trade name (any language); see kTransferTradeNames.
bool transferTradeName(std::u16string_view name) {
    const auto* begin = D::kTransferTradeNames;
    const auto* end = begin + D::kTransferTradeNameCount;
    return std::binary_search(begin, end, name, [](std::u16string_view a, std::u16string_view b) { return a < b; });
}

// PKHeX ContestStatInfo for a Gen III/IV origin (CorrelateSheen) from zero initial
// stats: Pokeblocks / Poffins raise sheen along with the stats. {minimum, maximum}.
std::array<int, 2> sheenRange(const Pk5& p, bool gen3) {
    // NatureAmp in visual order Atk, Def, SpA, SpD, Spe (nature = up * 5 + down, internal
    // order Atk, Def, Spe, SpA, SpD).
    auto amps = [](uint8_t nature) {
        std::array<int, 5> a{};
        static constexpr int kVisual[5] = {0, 1, 4, 2, 3};
        const int up = nature / 5, down = nature % 5;
        if (nature < 25 && up != down) { a[kVisual[up]] = 1; a[kVisual[down]] = -1; }
        return a;
    };
    // Cool Atk, Tough Def, Beauty SpA, Smart SpD, Cute Spe; PK5 order Cool Beauty Cute Smart Tough.
    auto gained = [&](uint8_t nature) {
        const auto a = amps(nature);
        const int stat[5] = {p.contest[0], p.contest[4], p.contest[1], p.contest[3], p.contest[2]};
        int sum = 0;
        for (int i = 0; i < 5; ++i) {
            const int gain = stat[i];
            if (gain <= 0) continue;
            if (a[i] == 0) { sum += gain; continue; }
            const int undo = a[i] == 1 ? 11 : 9;
            const int boost = gain / undo + (gain % undo >= 5 ? 1 : 0);
            sum += a[i] == -1 ? gain + boost : gain - boost;
        }
        return sum;
    };
    auto feel = [&](uint8_t nature) { return (gained(nature) + 4) / 5; }; // ceil(sum / 5)
    const bool none = std::all_of(p.contest.begin(), p.contest.end(), [](uint8_t v) { return v == 0; });
    if (none) return {0, 0};
    // CalculateMinimumSheen3 / 4.
    int minimum;
    const int raw = feel(0);
    if (raw == 255) {
        minimum = gen3 ? 214 : 255;
    } else {
        int avg = std::max(1, p.nature % 6 == 0 ? raw : feel(p.nature));
        avg = std::min(raw, avg);
        minimum = gen3 ? std::clamp(214 * avg / 255, 1, 214) : std::clamp(avg, 11, 255);
    }
    // CalculateMaximumSheen (+3).
    int maximum;
    if (std::any_of(p.contest.begin(), p.contest.begin() + 5, [](uint8_t v) { return v == 255; })) {
        maximum = 255;
    } else if (gen3) {
        const int sum = gained(p.nature);
        int gained2 = 0;
        for (int i = 0; i < 5; ++i) gained2 += p.contest[static_cast<std::size_t>(i)] >= 2;
        maximum = sum == 0 ? 0 : std::clamp(gained2 >= 3 ? sum * 83 / 6 : sum * 19 / 9, 1, 255);
    } else {
        const int avg = feel(p.nature);
        maximum = avg <= 0 ? 0 : avg <= 2 ? 59 : std::min(255, avg * 79);
    }
    return {minimum, maximum};
}

void Gen5Analyzer::transfer() {
    auto& cov = a_.report.coverage;
    cov.encounter = CoverageLevel::Partial;
    cov.pidRng = CoverageLevel::Partial;
    cov.eventGift = CoverageLevel::Partial;
    cov.transfer = CoverageLevel::Partial;
    if (p_.species > 493)
        a_.invalid(CheckIdentifier::Transfer, "Species above 493 cannot come from Gen III/IV");
    if (p_.nSparkle) a_.invalid(CheckIdentifier::Transfer, "N's sparkle flag on a transferred Pokemon");
    const bool celebi = p_.species == 251, beast = p_.species >= 243 && p_.species <= 245;
    const bool special = p_.fateful && isGen4Version(p_.version) && (celebi || beast);
    const bool met =
        p_.metLocation == kTransfer4 ||
        (special && celebi && (p_.metLocation == kTransfer4CelebiUnused || p_.metLocation == kTransfer4CelebiUsed)) ||
        (special && beast && (p_.metLocation == kTransfer4CrownUnused || p_.metLocation == kTransfer4CrownUsed));
    if (!met) a_.invalid(CheckIdentifier::Transfer, "Met location " + std::to_string(p_.metLocation) + " is not a Poke Transfer location");
    // PKHeX ContestStatVerifier: sheen follows the contest stats Pokeblocks / Poffins
    // gave. In-game trades and Gen IV events start with stats of their own (unknown here).
    const auto sheen = sheenRange(p_, isGen3Version(p_.version));
    if (p_.contest[5] < sheen[0] || p_.contest[5] > sheen[1]) {
        const std::string text = "Contest sheen " + std::to_string(p_.contest[5]) + " is outside " + std::to_string(sheen[0]) +
                                 "-" + std::to_string(sheen[1]) + ", the range its contest stats allow";
        // Only Gen IV cards (all fateful) and the EncounterTrade3 / EncounterTrade4PID
        // trades (whose OTs are trade names) start with contest stats in PKHeX.
        const bool gen4Card = p_.fateful && isGen4Version(p_.version);
        if (gen4Card || transferTradeName(T::load5(p_.ot))) a_.warn(CheckIdentifier::Misc, text + " from zero; it may have started with an event's or trade's stats");
        else a_.invalid(CheckIdentifier::Misc, text);
    }
    // PKHeX CXDVerifier: the Colosseum / XD player characters are male.
    if (p_.version == kVersionCXD && p_.otGender != 0)
        a_.invalid(CheckIdentifier::Trainer, "Colosseum / XD Pokemon have a male Original Trainer");
    if (isGen3Version(p_.version)) {
        // Pal Park (PKHeX PK3.ConvertToPK4): the PK3 has no egg location, so it stays 0,
        // and every Gen III encounter is a species up to Deoxys (386).
        if (p_.eggLocation != 0)
            a_.invalid(CheckIdentifier::Transfer, "Gen III-origin Pokemon cannot have an egg location");
        if (std::none_of(chain_.begin(), chain_.end(), [](const Stage& s) { return s.species <= 386; }))
            a_.invalid(CheckIdentifier::Transfer, speciesName(p_.species) + " and its pre-evolutions did not exist in Gen III");
        // PKHeX BallUseLegality: Gen III balls are Master..Premier (1-12).
        if (p_.ball > 12) a_.invalid(CheckIdentifier::Origin, "Ball " + std::to_string(p_.ball) + " did not exist in Gen III");
        if (p_.species == 172 && p_.form == 1)
            a_.invalid(CheckIdentifier::Species, "Spiky-eared Pichu did not exist in Gen III");
    } else {
        // Gen IV: no Dream Ball; Apricorn and Sport Balls only in HeartGold / SoulSilver
        // (PKHeX BallUseLegality WildPokeBalls4_DPPt / _HGSS, Bug-Catching Contest).
        if (p_.ball == kBallDream) a_.invalid(CheckIdentifier::Origin, "The Dream Ball did not exist in Gen IV");
        else if (p_.ball >= 17 && p_.ball <= 24 && p_.version >= 10)
            a_.invalid(CheckIdentifier::Origin, "Ball " + std::to_string(p_.ball) + " exists only in HeartGold / SoulSilver");
        // Spiky-eared Pichu: the HeartGold / SoulSilver Ilex Forest event encounter only.
        if (p_.species == 172 && p_.form == 1 && p_.version >= 10)
            a_.invalid(CheckIdentifier::Species, "Spiky-eared Pichu can only come from HeartGold / SoulSilver");
    }
    if (isGen3Version(p_.version) && p_.metLevel < 5 && p_.eggLocation == 0)
        a_.warn(CheckIdentifier::Transfer, "Transferred Gen III Pokemon below level 5");
    if (p_.metLevel == 0) a_.invalid(CheckIdentifier::Transfer, "Transferred Pokemon must have its transfer level as met level");
    a_.warn(CheckIdentifier::Transfer, "Gen III/IV origin: the original encounter and moves are not checked by the Gen V analyzer");
}

// PKHeX Breeding.GetCanInheritMoves: level-up moves pass to an egg only through a
// female (or the Nidoran / Volbeat-Illumise pairs).
bool canInheritLevelUp(uint16_t species) {
    if (species == 29 || species == 32 || species == 313 || species == 314) return true;
    const Personal p = personal(species, 0);
    return p.rec && p.genderRatio() != 0 && p.genderRatio() != 255;
}

// Moves a Pokemon can know when `t` is its encounter (nullptr: any stage of the
// chain, used when the encounter is not known).
std::bitset<kMaxMove + 1> Gen5Analyzer::movesFor(const Template* t) const {
    std::bitset<kMaxMove + 1> pool;
    // PKHeX EvolutionChain.GetEvolutionChainsAllGens: only the encounter species and
    // the species it evolved into can have learned moves, every stage up to the
    // current level (EvolutionUtil.CleanEvolve).
    std::vector<bool> stage(chain_.size(), t == nullptr);
    if (t) {
        int at = -1;
        for (std::size_t i = 0; i < chain_.size(); ++i)
            if (chain_[i].species == t->species) { at = static_cast<int>(i); break; }
        if (at < 0) std::fill(stage.begin(), stage.end(), true);
        for (; at >= 0; at = chain_[static_cast<std::size_t>(at)].child) stage[static_cast<std::size_t>(at)] = true;
    }
    for (std::size_t i = 0; i < chain_.size(); ++i) {
        if (!stage[i]) continue;
        const auto& s = chain_[i];
        const bool current = i == 0;
        if (s.species == 386 || s.species == 487 || s.species == 492) { // Deoxys, Giratina, Shaymin: every form
            const Personal p = personal(s.species, 0);
            for (uint8_t f = 0; p.rec && f < p.formCount(); ++f) addStageMoves(pool, s.species, f, level_, p_.form, current);
        } else {
            addStageMoves(pool, s.species, s.form, level_, p_.form, current);
        }
    }
    if (!t) return pool;
    for (uint16_t m : t->moves) if (m && m <= kMaxMove) pool.set(m);
    if (t->kind == Kind::Egg) {
        // PKHeX LearnGroup5.CheckEncounterMoves: egg moves, the hatching game's
        // level-up moves at any level when they can be inherited, Volt Tackle for Pichu.
        const uint16_t s = t->species;
        if (s < D::kEggMoveCount)
            for (uint16_t i = D::kEggMoveStart[s]; i < D::kEggMoveStart[s + 1]; ++i) pool.set(D::kEggMoves[i]);
        if (canInheritLevelUp(s)) {
            if (p_.version == 22 || p_.version == 23)
                addLevelUp(pool, D::kLevelUpB2W2, D::kLevelUpB2W2Start, D::kLearnsetB2W2Count, indexB2W2(s, p_.form), 100);
            else
                addLevelUp(pool, D::kLevelUpBW, D::kLevelUpBWStart, D::kLearnsetBWCount, indexBW(s, p_.form), 100);
        }
        if (s == 172) pool.set(344);
    }
    return pool;
}

void Gen5Analyzer::movePool() {
    auto& cov = a_.report.coverage;
    const bool smeargle = inChain(chain_, 235);
    auto missing = [&](const std::bitset<kMaxMove + 1>& pool) {
        std::vector<uint16_t> out;
        for (uint16_t m : p_.moves) {
            if (m == 0 || m > kMaxMove || pool.test(m)) continue;
            if (smeargle && m != 165 && m != 448) continue; // Sketch
            out.push_back(m);
        }
        return out;
    };
    if (!gen5Origin_ || !matched_) {
        const auto bad = missing(movesFor(nullptr));
        for (uint16_t m : bad)
            a_.warn(CheckIdentifier::Moves, moveName(m) + " is not learnable in Gen V; its source is not checked here");
        cov.moves = CoverageLevel::Partial;
        return;
    }
    // An unhatched egg from a template with moves (an event egg) still has exactly
    // those moves (PKHeX LearnVerifierEgg.VerifyFromEncounter).
    if (p_.isEgg && match_.kind != Kind::Egg) {
        if (std::all_of(match_.moves.begin(), match_.moves.end(), [](uint16_t m) { return m == 0; })) {
            cov.moves = CoverageLevel::Partial;
            a_.warn(CheckIdentifier::Moves, "Unhatched event egg without card moves: its initial moves are not checked here");
            return;
        }
        std::size_t n = 0;
        while (n < 4 && match_.moves[n]) ++n;
        for (std::size_t i = 0; i < 4; ++i) {
            const uint16_t expect = i < n ? match_.moves[i] : 0;
            if (p_.moves[i] != expect) {
                a_.invalid(CheckIdentifier::Moves, "Unhatched event egg's moves differ from the card's moves");
                break;
            }
        }
        cov.moves = CoverageLevel::Complete;
        return;
    }
    // PKHeX keeps the first matching encounter whose moves are all learnable.
    std::vector<uint16_t> best;
    bool found = false;
    for (const auto& t : matchAll_) {
        auto bad = missing(movesFor(&t));
        if (bad.empty()) { found = true; break; }
        if (best.empty() || bad.size() < best.size()) best = std::move(bad);
    }
    if (!found)
        for (uint16_t m : best) a_.invalid(CheckIdentifier::Moves, moveName(m) + " cannot be learned by this Pokemon in Gen V");
    // An unhatched bred egg's moves follow the inheritance order (PKHeX MoveBreed),
    // which is not replayed here; once hatched, each move only needs a source.
    if (p_.isEgg && match_.kind == Kind::Egg) {
        cov.eggBreeding = CoverageLevel::Partial;
        cov.moves = CoverageLevel::Partial;
        a_.warn(CheckIdentifier::Moves, "Unhatched egg: the order of inherited egg moves is not checked here");
        return;
    }
    cov.moves = CoverageLevel::Complete;
}

// PKHeX RibbonVerifier for a Gen III/IV-origin PK5 (RibbonVerifierCommon3/4,
// Unique3/4, Event3/4 with the Gen III/IV history every transfer has). The original
// encounter is not known, so a ribbon is Invalid only when no Gen III/IV encounter
// can explain it; a ribbon a Gen IV card may have given is a Warning.
void Gen5Analyzer::transferRibbons() {
    const auto& r = p_.ribbons; // 0x24-0x27, 0x3C-0x3F, 0x60-0x63
    auto has = [&](std::size_t i, int bit) { return ((r[i] >> bit) & 1) != 0; };
    const bool gen3 = isGen3Version(p_.version);
    const bool spiky = p_.species == 172 && p_.form == 1;
    // RibbonRules.BattleFrontierBanlist, up to Arceus.
    static constexpr uint16_t kBanned[] = {150, 151, 249, 250, 251, 382, 383, 384, 385, 386, 483, 484, 487, 489, 490, 491, 492, 493};
    const bool banned = std::find(std::begin(kBanned), std::end(kBanned), p_.species) != std::end(kBanned);
    std::vector<std::string> bad, card;
    auto flag = [&](std::size_t i, int bit, const char* name) { if (has(i, bit)) bad.emplace_back(name); };
    // Hoenn ribbons (Common3 / Unique3): Gen III only; Battle Tower ribbons not for banned species.
    if (!gen3) {
        flag(6, 4, "Hoenn Champion");
        flag(6, 7, "Artist");
    }
    if (!gen3 || banned) {
        flag(6, 5, "Winning");
        flag(6, 6, "Victory");
    }
    // Common4: the Record Ribbon was never distributed; spiky-eared Pichu cannot reach
    // the Sinnoh League or the Ribbon Syndicate, whose ribbons are bought in order.
    flag(2, 2, "Record");
    if (spiky) {
        flag(0, 0, "Sinnoh Champion");
        flag(1, 6, "Gorgeous");
        flag(1, 7, "Royal");
        flag(2, 0, "Gorgeous Royal");
    } else {
        if (has(1, 7) && !has(1, 6)) bad.emplace_back("Royal (needs Gorgeous)");
        if (has(2, 0) && !(has(1, 6) && has(1, 7))) bad.emplace_back("Gorgeous Royal (needs Gorgeous and Royal)");
    }
    // Unique4: Battle Frontier ability ribbons.
    if (banned || spiky) {
        static constexpr const char* kAbility[] = {"Ability", "Great Ability", "Double Ability", "Multi Ability", "Pair Ability", "World Ability"};
        for (int bit = 1; bit <= 6; ++bit) flag(0, bit, kAbility[bit - 1]);
    }
    // Contest ribbons: Hoenn ones only from Gen III; Sinnoh ones not for Ditto, Unown or
    // spiky-eared Pichu. Each rank needs the ranks below it.
    static constexpr const char* kCategory[] = {"Cool", "Beauty", "Cute", "Smart", "Tough"};
    auto contest = [&](std::size_t first, bool allowed, const char* region, const char* const* ranks) {
        for (int k = 0; k < 5; ++k) {
            const std::size_t i = first + static_cast<std::size_t>(k / 2);
            const int shift = (k % 2) * 4;
            for (int rank = 3; rank >= 0; --rank) {
                if (!has(i, shift + rank)) continue;
                const bool lower = rank == 0 || (has(i, shift + rank - 1) && (rank < 2 || has(i, shift + rank - 2)) &&
                                                 (rank < 3 || has(i, shift)));
                if (!allowed || !lower)
                    bad.emplace_back(std::string(region) + ' ' + kCategory[k] + ranks[rank] + (allowed ? " (needs the lower ranks)" : ""));
            }
        }
    };
    static constexpr const char* kRanks3[] = {"", " Super", " Hyper", " Master"};
    static constexpr const char* kRanks4[] = {"", " Great", " Ultra", " Master"};
    contest(4, gen3, "Hoenn", kRanks3);
    contest(8, !(p_.species == 132 || p_.species == 201 || spiky), "Sinnoh", kRanks4);
    // Event ribbons (Event3 / Event4). Gen III: EncounterGift3 can give only the National
    // Ribbon (the Metang gifts), Colosseum / XD shadow Pokemon get it when purified and
    // any Gen III Pokemon can earn the Earth Ribbon on Mt. Battle. Gen IV: only a card
    // (PCD, always fateful) gives event ribbons.
    struct EventRibbon { std::size_t i; int bit; const char* name; };
    static constexpr EventRibbon kEvent[] = {
        {7, 1, "Battle Champion"}, {7, 2, "Regional Champion"}, {7, 3, "National Champion"}, {7, 4, "Country"},
        {7, 5, "National"},        {7, 6, "Earth"},             {7, 7, "World"},             {2, 3, "Event"},
        {2, 5, "World Champion"},  {2, 6, "Birthday"},          {2, 7, "Special"},           {3, 0, "Souvenir"},
        {3, 1, "Wishing"},         {3, 2, "Classic"},           {3, 3, "Premier"}};
    const bool metang = std::any_of(chain_.begin(), chain_.end(), [](const Stage& s) { return s.species == 375; });
    for (const auto& e : kEvent) {
        if (!has(e.i, e.bit)) continue;
        const std::string name = e.name;
        if (gen3) {
            if (name == "Earth" || (name == "National" && (p_.version == kVersionCXD || metang))) continue;
            bad.push_back(name);
        } else if (p_.fateful) {
            card.push_back(name);
        } else {
            bad.push_back(name);
        }
    }
    auto join = [](const std::vector<std::string>& v) {
        std::string s;
        for (const auto& x : v) s += (s.empty() ? "" : ", ") + x;
        return s;
    };
    if (!bad.empty())
        a_.invalid(CheckIdentifier::Misc, "Ribbons no Gen III/IV history can give: " + join(bad));
    if (!card.empty())
        a_.warn(CheckIdentifier::Misc, "Ribbons only a Gen IV event card gives (the card is not identified here): " + join(card));
}

void Gen5Analyzer::ribbons() {
    if (!gen5Origin_) {
        if (isGen3Version(p_.version) || isGen4Version(p_.version)) transferRibbons();
        return;
    }
    // Gen V games have no contests; only a gift card can carry contest stats
    // (checked against the card in the match).
    bool cardContest = false;
    if (matched_ && match_.kind == Kind::Gift)
        for (int k = 0; k < 6; ++k) cardContest = cardContest || Pgf{match_.card}.contest(k) != 0;
    if (!matchUnknownEvent_ && !cardContest &&
        std::any_of(p_.contest.begin(), p_.contest.end(), [](uint8_t v) { return v != 0; }))
        a_.invalid(CheckIdentifier::Misc, "Contest stats are not obtainable on a Gen V-origin Pokemon");
    std::array<uint8_t, 12> allowed{};
    if (matched_ && match_.kind == Kind::Gift) allowed = pgfRibbons(Pgf{match_.card});
    else if (matchUnknownEvent_) return; // ribbons belong to the unknown card
    // Unused bits (0x27 high nibble, 0x62 high nibble, 0x63) are not ribbons.
    static constexpr std::array<uint8_t, 12> kUsed{0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x00};
    for (std::size_t i = 0; i < allowed.size(); ++i) {
        if ((p_.ribbons[i] & kUsed[i]) != allowed[i]) {
            a_.invalid(CheckIdentifier::Misc, matched_ && match_.kind == Kind::Gift
                ? "Ribbons differ from the event's ribbons"
                : "Gen V-origin Pokemon cannot have ribbons outside an event");
            return;
        }
    }
}

// ---- names (PKHeX NicknameVerifier, TrainerNameVerifier, LanguageVerifier)

// Word filter and digit limit for a player-chosen name (nickname or OT). Text with a
// unit outside Gen5Text's domain cannot be filtered here: Warning, coverage Partial.
// `definite` false: the name may be fixed by an encounter PKHeX does not filter, so a
// hit is a Warning.
void Gen5Analyzer::filterName(std::u16string_view text, uint8_t origin, const char* what, bool definite) {
    if (!T::inDomain(text)) {
        textUnknown_ = true;
        a_.warn(CheckIdentifier::Trainer, std::string(what) + " \"" + utf8(text) +
                    "\" has characters the word filter is not modelled for; the filter is not checked");
        return;
    }
    auto report = [&](std::string text) {
        if (definite) a_.invalid(CheckIdentifier::Trainer, std::move(text));
        else a_.warn(CheckIdentifier::Trainer, std::move(text) + "; allowed only for an in-game trade or event name");
    };
    const auto hit = T::wordFilter(text, origin);
    if (hit.list != T::Filter::None) {
        const char* list = hit.list == T::Filter::Switch ? "Switch" : hit.list == T::Filter::Console3DS ? "3DS" : "Gen V";
        report(std::string(what) + " \"" + utf8(text) + "\" is blocked by the " + list + " word filter (\"" +
               utf8(T::filterPattern(hit.list, hit.index)) + "\")");
    }
    if (T::tooManyNumbers(text, origin)) report(std::string(what) + " \"" + utf8(text) + "\" has more than 4 digits");
}

// PKHeX VerifyUnNicknamedEncounter + the word filter, for a Gen V-origin nickname
// that is not fixed by a trade (and not an egg).
void Gen5Analyzer::playerNickname(const std::u16string& nick) {
    const uint8_t lang = p_.language;
    if (p_.isNicknamed) {
        // A name of any species (any generation's list) is only "fishy" in PKHeX.
        if (T::speciesAnyLanguage(nick, 5) != 0) return;
        if (std::any_of(nick.begin(), nick.end(), [](char16_t c) { return c >= 0x4E00 && c <= 0x9FFF; })) {
            a_.invalid(CheckIdentifier::Trainer, "Nickname \"" + utf8(nick) + "\" has CJK ideographs, which Gen V cannot type");
            return;
        }
        const std::size_t max = lang == 1 || lang == 8 ? 5 : 10;
        if (nick.size() > max) {
            // A traded egg may keep a longer name from another language (English
            // maximum); internal() reports more than 10.
            const bool tradedEgg = match_.egg && p_.eggLocation == kLinkTrade5;
            if (nick.size() <= 10 && !tradedEgg)
                a_.invalid(CheckIdentifier::Trainer, "Japanese / Korean nickname is longer than 5 characters");
            return;
        }
        filterName(nick, 5, "Nickname");
        return;
    }
    // Not nicknamed: the species name for its language, or any language after evolving,
    // as a traded egg, or for a fixed-language card without a nickname.
    if (nick == T::speciesNameGeneration(p_.species, lang, 5)) return;
    const bool anyLanguage = match_.species != p_.species || p_.eggLocation == kLinkTrade5;
    if (anyLanguage && !T::isNicknamedAnyLanguage(p_.species, nick, 5)) return;
    if (match_.kind == Kind::Gift) {
        const Pgf g{match_.card};
        if (g.language() != 0 && g.nickname().empty() && !T::isNicknamedAnyLanguage(p_.species, nick, 5)) return;
    }
    a_.invalid(CheckIdentifier::Trainer, "Name \"" + utf8(nick) +
                   "\" is not the species name for its language, but the Pokemon is not flagged as nicknamed");
}

// PKHeX reads a name up to 0xFFFF or 0; internal() already reported a name that
// starts with 0xFFFF.
void Gen5Analyzer::emptyNames(const std::u16string& nick, const std::u16string& ot, bool otMatters) {
    if (nick.empty() && p_.nickname[0] != 0xFFFF) a_.invalid(CheckIdentifier::Trainer, "Nickname is empty");
    if (otMatters && ot.empty() && (p_.isEgg || p_.ot[0] != 0xFFFF))
        a_.invalid(CheckIdentifier::Trainer, "Original Trainer name is empty");
}

void Gen5Analyzer::namesGen5() {
    const std::u16string nick = T::load5(p_.nickname), ot = T::load5(p_.ot);
    const uint8_t lang = p_.language;
    const bool playerOT = matched_ && match_.kind != Kind::Trade && match_.kind != Kind::N && match_.kind != Kind::Gift;
    emptyNames(nick, ot, playerOT);
    if (!matched_) {
        textUnknown_ = true; // the name rules depend on the encounter
        return;
    }
    const bool trade = match_.kind == Kind::Trade, n = match_.kind == Kind::N, gift = match_.kind == Kind::Gift;
    // ---- nickname
    if (!nick.empty()) {
        bool done = false;
        if (gift && !match_.egg && p_.isNicknamed && Pgf{match_.card}.nickname().empty())
            a_.invalid(CheckIdentifier::Trainer, "Event Pokemon is nicknamed, but its card gives no nickname");
        if (gift && !match_.egg) {
            const std::u16string cardName = Pgf{match_.card}.nickname();
            if (!cardName.empty() && nick != cardName) {
                // PKHeX does not compare the name with the card's; the receiver cannot rename it.
                textUnknown_ = true;
                a_.warn(CheckIdentifier::EventGift, "Nickname \"" + utf8(nick) + "\" differs from the event card's \"" +
                            utf8(cardName) + "\"; PKHeX does not check this");
            }
        }
        if (trade || n) {
            const auto& t = D::kTrades[trade ? match_.trade : 0];
            int L = lang;
            if (trade && t.bw) {
                if (lang == 1)
                    a_.invalid(CheckIdentifier::Trainer, "Black/White in-game trades from Japanese games have language 0, not Japanese");
                L = std::max(L, 1);
            }
            const std::u16string_view expectOT = n ? (L == 1 ? std::u16string_view(u"\uff2e") : std::u16string_view(u"N"))
                                                   : (L < 9 ? std::u16string_view(D::kTradeTrainers[match_.trade][L]) : u"\uffff");
            if (ot != expectOT)
                a_.invalid(CheckIdentifier::Trainer, "OT name \"" + utf8(ot) + "\" is not the " +
                               (n ? std::string("N's Pokemon") : std::string("in-game trade")) + " OT for language " + std::to_string(L));
            if (trade && t.fixedNickname) {
                if (!(L < 9 && nick == std::u16string_view(D::kTradeNicknames[match_.trade][L])))
                    a_.invalid(CheckIdentifier::Trainer, "Nickname \"" + utf8(nick) + "\" is not the in-game trade's nickname for language " +
                                   std::to_string(L));
                done = true;
            }
        }
        if (!done && p_.isEgg) {
            // PKHeX VerifyNicknameEgg (the flag itself is checked with the egg).
            if (nick != T::eggName(lang))
                a_.invalid(CheckIdentifier::Egg, "Egg name \"" + utf8(nick) + "\" is not the egg name for language " + std::to_string(lang));
            done = true;
        }
        if (!done) playerNickname(nick);
    }
    // ---- OT (only when the player is the original trainer)
    if (!playerOT || ot.empty()) return;
    // PKHeX: 6 for Japanese / Korean, 12 otherwise (internal() stops Western at 7);
    // a traded egg may carry any language's name up to the Western length.
    const bool asian = lang == 1 || lang == 8;
    const bool eggEdge = match_.egg && (p_.isEgg ? p_.metLocation == kLinkTrade5 : p_.eggLocation == kLinkTrade5);
    if (asian && ot.size() > 6 && ot.size() <= 7 && !eggEdge)
        a_.invalid(CheckIdentifier::Trainer, "Japanese / Korean Original Trainer name is longer than 6 characters");
    else if (asian && ot.size() == 6) {
        // PKHeX accepts 6; Japanese and Korean name entry stops at 5.
        textUnknown_ = true;
        a_.warn(CheckIdentifier::Trainer, "Original Trainer name is longer than Japanese / Korean name entry allows");
    }
    filterName(ot, 5, "OT name");
}

// PKHeX NicknameVerifier / TrainerNameVerifier for a Gen III/IV-origin PK5. The
// original encounter is not known here, so a rule is Invalid only when no Gen III/IV
// encounter can explain the name: in-game trades fix their names (kTransferTradeNames;
// tools/pkhex_gen5_diff/run.sh checks the table holds every fixed Gen III/IV nickname
// and every non-fateful Gen IV fixed OT PKHeX knows), Gen III gifts and fateful Gen IV
// events fix their OT, and a traded egg or an evolved Pokemon may keep another
// language's species name.
void Gen5Analyzer::namesTransfer() {
    const std::u16string nick = T::load5(p_.nickname), ot = T::load5(p_.ot);
    textUnknown_ = true; // fixed-name encounters (trades, events) are not identified here
    emptyNames(nick, ot, false);
    const uint8_t origin = isGen3Version(p_.version) ? 3 : 4;
    const uint8_t lang = p_.language;
    const bool asian = lang == 1 || lang == 8;
    const bool tradedEgg = origin == 4 && p_.eggLocation == kLinkTrade4;
    if (!nick.empty()) {
        if (p_.isNicknamed) {
            // VerifyUnNicknamedEncounter: a species name is only "fishy" in PKHeX; an
            // in-game trade's nickname is checked against the trade instead.
            if (T::speciesAnyLanguage(nick, 5) == 0 && !transferTradeName(nick)) {
                if (std::any_of(nick.begin(), nick.end(), [](char16_t c) { return c >= 0x4E00 && c <= 0x9FFF; })) {
                    a_.invalid(CheckIdentifier::Trainer, "Nickname \"" + utf8(nick) + "\" has CJK ideographs, which Gen III/IV cannot type");
                } else if (nick.size() > (asian ? 5u : 10u)) {
                    // More than 10 units is reported by internal(); a traded Gen IV egg may
                    // keep a Western-length name.
                    if (nick.size() <= 10 && !tradedEgg)
                        a_.invalid(CheckIdentifier::Trainer, "Japanese / Korean nickname is longer than 5 characters");
                } else {
                    filterName(nick, origin, "Nickname");
                }
            }
        } else {
            // VerifyUnNicknamed / IsMatch45: the species name of its language in the Gen V or
            // Gen IV form (Farfetch'd also upper-cased).
            const uint16_t sp = p_.species;
            std::u16string upper = T::speciesNameGeneration(sp, lang, 5);
            for (auto& c : upper) c = T::toUpperInvariant(c);
            const bool own = nick == T::speciesNameGeneration(sp, lang, 5) || nick == T::speciesNameGeneration(sp, lang, 4) ||
                             (sp == 83 && nick == upper);
            const bool anyLanguage = !T::isNicknamedAnyLanguage(sp, nick, 5) || !T::isNicknamedAnyLanguage(sp, nick, 4);
            if (own || (anyLanguage && tradedEgg)) {
                // fine
            } else if (transferTradeName(nick)) {
                a_.warn(CheckIdentifier::Trainer, "Name \"" + utf8(nick) + "\" is an in-game trade nickname but the nickname flag is clear");
            } else if (anyLanguage && chain_.size() > 1) {
                a_.warn(CheckIdentifier::Trainer, "Name \"" + utf8(nick) +
                            "\" is the species name in another language: valid only if it evolved in that language's game");
            } else {
                a_.invalid(CheckIdentifier::Trainer, "Name \"" + utf8(nick) +
                               "\" is not the species name for its language, but the Pokemon is not flagged as nicknamed");
            }
        }
    }
    if (!ot.empty()) {
        // TrainerNameVerifier checks only a player's OT. Gen III gifts and Colosseum / XD
        // gifts, Gen IV events and My Pokemon Ranch gifts (fateful) and in-game trades
        // (the trade table) fix theirs.
        const bool definite = origin == 4 && !p_.fateful && !transferTradeName(ot);
        // PKHeX allows 6 Japanese / Korean characters (12 for a traded Gen IV egg).
        if (asian && ot.size() > 6 && ot.size() <= 7 && !tradedEgg) {
            if (definite) a_.invalid(CheckIdentifier::Trainer, "Japanese / Korean Original Trainer name is longer than 6 characters");
            else a_.warn(CheckIdentifier::Trainer, "Japanese / Korean Original Trainer name is longer than 6 characters; "
                                                   "allowed only for an in-game trade or event name");
        }
        filterName(ot, origin, "OT name", definite);
    }
}

Report Gen5Analyzer::run() {
    auto& cov = a_.report.coverage;
    cov.structure = CoverageLevel::Complete;
    cov.internal = CoverageLevel::Partial; // set by the name rules at the end
    cov.eggBreeding = CoverageLevel::Complete;
    cov.transfer = CoverageLevel::Complete;
    structure();
    cur_ = personal(p_.species <= kMaxSpecies ? p_.species : 0, p_.form);
    if (p_.species > kMaxSpecies || !cur_.rec) {
        cov.sourceGame = CoverageLevel::Partial;
        return a_.report;
    }
    chain_ = chainOf(p_.species, p_.form);
    internal();
    forms();
    eggState();
    movesBasic();
    if (gen5Origin_) encounterGen5();
    else if (isGen3Version(p_.version) || isGen4Version(p_.version)) transfer();
    abilities();
    genderFromDual();
    wurmple();
    movePool();
    ribbons();
    if (gen5Origin_) namesGen5();
    else namesTransfer();
    cov.internal = textUnknown_ ? CoverageLevel::Partial : CoverageLevel::Complete;
    if (!gen5Origin_) cov.eggBreeding = CoverageLevel::Partial;
    return a_.report;
}

} // namespace

Report analyzePk5(std::span<const uint8_t> record, const Pk5Input& input) {
    Report report{};
    if (record.size() != 136 && record.size() != 220) {
        report.coverage.structure = CoverageLevel::None;
        report.issues.push_back({Severity::Warning, "Not a 136-byte or 220-byte PK5 record", CheckIdentifier::Structure});
        return report;
    }
    if (std::all_of(record.begin(), record.end(), [](uint8_t b) { return b == 0; })) return report;
    std::vector<uint8_t> plain = input.encrypted
        ? PokeVault::Integration::Gen5::Crypto::decrypt(record)
        : std::vector<uint8_t>(record.begin(), record.end());
    Pk5 p;
    if (!Gen5::read(plain, p)) {
        report.coverage.structure = CoverageLevel::None;
        report.issues.push_back({Severity::Warning, "PK5 record could not be decrypted", CheckIdentifier::Structure});
        return report;
    }
    if (Gen5::checksum(plain) != p.checksum) {
        report.coverage.structure = CoverageLevel::Partial;
        report.issues.push_back({Severity::Invalid, "PK5 checksum does not match its data", CheckIdentifier::Checksum});
        return report;
    }
    if (p.sanity != 0) {
        report.coverage.structure = CoverageLevel::Partial;
        report.issues.push_back({Severity::Invalid, "PK5 sanity field is set (bad egg)", CheckIdentifier::Structure});
        return report;
    }
    if (p.species == 0) return report;
    return Gen5Analyzer(p).run();
}

} // namespace Legality
