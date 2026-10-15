// Gen VI personal data, level/EXP and party stat recomputation.
// Differential against PKSM-Core aa22d7a4: personals/personal6 (17-byte rows,
// source/personal/personal.cpp offsets) and the PK6::statImpl formula.
#include "Integration/Gen6/Gen6Stats.h"
#include "Integration/Gen6/Gen6StagedSession.h"

#include "fixtures/gen6/Gen6SyntheticSave.h"
#include "fixtures/gen6/Gen6TestSupport.h"

#include <fstream>
#include <iterator>
#include <random>

#ifndef GEN6_PKSM_PERSONAL6
#define GEN6_PKSM_PERSONAL6 "vendor/PKSM-Core/personals/personal6"
#endif

namespace Gen6 = PokeVault::Integration::Gen6;
namespace Stats = Gen6::Stats;
namespace PK6 = Gen6::PK6;
namespace Layout = Gen6::Layout;
using Gen6::Field;
using Gen6::Gen6Save;
using Gen6::SlotRef;

namespace {

std::vector<uint8_t> pksmPersonal6() {
    std::ifstream in(GEN6_PKSM_PERSONAL6, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

// PKSM-Core PK6::statImpl, written against its own personal6 row.
uint16_t pksmStat(const std::vector<uint8_t>& p6, size_t row, size_t stat, uint8_t iv, uint8_t ev,
                  uint8_t level, uint8_t nature) {
    constexpr size_t Entry = 17;
    const uint8_t base = p6[row * Entry + stat];
    uint16_t calc = stat == 0 ? static_cast<uint16_t>(10 + (2 * base + iv + ev / 4 + 100) * level / 100)
                              : static_cast<uint16_t>(5 + (2 * base + iv + ev / 4) * level / 100);
    uint8_t mult = 10;
    if (static_cast<size_t>(nature / 5 + 1) == stat) ++mult;
    if (static_cast<size_t>(nature % 5 + 1) == stat) --mult;
    return static_cast<uint16_t>(calc * mult / 10);
}

void testPersonalMatchesPksm() {
    const auto p6 = pksmPersonal6();
    GEN6_CHECK(p6.size() == Gen6::PersonalAO.size() * 17);
    if (p6.size() != Gen6::PersonalAO.size() * 17) return;
    for (size_t i = 0; i < Gen6::PersonalAO.size(); ++i) {
        const auto& r = Gen6::PersonalAO[i];
        const uint8_t* q = p6.data() + i * 17;
        GEN6_CHECK(r.hp == q[0] && r.atk == q[1] && r.def == q[2] && r.spe == q[3] && r.spa == q[4] && r.spd == q[5]);
        GEN6_CHECK(r.genderRatio == q[8] && r.growthRate == q[0xA]);
        GEN6_CHECK(r.formStatsIndex == static_cast<uint16_t>(q[0xE] | (q[0xF] << 8)));
        GEN6_CHECK(r.formCount == q[0x10]);
    }
}

void testFormResolution() {
    GEN6_CHECK(!Stats::personal(0, 0) && !Stats::personal(722, 0));
    GEN6_CHECK(Stats::personal(445, 0)->atk == 130);                  // Garchomp
    // Mega Charizard X (form 1) uses its own row; an out-of-range form is refused.
    const auto megaX = Stats::personal(6, 1);
    GEN6_CHECK(megaX && megaX->atk == 130 && megaX->def == 111);
    GEN6_CHECK(!Stats::personal(6, 3));
    // Cosmetic-only forms (no FormStatsIndex) use the species row.
    const auto vivillon = Stats::personal(666, 5);
    GEN6_CHECK(vivillon && vivillon->hp == 80);
    GEN6_CHECK(Stats::personal(292, 0)->hp == 1);                     // Shedinja
}

void testKnownValues() {
    // Garchomp Lv100, 31 IV / 252 EV Atk, Adamant (3): Atk 394; HP 31/0 = 357.
    Gen6Fixture::MonSpec spec;
    spec.species = 445;
    auto plain = Gen6Fixture::plainMon(spec, true);
    PK6::write32(plain, 0x10, 1250000);                  // Lv100, Slow growth
    plain[0x1C] = 3;
    for (size_t i = 0; i < 6; ++i) plain[0x1E + i] = 0;
    plain[0x1F] = 252;
    PK6::write32(plain, 0x74, 0x3FFFFFFFu);              // all IVs 31
    PK6::write16(plain, 0x06, PK6::checksum(plain));
    auto mon = *PK6::Pokemon6::decode(PK6::encrypt(plain));
    GEN6_CHECK(Stats::level(mon) == 100);
    const auto s = Stats::compute(mon);
    GEN6_CHECK(s && (*s)[0] == 357 && (*s)[1] == 394);
    GEN6_CHECK(s && (*s)[4] == 176);                     // SpA 80 base, -10% => 196*0.9
    GEN6_CHECK(Stats::refreshParty(mon));
    GEN6_CHECK(mon.statLevel() == 100 && mon.stat(0) == 357 && mon.stat(1) == 357 && mon.stat(2) == 394);
    // Shedinja always has 1 HP (PKHeX LoadStats).
    spec.species = 292;
    auto shed = *PK6::Pokemon6::decode(Gen6Fixture::encryptedMon(spec, true));
    GEN6_CHECK(Stats::compute(shed) && (*Stats::compute(shed))[0] == 1);
}

void testFormulaMatchesPksm() {
    const auto p6 = pksmPersonal6();
    if (p6.empty()) { GEN6_CHECK(false); return; }
    std::mt19937 rng(606);
    for (int n = 0; n < 4000; ++n) {
        Gen6Fixture::MonSpec spec;
        spec.species = static_cast<uint16_t>(1 + rng() % 721);
        if (spec.species == 292) continue;               // PKSM lacks the Shedinja HP rule
        auto plain = Gen6Fixture::plainMon(spec, false);
        const auto p = Stats::personal(spec.species, 0);
        const uint8_t lvl = static_cast<uint8_t>(1 + rng() % 100);
        PK6::write32(plain, 0x10, Pokemon::getExpForLevel(lvl, p->growthRate));
        plain[0x1C] = static_cast<uint8_t>(rng() % 25);
        uint32_t iv = 0;
        for (size_t i = 0; i < 6; ++i) iv |= (rng() % 32) << (5 * i);
        PK6::write32(plain, 0x74, iv);
        for (size_t i = 0; i < 6; ++i) plain[0x1E + i] = static_cast<uint8_t>(rng() % 253);
        PK6::write16(plain, 0x06, PK6::checksum(plain));
        const auto mon = *PK6::Pokemon6::decode(PK6::encrypt(plain));
        GEN6_CHECK(Stats::level(mon) == lvl);
        const auto s = Stats::compute(mon);
        for (size_t i = 0; i < 6; ++i)
            GEN6_CHECK(s && (*s)[i] == pksmStat(p6, spec.species, i, mon.iv(i), mon.ev(i), lvl, mon.nature()));
    }
}

void testPartyRecomputeSession() {
    const auto original = Gen6Fixture::makeSave();
    auto save = Gen6Save::parse(original);
    Gen6::Gen6StagedSession s(std::move(*save));
    std::string error;
    const auto p = SlotRef::party(0);
    GEN6_CHECK(s.stageValue(p, Field::IV, 0, 1, &error));
    GEN6_CHECK(s.stageValue(p, Field::EV, 200, 1, &error));
    GEN6_CHECK(s.stageValue(p, Field::Nature, 0, 0, &error));
    GEN6_CHECK(s.stageValue(p, Field::Level, 50, 0, &error));
    GEN6_CHECK(!s.stageValue(p, Field::Level, 0, 0, &error));
    GEN6_CHECK(!s.stageValue(p, Field::Level, 101, 0, &error));
    const auto cur = s.current(p);
    const auto expect = Stats::compute(*cur);
    GEN6_CHECK(cur && expect && cur->statLevel() == 50 && Stats::level(*cur) == 50);
    for (size_t i = 0; i < 6; ++i) GEN6_CHECK(cur->stat(i + 1) == (*expect)[i]);
    GEN6_CHECK(cur->stat(0) == (*expect)[0]);  // current HP = max HP
    const auto edited = s.buildWorkingCopy(&error);
    GEN6_CHECK(edited.has_value());
    if (!edited) { std::fprintf(stderr, "  %s\n", error.c_str()); return; }
    const auto re = Gen6Save::parse(*edited);
    GEN6_CHECK(re && re->party(0).pokemon->statLevel() == 50);
    GEN6_CHECK(re && re->party(0).pokemon->experience() == Pokemon::getExpForLevel(50, Stats::personal(1, 0)->growthRate));

    // Tampered cached stats (correct checksums) are refused by the verifier.
    auto forged = *edited;
    auto rec = *re->party(0).pokemon;
    std::vector<uint8_t> plain(rec.plain().begin(), rec.plain().end());
    PK6::write16(plain, 0xF4, static_cast<uint16_t>(PK6::read16(plain, 0xF4) + 1));
    Gen6Fixture::put(forged, Gen6Save::partyRecordOffset(0), PK6::encrypt(plain));
    Gen6Fixture::resignAll(forged, Layout::Family::XY);
    auto changes = s.changes();
    changes[0].stagedPlain = plain;
    GEN6_CHECK(!Gen6::verifyWorkingCopy(s.source(), forged, changes, &error));

    // Reverting every stat input restores the original cached block and drops the change.
    auto r = Gen6::Gen6StagedSession(*Gen6Save::parse(original));
    const auto before = *r.current(p);
    GEN6_CHECK(r.stageValue(p, Field::IV, before.iv(2) == 0 ? 1 : 0, 2, &error) && r.hasChanges());
    GEN6_CHECK(r.stageValue(p, Field::IV, before.iv(2), 2, &error) && !r.hasChanges());

    // Box Level edit sets EXP to the growth-rate minimum and never adds party bytes.
    auto b = Gen6::Gen6StagedSession(*Gen6Save::parse(original));
    GEN6_CHECK(b.stageValue(SlotRef::boxed(0, 0), Field::Level, 37, 0, &error));
    GEN6_CHECK(Stats::level(*b.current(SlotRef::boxed(0, 0))) == 37);
    GEN6_CHECK(b.buildWorkingCopy(&error).has_value());
}

} // namespace

int main() {
    testPersonalMatchesPksm();
    testFormResolution();
    testKnownValues();
    testFormulaMatchesPksm();
    testPartyRecomputeSession();
    return Gen6Test::finish("test_gen6_stats");
}
