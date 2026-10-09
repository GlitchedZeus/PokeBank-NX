#include "Encryption/Encryption4.h"
#include "Legality/Gen3PidIvCorrelation.h"
#include "Legality/Gen4PidIvCorrelation.h"
#include "Legality/Gen4WildRngCorrelation.h"
#include "Legality/Gen4LeadReportingEvidence.h"
#include "Legality/Legality.h"
#include "Names/ItemNames.h"
#include "Names/SpeciesNames.h"
#include "Pokemon/Pokemon4ReadOnly.h"
#include "Pokemon/Pokemon4ReadOnlyView.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Trainer {
const char* getSpeciesName(uint16_t speciesId) { return Names::getSpeciesName(speciesId); }
const char* getItemName(uint16_t itemId) { return Names::getItemName(itemId); }
}

namespace {
void wr16(std::vector<std::byte>& data, size_t at, uint16_t value) {
    data[at] = static_cast<std::byte>(value & 0xFFu);
    data[at+1] = static_cast<std::byte>(value >> 8);
}
void wr32(std::vector<std::byte>& data, size_t at, uint32_t value) {
    for (int i = 0; i < 4; ++i)
        data[at+static_cast<size_t>(i)] =
            static_cast<std::byte>((value >> (8*i)) & 0xFFu);
}
constexpr uint32_t iv32FromPidSeed(uint32_t origin) {
    using namespace Legality::Gen3PidIv::Detail;
    const uint16_t low = static_cast<uint16_t>((next3(origin) >> 16) & 0x7FFFu);
    const uint16_t high = static_cast<uint16_t>((next(next3(origin)) >> 16) & 0x7FFFu);
    return static_cast<uint32_t>(low) | (static_cast<uint32_t>(high) << 15);
}
constexpr std::array<uint8_t, 6> ivsFromSeed(uint32_t origin) {
    const uint32_t v=iv32FromPidSeed(origin);
    return {
        static_cast<uint8_t>(v & 31u),
        static_cast<uint8_t>((v >> 5) & 31u),
        static_cast<uint8_t>((v >> 10) & 31u),
        static_cast<uint8_t>((v >> 15) & 31u),
        static_cast<uint8_t>((v >> 20) & 31u),
        static_cast<uint8_t>((v >> 25) & 31u)
    };
}
bool hasInfo(const Legality::Report& report, const std::string& fragment) {
    for(const auto& finding : report.issues)
        if(finding.severity == Legality::Severity::Info &&
           finding.text.find(fragment) != std::string::npos)
            return true;
    return false;
}

Legality::Report analyzeNativeHgss(
        uint32_t pid, uint32_t originSeed, uint8_t level,
        uint16_t location = 207, uint8_t originGame = 7,
        uint8_t hgssBall = 24) {
    std::vector<std::byte> plain(Encryption::SIZE_STORED4,std::byte{0});
    wr32(plain, 0x00, pid);
    wr16(plain, 0x08, 14); // Kakuna, real encounter_hg.pkl BCC slot3
    wr16(plain, 0x0C, 12345);
    wr16(plain, 0x0E, 54321);
    wr32(plain, 0x10, 10000); // EXP is unrelated to the PID/IV source proof
    plain[0x17] = std::byte{2}; // English
    wr32(plain, 0x38, iv32FromPidSeed(originSeed));
    plain[0x5F] = static_cast<std::byte>(originGame);
    wr16(plain, 0x46, location);
    plain[0x83] = std::byte{4};  // D/P/Pt Poké Ball shadow
    plain[0x84] = static_cast<std::byte>(level);
    plain[0x86] = static_cast<std::byte>(hgssBall);
    auto encrypted = Encryption::encryptArray4(plain);
    Pokemon::Pokemon4ReadOnly source(encrypted, Enums::GameVersion::HGSS);
    assert(source.valid() && source.checksumValid() && !source.empty());
    assert(source.pid() == pid && source.metLevel() == level);
    assert(source.metLocationExtended() == location);
    assert(source.ballHGSS() == hgssBall);
    assert(source.ivs() == ivsFromSeed(originSeed));
    const auto correlation=Legality::Gen4PidIv::analyze(source.pid(),source.ivs());
    assert(correlation.matched());
    assert(correlation.originSeed == originSeed);
    Pokemon::Pokemon4ReadOnlyView view(source);
    assert(view.ball() == hgssBall);
    assert(view.metLocation() == location);
    return Legality::analyze(view, Enums::GameVersion::HGSS, "heartgold_nds");
}
} // namespace

int main() {
    using namespace Legality;
    constexpr uint64_t kRealRow=0x64D80412139E0EULL;
    static_assert(Gen4Wild::game(kRealRow) == Gen4Wild::Game::HeartGold);
    static_assert(Gen4Wild::species(kRealRow) == 14);
    static_assert(Gen4Wild::location(kRealRow) == 207);
    static_assert(Gen4Wild::slot(kRealRow) == 3);
    static_assert(Gen4Wild::rate(kRealRow) == 25);

    // Two rejected minimum-31 attempts: F-S-F Sync proc history.
    constexpr uint32_t seed2=0x000E11BEu, pid2=0x9C2C4659u;
    constexpr uint32_t seed3=0x0014FEAAu, pid3=0x8EE19004u;
    static_assert(Gen4PidIv::analyze(pid2, ivsFromSeed(seed2)).matched());
    static_assert(Gen4PidIv::analyze(pid3, ivsFromSeed(seed3)).matched());
    static_assert(Gen4PidIv::analyze(pid2, ivsFromSeed(seed2)).originSeed == seed2);
    static_assert(Gen4PidIv::analyze(pid3, ivsFromSeed(seed3)).originSeed == seed3);
    static_assert(!Gen4WildRng::directMinimum31Satisfied(seed3));
    const auto depth2=analyzeNativeHgss(pid2,seed2,16);
    assert(hasInfo(depth2, "Synchronize mixed two/three rerolls (BCC)"));
    assert(hasInfo(depth2, "other unproven lead histories remain incomplete"));
    assert(hasInfo(depth2,
        "Sport Ball has a compatible HeartGold/SoulSilver Bug-Catching Contest source"));

    // Three rejected minimum-31 attempts; retained fourth PK4 need not have
    // a 31 IV when all previous attempts lacked one. No hard-invalid here.
    const auto depth3=analyzeNativeHgss(pid3,seed3,10);
    assert(hasInfo(depth3, "Synchronize mixed two/three rerolls (BCC)"));
    assert(hasInfo(depth3, "other unproven lead histories remain incomplete"));

    // Native source and met data must be exact; missing positive evidence
    // is unresolved, not evidence of impossible provenance.
    assert(!hasInfo(analyzeNativeHgss(pid2,seed2,1),
        "Synchronize mixed two/three rerolls (BCC)"));
    assert(!hasInfo(analyzeNativeHgss(pid2,seed2,16,206),
        "Synchronize mixed two/three rerolls (BCC)"));
    assert(!hasInfo(analyzeNativeHgss(pid2,seed2,16,207,12),
        "Synchronize mixed two/three rerolls (BCC)"));
    assert(!hasInfo(analyzeNativeHgss(pid2,seed2,16,207,7,4),
        "Sport Ball has a compatible HeartGold/SoulSilver"));

    return 0;
}
