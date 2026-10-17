/**
 * Gen3Legality.cpp - Gen III PK3 legality analyzer (R/S/E/FR/LG and Colosseum/XD origins).
 *
 * A port of the PKHeX.Core Gen III legality pipeline at the commit pinned in
 * Gen3LegalityData.h: PID/IV method detection (MethodFinder), the encounter
 * generator and finder (EncounterGenerator3 / EncounterGenerator3GC /
 * EncounterFinder, including the Method H lead/slot frame search, event gift
 * seed revision, the GameCube shadow team locks (LockFinder / TeamLockResult),
 * Poke Spot seeds (MethodPokeSpot) and the starter / trainer-ID seeds of
 * MethodCXD), evolution chains, move learnability (LearnGroup3, MoveBreed3) and
 * every verifier that runs for a PK3. See docs/legality/GEN3_PKHEX_RULE_INVENTORY.md
 * for the rule list and the documented severity policy.
 *
 * The analyzer only reads its input and returns a Legality::Report.
 */
#include "Legality/Gen3Legality.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Legality/Gen3LegalityData.h"
#include "Legality/Gen3LegalityDataCxd.h"
#include "Names/MoveNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon.h"

namespace Legality {

namespace Gen3Pk3 {

namespace D = ::Legality::Gen3Data;
namespace X = ::Legality::Gen3DataCxd;

namespace {

// ---------------------------------------------------------------------------
// Personal data, experience and simple PK3 derivations
// ---------------------------------------------------------------------------

const D::Personal& personal(uint16_t species) noexcept {
    return D::kPersonal[species <= D::kMaxSpecies ? species : 0];
}

uint8_t genderFromRatio(uint32_t pid, uint8_t ratio) noexcept {
    if (ratio == 255) return 2;
    if (ratio == 254) return 1;
    if (ratio == 0) return 0;
    return (pid & 0xFFu) < ratio ? 1 : 0;
}

uint8_t levelFromExp(uint32_t exp, uint8_t growth) noexcept {
    const auto& table = D::kExpTable[growth < 6 ? growth : 0];
    if (exp >= table[99]) return 100;
    uint8_t level = 1;
    while (exp >= table[level]) ++level;
    return level;
}

uint32_t expForLevel(uint8_t level, uint8_t growth) noexcept {
    if (level < 1) level = 1;
    if (level > 100) level = 100;
    return D::kExpTable[growth < 6 ? growth : 0][level - 1];
}

uint8_t unownForm(uint32_t pid) noexcept {
    const uint32_t value = ((pid & 0x3000000u) >> 18) | ((pid & 0x30000u) >> 12) |
                           ((pid & 0x300u) >> 6) | (pid & 0x3u);
    return static_cast<uint8_t>(value % 28);
}

bool isShinyPid(uint32_t pid, uint16_t tid, uint16_t sid) noexcept {
    return ((tid ^ sid ^ (pid >> 16) ^ (pid & 0xFFFFu)) & 0xFFFFu) < 8;
}

const char16_t* speciesName3(uint16_t species, uint8_t language) noexcept {
    if (language >= 8 || species > D::kMaxSpecies) return u"";
    if (species == 0) return u"タマゴ"; // タマゴ: every Gen III egg is named in Japanese
    const char16_t* const* row = D::kSpeciesNames3[language];
    if (row == nullptr || row[species] == nullptr) return u"";
    return row[species];
}

// Version ids stored in PK3 Origins (GameVersion S=1, R=2, E=3, FR=4, LG=5, CXD=15).
constexpr uint8_t kVerS = 1, kVerR = 2, kVerE = 3, kVerFR = 4, kVerLG = 5, kVerCXD = 15;

uint8_t versionBit(uint8_t version) noexcept {
    switch (version) {
        case kVerS: return D::S;
        case kVerR: return D::R;
        case kVerE: return D::E;
        case kVerFR: return D::FR;
        case kVerLG: return D::LG;
        case kVerCXD: return D::CXD;
        default: return 0;
    }
}

bool isGbaVersion(uint8_t version) noexcept { return version >= kVerS && version <= kVerLG; }

// Learnset index (0 RS, 1 E, 2 FR, 3 LG) for a template version mask, following
// GameData.GetLearnSource (R/S/RS/RSE -> RS; E/FRLG/EFL/CXD/Gen3 -> E).
int learnSourceForMask(uint8_t mask) noexcept {
    if (mask == D::FR) return 2;
    if (mask == D::LG) return 3;
    if ((mask & ~(D::R | D::S | D::E)) == 0 && (mask & (D::R | D::S)) != 0) return 0;
    return 1;
}

int learnSourceForVersion(uint8_t version) noexcept {
    switch (version) {
        case kVerE: case kVerCXD: return 1; // GameData.GetLearnSource: COLO/XD/CXD -> LearnSource3E
        case kVerFR: return 2;
        case kVerLG: return 3;
        default: return 0;
    }
}

struct LevelRange { const D::LevelMove* begin; const D::LevelMove* end; };

LevelRange learnset(int source, uint16_t species) noexcept {
    if (source < 0 || source > 3 || species > D::kMaxSpecies) return {nullptr, nullptr};
    const auto* base = D::kLevelUp[source];
    const auto* start = D::kLevelUpStart[source];
    return {base + start[species], base + start[species + 1]};
}

/// Learnset.TryGetLevelLearnMove: level of the first occurrence of move.
bool levelLearnMove(int source, uint16_t species, uint16_t move, uint8_t& level) noexcept {
    const auto range = learnset(source, species);
    for (const auto* it = range.begin; it != range.end; ++it) {
        if (it->move == move) { level = it->level; return true; }
    }
    return false;
}

bool learnsetContains(int source, uint16_t species, uint16_t move) noexcept {
    uint8_t level = 0;
    return levelLearnMove(source, species, move, level);
}

/// Learnset.GetBaseEggMoves: the last (up to 4) moves learned at or below level.
std::vector<uint16_t> baseEggMoves(int source, uint16_t species, uint8_t level) {
    const auto range = learnset(source, species);
    size_t count = 0;
    for (const auto* it = range.begin; it != range.end; ++it) {
        if (it->level > level) break;
        ++count;
    }
    size_t start = 0;
    if (count > 4) { start = count - 4; count = 4; }
    std::vector<uint16_t> out;
    for (size_t i = 0; i < count; ++i) out.push_back(range.begin[start + i].move);
    return out;
}

bool isEggMove(uint16_t species, uint16_t move) noexcept {
    if (species >= D::kEggMoveCount) return false;
    for (uint16_t i = D::kEggMoveStart[species]; i < D::kEggMoveStart[species + 1]; ++i)
        if (D::kEggMoves[i] == move) return true;
    return false;
}

int indexOf(std::span<const uint16_t> list, uint16_t value) noexcept {
    for (size_t i = 0; i < list.size(); ++i)
        if (list[i] == value) return static_cast<int>(i);
    return -1;
}

bool canLearnTM(uint16_t species, uint16_t move) noexcept {
    const int index = indexOf(D::kMachineMovesTechnical, move);
    return index >= 0 && species <= D::kMaxSpecies && ((D::kMachineBits[species] >> index) & 1u) != 0;
}

bool canLearnHM(uint16_t species, uint16_t move) noexcept {
    const int index = indexOf(D::kMachineMovesHidden, move);
    return index >= 0 && species <= D::kMaxSpecies && ((D::kMachineBits[species] >> (50 + index)) & 1u) != 0;
}

bool canLearnTutorE(uint16_t species, uint16_t move) noexcept {
    const int index = indexOf(D::kTutorE, move);
    return index >= 0 && species <= D::kMaxSpecies && ((D::kTutorBits[species] >> index) & 1u) != 0;
}

bool sortedContains(const uint16_t* list, size_t count, uint16_t value) noexcept {
    return std::binary_search(list, list + count, value);
}

bool heldItemValid(uint16_t item) noexcept {
    if (item > D::kHeldItemMax) return false;
    return ((D::kHeldItemBits[item >> 3] >> (item & 7)) & 1u) != 0;
}

uint8_t movePP(uint16_t move) noexcept { return move <= D::kMaxMove ? D::kMovePP[move] : 0; }

// ---------------------------------------------------------------------------
// RNG (LCRNG, XDRNG, ARNG) and seed reversal
// ---------------------------------------------------------------------------

constexpr uint32_t lcNext(uint32_t s) noexcept { return s * 0x41C64E6Du + 0x6073u; }
constexpr uint32_t lcPrev(uint32_t s) noexcept { return s * 0xEEB9EB65u + 0x0A3561A1u; }
constexpr uint32_t lcNextN(uint32_t s, int n) noexcept { for (int i = 0; i < n; ++i) s = lcNext(s); return s; }
constexpr uint32_t lcPrevN(uint32_t s, int n) noexcept { for (int i = 0; i < n; ++i) s = lcPrev(s); return s; }
inline uint32_t lcNext16(uint32_t& s) noexcept { s = lcNext(s); return s >> 16; }
inline uint32_t lcPrev16(uint32_t& s) noexcept { s = lcPrev(s); return s >> 16; }
inline uint32_t lcNext15(uint32_t& s) noexcept { s = lcNext(s); return (s >> 16) & 0x7FFFu; }

constexpr uint32_t xdNext(uint32_t s) noexcept { return s * 0x343FDu + 0x269EC3u; }
constexpr uint32_t xdPrev(uint32_t s) noexcept { return s * 0xB9B33155u + 0xA170F641u; }
constexpr uint32_t xdNextN(uint32_t s, int n) noexcept { for (int i = 0; i < n; ++i) s = xdNext(s); return s; }
constexpr uint32_t xdPrevN(uint32_t s, int n) noexcept { for (int i = 0; i < n; ++i) s = xdPrev(s); return s; }
inline uint32_t xdPrev16(uint32_t& s) noexcept { s = xdPrev(s); return s >> 16; }

constexpr uint32_t arPrev(uint32_t s) noexcept { return s * 0x9638806Du + 0x69C77F93u; }

/// Small fixed seed buffer; PKHeX proves every reversal below yields at most 12 seeds.
struct Seeds {
    std::array<uint32_t, 32> v{};
    size_t n = 0;
    void push(uint32_t x) noexcept { if (n < v.size()) v[n++] = x; }
    const uint32_t* begin() const noexcept { return v.data(); }
    const uint32_t* end() const noexcept { return v.data() + n; }
};

constexpr uint32_t kLcMod = 0x67D3, kLcPat = 0xD3E, kLcInc = 0x4034;
constexpr uint32_t kLcMod2 = 0x3A89, kLcPat2 = 0x2E4C, kLcInc2 = 0x5831;

/// LCRNGReversal.GetSeeds: origin seeds whose next two 16-bit outputs are first/second.
Seeds lcSeeds(uint32_t first, uint32_t second) noexcept {
    Seeds out;
    const uint32_t diff = (second - first * 0x41C64E6Du) >> 16;
    uint32_t low = ((((diff * kLcMod) + kLcInc) >> 16) * kLcPat) % kLcMod;
    do {
        const uint32_t seed = first | low;
        if ((lcNext(seed) & 0xFFFF0000u) == second) out.push(lcPrev(seed));
    } while ((low += kLcMod) < 0x10000u);
    return out;
}

void lcAddSeedsIVs(Seeds& out, uint32_t low, uint32_t first, uint32_t second) noexcept {
    do {
        const uint32_t test = first | low;
        if ((lcNext(test) & 0x7FFF0000u) != second) continue;
        const uint32_t seed = lcPrev(test);
        out.push(seed);
        out.push(seed ^ 0x80000000u);
    } while ((low += kLcMod) < 0x10000u);
}

/// LCRNGReversal.GetSeedsIVs: seeds whose next two 15-bit outputs are first/second.
Seeds lcSeedsIVs(uint32_t first, uint32_t second) noexcept {
    Seeds out;
    const uint32_t diff = (second - first * 0x41C64E6Du) >> 16;
    const uint32_t start1 = ((((diff * kLcMod) + kLcInc) >> 16) * kLcPat) % kLcMod;
    const uint32_t start2 = (((((diff ^ 0x8000u) * kLcMod) + kLcInc) >> 16) * kLcPat) % kLcMod;
    lcAddSeedsIVs(out, start1, first, second);
    lcAddSeedsIVs(out, start2, first, second);
    return out;
}

/// LCRNGReversalSkip.GetSeeds: first and third outputs known (one skipped call).
Seeds lcSeedsSkip(uint32_t first, uint32_t third) noexcept {
    Seeds out;
    const uint32_t diff = (third - lcNextN(first, 2)) >> 16;
    uint32_t low = ((((diff * kLcMod2) + kLcInc2) >> 16) * kLcPat2) % kLcMod2;
    do {
        const uint32_t seed = first | low;
        if ((lcNextN(seed, 2) & 0xFFFF0000u) == third) out.push(lcPrev(seed));
    } while ((low += kLcMod2) < 0x10000u);
    return out;
}

/// XDRNG.GetSeeds: origin seeds whose next two 16-bit outputs are first/second.
Seeds xdSeeds(uint32_t first, uint32_t second) noexcept {
    Seeds out;
    constexpr uint32_t kMult = 0x343FDu;
    constexpr uint32_t kSub = 0x269EC3u - 0xFFFFu;
    constexpr uint64_t kBase = (uint64_t{kMult} + 1u) * 0xFFFFu;
    uint64_t t = static_cast<uint32_t>(second - first * kMult - kSub);
    const uint64_t kmax = (kBase - t) >> 32;
    for (uint64_t k = 0; k <= kmax; ++k, t += 0x100000000ull) {
        if (t % kMult < 0x10000u)
            out.push(xdPrev(first | static_cast<uint16_t>(t / kMult)));
    }
    return out;
}

inline uint32_t xdNext16(uint32_t& s) noexcept { s = xdNext(s); return s >> 16; }
inline uint32_t xdNext15(uint32_t& s) noexcept { s = xdNext(s); return (s >> 16) & 0x7FFFu; }
constexpr uint32_t xdPrev1000(uint32_t s) noexcept { return 0x251CC8E1u * s + 0x94750758u; } // XDRNG.Prev1000

/// XDRNG.GetSeedsIVs (XDRNG.cs:271-287): seeds whose next two 15-bit outputs are
/// first/second (both already shifted << 16); each hit also yields its top-bit flip.
Seeds xdSeedsIVs(uint32_t first, uint32_t second) noexcept {
    Seeds out;
    constexpr uint32_t kMult = 0x343FDu;
    constexpr uint32_t kSub = 0x269EC3u - 0xFFFFu;
    constexpr uint64_t kBase = (uint64_t{kMult} + 1u) * 0xFFFFu;
    uint64_t t = static_cast<uint32_t>(second - first * kMult - kSub) & 0x7FFFFFFFu;
    const uint64_t kmax = (kBase - t) >> 31;
    for (uint64_t k = 0; k <= kmax; ++k, t += 0x80000000ull) {
        if (t % kMult < 0x10000u) {
            const uint32_t s = xdPrev(first | static_cast<uint16_t>(t / kMult));
            out.push(s);
            out.push(s ^ 0x80000000u);
        }
    }
    return out;
}

uint32_t xdSequentialIV32(uint32_t seed) noexcept {
    uint32_t result = 0;
    for (int i = 0; i < 6; ++i) {
        seed = xdNext(seed);
        result |= (seed >> 27) << (i * 5);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Strings
// ---------------------------------------------------------------------------

char16_t quoteLeft(uint8_t language) noexcept {
    switch (language) {
        case 2: case 4: case 7: return u'“';
        case 3: return u'«';
        case 5: return u'„';
        default: return u'『';
    }
}

char16_t quoteRight(uint8_t language) noexcept {
    switch (language) {
        case 2: case 4: case 7: return u'”';
        case 3: return u'»';
        case 5: return u'“';
        default: return u'』';
    }
}

/// Invariant lower-casing for the characters a Gen III string or a species name can hold.
char16_t toLowerInvariant(char16_t c) noexcept {
    if (c >= u'A' && c <= u'Z') return static_cast<char16_t>(c + 0x20);
    if (c < 0xC0) return c;
    if ((c >= 0xC0 && c <= 0xDE && c != 0xD7)) return static_cast<char16_t>(c + 0x20);
    if (c >= 0x100 && c <= 0x137) return (c & 1) == 0 ? static_cast<char16_t>(c + 1) : c;
    if (c >= 0x139 && c <= 0x148) return (c & 1) == 1 ? static_cast<char16_t>(c + 1) : c;
    if (c >= 0x14A && c <= 0x177) return (c & 1) == 0 ? static_cast<char16_t>(c + 1) : c;
    if (c == 0x178) return 0xFF;
    if (c == 0x179 || c == 0x17B || c == 0x17D) return static_cast<char16_t>(c + 1);
    if (c >= 0x391 && c <= 0x3AB && c != 0x3A2) return static_cast<char16_t>(c + 0x20);
    if (c == 0x386) return 0x3AC;
    if (c >= 0x388 && c <= 0x38A) return static_cast<char16_t>(c + 0x25);
    if (c == 0x38C) return 0x3CC;
    if (c == 0x38E || c == 0x38F) return static_cast<char16_t>(c + 0x3F);
    if (c >= 0x400 && c <= 0x40F) return static_cast<char16_t>(c + 0x50);
    if (c >= 0x410 && c <= 0x42F) return static_cast<char16_t>(c + 0x20);
    if (c >= 0x2160 && c <= 0x216F) return static_cast<char16_t>(c + 0x10);
    if (c >= 0x24B6 && c <= 0x24CF) return static_cast<char16_t>(c + 0x1A);
    if (c >= 0xFF21 && c <= 0xFF3A) return static_cast<char16_t>(c + 0x20);
    return c;
}

std::u16string lowerInvariant(std::u16string_view s) {
    std::u16string out(s);
    for (auto& c : out) c = toLowerInvariant(c);
    return out;
}

bool isWhiteSpace(char16_t c) noexcept {
    return c == u' ' || (c >= 0x09 && c <= 0x0D) || c == 0x85 || c == 0xA0 || c == 0x1680 ||
           (c >= 0x2000 && c <= 0x200A) || c == 0x2028 || c == 0x2029 || c == 0x202F ||
           c == 0x205F || c == 0x3000;
}

bool allWhiteSpace(std::u16string_view s) noexcept {
    for (char16_t c : s) if (!isWhiteSpace(c)) return false;
    return true;
}

bool startsWithIgnoreCase(std::u16string_view s, std::u16string_view prefix) noexcept {
    if (s.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i)
        if (toLowerInvariant(s[i]) != toLowerInvariant(prefix[i])) return false;
    return true;
}

/// StringConverter.HasEastAsianScriptCharacters.
bool hasEastAsianScript(std::u16string_view s) noexcept {
    for (char16_t c : s) {
        if (c >= 0x4E00 && c <= 0x9FFF) return true;
    }
    return false;
}

} // namespace

std::u16string decodeString(std::span<const uint8_t> raw, uint8_t language) {
    const char16_t* table = language == 1 ? D::kCharsJP : D::kCharsEN;
    std::u16string out;
    for (uint8_t value : raw) {
        char16_t c = value == 0xB1 ? quoteLeft(language)
                   : value == 0xB2 ? quoteRight(language)
                   : table[value];
        if (c == 0x00FF) break;
        if (c == 0x246D) c = u'♂';
        else if (c == 0x246E) c = u'♀';
        out.push_back(c);
    }
    return out;
}

uint16_t checksum(std::span<const uint8_t> d) noexcept {
    if (d.size() < 0x50) return 0;
    uint32_t sum = 0;
    for (size_t i = 0x20; i < 0x50; i += 2) sum += static_cast<uint32_t>(d[i] | (d[i + 1] << 8));
    return static_cast<uint16_t>(sum);
}

bool read(std::span<const uint8_t> d, Pk3& o) {
    if (d.size() != 80 && d.size() != 100) return false;
    o = Pk3{};
    auto u16 = [&](size_t off) { return static_cast<uint16_t>(d[off] | (d[off + 1] << 8)); };
    auto u32 = [&](size_t off) {
        return static_cast<uint32_t>(d[off]) | (static_cast<uint32_t>(d[off + 1]) << 8) |
               (static_cast<uint32_t>(d[off + 2]) << 16) | (static_cast<uint32_t>(d[off + 3]) << 24);
    };
    o.party = d.size() == 100;
    o.pid = u32(0x00);
    o.tid = u16(0x04);
    o.sid = u16(0x06);
    for (size_t i = 0; i < 10; ++i) o.nicknameRaw[i] = d[0x08 + i];
    o.language = d[0x12];
    o.flags = d[0x13];
    for (size_t i = 0; i < 7; ++i) o.otRaw[i] = d[0x14 + i];
    o.markings = d[0x1B];
    o.checksum = u16(0x1C);
    o.speciesInternal = u16(0x20);
    o.species = o.speciesInternal < D::kInternalSpeciesCount ? D::kInternalToNational[o.speciesInternal] : 0;
    o.heldItem = u16(0x22);
    o.exp = u32(0x24);
    o.ppUps = d[0x28];
    o.friendship = d[0x29];
    for (size_t i = 0; i < 4; ++i) o.moves[i] = u16(0x2C + i * 2);
    for (size_t i = 0; i < 4; ++i) o.pp[i] = d[0x34 + i];
    for (size_t i = 0; i < 6; ++i) o.evs[i] = d[0x38 + i];
    for (size_t i = 0; i < 6; ++i) o.contest[i] = d[0x3E + i];
    o.pokerus = d[0x44];
    o.metLocation = d[0x45];
    const uint16_t origins = u16(0x46);
    o.metLevel = static_cast<uint8_t>(origins & 0x7F);
    o.version = static_cast<uint8_t>((origins >> 7) & 0xF);
    o.ball = static_cast<uint8_t>((origins >> 11) & 0xF);
    o.otGender = static_cast<uint8_t>(origins >> 15);
    o.iv32 = u32(0x48);
    for (size_t i = 0; i < 6; ++i) o.ivs[i] = static_cast<uint8_t>((o.iv32 >> (i * 5)) & 0x1F);
    o.isEgg = ((o.iv32 >> 30) & 1u) != 0;
    o.abilityBit = static_cast<uint8_t>(o.iv32 >> 31);
    o.ribbons = u32(0x4C);
    o.fateful = (o.ribbons >> 31) != 0;
    o.partyLevel = o.party ? d[0x54] : 0;

    const auto& pi = personal(o.species);
    o.form = o.species == 201 ? unownForm(o.pid) : 0;
    o.gender = genderFromRatio(o.pid, pi.gender);
    o.nature = static_cast<uint8_t>(o.pid % 25);
    o.currentLevel = levelFromExp(o.exp, pi.growth);
    o.shiny = isShinyPid(o.pid, o.tid, o.sid);
    o.nickname = decodeString(o.nicknameRaw, o.language);
    o.ot = decodeString(o.otRaw, o.language);
    o.isNicknamed = o.nickname != std::u16string_view(speciesName3(o.species, o.language));
    return true;
}

// ---------------------------------------------------------------------------
// Colosseum / XD RNG (PKHeX MethodCXD, MethodPokeSpot, LockFinder / TeamLockResult)
// ---------------------------------------------------------------------------

namespace {

constexpr uint32_t kNotForced = 0xFFFFFFFFu; // TeamLockResult.NOT_FORCED

/// PKHeX's GameCube searches have no bound; a guard that runs out makes the analysis
/// Unknown (every Invalid finding is reported as a Warning) instead of guessing.
struct CxdGuard {
    bool& capped;
    uint32_t budget;
    bool spend() noexcept {
        if (budget == 0) { capped = true; return false; }
        --budget;
        return true;
    }
};

/// MethodCXD.IsValidNameScreenEndSeed (MethodCXD.cs:644-677): on the name screen each
/// frame has a 10% chance (rand >> 16 <= 0x1999) to spawn a ball, which consumes four
/// more calls; a seed is reachable when the four frames before it all skipped the spawn,
/// or when an earlier reachable seed spawned a ball that lands on it.
bool isValidNameScreenEndSeed(uint32_t seed, CxdGuard& g, int depth = 0) noexcept {
    constexpr uint32_t kThreshold = 0x1999u;
    if (depth > 256 || !g.spend()) { g.capped = true; return false; }
    const bool p1 = (seed >> 16) > kThreshold;
    const bool p2 = xdPrev16(seed) > kThreshold;
    const bool p3 = xdPrev16(seed) > kThreshold;
    const bool p4 = xdPrev16(seed) > kThreshold;
    if (p1 && p2 && p3 && p4) return true;
    if (xdPrev16(seed) <= kThreshold && isValidNameScreenEndSeed(xdPrev(seed), g, depth + 1)) return true;
    if (xdPrev16(seed) <= kThreshold && p1 && isValidNameScreenEndSeed(xdPrev(seed), g, depth + 1)) return true;
    if (xdPrev16(seed) <= kThreshold && p1 && p2 && isValidNameScreenEndSeed(xdPrev(seed), g, depth + 1)) return true;
    if (xdPrev16(seed) <= kThreshold && p1 && p2 && p3 && isValidNameScreenEndSeed(xdPrev(seed), g, depth + 1)) return true;
    return false;
}

/// MethodCXD.TryGetSeedTrainerID (MethodCXD.cs:680-692): the TID then SID are the two
/// 16-bit outputs 1000 frames after a reachable name-screen seed.
bool cxdTrainerIdHasSeed(uint16_t tid, uint16_t sid, bool& capped) noexcept {
    CxdGuard g{capped, 100000};
    for (uint32_t s : xdSeeds(static_cast<uint32_t>(tid) << 16, static_cast<uint32_t>(sid) << 16))
        if (isValidNameScreenEndSeed(xdPrev1000(s), g)) return true;
    return false;
}

bool isMaleEevee(uint32_t pid) noexcept { return (pid & 0xFFu) >= 31; } // EntityGender.VM

/// MethodCXD.GetPIDRegular: two 16-bit outputs, high half first.
uint32_t xdPidRegular(uint32_t& seed) noexcept {
    const uint32_t a = xdNext16(seed);
    const uint32_t b = xdNext16(seed);
    return (a << 16) | b;
}

/// MethodCXD.GetColoStarterPID (MethodCXD.cs:510-523): rerolled until male and not shiny.
bool coloStarterPid(uint32_t& seed, uint16_t tid, uint16_t sid, uint32_t& pid, CxdGuard& g) noexcept {
    while (g.spend()) {
        pid = xdPidRegular(seed);
        if (isMaleEevee(pid) && !isShinyPid(pid, tid, sid)) return true;
    }
    return false;
}

/// MethodCXD.IsMatchIVs: the two 15-bit IV halves follow the seed.
bool xdMatchIVs(uint32_t iv1, uint32_t iv2, uint32_t seed) noexcept {
    if (iv1 != xdNext15(seed)) return false;
    return iv2 == xdNext15(seed);
}

/// MethodCXD.TryGetSeedStarterColo (MethodCXD.cs:571-612). Call structure after the seed
/// that produced TID/SID: fake PID x2, [IVs x2, ability, PID] for Umbreon, then the same
/// again for Espeon; both PIDs are rerolled until male and not shiny.
bool coloStarterSeed(const Pk3& pk, uint32_t iv1, uint32_t iv2, uint32_t& result, bool& capped) noexcept {
    CxdGuard g{capped, 100000};
    for (uint32_t seed : xdSeeds(static_cast<uint32_t>(pk.tid) << 16, static_cast<uint32_t>(pk.sid) << 16)) {
        if (pk.species == 197 && !xdMatchIVs(iv1, iv2, xdNextN(seed, 4))) continue;
        uint32_t s = xdNextN(seed, 7);
        uint32_t pid = 0;
        if (!coloStarterPid(s, pk.tid, pk.sid, pid, g)) return false;
        if (pk.species == 197) {
            if (pid != pk.pid) continue;
            result = seed;
            return true;
        }
        const uint32_t afterFake = xdNextN(s, 2);
        if (!xdMatchIVs(iv1, iv2, afterFake)) continue;
        s = xdNextN(afterFake, 3);
        if (!coloStarterPid(s, pk.tid, pk.sid, pid, g)) return false;
        if (pid != pk.pid) continue;
        result = seed;
        return true;
    }
    return false;
}

/// MethodCXD.TryGetSeedStarterXD (MethodCXD.cs:614-636): TID, SID, fake PID x2, IVs x2,
/// ability, PID; Eevee's PID is not rerolled.
bool xdStarterSeed(const Pk3& pk, uint32_t iv1, uint32_t iv2, uint32_t& result) noexcept {
    for (uint32_t seed : xdSeeds(static_cast<uint32_t>(pk.tid) << 16, static_cast<uint32_t>(pk.sid) << 16)) {
        if (!xdMatchIVs(iv1, iv2, xdNextN(seed, 4))) continue;
        uint32_t s = xdNextN(seed, 7);
        if (xdPidRegular(s) != pk.pid) continue;
        result = seed;
        return true;
    }
    return false;
}

/// MethodPokeSpot.IsValidActivation (MethodPokeSpot.cs:8-50): 0 invalid, 1 neither Munchlax
/// nor Bonsly could spawn, 2 only Munchlax could. seed is the frame before the PID.
int pokeSpotActivation(uint8_t slot, uint32_t seed, uint32_t& origin) noexcept {
    const uint32_t esv = (seed >> 16) % 100u;
    const uint8_t rolled = esv < 50 ? 0 : esv < 85 ? 1 : 2; // MethodPokeSpot.GetSlot
    if (rolled != slot) return 0;
    uint32_t preSlot = xdPrev16(seed);
    if (preSlot % 3 == 0) { origin = xdPrev(seed); return 1; }
    if (preSlot % 100 < 10) return 0;
    preSlot = xdPrev16(seed);
    if (preSlot % 3 == 0) { origin = xdPrev(seed); return 2; }
    return 0;
}

/// MethodPokeSpot.TryGetOriginSeedPID (MethodPokeSpot.cs:63-84).
bool pokeSpotOriginPid(uint32_t pid, uint8_t slot, uint32_t& origin) noexcept {
    for (uint32_t seed : xdSeeds(pid & 0xFFFF0000u, pid << 16))
        if (pokeSpotActivation(slot, seed, origin) != 0) return true;
    return false;
}

/// MethodPokeSpot.IsValidAnimation (MethodPokeSpot.cs:373-410): the spot animation is
/// rand(10) != 3, then 1, 2 or 5 more calls depending on its value.
bool pokeSpotAnimation(uint32_t seed) noexcept {
    uint32_t anim = xdPrev16(seed) % 10;
    if (anim < 5 && anim != 3) return true;
    anim = xdPrev16(seed) % 10;
    if (anim >= 5 && anim != 8) return true;
    seed = xdPrevN(seed, 3);
    return (seed >> 16) % 10 == 8;
}

/// MethodPokeSpot.TryGetOriginSeedIVs (MethodPokeSpot.cs:140-175) for a PK3 (original met
/// level): fake ID x2, level, fake PID x2, then IVs x2 and ability.
bool pokeSpotOriginIVs(const Pk3& pk, uint8_t levelMin, uint8_t levelMax) noexcept {
    const uint32_t iv32 = pk.iv32 & 0x3FFFFFFFu;
    const uint32_t iv1 = iv32 & 0x7FFFu;
    const uint32_t iv2 = iv32 >> 15;
    const uint32_t delta = 1u + levelMax - levelMin;
    for (uint32_t preIV : xdSeedsIVs(iv1 << 16, iv2 << 16)) {
        if (!pokeSpotAnimation(xdPrevN(preIV, 6))) continue;
        const uint32_t lvl16 = xdPrevN(preIV, 2) >> 16;
        if (pk.metLevel != levelMin + lvl16 % delta) continue;
        return true;
    }
    return false;
}

/// TeamLockResult (TeamLockResult.cs): reverse-generates the trainer's party from the
/// shadow's origin seed. The locks are popped from the end of the team array; a
/// non-shadow prior lock allows any gap (GetAllLocks), otherwise the member must sit
/// right before (GetSingleLock, with anti-shiny reroll frames); VerifyNPC then checks the
/// CPU trainer's shiny value against every regenerated member.
struct TeamLockSearch {
    const X::TeamLock& team;
    uint32_t tsv;
    CxdGuard& guard;
    std::vector<uint32_t> seeds;    // FrameCache: seeds[i] = Prev^(i+2)(origin), value = seed >> 16
    std::vector<uint32_t> teamPids; // Team stack, first pushed first
    uint32_t rcsv = kNotForced;

    TeamLockSearch(const X::TeamLock& t, uint32_t originSeed, uint32_t trainerSv, CxdGuard& g)
        : team(t), tsv(trainerSv), guard(g) {
        seeds.push_back(xdPrevN(originSeed, 2));
    }

    uint32_t cache(int index) {
        while (static_cast<size_t>(index) >= seeds.size()) seeds.push_back(xdPrev(seeds.back()));
        return seeds[static_cast<size_t>(index)] >> 16;
    }
    const X::NpcLock& lock(int i) const noexcept { return X::kNpcLocks[team.lockStart + static_cast<size_t>(i)]; }

    /// NPCLock.MatchesLock.
    static bool matches(const X::NpcLock& l, uint32_t pid) noexcept {
        if (l.state != 0 && l.nature == 0) return true; // non-locked shadow
        if (l.gender != 2 && l.gender != ((pid & 0xFFu) < l.ratio ? 1 : 0)) return false;
        return l.nature == pid % 25;
    }
    static int frames(const X::NpcLock& l) noexcept { return l.state > 1 ? 5 : 7; } // NPCLock.FramesConsumed

    bool run() { return find(0, nullptr, static_cast<int>(team.lockCount) - 1); }

    /// FindLockSeed: lockIndex is the lock popped next (Stack order reverses the array).
    bool find(int frame, const X::NpcLock* prior, int lockIndex) {
        if (lockIndex < 0) return verifyNpc(frame);
        const X::NpcLock& current = lock(lockIndex);
        if (prior != nullptr && prior->state == 0) return allLocks(frame, current, *prior, lockIndex);
        return singleLock(frame, current, lockIndex);
    }

    bool tryFrame(uint32_t pid, int frame, const X::NpcLock& current, int lockIndex) {
        teamPids.push_back(pid);
        if (find(frame, &current, lockIndex - 1)) return true;
        teamPids.pop_back();
        return false;
    }

    /// GetSingleLock (TeamLockResult.cs:86-129).
    bool singleLock(int ctr, const X::NpcLock& current, int lockIndex) {
        const uint32_t pid = (cache(ctr + 1) << 16) | cache(ctr);
        if (!matches(current, pid)) return false;
        if (tryFrame(pid, ctr + frames(current), current, lockIndex)) return true;
        bool forcedOT = false;
        int start = 2;
        while (guard.spend()) {
            const uint32_t upper = cache(start + 1);
            const uint32_t lower = cache(start);
            const uint32_t sv = (upper ^ lower) >> 3;
            if (sv == tsv) {
                // anti-shiny reroll for the player: a possible frame
            } else if (rcsv != kNotForced) {
                if (sv != rcsv) {
                    if (forcedOT) rcsv = kNotForced;
                    return false;
                }
                // A CPU-shiny reroll frame. PKHeX re-tests this same frame forever here
                // (TeamLockResult.cs:118-123, `continue` without advancing); the frame is
                // treated like the GetAllLocks case and the search moves on.
                forcedOT = true;
            }
            if (tryFrame(pid, start + frames(current), current, lockIndex)) return true;
            start += 2;
        }
        return false;
    }

    /// GetAllLocks (TeamLockResult.cs:131-178).
    bool allLocks(int ctr, const X::NpcLock& current, const X::NpcLock& prior, int lockIndex) {
        const int start = ctr;
        bool forcedOT = false;
        while (guard.spend()) {
            const int p7 = ctr - 7;
            if (p7 > start) {
                const uint32_t upper = cache(p7 + 1);
                const uint32_t lower = cache(p7);
                const uint32_t cid = (upper << 16) | lower;
                const uint32_t sv = (upper ^ lower) >> 3;
                if (sv == tsv) {
                    // interrupt ignored: the result is shiny for the player
                } else if (matches(prior, cid)) {
                    if (rcsv != kNotForced) {
                        if (sv != rcsv) {
                            if (forcedOT) rcsv = kNotForced;
                            return false;
                        }
                    } else {
                        rcsv = sv;
                        forcedOT = true;
                    }
                }
            }
            const uint32_t pid = (cache(ctr + 1) << 16) | cache(ctr);
            if (matches(current, pid) && tryFrame(pid, ctr + frames(current), current, lockIndex)) return true;
            ctr += 2;
        }
        return false;
    }

    /// VerifyNPC (TeamLockResult.cs:180-212). PKHeX enumerates its Team stack top first with
    /// pos counting down from Count - 1, which pairs the k-th pushed frame with Locks[k].
    bool verifyNpc(int ctr) {
        const uint32_t tid = cache(ctr + 1);
        const uint32_t sid = cache(ctr);
        const uint32_t cpusv = (tid ^ sid) >> 3;
        if (rcsv != kNotForced && rcsv != cpusv) return false;
        for (size_t k = 0; k < teamPids.size(); ++k) {
            const uint32_t pid = teamPids[k];
            const uint32_t psv = ((pid & 0xFFFFu) ^ (pid >> 16)) >> 3;
            if (psv != cpusv) continue;
            if (lock(static_cast<int>(k)).state == 0) return false; // no shiny CPU Pokemon
            if (tsv != kNotForced) return false;                      // XD: no shiny shadow Pokemon
        }
        return true;
    }
};

/// LockFinder.IsAllShadowLockValid (LockFinder.cs:7-29): XD never yields a shiny shadow;
/// with no prior team any seed is fine; otherwise one team permutation must regenerate.
bool shadowLockValid(const Pk3& pk, bool xd, uint8_t teamSet, uint32_t seed, bool& capped) {
    if (xd && pk.shiny) return false;
    const X::TeamSet& set = X::kTeamSets[teamSet];
    if (set.count == 0) return true;
    const uint32_t tsv = xd ? (static_cast<uint32_t>(pk.tid ^ pk.sid) >> 3) : kNotForced;
    CxdGuard g{capped, 400000};
    for (size_t i = 0; i < set.count; ++i) {
        TeamLockSearch search(X::kTeamLocks[X::kTeamSetMembers[set.start + i]], seed, tsv, g);
        if (search.run()) return true;
        if (capped) return false;
    }
    return false;
}

} // namespace

// ---------------------------------------------------------------------------
// PID/IV correlation (PKHeX MethodFinder, Gen III subset)
// ---------------------------------------------------------------------------

namespace {

enum PidType : uint8_t {
    None = 0, M1 = 1, M2 = 2, M3 = 3, M4 = 4, M1U = 5, M2U = 6, M3U = 7, M4U = 8, M1Roamer = 9,
    BACD = 10, BACD_A = 11, BACD_AX = 12, BACD_S = 13, BACD_EA = 14, BACD_EAX = 15, BACD_ES = 16,
    BACD_U = 17, BACD_U_AX = 18, BACD_R = 19, BACD_R_A = 20, BACD_RBCD = 21, BACD_TA = 22,
    BACD_TS = 23, BACD_M = 24, CuteCharmT = 25, ChainShiny = 26, CXD = 27, CXDAnti = 28,
    CXD_ColoStarter = 29, ChannelT = 30, PokeSpot = 31, G4MGAntiShiny = 32, G5MGShiny = 33,
    Pokewalker = 34,
};

const char* pidTypeName(uint8_t t) noexcept {
    static constexpr const char* kNames[] = {
        "None", "Method 1", "Method 2", "Method 3", "Method 4", "Method 1 (Unown)", "Method 2 (Unown)",
        "Method 3 (Unown)", "Method 4 (Unown)", "Method 1 (roamer)", "BACD", "BACD_A", "BACD_AX",
        "BACD_S", "BACD_EA", "BACD_EAX", "BACD_ES", "BACD_U", "BACD_U_AX", "BACD_R", "BACD_R_A",
        "BACD_RBCD", "BACD_TA", "BACD_TS", "BACD_M", "Cute Charm", "Chain Shiny", "CXD", "CXD anti-shiny",
        "CXD Colosseum starter", "Channel", "PokeSpot", "G4 MG anti-shiny", "G5 MG shiny", "Pokewalker",
    };
    return t < sizeof(kNames) / sizeof(kNames[0]) ? kNames[t] : "?";
}

bool isClassicMethod(uint8_t t) noexcept { return t >= M1 && t <= M4U; }

PidIv makePidIv(uint8_t type, uint32_t seed) noexcept { PidIv p; p.type = type; p.originSeed = seed; return p; }

bool lcrngMatch(uint32_t top, uint32_t bot, uint32_t iv1, uint32_t iv2, PidIv& out) noexcept {
    for (uint32_t seed : lcSeeds(bot, top)) {
        uint32_t s = lcNextN(seed, 2);
        if (iv1 == lcNext15(s)) {
            if (iv2 == lcNext15(s)) { out = makePidIv(M1, seed); return true; }
            if (iv2 == lcNext15(s)) { out = makePidIv(M4, seed); return true; }
        } else {
            if (iv1 != lcNext15(s)) continue;
            if (iv2 == lcNext15(s)) { out = makePidIv(M2, seed); return true; }
        }
    }
    for (uint32_t seed : lcSeedsSkip(bot, top)) {
        uint32_t s = lcNextN(seed, 3);
        if (iv1 != lcNext15(s)) continue;
        if (iv2 != lcNext15(s)) continue;
        out = makePidIv(M3, seed);
        return true;
    }
    return false;
}

bool lcrngUnownMatch(uint32_t top, uint32_t bot, uint32_t iv1, uint32_t iv2, PidIv& out) noexcept {
    for (uint32_t seed : lcSeeds(top, bot)) {
        uint32_t s = lcNextN(seed, 2);
        if (iv1 == lcNext15(s)) {
            if (iv2 == lcNext15(s)) { out = makePidIv(M1U, seed); return true; }
            if (iv2 == lcNext15(s)) { out = makePidIv(M4U, seed); return true; }
        } else {
            if (iv1 != lcNext15(s)) continue;
            if (iv2 == lcNext15(s)) { out = makePidIv(M2U, seed); return true; }
        }
    }
    for (uint32_t seed : lcSeedsSkip(top, bot)) {
        uint32_t s = lcNextN(seed, 3);
        if (iv1 != lcNext15(s)) continue;
        if (iv2 != lcNext15(s)) continue;
        out = makePidIv(M3U, seed);
        return true;
    }
    return false;
}

bool xdrngMatch(const Pk3& pk, uint32_t top, uint32_t bot, uint32_t iv1, uint32_t iv2, PidIv& out) noexcept {
    for (uint32_t seed : xdSeeds(top, bot)) {
        uint32_t b = xdPrev(seed);
        uint32_t a = xdPrev(b);
        uint32_t hi = (a >> 16) & 0x7FFFu;
        uint32_t lo = (b >> 16) & 0x7FFFu;
        if (hi == iv1 && lo == iv2) { out = makePidIv(CXD, xdPrev(a)); return true; }

        const uint32_t tsv = static_cast<uint32_t>(pk.tid ^ pk.sid) >> 3;
        uint32_t psv = (top ^ bot) >> 19;
        if (psv == tsv) continue;
        uint32_t p2 = seed;
        uint32_t p1 = b;
        psv = (p2 ^ p1) >> 19;
        if (psv != tsv) continue;
        do {
            b = xdPrev(a);
            a = xdPrev(b);
            hi = (a >> 16) & 0x7FFFu;
            lo = (b >> 16) & 0x7FFFu;
            if (hi == iv1 && lo == iv2) { out = makePidIv(CXDAnti, xdPrev(a)); return true; }
            p2 = xdPrev(p1);
            p1 = xdPrev(p2);
            psv = (p2 ^ p1) >> 19;
        } while (psv == tsv);
    }
    return false;
}

bool roamerMatch(uint32_t top, uint32_t bot, uint32_t iv32, PidIv& out) noexcept {
    if (iv32 > 0xFF) return false;
    for (uint32_t seed : lcSeeds(bot, top)) {
        const uint32_t ivC = (lcNextN(seed, 3) >> 16) & 0xFFu;
        if (iv32 != ivC) continue;
        out = makePidIv(M1Roamer, seed);
        return true;
    }
    return false;
}

bool channelMatch(const Pk3& pk, uint32_t top, uint32_t bot, uint32_t iv32, PidIv& out) noexcept {
    if (pk.version != kVerR && pk.version != kVerS) return false;
    const uint32_t undo = (top >> 16) ^ 0x8000u;
    if ((undo > 7 ? 0u : 1u) != ((bot >> 16) ^ pk.sid ^ 40122u)) top = undo << 16;
    for (uint32_t seed : xdSeeds(top, bot)) {
        const uint32_t c = xdNextN(seed, 3);
        const uint32_t dd = xdNext(c);
        if ((dd >> 31) + 1 != pk.version) continue;
        const uint32_t e = xdNext(dd);
        if ((e >> 31) != pk.otGender) continue;
        if (xdSequentialIV32(e) != iv32) continue;
        if ((seed >> 16) != pk.sid) continue;
        out = makePidIv(ChannelT, xdPrev(seed));
        return true;
    }
    return false;
}

bool mg4Match(uint32_t pid, uint32_t iv1, uint32_t iv2, PidIv& out) noexcept {
    auto psvOf = [](uint32_t x) { return ((x >> 16) ^ (x & 0xFFFFu)) >> 3; };
    const uint32_t current = psvOf(pid);
    pid = arPrev(pid);
    const uint32_t original = psvOf(pid);
    if (original == current) return false;
    for (int i = 0; i < 3; ++i) {
        for (uint32_t seed : lcSeeds(pid << 16, pid & 0xFFFF0000u)) {
            uint32_t s = lcNextN(seed, 2);
            if (iv1 != lcNext15(s)) continue;
            if (iv2 != lcNext15(s)) continue;
            out = makePidIv(G4MGAntiShiny, seed);
            return true;
        }
        pid = arPrev(pid);
        if (psvOf(pid) != original) break;
    }
    return false;
}

// CommonEvent3 helpers.
uint32_t ceAntishiny(uint32_t a16, uint32_t b16, uint32_t idXor) noexcept { return ((a16 ^ (idXor ^ b16)) << 16) | b16; }
uint32_t ceForceShiny(uint32_t x16, uint32_t b16, uint32_t idXor) noexcept {
    return (x16 << 16) | (((idXor ^ x16) & 0xFFF8u) | (b16 & 7u));
}
bool ceIsForceAntishiny(uint32_t pid, uint32_t a16, uint32_t b16, uint32_t idXor) noexcept {
    if ((a16 & ~7u) == 0) return false;
    return ceAntishiny(a16, b16, idXor) == pid;
}
bool ceIsForceShiny(uint32_t pid, uint32_t x16, uint32_t b16, uint32_t idXor) noexcept {
    if (x16 != pid >> 16) return false;
    return ceForceShiny(x16, b16, idXor) == pid;
}
bool ceIsRegularAntishiny(uint32_t actual, uint32_t expect) noexcept { return ((expect + 8) & 0xFFFFFFF8u) == actual; }
bool ceIsRegularAntishinyDifferentOT(uint32_t actual, uint32_t expect) noexcept {
    if ((actual & 7u) != 0) return false;
    const uint32_t delta = actual - expect;
    return delta != 0 && delta < 8;
}
bool ceIsForceAntishinyDifferentOT(uint32_t pid, uint32_t a16, uint32_t b16, uint32_t idXor) noexcept {
    if ((a16 & ~7u) == 0) return false;
    if ((pid & 0xFFFFu) != b16) return false;
    return (ceAntishiny(a16, b16, idXor) >> 19) != (pid >> 19);
}
bool ceIsForceShinyDifferentOT(uint32_t pid, uint32_t x16, uint32_t b16, uint32_t idXor) noexcept {
    if (x16 != pid >> 16) return false;
    if ((pid & 7u) != (b16 & 7u)) return false;
    return ((idXor ^ x16) & 0xFFF8u) != (pid & 0xFFF8u);
}
uint32_t genderBit0(uint32_t r) noexcept { return (r / 3) & 1u; }
uint32_t genderBit3(uint32_t r) noexcept { return (r >> 3) & 1u; }
uint32_t genderBit7(uint32_t r) noexcept { return ((r >> 7) & 1u) ^ 1u; }
uint32_t genderBit15(uint32_t r) noexcept { return (r >> 15) & 1u; }

bool bacdMatch(const Pk3& pk, uint32_t actual, uint32_t iv1, uint32_t iv2, PidIv& out) noexcept {
    const uint32_t idXor = static_cast<uint32_t>(pk.tid ^ pk.sid);
    for (uint32_t x : lcSeedsIVs(iv1 << 16, iv2 << 16)) {
        uint8_t type = BACD;
        uint32_t seed = x;
        const uint32_t b16 = seed >> 16;
        const uint32_t a16 = lcPrev16(seed);
        const uint32_t expect = (a16 << 16) | b16;
        if (expect != actual) {
            const bool shiny = ((idXor ^ (actual >> 16) ^ (actual & 0xFFFFu)) & 0xFFFFu) < 8;
            if (!shiny) {
                if (ceIsRegularAntishiny(actual, expect)) type = BACD_A;
                else if (ceIsForceAntishiny(actual, a16, b16, idXor)) type = BACD_AX;
                else if (ceIsForceShinyDifferentOT(actual, lcPrev16(seed), b16, idXor)) type = BACD_ES;
                else continue;
            } else {
                if (ceIsRegularAntishinyDifferentOT(actual, expect)) type = BACD_EA;
                else if (ceIsForceAntishinyDifferentOT(actual, a16, b16, idXor)) type = BACD_EAX;
                else if (ceIsForceShiny(actual, lcPrev16(seed), b16, idXor)) type = BACD_S;
                else continue;
            }
        }
        out = makePidIv(type, lcPrev(seed));
        return true;
    }
    return false;
}

bool chainShinyValid(const Pk3& pk, uint32_t pid, uint32_t seed, uint32_t& s) noexcept {
    s = seed;
    int i = 15;
    do {
        const uint32_t bit = (s >> 16) & 1u;
        if (bit != ((pid >> i) & 1u)) break;
        s = lcPrev(s);
    } while (--i != 2);
    if (i != 2) return false;
    const uint32_t upper = s;
    if (((upper >> 16) & 7u) != ((pid >> 16) & 7u)) return false;
    const uint32_t lower = lcPrev(upper);
    if (((lower >> 16) & 7u) != (pid & 7u)) return false;
    const uint32_t upid = (((pid & 0xFFFFu) ^ pk.tid ^ pk.sid) & 0xFFF8u) | ((upper >> 16) & 7u);
    if (upid != pid >> 16) return false;
    s = lcPrevN(lower, 2);
    return true;
}

bool chainShinyMatch(const Pk3& pk, uint32_t pid, uint32_t iv1, uint32_t iv2, PidIv& out) noexcept {
    for (uint32_t seed : lcSeedsIVs(iv1 << 16, iv2 << 16)) {
        uint32_t s = 0;
        if (!chainShinyValid(pk, pid, seed, s)) continue;
        out = makePidIv(ChainShiny, s);
        return true;
    }
    return false;
}

bool g5mgShinyMatch(const Pk3& pk, uint32_t pid, PidIv& out) noexcept {
    const uint32_t low = pid & 0xFFFFu;
    if (low > 0xFF) return false;
    const uint32_t abilityBit = (pid >> 16) & 1u;
    uint32_t expect = ((low ^ pk.tid ^ pk.sid) << 16) | low;
    if ((expect & 0x10000u) != (abilityBit << 16)) expect ^= 0x10000u;
    if (pid != expect) return false;
    out = makePidIv(G5MGShiny, 0);
    return true;
}

/// CuteCharm4.IsCuteCharm with the HG/SS (= R/S) gender ratios.
bool cuteCharmMatch(const Pk3& pk, uint32_t pid, PidIv& out) noexcept {
    uint16_t species = pk.species;
    uint8_t gender = pk.gender;
    if (species == 292) { // Shedinja: from Nincada
        species = 290;
        gender = genderFromRatio(pid, personal(290).gender);
    } else if ((species == 183 || species == 184) && pid >= 0xC8 && pid <= 0xE0) {
        species = 298; // Azurill
        gender = 0;
    }
    const uint8_t gr = personal(species).gender;
    bool ok = false;
    if (gender == 0) ok = gr < 254 && pid == 25u * ((gr / 25u) + 1u) + (pid % 25u);
    else if (gender == 1) ok = pid < 25 && gr < 254;
    if (!ok) return false;
    out = makePidIv(CuteCharmT, 0);
    return true;
}

uint32_t pokewalkerPid(uint16_t tid, uint16_t sid, uint32_t nature, uint8_t gender, uint8_t gr) noexcept {
    uint32_t pid = ((((static_cast<uint32_t>(tid) ^ sid) >> 8) ^ 0xFFu) << 24);
    pid += nature - (pid % 25);
    if (gr == 0 || gr >= 0xFE) return pid;
    const uint8_t pidGender = (pid & 0xFFu) < gr ? 1 : 0;
    if (gender == pidGender) return pid;
    if (gender == 0) {
        pid += ((((gr - (pid & 0xFFu)) / 25u) + 1u) * 25u);
        if ((nature & 1u) != (pid & 1u)) pid += 25;
    } else {
        pid -= (((((pid & 0xFFu) - gr) / 25u) + 1u) * 25u);
        if ((nature & 1u) != (pid & 1u)) pid -= 25;
    }
    return pid;
}

bool pokewalkerMatch(const Pk3& pk, uint32_t pid, PidIv& out) noexcept {
    const uint32_t mid = pid & 0x00FFFF00u;
    if (mid != 0 && mid != 0x00FFFF00u) return false;
    const uint32_t nature = pid % 25;
    if (nature == 24) return false;
    const uint8_t gender = pk.gender;
    const uint8_t gr = personal(pk.species).gender;
    if (pokewalkerPid(pk.tid, pk.sid, nature, gender, gr) != pid) {
        bool azurill = false;
        if (gender == 0 && (pk.species == 183 || pk.species == 184) && genderFromRatio(pid, 191) == 1)
            azurill = pokewalkerPid(pk.tid, pk.sid, nature, 1, 191) == pid;
        if (!azurill) return false;
    }
    out = makePidIv(Pokewalker, 0);
    return true;
}

} // namespace

namespace {

/// MethodFinder.GetColoStarterMatch (MethodFinder.cs:555-572): a CXD Espeon met at 25+
/// or Umbreon met at 26+ is first tried as a Colosseum starter, whose PID and IVs
/// follow from the trainer's TID/SID (MethodCXD.TryGetSeedStarterColo).
bool coloStarterMatch(const Pk3& pk, uint32_t iv1, uint32_t iv2, PidIv& out, bool& capped) noexcept {
    if (pk.version != kVerCXD) return false;
    const bool starter = (pk.species == 196 && pk.metLevel >= 25) || (pk.species == 197 && pk.metLevel >= 26);
    if (!starter) return false;
    uint32_t result = 0;
    if (!coloStarterSeed(pk, iv1, iv2, result, capped)) return false;
    out = makePidIv(CXD_ColoStarter, result);
    return true;
}

} // namespace

PidIv analyzePidIv(const Pk3& pk, bool& capped) {
    const uint32_t pid = pk.pid;
    const uint32_t top = pid & 0xFFFF0000u;
    const uint32_t bot = pid << 16;
    const uint32_t iv32 = pk.iv32 & 0x3FFFFFFFu;
    const uint32_t iv1 = iv32 & 0x7FFFu;
    const uint32_t iv2 = iv32 >> 15;
    PidIv r;
    if (lcrngMatch(top, bot, iv1, iv2, r)) return r;
    if (pk.species == 201 && lcrngUnownMatch(top, bot, iv1, iv2, r)) return r;
    if (coloStarterMatch(pk, iv1, iv2, r, capped)) return r;
    if (xdrngMatch(pk, top, bot, iv1, iv2, r)) return r;
    if (roamerMatch(top, bot, iv32, r)) return r;
    if (channelMatch(pk, top, bot, iv32, r)) return r;
    if (mg4Match(pid, iv1, iv2, r)) return r;
    if (bacdMatch(pk, pid, iv1, iv2, r)) return r;
    if (pk.shiny) {
        if (chainShinyMatch(pk, pid, iv1, iv2, r)) return r;
        if (g5mgShinyMatch(pk, pid, r)) return r;
        if (pid <= 0xFF && cuteCharmMatch(pk, pid, r)) return r;
    } else {
        if (pid <= 0xFF && cuteCharmMatch(pk, pid, r)) return r;
    }
    if (pokewalkerMatch(pk, pid, r)) return r;
    return PidIv{};
}

PidIv analyzePidIv(const Pk3& pk) {
    bool capped = false;
    return analyzePidIv(pk, capped);
}

namespace {

// ---------------------------------------------------------------------------
// Event gift seed revision (PKHeX CommonEvent3Checker / MystryMew /
// PCJPFifthAnniversary / ChannelJirachi)
// ---------------------------------------------------------------------------

uint32_t periodicWeight(uint32_t rand, uint32_t max) noexcept {
    const uint32_t high = rand >> 16;
    const uint32_t first = ((high << 2) & 0xFFFFu) + high;
    uint32_t second = ((rand & 0xFFFFu) << 1) + (first >> 16);
    second += high + (second >> 16);
    return (max * (second & 0xFFFFu)) >> 16;
}

struct PcjpResult { uint32_t index; bool wish; bool shiny; };

PcjpResult pcjpResult(uint16_t rand) noexcept {
    uint32_t seed = rand;
    const uint32_t r1 = lcNext16(seed);
    const uint32_t r2 = lcNext16(seed);
    const uint32_t result = periodicWeight((r1 << 16) | r2, 1000);
    const uint32_t eighth = result / 125;
    const uint32_t index = eighth >> 1;
    return {index, (eighth & 1u) == 1u, index == 0 && (result % 125) >= 100};
}

int mystryMewSearch(uint16_t value) noexcept {
    const uint16_t* begin = D::kMystryMewSeeds;
    const uint16_t* end = begin + D::kMystryMewSeedCount;
    const uint16_t* it = std::lower_bound(begin, end, value);
    const int pos = static_cast<int>(it - begin);
    if (it != end && *it == value) return pos;
    return ~pos; // .NET BinarySearch miss
}

bool channelAccept(uint32_t& seed, uint32_t pivot) noexcept {
    if (pivot == 0) {
        seed = xdPrev(seed);
        if (xdPrev16(seed) <= 0x547A) return false;
        if (xdPrev16(seed) <= 0x4000) return false;
    } else if (pivot == 1) {
        if (xdPrev16(seed) > 0x4000) return false;
    } else {
        if (xdPrev16(seed) > 0x547A) return false;
        if (xdPrev16(seed) <= 0x4000) return false;
    }
    seed = xdPrevN(seed, 5);
    return true;
}

bool channelMenu(uint32_t& seed, bool& capped) noexcept {
    const uint32_t end = seed >> 30;
    if (end == 0) return false;
    uint32_t seen = 1u << end;
    for (int guard = 0; guard < 1 << 16; ++guard) {
        seed = xdPrev(seed);
        const uint32_t pattern = seed >> 30;
        if (pattern == end) return false;
        seen |= 1u << pattern;
        if (seen >= 0xEu) return true;
    }
    capped = true;
    return false;
}

bool channelJirachiPossible(uint32_t seed, uint32_t& origin, bool& capped) noexcept {
    for (uint32_t i = 0; i < 3; ++i) {
        uint32_t o = seed;
        if (!channelAccept(o, i)) continue;
        if (!channelMenu(o, capped)) continue;
        origin = xdPrev(o);
        return true;
    }
    return false;
}

} // namespace

namespace {

// ---------------------------------------------------------------------------
// Evolution chains (PKHeX EvolutionGroup3 / EvolutionChain / EvolutionUtil)
// ---------------------------------------------------------------------------

constexpr uint8_t kEvoSentinel = 0xFF; // EvolutionType.Invalid (SentinelNotReached)

struct Evo {
    uint16_t species = 0;
    uint8_t form = 0;
    uint8_t levelUpRequired = 0;
    uint8_t levelMax = 0;
    uint8_t levelMin = 0;
    uint8_t method = 0;
};

bool isLevelUpRequiredType(uint8_t type) noexcept {
    return (type >= 1 && type <= 4) || (type >= 9 && type <= 16);
}

struct EvoLink { const D::Evolution* method = nullptr; uint16_t species = 0; };
struct EvoNode { EvoLink first, second; };

const std::array<EvoNode, D::kMaxSpecies + 1>& reverseLineage() {
    static const std::array<EvoNode, D::kMaxSpecies + 1> table = [] {
        std::array<EvoNode, D::kMaxSpecies + 1> t{};
        for (uint16_t s = 1; s <= D::kMaxSpecies; ++s) {
            for (uint16_t i = D::kEvolutionStart[s]; i < D::kEvolutionStart[s + 1]; ++i) {
                const auto& e = D::kEvolutions[i];
                if (e.species == 0) break;
                if (e.species > D::kMaxSpecies) continue;
                if (e.form != 255 && e.form != 0) continue; // only form-0 sources evolve in Gen III
                auto& node = t[e.species];
                if (node.first.species == 0) node.first = {&e, s};
                else if (node.second.species == 0) node.second = {&e, s};
            }
        }
        return t;
    }();
    return table;
}

/// EvolutionMethod.Check for the Gen III methods (default rule tweak, no skipping).
bool evoCheck(const D::Evolution& m, const Pk3& pk, uint8_t lvl, uint8_t levelMin) noexcept {
    if (!isLevelUpRequiredType(m.type)) {
        if (m.type == 17) return pk.gender == 0;
        if (m.type == 18) return pk.gender == 1;
        return true; // trade evolutions: a PK3 is never "untraded"
    }
    if (m.type == 16 && pk.contest[1] < m.argument) return false; // LevelUpBeauty
    if (lvl < m.level) return false;
    // LevelUp flag (1) is set for every level-up method in Gen III.
    if (m.level == 0 && lvl < 2) return false;
    if (lvl < levelMin + 1) return false;
    return true;
}

bool tryDevolve(const Evo& head, const Pk3& pk, uint8_t currentMaxLevel, uint8_t levelMin, Evo& result) {
    if (head.species == 0 || head.species > D::kMaxSpecies || head.form != 0) return false;
    const auto& node = reverseLineage()[head.species];
    for (const EvoLink* link : {&node.first, &node.second}) {
        if (link->species == 0) return false;
        if (!evoCheck(*link->method, pk, currentMaxLevel, levelMin)) continue;
        result = Evo{};
        result.species = link->species;
        result.form = 0;
        result.levelMax = currentMaxLevel;
        result.method = link->method->type;
        result.levelMin = link->method->level;
        result.levelUpRequired = isLevelUpRequiredType(link->method->type) ? 1 : 0;
        return true;
    }
    return false;
}

bool tryEvolve(const Evo& head, const Evo& next, const Pk3& pk, uint8_t currentMaxLevel, uint8_t levelMin, Evo& result) {
    if (head.species == 0 || head.species > D::kMaxSpecies) return false;
    for (uint16_t i = D::kEvolutionStart[head.species]; i < D::kEvolutionStart[head.species + 1]; ++i) {
        const auto& m = D::kEvolutions[i];
        if (m.species != next.species) continue;
        const uint8_t expectForm = m.form == 255 ? head.form : m.form;
        if (next.form != expectForm) continue;
        if (!evoCheck(m, pk, currentMaxLevel, levelMin)) continue;
        const uint8_t levelUp = isLevelUpRequiredType(m.type) ? 1 : 0;
        result = Evo{};
        result.species = next.species;
        result.form = next.form;
        result.levelMax = currentMaxLevel;
        result.method = m.type;
        result.levelMin = std::max<uint8_t>(static_cast<uint8_t>(levelMin + levelUp), m.level);
        result.levelUpRequired = levelUp;
        return true;
    }
    return false;
}

bool isBetterDevolution(const Evo& a, const Evo& ref) noexcept {
    if (ref.species == 0) return true;
    return a.levelMin + a.levelUpRequired < ref.levelMin + ref.levelUpRequired;
}

bool isBetterEvolution(const Evo& a, const Evo& ref) noexcept {
    if (ref.method == kEvoSentinel) return true;
    if (a.levelMin + a.levelUpRequired > ref.levelMin + ref.levelUpRequired) return false;
    return a.levelMax > ref.levelMax;
}

void cleanDevolve(std::span<Evo> chain, uint8_t levelMin) noexcept {
    uint8_t req = 0;
    uint8_t method = 0;
    for (size_t k = chain.size(); k-- > 0;) {
        Evo& evo = chain[k];
        const uint8_t nextMin = evo.levelMin;
        const uint8_t nextReq = evo.levelUpRequired;
        const uint8_t nextMethod = evo.method;
        evo.levelMin = std::min(evo.levelMax, levelMin);
        evo.levelUpRequired = req;
        evo.method = method;
        levelMin = std::max(nextMin, levelMin);
        req = nextReq;
        method = nextMethod;
    }
}

bool isPresentInGame(uint16_t species, uint8_t form) noexcept {
    if (species > D::kMaxSpecies) return false;
    if (form == 0) return true;
    if (species == 201) return form < 28;
    if (species == 351 || species == 386) return form < 4;
    return false;
}

/// EvolutionGroup3.Devolve over a 3-entry chain whose head is already set.
void devolve3(std::array<Evo, 3>& result, const Pk3& pk, uint8_t encLevelMin) {
    for (size_t i = 1; i < result.size(); ++i) {
        const Evo prev = result[i - 1];
        Evo evo;
        if (!tryDevolve(prev, pk, prev.levelMax, encLevelMin, evo)) continue;
        if (isBetterDevolution(evo, result[i])) result[i] = evo;
    }
    // EvolutionUtil.Discard with PersonalTable.E: drop species not present in game.
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i].species == 0) break;
        if (isPresentInGame(result[i].species, result[i].form)) continue;
        for (size_t j = i + 1; j < result.size(); ++j) result[j - 1] = result[j];
        result.back() = Evo{};
        --i;
    }
}

/// EncounterOrigin.GetOriginChain(pk, 3): the chain the generator searches.
std::vector<Evo> originChain(const Pk3& pk) {
    std::vector<Evo> out;
    const uint8_t maxLevel = pk.currentLevel;
    const uint8_t minLevel = pk.isEgg ? 5 : pk.metLevel;
    if (pk.isEgg) {
        Evo e; e.species = pk.species; e.form = pk.form; e.levelMax = maxLevel; e.levelMin = maxLevel;
        out.push_back(e);
        return out;
    }
    std::array<Evo, 3> result{};
    result[0].species = pk.species;
    result[0].form = pk.form;
    result[0].levelMax = maxLevel;
    devolve3(result, pk, minLevel);
    size_t count = 0;
    while (count < result.size() && result[count].species != 0) ++count;
    out.assign(result.begin(), result.begin() + static_cast<std::ptrdiff_t>(count));
    cleanDevolve(out, minLevel);
    return out;
}

/// EvolutionChain.GetEvolutionChainsAllGens(pk, enc).Gen3 for a PK3 still in Gen III.
std::vector<Evo> evolutionHistory(const Pk3& pk, uint16_t encSpecies, uint8_t encLevelMin, bool encInvalid) {
    const uint8_t minLevel = std::max(pk.metLevel, encLevelMin);
    const uint8_t maxLevel = pk.currentLevel;
    std::vector<Evo> chain;
    if (pk.isEgg) {
        Evo e; e.species = pk.species; e.form = pk.form; e.levelMax = maxLevel; e.levelMin = maxLevel;
        chain.push_back(e);
    } else {
        std::array<Evo, 3> result{};
        result[0].species = pk.species;
        result[0].form = pk.form;
        result[0].levelMax = maxLevel;
        devolve3(result, pk, minLevel);
        const uint16_t search = encInvalid ? pk.species : encSpecies;
        int index = -1;
        for (size_t i = 0; i < result.size(); ++i) {
            if (result[i].species == search) { index = static_cast<int>(i); break; }
        }
        if (index < 0) return {};
        chain.assign(result.begin(), result.begin() + index + 1);
        cleanDevolve(chain, minLevel);
    }
    // ConditionBaseChainForward: stages above the encounter species are not yet reached.
    const uint16_t baseSpecies = encInvalid ? pk.species : encSpecies;
    for (auto& evo : chain) {
        if (evo.species == baseSpecies) break;
        evo.method = kEvoSentinel;
    }
    // EvolutionGroup3.Evolve.
    for (size_t i = chain.size(); i-- > 1;) {
        Evo& dest = chain[i - 1];
        const Evo devolved = chain[i];
        Evo evo;
        if (!tryEvolve(devolved, dest, pk, maxLevel, devolved.levelMin, evo)) {
            if (dest.method == kEvoSentinel) break;
            continue;
        }
        if (isBetterEvolution(evo, dest)) dest = evo;
    }
    // EvolutionUtil.SetHistory with PersonalTable.E.
    size_t start = 0, count = 0;
    for (const auto& evo : chain) {
        if (evo.method == kEvoSentinel) { start += count + 1; count = 0; }
        else if (isPresentInGame(evo.species, evo.form)) ++count;
        else if (count == 0) ++start;
        else break;
    }
    std::vector<Evo> history(chain.begin() + static_cast<std::ptrdiff_t>(start),
                             chain.begin() + static_cast<std::ptrdiff_t>(start + count));
    // CleanEvolve.
    size_t i = 1;
    for (; i < history.size(); ++i) {
        const Evo next = history[i - 1];
        Evo& evo = history[i];
        if (evo.levelMin > next.levelMax - next.levelUpRequired) break;
        evo.levelMax = next.levelMax;
    }
    history.resize(std::min(i, history.size()));
    return history;
}

} // namespace

namespace {

// ---------------------------------------------------------------------------
// Encounter template view shared by the generator, the move check and the verifiers
// ---------------------------------------------------------------------------

enum class Kind : uint8_t {
    Invalid, Slot, Static, Trade, Gift, GiftNY, GiftJPN, Colo, Egg,
    // Colosseum / XD (GameCube) templates, EncounterEnumerator3GC order.
    XdTrade, ColoShadow, ColoStarter, ColoGiftCxd, XdShadow, XdStatic, XdSlot,
};

constexpr bool isShadowKind(Kind k) noexcept { return k == Kind::ColoShadow || k == Kind::XdShadow; }
/// IFatefulEncounterReadOnly { FatefulEncounter: true }: every XD template.
constexpr bool isXdFatefulKind(Kind k) noexcept {
    return k == Kind::XdTrade || k == Kind::XdShadow || k == Kind::XdStatic || k == Kind::XdSlot;
}

enum Rating : uint8_t { RMatch = 0, RDeferred = 1, RDeferredErrors = 2, RPartialMatch = 3, RMaxNotMatch = 4 };

struct Enc {
    Kind kind = Kind::Invalid;
    uint16_t index = 0;      // template index (slot index for Kind::Slot)
    uint16_t area = 0;       // area index for Kind::Slot
    uint16_t species = 0;
    uint8_t form = 0;
    uint8_t levelMin = 0, levelMax = 0;
    uint8_t versionMask = 0; // template version mask (learn source)
    uint8_t eggVersion = 0;  // Kind::Egg: version id the egg was bred in
    bool isEgg = false;      // template IsEgg
    bool hasMoveset = false; // IMoveset with HasMoves
    std::array<uint16_t, 4> moves{};
    uint8_t fixedBall = 0;   // 0 none, 4 Poke Ball, 5 Safari Ball
    uint8_t rating = RMatch;
    bool eReader = false;    // Kind::ColoShadow: Encounters3Colo.EReader row (Japanese Card e Room)
};

const char* kindName(Kind k) noexcept {
    switch (k) {
        case Kind::Slot: return "wild slot";
        case Kind::Static: return "static encounter";
        case Kind::Trade: return "in-game trade";
        case Kind::Gift: return "event gift";
        case Kind::GiftNY: return "PCNY gift";
        case Kind::GiftJPN: return "PCJP gift";
        case Kind::Colo: return "Colosseum bonus gift";
        case Kind::Egg: return "egg";
        case Kind::XdTrade: return "XD in-game trade";
        case Kind::ColoShadow: return "Colosseum shadow Pokemon";
        case Kind::ColoStarter: return "Colosseum starter";
        case Kind::ColoGiftCxd: return "Colosseum gift";
        case Kind::XdShadow: return "XD shadow Pokemon";
        case Kind::XdStatic: return "XD gift";
        case Kind::XdSlot: return "XD Poke Spot slot";
        default: return "none";
    }
}

// ---------------------------------------------------------------------------
// Moves (PKHeX LearnVerifier, LearnGroup3, MoveBreed3)
// ---------------------------------------------------------------------------

enum MoveCode : uint8_t {
    MNone = 0, MEmpty, MInitial, MLevelUp, MTMHM, MTutor, MSketch, MShedinjaEvo, MEggMove,
    MInheritLevelUp, MSpecialEgg, MEggSource, MDuplicate, MEmptyInvalid, MUnobtainable,
};

bool moveValid(uint8_t c) noexcept { return c >= MEmpty && c <= MEggSource; }

using MoveResults = std::array<uint8_t, 4>;

uint8_t learnStageE(uint16_t species, const Evo& evo, uint16_t move) noexcept {
    uint8_t level = 0;
    if (levelLearnMove(1, species, move, level) && level <= evo.levelMax) return MLevelUp;
    if (canLearnTM(species, move)) return MTMHM;
    if (canLearnHM(species, move)) return MTMHM;
    if (canLearnTutorE(species, move)) return MTutor;
    return MNone;
}

uint8_t learnStageRS(uint16_t species, const Evo& evo, uint16_t move) noexcept {
    uint8_t level = 0;
    if (levelLearnMove(0, species, move, level) && level <= evo.levelMax) return MLevelUp;
    if (species == 151 && indexOf(D::kTutorMew, move) >= 0) return MTutor;
    if (move == D::kMoveSelfDestruct && sortedContains(D::kXDSelfDestruct, D::kXDSelfDestructCount, species)) return MTutor;
    if (move == D::kMoveSkyAttack && sortedContains(D::kXDSkyAttack, D::kXDSkyAttackCount, species)) return MTutor;
    if (move == D::kMoveNightmare && sortedContains(D::kXDNightmare, D::kXDNightmareCount, species)) return MTutor;
    return MNone;
}

uint8_t learnStageFR(uint16_t species, const Evo& evo, uint16_t move) noexcept {
    uint8_t level = 0;
    if (levelLearnMove(2, species, move, level) && level <= evo.levelMax) return MLevelUp;
    if (move == D::kMoveBlastBurn && species == 6) return MTutor;
    if (move == D::kMoveHydroCannon && species == 9) return MTutor;
    if (move == D::kMoveFrenzyPlant && species == 3) return MTutor;
    return MNone;
}

uint8_t learnStageLG(uint16_t species, const Evo& evo, uint16_t move) noexcept {
    uint8_t level = 0;
    if (levelLearnMove(3, species, move, level) && level <= evo.levelMax) return MLevelUp;
    return MNone;
}

bool canInheritMoves(uint16_t species) noexcept {
    const uint8_t gr = personal(species).gender;
    if (gr != 0 && gr != 255) return true;
    return species == 29 || species == 32 || species == 313 || species == 314;
}

/// MoveBreed3.Validate: can a Gen III egg have exactly these moves?
struct Breed3 {
    std::array<uint8_t, 4> actual{};
    std::array<uint8_t, 4> possible{};
    std::array<uint16_t, 4> moves{};
    std::vector<uint16_t> base;
};

enum EggSrc : uint8_t { ESNone = 0, ESBase = 1, ESFatherEgg = 2, ESFatherTM = 3, ESParentLevelUp = 4, ESMax = 5, ESVoltTackle = 6 };

bool breedVerifyBase(const Breed3& b) noexcept {
    int count = 0;
    for (uint8_t x : b.actual) {
        if (x == ESBase) ++count;
        else break;
    }
    const int baseLen = static_cast<int>(b.base.size());
    if (baseLen < count) return false;
    if (b.moves[3] == 0 && count != baseLen) return false;
    for (int i = count - 1, k = baseLen - 1; i >= 0; --i, --k) {
        if (b.base[static_cast<size_t>(k)] != b.moves[static_cast<size_t>(i)]) return false;
    }
    if (baseLen == count) return true;
    for (int i = count; i < 4; ++i) {
        const bool isBase = (b.possible[static_cast<size_t>(i)] & (1u << ESBase)) != 0;
        if (!isBase) continue;
        const int baseIndex = indexOf(b.base, b.moves[static_cast<size_t>(i)]);
        const int min = 4 - baseLen + baseIndex;
        if (i < min + count) return false;
    }
    return true;
}

bool breedRecurse(Breed3& b, int start, uint8_t type) noexcept {
    int i = start;
    do {
        const uint8_t unpeel = static_cast<uint8_t>(type - 1);
        if (unpeel != 0 && breedRecurse(b, i, unpeel)) return true;
        const uint8_t permit = b.possible[static_cast<size_t>(i)];
        if ((permit & (1u << type)) == 0) return false;
        b.actual[static_cast<size_t>(i)] = type;
    } while (--i >= 0);
    return breedVerifyBase(b);
}

bool moveBreed3(uint16_t species, uint8_t version, const std::array<uint16_t, 4>& moves) {
    int count = indexOf(moves, 0);
    if (count == 0) return false;
    if (count == -1) count = 4;
    if (species > D::kMaxSpecies) return false;
    const int source = learnSourceForVersion(version);
    Breed3 b;
    b.moves = moves;
    b.base = baseEggMoves(source, species, 5);
    if (species == 172 && moves[static_cast<size_t>(count - 1)] == D::kMoveVoltTackle && version == kVerE)
        b.actual[static_cast<size_t>(--count)] = ESVoltTackle;
    if (count == 0) return breedVerifyBase(b);
    const bool inherit = canInheritMoves(species);
    for (int i = 0; i < count; ++i) {
        const uint16_t move = moves[static_cast<size_t>(i)];
        uint8_t& p = b.possible[static_cast<size_t>(i)];
        if (indexOf(b.base, move) >= 0) p |= 1u << ESBase;
        if (inherit && learnsetContains(source, species, move)) p |= 1u << ESParentLevelUp;
        if (isEggMove(species, move)) p |= 1u << ESFatherEgg;
        if (canLearnTM(species, move) || canLearnHM(species, move)) p |= 1u << ESFatherTM;
    }
    return breedRecurse(b, count - 1, ESMax - 1);
}

void finalizeMoves(MoveResults& r, const std::array<uint16_t, 4>& cur) noexcept {
    bool emptySlot = false;
    for (size_t i = 0; i < 4; ++i) {
        const uint16_t move = cur[i];
        if (move == 0) { emptySlot = true; continue; }
        if (emptySlot) {
            for (size_t k = i; k-- > 0;) {
                if (cur[k] != 0) break;
                r[k] = MEmptyInvalid;
            }
            emptySlot = false;
            continue;
        }
        for (size_t k = 3; k > i; --k) {
            if (cur[k] != move) continue;
            r[i] = MDuplicate;
            break;
        }
    }
    if (cur[0] == 0) r[0] = MEmptyInvalid;
}

void verifyMovesInitial(MoveResults& r, const std::array<uint16_t, 4>& cur, std::span<const uint16_t> initial) noexcept {
    for (size_t i = 0; i < initial.size() && i < 4; ++i) {
        if (cur[i] != initial[i]) r[i] = MUnobtainable;
        else if (cur[i] == 0) r[i] = MEmpty;
        else r[i] = MInitial;
    }
    for (size_t i = initial.size(); i < 4; ++i) r[i] = cur[i] == 0 ? MEmpty : MUnobtainable;
}

/// LearnVerifier.Verify for a PK3 against one candidate encounter.
MoveResults verifyMoves(const Pk3& pk, const Enc& enc, const std::vector<Evo>& history) {
    MoveResults r{};
    const auto& cur = pk.moves;
    if (pk.isEgg) {
        if (enc.kind == Kind::Egg) {
            if (moveBreed3(enc.species, enc.eggVersion, cur)) {
                for (size_t i = 0; i < 4; ++i) r[i] = cur[i] == 0 ? MEmpty : MEggSource;
            } else {
                // PKHeX marks the positions that differ from the closest legal egg moveset;
                // at least one always differs when the moveset cannot be bred.
                bool any = false;
                for (size_t i = 0; i < 4; ++i) {
                    if (cur[i] == 0) r[i] = MEmpty;
                    else { r[i] = MUnobtainable; any = true; }
                }
                if (!any) r[0] = MUnobtainable;
            }
            const uint16_t m1 = cur[0], m2 = cur[1], m3 = cur[2], m4 = cur[3];
            if (m1 != 0 && m1 == m2) r[1] = MDuplicate;
            else if (m3 != 0 && (m1 == m3 || m2 == m3)) r[2] = MDuplicate;
            else if (m4 != 0 && (m1 == m4 || m2 == m4 || m3 == m4)) r[3] = MDuplicate;
        } else if (enc.hasMoveset) {
            verifyMovesInitial(r, cur, enc.moves);
        } else if (enc.kind != Kind::Invalid) {
            const auto initial = baseEggMoves(learnSourceForMask(enc.versionMask), enc.species, enc.levelMin);
            verifyMovesInitial(r, cur, initial);
        } else {
            // EncounterInvalid: version-specific learnset of the record's own version.
            const auto initial = baseEggMoves(learnSourceForVersion(pk.version), enc.species, enc.levelMin);
            verifyMovesInitial(r, cur, initial);
        }
        finalizeMoves(r, cur);
        return r;
    }

    if (enc.species == 235) { // Smeargle: every move can be Sketched
        for (size_t i = 4; i-- > 0;) {
            const uint16_t move = cur[i];
            if (move == 0) r[i] = MEmpty;
            else if (move <= D::kMaxMove && move != D::kMoveStruggle) r[i] = MSketch;
            else r[i] = MUnobtainable;
        }
        finalizeMoves(r, cur);
        return r;
    }

    for (size_t i = 4; i-- > 0;) {
        if (cur[i] != 0) break;
        r[i] = MEmpty;
    }
    if (enc.hasMoveset) {
        for (uint16_t move : enc.moves) {
            if (move == 0) break;
            const int index = indexOf(cur, move);
            if (index != -1) r[static_cast<size_t>(index)] = MInitial;
        }
    }
    for (const auto& evo : history) {
        const uint16_t species = evo.species;
        if (species > D::kMaxSpecies) continue;
        for (size_t i = 4; i-- > 0;) {
            if (moveValid(r[i])) continue;
            const uint16_t move = cur[i];
            uint8_t chk = learnStageE(species, evo, move);
            if (chk == MNone) chk = learnStageRS(species, evo, move);
            if (chk == MNone) chk = learnStageFR(species, evo, move);
            if (chk == MNone) chk = learnStageLG(species, evo, move);
            if (chk != MNone) r[i] = chk;
        }
    }
    if (enc.kind == Kind::Egg) {
        const int source = learnSourceForVersion(enc.eggVersion);
        const bool inherit = canInheritMoves(enc.species);
        for (size_t i = 4; i-- > 0;) {
            if (moveValid(r[i])) continue;
            const uint16_t move = cur[i];
            if (isEggMove(enc.species, move)) r[i] = MEggMove;
            else if (inherit && learnsetContains(source, enc.species, move)) r[i] = MInheritLevelUp;
            else if (move == D::kMoveVoltTackle && enc.species == 172 && enc.eggVersion == kVerE) r[i] = MSpecialEgg;
        }
    }
    if (enc.species == 290 && history.size() == 2 && history[0].species == 292) {
        bool allParsed = true;
        for (uint8_t c : r) if (c == MNone) allParsed = false;
        if (!allParsed) {
            const Evo& nincada = history.back();
            for (size_t i = 0; i < 4; ++i) {
                if (moveValid(r[i])) continue;
                const uint16_t move = cur[i];
                if (move == 0) break;
                uint8_t level = 0;
                if (!levelLearnMove(1, 291, move, level)) continue;
                if (!(nincada.levelMin <= level && level <= nincada.levelMax)) continue;
                r[i] = MShedinjaEvo;
                break;
            }
        }
    }
    for (auto& c : r) if (c == MNone) c = MUnobtainable;
    finalizeMoves(r, cur);
    return r;
}

bool allMovesValid(const MoveResults& r) noexcept {
    for (uint8_t c : r) if (!moveValid(c)) return false;
    return true;
}

} // namespace

namespace {

// ---------------------------------------------------------------------------
// Encounter templates (EncounterSlot3, EncounterStatic3, EncounterTrade3,
// EncounterGift3, EncounterGift3NY, EncounterGift3JPN, EncounterGift3Colo, EncounterEgg3)
// ---------------------------------------------------------------------------

constexpr uint8_t kBallPoke = 4, kBallSafari = 5;
constexpr uint8_t kLocEvent = 255;
constexpr uint16_t kUnspecifiedId = 0xFFFF;

bool isSafariLocation3(uint8_t loc) noexcept { return loc == 57 || loc == 136; }
uint8_t hatchLocation3(uint8_t version) noexcept { return version == kVerFR || version == kVerLG ? 146 : 32; }

uint8_t hatchBit(uint8_t version) noexcept {
    switch (version) {
        case kVerR: case kVerS: return 1;
        case kVerE: return 2;
        case kVerFR: case kVerLG: return 4;
        default: return 0;
    }
}

/// EggHatchLocation3.IsValidMet3 / IsValidMet3Any.
bool isValidMet3(uint8_t met, uint8_t version) noexcept {
    return met < D::kHatchLocationCount && (D::kHatchLocations[met] & hatchBit(version)) != 0;
}
bool isValidMet3Any(uint8_t met) noexcept { return met < D::kHatchLocationCount && D::kHatchLocations[met] != 0; }

/// WurmpleUtil.IsWurmpleEvoValid for a Gen III record (EC == PID).
bool wurmpleEvoValid(const Pk3& pk) noexcept {
    const uint32_t evoVal = ((pk.pid >> 16) % 10) / 5;
    if (pk.species == 266 || pk.species == 267) return evoVal == 0;
    if (pk.species == 268 || pk.species == 269) return evoVal == 1;
    return false;
}

Enc makeSlotEnc(uint16_t areaIndex, uint16_t slotIndex) noexcept {
    const auto& area = D::kAreas[areaIndex];
    const auto& slot = D::kSlots[slotIndex];
    Enc e;
    e.kind = Kind::Slot;
    e.index = slotIndex;
    e.area = areaIndex;
    e.species = slot.species;
    e.form = slot.form;
    e.levelMin = slot.minLevel;
    e.levelMax = slot.maxLevel;
    e.versionMask = area.version;
    e.hasMoveset = slot.moves[0] != 0;
    for (size_t i = 0; i < 4; ++i) e.moves[i] = slot.moves[i];
    e.fixedBall = isSafariLocation3(area.location) ? kBallSafari : 0;
    return e;
}

Enc makeStaticEnc(uint16_t index) noexcept {
    const auto& s = D::kStatics[index];
    Enc e;
    e.kind = Kind::Static;
    e.index = index;
    e.species = s.species;
    e.form = s.form;
    e.levelMin = e.levelMax = s.level;
    e.versionMask = s.versions;
    e.isEgg = s.egg;
    e.hasMoveset = s.moves[0] != 0;
    for (size_t i = 0; i < 4; ++i) e.moves[i] = s.moves[i];
    e.fixedBall = s.ball;
    return e;
}

Enc makeTradeEnc(uint16_t index) noexcept {
    const auto& t = D::kTrades[index];
    Enc e;
    e.kind = Kind::Trade;
    e.index = index;
    e.species = t.species;
    e.levelMin = t.level;
    e.levelMax = 100;
    e.versionMask = t.versions;
    e.fixedBall = kBallPoke;
    return e;
}

Enc makeGiftEnc(uint16_t index) noexcept {
    const auto& g = D::kGifts[index];
    Enc e;
    e.kind = Kind::Gift;
    e.index = index;
    e.species = g.species;
    e.levelMin = e.levelMax = g.level;
    e.versionMask = g.versions;
    e.isEgg = g.egg;
    e.hasMoveset = g.moves[0] != 0;
    for (size_t i = 0; i < 4; ++i) e.moves[i] = g.moves[i];
    e.fixedBall = kBallPoke;
    return e;
}

Enc makeNYEnc(uint16_t index) noexcept {
    const auto& g = D::kGiftsNY[index];
    Enc e;
    e.kind = Kind::GiftNY;
    e.index = index;
    e.species = g.species;
    e.levelMin = e.levelMax = g.level;
    e.versionMask = D::R | D::S;
    e.hasMoveset = g.moves[0] != 0;
    for (size_t i = 0; i < 4; ++i) e.moves[i] = g.moves[i];
    e.fixedBall = kBallPoke;
    return e;
}

constexpr uint8_t kJpnLevel = 10;

Enc makeJPNEnc(uint16_t index) noexcept {
    const auto& g = D::kGiftsJPN[index];
    Enc e;
    e.kind = Kind::GiftJPN;
    e.index = index;
    e.species = g.species;
    e.levelMin = e.levelMax = kJpnLevel;
    e.versionMask = D::R;
    e.fixedBall = kBallPoke;
    return e;
}

Enc makeColoEnc(uint16_t index) noexcept {
    const auto& c = D::kColoGifts[index];
    Enc e;
    e.kind = Kind::Colo;
    e.index = index;
    e.species = c.species;
    e.levelMin = e.levelMax = c.level;
    e.versionMask = versionBit(c.version);
    e.hasMoveset = c.moves[0] != 0;
    for (size_t i = 0; i < 4; ++i) e.moves[i] = c.moves[i];
    e.fixedBall = kBallPoke;
    return e;
}

const X::Shadow& shadowOf(const Enc& e) noexcept {
    if (e.kind == Kind::XdShadow) return X::kXdShadows[e.index];
    return e.eReader ? X::kColoEReader[e.index] : X::kColoShadows[e.index];
}

void setMoves(Enc& e, const uint16_t* moves) noexcept {
    e.hasMoveset = moves[0] != 0;
    for (size_t i = 0; i < 4; ++i) e.moves[i] = moves[i];
}

Enc makeShadowEnc(Kind kind, uint16_t index, bool eReader) noexcept {
    Enc e;
    e.kind = kind;
    e.index = index;
    e.eReader = eReader;
    const X::Shadow& s = shadowOf(e);
    e.species = s.species;
    e.levelMin = e.levelMax = s.level;
    e.versionMask = D::CXD;
    setMoves(e, s.moves);
    e.fixedBall = s.fixedBall;
    return e;
}

Enc makeColoStarterEnc(uint16_t index) noexcept {
    const auto& s = X::kColoStarters[index];
    Enc e;
    e.kind = Kind::ColoStarter;
    e.index = index;
    e.species = s.species;
    e.levelMin = e.levelMax = s.level;
    e.versionMask = D::CXD;
    setMoves(e, s.moves);
    e.fixedBall = kBallPoke;
    return e;
}

Enc makeColoGiftCxdEnc(uint16_t index) noexcept {
    const auto& g = X::kColoGifts[index];
    Enc e;
    e.kind = Kind::ColoGiftCxd;
    e.index = index;
    e.species = g.species;
    e.levelMin = e.levelMax = g.level;
    e.versionMask = D::CXD;
    setMoves(e, g.moves);
    e.fixedBall = kBallPoke;
    return e;
}

Enc makeXdStaticEnc(uint16_t index) noexcept {
    const auto& g = X::kXdGifts[index];
    Enc e;
    e.kind = Kind::XdStatic;
    e.index = index;
    e.species = g.species;
    e.levelMin = e.levelMax = g.level;
    e.versionMask = D::CXD;
    setMoves(e, g.moves);
    e.fixedBall = kBallPoke;
    return e;
}

Enc makeXdTradeEnc(uint16_t index) noexcept {
    const auto& t = X::kXdTrades[index];
    Enc e;
    e.kind = Kind::XdTrade;
    e.index = index;
    e.species = t.species;
    e.levelMin = e.levelMax = t.level;
    e.versionMask = D::CXD;
    setMoves(e, t.moves);
    e.fixedBall = kBallPoke;
    return e;
}

/// EncounterSlot3XD: index is the slot number (0..2), area the Poke Spot (0..2).
Enc makeXdSlotEnc(uint16_t area, uint16_t slot) noexcept {
    const auto& s = X::kPokeSpots[area].slots[slot];
    Enc e;
    e.kind = Kind::XdSlot;
    e.index = slot;
    e.area = area;
    e.species = s.species;
    e.levelMin = s.levelMin;
    e.levelMax = s.levelMax;
    e.versionMask = D::CXD;
    return e;
}

Enc makeEggEnc(uint16_t species, uint8_t version) noexcept {
    Enc e;
    e.kind = Kind::Egg;
    e.species = species;
    e.levelMin = e.levelMax = 5;
    e.versionMask = versionBit(version);
    e.eggVersion = version;
    e.isEgg = true;
    e.fixedBall = kBallPoke;
    return e;
}

/// EncounterInvalid(pk): the stand-in PKHeX uses when nothing matched.
Enc makeInvalidEnc(const Pk3& pk) noexcept {
    Enc e;
    e.kind = Kind::Invalid;
    e.species = pk.species;
    e.form = pk.form;
    e.levelMin = pk.metLevel;
    e.levelMax = pk.currentLevel;
    e.versionMask = versionBit(pk.version);
    e.isEgg = pk.isEgg;
    return e;
}

bool nyTrainerNameValid(uint8_t distribution, std::u16string_view name) noexcept {
    const bool b = name == u"PCNYb", c = name == u"PCNYc", d = name == u"PCNYd";
    switch (distribution) {
        case 0: case 2: case 3: case 4: return b || c;   // Evolution, Monster, Halloween, EXDragon
        case 1: return b || c || d;                        // Dragon
        case 5: case 6: case 7: return c || d;            // UnknownSpring, Colosseum, Box
        default: return d;                                 // BabyTrade, SlitherSwim, AncientAliens, Sixth
    }
}

constexpr uint16_t kJpnTrainerIds[6] = {51126, 51224, 60114, 60227, 60321, 60505};

bool jpnTrainerNameValid(uint8_t distribution, std::u16string_view name) noexcept {
    if (name == u"トウキョー" || name == u"ヨコハマ" || name == u"ナゴヤ" || name == u"オーサカ" ||
        name == u"フクオカ")
        return true;
    return distribution != 5 && name == u"サッポロ";
}

bool isGenderSpecificMatch(uint8_t otGenderRule, uint8_t value) noexcept {
    switch (otGenderRule) {
        case 1: case 8: return value == 0; // Only0, RandD3_0
        case 2: case 9: return value == 1; // Only1, RandD3_1
        default: return true;
    }
}

bool isMatchExact(const Pk3& pk, const Enc& e, const Evo& evo) {
    switch (e.kind) {
        case Kind::Slot:
            if (e.form != evo.form) return false;
            return e.levelMin <= pk.metLevel && pk.metLevel <= e.levelMax;
        case Kind::Static: {
            const auto& s = D::kStatics[e.index];
            const uint8_t met = pk.metLocation;
            bool location;
            if (s.egg) location = !pk.isEgg || met == s.location;
            else if (!s.roaming) location = s.location == met;
            else if ((s.versions & (D::FR | D::LG)) == 0) location = met >= 16 && met <= 49;
            else location = met >= 101 && met <= 125;
            if (!location) return false;
            if (!s.egg) {
                if (pk.metLevel != s.level) return false;
            } else if (!(pk.metLevel == 0 && pk.currentLevel >= 5)) {
                return false;
            }
            if (s.form != evo.form && s.species != 386) return false; // FormInfo.IsFormChangeable: Deoxys
            return true;
        }
        case Kind::Trade: {
            const auto& t = D::kTrades[e.index];
            if (pk.pid != t.pid) return false;
            for (size_t i = 6; i-- > 0;) {
                if (t.ivs[i] > 31) continue;
                if (t.ivs[i] != pk.ivs[i]) return false;
            }
            if (pk.tid != t.tid || pk.sid != t.sid) return false;
            if (evo.levelMax < t.level) return false;
            if (evo.form != 0) return false;
            if (pk.gender != t.gender) return false;
            if (pk.otGender != t.otGender) return false;
            for (size_t i = 0; i < 6; ++i)
                if (pk.contest[i] < t.contest[i]) return false;
            return true;
        }
        case Kind::Gift: {
            const auto& g = D::kGifts[e.index];
            if ((g.versions & versionBit(pk.version)) == 0) return false;
            if (pk.isEgg && !g.egg) return false;
            const bool hatchedEgg = g.egg && !pk.isEgg;
            if (!hatchedEgg) {
                if (g.sid != kUnspecifiedId && g.sid != pk.sid) return false;
                if (g.tid != kUnspecifiedId && g.tid != pk.tid) return false;
                if (!isGenderSpecificMatch(g.otGender, pk.otGender)) return false;
                if (g.ot != nullptr && g.ot[0] != 0 && pk.ot != std::u16string_view(g.ot)) return false;
            }
            if (evo.form != 0) return false;
            if (g.language != 0) {
                if (g.language != pk.language) return false;
            } else if (!g.egg && pk.language == 1) {
                return false;
            }
            if (pk.ball != kBallPoke) return false;
            if (g.fateful != pk.fateful && !g.egg) return false;
            if (hatchedEgg) return true;
            return g.metLevel == pk.metLevel && pk.metLocation == kLocEvent;
        }
        case Kind::GiftNY: {
            const auto& g = D::kGiftsNY[e.index];
            if (pk.version != kVerR && pk.version != kVerS) return false;
            if (pk.isEgg) return false;
            if (pk.sid != 0) return false;
            if (pk.tid == 0 || pk.tid >= 3000) return false;
            if (pk.ot.size() <= 4) return false;
            if (!nyTrainerNameValid(g.distribution, pk.ot)) return false;
            if (evo.form != 0) return false;
            if (pk.language == 1) return false;
            if (pk.ball != kBallPoke) return false;
            return g.level == pk.metLevel && pk.metLocation == kLocEvent;
        }
        case Kind::GiftJPN: {
            const auto& g = D::kGiftsJPN[e.index];
            if (pk.version != kVerR) return false;
            if (pk.isEgg) return false;
            if (pk.sid != 0) return false;
            if (g.distribution >= 6 || pk.tid != kJpnTrainerIds[g.distribution]) return false;
            if (pk.ot.size() <= 2) return false;
            if (!jpnTrainerNameValid(g.distribution, pk.ot)) return false;
            if (evo.form != 0) return false;
            if (pk.language != 1) return false;
            if (pk.ball != kBallPoke) return false;
            return pk.metLevel == kJpnLevel && pk.metLocation == kLocEvent;
        }
        case Kind::Colo: {
            const auto& c = D::kColoGifts[e.index];
            if (pk.version != c.version) return false;
            if (pk.metLocation != c.location) return false;
            if (pk.metLevel != c.level) return false;
            return evo.form == 0;
        }
        // GameCube templates (PK3 egg location is always 0, so IsMatchEggLocation holds).
        case Kind::XdTrade: { // EncounterTrade3XD.IsMatchExact: the SID is the player's
            const auto& t = X::kXdTrades[e.index];
            return pk.metLocation == t.location && pk.metLevel == t.level && pk.tid == t.tid && evo.form == 0;
        }
        case Kind::ColoShadow: { // EncounterShadow3Colo.IsMatchExact
            const auto& s = shadowOf(e);
            return pk.metLocation == s.location && pk.metLevel == s.level && evo.form == 0;
        }
        case Kind::ColoStarter: { // EncounterStarter3Colo.IsMatchExact (male only)
            const auto& s = X::kColoStarters[e.index];
            return pk.metLocation == X::kColoStarterLocation && pk.metLevel == s.level && pk.gender == 0 &&
                   evo.form == 0;
        }
        case Kind::ColoGiftCxd: { // EncounterGift3Colo.IsMatchExact (Version CXD)
            const auto& g = X::kColoGifts[e.index];
            return pk.version == kVerCXD && pk.metLocation == g.location && pk.metLevel == g.level && evo.form == 0;
        }
        case Kind::XdShadow: { // EncounterShadow3XD.IsMatchLocation: Miror B. rebattles
            const auto& s = shadowOf(e);
            const uint8_t met = pk.metLocation;
            const bool location = met == s.location || met == 59 || met == 90 || met == 91 || met == 92 || met == 113;
            return location && pk.metLevel == s.level && evo.form == 0;
        }
        case Kind::XdStatic: { // EncounterStatic3XD.IsMatchExact
            const auto& g = X::kXdGifts[e.index];
            return pk.metLocation == g.location && pk.metLevel == g.level && evo.form == 0;
        }
        case Kind::XdSlot: // EncounterSlot3XD.IsMatchExact: handled by the area location
            return true;
        default:
            return false;
    }
}

uint8_t matchRating(const Pk3& pk, const Enc& e) {
    switch (e.kind) {
        case Kind::Slot: {
            const auto& area = D::kAreas[e.area];
            if ((pk.ball == kBallSafari) != isSafariLocation3(area.location)) return RPartialMatch;
            if (e.species == 265 && pk.species != 265 && !wurmpleEvoValid(pk)) return RPartialMatch;
            return RMatch;
        }
        case Kind::Static: {
            const auto& s = D::kStatics[e.index];
            if ((pk.ball == kBallSafari) != isSafariLocation3(s.location)) return RPartialMatch;
            if (s.ball == kBallPoke && pk.ball != kBallPoke) return RPartialMatch;
            if (s.fateful != pk.fateful) return RPartialMatch;
            return RMatch;
        }
        case Kind::Gift: {
            const auto& g = D::kGifts[e.index];
            if (g.egg && pk.isEgg && g.fateful != pk.fateful) return RDeferredErrors;
            return RMatch;
        }
        case Kind::Colo: {
            const auto& c = D::kColoGifts[e.index];
            if (pk.ball != kBallPoke) return RPartialMatch;
            if (c.version == kVerR && pk.language != 1) return RPartialMatch;
            return RMatch;
        }
        case Kind::ColoShadow: { // EncounterShadow3Colo.IsMatchPartial
            if (pk.fateful) return RPartialMatch;
            const auto& s = shadowOf(e);
            return (s.fixedBall != 0 && pk.ball != s.fixedBall) ? RPartialMatch : RMatch;
        }
        case Kind::XdShadow: { // EncounterShadow3XD.IsMatchPartial
            if (!pk.fateful) return RPartialMatch;
            const auto& s = shadowOf(e);
            return (s.fixedBall != 0 && pk.ball != s.fixedBall) ? RPartialMatch : RMatch;
        }
        case Kind::ColoStarter: // EncounterStarter3Colo.GetMatchRating: fateful clashes with XD's Eevee
            if (pk.ball != kBallPoke) return RPartialMatch;
            return pk.fateful ? RDeferredErrors : RMatch;
        case Kind::ColoGiftCxd: case Kind::XdStatic: case Kind::XdTrade: // FixedBall Poke
            return pk.ball != kBallPoke ? RPartialMatch : RMatch;
        default:
            return RMatch;
    }
}

// ---------------------------------------------------------------------------
// EncounterEnumerator3: template candidates in PKHeX's exact order
// ---------------------------------------------------------------------------

struct Enumerator {
    const Pk3& pk;
    const std::vector<Evo>& chain;
    std::vector<Enc> out;
    Enc deferred;
    bool hasDeferred = false;
    uint8_t rating = RMaxNotMatch;

    void consider(const Enc& enc) {
        for (const auto& evo : chain) {
            if (evo.species != enc.species) continue;
            if (!isMatchExact(pk, enc, evo)) break;
            const uint8_t r = matchRating(pk, enc);
            if (r == RMatch) {
                out.push_back(enc);
            } else if (r < rating) {
                deferred = enc;
                deferred.rating = r;
                rating = r;
                hasDeferred = true;
            }
            break;
        }
    }

    void statics(D::Range range) {
        for (uint16_t i = 0; i < range.count; ++i) consider(makeStaticEnc(static_cast<uint16_t>(range.start + i)));
    }

    void trades(D::Range range) {
        for (uint16_t i = 0; i < range.count; ++i) consider(makeTradeEnc(static_cast<uint16_t>(range.start + i)));
    }

    void areas(size_t begin, size_t end, uint8_t versionMask) {
        for (size_t a = begin; a < end; ++a) {
            const auto& area = D::kAreas[a];
            if (area.version != versionMask) continue;
            if (area.location != pk.metLocation) continue;
            for (uint16_t s = 0; s < area.slotCount; ++s)
                consider(makeSlotEnc(static_cast<uint16_t>(a), static_cast<uint16_t>(area.slotStart + s)));
        }
    }

    void slots() {
        if (pk.isEgg) return; // EncounterStateUtil.CanBeWildEncounter
        const uint8_t bit = versionBit(pk.version);
        if (pk.version == kVerFR || pk.version == kVerLG) {
            areas(0, D::kAreaCount, bit);
            return;
        }
        areas(D::kAreaCount, D::kAreaCount + D::kSwarmAreaCount, D::R | D::S | D::E);
        areas(0, D::kAreaCount, bit);
    }

    void staticsForVersion() {
        switch (pk.version) {
            case kVerR: statics(D::kStaticR); statics(D::kStaticRSE); break;
            case kVerS: statics(D::kStaticS); statics(D::kStaticRSE); break;
            case kVerE: statics(D::kStaticE); statics(D::kStaticRSE); break;
            case kVerFR: statics(D::kStaticFR); statics(D::kStaticFRLG); break;
            case kVerLG: statics(D::kStaticLG); statics(D::kStaticFRLG); break;
            default: break;
        }
    }
};

/// EvolutionTree.Evolves3.GetBaseSpeciesForm: walk the first pre-evolution links.
uint16_t baseSpecies3(uint16_t species) noexcept {
    for (int guard = 0; guard < 8; ++guard) {
        if (species == 0 || species > D::kMaxSpecies) break;
        const auto& node = reverseLineage()[species];
        if (node.first.species == 0) break;
        species = node.first.species;
    }
    return species;
}

bool isSplitBreedNotBaby3(uint16_t species) noexcept { return species == 183 || species == 202; }

bool canHatchAsEgg3(uint16_t species) noexcept {
    switch (species) {
        case 132: case 144: case 145: case 146: case 150: case 151: case 201:
        case 243: case 244: case 245: case 249: case 250: case 251:
            return false;
        default:
            return !(species >= 377 && species <= 386);
    }
}

bool tryGetEgg3(const std::vector<Evo>& chain, uint8_t version, Enc& out) {
    const Evo& devolved = chain.back();
    if (!(devolved.levelMin <= 5 && 5 <= devolved.levelMax)) return false;
    const uint16_t baby = baseSpecies3(devolved.species);
    if (baby != devolved.species && !isSplitBreedNotBaby3(devolved.species)) return false;
    if (!canHatchAsEgg3(baby)) return false;
    if (devolved.form != 0 && baby == devolved.species) return false; // Breeding.CanHatchAsEgg(species, form, Gen3)
    if (!isPresentInGame(baby, 0)) return false;
    out = makeEggEnc(baby, version);
    return true;
}

bool tryGetSplit3(const Enc& other, const std::vector<Evo>& chain, Enc& out) {
    const Evo* devolved = &chain.back();
    if (other.species == devolved->species) {
        if (chain.size() < 2) return false;
        devolved = &chain[chain.size() - 2];
    }
    if (!isSplitBreedNotBaby3(devolved->species)) return false;
    out = other;
    out.species = devolved->species;
    return true;
}

std::vector<Enc> enumerateEncounters(const Pk3& pk, const std::vector<Evo>& chain) {
    Enumerator en{pk, chain, {}, {}, false, RMaxNotMatch};
    if (chain.empty()) return {};
    for (uint16_t i = 0; i < D::kColoGiftCount; ++i) en.consider(makeColoEnc(i));
    for (uint16_t i = 0; i < D::kGiftCount; ++i) en.consider(makeGiftEnc(i));
    if (pk.language == 2) {
        for (uint16_t i = 0; i < D::kGiftNYCount; ++i) en.consider(makeNYEnc(i));
    } else if (pk.language == 1) {
        for (uint16_t i = 0; i < D::kGiftJPNCount; ++i) en.consider(makeJPNEnc(i));
    }
    switch (pk.version) {
        case kVerE: en.trades(D::kTradeGift_E); break;
        case kVerFR: en.trades(D::kTradeGift_FR); en.trades(D::kTradeGift_FRLG); break;
        case kVerLG: en.trades(D::kTradeGift_LG); en.trades(D::kTradeGift_FRLG); break;
        default: en.trades(D::kTradeGift_RS); en.trades(D::kTradeGift_E); break;
    }
    const bool mustBeSlot = pk.ball == kBallSafari;
    if (mustBeSlot) {
        en.slots();
        en.staticsForVersion();
    } else {
        en.staticsForVersion();
        en.slots();
    }
    Enc egg;
    if (tryGetEgg3(chain, pk.version, egg)) {
        en.out.push_back(egg);
        Enc split;
        if (tryGetSplit3(egg, chain, split)) en.out.push_back(split);
    }
    if (en.hasDeferred) en.out.push_back(en.deferred);
    return std::move(en.out);
}

/// EncounterEnumerator3GC: Colosseum / XD template candidates in PKHeX's exact order
/// (XD trades, Colosseum shadows, starters, Duking's Plusle, XD shadows, XD gifts, then
/// the Japanese e-Reader shadows and the Poke Spot slots at the met location, then the
/// deferred partial match). The state machine moves to Fallback once an e-Reader row
/// matched, so the Poke Spot slots are only enumerated when none did.
std::vector<Enc> enumerateEncountersGC(const Pk3& pk, const std::vector<Evo>& chain) {
    Enumerator en{pk, chain, {}, {}, false, RMaxNotMatch};
    if (chain.empty() || pk.isEgg) return {}; // EncounterStateUtil.CanBeWildEncounter (PK3 egg location is 0)
    for (uint16_t i = 0; i < X::kXdTradeCount; ++i) en.consider(makeXdTradeEnc(i));
    for (uint16_t i = 0; i < X::kColoShadowCount; ++i) en.consider(makeShadowEnc(Kind::ColoShadow, i, false));
    for (uint16_t i = 0; i < X::kColoStarterCount; ++i) en.consider(makeColoStarterEnc(i));
    for (uint16_t i = 0; i < X::kColoGiftCount; ++i) en.consider(makeColoGiftCxdEnc(i));
    for (uint16_t i = 0; i < X::kXdShadowCount; ++i) en.consider(makeShadowEnc(Kind::XdShadow, i, false));
    for (uint16_t i = 0; i < X::kXdGiftCount; ++i) en.consider(makeXdStaticEnc(i));
    bool eReaderMatched = false;
    if (pk.language == 1) {
        for (uint16_t i = 0; i < X::kColoEReaderCount && !eReaderMatched; ++i) {
            const size_t before = en.out.size();
            en.consider(makeShadowEnc(Kind::ColoShadow, i, true));
            eReaderMatched = en.out.size() > before;
        }
    }
    if (!eReaderMatched) {
        for (uint16_t a = 0; a < X::kPokeSpotCount; ++a) {
            if (X::kPokeSpots[a].location != pk.metLocation) continue;
            for (uint16_t slot = 0; slot < 3; ++slot) en.consider(makeXdSlotEnc(a, slot));
        }
    }
    if (en.hasDeferred) en.out.push_back(en.deferred);
    return std::move(en.out);
}

// ---------------------------------------------------------------------------
// Method H: wild slot lead/frame search (PKHeX MethodH, SlotMethodH, LeadFinder)
// ---------------------------------------------------------------------------

enum Lead : uint8_t {
    LInvalid = 0, LStaticMagnetFail = 1, LCuteCharmFail = 2, LSynchronizeFail = 3, LPressureFail = 4,
    LIntimidateFail = 5, LIlluminate = 6, LSuctionCups = 7, LPressure = 8, LMagnetPull = 9, LStatic = 10,
    LCuteCharm = 11, LSynchronize = 12, LNone = 255,
};

constexpr uint32_t kWindowCap = 1u << 22; // guard for the unbounded PKHeX reversal loops

struct SlotCtx {
    const D::Slot* slot = nullptr;
    const D::Area* area = nullptr;
    uint8_t met = 0; // evo LevelMin == LevelMax == MetLevel
};

uint8_t slotMethodH(uint8_t type, uint32_t rand) noexcept {
    const uint32_t roll = rand % 100;
    switch (type) {
        case 0:
            if (roll < 20) return 0;
            if (roll < 40) return 1;
            if (roll < 50) return 2;
            if (roll < 60) return 3;
            if (roll < 70) return 4;
            if (roll < 80) return 5;
            if (roll < 85) return 6;
            if (roll < 90) return 7;
            if (roll < 94) return 8;
            if (roll < 98) return 9;
            if (roll < 99) return 10;
            return 11;
        case 1: case 5:
            if (roll < 60) return 0;
            if (roll < 90) return 1;
            if (roll < 95) return 2;
            if (roll < 99) return 3;
            return 4;
        case 2: return roll < 70 ? 0 : 1;
        case 3:
            if (roll < 60) return 0;
            if (roll < 80) return 1;
            return 2;
        case 4:
            if (roll < 40) return 0;
            if (roll < 80) return 1;
            if (roll < 95) return 2;
            if (roll < 99) return 3;
            return 4;
        case 6: case 7: return roll < 50 ? 0 : 255;
        default: return 255;
    }
}

struct FrameCheck {
    uint32_t s1, s2, s3;
    explicit FrameCheck(uint32_t seed) noexcept : s1(lcPrev(seed)), s2(lcPrev(s1)), s3(lcPrev(s2)) {}
    uint32_t p1() const noexcept { return s1 >> 16; }
    uint32_t p2() const noexcept { return s2 >> 16; }
    uint32_t p3() const noexcept { return s3 >> 16; }
    uint32_t s4() const noexcept { return lcPrev(s3); }
};

bool slotValid(const SlotCtx& c, uint32_t rand) noexcept { return slotMethodH(c.area->type, rand) == c.slot->slot; }

bool levelValid(const SlotCtx& c, uint32_t rand) noexcept {
    const uint32_t mod = 1u + c.slot->maxLevel - c.slot->minLevel;
    return (rand % mod) + c.slot->minLevel == c.met;
}

bool levelValidMinus1(const SlotCtx& c, uint32_t rand) noexcept {
    const uint32_t mod = 1u + c.slot->maxLevel - c.slot->minLevel;
    uint32_t bias = rand % mod;
    if (bias != 0) --bias;
    return c.slot->minLevel + bias == c.met;
}

uint8_t pressureLevel(const SlotCtx& c) noexcept {
    uint8_t levelMax = c.slot->maxLevel;
    if (c.area->type != 0) return levelMax;
    for (uint16_t i = 0; i < c.area->slotCount; ++i) {
        const auto& s = D::kSlots[c.area->slotStart + i];
        if (s.species != c.slot->species) continue;
        if (s.maxLevel < levelMax) continue;
        levelMax = s.maxLevel;
    }
    return levelMax;
}

bool activation(const SlotCtx& c, uint32_t seed) noexcept {
    if (c.area->type != 5) return true; // only Rock Smash checks the encounter rate
    uint32_t encRate = c.area->rate * 16u;
    encRate += encRate >> 1;
    return (seed >> 16) % 2880u < encRate;
}

bool slotRegular(const SlotCtx& c, const FrameCheck& f) noexcept {
    return levelValid(c, f.p1()) && slotValid(c, f.p2()) && activation(c, f.s3);
}

bool slotFrom1Skip(const SlotCtx& c, const FrameCheck& f) noexcept {
    return levelValid(c, f.p2()) && slotValid(c, f.p3()) && activation(c, f.s4());
}

bool slotFrom1SkipMinus1(const SlotCtx& c, const FrameCheck& f) noexcept {
    return levelValidMinus1(c, f.p2()) && slotValid(c, f.p3()) && activation(c, f.s4());
}

bool noSync(const SlotCtx& c, const FrameCheck& f, uint8_t& lead) noexcept {
    if (slotRegular(c, f)) { lead = LNone; return true; }
    if ((f.p1() & 1) != 0 && slotFrom1Skip(c, f)) { lead = LSynchronizeFail; return true; }
    if (f.p1() % 3 == 0 && slotFrom1Skip(c, f)) { lead = LCuteCharmFail; return true; }
    if ((f.p1() & 1) != 1 && slotFrom1SkipMinus1(c, f)) { lead = LPressureFail; return true; }
    if ((f.p3() & 1) != 0 && levelValid(c, f.p1()) && slotValid(c, f.p2()) && activation(c, f.s4())) {
        lead = LStaticMagnetFail;
        return true;
    }
    if ((f.p3() & 1) == 0 && levelValid(c, f.p1())) {
        const auto& s = *c.slot;
        uint8_t sm = LNone;
        if (s.staticCount != 0 && f.p2() % s.staticCount == s.staticIndex) sm = LStatic;
        else if (s.magnetCount != 0 && f.p2() % s.magnetCount == s.magnetIndex) sm = LMagnetPull;
        if (sm != LNone && activation(c, f.s4())) { lead = sm; return true; }
    }
    if ((f.p1() & 1) == 1 && pressureLevel(c) == c.met && slotValid(c, f.p3()) && activation(c, f.s4())) {
        lead = LPressure;
        return true;
    }
    if ((f.p1() & 1) != 1 && slotFrom1Skip(c, f)) { lead = LIntimidateFail; return true; }
    if (f.p1() % 3 != 0 && slotFrom1Skip(c, f)) { lead = LCuteCharm; return true; }
    return false;
}

uint32_t safariBlockSeed(uint32_t seed) noexcept { return 0xC048C851u * seed + 0x196302B4u; }
bool safariBlockProc(uint32_t seed) noexcept { return (seed >> 16) % 100 < 80; }

bool tryMatchNoLead(const SlotCtx& c, uint32_t seed, uint8_t nature) noexcept {
    const uint32_t p0 = seed >> 16;
    const bool safariHoenn = c.area->location == 57;
    if (safariHoenn) {
        const uint32_t sb = safariBlockSeed(seed);
        if (safariBlockProc(sb) && slotRegular(c, FrameCheck(sb))) return true;
    }
    if (c.slot->species == 201) return slotRegular(c, FrameCheck(lcNext(seed)));
    if (p0 % 25 == nature) {
        if (safariHoenn) seed = lcPrev(seed);
        if (slotRegular(c, FrameCheck(seed))) return true;
    }
    return false;
}

bool tryMatchEmerald(const SlotCtx& c, uint32_t seed, uint8_t nature, uint8_t& lead) noexcept {
    const uint32_t p0 = seed >> 16;
    if (c.area->location == 57) {
        const uint32_t sb = safariBlockSeed(seed);
        if (safariBlockProc(sb) && noSync(c, FrameCheck(sb), lead)) return true;
        seed = lcPrev(seed);
    }
    if ((p0 & 1) == 0 && slotRegular(c, FrameCheck(seed))) { lead = LSynchronize; return true; }
    if (p0 % 25 == nature && noSync(c, FrameCheck(seed), lead)) return true;
    return false;
}

/// MethodJ.GetReversalWindow.
bool reversalWindow(uint32_t seed, uint8_t nature, uint32_t& count) noexcept {
    uint32_t ctr = 0;
    uint32_t b = seed >> 16;
    while (ctr < kWindowCap) {
        const uint32_t a = lcPrev16(seed);
        const uint32_t pid = (b << 16) | a;
        if (pid % 25 == nature) { count = ctr; return true; }
        b = lcPrev16(seed);
        ++ctr;
    }
    return false;
}

bool reversalWindowCute(uint32_t seed, uint8_t nature, uint8_t gmin, uint8_t gmax, uint32_t& noLead, uint32_t& cute) noexcept {
    uint32_t nl = 0, ctr = 0;
    uint32_t b = seed >> 16;
    while (ctr < kWindowCap) {
        const uint32_t a = lcPrev16(seed);
        const uint32_t pid = (b << 16) | a;
        if (pid % 25 == nature) {
            if (nl == 0) nl = ctr;
            const uint32_t g = pid & 0xFFu;
            if (gmin <= g && g <= gmax) { noLead = nl; cute = ctr - nl; return true; }
        }
        b = lcPrev16(seed);
        ++ctr;
    }
    return false;
}

bool reversalWindowUnown(uint32_t seed, uint8_t form, uint32_t& count) noexcept {
    uint32_t ctr = 0;
    uint32_t b = seed >> 16;
    while (ctr < kWindowCap) {
        const uint32_t a = lcPrev16(seed);
        const uint32_t pid = (a << 16) | b;
        if (unownForm(pid) == form) { count = ctr; return true; }
        b = lcPrev16(seed);
        ++ctr;
    }
    return false;
}

/// MethodH.GetSeed: best lead that can produce this slot from the PID/IV origin seed.
/// Returns LInvalid when no frame matches; capped is set if a reversal window hit the guard.
uint8_t methodHLead(const SlotCtx& c, uint32_t seed, bool emerald, uint8_t gender, bool& capped) noexcept {
    const uint32_t r1 = lcNext(seed);
    const uint32_t r2 = lcNext(r1);
    const uint32_t pid = c.slot->species != 201 ? ((r2 >> 16) << 16) | (r1 >> 16)
                                                : ((r1 >> 16) << 16) | (r2 >> 16);
    const uint8_t nature = static_cast<uint8_t>(pid % 25);
    if (emerald) {
        uint32_t regular = 0, cute = 0;
        const uint8_t ratio = personal(c.slot->species).gender;
        if (ratio == 255) {
            if (!reversalWindow(seed, nature, regular)) { capped = true; return LInvalid; }
        } else {
            uint8_t gmin = 0, gmax = 255;
            if (ratio != 0 && ratio != 254) {
                if (gender == 0) gmin = ratio;
                else if (gender == 1) gmax = static_cast<uint8_t>(ratio - 1);
            }
            if (!reversalWindowCute(seed, nature, gmin, gmax, regular, cute)) { capped = true; return LInvalid; }
        }
        int reverseCount = static_cast<uint16_t>(regular);
        int revCute = static_cast<uint16_t>(cute);
        uint8_t prefer = LInvalid;
        bool toCute = false;
        while (true) {
            uint8_t lead = LInvalid;
            if (tryMatchEmerald(c, seed, nature, lead)) {
                if (lead == LNone) return LNone;
                if (lead > prefer) prefer = lead;
            }
            if (reverseCount == 0) {
                if (revCute == 0 || prefer != LInvalid) return prefer;
                toCute = true;
                break;
            }
            --reverseCount;
            seed = lcPrevN(seed, 2);
        }
        if (toCute) {
            while (true) {
                uint8_t lead = LInvalid;
                if (tryMatchEmerald(c, seed, nature, lead) && lead == LNone) {
                    if (LCuteCharm > prefer) prefer = LCuteCharm;
                }
                --revCute;
                if (revCute == 0) return prefer;
                seed = lcPrevN(seed, 2);
            }
        }
        return prefer;
    }
    uint32_t window = 0;
    const bool ok = c.slot->species == 201 ? reversalWindowUnown(seed, c.slot->form, window)
                                           : reversalWindow(seed, nature, window);
    if (!ok) { capped = true; return LInvalid; }
    int reverseCount = static_cast<uint16_t>(window);
    while (true) {
        if (tryMatchNoLead(c, seed, nature)) return LNone;
        if (reverseCount == 0) break;
        --reverseCount;
        seed = lcPrevN(seed, 2);
    }
    return LInvalid;
}

// ---------------------------------------------------------------------------
// EncounterGenerator3: PID/IV type filter, ball filter, lead queue and deferral
// ---------------------------------------------------------------------------

enum Compat : uint8_t { CMatch = 0, CNotIdeal = 1, CMismatch = 2 };

uint8_t giftGenderOf(uint8_t rule, uint32_t seed) noexcept {
    switch (rule) {
        case 1: case 8: return 0;
        case 2: case 9: return 1;
        case 4: return static_cast<uint8_t>(genderBit0(lcNext(seed) >> 16));
        case 5: return static_cast<uint8_t>(genderBit3(lcNext(seed) >> 16));
        case 6: return static_cast<uint8_t>(genderBit7(lcNext(seed) >> 16));
        case 7: return static_cast<uint8_t>(genderBit15(lcNextN(seed, 2) >> 16));
        default: return 0;
    }
}

bool giftIsMatchGender(uint8_t rule, uint8_t otGender, uint32_t seed) noexcept {
    const uint8_t expect = (rule == 8 || rule == 9)
        ? static_cast<uint8_t>(genderBit0(lcNextN(seed, 6) >> 16))
        : giftGenderOf(rule, lcNextN(seed, 4));
    return otGender == expect;
}

bool hasMove(const uint16_t (&moves)[4], uint16_t move) noexcept {
    for (uint16_t m : moves) if (m == move) return true;
    return false;
}

bool restrictedTableMatch(PidIv& v, uint16_t species, bool isWish, bool isShiny) noexcept {
    const uint32_t origin = lcPrevN(v.originSeed, 2);
    if (origin > 0xFFFFu) return false;
    if (species != 385) {
        const uint32_t index = ((static_cast<uint32_t>(species) >> 2) + 1) & 3u;
        const PcjpResult r = pcjpResult(static_cast<uint16_t>(origin));
        if (r.shiny != isShiny || r.index != index || r.wish != isWish) return false;
    }
    v.mutated = BACD_R;
    return true;
}

bool mystryMewMatch(PidIv& v, uint8_t type) noexcept {
    if (type != BACD) return false;
    uint32_t seed = v.originSeed;
    int index = -1, sub = -1;
    if (seed <= 0xFFFFu) {
        index = mystryMewSearch(static_cast<uint16_t>(seed));
        sub = 0;
    } else {
        uint32_t s = seed;
        for (int i = 1; i < 5; ++i) {
            s = lcPrevN(s, 5);
            if (s <= 0xFFFFu) {
                index = mystryMewSearch(static_cast<uint16_t>(s));
                sub = i;
                break;
            }
        }
    }
    if (index == -1) return false;
    if (sub != 0) {
        const uint32_t ui = static_cast<uint32_t>(index);
        seed = D::kMystryMewSeeds[ui >= D::kMystryMewSeedCount ? 0 : ui];
    }
    if (seed == 0x6065u && sub != 2) return false;
    v.mutated = BACD_R;
    return true;
}

uint8_t giftRevise(const Pk3& pk, const D::Gift& g, PidIv& v, bool& capped) noexcept {
    const uint8_t type = v.mutated != 0 ? v.mutated : v.type;
    if ((type == BACD_EA || type == BACD_ES) && !g.egg) return CMismatch;
    if (g.otGender != 0 && g.otGender != 3 && (!g.egg || pk.isEgg) &&
        !giftIsMatchGender(g.otGender, pk.otGender, v.originSeed))
        return CMismatch;
    const bool wish = hasMove(g.moves, D::kMoveWish);
    bool result = false;
    switch (g.method) {
        case BACD_U: result = type == BACD; break;
        case BACD_R:
            if (type == BACD && v.originSeed <= 0xFFFFu) { v.mutated = BACD_R; result = true; }
            break;
        case BACD_R_A:
            if ((type == BACD || type == BACD_A || type == BACD_EA) && v.originSeed <= 0xFFFFu) {
                v.mutated = BACD_R;
                result = true;
            }
            break;
        case BACD_U_AX:
            if (type == BACD_AX) { v.mutated = BACD_AX; result = true; }
            break;
        case BACD_TA:
            result = (type == BACD || type == BACD_A || type == BACD_EA) && restrictedTableMatch(v, g.species, wish, false);
            break;
        case BACD_TS:
            result = (type == BACD_S || type == BACD_ES) && restrictedTableMatch(v, g.species, wish, true);
            break;
        case BACD_RBCD:
            if (type == BACD_S && v.originSeed >= 3 && v.originSeed <= 213) { v.mutated = BACD_R; result = true; }
            break;
        case BACD_M: result = mystryMewMatch(v, type); break;
        case ChannelT:
            if (type == ChannelT) {
                uint32_t origin = 0;
                if (channelJirachiPossible(v.originSeed, origin, capped)) { v.mutated = ChannelT; result = true; }
            }
            break;
        case M2: result = type == M2 || type == M1 || type == M4; break;
        default: break;
    }
    return result ? CMatch : CMismatch;
}

uint8_t typeCompatible(const Pk3& pk, const Enc& e, PidIv& v, bool& capped) noexcept {
    switch (e.kind) {
        case Kind::Gift: return giftRevise(pk, D::kGifts[e.index], v, capped);
        case Kind::GiftNY: {
            const uint8_t type = v.mutated != 0 ? v.mutated : v.type;
            return type == BACD_AX ? CMatch : CMismatch;
        }
        case Kind::GiftJPN: {
            const uint8_t type = v.mutated != 0 ? v.mutated : v.type;
            if (type != BACD && type != BACD_R) return CMismatch;
            const uint32_t rand5 = lcNextN(v.originSeed, 5) >> 16;
            return pk.otGender == genderBit7(rand5) ? CMatch : CMismatch;
        }
        case Kind::Slot:
            if (e.species == 201) return (v.type >= M1U && v.type <= M4U) ? CMatch : CMismatch;
            return (v.type >= M1 && v.type <= M4) ? CMatch : CMismatch;
        case Kind::Static: {
            const auto& s = D::kStatics[e.index];
            if (pk.version == kVerE) return v.type == M1 ? CMatch : CMismatch;
            if (s.roaming) {
                if (v.type == M1Roamer) return CMatch;
                if (v.type != M1) return CMismatch;
                return (pk.iv32 & 0x3FFFFFFFu) <= 0xFFu ? CMatch : CMismatch;
            }
            if (v.type == M1) return CMatch;
            if (pk.version == kVerR || pk.version == kVerS) return v.type == M4 ? CNotIdeal : CMismatch;
            return v.type == M4 && s.species == 175 ? CNotIdeal : CMismatch;
        }
        case Kind::Colo: return (v.type == CXD || v.type == CXDAnti) ? CMatch : CMismatch;
        case Kind::XdTrade: return v.type == CXD ? CMatch : CMismatch;                          // EncounterTrade3XD
        case Kind::ColoShadow:                                                                   // EncounterShadow3Colo
            if (e.eReader) return CMatch; // fixed IVs: no PID/IV correlation
            return v.type == CXD ? CMatch : CMismatch;
        case Kind::ColoStarter: return v.type == CXD_ColoStarter ? CMatch : CMismatch;          // EncounterStarter3Colo
        case Kind::ColoGiftCxd: case Kind::XdShadow:                                             // Gift3Colo, Shadow3XD
            return (v.type == CXD || v.type == CXDAnti) ? CMatch : CMismatch;
        case Kind::XdStatic:                                                                     // EncounterStatic3XD
            if (v.type == CXD || v.type == CXDAnti) return CMatch;
            return (v.type == CXD_ColoStarter && pk.species == 197) ? CMatch : CMismatch;
        case Kind::XdSlot: return v.type == PokeSpot ? CMatch : CMismatch;                      // EncounterSlot3XD
        default: return v.type == None ? CMatch : CMismatch; // Trade3 (no correlation), Egg3
    }
}

bool ballCompatible(const Enc& e, const Pk3& pk) noexcept {
    if (e.fixedBall == kBallSafari && pk.ball == kBallSafari) return true;
    return pk.ball != kBallSafari;
}

enum DeferType : uint8_t { DNone = 0, DPidivDefer = 1, DSlotNumber = 2, DBall = 3, DPidiv = 4 };
enum YieldFlag : uint8_t { YNone = 0, YInvalidPidiv = 1, YInvalidFrame = 2 };

struct Generated {
    std::vector<Enc> list;
    int lastResort = -1;     // index of the deferred last resort in list, or -1
    uint8_t lastResortFlag = YNone;
    bool capped = false;     // a search guard was hit; the result is Unknown
};

Generated generateEncounters(const Pk3& pk, const std::vector<Evo>& chain, PidIv pidiv) {
    Generated g;
    if (chain.empty()) return g;
    const std::vector<Enc> candidates = enumerateEncounters(pk, chain);
    const bool emerald = pk.version == kVerE;
    uint8_t gender = pk.gender;
    if (pk.species == 183 || pk.species == 184) gender = genderFromRatio(pk.pid, 63);

    uint8_t deferType = DNone;
    bool hasDefer = false;
    Enc defer;
    auto update = [&](uint8_t type, const Enc& e) {
        if (type >= deferType && hasDefer) return;
        deferType = type;
        defer = e;
        hasDefer = true;
    };
    struct Queued { uint8_t lead; Enc enc; };
    std::vector<Queued> queue;

    for (const Enc& e : candidates) {
        const uint8_t compat = typeCompatible(pk, e, pidiv, g.capped);
        if (compat != CMatch) {
            update(compat == CNotIdeal ? DPidivDefer : DPidiv, e);
            continue;
        }
        if (!ballCompatible(e, pk)) {
            update(DBall, e);
            continue;
        }
        if (e.kind != Kind::Slot) {
            g.list.push_back(e);
            continue;
        }
        uint8_t lead = LInvalid;
        if (isClassicMethod(pidiv.type)) {
            SlotCtx ctx;
            ctx.slot = &D::kSlots[e.index];
            ctx.area = &D::kAreas[e.area];
            ctx.met = pk.metLevel;
            lead = methodHLead(ctx, pidiv.originSeed, emerald, gender, g.capped);
        }
        if (lead == LInvalid) {
            update(DSlotNumber, e);
            continue;
        }
        size_t pos = queue.size();
        for (size_t i = 0; i < queue.size(); ++i) {
            if (lead > queue[i].lead) { pos = i; break; }
        }
        queue.insert(queue.begin() + static_cast<std::ptrdiff_t>(pos), Queued{lead, e});
    }
    for (const auto& q : queue) g.list.push_back(q.enc);
    if (hasDefer) {
        g.lastResort = static_cast<int>(g.list.size());
        g.list.push_back(defer);
        g.lastResortFlag = deferType == DPidiv ? YInvalidPidiv : deferType == DSlotNumber ? YInvalidFrame : YNone;
    }
    return g;
}

/// EncounterGenerator3GC.GetIsShadowLockValidEReader: e-Reader shadows have fixed IVs, so
/// the origin is searched from the PID alone; a valid lock sets the PIDIV to (CXD, origin).
bool eReaderLockValid(const Pk3& pk, const Enc& e, PidIv& pidiv, bool& capped) {
    for (uint32_t seed : xdSeeds(pk.pid & 0xFFFF0000u, pk.pid << 16)) {
        const uint32_t origin = xdNextN(seed, 4);
        if (!shadowLockValid(pk, false, shadowOf(e).teamSet, origin, capped)) continue;
        pidiv = makePidIv(CXD, origin);
        return true;
    }
    return false;
}

/// EncounterGenerator3GC.GetEncounters: the PIDIV is refined per template (Poke Spot seeds,
/// e-Reader origin), shadow locks are verified before the type check, and the first
/// candidate that failed either check is the InvalidPIDIV last resort.
Generated generateEncountersGC(const Pk3& pk, const std::vector<Evo>& chain, PidIv& pidiv) {
    Generated g;
    if (chain.empty()) return g;
    bool hasPartial = false;
    Enc partial;
    for (const Enc& e : enumerateEncountersGC(pk, chain)) {
        if (e.kind == Kind::XdSlot) {
            const auto& slot = X::kPokeSpots[e.area].slots[e.index];
            uint32_t pidOrigin = 0;
            if (pokeSpotOriginPid(pk.pid, static_cast<uint8_t>(e.index), pidOrigin) &&
                pokeSpotOriginIVs(pk, slot.levelMin, slot.levelMax)) {
                pidiv = makePidIv(PokeSpot, pidOrigin);
                pidiv.mutated = PokeSpot;
            }
        } else if (isShadowKind(e.kind)) {
            const bool valid = e.eReader
                ? eReaderLockValid(pk, e, pidiv, g.capped)
                : shadowLockValid(pk, e.kind == Kind::XdShadow, shadowOf(e).teamSet, pidiv.originSeed, g.capped);
            if (!valid) {
                if (!hasPartial) { partial = e; hasPartial = true; }
                continue;
            }
        }
        if (typeCompatible(pk, e, pidiv, g.capped) == CMatch) {
            g.list.push_back(e);
        } else if (!hasPartial) {
            partial = e;
            hasPartial = true;
        }
    }
    if (hasPartial) {
        g.lastResort = static_cast<int>(g.list.size());
        g.list.push_back(partial);
        g.lastResortFlag = YInvalidPidiv;
    }
    return g;
}

} // namespace

namespace {

// ---------------------------------------------------------------------------
// Species-name dictionaries (SpeciesName.TryGetSpeciesAnyLanguage[CaseInsensitive])
// ---------------------------------------------------------------------------

constexpr uint8_t kGen3Languages[] = {1, 2, 3, 4, 5, 7};
constexpr uint8_t kGen7Languages[] = {1, 2, 3, 4, 5, 7, 8, 9, 10};

bool dictLookup(const D::NameKey* keys, uint16_t count, std::u16string_view name, uint16_t& species) noexcept {
    if (keys == nullptr || count == 0) return false;
    const D::NameKey* end = keys + count;
    const D::NameKey* it = std::lower_bound(keys, end, name, [](const D::NameKey& k, std::u16string_view v) {
        return std::u16string_view(k.key) < v;
    });
    if (it == end || std::u16string_view(it->key) != name) return false;
    species = it->species;
    return true;
}

/// SpeciesName.TryGetSpeciesAnyLanguage(name, out species, EntityContext.Gen3): exact match.
bool speciesAnyLanguage3(std::u16string_view name, uint16_t& species) noexcept {
    for (uint8_t lang : kGen3Languages)
        if (dictLookup(D::kNameDict[lang], D::kNameDictCount[lang], name, species)) return true;
    return false;
}

/// SpeciesName.TryGetSpeciesAnyLanguageCaseInsensitive over the given languages.
template <size_t N>
bool speciesAnyLanguageLower(std::u16string_view name, const uint8_t (&langs)[N], uint16_t& species) {
    const std::u16string lower = lowerInvariant(name);
    for (uint8_t lang : langs)
        if (dictLookup(D::kNameDictLower[lang], D::kNameDictLowerCount[lang], lower, species)) return true;
    return false;
}

/// SpeciesName.IsNicknamedAnyLanguage(species, name, EntityContext.Gen3).
bool isNicknamedAnyLanguage3(uint16_t species, std::u16string_view name) noexcept {
    for (uint8_t lang : kGen3Languages)
        if (name == std::u16string_view(speciesName3(species, lang))) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Word filter (WordFilter.IsFiltered look-back for a Gen III name: Switch, then 3DS)
// ---------------------------------------------------------------------------

/// TextNormalizer.Normalize.
std::u16string normalizeText(std::u16string_view input) {
    static constexpr std::u16string_view kDakuten = u"ｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾊﾋﾌﾍﾎ";
    static constexpr std::u16string_view kHandakuten = u"ﾊﾋﾌﾍﾎ";
    static constexpr std::u16string_view kFullwidthKana =
        u"ヲァィゥェォャュョッーアイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワン";
    static constexpr std::u16string_view kSmallKana = u"ァィゥェォッャュョヮ";
    std::u16string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        char16_t c = input[i];
        if (c == u' ' || c == u'　' || c == u'ﾞ' || c == u'ﾟ') continue;
        uint16_t ofs = 0;
        if (c >= u'ｦ' && c <= u'ﾝ' && i + 1 < input.size()) {
            const char16_t d = input[i + 1];
            if (d == u'ﾞ' && kDakuten.find(c) != std::u16string_view::npos) ofs = 1;
            else if (d == u'ﾟ' && kHandakuten.find(c) != std::u16string_view::npos) ofs = 2;
            else if (d == u'ﾞ' && c == u'ｳ') ofs = static_cast<uint16_t>(u'ヴ' - u'ウ');
        }
        c = toLowerInvariant(c);
        if (c >= u'ぁ' && c <= u'ゖ') c = static_cast<char16_t>(c + 0x60);
        else if ((c >= u'０' && c <= u'９') || (c >= u'ａ' && c <= u'ｚ')) c = static_cast<char16_t>(c - 0xFEE0);
        else if (c >= u'ｦ' && c <= u'ﾝ') c = static_cast<char16_t>(kFullwidthKana[static_cast<size_t>(c - u'ｦ')] + ofs);
        if (c >= u'ァ' && c <= u'ヶ') {
            if (kSmallKana.find(c) != std::u16string_view::npos) c = static_cast<char16_t>(c + 1);
            else if (c == u'ヵ') c = u'カ';
            else if (c == u'ヶ') c = u'ケ';
        }
        out.push_back(c);
    }
    return out;
}

/// The regular-expression subset used by badwords_switch / badwords_3ds:
/// literals, '.', '\x' escapes, [set] / [^set], (alt|alt) literal groups,
/// '*' and '?' quantifiers, '^' at the start and '$' at the end.
struct RxAtom {
    enum Type : uint8_t { Lit, Any, Set, NotSet, Group } type = Lit;
    char16_t c = 0;
    std::u16string set;              // Set / NotSet members (ranges expanded)
    std::vector<std::u16string> alts; // Group alternatives
    uint8_t quant = 0;               // 0 once, 1 '*', 2 '?'
};

struct RxPattern {
    enum Fast : uint8_t { General, Contains, Exact } fast = General;
    bool anchorStart = false, anchorEnd = false, supported = true;
    std::u16string literal;
    std::vector<RxAtom> atoms;
};

RxPattern parsePattern(std::u16string_view p) {
    RxPattern r;
    size_t i = 0, n = p.size();
    if (n > 0 && p[0] == u'^') { r.anchorStart = true; i = 1; }
    if (n > i && p[n - 1] == u'$' && !(n >= 2 && p[n - 2] == u'\\')) { r.anchorEnd = true; --n; }
    while (i < n) {
        RxAtom a;
        const char16_t c = p[i];
        if (c == u'\\') {
            if (i + 1 >= n) { r.supported = false; break; }
            a.type = RxAtom::Lit; a.c = p[i + 1]; i += 2;
        } else if (c == u'.') {
            a.type = RxAtom::Any; ++i;
        } else if (c == u'[') {
            size_t j = i + 1;
            if (j < n && p[j] == u'^') { a.type = RxAtom::NotSet; ++j; } else { a.type = RxAtom::Set; }
            bool closed = false;
            for (; j < n; ++j) {
                if (p[j] == u']') { closed = true; break; }
                if (p[j] == u'\\' || p[j] == u'[') { r.supported = false; break; }
                if (j + 2 < n && p[j + 1] == u'-' && p[j + 2] != u']') {
                    for (char16_t x = p[j]; x <= p[j + 2]; ++x) { a.set.push_back(x); if (x == 0xFFFF) break; }
                    j += 2;
                } else {
                    a.set.push_back(p[j]);
                }
            }
            if (!closed || !r.supported) { r.supported = false; break; }
            i = j + 1;
        } else if (c == u'(') {
            a.type = RxAtom::Group;
            size_t j = i + 1;
            std::u16string cur;
            bool closed = false;
            for (; j < n; ++j) {
                const char16_t x = p[j];
                if (x == u')') { closed = true; break; }
                if (x == u'|') { a.alts.push_back(cur); cur.clear(); continue; }
                if (x == u'.' || x == u'*' || x == u'?' || x == u'[' || x == u'(' || x == u'\\' || x == u'+') {
                    r.supported = false;
                    break;
                }
                cur.push_back(x);
            }
            if (!closed || !r.supported) { r.supported = false; break; }
            a.alts.push_back(cur);
            i = j + 1;
        } else if (c == u'*' || c == u'?' || c == u'+' || c == u'{' || c == u'}' || c == u')' || c == u']' ||
                   c == u'|' || c == u'^' || c == u'$') {
            r.supported = false;
            break;
        } else {
            a.type = RxAtom::Lit; a.c = c; ++i;
        }
        if (i < n && (p[i] == u'*' || p[i] == u'?')) { a.quant = p[i] == u'*' ? 1 : 2; ++i; }
        if (i < n && (p[i] == u'*' || p[i] == u'?' || p[i] == u'+' || p[i] == u'{')) { r.supported = false; break; }
        r.atoms.push_back(std::move(a));
    }
    if (!r.supported) return r;
    // Fast paths: ".*literal.*" (unanchored) and "^literal$".
    auto allLiteral = [&](size_t from, size_t to) {
        for (size_t k = from; k < to; ++k)
            if (r.atoms[k].type != RxAtom::Lit || r.atoms[k].quant != 0) return false;
        return true;
    };
    auto isDotStar = [&](size_t k) { return r.atoms[k].type == RxAtom::Any && r.atoms[k].quant == 1; };
    const size_t m = r.atoms.size();
    if (!r.anchorStart && !r.anchorEnd && m >= 2 && isDotStar(0) && isDotStar(m - 1) && allLiteral(1, m - 1)) {
        r.fast = RxPattern::Contains;
        for (size_t k = 1; k + 1 < m; ++k) r.literal.push_back(r.atoms[k].c);
    } else if (r.anchorStart && r.anchorEnd && allLiteral(0, m)) {
        r.fast = RxPattern::Exact;
        for (const auto& a : r.atoms) r.literal.push_back(a.c);
    }
    return r;
}

bool atomMatchesChar(const RxAtom& a, char16_t c) noexcept {
    switch (a.type) {
        case RxAtom::Lit: return c == a.c;
        case RxAtom::Any: return c != u'\n';
        case RxAtom::Set: return a.set.find(c) != std::u16string::npos;
        case RxAtom::NotSet: return a.set.find(c) == std::u16string::npos;
        default: return false;
    }
}

bool rxEndOk(const RxPattern& p, std::u16string_view s, size_t pos) noexcept {
    if (!p.anchorEnd) return true;
    return pos == s.size() || (pos + 1 == s.size() && s[pos] == u'\n');
}

bool rxMatchHere(const RxPattern& p, size_t ai, std::u16string_view s, size_t pos, int depth) noexcept {
    if (depth > 4096) return false;
    if (ai == p.atoms.size()) return rxEndOk(p, s, pos);
    const RxAtom& a = p.atoms[ai];
    if (a.type == RxAtom::Group) {
        // Each alternative once, repeated for '*' (backtracking finds a match if one exists).
        for (const auto& alt : a.alts) {
            if (s.substr(pos, alt.size()) != std::u16string_view(alt)) continue;
            const size_t next = pos + alt.size();
            if (a.quant == 1 && !alt.empty() && rxMatchHere(p, ai, s, next, depth + 1)) return true;
            if (rxMatchHere(p, ai + 1, s, next, depth + 1)) return true;
        }
        if (a.quant != 0) return rxMatchHere(p, ai + 1, s, pos, depth + 1);
        return false;
    }
    if (a.quant == 1) {
        size_t k = pos;
        while (k < s.size() && atomMatchesChar(a, s[k])) ++k;
        for (size_t end = k + 1; end-- > pos;)
            if (rxMatchHere(p, ai + 1, s, end, depth + 1)) return true;
        return false;
    }
    if (a.quant == 2) {
        if (pos < s.size() && atomMatchesChar(a, s[pos]) && rxMatchHere(p, ai + 1, s, pos + 1, depth + 1)) return true;
        return rxMatchHere(p, ai + 1, s, pos, depth + 1);
    }
    if (pos < s.size() && atomMatchesChar(a, s[pos])) return rxMatchHere(p, ai + 1, s, pos + 1, depth + 1);
    return false;
}

bool rxSearch(const RxPattern& p, std::u16string_view s) noexcept {
    switch (p.fast) {
        case RxPattern::Contains: // ".*X.*" with empty ".*" on either side
            return s.find(p.literal) != std::u16string_view::npos;
        case RxPattern::Exact:
            return s == std::u16string_view(p.literal) ||
                   (s.size() == p.literal.size() + 1 && s.back() == u'\n' &&
                    s.substr(0, p.literal.size()) == std::u16string_view(p.literal));
        default: break;
    }
    if (p.anchorStart) return rxMatchHere(p, 0, s, 0, 0);
    for (size_t start = 0; start <= s.size(); ++start)
        if (rxMatchHere(p, 0, s, start, 0)) return true;
    return false;
}

struct FilterSet {
    std::vector<RxPattern> patterns;
    size_t unsupported = 0;
};

FilterSet buildFilter(const char16_t* const* list, size_t count) {
    FilterSet f;
    f.patterns.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        RxPattern p = parsePattern(list[i]);
        if (!p.supported) ++f.unsupported;
        f.patterns.push_back(std::move(p));
    }
    return f;
}

const FilterSet& filterSwitch() {
    static const FilterSet f = buildFilter(D::kBadwordsSwitch, D::kBadwordSwitchCount);
    return f;
}

const FilterSet& filter3DS() {
    static const FilterSet f = buildFilter(D::kBadwords3DS, D::kBadword3DSCount);
    return f;
}

/// WordFilter.TryMatch: returns true and sets unsupported if a pattern could not be evaluated.
bool filterMatch(const FilterSet& f, std::u16string_view clean, bool& unsupported) {
    for (const auto& p : f.patterns) {
        if (!p.supported) { unsupported = true; continue; }
        if (rxSearch(p, clean)) return true;
    }
    return false;
}

/// WordFilter.IsFiltered(message, Gen3, Gen3) with the default look-back settings.
/// unsupported is set when a filter pattern could not be evaluated (never for the pinned lists).
bool isWordFiltered3(std::u16string_view message, bool& unsupported) {
    if (allWhiteSpace(message) || message.size() <= 1) return false;
    const std::u16string clean = normalizeText(message);
    uint16_t species = 0;
    // WordFilterNX.IsFiltered(message, out, Gen3)
    const bool speciesNX = speciesAnyLanguageLower(message, kGen3Languages, species) && species <= D::kMaxSpecies;
    if (!speciesNX && filterMatch(filterSwitch(), clean, unsupported)) return true;
    // WordFilter3DS.IsFiltered(message, out, Gen3) -> IsSpeciesNameGen7
    const bool species3DS = speciesAnyLanguageLower(message, kGen7Languages, species) && species <= D::kMaxSpecies7;
    if (!species3DS && filterMatch(filter3DS(), clean, unsupported)) return true;
    return false;
}

} // namespace

bool isWordFiltered(std::u16string_view message) {
    bool unsupported = false;
    return isWordFiltered3(message, unsupported);
}

size_t unsupportedWordFilterPatterns() {
    return filterSwitch().unsupported + filter3DS().unsupported;
}

namespace {

// ---------------------------------------------------------------------------
// Findings (PKHeX CheckResult with Severity Valid / Fishy / Invalid)
// ---------------------------------------------------------------------------

enum Sev : uint8_t { SInfo = 0, SFishy = 1, SInvalid = 2 };

struct Finding {
    uint8_t sev = SInvalid;
    CheckIdentifier id = CheckIdentifier::Misc;
    std::string text;
};

struct Analysis {
    Enc enc;                    // EncounterOriginal == EncounterMatch for a PK3
    std::vector<Evo> history;   // EvoChainsAllGens.Gen3
    MoveResults moves{};
    PidIv pidiv;
    bool capped = false;        // a reversal guard was hit: the result is Unknown
    bool unsupported = false;   // a filter pattern could not be evaluated: Unknown
    std::vector<Finding> findings;

    void add(uint8_t sev, CheckIdentifier id, std::string text) {
        findings.push_back(Finding{sev, id, std::move(text)});
    }
    void invalid(CheckIdentifier id, std::string text) { add(SInvalid, id, std::move(text)); }
    void fishy(CheckIdentifier id, std::string text) { add(SFishy, id, std::move(text)); }
    void note(CheckIdentifier id, std::string text) { add(SInfo, id, std::move(text)); }
};

std::string speciesLabel(uint16_t species) {
    const char* n = Names::getSpeciesName(species);
    if (n != nullptr && *n != '\0') return n;
    return "species #" + std::to_string(species);
}

std::string moveLabel(uint16_t move) {
    const char* n = Names::getMoveName(move);
    if (n != nullptr && *n != '\0') return n;
    return "move #" + std::to_string(move);
}

std::string encLabel(const Enc& e) { return std::string(kindName(e.kind)) + " (" + speciesLabel(e.species) + ")"; }

bool historyHas(const std::vector<Evo>& history, uint16_t species) noexcept {
    for (const auto& evo : history)
        if (evo.species == species) return true;
    return false;
}

// ---------------------------------------------------------------------------
// EncounterVerifier / EvolutionVerifier / EncounterFinder
// ---------------------------------------------------------------------------

struct EncCheck { bool valid = true; std::string text; };

EncCheck verifyEncounter3(const Pk3& pk, const Enc& e) {
    auto bad = [](std::string text) { return EncCheck{false, std::move(text)}; };
    if (e.kind == Kind::ColoShadow && e.eReader && pk.language != 1) // EncounterVerifier G3EReader
        return bad("Colosseum e-Reader shadow Pokemon were only distributed to Japanese games");
    if (e.kind == Kind::Static) {
        const auto& s = D::kStatics[e.index];
        if (s.species == 151 && pk.language != 1)
            return bad("Mew from Faraway Island was only distributed to Japanese Emerald");
        if (s.species == 386 && s.location == 200 && pk.language == 1)
            return bad("Deoxys from Birth Island was never distributed to Japanese FireRed/LeafGreen");
    }
    if (e.kind == Kind::Egg && pk.isEgg) {
        if (pk.metLevel != 0) return bad("Unhatched egg must have met level 0");
        if (pk.metLocation != hatchLocation3(pk.version)) return bad("Unhatched egg has an impossible met location");
        return {};
    }
    if (e.isEgg && !pk.isEgg) {
        if (pk.metLevel != 0) return bad("Hatched Pokemon must have met level 0");
        const uint8_t inaccessible = pk.language == 1 ? 200 : 201;
        if (pk.metLocation == inaccessible)
            return bad("Egg hatch location " + std::to_string(pk.metLocation) + " is inaccessible for this language");
        if (isValidMet3(pk.metLocation, pk.version)) return {};
        if (isValidMet3Any(pk.metLocation)) return {};
        return bad("Egg hatch location " + std::to_string(pk.metLocation) + " is not reachable in Gen III");
    }
    if (e.kind == Kind::Invalid) return bad("No Gen III encounter matches this Pokemon");
    return {};
}

bool verifyEvolution(const Pk3& pk, const Enc& e, const std::vector<Evo>& history) noexcept {
    if (pk.species == e.species) return true;
    return historyHas(history, pk.species) && historyHas(history, e.species);
}

/// EncounterFinder.FindVerifiedEncounter over the eagerly generated candidate list.
void findEncounter(const Pk3& pk, Analysis& a) {
    const std::vector<Evo> chain = originChain(pk);
    a.pidiv = analyzePidIv(pk, a.capped);
    const Generated g = pk.version == kVerCXD ? generateEncountersGC(pk, chain, a.pidiv)
                                              : generateEncounters(pk, chain, a.pidiv);
    a.capped = a.capped || g.capped;
    const size_t n = g.list.size();
    if (n == 0) {
        a.enc = makeInvalidEnc(pk);
        std::string hint;
        if (!pk.fateful && pk.isEgg && pk.metLocation == 253) hint = "Unable to match a gift egg encounter";
        else if (pk.isEgg && pk.metLocation == kLocEvent) hint = "Unable to match an event egg encounter";
        else if (pk.fateful || pk.metLocation == kLocEvent) hint = "Unable to match an event gift encounter";
        else hint = "Unable to match an encounter from the origin game";
        a.invalid(CheckIdentifier::Encounter, std::move(hint));
        a.history = evolutionHistory(pk, pk.species, a.enc.levelMin, true);
        a.moves = verifyMoves(pk, a.enc, a.history);
        return;
    }

    // The generator is lazy in PKHeX; track how far the finder pulled it, because the
    // deferred last resort only raises its yield flag once it has been fetched.
    size_t fetched = 0;
    auto peekIsNext = [&](size_t i) {
        if (i + 1 >= n) return false;
        fetched = std::max(fetched, i + 1);
        return true;
    };
    std::vector<Finding> parse;
    for (size_t i = 0; i < n; ++i) {
        fetched = std::max(fetched, i);
        const Enc& e = g.list[i];
        const EncCheck check = verifyEncounter3(pk, e);
        if (!check.valid && peekIsNext(i)) continue;

        a.enc = e;
        parse.clear();
        if (!check.valid) parse.push_back(Finding{SInvalid, CheckIdentifier::Encounter, check.text});

        a.history = evolutionHistory(pk, e.species, e.levelMin, e.kind == Kind::Invalid);
        a.moves = verifyMoves(pk, e, a.history);
        if (!allMovesValid(a.moves) && peekIsNext(i)) continue;
        if (!parse.empty() && peekIsNext(i)) continue;

        const bool evoValid = verifyEvolution(pk, e, a.history);
        if (!evoValid && peekIsNext(i)) continue;
        if (!evoValid)
            parse.push_back(Finding{SInvalid, CheckIdentifier::Species,
                                    speciesLabel(pk.species) + " cannot have evolved from " + encLabel(e)});

        if (e.kind == Kind::Egg) break; // EncounterEgg3 is not an IEncounterMatch
        if (matchRating(pk, e) < RPartialMatch) break;
        if (peekIsNext(i)) continue;
        parse.push_back(Finding{SInvalid, CheckIdentifier::Encounter,
                                "Encounter " + encLabel(e) + " is only a partial match"});
        break;
    }
    for (auto& f : parse) a.findings.push_back(std::move(f));

    if (g.lastResort >= 0 && fetched >= static_cast<size_t>(g.lastResort)) {
        if (g.lastResortFlag == YInvalidFrame)
            a.fishy(CheckIdentifier::PidRng, "No RNG frame produces this wild slot for " + encLabel(a.enc));
        else if (g.lastResortFlag == YInvalidPidiv)
            a.invalid(CheckIdentifier::PidRng,
                      std::string("PID/IV correlation (") + pidTypeName(a.pidiv.type) + ") does not match " + encLabel(a.enc));
    }
}

// ---------------------------------------------------------------------------
// Template properties used by the verifiers
// ---------------------------------------------------------------------------

/// Shiny rule: 0 random, 1 never, 2 always.
uint8_t shinyRule(const Enc& e) noexcept {
    switch (e.kind) {
        case Kind::Gift: return D::kGifts[e.index].shiny;
        case Kind::GiftNY: case Kind::GiftJPN: case Kind::Colo: return 1;
        case Kind::ColoStarter: case Kind::ColoGiftCxd: case Kind::XdShadow: return 1; // Shiny.Never
        default: return 0;
    }
}

bool giftHasFixedTrainer(const D::Gift& g) noexcept {
    return g.ot != nullptr && !allWhiteSpace(std::u16string_view(g.ot));
}

/// IFixedTrainer { IsFixedTrainer: true }.
bool isFixedTrainer(const Enc& e) noexcept {
    switch (e.kind) {
        case Kind::Trade: case Kind::GiftNY: case Kind::GiftJPN: case Kind::Colo: return true;
        case Kind::XdTrade: case Kind::ColoGiftCxd: return true; // EncounterTrade3XD, EncounterGift3Colo
        case Kind::Gift: return giftHasFixedTrainer(D::kGifts[e.index]);
        default: return false;
    }
}

/// TrainerNameVerifier.IsPlayerOriginalTrainer.
bool isPlayerOriginalTrainer(const Enc& e) noexcept { return !isFixedTrainer(e); }

uint8_t tradeLanguage(const Pk3& pk, const Enc& e) noexcept {
    uint8_t lang = pk.language;
    if (e.species == 124 && pk.version == kVerLG && lang == 4) lang = 2; // Jynx: Italian LG uses English text
    return lang;
}

std::u16string_view tradeString(const D::Trade& t, uint8_t lang, bool trainer) noexcept {
    if (lang == 0 || lang >= 8 || t.names > 1) return {};
    const auto* list = D::kTradeNames[t.names][lang];
    const size_t count = D::kTradeNameCounts[t.names][lang];
    const size_t index = trainer ? t.nameIndex + (D::kTradeNameCounts[t.names][1] >> 1) : t.nameIndex;
    if (list == nullptr || index >= count || list[index] == nullptr) return {};
    return list[index];
}

/// EncounterGift3Colo.IsTrainerMatch: the GameCube name is cut to the GBA length (5 JPN, 7 INT).
bool coloTrainerMatch(const Pk3& pk, const char16_t* const* names) noexcept {
    if (pk.language >= 8 || names[pk.language] == nullptr) return false;
    std::u16string_view expect(names[pk.language]);
    const size_t max = pk.language == 1 ? 5 : 7;
    if (expect.size() > max) expect = expect.substr(0, max);
    return std::u16string_view(pk.ot) == expect;
}

bool coloTrainerMatch(const Pk3& pk, const D::ColoGift& c) noexcept { return coloTrainerMatch(pk, c.names); }

/// EncounterTrade3XD.IsTrainerMatch / IsNicknameMatch in a Gen III context: exact per-language
/// name; language 0 never matches.
bool xdFixedNameMatch(uint8_t language, const char16_t* const* names, std::u16string_view actual) noexcept {
    if (language == 0 || language >= 8 || names[language] == nullptr) return false;
    return actual == std::u16string_view(names[language]);
}

bool withinEncounterRange(const Pk3& pk, const Enc& e) noexcept {
    const uint8_t level = pk.currentLevel;
    if (e.isEgg) return level == e.levelMin;
    if (e.kind == Kind::Gift) return e.levelMin <= level && level <= e.levelMax; // IMetLevel
    if (pk.metLevel != 0) return level == pk.metLevel;
    return e.levelMin <= level && level <= e.levelMax;
}

// ---------------------------------------------------------------------------
// Verifiers (LegalityAnalysis.UpdateChecks order)
// ---------------------------------------------------------------------------

void verifyPid(const Pk3& pk, Analysis& a) {
    const Enc& e = a.enc;
    if (e.species == 265 && pk.species != 265 && !wurmpleEvoValid(pk))
        a.invalid(CheckIdentifier::PidRng, "PID does not allow this Wurmple evolution");
    if (pk.pid == 0) a.fishy(CheckIdentifier::PidRng, "PID is zero");
    if (e.kind == Kind::Egg) {
        const bool procValid = e.eggVersion == kVerE ? pk.pid != 0 : (pk.pid & 0xFFFFu) != 0;
        if (!procValid) a.invalid(CheckIdentifier::PidRng, "Day Care never generates this egg PID");
        if (e.species == 29 || e.species == 32 || e.species == 313 || e.species == 314) {
            const uint8_t bit = static_cast<uint8_t>((pk.pid & 0x8000u) >> 15);
            if (pk.gender == bit) {
                bool inherited = false;
                for (uint8_t c : a.moves) if (c == MInheritLevelUp) inherited = true;
                if (pk.gender == 1 || inherited)
                    a.invalid(CheckIdentifier::PidRng, "PID bit 15 does not match the bred species gender");
            }
        }
    }
    const uint8_t rule = shinyRule(e);
    if (rule == 1 && pk.shiny) a.invalid(CheckIdentifier::PidRng, "This encounter can never be shiny");
    if (rule == 2 && !pk.shiny) a.invalid(CheckIdentifier::PidRng, "This encounter is always shiny");
}

void verifyLanguage(const Pk3& pk, Analysis& a) {
    if (pk.language == 0 || pk.language == 6 || pk.language > 7)
        a.invalid(CheckIdentifier::Trainer, "Language ID " + std::to_string(pk.language) + " is not a Gen III language");
}

void verifyWordFilter(Analysis& a, std::u16string_view name, const char* what) {
    bool unsupported = false;
    if (isWordFiltered3(name, unsupported))
        a.invalid(CheckIdentifier::Trainer, std::string(what) + " is blocked by the Nintendo word filter");
    if (unsupported) a.unsupported = true;
}

void verifyNickname(const Pk3& pk, Analysis& a) {
    const std::u16string& nick = pk.nickname;
    if (nick.empty()) {
        a.invalid(CheckIdentifier::Trainer, "Nickname is empty");
        return;
    }
    const Enc& e = a.enc;
    if (e.kind == Kind::Trade) {
        const auto& t = D::kTrades[e.index];
        const uint8_t lang = tradeLanguage(pk, e);
        if (lang == 0 || lang >= 8 || std::u16string_view(pk.ot) != tradeString(t, lang, true))
            a.invalid(CheckIdentifier::Trainer, "OT name does not match the in-game trade");
        if (lang == 0 || lang >= 8 || std::u16string_view(nick) != tradeString(t, lang, false))
            a.invalid(CheckIdentifier::Trainer, "Nickname does not match the in-game trade");
        return; // IFixedNickname
    }
    if (e.kind == Kind::Colo && !coloTrainerMatch(pk, D::kColoGifts[e.index]))
        a.invalid(CheckIdentifier::Trainer, "OT name does not match the Colosseum gift");
    if (e.kind == Kind::ColoGiftCxd && !coloTrainerMatch(pk, X::kColoGifts[e.index].names))
        a.invalid(CheckIdentifier::Trainer, "OT name does not match the Colosseum gift");
    if (e.kind == Kind::XdTrade) { // NicknameVerifier.VerifyTrade for IFixedTrainer / IFixedNickname
        const auto& t = X::kXdTrades[e.index];
        if (!xdFixedNameMatch(pk.language, t.trainerNames, pk.ot))
            a.invalid(CheckIdentifier::Trainer, "OT name does not match the XD in-game trade");
        if (t.nicknames != nullptr) {
            if (!xdFixedNameMatch(pk.language, t.nicknames, nick))
                a.invalid(CheckIdentifier::Trainer, "Nickname does not match the XD in-game trade");
            return; // IFixedNickname
        }
    }

    if (pk.isEgg) {
        if (!pk.isNicknamed) a.invalid(CheckIdentifier::Egg, "Egg must be flagged as nicknamed");
        if (nick != u"タマゴ") a.invalid(CheckIdentifier::Egg, "Egg name does not match the Gen III egg name");
        return;
    }
    if (!pk.isNicknamed) return;
    uint16_t other = 0;
    if (speciesAnyLanguage3(nick, other)) {
        a.fishy(CheckIdentifier::Trainer, other == pk.species ? "Nickname matches the species name of another language"
                                                              : "Nickname matches another species name");
        return;
    }
    if (hasEastAsianScript(nick)) {
        a.invalid(CheckIdentifier::Trainer, "Nickname contains characters a Gen III game cannot enter");
        return;
    }
    const size_t maxLen = (pk.language == 1 || pk.language == 8) ? 5 : 10;
    if (nick.size() > maxLen) {
        a.invalid(CheckIdentifier::Trainer, "Nickname is longer than " + std::to_string(maxLen) + " characters");
        return;
    }
    verifyWordFilter(a, nick, "Nickname");
}

void verifyTrainerName(const Pk3& pk, Analysis& a) {
    const Enc& e = a.enc;
    if (!isPlayerOriginalTrainer(e)) return;
    const std::u16string& ot = pk.ot;
    if (ot.empty()) {
        a.invalid(CheckIdentifier::Trainer, "OT name is empty");
        return;
    }
    if (startsWithIgnoreCase(ot, u"PKHeX") || startsWithIgnoreCase(ot, u"ＰＫＨｅＸ"))
        a.fishy(CheckIdentifier::Trainer, "OT name is suspicious");
    const size_t maxLen = (pk.language == 1 || pk.language == 8) ? 6 : 12;
    if (ot.size() > maxLen) {
        bool edge = false;
        if (e.isEgg) {
            if (e.kind == Kind::Gift && pk.isEgg && D::kGifts[e.index].ot != nullptr &&
                std::u16string_view(ot) == std::u16string_view(D::kGifts[e.index].ot))
                edge = true;
            else if (pk.isEgg)
                edge = ot.size() <= 12;
        }
        if (!edge) a.invalid(CheckIdentifier::Trainer, "OT name is longer than " + std::to_string(maxLen) + " characters");
    }
    verifyWordFilter(a, ot, "OT name");
}

void verifyTrainerId(const Pk3& pk, Analysis& a) {
    const Enc& e = a.enc;
    if (!isPlayerOriginalTrainer(e)) return;
    if (pk.version == kVerR || pk.version == kVerS) {
        if (lcSeeds(static_cast<uint32_t>(pk.sid) << 16, static_cast<uint32_t>(pk.tid) << 16).n == 0) {
            const uint8_t sev = e.kind == Kind::Egg && !pk.isEgg ? SFishy : SInvalid;
            a.add(sev, CheckIdentifier::Trainer, "Ruby/Sapphire cannot generate this TID/SID pair");
        }
    } else if (pk.version == kVerCXD) {
        // TrainerIDVerifier.VerifyTrainerID_CXD: the TID/SID follow the name screen's seed by
        // 1000 frames. PAL copies of XD skip the name screen's ball sequence, so an XD
        // encounter is exempt for languages above English and only Fishy for English.
        uint8_t sev = SInvalid;
        bool check = true;
        if (e.kind == Kind::XdSlot || e.kind == Kind::XdShadow || e.kind == Kind::XdStatic) {
            if (pk.language > 2) check = false;
            else if (pk.language == 2) sev = SFishy;
        }
        if (check) {
            bool capped = false;
            const bool found = cxdTrainerIdHasSeed(pk.tid, pk.sid, capped);
            if (capped) a.capped = true;
            else if (!found) a.add(sev, CheckIdentifier::Trainer, "Colosseum/XD cannot generate this TID/SID pair from the name screen");
        }
    }
    if (pk.tid == 0 && pk.sid == 0) a.fishy(CheckIdentifier::Trainer, "TID and SID are both zero");
    else if (pk.tid == pk.sid) a.fishy(CheckIdentifier::Trainer, "TID and SID are equal");
    else if (pk.tid == 0) a.fishy(CheckIdentifier::Trainer, "TID is zero");
    else if (pk.sid == 0) a.fishy(CheckIdentifier::Trainer, "SID is zero");
    else if ((pk.tid == 12345 && pk.sid == 54321) || (pk.tid == 15040 && pk.sid == 18831))
        a.fishy(CheckIdentifier::Trainer, "TID/SID pair is suspicious");
}

void verifyIvs(const Pk3& pk, Analysis& a) {
    const uint8_t hp = pk.ivs[0];
    if (hp >= 30) return;
    for (uint8_t iv : pk.ivs)
        if (iv != hp) return;
    a.fishy(CheckIdentifier::Stats, "All IVs are " + std::to_string(hp));
}

void verifyEvs(const Pk3& pk, Analysis& a) {
    int sum = 0;
    for (uint8_t ev : pk.evs) sum += ev;
    if (pk.isEgg) {
        if (sum != 0) a.invalid(CheckIdentifier::Stats, "Eggs cannot have EVs");
        return;
    }
    if (sum > 510) {
        a.invalid(CheckIdentifier::Stats, "EV total " + std::to_string(sum) + " is above 510");
        return;
    }
    const Enc& e = a.enc;
    bool vitamin = true;
    for (uint8_t ev : pk.evs)
        if (ev != 0 && (ev > 100 || ev % 10 != 0)) vitamin = false;
    if (!vitamin) {
        if (e.levelMin == 100) {
            a.invalid(CheckIdentifier::Stats, "A level 100 encounter can only gain EVs from vitamins (max 100 each)");
        } else {
            const uint8_t met = std::max(pk.metLevel, e.levelMin);
            if (expForLevel(met, personal(e.species).growth) == pk.exp)
                a.invalid(CheckIdentifier::Stats, "EVs above the vitamin pattern require EXP gain");
        }
    }
    bool allEqual = true;
    for (uint8_t ev : pk.evs) if (ev != pk.evs[0]) allEqual = false;
    if (sum == 0 && !withinEncounterRange(pk, e))
        a.note(CheckIdentifier::Stats, "Gained EXP without gaining EVs"); // EffortEXPIncreased
    else if (sum == 508) a.note(CheckIdentifier::Stats, "EV total is 508 (2 points unusable)");
    else if (pk.evs[0] != 0 && allEqual) a.fishy(CheckIdentifier::Stats, "All EVs are equal");
}

void verifyLevel(const Pk3& pk, Analysis& a) {
    const Enc& e = a.enc;
    const uint8_t growth = personal(pk.species).growth;
    if (pk.isEgg) {
        if (pk.currentLevel != e.levelMin) {
            a.invalid(CheckIdentifier::Stats, "Egg level must be " + std::to_string(e.levelMin));
            return;
        }
        if (expForLevel(e.levelMin, growth) != pk.exp)
            a.invalid(CheckIdentifier::Stats, "Egg EXP must be " + std::to_string(expForLevel(e.levelMin, growth)));
        return;
    }
    const uint8_t level = pk.currentLevel;
    if (level >= 100 && pk.exp != expForLevel(100, growth))
        a.invalid(CheckIdentifier::Stats, "EXP is above the level 100 maximum");
    if (level < pk.metLevel)
        a.invalid(CheckIdentifier::Stats, "Current level is below the met level");
    else if (!withinEncounterRange(pk, e) && level != 100 && pk.exp == expForLevel(level, growth))
        a.note(CheckIdentifier::Stats, "EXP is exactly at a level threshold");
}

bool isAllowedBattleFrontier3(uint16_t species) noexcept {
    switch (species) {
        case 150: case 151: case 249: case 250: case 251:
        case 382: case 383: case 384: case 385: case 386:
            return false;
        default:
            return true;
    }
}

void verifyRibbons(const Pk3& pk, Analysis& a) {
    const uint32_t r = pk.ribbons;
    auto bit = [r](int b) { return ((r >> b) & 1u) != 0; };
    std::vector<std::string> bad;
    std::vector<std::string> missing;
    static constexpr const char* kContest[5] = {"Cool", "Beauty", "Cute", "Smart", "Tough"};
    const bool visited3 = !a.history.empty();
    if (pk.isEgg) {
        if (bit(26)) bad.push_back("World");
        for (int i = 0; i < 5; ++i) if (((r >> (i * 3)) & 7u) != 0) bad.push_back(kContest[i]);
        if (bit(25)) bad.push_back("Earth");
        if (bit(24)) bad.push_back("National");
        if (bit(23)) bad.push_back("Country");
        if (bit(20)) bad.push_back("Champion Battle");
        if (bit(21)) bad.push_back("Champion Regional");
        if (bit(22)) bad.push_back("Champion National");
        if (bit(16)) bad.push_back("Winning");
        if (bit(17)) bad.push_back("Victory");
        if (bit(15)) bad.push_back("Champion (Hoenn)");
        if (bit(18)) bad.push_back("Artist");
        if (bit(19)) bad.push_back("Effort");
    } else {
        if (bit(26)) bad.push_back("World");
        for (int i = 0; i < 5; ++i) {
            const uint32_t count = (r >> (i * 3)) & 7u;
            if (visited3 ? count > 4 : count != 0) bad.push_back(kContest[i]);
        }
        const Enc& e = a.enc;
        if (e.kind == Kind::Gift) {
            const bool national = D::kGifts[e.index].national;
            if (bit(24) != national) (national ? missing : bad).push_back("National");
        } else if (isShadowKind(e.kind)) {
            // RibbonRules.GetValidRibbonStateNational: a shadow Pokemon is purified (and gets the
            // ribbon) before it can leave Colosseum/XD.
            if (!bit(24)) missing.push_back("National");
        } else if (bit(24)) {
            bad.push_back("National");
        }
        if (bit(23)) bad.push_back("Country");
        if (bit(20)) bad.push_back("Champion Battle");
        if (bit(21)) bad.push_back("Champion Regional");
        if (bit(22)) bad.push_back("Champion National");
        if (visited3) {
            const uint16_t head = a.history[0].species;
            if (bit(16) && !(isAllowedBattleFrontier3(head) && pk.metLevel <= 50)) bad.push_back("Winning");
            if (bit(17) && !isAllowedBattleFrontier3(head)) bad.push_back("Victory");
        } else {
            if (bit(16)) bad.push_back("Winning");
            if (bit(17)) bad.push_back("Victory");
        }
        if (bit(15) && !visited3) bad.push_back("Champion (Hoenn)");
        if (bit(18) && !visited3) bad.push_back("Artist");
        if (bit(19) && !visited3) bad.push_back("Effort");
    }
    auto join = [](const std::vector<std::string>& v) {
        std::string s;
        for (const auto& x : v) { if (!s.empty()) s += ", "; s += x; }
        return s;
    };
    if (!missing.empty()) a.invalid(CheckIdentifier::Misc, "Missing ribbons: " + join(missing));
    if (!bad.empty()) a.invalid(CheckIdentifier::Misc, "Unobtainable ribbons: " + join(bad));
}

void verifyAbility(const Pk3& pk, Analysis& a) {
    const auto& pi = personal(pk.species);
    const bool same = pi.ability1 == pi.ability2;
    const uint8_t abit = pk.abilityBit;
    const uint8_t ability = (abit != 0 && !same) ? pi.ability2 : pi.ability1;
    const int abilIndex = ability == pi.ability1 ? 0 : 1;
    const int abilityNumber = 1 << abit;
    const Enc& e = a.enc;
    if (e.kind == Kind::Trade) {
        const auto& t = D::kTrades[e.index];
        if (t.ability == 1 || t.ability == 2) {
            const int single = t.ability - 1;
            if (e.species != pk.species) {
                if (single != abilIndex) a.invalid(CheckIdentifier::Ability, "Ability does not match the in-game trade");
                return;
            }
            if (single == abilIndex || abilityNumber == (1 << single)) return;
            a.invalid(CheckIdentifier::Ability, "Ability does not match the in-game trade");
            return;
        }
    }
    if (pk.version == kVerCXD) {
        // AbilityVerifier.VerifyAbilityPreCapsule: a CXD Pokemon with two abilities can have
        // either without matching the PID (CanMismatch). A single-ability species must not set
        // the bit unless it is a shadow Pokemon (re-battles flip it; CheckMatch).
        if (same && pk.species != 210 && pk.species != 329 && pk.species != 330 && abit != 0 && !isShadowKind(e.kind))
            a.invalid(CheckIdentifier::Ability, "Ability bit is set for a species with one ability");
        return;
    }
    if (same && pk.species != 210 && pk.species != 329 && pk.species != 330) {
        if (abit != 0) a.invalid(CheckIdentifier::Ability, "Ability bit is set for a species with one ability");
    } else if (abit != (pk.pid & 1u)) {
        a.invalid(CheckIdentifier::Ability, "Ability bit does not match the PID");
    }
}

bool wildBall3(uint8_t ball) noexcept { return ball >= 1 && ball <= 12 && ball != 5; }

void verifyBall(const Pk3& pk, Analysis& a) {
    const Enc& e = a.enc;
    auto regular = [&]() -> bool {
        if (e.kind == Kind::Invalid) return true;
        if (e.kind == Kind::Egg) return pk.ball == kBallPoke;
        if (e.fixedBall != 0) return pk.ball == e.fixedBall;
        return wildBall3(pk.ball);
    };
    bool ok;
    if (e.species == 290 && pk.species == 292)
        ok = (e.kind == Kind::Slot && a.history.size() == 2) ? regular() : pk.ball == kBallPoke;
    else
        ok = regular();
    if (!ok) a.invalid(CheckIdentifier::Items, "Ball " + std::to_string(pk.ball) + " is not possible for " + encLabel(e));
}

int terminatorIndex(std::span<const uint8_t> d) noexcept {
    for (size_t i = 0; i < d.size(); ++i)
        if (d[i] == 0xFF) return static_cast<int>(i);
    return -1;
}

bool allBytes(std::span<const uint8_t> d, uint8_t value) noexcept {
    for (uint8_t b : d) if (b != value) return false;
    return true;
}

bool isTerminatedZero(std::span<const uint8_t> d) noexcept {
    const int first = terminatorIndex(d);
    if (first == -1 || first >= static_cast<int>(d.size()) - 1) return true;
    return allBytes(d.subspan(static_cast<size_t>(first) + 1), 0);
}

bool isTerminatedFF(std::span<const uint8_t> d) noexcept {
    const int first = terminatorIndex(d);
    if (first == -1 || first >= static_cast<int>(d.size()) - 1) return true;
    return allBytes(d.subspan(static_cast<size_t>(first) + 1), 0xFF);
}

bool isTerminatedFFZero(std::span<const uint8_t> d, size_t preFill) noexcept {
    if (preFill == 0) return isTerminatedZero(d);
    const int firstIndex = terminatorIndex(d);
    if (firstIndex == -1 || firstIndex >= static_cast<int>(d.size()) - 1) return true;
    size_t first = static_cast<size_t>(firstIndex) + 1;
    if (first < preFill) {
        if (!allBytes(d.subspan(first, preFill - first), 0xFF)) return false;
        first = preFill;
        if (first >= d.size()) return true;
    }
    return allBytes(d.subspan(first), 0);
}

bool isPatternDefault(std::span<const uint8_t> trash, uint8_t version, uint8_t language, uint8_t gender) noexcept {
    const uint8_t bit = versionBit(version);
    if (bit != D::R && bit != D::S && bit != D::E) return false;
    for (size_t i = 0; i < D::kTrashPatternCount; ++i) {
        const auto& p = D::kTrashPatterns[i];
        if (p.version != bit || p.language != language) continue;
        bool same = true;
        for (size_t k = 0; k < 7; ++k) if (p.bytes[k] != trash[k]) same = false;
        if (same) return p.gender == gender;
    }
    return false;
}

bool isDefaultTrainer(const Pk3& pk, const Enc& e) noexcept {
    if (pk.language == 1) return false;
    if (pk.version != kVerR && pk.version != kVerS && pk.version != kVerE) return false;
    const std::span<const uint8_t> trash(pk.otRaw);
    if (isPatternDefault(trash, pk.version, pk.language, pk.otGender) &&
        (!e.isEgg || pk.isEgg || isValidMet3(pk.metLocation, pk.version)))
        return true;
    if (!e.isEgg || pk.isEgg) return false;
    for (uint8_t v = kVerS; v <= kVerE; ++v) {
        if (v == pk.version) continue;
        if (!isPatternDefault(trash, v, pk.language, pk.otGender)) continue;
        if (isValidMet3(pk.metLocation, v)) return true;
    }
    return false;
}

void flagIsNicknameClean(const Pk3& pk, Analysis& a) {
    if (pk.isEgg) return;
    const std::u16string& name = pk.nickname;
    if (name == std::u16string_view(speciesName3(pk.species, pk.language))) return;
    if (a.history.size() > 1) {
        for (size_t i = 1; i < a.history.size(); ++i)
            if (isNicknamedAnyLanguage3(a.history[i].species, name)) return; // PKHeX IsNicknameLanguageEvolution
    }
    const std::span<const uint8_t> nick(pk.nicknameRaw);
    const auto region = pk.language == 1 ? nick.first(6) : nick;
    if (isTerminatedFF(region)) return;
    if (!isTerminatedFFZero(region, 0))
        a.invalid(CheckIdentifier::Trainer, "Nickname trash bytes do not match a nickname entered in-game");
}

void verifyTrash(const Pk3& pk, Analysis& a) {
    const std::span<const uint8_t> ot(pk.otRaw);
    const std::span<const uint8_t> nick(pk.nicknameRaw);
    if (!pk.isEgg && isTerminatedZero(ot) && (pk.isNicknamed || isTerminatedZero(nick))) return; // reset via Colosseum/XD
    const Enc& e = a.enc;
    if (e.kind == Kind::Trade) {
        if (!isTerminatedZero(ot)) a.invalid(CheckIdentifier::Trainer, "OT name trash bytes should be empty for an in-game trade");
        if (!isTerminatedZero(nick)) a.invalid(CheckIdentifier::Trainer, "Nickname trash bytes should be empty for an in-game trade");
        return;
    }
    if (e.kind == Kind::Gift || e.kind == Kind::GiftJPN || e.kind == Kind::GiftNY) return;
    if (pk.language == 1 && !(pk.isEgg && pk.otRaw[6] == 0xFF)) {
        if (!isTerminatedFFZero(ot, 6)) a.invalid(CheckIdentifier::Trainer, "OT name is missing its final terminator padding");
    } else if (!isTerminatedFFZero(ot, 7) && !isDefaultTrainer(pk, e)) {
        a.invalid(CheckIdentifier::Trainer, "OT name is missing its final terminator padding");
    }
    flagIsNicknameClean(pk, a);
}

void verifyMisc(const Pk3& pk, Analysis& a) {
    const Enc& e = a.enc;
    if (pk.isEgg) {
        if (pk.friendship > personal(pk.species).hatchCycles)
            a.invalid(CheckIdentifier::Egg, "Egg hatch counter is above the species maximum");
        bool contest = false;
        for (uint8_t c : pk.contest) if (c != 0) contest = true;
        if (contest) a.invalid(CheckIdentifier::Egg, "Eggs cannot have contest stats");
        if (pk.language != 1) a.invalid(CheckIdentifier::Egg, "Gen III eggs are always flagged Japanese");
    }
    verifyTrash(pk, a);
    // MiscFatefulEncounterVerifier
    bool checkInactive = true;
    if (e.kind == Kind::Gift) {
        const auto& g = D::kGifts[e.index];
        auto giftShiny = [&]() {
            if ((g.shiny == 1 && pk.shiny) || (g.shiny == 2 && !pk.shiny))
                a.invalid(CheckIdentifier::EventGift, "Shiny state does not match the event gift");
        };
        if (g.fateful) {
            checkInactive = false;
            if (!(g.egg && pk.metLocation <= 87)) {
                if (!pk.fateful) a.invalid(CheckIdentifier::EventGift, "Event gift is missing its fateful encounter flag");
                giftShiny();
            }
        } else {
            giftShiny();
        }
    } else if (e.kind == Kind::Static && D::kStatics[e.index].fateful) {
        checkInactive = false;
        if (!pk.fateful) a.invalid(CheckIdentifier::EventGift, "Event encounter is missing its fateful encounter flag");
    } else if (isXdFatefulKind(e.kind)) {
        // IFatefulEncounterReadOnly { FatefulEncounter: true }: XD sets the obedience flag on
        // every Pokemon it sends to a GBA game (XK3.ConvertToPK3); Colosseum never does.
        checkInactive = false;
        if (!pk.fateful) a.invalid(CheckIdentifier::EventGift, "XD Pokemon are missing the fateful encounter flag");
    }
    if (checkInactive && pk.fateful) a.invalid(CheckIdentifier::EventGift, "Fateful encounter flag is not possible for this encounter");
    // MiscPokerusVerifier
    const uint8_t strain = pk.pokerus >> 4;
    const uint8_t days = pk.pokerus & 0xF;
    if (days > (strain & 3) + 1) a.invalid(CheckIdentifier::Misc, "Pokerus days remaining exceed the strain duration");
}

void verifyMovePP(const Pk3& pk, Analysis& a) {
    if (pk.isEgg) {
        if (pk.ppUps != 0) a.invalid(CheckIdentifier::Egg, "Eggs cannot have PP Ups");
        for (size_t i = 0; i < 4; ++i) {
            if (pk.pp[i] != movePP(pk.moves[i])) {
                a.invalid(CheckIdentifier::Egg, "Egg move PP must be the base PP");
                break;
            }
        }
        return;
    }
    for (size_t i = 0; i < 4; ++i) {
        const uint8_t ups = static_cast<uint8_t>((pk.ppUps >> (i * 2)) & 3u);
        const uint16_t move = pk.moves[i];
        if (ups != 0 && (move == 0 || move == D::kMoveSketch))
            a.invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " cannot have PP Ups");
        const uint32_t healed = static_cast<uint32_t>(movePP(move)) * (5u + ups) / 5u;
        if (pk.pp[i] > healed)
            a.invalid(CheckIdentifier::Moves, "Move " + std::to_string(i + 1) + " PP is above its maximum");
    }
}

void verifyItem(const Pk3& pk, Analysis& a, bool& enigma) {
    const uint16_t item = pk.heldItem;
    if (pk.isEgg && item != 0) a.invalid(CheckIdentifier::Items, "Eggs cannot hold items");
    if (item == 0) return;
    if (!heldItemValid(item)) {
        a.invalid(CheckIdentifier::Items, "Held item " + std::to_string(item) + " cannot be held in Gen III");
        return;
    }
    if (item == 175) enigma = true; // depends on the save's e-Reader berry
}

/// ContestStatInfo (Gen III Pokeblock correlation).
void verifyContest(const Pk3& pk, Analysis& a) {
    const auto& s = pk.contest; // Cool Beauty Cute Smart Tough Sheen
    bool any = false;
    for (uint8_t c : s) if (c != 0) any = true;
    if (!any) return;
    std::array<uint8_t, 6> initial{};
    if (a.enc.kind == Kind::Trade) {
        const auto& t = D::kTrades[a.enc.index];
        for (size_t i = 0; i < 6; ++i) initial[i] = t.contest[i];
        if (s == initial) return;
    }
    const uint8_t nature = pk.nature;
    auto gainedSum = [&](uint8_t n) {
        static constexpr size_t kStat[5] = {0, 4, 1, 3, 2}; // Cool, Tough, Beauty, Smart, Cute
        int sum = 0;
        for (size_t ampIndex = 0; ampIndex < 5; ++ampIndex) {
            const size_t stat = kStat[ampIndex];
            const int gain = static_cast<int>(s[stat]) - static_cast<int>(initial[stat]);
            if (gain <= 0) continue;
            const int amp = D::kNatureAmps[static_cast<size_t>(n % 25) * 5 + ampIndex];
            if (amp == 0) { sum += gain; continue; }
            const int factor = amp == 1 ? 11 : 9;
            int boost = gain / factor;
            if (gain % factor >= 5) ++boost;
            sum += gain + (amp == -1 ? boost : -boost);
        }
        return sum;
    };
    auto averageFeel = [&](uint8_t n) { return (gainedSum(n) + 4) / 5; };
    const bool equal = s == initial;
    // CalculateMinimumSheen3
    int minSheen;
    if (equal) {
        minSheen = initial[5];
    } else {
        const int rawAvg = averageFeel(0);
        if (rawAvg == 255) {
            minSheen = 214;
        } else {
            int avg = std::max(1, nature % 6 == 0 ? rawAvg : averageFeel(nature));
            avg = std::min(rawAvg, avg);
            avg = (214 * avg) / 255;
            minSheen = std::clamp(avg, 1, 214);
        }
    }
    if (s[5] < minSheen)
        a.invalid(CheckIdentifier::Stats, "Contest sheen must be at least " + std::to_string(minSheen));
    // CalculateMaximumSheen (pokeBlock3)
    int maxSheen;
    if (s[0] == 255 || s[1] == 255 || s[2] == 255 || s[3] == 255 || s[4] == 255) {
        maxSheen = 255;
    } else if (equal) {
        maxSheen = initial[5];
    } else {
        const int sum = gainedSum(nature);
        if (sum == 0) {
            maxSheen = 0;
        } else {
            int gained2 = 0;
            for (size_t i = 0; i < 5; ++i) if (static_cast<int>(s[i]) - static_cast<int>(initial[i]) >= 2) ++gained2;
            const int permit = gained2 >= 3 ? (sum * 83 / 6) : (sum * 19 / 9);
            maxSheen = std::clamp(permit, 1, 255);
        }
    }
    if (s[5] > maxSheen)
        a.invalid(CheckIdentifier::Stats, "Contest sheen must be at most " + std::to_string(maxSheen));
}

void verifyMarking(const Pk3& pk, Analysis& a) {
    if ((pk.markings & 0xF0u) != 0) a.invalid(CheckIdentifier::Misc, "Unused marking bits are set");
}

const char* moveProblem(uint8_t code) noexcept {
    switch (code) {
        case MDuplicate: return "is a duplicate";
        case MEmptyInvalid: return "slot is empty before a later move";
        default: return "cannot be learned by this Pokemon";
    }
}

void reportMoves(const Pk3& pk, Analysis& a) {
    for (size_t i = 0; i < 4; ++i) {
        if (moveValid(a.moves[i])) continue;
        const uint16_t move = pk.moves[i];
        std::string text = "Move " + std::to_string(i + 1);
        if (move != 0) text += " (" + moveLabel(move) + ")";
        text += std::string(" ") + moveProblem(a.moves[i]);
        a.invalid(CheckIdentifier::Moves, std::move(text));
    }
}

/// CXDVerifier (LegalityAnalysis.ParsePK3 runs it for Version CXD): the XD starter Eevee's
/// PID and IVs follow from the TID/SID, and the Colosseum/XD player characters are male.
void verifyCxd(const Pk3& pk, Analysis& a) {
    if (pk.version != kVerCXD) return;
    if (a.enc.kind == Kind::XdStatic && a.enc.species == 133 &&
        (a.pidiv.type == CXD || a.pidiv.type == CXD_ColoStarter)) {
        const uint32_t iv32 = pk.iv32 & 0x3FFFFFFFu;
        uint32_t seed = 0;
        if (!xdStarterSeed(pk, iv32 & 0x7FFFu, iv32 >> 15, seed))
            a.invalid(CheckIdentifier::PidRng, "XD starter Eevee PID and IVs do not follow from this TID/SID");
        else
            a.pidiv = makePidIv(CXD, seed);
    }
    if (pk.otGender == 1) a.invalid(CheckIdentifier::Trainer, "Colosseum/XD Pokemon have a male Original Trainer");
}

/// Full PKHeX pipeline for a Gen III PK3 (version R/S/E/FR/LG or Colosseum/XD, species 1-386).
void runChecks(const Pk3& pk, Analysis& a, bool& enigma) {
    findEncounter(pk, a);
    verifyPid(pk, a);
    verifyLanguage(pk, a);
    verifyNickname(pk, a);
    verifyTrainerName(pk, a);
    verifyTrainerId(pk, a);
    verifyIvs(pk, a);
    verifyEvs(pk, a);
    verifyLevel(pk, a);
    verifyRibbons(pk, a);
    verifyAbility(pk, a);
    verifyBall(pk, a);
    verifyMisc(pk, a);
    verifyMovePP(pk, a);
    verifyItem(pk, a, enigma);
    verifyContest(pk, a);
    verifyMarking(pk, a);
    verifyCxd(pk, a);
    reportMoves(pk, a);
}

} // namespace

namespace {

// ---------------------------------------------------------------------------
// Report assembly
// ---------------------------------------------------------------------------

bool isGbaSourceGame(std::string_view id) noexcept {
    return id == "ruby_gba" || id == "sapphire_gba" || id == "emerald_gba" || id == "firered_gba" ||
           id == "leafgreen_gba";
}

void setAllCoverage(CoverageSummary& c, CoverageLevel level) noexcept {
    c.structure = c.sourceGame = c.internal = c.moves = c.encounter = level;
    c.eventGift = c.pidRng = c.eggBreeding = c.transfer = level;
}

bool allCoverageComplete(const CoverageSummary& c) noexcept {
    return c.structure == CoverageLevel::Complete && c.sourceGame == CoverageLevel::Complete &&
           c.internal == CoverageLevel::Complete && c.moves == CoverageLevel::Complete &&
           c.encounter == CoverageLevel::Complete && c.eventGift == CoverageLevel::Complete &&
           c.pidRng == CoverageLevel::Complete && c.eggBreeding == CoverageLevel::Complete &&
           c.transfer == CoverageLevel::Complete;
}

Severity mapSeverity(uint8_t sev) noexcept {
    switch (sev) {
        case SInvalid: return Severity::Invalid;
        case SFishy: return Severity::Warning;
        default: return Severity::Info;
    }
}

bool isUnusedInternalSpecies(uint16_t internal) noexcept {
    return internal >= D::kInternalSpeciesCount || (internal != 0 && D::kInternalToNational[internal] == 0);
}

/// "wild slot (mass outbreak) Seedot at met location 32, level 13" for a matched encounter.
std::string describeEncounter(const Pk3& pk, const Enc& e) {
    std::string text = kindName(e.kind);
    if (e.kind == Kind::Slot && e.area >= D::kAreaCount) text += " (mass outbreak)";
    if (e.kind == Kind::Static && D::kStatics[e.index].roaming) text += " (roaming)";
    if (e.kind == Kind::ColoShadow && e.eReader) text += " (e-Reader)";
    if (e.kind == Kind::XdShadow && shadowOf(e).location != pk.metLocation) text += " (Miror B. rebattle)";
    text += " " + speciesLabel(e.species);
    text += " at met location " + std::to_string(pk.metLocation) + ", level " + std::to_string(pk.metLevel);
    return text;
}

bool isCertified(const Report& r) noexcept { return allCoverageComplete(r.coverage) && r.problemCount() == 0; }

Report analyzeDetailed(std::span<const uint8_t> decrypted, std::string_view exactSourceGameId, Verification* out);

} // namespace

std::vector<uint16_t> lineage(uint16_t species) {
    std::vector<uint16_t> out{species};
    for (size_t i = 0; i < out.size() && out.size() < 16; ++i) {
        const uint16_t s = out[i];
        if (s == 0 || s > D::kMaxSpecies) continue;
        const auto& node = reverseLineage()[s];
        for (const EvoLink* link : {&node.first, &node.second})
            if (link->species != 0 && std::find(out.begin(), out.end(), link->species) == out.end())
                out.push_back(link->species);
    }
    return out;
}

Verification verify(const Pokemon::Pokemon& pk, std::string_view exactSourceGameId) {
    Verification v;
    if (pk.getGameGroup() != Enums::GameVersion::FRLG) return v;
    const size_t size = pk.getDataSize();
    if (size != 80 && size != 100) return v;
    const std::span<const std::byte> data = pk.getData();
    if (data.size() < size) return v;
    std::vector<uint8_t> raw(size);
    for (size_t i = 0; i < size; ++i) raw[i] = static_cast<uint8_t>(data[i]);
    v.ran = true;
    Pk3 fields;
    read(raw, fields);
    if (fields.speciesInternal == 0) { // empty slot: Legality::analyze has nothing to validate
        v.emptySlot = true;
        return v;
    }
    v.report = analyzeDetailed(raw, exactSourceGameId, &v);
    v.certified = isCertified(v.report);
    return v;
}

namespace {

Report analyzeDetailed(std::span<const uint8_t> decrypted, std::string_view exactSourceGameId, Verification* out) {
    Report r;
    setAllCoverage(r.coverage, CoverageLevel::Partial);
    auto add = [&r](Severity sev, CheckIdentifier id, std::string text) {
        r.issues.push_back(Issue{sev, std::move(text), id});
    };
    if (decrypted.size() != 80 && decrypted.size() != 100) {
        r.coverage.structure = CoverageLevel::None;
        add(Severity::Warning, CheckIdentifier::Structure,
            "Record is not a Gen III PK3 (" + std::to_string(decrypted.size()) + " bytes); Gen III rules were not run");
        return r;
    }
    Pk3 pk;
    read(decrypted, pk);
    bool certify = true;
    if (checksum(decrypted) != pk.checksum) {
        add(Severity::Warning, CheckIdentifier::Checksum,
            "PK3 checksum does not match its data; the game shows this record as a Bad Egg");
        certify = false;
    }
    if (pk.speciesInternal == 0) {
        add(Severity::Warning, CheckIdentifier::Species, "PK3 record holds no species (empty slot)");
        return r;
    }
    if (isUnusedInternalSpecies(pk.speciesInternal)) {
        add(Severity::Invalid, CheckIdentifier::Species,
            "Species index " + std::to_string(pk.speciesInternal) + " is not a Gen III Pokemon");
        return r;
    }
    if (!isGbaVersion(pk.version) && pk.version != kVerCXD) {
        add(Severity::Warning, CheckIdentifier::SourceGame,
            "Origin game id " + std::to_string(pk.version) + " is not a Gen III cartridge; encounter rules were not run");
        return r;
    }

    Analysis a;
    bool enigma = false;
    runChecks(pk, a, enigma);
    bool problem = false;
    for (auto& f : a.findings) {
        if (f.sev != SInfo) problem = true;
        // A search guard was hit: nothing below is proven, so no finding may stay Invalid.
        const Severity sev = (a.capped && f.sev == SInvalid) ? Severity::Warning : mapSeverity(f.sev);
        add(sev, f.id, std::move(f.text));
    }
    if (enigma) {
        problem = true;
        add(Severity::Warning, CheckIdentifier::Items,
            "Enigma Berry: legality depends on the e-Reader berry stored in the original save");
    }
    if (a.capped) {
        problem = true;
        add(Severity::Warning, CheckIdentifier::PidRng,
            "RNG search reached its limit; the encounter could not be decided (Invalid findings above are undecided)");
    }
    if (a.unsupported) {
        problem = true;
        add(Severity::Warning, CheckIdentifier::Trainer, "A word-filter pattern could not be evaluated");
    }
    r.coverage.structure = certify ? CoverageLevel::Complete : CoverageLevel::Partial;
    if (certify && !problem && a.enc.kind != Kind::Invalid && isGbaSourceGame(exactSourceGameId))
        setAllCoverage(r.coverage, CoverageLevel::Complete);
    if (out != nullptr && a.enc.kind != Kind::Invalid) {
        out->encounter = describeEncounter(pk, a.enc);
        out->inGameTrade = a.enc.kind == Kind::Trade;
    }
    return r;
}

} // namespace

} // namespace Gen3Pk3

Report analyzePk3(std::span<const uint8_t> decrypted, std::string_view exactSourceGameId) {
    return Gen3Pk3::analyzeDetailed(decrypted, exactSourceGameId, nullptr);
}

void applyGen3Analyzer(Report& r, const Gen3Pk3::Verification& verified, std::string_view exactSourceGameId) {
    if (!verified.ran || verified.emptySlot) return;
    for (const auto& issue : verified.report.issues) {
        if (issue.severity == Severity::Info) continue;
        bool duplicate = false;
        for (const auto& existing : r.issues)
            if (existing.text == issue.text) { duplicate = true; break; }
        if (!duplicate) r.issues.push_back(issue);
    }
    if (verified.certified && r.problemCount() == 0 && Gen3Pk3::isGbaSourceGame(exactSourceGameId))
        Gen3Pk3::setAllCoverage(r.coverage, CoverageLevel::Complete);
}

void applyGen3Analyzer(Report& r, const Pokemon::Pokemon& pk, std::string_view exactSourceGameId) {
    applyGen3Analyzer(r, Gen3Pk3::verify(pk, exactSourceGameId), exactSourceGameId);
}

} // namespace Legality
