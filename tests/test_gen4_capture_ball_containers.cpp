// A PK4's capture Ball belongs to the Pokemon, not to the save that holds it.
//
// PK4 keeps two Ball bytes. 0x83 is the Ball D/P/Pt can display; 0x86 is written only
// by HeartGold/SoulSilver (catches, hatches and Pal Park) and holds the real Ball, with
// 0x83 set to a Poke Ball stand-in for the Apricorn and Sport Balls D/P/Pt cannot show.
// Every other origin, event gifts included, leaves 0x86 at zero. The real Ball is
// therefore max(0x83, 0x86) wherever the Pokemon sits (PKHeX G4PKM.Ball at
// kwsch/PKHeX@6501f0ab; Pokemon4Mutable::ball()).
//
// This checks every origin family against every container: the record alone, party and
// box records in Diamond/Pearl, Platinum and HeartGold/SoulSilver group contexts, real
// checksum-valid saves of all three layouts parsed by Gen4ReadOnlySave, the mutable
// editor model, and the legality analyzer's Ball evidence.
#include "Encryption/Encryption4.h"
#include "Enums/LanguageID.h"
#include "Integration/Gen4/Gen4ReadOnlySave.h"
#include "Legality/Gen3PidIvCorrelation.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon4Mutable.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"
#include "Utils/Gen4TextCodec.h"
#include "utils/crypto.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <vector>

#ifdef NDEBUG
#error "Gen IV capture Ball regression requires assertions"
#endif

namespace Trainer {
const char* getSpeciesName(uint16_t speciesId) { return Names::getSpeciesName(speciesId); }
const char* getItemName(uint16_t itemId) { return Names::getItemName(itemId); }
}

using PokeVault::Integration::Gen4::Gen4ReadOnlySave;
using PokeVault::Integration::Gen4::Layout;

namespace {

struct Origin {
    const char* name;
    uint8_t version;  // PK4 0x5F
    uint8_t ballDPPt; // PK4 0x83
    uint8_t ballHGSS; // PK4 0x86
    uint8_t ball;     // the Pokemon's real capture Ball
};

// Ball ids: 1 Master, 2 Ultra, 4 Poke, 5 Safari, 12 Premier, 13 Dusk, 15 Quick, 16 Cherish,
// 17 Fast, 18 Level, 19 Lure, 20 Heavy, 21 Love, 22 Friend, 23 Moon, 24 Sport.
constexpr std::array<Origin, 17> kOrigins{{
    {"Diamond, Poke Ball", 10, 4, 0, 4},
    {"Pearl, Dusk Ball", 11, 13, 0, 13},
    {"Platinum, Quick Ball", 12, 15, 0, 15},
    {"Platinum event gift, Cherish Ball", 12, 16, 0, 16},
    {"HeartGold, Ultra Ball", 7, 2, 2, 2},
    {"SoulSilver, Safari Ball", 8, 5, 5, 5},
    {"HeartGold, Fast Ball", 7, 4, 17, 17},
    {"SoulSilver, Level Ball", 8, 4, 18, 18},
    {"HeartGold, Lure Ball", 7, 4, 19, 19},
    {"SoulSilver, Heavy Ball", 8, 4, 20, 20},
    {"HeartGold, Love Ball", 7, 4, 21, 21},
    {"SoulSilver, Friend Ball", 8, 4, 22, 22},
    {"HeartGold, Moon Ball", 7, 4, 23, 23},
    {"HeartGold Bug-Catching Contest, Sport Ball", 7, 4, 24, 24},
    {"HeartGold event gift, Cherish Ball", 7, 16, 0, 16},
    {"Emerald via Pal Park in Diamond/Pearl/Platinum, Master Ball", 3, 1, 0, 1},
    {"FireRed via Pal Park in HeartGold/SoulSilver, Premier Ball", 4, 12, 12, 12},
}};

constexpr std::array<Enums::GameVersion, 4> kGroups{
    Enums::GameVersion::DP, Enums::GameVersion::PT, Enums::GameVersion::HGSS,
    Enums::GameVersion::Invalid};

void wr16(std::vector<std::byte>& d, std::size_t at, uint16_t v) {
    d[at] = static_cast<std::byte>(v & 0xFFu);
    d[at + 1] = static_cast<std::byte>(v >> 8);
}
void wr32(std::vector<std::byte>& d, std::size_t at, uint32_t v) {
    for (std::size_t i = 0; i < 4; ++i)
        d[at + i] = static_cast<std::byte>((v >> (8 * i)) & 0xFFu);
}
void put(std::vector<std::byte>& d, std::size_t at, std::span<const std::byte> src) {
    for (std::size_t i = 0; i < src.size(); ++i)
        d[at + i] = src[i];
}

/// An encrypted PK4 (box or party size) with the given origin's Ball bytes.
std::vector<std::byte> makeRecord(const Origin& o, bool party) {
    std::vector<std::byte> d(party ? Encryption::SIZE_PARTY4 : Encryption::SIZE_STORED4, std::byte{0});
    wr32(d, 0x00, 0x2468ACE1u);
    wr16(d, 0x08, 25); // Pikachu
    wr16(d, 0x0C, 12345);
    wr16(d, 0x0E, 54321);
    wr32(d, 0x10, 10000);
    d[0x15] = std::byte{9}; // Static
    d[0x17] = std::byte{2}; // English
    put(d, 0x48, Utils::encodeGen4Field(u"PIKA", 11, 10, 2));
    d[0x5F] = static_cast<std::byte>(o.version);
    put(d, 0x68, Utils::encodeGen4Field(u"ASH", 8, 7, 2));
    d[0x83] = static_cast<std::byte>(o.ballDPPt);
    d[0x84] = std::byte{20};
    d[0x86] = static_cast<std::byte>(o.ballHGSS);
    if (party)
        d[0x8C] = std::byte{25}; // party level
    return Encryption::encryptArray4(d);
}

void checkRecord(const Origin& o, std::span<const std::byte> encrypted, Enums::GameVersion group) {
    const std::vector<std::byte> before(encrypted.begin(), encrypted.end());
    Pokemon::Pokemon4ReadOnly pk(encrypted, group);
    assert(pk.valid() && !pk.empty() && pk.species() == 25);
    assert(pk.ballDPPt() == o.ballDPPt && pk.ballHGSS() == o.ballHGSS);
    const Pokemon::Pokemon4ReadOnlyView view(pk);
    if (view.ball() != o.ball) {
        std::cerr << "FAIL " << o.name << " in group " << static_cast<int>(group) << ": ball "
                  << static_cast<int>(view.ball()) << ", expected " << static_cast<int>(o.ball) << "\n";
        assert(false);
    }
    // The two stored bytes stay individually readable.
    assert(view.gen4BallDPPt() == o.ballDPPt && view.gen4BallHGSS() == o.ballHGSS);
    const auto mutableModel = Pokemon::Pokemon4Mutable::fromEncrypted(encrypted, group);
    assert(mutableModel && mutableModel->valid() && mutableModel->ball() == o.ball);
    assert(std::equal(before.begin(), before.end(), encrypted.begin()));
}

// ---- checksum-valid Gen IV saves (block layout per PKHeX SAV4DP/SAV4Pt/SAV4HGSS) ----

constexpr std::size_t kSaveSize = 0x80000, kPartition = 0x40000;
constexpr uint32_t kMagic = 0x20060623;

struct Spec {
    std::size_t generalSize, storageStart, storageSize, footerSize;
    std::size_t trainer, party, boxData, boxStride, currentBox, boxNames;
};
Spec spec(Layout layout) {
    if (layout == Layout::DiamondPearl)
        return {0xC100, 0xC100, 0x121E0, 0x14, 0x64, 0x98, 4, 0xFF0, 0, 4 + 18 * 0xFF0};
    if (layout == Layout::Platinum)
        return {0xCF2C, 0xCF2C, 0x121E4, 0x14, 0x68, 0xA0, 4, 0xFF0, 0, 4 + 18 * 0xFF0};
    return {0xF628, 0xF700, 0x12310, 0x10, 0x64, 0x98, 0, 0x1000, 0x12000, 0x12008};
}

void w16(std::vector<uint8_t>& b, std::size_t o, uint16_t v) {
    b[o] = static_cast<uint8_t>(v);
    b[o + 1] = static_cast<uint8_t>(v >> 8);
}
void w32(std::vector<uint8_t>& b, std::size_t o, uint32_t v) {
    for (std::size_t i = 0; i < 4; ++i)
        b[o + i] = static_cast<uint8_t>(v >> (8 * i));
}
void copyTo(std::vector<uint8_t>& dst, std::size_t off, std::span<const std::byte> src) {
    for (std::size_t i = 0; i < src.size(); ++i)
        dst[off + i] = static_cast<uint8_t>(src[i]);
}
void stamp(std::vector<uint8_t>& save, std::size_t off, std::size_t len, std::size_t footer, uint32_t major) {
    const std::size_t end = off + len;
    w32(save, end - 0x14, major);
    w32(save, end - 0x10, 1);
    w32(save, end - 0x0C, static_cast<uint32_t>(len));
    w32(save, end - 0x08, kMagic);
    // Pinned PKSM-Core CRC, independent of the parser under test.
    w16(save, end - 2, pksm::crypto::ccitt16(std::span<const uint8_t>(save).subspan(off, len - footer)));
}

/// A save whose party slot 1 and box 1 slot 1 both hold the origin's Pokemon.
std::vector<uint8_t> makeSave(Layout layout, const Origin& o) {
    const Spec s = spec(layout);
    std::vector<uint8_t> save(kSaveSize, 0xFF);
    const auto blank = Encryption::blankRecord4(Encryption::SIZE_STORED4);
    const auto party = makeRecord(o, true);
    const auto stored = makeRecord(o, false);
    for (std::size_t p = 0; p < 2; ++p) {
        const std::size_t base = p * kPartition, st = base + s.storageStart;
        std::fill(save.begin() + static_cast<std::ptrdiff_t>(base),
                  save.begin() + static_cast<std::ptrdiff_t>(base + s.generalSize), 0);
        std::fill(save.begin() + static_cast<std::ptrdiff_t>(st),
                  save.begin() + static_cast<std::ptrdiff_t>(st + s.storageSize), 0);
        const std::size_t tr = base + s.trainer;
        copyTo(save, tr, Utils::encodeGen4Field(u"ASH", 8, 7, 2));
        w16(save, tr + 0x10, 12345);
        w16(save, tr + 0x12, 54321);
        save[tr + 0x19] = 2; // English
        save[tr + 0x1C] = layout == Layout::HeartGoldSoulSilver ? 7 : (layout == Layout::Platinum ? 12 : 10);
        save[base + s.party - 4] = 1;
        copyTo(save, base + s.party, party);
        for (std::size_t box = 0; box < 18; ++box)
            for (std::size_t slot = 0; slot < 30; ++slot)
                copyTo(save, st + s.boxData + box * s.boxStride + slot * Encryption::SIZE_STORED4, blank);
        copyTo(save, st + s.boxData, stored);
        for (std::size_t box = 0; box < 18; ++box)
            copyTo(save, st + s.boxNames + box * 40, Utils::encodeGen4Field(u"BOX", 20, 8, 2));
        stamp(save, base, s.generalSize, s.footerSize, p == 0 ? 20 : 10);
        stamp(save, st, s.storageSize, s.footerSize, p == 0 ? 30 : 15);
    }
    return save;
}

// ---- the analyzer reads the same Ball in every container ----

uint32_t iv32FromPidSeed(uint32_t origin) {
    using namespace Legality::Gen3PidIv::Detail;
    const uint16_t low = static_cast<uint16_t>((next3(origin) >> 16) & 0x7FFFu);
    const uint16_t high = static_cast<uint16_t>((next(next3(origin)) >> 16) & 0x7FFFu);
    return static_cast<uint32_t>(low) | (static_cast<uint32_t>(high) << 15);
}

bool hasInfo(const Legality::Report& report, const std::string& fragment) {
    for (const auto& issue : report.issues)
        if (issue.severity == Legality::Severity::Info && issue.text.find(fragment) != std::string::npos)
            return true;
    return false;
}

std::vector<std::string> issueLines(const Legality::Report& report) {
    std::vector<std::string> out;
    for (const auto& issue : report.issues)
        out.push_back(std::to_string(static_cast<int>(issue.severity)) + " " + issue.text);
    return out;
}

int checkAnalyzer() {
    // A HeartGold Bug-Catching Contest Kakuna in a Sport Ball (the real encounter_hg.pkl
    // slot and Method K spread used by test_gen4_bcc_multiretry_native_pk4).
    std::vector<std::byte> plain(Encryption::SIZE_STORED4, std::byte{0});
    wr32(plain, 0x00, 0x9C2C4659u);
    wr16(plain, 0x08, 14);
    wr16(plain, 0x0C, 12345);
    wr16(plain, 0x0E, 54321);
    wr32(plain, 0x10, 10000);
    plain[0x17] = std::byte{2};
    wr32(plain, 0x38, iv32FromPidSeed(0x000E11BEu));
    plain[0x5F] = std::byte{7};
    wr16(plain, 0x46, 207);
    plain[0x83] = std::byte{4};
    plain[0x84] = std::byte{16};
    plain[0x86] = std::byte{24};
    const auto encrypted = Encryption::encryptArray4(plain);

    struct Container {
        Enums::GameVersion group;
        const char* gameId;
    };
    const std::array<Container, 3> containers{{{Enums::GameVersion::DP, "diamond_nds"},
                                               {Enums::GameVersion::PT, "platinum_nds"},
                                               {Enums::GameVersion::HGSS, "heartgold_nds"}}};
    std::vector<std::string> reference;
    int checks = 0;
    for (const auto& c : containers) {
        Pokemon::Pokemon4ReadOnly pk(encrypted, c.group);
        assert(pk.valid());
        const Pokemon::Pokemon4ReadOnlyView view(pk);
        assert(view.ball() == 24);
        const auto report = Legality::analyze(view, c.group, c.gameId);
        assert(hasInfo(report, "Sport Ball has a compatible HeartGold/SoulSilver Bug-Catching Contest source"));
        const auto lines = issueLines(report);
        if (reference.empty())
            reference = lines;
        // Same Pokemon, same Ball, same findings, whichever save it was traded into.
        assert(lines == reference);
        checks += 3;
    }

    // A Diamond Poke Ball Pokemon in a HeartGold save is still in a Poke Ball (0x86 is 0).
    Pokemon::Pokemon4ReadOnly diamond(makeRecord(kOrigins[0], false), Enums::GameVersion::HGSS);
    const Pokemon::Pokemon4ReadOnlyView diamondView(diamond);
    assert(diamondView.ball() == 4);
    const auto report = Legality::analyze(diamondView, Enums::GameVersion::HGSS, "heartgold_nds");
    for (const auto& issue : report.issues)
        assert(issue.text.find("Ball id") == std::string::npos);
    return checks + 2;
}

} // namespace

int main() {
    int checks = 0;
    for (const Origin& o : kOrigins) {
        for (bool party : {false, true}) {
            const auto encrypted = makeRecord(o, party);
            for (const auto group : kGroups) {
                checkRecord(o, encrypted, group);
                ++checks;
            }
        }
        for (const Layout layout : {Layout::DiamondPearl, Layout::Platinum, Layout::HeartGoldSoulSilver}) {
            const auto save = makeSave(layout, o);
            std::string error;
            const auto parsed = Gen4ReadOnlySave::parse(save, layout, {}, &error);
            if (!parsed) {
                std::cerr << "FAIL parse " << o.name << ": " << error << "\n";
                return 1;
            }
            assert(parsed->party().size() == 1);
            for (const Pokemon::Pokemon4ReadOnly* pk : {&parsed->party()[0], &parsed->box(0, 0)}) {
                assert(pk->valid() && pk->species() == 25);
                const Pokemon::Pokemon4ReadOnlyView view(*pk);
                if (view.ball() != o.ball) {
                    std::cerr << "FAIL " << o.name << " in a parsed save (layout "
                              << static_cast<int>(layout) << ", " << (pk->isParty() ? "party" : "box")
                              << "): ball " << static_cast<int>(view.ball()) << "\n";
                    return 1;
                }
                ++checks;
            }
        }
    }
    checks += checkAnalyzer();
    std::cout << "Gen IV capture Ball across " << kOrigins.size()
              << " origins, party/box records, D/P, Pt, HG/SS and unknown containers, parsed saves "
                 "and the analyzer: "
              << checks << " checks PASS\n";
    return 0;
}
