// Gen VII personal data, hyper-training aware stats and party recomputation.
// Differential against PKSM-Core aa22d7a4 personals/personal7 and PK7::statImpl
// (hyperTrainLookup {0,1,2,5,3,4}).
#include "Integration/Gen7/Gen7StagedSession.h"
#include "Integration/Gen7/Gen7Stats.h"

#include "fixtures/gen6/Gen6TestSupport.h"
#include "fixtures/gen7/Gen7SyntheticSave.h"

#include <fstream>
#include <iterator>
#include <random>

#ifndef GEN7_PKSM_PERSONAL7
#define GEN7_PKSM_PERSONAL7 "vendor/PKSM-Core/personals/personal7"
#endif

namespace Gen7 = PokeVault::Integration::Gen7;
namespace Stats = Gen7::Stats;
namespace PK7 = Gen7::PK7;
namespace Layout = Gen7::Layout;
using Gen7::Field;
using Gen7::SlotRef;

extern "C" const char* __lsan_default_suppressions() {
    return "leak:rsa_encrypt\nleak:rsa_decrypt\n";
}

namespace {

std::vector<uint8_t> personal7() {
    std::ifstream in(GEN7_PKSM_PERSONAL7, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

constexpr uint8_t PksmHyper[6] = {0, 1, 2, 5, 3, 4};
uint16_t pksmStat(const std::vector<uint8_t>& p7, size_t row, size_t stat, const PK7::Pokemon6& mon, uint8_t level) {
    const uint8_t base = p7[row * 17 + stat];
    const uint32_t iv = ((mon.plain()[0xDE] >> PksmHyper[stat]) & 1) ? 31u : mon.iv(stat);
    uint32_t calc = stat == 0 ? 10 + (2 * base + iv + mon.ev(stat) / 4 + 100) * level / 100
                              : 5 + (2 * base + iv + mon.ev(stat) / 4) * level / 100;
    uint32_t mult = 10;
    if (static_cast<size_t>(mon.nature() / 5 + 1) == stat) ++mult;
    if (static_cast<size_t>(mon.nature() % 5 + 1) == stat) --mult;
    return static_cast<uint16_t>(calc * mult / 10);
}

void testPersonal() {
    const auto p7 = personal7();
    GEN6_CHECK(p7.size() == Gen7::PersonalUSUM.size() * 17);
    if (p7.size() != Gen7::PersonalUSUM.size() * 17) return;
    for (size_t i = 0; i < Gen7::PersonalUSUM.size(); ++i) {
        const auto& r = Gen7::PersonalUSUM[i];
        const uint8_t* q = p7.data() + i * 17;
        GEN6_CHECK(r.hp == q[0] && r.atk == q[1] && r.def == q[2] && r.spe == q[3] && r.spa == q[4] && r.spd == q[5]);
        GEN6_CHECK(r.genderRatio == q[8] && r.growthRate == q[0xA] && r.formCount == q[0x10]);
        GEN6_CHECK(r.formStatsIndex == static_cast<uint16_t>(q[0xE] | (q[0xF] << 8)));
    }
    GEN6_CHECK(Stats::personal(807, 0) && !Stats::personal(808, 0));
    GEN6_CHECK(Stats::personal(292, 0)->hp == 1);
}

PK7::Pokemon6 makeMon(uint16_t species, uint8_t level, uint8_t nature, uint32_t iv, uint8_t ht,
                      const std::array<uint8_t, 6>& evs) {
    Gen6Fixture::MonSpec spec;
    spec.species = species;
    auto plain = Gen6Fixture::plainMon(spec, true);
    PK7::write32(plain, 0x10, Pokemon::getExpForLevel(level, Stats::personal(species, 0)->growthRate));
    plain[0x1C] = nature;
    PK7::write32(plain, 0x74, iv);
    for (size_t i = 0; i < 6; ++i) plain[0x1E + i] = evs[i];
    plain[0xDE] = ht;
    PK7::write16(plain, 0x06, PK7::checksum(plain));
    return *PK7::Pokemon6::decode(PK7::encrypt(plain));
}

void testFormula() {
    // Known value: Lv100 Garchomp, 0 Atk IV but hyper-trained Atk, 252 Atk EV, Adamant -> 394.
    const auto g = makeMon(445, 100, 3, 0, 0x02, {0, 252, 0, 0, 0, 0});
    const auto s = Stats::compute(g);
    GEN6_CHECK(s && (*s)[1] == 394);
    // Hyper-trained Speed uses bit 5 (not bit 3).
    const auto spe = makeMon(445, 100, 0, 0, 0x20, {0, 0, 0, 0, 0, 0});
    GEN6_CHECK(Stats::hyperTrained(spe, 3) && !Stats::hyperTrained(spe, 4));
    const auto p7 = personal7();
    if (p7.empty()) { GEN6_CHECK(false); return; }
    std::mt19937 rng(707);
    for (int n = 0; n < 4000; ++n) {
        const uint16_t species = static_cast<uint16_t>(1 + rng() % 807);
        if (species == 292) continue;
        const uint8_t lvl = static_cast<uint8_t>(1 + rng() % 100);
        uint32_t iv = 0;
        for (size_t i = 0; i < 6; ++i) iv |= (rng() % 32) << (5 * i);
        std::array<uint8_t, 6> evs{};
        for (auto& e : evs) e = static_cast<uint8_t>(rng() % 253);
        const auto mon = makeMon(species, lvl, static_cast<uint8_t>(rng() % 25), iv, static_cast<uint8_t>(rng() & 0x3F), evs);
        GEN6_CHECK(Stats::level(mon) == lvl);
        const auto out = Stats::compute(mon);
        for (size_t i = 0; i < 6; ++i) GEN6_CHECK(out && (*out)[i] == pksmStat(p7, species, i, mon, lvl));
    }
}

void testSession() {
    const auto bytes = Gen7Fixture::makeSave({Layout::Family::UltraSunUltraMoon, Layout::VersionUltraMoon, 2, true, true});
    std::string error;
    Gen7::Gen7StagedSession s(*Gen7::Gen7Save::parse(bytes, {}, &error));
    const auto p = SlotRef::party(0);
    GEN6_CHECK(s.stageValue(p, Field::Level, 40, 0, &error));
    GEN6_CHECK(s.stageValue(p, Field::EV, 100, 2, &error));
    GEN6_CHECK(s.stageValue(p, Field::Nature, 7, 0, &error));
    const auto cur = s.current(p);
    const auto expect = Stats::compute(*cur);
    GEN6_CHECK(cur && expect && cur->statLevel() == 40 && cur->stat(0) == (*expect)[0]);
    for (size_t i = 0; i < 6; ++i) GEN6_CHECK(cur->stat(i + 1) == (*expect)[i]);
    const auto candidate = s.buildUnsignedCandidate(&error);
    GEN6_CHECK(candidate.has_value());
    if (!candidate) { std::fprintf(stderr, "  %s\n", error.c_str()); return; }
    // Forged cached stat (re-CRC'd) is refused.
    auto changes = s.changes();
    PK7::write16(changes[0].stagedPlain, 0xF4, static_cast<uint16_t>(PK7::read16(changes[0].stagedPlain, 0xF4) + 1));
    auto forged = candidate->bytes;
    Gen6Fixture::put(forged, Gen7::Gen7Save::partyRecordOffset(Layout::Family::UltraSunUltraMoon, 0),
                     PK7::encrypt(changes[0].stagedPlain));
    Gen7Fixture::resignBlocks(forged, Layout::Family::UltraSunUltraMoon);
    GEN6_CHECK(!Gen7::verifyUnsignedCandidate(s.source(), forged, changes, &error));
    GEN6_CHECK(!s.stageValue(p, Field::Level, 101, 0, &error));
}

} // namespace

int main() {
    testPersonal();
    testFormula();
    testSession();
    return Gen6Test::finish("test_gen7_stats");
}
