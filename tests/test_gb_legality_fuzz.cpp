// Malformed-input hardening for the Gen I/II legality path. Nothing here asserts a
// verdict: every input is random or deliberately broken, and the checks are that the
// engine, the decoders, the save parsers and the production analyzer return a well-formed
// answer without reading out of bounds, overflowing or crashing. Built for the sanitizer
// configuration (host-sanitize: -fsanitize=address,undefined) and also run plain.
//
//  1. GB::checkGen1 / checkGen2 / formatProblems / describe on random Entity fields,
//     random and truncated name bytes, invalid UTF-8 in the decoded names.
//  2. decodeGen1String / decodeGen2String on random name fields of every length 0..16.
//  3. Gen1::parse / Gen2::parse on random buffers of many sizes, a checksum-valid Red
//     save truncated at every tenth byte, and a Red save with random bytes flipped.
//  4. Legality::analyze on random PokemonRecord values (what a damaged save parses to).
#include "Integration/Gen1/Gen1ReadOnlySave.h"
#include "Integration/Gen2/Gen2ReadOnlySave.h"
#include "Legality/GBLegality.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon1ReadOnly.h"
#include "Pokemon/Pokemon2ReadOnly.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <span>
#include <string>
#include <vector>

namespace Trainer {
const char* getSpeciesName(uint16_t id) { return Names::getSpeciesName(id); }
const char* getItemName(uint16_t id) { return Names::getItemName(id); }
}

namespace {

namespace GB = Legality::GB;
namespace Gen1 = PokeVault::Integration::Gen1;
namespace Gen2 = PokeVault::Integration::Gen2;

int checks = 0, failures = 0;

void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::fprintf(stderr, "FAIL %s\n", what.c_str());
    }
}

// Deterministic: the same inputs every run (SplitMix64).
struct Rng {
    uint64_t state;
    uint64_t next() {
        uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    uint32_t below(uint32_t n) { return n == 0 ? 0 : static_cast<uint32_t>(next() % n); }
    uint8_t byte() { return static_cast<uint8_t>(next()); }
    bool coin() { return (next() & 1) != 0; }
};

std::vector<uint8_t> randomBytes(Rng& rng, std::size_t n) {
    std::vector<uint8_t> out(n);
    for (auto& b : out)
        b = rng.byte();
    return out;
}

// Random bytes, biased towards what name fields hold: letters, terminators, Hangul pages,
// the trade marker, and invalid UTF-8 when read as text.
std::vector<uint8_t> nameBytes(Rng& rng) {
    const std::size_t n = rng.below(17);
    std::vector<uint8_t> out(n);
    for (auto& b : out) {
        switch (rng.below(6)) {
            case 0: b = 0x50; break;
            case 1: b = static_cast<uint8_t>(0x80 + rng.below(26)); break;
            case 2: b = static_cast<uint8_t>(rng.below(12)); break;
            case 3: b = 0x5D; break;
            default: b = rng.byte(); break;
        }
    }
    return out;
}

std::string randomText(Rng& rng) {
    std::string s;
    const std::size_t n = rng.below(14);
    for (std::size_t i = 0; i < n; ++i) {
        switch (rng.below(5)) {
            case 0: s += static_cast<char>('A' + rng.below(26)); break;
            case 1: s += static_cast<char>(0xE3); break;  // a kana lead byte, possibly dangling
            case 2: s += "サ"; break;
            case 3: s += '*'; break;
            default: s += static_cast<char>(rng.byte()); break;
        }
    }
    return s;
}

bool wellFormed(const GB::Result& r) {
    if (r.status != GB::Status::Legal && r.status != GB::Status::Illegal && r.status != GB::Status::Uncertain)
        return false;
    if (r.status != GB::Status::Legal && r.problems.empty())
        return false;
    for (const auto& p : r.problems)
        if (p.text.empty())
            return false;
    return true;
}

// ---------------------------------------------------------------- 1. the engine

void engine(Rng& rng) {
    int ok1 = 0, ok2 = 0;
    for (int i = 0; i < 6000; ++i) {
        GB::Entity1 e;
        e.species = rng.coin() ? static_cast<uint8_t>(1 + rng.below(151)) : rng.byte();
        e.level = rng.coin() ? static_cast<uint8_t>(1 + rng.below(100)) : rng.byte();
        for (int k = 0; k < 4; ++k) {
            e.moves[k] = rng.coin() ? static_cast<uint8_t>(rng.below(166)) : rng.byte();
            e.pp[k] = rng.byte();
            e.ppUps[k] = static_cast<uint8_t>(rng.below(5));
        }
        e.exp = rng.coin() ? static_cast<uint32_t>(rng.below(0x1000000)) : static_cast<uint32_t>(rng.next());
        e.catchRate = rng.byte();
        e.type1 = rng.byte();
        e.type2 = rng.byte();
        for (auto& dv : e.dvs)
            dv = rng.coin() ? static_cast<uint8_t>(rng.below(16)) : rng.byte();
        for (auto& se : e.statExp)
            se = static_cast<uint16_t>(rng.next());
        e.tid = static_cast<uint16_t>(rng.next());
        e.tradeTrainer = rng.coin();
        e.otName = randomText(rng);
        e.nickname = randomText(rng);
        e.language = static_cast<GB::Language>(rng.below(5));  // 4 is outside the enum
        if (rng.coin()) {
            e.otRaw = nameBytes(rng);
            e.nickRaw = nameBytes(rng);
        }
        const GB::Result r = GB::checkGen1(e);
        ok1 += wellFormed(r);
        (void)GB::describe(r.origin);
        (void)GB::formatProblems(e);
    }
    check(ok1 == 6000, "6000 random Entity1: every result well-formed (" + std::to_string(ok1) + ")");
    for (int i = 0; i < 6000; ++i) {
        GB::Entity2 e;
        e.species = rng.coin() ? static_cast<uint8_t>(1 + rng.below(251)) : rng.byte();
        e.level = rng.coin() ? static_cast<uint8_t>(1 + rng.below(100)) : rng.byte();
        for (int k = 0; k < 4; ++k) {
            e.moves[k] = rng.coin() ? static_cast<uint8_t>(rng.below(252)) : rng.byte();
            e.pp[k] = rng.byte();
            e.ppUps[k] = static_cast<uint8_t>(rng.below(5));
        }
        e.heldItem = rng.byte();
        for (auto& dv : e.dvs)
            dv = rng.coin() ? static_cast<uint8_t>(rng.below(16)) : rng.byte();
        for (auto& se : e.statExp)
            se = static_cast<uint16_t>(rng.next());
        e.pokerus = rng.byte();
        e.tid = static_cast<uint16_t>(rng.next());
        e.exp = rng.coin() ? static_cast<uint32_t>(rng.below(0x1000000)) : static_cast<uint32_t>(rng.next());
        e.friendship = rng.byte();
        e.isEgg = rng.below(4) == 0;
        e.caughtData = rng.coin() ? 0 : static_cast<uint16_t>(rng.next());
        e.tradeTrainer = rng.coin();
        e.otName = randomText(rng);
        e.nickname = randomText(rng);
        e.language = static_cast<GB::Language>(rng.below(5));
        if (rng.coin()) {
            e.otRaw = nameBytes(rng);
            e.nickRaw = nameBytes(rng);
        }
        const GB::Result r = GB::checkGen2(e);
        ok2 += wellFormed(r);
        (void)GB::describe(r.origin);
        (void)GB::formatProblems(e);
    }
    check(ok2 == 6000, "6000 random Entity2: every result well-formed (" + std::to_string(ok2) + ")");
}

// ---------------------------------------------------------------- 2. the decoders

void decoders(Rng& rng) {
    for (std::size_t n = 0; n <= 16; ++n) {
        for (int i = 0; i < 200; ++i) {
            const std::vector<uint8_t> bytes = n == 0 ? std::vector<uint8_t>{} : randomBytes(rng, n);
            const std::span<const uint8_t> view(bytes);
            (void)Gen1::decodeGen1String(view, Gen1::RegionLayout::International);
            (void)Gen1::decodeGen1String(view, Gen1::RegionLayout::Japanese);
            (void)Gen2::decodeGen2String(view, Gen2::RegionLayout::International);
            (void)Gen2::decodeGen2String(view, Gen2::RegionLayout::Japanese);
        }
    }
    // Every single byte, alone and followed by a terminator.
    for (unsigned b = 0; b < 256; ++b) {
        const std::vector<uint8_t> one{static_cast<uint8_t>(b)}, two{static_cast<uint8_t>(b), 0x50};
        for (const auto& bytes : {one, two}) {
            const std::span<const uint8_t> view(bytes);
            (void)Gen1::decodeGen1String(view, Gen1::RegionLayout::International);
            (void)Gen1::decodeGen1String(view, Gen1::RegionLayout::Japanese);
            (void)Gen2::decodeGen2String(view, Gen2::RegionLayout::International);
            (void)Gen2::decodeGen2String(view, Gen2::RegionLayout::Japanese);
        }
    }
    check(true, "decoders survived every byte and random fields of length 0..16");
}

// ---------------------------------------------------------------- 3. the save parsers

// The international Red/Blue layout as in test_gb_legality_structural.cpp.
constexpr unsigned kSaveSize = 0x8000, kMain = 0x2598, kMainLength = 0x0F8B, kChecksum = 0x3523;
constexpr unsigned kParty = 0x2F2C, kTid = 0x2605, kMoney = 0x25F3, kBoxIndex = 0x284C, kBox = 0x30C0;

void put16(std::vector<uint8_t>& b, unsigned o, unsigned v) {
    b[o] = static_cast<uint8_t>(v >> 8);
    b[o + 1] = static_cast<uint8_t>(v);
}

std::vector<uint8_t> redSave() {
    std::vector<uint8_t> b(kSaveSize, 0);
    b[kMain] = 0x80;
    b[kMain + 1] = 0x50;
    b[kMoney + 1] = 0x30;
    put16(b, kTid, 1234);
    b[kBoxIndex] = 0;
    b[kBox] = 0;
    b[kBox + 1] = 0xFF;
    b[kParty] = 1;
    b[kParty + 1] = 0x99;
    b[kParty + 2] = 0xFF;
    const unsigned o = kParty + 8;
    b[o] = 0x99;
    put16(b, o + 1, 45);
    b[o + 3] = 10;
    b[o + 5] = 0x16;
    b[o + 6] = 0x03;
    b[o + 7] = 45;
    b[o + 8] = 33;
    b[o + 9] = 45;
    put16(b, o + 0x0C, 1234);
    b[o + 0x10] = 0x40;  // 64... EXP 1000 at level 10 is 0x0003E8
    b[o + 0x0F] = 0x03;
    b[o + 0x10] = 0xE8;
    put16(b, o + 0x1B, 0x8888);
    b[o + 0x1D] = 35;
    b[o + 0x1E] = 40;
    b[o + 0x21] = 10;
    const unsigned ot = o + 6 * 44, nick = ot + 6 * 11;
    b[ot] = 0x80;
    b[ot + 1] = 0x50;
    b[nick] = 0x81;
    b[nick + 1] = 0x50;
    unsigned sum = 0xFF;
    for (unsigned i = kMain; i < kMain + kMainLength; ++i)
        sum = (sum - b[i]) & 0xFF;
    b[kChecksum] = static_cast<uint8_t>(sum);
    return b;
}

void analyzeAll(const Gen1::ParseResult& parsed) {
    if (!parsed || !parsed.save)
        return;
    for (const auto& rec : parsed.save->party()) {
        const Pokemon::Pokemon1ReadOnly pk(rec);
        (void)Legality::analyze(pk, pk.getGameGroup(), "red_gb");
    }
}

void saveParsers(Rng& rng) {
    const std::vector<uint8_t> good = redSave();
    {
        const auto parsed = Gen1::parse(good, Gen1::SourceGame::Red);
        check(parsed && parsed.save && parsed.save->party().size() == 1, "the reference Red save parses");
        analyzeAll(parsed);
    }
    // Random buffers of many sizes, through both parsers with every game hint.
    for (std::size_t size : {std::size_t{0}, std::size_t{1}, std::size_t{7}, std::size_t{100}, std::size_t{0x1FFF},
                             std::size_t{0x2000}, std::size_t{0x7FFF}, std::size_t{0x8000}, std::size_t{0x8001},
                             std::size_t{0x10000}}) {
        for (int i = 0; i < 4; ++i) {
            const std::vector<uint8_t> bytes = randomBytes(rng, size);
            for (auto game : {Gen1::SourceGame::Red, Gen1::SourceGame::Blue, Gen1::SourceGame::Yellow})
                analyzeAll(Gen1::parse(bytes, game));
            for (auto game : {Gen2::SourceGame::Gold, Gen2::SourceGame::Silver, Gen2::SourceGame::Crystal}) {
                const auto parsed = Gen2::parse(bytes, game);
                if (parsed && parsed.save) {
                    for (const auto& rec : parsed.save->party()) {
                        const Pokemon::Pokemon2ReadOnly pk(rec);
                        (void)Legality::analyze(pk, pk.getGameGroup(), "");
                    }
                }
            }
        }
    }
    // The good save truncated at every tenth byte.
    for (std::size_t cut = 0; cut < good.size(); cut += 10) {
        const std::vector<uint8_t> part(good.begin(), good.begin() + static_cast<std::ptrdiff_t>(cut));
        analyzeAll(Gen1::parse(part, Gen1::SourceGame::Red));
    }
    // The good save with bytes flipped in and around the party list, checksum repaired so
    // the record itself is what gets parsed.
    for (int i = 0; i < 400; ++i) {
        std::vector<uint8_t> b = good;
        const int flips = 1 + static_cast<int>(rng.below(8));
        for (int k = 0; k < flips; ++k) {
            const unsigned at = kParty + rng.below(8 + 6 * 44 + 12 * 11);
            b[at] = rng.byte();
        }
        if (rng.coin()) {
            unsigned sum = 0xFF;
            for (unsigned j = kMain; j < kMain + kMainLength; ++j)
                sum = (sum - b[j]) & 0xFF;
            b[kChecksum] = static_cast<uint8_t>(sum);
        }
        analyzeAll(Gen1::parse(b, Gen1::SourceGame::Red));
    }
    check(true, "save parsers survived random, truncated and flipped buffers");
}

// ---------------------------------------------------------------- 4. random records

void records(Rng& rng) {
    int verdicts = 0;
    for (int i = 0; i < 1500; ++i) {
        Gen1::PokemonRecord rec{};
        rec.rawSpecies = rng.byte();
        rec.species = rng.coin() ? static_cast<uint16_t>(rng.below(256)) : static_cast<uint16_t>(rng.next());
        rec.type1 = rng.byte();
        rec.type2 = rng.byte();
        rec.catchRate = rng.byte();
        for (auto& m : rec.moves)
            m = rng.byte();
        rec.trainerId = static_cast<uint16_t>(rng.next());
        rec.experience = static_cast<uint32_t>(rng.next());
        for (auto& se : rec.statExperience)
            se = static_cast<uint16_t>(rng.next());
        for (auto& dv : rec.dvs)
            dv = rng.byte();
        for (auto& pp : rec.pp)
            pp = rng.byte();
        for (auto& up : rec.ppUps)
            up = rng.byte();
        rec.level = rng.byte();
        rec.originalTrainer = randomText(rng);
        rec.nickname = randomText(rng);
        rec.partyRecord = rng.coin();
        rec.rawBodySize = rng.coin() ? 44 : rng.below(45);
        for (auto& b : rec.rawBody)
            b = rng.byte();
        const Pokemon::Pokemon1ReadOnly pk(rec);
        const auto report = Legality::analyze(pk, pk.getGameGroup(), rng.coin() ? "red_gb" : "");
        verdicts += report.verdict() == Legality::Verdict::Invalid || report.verdict() == Legality::Verdict::Incomplete ||
                    report.verdict() == Legality::Verdict::NoProblemsFound;
    }
    for (int i = 0; i < 1500; ++i) {
        Gen2::PokemonRecord rec{};
        rec.species = rng.coin() ? static_cast<uint16_t>(rng.below(256)) : static_cast<uint16_t>(rng.next());
        rec.heldItem = rng.byte();
        for (auto& m : rec.moves)
            m = rng.byte();
        rec.trainerId = static_cast<uint16_t>(rng.next());
        rec.experience = static_cast<uint32_t>(rng.next());
        for (auto& se : rec.statExperience)
            se = static_cast<uint16_t>(rng.next());
        for (auto& dv : rec.dvs)
            dv = rng.byte();
        for (auto& pp : rec.pp)
            pp = rng.byte();
        for (auto& up : rec.ppUps)
            up = rng.byte();
        rec.friendship = rng.byte();
        rec.pokerus = rng.byte();
        rec.caughtData = static_cast<uint16_t>(rng.next());
        rec.level = rng.byte();
        rec.originalTrainer = randomText(rng);
        rec.nickname = randomText(rng);
        rec.partyRecord = rng.coin();
        rec.isEgg = rng.below(4) == 0;
        rec.gender = rng.byte();
        rec.form = rng.byte();
        rec.rawBodySize = rng.coin() ? 48 : rng.below(49);
        for (auto& b : rec.rawBody)
            b = rng.byte();
        const Pokemon::Pokemon2ReadOnly pk(rec);
        const auto report = Legality::analyze(pk, pk.getGameGroup(), rng.coin() ? "gold_gbc" : "");
        verdicts += report.verdict() == Legality::Verdict::Invalid || report.verdict() == Legality::Verdict::Incomplete ||
                    report.verdict() == Legality::Verdict::NoProblemsFound;
    }
    check(verdicts == 3000, "3000 random records through Legality::analyze: a verdict for each (" +
                                std::to_string(verdicts) + ")");
}

} // namespace

int main() {
    Rng rng{0x6C616E652D44ULL};
    engine(rng);
    decoders(rng);
    saveParsers(rng);
    records(rng);
    if (failures != 0) {
        std::fprintf(stderr, "Gen I/II malformed-input hardening: %d of %d checks FAILED\n", failures, checks);
        return EXIT_FAILURE;
    }
    std::printf("Gen I/II malformed-input hardening: %d checks PASS\n", checks);
    return EXIT_SUCCESS;
}
